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
 * File:    synctxt_raw.cpp
 * Authors: Björn Petersen
 * Purpose: Low-level LRC etc. handling
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/tokenzr.h>
#include <sjmodules/vis/vis_synctxt_raw.h>

#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArraySyncTxtWord);


SjSyncTxtRaw::SjSyncTxtRaw(const wxString& artist, const wxString& title)
{
	m_artist        = artist;
	m_title         = title;
	m_lastAddedMs   = 0;
}


bool SjSyncTxtRaw::AddWord(long ms, const wxString& word__, bool newLine, bool newPage)
{
	// replace tabs by spaces, remove multiple spaces in the word
	wxString word(word__);
	word.Replace(wxT("\t"), wxT(" "));
	while( word.Find(wxT("  ")) != -1 )
		word.Replace(wxT("  "), wxT(" "));

	// do not add nothing
	long wordLen = word.Len();
	if( wordLen == 0 || wordLen > 1000 )
		return false;

	if( newPage )
		newLine = true;

	// if the word contains spaces INSIDE (beginning and end are okay),
	// add each tag separately by a recursive call
	int spacePos = word.find(' ', 1);
	if( spacePos > 0 && spacePos < wordLen-1 )
	{
		bool add1 = AddWord(ms, word.Left(spacePos), newLine, newPage);
		bool add2 = AddWord(ms, word.Mid(spacePos), false);
		return (add1||add2);
	}

	// validate the timestamp
	if( ms < m_lastAddedMs )
		ms = m_lastAddedMs;
	m_lastAddedMs = ms;

	// add a new line?
	if( m_words.IsEmpty() )
	{
		newLine = true;
		newPage = true;
	}

	// add the word
	m_words.Add(new SjSyncTxtWord(ms, word, newLine, newPage));
	return true;
}


static bool CanBreak(const wxString& word1, const wxString& word2)
{
	wxChar firstChar = word1.Last();
	wxChar lastChar  = word2[0];
	if( firstChar == wxT(' ')
	        || lastChar == wxT(' ') )
	{
		return true;
	}

	return false;
}


void SjSyncTxtRaw::TrimBreak(long wordIndex)
{
	// trim word in new line at start
	m_words[wordIndex].m_word.Trim(false);

	// trim word in prev. line at end
	if( wordIndex > 0 )
		m_words[wordIndex-1].m_word.Trim(true);
}


void SjSyncTxtRaw::Finalize()
{
	// there is a lot to do with the raw words ...

	// add some dummy words if the list is empty
	if( m_words.GetCount() == 0 )
	{
		AddWord(0, _("No lyrics found."), true);
	}

	long wordIndex, wCount = m_words.GetCount();

	// wrap lines that are too long (soft wraps, we try to avoid page breaks here later)
	{
		long charsPerLine = 0;
		for( wordIndex = 0; wordIndex < wCount; wordIndex++ )
		{
			SjSyncTxtWord& word = m_words[wordIndex];
			if( word.m_isLineStart )
			{
				TrimBreak(wordIndex);
				charsPerLine = 0;
			}

			charsPerLine += word.m_word.Len();

			if( charsPerLine >= MAX_CHARS_PER_LINE )
			{
				bool lineBreakDone = false;
				long testIndex;
				for( testIndex = wordIndex; testIndex > 0; testIndex-- )
				{
					SjSyncTxtWord& test1 = m_words[testIndex-1];
					SjSyncTxtWord& test2 = m_words[testIndex];

					if( test1.m_isLineStart )
					{
						break; // no chance for a fine break
					}
					else if( CanBreak(test1.m_word, test2.m_word) )
					{
						TrimBreak(testIndex);
						test2.m_isLineStart = 2; // soft break
						charsPerLine = test2.m_word.Len();
						wordIndex = testIndex; // start over here
						lineBreakDone = true;
						break;
					}
				}

				if( !lineBreakDone )
				{
					word.m_isLineStart = 2; // soft break
					charsPerLine = word.m_word.Len();
				}
			}
		}
	}

	// set all m_lineWordCount offsets,
	// see if we can define static pages
	int canDefineStaticPages = 0;
	{
		long lastLineStartIndex = 0;
		long lineStartMs = m_words[0].m_wordSingMs;
		for( wordIndex = 1; wordIndex < wCount; wordIndex++ )
		{
			SjSyncTxtWord& word = m_words[wordIndex];
			if( word.m_isLineStart )
			{
				m_words[lastLineStartIndex].m_lineWordCount = (wordIndex-lastLineStartIndex);
				lastLineStartIndex = wordIndex;
				lineStartMs = word.m_wordSingMs;
			}
			else if( word.m_wordSingMs > lineStartMs )
			{
				canDefineStaticPages = 2;
			}
		}
		m_words[lastLineStartIndex].m_lineWordCount = ((wCount-1)-lastLineStartIndex)+1;
	}

	// break down pages with at least BREAK_PAGES_AT_SILENCE_MS silence (no words) in between
	{
		for( wordIndex = 1; wordIndex < wCount; wordIndex++ )
		{
			SjSyncTxtWord& word = m_words[wordIndex];
			if(  word.m_isLineStart == 1 )
			{
				long silenceMs = word.m_wordSingMs-m_words[wordIndex-1].m_wordSingMs;
				if( silenceMs >= BREAK_PAGES_AT_SILENCE_MS )
					word.m_isPageStart = true;
			}
		}
	}

	// hard breaking of pages that have more than MAX_LINES_PER_PAGE lines
	{
		long linesPerPage = MAX_LINES_PER_PAGE;
		for( wordIndex = 0; wordIndex < wCount; wordIndex++ )
		{
			SjSyncTxtWord& word = m_words[wordIndex];
			if( word.m_isLineStart )
			{
				if( word.m_isPageStart )
				{
					// found a predefined page break
					linesPerPage = 1;
				}
				else if( linesPerPage >= MAX_LINES_PER_PAGE )
				{
					// we have to add a manial page break
					// we try to do this only on predefined "hard" line breaks
					// (not on the "soft" line breaks added above)
					bool pageBreakDone = false;
					if( word.m_isLineStart == 2 )
					{
						// go back searching a hard break
						long testIndex;
						for( testIndex = wordIndex-1; testIndex > 0; testIndex-- )
						{
							SjSyncTxtWord& test = m_words[testIndex];
							if( test.m_isPageStart )
							{
								break; // no chance
							}
							else if( test.m_isLineStart == 1 )
							{
								test.m_isPageStart = true;
								linesPerPage = 1;
								wordIndex = testIndex; // start over here
								pageBreakDone = true;
								break;
							}
						}
					}

					if( !pageBreakDone )
					{
						linesPerPage = 1;
						word.m_isPageStart = true;
					}
				}
				else
				{
					// no page break
					linesPerPage++;
				}
			}
		}
	}

	// set all m_pageEndIndex and m_lineCount offsets, set static pages
	{
		long lastPageStartIndex = 0;
		long lineCount = 1;
		long pageStartMs = m_words[0].m_wordSingMs;
		bool isStaticPage = true;
		for( wordIndex = 1; wordIndex < wCount; wordIndex++ )
		{
			SjSyncTxtWord& word = m_words[wordIndex];

			if( word.m_isPageStart )
			{
				wxASSERT( DISPLAY_AFTER_SUNG_MS < DISPLAY_BEFORE_SING_MS );

				word.m_pageStartMs = word.m_wordSingMs - DISPLAY_BEFORE_SING_MS;
				if( word.m_pageStartMs < m_words[wordIndex-1].m_wordSingMs + DISPLAY_AFTER_SUNG_MS )
				{
					word.m_pageStartMs = word.m_wordSingMs -
					                     (word.m_wordSingMs - m_words[wordIndex-1].m_wordSingMs) / 2;
				}

				m_words[lastPageStartIndex].m_pageWordCount = (wordIndex-lastPageStartIndex);
				m_words[lastPageStartIndex].m_pageLineCount = lineCount;
				if( isStaticPage )
				{
					m_words[lastPageStartIndex].m_isStaticPage = canDefineStaticPages;
				}
				else if( canDefineStaticPages )
				{
					canDefineStaticPages = 1; // set to inside song
				}

				lastPageStartIndex = wordIndex;
				lineCount = 1;

				pageStartMs = word.m_wordSingMs;
				isStaticPage = true;
			}
			else
			{
				if( word.m_wordSingMs != pageStartMs )
				{
					isStaticPage = false;
				}

				if( word.m_isLineStart )
				{
					lineCount++;
				}
			}
		}
		m_words[lastPageStartIndex].m_pageWordCount = ((wCount-1)-lastPageStartIndex)+1;
		m_words[lastPageStartIndex].m_pageLineCount = lineCount;
		if( isStaticPage )
		{
			m_words[lastPageStartIndex].m_isStaticPage = canDefineStaticPages;
		}
	}

	// if the first page is not static, try to add one with the
	// artist / title information
	if( m_words[0].m_isStaticPage == 0
	        && m_words[0].m_wordSingMs > 1000 )
	{
		SjSyncTxtWord* artistLine = new SjSyncTxtWord(0L, m_artist, true, true);
		artistLine->m_pageWordCount = 2;
		artistLine->m_pageLineCount = 2;
		artistLine->m_isStaticPage = 2;
		SjSyncTxtWord* titleLine = new SjSyncTxtWord(0L, m_title,  false, false);


		m_words.Insert(titleLine, 0L); // note the reverse order!
		m_words.Insert(artistLine, 0L);
	}

	// insert "pause" pages for two pages with at least PAUSE_PAGES_AT_SILENCE_MS silence (no words) in between
	{
		for( wordIndex = 1; wordIndex < wCount; wordIndex++ )
		{
			SjSyncTxtWord& thisWord = m_words[wordIndex];
			SjSyncTxtWord& lastWord = m_words[wordIndex-1];
			if( thisWord.m_isPageStart == 1
			        && thisWord.m_isStaticPage == 0 )
			{
				long silenceMs = thisWord.m_wordSingMs-lastWord.m_wordSingMs;
				if( silenceMs >= PAUSE_PAGES_AT_SILENCE_MS )
				{
					// also add a "Pause" screen before
					wxString text;
					text << _("Pause") << wxT(" (") << SjTools::FormatTime((silenceMs-DISPLAY_AFTER_SUNG_MS)/1000) << wxT(")");
					SjSyncTxtWord* breakPage = new SjSyncTxtWord(lastWord.m_wordSingMs+DISPLAY_AFTER_SUNG_MS*2, text, true, true);
					breakPage->m_pageWordCount = 1;
					breakPage->m_pageLineCount = 1;
					breakPage->m_isStaticPage = 1;

					m_words.Insert(breakPage, wordIndex);

					// make sure, the for() look continues safely
					wordIndex++;
					wCount++;
				}
			}
		}
	}

	// finally, add a black screen at the end of all
	{
		SjSyncTxtWord* blackPage = new SjSyncTxtWord(m_words.Last().m_wordSingMs+DISPLAY_AFTER_SUNG_MS*2, wxT(""), true, true);
		blackPage->m_pageWordCount = 1;
		blackPage->m_pageLineCount = 1;
		blackPage->m_isStaticPage = 1;

		m_words.Add(blackPage);
		wCount++;
	}
}


/*******************************************************************************
 * Parsing an LRC file
 ******************************************************************************/


/* content__ is a complete LRC-File in the format:

        [artist:Elton John]
        [title:Honky Cat]
        [album:Honky Chateau]

        [00:00.00]
        [00:00.93] Honky Cat
        [00:13.18] When I look back boy I must have been green

in the format from lrcdb.org:

        [artist:Elton John]
        [title:Honky Cat]
        [album:Honky Chateau]

        [0:00]
        [0:00.930] Honky Cat
        [0:13.180] When I look back boy I must have been green

There may also be timing information in the lines:

        [ti:Heartbreak Hotel]
        [ar:Elvis Presley]
        [au:Axton / Durden / Presley]
        [al:Elvis - 30 #1 Hits]
        [by:Wooden Ghost]
        [re:A2 Media Player V2.2 lrc format]
        [ve:V2.20]
        [00:00.50]Well, <00:00.85>since <00:01.15>my <00:01.70>ba<00:01.90>by <00:02.10>left <00:02.47>me<00:02.72>
        [00:03.05]Well, <00:03.25>I <00:03.45>found <00:03.75>a <00:03.90>new <00:04.10>place <00:04.50>to <00:04.78>dwell<00:05.00>


For choruses, the following shortcut method may be used:

        [02:49][01:46][01:01]You Shook me all night long
        [02:56][01:52][01:07]Yeah you shook me all night long

Further information:

        http://de.wikipedia.org/wiki/LRC_(Dateiformat)
        http://www.lrcfiles.com   */


static bool parseLrcTime(const wxString& str__, long& ms)
{
	// the given string should start with sth. like
	//      00:00.00
	// or   0:00
	// or   0:00.000

#define     IS_DIG(a)   ((a)>=wxT('0') && (a)<=wxT('9'))
#define     IS_SEP(a)   ((a)==wxT(':') || (a)<=wxT('.'))

	long        minutes = 0, seconds = 0, thousands = 0;

	// remove leading spaces and ensure some min. length to avoid index errors
	wxString p = str__ + wxT("~~~~~~~~~~~~~~~~");
	p.Trim(false);

	// read the minutes
	if( IS_DIG(p[0]) && IS_DIG(p[1]) && IS_SEP(p[2]) )
	{
		if( !p.Left(2).ToLong(&minutes) ) { minutes = 0; }
		p = p.Mid(3);
	}
	else if( IS_DIG(p[0]) && IS_SEP(p[1]) )
	{
		if( !p.Left(1).ToLong(&minutes) ) { minutes = 0; }
		p = p.Mid(2);
	}
	else
	{
		return false;
	}

	// read the seconds
	if( IS_DIG(p[0]) && IS_DIG(p[1]) && !IS_DIG(p[2]) )
	{
		if( !p.Left(2).ToLong(&seconds) ) { seconds = 0; }
		p = p.Mid(2);
	}
	else
	{
		return false;
	}

	// read the thousands or hundrets
	if( IS_SEP(p[0]) && IS_DIG(p[1]) && IS_DIG(p[2]) )
	{
		if( IS_DIG(p[3]) )
		{
			if( !p.Mid(1, 3).ToLong(&thousands) ) { thousands = 0; }
		}
		else
		{
			if( !p.Mid(1, 2).ToLong(&thousands) ) { thousands = 0; }
			thousands *= 10;
		}
	}

	// done
	ms = minutes*60000 + seconds*1000 + thousands;
	return true;
}


void SjSyncTxtRaw::ParseLrc(const wxString& content__)
{
	// prepare content to contain only [mm:ss:hh] tags
	wxString content(content__);
	content.Replace(wxT("<"), wxT("["));
	content.Replace(wxT(">"), wxT("]"));

	// go through each line of content
	SjLineTokenizer     contentTkz(content);
	wxChar*             currLinePtr;
	wxString            token, metaType, metaContent;
	long                ms;
	bool                firstInLine;
	while( (currLinePtr=contentTkz.GetNextLine()) )
	{
		if( *currLinePtr == 0 || *currLinePtr != wxT('[') )
		{
			// skip empty lines or lines not beginning with a '['
			// (the line is already trimmed by the SjLineTokenizer object)
			continue;
		}

		currLinePtr++; // remove the leading '['
		wxStringTokenizer lineTkz(currLinePtr, wxT("["));
		firstInLine = true;
		while( lineTkz.HasMoreTokens() )
		{
			// get token as sth. like "00:36.95]text" or "ti:Deeper And Deeper]"
			token = lineTkz.GetNextToken();

			// check if the first two letters contain numbers
			if( parseLrcTime(token, ms) )
			{
				if( AddWord(ms, token.AfterFirst(']'), firstInLine) )
					firstInLine = false;
			}
			else if( token[2]==wxT(':') && token.Len() > 4 )
			{
				// got some meta information
				metaType = token.Left(2);
				metaContent = token.Mid(3, token.Len()-4);

				if( metaType == wxT("ti") && m_title.IsEmpty() )
				{
					m_title = metaContent;
				}
				else if( metaType == wxT("ar") && m_artist.IsEmpty() )
				{
					m_artist = metaContent;
				}
			}
		}
	}
}
