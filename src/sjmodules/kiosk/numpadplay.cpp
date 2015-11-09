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
 * File:    numpadplay.cpp
 * Authors: Björn Petersen
 * Purpose: The numpad handler for selecting and playing tracks
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/browser.h>
#include <sjmodules/kiosk/numpad.h>
#include <sjmodules/kiosk/numpadplay.h>


SjNumpadPlayHandler::SjNumpadPlayHandler(SjNumpadInput* numpadInput)
	: SjNumpadHandler(numpadInput, SJ_NUMPAD_PLAY)
{
}


SjNumpadPlayReturn SjNumpadPlayHandler::GetPressedDigitsInfo(wxString& digits, wxArrayString& urls) const
{
	// prepare
	SjNumpadPlayReturn  ret = SJ_NUMPAD_WAITING;
	SjCol*              col = NULL;
	SjRow*              row;
	int                 i, iCount;
	long                numAlbumDigits = m_numpadInput->GetNumAlbumDigits();
	long                pressedAlbumNumber,
	                    pressedAlbumIndex,
	                    pressedTrackNumber,
	                    maxTrackNumber,
	                    tempTrackNumber,
	                    numTrackDigits;
	wxString            pressedUrl;

	wxASSERT( urls.IsEmpty() );

	// get all digits as a string
	digits.Empty();
	iCount = m_pressedDigits.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		digits.Append((wxChar)((long)'0'+m_pressedDigits[i]));
	}

	// get album and track number (if complete)
	if( (long)digits.Len() < numAlbumDigits )
	{
		goto GetPressedDigitsInfo_Cleanup; // wait until the album number is complete
	}

	// get album number
	wxASSERT( (long)digits.Len() >= numAlbumDigits );
	if( !digits.Left(numAlbumDigits).ToLong(&pressedAlbumNumber, 10) ) { pressedAlbumNumber = 0; }

	// check album number
	if( pressedAlbumNumber == 0 )
	{
		ret = SJ_NUMPAD_ENTER_MENU;
		goto GetPressedDigitsInfo_Cleanup;
	}
	else if( pressedAlbumNumber < 0 || pressedAlbumNumber > m_numpadInput->GetAlbumCount() )
	{
		ret = SJ_NUMPAD_ERROR;
		goto GetPressedDigitsInfo_Cleanup;
	}

	// any track number yet added?
	wxASSERT( (long)digits.Len() >= numAlbumDigits );
	if( (long)digits.Len() == numAlbumDigits )
	{
		goto GetPressedDigitsInfo_Cleanup;
	}

	// load album information
	wxASSERT( pressedAlbumNumber > 0 );
	pressedAlbumIndex = pressedAlbumNumber-1;
	col = g_mainFrame->m_columnMixer.GetUnmaskedCol(pressedAlbumIndex);
	if( col == NULL )
	{
		ret = SJ_NUMPAD_ERROR;
		goto GetPressedDigitsInfo_Cleanup;
	}

	// get max. track number
	maxTrackNumber = 0;
	for( i = 0; i < col->m_rowCount; i++ )
	{
		row = col->m_rows[i];

		if( row->m_roughType == SJ_RRTYPE_NORMAL )
		{
			tempTrackNumber = m_numpadInput->UnfinalizeTrackNumber(row->m_textl);
			if( tempTrackNumber > maxTrackNumber )
			{
				maxTrackNumber = tempTrackNumber;
			}
		}
	}
	numTrackDigits = m_numpadInput->GetNumDigits(maxTrackNumber);

	// get entered track number
	if( !digits.Mid(numAlbumDigits).ToLong(&pressedTrackNumber, 10) ) { pressedTrackNumber = 0; }
	if( pressedTrackNumber > maxTrackNumber || pressedTrackNumber < 0 )
	{
		ret = SJ_NUMPAD_ERROR;
		goto GetPressedDigitsInfo_Cleanup;
	}

	// Track number complete?
	wxASSERT( (long)digits.Len() > numAlbumDigits );
	if( (long)digits.Len()-numAlbumDigits >= numTrackDigits )
	{
		if(  pressedTrackNumber == 0
		        && !g_mainFrame->IsOpAvailable(SJ_OP_MULTI_ENQUEUE) )
		{
			ret = SJ_NUMPAD_ERROR;
			goto GetPressedDigitsInfo_Cleanup;
		}

		for( i = 0; i < col->m_rowCount; i++ )
		{
			row = col->m_rows[i];

			if( row->m_roughType == SJ_RRTYPE_NORMAL )
			{
				tempTrackNumber = m_numpadInput->UnfinalizeTrackNumber(col->m_rows[i]->m_textl);
				if( pressedTrackNumber == tempTrackNumber
				        || pressedTrackNumber == 0 )
				{
					if( urls.IsEmpty() )
						g_mainFrame->m_columnMixer.SelectAll(FALSE);
					row->Select(TRUE);

					urls.Add(row->m_url);
				}
			}
		}

		if( !urls.IsEmpty() )
		{
			ret = SJ_NUMPAD_URL_PRESSED;
			g_mainFrame->m_browser->RefreshSelection();

			goto GetPressedDigitsInfo_Cleanup;
		}
	}

	// Cleanup
GetPressedDigitsInfo_Cleanup:

	if( col )
	{
		delete col;
	}

	return ret;
}


SjNumpadHandlerId SjNumpadPlayHandler::KeyPressed(int idt)
{
	if( idt == IDT_NUMPAD_MENU )
	{
		return SJ_NUMPAD_MENU;
	}
	else if( idt >= IDT_NUMPAD_0 && idt <= IDT_NUMPAD_9 )
	{
		wxString digits;
		wxArrayString urls;

		m_pressedDigits.Add(idt-IDT_NUMPAD_0);
		SjNumpadPlayReturn state = GetPressedDigitsInfo(digits, urls);
		if( state == SJ_NUMPAD_ERROR )
		{
			g_mainFrame->SetDisplayMsg(_("Invalid track number"), m_numpadInput->GetHoldMs(SJ_NUMPAD_HOLD_ERROR));
			return SJ_NUMPAD_DELETEME;
		}
		else if( state == SJ_NUMPAD_WAITING )
		{
			g_mainFrame->SetDisplayMsg(_("Track number") + wxString(wxT(": ")) + digits, m_numpadInput->GetHoldMs(SJ_NUMPAD_HOLD_NORMAL));
		}
		else if( state == SJ_NUMPAD_URL_PRESSED )
		{
			g_mainFrame->SetDisplayMsg(wxT(""), 0);
			g_mainFrame->Enqueue(urls, -1, TRUE);
			return SJ_NUMPAD_DELETEME;
		}
		else if( state == SJ_NUMPAD_ENTER_MENU )
		{
			return SJ_NUMPAD_MENU;
		}
	}
	else
	{
		// we forgot resetting the display message before V2.52 ...
		// see http://www.silverjuke.net/forum/post.php?p=4829#4829 - "Numpad cancel"
		g_mainFrame->SetDisplayMsg(_("Cancel"), m_numpadInput->GetHoldMs(SJ_NUMPAD_HOLD_ERROR));
		return SJ_NUMPAD_DELETEME;
	}

	return SJ_NUMPAD_VOID;
}



