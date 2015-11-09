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
 * File:    folder_scanner.h
 * Authors: Björn Petersen
 * Purpose: The "folder scanner" module
 *
 ******************************************************************************/


#ifndef __SJ_FOLDER_SCANNER_H__
#define __SJ_FOLDER_SCANNER_H__


WX_DECLARE_STRING_HASH_MAP(int/*data not interesting*/, SjIgnoreExtHash);


class SjFolderScannerSource
{
public:
	SjFolderScannerSource()
	{
		#define SJ_FOLDERSCANNER_DEFFLAGS           0x0000FFFFL  // for a possible future merging, try to be compatible with SJ_SERVERSCANNER_FLAGS
		#define SJ_FOLDERSCANNER_ENABLED            0x00000004L
		#define SJ_FOLDERSCANNER_DOUPDATE           0x00000010L
		#define SJ_FOLDERSCANNER_READID3            0x00000020L
		#define SJ_FOLDERSCANNER_READHIDDENFILES    0x00020000L
		#define SJ_FOLDERSCANNER_READHIDDENDIRS     0x00040000L
		m_flags = SJ_FOLDERSCANNER_DEFFLAGS;
	}

	wxString        m_url;
	wxString        m_file; // empty if the URL specifies a directory that should be read recursive
	SjExtList       m_ignoreExt;
	SjTrackInfoMatcher m_trackInfoMatcher;
	long            m_flags;

	wxString        UrlPlusFile         ();
	SjIcon          GetIcon             ();
	bool            IsDir               () { return m_file.IsEmpty(); }
};



WX_DECLARE_LIST(SjFolderScannerSource, SjFolderScannerSourceList);


class SjFolderScannerModule : public SjScannerModule
{
public:
	                SjFolderScannerModule(SjInterfaceBase*);

	long            GetSourceCount      ();
	wxString        GetSourceUrl        (long index);
	wxString        GetSourceNotes      (long index);
	SjIcon          GetSourceIcon       (long index);
	long            AddSources          (int sourceType, wxWindow* parent);
	bool            DeleteSource        (long index, wxWindow* parent);
	bool            ConfigSource        (long index, wxWindow* parent);

	bool            IterateTrackInfo    (SjColModule* receiver);
	bool            SetTrackInfo        (const wxString& url, SjTrackInfo&);
	bool            IsMyUrl             (const wxString& url) { return GetSourceObj__(url)!=NULL; }

	bool            AddUrl              (const wxString& url);

protected:
	bool            FirstLoad           ();

private:
	SjFolderScannerSourceList m_listOfSources;

	SjFolderScannerSource* GetSourceObj__      (long index);
	SjFolderScannerSource* GetSourceObj__      (const wxString& url);

	SjTrackInfo     m_trackInfoMatcherObj;

	void            LoadSettings__      ();
	void            SaveSettings__      ();

	bool            IterateDir__        (const wxString& url, const wxString& onlyThisFile, bool deepUpdate,
	                                     SjFolderScannerSource*, SjColModule* receiver,
	                                     long& retTrackCount);
	bool            IterateFile__       (const wxString& url, bool deepUpdate,
	                                     const wxString& arts, unsigned long crc32,
	                                     SjFolderScannerSource*, SjColModule* receiver,
	                                     long& retTrackCount);
	long            GetTrackCount__     (SjFolderScannerSource*);
	long            DoAddUrl            (const wxString& newUrl, const wxString& newFile, bool& sthAdded);

	friend class    SjFolderSettingsDialog;
};



#endif // __SJ_FOLDER_SCANNER_H__

