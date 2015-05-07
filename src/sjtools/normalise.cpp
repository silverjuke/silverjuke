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
 * File:    normalise.cpp
 * Authors: Björn Petersen
 * Purpose: Normalizing Strings for easy sorting and searching
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/normalise.h>


static const wxChar* s_normaliseIso8859_1[256] =
{
	/*  +       0    1    2    3       4     5    6    7       8    9    A    B       C    D    E    F   */
	/*00-0F*/   0,   0,   0,   0,      0,    0,   0,   0,      0,   0,   0,   0,      0,   0,   0,   0,
	/*10-1F*/   0,   0,   0,   0,      0,    0,   0,   0,      0,   0,   0,   0,      0,   0,   0,   0,
	/*20-2F*/   0,   0,   0,   0,      0,    0,   0,   0,      0,   0,   0,   0,      0,   0,   0,   0,
	/*30-3F*/   wxT("0"), wxT("1"), wxT("2"), wxT("3"),    wxT("4"),  wxT("5"), wxT("6"), wxT("7"),    wxT("8"), wxT("9"), 0,   0,      0,   0,   0,   0,
	/*40-4F*/   0,   wxT("a"), wxT("b"), wxT("c"),    wxT("d"),  wxT("e"), wxT("f"), wxT("g"),    wxT("h"), wxT("i"), wxT("j"), wxT("k"),    wxT("l"), wxT("m"), wxT("n"), wxT("o"),
	/*50-5F*/   wxT("p"), wxT("q"), wxT("r"), wxT("s"),    wxT("t"),  wxT("u"), wxT("v"), wxT("w"),    wxT("x"), wxT("y"), wxT("z"), 0,      0,   0,   0,   0,
	/*60-6F*/   0,   wxT("a"), wxT("b"), wxT("c"),    wxT("d"),  wxT("e"), wxT("f"), wxT("g"),    wxT("h"), wxT("i"), wxT("j"), wxT("k"),    wxT("l"), wxT("m"), wxT("n"), wxT("o"),
	/*70-7F*/   wxT("p"), wxT("q"), wxT("r"), wxT("s"),    wxT("t"),  wxT("u"), wxT("v"), wxT("w"),    wxT("x"), wxT("y"), wxT("z"), 0,      0,   0,   0,   0,
	/*80-8F*/   0,   0,   0,   0,      0,    0,   0,   0,      0,   0,   wxT("s"), 0,      wxT("o"), 0,   0,   0,
	/*90-9F*/   0,   0,   0,   0,      0,    0,   0,   0,      0,   0,   wxT("s"), 0,      wxT("o"), 0,   0,   wxT("y"),
	/*A0-AF*/   0,   0,   0,   0,      0,    0,   0,   0,      0,   0,   0,   0,      0,   0,   0,   0,
	/*B0-BF*/   0,   0,   0,   0,      0,    0,   0,   0,      0,   0,   0,   0,      0,   0,   0,   0,
	/*C0-CF*/   wxT("a"), wxT("a"), wxT("a"), wxT("a"),    wxT("ae"), wxT("a"), wxT("a"), wxT("c"),    wxT("e"), wxT("e"), wxT("e"), wxT("e"),    wxT("i"), wxT("i"), wxT("i"), wxT("i"),
	/*D0-DF*/   wxT("d"), wxT("n"), wxT("o"), wxT("o"),    wxT("o"),  wxT("o"), wxT("oe"),0,      wxT("o"), wxT("u"), wxT("u"), wxT("u"),    wxT("ue"),wxT("y"), wxT("t"), wxT("ss"),
	/*E0-EF*/   wxT("a"), wxT("a"), wxT("a"), wxT("a"),    wxT("ae"), wxT("a"), wxT("a"), wxT("c"),    wxT("e"), wxT("e"), wxT("e"), wxT("e"),    wxT("i"), wxT("i"), wxT("i"), wxT("i"),
	/*F0-FF*/   wxT("d"), wxT("n"), wxT("o"), wxT("o"),    wxT("o"),  wxT("o"), wxT("oe"),0,      wxT("o"), wxT("u"), wxT("u"), wxT("u"),    wxT("ue"),wxT("y"), wxT("t"), wxT("y")
};


wxString SjNormaliseString(const wxString& srcString__dontUse, long flags)
{
	wxASSERT( s_normaliseIso8859_1[0]==NULL );

	// first make a copy of the source string as this may get changed in the following preparation
	wxString srcString(srcString__dontUse);

	// prepare source string
	if( flags&(SJ_OMIT_ARTIST|SJ_OMIT_ALBUM)
	 && g_mainFrame && g_mainFrame->m_libraryModule )
	{
		const SjOmitWords* omit = (flags&SJ_OMIT_ARTIST)? g_mainFrame->m_libraryModule->GetOmitArtist() : g_mainFrame->m_libraryModule->GetOmitAlbum();
		srcString = omit->Apply(srcString);
	}

	const wxUChar*       srcPtr = (wxUChar*)srcString.c_str();

	// prepare destination string
	#define              destChars (128+srcString.Len()*6)   // The worst case of output length is sth. like "Ae1Ae1Ae1",
	// the numbers increase the string by 9 characters and the
	// umlauts by 1 character; so 2 characters may become 12 characters. The
	// "128" is for prependend "zzzzzzzzzz", for the nullcharacter and if the string is _only_ a number.
	wxUChar*             destStart = (wxUChar*)malloc(destChars * sizeof(wxChar)); if( destStart == NULL ) { return wxEmptyString; }
	#ifdef __WXDEBUG__
	wxUChar*             destEnd = destStart + destChars;
	#endif
	wxUChar*             destPtr = destStart;

	// start conversion
	int                  numberLen, i;
	const wxChar*        norm;
	while( *srcPtr )
	{
		#if wxUSE_UNICODE
		if( *srcPtr < 1 || *srcPtr > 255 )
		{
			norm = NULL;
		}
		else
		#endif
		{
			norm = s_normaliseIso8859_1[*srcPtr];
		}

		if( norm )
		{
			wxASSERT( wxStrlen(norm) >= 1 && wxStrlen(norm) <= 2 );

			if( *norm >= wxT('0') && *norm <= wxT('9') )
			{
				// add a number...
				if( destPtr == destStart && (flags&SJ_NUM_TO_END) )
				{
					// if a string begins with a number, move them to the very end
					wxASSERT( wxStrlen(SJ_NUM_TO_END_ZZZZZZZZZZ) == 10 );
					wxStrcpy((wxChar*)destPtr, SJ_NUM_TO_END_ZZZZZZZZZZ);
					destPtr += 10;
					wxASSERT( destPtr < destEnd );
				}

				const wxUChar* numberStart = srcPtr;
				numberLen = 0;
				while(
					#if wxUSE_UNICODE
				    *srcPtr >= 1 && *srcPtr < 256 &&
					#endif
				    s_normaliseIso8859_1[*srcPtr]
				    && *(s_normaliseIso8859_1[*srcPtr]) >= wxT('0')
				    && *(s_normaliseIso8859_1[*srcPtr]) <= wxT('9') )
				{
					wxASSERT( wxStrlen(s_normaliseIso8859_1[*srcPtr])==1 );
					numberLen ++;
					srcPtr ++;
				}

				wxASSERT(numberLen>0);

				// ...prepend zeros
				if( flags&SJ_NUM_SORTABLE )
				{
					i = numberLen;
					while(i<10)
					{
						*destPtr++ = wxT('0');
						wxASSERT( destPtr < destEnd );
						i++;
					}
				}

				// ...add the number itself
				for( i=0; i<numberLen; i++ )
				{
					*destPtr++ = *(s_normaliseIso8859_1[*numberStart++]);
					wxASSERT( destPtr < destEnd );
				}
			}
			else
			{
				// add the characters given
				while( *norm )
				{
					wxASSERT( *norm >= wxT('a') && *norm <= wxT('z') );
					*destPtr++ = *norm++;
					wxASSERT( destPtr < destEnd );
				}

				srcPtr++;
			}
		}
		else
		{
			srcPtr++;
		}
	}

	// done so far, cleanup
	*destPtr = 0;

	wxASSERT( destPtr < destEnd );
	wxASSERT( wxStrlen((wxChar*)destStart) == (size_t)(destPtr-destStart) );

	wxString destString(destStart);
	free(destStart);

	if( (flags & SJ_EMPTY_TO_END) && destString.IsEmpty() )
	{
		destString = SJ_NUM_TO_END_ZZZZZZZZZZ SJ_NUM_TO_END_ZZZZZZZZZZ SJ_NUM_TO_END_ZZZZZZZZZZ SJ_NUM_TO_END_ZZZZZZZZZZ;
	}

	return destString;
}
