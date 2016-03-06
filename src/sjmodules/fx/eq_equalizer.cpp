/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2016 Björn Petersen Software Design and Development
 *                   Contact: r10s@b44t.com, http://b44t.com
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see http://www.gnu.org/licenses/ .
 *
 *******************************************************************************
 *
 * File:    eq_equalizer.cpp
 * Authors: Björn Petersen
 * Purpose: Call the SuperEQ
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/fx/eq_equalizer.h>
#include <supereq/Fftsg_fl.cpp>
#include <supereq/paramlist.hpp>


/*******************************************************************************
 * Equ.cpp
 ******************************************************************************/


static const REAL bands[] = {
  65.406392,92.498606,130.81278,184.99721,261.62557,369.99442,523.25113,
  739.9884 ,1046.5023,1479.9768,2093.0045,2959.9536,4186.0091,5919.9072,
  8372.0181,11839.814,16744.036
};


class SjSuperEQ
{
public:

#define M 15

#define PI 3.1415926535897932384626433832795

#define RINT(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))

//#define DITHERLEN 65536

// play -c 2 -r 44100 -fs -sw

REAL fact[M+1];
REAL aa;
REAL iza;
REAL *lires,*lires1,*lires2,*rires,*rires1,*rires2,*irest;
REAL *fsamples;
//REAL *ditherbuf;
//int ditherptr;
volatile int chg_ires,cur_ires;
int winlen,winlenbit,tabsize,nbufsamples;
short *inbuf;
REAL *outbuf;
//int enable, dither;

#define NCH 2

#define NBANDS 17

REAL alpha(REAL a)
{
  if (a <= 21) return 0;
  if (a <= 50) return 0.5842*pow(a-21,0.4)+0.07886*(a-21);
  return 0.1102*(a-8.7);
}

REAL izero(REAL x)
{
  REAL ret = 1;
  int m;

  for(m=1;m<=M;m++)
    {
      REAL t;
      t = pow(x/2,m)/fact[m];
      ret += t*t;
    }

  return ret;
}

SjSuperEQ(int wb)
{
  int i,j;

  aa = 96;
  //ditherptr = 0;
  //enable = 1;
  //dither = 0;

  winlen = (1 << (wb-1))-1;
  winlenbit = wb;
  tabsize  = 1 << wb;

  lires1   = (REAL *)malloc(sizeof(REAL)*tabsize);
  lires2   = (REAL *)malloc(sizeof(REAL)*tabsize);
  rires1   = (REAL *)malloc(sizeof(REAL)*tabsize);
  rires2   = (REAL *)malloc(sizeof(REAL)*tabsize);
  irest    = (REAL *)malloc(sizeof(REAL)*tabsize);
  fsamples = (REAL *)malloc(sizeof(REAL)*tabsize);
  inbuf    = (short *)calloc(winlen*NCH,sizeof(int));
  outbuf   = (REAL *)calloc(tabsize*NCH,sizeof(REAL));
  //ditherbuf = (REAL *)malloc(sizeof(REAL)*DITHERLEN);

  lires = lires1;
  rires = rires1;
  cur_ires = 1;
  chg_ires = 1;

  //for(i=0;i<DITHERLEN;i++)
  //	ditherbuf[i] = (float(rand())/RAND_MAX-0.5);

  for(i=0;i<=M;i++)
    {
      fact[i] = 1;
      for(j=1;j<=i;j++) fact[i] *= j;
    }

  iza = izero(alpha(aa));

  last_srate = 0;
  last_nch = 0;
  //last_bps = 0;
  for( i = 0; i < 18; i++ ) { lbands[i]=1.0; rbands[i]=1.0; }
  bands_changed = false;
  //equ_modifySamples_hm1 = 0;
  rfft_ipsize = 0;
  rfft_wsize=0;
  rfft_ip = NULL;
  rfft_w = NULL;
}

int rfft_ipsize, rfft_wsize;
int *rfft_ip;
REAL *rfft_w;

void rfft(int n,int isign,REAL x[])
{
  int newipsize,newwsize;

  if (n == 0) {
    free(rfft_ip); rfft_ip = NULL; rfft_ipsize = 0;
    free(rfft_w);  rfft_w  = NULL; rfft_wsize  = 0;
    return;
  }

  newipsize = 2+sqrt(n/2);
  if (newipsize > rfft_ipsize) {
    rfft_ipsize = newipsize;
    rfft_ip = (int *)realloc(rfft_ip,sizeof(int)*rfft_ipsize);
    rfft_ip[0] = 0;
  }

  newwsize = n/2;
  if (newwsize > rfft_wsize) {
    rfft_wsize = newwsize;
    rfft_w = (REAL *)realloc(rfft_w,sizeof(REAL)*rfft_wsize);
  }

  rdft(n,isign,x,rfft_ip,rfft_w);
}

// -(N-1)/2 <= n <= (N-1)/2
REAL win(REAL n,int N)
{
  return izero(alpha(aa)*sqrt(1-4*n*n/((N-1)*(N-1))))/iza;
}

REAL sinc(REAL x)
{
  return x == 0 ? 1 : sin(x)/x;
}

REAL hn_lpf(int n,REAL f,REAL fs)
{
  REAL t = 1/fs;
  REAL omega = 2*PI*f;
  return 2*f*t*sinc(n*omega*t);
}

REAL hn_imp(int n)
{
  return n == 0 ? 1.0 : 0.0;
}

REAL hn(int n,paramlist &param2,REAL fs)
{
  paramlistelm *e;
  REAL ret,lhn;

  lhn = hn_lpf(n,param2.elm->upper,fs);
  ret = param2.elm->gain*lhn;

  for(e=param2.elm->next;e->next != NULL && e->upper < fs/2;e = e->next)
    {
      REAL lhn2 = hn_lpf(n,e->upper,fs);
      ret += e->gain*(lhn2-lhn);
      lhn = lhn2;
    }

  ret += e->gain*(hn_imp(n)-lhn);

  return ret;
}

void process_param(const REAL *bc,paramlist *param,paramlist &param2,REAL fs,int ch)
{
  paramlistelm **pp,*p,*e,*e2;
  int i;

  delete param2.elm;
  param2.elm = NULL;

  for(i=0,pp=&param2.elm;i<=NBANDS;i++,pp = &(*pp)->next)
  {
    (*pp) = new paramlistelm;
	(*pp)->lower = i == 0        ?  0 : bands[i-1];
	(*pp)->upper = i == NBANDS-1 ? fs : bands[i  ];
	(*pp)->gain  = bc[i];
  }

  for(e = param->elm;e != NULL;e = e->next)
  {
	if ((ch == 0 && !e->left) || (ch == 1 && !e->right)) continue;
	if (e->lower >= e->upper) continue;

	for(p=param2.elm;p != NULL;p = p->next)
		if (p->upper > e->lower) break;

	while(p != NULL && p->lower < e->upper)
	{
		if (e->lower <= p->lower && p->upper <= e->upper) {
			p->gain *= pow(10,e->gain/20);
			p = p->next;
			continue;
		}
		if (p->lower < e->lower && e->upper < p->upper) {
			e2 = new paramlistelm;
			e2->lower = e->upper;
			e2->upper = p->upper;
			e2->gain  = p->gain;
			e2->next  = p->next;
			p->next   = e2;

			e2 = new paramlistelm;
			e2->lower = e->lower;
			e2->upper = e->upper;
			e2->gain  = p->gain * pow(10,e->gain/20);
			e2->next  = p->next;
			p->next   = e2;

			p->upper  = e->lower;

			p = p->next->next->next;
			continue;
		}
		if (p->lower < e->lower) {
			e2 = new paramlistelm;
			e2->lower = e->lower;
			e2->upper = p->upper;
			e2->gain  = p->gain * pow(10,e->gain/20);
			e2->next  = p->next;
			p->next   = e2;

			p->upper  = e->lower;
			p = p->next->next;
			continue;
		}
		if (e->upper < p->upper) {
			e2 = new paramlistelm;
			e2->lower = e->upper;
			e2->upper = p->upper;
			e2->gain  = p->gain;
			e2->next  = p->next;
			p->next   = e2;

			p->upper  = e->upper;
			p->gain   = p->gain * pow(10,e->gain/20);
			p = p->next->next;
			continue;
		}
		abort();
	}
  }
}

void equ_makeTable(const REAL *lbc,const REAL *rbc,paramlist *param,REAL fs)
{
  int i,cires = cur_ires;
  REAL *nires;

  if (fs <= 0) return;

  paramlist param2;

  // L

  process_param(lbc,param,param2,fs,0);

  for(i=0;i<winlen;i++)
    irest[i] = hn(i-winlen/2,param2,fs)*win(i-winlen/2,winlen);

  for(;i<tabsize;i++)
    irest[i] = 0;

  rfft(tabsize,1,irest);

  nires = cires == 1 ? lires2 : lires1;

  for(i=0;i<tabsize;i++)
    nires[i] = irest[i];

  process_param(rbc,param,param2,fs,1);

  // R

  for(i=0;i<winlen;i++)
    irest[i] = hn(i-winlen/2,param2,fs)*win(i-winlen/2,winlen);

  for(;i<tabsize;i++)
    irest[i] = 0;

  rfft(tabsize,1,irest);

  nires = cires == 1 ? rires2 : rires1;

  for(i=0;i<tabsize;i++)
    nires[i] = irest[i];

  //

  chg_ires = cires == 1 ? 2 : 1;
}

~SjSuperEQ()
{
  free(lires1);
  free(lires2);
  free(rires1);
  free(rires2);
  free(irest);
  free(fsamples);
  free(inbuf);
  free(outbuf);

  rfft(0,0,NULL);
}

void equ_clearbuf(/*int bps,int srate*/)
{
	int i;

	nbufsamples = 0;
	for(i=0;i<tabsize*NCH;i++) outbuf[i] = 0;
}

//float equ_modifySamples_hm1;

void equ_modifySamples(char *buf,int nsamples,int nch,int bps)
{
  int i,p,ch;
  REAL *ires;
  int amax =  (1 << (bps-1))-1;
  int amin = -(1 << (bps-1));

  if (chg_ires) {
	  cur_ires = chg_ires;
	  lires = cur_ires == 1 ? lires1 : lires2;
	  rires = cur_ires == 1 ? rires1 : rires2;
	  chg_ires = 0;
  }

  p = 0;

  while(nbufsamples+nsamples >= winlen) // enough samples collected for EQ-processing?
    {
	  /*switch(bps)
	  {
	  case 8:
		for(i=0;i<(winlen-nbufsamples)*nch;i++)
			{
				inbuf[nbufsamples*nch+i] = ((unsigned char *)buf)[i+p*nch] - 0x80;
				float s = outbuf[nbufsamples*nch+i];
				if (dither) {
					float u;
					s -= equ_modifySamples_hm1;
					u = s;
					s += ditherbuf[(ditherptr++) & (DITHERLEN-1)];
					if (s < amin) s = amin;
					if (amax < s) s = amax;
					s = RINT(s);
					equ_modifySamples_hm1 = s - u;
					((unsigned char *)buf)[i+p*nch] = s + 0x80;
				} else {
					if (s < amin) s = amin;
					if (amax < s) s = amax;
					((unsigned char *)buf)[i+p*nch] = RINT(s) + 0x80;
				}
			}
		for(i=winlen*nch;i<tabsize*nch;i++)
			outbuf[i-winlen*nch] = outbuf[i];

		break;

	  case 16:*/
		for(i=0;i<(winlen-nbufsamples)*nch;i++)
			{
				inbuf[nbufsamples*nch+i] = ((short *)buf)[i+p*nch];
				float s = outbuf[nbufsamples*nch+i];
				/*if (dither) {
					float u;
					s -= equ_modifySamples_hm1;
					u = s;
					s += ditherbuf[(ditherptr++) & (DITHERLEN-1)];
					if (s < amin) s = amin;
					if (amax < s) s = amax;
					s = RINT(s);
					equ_modifySamples_hm1 = s - u;
					((short *)buf)[i+p*nch] = s;
				} else {*/
					if (s < amin) s = amin;
					if (amax < s) s = amax;
					((short *)buf)[i+p*nch] = RINT(s);
				/*}*/
			}
		for(i=winlen*nch;i<tabsize*nch;i++)
			outbuf[i-winlen*nch] = outbuf[i];
		/*break;

	  case 24:
		for(i=0;i<(winlen-nbufsamples)*nch;i++)
			{
				((int *)inbuf)[nbufsamples*nch+i] =
					(((unsigned char *)buf)[(i+p*nch)*3  ]      ) +
					(((unsigned char *)buf)[(i+p*nch)*3+1] <<  8) +
					(((  signed char *)buf)[(i+p*nch)*3+2] << 16) ;

				float s = outbuf[nbufsamples*nch+i];
				//if (dither) s += ditherbuf[(ditherptr++) & (DITHERLEN-1)];
				if (s < amin) s = amin;
				if (amax < s) s = amax;
				int s2 = RINT(s);
				((signed char *)buf)[(i+p*nch)*3  ] = s2 & 255; s2 >>= 8;
				((signed char *)buf)[(i+p*nch)*3+1] = s2 & 255; s2 >>= 8;
				((signed char *)buf)[(i+p*nch)*3+2] = s2 & 255;
			}
		for(i=winlen*nch;i<tabsize*nch;i++)
			outbuf[i-winlen*nch] = outbuf[i];

		break;

	  default:
		assert(0);
	  }*/

      p += winlen-nbufsamples;
      nsamples -= winlen-nbufsamples;
      nbufsamples = 0;

      for(ch=0;ch<nch;ch++)
		{
			ires = ch == 0 ? lires : rires;

			if (bps == 24) {
				for(i=0;i<winlen;i++)
					fsamples[i] = ((int *)inbuf)[nch*i+ch];
			} else {
				for(i=0;i<winlen;i++)
					fsamples[i] = inbuf[nch*i+ch];
			}

			for(i=winlen;i<tabsize;i++)
				fsamples[i] = 0;

			/*if (enable) {*/
				rfft(tabsize,1,fsamples);

				fsamples[0] = ires[0]*fsamples[0];
				fsamples[1] = ires[1]*fsamples[1];

				for(i=1;i<tabsize/2;i++)
					{
						REAL re,im;

						re = ires[i*2  ]*fsamples[i*2] - ires[i*2+1]*fsamples[i*2+1];
						im = ires[i*2+1]*fsamples[i*2] + ires[i*2  ]*fsamples[i*2+1];

						fsamples[i*2  ] = re;
						fsamples[i*2+1] = im;
					}

				rfft(tabsize,-1,fsamples);
			/*} else {
				for(i=winlen-1+winlen/2;i>=winlen/2;i--) fsamples[i] = fsamples[i-winlen/2]*tabsize/2;
				for(;i>=0;i--) fsamples[i] = 0;
			}*/

			for(i=0;i<winlen;i++) outbuf[i*nch+ch] += fsamples[i]/tabsize*2;

			for(i=winlen;i<tabsize;i++) outbuf[i*nch+ch] = fsamples[i]/tabsize*2;
		}

    } // while(nbufsamples+nsamples >= winlen) // enough samples collected for EQ-processing?

	/*switch(bps)
	  {
	  case 8:
		for(i=0;i<nsamples*nch;i++)
			{
				inbuf[nbufsamples*nch+i] = ((unsigned char *)buf)[i+p*nch] - 0x80;
				float s = outbuf[nbufsamples*nch+i];
				if (dither) {
					float u;
					s -= equ_modifySamples_hm1;
					u = s;
					s += ditherbuf[(ditherptr++) & (DITHERLEN-1)];
					if (s < amin) s = amin;
					if (amax < s) s = amax;
					s = RINT(s);
					equ_modifySamples_hm1 = s - u;
					((unsigned char *)buf)[i+p*nch] = s + 0x80;
				} else {
					if (s < amin) s = amin;
					if (amax < s) s = amax;
					((unsigned char *)buf)[i+p*nch] = RINT(s) + 0x80;
				}
			}
		break;

	  case 16:*/
		// collect rest samples
		for(i=0;i<nsamples*nch;i++)
			{
				inbuf[nbufsamples*nch+i] = ((short *)buf)[i+p*nch];
				float s = outbuf[nbufsamples*nch+i];
				/*if (dither) {
					float u;
					s -= equ_modifySamples_hm1;
					u = s;
					s += ditherbuf[(ditherptr++) & (DITHERLEN-1)];
					if (s < amin) s = amin;
					if (amax < s) s = amax;
					s = RINT(s);
					equ_modifySamples_hm1 = s - u;
					((short *)buf)[i+p*nch] = s;
				} else {*/
					if (s < amin) s = amin;
					if (amax < s) s = amax;
					((short *)buf)[i+p*nch] = RINT(s);
				/*}*/
			}
		/*break;

	  case 24:
		for(i=0;i<nsamples*nch;i++)
			{
				((int *)inbuf)[nbufsamples*nch+i] =
					(((unsigned char *)buf)[(i+p*nch)*3  ]      ) +
					(((unsigned char *)buf)[(i+p*nch)*3+1] <<  8) +
					(((  signed char *)buf)[(i+p*nch)*3+2] << 16) ;

				float s = outbuf[nbufsamples*nch+i];
				//if (dither) s += ditherbuf[(ditherptr++) & (DITHERLEN-1)];
				if (s < amin) s = amin;
				if (amax < s) s = amax;
				int s2 = RINT(s);
				((signed char *)buf)[(i+p*nch)*3  ] = s2 & 255; s2 >>= 8;
				((signed char *)buf)[(i+p*nch)*3+1] = s2 & 255; s2 >>= 8;
				((signed char *)buf)[(i+p*nch)*3+2] = s2 & 255;
			}
		break;

	  default:
		assert(0);
	}*/

  //p += nsamples__;
  nbufsamples += nsamples;

  //return p;
}


/*******************************************************************************
 * dsp_superequ.cpp
 ******************************************************************************/


float last_srate;
int last_nch /*, last_bps*/;
float lbands[18];
float rbands[18];
bool bands_changed;
paramlist paramroot;

int modify_samples(void *this_mod, short int *samples, int numsamples, int bps, int nch, int srate)
{
	if ((nch != 1 && nch != 2) || (bps != 8 && bps != 16 && bps != 24)) return numsamples;

	if (last_srate != srate || bands_changed) {
		equ_makeTable(lbands,rbands,&paramroot,srate);
		bands_changed = false;
		last_srate = srate;
		last_nch = nch;
		//last_bps = bps;
		equ_clearbuf(/*bps,srate*/);
	} else if (last_nch != nch /*|| last_bps != bps*/) {
		last_nch = nch;
		//last_bps = bps;
		equ_clearbuf(/*bps,srate*/);
	}

	equ_modifySamples((char *)samples,numsamples,nch,bps);

	return numsamples;
}

}; // class SjSuperEQ


/*******************************************************************************
 * SjEqualizer
 ******************************************************************************/


SjEqualizer::SjEqualizer()
{
	m_superEq             = NULL;
	m_enabled             = false;
	m_currParamChanged    = true; // force init
	m_currChannels        = 0;
	m_currSamplerate      = 0;
	m_deinterlaceBuf      = NULL;
	m_deinterlaceBufBytes = 0;
}


SjEqualizer::~SjEqualizer()
{
	if( m_superEq ) {
		delete m_superEq;
	}

	if( m_deinterlaceBuf ) free(m_deinterlaceBuf);
}


void SjEqualizer::SetParam(bool newEnabled, const SjEqParam& newParam)
{
	m_paramCritical.Enter();
		m_enabled          = newEnabled;
		if( m_currParam != newParam ) {
			m_currParam        = newParam;
			m_currParamChanged = true;
		}
	m_paramCritical.Leave();
}


void SjEqualizer::AdjustBuffer(float* buffer, long bytes, int samplerate, int channels)
{
	if( !m_enabled || buffer == NULL || bytes <= 0 || samplerate <= 0 || channels <= 0 || channels > SJ_EQ_MAX_CHANNELS ) return; // nothing to do/error

	if( m_superEq == NULL ) {
		m_superEq = new SjSuperEQ(14);
		if( m_superEq == NULL ) { return; } // error
	}

	m_paramCritical.Enter();
		if( m_currParamChanged )
		{
			// realize the new parameters
			float gain;
			for( int b = 0; b < SJ_EQ_BANDS; b++ )
			{
				if( m_currParam.m_bandDb[b] <= -20.0F ) {
					gain = 0.0F;
				}
				else {
					gain = (float)SjDecibel2Gain(m_currParam.m_bandDb[b]);
				}
				m_superEq->lbands[b] = gain;
				m_superEq->rbands[b] = gain;
				m_superEq->bands_changed = true;
			}
			m_currParamChanged = false;
		}
	m_paramCritical.Leave();

	// TEMP: Convert float to pcm16
	if( channels != 2 ) return; // error
	SjFloatToPcm16(buffer, (signed short*)buffer, bytes);
	bytes /= 2;

	// TEMP: DSP
	m_superEq->modify_samples(NULL, (signed short*)buffer, bytes/4, 16, channels, samplerate);

	// TEMP: Convert pcm16 back to float
	SjPcm16ToFloat((const signed short*)buffer, buffer, bytes);
}

