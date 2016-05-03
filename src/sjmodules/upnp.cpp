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
 * File:    upnp.cpp
 * Authors: Björn Petersen
 * Purpose: General UPNP/DLNA functionality
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

 ******************************************************************************/


#include <sjbase/base.h>
#if SJ_USE_UPNP
#include <sjmodules/upnp.h>


/*******************************************************************************
 * XML tools
 ******************************************************************************/


// Returns the value of a child element, or NULL on error
const char* xml_getChildElementValue(IXML_Element* p_parent, const char* psz_tag_name)
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
const char* xml_getChildElementValue(IXML_Document* p_doc, const char* psz_tag_name)
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
int xml_getNumber(IXML_Document* p_doc, const char* psz_tag_name)
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
 * Init/Exit UPnP
 ******************************************************************************/


SjUpnpModule* g_upnpModule = NULL;


SjUpnpModule::SjUpnpModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	g_upnpModule          = this;
	m_file                = "memory:upnp.lib";
	m_name                = "UPnP module" /*n/t*/;
	m_libupnp_initialized = false;
}


void SjUpnpModule::LastUnload()
{
	g_upnpModule = NULL;
	ExitLibupnp();
}


wxString SjUpnpModule::FormatUpnpError(const wxString& descr, int errCode, const wxString& object)
{
	wxString ret = "UPnP Errror: " + descr + ": ";

	if( errCode == 801 ) {
		ret += "Unauthorized access - "; // this error is send by WMP but does not exist in libupnp?!
	}

	ret += wxString(UpnpGetErrorMessage(errCode), wxConvUTF8);
	ret += wxString::Format(" (Error %i)", (int)errCode);

	if( object != "" ) {
		ret += " ("+object+")";
	}

	return ret;
}


void SjUpnpModule::LogUpnpError(const wxString& descr, int errCode, const wxString& object)
{
	wxLogError("%s", FormatUpnpError(descr, errCode, object).c_str());
}


bool SjUpnpModule::InitLibupnp()
{
	if( m_libupnp_initialized ) { return true; } // already initalized

	// init library - NB: we may be in a working thread here (eg. ImgThread), however, this seems not to be a problem
	int error;
	if( (error=UpnpInit(NULL, 0)) != UPNP_E_SUCCESS ) {
		LogUpnpError("Cannot init", error);
		ExitLibupnp();
		return false; // error
	}

	char* ip_address = UpnpGetServerIpAddress(); // z.B. 192.168.178.38
	unsigned short port = UpnpGetServerPort();   // z.B. 49152
	wxLogInfo("Loading libupnp on %s:%i", ip_address, (int)port); // if we're in a working thread, the message may not appear in the console window

    // Increase max. content length to maximum; without this call, the default is DEFAULT_SOAP_CONTENT_LENGTH (=16K) which is far too
    // small for even simple dirs (eg. "video" or "Bibi Blocksberg" on my diskstation) and will result in UPNP_E_OUTOF_BOUNDS errors.
    // (libupnp does not treat a maximum content length of 0 as unlimited
    // until 64dedf (~ pupnp v1.6.7) and provides no sane way to discriminate between versions)
    if( (error=UpnpSetMaxContentLength(INT_MAX)) != UPNP_E_SUCCESS )
    {
        LogUpnpError("Cannot set maximum content length", error, wxString::Format("INT_MAX=%i", (int)INT_MAX));
        // continue, anyway
    }

	m_libupnp_initialized = true;
	return true;
}


void SjUpnpModule::ExitLibupnp()
{
	if( m_libupnp_initialized )
	{
		UpnpFinish();
		m_libupnp_initialized = false;
	}
}


bool SjUpnpModule::DownloadFileCached(const wxString& url, wxString& retFileName)
{
	#define IN_DATA_LEN (128 * 1024)
	void* http_handle = NULL;
	char* content_type = NULL;
	int content_len = 0, http_status = 0, http_ret;
	wxFile destFile;
	char buf[IN_DATA_LEN];
	size_t data_len, bytes_read = 0;

	// check cache
	retFileName = g_tools->m_cache.LookupCache(url);
	if( !retFileName.IsEmpty() )
	{
		// success - file read from cache
		return true;
	}

	// Init libupnp
	if( !InitLibupnp() )
	{
		goto DownloadFile_Error;
	}

	// open remote file
	http_ret = UpnpOpenHttpGet(url.mb_str(wxConvUTF8), &http_handle, &content_type, &content_len, &http_status, 1);
	if( http_ret != UPNP_E_SUCCESS )
	{
		// wait a moment and try a second time (I don't know why, but this fixed 99% of the errors)
		::wxSleep(2);
		http_ret = UpnpOpenHttpGet(url.mb_str(wxConvUTF8), &http_handle, &content_type, &content_len, &http_status, 1);
		if( http_ret != UPNP_E_SUCCESS )
		{
			http_handle = NULL;
			goto DownloadFile_Error;
		}
	}

	// open local file
	retFileName = g_tools->m_cache.AddToCache(url);
	destFile.Create(retFileName, TRUE/*overwrite*/);
	if( !destFile.IsOpened() )
	{
		goto DownloadFile_Error;
	}

	// copy file
	while( 1 )
	{
		data_len = IN_DATA_LEN;
		http_ret = UpnpReadHttpGet(http_handle, buf, &data_len, 10 /*timeout seconds*/);

		if( http_ret == UPNP_E_SUCCESS && data_len > 0 )
		{
			destFile.Write(buf, data_len);
			bytes_read += data_len;
		}
		else
		{
			break;
		}
	}

	if( bytes_read == 0 )
	{
		goto DownloadFile_Error;
	}

	// success
	UpnpCloseHttpGet(http_handle);
	return true;

	// error
DownloadFile_Error:

	if( http_handle )
	{
		UpnpCloseHttpGet(http_handle);
	}

	if( destFile.IsOpened() )
	{
		destFile.Close();
		wxRemoveFile(retFileName);
	}

	g_tools->m_cache.RemoveFromCache(url);

	return false;
}


#endif // SJ_USE_UPNP

