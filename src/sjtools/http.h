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
 * File:    httpsj.h
 * Authors: Björn Petersen
 * Purpose: Asynchronous HTTP access
 *
 ******************************************************************************/


#ifndef __SJ_HTTP_H__
#define __SJ_HTTP_H__


class SjHttpThread;


class SjHttp
{
public:
	SjHttp              ();
	~SjHttp             () { Cleanup(); }

	void            Init                (wxEvtHandler* rcvEvtHandler, int rcvCommandId);

	// Load a URL, the function emmits a wxSocketEvent if the URL is ready and GetString() can be called
	void            OpenFile            (const wxString& url, wxMBConv*, const SjSSHash* requestHeader=NULL, const wxString& postData=wxEmptyString);
	wxString        GetString           () const { return m_content; }
	int             GetHttpStatusCode   () const { return m_httpStatusCode; }
	wxString        GetHttpResponseHeader(const wxString& key) const;

	// OnSilverjukeStartup()/OnSilverjukeShutdown() should be called when the program starts/terminates
	static void     OnSilverjukeStartup ();
	static void     OnSilverjukeShutdown();

private:
	// private stuff
	wxEvtHandler*   m_rcvEvtHandler;
	int             m_rcvCommandId;

	void            Cleanup             ();

	int             m_httpStatusCode;
	wxString        m_content;
	SjSSHash*       m_responseHeader;


	static wxString ReadFile_           (const wxString& urlStr, wxMBConv*, const SjSSHash* requestHeader, const wxString& postData, int& retHttpStatusCode, SjSSHash** responseHeader);
	void            ReadReady           ();

	#define SJ_HTTP_THREAD
	#ifdef SJ_HTTP_THREAD
		SjHttpThread*   m_thread;
		friend class    SjHttpThread;
	#endif
};




#endif // __SJ_HTTP_H__
