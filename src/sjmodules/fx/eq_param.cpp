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
 * File:    eq_param.cpp
 * Authors: Björn Petersen
 * Purpose: Handle equalizer parameters, usable without the equalizer itself
 *
 ******************************************************************************/


#include <sjbase/base.h>


const char* SjEqParam::s_bandNames[SJ_EQ_BANDS] =
	{ "55", "77", "110", "156", "220", "311", "440", "622", "880", "1.2", "1.8", "2.5", "3.5", "5", "7", "10", "14", "20" };
	// to save horizontal space, we use "Hz" for the lower bands and "KHz" starting with "1.2"; in the dialog we show a legend for this


bool SjEqParam::IsEqualTo(const SjEqParam& o) const
{
	for(int i=0; i<SJ_EQ_BANDS; i++) {
		if(m_bandDb[i]!=o.m_bandDb[i]) {
			return false; // at least on band is different
		}
	}

	return true; // all bands are equal
}


wxString SjEqParam::ToString(const wxString& sep) const
{
	wxString ret;
	for( int i = 0; i < SJ_EQ_BANDS; i++ )
	{
		ret += wxString::Format("%.1f"+sep, m_bandDb[i]);
	}

	ret.Replace(",", ".");      // force using the point instead of the comma as decimal separator
	ret.Replace(".0"+sep, sep); // shorten values with a single zero after the comma
	return ret;
}


void SjEqParam::FromString(const wxString& str__)
{
	wxString str(str__);
	str.Replace(" ", "");
	str.Replace("\r", "");
	str.Replace("\n", ";"); // the bands may be separated by a semicolon (used in the INI) for by a new-line (used in *.feq-files)

	wxArrayString bands = SjTools::Explode(str, ';', SJ_EQ_BANDS, SJ_EQ_BANDS);
	for( int i = 0; i < SJ_EQ_BANDS; i++ )
	{
		m_bandDb[i] = SjTools::ParseFloat(bands[i], 0.0F);
		if( m_bandDb[i] < SJ_EQ_BAND_MIN ) { m_bandDb[i] = SJ_EQ_BAND_MIN; }
		if( m_bandDb[i] > SJ_EQ_BAND_MAX ) { m_bandDb[i] = SJ_EQ_BAND_MAX; }
	}
}


void SjEqParam::FromFile(const wxString& fileName)
{
	// open file
	wxFileSystem fileSystem;
	wxFSFile* fsFile = fileSystem.OpenFile(fileName, wxFS_READ|wxFS_SEEKABLE);
	if( fsFile == NULL ) { wxLogError(_("Cannot read \"%s\"."), fileName.c_str()); return; }

	wxString ext = SjTools::GetExt(fileName);
	if( ext == "fx-eq" )
	{
		// set from old silverjuke ini-format, AutoLevel() as this equalizer works completely different
		wxString content = SjTools::GetFileContent(fsFile->GetStream(), &wxConvISO8859_1 /*file is a windows file, however, we only use ASCII*/);
		float bands[10];
		SjCfgTokenizer cfg;
		cfg.AddFromString(content);
		for( int b = 0; b < 10; b++ ) {
			wxString value = cfg.Lookup(wxString::Format("eq.gain%i", b), "0.0");
			bands[b] = SjTools::ParseFloat(value, 0.0F);
		}
		FromTypical10Band(bands, SJ_TYPICAL_EQ_SJ2);
	}
	else if( ext == "eqf" )
	{
		// set from Winamp, see http://forums.winamp.com/archive/index.php/t-37207.html
		float bands[10] = {0.0F,0.0F,0.0F,0.0F,0.0F,0.0F,0.0F,0.0F,0.0F,0.0F};
		signed char content[299];
		wxInputStream* is = fsFile->GetStream();
		is->Read(content, 299);
		wxString dbg;
		if( content[0]=='W' && content[1]=='i' ) { // rough check for "Winamp EQ library file v1.1" header
			for( int b = 0; b < 10; b++ ) {
				bands[b] = (content[288+b]-31)*-0.375F;            // 0=+12 dB .. 31=0 dB 63=-12 dB
				dbg += wxString::Format("%f ", bands[b]);
			}
			wxLogWarning("%s", dbg.c_str());
		}
		FromTypical10Band(bands, SJ_TYPICAL_EQ_SJ2);
	}
	else if( ext == "eq" )
	{
        // set from Shibatch SuperEQ Preset (space separated integers as: "lpreamp lband1 lband2 ... lband18 rpreamp rband1 rband2 ... rband18" all values are negative, however, the sign is skipped
		wxString content = SjTools::GetFileContent(fsFile->GetStream(), &wxConvISO8859_1 /*file is a windows file, however, we only use ASCII*/);
		content = content.AfterFirst(' '); // skip preamp
		content.Replace(" ", ";");
		FromString(content); // this will just discard the additional values
		for( int b = 0; b < SJ_EQ_BANDS; b++ ) { m_bandDb[b] *= -1; }
	}
	else
	{
		// set from our own format
		wxString content = SjTools::GetFileContent(fsFile->GetStream(), &wxConvISO8859_1 /*file is a windows file, however, we only use ASCII*/);
		FromString(content);
	}

	// close file
	delete fsFile;

	if( GetAutoLevelShift() ) {
		//wxLogWarning(_("Values above +0 dB may cause sound distortion! Use \"Auto Level\" to fix this issue."));
	}
}


void SjEqParam::FromTypical10Band(const float* i10bandsDb, int typical10band)
{
	if( typical10band == SJ_TYPICAL_EQ_WINAMP2 )
	{
		/*
		  #       0              1       2          3         4               5           6        7   8   9      [Winamp2]
		 Hz       60            170     310        600       1K               3K          6K      12K 14K 16K
				/  \        /  /  \     |    \     |   \      |   \          |  \        /  \     |   |    \
		 Hz    55  77   110   156  220  311   440  622   880  1.2K   1.8K  2.5K   3.5K  5K   7K   10K  14K   20K
		  #    0   1    2     3    4    5     6    7     8    9      10    11     12    13   14   15   16    17
		*/
		m_bandDb[0]=m_bandDb[1]             = i10bandsDb[0];
		m_bandDb[2]=m_bandDb[3]=m_bandDb[4] = i10bandsDb[1];
		m_bandDb[5]=m_bandDb[6]             = i10bandsDb[2];
		m_bandDb[7]=m_bandDb[8]             = i10bandsDb[3];
		m_bandDb[9]=m_bandDb[10]            = i10bandsDb[4];
		m_bandDb[11]=m_bandDb[12]           = i10bandsDb[5];
		m_bandDb[13]=m_bandDb[14]           = i10bandsDb[6];
		m_bandDb[15]                        = i10bandsDb[7];
		m_bandDb[16]                        = i10bandsDb[8];
		m_bandDb[17]                        = i10bandsDb[9];
	}
	else
	{
		/*
		  # 0     1      2         3           4             5             6        7          8          9       [Sj2/iTunes]
		 Hz 31    62     125       250         500           1K            2K       4K         8K         16K
			  \    \      |      /  |  \        | \         / |           / |      /   \      /   \      /   \
		 Hz    55  77   110   156  220  311   440  622   880  1.2K   1.8K  2.5K   3.5K  5K   7K   10K  14K   20K
		  #    0   1    2     3    4    5     6    7     8    9      10    11     12    13   14   15   16    17
		*/
		m_bandDb[0]                         = i10bandsDb[0];
		m_bandDb[1]                         = i10bandsDb[1];
		m_bandDb[2]                         = i10bandsDb[2];
		m_bandDb[3]=m_bandDb[4]=m_bandDb[5] = i10bandsDb[3];
		m_bandDb[6]=m_bandDb[7]             = i10bandsDb[4];
		m_bandDb[8]=m_bandDb[9]             = i10bandsDb[5];
		m_bandDb[10]=m_bandDb[11]           = i10bandsDb[6];
		m_bandDb[12]=m_bandDb[13]           = i10bandsDb[7];
		m_bandDb[14]=m_bandDb[15]           = i10bandsDb[8];
		m_bandDb[16]=m_bandDb[17]           = i10bandsDb[9];
	}
}


void SjEqParam::Shift(float add)
{
	for( int i = 0; i < SJ_EQ_BANDS; i++ )
	{
		m_bandDb[i] += add;
		if( m_bandDb[i] < SJ_EQ_BAND_MIN ) { m_bandDb[i] = SJ_EQ_BAND_MIN; }
		if( m_bandDb[i] > SJ_EQ_BAND_MAX ) { m_bandDb[i] = SJ_EQ_BAND_MAX; }
	}
}


float SjEqParam::GetAutoLevelShift()
{
	// Get offset that moves all bands below 0.0 - avoids overdrive and distortions.
	// Moreover, if possible, move all bands up - to be as loud as possible.
	float maxGainAbove0 = 0.0F, minGainBelow0 = 666.6F;
	for( int i = 0; i < SJ_EQ_BANDS; i++ )
	{
		if( m_bandDb[i] > maxGainAbove0 ) {
			maxGainAbove0 = m_bandDb[i];
		}
		else if( m_bandDb[i] < 0.0F ) {
			if( (m_bandDb[i]*-1) < minGainBelow0 ) {
				minGainBelow0 = (m_bandDb[i]*-1);
			}
		}
	}

	if( maxGainAbove0 > 0.0F ) {
		return maxGainAbove0 * -1;
	}
	else if( minGainBelow0 < 666.6F ) {
		for( int i = 0; i < SJ_EQ_BANDS; i++ ) {
			if( m_bandDb[i]+minGainBelow0 > 0.0F ) { return 0.0F; }
		}
		return minGainBelow0;
	}

    return 0.0F;
}

