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
 * File:    wavework.h
 * Authors: Björn Petersen
 * Purpose: Working with waves
 *
 ******************************************************************************/


#ifndef __SJ_WAVEWORK_H__
#define __SJ_WAVEWORK_H__


class SjBackend;


double  SjGain2Decibel      (double gain);
double  SjDecibel2Gain      (double dB);
long    SjGain2Long         (double gain);
double  SjLong2Gain         (long lng);
void    SjApplyVolume       (float* buffer, long bytes, float gain);
void    SjMixdownChannels   (float* buffer, long bytes, int channels, int destCh);

void    SjDetectSilence     (SjBackend*, const wxString& url, long& silenceBegMs, long& silenceEndMs);


#endif // __SJ_WAVEWORK_H__

