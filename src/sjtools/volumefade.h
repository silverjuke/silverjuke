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
 * File:    volumefade.h
 * Authors: Björn Petersen
 * Purpose: Fading Buffers
 *
 ******************************************************************************/


#ifndef __SJ_VOLUMEFADE_H__
#define __SJ_VOLUMEFADE_H__


class SjVolumeFade
{
public:
	                  SjVolumeFade        ();
	void              SetVolume           (float gain);
	void              SlideVolume         (float gain, long ms);
	void              AdjustBuffer        (float* buffer, long bytes, int freq, int channels);

private:
	wxCriticalSection m_critical;

	float             m_startGain;
	float             m_destGain;

	long              m_msToSlide;
	long              m_subsamsToSlide;     // subsam = on channel of a multi-channel-sample; it has always sizeof(float) bytes
	long              m_subsamsPos;
};


#endif // __SJ_VOLUMEFADE_H__

