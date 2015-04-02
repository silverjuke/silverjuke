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
#include <sjmodules/kiosk/numpadtextentry.h>


/*******************************************************************************
 * SjNumpadTextEntryTimer
 ******************************************************************************/


class SjNumpadTextEntryTimer : public wxTimer
{
public:
	SjNumpadTextEntryTimer
	(SjNumpadTextEntryHandler*);
	void            Notify              ();
	SjNumpadTextEntryHandler*
	m_handler;
	unsigned long   m_nextCharTimestamp;
};


SjNumpadTextEntryTimer::SjNumpadTextEntryTimer(SjNumpadTextEntryHandler* handler)
	: wxTimer()
{
	m_handler = handler;
	m_nextCharTimestamp = 0/*pause*/;
}


void SjNumpadTextEntryTimer::Notify()
{
	if( m_handler && m_nextCharTimestamp )
	{
		if( SjTools::GetMsTicks() > m_nextCharTimestamp )
		{
			m_nextCharTimestamp = 0;
			m_handler->m_lastKeyHit = -1;
			m_handler->SetDisplayMsg();
			m_handler->CallTextUpdate();
		}
	}
}


/*******************************************************************************
 * SjNumpadTextEntryHandler
 ******************************************************************************/


SjNumpadTextEntryHandler::SjNumpadTextEntryHandler(SjNumpadInput* numpadInput, SjNumpadHandlerId handlerId, const wxString& label, bool password)
	: SjNumpadHandler(numpadInput, handlerId)
{
	m_lastKeyHit = -1;
	m_label = label;
	m_password = password;

	m_timer = new SjNumpadTextEntryTimer(this);
	m_timer->Start(200/*5 times a second, the exact timeout is calculated using GetMsTicks()*/);

	SetDisplayMsg();
}


SjNumpadTextEntryHandler::~SjNumpadTextEntryHandler()
{
	if( m_timer )
	{
		m_timer->Stop();
		m_timer->m_handler = NULL;
		delete m_timer;
		m_timer = NULL;
	}
}


void SjNumpadTextEntryHandler::GetChars(int key/*2-9*/, wxString& base, wxString& add)
{
	switch( key )
	{
			// ANSI
		default:    base  = wxT("abc");
			add   = wxT("012")                          // numbers
			        wxT("\xE4\xE0\xE1\xE2\xE3\xE5\xE6") // accents for "a"
			        wxT("\xE7")                         // accents for "c"
			        wxT("-+*/.,:;!\xA1?\xBF\"'()[]{}|") // punctuation (placed here as people will search here first)
			        wxT("@<>_\\=%&\xA7#~$");            // other signs (                    - " -                   )
			break;

		case 3:     base  = wxT("def");
			add   = wxT("3")                            // numbers
			        wxT("\xEB\xE8\xE9\xEA")             // accents for "e"
			        wxT("\xF0");                        // accents for "d"
			break;

		case 4:     base  = wxT("ghi");
			add   = wxT("4")                            // numbers
			        wxT("\xEF\xEC\xED\xEE");            // accents for "i"
			break;

		case 5:     base  = wxT("jkl");
			add   = wxT("5")                            // numbers
			        wxT("\x80\xA2\xA3\xA5\xA4")         // currency signs (placed here as here is place...)
			        wxT("\xA9\xAE\x99");                // trade signs    (          - " -                )
			break;

		case 6:     base  = wxT("mno");
			add   = wxT("6")                            // numbers
			        wxT("\xF6\xF2\xF3\xF4\xF5\xF8\x9C") // accents for "o"
			        wxT("\xF1");                        // accents for "n"
			break;

		case 7:     base  = wxT("pqrs");
			add   = wxT("7")                            // numbers
			        wxT("\xDF\x9A");                    // accents for "s"
			break;

		case 8:     base  = wxT("tuv");
			add   = wxT("8")                            // numbers
			        wxT("\xFC\xF9\xFA\xFB")             // accents for "u"
			        wxT("\xFE");                        // accents for "t"
			break;

		case 9:     base  = wxT("wxyz");
			add   = wxT("9")                            // numbers
			        wxT("\xFF");                        // accents for "y"
			break;
	}
}


void SjNumpadTextEntryHandler::SetDisplayMsg()
{
	int i;
	wxString msg(m_label+wxT(" ")), possibleBase, possibleAdd;

	if( m_password && m_text.Len() > 0 )
	{
		if( m_lastKeyHit == -1 )
		{
			msg += wxString('*', m_text.Len());
		}
		else
		{
			msg += wxString('*', m_text.Len()-1) + m_text.Last();
		}
	}
	else
	{
		msg += m_text;
	}

	if( m_lastKeyHit == -1 )
	{
		msg += wxString((wxT("_")));
	}

	msg += wxT("\n0=OK 1=<");


	for( i = 2; i <= 9; i++ )
	{
		GetChars(i, possibleBase, possibleAdd);
		msg += wxString::Format(wxT(" %i="), i) + possibleBase;
	}

	g_mainFrame->SetDisplayMsg(msg, m_numpadInput->GetHoldMs(SJ_NUMPAD_HOLD_TEXTENTRY));
}


void SjNumpadTextEntryHandler::CallTextUpdate()
{
	// m_textUpdateSend is initially empty; this is okay
	// as we do not
	if( m_textUpdateSend != m_text )
	{
		TextUpdate(m_text);
		m_textUpdateSend = m_text;
	}
}


SjNumpadHandlerId SjNumpadTextEntryHandler::KeyPressed(int idt)
{
	// pause timer
	m_timer->m_nextCharTimestamp = 0;

	if( idt >= IDT_NUMPAD_2 && idt <= IDT_NUMPAD_9 )
	{
		// char input, timer will be restarted as there is no "return" here
		int         thisKeyHit = idt-IDT_NUMPAD_0;
		wxString    possibleBase, possibleAdd;
		GetChars(thisKeyHit, possibleBase, possibleAdd);
		wxString    possibleChars = possibleBase+possibleAdd+wxT(" ");

		if( possibleChars.Len() )
		{
			int     nextCharPos = 0;

			if( thisKeyHit == m_lastKeyHit && m_text.Len()>0 )
			{
				// remove last character from the text
				wxString lastChar = m_text.Last();
				m_text.Truncate(m_text.Len()-1);

				// get the next character in the string of possible characters
				int lastCharPos = possibleChars.Find(lastChar);
				if( lastCharPos != -1 && lastCharPos != (int)possibleChars.Len()-1 )
				{
					nextCharPos = lastCharPos+1;
				}
			}
			else
			{
				// another numpad key is pressed, keep the old one
				CallTextUpdate();
			}

			m_text += possibleChars.Mid(nextCharPos, 1);
			m_lastKeyHit = thisKeyHit;
			SetDisplayMsg();

			// start timer
			m_timer->m_nextCharTimestamp = SjTools::GetMsTicks()+m_numpadInput->GetHoldMs(SJ_NUMPAD_HOLD_CHARENTRY);
		}

		return SJ_NUMPAD_VOID;
	}
	else if( idt == IDT_NUMPAD_1 )
	{
		// "backspace", timer stays paused
		if( m_text.Len() > 0 )
		{
			m_text.Truncate(m_text.Len()-1);
			m_lastKeyHit = -1;

			CallTextUpdate();

			SetDisplayMsg();
		}
		return SJ_NUMPAD_VOID;
	}
	else if( idt == IDT_NUMPAD_0 )
	{
		// "ok", timer stays paused
		CallTextUpdate();
		return TextEntryDone(m_text);
	}
	else
	{
		// "cancel" or "menu", timer stays paused
		m_text.Empty();
		CallTextUpdate();
		return SJ_NUMPAD_DELETEME;
	}
}



