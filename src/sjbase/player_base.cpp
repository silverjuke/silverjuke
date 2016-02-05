/*******************************************************************************
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
 *******************************************************************************
 *
 * File:    player_base.cpp
 * Authors: Björn Petersen
 * Purpose: Player basic handling for the player (user settings etc.)
 *
 *******************************************************************************
 *
 * For a better overview, this file contains everything that is not directly
 * needed for playback but, however, belongs to SjPlayer.
 *
 * SjPlayer does a lot of parameter-range checking as it is also used
 * by some external modules and we don't know how they were programmed ;-)
 *
 * We DO NOT set any stats in SjMainFrame, this should be done by the caller.
 * We DO promote IDMODMSG_* messages to the module.
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/queue.h>
#include <sjbase/player.h>
#include <sjbase/columnmixer.h>
#include <sjmodules/vis/vis_module.h>
#include <see_dom/sj_see.h>

#if SJ_USE_GSTREAMER
	#include <sjbase/backend_gstreamer.h>
#elif SJ_USE_XINE
	#include <sjbase/backend_xine.h>
#elif SJ_USE_BASS
	#include <sjbase/backend_bass.h>
#endif


/*******************************************************************************
 * SjPlayerModule
 ******************************************************************************/


// internal messages
#define THREAD_PREPARE_NEXT         (IDPLAYER_FIRST+0)
#define THREAD_OUT_OF_DATA          (IDPLAYER_FIRST+1)
#define THREAD_DELETE_STREAM        (IDPLAYER_FIRST+2)
#define THREAD_RECALL_TRASH         (IDPLAYER_FIRST+5)
#define THREAD_NEW_TRACK_ON_AIR     (IDPLAYER_FIRST+6)
#define THREAD_NEW_META_DATA        (IDPLAYER_FIRST+7)


SjPlayerModule::SjPlayerModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	m_file              = wxT("memory:player.lib");
	m_name              = wxT("Player");
}


void SjPlayerModule::GetLittleOptions (SjArrayLittleOption& lo)
{
	if( g_mainFrame->m_player.m_backend )
	{
		SjLittleOption::SetSection(_("Playback"));
		g_mainFrame->m_player.m_backend->GetLittleOptions(lo);
	}
}


/*******************************************************************************
 * SjPlayer - Construct, Init etc.
 ******************************************************************************/


SjPlayer::SjPlayer()
{
	m_isInitialized         = FALSE;

	m_paused                = FALSE;

	m_stopAfterThisTrack    = FALSE;
	m_stopAfterEachTrack    = false;
	m_inOpening             = FALSE;

	m_mainVol               = SJ_DEF_VOLUME;
	m_mainGain              = 1.0F;
	m_mainBackupVol         = -1;

	m_avEnabled             = SJ_AV_DEF_STATE;
	m_avDesiredVolume       = SJ_AV_DEF_DESIRED_VOLUME;
	m_avMaxGain             = SJ_AV_DEF_MAX_GAIN;
	m_avUseAlbumVol         = SJ_AV_DEF_USE_ALBUM_VOL;
	m_avCalculatedGain      = 1.0F;

	m_autoCrossfade         = SJ_DEF_AUTO_CROSSFADE_ENABLED;
	m_autoCrossfadeSubseqDetect = TRUE;
	m_autoCrossfadeMs       = SJ_DEF_CROSSFADE_MS;
	m_manCrossfadeMs        = SJ_DEF_CROSSFADE_MS;
	m_skipSilence           = TRUE; // no discussion, always recommended
	m_onlyFadeOut           = false; // radio-like: only fade out, the new track starts with the full volume
	m_ffPause2PlayMs        = SJ_FF_DEF_PAUSE2PLAY_MS;
	m_ffPlay2PauseMs        = SJ_FF_DEF_PLAY2PAUSE_MS;
	m_ffGotoMs              = SJ_FF_DEF_GOTO_MS;
	m_backend               = NULL;
	m_streamA               = NULL;
	m_visBufferBytes        = 1024;
	m_visBuffer             = malloc(m_visBufferBytes);
}


void SjPlayer::Init()
{
	if( !m_isInitialized )
	{
		m_isInitialized = true;
		m_queue.Init();

		#define MAX_LANES 3 // two for crossfading + 1 for prelistening if there is no explicit device
		#if SJ_USE_GSTREAMER
			m_backend = new SjGstreamerBackend(SJBE_DEVICE_DEFAULT, MAX_LANES);
		#elif SJ_USE_XINE
			m_backend = new SjXineBackend(SJBE_DEVICE_DEFAULT, MAX_LANES);
		#elif SJ_USE_BASS
			m_backend = new SjBassBackend(SJBE_DEVICE_DEFAULT, MAX_LANES);
		#endif

		// LoadSettings() should be called by the caller, if needed
	}
}


void SjPlayer::Exit()
{
	if( m_isInitialized )
	{
		// SaveSettings() should be called by the caller, if needed
		m_isInitialized = false;

		if( m_streamA)
		{
			m_streamA->DestroyStream();
			m_streamA = NULL;
		}

		if( m_backend )
		{
			m_backend->DestroyBackend();
			m_backend = NULL;
		}

		m_queue.Exit();
	}
}


SjPlayer::~SjPlayer()
{
	free(m_visBuffer);
}


/*******************************************************************************
 * SjPlayer - Misc.
 ******************************************************************************/


const SjExtList* SjPlayer::GetExtList()
{
	static SjExtList s_exstList;
	bool s_extListInitialized = false;
	if( !s_extListInitialized )
	{
		// TODO: return really supported extensions here - or simply define a global "default" list and allow the user to modify it
		// (instead of let the user define extensions to ignore)
		s_exstList.AddExt("16sv, 4xm, 669, 8svx, aac, ac3, aif, aiff, amf, anim, anim3, anim5, anim7, anim8, anx, asc, "
			"asf, ass, asx, au, aud, avi, axa, axv, cak, cin, cpk, dat, dif, dps, dts, dv, f4a, f4v, film, flac, flc, fli, flv, "
			"ik2, iki, ilbm, it, m2t, m2ts, m4a, m4b, mdl, med, mjpg, mkv, mng, mod, mov, mp+, mp2, mp3, mp4, mpa, mpc, mpeg, mpega, "
			"mpg, mpp, mpv, mts, mv8, mve, nsv, oga, ogg, ogm, ogv, ogx, pes, pva, qt, qtl, ra, rm, rmvb, roq, s3m, shn, smi, snd, "
			"spx, srt, ssa, stm, str, sub, svx, trp, ts, tta, vmd, vob, voc, vox, vqa, wav, wax, wbm, webm, wma, wmv, wv, wve, wvp, "
			"wvx, xa, xa1, xa2, xap, xas, xm, y4m"); // (list is from xine)
	}
	return &s_exstList;
}


bool SjPlayer::TestUrl(const wxString& url)
{
	if( !m_isInitialized ) {
		return false;
	}

	wxString ext = SjTools::GetExt(url);
	if( ext.IsEmpty() ) {
		return false;
	}

	const SjExtList* extList = GetExtList();
	if( extList && !extList->LookupExt(ext) ) {
		return false;
	}

	return true; // URL is fine
}


bool SjPlayer::IsAutoPlayOnAir()
{
	if( !m_isInitialized ) {
		return false;
	}

	wxString urlOnAir = GetUrlOnAir();
	if( !urlOnAir.IsEmpty() )
	{
		long urlOnAirPos = m_queue.GetClosestPosByUrl(urlOnAir);
		if( urlOnAirPos != wxNOT_FOUND )
		{
			if( m_queue.GetFlags(urlOnAirPos)&SJ_PLAYLISTENTRY_AUTOPLAY )
			{
				return true;
			}
		}
	}

	return false;
}


void SjPlayer::SaveGatheredInfo(const wxString& url, unsigned long startingTime, SjVolumeCalc* volumeCalc, long realDecodedBytes)
{
	if( g_mainFrame
	 && !SjMainApp::IsInShutdown()
	 && !url.IsEmpty() )
	{
		double newGain = -1.0L;
		if( volumeCalc && volumeCalc->IsGainWorthSaving() )
		{
			newGain = volumeCalc->GetGain();
		}

		g_mainFrame->m_libraryModule->PlaybackDone(
		    url,
		    startingTime,   // 0 is okay here
		    newGain,        // -1 for unknown
		    realDecodedBytes);

		#if SJ_USE_SCRIPTS
		SjSee::Player_onPlaybackDone(url);
		#endif
	}
}


void SjPlayer::SendSignalToMainThread(int id, uintptr_t extraLong) const
{
	if(  g_mainFrame
	 &&  m_isInitialized
	 && !SjMainApp::IsInShutdown() ) // do not send the message on shutdown - it won't be received and there will be no memory leak as the OS normally free all program memory
	{
		wxCommandEvent* evt = new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, id);
		evt->SetClientData((void*)extraLong);
		g_mainFrame->GetEventHandler()->QueueEvent(evt);
	}
}


/*******************************************************************************
 * Loading and Saving user settings
 ******************************************************************************/


void SjPlayer::LoadSettings()
{
	// LoadSettings() is only called by the client, if needed. SjPlayer does not call LoadSettings()
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	wxConfigBase* c = g_tools->m_config;

	// load base
	SetMainVol                  (c->Read(wxT("player/volume"),              SJ_DEF_VOLUME));
	m_queue.SetShuffle          (c->Read(wxT("player/shuffle"),             SJ_DEF_SHUFFLE_STATE? 1L : 0L)!=0);
	m_queue.SetShuffleIntensity (c->Read(wxT("player/shuffleIntensity"),    SJ_DEF_SHUFFLE_INTENSITY));
	m_queue.SetRepeat ((SjRepeat)c->Read(wxT("player/repeat"),              0L));
	m_queue.SetQueueFlags       (c->Read(wxT("player/avoidBoredom"),        SJ_QUEUEF_DEFAULT), c->Read(wxT("player/boredomMinutes"), SJ_DEF_BOREDOM_TRACK_MINUTES), c->Read(wxT("player/boredomArtistMinutes"), SJ_DEF_BOREDOM_ARTIST_MINUTES));
	SetAutoCrossfade            (c->Read(wxT("player/crossfadeActive"),     SJ_DEF_AUTO_CROSSFADE_ENABLED? 1L : 0L)!=0);
	SetAutoCrossfadeSubseqDetect(c->Read(wxT("player/crossfadeSubseqDetect"),1L/*always recommended*/)!=0);
	m_autoCrossfadeMs           =c->Read(wxT("player/crossfadeMs"),         SJ_DEF_CROSSFADE_MS);
	m_manCrossfadeMs            =c->Read(wxT("player/crossfadeManMs"),      SJ_DEF_CROSSFADE_MS);
	SetSkipSilence              (c->Read(wxT("player/crossfadeSkipSilence"),1L/*always recommended*/)!=0);
	SetOnlyFadeOut              (c->Read(wxT("player/onlyFadeOut"),         0L/*defaults to off*/)!=0);

	StopAfterEachTrack          (c->Read(wxT("player/stopAfterEachTrack"),  0L)!=0);

	m_ffPause2PlayMs            =c->Read(wxT("player/ffPause2Play"),        SJ_FF_DEF_PAUSE2PLAY_MS);
	m_ffPlay2PauseMs            =c->Read(wxT("player/ffPlay2Pause"),        SJ_FF_DEF_PLAY2PAUSE_MS);
	m_ffGotoMs                  =c->Read(wxT("player/ffGoto"),              SJ_FF_DEF_GOTO_MS);

	AvEnable                    (c->Read(wxT("player/autovol"),             SJ_AV_DEF_STATE? 1L : 0L)!=0);
	AvSetUseAlbumVol            (c->Read(wxT("player/usealbumvol"),         SJ_AV_DEF_USE_ALBUM_VOL? 1L : 0L)!=0);
	m_avDesiredVolume           = (float)c->Read(wxT("player/autovoldes"),  (long)(SJ_AV_DEF_DESIRED_VOLUME*1000.0F)) / 1000.0F;
	m_avMaxGain                 = (float)c->Read(wxT("player/autovolmax"),  (long)(SJ_AV_DEF_MAX_GAIN*1000.0F)) / 1000.0F;
}


void SjPlayer::SaveSettings() const
{
	// SaveSettings() is only called by the client, if needed. SjPlayer does not call SaveSettings()
	// This function may only be called from the main thread.

	wxASSERT( wxThread::IsMain() );

	wxConfigBase* c = g_tools->m_config;

	// save base - mute is not saved by design

	c->Write(wxT("player/volume"), m_mainBackupVol==-1? m_mainVol : m_mainBackupVol);

	// save misc.
	c->Write(wxT("player/shuffle"),             m_queue.GetShuffle()? 1L : 0L);
	c->Write(wxT("player/shuffleIntensity"),    m_queue.GetShuffleIntensity());

	{
		long queueFlags, boredomTrackMinutes, boredomArtistMinutes;
		m_queue.GetQueueFlags(queueFlags, boredomTrackMinutes, boredomArtistMinutes);
		c->Write(wxT("player/avoidBoredom"),        queueFlags); // the name "avoidBoredom" is historical ...
		c->Write(wxT("player/boredomMinutes"),      boredomTrackMinutes);
		c->Write(wxT("player/boredomArtistMinutes"),boredomArtistMinutes);
	}

	c->Write(wxT("player/crossfadeActive"),     GetAutoCrossfade()? 1L : 0L);
	c->Write(wxT("player/crossfadeMs"),         m_autoCrossfadeMs);
	c->Write(wxT("player/crossfadeManMs"),      m_manCrossfadeMs);
	c->Write(wxT("player/crossfadeSkipSilence"),GetSkipSilence()? 1L : 0L);
	c->Write(wxT("player/onlyFadeOut"),         GetOnlyFadeOut()? 1L : 0L);
	c->Write(wxT("player/stopAfterEachTrack"),  StopAfterEachTrack()? 1L : 0L);
	c->Write(wxT("player/crossfadeSubseqDetect"),GetAutoCrossfadeSubseqDetect()? 1L : 0L);

	c->Write(wxT("player/ffPause2Play"),        m_ffPause2PlayMs);
	c->Write(wxT("player/ffPlay2Pause"),        m_ffPlay2PauseMs);
	c->Write(wxT("player/ffGoto"),              m_ffGotoMs);

	c->Write(wxT("player/autovol"),             AvIsEnabled()? 1L : 0L);
	c->Write(wxT("player/usealbumvol"),         AvGetUseAlbumVol()? 1L : 0L);
	c->Write(wxT("player/autovoldes"),   (long)(m_avDesiredVolume*1000.0F));
	c->Write(wxT("player/autovolmax"),   (long)(m_avMaxGain*1000.0F));

	// save repeat - repeating a single track is not remembered by design

	if( m_queue.GetRepeat()!=1 )
	{
		c->Write(wxT("player/repeat"), (long)m_queue.GetRepeat());
	}
}


/*******************************************************************************
 * Volume and AutoVol
 ******************************************************************************/


long SjPlayer_BackendCallback(SjBackendCallbackParam* cbp)
{
	// TAKE CARE: this function ist called while processing the audio data,
	// just before the output.  So please, do not do weird things here.
	switch( cbp->msg )
	{
		case SJBE_MSG_END_OF_STREAM:
			{
				SjPlayer* player = (SjPlayer*)cbp->userdata;
				player->SendSignalToMainThread(THREAD_PREPARE_NEXT);
			}
			return 1;

		case SJBE_MSG_DSP:
			if( g_visModule->IsVisStarted() )
			{
				g_visModule->AddVisData(cbp->buffer, cbp->bytes);
			}
			return 1;

		default:
			return 0;
	}

	return 0;
}


void SjPlayer::GotoAbsPos(long queuePos, bool fadeToPos)
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	if( !m_isInitialized || !m_backend ) {
		return;
	}

	static bool inHere = false;
	if( !inHere )
	{
		inHere = true;

		m_stopAfterThisTrack = false;

		// check offset
		if( queuePos < 0
		 || queuePos >= m_queue.GetCount() )
		{
			inHere = false; // don't forget this!
			return;
		}

		// goto absolute track
		m_queue.SetCurrPos(queuePos);

		if( IsPaused() )
		{
			// switch from pause to stop
			Stop();
		}
		else if( IsPlaying() )
		{
			// playing, realize the new position
            if( m_streamA )
            {
				SaveGatheredInfo(m_streamA->GetUrl(), m_streamA->GetStartingTime(), NULL, 0);
				m_streamA->DestroyStream();
				m_streamA = NULL;
            }

			if( m_backend )
			{
				wxString url = m_queue.GetUrlByPos(queuePos);
				if( !url.IsEmpty() )
				{
					bool deviceOpendedBefore = m_backend->IsDeviceOpened();
					m_streamA = m_backend->CreateStream(0, url, 0, SjPlayer_BackendCallback, this); // may be NULL, we send the signal anyway!
					if( m_streamA )
					{
						if( !deviceOpendedBefore )
						{
							m_backend->SetDeviceVol(m_mainGain);
						}
					}
				}
			}
		}

		SendSignalToMainThread(IDMODMSG_TRACK_ON_AIR_CHANGED);

		inHere = false;
	}
}


void SjPlayer::SetMainVol(int newVol) // 0..255
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	if( newVol < 0 )   { newVol = 0;   }
	if( newVol > 255 ) { newVol = 255; }

	m_mainVol   = newVol;
	m_mainGain  = ((double)newVol)/255.0F;

	if( m_backend && m_backend->IsDeviceOpened() )
	{
		m_backend->SetDeviceVol((float)m_mainGain);
	}
}


void SjPlayer::SetMainVolMute(bool mute)
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	if( mute )
	{
		if( m_mainBackupVol == -1 ) { m_mainBackupVol = m_mainVol; }
		SetMainVol(0);
	}
	else
	{
		SetMainVol(m_mainBackupVol > 8? m_mainBackupVol : SJ_DEF_VOLUME);
		m_mainBackupVol = -1;
	}
}


void SjPlayer::AvSetUseAlbumVol(bool useAlbumVol)
{
	if( m_avUseAlbumVol != useAlbumVol )
	{
		m_avUseAlbumVol = useAlbumVol;
	}
}


/*******************************************************************************
 * Common Player Functionality
 ******************************************************************************/


long SjPlayer::GetEnqueueTime()
{
	// if the user enqueues a track NOW,
	// this function calculates the soonest time it can get played.
	// (if shuffle is enabled, this may be directly after the current track, otherwise this is the end of the playlist)

	#define EST_AVG_TRACK_LENGTH_MS 180000L  // assume three minutes avg. length of a track

	wxASSERT( wxThread::IsMain() );
	long totalRemainingMs = 0, entryRemainingMs, i;
	long queueCount = m_queue.GetCount();

	if( queueCount > 0 )
	{
		totalRemainingMs = GetRemainingTime();
		if( !m_queue.GetShuffle() )
		{
			for( i = m_queue.GetCurrPos()+1; i < queueCount; i++ )
			{
				entryRemainingMs = m_queue.GetInfo(i).GetPlaytimeMs();
				totalRemainingMs += entryRemainingMs>0? entryRemainingMs : EST_AVG_TRACK_LENGTH_MS;
			}
		}
	}

	return totalRemainingMs;
}


void SjPlayer::Stop(bool stopVisIfPlaying)
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	if( !m_isInitialized ) {
		return;
	}

	m_stopAfterThisTrack = false;
	m_failedUrls.Clear();

	// stop visualisation
	if( stopVisIfPlaying )
	{
		g_visModule->StopVisIfOverWorkspace();
	}

	// Do the "real stop" in the implementation part
	if( m_streamA )
	{
		SaveGatheredInfo(m_streamA->GetUrl(), m_streamA->GetStartingTime(), NULL, 0);
		m_streamA->DestroyStream();
		m_streamA = NULL;
	}

	if( m_backend )
	{
		m_backend->SetDeviceState(SJBE_STATE_CLOSED);
	}

	m_paused = false;
}


void SjPlayer::ReceiveSignal(int signal, uintptr_t extraLong)
{
	if( !m_isInitialized || m_backend == NULL ) {
		return;
	}

	if( signal == THREAD_PREPARE_NEXT || signal == THREAD_OUT_OF_DATA )
	{
		// just stop after this track?
		if( m_stopAfterThisTrack || m_stopAfterEachTrack )
		{
			if( signal == THREAD_OUT_OF_DATA )
			{
				Stop(); // Stop() clears the m_stopAfterThisTrack flag

				if( HasNextIgnoreAP() ) // make sure, the next hit on play goes to the next track, see
				{	// http://www.silverjuke.net/forum/topic-1769.html
					GotoNextIgnoreAP(false);
					g_mainFrame->UpdateDisplay();
				}

			}
			wxLogDebug(" SjPlayer::ReceiveSignal(): \"stop after this/each track\" executed.");
			return;
		}
	}

	// more signal handling (old implementation part)
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
					wxLogDebug(" ... receiving THREAD_OUT_OF_DATA, stopping and sending IDMODMSG_PLAYER_STOPPED_BY_EOQ");
					Stop();
					SendSignalToMainThread(IDMODMSG_PLAYER_STOPPED_BY_EOQ);
				}
				return;
			}
		}
		newUrl = m_queue.GetUrlByPos(newQueuePos);

		// has the URL just failed? try again in the next message loop
		wxLogDebug(" ... new URL is \"%s\"", newUrl.c_str());

		if( m_failedUrls.Index( newUrl ) != wxNOT_FOUND )
		{
			wxLogDebug(" ... the URL has failed before, starting over.");
			m_queue.SetCurrPos(newQueuePos);
			SendSignalToMainThread(signal); // start over
			return;
		}

		// try to create the next stream
		if( m_streamA )
		{
			SaveGatheredInfo(m_streamA->GetUrl(), m_streamA->GetStartingTime(), NULL, 0);
			m_streamA->DestroyStream();
			m_streamA = NULL;
		}

		m_streamA = m_backend->CreateStream(0, newUrl, 0, SjPlayer_BackendCallback, this); // may be NULL, we send the signal anyway!

		// realize the new position in the UI
		m_queue.SetCurrPos(newQueuePos);
		SendSignalToMainThread(IDMODMSG_TRACK_ON_AIR_CHANGED);
	}
}


void SjPlayer::SeekAbs(long seekMs)
{
	if( !m_isInitialized ) {
		return;
	}

	if( m_streamA ) {
		m_streamA->SeekAbs(seekMs);
	}
}


void SjPlayer::Play(long seekMs, bool fadeToPlay)
{
	if( !m_isInitialized || !m_backend ) {
		return;
	}

	// play!
	if( !m_streamA )
	{
		// set source URI
		wxString url = m_queue.GetUrlByPos(-1);
		if( url.IsEmpty() ) {
			return; // error;
		}

		m_streamA = m_backend->CreateStream(0, url, seekMs, SjPlayer_BackendCallback, this);
		if( !m_streamA ) {
			return; // error;
		}
	}
	else
	{
		m_backend->SetDeviceState(SJBE_STATE_PLAYING);
	}

	// success
	m_paused = false;
	return;
}


void SjPlayer::Pause(bool fadeToPause)
{
	if( !m_isInitialized || !m_backend ) {
		return;
	}

	// if the player is stopped or if we are already paused, there is nothing to do
	if( !m_backend->IsDeviceOpened() || m_paused ) {
		return;
	}

	// real changing of the pause state in the implementation, this should also set the m_paused flag
	m_backend->SetDeviceState(SJBE_STATE_PAUSED);

	m_paused = true;
}


void SjPlayer::GetTime(long& totalMs, long& elapsedMs, long& remainingMs)
{
	wxASSERT( wxThread::IsMain() );

	if( !m_isInitialized || !m_backend || !m_streamA ) {
		totalMs = 0;
		elapsedMs = 0;
		remainingMs = 0;
		return; // Init() not called, may happen on startup display updates
	}

	// calculate totalMs and elapsedMs, if unknown, -1 is returned.
	m_streamA->GetTime(totalMs, elapsedMs);

	// remaining time
	remainingMs = -1;
	if( totalMs != -1 && elapsedMs != -1 )
	{
		remainingMs = totalMs - elapsedMs;
		if( remainingMs < 0 )
		{
			remainingMs = 0;
		}
	}
}



wxString SjPlayer::GetUrlOnAir()
{
	if( !m_isInitialized || !m_backend || !m_backend->IsDeviceOpened() || !m_streamA ) {
		return wxEmptyString;
	}

	return m_streamA->GetUrl();
}



/*******************************************************************************
 * Resume
 ******************************************************************************/


wxString SjPlayer::GetResumeFile() const
{
	wxSqltDb* db = wxSqltDb::GetDefault();
	wxASSERT( db ); if( db == NULL ) return "";

	wxFileName fn(db->GetFile());
	fn.SetFullName("." + fn.GetFullName() + "-resume");
	return fn.GetFullPath(); // results in ".filename.jukebox-resume", a hidden file
}


void SjPlayer::SaveToResumeFile()
{
	// this function is called on shutdown, where our program may be _killed_ by the operating system.
	// so we do not risk to write larger data to the sqlite database but use a simple text file instead.

	// create header with common information
	unsigned long startMs = SjTools::GetMsTicks();
	wxString content("resumeversion=2\n");
	int i, iCount = m_queue.GetCount(), iPos = m_queue.GetCurrPos();

	// collect all URLs
	bool addPlayed = (m_queue.GetQueueFlags()&SJ_QUEUEF_RESUME_LOAD_PLAYED)!=0;
	long playcount, entryflags;
	for( i = 0; i < iCount; i++ )
	{
		SjPlaylistEntry& e = m_queue.GetInfo(i);
		playcount = e.GetPlayCount();
		entryflags = e.GetFlags();
		if( playcount==0 || addPlayed || i==iPos )
		{
			if( playcount > 0 ) {
				content += "played=1\n"; // setting for the URL following
			}

			if( entryflags & SJ_PLAYLISTENTRY_AUTOPLAY ) {
				content += "autoplay=1\n"; // setting for the URL following
			}

			if( i==iPos ) {
				long totalMs, elapsedMs = -1/*stop or pause*/, remainingMs;
				if( IsPlaying() ) {
					GetTime(totalMs, elapsedMs/*may be -1 for 'unknown'*/, remainingMs);
					if( elapsedMs < 0 ) {
						elapsedMs = 0; // >=0: playing
					}
				}
				content += wxString::Format("playing=%i\n", (int)elapsedMs); // setting for the URL following, save independingly of SJ_QUEUEF_RESUME_START_PLAYBACK as we always init the queue positions
			}

			content += "url=" + e.GetUnverifiedUrl() + "\n"; // the unverified URL may contain Tabs! When we reload the information. If we load the file later, we treat all URLs as unverfied (eg. the files may have changed)
		}
	}

	// open file to write
	{
		wxLogNull null;
		wxFile file(GetResumeFile(), wxFile::write);
		if( !file.IsOpened() )
		{
			return; // do not log any error, we're in shutdown
		}

		// add some footer information and write
		wxDateTime dt = wxDateTime::Now().ToUTC();
		content += dt.Format("created=%Y-%m-%dT%H:%M:%S+00:00\n");

		content += wxString::Format("ms=%i\n", (int)(SjTools::GetMsTicks()-startMs)); // speed is about 500 tracks per millisecond on my system from the year 2012
		file.Write(content, wxConvUTF8);
	}
}


void SjPlayer::LoadFromResumeFile()
{
	// load content from file
    // we do not delete the file physically after loading.
    // If the next shutdown fails it is better to load "too many" tracks with a repeated playing position than nothing.	wxString content;
	wxString content;
	{
		wxString resumeFile = GetResumeFile();
		wxFileSystem fileSystem;
		wxFSFile* fsFile = fileSystem.OpenFile(resumeFile, wxFS_READ);
		if( fsFile == NULL )
		{
			return; // do not log any error, there is simply no resume file
		}

		wxLogInfo("Loading %s", resumeFile.c_str());
		content = SjTools::GetFileContent(fsFile->GetStream(), &wxConvUTF8);
		delete fsFile;
	}

	// go through all lines of the content
	wxString            istVersion;
	SjLineTokenizer     tkz(content);
	wxChar*             currLinePtr;
	wxString            currLine, currKey, currValue;
	long                currPlayed = 0, currAutoplay = 0;
	wxArrayString       allUrls;
	wxArrayLong         allPlayed;
	wxArrayLong         allAutoplay;
	long                allPos = -1, allElapsed = -1;
	while( (currLinePtr=tkz.GetNextLine()) )
	{
		currLine  = currLinePtr;
		currKey   = currLine.BeforeFirst('=');
		currValue = currLine.AfterFirst('=');

		if( currKey == "played" )
		{
			currPlayed = 1;
		}
		else if( currKey == "autoplay" )
		{
			currAutoplay = 1;
		}
		else if( currKey == "playing" )
		{
			allPos = allUrls.Count(); // the next added URL is our queue position
			if( !currValue.ToLong(&allElapsed) ) {
				allElapsed = -1; // stopped or paused, to not play
			}
		}
		else if( currKey == "url" )
		{
			allUrls.Add(currValue);
			allPlayed.Add(currPlayed);
			allAutoplay.Add(currAutoplay);

			// prepare for next URL
			currPlayed = 0;
			currAutoplay = 0;
		}
	}

	// enqueue the urls
	g_mainFrame->Enqueue(allUrls, -1, false, NULL, 0);

	// start playback
	if( allPos >= 0 && allPos < m_queue.GetCount() )
	{
		m_queue.SetCurrPos(allPos);
		if( m_queue.GetQueueFlags()&SJ_QUEUEF_RESUME_START_PLAYBACK && allElapsed >= 0 )
		{
			g_mainFrame->Play(allElapsed);
		}
	}

	// mark URLs as autoplay/played
	wxASSERT( allUrls.Count() == allPlayed.Count() );
	wxASSERT( allUrls.Count() == allAutoplay.Count() );
	int i, iCount = m_queue.GetCount(); if( iCount > (int)allUrls.Count() ) { iCount = (int)allUrls.Count(); } // get min
	for( i = 0; i < iCount; i++ )
	{
		SjPlaylistEntry& e = m_queue.GetInfo(i);
		if( allPlayed[i] )
		{
			e.SetPlayCount(1);
		}
		else if( e.GetPlayCount() > 0 )
		{
			e.SetPlayCount(0); // the Play() call above may have marked previous track as being played. fix that.
		}

		if( allAutoplay[i] )
		{
			e.SetFlags(e.GetFlags()|SJ_PLAYLISTENTRY_AUTOPLAY);
		}
	}
}

