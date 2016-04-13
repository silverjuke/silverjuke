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
#include <sjmodules/upnp.h>
#include <sjmodules/scanner/upnp_scanner.h>
#include <sjmodules/scanner/upnp_scanner_dlg.h>
#include <sjtools/msgbox.h>



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
		if( IXML_Node* p_url_node = ixmlNodeList_item(p_url_list, 0) )
		{
			IXML_Node* p_text_node = ixmlNode_getFirstChild(p_url_node);
			if ( p_text_node ) psz_base_url = ixmlNode_getNodeValue(p_text_node);
		}

		ixmlNodeList_free( p_url_list );
	}

    /* Get devices */
	IXML_NodeList* p_device_list = ixmlDocument_getElementsByTagName(p_doc, "device");
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

			const char* psz_udn = xml_getChildElementValue(p_device_element, "UDN");
			if ( !psz_udn ) {
				g_upnpModule->LogUpnpError("UDN missing", UPNP_E_INVALID_PARAM);
				continue;
			}

			/* Check if server is already added */
			if ( addToList->Lookup(psz_udn) ) {
				continue;
			}

			SjUpnpMediaServer* p_server = new SjUpnpMediaServer(module);
			p_server->m_udn          = psz_udn;
			p_server->m_deviceType   = psz_device_type;

			const char* psz_temp = xml_getChildElementValue(p_device_element, "friendlyName");
			p_server->m_friendlyName = psz_temp? psz_temp : psz_udn;

			psz_temp = xml_getChildElementValue(p_device_element, "manufacturer");
			if( psz_temp ) { p_server->m_manufacturer = psz_temp; }

			psz_temp = xml_getChildElementValue(p_device_element, "modelDescription");
			if( psz_temp ) { p_server->m_modelDescription = psz_temp; }

			addToList->Insert(p_server->m_udn, p_server);

			/* Check for ContentDirectory service. */
			IXML_NodeList* p_service_list = ixmlElement_getElementsByTagName(p_device_element, "service");
			if ( p_service_list )
			{
				for ( unsigned int j = 0; j < ixmlNodeList_length(p_service_list); j++ )
				{
                    IXML_Element* p_service_element = (IXML_Element*)ixmlNodeList_item(p_service_list, j);

                    const char* psz_service_type = xml_getChildElementValue(p_service_element, "serviceType");
                    if ( !psz_service_type ) {
						g_upnpModule->LogUpnpError("No service type found", UPNP_E_INVALID_PARAM);
                        continue;
                    }

                    int k = strlen(CONTENT_DIRECTORY_SERVICE_TYPE) - 1; // compare string ignoring the last character
                    if ( strncmp(CONTENT_DIRECTORY_SERVICE_TYPE, psz_service_type, k) != 0 ) {
                        continue;
					}

					p_server->m_serviceType = psz_service_type;

                    const char* psz_event_sub_url = xml_getChildElementValue(p_service_element, "eventSubURL");
                    if ( !psz_event_sub_url ) {
                        g_upnpModule->LogUpnpError("No event subscription url found", UPNP_E_INVALID_PARAM);
                        continue;
                    }

                    const char* psz_control_url = xml_getChildElementValue(p_service_element, "controlURL");
                    if ( !psz_control_url ) {
                        g_upnpModule->LogUpnpError("No control url found", UPNP_E_INVALID_PARAM);
                        continue;
                    }

                    /* Try to subscribe to ContentDirectory service */

					/*
                    char* psz_url = (char*) malloc(strlen(psz_base_url) + strlen(psz_event_sub_url) + 1 );
                    if ( psz_url )
                    {
                        if ( UpnpResolveURL(psz_base_url, psz_event_sub_url, psz_url) == UPNP_E_SUCCESS )
                        {
                            p_server->m_absEventSubUrl = psz_url;
                            p_server->subscribeToContentDirectory();
                        }

                        free(psz_url);
                    }
                    */

                    /* Try to browse content directory. */

                    char* psz_url = (char*) malloc(strlen(psz_base_url) + strlen(psz_control_url) + 1);
                    if ( psz_url )
                    {
                        if ( UpnpResolveURL(psz_base_url, psz_control_url, psz_url) == UPNP_E_SUCCESS )
                        {
                            p_server->m_absControlUrl = psz_url;
                            //p_server->fetchContents(); // done when the user selects the media server
                        }

                        free( psz_url );
                    }
				}
				ixmlNodeList_free(p_service_list);
			}
		}
		ixmlNodeList_free(p_device_list);
	}
}


/*
 * Extracts the result document from a SOAP response
 */
static IXML_Document* parseBrowseResult(IXML_Document* p_doc)
{
    wxASSERT(p_doc);

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
    if( !psz_raw_didl ) {
		g_upnpModule->LogUpnpError("Result missing in response", UPNP_E_INVALID_PARAM);
        return NULL;
	}

    if( -1 == asprintf( &psz_xml_result_string,
                         psz_xml_result_fmt,
                         psz_raw_didl) ) {
        g_upnpModule->LogUpnpError("asprintf failed", UPNP_E_INVALID_PARAM);
        return NULL;
	}

    IXML_Document* p_result_doc = ixmlParseBuffer( psz_xml_result_string );
    free( psz_xml_result_string );

    if( !p_result_doc ) {
		g_upnpModule->LogUpnpError("Parse buffere failed", UPNP_E_INVALID_PARAM);
        return NULL;
	}

    IXML_NodeList *p_elems = ixmlDocument_getElementsByTagName( p_result_doc,
                                                                "DIDL-Lite" );

    IXML_Node *p_node = ixmlNodeList_item( p_elems, 0 );
    ixmlNodeList_free( p_elems );

    return (IXML_Document*)p_node;
}


bool SjUpnpMediaServer::FetchContents(SjUpnpDir& dir)
{
	dir.Clear();
	int i_offset = 0, i_number_returned, i_total_matches;

	while( 1 ) // exit by break at end of loop
	{
		IXML_Document* p_result = NULL;
		{
			IXML_Document* p_response = NULL;
			{
				IXML_Document* p_action = NULL;
					UpnpAddToAction(&p_action, "Browse", m_serviceType, "ObjectID",      dir.m_objectId); // "0" = root
					UpnpAddToAction(&p_action, "Browse", m_serviceType, "BrowseFlag",    "BrowseDirectChildren");
					UpnpAddToAction(&p_action, "Browse", m_serviceType, "Filter",        "id,dc:title,dc:creator,upnp:album,upnp:genre,res"); // dc=Dublin Core
					UpnpAddToAction(&p_action, "Browse", m_serviceType, "StartingIndex", wxString::Format("%i", (int)i_offset));
					UpnpAddToAction(&p_action, "Browse", m_serviceType, "RequestedCount","0");
					UpnpAddToAction(&p_action, "Browse", m_serviceType, "SortCriteria",  "");
					int error = UpnpSendAction(m_module->m_clientHandle, m_absControlUrl, m_serviceType, 0, p_action, &p_response);
				ixmlDocument_free(p_action);
				if ( error != UPNP_E_SUCCESS || p_response==NULL ) {
					g_upnpModule->LogUpnpError("Cannot send message", error);
					if( p_response ) { ixmlDocument_free(p_response); }
					return false; // error
				}
			}

			p_result = parseBrowseResult(p_response);
			i_number_returned = xml_getNumber(p_response, "NumberReturned");
			i_total_matches   = xml_getNumber(p_response, "TotalMatches");
			ixmlDocument_free(p_response);
		}

		if( i_number_returned <= 0 || i_total_matches <= 0 ) {
			return true; // no error, may be an empty dir
		}

		if ( !p_result ) {
			return false; // error
		}

		IXML_NodeList* containerNodeList = ixmlDocument_getElementsByTagName(p_result, "container");
		if ( containerNodeList )
		{
			for ( unsigned int i = 0; i < ixmlNodeList_length( containerNodeList ); i++ )
			{
				IXML_Element* containerElement = (IXML_Element*)ixmlNodeList_item(containerNodeList, i);

				const char* id = ixmlElement_getAttribute(containerElement, "id");
				const char* dc_title = xml_getChildElementValue(containerElement, "dc:title");
				if ( !id || !dc_title  ) {
					continue;
				}

				SjUpnpDirEntry* entry = new SjUpnpDirEntry();
				entry->m_isDir    = true;
				entry->m_dc_title = dc_title;
				entry->m_objectId = id;
				dir.Add(entry); // entry is now owned by SjUpnpDir
			}
			ixmlNodeList_free( containerNodeList );
		}

		IXML_NodeList* itemNodeList = ixmlDocument_getElementsByTagName(p_result, "item");
		if ( itemNodeList )
		{
			for ( unsigned int i = 0; i < ixmlNodeList_length( itemNodeList ); i++ )
			{
				IXML_Element* itemElement = (IXML_Element*)ixmlNodeList_item(itemNodeList, i);

				const char* id         = ixmlElement_getAttribute(itemElement, "id");
				const char* dc_title   = xml_getChildElementValue(itemElement, "dc:title"); // if you add lines here, do not forget them above at "Filter"
				const char* dc_creator = xml_getChildElementValue(itemElement, "dc:creator");
				const char* upnp_album = xml_getChildElementValue(itemElement, "upnp:album");
				const char* upnp_genre = xml_getChildElementValue(itemElement, "upnp:genre"); // may be sth. like "(254)", seen on Fritzbox, however, I'm not sure, what this is about; it is _not_ ID3 (max. 192 genres)

				if ( !id || !dc_title ) {
					continue;
				}

				// Try to extract all resources in DIDL
				// (the loop is required as we go through all resources and use the first fine one)
				IXML_NodeList* p_resource_list = ixmlDocument_getElementsByTagName((IXML_Document*)itemElement, "res");
				if ( p_resource_list )
				{
					int i_length = ixmlNodeList_length(p_resource_list);
					for(int i = 0; i < i_length; i++)
					{
						IXML_Element* p_resource = (IXML_Element*) ixmlNodeList_item(p_resource_list, i);
						const char* psz_resource_url = xml_getChildElementValue(p_resource, "res");
						if( !psz_resource_url ) {
							continue; // this is the reason, we need the loop
						}
						const char* psz_duration = ixmlElement_getAttribute(p_resource, "duration");
						long playtimeMs = -1;
						if ( psz_duration )
						{
							int i_hours, i_minutes, i_seconds;
							if( sscanf(psz_duration, "%d:%02d:%02d", &i_hours, &i_minutes, &i_seconds) ) {
								playtimeMs = (i_hours*3600 + i_minutes*60 + i_seconds) * 1000;
							}
						}

						SjUpnpDirEntry* entry = new SjUpnpDirEntry();
						entry->m_isDir      = false;
						entry->m_dc_title   = dc_title;
						entry->m_dc_creator = dc_creator? wxString(dc_creator) : wxString();
						entry->m_upnp_album = upnp_album? wxString(upnp_album) : wxString();
						entry->m_upnp_genre = upnp_genre? wxString(upnp_genre) : wxString();
						entry->m_objectId   = id;
						entry->m_url        = psz_resource_url;
						entry->m_playtimeMs = playtimeMs;
						dir.Add(entry); // entry is now owned by SjUpnpDir
						break; // only one resource per ID
					}
					ixmlNodeList_free(p_resource_list);
				}
			}
			ixmlNodeList_free( itemNodeList );
		}

		ixmlDocument_free( p_result );

		if( i_offset + i_number_returned < i_total_matches ) {
			i_offset += i_number_returned;
		}
		else {
			break; // done, exit loop
		}
	}

    return true;
}


SjUpnpMediaServer::SjUpnpMediaServer(SjUpnpScannerModule* module)
{
	m_module = module;
	//m_subscriptionTimeout = 0;
	//memset(m_subscriptionId, 0, sizeof(Upnp_SID));
}


/*
void SjUpnpMediaServer::subscribeToContentDirectory()
{
	// TODO: is this really needed?
	// What if we use UpnpSendAction() without UpnpSubscribe()?

	// Subscribes current client handle to Content Directory Service.
	// CDS exports the server shares to clients.

	// currently, we do not use UpnpUnSubscribe() - instead, if Media servers are no longer used by us, they're not renewed

	int i_timeout = 1810; // corrected to 1800 seconds on my system
	Upnp_SID sid;

	int i_res = UpnpSubscribe(m_module->m_clientHandle, m_absEventSubUrl, &i_timeout, sid);
	if ( i_res == UPNP_E_SUCCESS )
	{
		m_subscriptionTimeout = i_timeout;
		memcpy(m_subscriptionId, sid, sizeof(Upnp_SID));
	}
	else
	{
		memset(m_subscriptionId, 0, sizeof( Upnp_SID ));
		g_upnpModule->LogUpnpError("Subscription failed", i_res, m_friendlyName);
	}
}
*/


/*
bool SjUpnpMediaServer::compareSID( const char* psz_sid )
{
    return (strncmp(m_subscriptionId, psz_sid, sizeof(Upnp_SID)) == 0 );
}
*/


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


/*
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
*/


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
	m_clientHandle          = -1;
	m_sourcesLoaded         = false;

	m_addSourceTypes_.Add(_("Add an UPnP/DLNA server"));
	m_addSourceIcons_.Add(SJ_ICON_UPNP_SERVER);
}


SjUpnpScannerModule::~SjUpnpScannerModule()
{
	wxCriticalSectionLocker locker(m_mediaServerCritical);
	clear_media_server_list();
	clear_sources_list();
}


void SjUpnpScannerModule::LastUnload()
{
	exit_client();
}


static int client_event_handler(Upnp_EventType eventType, void* p_event, void* user_data)
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
				int error = UpnpDownloadXmlDoc(discoverEvent->Location, &p_description_doc);
				if( error != UPNP_E_SUCCESS ) {
					// happens eg. with DroidUPnP, error -207, TIMEOUT
					g_upnpModule->LogUpnpError("Cannot download device description", error, discoverEvent->Location);
					return error;
				}

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
				//wxCriticalSectionLocker locker(this_->m_mediaServerCritical);
				//Upnp_Event* p_e = ( Upnp_Event* )p_event;

				//SjUpnpMediaServer* p_server = this_->get_media_server_by_sid( p_e->Sid );
				//if ( p_server ) p_server->fetchContents();
			}
			break;

		case UPNP_EVENT_AUTORENEWAL_FAILED:
		case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
			{
				/*
				// Re-subscribe.

				wxCriticalSectionLocker locker(this_->m_mediaServerCritical);
				Upnp_Event_Subscribe* p_s = ( Upnp_Event_Subscribe* )p_event;

				SjUpnpMediaServer* p_server = this_->get_media_server_by_sid( p_s->Sid );
				if ( p_server ) p_server->subscribeToContentDirectory();
				*/
			}
			break;

		case UPNP_EVENT_SUBSCRIBE_COMPLETE: // only send if UpnpSubscribeAsync() is used
			break;

		default:
			break;
    }

	return UPNP_E_SUCCESS;
}


bool SjUpnpScannerModule::init_client()
{
	// as initialisation may take a second, we init at late as possible

	if( m_clientHandle != -1 ) { return true; } // already initalized

	if( !g_upnpModule->InitLibupnp() ) {
		return false; // error already logged
	}

	// create our control point
	int error = UpnpRegisterClient(client_event_handler, this/*user data*/, &m_clientHandle);
	if( error != UPNP_E_SUCCESS ) {
		g_upnpModule->LogUpnpError("Cannot register client", error);
		m_clientHandle = -1;
		exit_client();
		return false; // error
	}

	// done, m_clientHandle is != -1 now
	return true;
}


void SjUpnpScannerModule::exit_client()
{
	if( m_clientHandle != -1 ) {
		UpnpUnRegisterClient(m_clientHandle);
		m_clientHandle = -1;
	}
}


bool SjUpnpScannerModule::load_sources()
{
	if( m_sourcesLoaded ) { return true; } // sources already loaded, nothing to do

	clear_sources_list();

	wxSqlt         sql;
	int            sourceCount = sql.ConfigRead("upnpscanner/sCount", 0L), i;
	SjUpnpSource*  source;
	for( i = 0; i < sourceCount; i++ )
	{
		source                  = new SjUpnpSource();
		source->m_udn           = sql.ConfigRead(wxString::Format("upnpscanner/s%iudn",        (int)i), wxT(""));
		source->m_objectId      = sql.ConfigRead(wxString::Format("upnpscanner/s%iid",         (int)i), wxT(""));
		source->m_absControlUrl = sql.ConfigRead(wxString::Format("upnpscanner/s%icontrolUrl", (int)i), wxT(""));
		source->m_serviceType   = sql.ConfigRead(wxString::Format("upnpscanner/s%iserviceType",(int)i), wxT(""));
		source->m_descr         = sql.ConfigRead(wxString::Format("upnpscanner/s%idescr",      (int)i), wxT(""));
		if( source->m_udn.IsEmpty() || source->m_objectId.IsEmpty()
		 || source->m_absControlUrl.IsEmpty() || source->m_serviceType.IsEmpty() || source->m_descr.IsEmpty() )
		{
			delete source;
		}
		else
		{
			m_sources.Add(source);
		}
	}

	m_sourcesLoaded = true;
	return true; // done
}


void SjUpnpScannerModule::save_sources()
{
	if( !m_sourcesLoaded ) { return; } // without loaded sources, we cannot save them
	wxSqltTransaction transaction; // for speed reasons

	{
		wxSqlt sql;

		int sourceCount = m_sources.GetCount(), i;
		sql.ConfigWrite("upnpscanner/sCount", sourceCount);
		for( i = 0; i < sourceCount; i++ )
		{
			SjUpnpSource* source = get_source(i);
			sql.ConfigWrite(wxString::Format("upnpscanner/s%iudn",        (int)i), source->m_udn);
			sql.ConfigWrite(wxString::Format("upnpscanner/s%iid",         (int)i), source->m_objectId);
			sql.ConfigWrite(wxString::Format("upnpscanner/s%icontrolUrl", (int)i), source->m_absControlUrl);
			sql.ConfigWrite(wxString::Format("upnpscanner/s%iserviceType",(int)i), source->m_serviceType);
			sql.ConfigWrite(wxString::Format("upnpscanner/s%idescr",      (int)i), source->m_descr);
		}
	}

	transaction.Commit();
}


long SjUpnpScannerModule::get_source_by_udn_and_id(const wxString& udn, const wxString& id)
{
	 int i, cnt=m_sources.GetCount();
	 for( i=0; i<cnt; i++ )
	 {
		SjUpnpSource* source = get_source(i);
		if( source->m_udn == udn && source->m_objectId == id ) {
			return i;
		}
	 }
	 return -1;
}


long SjUpnpScannerModule::AddSources(int sourceType, wxWindow* parent)
{
	if( !init_client() || !load_sources() ) { return -1; } // error

	long ret = -1; // nothing added

	{
		wxCriticalSectionLocker locker(m_mediaServerCritical);
		clear_media_server_list();
	}

	wxASSERT( m_dlg == NULL );
	m_dlg = new SjUpnpDialog(parent, this, NULL);

	UpnpSearchAsync(m_clientHandle,
		120 /*wait 2 minutes (my diskstation may take 30 seconds to appear (bp))*/,
		MEDIA_SERVER_DEVICE_TYPE, this/*user data*/);

	if( m_dlg->ShowModal() == wxID_OK )
	{
		wxCriticalSectionLocker locker(m_mediaServerCritical);
		SjUpnpMediaServer* mediaServer = m_dlg->GetSelectedMediaServer();
		SjUpnpDirEntry* dir = m_dlg->GetSelectedDir();
		if( mediaServer )
		{
			SjUpnpSource* source = new SjUpnpSource();
			source->m_udn           = mediaServer->m_udn;
			source->m_objectId      = dir->m_objectId;
			source->m_absControlUrl = mediaServer->m_absControlUrl;
			source->m_serviceType   = mediaServer->m_serviceType;
			source->m_descr         = mediaServer->m_friendlyName + "/" + dir->m_dc_title;

			if( (ret=get_source_by_udn_and_id(source->m_udn, source->m_objectId))!=-1 ) {
				delete source; // source is already existant, use existing index
			}
			else {
				m_sources.Add(source);
				ret = m_sources.GetCount()-1; // return the index of the new source
			}
		}

		save_sources();
	}

	delete m_dlg;
	m_dlg = NULL;

	// clear the list to avoid renewing the event subscription forever.
	/* -- not needed as we do not use UpnpSubscribe() at the moment; _if_ we do, however, this is probably the wrong method.
	{
		wxCriticalSectionLocker locker(m_mediaServerCritical);
		clear_media_server_list();
	}
	*/

	return ret; // nothing added
}


bool SjUpnpScannerModule::ConfigSource(long index, wxWindow* parent)
{
	if( !init_client() || !load_sources() ) { return false; } // error
	SjUpnpSource* source = get_source(index); if( source == NULL ) { return false; } // error

	wxASSERT( m_dlg == NULL );
	m_dlg = new SjUpnpDialog(parent, this, source);

	if( m_dlg->ShowModal() == wxID_OK )
	{
	}

	delete m_dlg;
	m_dlg = NULL;

	save_sources();
	return false; // needs update?
}


bool SjUpnpScannerModule::DeleteSource(long index, wxWindow* parent)
{
	delete get_source(index);
	m_sources.RemoveAt(index);

	save_sources();
	return true;
}


bool SjUpnpScannerModule::iterate_dir(SjColModule* receiver, SjUpnpMediaServer* mediaServer, const wxString& objectId, const wxString& objectDescr)
{
	SjUpnpDir dir;

	if( !SjBusyInfo::Set(objectDescr + " (" + objectId + ")", false) ) {
		return false; // abort
	}

	dir.m_objectId = objectId;
	if( !mediaServer->FetchContents(dir) ) {
		return false; // error
	}

	int i, cnt = dir.GetCount();
	for( i = 0; i < cnt; i++ )
	{
		SjUpnpDirEntry* entry = dir.Item(i);
		if( entry->m_isDir )
		{
			// iterate to subdirectory
			if( !iterate_dir(receiver, mediaServer, entry->m_objectId, objectDescr + "/" + entry->m_dc_title) ) {
				return false; // error
			}
		}
		else
		{
			// add a single file
			SjTrackInfo* trackInfo = new SjTrackInfo;
			trackInfo->m_url            = entry->m_url;
			trackInfo->m_trackName      = entry->m_dc_title;
			trackInfo->m_leadArtistName = entry->m_dc_creator;
			trackInfo->m_albumName      = entry->m_upnp_album;
			trackInfo->m_genreName      = entry->m_upnp_genre;
			receiver->Callback_ReceiveTrackInfo(trackInfo);
		}
	}

	// success/continue
	return true;
}


bool SjUpnpScannerModule::IterateTrackInfo(SjColModule* receiver)
{
	if( !init_client() || !load_sources() ) { return false; } // error

	SjUpnpMediaServer mediaServer(this);

	int sourceIndex, sourceCnt = m_sources.GetCount();
	for( sourceIndex = 0; sourceIndex < sourceCnt; sourceIndex++ )
	{
		SjUpnpSource* source = get_source(sourceIndex);
		mediaServer.m_udn           = source->m_udn;
		mediaServer.m_absControlUrl = source->m_absControlUrl;
		mediaServer.m_serviceType   = source->m_serviceType;

		if( !iterate_dir(receiver, &mediaServer, source->m_objectId, source->m_descr) )
		{
			return false;
		}
	}

	return true;
}


#endif // SJ_USE_UPNP
