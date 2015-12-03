/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2015 Björn Petersen Software Design and Development
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
 * File:    server_scanner.h
 * Authors: Björn Petersen
 * Purpose: The "server scanner" module
 *
 ******************************************************************************/


#ifndef __SJ_SERVER_SCANNER_H__
#define __SJ_SERVER_SCANNER_H__


enum SjServerScannerType
{
     SJ_SERVERSCANNER_TYPE_UNKNOWN = 0
    ,SJ_SERVERSCANNER_TYPE_HTTP
};


class SjServerScannerSource
{
public:
	SjServerScannerSource()
	{
		#define SJ_SERVERSCANNER_DEF_FLAGS      0x0000FFFFL // for a possible future merging, try to be compatible with SJ_FOLDERSCANNER_FLAGS
		#define SJ_SERVERSCANNER_ENABLED        0x00000004L
		#define SJ_SERVERSCANNER_DO_UPDATE      0x00000010L
		m_flags      = SJ_SERVERSCANNER_DEF_FLAGS;
		m_serverType = SJ_SERVERSCANNER_TYPE_HTTP;
	}

	SjIcon              GetIcon     () const { return SJ_ICON_INTERNET_SERVER; }

	wxString            m_serverName;
	SjServerScannerType m_serverType;
	wxString            m_loginName;
	wxString            m_loginPassword;
	long                m_flags;
	wxString            m_lastCfgFile; // for state information purposes only, may be empty if no scan was done before

	wxString            GetPath     (const wxString& path) const;

};


WX_DECLARE_OBJARRAY(SjServerScannerSource, SjArrayServerScannerSource);


class SjServerScannerModule : public SjScannerModule
{
public:
					SjServerScannerModule(SjInterfaceBase*);

	long            GetSourceCount      () { return m_sources.GetCount(); }
	wxString        GetSourceUrl        (long index) { return m_sources[index].m_serverName; }
	wxString        GetSourceNotes      (long index);
	SjIcon          GetSourceIcon       (long index);
	long            AddSources          (int sourceType, wxWindow* parent);
	bool            DeleteSource        (long index, wxWindow* parent);
	bool            ConfigSource        (long index, wxWindow* parent);

	bool            IterateTrackInfo    (SjColModule* receiver);
	bool            SetTrackInfo        (const wxString& url, SjTrackInfo&) { return FALSE; }
	bool            IsMyUrl             (const wxString& url);

protected:
	bool            FirstLoad           ();

private:
	SjArrayServerScannerSource m_sources;
	void            LoadSettings        ();
	void            SaveSettings        ();

	bool            IterateCsvOverHttp  (SjColModule* receiver, SjServerScannerSource*, SjCfgTokenizer* options);
	bool            IterateHttp         (SjColModule* receiver, SjServerScannerSource*, bool& retSaveSettings);
};


#endif // __SJ_SERVER_SCANNER_H__
