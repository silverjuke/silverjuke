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
 * File:    backend_xine.cpp
 * Authors: Björn Petersen
 * Purpose: Player xine1 implementation, link with `-lxine`
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
#if SJ_USE_XINE
#include <sjbase/backend_xine.h>


/*******************************************************************************
 * Configuration
 ******************************************************************************/


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


void SjXineBackend::GetLittleOptions(SjArrayLittleOption& lo)
{
	if( !s_xine ) {
		return; // error
	}

	wxString options = "auto|" + wxString(_("Default")), currOption;
	const char* const* pl = xine_list_audio_output_plugins(s_xine);
	if( pl ) {
		while( *pl ) {
			currOption = wxString(*pl, wxConvUTF8);
			if( !currOption.IsEmpty() ) {
				options += "|" + currOption +  "|" + currOption;
			}
			pl++;
		}
	}

	#define DEVICE_ININAME "xine/"+GetName()+"device"
	#define DEVICE_DEFAULT "auto"
	lo.Add(new SjLittleReplayEnum( _("Device"), options,
						&m_iniDevice, DEVICE_DEFAULT, DEVICE_ININAME, SJ_ICON_MODULE));
}


/*******************************************************************************
 * Public Backend Implementation
 ******************************************************************************/


xine_t* SjXineBackend::s_xine       = NULL;
int     SjXineBackend::s_xine_usage = 0;


SjXineBackend::SjXineBackend(SjBackendId device)
	: SjBackend(device)
{
	m_ao_port            = NULL;
	m_currStream         = NULL;

	// load options from INI
	wxConfigBase* c = g_tools->m_config;
	m_iniDevice = c->Read(DEVICE_ININAME, DEVICE_DEFAULT);

	// TODO: XInitThreads may be needed (for videos only?), see hackersguide.html
	/*
	if (!XInitThreads()) {
	    wxLogError("Xine Error: XInitThreads() failed.");
	    return;
	}
	*/

	// preinit xine
	s_xine_usage++;
	if( s_xine_usage == 1 )
	{
		s_xine = xine_new();
		if( s_xine )
		{
			// load xine config file and init xine
			// TODO: this may also be possible by
			//xine_config_load(m_xine, "$HOME/.xine/config");

			// postinit xine
			xine_init(s_xine);

			// show version information
			wxString version(xine_get_version_string());
			wxLogInfo("Loading xine %s using output \"%s\".", version.c_str(), m_iniDevice.c_str());
		}
		else
		{
			wxLogError("Xine Error: Cannot init.");
		}
	}
}


SjXineBackend::~SjXineBackend()
{
	SetDeviceState(SJBE_STATE_CLOSED);

	s_xine_usage--;
	if( s_xine_usage == 0 )
	{
		if( s_xine ) {
			xine_exit(s_xine);
		}
	}

	delete this;
}


static void xine_event_listener_cb(void* user_data, const xine_event_t* event)
{
	SjXineBackendStream* stream = (SjXineBackendStream*)user_data;
	if( stream == NULL ) { return; }

	switch( event->type )
	{
		case XINE_EVENT_UI_PLAYBACK_FINISHED:
			// start the next stream, NOTE: we're in a non-main thread here!
			stream->m_cbp.msg = SJBE_MSG_END_OF_STREAM;
			stream->m_cb(&stream->m_cbp);
			break;
	}
}


SjBackendStream* SjXineBackend::CreateStream(const wxString& url, long seekMs, SjBackendCallback* cb, void* userdata)
{
	if( !s_xine )      { wxLogError("Xine Error: Initialization falied."); return NULL; }
	if( m_currStream ) { wxLogError("Xine Error: There is already a stream on this lane."); return NULL;}

	// open audio port
	if( !m_ao_port )
	{
		m_ao_port = xine_open_audio_driver(s_xine , static_cast<const char*>(m_iniDevice.mb_str(wxConvUTF8)), NULL);
		if( !m_ao_port ) {
			wxLogError("Xine Error: xine_open_audio_driver() failed.");
			return NULL;
		}
	}

	// create stream objects
	SjXineBackendStream* newStream = new SjXineBackendStream(url, this, cb, userdata);
	if( !newStream ) {
		wxLogError("Xine Error: new SjXineBackendStream() failed.");
		return NULL;
	}

	newStream->m_xine_stream = xine_stream_new(s_xine, m_ao_port, NULL);
	if( newStream->m_xine_stream == NULL ) {
		delete newStream;
		wxLogError("Xine Error: xine_stream_new() failed.");
		return NULL;
	}

	// add event listener to stream
	newStream->m_event_queue = xine_event_new_queue(newStream->m_xine_stream);
	if( newStream->m_event_queue == NULL ) {
		delete newStream;
		wxLogError("Xine Error: xine_event_new_queue() failed.");
		return NULL;
	}
	xine_event_create_listener_thread(newStream->m_event_queue, xine_event_listener_cb, (void*)newStream);

	// open URL for this stream
	if( !xine_open(newStream->m_xine_stream, static_cast<const char*>(url.mb_str(wxConvUTF8))) )
	{
		// failed, as xine is compatible to file:-URLs, this URL may be http, ZIP or sth. like that.
		// try over by copying the URL to a local file
		bool failed = true;
		wxString tryOver = SjTempNCache::GetAsLocalFile(url);
		if( !tryOver.IsEmpty() ) {
			if( xine_open(newStream->m_xine_stream, static_cast<const char*>(tryOver.mb_str(wxConvUTF8))) ) {
				failed = false;
			}
		}

		if( failed ) {
			delete newStream;
			wxLogError("Xine Error: xine_open() failed.");
			return NULL;
		}
	}

	// finally, play
	if( !xine_play(newStream->m_xine_stream, 0, seekMs) ) {
		delete newStream;
		wxLogError("Xine Error: xine_play() failed.");
		return NULL;
	}

	// success
	m_currStream = newStream;
	return newStream;
}


SjBackendState SjXineBackend::GetDeviceState() const
{
	if( !s_xine || !m_ao_port ) {
		return SJBE_STATE_CLOSED;
	}

	if( !m_currStream || !m_currStream->m_xine_stream ) {
		return SJBE_STATE_PLAYING; // the stream should appear every moment
	}

	if( xine_get_param(m_currStream->m_xine_stream,XINE_PARAM_SPEED) == XINE_SPEED_PAUSE ) {
		return SJBE_STATE_PAUSED;
	}

	return SJBE_STATE_PLAYING;
}


void SjXineBackend::SetDeviceState(SjBackendState state)
{
	if( !s_xine || !m_currStream || !m_currStream->m_xine_stream )  { return; }

	switch( state )
	{
		case SJBE_STATE_CLOSED:
			xine_close_audio_driver(s_xine, m_ao_port);
			m_ao_port = NULL;
			break;

		case SJBE_STATE_PLAYING:
			xine_set_param(m_currStream->m_xine_stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
			break;

		case SJBE_STATE_PAUSED:
			xine_set_param(m_currStream->m_xine_stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE);
			break;
	}
}


void SjXineBackend::SetDeviceVol(double gain)
{
	if( !s_xine || !m_currStream || !m_currStream->m_xine_stream ) { return; }

	int v;
	v = (int)((double)100*gain);
	if( v < 0 ) v = 0;
	if( v > 100 ) v = 0;
	xine_set_param(m_currStream->m_xine_stream, XINE_PARAM_AUDIO_VOLUME, v); // XINE_PARAM_AUDIO_VOLUME sets internally AO_PROP_MIXER_VOL,

}


void SjXineBackendStream::GetTime(long& totalMs, long& elapsedMs)
{
	totalMs   = -1;
	elapsedMs = -1;
	if( !m_xine_stream ) { return; }

	int pos_stream = -1, pos_time_ms = -1, length_time_ms = -1;
	if( xine_get_pos_length(m_xine_stream, &pos_stream, &pos_time_ms, &length_time_ms) )
	{
		// for HTTP-streams, xine sometimes returns bad values as 1083696000 or 285440000 for the length,
		// so, do not allow length > 1 day
		if( length_time_ms >= 0 && length_time_ms < (24*60*60*1000) )
		{
			totalMs = length_time_ms;
		}

		if( pos_time_ms >= 0 ) {
			elapsedMs = pos_time_ms;
		}
	}
}


void SjXineBackendStream::SeekAbs(long seekMs)
{
	if( !m_xine_stream ) { return; }

	xine_play(m_xine_stream, 0, seekMs);
}


SjXineBackendStream::~SjXineBackendStream()
{
	if( m_backend )
	{
		if( m_backend->m_currStream == this )
		{
			m_backend->m_currStream = NULL;
		}
	}

	if( m_event_queue )
	{
		xine_event_dispose_queue(m_event_queue);
		m_event_queue = NULL;
	}

	if( m_xine_stream )
	{
		xine_dispose(m_xine_stream);
		m_xine_stream = NULL;
	}

	delete this;
}


#endif // SJ_USE_XINE

