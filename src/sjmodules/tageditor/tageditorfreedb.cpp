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
 * File:    tageditorfreedb.cpp
 * Authors: Björn Petersen
 * Purpose: Tag editor freedb plugin
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/tageditor/tageditor.h>
#include <sjmodules/tageditor/tageditorplugin.h>
#include <sjmodules/tageditor/tageditorfreedb.h>


/*******************************************************************************
 * SjFreedbPlugin
 ******************************************************************************/


#define IDC_SOCKET                  (IDM_FIRSTPRIVATE+2)
#define IDC_LISTBOX                 (IDM_FIRSTPRIVATE+3)


BEGIN_EVENT_TABLE(SjFreedbPlugin, SjTagEditorPlugin)
	EVT_INIT_DIALOG     (               SjFreedbPlugin::OnMyInitDialog      )
	EVT_BUTTON          (wxID_OK,       SjFreedbPlugin::OnMyOkEvent         )
	EVT_SOCKET          (IDC_SOCKET,    SjFreedbPlugin::OnSocketEvent       )
	EVT_LISTBOX_DCLICK  (IDC_LISTBOX,   SjFreedbPlugin::OnListboxDblClick   )
END_EVENT_TABLE()


SjFreedbPlugin::SjFreedbPlugin(wxWindow* parent, SjTrackInfo* exampleTrackInfo, const wxArrayString& urls)
	: SjTagEditorPlugin(parent, wxT("freedb"), _("Query online database"), exampleTrackInfo, SJ_HELP_TAGEDITOR_FREEDB)
{
	m_okHit = FALSE;
	m_urls = urls;

	// create dialog
	m_sizer1->Add(SJ_DLG_SPACE, SJ_DLG_SPACE); // some space

	m_msgTextCtrl = new wxStaticText(this, -1, wxString::Format(_("Loading %s"), SjFreedbQuery::GetFreedbHost().c_str()));
	m_sizer1->Add(m_msgTextCtrl, 0, wxALL|wxGROW, SJ_DLG_SPACE);

	m_listBox = new wxListBox(this, IDC_LISTBOX,
	                          wxDefaultPosition, wxSize(380, 120));
	m_sizer1->Add(m_listBox, 1, wxALL|wxGROW, SJ_DLG_SPACE);

}


void SjFreedbPlugin::OnMyInitDialog(wxInitDialogEvent&)
{
	wxWindowDisabler disabler;
	wxBusyCursor busy;

	if( !m_freedbQuery.Query(m_urls, this, IDC_SOCKET) )
	{
		m_msgTextCtrl->SetLabel(m_freedbQuery.GetError());
	}
}


void SjFreedbPlugin::OnMyOkDo()
{
	// make sure, "OK" is only pressed once
	if( m_okHit ) return;
	m_okHit = TRUE;

	// transfer data to window
	// (needed to save the size)
	TransferDataFromWindow();

	// anything to select?
	if( !m_freedbQuery.IsQueryDone()
	        ||  m_freedbQuery.GetPossibleResultCount() <= 0 )
	{
		EndModal(wxID_OK);
		return;
	}

	// select the result
	long selectedIndex = m_listBox->GetSelection();
	if( selectedIndex >= 0 )
	{
		selectedIndex = (long)m_listBox->GetClientData(selectedIndex);
		if( selectedIndex == -1 )
		{
			// "more..." clicked
			FillListbox(TRUE);
			m_okHit = FALSE;
			return;
		}
	}

	if( selectedIndex < 0 || selectedIndex >= m_freedbQuery.GetPossibleResultCount() )
	{
		EndModal(wxID_OK);
		return;
	}

	m_msgTextCtrl->SetLabel(wxString::Format(_("Loading %s"), SjFreedbQuery::GetFreedbHost().c_str()));
	if( !m_freedbQuery.SelectResult(selectedIndex) )
	{
		wxLogError(m_freedbQuery.GetError());
		EndModal(wxID_OK);
		return;
	}
	// continued in (**)
}


void SjFreedbPlugin::FillListbox(bool alsoAddSecondary)
{
	m_listBox->Clear();

	int i, iCount = m_freedbQuery.GetPossibleResultCount();
	bool hasSecondary = FALSE;
	wxASSERT( iCount );
	for( i = 0; i < iCount; i++ )
	{
		const SjFreedbPossibleResult* currResult = m_freedbQuery.GetPossibleResult(i);

		if( !currResult->m_isSecondary
		        ||  alsoAddSecondary )
		{
			m_listBox->Append(currResult->m_discName, (void*)(uintptr_t)i);
		}
		else
		{
			hasSecondary = TRUE;
		}
	}

	if( hasSecondary )
	{
		m_listBox->Append(_("more..."), (void*)-1);
	}

	if( !alsoAddSecondary )
	{
		m_listBox->SetSelection(0);
	}
	// else: do not set the selection
}


void SjFreedbPlugin::OnSocketEvent(wxSocketEvent&)
{
	bool queryDoneBefore = m_freedbQuery.IsQueryDone();
	bool resultSelectedBefore = m_freedbQuery.IsResultSelected();
	m_freedbQuery.OnHttpEvent();

	if( !queryDoneBefore
	        &&  m_freedbQuery.IsQueryDone() )
	{
		// query just done
		if( m_freedbQuery.HasErrors() )
		{
			m_msgTextCtrl->SetLabel(m_freedbQuery.GetError());
		}
		else
		{
			m_msgTextCtrl->SetLabel(_("Please select one of the following albums:"));

			FillListbox(FALSE);
		}
	}
	else if( !resultSelectedBefore
	         &&  m_freedbQuery.IsResultSelected() )
	{
		// result just selected - continued from (**)
		if( m_freedbQuery.HasErrors() )
		{
			wxLogError(m_freedbQuery.GetError());
		}
		EndModal(wxID_OK);
	}
}


bool SjFreedbPlugin::PrepareModify()
{
	if(  m_freedbQuery.HasErrors()
	        || !m_freedbQuery.IsResultSelected() )
	{
		return FALSE;
	}

	return TRUE;
}


void SjFreedbPlugin::ModifyTrackInfo(SjTrackInfo& retTi, int index, SjModifyInfo& mod)
{
	const SjFreedbTrack* freedbTrack = m_freedbQuery.GetResultingTrack(retTi.m_url);
	if( freedbTrack == NULL )
	{
		return;
	}

	if( !freedbTrack->m_freedbGenre.IsEmpty() )     mod.Add(SJ_TI_GENRENAME, freedbTrack->m_freedbGenre);
	if( !freedbTrack->m_freedbArtist.IsEmpty() )    mod.Add(SJ_TI_LEADARTISTNAME, freedbTrack->m_freedbArtist);
	if( !freedbTrack->m_freedbAlbum.IsEmpty() )     mod.Add(SJ_TI_ALBUMNAME, freedbTrack->m_freedbAlbum);
	if( !freedbTrack->m_freedbTitle.IsEmpty() )     mod.Add(SJ_TI_TRACKNAME, freedbTrack->m_freedbTitle);
	if(  freedbTrack->m_freedbYear )                mod.Add(SJ_TI_YEAR, freedbTrack->m_freedbYear);
	if(  freedbTrack->m_freedbTrackNr )             mod.Add(SJ_TI_TRACKNR, freedbTrack->m_freedbTrackNr);
	if(  freedbTrack->m_freedbTrackCount )          mod.Add(SJ_TI_TRACKCOUNT, freedbTrack->m_freedbTrackCount);
}


wxString SjFreedbPlugin::PostModify()
{
	return wxT("");
}




