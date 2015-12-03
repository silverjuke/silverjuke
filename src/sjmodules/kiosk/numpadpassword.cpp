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
#include <sjmodules/kiosk/numpadpassword.h>
#include <sjmodules/kiosk/kiosk.h>


SjNumpadPasswordHandler::SjNumpadPasswordHandler(SjNumpadInput* numpadInput)
	: SjNumpadTextEntryHandler(numpadInput, SJ_NUMPAD_PASSWORD, _("Password:"), TRUE)
{
}


SjNumpadHandlerId SjNumpadPasswordHandler::TextEntryDone(const wxString& text)
{
	if( !text.IsEmpty()
	        &&  g_kioskModule->ExitRequest(SJ_KIOSKF_EXIT_KEY|SJ_KIOSKF_EXIT_CORNER, &text) )
	{
		g_mainFrame->ClearDisplayMsg();
	}
	else
	{
		g_mainFrame->SetDisplayMsg(_("Invalid password. Please try again."),
		                           m_numpadInput->GetHoldMs(SJ_NUMPAD_HOLD_ERROR));
	}
	return SJ_NUMPAD_DELETEME;
}



