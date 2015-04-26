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
 * File:    tageditorfreedb.h
 * Authors: Björn Petersen
 * Purpose: Tag editor freedb plugin
 *
 ******************************************************************************/


#ifndef __SJ_TAGEDITOR_FREEDB_H__
#define __SJ_TAGEDITOR_FREEDB_H__


#include "freedb.h"
#include <wx/socket.h>


class SjFreedbPlugin : public SjTagEditorPlugin
{
public:
	                SjFreedbPlugin      (wxWindow* parent, SjTrackInfo* exampleTrackInfo, const wxArrayString& urls);

	bool            PrepareModify       ();
	void            ModifyTrackInfo     (SjTrackInfo&, int index, SjModifyInfo&);
	wxString        PostModify          ();

private:
	wxArrayString   m_urls;
	SjFreedbQuery   m_freedbQuery;
	wxStaticText*   m_msgTextCtrl;
	wxListBox*      m_listBox;
	bool            m_okHit;

	void            FillListbox         (bool alsoAddSecondary);

	void            OnMyInitDialog      (wxInitDialogEvent&);
	void            OnListboxDblClick   (wxCommandEvent&) { if(m_listBox->GetSelection()>=0) OnMyOkDo(); }
	void            OnMyOkEvent         (wxCommandEvent&) { OnMyOkDo(); }
	void            OnMyOkDo            ();
	void            OnSocketEvent       (wxSocketEvent&);
	DECLARE_EVENT_TABLE ()
};


#endif // __SJ_TAGEDITOR_FREEDB_H__
