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
 * File:    numpadtextentry.h
 * Authors: Björn Petersen
 * Purpose: The numpad handler for text input
 *
 ******************************************************************************/




#ifndef __SJ_NUMPADTEXTENTRY_H___
#define __SJ_NUMPADTEXTENTRY_H___



class SjNumpadTextEntryTimer;



class SjNumpadTextEntryHandler : public SjNumpadHandler
{
public:
	SjNumpadTextEntryHandler
	(SjNumpadInput*, SjNumpadHandlerId, const wxString& label, bool password);
	~SjNumpadTextEntryHandler
	();
	SjNumpadHandlerId
	KeyPressed          (int idt);


protected:
	// these function is called if the text input is changed/is ready,
	// should be implemented by derived classes
	virtual void    TextUpdate          (const wxString& text) { };
	virtual SjNumpadHandlerId
	TextEntryDone       (const wxString& text) = 0;

private:
	SjNumpadTextEntryTimer*
	m_timer;
	bool            m_password;
	wxString        m_label;
	wxString        m_text;
	int             m_lastKeyHit;
	void            GetChars            (int key/*2-9*/, wxString& base, wxString& add);
	void            SetDisplayMsg       ();

	// helper for text updates
	void            CallTextUpdate      ();
	wxString        m_textUpdateSend;

	friend class    SjNumpadTextEntryTimer;

};



#endif // __SJ_NUMPADTEXTENTRY_H___


