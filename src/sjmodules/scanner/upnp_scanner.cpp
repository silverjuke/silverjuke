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
 ******************************************************************************/


#include <sjbase/base.h>
#if SJ_USE_UPNP
#include <sjmodules/scanner/upnp_scanner.h>
#include <sjmodules/scanner/upnp_source.h>
#include <upnp/upnp.h>
#include <upnp/ixml.h>


static UpnpClient_Handle s_ctrlpt_handle = -1;
static const char* MediaServerType = "urn:schemas-upnp-org:device:MediaServer:1";


SjUpnpScannerModule::SjUpnpScannerModule(SjInterfaceBase* interf)
	: SjScannerModule(interf)
{
	m_file                  = "memory:upnpscanner.lib";
	m_sort                  = 2; // second in list
	m_name                  = _("Read UPNP/DLNA servers");

	m_addSourceTypes_.Add(_("UPnP/DLNA server"));
	m_addSourceIcons_.Add(SJ_ICON_INTERNET_SERVER);

	m_libupnp_ok = false;
}


void SjUpnpScannerModule::LastUnload()
{
	exit_libupnp();
}


int ctrl_point_event_handler(Upnp_EventType eventType, void* pEvent, void* cookie)
{
    switch( eventType )
    {
		//case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE: // received on new devices
		case UPNP_DISCOVERY_SEARCH_RESULT:
			{
				struct Upnp_Discovery* pDiscEvent = (struct Upnp_Discovery*) pEvent;
				IXML_Document* pDoc = NULL;

				if (pDiscEvent->ErrCode != UPNP_E_SUCCESS)
				{
					wxLogError("UPnP: Error %i in discovery callback.", (int)pDiscEvent->ErrCode);
				}

				int ret = UpnpDownloadXmlDoc(pDiscEvent->Location, &pDoc);
				if (ret != UPNP_E_SUCCESS)
				{
					wxLogError("UPnP: Error %i when reading device description from %s", (int)ret, pDiscEvent->Location);
				}
				else
				{
					wxLogWarning("UPnP: Discovered location %s", pDiscEvent->Location);
					// see http://gejengel.googlecode.com/svn/trunk/src/UPnP/upnpcontrolpoint.cpp for details about parsing
					//pUPnP->onDevice(pDoc, pDiscEvent->Location, pDiscEvent->Expires);
				}

				if( pDoc )
				{
					ixmlDocument_free(pDoc);
				}

				break;
			}

		default:
			break;
    }

	return 0;
}


bool SjUpnpScannerModule::init_libupnp()
{
	// as initialisation may take a second, we init at late as possible

	if( m_libupnp_ok ) {
		return true; // already initalized
	}

	// init library
	int error = UpnpInit(NULL, 0);
	if( error != UPNP_E_SUCCESS ) {
		return false; // error
	}

	char* ip_address = UpnpGetServerIpAddress(); // z.B. 192.168.178.38
	unsigned short port = UpnpGetServerPort();   // z.B. 49152
	wxLogInfo("Loading libupnp on %s:%i", ip_address, (int)port);

	// create our control point
	error = UpnpRegisterClient(ctrl_point_event_handler, this/*user data*/, &s_ctrlpt_handle);
	if( error != UPNP_E_SUCCESS ) {
		wxLogError("Cannot register UPnP client.");
		s_ctrlpt_handle = -1;
		exit_libupnp();
		return false; // error
	}

	// done
	m_libupnp_ok = true;
	return true;
}


void SjUpnpScannerModule::exit_libupnp()
{
	if( s_ctrlpt_handle != -1 ) {
		UpnpUnRegisterClient(s_ctrlpt_handle);
		s_ctrlpt_handle = -1;
	}

	if( m_libupnp_ok ) {
		UpnpFinish();
		m_libupnp_ok = false;
	}
}


long SjUpnpScannerModule::AddSources(int sourceType, wxWindow* parent)
{
	if( !init_libupnp() ) { return -1; } // error

	UpnpSearchAsync(s_ctrlpt_handle, 60 /*seconds to wait*/, MediaServerType, this/*user data*/);

	return -1; // nothing added
}


#endif // SJ_USE_UPNP


