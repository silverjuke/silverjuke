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
 * File:    volumecalc.h
 * Authors: Björn Petersen
 * Purpose: Working with waves
 *
 ******************************************************************************/



#ifndef __SJ_VOLUMECALC_H__
#define __SJ_VOLUMECALC_H__


class SjVolumeCalc
{
public:
	// constructor
	SjVolumeCalc        ();

	// optionally you can set a loaded, precalculated gain;
	// the gain applied to AdjustBuffer will never be larger
	// than this value then.
	void            SetPrecalculatedGain(float gain) { m_precalculatedGain = gain; }

	// init the number of channels; this is also done automatically
	// on the first call to AddBuffer(), however, Init() must not be called
	// after Init() has been called.
	void            Init                (int freq, int channels);

	// add a sample buffer
	void            AddBuffer           (const float* data, long bytes, int freq, int channels);

	// apply the calculated gain to a buffer
	void            AdjustBuffer        (float* data, long bytes, float desiredGain, float maxGain);

	// get the calculated information
	float           GetGain             () const { return m_gain; };
	bool            IsGainWorthSaving   () const { return (m_maxLevel>0 /*at least the first calculation is done*/); }

private:
	// private stuff
	bool            m_isInitialized;

	float           m_precalculatedGain;

	double          m_gain;
	double          m_maxLevel;
	double          m_sums[SJ_WW_MAX_CH];

#define MAX_GAIN        5.0F    // max. 500 % gain

#define SMOOTH_SIZE     100     // we take max 100 probes per second...
	// so does xmms volnorm - little more gain;
	// vorbisgain and mp3gain use only 20


	double          m_smooth[SJ_WW_MAX_CH][SMOOTH_SIZE];
	int             m_smoothN[SJ_WW_MAX_CH],
	                m_smoothAdd;
	double          GetSmoothedData         (const double* s, int n);
};



double  SjGain2Decibel      (double gain);
double  SjDecibel2Gain      (double dB);
long    SjGain2Long         (double gain);
double  SjLong2Gain         (long lng);




#endif // __SJ_VOLUMECALC_H__

