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
 * File:    wavework.cpp
 * Authors: Björn Petersen
 * Purpose: Working with waves
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/backend.h>


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


void SjApplyVolume(float* buffer, long bytes, float gain)
{
	const float* bufferEnd = buffer + (bytes/sizeof(float));
	while( buffer < bufferEnd ) {
		*buffer++ *= gain;
	}
}


/*******************************************************************************
 * Detect Silence
 ******************************************************************************/


bool SjDetectSilence(SjBackend* backend, const wxString& url, long& silenceBegMs, long& silenceEndMs)
{
	silenceBegMs = -1;
	silenceEndMs = -1;
	return true;
}
