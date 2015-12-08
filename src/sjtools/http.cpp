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
 * File:    httpsj.cpp
 * Authors: Björn Petersen
 * Purpose: Asynchronous HTTP access
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/http.h>
#include <sjtools/console.h>
#include <wx/dialup.h>
#include <wx/url.h>


/*******************************************************************************
 * SjHttpThread
 ******************************************************************************/


#ifdef SJ_HTTP_THREAD

class SjHttpThread : public wxThread
{
public:
	                SjHttpThread        (SjHttp*, const wxString& urlStr, wxMBConv*, const SjSSHash* requestHeader, const wxString& postData);
	void*           Entry               ();

	SjHttp*         m_http;
	wxString        m_urlStr;
	wxMBConv*       m_urlMbConv;
	const SjSSHash* m_requestHeader;
	wxString        m_postData;

	static wxCriticalSection s_critical;
	static long     s_httpThreadCount;
	static bool     s_httpThreadInitialized;
};

wxCriticalSection   SjHttpThread::s_critical;
long                SjHttpThread::s_httpThreadCount = 0;
bool                SjHttpThread::s_httpThreadInitialized = FALSE;

SjHttpThread::SjHttpThread(SjHttp* http, const wxString& urlStr, wxMBConv* mbConv, const SjSSHash* requestHeader, const wxString& postData)
	: wxThread(wxTHREAD_DETACHED)
{
	m_http = http;
	m_urlStr = urlStr;
	m_urlMbConv = mbConv;
	m_requestHeader = requestHeader;
	m_postData = postData;

	Create();
	Run();
}

void* SjHttpThread::Entry()
{
	wxLog::SetThreadActiveTarget(SjLogGui::s_this);

	int         httpStatusCode;
	SjSSHash*   responseHeader = NULL;
	wxString content = SjHttp::ReadFile_(m_urlStr, m_urlMbConv, m_requestHeader, m_postData, httpStatusCode, &responseHeader);

	SjHttpThread::s_critical.Enter();
	if( m_http )
	{
		m_http->m_httpStatusCode = httpStatusCode;
		m_http->m_responseHeader = responseHeader;
		m_http->m_content = content;
		m_http->ReadReady();
		m_http->m_thread = NULL;
	}
	SjHttpThread::s_httpThreadCount--;
	SjHttpThread::s_critical.Leave();

	return 0;
}

#endif


/*******************************************************************************
 * SjHttp
 ******************************************************************************/


SjHttp::SjHttp()
{
	m_rcvEvtHandler  = NULL;
	m_rcvCommandId   = 0;
	m_httpStatusCode = 400; // Bad Request
	m_responseHeader = NULL;

	#ifdef SJ_HTTP_THREAD
		m_thread = NULL;
	#endif
}


void SjHttp::Cleanup()
{
	#ifdef SJ_HTTP_THREAD
		SjHttpThread::s_critical.Enter();
		if( m_thread )
		{
			m_thread->m_http = NULL;
			m_thread = NULL;
		}
		SjHttpThread::s_critical.Leave();
	#endif

	m_content.Clear();
	m_httpStatusCode = 400; // Bad Request
	if( m_responseHeader ) {delete m_responseHeader; m_responseHeader = NULL;}
}


void SjHttp::Init(wxEvtHandler* rcvEvtHandler, int rcvCommandId)
{
	wxASSERT( rcvEvtHandler != NULL );

	m_rcvEvtHandler = rcvEvtHandler;
	m_rcvCommandId  = rcvCommandId;
}


void SjHttp::OpenFile(const wxString& urlStr, wxMBConv* mbConv, const SjSSHash* requestHeader, const wxString& postData)
{
	wxASSERT( m_rcvEvtHandler != NULL );

	Cleanup();

	#ifdef SJ_HTTP_THREAD

		SjHttpThread::s_critical.Enter();
		SjHttpThread::s_httpThreadCount++;
		SjHttpThread::s_critical.Leave();

		m_thread = new SjHttpThread(this, urlStr, mbConv, requestHeader, postData);

	#else

		m_content = ReadFile_(urlStr, mbConv, requestHeader, postData, m_httpStatusCode, &m_responseHeader);
		ReadReady();

	#endif
}


wxString SjHttp::ReadFile_(const wxString& urlStr, wxMBConv* mbConv, const SjSSHash* requestHeader, const wxString& postData,
                           int& retHttpStatusCode, SjSSHash** retResponseHeader)
{
	wxURL url(urlStr);

	// get protocol pointer
	wxHTTP* http;
	{
		wxProtocol* protocol = &(url.GetProtocol());
		if( protocol == NULL ) { wxASSERT_MSG(0, wxT("protocol expected in SjHttp::ReadFile_().")); return wxT(""); } // error
		wxString classname = protocol->GetClassInfo()->GetClassName();
		if( classname !=  wxT("wxHTTP") ) { wxASSERT_MSG(0, wxT("wxHTTP expected in SjHttp::ReadFile_().")); return wxT(""); } // error
		http = (wxHTTP*)protocol;
	}

	// set post buffer (if any) (this forces the POST method, otherwise GET is used)
	if( !postData.IsEmpty() )
	{
		http->SetPostText("", // eg. "application/x-www-form-urlencoded; charset=UTF-8," however, this can also be set using the request header
						  postData, wxConvUTF8);
	}

	// set header (if any) (when using POST, make sure, you use eg. "Content-Type: application/x-www-form-urlencoded" here)
	if( requestHeader )
	{
		SjHashIterator iterator;
		wxString headerName, *headerValue;
		while( (headerValue=requestHeader->Iterate(iterator, headerName))!=NULL )
		{
			http->SetHeader(headerName, *headerValue);
		}
	}

	// read the returned content
	wxString content;
	wxInputStream* inputStream = url.GetInputStream();
	if( inputStream )
	{
		content = SjTools::GetFileContent(inputStream, mbConv);
		delete inputStream;
	}

	// get the returned headers
	SjSSHash* responseHeader = new SjSSHash();
	{
		#if 0 // TODO: http->GetAllHeaders() was a hack by me ...
			wxStringToStringHashMap* allHeaders = http->GetAllHeaders();
			if( allHeaders )
			{
				typedef wxStringToStringHashMap::iterator iterator;
				for( iterator it = allHeaders->begin(), en = allHeaders->end(); it != en; ++it )
				{
					responseHeader->Insert(it->first.Lower(), it->second);
				}
			}
		#endif
	}
	*retResponseHeader = responseHeader;

	// get the returned status
	retHttpStatusCode = http->GetResponse();

	return content;
}


wxString SjHttp::GetHttpResponseHeader(const wxString& key_) const
{
	wxString ret, *ptr, keyLower = key_.Lower();
	if( m_responseHeader )
	{
		ptr = m_responseHeader->Lookup(keyLower);
		if( ptr )
		{
			ret = *ptr;
		}
	}
	return ret;
}


void SjHttp::ReadReady()
{
	if( m_rcvEvtHandler )
	{
		wxSocketEvent event(m_rcvCommandId);
		m_rcvEvtHandler->AddPendingEvent(event);
	}
}


void SjHttp::OnSilverjukeStartup()
{
	#ifdef SJ_HTTP_THREAD
		if( !SjHttpThread::s_httpThreadInitialized )
		{
			// this is a little hack to allow using sockets (which are used by wxURL)
			// from a thread, see http://wiki.wxwidgets.org/wiki.pl?WxSocket
			// and http://www.litwindow.com/Knowhow/wxSocket/wxsocket.html
			wxASSERT( wxThread::IsMain() );
			wxSocketBase::Initialize();
			SjHttpThread::s_httpThreadInitialized = TRUE;
		}
	#endif
}


void SjHttp::OnSilverjukeShutdown()
{
	#ifdef SJ_HTTP_THREAD
		if( SjHttpThread::s_httpThreadInitialized
		 && SjHttpThread::s_httpThreadCount == 0 )
		{
			if( wxSocketBase::IsInitialized() )
			{
				wxSocketBase::Shutdown();
			}
		}
	#endif
}


