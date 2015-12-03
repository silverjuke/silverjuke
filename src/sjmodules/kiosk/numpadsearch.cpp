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
 * File:    numpadtextentry.cpp
 * Authors: Björn Petersen
 * Purpose: The numpad handler for text input
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/kiosk/numpad.h>
#include <sjmodules/kiosk/numpadsearch.h>


SjNumpadSearchHandler::SjNumpadSearchHandler(SjNumpadInput* numpadInput)
	: SjNumpadTextEntryHandler(numpadInput, SJ_NUMPAD_SEARCH, _("Search")+wxString(wxT(":")), FALSE)
{
	g_mainFrame->EndSimpleSearch();
}


void SjNumpadSearchHandler::TextUpdate(const wxString& text)
{
	if( text.Len() == 0 )
	{
		// end search
		g_mainFrame->EndSimpleSearch();
		return; // done
	}
	else if( text.Len() == 1 )
	{
		wxChar firstChar = text.Lower()[0u];
		if( firstChar >= 'a' && firstChar <= 'z' )
		{
			// goto albums starting with ...
			SjSkinValue value;
			g_mainFrame->OnSkinTargetEvent(IDT_WORKSPACE_GOTO_A+(firstChar-'a'), value, 0);
			return; // done
		}
	}

	// default: simple search
	g_mainFrame->SetSearch(SJ_SETSEARCH_SETSIMPLE, text);
}


SjNumpadHandlerId SjNumpadSearchHandler::TextEntryDone(const wxString& text)
{
	g_mainFrame->ClearDisplayMsg();
	return SJ_NUMPAD_DELETEME;
}


