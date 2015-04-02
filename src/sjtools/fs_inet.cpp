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
 * File:    fs_inet_sj.cpp
 * Authors: Björn Petersen
 * Purpose: Internet Filesystem Handler
 *
 *******************************************************************************
 *
 * Notes: wxURL/wxInputStream are created just in time, this means
 * OpenFile() will always succeed if the URL is wellformed.
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/wfstream.h>
#include <wx/url.h>
#include <sjtools/fs_inet.h>

static SjInternetFSHandler* this_ = NULL;


/*******************************************************************************
 * SjInternetStream
 ******************************************************************************/


#if (wxMAJOR_VERSION==2 && wxMINOR_VERSION==4 && wxRELEASE_NUMBER==2)
#define wxFileOffset off_t
#endif


class SjInternetStream : public wxInputStream
{
public:
	SjInternetStream    (const wxString& urlWithoutAuth, wxURL* url);
	virtual         ~SjInternetStream   ();

	virtual size_t  GetSize             () const
	{
		if( m_cachedSize == 0xFFFFFFFFL )
		{
			m_cachedSize = ((SjInternetStream*)this)->InitStream()?
			               m_stream->GetSize() : 0;
		}
		return m_cachedSize;
	}
	virtual bool    Eof                 () const
	{
		return ((SjInternetStream*)this)->InitStream()?
		       (m_dataEof && m_pos>=m_dataValidBytes) : TRUE;
	}

	virtual bool    IsSeekable          () const
	{
		return true;
	}

protected:
	size_t          OnSysRead           (void *buffer, size_t nbytes);
	wxFileOffset    OnSysSeek           (wxFileOffset pos, wxSeekMode mode);

	wxFileOffset    OnSysTell           () const { return m_pos; };

private:
	mutable size_t  m_cachedSize;

	wxString        m_urlWithoutAuth;
	wxURL*          m_url;
	wxInputStream*  m_stream;
	bool            m_streamInitializationTried;
	bool            InitStream          ();

	unsigned char*  m_data;
	long            m_dataTotalBytes;
	long            m_dataValidBytes;

	bool            m_dataEof;          // this only indicates, we cannot get more data from
	// the original stream, m_pos may be somewhere else!

	bool            m_dataComplete;     // have we all read that can be read?
	// otherwise we won't add the stream to the cache.

#define         DATA_INCR_BYTES 0x20000L // 128K

	long            m_pos;
};


SjInternetStream::SjInternetStream(const wxString& urlWithoutAuth, wxURL* url)
{
	m_urlWithoutAuth            = urlWithoutAuth;
	m_url                       = url;
	m_stream                    = NULL;
	m_streamInitializationTried = FALSE;

	m_data                      = (unsigned char*)malloc(DATA_INCR_BYTES);
	m_dataTotalBytes            = DATA_INCR_BYTES;
	m_dataValidBytes            = 0;
	m_dataEof                   = FALSE;
	m_dataComplete              = FALSE;

	m_cachedSize                = 0xFFFFFFFFL;

	m_pos                       = 0;
}


bool SjInternetStream::InitStream()
{
	if( m_stream )
	{
		// stream already created before
		return TRUE;
	}

	if( !m_streamInitializationTried )
	{
		m_streamInitializationTried = TRUE;
		m_stream = m_url->GetInputStream();
		if( m_stream )
		{
			// stream successfully created
			return TRUE;
		}
	}

	// error - cannot create stream
	return FALSE;
}


SjInternetStream::~SjInternetStream()
{
	if( m_data )
	{
		if( g_tools )
		{
			if( !m_dataComplete && m_dataValidBytes > 0 )
			{
				long totalFileBytes = GetSize();
				if( totalFileBytes == m_dataValidBytes )
				{
					m_dataComplete = true;
				}
				else
				{
					// my first idea was, that we should complete
					// the file, if there are only a few bytes missing --
					// however, if have not seen this part of the code
					// being reached - only the above.
				}
			}

			if( m_dataComplete )
			{
				// write buffer to cache ...
				g_tools->m_cache.AddToCache(m_urlWithoutAuth, m_data, m_dataValidBytes);
			}
		}

		free(m_data);
	}

	// delete the stream AFTER the data; it may be needed eg. for GetSize()
	if( m_stream )
	{
		delete m_stream;
	}

	if( m_url )
	{
		delete m_url;
	}
}


size_t SjInternetStream::OnSysRead(void* buffer, size_t bytesWanted)
{
	// init the stream, if not yet done
	if( !InitStream() )
	{
		return 0;
	}

	// buffer the data, if not yet done
	if( !m_dataEof
	        &&  m_pos+bytesWanted > (size_t)m_dataValidBytes )
	{
		// how much to read? We always read at least 4K.
		long bytesToRead = (m_pos+bytesWanted) - m_dataValidBytes;
		if( bytesToRead < 4096 )
		{
			bytesToRead = 4096;
		}

		// enlarge the buffer, if needed
		if( bytesToRead > (m_dataTotalBytes-m_dataValidBytes) )
		{
			m_dataTotalBytes += DATA_INCR_BYTES+bytesToRead;
			m_data = (unsigned char*)realloc(m_data, m_dataTotalBytes);
		}

		// fill the buffer
		wxASSERT( bytesToRead > 0 );

		m_stream->Read(m_data+m_dataValidBytes, bytesToRead);
		long bytesRead = m_stream->LastRead();

		if( bytesRead <= 0 )
		{
			m_dataEof = TRUE;       // this only indicates, we cannot get more data from
			// the original stream, m_pos may be somewhere else!
			/*if( bytesRead == 0
			 && m_dataValidBytes == (long)GetSize() )
			{
			    m_dataComplete = TRUE;
			}*/
		}

		if( m_stream->Eof() )
		{
			m_dataEof = TRUE;       // see remark above

			m_dataComplete = TRUE;  // this indicates, we can write the data to the stream
		}

		m_dataValidBytes += bytesRead;
	}

	// get the data from the buffer
	if( m_pos > m_dataValidBytes )
	{
		bytesWanted = 0;
	}
	else if( m_pos+bytesWanted > (size_t)m_dataValidBytes )
	{
		bytesWanted = m_dataValidBytes - m_pos;
	}

	if( bytesWanted )
	{
		memcpy(buffer, m_data+m_pos, bytesWanted);
	}

	m_pos += bytesWanted;
	return bytesWanted;
}


wxFileOffset SjInternetStream::OnSysSeek(wxFileOffset wantedPos, wxSeekMode mode)
{
	if( InitStream() )
	{
		switch( mode )
		{
			case wxFromStart:   m_pos = wantedPos;              break;
			case wxFromEnd:     m_pos = GetSize()-wantedPos;    break;
			default:            m_pos += wantedPos;             break;
		}
	}

	return m_pos;
}


/*******************************************************************************
 * SjInternetFSHandler
 ******************************************************************************/


static wxString StripProtocolAnchor(const wxString& location)
{
	// only use the stuff before the hash anchor and after the ":"
	wxString myloc(location.BeforeLast(wxT('#')));
	if (myloc.IsEmpty())
	{
		myloc = location.AfterFirst(wxT(':'));
	}
	else
	{
		myloc = myloc.AfterFirst(wxT(':'));
	}

	// fix malformed url:
	if (myloc.Left(2) != wxT("//"))
	{
		if (myloc.GetChar(0) != wxT('/')) myloc = wxT("//") + myloc;
		else myloc = wxT("/") + myloc;
	}
	if (myloc.Mid(2).Find(wxT('/')) == wxNOT_FOUND) myloc << wxT('/');

	return myloc;
}


bool SjInternetFSHandler::CanOpen(const wxString& location)
{
	wxString p = GetProtocol(location);
	if ((p == wxT("http")) || (p == wxT("ftp")))
	{
		wxURL url(p + wxT(":") + StripProtocolAnchor(location));
		return (url.GetError() == wxURL_NOERR);
	}

	return FALSE;
}


wxFSFile* SjInternetFSHandler::OpenFile(wxFileSystem& WXUNUSED(fs), const wxString& location)
{
	wxString right = GetProtocol(location) + wxT(":") + StripProtocolAnchor(location);

	// does the file exist in the cache?

	if( g_tools )
	{
		wxString cacheFile = g_tools->m_cache.LookupCache(right);
		if( !cacheFile.IsEmpty() )
		{
			return new wxFSFile(new wxFileInputStream(cacheFile),
			                    right,
			                    GetMimeTypeFromExt(location),
			                    GetAnchor(location),
			                    wxDateTime::Now());
		}
	}

	// create streaming FSFile
	wxURL* url = new wxURL(SjInternetFSHandler::AddAuthToUrl(right));
	if( url )
	{
		if( url->GetError() == wxURL_NOERR )
		{
			return new wxFSFile(new SjInternetStream(right, url),
			                    right,
			                    GetMimeTypeFromExt(location),
			                    GetAnchor(location),
			                    wxDateTime::Now());
		}

		delete url;
	}

	// we can't open the URL
	return (wxFSFile*) NULL;
}


SjInternetFSHandler::SjInternetFSHandler()
{
	this_ = this;
}


SjInternetFSHandler::~SjInternetFSHandler()
{
	this_ = NULL;
}


void SjInternetFSHandler::SetAuth(const wxString& urlBeg,
                                  const wxString& user,
                                  const wxString& passwd)
{
	wxCriticalSectionLocker locker(this_->m_authCritical);

	wxASSERT( this_ );

	int index = this_->m_authUrlBeg.Index(urlBeg);
	if( index == wxNOT_FOUND )
	{
		if( user.IsEmpty() && passwd.IsEmpty() )
			return;

		this_->m_authUrlBeg.Add(urlBeg);
		this_->m_authUserNPasswd.Add(user+wxT(":")+passwd);
	}
	else if( user.IsEmpty() && passwd.IsEmpty() )
	{
		this_->m_authUserNPasswd[index] = wxT("");
	}
	else
	{
		this_->m_authUserNPasswd[index] = user+wxT(":")+passwd;
	}
}


wxString SjInternetFSHandler::AddAuthToUrl(const wxString& url)
{
	if( this_ )
	{
		wxCriticalSectionLocker locker(this_->m_authCritical);

		int afterProt = url.Find(wxT("://"));
		if( afterProt != -1 && url.Find(wxT('@')) == -1 )
		{
			afterProt += 3;

			int i, iCount = this_->m_authUrlBeg.GetCount();
			for( i = 0; i < iCount; i++ )
			{
				if( url.Len() >= afterProt+this_->m_authUrlBeg[i].Len()
				        && url.Mid(afterProt, this_->m_authUrlBeg[i].Len()) == this_->m_authUrlBeg[i] )
				{
					return  url.Left(afterProt)
					        +   this_->m_authUserNPasswd[i]
					        +   wxT("@")
					        +   url.Mid(afterProt);
				}
			}
		}
	}
	return url;
}

