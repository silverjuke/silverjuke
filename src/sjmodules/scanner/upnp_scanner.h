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
 * File:    upnp_scanner.h
 * Authors: Björn Petersen
 * Purpose: Using UPNP/DLNA devices
 *
 ******************************************************************************/


#ifndef __SJ_UPNP_SCANNER_H__
#define __SJ_UPNP_SCANNER_H__
#if SJ_USE_UPNP


class SjUpnpSource;


class SjUpnpScannerModule : public SjScannerModule
{
public:
					SjUpnpScannerModule (SjInterfaceBase*);

	long            GetSourceCount      () { return m_sources.GetCount(); }
	wxString        GetSourceUrl        (long index) { return ""; }
	wxString        GetSourceNotes      (long index) { return ""; }
	SjIcon          GetSourceIcon       (long index) { return SJ_ICON_INTERNET_SERVER; }
	long            AddSources          (int sourceType, wxWindow* parent);
	bool            DeleteSource        (long index, wxWindow* parent) { return false; }
	bool            ConfigSource        (long index, wxWindow* parent) { return false; }

	bool            IterateTrackInfo    (SjColModule* receiver) { return true; }
	bool            SetTrackInfo        (const wxString& url, SjTrackInfo&) { return false; }
	bool            IsMyUrl             (const wxString& url) { return false; }

private:
	bool            m_libupnp_ok;
	bool            init_libupnp        ();
	void            exit_libupnp        ();

	void            LastUnload          ();

	wxArrayPtrVoid  m_sources;
	SjUpnpSource*   get_source          (long index) { return (SjUpnpSource*)m_sources.Item(index); }
};


#endif // SJ_USE_UPNP
#endif // __SJ_UPNP_SCANNER_H__

