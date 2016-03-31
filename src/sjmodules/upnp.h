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


class SjUpnpModule : public SjCommonModule
{
public:
	                SjUpnpModule        (SjInterfaceBase*);

private:
	void            LastUnload          ();
};


extern SjUpnpModule* g_upnpModule;


#endif // SJ_USE_UPNP
#endif // __SJ_UPNP_H__
