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
 * File:    upnp_scanner.cpp
 * Authors: Björn Petersen
 * Purpose: Using UPNP/DLNA devices
 *
 *******************************************************************************
 *
 * See http://gejengel.googlecode.com/svn/trunk/src/UPnP/upnpcontrolpoint.cpp
 * for a basic example.
 *
 * Common information about UPnP in german:
 * http://www.heise.de/netze/artikel/Netzwerke-mit-UPnP-einrichten-und-steuern-221520.html
 *
 ******************************************************************************/


#include <sjbase/base.h>
#if SJ_USE_UPNP
#include <sjmodules/scanner/upnp_scanner.h>
#include <upnp/upnp.h>
#include <upnp/ixml.h>


class SjUpnpDialog;
static const char* MEDIA_SERVER_DEVICE_TYPE = "urn:schemas-upnp-org:device:MediaServer:1";
//static const char* CONTENT_DIRECTORY_SERVICE_TYPE = "urn:schemas-upnp-org:service:ContentDirectory:1";


class SjUpnpScannerModuleInternal
{
public:
	SjUpnpScannerModuleInternal()
	{
		m_libupnp_initialized = false;
		m_ctrlpt_handle = -1;
		m_dlg = NULL;
	}

	~SjUpnpScannerModuleInternal()
	{
		ClearDeviceList();
	}

	bool              m_libupnp_initialized;
	UpnpClient_Handle m_ctrlpt_handle;

	// pointer is only valid if the dialog is presets, the pointer is needed only to send events from the UPnP thread to the dialog
	SjUpnpDialog*     m_dlg;

	// the list of devices, if it changes, the MSG_UPDATEDEVICELIST is sent
	SjSPHash          m_deviceList;
	wxCriticalSection m_deviceListCritical;
	void              ClearDeviceList     ();
};


static wxString getFirstDocumentItem(IXML_Document* pDoc, const wxString& item)
{
    wxString result;

    IXML_NodeList* pNodeList = ixmlDocument_getElementsByTagName(pDoc, item.c_str());
    if (pNodeList)
    {
        IXML_Node* pTmpNode = ixmlNodeList_item(pNodeList, 0);
        if (pTmpNode)
        {
            IXML_Node* pTextNode = ixmlNode_getFirstChild(pTmpNode);
            const char* pValue = ixmlNode_getNodeValue(pTextNode);
            if (pValue)
            {
                result = pValue;
            }
        }

        ixmlNodeList_free(pNodeList);
    }

    return result;
}


class SjUpnpDevice
{
public:
	SjUpnpDevice(IXML_Document* xml, const wxString& location)
	{
		// TODO: ein server kann mehrere Devices haben, wir sollten hier vernünftig parsen, s. /home/bpetersen/temp/upnp/vlc-2.2.1/modules/services_discovery
		m_location        = location;
		m_udn             = getFirstDocumentItem(xml, "UDN");
		m_deviceType      = getFirstDocumentItem(xml, "deviceType");
		m_friendlyName    = getFirstDocumentItem(xml, "friendlyName");
		m_urlBase         = getFirstDocumentItem(xml, "URLBase");
		m_presentationUrl = getFirstDocumentItem(xml, "presentationURL");
	}

	wxString m_location;
	wxString m_udn;
	wxString m_deviceType;
	wxString m_friendlyName;
	wxString m_urlBase;
	wxString m_presentationUrl;
};


/*******************************************************************************
 * SjUpnpSource - a source as used in Silverjuke's music library
 ******************************************************************************/


class SjUpnpSource
{
public:
	wxString GetDisplayUrl () { return ""; }
private:
};


/*******************************************************************************
 * SjUpnpDialog
 ******************************************************************************/


#define IDC_ENABLECHECK      (IDM_FIRSTPRIVATE+0)
#define IDC_DEVICELISTBOX    (IDM_FIRSTPRIVATE+2)
#define IDC_DIRLISTBOX       (IDM_FIRSTPRIVATE+3)
#define MSG_UPDATEDEVICELIST (IDM_FIRSTPRIVATE+50)
#define MSG_SCANDONE         (IDM_FIRSTPRIVATE+51)


class SjUpnpDialog : public SjDialog
{
public:
	SjUpnpDialog (wxWindow* parent, SjUpnpScannerModule* upnpModule, SjUpnpSource* upnpSource)
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

			m_deviceListbox = new wxListBox(this, IDC_DEVICELISTBOX, wxDefaultPosition, wxSize(380, SJ_DLG_SPACE*20));
			sizer1->Add(m_deviceListbox, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);

			staticText = new wxStaticText(this, -1, "2. "+_("Select directory:"));
			sizer1->Add(staticText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);

			m_dirListbox = new wxListBox(this, IDC_DIRLISTBOX, wxDefaultPosition, wxSize(380, SJ_DLG_SPACE*20));
			sizer1->Add(m_dirListbox, 1, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);

		// buttons
		sizer1->Add(CreateButtons(SJ_DLG_OK_CANCEL), 0, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

		// init done, center dialog
		UpdateDeviceList();
		EnableDisable();
		sizer1->SetSizeHints(this);
		CentreOnParent();
	}

private:
	void EnableDisable()
	{
	}

	void UpdateDeviceList()
	{
		wxCriticalSectionLocker locker(m_upnpModule->m_i->m_deviceListCritical);

		SjUpnpDevice* selDevice = NULL;
		long selIndex = m_deviceListbox->GetSelection();
		if( selIndex >= 0 ) { selDevice = (SjUpnpDevice*)m_deviceListbox->GetClientData(selIndex); }

		m_deviceListbox->Clear();

		SjHashIterator iterator;
		wxString       location;
		SjUpnpDevice*  device;
		while( (device=(SjUpnpDevice*)m_upnpModule->m_i->m_deviceList.Iterate(iterator, location))!=NULL ) {
			m_deviceListbox->Append(device->m_friendlyName, (void*)device /*CAVE: this is only safe if we do not remove servers while the dialog is opened! (*) */);
			if( device == selDevice ) {
				m_deviceListbox->SetSelection(m_deviceListbox->GetCount()-1);
			}
		}
	}

	void OnEnableCheck(wxCommandEvent&)
	{
		EnableDisable();
	}

	void OnUpdateDeviceList(wxCommandEvent&)
	{
		UpdateDeviceList();
		EnableDisable();
	}

	void OnScanDone(wxCommandEvent&)
	{
		m_stillScanningText->Hide();
	}

	bool                 m_isNew;
	bool                 m_stillLoading;
	SjUpnpScannerModule* m_upnpModule;
	SjUpnpSource*        m_upnpSource;

	wxListBox*           m_deviceListbox;
	wxStaticText*        m_stillScanningText;
	wxListBox*           m_dirListbox;

	                     DECLARE_EVENT_TABLE ()
};


BEGIN_EVENT_TABLE(SjUpnpDialog, SjDialog)
	EVT_CHECKBOX (IDC_ENABLECHECK,      SjUpnpDialog::OnEnableCheck     )
	EVT_MENU     (MSG_UPDATEDEVICELIST, SjUpnpDialog::OnUpdateDeviceList)
	EVT_MENU     (MSG_SCANDONE,         SjUpnpDialog::OnScanDone        )
END_EVENT_TABLE()


/*******************************************************************************
 * SjUpnpScannerModule
 ******************************************************************************/


SjUpnpScannerModule::SjUpnpScannerModule(SjInterfaceBase* interf)
	: SjScannerModule(interf)
{
	m_file                  = "memory:upnpscanner.lib";
	m_sort                  = 2; // second in list
	m_name                  = _("Read UPNP/DLNA servers");

	m_addSourceTypes_.Add(_("Add an UPnP/DLNA server"));
	m_addSourceIcons_.Add(SJ_ICON_INTERNET_SERVER);

	m_i = new SjUpnpScannerModuleInternal(); // pointer checked in init_libupnp(), however, this should normally not fail
}


SjUpnpScannerModule::~SjUpnpScannerModule()
{
	delete m_i;
}


void SjUpnpScannerModule::LastUnload()
{
	exit_libupnp();
}


int ctrl_point_event_handler(Upnp_EventType eventType, void* eventPtr, void* user_data)
{
	// CAVE: We may be in _any_ thread here!

	SjUpnpScannerModule* this_ = (SjUpnpScannerModule*)user_data;
	if( this_->m_i == NULL ) { return 0; } // already in destruction?

    switch( eventType )
    {
		case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE: // a new devices
		case UPNP_DISCOVERY_SEARCH_RESULT:       // normal search result, we may be more of htis
			{
				// get device structure
				struct Upnp_Discovery* discoverEvent = (struct Upnp_Discovery*)eventPtr;

				IXML_Document* xml = NULL;
				int result = UpnpDownloadXmlDoc(discoverEvent->Location, &xml);
				if( result != UPNP_E_SUCCESS ) { wxLogError("UPnP Error: Fetching data from %s failed with error %i", discoverEvent->Location, (int)result); return result; } // error

					SjUpnpDevice* device = new SjUpnpDevice(xml, discoverEvent->Location);

				ixmlDocument_free(xml);

				if (device->m_udn.empty() || device->m_deviceType != MEDIA_SERVER_DEVICE_TYPE ) {
					delete device;
					return 0; // just not the requested type - this may happen by definition
				}

				// add device to list, update the dialog (if any)
				{
					wxCriticalSectionLocker locker(this_->m_i->m_deviceListCritical);
					if( this_->m_i->m_deviceList.Lookup(discoverEvent->Location) == NULL ) {
						this_->m_i->m_deviceList.Insert(discoverEvent->Location, device);
					}
					else {
						delete device;
						return 0;
					}
				}

				if( this_->m_i->m_dlg ) {
					this_->m_i->m_dlg->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, MSG_UPDATEDEVICELIST));
				}
			}
			break;

		case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
			// struct Upnp_Discovery* discoverEvent = (struct Upnp_Discovery*)eventPtr;
			// send if a device is no longer available, however, as we keep the pointers in (*), we simply ignore this message.
			// (the device list is created from scratch everytime the dialog is opened, so it is no big deal to ignore this message)
			break;

		case UPNP_DISCOVERY_SEARCH_TIMEOUT:
			if( this_->m_i->m_dlg ) {
				this_->m_i->m_dlg->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, MSG_SCANDONE));
			}
			break;

		default:
			break;
    }

	return UPNP_E_SUCCESS;
}


bool SjUpnpScannerModule::init_libupnp()
{
	// as initialisation may take a second, we init at late as possible

	if( m_i == NULL ) { return false; } // error
	if( m_i->m_libupnp_initialized ) { return true; } // already initalized

	// init library
	int error = UpnpInit(NULL, 0);
	if( error != UPNP_E_SUCCESS ) {
		wxLogError("UPnP Error: Cannot init libupnp.");
		exit_libupnp();
		return false; // error
	}

	char* ip_address = UpnpGetServerIpAddress(); // z.B. 192.168.178.38
	unsigned short port = UpnpGetServerPort();   // z.B. 49152
	wxLogInfo("Loading libupnp on %s:%i", ip_address, (int)port);

	// create our control point
	error = UpnpRegisterClient(ctrl_point_event_handler, this/*user data*/, &m_i->m_ctrlpt_handle);
	if( error != UPNP_E_SUCCESS ) {
		wxLogError("UPnP Error: Cannot register client.");
		m_i->m_ctrlpt_handle = -1;
		exit_libupnp();
		return false; // error
	}

	// done
	m_i->m_libupnp_initialized = true;
	return true;
}


void SjUpnpScannerModule::exit_libupnp()
{
	if( m_i->m_libupnp_initialized )
	{
		if( m_i->m_ctrlpt_handle != -1 ) {
			UpnpUnRegisterClient(m_i->m_ctrlpt_handle);
			m_i->m_ctrlpt_handle = -1;
		}

		UpnpFinish();

		m_i->m_libupnp_initialized = false;
	}
}


void SjUpnpScannerModuleInternal::ClearDeviceList()
{
	wxCriticalSectionLocker locker(m_deviceListCritical);

	SjHashIterator iterator;
	wxString       location;
	SjUpnpDevice*  device;
	while( (device=(SjUpnpDevice*)m_deviceList.Iterate(iterator, location))!=NULL ) {
		delete device;
	}
	m_deviceList.Clear();
}


long SjUpnpScannerModule::AddSources(int sourceType, wxWindow* parent)
{
	if( !init_libupnp() ) { return -1; } // error

	m_i->ClearDeviceList();

	m_i->m_dlg = new SjUpnpDialog(parent, this, NULL);

	UpnpSearchAsync(m_i->m_ctrlpt_handle,
		120 /*wait 2 minutes (my diskstation may take 30 seconds to appear (br))*/,
		MEDIA_SERVER_DEVICE_TYPE, this/*user data*/);

	if( m_i->m_dlg->ShowModal() == wxID_OK )
	{
	}

	delete m_i->m_dlg;
	m_i->m_dlg = NULL;
	return -1; // nothing added
}


#endif // SJ_USE_UPNP


