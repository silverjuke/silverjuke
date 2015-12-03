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
 * File:    synctxt_raw.h
 * Authors: Björn Petersen
 * Purpose: Low-level LRC etc. handling
 *
 ******************************************************************************/


#ifndef __SJ_SYNCTXT_RAW_H__
#define __SJ_SYNCTXT_RAW_H__


#define MAX_LINES_PER_PAGE           8
#define MAX_CHARS_PER_LINE          24

#define BREAK_PAGES_AT_SILENCE_MS    3000
#define PAUSE_PAGES_AT_SILENCE_MS   10000
#define DISPLAY_BEFORE_SING_MS       2000 // high priority
#define DISPLAY_AFTER_SUNG_MS        1000 // lower priority, must be smaller than DISPLAY_BEFORE_SING_MS


class SjSyncTxtWord
{
public:
	SjSyncTxtWord(long wordSingMs, const wxString& word, bool lineStart, bool pageStart)
	{
		m_isLineStart   = lineStart? 1 : 0;
		m_isPageStart   = pageStart;
		m_word          = word;
		m_wordSingMs    = wordSingMs;
		m_pageStartMs   = wordSingMs;
		m_isStaticPage  = 0;
		m_colour        = NULL;
		m_lineWordCount = 1; // makes adding static frames easied, else set in Finalize()
	}

	// seeking information
	bool            m_isPageStart;  // may be given to constructor, additional breaks may be added on Finalize()
	long            m_pageWordCount;// number of words on page, only valid if m_isPageStart=true, calculated on Finalize()
	long            m_pageLineCount;// number of lines on page, only valid if m_isPageStart=true, calculated on Finalize()

	int             m_isLineStart;  // 1=hard break (given to constructor), 2=soft break (added on Finalize); page breaks are preferred on hard breaks
	long            m_lineWordCount;// number of words on line, only valid if m_isLineStart=true, calculated on Finalize()

	// timing information
	wxString        m_word;
	long            m_wordSingMs;
	long            m_pageStartMs;

	// a "static page" is a page that normally contains additional information
	// static pages are identified by missing word timing information.
	// we have two types of static pages:
	// 2: at the beginning of the song (very hilited, yellow background to the the people notice there is a new song)
	// 1: in or at the end of the song (little hilited to differ from the vocals)
	int             m_isStaticPage;

	// drawing information, may be used by derived classes or by the user
	wxColour*       m_colour; // NULL: don't draw
	wxCoord         m_pixelW;
};


WX_DECLARE_OBJARRAY(SjSyncTxtWord, SjArraySyncTxtWord);


class SjSyncTxtRaw
{
public:
	                    SjSyncTxtRaw        (const wxString& artist, const wxString& title);
	                    ~SjSyncTxtRaw       () {}
	void                ParseLrc            (const wxString& content);
	void                Finalize            ();

	SjArraySyncTxtWord  m_words;
	wxString            m_artist, m_title;

private:
	bool                AddWord             (long ms, const wxString& word, bool newLine=false, bool newPage=false);

	long                m_lastAddedMs;

	void                TrimBreak           (long wordIndex);
};


#endif // __SJ_SYNCTXT_RAW_H__
