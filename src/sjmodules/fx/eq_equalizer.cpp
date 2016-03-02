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
 * File:    eq_equalizer.cpp
 * Authors: Björn Petersen
 * Purpose: Call the SuperEQ
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/fx/eq_equalizer.h>
#include <supereq/Fftsg_fl.cpp>


SjEqualizer::SjEqualizer()
{
	m_currParamChanged    = true;
	m_currChannels        = 0;
	m_currSamplerate      = 0;
	m_deinterlaceBuf      = NULL;
	m_deinterlaceBufBytes = 0;

	for( int b = 0; b < SJ_EQ_BANDS; b++ )
	{
		//int db = 0; // -20..0..20
		//m_bands[b] = SjDecibel2Gain(db);
	}
}


SjEqualizer::~SjEqualizer()
{
	delete_all();
	if( m_deinterlaceBuf ) free(m_deinterlaceBuf);
}


void SjEqualizer::delete_all()
{
	for( int c = 0; c < m_currChannels; c++ ) {
		//delete m_eqs[c];
	}
	m_currChannels = 0;
}


void SjEqualizer::SetParam(const SjEqParam& newParam)
{
	m_paramCritical.Enter();
		m_currParam        = newParam;
		m_currParamChanged = true;
	m_paramCritical.Leave();
}


void SjEqualizer::AdjustBuffer(float* buffer, long bytes, int samplerate, int channels)
{
	if( buffer == NULL || bytes <= 0 || samplerate <= 0 || channels <= 0 || channels > SJ_EQ_MAX_CHANNELS ) return; // error

	m_paramCritical.Enter();
		if( m_currParamChanged )
		{
			// realize the new parameters
			m_currParamChanged = false;
		}
	m_paramCritical.Leave();

}

