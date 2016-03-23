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

		m_deviceListCtrl = new wxListCtrl(this, IDC_DEVICELISTCTRL, wxDefaultPosition, wxSize(380, SJ_DLG_SPACE*20), wxLC_REPORT | wxLC_SINGLE_SEL | wxSUNKEN_BORDER | wxLC_NO_HEADER);
		m_deviceListCtrl->SetImageList(g_tools->GetIconlist(FALSE), wxIMAGE_LIST_SMALL);
		m_deviceListCtrl->InsertColumn(0, _("Name"));
		sizer1->Add(m_deviceListCtrl, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);

		staticText = new wxStaticText(this, -1, "2. "+_("Select directory:"));
		sizer1->Add(staticText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);

		m_dirListCtrl = new wxListCtrl(this, IDC_DIRLISTCTRL, wxDefaultPosition, wxSize(380, SJ_DLG_SPACE*20), wxLC_REPORT | wxLC_SINGLE_SEL | wxSUNKEN_BORDER | wxLC_NO_HEADER);
		m_dirListCtrl->SetImageList(g_tools->GetIconlist(FALSE), wxIMAGE_LIST_SMALL);
		m_dirListCtrl->InsertColumn(0, _("Directory"));
		sizer1->Add(m_dirListCtrl, 1, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);

	// buttons
	sizer1->Add(CreateButtons(SJ_DLG_OK_CANCEL), 0, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	// init done, center dialog
	UpdateDeviceList();
	sizer1->SetSizeHints(this);
	CentreOnParent();
}


SjUpnpMediaServer* SjUpnpDialog::GetSelectedDevice()
{
	SjUpnpMediaServer* selDevice = NULL;
	long selIndex = GetSelListCtrlItem(m_deviceListCtrl);
	if( selIndex >= 0 ) { selDevice = (SjUpnpMediaServer*)m_deviceListCtrl->GetItemData(selIndex); }
	return selDevice;
}

void SjUpnpDialog::UpdateDeviceList()
{
	wxCriticalSectionLocker locker(m_upnpModule->m_deviceListCritical);

	SjUpnpMediaServer* selDevice = GetSelectedDevice();

	m_deviceListCtrl->DeleteAllItems();

	SjHashIterator iterator;
	wxString       udn;
	SjUpnpMediaServer*  device;
	int i = 0;
	while( (device=(SjUpnpMediaServer*)m_upnpModule->m_deviceList.Iterate(iterator, udn))!=NULL ) {
		wxListItem li;
		li.SetId(i++);
		li.SetMask(wxLIST_MASK_IMAGE | wxLIST_MASK_TEXT);
		li.SetText(device->_friendly_name);
		li.SetImage(SJ_ICON_INTERNET_SERVER);
		li.SetData((void*)device);
		int new_i = m_deviceListCtrl->InsertItem(li);
		if( device == selDevice ) {
			m_deviceListCtrl->SetItemState(new_i, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
		}
	}
}


void SjUpnpDialog::OnUpdateDeviceList(wxCommandEvent&)
{
	UpdateDeviceList();
}


void SjUpnpDialog::OnScanDone(wxCommandEvent&)
{
	m_stillScanningText->Hide();
}


void SjUpnpDialog::OnSize(wxSizeEvent& e)
{
	wxSize size = m_deviceListCtrl->GetClientSize();
	m_deviceListCtrl->SetColumnWidth(0, size.x-SJ_DLG_SPACE);
	e.Skip();
}

void SjUpnpDialog::OnDeviceClick(wxListEvent&)
{
    SjUpnpMediaServer* device = GetSelectedDevice();
    if( device == NULL ) { return; } // nothing selected
}

void SjUpnpDialog::OnDeviceContextMenu(wxListEvent&)
{
    wxPoint pt = ScreenToClient(::wxGetMousePosition());
    bool hasSelectedDevice = GetSelectedDevice()!=NULL;

    SjMenu m(0);
    m.Append(IDC_DEVICEINFO, _("Info..."));
    m.Enable(IDC_DEVICEINFO, hasSelectedDevice);

    PopupMenu(&m, pt);
}


void SjUpnpDialog::OnDeviceInfo(wxCommandEvent&)
{
	SjUpnpMediaServer* device = GetSelectedDevice();
	if( device == NULL ) { return; } // nothing selected

	wxMessageBox(
		wxString::Format("UDN: %s\n\nfriendlyName: %s\n\ndeviceType: %s\n\nContentDirectory.eventSubURL: %s\n\nContentDirectory.controlURL: %s\n\nContentDirectory.serviceType: %i",
			device->_UDN.c_str(),
			device->_friendly_name.c_str(),
			device->m_deviceType.c_str(),
			device->_content_directory_event_url.c_str(),
			device->_content_directory_control_url.c_str(),
			(int)device->_i_content_directory_service_version)
		, device->_friendly_name, wxOK, this);
}


BEGIN_EVENT_TABLE(SjUpnpDialog, SjDialog)
	EVT_LIST_ITEM_SELECTED    (IDC_DEVICELISTCTRL,   SjUpnpDialog::OnDeviceClick           )
	EVT_LIST_ITEM_RIGHT_CLICK (IDC_DEVICELISTCTRL,   SjUpnpDialog::OnDeviceContextMenu     )
	EVT_MENU                  (IDC_DEVICEINFO,       SjUpnpDialog::OnDeviceInfo            )
	EVT_SIZE                  (                      SjUpnpDialog::OnSize                  )
	EVT_MENU                  (MSG_UPDATEDEVICELIST, SjUpnpDialog::OnUpdateDeviceList      )
	EVT_MENU                  (MSG_SCANDONE,         SjUpnpDialog::OnScanDone              )
END_EVENT_TABLE()


#endif // SJ_USE_UPNP


