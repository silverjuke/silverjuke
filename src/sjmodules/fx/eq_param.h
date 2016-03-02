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
 * File:    eq_param.h
 * Authors: Björn Petersen
 * Purpose: Handle equalizer parameters, usable without the equalizer itself
 *
 ******************************************************************************/


#ifndef __SJ_EQ_PARAM_H__
#define __SJ_EQ_PARAM_H__


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


#endif // __SJ_EQ_PARAM_H__

