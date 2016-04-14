/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2016 Björn Petersen Software Design and Development
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
 * File:    upnp_scanner_dlg.cpp
 * Authors: Björn Petersen
 * Purpose: Using UPNP/DLNA devices
 *
 ******************************************************************************/


#include <sjbase/base.h>
#if SJ_USE_UPNP
#include <sjmodules/scanner/upnp_scanner.h>
#include <sjmodules/scanner/upnp_scanner_dlg.h>
#include <sjtools/msgbox.h>


SjUpnpDialog::SjUpnpDialog (wxWindow* parent, SjUpnpScannerModule* upnpModule, SjUpnpSource* upnpSource)
	: SjDialog(parent, "", SJ_MODAL, SJ_RESIZEABLE_IF_POSSIBLE)
{
	m_upnpModule          = upnpModule;
	m_upnpSource          = upnpSource; // may be NULL!
	m_isNew               = (upnpSource==NULL);
	m_stillLoading        = true;
	m_dirListFor          = NULL;
	m_stillScanningText   = NULL;
	m_mediaServerListCtrl = NULL;
	m_dirListCtrl         = NULL;

	wxString title;
	if( m_isNew ) {
		title = _("Add an UPnP/DLNA server");
	}
	else {
		title = wxString::Format(_("Options for \"%s\""), upnpSource->GetDisplayUrl().c_str());
	}
	title += " (beta)";
	SetTitle(title);


	// create dialog
	wxBoxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

		if( m_isNew )
		{
			wxBoxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);
			sizer1->Add(sizer2, 0, wxALL|wxGROW, SJ_DLG_SPACE);

				wxStaticText* staticText = new wxStaticText(this, -1, "1. "+_("Select server:"));
				sizer2->Add(staticText, 1, 0, SJ_DLG_SPACE);

				m_stillScanningText = new wxStaticText(this, -1, _("(still scanning)"));
				sizer2->Add(m_stillScanningText, 0, 0, SJ_DLG_SPACE);

			m_mediaServerListCtrl = new wxListCtrl(this, IDC_MEDIASERVERLISTCTRL, wxDefaultPosition, wxSize(380, SJ_DLG_SPACE*15), wxLC_REPORT | wxLC_SINGLE_SEL | wxSUNKEN_BORDER | wxLC_NO_HEADER);
			m_mediaServerListCtrl->SetImageList(g_tools->GetIconlist(FALSE), wxIMAGE_LIST_SMALL);
			m_mediaServerListCtrl->InsertColumn(0, _("Name"));
			sizer1->Add(m_mediaServerListCtrl, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);

			staticText = new wxStaticText(this, -1, "2. "+_("Select directory:"));
			sizer1->Add(staticText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);

			m_dirListCtrl = new wxListCtrl(this, IDC_DIRLISTCTRL, wxDefaultPosition, wxSize(380, SJ_DLG_SPACE*30), wxLC_REPORT | wxLC_SINGLE_SEL | wxSUNKEN_BORDER | wxLC_NO_HEADER);
			m_dirListCtrl->SetImageList(g_tools->GetIconlist(FALSE), wxIMAGE_LIST_SMALL);
			m_dirListCtrl->InsertColumn(0, _("Directory"));
			sizer1->Add(m_dirListCtrl, 1, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);
		}

	// buttons
	sizer1->Add(CreateButtons(SJ_DLG_OK_CANCEL), 0, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	// init done, center dialog
	UpdateMediaServerList();
	sizer1->SetSizeHints(this);
	CentreOnParent();
}


SjUpnpMediaServer* SjUpnpDialog::GetSelectedMediaServer()
{
	SjUpnpMediaServer* selMediaServer = NULL;
	if( m_mediaServerListCtrl )
	{
		long selIndex = GetSelListCtrlItem(m_mediaServerListCtrl);
		if( selIndex >= 0 )
		{
			selMediaServer = (SjUpnpMediaServer*)m_mediaServerListCtrl->GetItemData(selIndex);
		}
	}

	return selMediaServer;
}


SjUpnpDirEntry* SjUpnpDialog::GetSelectedDirEntry()
{
	if( m_dirListCtrl )
	{
		long selIndex = GetSelListCtrlItem(m_dirListCtrl);
		if( selIndex >= 0 && selIndex < m_dirListCtrl->GetItemCount() )
		{
			selIndex = m_dirListCtrl->GetItemData(selIndex);
			if( selIndex == -1 ) {
				if( m_parentIds.GetCount() <= 0 ) { return NULL; } // error
				return &m_parentDirEntry;
			}
			else if( selIndex >= 0 && selIndex < m_currDir.GetCount() ) {
				return m_currDir.Item(selIndex);
			}
		}
	}

	return NULL;
}


SjUpnpDirEntry* SjUpnpDialog::GetSelectedDir()
{
	SjUpnpDirEntry* sel;
	if( (sel = GetSelectedDirEntry())!=NULL )
	{
		if( sel->m_isDir ) {
			return sel;
		}
	}

	// no item or no directory is selected in the list control; return the parent directory
	// (this allows to selected
	m_parentDirEntry.m_objectId = m_currDir.m_objectId;
	m_parentDirEntry.m_dc_title = m_currDir.m_dc_title;
    return &m_parentDirEntry;
}


void SjUpnpDialog::UpdateMediaServerList()
{
	if( m_mediaServerListCtrl )
	{
		wxCriticalSectionLocker locker(m_upnpModule->m_mediaServerCritical);

		SjUpnpMediaServer* selMediaServer = GetSelectedMediaServer();

		m_mediaServerListCtrl->DeleteAllItems();

		SjHashIterator     iterator;
		wxString           udn;
		SjUpnpMediaServer* mediaServer;
		int i = 0;
		while( (mediaServer=(SjUpnpMediaServer*)m_upnpModule->m_mediaServerList.Iterate(iterator, udn))!=NULL ) {
			wxListItem li;
			li.SetId(i++);
			li.SetMask(wxLIST_MASK_IMAGE | wxLIST_MASK_TEXT);
			li.SetText(mediaServer->m_friendlyName);
			li.SetImage(SJ_ICON_UPNP_SERVER);
			li.SetData((void*)mediaServer); // the pointers can be used savely, see comment in UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE
			int new_i = m_mediaServerListCtrl->InsertItem(li);
			if( mediaServer == selMediaServer ) {
				m_mediaServerListCtrl->SetItemState(new_i, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
			}
		}
	}
}


void SjUpnpDialog::UpdateDirList(const wxString& selId)
{
	if( m_dirListCtrl )
	{
		m_dirListCtrl->DeleteAllItems();

		long zero_based_pos = 0;
		if( m_currDir.m_objectId!="0" ) {
			wxListItem li;
			li.SetId(zero_based_pos);
			li.SetMask(wxLIST_MASK_IMAGE | wxLIST_MASK_TEXT);
			li.SetText("..");
			li.SetImage(SJ_ICON_EMPTY);
			li.SetData(-1);
			m_dirListCtrl->InsertItem(li);

			zero_based_pos++;
		}

		int i, cnt = m_currDir.GetCount();
		for( i = 0; i < cnt; i++ )
		{
			SjUpnpDirEntry* entry = m_currDir.Item(i);

			wxListItem li;
			li.SetId(zero_based_pos);
			li.SetMask(wxLIST_MASK_IMAGE | wxLIST_MASK_TEXT);
			li.SetText(entry->m_dc_title);
			li.SetImage(entry->m_isDir? SJ_ICON_MUSIC_FOLDER : SJ_ICON_ANYFILE);
			li.SetData(i);
			m_dirListCtrl->InsertItem(li);

			if( entry->m_objectId == selId ) {
				m_dirListCtrl->SetItemState(zero_based_pos, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
			}

			zero_based_pos++;
		}

		EnsureSelListCtrlItemVisible(m_dirListCtrl);
	}
}


void SjUpnpDialog::OnUpdateMediaServerList(wxCommandEvent&)
{
	UpdateMediaServerList();
}


void SjUpnpDialog::OnScanDone(wxCommandEvent&)
{
	if( m_stillScanningText )
	{
		m_stillScanningText->Hide();
	}
}


void SjUpnpDialog::OnSize(wxSizeEvent& e)
{
	if( m_mediaServerListCtrl )
	{
		wxSize size = m_mediaServerListCtrl->GetClientSize();
		m_mediaServerListCtrl->SetColumnWidth(0, size.x-SJ_DLG_SPACE);
	}

	if( m_dirListCtrl )
	{
		wxSize size = m_dirListCtrl->GetClientSize();
		m_dirListCtrl->SetColumnWidth(0, size.x-SJ_DLG_SPACE);
	}

	e.Skip();
}


void SjUpnpDialog::OnMediaServerClick(wxListEvent&)
{
    SjUpnpMediaServer* mediaServer = GetSelectedMediaServer();
    if( mediaServer == NULL ) { return; } // nothing selected
    if( mediaServer == m_dirListFor ) { return; } // already selected
	m_dirListFor = mediaServer;

	wxBusyCursor busy;

	m_currDir.m_objectId = "0";
	m_currDir.m_dc_title = "";
    mediaServer->FetchContents(m_currDir);

    UpdateDirList();
}


void SjUpnpDialog::OnMediaServerContextMenu(wxListEvent&)
{
    wxPoint pt = ScreenToClient(::wxGetMousePosition());
    bool sthSelected = GetSelectedMediaServer()!=NULL;

    SjMenu m(0);
    m.Append(IDC_MEDIASERVERINFO, _("Info..."));
    m.Enable(IDC_MEDIASERVERINFO, sthSelected);

    PopupMenu(&m, pt);
}


void SjUpnpDialog::OnMediaServerInfo(wxCommandEvent&)
{
	SjUpnpMediaServer* mediaServer = GetSelectedMediaServer();
	if( mediaServer == NULL ) { return; } // nothing selected

	wxMessageBox(
			  "UDN: "                          + mediaServer->m_udn                                              + "\n\n"
			+ "friendlyName: "                 + mediaServer->m_friendlyName                                     + "\n\n"
			+ "modelDescription: "             + mediaServer->m_modelDescription                                 + "\n\n"
			+ "manufacturer: "                 + mediaServer->m_manufacturer                                     + "\n\n"
			+ "deviceType: "                   + mediaServer->m_deviceType                                       + "\n\n"
			+ "serviceType: "                  + mediaServer->m_serviceType                                      + "\n\n"
			+ "ContentDirectory.controlURL: "  + mediaServer->m_absControlUrl                                    + "\n\n"
			//+ "ContentDirectory.eventSubURL: " + mediaServer->m_absEventSubUrl                                   + "\n\n"
			//+ "Subscription-ID: "              + wxString(mediaServer->m_subscriptionId, sizeof(Upnp_SID))       + "\n\n"
			//+ "Subscription-Timeout: "         + wxString::Format("%i", (int)mediaServer->m_subscriptionTimeout) + "\n\n"
			, mediaServer->m_friendlyName, wxOK, this);
}


void SjUpnpDialog::OnDirDoubleClick(wxListEvent&)
{
	if( m_dirListCtrl )
	{
		wxBusyCursor busy;

		SjUpnpMediaServer* mediaServer = GetSelectedMediaServer();
		if( mediaServer == NULL ) { return; } // nothing selected
		if( mediaServer != m_dirListFor ) { return; } // sth. went wrong

		// find out the clicked folder, clear directory entries
		wxString clickedId, clickedName, selId;
		{
			SjUpnpDirEntry* dirEntry = GetSelectedDirEntry();
			if( dirEntry == NULL ) { return; } // nothing selected

			if( dirEntry == &m_parentDirEntry ) {
				clickedId   = m_parentIds.Last();    m_parentIds.RemoveAt(m_parentIds.GetCount()-1);
				clickedName = m_parentNames.Last();  m_parentNames.RemoveAt(m_parentNames.GetCount()-1);
				selId       = m_parentSelIds.Last(); m_parentSelIds.RemoveAt(m_parentSelIds.GetCount()-1);
			}
			else {
				if( !dirEntry->m_isDir ) { return; } // double click on a file -> nothing to do
				clickedId = dirEntry->m_objectId;   m_parentIds.Add(m_currDir.m_objectId);
				clickedName = dirEntry->m_dc_title; m_parentNames.Add(m_currDir.m_dc_title);
				m_parentSelIds.Add(dirEntry->m_objectId);
			}
			m_dirListCtrl->DeleteAllItems();
			m_currDir.Clear();
		}

		// load new directory entries
		m_currDir.m_objectId = clickedId;
		m_currDir.m_dc_title = clickedName;
		mediaServer->FetchContents(m_currDir);

		UpdateDirList(selId);
	}
}


void SjUpnpDialog::OnDirContextMenu(wxListEvent&)
{
    wxPoint pt = ScreenToClient(::wxGetMousePosition());
    bool sthSelected = GetSelectedDirEntry()!=NULL;

    SjMenu m(0);
    m.Append(IDC_DIRENTRYINFO, _("Info..."));
    m.Enable(IDC_DIRENTRYINFO, sthSelected);

    PopupMenu(&m, pt);
}


void SjUpnpDialog::OnDirEntryInfo(wxCommandEvent&)
{
	SjUpnpDirEntry* dirEntry = GetSelectedDirEntry();
	if( dirEntry == NULL ) { return; } // nothing selected

	wxString playtimeStr = "?";
	if( dirEntry->m_playtimeMs >= 0 ) {
		playtimeStr = SjTools::FormatMs(dirEntry->m_playtimeMs);
	}

	wxMessageBox(
			  "dc:title: "    + dirEntry->m_dc_title              + "\n\n"
			+ "dc:creator: "  + dirEntry->m_dc_creator            + "\n\n"
			+ "upnp:album: "  + dirEntry->m_upnp_album            + "\n\n"
			+ "upnp:genre: "  + dirEntry->m_upnp_genre            + "\n\n"
			+ "Directory: "   + (dirEntry->m_isDir? "yes" : "no") + "\n\n"
			+ "ID: "          + dirEntry->m_objectId              + "\n\n"
			+ "URL: "         + dirEntry->m_url                   + "\n\n"
			+ "Playtime: "    + playtimeStr                       + "\n\n"
			, dirEntry->m_dc_title, wxOK, this);
}


BEGIN_EVENT_TABLE(SjUpnpDialog, SjDialog)
	EVT_LIST_ITEM_SELECTED    (IDC_MEDIASERVERLISTCTRL,  SjUpnpDialog::OnMediaServerClick       )
	EVT_LIST_ITEM_RIGHT_CLICK (IDC_MEDIASERVERLISTCTRL,  SjUpnpDialog::OnMediaServerContextMenu )
	EVT_MENU                  (IDC_MEDIASERVERINFO,      SjUpnpDialog::OnMediaServerInfo        )
	EVT_LIST_ITEM_ACTIVATED   (IDC_DIRLISTCTRL,          SjUpnpDialog::OnDirDoubleClick         )
	EVT_LIST_ITEM_RIGHT_CLICK (IDC_DIRLISTCTRL,          SjUpnpDialog::OnDirContextMenu         )
	EVT_MENU                  (IDC_DIRENTRYINFO,         SjUpnpDialog::OnDirEntryInfo           )
	EVT_SIZE                  (                          SjUpnpDialog::OnSize                   )
	EVT_MENU                  (MSG_UPDATEMEDIASERVERLIST,SjUpnpDialog::OnUpdateMediaServerList  )
	EVT_MENU                  (MSG_SCANDONE,             SjUpnpDialog::OnScanDone               )
END_EVENT_TABLE()


#endif // SJ_USE_UPNP


