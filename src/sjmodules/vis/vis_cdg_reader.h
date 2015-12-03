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
 * File:    cdg_reader.h
 * Authors: Björn Petersen
 * Purpose: High-level CDG handling
 *
 ******************************************************************************/


#ifndef __SJ_CDG_READER_H__
#define __SJ_CDG_READER_H__


#include "vis_karaoke_module.h"
#include "vis_cdg_raw.h"


class SjCdgReader : public SjKaraokeReader
{
public:
	                SjCdgReader         (wxFSFile* fsFile);
	virtual         ~SjCdgReader        ();
	virtual bool    SetPosition         (long ms);
	virtual void    Render              (wxDC&, SjVisBg&, bool pleaseUpdateAll);

private:
	SjCdgRaw        m_screen;
	wxFSFile*       m_fsFile;

	bool            m_eof;
	long            m_eokMs; // end of Karaoke ms, 0 if unknown

	#define         SJ_CDG_REALLOC_EVERY SJ_ONE_MB
	long            m_fsFileDataAllocated;
	long            m_fsFileDataLoaded;
	unsigned char*  m_fsFileData;

	long            m_fsFileDataPos;

	static long     Ms2Bytes            (long ms);
};


#endif /*__SJ_CDG_READER_H__*/

