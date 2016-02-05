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
 * File:    backend_gstreamer.cpp
 * Authors: Björn Petersen
 * Purpose: GSteamer Backend
 *
 ******************************************************************************/


#include <sjbase/base.h>
#if SJ_USE_GSTREAMER
#include <sjbase/backend_gstreamer.h>


/*******************************************************************************
 * Tools and Internals
 ******************************************************************************/


#define GST_TO_WXSTRING(a) \
	 wxString a##WxStr = wxString((a), wxConvUTF8);

#define WXSTRING_TO_GST(a) \
	 const wxCharBuffer a##__tempCharBuf = (a).mb_str(wxConvUTF8); \
	 const char* a##GstStr = a##__tempCharBuf.data();

#define MILLISEC_TO_NANOSEC_FACTOR     1000000L
#define NANOSEC_TO_MILLISEC_DIVISOR    1000000L


void SjGstreamerBackend::set_pipeline_state(GstState s)
{
	if( !m_pipeline ) {
		return; // not ready
	}

	if( gst_element_set_state(m_pipeline, s) == GST_STATE_CHANGE_ASYNC ) {
		gst_element_get_state(m_pipeline, NULL, NULL, 3000*MILLISEC_TO_NANOSEC_FACTOR /*async change, wait max. 3 seconds*/);
	}
}


gboolean on_bus_message(GstBus* bus, GstMessage* msg, gpointer userdata)
{
	SjGstreamerBackend* backend = (SjGstreamerBackend*)userdata;
	if( backend == NULL ) { return true; }

	bool prepareNext = false;

	switch( GST_MESSAGE_TYPE(msg) )
	{
		case GST_MESSAGE_ERROR:
			// there may be series of error messages for one stream.
			// error messages are normally not followed by a GST_MESSAGE_EOS error
			{
				// get information about the error
				GError* error = NULL;
				gchar*  debug = NULL;
				gst_message_parse_error(msg, &error, &debug);

				// log the error
				const gchar* errormessage = error->message;        GST_TO_WXSTRING(errormessage);
				const gchar* objname =  GST_OBJECT_NAME(msg->src); GST_TO_WXSTRING(objname);
				wxString debugStr; if( debug ) { GST_TO_WXSTRING(debug); debugStr = " (" + debugWxStr + ")"; }
				wxLogError("GStreamer Error: %s: %s%s", objnameWxStr.c_str(), errormessageWxStr.c_str(), debugStr);

				g_free(debug);
				g_error_free(error);

				prepareNext = true;
			}
			break;

		case GST_MESSAGE_EOS:
			// we go here if we reach the end of the stream
			prepareNext = true;
			break;

		default:
			break;
	}

	if( prepareNext )
	{
		SjGstreamerBackendStream* stream = backend->m_currStream;
		if( stream )
		{
			if( prepareNext && !stream->m_eosSend )
			{
				stream->m_cbp.msg = SJBE_MSG_END_OF_STREAM;
				stream->m_cb(&stream->m_cbp);
				stream->m_eosSend = true;
			}
		}
	}

	return true;
}


void on_pad_added(GstElement* decodebin, GstPad* newSourcePad, gpointer userdata)
{
	// a new pad appears in the "decodebin" element - link this to the
	// element with the name "sjAudioEntry"
	SjGstreamerBackend* backend = (SjGstreamerBackend*)userdata;
	if( backend == NULL ) { return; }

	GstElement* audioEntry = gst_bin_get_by_name(GST_BIN(backend->m_pipeline), "sjAudioEntry");
	if( audioEntry )
	{
		// get sink pad of the "audio entry element"
		GstPad* destSinkPad = gst_element_get_static_pad(audioEntry, "sink");

		// link the new source pad to the "audio entry element"
		GstPadLinkReturn linkret = gst_pad_link(newSourcePad, destSinkPad);
		if( linkret!=GST_PAD_LINK_OK ) {
			wxLogError("GStreamer error: Cannot link objects in callback.");
		}

		gst_object_unref(audioEntry);

	}
}


GstPadProbeReturn on_pad_data(GstPad* pad, GstPadProbeInfo* info, gpointer userdata)
{
	SjGstreamerBackend*       backend = (SjGstreamerBackend*)userdata; if( backend == NULL ) { return GST_PAD_PROBE_OK; }
	SjGstreamerBackendStream* stream  = backend->m_currStream;       if( stream == NULL )  { return GST_PAD_PROBE_OK; }

	GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
	buffer = gst_buffer_make_writable(buffer);

		GstMapInfo map;
		gst_buffer_map(buffer, &map, GST_MAP_WRITE);

			stream->m_cbp.msg    = SJBE_MSG_DSP;
			stream->m_cbp.buffer = (float*)map.data;
			stream->m_cbp.bytes  = map.size;
			stream->m_cb(&stream->m_cbp);

		gst_buffer_unmap(buffer, &map);

	GST_PAD_PROBE_INFO_DATA(info) = buffer;

	return GST_PAD_PROBE_OK;
}


/*******************************************************************************
 * Public Backend Implementation
 ******************************************************************************/


SjGstreamerBackend::SjGstreamerBackend(SjBackendDeviceId device, int pipelines)
	: SjBackend(device, pipelines)
{
	m_pipeline     = NULL;
	m_bus_watch_id = 0;
	m_currStream   = NULL;

	// init, log version information
	gst_init(NULL, NULL);

	guint major, minor, micro, nano;
	gst_version(&major, &minor, &micro, &nano);
	wxLogInfo("Using GStreamer %i.%i.%i.%i", (int)major, (int)minor, (int)micro, (int)nano);
}


void SjGstreamerBackend::GetLittleOptions(SjArrayLittleOption& lo)
{
}


SjBackendStream* SjGstreamerBackend::CreateStream(int lane, const wxString& uri, long seekMs, SjBackendCallback* cb, void* userdata)
{
	if( m_currStream )
	{
		wxLogError("GSteamer Error: There is already a stream on the pipeline.");
		return NULL;
	}

	// create pipeline, add message handler to it
	if( m_pipeline == NULL )
	{
		/*
		  decodebin1 --.
		               |--> [funnel] ---> audioconvert -> capsfilter -> volume-> audiosink
		[decodebin2] --'              |
		                              |
		                              '--> [videosink]
		*/

		// create objects
		m_pipeline               = gst_pipeline_new        (                 "sjPlayer"    );
		GstElement* decodebin    = gst_element_factory_make("uridecodebin",  "sjSource"    );
		GstElement* audioconvert = gst_element_factory_make("audioconvert",  "sjAudioEntry");
		GstElement* capsfilter   = gst_element_factory_make("capsfilter",    NULL          );
		GstElement* volume       = gst_element_factory_make("volume",        "sjVolume"    );
		GstElement* audiosink    = gst_element_factory_make("autoaudiosink", NULL          );
		if( !m_pipeline || !decodebin || !audioconvert || !capsfilter || !volume || !audiosink ) {
			wxLogError("GStreamer error: Cannot create objects.");
			return NULL; // error
		}

		// create pipeline
		gst_bin_add_many(GST_BIN(m_pipeline), decodebin, audioconvert, capsfilter, volume, audiosink, NULL); // NULL marks end of list
		gst_element_link_many(audioconvert, capsfilter, volume, audiosink, NULL);
		g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_pad_added), this /*userdata*/);

		// add a message handler
		GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
			m_bus_watch_id = gst_bus_add_watch(bus, on_bus_message, this /*userdata*/);
		gst_object_unref(bus);

		// setup capsfilter and dsp callback
		GstCaps* caps = gst_caps_new_simple("audio/x-raw",
					"format", G_TYPE_STRING, "F32LE", // or S16LE, U8, ...
					"layout", G_TYPE_STRING, "interleaved",
					//"rate", G_TYPE_INT, info->rate, -- not interesting
					"channels", G_TYPE_INT, 2, // force stereo for easier DSP calculcations, maybe we will support more channels later, however, this has a very low priority
					NULL);
			 g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
		gst_caps_unref(caps);

		GstPad* pad = gst_element_get_static_pad(volume, "src");
			gulong probeid = gst_pad_add_probe(pad,
				(GstPadProbeType)(GST_PAD_PROBE_TYPE_BUFFER),
				on_pad_data, (gpointer)this/*userdata*/, NULL);
			if( probeid==0 ) {
				wxLogError("GStreamer Error: Cannot add probe callback.");
			}
		gst_object_unref(pad);
	}

	m_currStream = new SjGstreamerBackendStream(lane, uri, this, cb, userdata);
	if( m_currStream == NULL )
	{
		return NULL;
	}

	set_pipeline_state(GST_STATE_READY);

		GstElement* source = gst_bin_get_by_name(GST_BIN(m_pipeline), "sjSource");
		if( source )
		{
			WXSTRING_TO_GST(uri);
			g_object_set(G_OBJECT(source), "uri", uriGstStr, NULL /*NULL marks end of list*/);
		}

		if( seekMs > 0 )
		{
			gst_element_seek_simple(m_pipeline, GST_FORMAT_TIME,
				(GstSeekFlags)(GST_SEEK_FLAG_FLUSH|GST_SEEK_FLAG_KEY_UNIT), seekMs*MILLISEC_TO_NANOSEC_FACTOR);
		}

	set_pipeline_state(GST_STATE_PLAYING);

	return m_currStream;
}


SjBackendState SjGstreamerBackend::GetDeviceState()
{
	if( !m_pipeline ) { return SJBE_STATE_CLOSED; }

	GstState state;
	if( gst_element_get_state(m_pipeline, &state, NULL, 3000*MILLISEC_TO_NANOSEC_FACTOR /*wait max. 3 seconds*/) != GST_STATE_CHANGE_SUCCESS ) {
		return SJBE_STATE_PLAYING; // we assume, a stream is coming very soon
	}

	return state == GST_STATE_PAUSED? SJBE_STATE_PAUSED : SJBE_STATE_PLAYING;
}


void SjGstreamerBackend::SetDeviceState(SjBackendState state)
{
	if( !m_pipeline ) { return; }

	switch( state )
	{
		case SJBE_STATE_CLOSED:
			set_pipeline_state(GST_STATE_NULL);
			gst_object_unref(GST_OBJECT(m_pipeline));
			m_pipeline = NULL;
			g_source_remove(m_bus_watch_id);
			break;

		case SJBE_STATE_PLAYING:
			set_pipeline_state(GST_STATE_PLAYING);
			break;

		case SJBE_STATE_PAUSED:
			set_pipeline_state(GST_STATE_PAUSED);
			break;
	}
}


void SjGstreamerBackend::SetDeviceVol(double gain)
{
	if( m_pipeline == NULL ) {
		return; // not ready
	}

	// this does not set the "main" volume but the volume of the stream;
	// we cannot get louder than the OS-setting this way, however, this may be useful for our crossfading.
	GstElement* volumeElem = gst_bin_get_by_name(GST_BIN(m_pipeline), "sjVolume");
	if( volumeElem )
	{
		g_object_set(G_OBJECT(volumeElem), "volume", (gdouble)gain /*0: mute, 1.0: 100%, >1.0: additional gain*/, NULL /*NULL marks end of list*/);
	}
}


void SjGstreamerBackendStream::GetTime(long& totalMs, long& elapsedMs)
{
	totalMs   = -1; // unknown total time
	elapsedMs = -1; // unknown elapsed time

	gint64 ns;
	if( gst_element_query_duration(m_backend->m_pipeline, GST_FORMAT_TIME, &ns) ) {
		totalMs = ns/NANOSEC_TO_MILLISEC_DIVISOR;
	}

	if( gst_element_query_position(m_backend->m_pipeline, GST_FORMAT_TIME, &ns) ) {
		elapsedMs = ns/NANOSEC_TO_MILLISEC_DIVISOR;
	}
}


void SjGstreamerBackendStream::SeekAbs(long seekMs)
{
	gst_element_seek_simple(m_backend->m_pipeline, GST_FORMAT_TIME,
		(GstSeekFlags)(GST_SEEK_FLAG_FLUSH|GST_SEEK_FLAG_KEY_UNIT), seekMs*MILLISEC_TO_NANOSEC_FACTOR);
}


void SjGstreamerBackendStream::DestroyStream()
{
	if( m_backend->m_currStream == this )
	{
		m_backend->m_currStream = NULL;
	}

	m_backend->set_pipeline_state(GST_STATE_NULL);
	delete this;
}


#endif // SJ_USE_GSTREAMER
