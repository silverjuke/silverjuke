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
 * File:    numpad.cpp
 * Authors: Björn Petersen
 * Purpose: Numpad handling
 *
 *******************************************************************************
 *
 * Possible menu:
 *
 * 0=OK 1=? 2=abc 3=def 4=ghi 5=jkl 6=mno 7=pqrs 8=tuv 9=wxyz
 *
 * 1=Play  2=Pause  3=Stop  0=>
 * 1=Prev 2=Next  0=>
 * 1=Volume down  2=Volume up  0=>
 * 1=Repeat  2=Shuffle  0=>
 *
 * 1=End search  0=>
 *
 * 1=Clear playlist  0=>
 *
 * 1=Exit kiosk mode 0=>
 *
 * 1=Cancel  0=>
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/browser.h>
#include <sjmodules/kiosk/numpad.h>
#include <sjmodules/kiosk/numpadplay.h>
#include <sjmodules/kiosk/numpadmenu.h>
#include <sjmodules/kiosk/numpadsearch.h>
#include <sjmodules/kiosk/numpadpassword.h>
#include <sjmodules/kiosk/kiosk.h>
#include <sjmodules/vis/vis_module.h>


/*******************************************************************************
 * Constructor / Destructor
 ******************************************************************************/


SjNumpadInput::SjNumpadInput()
{
	m_currHandler = NULL;

	int i;
	for( i = 0; i < SJ_NUMPAD_HOLD_COUNT; i++ )
	{
		m_holdSeconds_DontUse[i] = -1; // use GetHoldMs() to retrieve the value
	}
}


SjNumpadInput::~SjNumpadInput()
{
	if( m_currHandler )
	{
		delete m_currHandler;
		m_currHandler = NULL;
	}
}


/*******************************************************************************
 * Global Column / Album Information
 ******************************************************************************/


long SjNumpadInput::GetAlbumCount() const
{
	if( g_mainFrame )
	{
		return g_mainFrame->m_columnMixer.GetUnmaskedColCount();
	}
	else
	{
		return 0;
	}
}


long SjNumpadInput::GetNumDigits(long count) const
{
	if( count < 100 )          { return 2; }
	else if( count < 1000 )         { return 3; }
	else if( count < 10000 )        { return 4; }
	else if( count < 100000 )       { return 5; }
	else if( count < 1000000 )      { return 6; }
	else if( count < 10000000 )     { return 7; }
	else                            { return 8; }
}


wxString SjNumpadInput::DbAlbumIndex2String(long dbAlbumIndex) const
{
	wxString numDigits = wxString::Format(wxT("%i"), (int)GetNumAlbumDigits());
	return wxString::Format(wxT("%0")+numDigits+wxT("i"), (int)DbAlbumIndex2Number(dbAlbumIndex));
}


void SjNumpadInput::FinalizeTrackNumber(wxString& trackNumber, long maxTrackNumber)
{
	long numDigits = GetNumDigits(maxTrackNumber);
	while( (long)trackNumber.Len() < numDigits )
	{
		trackNumber.Prepend(wxT('0'));
	}
	trackNumber.Prepend(wxT('-'));
}


long SjNumpadInput::UnfinalizeTrackNumber(const wxString& trackNumber) const
{
	long ret = 0;
	if( trackNumber.Len() )
	{
		if( trackNumber[0] == '-' )
		{
			trackNumber.Mid(1).ToLong(&ret, 10);
		}
		else
		{
			trackNumber.ToLong(&ret, 10);
		}
	}
	return ret;
}


/*******************************************************************************
 * Settings
 ******************************************************************************/


static const long s_defHoldSeconds[SJ_NUMPAD_HOLD_COUNT] =
{
	20, // SJ_NUMPAD_HOLD_NORMAL
	2,  // SJ_NUMPAD_HOLD_ERROR
	35, // SJ_NUMPAD_HOLD_TEXTENTRY
	3   // SJ_NUMPAD_HOLD_CHARENTRY
};


long SjNumpadInput::GetHoldSeconds(int i)
{
	wxASSERT( i >= 0 && i < SJ_NUMPAD_HOLD_COUNT );

	if( m_holdSeconds_DontUse[i] == -1 )
	{
		m_holdSeconds_DontUse[i] = g_tools->m_config->Read(wxString::Format(wxT("kiosk/numpadHold%i"), (int)i),
		                           s_defHoldSeconds[i]);
	}

	return m_holdSeconds_DontUse[i];
}


void SjNumpadInput::SetHoldSeconds(int i, long seconds)
{
	wxASSERT( i >= 0 && i < SJ_NUMPAD_HOLD_COUNT );

	m_holdSeconds_DontUse[i] = seconds;
	g_tools->m_config->Write(wxString::Format(wxT("kiosk/numpadHold%i"), (int)i), m_holdSeconds_DontUse[i]);
}


/*******************************************************************************
 * Handling User Input
 ******************************************************************************/


void SjNumpadInput::KeyPressed(long idt)
{
	if( g_mainFrame==NULL || g_accelModule==NULL || g_kioskModule==NULL )
	{
		// in silverjuke contruction/destruction?
		return;
	}

	g_mainFrame->GotBrowserInputFromUser();

	// handler-independent navigation
	if( idt == IDT_NUMPAD_PAGE_LEFT
	        || idt == IDT_NUMPAD_PAGE_RIGHT
	        || idt == IDT_NUMPAD_PAGE_UP
	        || idt == IDT_NUMPAD_PAGE_DOWN )
	{
		bool upDefined = g_accelModule->IsShortcutDefined(IDT_NUMPAD_PAGE_UP);
		bool downDefined = g_accelModule->IsShortcutDefined(IDT_NUMPAD_PAGE_DOWN);

		if( idt == IDT_NUMPAD_PAGE_UP && !downDefined )
		{
			idt = IDT_NUMPAD_PAGE_DOWN;
			upDefined = FALSE;
		}

		Navigate(idt, !upDefined/*wrap?*/);
		return;
	}

	// create the "play" handler if not handler is yet loaded
	if( m_currHandler == NULL )
	{
		m_currHandler = new SjNumpadPlayHandler(this);
	}

	// send the key to the current handler
	SjNumpadHandlerId newHandlerId = m_currHandler->KeyPressed(idt);
	if( newHandlerId != SJ_NUMPAD_VOID )
	{
		delete m_currHandler;
		m_currHandler = NULL;

		switch( newHandlerId )
		{
			case SJ_NUMPAD_PLAY:    m_currHandler = new SjNumpadPlayHandler(this);      break;
			case SJ_NUMPAD_MENU:    m_currHandler = new SjNumpadMenuHandler(this);      break;
			case SJ_NUMPAD_SEARCH:  m_currHandler = new SjNumpadSearchHandler(this);    break;
			case SJ_NUMPAD_PASSWORD:m_currHandler = new SjNumpadPasswordHandler(this);  break;
			default:                /*no handler*/                                      break;
		}
	}
}


void SjNumpadInput::Navigate(int idt, bool wrapVscroll)
{
	wxASSERT( idt == IDT_NUMPAD_PAGE_LEFT || idt == IDT_NUMPAD_PAGE_RIGHT || idt == IDT_NUMPAD_PAGE_UP || idt == IDT_NUMPAD_PAGE_DOWN );

	g_visModule->StopVisIfOverWorkspace();

	SjSkinValue dummy;
	if( idt == IDT_NUMPAD_PAGE_LEFT || idt == IDT_NUMPAD_PAGE_RIGHT )
	{
		// ...scroll horizontally one page left or right
		g_mainFrame->OnSkinTargetEvent(idt == IDT_NUMPAD_PAGE_LEFT? IDT_WORKSPACE_PAGE_LEFT : IDT_WORKSPACE_PAGE_RIGHT, dummy, 0);

		// ...reset vertical scrolling when scrolling horizontally
		g_mainFrame->OnSkinTargetEvent(IDT_WORKSPACE_MINOR_HOME, dummy, 0);
	}
	else
	{
		// ...scroll vertically one page up or down
		//    or reset vertical scrolling if we're already abottom and no "up" key is available
		if( wrapVscroll
		        && g_mainFrame->m_browser->IsVScrollAbottom() )
		{
			g_mainFrame->OnSkinTargetEvent(IDT_WORKSPACE_MINOR_HOME, dummy, 0);
		}
		else
		{
			g_mainFrame->OnSkinTargetEvent(idt == IDT_NUMPAD_PAGE_DOWN? IDT_WORKSPACE_PAGE_DOWN : IDT_WORKSPACE_PAGE_UP, dummy, 0);
		}
	}
}


void SjNumpadInput::DisplayTimeout()
{
	if( m_currHandler )
	{
		delete m_currHandler;
		m_currHandler = NULL;
	}
}

