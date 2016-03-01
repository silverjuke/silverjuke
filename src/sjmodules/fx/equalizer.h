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


class SjEqParam
{
public:
	#define         SJ_EQ_BANDS         18  // must not be modified!

	#define         SJ_EQ_BAND_MIN      -20.0F
	#define         SJ_EQ_BAND_NULL       0.0F
	#define         SJ_EQ_BAND_MAX       20.0F

	                SjEqParam           () { for(int i=0; i<SJ_EQ_BANDS; i++) { m_bandDb[i] = SJ_EQ_BAND_NULL; } }
	                SjEqParam           (const SjEqParam& o) { CopyFrom(o); }
	void            CopyFrom            (const SjEqParam& o) { for(int i=0; i<SJ_EQ_BANDS; i++) { m_bandDb[i] = o.m_bandDb[i]; } }
	SjEqParam&      operator =          (const SjEqParam& o) { CopyFrom(o); return *this; }

	float           m_bandDb[SJ_EQ_BANDS]; // -20..0..20 dB
	static const char* s_bandNames[SJ_EQ_BANDS];

	wxString        ToString            (const wxString& sep=";") const;
	void            FromString          (const wxString&);
};


class SjEqualizer
{
public:
				    SjEqualizer         ();
				    ~SjEqualizer        ();
	void            SetParam            (const SjEqParam&);
	void            AdjustBuffer        (float* data, long bytes, int samplerate, int channels);

private:
	#define         SJ_EQ_MAX_CHANNELS  64 // we define a maximum just for easier allocation, only wastes 4-8 Byte per unsued channel ...
	SjEqParam       m_currParam;
	bool            m_currParamChanged;
	int             m_currSamplerate;
	int             m_currChannels;

	float*          m_deinterlaceBuf;
	int             m_deinterlaceBufBytes;

	wxCriticalSection m_paramCritical;

	void            delete_all          ();
};


#endif // __SJ_EQUALIZER_H__
