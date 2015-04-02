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
 * File:    fs_inet_sj.h
 * Authors: Björn Petersen
 * Purpose: Internet Filesystem Handler
 *
 ******************************************************************************/



#ifndef __SJ_FS_INET_H__
#define __SJ_FS_INET_H__



class WXDLLEXPORT SjInternetFSHandler : public wxFileSystemHandler
{
public:
	SjInternetFSHandler ();
	~SjInternetFSHandler();

	virtual bool        CanOpen             (const wxString& location);
	virtual wxFSFile*   OpenFile            (wxFileSystem& fs, const wxString& location);

	static void         SetAuth             (const wxString& urlBeg,
	        const wxString& user,
	        const wxString& passwd);

	static wxString     AddAuthToUrl        (const wxString& url);

private:
	wxArrayString       m_authUrlBeg;
	wxArrayString       m_authUserNPasswd;
	wxCriticalSection   m_authCritical;
};



#endif // __SJ_FS_INET_H__

