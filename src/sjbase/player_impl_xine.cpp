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
 * - <xine-lib-N.N.N>/doc/hackersguide/hackersguide.html
 * - /usr/include/xine.h
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
	m_impl                      = new SjPlayerImpl;
	m_impl->m_player            = this;
	m_impl->m_xine              = NULL;
	m_impl->m_ao_port           = NULL;
	m_impl->m_curr_stream       = NULL;
	m_impl->m_extListInitialized= false;
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


void SjPlayer::DoPlay(long ms, bool fadeToPlay) // press on play
{
	if( m_impl->m_curr_stream )
	{
		if( m_paused )
		{
			// just switch state from pause to play
			xine_set_param(m_impl->m_curr_stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
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
			m_impl->m_ao_port = xine_open_audio_driver(m_impl->m_xine , m_impl->m_iniDevice.mb_str(wxConvUTF8), NULL);
			if( m_impl->m_ao_port == NULL ) {
				wxLogError(wxT("Play()/xine_open_audio_driver() failed."));
				return;
			}
		}

		// create a stream bound to the output
		m_impl->m_curr_stream = xine_stream_new(m_impl->m_xine, m_impl->m_ao_port, NULL);
		if( m_impl->m_curr_stream == NULL ) {
			wxLogError(wxT("Play()/xine_stream_new() failed."));
			return;
		}

		// open a URL for the stream
		long queuePos = m_queue.GetCurrPos();
		m_impl->m_curr_url = m_queue.GetUrlByPos(queuePos);
		if( !xine_open(m_impl->m_curr_stream, m_impl->m_curr_url.mb_str(wxConvUTF8)) ) {
			xine_dispose(m_impl->m_curr_stream);
			m_impl->m_curr_stream = NULL;
			wxLogError(wxT("Play()/xine_open() failed."));
			return;
		}

		// finally, play
		DoSetMainVol();
		if( !xine_play(m_impl->m_curr_stream, 0, ms) ) {
			xine_dispose(m_impl->m_curr_stream);
			m_impl->m_curr_stream = NULL;
			wxLogError(wxT("Play()/xine_play() failed."));
			return;
		}
	}
}


void SjPlayer::DoPause(bool fadeToPause)
{
	if( m_impl->m_curr_stream )
	{
		xine_set_param(m_impl->m_curr_stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE);
		m_paused = true;
	}
}


void SjPlayer::DoStop()
{
	if( m_impl->m_curr_stream )
	{
		xine_dispose(m_impl->m_curr_stream);
		m_impl->m_curr_stream = NULL;
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
	xine_dispose(m_impl->m_curr_stream);
	m_impl->m_curr_stream = NULL;

	// create a new stream
	m_impl->m_curr_stream = xine_stream_new(m_impl->m_xine, m_impl->m_ao_port, NULL);
	if( m_impl->m_curr_stream == NULL ) {
		wxLogError(wxT("GotoAbsPos()/xine_stream_new() failed."));
		return;
	}

	// open a URL for the stream
	m_impl->m_curr_url = m_queue.GetUrlByPos(queuePos);
	if( !xine_open(m_impl->m_curr_stream, m_impl->m_curr_url.mb_str(wxConvUTF8)) ) {
		xine_dispose(m_impl->m_curr_stream);
		m_impl->m_curr_stream = NULL;
		wxLogError(wxT("GotoAbsPos()/xine_open() failed."));
		return;
	}

	// finally, play
	if( !xine_play(m_impl->m_curr_stream, 0, 0) ) {
		xine_dispose(m_impl->m_curr_stream);
		m_impl->m_curr_stream = NULL;
		wxLogError(wxT("GotoAbsPos()/xine_play() failed."));
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
	if( m_impl == NULL || m_impl->m_curr_stream == NULL ) {
		return wxT(""); // no stream on air
	}

	return m_impl->m_curr_url;
}


void SjPlayer::DoGetTime(long& totalMs, long& elapsedMs)
{
	if( m_impl->m_curr_stream )
	{
		totalMs = -1; // if there is a stream, the pos/length may be unknown
		elapsedMs = -1;
		int pos_stream, pos_time_ms, length_time_ms;
		if( xine_get_pos_length(m_impl->m_curr_stream, &pos_stream, &pos_time_ms, &length_time_ms) )
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
	if( m_impl->m_curr_stream )
	{
		int v;
		v = (int)((double)100*m_mainGain);
		if( v < 0 ) v = 0;
		if( v > 100 ) v = 0;
		xine_set_param(m_impl->m_curr_stream, XINE_PARAM_AUDIO_VOLUME, v); // XINE_PARAM_AUDIO_VOLUME sets internally AO_PROP_MIXER_VOL,
	}
}


void SjPlayer::DoSeekAbs(long seekMs)
{
	if( m_impl->m_curr_stream )
	{
		if( xine_play(m_impl->m_curr_stream, 0, seekMs) ) {
			wxLogDebug(wxT("DoSeekAbs() failed."));
		}
	}
}



void SjPlayer::DoGetVisData(unsigned char* pcmBuffer, long bytes, long visLatencyBytes)
{
}


long SjPlayer::DoGetVisPosition()
{
	if( !m_impl->InitXine() ) {
		return 0; // error
	}

	return 0;
}


void SjPlayer::DoReceiveSignal(int signal, uintptr_t extraLong)
{
	if( !m_impl->InitXine() ) {
		return; // error
	}
}




