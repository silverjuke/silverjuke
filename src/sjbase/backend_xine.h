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
 * File:    backend_xine.h
 * Authors: Björn Petersen
 * Purpose: Xine Backend
 *
 ******************************************************************************/


#ifndef __SJ_BACKEND_XINE_H__
#define __SJ_BACKEND_XINE_H__


#include <sjbase/backend.h>
#include <xine.h>


class SjXineBackendStream;


class SjXineBackend : public SjBackend
{
public:
	                     SjXineBackend       (SjBackendId);
	                     ~SjXineBackend      ();
	void                 GetLittleOptions    (SjArrayLittleOption&);
	SjBackendStream*     CreateStream        (const wxString& url, long seekMs, SjBackendCallback*, SjBackendUserdata*);
	SjBackendState       GetDeviceState      () const;
	void                 SetDeviceState      (SjBackendState);
	void                 SetDeviceVol        (double gain);

protected:
	static int           s_xine_usage;
	static xine_t*       s_xine;
	xine_audio_port_t*   m_ao_port;
	SjXineBackendStream* m_currStream;
	wxString             m_iniDevice;

	friend SjXineBackendStream;
};


class SjXineBackendStream : public SjBackendStream
{
public:
	                 ~SjXineBackendStream();
    void             GetTime             (long& totalMs, long& elapsedMs); // -1=unknown
    void             SeekAbs             (long ms);

protected:
    SjXineBackend*      m_backend;
   	xine_stream_t*      m_xine_stream;
	xine_event_queue_t*	m_event_queue;

	SjXineBackendStream(const wxString& url, SjXineBackend* backend, SjBackendCallback* cb, SjBackendUserdata* userdata)
		: SjBackendStream(url, backend, cb, userdata)
	{
		m_backend     = backend;
		m_xine_stream = NULL;
		m_event_queue = NULL;
	}
	friend class SjXineBackend;
};


#endif // __SJ_BACKEND_XINE_H__

