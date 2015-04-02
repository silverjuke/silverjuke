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
 * File:    htmlwindowsj.h
 * Authors: Björn Petersen
 * Purpose: The html window used in the help and in the tag editor
 *
 ******************************************************************************/



#ifndef __SJ_HTMLWINDOW_H__
#define __SJ_HTMLWINDOW_H__


#include <wx/html/htmlwin.h>


class SjHtmlWindow : public wxHtmlWindow
{
public:
	SjHtmlWindow        (wxWindow* parent, int id, const wxPoint& pos, const wxSize& size, long style = wxHW_SCROLLBAR_AUTO);

	void            InitPage            ();
	void            AddPropToPage       (SjProp& prop);

	wxString        GetClickedLink      () { return m_clickedLink; }

private:
	void            OnLinkClicked       (const wxHtmlLinkInfo& link);
	wxString        m_clickedLink;
};




#endif // __SJ_HTMLWINDOW_H__
