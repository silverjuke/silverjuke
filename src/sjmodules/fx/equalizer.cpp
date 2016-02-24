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
 * File:    equalizer.cpp
 * Authors: Björn Petersen
 * Purpose: Equalizer
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/fx/equalizer.h>
/*
#include <supereq/pfc/int_types_condensed.h>
#include <supereq/pfc/traits.h>
#include <supereq/pfc/primitives.h>
#include <supereq/pfc/alloc.h>
#include <supereq/pfc/array.h>
#include <supereq/eq_paramlist.h>
#include <supereq/eq_supereq.h>
*/
/*
#include <supereq/Fftsg_fl.cpp>
#include <supereq/Equ.cpp>
*/

SjEqualizer::SjEqualizer()
{
	m_eqsCount            = 0;
	m_currSamplerate      = 0;
	//m_paramroot           = new paramlist();
	m_deinterlaceBuf      = NULL;
	m_deinterlaceBufBytes = 0;

	for( int b = 0; b < SJ_EQ_BANDS; b++ )
	{
		int db = 0; // -20..0..20
		m_bands[b] = SjDecibel2Gain(db);
	}
}


SjEqualizer::~SjEqualizer()
{
	delete_all();
	//delete m_paramroot;
	if( m_deinterlaceBuf ) free(m_deinterlaceBuf);
}


void SjEqualizer::delete_all()
{
	for( int c = 0; c < m_eqsCount; c++ ) {
		//delete m_eqs[c];
	}
	m_eqsCount = 0;
}


void SjEqualizer::AdjustBuffer(float* buffer, long bytes, int samplerate, int channels)
{
	if( buffer == NULL || bytes <= 0 || samplerate <= 0 || channels <= 0 || channels > SJ_EQ_MAX_CHANNELS ) return; // error
}


/*
void SjEqualizer::AdjustBuffer(float* buffer, long bytes, int samplerate, int channels)
{
	int c, samples_out;

	if( buffer == NULL || bytes <= 0 || samplerate <= 0 || channels <= 0 || channels > SJ_EQ_MAX_CHANNELS ) return; // error

	// (re)create the equalizer objects, one per channel
	if( samplerate != m_currSamplerate || channels != m_eqsCount )
	{
		samples_out = 0;
		for( c = 0; c < m_eqsCount; c++ )
		{
			m_eqs[c]->write_samples(0, 0);     // flush any pending data
			m_eqs[c]->get_output(&samples_out);
		}

		delete_all();                          // delete all equalizer objects
		wxASSERT( m_eqsCount == 0 );

		for( c = 0; c < channels; c++ )        // create new equalizer objects
		{
			m_eqs[c] = new supereq<float>(8);
			if( m_eqs[c] == NULL ) { return; } // error

			m_eqs[c]->equ_makeTable(m_bands, m_paramroot, (double)samplerate);
		}

		m_eqsCount = channels;
		m_currSamplerate = samplerate;
	}
	#if 0
	else if( <band sliders modified> )
	{
		m_eqs[c]->equ_makeTable(<new gain parameters>, m_paramroot, (double)samplerate);
	}
	#endif

	// (re)allocate the temporary buffer for deinterlacing
	if( bytes > m_deinterlaceBufBytes )
	{
		if( m_deinterlaceBuf ) {
			free(m_deinterlaceBuf);
			m_deinterlaceBuf = NULL;
		}
		m_deinterlaceBuf = (float*)malloc(bytes);
		if( m_deinterlaceBuf == NULL ) { return; } // error;
		m_deinterlaceBufBytes = bytes;
	}

	// process the samples
	const float *srcPtr, *bufferEnd = buffer + (bytes/sizeof(float));
	float *destPtr;
	for( c=0; c < channels; c++ )
	{
		// deinterlace from `buffer` to `m_deinterlaceBuf`
		srcPtr  = buffer + c;
		destPtr =  m_deinterlaceBuf;
		while( srcPtr < bufferEnd )  {
			*destPtr++ = *srcPtr;
			srcPtr += channels;
		}

		// call equalizer
		m_eqs[c]->write_samples(m_deinterlaceBuf, bytes/sizeof(float));

		samples_out = 0;
		srcPtr = m_eqs[c]->get_output(&samples_out);

		// interlace equalized data from `m_deinterlaceBuf` back to buffer
		if( samples_out == (int)(bytes/sizeof(float)) )
		{
			destPtr = buffer + c;
			while( destPtr < bufferEnd )  {
				*destPtr = *srcPtr++;
				destPtr += channels;
			}
		}
	}
	#if 0
	<eq_write_chunk(p_chunk); eq_read_chunk(p_chunk);>
	#endif
}
*/

