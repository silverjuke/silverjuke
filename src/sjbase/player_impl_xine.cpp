/*********************************************************************************
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
 *********************************************************************************
 *
 * File:    player_impl_xine.cpp
 * Authors: Björn Petersen
 * Purpose: Player xine1 implementation, link with `-lxine`
 * OS:      all OS that have access to a xine library (open source)
 *
 *******************************************************************************
 *
 * Docs:
 * - xdg-open /usr/share/doc/libxine-dev/hackersguide/hackersguide.html &
 * - xdg-open /usr/include/xine.h &
 * - http://sourceforge.net/p/xine/mailman/search/
 *
 * Howto set audio volume:
 * - XINE_PARAM_AUDIO_VOLUME sets internally AO_PROP_MIXER_VOL, this seems not to be the system volume
 *
 * About the config-files:
 * - https://bugs.launchpad.net/ubuntu/+source/xine-ui/+bug/231507
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/player_impl_xine.h>
#include <xine.h>


/*******************************************************************************
 * SjPlayer - Init, Exit
 ******************************************************************************/


void SjPlayer::DoInit()
{
	m_impl                       = new SjPlayerImpl;
	m_impl->m_player             = this;
	m_impl->m_xine               = NULL;
	m_impl->m_ao_port            = NULL;
	m_impl->m_currStream         = NULL;
	m_impl->m_extListInitialized = false;
}


void SjPlayer::DoExit()
{
	if( m_impl )
	{
		DoStop();

		delete m_impl;
		m_impl  = NULL;
	}
}


bool SjPlayerImpl::InitXine()
{
	if( m_xine ) {
		return true; // success, already open
	}

	// load options from INI
	wxConfigBase* c = g_tools->m_config;
	m_iniDevice = c->Read(wxT("xine/device"), wxT("auto"));

	// TODO: XInitThreads may be needed (for videos only?), see hackersguide.html
	/*
	if (!XInitThreads()) {
	    wxLogDebug(wxT("InitXine()/XInitThreads() failed"));
	    return 1;
	}
	*/

	// preinit xine
	m_xine = xine_new();
	if( m_xine == NULL ) {
		goto Cleanup;
	}

	// load xine config file and init xine
	// TODO: this may also be possible by
	//xine_config_load(m_xine, "$HOME/.xine/config");

	// postinit xine
	xine_init(m_xine);

	// success, xine opened
	return true;

Cleanup:
	// error
	if( m_xine ) {
		xine_exit(m_xine);
		m_xine = NULL;
	}
	return false;
}



/*******************************************************************************
 *  SjPlayer - Configuration
 ******************************************************************************/


const SjExtList* SjPlayer::DoGetExtList()
{
	wxASSERT( m_impl );
	if( m_impl == NULL ) { return NULL; }

	if( !m_impl->m_extListInitialized )
	{
		m_impl->m_extListInitialized = true;
		if( m_impl->InitXine() )
		{
			char* spaceSeparatedExtListPtr = xine_get_file_extensions(m_impl->m_xine); // the returned pointer must be free()d when no longer used
			if( spaceSeparatedExtListPtr )
			{
				SjExtList allExt(wxString(spaceSeparatedExtListPtr, wxConvUTF8)); // allExt also includes images, playlists etc.
				free(spaceSeparatedExtListPtr);

				// do not add extensions that are known-unplayable (we do not use a positive list to be ready for unknwon formats,
				// if we fail here, ther user can ignore the extension manually)
				for( int i = allExt.GetCount()-1; i>=0; i-- ) {
					if( SjExtList::RepositoryType(allExt.GetExt(i)) != SJ_EXT_TYPE_KNOWN_UNPLAYABLE ) {
						m_impl->m_extList.AddExt(allExt.GetExt(i));
					}
				}
			}
		}
	}


	return &m_impl->m_extList;
}


class SjLittleReplayEnum : public SjLittleEnumStr
{
public:
	SjLittleReplayEnum(const wxString& name, const wxString& valuesNoptions,
	                     wxString* value, const wxString& defaultValue,
	                     const wxString& ini=wxEmptyString, SjIcon icon=SJ_ICON_LITTLEDEFAULT, bool saveToObject=TRUE)
		: SjLittleEnumStr(name, valuesNoptions, value, defaultValue, ini, icon, saveToObject)
	{
	}

	void OnFeedback()
	{
		g_mainFrame->ReplayIfPlaying();
	}
};


void SjPlayer::DoGetLittleOptions(SjArrayLittleOption& lo)
{
	if( !m_impl->InitXine() ) {
		return; // error
	}

	wxString options = wxT("auto|") + wxString(_("Default")), currOption;
	const char* const* pl = xine_list_audio_output_plugins(m_impl->m_xine);
	if( pl ) {
		while( *pl ) {
			currOption = wxString(*pl, wxConvUTF8);
			if( !currOption.IsEmpty() ) {
				options += wxT("|") + currOption +  wxT("|") + currOption;
			}
			pl++;
		}
	}
	lo.Add(new SjLittleReplayEnum( _("Device"), options,
						&m_impl->m_iniDevice, wxT("auto"), wxT("xine/device"), SJ_ICON_MODULE));
}


/*******************************************************************************
 *  SjPlayer - Play, Pause, etc.
 ******************************************************************************/


static void xine_event_listener_cb(void* user_data, const xine_event_t* event)
{
	SjXineStream* m_stream = (SjXineStream*)user_data;
	if( m_stream == NULL ) {
		return;
	}

	switch( event->type )
	{
		case XINE_EVENT_UI_PLAYBACK_FINISHED:
			// start the next stream, NOTE: we're in a non-main thread here!
			g_mainFrame->m_player.SendSignalToMainThread(THREAD_PREPARE_NEXT);
			break;
	}
}

class SjXineStream
{
public:
	SjXineStream(SjPlayerImpl* impl, const wxString& url)
	{
		wxASSERT(impl);
		m_impl        = impl;
		m_stream      = NULL;
		m_event_queue = NULL;
		m_url         = url;
	}

	~SjXineStream()
	{
		cleanup();
	}

	bool XinePlay(long ms = 0)
	{
		wxASSERT( m_stream == NULL && m_event_queue == NULL );
		if( m_stream || m_event_queue ) {
			wxLogError(wxT("xine_play() already called."));
			return false;
		}

		wxASSERT( m_impl->m_xine && m_impl->m_ao_port );
		if( m_impl->m_xine==NULL || m_impl->m_ao_port==NULL ) {
			wxLogError(wxT("xine not initialized."));
			return false;
		}

		// create a new stream
		m_stream = xine_stream_new(m_impl->m_xine, m_impl->m_ao_port, NULL);
		if( m_stream == NULL ) {
			wxLogError(wxT("xine_stream_new() failed."));
			return false;
		}

		// add event listener to stream
		m_event_queue = xine_event_new_queue(m_stream);
		if( m_event_queue == NULL ) {
			cleanup();
			wxLogError(wxT("xine_event_new_queue() failed."));
			return false;
		}
		xine_event_create_listener_thread(m_event_queue, xine_event_listener_cb, (void*)this);

		// open a URL for the stream
		if( !xine_open(m_stream, static_cast<const char*>(m_url.mb_str(wxConvUTF8))) )
		{
			// failed, as xine is compatible to file:-URLs, this URL may be http, ZIP or sth. like that.
			// try over by copying the URL to a local file
			bool failed = true;
			wxFileSystem fs;
			wxFSFile* fsFile = fs.OpenFile(m_url, wxFS_READ);
			if( fsFile )
			{
				wxString tempFileName = g_tools->m_cache.AddToManagedTemp(SjTools::GetExt(m_url), SJ_TEMP_PROTECT_TIL_EXIT);
				wxFile tempFile(tempFileName, wxFile::write);
				if( SjTools::CopyStreamToFile(*(fsFile->GetStream()), tempFile) )
				{
					if( xine_open(m_stream, static_cast<const char*>(tempFileName.mb_str(wxConvUTF8))) )
					{
						failed = false;
					}
				}
				delete fsFile;
			}

			if( failed )
			{
				cleanup();
				wxLogError(wxT("xine_open() failed."));
				return false;
			}
		}

		// finally, play
		if( !xine_play(m_stream, 0, ms) ) {
			cleanup();
			wxLogError(wxT("GotoAbsPos()/xine_play() failed."));
			return false;
		}

		return true;
	}

	xine_stream_t* GetXineStream() const
	{
		return m_stream; // may be NULL on errors or if XinePlay is not called!
	}

	wxString GetUrl() const
	{
        return m_url;
	}

private:
	void cleanup()
	{
		if( m_event_queue ) {
			xine_event_dispose_queue(m_event_queue);
			m_event_queue = NULL;
		}

		if( m_stream ) {
			xine_dispose(m_stream);
			m_stream = NULL;
		}
	}

	SjPlayerImpl*       m_impl;
	xine_stream_t*      m_stream;
	xine_event_queue_t*	m_event_queue;
	wxString            m_url; // may also be set it m_curr_stream is NULL (not cleared on errors)
};


void SjPlayer::DoPlay(long ms, bool fadeToPlay) // press on play
{
	if( m_impl->m_currStream )
	{
		if( m_paused )
		{
			// just switch state from pause to play
			xine_set_param(m_impl->m_currStream->GetXineStream(), XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
			m_paused = false;
		}
	}
	else
	{
		// if we're not playing, we're also not paused...
		m_paused = false;

		// open player and start the stream at the current position
		if( m_queue.GetCount()<=0 )
		{
			return; // don't log any error
		}

		if( !m_impl->InitXine() ) {
			return; // error
		}

		// open xine output ports (currently audio only)
		if( m_impl->m_ao_port == NULL ) {
			m_impl->m_ao_port = xine_open_audio_driver(m_impl->m_xine , static_cast<const char*>(m_impl->m_iniDevice.mb_str(wxConvUTF8)), NULL);
			if( m_impl->m_ao_port == NULL ) {
				wxLogError(wxT("Play()/xine_open_audio_driver() failed."));
				return;
			}
		}

		DoSetMainVol();

		// create a stream bound to the output
		long queuePos = m_queue.GetCurrPos();
		m_impl->m_currStream = new SjXineStream(m_impl, m_queue.GetUrlByPos(queuePos));
		if( !m_impl->m_currStream->XinePlay(ms) ) {
			wxLogError(wxT("DoPlay() failed."));

			// Xine stream is non-functional, don't touch
			delete m_impl->m_currStream;
			m_impl->m_currStream = NULL;
		}
	}
}


void SjPlayer::DoPause(bool fadeToPause)
{
	if( m_impl->m_currStream )
	{
		xine_set_param(m_impl->m_currStream->GetXineStream(), XINE_PARAM_SPEED, XINE_SPEED_PAUSE);
		m_paused = true;
	}
}


void SjPlayer::DoStop()
{
	if( m_impl->m_currStream )
	{
		delete m_impl->m_currStream;
		m_impl->m_currStream = NULL;
	}

	if( m_impl->m_ao_port )
	{
		xine_close_audio_driver(m_impl->m_xine, m_impl->m_ao_port);
		m_impl->m_ao_port = NULL;
	}

	if( m_impl->m_xine )
	{
		xine_exit(m_impl->m_xine);
		m_impl->m_xine = NULL;
	}

	// clear the extension list to allow changes of plugins
	m_impl->m_extListInitialized = false;
	m_impl->m_extList.Clear();
}


void SjPlayer::DoGotoAbsPos(long queuePos, bool fadeToPos)
{
	// DoGotoAbsPos() is only called if we're playing
	wxASSERT( m_impl->m_ao_port );
	if( !m_impl->m_ao_port ) { return; }

	// close old stream
	if( m_impl->m_currStream ) {
		delete m_impl->m_currStream;
		m_impl->m_currStream = NULL;
	}

	// create a new stream
	m_impl->m_currStream = new SjXineStream(m_impl, m_queue.GetUrlByPos(queuePos));
	if( !m_impl->m_currStream->XinePlay() ) {
		wxLogError(wxT("DoGotoAbsPos() failed."));
		return;
	}
}


/*******************************************************************************
 *  SjPlayer - About the playing URL, Misc.
 ******************************************************************************/


wxString SjPlayer::DoGetUrlOnAir()
{
	// the returned URL must be playing or paused (this function is also used to implement IsPlaying() etc.)
	wxASSERT( m_impl );
	if( m_impl == NULL || m_impl->m_currStream == NULL ) {
		return wxT(""); // no stream on air
	}

	return m_impl->m_currStream->GetUrl();
}


void SjPlayer::DoGetTime(long& totalMs, long& elapsedMs)
{
	if( m_impl->m_currStream )
	{
		totalMs = -1; // if there is a stream, the pos/length may be unknown
		elapsedMs = -1;
		int pos_stream, pos_time_ms, length_time_ms;
		if( xine_get_pos_length(m_impl->m_currStream->GetXineStream(), &pos_stream, &pos_time_ms, &length_time_ms) )
		{
			if( length_time_ms >= 0 ) {
				totalMs = length_time_ms;
			}

			if( pos_time_ms >= 0 ) {
				elapsedMs = pos_time_ms;
			}
		}
	}
	else
	{
		totalMs = 0; // "no stream" has a length of "0"
		elapsedMs = 0;
	}
}


void SjPlayer::DoSetMainVol()
{
	if( m_impl->m_currStream )
	{
		int v;
		v = (int)((double)100*m_mainGain);
		if( v < 0 ) v = 0;
		if( v > 100 ) v = 0;
		xine_set_param(m_impl->m_currStream->GetXineStream(), XINE_PARAM_AUDIO_VOLUME, v); // XINE_PARAM_AUDIO_VOLUME sets internally AO_PROP_MIXER_VOL,
	}
}


void SjPlayer::DoSeekAbs(long seekMs)
{
	if( m_impl->m_currStream )
	{
		if( !xine_play(m_impl->m_currStream->GetXineStream(), 0, seekMs) ) {
			wxLogDebug(wxT("DoSeekAbs() failed."));
		}
	}
}


void SjPlayer::DoGetVisData(unsigned char* pcmBuffer, long bytes, long visLatencyBytes)
{
	memset(pcmBuffer, 0, bytes); // TODO: fill the buffer with meaningful data ...
}


void SjPlayer::DoReceiveSignal(int signal, uintptr_t extraLong)
{
	wxLogDebug("SjPlayer::DoReceiveSignal() called ... ");

	if( !m_impl->InitXine() ) {
		wxLogDebug(wxT(" ... error: xine ist not ready."));
		return; // error
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
        if( m_impl->m_currStream ) {
			delete m_impl->m_currStream;
			m_impl->m_currStream = NULL;
		}
		m_impl->m_currStream = new SjXineStream(m_impl, newUrl);
		if( !m_impl->m_currStream->XinePlay() ) {
			wxLogDebug(wxT(" ... cannot create the new stream."));
			delete m_impl->m_currStream;
			m_impl->m_currStream = NULL;
		}

		// realize the new position in the UI
		m_queue.SetCurrPos(newQueuePos);
		SendSignalToMainThread(IDMODMSG_TRACK_ON_AIR_CHANGED);
	}
}




