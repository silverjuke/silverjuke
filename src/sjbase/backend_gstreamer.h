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
	                 SjGstreamerBackend  (SjBackendId);
	                 ~SjGstreamerBackend () { SetDeviceState(SJBE_STATE_CLOSED); }
	void             GetLittleOptions    (SjArrayLittleOption&);
	SjBackendStream* CreateStream        (const wxString& url, long seekMs, SjBackendCallback*, void* userdata);
	SjBackendState   GetDeviceState      () const;
	void             SetDeviceState      (SjBackendState);
	void             SetDeviceVol        (double gain);

protected:
	wxString         m_iniAudioPipeline;
	wxString         m_iniVideoPipeline;
};


class SjGstreamerBackendStream : public SjBackendStream
{
public:
						~SjGstreamerBackendStream ();
    void                GetTime                   (long& totalMs, long& elapsedMs); // -1=unknown
    void                SeekAbs                   (long ms);
    bool                HasVideo                  () { return m_hasVideo; }

protected:
	SjGstreamerBackendStream(const wxString& url, SjGstreamerBackend* backend, SjBackendCallback* cb, void* userdata)
		: SjBackendStream(url, backend, cb, userdata)
    {
		m_backend      = backend;
		m_pipeline     = NULL;
		m_bus_watch_id = 0;
		m_hasVideo     = false;
		m_capsChecked  = false;
		m_eosSend      = false;
    }

	GstElement*         m_pipeline;
	guint               m_bus_watch_id;
	SjGstreamerBackend* m_backend;
	bool                m_hasVideo;
	bool                m_capsChecked;
	bool                m_eosSend;
	void                set_pipeline_state  (GstState s);

	friend class             SjGstreamerBackend;
	friend void              on_pad_added  (GstElement*, GstPad*, gpointer);
	friend GstPadProbeReturn on_pad_data   (GstPad*, GstPadProbeInfo*, gpointer);
	friend gboolean          on_bus_message(GstBus*, GstMessage*, gpointer);
};


#endif // __SJ_BACKEND_GSTREAMER_H__
