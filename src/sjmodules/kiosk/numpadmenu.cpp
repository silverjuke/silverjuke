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
 * File:    numpadmenu.cpp
 * Authors: Björn Petersen
 * Purpose: The numpad handler for the menu
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/kiosk/numpad.h>
#include <sjmodules/kiosk/numpadmenu.h>
#include <sjmodules/kiosk/kiosk.h>
#include <sjmodules/vis/vis_module.h>

#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArrayNumpadMenuPage);


/*******************************************************************************
 * IDs
 ******************************************************************************/


#define IDM_ENDSIMPLESEARCH (IDM_FIRSTPRIVATE+0)


/*******************************************************************************
 * SjNumpadMenuHandler
 ******************************************************************************/


SjNumpadMenuHandler::SjNumpadMenuHandler(SjNumpadInput* numpadInput)
	: SjNumpadHandler(numpadInput, SJ_NUMPAD_MENU)
{
	m_0isNext = TRUE;
	UpdatePages();

	if( m_pages.GetCount() == 0 )
	{
		g_mainFrame->SetDisplayMsg(wxT(""), 0);
	}
	else
	{
		m_menuPage = m_pages[0].m_pageId;
		SetDisplayMsg(m_menuPage, m_0isNext);
	}
}


void SjNumpadMenuHandler::AddEntry(int key, int action, int flags, const wxChar* title)
{
	int i = m_pages.GetCount()-1;
	wxASSERT( i>= 0 );
	wxASSERT( key >= 0 && key <= 9 );
	if( i >= 0 )
	{
		m_pages[i].m_entries[key].m_action  = action;
		m_pages[i].m_entries[key].m_flags   = flags;
		m_pages[i].m_entries[key].m_title   = title? wxString(title) : g_accelModule->GetCmdNameById(action, TRUE/*shortName*/);
	}
}


void SjNumpadMenuHandler::UpdatePages()
{
	m_pages.Empty();

	// search
	if( g_mainFrame->IsOpAvailable(SJ_OP_SEARCH)
	        || g_mainFrame->HasSimpleSearch() )
	{
		int currKey = 1;
		AddPage (IDT_SEARCH);
		if( g_mainFrame->IsOpAvailable(SJ_OP_SEARCH) )
		{
			AddEntry(currKey++, IDT_SEARCH, 0, _("Search"));
		}
		AddEntry(currKey++, IDM_ENDSIMPLESEARCH, 0, _("End search"));
	}

	// zoom / scroll
	bool zoomPageStarted = false;
	if( g_mainFrame->IsOpAvailable(SJ_OP_ZOOM) )
	{
		AddPage (IDT_ZOOM_OUT);
		AddEntry(1, IDT_ZOOM_OUT,   SJ_ENTRY_HOLD);
		AddEntry(2, IDT_ZOOM_IN,    SJ_ENTRY_HOLD);
		zoomPageStarted = true;
	}

	if( !(g_accelModule->IsShortcutDefined(IDT_NUMPAD_PAGE_UP) || g_accelModule->IsShortcutDefined(IDT_NUMPAD_PAGE_DOWN))
	        ||  !g_accelModule->IsShortcutDefined(IDT_NUMPAD_PAGE_LEFT)
	        ||  !g_accelModule->IsShortcutDefined(IDT_NUMPAD_PAGE_RIGHT) )
	{
		if( !zoomPageStarted )
			AddPage (IDT_ZOOM_OUT);
		AddEntry(3, IDT_NUMPAD_PAGE_DOWN,   SJ_ENTRY_HOLD, _("Down"));
		AddEntry(4, IDT_NUMPAD_PAGE_LEFT,   SJ_ENTRY_HOLD, _("Left"));
		AddEntry(5, IDT_NUMPAD_PAGE_RIGHT,  SJ_ENTRY_HOLD, _("Right"));
		zoomPageStarted = true;
	}

	// volume / play / pause / stop
	if( g_mainFrame->IsOpAvailable(SJ_OP_MAIN_VOL)
	        || g_mainFrame->IsOpAvailable(SJ_OP_PLAYPAUSE) )
	{
		AddPage (IDT_MAIN_VOL_UP);
		if( g_mainFrame->IsOpAvailable(SJ_OP_MAIN_VOL) )
		{
			AddEntry(1, IDT_MAIN_VOL_DOWN, SJ_ENTRY_HOLD);
			AddEntry(2, IDT_MAIN_VOL_UP, SJ_ENTRY_HOLD);
		}
		if( g_mainFrame->IsOpAvailable(SJ_OP_PLAYPAUSE) )
		{
			AddEntry(3, IDT_PLAY, 0, g_mainFrame->IsPlaying()? _("Pause") : _("Play"));
			AddEntry(4, IDT_STOP);
		}
	}

	// prev / next / unqueue all
	if( g_mainFrame->IsOpAvailable(SJ_OP_EDIT_QUEUE) )
	{
		AddPage (IDT_PREV);
		AddEntry(1, IDT_PREV, SJ_ENTRY_HOLD);
		AddEntry(2, IDT_NEXT, SJ_ENTRY_HOLD);
	}

	if( g_mainFrame->IsOpAvailable(SJ_OP_UNQUEUE) )
	{
		AddEntry(3, IDT_UNQUEUE_ALL);
	}

	// shuffle / repeat
	if( g_mainFrame->IsOpAvailable(SJ_OP_EDIT_QUEUE) || g_mainFrame->IsOpAvailable(SJ_OP_REPEAT) )
	{
		AddPage (IDT_SHUFFLE);
		if( g_mainFrame->IsOpAvailable(SJ_OP_EDIT_QUEUE) )
		{
			AddEntry(1, IDT_SHUFFLE, SJ_ENTRY_HOLD);
		}
		if( g_mainFrame->IsOpAvailable(SJ_OP_REPEAT) )
		{
			AddEntry(2, IDT_REPEAT, SJ_ENTRY_HOLD);
		}
	}

	// end search / vis. / kiosk
	AddPage (IDT_TOGGLE_KIOSK);
	if(  g_mainFrame->IsOpAvailable(SJ_OP_STARTVIS)
	 || (g_visModule->IsVisStarted() && g_visModule->IsOverWorkspace()) )
	{
		AddEntry(1, IDT_START_VIS, 0, _("Video screen"));
	}
	AddEntry(2, IDT_TOGGLE_KIOSK, 0, g_mainFrame->IsKioskStarted()? wxString(_("Exit kiosk mode")) : wxString(_("Start kiosk mode")));
}


SjNumpadMenuPage* SjNumpadMenuHandler::FindPage(int pageId, bool findNext)
{
	int i, iCount = m_pages.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		if( m_pages[i].m_pageId == pageId )
		{
			if( findNext )
			{
				return (i < iCount-1)? &(m_pages[i+1]) : NULL;
			}
			else
			{
				return &(m_pages[i]);
			}
		}
	}
	return NULL;
}


void SjNumpadMenuHandler::SetDisplayMsg(int pageId, bool nullIsNext)
{
	wxString msg;
	SjNumpadMenuPage* page = FindPage(pageId);
	if( page != NULL )
	{
		int e;
		for( e = 0; e <= 9; e++ )
		{
			if( page->m_entries[e].m_action )
			{
				if( !msg.IsEmpty() ) msg += wxT("  ");
				msg += wxString::Format(wxT("%i=%s"), (int)e, page->m_entries[e].m_title.c_str());
			}
		}

		if( !msg.IsEmpty() )
		{
			if( nullIsNext )
			{
				msg += wxT("\n0=>");
			}
			else
			{
				msg += wxT("\n0=") + wxString(_("OK"));
			}
		}
	}

	g_mainFrame->SetDisplayMsg(msg, msg.IsEmpty()? 0 : m_numpadInput->GetHoldMs(SJ_NUMPAD_HOLD_NORMAL));
}


SjNumpadHandlerId SjNumpadMenuHandler::ExitMenu()
{
	g_mainFrame->SetDisplayMsg(wxT(""), 0);
	return SJ_NUMPAD_DELETEME;
}


SjNumpadHandlerId SjNumpadMenuHandler::KeyPressed(int idt)
{
	UpdatePages();
	SjNumpadMenuPage* page = FindPage(m_menuPage);

	if(  page
	        && (idt >= IDT_NUMPAD_0 && idt <= IDT_NUMPAD_9) )
	{
		int action = page->m_entries[idt-IDT_NUMPAD_0].m_action;
		int flags  = page->m_entries[idt-IDT_NUMPAD_0].m_flags;
		if( action )
		{
			if( action == IDT_TOGGLE_KIOSK
			        && g_mainFrame->IsKioskStarted()
			        && g_kioskModule->IsPasswordInUse() )
			{
				return SJ_NUMPAD_PASSWORD;
			}
			else if( action == IDT_SEARCH )
			{
				return SJ_NUMPAD_SEARCH;
			}
			else if( action == IDM_ENDSIMPLESEARCH )
			{
				g_mainFrame->EndSimpleSearch();
			}
			else if( action == IDT_NUMPAD_PAGE_LEFT || action == IDT_NUMPAD_PAGE_RIGHT || action == IDT_NUMPAD_PAGE_DOWN )
			{
				m_numpadInput->Navigate(action, TRUE);
			}
			else if( action == IDT_SHUFFLE )
			{
				if( g_mainFrame->IsOpAvailable(SJ_OP_EDIT_QUEUE) )  // we do not use the default section for IDT_SHUFFLE, as this also calls SetDisplayMsg() which
					g_mainFrame->ToggleShuffle();                   // results in a little bit of flickering
			}
			else if( action == IDT_REPEAT )
			{
				if( !g_mainFrame->IsKioskStarted() )                // we do not use the default section for IDT_REPEAT, as this also calls SetDisplayMsg() which
					g_mainFrame->ToggleRepeat();                    // results in a little bit of flickering
			}
			else
			{
				SjSkinValue value;
				g_mainFrame->OnSkinTargetEvent(action, value, 0);
			}

			if( flags & SJ_ENTRY_HOLD )
			{
				m_0isNext = FALSE;
				SetDisplayMsg(m_menuPage, m_0isNext);
				return SJ_NUMPAD_VOID;
			}
			else
			{
				return ExitMenu();
			}
		}
	}

	if( (idt == IDT_NUMPAD_0 || idt == IDT_NUMPAD_MENU)
	        &&  m_0isNext )
	{
		page = FindPage(m_menuPage, TRUE/*findNext*/);
		if( page == NULL )
		{
			return ExitMenu();
		}

		m_menuPage = page->m_pageId;
		SetDisplayMsg(m_menuPage, m_0isNext);
		return SJ_NUMPAD_VOID;
	}

	return ExitMenu();
}


