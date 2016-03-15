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
#include <sjmodules/vis/vis_vidout_module.h>
#include <gst/video/videooverlay.h>


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


void SjGstreamerBackendStream::set_pipeline_state(GstState s)
{
	if( !m_pipeline ) {
		return; // not ready
	}

	if( gst_element_set_state(m_pipeline, s) == GST_STATE_CHANGE_ASYNC ) {
		gst_element_get_state(m_pipeline, NULL, NULL, 3000*MILLISEC_TO_NANOSEC_FACTOR /*async change, wait max. 3 seconds*/);
	}
}


GstBusSyncReply on_bus_sync_handler(GstBus* bus, GstMessage* msg, gpointer user_data)
{
	// ignore anything but 'prepare-window-handle' element messages
	if( !gst_is_video_overlay_prepare_window_handle_message(msg) )
		return GST_BUS_PASS;

	if( !g_vidoutModule || !g_vidoutModule->m_os_window_handle ) {
		wxLogError("GStreamer Error: Window for embedding not ready.");
		return GST_BUS_PASS; // normally, the video is shown in a separate window now.
	}

	// set window to use for video output, see
	// http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-base-libs/html/gst-plugins-base-libs-gstvideooverlay.html
	gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(msg)), (guintptr)g_vidoutModule->m_os_window_handle);
	gst_message_unref(msg);

	// success, we can drop the message from the asynchronous message pipe
	return GST_BUS_DROP;
}


gboolean on_bus_message(GstBus* bus, GstMessage* msg, gpointer userdata)
{
	SjGstreamerBackendStream* stream = (SjGstreamerBackendStream*)userdata;
	if( stream == NULL ) { return true; }

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
				wxLogError("GStreamer Error: %s: %s%s", objnameWxStr.c_str(), errormessageWxStr.c_str(), debugStr.c_str());

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
		if( prepareNext && !stream->m_eosSend )
		{
			stream->m_cbp.msg = SJBE_MSG_END_OF_STREAM;
			stream->m_cb(&stream->m_cbp);
			stream->m_eosSend = true;
		}
	}

	return true;
}


void on_pad_added(GstElement* decodebin, GstPad* newSourcePad, gpointer userdata)
{
	// a new pad appears in the "decodebin" element - link this to the
	// element with the name "sjAudioEntry"
	SjGstreamerBackendStream* stream = (SjGstreamerBackendStream*)userdata;
	if( stream == NULL ) { return; }

	// find out the type of the pad
	bool isVideoPad = false;
	{
		GstCaps* caps = gst_pad_get_current_caps(newSourcePad);
		if( !caps ) { return; /*error*/ }
			GstStructure* s = gst_caps_get_structure(caps, 0); // no need to free or unref the structure, it belongs to the GstCaps.
			const gchar* name = gst_structure_get_name (s); // sth. like "audio/x-raw" or "video/x-raw"
			GST_TO_WXSTRING(name);
			if( nameWxStr.StartsWith("video") )
			{
				stream->m_cbp.msg = SJBE_MSG_VIDEO_DETECTED;
				stream->m_cb(&stream->m_cbp);
				isVideoPad = true;
			}
		gst_caps_unref(caps);
	}

	if( isVideoPad )
	{
		// create video sink and connect the pad to it
		if( stream->m_backend->WantsVideo() )
		{
			GError* error = NULL;
			GstElement* videosink = gst_parse_bin_from_description(stream->m_backend->m_iniVideoPipeline, true, &error);
			if( error ) {
				const gchar* errormessage = error->message; GST_TO_WXSTRING(errormessage);
				wxLogError("GStreamer Error: %s. Please check the video configuration at Settings/Advanced.", errormessageWxStr.c_str());
				g_error_free(error);
			} // no else - we may be an error and a valid return object
			if( !videosink ) { wxLogError("GStream Error: Cannot create video sink."); return; }
			gst_bin_add(GST_BIN(stream->m_pipeline), videosink);

			GstPad* destSinkPad = gst_element_get_static_pad(videosink, "sink");
			if( !destSinkPad ) { wxLogError("GStream Error: Cannot get pad of video sink."); return; }

			GstPadLinkReturn linkret = gst_pad_link(newSourcePad, destSinkPad);
			if( linkret!=GST_PAD_LINK_OK ) {
				wxLogError("GStreamer error: Cannot link video.");
			}

			gst_element_sync_state_with_parent(videosink);
		}
	}
	else
	{
		// add audio pad to our audio sink
		GstElement* audioEntry = gst_bin_get_by_name(GST_BIN(stream->m_pipeline), "sjAudioEntry");
		if( audioEntry )
		{
			// get sink pad of the "audio entry element"
			GstPad* destSinkPad = gst_element_get_static_pad(audioEntry, "sink");

			// link the new source pad to the "audio entry element"
			GstPadLinkReturn linkret = gst_pad_link(newSourcePad, destSinkPad);
			if( linkret!=GST_PAD_LINK_OK ) {
				wxLogError("GStreamer error: Cannot link audio.");
			}

			gst_object_unref(audioEntry);
		}
	}
}


GstPadProbeReturn on_pad_data(GstPad* pad, GstPadProbeInfo* info, gpointer userdata)
{
	SjGstreamerBackendStream* stream = (SjGstreamerBackendStream*)userdata; if( stream == NULL ) { return GST_PAD_PROBE_OK; }

	// on the first call on a new stream, correct the "channels" and "samplerate"
	if( !stream->m_capsChecked )
	{
		stream->m_capsChecked = true;
		GstCaps* caps = gst_pad_get_current_caps(pad);
		if( caps ) {
			GstStructure* s = gst_caps_get_structure(caps, 0); // no need to free or unref the structure, it belongs to the GstCaps.
			if( s ) {
				gint v;
				if( gst_structure_get_int(s, "rate", &v) ) {
					if( v >= 1000 && v <= 1000000 ) {
						stream->m_cbp.samplerate = v;
					}
				}
				if( gst_structure_get_int(s, "channels", &v) ) {
					if( v >= 1 && v <= 32 ) {
						stream->m_cbp.channels = v;
					}
				}
			}
			gst_caps_unref(caps);
		}
	}

	// forward the buffer to the given callback
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


SjGstreamerBackend::SjGstreamerBackend(SjBackendId id)
	: SjBackend(id)
{
	// load settings
	// some pipeline examples:
	//     audioecho delay=500000000 intensity=0.6 feedback=0.4 ! autoaudiosink
	//     pulsesink
	//     filesink location=/tmp/raw
	#define AUDIOPIPELINE_ININAME "gstreamer/"+GetName()+"AudioPipeline"
	#define AUDIOPIPELINE_DEFAULT "autoaudiosink"
	#define VIDEOPIPELINE_ININAME "gstreamer/"+GetName()+"VideoPipeline"
	#define VIDEOPIPELINE_DEFAULT "autovideosink"
	wxConfigBase* c = g_tools->m_config;
	m_iniAudioPipeline = c->Read(AUDIOPIPELINE_ININAME, AUDIOPIPELINE_DEFAULT);
	if( WantsVideo() ) {
		m_iniVideoPipeline = c->Read(VIDEOPIPELINE_ININAME, VIDEOPIPELINE_DEFAULT);
	}

	// init, log version information
	static bool s_initialized = false;
	if( !s_initialized )
	{
		gst_init(NULL, NULL);
		s_initialized = true;

		guint major, minor, micro, nano;
		gst_version(&major, &minor, &micro, &nano);
		wxLogInfo("Using GStreamer %i.%i.%i.%i with pipeline \"%s\" (\"%s\")", (int)major, (int)minor, (int)micro, (int)nano,
			m_iniAudioPipeline.c_str(), m_iniVideoPipeline.c_str());
	}
}


void SjGstreamerBackend::GetLittleOptions(SjArrayLittleOption& lo)
{
	lo.Add(new SjLittleStringSel("GStreamer-Pipeline", &m_iniAudioPipeline, AUDIOPIPELINE_DEFAULT, AUDIOPIPELINE_ININAME, SJ_ICON_MODULE));

	if( WantsVideo() ) {
		lo.Add(new SjLittleStringSel("GStreamer-Video-Pipeline", &m_iniVideoPipeline, VIDEOPIPELINE_DEFAULT, VIDEOPIPELINE_ININAME, SJ_ICON_MODULE));
	}
}


SjBackendStream* SjGstreamerBackend::CreateStream(const wxString& uri, long seekMs, SjBackendCallback* cb, SjBackendUserdata* userdata)
{
	SjGstreamerBackendStream* stream = new SjGstreamerBackendStream(uri, this, cb, userdata);
	if( stream == NULL ) { return NULL; }

	/*
	              .--> audioconvert --> capsfilter --> (X) volume -> audiosink
	decodebin --> |                                     :
	              '--> videosink                        :
	                                                    : here we add our DSP handler
	*/

	// create objects
	// NB: is is the far faster part, on my computer creating the pipeline takes 4 ms while starting the stream takes 40 ms -
	// if there are no other problems on recreating the pipeline on every call, there is no need to change this - esp. as there are some advantages as easier cleaning up, different samplerates etc.
	GError* error = NULL;
	stream->m_pipeline       = gst_pipeline_new        (                 "sjPlayer"    );
	GstElement* decodebin    = gst_element_factory_make("uridecodebin",  "sjSource"    );
	GstElement* audioconvert = gst_element_factory_make("audioconvert",  "sjAudioEntry");
	GstElement* capsfilter   = gst_element_factory_make("capsfilter",    NULL          );
	GstElement* volume       = gst_element_factory_make("volume",        "sjVolume"    );
	GstElement* audiosink    = gst_parse_bin_from_description(m_iniAudioPipeline, true, &error);
	if( error ) {
		const gchar* errormessage = error->message; GST_TO_WXSTRING(errormessage);
		wxLogError("GStreamer Error: %s. Please check the audio configuration at Settings/Advanced.", errormessageWxStr.c_str());
		g_error_free(error);
	} // no "return", no "else" - it may be possible, the pipeline is created even on errors, see http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/gstreamer-GstParse.html#gst-parse-launch
	if( !stream->m_pipeline || !decodebin || !audioconvert || !capsfilter || !volume || !audiosink ) {
		wxLogError("GStreamer error: Cannot create objects.");
		delete stream;
		return NULL; // error
	}

	// create pipeline
	gst_bin_add_many(GST_BIN(stream->m_pipeline), decodebin, audioconvert, capsfilter, volume, audiosink, NULL); // NULL marks end of list
	gst_element_link_many(audioconvert, capsfilter, volume, audiosink, NULL);
	g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_pad_added), stream /*userdata*/);

	// add a message handler
	GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(stream->m_pipeline));
		stream->m_bus_watch_id = gst_bus_add_watch(bus, on_bus_message, stream /*userdata*/);
		gst_bus_set_sync_handler(bus, on_bus_sync_handler, stream /*userdata*/, NULL);
	gst_object_unref(bus);

	// setup capsfilter and dsp callback
	GstCaps* caps = gst_caps_new_simple("audio/x-raw",
				"format", G_TYPE_STRING, "F32LE",       // or S16LE, U8, ...
				"layout", G_TYPE_STRING, "interleaved", // LRLRLRLRLRLRLR ...
				//"rate", G_TYPE_INT, info->rate,       // if we set a fixed rate, we must probably add a resampler to the pipeline, however, currently this is not needed
				//"channels", G_TYPE_INT, 2,            // enable this to force a fixed number of channels, currently not needed
				NULL);
		 g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
	gst_caps_unref(caps);

	GstPad* pad = gst_element_get_static_pad(volume, "sink"); // sink/src is defined from the view of within the element: elements receive data on their sink pads and generate data on their source pads.
		gulong probeid = gst_pad_add_probe(pad,
			(GstPadProbeType)(GST_PAD_PROBE_TYPE_BUFFER),
			on_pad_data, (gpointer)stream/*userdata*/, NULL);
		if( probeid==0 ) {
			wxLogError("GStreamer Error: Cannot add probe callback.");
		}
	gst_object_unref(pad);

	// open stream
	stream->set_pipeline_state(GST_STATE_READY);

		GstElement* source = gst_bin_get_by_name(GST_BIN(stream->m_pipeline), "sjSource");
		if( source )
		{
			WXSTRING_TO_GST(uri);
			g_object_set(G_OBJECT(source), "uri", uriGstStr, NULL /*NULL marks end of list*/);
		}

	stream->set_pipeline_state(GST_STATE_PLAYING);

	if( seekMs > 0 )
	{
		gst_element_seek_simple(stream->m_pipeline, GST_FORMAT_TIME,
			(GstSeekFlags)(GST_SEEK_FLAG_FLUSH|GST_SEEK_FLAG_KEY_UNIT), seekMs*MILLISEC_TO_NANOSEC_FACTOR);
	}

	return stream;
}


SjBackendState SjGstreamerBackend::GetDeviceState() const
{
	const wxArrayPtrVoid& allStreams = GetAllStreams();
	size_t i, iCnt = allStreams.GetCount();
	if( iCnt == 0 ) {
		return SJBE_STATE_CLOSED;
	}

	for( i = 0; i < iCnt; i++ )
	{
		SjGstreamerBackendStream* stream = (SjGstreamerBackendStream*)allStreams.Item(i);
		GstState state;
		if( gst_element_get_state(stream->m_pipeline, &state, NULL, 3000*MILLISEC_TO_NANOSEC_FACTOR /*wait max. 3 seconds*/) != GST_STATE_CHANGE_SUCCESS ) {
			return SJBE_STATE_PLAYING; // we assume, a stream is coming very soon
		}
		if( state != GST_STATE_PAUSED ) {
			return SJBE_STATE_PLAYING;
		}
	}

	return SJBE_STATE_PAUSED;
}


void SjGstreamerBackend::SetDeviceState(SjBackendState state)
{
	if( state == SJBE_STATE_CLOSED ) {
		return; // if there are no streams on the device, it is closed, nothing special to do here
	}

	const wxArrayPtrVoid& allStreams = GetAllStreams();
	size_t i, iCnt = allStreams.GetCount();
	for( i = 0; i < iCnt; i++ )
	{
		SjGstreamerBackendStream* stream = (SjGstreamerBackendStream*)allStreams.Item(i);

		stream->set_pipeline_state(state==SJBE_STATE_PLAYING? GST_STATE_PLAYING : GST_STATE_PAUSED);
	}
}


void SjGstreamerBackend::SetDeviceVol(double gain)
{
	const wxArrayPtrVoid& allStreams = GetAllStreams();
	size_t i, iCnt = allStreams.GetCount();
	for( i = 0; i < iCnt; i++ )
	{
		SjGstreamerBackendStream* stream = (SjGstreamerBackendStream*)allStreams.Item(i);

		// this does not set the "main" volume but the volume of the stream;
		// we cannot get louder than the OS-setting this way, however, this may be useful for our crossfading.
		GstElement* volumeElem = gst_bin_get_by_name(GST_BIN(stream->m_pipeline), "sjVolume");
		if( volumeElem )
		{
			g_object_set(G_OBJECT(volumeElem), "volume", (gdouble)gain /*0: mute, 1.0: 100%, >1.0: additional gain*/, NULL /*NULL marks end of list*/);
		}
	}
}


void SjGstreamerBackendStream::GetTime(long& totalMs, long& elapsedMs)
{
	totalMs   = -1; // unknown total time
	elapsedMs = -1; // unknown elapsed time

	gint64 ns;
	if( gst_element_query_duration(m_pipeline, GST_FORMAT_TIME, &ns) ) {
		totalMs = ns/NANOSEC_TO_MILLISEC_DIVISOR;
	}

	if( gst_element_query_position(m_pipeline, GST_FORMAT_TIME, &ns) ) {
		elapsedMs = ns/NANOSEC_TO_MILLISEC_DIVISOR;
	}
}


void SjGstreamerBackendStream::SeekAbs(long seekMs)
{
	gst_element_seek_simple(m_pipeline, GST_FORMAT_TIME,
		(GstSeekFlags)(GST_SEEK_FLAG_FLUSH|GST_SEEK_FLAG_KEY_UNIT), seekMs*MILLISEC_TO_NANOSEC_FACTOR);
}


SjGstreamerBackendStream::~SjGstreamerBackendStream()
{
	set_pipeline_state(GST_STATE_NULL);
	gst_object_unref(GST_OBJECT(m_pipeline));
	m_pipeline = NULL;
	g_source_remove(m_bus_watch_id);
	m_bus_watch_id = 0;
}


#endif // SJ_USE_GSTREAMER
