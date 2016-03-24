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
	m_upnpModule   = upnpModule;
	m_upnpSource   = upnpSource; // may be NULL!
	m_isNew        = (upnpSource==NULL);
	m_stillLoading = true;
	m_dirListFor   = NULL;

	if( m_isNew ) {
		SetTitle(_("Add an UPnP/DLNA server"));
	}
	else {
		wxString::Format(_("Options for \"%s\""), upnpSource->GetDisplayUrl().c_str());
	}


	// create dialog
	wxBoxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

		wxBoxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);
		sizer1->Add(sizer2, 0, wxALL|wxGROW, SJ_DLG_SPACE);

			wxStaticText* staticText = new wxStaticText(this, -1, "1. "+_("Select server:"));
			sizer2->Add(staticText, 1, 0, SJ_DLG_SPACE);

			m_stillScanningText = new wxStaticText(this, -1, _("(still scanning)"));
			sizer2->Add(m_stillScanningText, 0, 0, SJ_DLG_SPACE);

		m_mediaServerListCtrl = new wxListCtrl(this, IDC_MEDIASERVERLISTCTRL, wxDefaultPosition, wxSize(380, SJ_DLG_SPACE*20), wxLC_REPORT | wxLC_SINGLE_SEL | wxSUNKEN_BORDER | wxLC_NO_HEADER);
		m_mediaServerListCtrl->SetImageList(g_tools->GetIconlist(FALSE), wxIMAGE_LIST_SMALL);
		m_mediaServerListCtrl->InsertColumn(0, _("Name"));
		sizer1->Add(m_mediaServerListCtrl, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);

		staticText = new wxStaticText(this, -1, "2. "+_("Select directory:"));
		sizer1->Add(staticText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);

		m_dirListCtrl = new wxListCtrl(this, IDC_DIRLISTCTRL, wxDefaultPosition, wxSize(380, SJ_DLG_SPACE*20), wxLC_REPORT | wxLC_SINGLE_SEL | wxSUNKEN_BORDER | wxLC_NO_HEADER);
		m_dirListCtrl->SetImageList(g_tools->GetIconlist(FALSE), wxIMAGE_LIST_SMALL);
		m_dirListCtrl->InsertColumn(0, _("Directory"));
		sizer1->Add(m_dirListCtrl, 1, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);

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
	long selIndex = GetSelListCtrlItem(m_mediaServerListCtrl);
	if( selIndex >= 0 ) { selMediaServer = (SjUpnpMediaServer*)m_mediaServerListCtrl->GetItemData(selIndex); }
	return selMediaServer;
}


void SjUpnpDialog::UpdateMediaServerList()
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
		li.SetText(mediaServer->_friendly_name);
		li.SetImage(SJ_ICON_INTERNET_SERVER);
		li.SetData((void*)mediaServer);
		int new_i = m_mediaServerListCtrl->InsertItem(li);
		if( mediaServer == selMediaServer ) {
			m_mediaServerListCtrl->SetItemState(new_i, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
		}
	}
}


void SjUpnpDialog::OnUpdateMediaServerList(wxCommandEvent&)
{
	UpdateMediaServerList();
}


void SjUpnpDialog::OnScanDone(wxCommandEvent&)
{
	m_stillScanningText->Hide();
}


void SjUpnpDialog::OnSize(wxSizeEvent& e)
{
	wxSize size = m_mediaServerListCtrl->GetClientSize();
	m_mediaServerListCtrl->SetColumnWidth(0, size.x-SJ_DLG_SPACE);
	e.Skip();
}


void SjUpnpDialog::OnMediaServerClick(wxListEvent&)
{
    SjUpnpMediaServer* mediaServer = GetSelectedMediaServer();
    if( mediaServer == NULL ) { return; } // nothing selected
    if( mediaServer == m_dirListFor ) { return; } // already selected

    mediaServer->fetchContents();

    m_dirListFor = mediaServer;
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

	wxString subscriptionId(mediaServer->_subscription_id, sizeof(Upnp_SID));
	wxMessageBox(
		wxString::Format(
			"UDN: %s\n\nfriendlyName: %s\n\nmodelDescription: %s\n\nmanufacturer: %s\n\ndeviceType: %s\n\n"
			"ContentDirectory.eventSubURL: %s\n\nContentDirectory.controlURL: %s\n\nContentDirectory.serviceType: %i\n\n"
			"Subscription-ID: %s\n\nSubscription-Timeout: %i seconds",
			mediaServer->_UDN.c_str(),
			mediaServer->_friendly_name.c_str(),
			mediaServer->m_modelDescription.c_str(),
			mediaServer->m_manufacturer.c_str(),
			mediaServer->m_deviceType.c_str(),
			mediaServer->_content_directory_event_url.c_str(),
			mediaServer->_content_directory_control_url.c_str(),
			(int)mediaServer->_i_content_directory_service_version,
			subscriptionId.c_str(),
			(int)mediaServer->_i_subscription_timeout)
		, mediaServer->_friendly_name, wxOK, this);
}


BEGIN_EVENT_TABLE(SjUpnpDialog, SjDialog)
	EVT_LIST_ITEM_SELECTED    (IDC_MEDIASERVERLISTCTRL,  SjUpnpDialog::OnMediaServerClick       )
	EVT_LIST_ITEM_RIGHT_CLICK (IDC_MEDIASERVERLISTCTRL,  SjUpnpDialog::OnMediaServerContextMenu )
	EVT_MENU                  (IDC_MEDIASERVERINFO,      SjUpnpDialog::OnMediaServerInfo        )
	EVT_SIZE                  (                          SjUpnpDialog::OnSize                   )
	EVT_MENU                  (MSG_UPDATEMEDIASERVERLIST,SjUpnpDialog::OnUpdateMediaServerList  )
	EVT_MENU                  (MSG_SCANDONE,             SjUpnpDialog::OnScanDone               )
END_EVENT_TABLE()


#endif // SJ_USE_UPNP


