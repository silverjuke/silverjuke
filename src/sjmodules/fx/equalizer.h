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
 * File:    equalizer.h
 * Authors: Björn Petersen
 * Purpose: Equalizer
 *
 ******************************************************************************/


#ifndef __SJ_EQUALIZER_H__
#define __SJ_EQUALIZER_H__


/*
class supereq_base;
class paramlist;
*/


#define SJ_EQ_BANDS        18 // must not be modified!
#define SJ_EQ_MAX_CHANNELS 64


class SjEqualizer
{
public:
				    SjEqualizer         ();
				    ~SjEqualizer        ();
	void            AdjustBuffer        (float* data, long bytes, int samplerate, int channels);

private:
	//supereq_base*   m_eqs[SJ_EQ_MAX_CHANNELS];
	int             m_eqsCount;

	//paramlist*      m_paramroot;

	double          m_bands[SJ_EQ_BANDS];

	int             m_currSamplerate;

	float*          m_deinterlaceBuf;
	int             m_deinterlaceBufBytes;

	void            delete_all          ();
};


#endif // __SJ_EQUALIZER_H__
