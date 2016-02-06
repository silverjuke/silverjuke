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


void SjBackend::DestroyBackend()
{
	ReleaseBackend();
}


SjBackendStream::SjBackendStream(const wxString& url, SjBackend* backend, SjBackendCallback* cb, void* userdata)
{
	m_url              = url;
	m_cb               = cb;
	m_cbp.samplerate   = 44100;
	m_cbp.channels     = 2;
	m_cbp.startingTime = wxDateTime::Now().GetAsDOS();
	m_cbp.msg          = SJBE_MSG_NONE;
	m_cbp.buffer       = NULL;
	m_cbp.bytes        = 0;
	m_cbp.userdata     = userdata;
	m_cbp.backend      = backend;
	m_cbp.stream       = this;

	backend->m_allStreams.Add(this);
}


void SjBackendStream::RemoveFromList()
{
	wxArrayPtrVoid* allStreams =  &m_cbp.backend->m_allStreams;
	size_t i, iCnt = allStreams->GetCount();
	for( i = 0; i < iCnt; i++ )
	{
		if( allStreams->Item(i) == this ) {
			allStreams->RemoveAt(i);
			break;
		}
	}
}


void SjBackendStream::DestroyStream()
{
	RemoveFromList(); // remove it before calling ReleaseStream()
	ReleaseStream();
}


SjBackendStream::~SjBackendStream()
{
	RemoveFromList(); // we call this to remove the stream if CreateStream() fails.
}

