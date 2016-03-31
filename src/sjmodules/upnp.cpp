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
}


#endif // SJ_USE_UPNP

