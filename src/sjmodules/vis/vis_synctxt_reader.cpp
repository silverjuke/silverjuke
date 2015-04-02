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
 * File:    synctxt_reader.cpp
 * Authors: Björn Petersen
 * Purpose: High-level LRC & Co. handling
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/vis/vis_synctxt_reader.h>
#include <sjmodules/vis/vis_synctxt_raw.h>


/*******************************************************************************
 * Constructor / Destructor
 ******************************************************************************/


SjSyncTxtReader::SjSyncTxtReader(wxFSFile* fsFile, SjSyncTxtFormat format,
                                 const wxString& artist, const wxString& title)
{
	m_artist = artist;
	m_title  = title;

	// load the file content
	wxString content = SjTools::GetFileContent(fsFile->GetStream(), &wxConvISO8859_1);
	delete fsFile; // no longer needed

	// create object to hold the raw data
	m_raw = new SjSyncTxtRaw(artist, title);

	// prepare the data
	if( format == SJ_SYNCTXT_LRC )
	{
		m_raw->ParseLrc(content);
	}

	m_raw->Finalize();

	// allocate some drawing objects
	m_font = wxFont(10/*recalculated later*/, wxFONTFAMILY_SWISS, wxNORMAL/*style*/, wxBOLD/*weight*/, false/*underline*/);
	m_fontSmall = m_font;
	m_fontPixelH = 0;

	m_colourStatic  /*red*/   = wxColour(240, 0, 0);
	m_bgBrushStatic /*yellow*/= wxBrush(wxColour(240, 240, 0));

	m_colourUpcoming/*red*/   = wxColour(240, 0, 0);
	m_colourSung    /*yellow*/= wxColour(240, 240, 0);

	// set first page
	m_currMs        = 0;
	m_currWord      = 0;
	m_currWord2     = 0;
	m_currPageStart = 0;
	m_updateAll     = true;
	m_updateSth     = true;
	SetPosition(0);
}


SjSyncTxtReader::~SjSyncTxtReader()
{
	delete m_raw;
}


/*******************************************************************************
 * Playing
 ******************************************************************************/


bool SjSyncTxtReader::SetPosition(long ms)
{
	// rewind?
	if( ms < m_currMs )
	{
		m_currMs = 0;
		m_currPageStart = 0;
		m_currWord = 0;
		m_currWord2 = 0;
		m_updateAll = true;
		m_updateSth = true;
	}
	else
	{
		m_currMs = ms;
	}

	// current page changed?
	long wordIndex, wCount = m_raw->m_words.GetCount();
	for( wordIndex = m_currPageStart+1; wordIndex < wCount; wordIndex++ )
	{
		SjSyncTxtWord& word = m_raw->m_words[wordIndex];
		if( word.m_isPageStart )
		{
			if( ms >= word.m_pageStartMs )
			{
				m_currPageStart = wordIndex;
				m_updateAll = true;
				m_updateSth = true;
			}
			else
			{
				break;
			}
		}
	}

	// check all timing information
	// (this cannot be combined with the loop above as the prior may change m_currPageStart)
	long newWord = m_currWord;
	long newWord2 = m_currWord2;
	long currPageEnd = (m_currPageStart + m_raw->m_words[m_currPageStart].m_pageWordCount) - 1;
	int isStaticPage = m_raw->m_words[m_currPageStart].m_isStaticPage;
	long currPageStartMs = m_raw->m_words[m_currPageStart].m_pageStartMs;
	long chars = 0;
#define CHARS_MS 6
	for( wordIndex = m_currPageStart; wordIndex <= currPageEnd; wordIndex++ )
	{
		SjSyncTxtWord& word = m_raw->m_words[wordIndex];
		chars += word.m_word.Len();

		word.m_colour = NULL;
		if( ms >= word.m_wordSingMs || isStaticPage )
		{
			word.m_colour = isStaticPage == 2? &m_colourStatic : &m_colourSung;
			newWord = wordIndex;
		}
		else
		{
			if( ms > currPageStartMs+chars*CHARS_MS )
			{
				word.m_colour = &m_colourUpcoming;
				newWord2 = wordIndex;
			}
		}
	}

	if( newWord != m_currWord
	        || newWord2 != m_currWord2 )
	{
		m_currWord = newWord;
		m_currWord2 = newWord2;
		m_updateSth = true;
	}

	return (m_currWord != (long)m_raw->m_words.GetCount()-1);
}


void SjSyncTxtReader::Render(wxDC& dc, SjVisBg& bg, bool pleaseUpdateAll)
{
	// update needed?
	if( m_updateAll )
		pleaseUpdateAll = true;

	if( !m_updateSth && !pleaseUpdateAll )
		return;

	// basic screen calculations
	wxSize      dcSize = dc.GetSize(); if( dcSize.y <= 0 ) return;
	float       dcAspect = (float)dcSize.x / (float)dcSize.y;

	float       areaAspect = 300.0F / 216.0F; // for a good compatibility to CDG, use the same area and aspect

	long areaX, areaY, areaW, areaH;
	if( dcAspect > areaAspect )
	{
		// scale by height
		areaH = long( (float)dcSize.y * KARAOKE_RENDER_SHRINK );
		areaW = long( (float)areaH * areaAspect );
	}
	else
	{
		// scale by width
		areaW = long( (float)dcSize.x * KARAOKE_RENDER_SHRINK );
		areaH = long( (float)areaW / areaAspect );
	}

	// find center
	areaX = (dcSize.x - areaW) / 2;
	areaY = (dcSize.y - areaH) / 2;

	// update the font
	int isStaticPage = m_raw->m_words[m_currPageStart].m_isStaticPage;
	wxCoord fontPixelH;
	{
		wxCoord dummy;
		fontPixelH = areaH/MAX_LINES_PER_PAGE;
		if( fontPixelH != m_fontPixelH )
		{
			wxCoord fontPtSize = SjVisBg::SetFontPixelH(dc, m_font, fontPixelH);
			m_fontSmall.SetPointSize((wxCoord)((float)fontPtSize*0.6F));
			m_fontPixelH = fontPixelH;
		}

		dc.SetFont(isStaticPage==1? m_fontSmall : m_font);
		dc.GetTextExtent(wxT("Ag"), &dummy, &fontPixelH);
		dc.SetBackgroundMode(wxTRANSPARENT);
	}

	long offset = fontPixelH/16;
	if( offset < 2 ) offset = 2;

	// draw background
	if( pleaseUpdateAll )
	{
		if( isStaticPage == 2 )
		{
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.SetBrush(m_bgBrushStatic);
			dc.DrawRectangle(areaX, areaY, areaW, areaH);
		}
		else
		{
			dc.SetClippingRegion(areaX, areaY, areaW, areaH);
			bg.DrawBackground(dc);
			dc.DestroyClippingRegion();
		}
	}

#ifdef __WXDEBUG__
	/*dc.SetPen(wxPen(m_colourUpcoming));
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.DrawRectangle(areaX, areaY, areaW, areaH);*/
#endif

	// set lineY
	wxCoord dummy, lineW;
	long lineCount = m_raw->m_words[m_currPageStart].m_pageLineCount;
	long lineY = areaY + (areaH-fontPixelH*lineCount) / 2;

	// go through all lines and draw them
	dc.SetBackgroundMode(wxTRANSPARENT);
	long lineStartIndex = m_currPageStart, wordIndex, lineEndIndex,
	     currPageEnd = (m_currPageStart + m_raw->m_words[m_currPageStart].m_pageWordCount) - 1;
	while( 1 )
	{
		// calculate the width of the line
		lineW = 0;
		lineEndIndex = (lineStartIndex + m_raw->m_words[lineStartIndex].m_lineWordCount) - 1;
		for( wordIndex = lineStartIndex; wordIndex <= lineEndIndex; wordIndex++ )
		{
			SjSyncTxtWord& word = m_raw->m_words[wordIndex];
			dc.GetTextExtent(word.m_word, &word.m_pixelW, &dummy);
			lineW += word.m_pixelW;
		}

		// draw all words of the line
		long lineX = areaX + (areaW-lineW) / 2;
		if( lineX < areaX ) lineX = areaX;

		if( lineW > areaX )
		{
			dc.SetClippingRegion(areaX, areaY, areaW, areaH);
		}

		for( wordIndex = lineStartIndex; wordIndex <= lineEndIndex; wordIndex++ )
		{
			SjSyncTxtWord& word = m_raw->m_words[wordIndex];

			if( word.m_colour )
			{
				// draw shadow
				if( pleaseUpdateAll || word.m_colour == &m_colourUpcoming )
				{
					dc.SetTextForeground(*wxBLACK);
					dc.DrawText(word.m_word, lineX+offset, lineY+offset);
				}

				// draw text
				dc.SetTextForeground(*word.m_colour);
				dc.DrawText(word.m_word, lineX, lineY);
			}

			lineX += word.m_pixelW;
		}

		if( lineW > areaX )
		{
			dc.DestroyClippingRegion();
		}

		// next line
		lineY += fontPixelH;
		lineStartIndex = lineEndIndex+1;
		if( lineStartIndex > currPageEnd )
		{
			break;
		}
	}

	// update done - reset update flags
	m_updateAll = false;
	m_updateSth = false;
}

