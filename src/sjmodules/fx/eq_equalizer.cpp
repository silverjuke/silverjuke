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

//#define RINT(x) ((x) >= 0 ? ((int)((x) + 0.5)) : ((int)((x) - 0.5)))

REAL fact[M+1];
REAL aa;
REAL iza;
REAL *lires,*lires1,*lires2,*rires,*rires1,*rires2,*irest;
REAL *fsamples;
volatile int chg_ires,cur_ires;
int winlen,winlenbit,tabsize,nbufsamples;
REAL *inbuf;
REAL *outbuf;

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

  winlen = (1 << (wb-1))-1;
  winlenbit = wb;
  tabsize  = 1 << wb;

  lires1   = (REAL *)malloc(sizeof(REAL)*tabsize);
  lires2   = (REAL *)malloc(sizeof(REAL)*tabsize);
  rires1   = (REAL *)malloc(sizeof(REAL)*tabsize);
  rires2   = (REAL *)malloc(sizeof(REAL)*tabsize);
  irest    = (REAL *)malloc(sizeof(REAL)*tabsize);
  fsamples = (REAL *)malloc(sizeof(REAL)*tabsize);
  inbuf    = (REAL *)calloc(winlen*NCH,sizeof(REAL));
  outbuf   = (REAL *)calloc(tabsize*NCH,sizeof(REAL));

  lires = lires1;
  rires = rires1;
  cur_ires = 1;
  chg_ires = 1;

  for(i=0;i<=M;i++)
    {
      fact[i] = 1;
      for(j=1;j<=i;j++) fact[i] *= j;
    }

  iza = izero(alpha(aa));

  last_srate = 0;
  last_nch = 0;
  for( i = 0; i < 18; i++ ) { lbands[i]=1.0; rbands[i]=1.0; }
  bands_changed = false;
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

  newipsize = 2+sqrt((float)(n/2));
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

void equ_clearbuf()
{
	int i;

	nbufsamples = 0;
	for(i=0;i<tabsize*NCH;i++) outbuf[i] = 0;
}

void equ_modifySamples(REAL *buf,int nsamples,int nch)
{
  int i,p,ch;
  REAL *ires;

  if (chg_ires) {
	  cur_ires = chg_ires;
	  lires = cur_ires == 1 ? lires1 : lires2;
	  rires = cur_ires == 1 ? rires1 : rires2;
	  chg_ires = 0;
  }

  p = 0;

  while(nbufsamples+nsamples >= winlen) // enough samples collected for EQ-processing?
    {
		for(i=0;i<(winlen-nbufsamples)*nch;i++)
			{
				inbuf[nbufsamples*nch+i] = buf[i+p*nch];
				buf[i+p*nch] = outbuf[nbufsamples*nch+i];
			}
		for(i=winlen*nch;i<tabsize*nch;i++)
			outbuf[i-winlen*nch] = outbuf[i];

      p += winlen-nbufsamples;
      nsamples -= winlen-nbufsamples;
      nbufsamples = 0;

      for(ch=0;ch<nch;ch++)
		{
			ires = ch == 0 ? lires : rires;

			for(i=0;i<winlen;i++)
				fsamples[i] = inbuf[nch*i+ch];

			for(i=winlen;i<tabsize;i++)
				fsamples[i] = 0;

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

			for(i=0;i<winlen;i++) outbuf[i*nch+ch] += fsamples[i]/tabsize*2;

			for(i=winlen;i<tabsize;i++) outbuf[i*nch+ch] = fsamples[i]/tabsize*2;
		}

    }

		// collect rest samples
		for(i=0;i<nsamples*nch;i++)
			{
				inbuf[nbufsamples*nch+i] = buf[i+p*nch];
				buf[i+p*nch] = outbuf[nbufsamples*nch+i];
			}

  nbufsamples += nsamples;
}


/*******************************************************************************
 * dsp_superequ.cpp
 ******************************************************************************/


float last_srate;
int last_nch;
float lbands[18];
float rbands[18];
bool bands_changed;
paramlist paramroot;

int modify_samples(void *this_mod, REAL *samples, int numsamples, int nch, int srate)
{
	if ((nch != 1 && nch != 2) ) return numsamples;

	if (last_srate != srate || bands_changed) {
		equ_makeTable(lbands,rbands,&paramroot,srate);
		bands_changed = false;
		last_srate = srate;
		last_nch = nch;
		equ_clearbuf();
	} else if (last_nch != nch ) {
		last_nch = nch;
		equ_clearbuf();
	}

	equ_modifySamples(samples,numsamples,nch);

	return numsamples;
}

}; // class SjSuperEQ


/*******************************************************************************
 * SjEqualizer
 ******************************************************************************/


SjEqualizer::SjEqualizer()
{
	m_enabled             = false;
	m_superEqCnt          = 0;
	m_currParamChanged    = true; // force init
	m_currSamplerate      = 0;
	m_deinterlaceBuf      = NULL;
	m_deinterlaceBufBytes = 0;
}


SjEqualizer::~SjEqualizer()
{
	delete_eqs();

	if( m_deinterlaceBuf ) { free(m_deinterlaceBuf); }
}


void SjEqualizer::delete_eqs()
{
	for( int c = 0; c < m_superEqCnt; c++ ) {
		delete m_superEq[c];
	}
	m_superEqCnt = 0;
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

	// (re-)allocate equalizer objects, one per channel
	if( m_superEqCnt != channels )
	{
		delete_eqs();
		for( int c = 0; c < channels; c++ ) {
			m_superEq[c] = new SjSuperEQ(14);
			if( m_superEq[c] == NULL ) { return; } // error
		}
		m_superEqCnt = channels;
	}

	// realize new parameters, if any
	m_paramCritical.Enter();
		if( m_currParamChanged )
		{
			for( int b = 0; b < SJ_EQ_BANDS; b++ )
			{
				float gain = m_currParam.m_bandDb[b] <= -20.0F? 0.0F : (float)SjDecibel2Gain(m_currParam.m_bandDb[b]);
				for( int c = 0; c < channels; c++ )
				{
					m_superEq[c]->lbands[b] = gain;
					m_superEq[c]->rbands[b] = gain;
					m_superEq[c]->bands_changed = true;
				}
			}
			m_currParamChanged = false;
		}
	m_paramCritical.Leave();

	// (re-)allocate help buffer for deinterlacing
	if( bytes > m_deinterlaceBufBytes )
	{
		if( m_deinterlaceBuf ) { free(m_deinterlaceBuf); m_deinterlaceBuf = NULL; }
		m_deinterlaceBuf = (float*)malloc(bytes);
		if( m_deinterlaceBuf == NULL ) { return; } // error;
		m_deinterlaceBufBytes = bytes;
	}

	// eq processing
	const float *bufferEnd = buffer + (bytes/sizeof(float));
	for( int c = 0; c < channels; c++ )
	{
		// deinterlace from `buffer` to `m_deinterlaceBuf`
		const float *srcPtr = buffer + c;
		float *destPtr =  m_deinterlaceBuf;
		while( srcPtr < bufferEnd )  {
			*destPtr++ = *srcPtr;
			srcPtr += channels;
		}

		// call equalizer
		m_superEq[c]->modify_samples(NULL, m_deinterlaceBuf, bytes/channels/sizeof(float), 1, samplerate);

		// interlace equalized data from `m_deinterlaceBuf` back to buffer
		srcPtr = m_deinterlaceBuf;
		destPtr = buffer + c;
		while( destPtr < bufferEnd )  {
			*destPtr = *srcPtr++;
			destPtr += channels;
		}
	}
}

