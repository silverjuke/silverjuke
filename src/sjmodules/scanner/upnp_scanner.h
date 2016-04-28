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


#include <upnp/upnp.h>


class SjUpnpDialog;
class SjUpnpScannerModule;


class SjUpnpSource
{
public:
	wxString GetDisplayUrl () { return m_descr; }
	wxString m_udn;
	wxString m_objectId; // = directoryId
	wxString m_absControlUrl;
	wxString m_serviceType;
	wxString m_descr; // any friendly name
};


class SjUpnpDirEntry
{
public:
	           SjUpnpDirEntry() { m_isDir=false; m_upnp_originalTrackNumber=0; m_res_size=0; m_res_duration_ms=0; m_res_bitrate=0; m_res_sampleFrequency=0; m_res_nrAudioChannels=0; }
	wxString   m_dc_title;        // goes to m_trackName
	wxString   m_dc_creator;      // goes to m_leadArtistName
	wxString   m_dc_date;         // goes to m_year
	wxString   m_upnp_class;
	wxString   m_upnp_album;
	wxString   m_upnp_genre;
	long       m_upnp_originalTrackNumber;
	wxString   m_upnp_albumArtURI;
	wxString   m_res;             // goes to m_url
	long       m_res_size;
	long       m_res_duration_ms;
	long       m_res_bitrate;
	long       m_res_sampleFrequency;
	long       m_res_nrAudioChannels;
	bool       m_isDir;
	wxString   m_objectId;

};


class SjUpnpDir
{
public:
					SjUpnpDir () { }
                    ~SjUpnpDir() { Clear(); }
	void            Add       (SjUpnpDirEntry* i) { m_items.Add((void*)i); } // SjUpnpDir takes ownership of the item
	int             GetCount  () { return m_items.GetCount(); }
	SjUpnpDirEntry* Item      (int i) { return (SjUpnpDirEntry*)m_items.Item(i); }
	void            Clear     () { int i, cnt=GetCount(); for( i=0; i<cnt; i++ ) { delete Item(i); } m_items.Empty(); }

	wxString        m_objectId;
	wxString        m_dc_title;

	#define SJ_SHOW_UPNP_RAW 0
	#if SJ_SHOW_UPNP_RAW
	wxString        m_raw;
	#endif

private:
	wxArrayPtrVoid  m_items;
};


class SjUpnpMediaServer
{
public:
	         SjUpnpMediaServer(SjUpnpScannerModule*);

	wxString m_udn; // always unique
	wxString m_deviceType;
	wxString m_friendlyName;
	wxString m_manufacturer;
	wxString m_modelDescription;
	//wxString m_absEventSubUrl;
	wxString m_absControlUrl;
	wxString m_serviceType;
	//int      m_subscriptionTimeout;
	//Upnp_SID m_subscriptionId;

    bool FetchContents(SjUpnpDir&); // it's up to the caller to delete the returned object

// private
    //void subscribeToContentDirectory();
	//bool compareSID( const char* psz_sid );

private:
	SjUpnpScannerModule* m_module;
};


class SjUpnpScannerModule : public SjScannerModule
{
public:
					  SjUpnpScannerModule (SjInterfaceBase*);
					  ~SjUpnpScannerModule();

	long              GetSourceCount      () { load_sources(); return m_sources.GetCount(); }
	wxString          GetSourceUrl        (long index) { SjUpnpSource* s=get_source(index); if(s==NULL) { return ""; } return "upnp://"+s->GetDisplayUrl(); }
	wxString          GetSourceNotes      (long index) { return ""; }
	SjIcon            GetSourceIcon       (long index) { return SJ_ICON_UPNP_SERVER; }
	long              AddSources          (int sourceType, wxWindow* parent);
	bool              DeleteSource        (long index, wxWindow* parent);
	bool              ConfigSource        (long index, wxWindow* parent);

	bool              IterateTrackInfo    (SjColModule* receiver);
	bool              SetTrackInfo        (const wxString& url, SjTrackInfo&) { return false; }
	bool              IsMyUrl             (const wxString& url) { return false; }

//private: - however, needed in friend callbacks with enum parameters which cannot use forward declararions
	UpnpClient_Handle m_clientHandle;

	// the list of devices, if it changes, the MSG_UPDATEDEVICELIST is sent
	SjSPHash           m_mediaServerList;
	wxCriticalSection  m_mediaServerCritical;
	void               clear_media_server_list();
	//SjUpnpMediaServer* get_media_server_by_sid(const char* psz_sid);

	// opened dialog, NULL if none
	SjUpnpDialog*     m_dlg;

private:
	bool              init_client         ();
	void              exit_client         ();
	void              LastUnload          ();

	bool              load_sources        ();
	void              save_sources        ();
	void              clear_sources_list  () { int i, cnt=m_sources.GetCount(); for( i=0; i<cnt; i++ ) { delete get_source(i); } m_sources.Empty(); m_sourcesLoaded=false; }
	bool              m_sourcesLoaded;
	wxArrayPtrVoid    m_sources;
	SjUpnpSource*     get_source          (long index) { return (SjUpnpSource*)m_sources.Item(index); }
	long              get_source_by_udn_and_id (const wxString& udn, const wxString& id);

	bool              iterate_dir         (SjColModule*, SjUpnpMediaServer*, const wxString& objectId, const wxString& objectDescr);

};


#endif // SJ_USE_UPNP
#endif // __SJ_UPNP_SCANNER_H__

