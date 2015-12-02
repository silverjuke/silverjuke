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
 * File:    numpadmenu.h
 * Authors: Björn Petersen
 * Purpose: The numpad handler for the menu
 *
 ******************************************************************************/


#ifndef __SJ_NUMPADMENU_H___
#define __SJ_NUMPADMENU_H___


class SjNumpadMenuEntry
{
public:
	                SjNumpadMenuEntry   () { m_action = 0; m_flags = 0; }
	int             m_action;
	#define         SJ_ENTRY_HOLD   0x01
	int             m_flags;
	wxString        m_title;
};


class SjNumpadMenuPage
{
public:
	                SjNumpadMenuPage    (int pageId) { m_pageId = pageId; }
	int             m_pageId;
	SjNumpadMenuEntry
	m_entries[10];
};

WX_DECLARE_OBJARRAY(SjNumpadMenuPage, SjArrayNumpadMenuPage);


class SjNumpadMenuHandler : public SjNumpadHandler
{
public:
	                   SjNumpadMenuHandler (SjNumpadInput*);
	SjNumpadHandlerId  KeyPressed          (int idt);

private:
	void               UpdatePages         ();
	void               AddPage             (int pageId) { m_pages.Add(new SjNumpadMenuPage(pageId)); }
	void               AddEntry            (int key, int action, int flags=0, const wxChar* title=NULL);
	SjNumpadMenuPage*  FindPage            (int pageId, bool findNext=FALSE);
	void               SetDisplayMsg       (int pageId, bool nullIsNext);

	bool               m_0isNext;
	SjArrayNumpadMenuPage m_pages;
	int                m_menuPage;
	SjNumpadHandlerId  ExitMenu            ();
};


#endif // __SJ_NUMPADMENU_H___

