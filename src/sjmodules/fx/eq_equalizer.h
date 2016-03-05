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
 * File:    eq_equalizer.h
 * Authors: Björn Petersen
 * Purpose: Call the SuperEQ
 *
 ******************************************************************************/


#ifndef __SJ_EQUALIZER_H__
#define __SJ_EQUALIZER_H__


class SjSuperEQ;


class SjEqualizer
{
public:
				    SjEqualizer         ();
				    ~SjEqualizer        ();
	void            SetParam            (const bool enable, const SjEqParam&);
	void            AdjustBuffer        (float* data, long bytes, int samplerate, int channels);

private:
	#define         SJ_EQ_MAX_CHANNELS  64 // we define a maximum just for easier allocation, only wastes 4-8 Byte per unsued channel ...
	SjSuperEQ*      m_superEq;
	bool            m_enabled;
	SjEqParam       m_currParam;
	bool            m_currParamChanged;
	int             m_currSamplerate;
	int             m_currChannels;

	float*          m_deinterlaceBuf;
	int             m_deinterlaceBufBytes;

	wxCriticalSection m_paramCritical;
};


#endif // __SJ_EQUALIZER_H__
