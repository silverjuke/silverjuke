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
#include <sjmodules/scanner/upnp_scanner_dlg.h>
#include <sjtools/msgbox.h>
#include <upnp/upnp.h>
#include <upnp/upnptools.h>
#include <upnp/ixml.h>


static const char* MEDIA_SERVER_DEVICE_TYPE = "urn:schemas-upnp-org:device:MediaServer:1";
static const char* CONTENT_DIRECTORY_SERVICE_TYPE = "urn:schemas-upnp-org:service:ContentDirectory:1";


static const char* xml_getChildElementValue( IXML_Element* p_parent,
                                             const char*   psz_tag_name )
{
    wxASSERT( p_parent );
    wxASSERT( psz_tag_name );

    IXML_NodeList* p_node_list;
    p_node_list = ixmlElement_getElementsByTagName( p_parent, psz_tag_name );
    if ( !p_node_list ) return NULL;

    IXML_Node* p_element = ixmlNodeList_item( p_node_list, 0 );
    ixmlNodeList_free( p_node_list );
    if ( !p_element )   return NULL;

    IXML_Node* p_text_node = ixmlNode_getFirstChild( p_element );
    if ( !p_text_node ) return NULL;

    return ixmlNode_getNodeValue( p_text_node );
}



static void parseDeviceDescription(IXML_Document* p_doc, const char* p_location, SjSPHash* addToList)
{
	if( p_doc == NULL || p_location == NULL || addToList == NULL ) return;

	/* Try to extract baseURL */
	const char* psz_base_url = p_location;
	IXML_NodeList* p_url_list = ixmlDocument_getElementsByTagName( p_doc, "URLBase" );
	if ( p_url_list )
	{
		if ( IXML_Node* p_url_node = ixmlNodeList_item( p_url_list, 0 ) )
		{
			IXML_Node* p_text_node = ixmlNode_getFirstChild( p_url_node );
			if ( p_text_node ) psz_base_url = ixmlNode_getNodeValue( p_text_node );
		}

		ixmlNodeList_free( p_url_list );
	}

    /* Get devices */
	IXML_NodeList* p_device_list = ixmlDocument_getElementsByTagName( p_doc, "device" );
	if ( p_device_list )
	{
		for ( unsigned int i = 0; i < ixmlNodeList_length( p_device_list ); i++ )
		{
			IXML_Element* p_device_element = ( IXML_Element* ) ixmlNodeList_item( p_device_list, i );
			if( !p_device_element ) {
				continue;
			}

			const char* psz_device_type = xml_getChildElementValue( p_device_element, "deviceType" );
			if ( !psz_device_type ) {
				wxLogWarning( "No deviceType found!" );
				continue;
			}

			if ( strncmp( MEDIA_SERVER_DEVICE_TYPE, psz_device_type, strlen( MEDIA_SERVER_DEVICE_TYPE ) - 1 ) != 0 ) {
				continue;
			}

			const char* psz_udn = xml_getChildElementValue( p_device_element, "UDN" );
			if ( !psz_udn ) {
				wxLogWarning( "No UDN!" );
				continue;
			}

			/* Check if server is already added */
			if ( addToList->Lookup(psz_udn) ) {
				continue;
			}

			const char* psz_friendly_name = xml_getChildElementValue( p_device_element, "friendlyName" );
			if ( !psz_friendly_name ) {
				wxLogWarning( "No friendlyName!" );
				continue;
			}

			SjUpnpMediaServer* p_server = new SjUpnpMediaServer();
			p_server->_UDN          = psz_udn;
			p_server->m_deviceType   = psz_device_type;
			p_server->_friendly_name = psz_friendly_name;

			addToList->Insert(p_server->_UDN, p_server);

			/* Check for ContentDirectory service. */
			IXML_NodeList* p_service_list = ixmlElement_getElementsByTagName( p_device_element, "service" );
			if ( p_service_list )
			{
				for ( unsigned int j = 0; j < ixmlNodeList_length( p_service_list ); j++ )
				{
                    IXML_Element* p_service_element = ( IXML_Element* ) ixmlNodeList_item( p_service_list, j );

                    const char* psz_service_type = xml_getChildElementValue( p_service_element, "serviceType" );
                    if ( !psz_service_type ) {
						wxLogWarning( "No service type found." );
                        continue;
                    }

                    int k = strlen( CONTENT_DIRECTORY_SERVICE_TYPE ) - 1;
                    if ( strncmp( CONTENT_DIRECTORY_SERVICE_TYPE, psz_service_type, k ) != 0 ) {
                        continue;
					}

					p_server->_i_content_directory_service_version = psz_service_type[k];

                    const char* psz_event_sub_url = xml_getChildElementValue( p_service_element, "eventSubURL" );
                    if ( !psz_event_sub_url ) {
                        wxLogWarning("No event subscription url found.");
                        continue;
                    }

                    const char* psz_control_url = xml_getChildElementValue( p_service_element, "controlURL" );
                    if ( !psz_control_url ) {
                        wxLogWarning("No control url found." );
                        continue;
                    }

                    /* Try to subscribe to ContentDirectory service */

                    char* psz_url = ( char* ) malloc( strlen( psz_base_url ) + strlen( psz_event_sub_url ) + 1 );
                    if ( psz_url )
                    {
                        if ( UpnpResolveURL( psz_base_url, psz_event_sub_url, psz_url ) == UPNP_E_SUCCESS )
                        {
                            p_server->_content_directory_event_url = psz_url;
                            //p_server->subscribeToContentDirectory(); // TODO!
                        }

                        free( psz_url );
                    }

                    /* Try to browse content directory. */

                    psz_url = ( char* ) malloc( strlen( psz_base_url ) + strlen( psz_control_url ) + 1 );
                    if ( psz_url )
                    {
                        if ( UpnpResolveURL( psz_base_url, psz_control_url, psz_url ) == UPNP_E_SUCCESS )
                        {
                            p_server->_content_directory_control_url = psz_url;
                            //p_server->fetchContents(); // TODO!
                        }

                        free( psz_url );
                    }
				}
				ixmlNodeList_free( p_service_list );
			}
		}
		ixmlNodeList_free( p_device_list );
	}
}


/*******************************************************************************
 * SjUpnpScannerModule
 ******************************************************************************/


SjUpnpScannerModule::SjUpnpScannerModule(SjInterfaceBase* interf)
	: SjScannerModule(interf)
{
	m_file                  = "memory:upnpscanner.lib";
	m_sort                  = 2; // second in list
	m_name                  = _("Read UPNP/DLNA servers");
	m_dlg                   = NULL;
	m_libupnp_initialized = false;
	m_ctrlpt_handle = -1;

	m_addSourceTypes_.Add(_("Add an UPnP/DLNA server"));
	m_addSourceIcons_.Add(SJ_ICON_INTERNET_SERVER);
}


SjUpnpScannerModule::~SjUpnpScannerModule()
{
	clear_device_list();
}


void SjUpnpScannerModule::LastUnload()
{
	exit_libupnp();
}


int ctrl_point_event_handler(Upnp_EventType eventType, void* eventPtr, void* user_data)
{
	// CAVE: We may be in _any_ thread here!

	SjUpnpScannerModule* this_ = (SjUpnpScannerModule*)user_data;

    switch( eventType )
    {
		case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE: // a new devices
		case UPNP_DISCOVERY_SEARCH_RESULT:       // normal search result, we may be more of this
			{
				// get device structure
				struct Upnp_Discovery* discoverEvent = (struct Upnp_Discovery*)eventPtr;

				IXML_Document* p_description_doc = NULL;
				int result = UpnpDownloadXmlDoc(discoverEvent->Location, &p_description_doc);
				if( result != UPNP_E_SUCCESS ) { wxLogError("UPnP Error: Fetching data from %s failed with error %i", discoverEvent->Location, (int)result); return result; } // error

				{
					wxCriticalSectionLocker locker(this_->m_deviceListCritical);
					parseDeviceDescription(p_description_doc, discoverEvent->Location, &this_->m_deviceList);
				}

				ixmlDocument_free(p_description_doc);

				if( this_->m_dlg ) {
					this_->m_dlg->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, MSG_UPDATEDEVICELIST));
				}
			}
			break;

		case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
			// struct Upnp_Discovery* discoverEvent = (struct Upnp_Discovery*)eventPtr;
			// send if a device is no longer available, however, as we keep the pointers in (*), we simply ignore this message.
			// (the device list is created from scratch everytime the dialog is opened, so it is no big deal to ignore this message)
			break;

		case UPNP_DISCOVERY_SEARCH_TIMEOUT:
			if( this_->m_dlg ) {
				this_->m_dlg->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, MSG_SCANDONE));
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

	if( m_libupnp_initialized ) { return true; } // already initalized

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
	error = UpnpRegisterClient(ctrl_point_event_handler, this/*user data*/, &m_ctrlpt_handle);
	if( error != UPNP_E_SUCCESS ) {
		wxLogError("UPnP Error: Cannot register client.");
		m_ctrlpt_handle = -1;
		exit_libupnp();
		return false; // error
	}

	// done
	m_libupnp_initialized = true;
	return true;
}


void SjUpnpScannerModule::exit_libupnp()
{
	if( m_libupnp_initialized )
	{
		if( m_ctrlpt_handle != -1 ) {
			UpnpUnRegisterClient(m_ctrlpt_handle);
			m_ctrlpt_handle = -1;
		}

		UpnpFinish();

		m_libupnp_initialized = false;
	}
}


void SjUpnpScannerModule::clear_device_list()
{
	wxCriticalSectionLocker locker(m_deviceListCritical);

	SjHashIterator iterator;
	wxString       udn;
	SjUpnpMediaServer*  device;
	while( (device=(SjUpnpMediaServer*)m_deviceList.Iterate(iterator, udn))!=NULL ) {
		delete device;
	}
	m_deviceList.Clear();
}


long SjUpnpScannerModule::AddSources(int sourceType, wxWindow* parent)
{
	if( !init_libupnp() ) { return -1; } // error

	clear_device_list();

	wxASSERT( m_dlg == NULL );
	m_dlg = new SjUpnpDialog(parent, this, NULL);

	UpnpSearchAsync(m_ctrlpt_handle,
		120 /*wait 2 minutes (my diskstation may take 30 seconds to appear (bp))*/,
		MEDIA_SERVER_DEVICE_TYPE, this/*user data*/);

	if( m_dlg->ShowModal() == wxID_OK )
	{
	}

	delete m_dlg;
	m_dlg = NULL;
	return -1; // nothing added
}


#endif // SJ_USE_UPNP


