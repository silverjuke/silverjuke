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
 ******************************************************************************/


#ifndef __SJ_UPNP_H__
#define __SJ_UPNP_H__
#if SJ_USE_UPNP


#include <upnp/upnp.h>
#include <upnp/upnptools.h>
#include <upnp/ixml.h>


// UPnP types
#define MEDIA_SERVER_DEVICE_TYPE       "urn:schemas-upnp-org:device:MediaServer:1"
#define CONTENT_DIRECTORY_SERVICE_TYPE "urn:schemas-upnp-org:service:ContentDirectory:1"


// XML tools
const char* xml_getChildElementValue (IXML_Element*, const char* psz_tag_name);
const char* xml_getChildElementValue (IXML_Document*, const char* psz_tag_name);
int         xml_getNumber            (IXML_Document*, const char* psz_tag_name);


// the UPnP Module
class SjUpnpModule : public SjCommonModule
{
public:
	                SjUpnpModule        (SjInterfaceBase*);
	bool            InitLibupnp         ();

private:
	bool            m_libupnp_initialized;
	void            ExitLibupnp         ();
	void            LastUnload          ();
};

extern SjUpnpModule* g_upnpModule;


#endif // SJ_USE_UPNP
#endif // __SJ_UPNP_H__
