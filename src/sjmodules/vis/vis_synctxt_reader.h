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
 * File:    synctxt_reader.h
 * Authors: Björn Petersen
 * Purpose: High-level LRC & Co. handling
 *
 ******************************************************************************/


#ifndef __SJ_SYNCTXT_READER_H__
#define __SJ_SYNCTXT_READER_H__


#include "vis_karaoke_module.h"


class SjSyncTxtRaw;


enum SjSyncTxtFormat
{
    SJ_SYNCTXT_LRC=1
};


class SjSyncTxtReader : public SjKaraokeReader
{
public:
	                SjSyncTxtReader     (wxFSFile* fsFile, SjSyncTxtFormat, const wxString& artist, const wxString& title);
	virtual         ~SjSyncTxtReader    ();
	virtual bool    SetPosition         (long ms);
	virtual void    Render              (wxDC&, SjVisBg&, bool pleaseUpdateAll);

private:
	// raw data
	SjSyncTxtRaw*   m_raw;

	// the "screen"
	wxFont          m_font;
	wxFont          m_fontSmall;
	wxCoord         m_fontPixelH;

	wxColour        m_colourStatic;
	wxBrush         m_bgBrushStatic;
	wxColour        m_colourUpcoming;
	wxColour        m_colourSung;
	bool            m_updateAll;
	bool            m_updateSth;

	long            m_currPageStart; // index in the word list
	long            m_currMs;
	long            m_currWord, m_currWord2;
};


#endif /*__SJ_SYNCTXT_READER_H__*/

