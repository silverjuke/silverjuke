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
 * File:    backend.cpp
 * Authors: Björn Petersen
 * Purpose: Backend base class, derived classes implement engines as Xine,
 *          GStreamer, BASS, DirectShow etc.
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/backend.h>


SjBackend::SjBackend(SjBackendId id)
{
	wxASSERT( wxThread::IsMain() );

	m_id = id;
}


SjBackend::~SjBackend()
{
	wxASSERT( wxThread::IsMain() );
	// if crossfading takes place and is not yet finished, there may be a pending stream at this moment.
	// however, as we're on shutdown, I think, we can safely ignore this situaltion.
}


wxString SjBackend::GetName() const
{
	     if(m_id==SJBE_ID_STDOUTPUT )  { return "stdoutput";  }
	else if(m_id==SJBE_ID_PRELISTEN )  { return "prelisten";  }
	else                               { return "unknown";    }
}


SjBackendStream::SjBackendStream(const wxString& url, SjBackend* backend, SjBackendCallback* cb, SjBackendUserdata* userdata)
{
	wxASSERT( wxThread::IsMain() );

	m_url              = url;
	m_cb               = cb;
	m_cbp.samplerate   = 44100;
	m_cbp.channels     = 2;
	m_cbp.startingTime = wxDateTime::Now().GetAsDOS();
	m_cbp.buffer       = NULL;
	m_cbp.bytes        = 0;
	m_cbp.backend      = backend;
	m_cbp.stream       = this;
	m_userdata         = userdata;

	// we keep a list of all streams created on the backend; may be useful for the implementations
	backend->m_allStreams.Add(this);

	// inform the user about the new stream, can be used to set up m_userdata_*
	// startingTime is set while samplerate/channels are still invalid!
	m_cbp.msg          = SJBE_MSG_CREATE;
	m_cb(&m_cbp);
}


SjBackendStream::~SjBackendStream()
{
	wxASSERT( wxThread::IsMain() );

	// inform the user that the userdata can be destroyed now; it's up to the user what to do.
	m_cbp.msg = SJBE_MSG_DESTROY_USERDATA;
	m_cb(&m_cbp);

	// remove stream from list
	wxArrayPtrVoid* allStreams =  &m_cbp.backend->m_allStreams;
	size_t i, iCnt = allStreams->GetCount();
	for( i = 0; i < iCnt; i++ )
	{
		if( allStreams->Item(i) == this )
		{
			allStreams->RemoveAt(i);
			break;
		}
	}
}

