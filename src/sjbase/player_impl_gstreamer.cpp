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
 * File:    player_impl_gstreamer.cpp
 * Authors: Björn Petersen
 * Purpose: Player GStreamer implementation
 * OS:      all OS that have access to GStreamer (open source)
 *
 *******************************************************************************
 *
 * playbin:
 * http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-base-plugins/html/gst-plugins-base-plugins-playbin.html
 * playbin2:
 * http://www.freedesktop.org/software/gstreamer-sdk/data/docs/2012.5/gst-plugins-base-plugins-0.10/gst-plugins-base-plugins-playbin2.html#playbin2
 *
 ******************************************************************************/


#include <sjbase/base.h>
#if SJ_USE_GSTREAMER
#include <gst/gst.h>


/*******************************************************************************
 * SjPlayer - Tools
 ******************************************************************************/


#define GST_TO_WXSTRING(a) \
	 wxString a##WxStr = wxString((a), wxConvUTF8);

#define WXSTRING_TO_GST(a) \
	 const wxCharBuffer a##__tempCharBuf = (a).mb_str(wxConvUTF8); \
	 const char* a##GstStr = a##__tempCharBuf.data();

#define MILLISEC_TO_NANOSEC_FACTOR     1000000L
#define NANOSEC_TO_MILLISEC_DIVISOR    1000000L


/*******************************************************************************
 * SjPlayer - Init, Exit
 ******************************************************************************/


class SjPlayerImpl
{
	public:
		GstElement* m_pipeline;
		guint       m_bus_watch_id;

		wxString    m_pipelineUrl;
		uint32_t    m_startingTime;
		bool        m_eosSend;

		SjExtList	m_extList;
		bool		m_extListInitialized;

		void set_pipeline_state(GstState s)
		{
			if( !m_pipeline ) {
				return; // not ready
			}

			if( gst_element_set_state(m_pipeline, s) == GST_STATE_CHANGE_ASYNC ) {
				gst_element_get_state(m_pipeline, NULL, NULL, 3000*MILLISEC_TO_NANOSEC_FACTOR /*async change, wait max. 3 seconds*/);
			}
		}

		void set_pipeline_uri(const wxString& uri)
		{
			if( !m_pipeline ) {
				return; // not ready
			}

			WXSTRING_TO_GST(uri);
			g_object_set(G_OBJECT(m_pipeline), "uri", uriGstStr, NULL);

			m_pipelineUrl = uri;
			m_startingTime = wxDateTime::Now().GetAsDOS();
			m_eosSend = false;
		}
};


void SjPlayer::DoInit()
{
	// called one time upon program start
	m_impl = new SjPlayerImpl;
	m_impl->m_pipeline = NULL;
	m_impl->m_bus_watch_id = 0;
	m_impl->m_extListInitialized = false;
	m_impl->m_startingTime = 0;
	m_impl->m_eosSend = false;

	// init, log version information
	gst_init(NULL, NULL);

	guint major, minor, micro, nano;
	gst_version(&major, &minor, &micro, &nano);
	wxLogInfo("Using GStreamer %i.%i.%i.%i", (int)major, (int)minor, (int)micro, (int)nano);
}


void SjPlayer::DoExit()
{
	// called one time upon program shutdown
	if( m_impl )
	{
		DoStop();

		delete m_impl;
		m_impl  = NULL;
	}
}


/*******************************************************************************
 * SjPlayer - Configuration
 ******************************************************************************/


const SjExtList* SjPlayer::DoGetExtList()
{
	if( !m_impl->m_extListInitialized )
	{
		// TODO: return really supported extensions here - or simply define a global "default" list and allow the user to modify it
		// (instead of let the user define extensions to ignore)
		m_impl->m_extList.AddExt("16sv, 4xm, 669, 8svx, aac, ac3, aif, aiff, amf, anim, anim3, anim5, anim7, anim8, anx, asc, "
			"asf, ass, asx, au, aud, avi, axa, axv, cak, cin, cpk, dat, dif, dps, dts, dv, f4a, f4v, film, flac, flc, fli, flv, "
			"ik2, iki, ilbm, it, m2t, m2ts, m4a, m4b, mdl, med, mjpg, mkv, mng, mod, mov, mp+, mp2, mp3, mp4, mpa, mpc, mpeg, mpega, "
			"mpg, mpp, mpv, mts, mv8, mve, nsv, oga, ogg, ogm, ogv, ogx, pes, pva, qt, qtl, ra, rm, rmvb, roq, s3m, shn, smi, snd, "
			"spx, srt, ssa, stm, str, sub, svx, trp, ts, tta, vmd, vob, voc, vox, vqa, wav, wax, wbm, webm, wma, wmv, wv, wve, wvp, "
			"wvx, xa, xa1, xa2, xap, xas, xm, y4m"); // (list is from xine)
	}
	return &m_impl->m_extList;
}


void SjPlayer::DoGetLittleOptions(SjArrayLittleOption& lo)
{
}


/*******************************************************************************
 *  SjPlayer - Play, Pause, etc.
 ******************************************************************************/


static gboolean bus_call(GstBus* bus, GstMessage* msg, gpointer userdata)
{
	SjPlayer* player = (SjPlayer*)userdata;
	if( player == NULL || player->m_impl == NULL ) { return true; }

	switch( GST_MESSAGE_TYPE(msg) )
	{
		case GST_MESSAGE_ERROR:
			// there may be series of error messages for one stream.
			// error messages are normally not followed by a GST_MESSAGE_EOS error
			{
				// get information about the error
				gchar*  debug = NULL;
				GError* error = NULL;
				gst_message_parse_error(msg, &error, &debug);

				// log the error
				if( debug ) {
					GST_TO_WXSTRING(debug);
					wxLogError("GStreamer error: %s", debugWxStr.c_str());
				}
				const gchar* errormessage = error->message;        GST_TO_WXSTRING(errormessage);
				const gchar* objname =  GST_OBJECT_NAME(msg->src); GST_TO_WXSTRING(objname);
				wxLogError("GStreamer error: %s: %s", objnameWxStr.c_str(), errormessageWxStr.c_str());

				g_free (debug);
				g_error_free (error);
			}
			// fall through

		case GST_MESSAGE_EOS:
			// we go here if we reach the end of the stream or if an error occurs (fall through from above)
			if( !player->m_impl->m_eosSend )
			{
				player->m_impl->m_eosSend = true;
				player->SendSignalToMainThread(THREAD_PREPARE_NEXT);
			}
			break;

		default:
			break;
	}

	return true;
}


void SjPlayer::DoPlay(long seekMs, bool fadeToPlay) // press on play
{
	if( m_impl->m_pipelineUrl.IsEmpty() )
	{
		// get file to play
		wxString url = m_queue.GetUrlByPos(-1);
		if( url.IsEmpty() ) {
			wxLogError("GStreamer error: URL is empty.");
			return; // error;
		}

		// create pipeline, add message handler to it
		if( m_impl->m_pipeline == NULL )
		{
			m_impl->m_pipeline = gst_element_factory_make("playbin" /*playbin2 does not exist in 1.2.4*/, "sjPlaybin");
			if( !m_impl->m_pipeline ) {
				wxLogError("GStreamer error: Cannot create playbin.");
				return; // error
			}

			GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_impl->m_pipeline));
			m_impl->m_bus_watch_id = gst_bus_add_watch(bus, bus_call, this /*userdata*/);
			gst_object_unref(bus);
		}

		// set source URI
		m_impl->set_pipeline_uri(url);

		// initial seek?
		if( seekMs > 0 )
		{
			gst_element_seek_simple(m_impl->m_pipeline, GST_FORMAT_TIME,
				(GstSeekFlags)(GST_SEEK_FLAG_FLUSH|GST_SEEK_FLAG_KEY_UNIT), seekMs*MILLISEC_TO_NANOSEC_FACTOR);
		}
	}

	// Set the pipeline to "playing" state
    m_impl->set_pipeline_state(GST_STATE_PLAYING);

	// success
	m_paused = false;
	return;
}


void SjPlayer::DoPause(bool fadeToPause)
{
	if( m_impl->m_pipelineUrl.IsEmpty() ) {
		return; // no stream
	}

	if( m_paused ) {
		return; // already paused
	}

	m_impl->set_pipeline_state(GST_STATE_PAUSED);

	// success
	m_paused = true;
}


void SjPlayer::DoStop()
{
	if( !m_impl->m_pipeline ) {
		return;
	}

	SaveGatheredInfo(m_impl->m_pipelineUrl, m_impl->m_startingTime, NULL, 0);

	m_impl->set_pipeline_state(GST_STATE_NULL);

	gst_object_unref(GST_OBJECT(m_impl->m_pipeline));
	m_impl->m_pipeline = NULL;
	m_impl->m_pipelineUrl.Clear();
	m_impl->m_startingTime = 0;

	g_source_remove(m_impl->m_bus_watch_id);
}


void SjPlayer::DoGotoAbsPos(long queuePos, bool fadeToPos)
{
	// only called if the player is playing, not on paused/stopped
	if( m_impl->m_pipeline == NULL || m_paused ) {
		return;
	}

	if( !m_impl->m_pipelineUrl.IsEmpty() )
	{
		SaveGatheredInfo(m_impl->m_pipelineUrl, m_impl->m_startingTime, NULL, 0);
		m_impl->m_pipelineUrl.Clear();
		m_impl->m_startingTime = 0;
	}

	// first, we need to set the pipeline state to "ready"
	m_impl->set_pipeline_state(GST_STATE_READY);

	// then, open the next URL
	wxString url = m_queue.GetUrlByPos(queuePos);
	if( url.IsEmpty() ) {
		wxLogError("GStreamer error: URL is empty.");
		return; // error;
	}

	m_impl->set_pipeline_uri(url);

	// finally, set the pipeline state to "playing" again
	m_impl->set_pipeline_state(GST_STATE_PLAYING);
}


/*******************************************************************************
 * SjPlayer - About the playing URL, Misc.
 ******************************************************************************/


wxString SjPlayer::DoGetUrlOnAir()
{
	// the returned URL must be playing or paused (this function is also used to implement IsPlaying() etc.)

	if( m_impl->m_pipeline == NULL || m_impl->m_pipelineUrl.IsEmpty() ) {
		return ""; // there is no stream
	}

	GstState state;
	if( gst_element_get_state(m_impl->m_pipeline, &state, NULL, 3000*MILLISEC_TO_NANOSEC_FACTOR /*wait max. 3 seconds*/)!=GST_STATE_CHANGE_SUCCESS ) {
		return ""; // error
	}

	if( state == GST_STATE_PAUSED
	 || state == GST_STATE_PLAYING )
	{
		return m_impl->m_pipelineUrl;
	}

	return ""; // no stream on air
}


void SjPlayer::DoGetTime(long& totalMs, long& elapsedMs)
{
	totalMs   = -1; // unknown total time
	elapsedMs = -1; // unknown elapsed time

	if( m_impl->m_pipeline == NULL || m_impl->m_pipelineUrl.IsEmpty() ) {
		return; // there is no stream
	}

	gint64 ns;
	if( gst_element_query_duration(m_impl->m_pipeline, GST_FORMAT_TIME, &ns) ) {
		totalMs = ns/NANOSEC_TO_MILLISEC_DIVISOR;
	}

	if( gst_element_query_position(m_impl->m_pipeline, GST_FORMAT_TIME, &ns) ) {
		elapsedMs = ns/NANOSEC_TO_MILLISEC_DIVISOR;
	}
}


void SjPlayer::DoSetMainVol()
{
	if( m_impl->m_pipeline == NULL ) {
		return; // not ready
	}

	// this does not set the "main" volume but the volume of the stream;
	// we cannot get louder than the OS-setting this way, however, this may be useful for our crossfading.
	gdouble vol = m_mainGain;
	g_object_set(G_OBJECT(m_impl->m_pipeline), "volume", vol /*0: mute, 1.0: 100%, >1.0: additional gain*/, NULL);
}


void SjPlayer::DoSeekAbs(long seekMs)
{
	if( m_impl->m_pipeline == NULL || m_impl->m_pipelineUrl.IsEmpty() ) {
		return; // there is no stream
	}

	gst_element_seek_simple(m_impl->m_pipeline, GST_FORMAT_TIME,
		(GstSeekFlags)(GST_SEEK_FLAG_FLUSH|GST_SEEK_FLAG_KEY_UNIT), seekMs*MILLISEC_TO_NANOSEC_FACTOR);
}


void SjPlayer::DoGetVisData(unsigned char* pcmBuffer, long bytes, long visLatencyBytes)
{
	memset(pcmBuffer, 0, bytes); // TODO: fill the buffer with meaningful data ...
}


void SjPlayer::DoReceiveSignal(int signal, uintptr_t extraLong)
{
	if( !m_impl->m_pipeline )
	{
		return; // player not ready
	}

	if( signal == THREAD_PREPARE_NEXT || signal == THREAD_OUT_OF_DATA )
	{
		// find out the next url to play
		wxString	newUrl;

		// try to get next url from queue
		long newQueuePos = m_queue.GetNextPos(SJ_PREVNEXT_REGARD_REPEAT);
		if( newQueuePos == -1 )
		{
			// try to enqueue auto-play url
			g_mainFrame->m_autoCtrl.DoAutoPlayIfEnabled(false /*ignoreTimeouts*/);
			newQueuePos = m_queue.GetNextPos(SJ_PREVNEXT_REGARD_REPEAT);

			if( newQueuePos == -1 )
			{
				// no chance, there is nothing more to play ...
				if( signal == THREAD_PREPARE_NEXT )
				{
					g_mainFrame->m_player.SendSignalToMainThread(THREAD_OUT_OF_DATA); // send a modified signal, no direct call as Receivesignal() will handle some cases exclusively
				}
				else if( signal == THREAD_OUT_OF_DATA )
				{
					wxLogDebug(wxT(" ... receiving THREAD_OUT_OF_DATA, stopping and sending IDMODMSG_PLAYER_STOPPED_BY_EOQ"));
					Stop();
					SendSignalToMainThread(IDMODMSG_PLAYER_STOPPED_BY_EOQ);
				}
				return;
			}
		}
		newUrl = m_queue.GetUrlByPos(newQueuePos);

		// has the URL just failed? try again in the next message look
		wxLogDebug(wxT(" ... new URL is \"%s\""), newUrl.c_str());

		if( m_failedUrls.Index( newUrl ) != wxNOT_FOUND )
		{
			wxLogDebug(wxT(" ... the URL has failed before, starting over."));
			m_queue.SetCurrPos(newQueuePos);
			SendSignalToMainThread(signal); // start over
			return;
		}

		// try to create the next stream
        if( !m_impl->m_pipelineUrl.IsEmpty() ) {
			SaveGatheredInfo(m_impl->m_pipelineUrl, m_impl->m_startingTime, NULL, 0);
			m_impl->m_pipelineUrl.Clear();
			m_impl->m_startingTime = 0;
		}

		m_impl->set_pipeline_state(GST_STATE_READY);

		m_impl->set_pipeline_uri(newUrl);

		m_impl->set_pipeline_state(GST_STATE_PLAYING);

		// realize the new position in the UI
		m_queue.SetCurrPos(newQueuePos);
		SendSignalToMainThread(IDMODMSG_TRACK_ON_AIR_CHANGED);
	}
}


#endif // SJ_USE_GSTREAMER

