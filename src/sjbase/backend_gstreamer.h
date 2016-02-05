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
 * File:    backend_gstreamer.h
 * Authors: Björn Petersen
 * Purpose: GSteamer Backend
 *
 ******************************************************************************/


#ifndef __SJ_BACKEND_GSTREAMER_H__
#define __SJ_BACKEND_GSTREAMER_H__


#include <sjbase/backend.h>
#include <gst/gst.h>


class SjGstreamerBackendStream;


class SjGstreamerBackend : public SjBackend
{
public:
	                 SjGstreamerBackend  (SjBackendId, int lanes);
	void             GetLittleOptions    (SjArrayLittleOption&);
	SjBackendStream* CreateStream        (int lane, const wxString& url, long seekMs, SjBackendCallback*, void* userdata);
	SjBackendState   GetDeviceState      ();
	void             SetDeviceState      (SjBackendState);
	void             SetDeviceVol        (double gain);
	void             DestroyBackend      () { SetDeviceState(SJBE_STATE_CLOSED); delete this; }

/*private:
however, declared as public to be usable from callbacks (for speed reasons, this avoids one level of iteration)*/
	GstElement*      m_pipeline;
	SjGstreamerBackendStream* m_currStream;
	guint            m_bus_watch_id;
	wxString         m_iniPipeline;
	void             set_pipeline_state  (GstState s);
};


class SjGstreamerBackendStream : public SjBackendStream
{
public:
    void             GetTime             (long& totalMs, long& elapsedMs); // -1=unknown
    void             SeekAbs             (long ms);
	void             DestroyStream       ();

/*private:
however, declared as public to be usable from callbacks (for speed reasons, this avoids one level of iteration)*/
    SjGstreamerBackendStream(int lane, const wxString& url, SjGstreamerBackend* backend, SjBackendCallback* cb, void* userdata)
		: SjBackendStream(lane, url, backend, cb, userdata)
    {
		m_backend = backend;
		m_eosSend = false;
    }
    SjGstreamerBackend* m_backend;
    bool                m_eosSend;
};


#endif // __SJ_BACKEND_GSTREAMER_H__
