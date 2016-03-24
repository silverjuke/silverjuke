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
 * A rough overview:
 *
 * UpnpInit();
 *   UpnpRegisterClient(..., &handle);    // register a callback function for receiving information, get a handle
 *     UpnpSearchAsync(handle, ...);      // search for devices, sends UPNP_DISCOVERY_SEARCH_RESULT etc.
 *     UpnpSubscribe(handle, ...., &sid); // subscribe to events eg. for browsing directories, if not renewed in UPNP_EVENT_SUBSCRIPTION_EXPIRED
 *   UpnpUnRegisterClient(handle);
 * UpnpFinish():
 *
 * Names:
 *
 * - a "Device" is any hard- or software in the UPnP-Network, a device may contain several services
 * - a "Media Server" is a service that may be provided on devices
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


/*******************************************************************************
 * XML tools
 ******************************************************************************/


// Returns the value of a child element, or NULL on error
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


// Returns the value of a child element, or NULL on error
static const char* xml_getChildElementValue( IXML_Document*  p_doc,
                                      const char*     psz_tag_name )
{
    assert( p_doc );
    assert( psz_tag_name );

    IXML_NodeList* p_node_list;
    p_node_list = ixmlDocument_getElementsByTagName( p_doc, psz_tag_name );
    if ( !p_node_list )  return NULL;

    IXML_Node* p_element = ixmlNodeList_item( p_node_list, 0 );
    ixmlNodeList_free( p_node_list );
    if ( !p_element )    return NULL;

    IXML_Node* p_text_node = ixmlNode_getFirstChild( p_element );
    if ( !p_text_node )  return NULL;

    return ixmlNode_getNodeValue( p_text_node );
}


// Get the number value from a SOAP response
static int xml_getNumber( IXML_Document* p_doc,
                   const char* psz_tag_name )
{
    assert( p_doc );
    assert( psz_tag_name );

    const char* psz = xml_getChildElementValue( p_doc, psz_tag_name );

    if( !psz )
        return 0;

    char *psz_end;
    long l = strtol( psz, &psz_end, 10 );

    if( *psz_end || l < 0 || l > INT_MAX )
        return 0;

    return (int)l;
}


/*******************************************************************************
 * SjUpnpMediaServer class
 ******************************************************************************/


static void parseDeviceDescription(IXML_Document* p_doc, const char* p_location, SjUpnpScannerModule* module, SjSPHash* addToList)
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
			IXML_Element* p_device_element = (IXML_Element*)ixmlNodeList_item(p_device_list, i);
			if( !p_device_element ) {
				continue;
			}

			const char* psz_device_type = xml_getChildElementValue(p_device_element, "deviceType");
			if ( !psz_device_type || strncmp( MEDIA_SERVER_DEVICE_TYPE, psz_device_type, strlen( MEDIA_SERVER_DEVICE_TYPE ) - 1 ) != 0 ) {
				continue;
			}

			const char* psz_udn = xml_getChildElementValue( p_device_element, "UDN" );
			if ( !psz_udn ) {
				wxLogWarning("UPnP Error: No UDN!");
				continue;
			}

			/* Check if server is already added */
			if ( addToList->Lookup(psz_udn) ) {
				continue;
			}

			SjUpnpMediaServer* p_server = new SjUpnpMediaServer(module);
			p_server->_UDN          = psz_udn;
			p_server->m_deviceType   = psz_device_type;

			const char* psz_temp = xml_getChildElementValue( p_device_element, "friendlyName" );
			p_server->_friendly_name = psz_temp? psz_temp : psz_udn;

			psz_temp = xml_getChildElementValue( p_device_element, "manufacturer" );
			if( psz_temp ) { p_server->m_manufacturer = psz_temp; }

			psz_temp = xml_getChildElementValue( p_device_element, "modelDescription" );
			if( psz_temp ) { p_server->m_modelDescription = psz_temp; }

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
                            p_server->subscribeToContentDirectory();
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
                            //p_server->fetchContents(); // done when the user selects the media server
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


/*
 * Extracts the result document from a SOAP response
 */
static IXML_Document* parseBrowseResult( IXML_Document* p_doc )
{
    wxASSERT( p_doc );

    /* Missing namespaces confuse the ixml parser. This is a very ugly
     * hack but it is needeed until devices start sending valid XML.
     *
     * It works that way:
     *
     * The DIDL document is extracted from the Result tag, then wrapped into
     * a valid XML header and a new root tag which contains missing namespace
     * definitions so the ixml parser understands it.
     *
     * If you know of a better workaround, please oh please fix it */
    const char* psz_xml_result_fmt = "<?xml version=\"1.0\" ?>"
        "<Result xmlns:sec=\"urn:samsung:metadata:2009\">%s</Result>";

    char* psz_xml_result_string = NULL;
    const char* psz_raw_didl = xml_getChildElementValue( p_doc, "Result" );

    if( !psz_raw_didl )
        return NULL;

    if( -1 == asprintf( &psz_xml_result_string,
                         psz_xml_result_fmt,
                         psz_raw_didl) )
        return NULL;


    IXML_Document* p_result_doc = ixmlParseBuffer( psz_xml_result_string );
    free( psz_xml_result_string );

    if( !p_result_doc )
        return NULL;

    IXML_NodeList *p_elems = ixmlDocument_getElementsByTagName( p_result_doc,
                                                                "DIDL-Lite" );

    IXML_Node *p_node = ixmlNodeList_item( p_elems, 0 );
    ixmlNodeList_free( p_elems );

    return (IXML_Document*)p_node;
}


IXML_Document* SjUpnpMediaServer::_browseAction( const char* psz_object_id_,
                                           const char* psz_browser_flag_,
                                           const char* psz_filter_,
                                           int i_starting_index_,
                                           const char* psz_requested_count_,
                                           const char* psz_sort_criteria_ )
{
	wxString psz_starting_index_ = wxString::Format("%i", (int)i_starting_index_);
    IXML_Document* p_action = 0;
    IXML_Document* p_response = 0;

    char* psz_service_type = strdup( CONTENT_DIRECTORY_SERVICE_TYPE );
    psz_service_type[strlen( psz_service_type ) - 1] = _i_content_directory_service_version;

    int i_res = UpnpAddToAction( &p_action, "Browse", psz_service_type, "ObjectID", psz_object_id_ );
    if ( i_res != UPNP_E_SUCCESS ) {
        wxLogWarning("UPnP: AddToAction/ObjectID failed: %s", wxString(UpnpGetErrorMessage(i_res)).c_str() );
        goto browseActionCleanup;
    }

    i_res = UpnpAddToAction( &p_action, "Browse", psz_service_type, "BrowseFlag", psz_browser_flag_ );
    if ( i_res != UPNP_E_SUCCESS ) {
        wxLogWarning("UPnP: AddToAction/BrowseFlag failed: %s", wxString(UpnpGetErrorMessage(i_res)).c_str() );
        goto browseActionCleanup;
    }

    i_res = UpnpAddToAction( &p_action, "Browse", psz_service_type, "Filter", psz_filter_ );
    if ( i_res != UPNP_E_SUCCESS ) {
        wxLogWarning("UPnP: AddToAction/Filter failed: %s", wxString(UpnpGetErrorMessage(i_res)).c_str() );
        goto browseActionCleanup;
    }

    i_res = UpnpAddToAction( &p_action, "Browse", psz_service_type, "StartingIndex", psz_starting_index_ );
    if ( i_res != UPNP_E_SUCCESS ) {
        wxLogWarning("UPnP: AddToAction/StartingIndex failed: %s", wxString(UpnpGetErrorMessage(i_res)).c_str() );
        goto browseActionCleanup;
    }

    i_res = UpnpAddToAction( &p_action, "Browse", psz_service_type, "RequestedCount", psz_requested_count_ );
    if ( i_res != UPNP_E_SUCCESS ) {
        wxLogWarning("UPnP: AddToAction/RequestedCount failed: %s", wxString(UpnpGetErrorMessage(i_res)).c_str() );
        goto browseActionCleanup;
    }

    i_res = UpnpAddToAction( &p_action, "Browse", psz_service_type, "SortCriteria", psz_sort_criteria_ );
    if ( i_res != UPNP_E_SUCCESS ) {
        wxLogWarning("UPnP: AddToAction/SortCriteria failed: %s", wxString(UpnpGetErrorMessage(i_res)).c_str() );
        goto browseActionCleanup;
    }

    i_res = UpnpSendAction( m_module->m_ctrlpt_handle,
              _content_directory_control_url,
              psz_service_type,
              0, /* ignored in SDK, must be NULL */
              p_action,
              &p_response );

    if ( i_res != UPNP_E_SUCCESS ) {
        wxLogWarning("UPnP: SendAction failed: %s", wxString(UpnpGetErrorMessage(i_res)).c_str() );
        if( p_response ) {
			ixmlDocument_free( p_response );
			p_response = 0;
		}
    }

browseActionCleanup:

    free( psz_service_type );

    ixmlDocument_free( p_action );
    return p_response;
}


bool SjUpnpMediaServer::_fetchContents()
{
    IXML_Document* p_response = _browseAction( "0", //p_parent->getObjectID() - root is "0" here
                                      "BrowseDirectChildren",
                                      "id,dc:title,res," /* Filter */
                                      "sec:CaptionInfo,sec:CaptionInfoEx,"
                                      "pv:subtitlefile",
                                      0, /* StartingIndex */
                                      "0", /* RequestedCount */
                                      "" /* SortCriteria */
                                      );

    if ( !p_response )
    {
        wxLogWarning("UPnP: No response from browse() action" );
        return false;
    }

    IXML_Document* p_result = parseBrowseResult( p_response );
    int i_number_returned = xml_getNumber( p_response, "NumberReturned" );
    int i_total_matches   = xml_getNumber( p_response , "TotalMatches" );

    ixmlDocument_free( p_response );

    if ( !p_result )
    {
        wxLogError("UPnP Error: browse() response parsing failed" );
        return false;
    }

    IXML_NodeList* containerNodeList = ixmlDocument_getElementsByTagName( p_result, "container" );
    if ( containerNodeList )
    {
        for ( unsigned int i = 0; i < ixmlNodeList_length( containerNodeList ); i++ )
        {
            IXML_Element* containerElement = ( IXML_Element* )ixmlNodeList_item( containerNodeList, i );

            const char* objectID = ixmlElement_getAttribute( containerElement,
                                                             "id" );
            if ( !objectID )
                continue;

            const char* title = xml_getChildElementValue( containerElement,
                                                          "dc:title" );

            if ( !title )
                continue;

			/* TODO: iterate?
            Container* container = new Container( p_parent, objectID, title );
            p_parent->addContainer( container );
            _fetchContents( container, 0 );
            */

            wxLogWarning("container: %s", wxString(title).c_str());
        }
        ixmlNodeList_free( containerNodeList );
    }

    IXML_NodeList* itemNodeList = ixmlDocument_getElementsByTagName( p_result, "item" );
    if ( itemNodeList )
    {
        for ( unsigned int i = 0; i < ixmlNodeList_length( itemNodeList ); i++ )
        {
            IXML_Element* itemElement =
                        ( IXML_Element* )ixmlNodeList_item( itemNodeList, i );

            const char* objectID =
                        ixmlElement_getAttribute( itemElement, "id" );

            if ( !objectID )
                continue;

            const char* title =
                        xml_getChildElementValue( itemElement, "dc:title" );

            if ( !title )
                continue;

            const char* psz_subtitles = xml_getChildElementValue( itemElement,
                    "sec:CaptionInfo" );

            if ( !psz_subtitles )
                psz_subtitles = xml_getChildElementValue( itemElement,
                        "sec:CaptionInfoEx" );

            if ( !psz_subtitles )
                psz_subtitles = xml_getChildElementValue( itemElement,
                        "pv:subtitlefile" );

			wxLogWarning("item: %s", wxString(title).c_str());

            /* Try to extract all resources in DIDL */
            IXML_NodeList* p_resource_list = ixmlDocument_getElementsByTagName( (IXML_Document*) itemElement, "res" );
            if ( p_resource_list )
            {
                int i_length = ixmlNodeList_length( p_resource_list );
                for ( int i = 0; i < i_length; i++ )
                {
                    //mtime_t i_duration = -1;
                    //int i_hours, i_minutes, i_seconds;
                    IXML_Element* p_resource = ( IXML_Element* ) ixmlNodeList_item( p_resource_list, i );
                    const char* psz_resource_url = xml_getChildElementValue( p_resource, "res" );
                    if( !psz_resource_url )
                        continue;
                    const char* psz_duration = ixmlElement_getAttribute( p_resource, "duration" );

					/*
                    if ( psz_duration )
                    {
                        if( sscanf( psz_duration, "%d:%02d:%02d",
                            &i_hours, &i_minutes, &i_seconds ) )
                            i_duration = INT64_C(1000000) * ( i_hours*3600 +
                                                              i_minutes*60 +
                                                              i_seconds );
                    }
                    */

					/*
                    Item* item = new Item( p_parent, objectID, title, psz_resource_url, psz_subtitles, i_duration );
                    p_parent->addItem( item );
                    */
                }
                ixmlNodeList_free( p_resource_list );
            }
            else continue;
        }
        ixmlNodeList_free( itemNodeList );
    }

    ixmlDocument_free( p_result );

	/* TODO: was das?
    if( i_offset + i_number_returned < i_total_matches )
        return _fetchContents( p_parent, i_offset + i_number_returned );
	*/

    return true;
}


void SjUpnpMediaServer::fetchContents()
{
	// delete old contents
	_p_contents.Clear();

	_fetchContents();
}


SjUpnpMediaServer::SjUpnpMediaServer(SjUpnpScannerModule* module)
{
	m_module = module;
	_i_subscription_timeout = 0;
	_i_content_directory_service_version = 0;
	memset( _subscription_id, 0, sizeof( Upnp_SID ) );
}


void SjUpnpMediaServer::subscribeToContentDirectory()
{
	// Subscribes current client handle to Content Directory Service.
	// CDS exports the server shares to clients.

	// currently, we do not use UpnpUnSubscribe() - instead, if Media servers are no longer used by us, they're not renewed

    int i_timeout = 1810; // corrected to 1800 seconds on my systrem
    Upnp_SID sid;

    int i_res = UpnpSubscribe( m_module->m_ctrlpt_handle, _content_directory_event_url, &i_timeout, sid );
    if ( i_res == UPNP_E_SUCCESS )
    {
        _i_subscription_timeout = i_timeout;
        memcpy( _subscription_id, sid, sizeof( Upnp_SID ) );
    }
    else
    {
		memset( _subscription_id, 0, sizeof( Upnp_SID ) );
        wxLogWarning("UPnP: Subscribe to %s failed: %s (%i)", _friendly_name.c_str(), wxString(UpnpGetErrorMessage(i_res)).c_str(), i_res);
    }
}


bool SjUpnpMediaServer::compareSID( const char* psz_sid )
{
    return ( strncmp( _subscription_id, psz_sid, sizeof( Upnp_SID ) ) == 0 );
}


void SjUpnpScannerModule::clear_media_server_list()
{
	// CAVE: the caller is responsible for locking m_deviceListCritical!

	SjHashIterator      iterator;
	wxString            udn;
	SjUpnpMediaServer*  server;
	while( (server=(SjUpnpMediaServer*)m_mediaServerList.Iterate(iterator, udn))!=NULL )
	{
		delete server;
	}
	m_mediaServerList.Clear();
}


SjUpnpMediaServer* SjUpnpScannerModule::get_media_server_by_sid( const char* psz_sid )
{
	// CAVE: the caller is responsible for locking m_deviceListCritical!
	SjHashIterator      iterator;
	wxString            udn;
	SjUpnpMediaServer*  server;
	while( (server=(SjUpnpMediaServer*)m_mediaServerList.Iterate(iterator, udn))!=NULL )
	{
		if( server->compareSID( psz_sid ) )
		{
			return server;
		}
	}

	return NULL;
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
	{
		wxCriticalSectionLocker locker(m_mediaServerCritical);
		clear_media_server_list();
	}
}


void SjUpnpScannerModule::LastUnload()
{
	exit_libupnp();
}


int ctrl_point_event_handler(Upnp_EventType eventType, void* p_event, void* user_data)
{
	// CAVE: We may be in _any_ thread here!

	SjUpnpScannerModule* this_ = (SjUpnpScannerModule*)user_data;

    switch( eventType )
    {
		case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE: // a new devices
		case UPNP_DISCOVERY_SEARCH_RESULT:       // normal search result, we may be more of this
			{
				// get device structure
				struct Upnp_Discovery* discoverEvent = (struct Upnp_Discovery*)p_event;

				IXML_Document* p_description_doc = NULL;
				int result = UpnpDownloadXmlDoc(discoverEvent->Location, &p_description_doc);
				if( result != UPNP_E_SUCCESS ) { wxLogError("UPnP Error: Fetching data from %s failed with error %i", discoverEvent->Location, (int)result); return result; } // error

				{
					wxCriticalSectionLocker locker(this_->m_mediaServerCritical);
					parseDeviceDescription(p_description_doc, discoverEvent->Location, this_, &this_->m_mediaServerList);
				}

				ixmlDocument_free(p_description_doc);

				if( this_->m_dlg ) {
					this_->m_dlg->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, MSG_UPDATEMEDIASERVERLIST));
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

		case UPNP_EVENT_RECEIVED:
			{
				wxCriticalSectionLocker locker(this_->m_mediaServerCritical);
				Upnp_Event* p_e = ( Upnp_Event* )p_event;

				SjUpnpMediaServer* p_server = this_->get_media_server_by_sid( p_e->Sid );
				//if ( p_server ) p_server->fetchContents();
			}
			break;

		case UPNP_EVENT_AUTORENEWAL_FAILED:
		case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
			{
				/* Re-subscribe. */

				wxCriticalSectionLocker locker(this_->m_mediaServerCritical);
				Upnp_Event_Subscribe* p_s = ( Upnp_Event_Subscribe* )p_event;

				SjUpnpMediaServer* p_server = this_->get_media_server_by_sid( p_s->Sid );
				if ( p_server ) p_server->subscribeToContentDirectory();
			}
			break;

		case UPNP_EVENT_SUBSCRIBE_COMPLETE: // only send if UpnpSubscribeAsync() is used
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


long SjUpnpScannerModule::AddSources(int sourceType, wxWindow* parent)
{
	if( !init_libupnp() ) { return -1; } // error

	{
		wxCriticalSectionLocker locker(m_mediaServerCritical);
		clear_media_server_list();
	}

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

	// clear the list to avoid renewing the event subscription forever.
	{
		wxCriticalSectionLocker locker(m_mediaServerCritical);
		clear_media_server_list();
	}

	return -1; // nothing added
}


#endif // SJ_USE_UPNP


