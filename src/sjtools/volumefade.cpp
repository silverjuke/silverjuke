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
 * File:    volumefade.cpp
 * Authors: Björn Petersen
 * Purpose: Fading Buffers
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/volumefade.h>


#define NEEDS_RECALCULATION -1


SjVolumeFade::SjVolumeFade()
{
	SetVolume(1.0);
}


void SjVolumeFade::SetVolume(float newGain)
{
	m_critical.Enter();

		m_startGain         = newGain;
		m_destGain          = newGain;

		m_msToSlide         = 0;
		m_subsamsToSlide    = 0;
		m_subsamsPos        = 0;

	m_critical.Leave();
}


void SjVolumeFade::SlideVolume(float newGain, long ms)
{
	m_critical.Enter();

		m_startGain         = m_destGain;
		m_destGain          = newGain;

		m_msToSlide         = ms;
		m_subsamsToSlide    = NEEDS_RECALCULATION; // we calculate here - freq/channels is unknown
		m_subsamsPos        = 0;

	m_critical.Leave();
}


bool SjVolumeFade::AdjustBuffer(float* buffer, long bufferBytes, int freq, int channels)
{
	long bufferSubsams = bufferBytes/sizeof(float);
	bool sthAdjusted = false;

	m_critical.Enter();

		if( m_subsamsToSlide )
		{
			sthAdjusted = true;

			// calculate the total number of subsams to slide
			if( m_subsamsToSlide == NEEDS_RECALCULATION )
			{
				m_subsamsToSlide = (long)( (float)(m_msToSlide * channels * freq) / (float)1000 );
			}

			// calculate the number of subsams that can be slided now
			long subsamsToSlideNow = m_subsamsToSlide - m_subsamsPos;
			if( subsamsToSlideNow > bufferSubsams ) {
				subsamsToSlideNow = bufferSubsams;
			}

			// go through all subsams
			long  subsamsPos = m_subsamsPos, i;
			float subsamsToSlideF = (float)m_subsamsToSlide;
			float gainDiff = m_destGain-m_startGain; // is a value between -1..0 or 0..1
			float sliderPos;
			float startGain = m_startGain;
			for( i = 0; i < subsamsToSlideNow; i++ )
			{
				sliderPos = (float)(subsamsPos+i) / subsamsToSlideF; // this is a value between 0..1 now, reflecting the current fading position
				buffer[i] *= startGain + gainDiff*sliderPos;
				//if( i==0 || i%500==0 ) {
				//	wxLogDebug("pos: %f -- gain: %f", gainPos, startGain + gainDiff*gainPos);
				//}
			}

			// correct the given buffer
			buffer += subsamsToSlideNow;
			bufferSubsams -= subsamsToSlideNow;

			// done with sliding?
			m_subsamsPos += subsamsToSlideNow;
			if( m_subsamsPos >= m_subsamsToSlide ) {
				m_subsamsToSlide = 0;
			}
		}

		// set rest subsams to resulting gain (we can leave the critical section before adjusting the buffer)
		float gain = m_destGain;

	m_critical.Leave();

	if( gain != 1.0 )
	{
		const float* bufferEnd = buffer + bufferSubsams;
		while( buffer < bufferEnd )
		{
			*buffer++ *= gain;
		}
	}

	return sthAdjusted;
}


