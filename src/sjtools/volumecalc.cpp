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
 * File:    volumecalc.cpp
 * Authors: Björn Petersen
 * Purpose: Working with waves
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/volumecalc.h>
#include <math.h>


/*******************************************************************************
 * SjVolumeCalc
 ******************************************************************************/


// do _not_ change this value as all gain calculations are based upon it!!
#define SJ_AV_DEF_LEVEL 0.25F


SjVolumeCalc::SjVolumeCalc()
{
	m_gain          = 1.0F;
	m_maxLevel      = -1.0F;
	m_isInitialized = FALSE;
}


void SjVolumeCalc::Init(int freq, int channels)
{
	wxASSERT( channels <= SJ_WW_MAX_CH );

	m_smoothAdd = freq / SMOOTH_SIZE;
	for( int c = 0; c < channels; c++ )
	{
		m_sums[c]   = 0.0F;
		m_smoothN[c]= 0;
	}
}


double SjVolumeCalc::GetSmoothedData(const double* s, int n)
{
	int i;
	double smoothed;

	smoothed = 0.0F;
	for( i = 0; i < n; i++ )
	{
		smoothed += s[i];
	}

	smoothed = smoothed / n;

	return smoothed;
}


void SjVolumeCalc::AddBuffer(const float* data, long bytes, int freq, int channels)
{
	const float*        dataEnd = data + bytes/sizeof(float);
	double              sample, level;
	int                 c;
	bool                smoothNModified = FALSE;

	if( channels > SJ_WW_MAX_CH )
	{
		return;
	}

	if( !m_isInitialized )
	{
		// we cannot do the initialisation on construction as the number of channels is not known there
		Init(freq, channels);
		m_isInitialized = TRUE;
	}

	while( data < dataEnd )
	{
		// sum the samples
		for( c = 0; c < channels; c++ )
		{
			sample = ( *data++ );

			// sometimes BASS may give me data widely out of range, see http://www.silverjuke.net/forum/viewtopic.php?t=1007
			// never trust incoming data
#define TOLERANCE 1.4
			if( sample < TOLERANCE*-1 ) sample = TOLERANCE*-1;
			if( sample > TOLERANCE    ) sample = TOLERANCE;

			sample *= 32767.0F;

			m_sums[c] += ( sample*sample );
		}

		m_smoothAdd--;
		if( m_smoothAdd == 0 )
		{
			// calculate the power for this slice (normally 1/100 ... 1/20 second)
			static const double NORMAL = 1.0/( (double)SHRT_MAX );
			static const double NORMAL_SQUARED = NORMAL*NORMAL;
			double channel_length = 2.0/((freq / SMOOTH_SIZE)*2/*we use shorts here!*/ *channels);

			for( c = 0; c < channels; c++ )
			{
				level = m_sums[c] * channel_length * NORMAL_SQUARED;

				level = sqrt(level);

				m_smooth[c][m_smoothN[c]] = level;
				m_smoothN[c]++;

				if( m_smoothN[c] == SMOOTH_SIZE )
				{
					m_smoothN[c] = 0;

					level = GetSmoothedData(m_smooth[c], SMOOTH_SIZE);

					if( level > m_maxLevel )
					{
						m_maxLevel = level;
						m_gain = ((double)SJ_AV_DEF_LEVEL) / m_maxLevel;
					}
				}

				// prepare for the next time slice
				m_sums[c] = 0.0F;
			}

			m_smoothAdd = freq / SMOOTH_SIZE;
			smoothNModified = TRUE;
		}
	}

	if( m_maxLevel < 0 && smoothNModified )
	{
		// calculate gain for the first slices
		double maxLevel = -1.0F;
		for( c = 0; c < channels; c++ )
		{
			level = GetSmoothedData(m_smooth[c], m_smoothN[c]);
			if( level > maxLevel )
			{
				maxLevel = level;
			}
		}

		m_gain = ((double)SJ_AV_DEF_LEVEL) / maxLevel;
	}
}


void SjVolumeCalc::AdjustBuffer(float* data, long bytes, float desiredGain, float maxGain)
{
	// first, assume the calculated gain
	float gain = m_gain;

	// compare against the precalculated gain
	if( m_precalculatedGain > 0.0F )
	{
		// we have a precalculated gain, make sure, the current gain is not larger,
		// the max. gain stays as it is.
		if( gain > m_precalculatedGain )
			gain = m_precalculatedGain;
	}
	else
	{
		// we do not have a precalculated gain, limit the max gain;
		// this is to avoid tracks starting MUCH too loud if the track is completely unknown
		// (the value comes from my quietest album "TOTO IV")
#define MAX_UNKNOWN_GAIN 3.1F
		if( maxGain > MAX_UNKNOWN_GAIN )
			maxGain = MAX_UNKNOWN_GAIN;
	}

	// apply with the desired volume (normally just 1.0)
	gain *= desiredGain;

	// cut too hard gains
	if( gain > maxGain )
		gain = maxGain;

#define MIN_GAIN 0.5F
	if( gain < MIN_GAIN )
		gain = MIN_GAIN;

	// apply all this to all samples
	const float* dataEnd = data + bytes/sizeof(float);
	while( data < dataEnd )
	{
		*data++ *= gain;
	}
}


/*******************************************************************************
 * Misc.
 ******************************************************************************/


double SjDecibel2Gain(double db)
{
	// for some common parameters, we're using predefined
	// return values to avoid rounding errors
	if( db == 14.00 )
	{
		return 5.0;
	}
	else if( db == 12.00 )
	{
		return 4.0;
	}
	else
	{
		return pow(10.0, db / 20.0);
	}
}


double SjGain2Decibel(double gain)
{
	// for some common parameters, we're using predefined
	// return values to avoid rounding errors
	if( gain == 5.0 )
	{
		return 14.0;
	}
	else if( gain > 0.0 )
	{
		return (20.0 * log10(gain));
	}
	else
	{
		return 0.0;
	}
}


long SjGain2Long(double gain)
{
	if( gain > 0 )
	{
		return (long)(gain*1000.0F);
	}
	else
	{
		return 0;
	}
}


double SjLong2Gain(long lng)
{
	return (double) (((double)lng) / 1000.0F);
}


/* -- not needed at the moment, from Silverjuke 1.x
long SjGetSilentBytes(const unsigned char* buffer, long bytes, bool onEnd, signed short threshold)
{
    #define SILENCE_THRESHOLD_ON_BEG 128    // i used 128 as threshold for begin _and_ end in versions before 0.28;
    #define SILENCE_THRESHOLD_ON_END 256    // however, i do not really know what is best

    wxASSERT( buffer && bytes > 0 );
    wxASSERT( bytes % (SJ_WW_BYTERES*SJ_WW_CH) == 0 );

    signed short    sampleL, sampleR;
    long            samples = bytes / 4;
    long            silentBytes = 0;
    long            incr;

    if( bytes <= 0 )
    {
        return 0; // this should not happend, assert above
    }

    if( onEnd )
    {
        buffer = &buffer[(samples-1)*4];
        incr = -4;
    }
    else
    {
        incr = 4;
    }

    while ( samples-- )
    {
        sampleL = ((buffer[1])<<8|buffer[0]);
        sampleR = ((buffer[3])<<8|buffer[2]);

        if( sampleL >  threshold
         || sampleL < -threshold
         || sampleR >  threshold
         || sampleR < -threshold )
        {
            break;
        }

        buffer += incr;
        silentBytes += 4;
    }

    return silentBytes;
}
*/
