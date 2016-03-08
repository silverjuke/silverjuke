/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2015 Björn Petersen Software Design and Development
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
 * File:    oscilloscope.cpp
 * Authors: Björn Petersen
 * Purpose: Simple "Oscilloscope visualization"
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/vis/vis_oscilloscope.h>
#include <sjmodules/vis/vis_window.h>
#include <math.h>
#include <kiss_fft/tools/kiss_fftr.h>

// you should not change SLEEP_MS without reasons.
// IF you change it, also check if really all time-depending calculations are still correct.
// (a sleep of 30 ms will render about 33 frames/s; on my 2012er i7, 2,9 GHz each frames needs about 0-1 ms for calculation)
#define SLEEP_MS 30


/*******************************************************************************
 *  SjOscStarfield
 ******************************************************************************/


class SjOscStar
{
public:
	void            Init                ();
	void            Exit                () { m_exitRequest = TRUE; }
	bool            Draw                (wxDC& dc, const wxSize&, double rot, wxPen&, wxBrush&);

private:
	// internal calculations are done in a 1000 x 1000 map
	#define         STAR_DEPTH 100
	int             m_x, m_y, m_z;
	bool            m_doDraw,
	                m_exitRequest;
};


void SjOscStar::Init()
{
	m_x = (int)SjTools::Rand(1000) - 500;
	m_y = (int)SjTools::Rand(1000) - 500;
	m_z = (int)SjTools::Rand(STAR_DEPTH);

	if( (m_x == 0) && (m_y == 0 ) )
	{
		m_x = 10;
	}

	m_doDraw = FALSE;
	m_exitRequest = FALSE;
}


bool SjOscStar::Draw(wxDC& dc, const wxSize& clientSize, double rot, wxPen& pen, wxBrush& brush)
{
	double  xfloat, yfloat;
	int     x, y, hh, vv;
	int     d, intensity;

	m_z -= 2;
	if( m_z < -63 )
	{
		m_z = STAR_DEPTH;
		m_doDraw = !m_exitRequest;
	}

	hh = (m_x*64)/(64+m_z);
	vv = (m_y*64)/(64+m_z);

	// check position
	x = hh + 500;
	y = vv + 500;
	if( x < -300 || x > 1300 || y < -300 || y > 1300 )
	{
		m_z = STAR_DEPTH;
		m_doDraw = !m_exitRequest;

		hh = (m_x*64)/(64+m_z);
		vv = (m_y*64)/(64+m_z);
	}

	// calculate position
	xfloat = (hh*cos(rot))-(vv*sin(rot));
	yfloat = (hh*sin(rot))+(vv*cos(rot));

	x = (int)xfloat + 500;
	y = (int)yfloat + 500;

	// use star?
	if( !m_doDraw )
	{
		if( m_exitRequest || x < 450 || x > 550 || y < 450 || y > 550 )
		{
			return FALSE;
		}
		else
		{
			m_doDraw = TRUE;
		}
	}

	// map star position to client size, keep aspect ratio
	d = clientSize.x;
	if( clientSize.y > d )
	{
		d = clientSize.y;
	}

	if( d == 0 )
	{
		d = 10;
	}

	x = (x * d) / 1000 - (d-clientSize.x)/2;
	y = (y * d) / 1000 - (d-clientSize.y)/2;

	// calculate size
	d = (STAR_DEPTH-m_z) / (38400/d);
	if( d == 0 ) d = 1;

	// calculate light intensity
	intensity = STAR_DEPTH-m_z;
	intensity = 55 + ((intensity * 200) / STAR_DEPTH);
	//if( intensity < light ) intensity = light + 10;
	if( intensity < 0   )   intensity = 0;
	if( intensity > 255 )   intensity = 255;

	// draw star
	if( d==1 )
	{
		pen.SetColour(intensity, intensity, intensity);
		dc.SetPen(pen);
		dc.DrawPoint(x, y);
	}
	else
	{
		dc.SetPen(*wxTRANSPARENT_PEN);

		brush.SetColour(intensity, intensity, intensity);
		dc.SetBrush(brush);

		dc.DrawRectangle(x, y, d, d);
	}

	return TRUE;
}


class SjOscStarfield
{
public:
	#define         STARFIELD_MODE_SLEEP    1
	#define         STARFIELD_MODE_DO       2
	#define         STARFIELD_MODE_FADEOUT  3
					SjOscStarfield      ();
	void            Draw                (wxDC& dc, const wxSize& clientSize,
	                                     bool otherRunning, bool newTitle, bool on);
	bool            IsRunning           () const { return m_mode==STARFIELD_MODE_DO; }

private:
	#define         STAR_COUNT 300
	SjOscStar       pol[STAR_COUNT];
	double          m_rotate;
	char            m_rsMode;
	long            m_rsStepsLeft;
	double          m_rsAdd,
	                m_rsInc;

	int             m_mode;
	long            m_progress;

	wxPen           m_pen;
	wxBrush         m_brush;
};


SjOscStarfield::SjOscStarfield()
{
	m_mode      = STARFIELD_MODE_SLEEP;
	m_progress  = 400 / SLEEP_MS;
}


void SjOscStarfield::Draw(wxDC& dc, const wxSize& clientSize,
                          bool otherRunning, bool newTitle, bool on)
{
	int     i;

	if( m_mode == STARFIELD_MODE_SLEEP )
	{
		// sleep
		if( m_progress > 0 )
			m_progress--;

		if( !on )
			return;

		if( (m_progress <= 0 && !otherRunning) )
		{
			m_rotate = 0;
			m_rsMode = 0;
			m_rsStepsLeft = (1000+SjTools::Rand(2000)) / SLEEP_MS;

			m_mode = STARFIELD_MODE_DO;
			m_progress = (30000 + SjTools::Rand(90000)) / SLEEP_MS;

			for( i = 0; i < STAR_COUNT; i++ )
			{
				pol[i].Init();
			}
		}
	}
	else
	{
		// do or fadeout
		if( m_mode == STARFIELD_MODE_DO )
		{
			m_progress--;
			if(  m_progress <= 0
			        ||  newTitle
			        || !on )
			{
				m_mode = STARFIELD_MODE_FADEOUT;
				for( i = 0; i < STAR_COUNT; i++ )
				{
					pol[i].Exit();
				}
			}
		}

		// rotate stars (rs = rotate settings)
		m_rsStepsLeft--;
		if( m_rsMode == 0 && m_rsStepsLeft <= 0 )
		{
			m_rsStepsLeft = (1000+SjTools::Rand(2000)) / SLEEP_MS;
			long sign = SjTools::Rand(2)==0? 1 : -1;
			m_rsAdd = 0.01 * sign;
			m_rsInc = 0.001 * sign;
			m_rsMode = 'b';
		}
		if( m_rsMode == 'b' && m_rsStepsLeft <= 0 )
		{
			m_rsInc *= (1+SjTools::Rand(4)) * -1;
			m_rsMode = 'a';
		}
		else if( m_rsMode == 'a' && m_rsAdd < 0.01 && m_rsAdd > -0.01 )
		{
			m_rsStepsLeft = SjTools::Rand(SjTools::Rand(3)==0? 8000 : 2000) / SLEEP_MS;
			m_rsMode = 0;
		}

		if( m_rsMode != 0 )
		{
			m_rotate += m_rsAdd;
			m_rsAdd += m_rsInc;
		}

		// draw
		bool anythingDrawn = FALSE;
		for( i = 0; i < STAR_COUNT; i++ )
		{
			if( pol[i].Draw(dc, clientSize, m_rotate, m_pen, m_brush) )
			{
				anythingDrawn = TRUE;
			}
		}

		if( !anythingDrawn && m_mode == STARFIELD_MODE_FADEOUT )
		{
			m_mode = STARFIELD_MODE_SLEEP;
			m_progress = (3000 + SjTools::Rand(3000)) / SLEEP_MS;
		}
	}
}


/*******************************************************************************
 *  SjOscFirework
 ******************************************************************************/


class SjOscRocket
{
public:
	                SjOscRocket         ();
	void            Init                (int energy, int patch, int length, long seed, int mx, int my);
	void            Start               ();
	void            Show                (wxDC& dc, long light);
	double          NextDouble          () { return (double)SjTools::PrivateRand(m_random, 10000)/10000.0F; }

	bool            m_sleep;

private:
	#define         MAX_ROCKET_PATCH_NUMBER 90
	#define         ROCKET_GRAVITY 400
	int             m_energy, m_patch, m_length,
	                m_mx, m_my,
	                m_ox, m_oy,
	                m_vx[MAX_ROCKET_PATCH_NUMBER],
	                m_vy[MAX_ROCKET_PATCH_NUMBER],
	                m_t;
	wxPen           m_pen;
	long            m_random;
};


SjOscRocket::SjOscRocket()
{
	m_sleep = TRUE;
}


void SjOscRocket::Init(int energy, int patch, int length, long seed, int mx, int my)
{
	int i;

	m_mx        = mx;
	m_my        = my;

	m_energy    = energy;
	m_patch     = patch;
	m_length    = length;
	m_random    = seed;

	m_pen.SetColour(255, 255, 255);

	m_ox=(int)SjTools::Rand(m_mx/2)+m_mx/4;
	m_oy=(int)SjTools::Rand(m_my/2)+m_my/4;

	for(i=0; i<m_patch; i++)
	{
		m_vx[i]=(int)SjTools::Rand(m_energy)-m_energy/2;
		m_vy[i]=(int)SjTools::Rand(m_energy*7/8)-m_energy/8;
	}
}


void SjOscRocket::Start()
{
	m_t = 0;
	m_sleep = FALSE;
}


void SjOscRocket::Show(wxDC& dc, long light)
{
	if(m_sleep)
	{
		return;
	}

	if( m_t<m_length )
	{
		int i, j, x, y, maxL;
		double s;

		#define VIEW 20
		maxL = VIEW;
		if( maxL > m_length-m_t )
		{
			maxL = m_length-m_t;
			long intensity = (255/VIEW)*(m_length-m_t);
			if( light > intensity ) intensity = light;
			m_pen.SetColour(intensity, intensity, intensity);
		}

		dc.SetPen(m_pen);

		wxPoint points[VIEW];
		for(i=0; i<m_patch; i++)
		{
			for( j = 0; j < maxL; j++ )
			{
				s=(double)(m_t+j)/100;
				x=(int)(m_vx[i]*s);
				y=(int)(m_vy[i]*s-ROCKET_GRAVITY*s*s);

				points[j].x = m_ox+x;
				points[j].y = m_oy-y;
			}

			dc.DrawLines(maxL, points);
		}

		m_t++;
	}
	else
	{
		m_sleep = TRUE;
	}
}


class SjOscFirework
{
public:
	                SjOscFirework       ();
	void            Draw                (wxDC& dc, const wxSize& clientSize, bool newTitle, bool otherRunning, bool volumeBeat, long light);

private:
	#define         MAX_ROCKET_NUMBER       4
	#define         MAX_ROCKET_PATCH_LEN    68
	#define         MAX_ROCKET_EXPL_ENERGY  850
	SjOscRocket     m_rockets[MAX_ROCKET_NUMBER];

	#define         FIREWORK_MODE_SLEEP 1
	#define         FIREWORK_MODE_DO    3
	int             m_mode;
	long            m_progress;
};


SjOscFirework::SjOscFirework()
{
	m_mode = FIREWORK_MODE_SLEEP;
	m_progress = SjTools::Rand(120000/SLEEP_MS);
}


void SjOscFirework::Draw(wxDC& dc, const wxSize& clientSize,
                         bool newTitle, bool otherRunning, bool volumeBeat,
                         long light)
{
	int i, runningRockets = 0;

	// sleep
	if( m_mode == FIREWORK_MODE_SLEEP )
	{
		m_progress--;

		if( (m_progress <= 0 && !otherRunning && volumeBeat) )
		{
			m_mode = FIREWORK_MODE_DO;
			m_progress = MAX_ROCKET_NUMBER+SjTools::Rand(MAX_ROCKET_NUMBER);
		}
		else
		{
			return;
		}
	}

	// do
	if( newTitle )
	{
		m_progress = 0;
	}

	for( i=0; i<MAX_ROCKET_NUMBER; i++ )
	{
		if( m_rockets[i].m_sleep
		        && m_progress > 0
		        && (volumeBeat || SjTools::Rand(1000/SLEEP_MS)==0) )
		{
			int e=(int)SjTools::Rand(MAX_ROCKET_EXPL_ENERGY*3/4)+
			      MAX_ROCKET_EXPL_ENERGY/4+1;

			int p=(int)SjTools::Rand(MAX_ROCKET_PATCH_NUMBER*3/4)+
			      MAX_ROCKET_PATCH_NUMBER/4;
			if( p >= MAX_ROCKET_PATCH_NUMBER ) p = MAX_ROCKET_PATCH_NUMBER-1;

			int l=(int)SjTools::Rand(MAX_ROCKET_PATCH_LEN*3/4)+
			      MAX_ROCKET_PATCH_LEN/4+1;

			long s=(long)SjTools::Rand(10000);

			m_rockets[i].Init(e,p,l,s, clientSize.x, clientSize.y);
			m_rockets[i].Start();

			m_progress--;
			volumeBeat = FALSE; // one racket ALWAYS per beat
		}

		if( !m_rockets[i].m_sleep )
		{
			runningRockets++;
			m_rockets[i].Show(dc, light);
		}
	}

	if( runningRockets <= 0 && m_progress <= 0 )
	{
		m_mode = FIREWORK_MODE_SLEEP;
		m_progress = SjTools::Rand(120000/SLEEP_MS);
	}
}


/*******************************************************************************
 * SjOscHands
 ******************************************************************************/


class SjOscHands
{
public:
	                SjOscHands          ();
	void            Draw                (wxDC& dc, const wxSize& clientSize, long volume, long light, bool newTitle);

private:
	#define         HAND_POINTS         19
	#define         HAND_W              54
	#define         HAND_H              81
	#define         MAX_HANDS           10
	#define         HAND_MODE_NOP       0
	#define         HAND_MODE_FADEIN    1
	#define         HAND_MODE_DO        2
	#define         HAND_MODE_FADEOUT   3
	wxPoint         m_logHandsPos[MAX_HANDS];
	long            m_logHandRnd[MAX_HANDS];
	long            m_pause;
	long            m_logHandMode;
	long            m_logHandModeData;
	bool            m_firstTitle;
	wxPen           m_pen;
};


static const unsigned char s_logHandPoints[HAND_POINTS*2] =
{
	21, 255,        21, 54,         15, 46,         12, 31,
	0,  10,         7,  6,          18, 29,         18, 21,
	25, 21,         25, 30,         25, 21,         31, 19,
	31, 29,         46, 0,          53, 4,          38, 33,
	38, 43,         31, 53,         33, 255
};


SjOscHands::SjOscHands()
{
	m_pause         = SjTools::Rand(90000/SLEEP_MS);
	m_logHandMode   = HAND_MODE_NOP;
	m_firstTitle    = TRUE;
}


void SjOscHands::Draw(wxDC& dc, const wxSize& clientSize,
                      long volume, long light, bool newTitle)
{
	long i, offset = 0;

	// (re-)calculate log. hand data
	if( m_logHandMode == HAND_MODE_NOP )
	{
		m_pause--;

		if( m_pause <= 0 // about once in 90 seconds or every 4 titles
		        || (newTitle && !m_firstTitle && SjTools::Rand(4) == 0) )
		{
			for( i = 0; i < MAX_HANDS; i++ )
			{
				m_logHandsPos[i].x = i*(1024/MAX_HANDS)+SjTools::Rand((1024/MAX_HANDS)-HAND_W/2);
				m_logHandsPos[i].y = ((768-HAND_H) + 10) - SjTools::Rand(20);
				m_logHandRnd[i]    = 5-SjTools::Rand(10);
				if( m_logHandRnd[i] == 0 ) m_logHandRnd[i] = 1;
			}
			m_logHandMode = HAND_MODE_FADEIN;
			m_logHandModeData = HAND_H+volume;
			m_pause = (10000+SjTools::Rand(90000))/SLEEP_MS;
			offset = m_logHandModeData;
			m_firstTitle = FALSE;
		}
		else
		{
			if( newTitle ) m_firstTitle = FALSE;
			return;
		}
	}
	else if( m_logHandMode == HAND_MODE_FADEIN )
	{
		m_logHandModeData--;
		offset = m_logHandModeData;

		if( m_logHandModeData == 0 )
		{
			m_logHandMode = HAND_MODE_DO;
			m_logHandModeData = (10000 + SjTools::Rand(50000)) / SLEEP_MS; // 10-60 seconds
		}
	}
	else if( m_logHandMode == HAND_MODE_DO )
	{
		m_logHandModeData--;
		offset = 0;

		if( m_logHandModeData == 0
		        || (newTitle && SjTools::Rand(4)==0) )
		{
			m_logHandMode = HAND_MODE_FADEOUT;
			m_logHandModeData = (HAND_H+80);
		}
	}
	else if( m_logHandMode == HAND_MODE_FADEOUT )
	{
		m_logHandModeData--;
		offset = (HAND_H+80) - m_logHandModeData;

		if( m_logHandModeData == 0 )
		{
			m_logHandMode = HAND_MODE_NOP;
			return;
		}
	}

	// calculate a hand for the given screen resolution
	wxPoint scrHandPoints[19];
	for( i = 0; i < HAND_POINTS; i++ )
	{
		scrHandPoints[i].x = (s_logHandPoints[i*2  ] * clientSize.x) / 1024;
		scrHandPoints[i].y = (s_logHandPoints[i*2+1] * clientSize.x) / 1024/*not:768*/;
	}

	// draw the hands
	long intensity = 42+light;
	if( intensity < 70 ) intensity = 70;
	if( intensity > 255 ) intensity = 255;
	m_pen.SetColour(intensity, intensity, intensity);

	dc.SetPen(m_pen);

	for( i = 0; i < MAX_HANDS; i++ )
	{
		dc.DrawLines(HAND_POINTS, scrHandPoints,
		             (m_logHandsPos[i].x * clientSize.x) / 1024,
		             offset + (((m_logHandsPos[i].y * clientSize.y) / 768) - volume/m_logHandRnd[i]));
	}
}


/*******************************************************************************
 * SjOscRotor
 ******************************************************************************/


class SjOscRotor
{
public:
	#define         PI 3.14159265F
	#define         ROTOR_MODE_NOP          0
	#define         ROTOR_MODE_FADEIN       1
	#define         ROTOR_MODE_DO           2
	#define         ROTOR_MODE_FADEOUT      3
	#define         ROTOR_MODE_FADEOUTFAST  4
	                SjOscRotor          ();
	void            Draw                (wxDC& dc, const wxSize& clientSize, bool otherRunning, bool newTitle, long volume, long light);
	bool            IsRunning           () const { return (m_mode==ROTOR_MODE_FADEIN||m_mode==ROTOR_MODE_DO); }

private:
	long            m_pause;
	long            m_mode;
	int             m_logCenterX,
	                m_logCenterY,
	                m_logColour;
	long            m_logDo;
	double          m_delta;
	bool            m_followVolume;
	wxPen           m_pen;
};


SjOscRotor::SjOscRotor()
{
	#define ROTOR_SLEEP_MS 120000
	m_pause = SjTools::Rand(ROTOR_SLEEP_MS/SLEEP_MS);
	m_mode = ROTOR_MODE_NOP;
}


void SjOscRotor::Draw(wxDC& dc, const wxSize& clientSize, bool otherRunning, bool newTitle, long volume, long light)
{
	if( m_mode == ROTOR_MODE_NOP )
	{
		m_pause--;

		if( (m_pause <= 0 && !otherRunning) ) // about once in two minutes
		{
			m_mode          = ROTOR_MODE_FADEIN;
			m_logCenterX    = SjTools::Rand(100);
			m_logCenterY    = SjTools::Rand(100);
			m_logColour     = 0;
			m_delta         = 0;
			m_logDo         = (10000 + SjTools::Rand(50000)) / SLEEP_MS; // 10-60 seconds
			m_pause         = SjTools::Rand(ROTOR_SLEEP_MS/SLEEP_MS);
			m_followVolume  = SjTools::Rand(2)==0;
		}

		return;
	}
	else if( m_mode == ROTOR_MODE_FADEIN )
	{
		m_logColour++;

		long intensity = m_logColour;
		if( light > intensity ) intensity = light;
		m_pen.SetColour(intensity, intensity, intensity);

		if( m_logColour == 255 )
		{
			m_pen.SetColour(255, 255, 255);
			m_mode = ROTOR_MODE_DO;
		}
	}
	else if( m_mode == ROTOR_MODE_DO )
	{
		m_logDo--;
		if( m_logDo <= 0 || newTitle || otherRunning )
		{
			m_logDo = 0;
			m_mode = ROTOR_MODE_FADEOUT;
			if( newTitle || otherRunning )
			{
				m_mode = ROTOR_MODE_FADEOUTFAST;
				if( SjTools::Rand(2) == 0 && !otherRunning )
				{
					m_pause = 0;
				}
			}
		}
	}
	else if( m_mode == ROTOR_MODE_FADEOUT || m_mode == ROTOR_MODE_FADEOUTFAST )
	{
		m_logColour -= m_mode == ROTOR_MODE_FADEOUT? 1 : 3;
		if( m_logColour < 0 ) m_logColour = 0;

		long intensity = m_logColour;
		if( light > intensity ) intensity = light;

		m_pen.SetColour(intensity, intensity, intensity);
		if( m_logColour == 0 )
		{
			m_mode = ROTOR_MODE_NOP;
		}
	}

	if( m_followVolume )
	{
		double add = 0.001*volume;
		if( add < 0.005 ) add = 0.005;
		m_delta += add;
	}
	else
	{
		m_delta += 0.01;
	}

	if( m_delta >= PI )
	{
		m_delta -= PI;
	}

	int centerx = (clientSize.x*m_logCenterX) / 100,
	    centery = (clientSize.y*m_logCenterY) / 150/*not:100 - center is not in the lower part*/;
	int radius = (clientSize.x+clientSize.y)*2;

	dc.SetPen(m_pen);

	dc.DrawLine(centerx+sin(m_delta         )*radius, centery+cos(m_delta         )*radius,
	            centerx+sin(m_delta+PI      )*radius, centery+cos(m_delta+PI      )*radius);
	dc.DrawLine(centerx+sin(m_delta+PI/2    )*radius, centery+cos(m_delta+PI/2    )*radius,
	            centerx+sin(m_delta+PI/2+PI )*radius, centery+cos(m_delta+PI/2+PI )*radius);
}


/*******************************************************************************
 *  SjOscOscilloscope
 ******************************************************************************/


class SjOscOscilloscope
{
public:
	                SjOscOscilloscope   (long sampleCount);
	                ~SjOscOscilloscope  ();
	void            Calc                (const wxSize& clientSize,
	                                     const unsigned char* bufferStart,
	                                     long& retVolume);
	void            Draw                (wxDC& dc, bool forceAnim);

private:
	wxSize          m_clientSize;
	long            m_sampleCount;
	long            m_pointsCount;
	wxPoint*        m_lPoints;
	wxPoint*        m_rPoints;
	double          m_anim;
};


SjOscOscilloscope::SjOscOscilloscope(long sampleCount)
{
	m_anim = -1.0F;
	m_sampleCount = sampleCount;
	m_lPoints = new wxPoint[sampleCount];
	m_rPoints = new wxPoint[sampleCount];
}


SjOscOscilloscope::~SjOscOscilloscope()
{
	if( m_lPoints ) { delete [] m_lPoints; }
	if( m_rPoints ) { delete [] m_rPoints; }
}


void SjOscOscilloscope::Calc(const wxSize& clientSize, const unsigned char* bufferStart, long& retVolume)
{
	m_clientSize = clientSize;

	long maxY     = clientSize.y/4, i;
	long lCenterY = maxY;
	long rCenterY = maxY * 3;
	long lSample;
	long rSample;

	const unsigned char* bufferCurr = bufferStart;
	retVolume = 0;
	m_pointsCount = 0;
	for( i = 0; i < m_sampleCount; i++ )
	{
		// get 8 bit sample
		bufferCurr++;
		lSample = (signed char)*bufferCurr++;

		bufferCurr++;
		rSample = (signed char)*bufferCurr++;

		m_lPoints[m_pointsCount].x =
		    m_rPoints[m_pointsCount].x = (i*clientSize.x) / m_sampleCount;

		if( m_pointsCount==0 || m_lPoints[m_pointsCount].x!=m_lPoints[m_pointsCount-1].x )
		{
			// calculate y
			m_lPoints[m_pointsCount].y = lCenterY + (lSample*maxY) / 180; // 128=full amplitude height
			m_rPoints[m_pointsCount].y = rCenterY + (rSample*maxY) / 180;
			m_pointsCount++;

			// collect volume
			retVolume += lSample<0? -lSample : lSample;
			retVolume += rSample<0? -rSample : rSample;
		}
	}

	retVolume /= m_sampleCount;
}


void SjOscOscilloscope::Draw(wxDC& dc, bool forceAnim)
{
	wxCoord animOffset = 0;

	if( forceAnim )
	{
		m_anim = 1.4F;
	}

	if( m_anim >= 0.0F )
	{
		m_anim -= 2.0/(double)SLEEP_MS;
		if( m_anim > 0.0F )
		{
			animOffset = (wxCoord) ( ((double)m_clientSize.y/4.0F)*m_anim );
		}
	}

	dc.DrawLines(m_pointsCount, m_lPoints, 0, 0-animOffset);
	dc.DrawLines(m_pointsCount, m_lPoints, 0, 1-animOffset);

	dc.DrawLines(m_pointsCount, m_rPoints, 0, 0+animOffset);
	dc.DrawLines(m_pointsCount, m_rPoints, 0, 1+animOffset);
}


/*******************************************************************************
 * SjOscSpectrum
 ******************************************************************************/


class SjOscSpectrumChData
{
public:
	#define         NUM_BOXES 20
	#define         SPEC_NUM  576
	long            chNum;
	double          m_boxMax[NUM_BOXES];
	double          m_boxY[NUM_BOXES];

	SjOscSpectrumChData()
	{
		chNum = 0;
		for( int i = 0; i < NUM_BOXES; i++ )
		{
			m_boxMax[i] = 0;
			m_boxY[i] = 0;
		}
	}
};


class SjOscSpectrum
{
public:
	                SjOscSpectrum       ();
	                ~SjOscSpectrum      ();
	void            Calc                (const wxSize& clientSize,
	                                     const unsigned char* bufferStart);
	void            Draw                (wxDC& dc, bool volumeBeat, bool showFigures, bool forceAnim)
	{	Draw(dc, &m_chData[0], volumeBeat, showFigures, forceAnim);
		Draw(dc, &m_chData[1], volumeBeat, showFigures, forceAnim);
	}

private:
	wxSize          m_clientSize;
	kiss_fftr_cfg   m_kiss_fft_setup;
	SjOscSpectrumChData m_chData[2];

	void            Draw                (wxDC&, SjOscSpectrumChData* chData, bool volumeBeat, bool showFigures, bool forceAnim);
	void            DrawBand            (wxDC& dc, int x, int y, int w, int h, double val, double crazy);

	int             m_freqToBox[SPEC_NUM];
	long            m_boxSampleCount[NUM_BOXES];

	#define         CRAZY_MAX 2.00
	#define         CRAZY_INC  0.1
	#define         CRAZY_RAND (120000/SLEEP_MS)
	#define         CRAZY_NONE 0
	#define         CRAZY_FADEIN 1
	#define         CRAZY_STAY 2
	#define         CRAZY_FADEOUT 3
	int             m_crazyState;
	double          m_crazy;
	long            m_crazySleep;
	bool            m_firstCrazy;
};


SjOscSpectrum::SjOscSpectrum()
{
	m_kiss_fft_setup    = kiss_fftr_alloc(SPEC_NUM*2, 0, NULL, 0);
	m_chData[0].chNum   = 0;
	m_chData[1].chNum   = 1;

	m_crazyState = CRAZY_NONE;
	m_firstCrazy = FALSE;

	// calculate the frequency band -> box array
	static const unsigned int analyzer[20]= {1,3,5,7,9,13,19,25,38,58,78,116,156,195,235,274,313,352,391,430};
	int b;
	for( b = 0; b < NUM_BOXES; b++ )
	{
		int from = b?             analyzer[b-1]+(analyzer[b]  -analyzer[b-1])/2 : 0;
		int to   = b<NUM_BOXES-1? analyzer[b]  +(analyzer[b+1]-analyzer[b]  )/2 : SPEC_NUM;

		int j;
		for( j = from; j < to; j++ )
		{
			m_freqToBox[j] = b;
		}

		m_boxSampleCount[b] = to-from;
	}
}


void SjOscSpectrum::Calc(const wxSize& clientSize, const unsigned char* bufferStart)
{
	m_clientSize = clientSize;

	// convert to double
	signed short*       bufferSShort = (signed short*)bufferStart;
	long                i, ch;
	kiss_fft_scalar     kiss_in[2][SPEC_NUM*2];

	for( i = 0; i < SPEC_NUM*2; i++ )
	{
		#define  float_to_short ((float)0x7FFF) // Multiplier for making 16-bit integer
		kiss_in[0][i]  = (float)(*bufferSShort++) / float_to_short;
		kiss_in[1][i] = (float)(*bufferSShort++) / float_to_short;
	}

	static const int equalizer[20] = {9,11,12,13,20,23,28,36,52,60,70,90,110,140,140,150,150,150,150,160};
	kiss_fft_cpx kiss_out[SPEC_NUM*2];
	for( ch = 0; ch < 2; ch++ )
	{
		// get spectrum data
		kiss_fftr(m_kiss_fft_setup, kiss_in[ch], kiss_out);

		// calculate all boxes
		double* boxY = m_chData[ch].m_boxY;
		for( i = 0; i < NUM_BOXES; i++ )
		{
			boxY[i] = 0;
		}

		double amplitude = 0.03f;
		for( i = 0; i < SPEC_NUM; i++ )
		{
			kiss_fft_scalar absval = sqrt(static_cast<double>(kiss_out[i].r * kiss_out[i].r)
			                              + static_cast<double>(kiss_out[i].i * kiss_out[i].i));
			double y = amplitude*absval;

			boxY[m_freqToBox[i]] += y;
		}

		for( i = 0; i < NUM_BOXES; i++ )
		{
			boxY[i] /= m_boxSampleCount[i];
			boxY[i] *= ((double)equalizer[i]/100.0F) * 3;
			if( boxY[i] > 1.0 )
			{
				boxY[i] = 1.0;
			}
		}
	}
}


void SjOscSpectrum::DrawBand(wxDC& dc, int x, int y, int w, int h, double val, double crazy)
{
	// make sure, "val" is in range.  normally, there should be no overflows, but if, for any reasons,
	// silverjuke may get very slow as eg. handH gets way too large.
	if( val > 1.0 ) val = 1.0;
	if( val < 0 ) val = 0;

	// draw the band to the given box
	long bandH = (long)((double)h*val);
	int  yy;

	long crazy1curr = 0, crazy2curr = 0;
	if( crazy > 0.0F )
	{
		double crazy1full = (bandH-h/3)*-1,
		       crazy2full = (h-bandH-h/3)*-1;
		crazy1curr = (long) ( crazy1full * crazy );
		crazy2curr = (long) ( crazy2full * crazy );
	}

	for( yy = 0; yy < bandH; yy += 4 )
	{
		dc.DrawLine(x, y+h-yy+crazy1curr, x+w, y+h-yy+crazy2curr);
	}
}


void SjOscSpectrum::Draw(wxDC& dc, SjOscSpectrumChData* chData, bool volumeBeat, bool showFigures, bool forceAnim)
{
	if( forceAnim )
	{
		// force an initial animation
		m_crazyState = CRAZY_FADEOUT;
		m_crazy = CRAZY_MAX;
		m_firstCrazy = TRUE;
		int b;
		if( !g_mainFrame->IsPlaying() )
		{
			for( b = 0; b < NUM_BOXES; b++ )
			{
				m_chData[0].m_boxMax[b] =  (double)(NUM_BOXES-b)/(double)NUM_BOXES;
				m_chData[1].m_boxMax[b] =  (double)(b)/(double)NUM_BOXES;;
			}
		}
	}

	// draw the boxes

	{
		double analyzerW = (double)m_clientSize.x * 0.7;
		long   analyzerX = (long)( (m_clientSize.x-analyzerW)/2 );

		double analyzerH = (double)(m_clientSize.y / 2) * 0.5;
		long   analyzerY = (m_clientSize.y / 16);

		double cellW = (analyzerW / (double)NUM_BOXES);
		double boxW = cellW * 0.8;

		if( m_crazyState == CRAZY_FADEIN )
		{
			m_crazy += CRAZY_INC;
			if( m_crazy > CRAZY_MAX )
			{
				m_crazyState = CRAZY_STAY;
				m_crazySleep = SjTools::Rand(5000/SLEEP_MS);
			}
		}
		else if( m_crazyState == CRAZY_STAY )
		{
			m_crazySleep--;
			if( m_crazySleep <= 0 )
			{
				m_crazyState = CRAZY_FADEOUT;
			}
		}
		else if( m_crazyState == CRAZY_FADEOUT )
		{
			m_crazy -= CRAZY_INC;
			if( m_crazy <= 0 )
			{
				m_crazy = 0.0;
				m_crazyState = CRAZY_NONE;
				if( !m_firstCrazy && SjTools::Rand(3)!=0 )
				{
					m_crazySleep = SjTools::Rand(1000/SLEEP_MS);
				}
				else
				{
					m_crazySleep = (10000/SLEEP_MS) + SjTools::Rand(CRAZY_RAND);
				}
				m_firstCrazy = FALSE;
			}
		}
		else
		{
			m_crazySleep--;
			if( m_crazySleep <= 0 && volumeBeat && showFigures )
			{
				m_crazyState = CRAZY_FADEIN;
			}
		}

		long i;
		for( i = 0; i < NUM_BOXES; i++ )
		{
			double val = chData->m_boxY[i];

			#define FALLOFF_MS 800
			chData->m_boxMax[i] -= 1.0F/((double)FALLOFF_MS/(double)SLEEP_MS);
			if( val > chData->m_boxMax[i] )
			{
				chData->m_boxMax[i] = val;
			}

			DrawBand(dc,
			         (int)( analyzerX + i*cellW ),
			         (int)( analyzerY + m_clientSize.y/2*chData->chNum ),
			         (int)( boxW ),
			         (int)( analyzerH ),
			         chData->m_boxMax[i],
			         m_crazy);
		}
	}
}


SjOscSpectrum::~SjOscSpectrum()
{
	kiss_fftr_free(m_kiss_fft_setup);
}


/*******************************************************************************
 * SjOscTitle
 ******************************************************************************/


class SjOscTitle
{
public:
	SjOscTitle          ();
	void            Draw                (wxDC& dc, const wxSize& clientSize, bool titleChanged, const wxString& newTitle);

private:
	wxSize          m_clientSize;
	wxFont          m_font;
	wxString        m_currTitle;
	wxCoord         m_currTitleX,
	                m_currTitleY,
	                m_currTitleW,
	                m_currTitleH;
	void            CalcCurrTitleSize   (wxDC& dc);
	wxString        m_nextTitle;

	#define         SCROLL_OFF      0
	#define         SCROLL_WAIT     1
	#define         SCROLL_LEFT     2
	#define         SCROLL_RIGHT    3
	long            m_scroll;
	long            m_scrollCnt;

	#define         TITLEOP_NOP     0
	#define         TITLEOP_WIPEIN  1
	#define         TITLEOP_WIPEOUT 2
	int             m_titleOp;
	double          m_titleWipe;

};


SjOscTitle::SjOscTitle()
{
	m_font = wxFont(10, wxSWISS, wxITALIC, wxNORMAL, FALSE, g_mainFrame->GetBaseFontFace());
}


void SjOscTitle::Draw(wxDC& dc, const wxSize& clientSize, bool titleChanged, const wxString& newTitle)
{
	// client size changed?
	if( m_clientSize != clientSize )
	{
		m_clientSize = clientSize;

		// create new font
		int pt = clientSize.y / 20; if( pt < 7 ) pt = 7;
		m_font.SetPointSize(pt);

		// calculate new size
		CalcCurrTitleSize(dc);
	}

	// title changed?
	if( titleChanged )
	{
		if( m_currTitle.IsEmpty() )
		{
			m_currTitle = newTitle;
			CalcCurrTitleSize(dc);
			m_titleOp = TITLEOP_WIPEIN;
			m_titleWipe = 0.0F;
		}
		else if( m_titleOp == TITLEOP_WIPEIN )
		{
			m_nextTitle = newTitle; // fast switch - however, this only happens for very short tracks
			CalcCurrTitleSize(dc);
		}
		else if( newTitle != m_currTitle )
		{
			m_nextTitle = newTitle;
			CalcCurrTitleSize(dc);
			if( m_titleOp == TITLEOP_NOP )
			{
				m_titleOp = TITLEOP_WIPEOUT;
				m_titleWipe = 0.0F;
			}
		}
	}

	// draw the title
	dc.SetFont(m_font);

	// scroll?
	#define SCROLL_PIX 8
	#define SCROLL_INITIAL_WAIT_MS 2000
	#define SCROLL_LEFT_END_WAIT_MS 2000
	#define SCROLL_RIGHT_END_WAIT_MS 12000
	if( m_scroll == SCROLL_WAIT )
	{
		m_scrollCnt--;
		if( m_scrollCnt <= 0 )
		{
			m_scroll = m_currTitleX<0? SCROLL_RIGHT : SCROLL_LEFT;
		}
	}
	else if( m_scroll == SCROLL_LEFT )
	{
		m_currTitleX -= SCROLL_PIX;
		if( m_currTitleX+m_currTitleW <= m_clientSize.x )
		{
			m_scroll = SCROLL_WAIT;
			m_scrollCnt = (SCROLL_LEFT_END_WAIT_MS / SLEEP_MS);
		}
	}
	else if( m_scroll == SCROLL_RIGHT )
	{
		m_currTitleX += SCROLL_PIX;
		if( m_currTitleX >= 0 )
		{
			m_scroll = SCROLL_WAIT;
			m_scrollCnt = (SCROLL_RIGHT_END_WAIT_MS / SLEEP_MS);
		}
	}

	// apply our wipe effect
	if( m_titleOp )
	{
		m_titleWipe += 1.0L/(double)SLEEP_MS;

		wxRect wipeRect(0, m_currTitleY, m_clientSize.x, m_currTitleH);

		if( m_titleOp == TITLEOP_WIPEIN )
		{
			wipeRect.width = (int) ( wipeRect.width * m_titleWipe );
		}
		else
		{
			wipeRect.x = (int) ( wipeRect.width * m_titleWipe );
		}

		// clip out the wipe rectangle and draw the text
		//dc.DrawRectangle(wipeRect);
		dc.SetClippingRegion(wipeRect);
		dc.DrawText(m_currTitle, m_currTitleX, m_currTitleY);
		dc.DestroyClippingRegion();

		// this wipe done?
		if( m_titleWipe >= 1.0 )
		{
			if( m_titleOp == TITLEOP_WIPEOUT )
			{
				m_currTitle = m_nextTitle;
				CalcCurrTitleSize(dc);
				m_titleOp = TITLEOP_WIPEIN;
				m_titleWipe = 0.0F;
			}
			else
			{
				m_titleOp = TITLEOP_NOP;
			}
		}
	}
	else
	{
		// just draw the text, no wiping currently needed
		dc.DrawText(m_currTitle, m_currTitleX, m_currTitleY);
	}
}


void SjOscTitle::CalcCurrTitleSize(wxDC& dc)
{
	dc.SetFont(m_font);
	dc.GetTextExtent(m_currTitle, &m_currTitleW, &m_currTitleH);

	m_currTitleX = m_clientSize.x/2-m_currTitleW/2;
	if( m_currTitleX < 0 )
		m_currTitleX = 0;

	m_currTitleY = m_clientSize.y/2-m_currTitleH/2;

	if( m_currTitleW > m_clientSize.x )
	{
		m_scroll = SCROLL_WAIT;
		m_scrollCnt = (SCROLL_INITIAL_WAIT_MS / SLEEP_MS);
	}
	else
	{
		m_scroll = SCROLL_OFF;
	}

}


/*******************************************************************************
 *  SjOscWindow
 ******************************************************************************/


class SjOscWindow : public wxWindow
{
public:
	                    SjOscWindow         (SjOscModule*, wxWindow* parent);
	                    ~SjOscWindow        ();

private:
	SjOscModule*        m_oscModule;
    wxBitmap            m_offscreenBitmap;
    wxMemoryDC          m_offscreenDc;
    unsigned char*      m_bufferStart;
    unsigned char*      m_bufferTemp;
    #define             BUFFER_MIN_BYTES (576*2*sizeof(float))
    long                m_bufferTotalBytes;
    long                m_bufferValidBytes;
    bool                m_bufferDrawn;
    wxCriticalSection   m_bufferCritical;
    long                m_sampleCount_;
	wxColour            m_textColour;
	wxColour            m_fgColour;
	wxPen               m_fgPen;
	wxBrush             m_bgBrush;
	SjOscSpectrum*      m_spectrum;
	SjOscOscilloscope*  m_oscilloscope;
	SjOscTitle*         m_title;
	SjOscRotor*         m_rotor;
	SjOscHands*         m_hands;
	SjOscFirework*      m_firework;
	SjOscStarfield*     m_starfield;
	wxTimer             m_timer;
	void                ShowFigures         (int show); // -1 = toggle
	void                OnEraseBackground   (wxEraseEvent&)     { /* we won't erease the background explcitly, this is done in the thread */ }
	void                OnPaint             (wxPaintEvent&)     { wxPaintDC dc(this); /* even if we do not paint the window here and do this in the thread, wxPaintDC MUST be constructed for validating the window list! */ }
	bool                ImplOk              () const            { return (m_oscModule&&m_oscModule->m_impl); }
	void                OnKeyDown           (wxKeyEvent& e)     { if(ImplOk()) m_oscModule->m_impl->OnKeyDown(e); }
	void                OnKeyUp             (wxKeyEvent& e)     { if(ImplOk()) m_oscModule->m_impl->OnKeyUp(e); }
	void                OnMouseLeftDown     (wxMouseEvent& e)   { if(ImplOk()) m_oscModule->m_impl->OnMouseLeftDown(e); }
	void                OnMouseLeftUp       (wxMouseEvent& e)   { if(ImplOk()) m_oscModule->m_impl->OnMouseLeftUp(e); }
	void                OnMouseRightUp      (wxContextMenuEvent& e)   { if(ImplOk()) m_oscModule->m_impl->OnMouseRightUp(e); }
	void                OnMouseLeftDClick   (wxMouseEvent& e)   { if(ImplOk()) m_oscModule->m_impl->OnMouseLeftDClick(e); }
	void                OnMouseEnter        (wxMouseEvent& e)   { if(ImplOk()) m_oscModule->m_impl->OnMouseEnter(e); }
	void                OnShowFigures       (wxCommandEvent&)   { ShowFigures(-1); }
	void                OnTimer             (wxTimerEvent&);
	friend class        SjOscModule;
	                    DECLARE_EVENT_TABLE ();
};


#define IDC_SHOWSPECTRUM        (IDO_VIS_OPTIONFIRST+1)
#define IDC_SHOWOSCILLOSCOPE    (IDO_VIS_OPTIONFIRST+2)
#define IDC_SHOWSTARFIELD       (IDO_VIS_OPTIONFIRST+3)
#define IDC_SHOWFIGURES         (IDO_VIS_OPTIONFIRST+4)
#define IDC_TIMER               (IDO_VIS_OPTIONFIRST+7)


BEGIN_EVENT_TABLE(SjOscWindow, wxWindow)
	EVT_ERASE_BACKGROUND(                       SjOscWindow::OnEraseBackground      )
	EVT_PAINT           (                       SjOscWindow::OnPaint                )
	EVT_KEY_DOWN        (                       SjOscWindow::OnKeyDown              )
	EVT_KEY_UP          (                       SjOscWindow::OnKeyUp                )
	EVT_ENTER_WINDOW    (                       SjOscWindow::OnMouseEnter           )
	EVT_LEFT_DOWN       (                       SjOscWindow::OnMouseLeftDown        )
	EVT_LEFT_UP         (                       SjOscWindow::OnMouseLeftUp          )
	EVT_LEFT_DCLICK     (                       SjOscWindow::OnMouseLeftDClick      )
	EVT_CONTEXT_MENU    (                       SjOscWindow::OnMouseRightUp         )
	EVT_TIMER           (IDC_TIMER,             SjOscWindow::OnTimer                )
END_EVENT_TABLE()



SjOscWindow::SjOscWindow(SjOscModule* oscModule, wxWindow* parent)
	: wxWindow( parent, -1, /*oscModule->m_name,*/
	            wxPoint(-1000,-1000), wxSize(100,100),
	            wxNO_BORDER | wxCLIP_CHILDREN ),
	m_offscreenBitmap(16,16)
{
	m_oscModule = oscModule;

	m_bufferTotalBytes = 0;
	m_bufferValidBytes = 0;
	m_bufferDrawn = false;
	m_bufferStart = (unsigned char*)malloc(BUFFER_MIN_BYTES);
	m_bufferTemp = (unsigned char*)malloc(BUFFER_MIN_BYTES);
	if( m_bufferStart && m_bufferTemp ) {
		m_bufferTotalBytes = BUFFER_MIN_BYTES;
	}

	// set colors
	m_textColour = wxColour(0x2F, 0x60, 0xA3);
	m_fgColour = wxColour(0x3D, 0x80, 0xDF);
	m_fgPen = wxPen(m_fgColour, 1, wxSOLID);

	// create the drawing objects
	m_spectrum = new SjOscSpectrum();
	m_oscilloscope = new SjOscOscilloscope(BUFFER_MIN_BYTES/sizeof(float));
	m_title = new SjOscTitle();
	m_rotor = new SjOscRotor();
	m_hands = new SjOscHands();
	m_firework = new SjOscFirework();
	m_starfield = new SjOscStarfield();

	// finally, start the timer
	m_timer.SetOwner(this, IDC_TIMER);
	m_timer.Start(SLEEP_MS);
}


SjOscWindow::~SjOscWindow()
{
	m_timer.Stop();
	if( m_spectrum )     { delete m_spectrum; }
	if( m_oscilloscope ) { delete m_oscilloscope; }
	if( m_title )        { delete m_title; }
	if( m_rotor )        { delete m_rotor; }
	if( m_hands )        { delete m_hands; }
	if( m_firework )     { delete m_firework; }
	if( m_starfield )    { delete m_starfield; }
	if( m_bufferStart )  { free(m_bufferStart); }
	if( m_bufferTemp )   { free(m_bufferTemp); }
	m_oscModule = NULL;
}


void SjOscWindow::OnTimer(wxTimerEvent&)
{
	SJ_FORCE_IN_HERE_ONLY_ONCE;

	if( m_oscModule )
	{
		long                validBytes;

		// volume stuff
		long                volume, maxVolume = 1;
		bool                volumeBeat;

		// other objects
		long                i;
		bool                titleChanged, forceOscAnim, forceSpectrAnim;
		wxString            newTitle;

		// get data
		if( m_bufferStart==NULL || m_bufferTemp == NULL ) return;
		m_bufferCritical.Enter();
			validBytes = m_bufferValidBytes;
			m_bufferDrawn = true;
			if( validBytes > 0 ) {
				memcpy(m_bufferTemp, m_bufferStart, validBytes);
			}
			else {
				validBytes = BUFFER_MIN_BYTES;
				memset(m_bufferTemp, 0, validBytes);
			}
		m_bufferCritical.Leave();

		// get window client size, correct offscreen DC if needed
		wxSize clientSize = m_oscModule->m_oscWindow->GetClientSize();
		if( clientSize.x != m_offscreenBitmap.GetWidth() || clientSize.y != m_offscreenBitmap.GetHeight() )
		{
			m_offscreenBitmap.Create(clientSize.x, clientSize.y);
			m_offscreenDc.SelectObject(m_offscreenBitmap);
		}

		// calculate the points for the lines, collect volume
		m_oscilloscope->Calc(clientSize, m_bufferTemp, volume);
		if( m_oscModule->m_showFlags&SJ_OSC_SHOW_SPECTRUM )
		{
			m_spectrum->Calc(clientSize, m_bufferTemp);
		}

		// get data that are shared between the threads
		{
			titleChanged = m_oscModule->m_titleChanged;
			m_oscModule->m_titleChanged = FALSE;
			if( titleChanged )
			{
				newTitle = m_oscModule->m_trackName;
				if( newTitle.IsEmpty() )
				{
					newTitle = SJ_PROGRAM_NAME;
				}
				else if( !m_oscModule->m_leadArtistName.IsEmpty() )
				{
					newTitle.Prepend(m_oscModule->m_leadArtistName + wxT(" - "));
				}
			}

			forceOscAnim = m_oscModule->m_forceOscAnim;
			m_oscModule->m_forceOscAnim = FALSE;

			forceSpectrAnim = m_oscModule->m_forceSpectrAnim;
			m_oscModule->m_forceSpectrAnim = FALSE;
		}

		// calculate volume, volume is theoretically max. 255, normally lesser
		if( titleChanged )              { maxVolume = 1;        }
		if( volume > maxVolume )        { maxVolume = volume;   }

		volumeBeat = (volume > maxVolume/2);

		// erase screen
		m_offscreenDc.SetPen(*wxTRANSPARENT_PEN);
		{
			// blue gradient background
			#define BG_STEPS 88
			int rowH = (clientSize.y/BG_STEPS)+1;
			for( i = 0; i < BG_STEPS; i++ )
			{
				m_bgBrush.SetColour(0, 0, i);
				m_offscreenDc.SetBrush(m_bgBrush);
				m_offscreenDc.DrawRectangle(0, i*rowH, clientSize.x, rowH);
			}
		}

		// draw text (very background)
		{
			m_offscreenDc.SetBackgroundMode(wxTRANSPARENT);
			m_offscreenDc.SetTextForeground(m_textColour);
			m_title->Draw(m_offscreenDc, clientSize, titleChanged, newTitle);
		}

		// draw figures (they lay in backgroud)
		if( m_oscModule->m_showFlags&SJ_OSC_SHOW_FIGURES )
		{
			long bgLight = 100;

			// draw hands (optional)
			m_hands->Draw(m_offscreenDc, clientSize, volume, bgLight, titleChanged);

			// draw rotor (optional)
			m_rotor->Draw(m_offscreenDc, clientSize, m_starfield->IsRunning(), titleChanged, volume, bgLight);

			// draw firework (optional)
			m_firework->Draw(m_offscreenDc, clientSize, titleChanged, m_starfield->IsRunning(), volumeBeat, bgLight);
		}

		// draw starfield (optional)
		m_starfield->Draw(m_offscreenDc, clientSize, false, titleChanged, (m_oscModule->m_showFlags&SJ_OSC_SHOW_STARFIELD)!=0);

		// draw spectrum and/or oscilloscope (for both, the spectrum lays over the oscillosope)
		m_offscreenDc.SetPen(m_fgPen);

		if( m_oscModule->m_showFlags&SJ_OSC_SHOW_OSC )
		{
			m_oscilloscope->Draw(m_offscreenDc, forceOscAnim);
		}

		if( m_oscModule->m_showFlags&SJ_OSC_SHOW_SPECTRUM )
		{
			m_spectrum->Draw(m_offscreenDc, volumeBeat,
						  (m_oscModule->m_showFlags&SJ_OSC_SHOW_FIGURES)? true : false,
						  forceSpectrAnim);
		}

		// draw offscreen bitmap to screen
		wxClientDC dc(this);
		dc.Blit(0, 0, m_offscreenBitmap.GetWidth(), m_offscreenBitmap.GetHeight(), &m_offscreenDc, 0, 0);
	}
}


/*******************************************************************************
 * SjOscModule
 ******************************************************************************/


SjOscModule::SjOscModule(SjInterfaceBase* interf)
	: SjVisRendererModule(interf)
{
	m_file          = wxT("memory:oscilloscope.lib");
	m_name          = _("Spectrum Monitor");
	m_oscWindow     = NULL;
	m_impl          = NULL;
	m_sort          = 3001; // very end of list, default is 1000
}


bool SjOscModule::Start(SjVisWindow* impl)
{
	if( m_oscWindow == NULL )
	{
		m_impl = impl;

		m_showFlags		= g_tools->m_config->Read(wxT("player/oscflags"), SJ_OSC_SHOW_DEFAULT);
		m_oscWindow = new SjOscWindow(this, impl);

		if( m_oscWindow  )
		{
			ReceiveMsg(IDMODMSG_TRACK_ON_AIR_CHANGED);

			m_forceSpectrAnim= true;
			m_forceOscAnim   = true;

			wxRect visRect = impl->GetRendererClientRect();
			m_oscWindow->SetSize(visRect);

			m_oscWindow->Show();

			return TRUE;
		}
	}

	return FALSE;
}


void SjOscModule::Stop()
{
	if( m_oscWindow )
	{
		m_oscWindow->m_oscModule = NULL;
		m_oscWindow->Hide();
		g_mainFrame->Update();
		m_oscWindow->Destroy();
		m_oscWindow = NULL;
	}

	m_impl = NULL;
}


void SjOscModule::ReceiveMsg(int msg)
{
	if( msg == IDMODMSG_TRACK_ON_AIR_CHANGED )
	{
		m_titleChanged = TRUE;
		SjPlaylistEntry& urlInfo = g_mainFrame->GetQueueInfo(-1);
		m_trackName = urlInfo.GetTrackName();
		m_leadArtistName = urlInfo.GetLeadArtistName();
	}
}


void SjOscModule::PleaseUpdateSize(SjVisWindow* impl)
{
	wxRect visRect = impl->GetRendererClientRect();
	m_oscWindow->SetSize(visRect);
}


void SjOscModule::AddMenuOptions(SjMenu& m)
{
	m.AppendCheckItem(IDC_SHOWSPECTRUM, _("Show spectrum"));
	m.Check(IDC_SHOWSPECTRUM, (m_showFlags&SJ_OSC_SHOW_SPECTRUM)!=0);

	m.AppendCheckItem(IDC_SHOWOSCILLOSCOPE, _("Show oscilloscope"));
	m.Check(IDC_SHOWOSCILLOSCOPE, (m_showFlags&SJ_OSC_SHOW_OSC)!=0);

	m.AppendCheckItem(IDC_SHOWSTARFIELD, _("Show starfield"));
	m.Check(IDC_SHOWSTARFIELD, (m_showFlags&SJ_OSC_SHOW_STARFIELD)!=0);

	m.AppendCheckItem(IDC_SHOWFIGURES, _("Show other figures"));
	m.Check(IDC_SHOWFIGURES, (m_showFlags&SJ_OSC_SHOW_FIGURES)!=0);
}


void SjOscModule::OnMenuOption(int i)
{
	switch( i )
	{
		case IDC_SHOWSPECTRUM:
			SjTools::ToggleFlag(m_showFlags, SJ_OSC_SHOW_SPECTRUM);
			if( m_showFlags&SJ_OSC_SHOW_SPECTRUM ) m_forceSpectrAnim = TRUE;
			break;

		case IDC_SHOWOSCILLOSCOPE:
			SjTools::ToggleFlag(m_showFlags, SJ_OSC_SHOW_OSC);
			if( m_showFlags&SJ_OSC_SHOW_OSC ) m_forceOscAnim = TRUE;
			break;

		case IDC_SHOWSTARFIELD:
			SjTools::ToggleFlag(m_showFlags, SJ_OSC_SHOW_STARFIELD);
			break;

		case IDC_SHOWFIGURES:
			SjTools::ToggleFlag(m_showFlags, SJ_OSC_SHOW_FIGURES);
			break;
	}

	g_tools->m_config->Write(wxT("player/oscflags"), m_showFlags);
}



void SjOscModule::AddVisData(const float* src, long bytes)
{
	if( m_oscWindow && m_oscWindow->m_bufferStart && bytes )
	{
		m_oscWindow->m_bufferCritical.Enter();

			// resize the buffer, if needed
			if( m_oscWindow->m_bufferDrawn ) {
				m_oscWindow->m_bufferValidBytes = 0;
				m_oscWindow->m_bufferDrawn = false;
			}
			long bytesWanted = m_oscWindow->m_bufferValidBytes + bytes / 2/*use use short, not float*/;
			if( bytesWanted > m_oscWindow->m_bufferTotalBytes )
			{
                m_oscWindow->m_bufferStart = (unsigned char*)realloc(m_oscWindow->m_bufferStart, bytesWanted);
                m_oscWindow->m_bufferTemp  = (unsigned char*)realloc(m_oscWindow->m_bufferTemp,  bytesWanted);
                if( m_oscWindow->m_bufferStart == NULL || m_oscWindow->m_bufferTemp == NULL ) { return; }
			}

			// copy the buffer, convert float to singed shorts
			long s, numSamples = bytes/4;
			signed short* dest =  (signed short*)(&m_oscWindow->m_bufferStart[m_oscWindow->m_bufferValidBytes]);
			float sample;
			for( s = 0; s < numSamples; s++ )
			{
				sample = src[s] * float_to_short;
				if( sample < -32768 ) sample = -32768;
				if( sample >  32767 ) sample =  32767;
				dest[s] = sample;
			}

			m_oscWindow->m_bufferValidBytes += bytes / 2;

		m_oscWindow->m_bufferCritical.Leave();
	}
}

