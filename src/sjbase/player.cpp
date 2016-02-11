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
 * File:    player.cpp
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
#include <sjtools/volumecalc.h>
#include <sjtools/volumefade.h>
#include <sjmodules/vis/vis_module.h>
#include <see_dom/sj_see.h>

#if SJ_USE_GSTREAMER
	#include <sjbase/backend_gstreamer.h>
	#define BACKEND_CLASSNAME SjGstreamerBackend
#elif SJ_USE_XINE
	#include <sjbase/backend_xine.h>
	#define BACKEND_CLASSNAME SjXineBackend
#elif SJ_USE_BASS
	#include <sjbase/backend_bass.h>
	#define BACKEND_CLASSNAME SjBassBackend
#endif


/*******************************************************************************
 * SjPlayerModule
 ******************************************************************************/


// internal messages
#define THREAD_END_OF_STREAM         (IDPLAYER_FIRST+0)


SjPlayerModule::SjPlayerModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	m_file              = "memory:player.lib";
	m_name              = "Player";
}


void SjPlayerModule::GetLittleOptions (SjArrayLittleOption& lo)
{
	// prelisten options (they belong to the device options; all other stuff has explicit options in the dialog on different pages)
	wxString options = wxString::Format( "%i|", SJ_PL_MIX) + _("Mix")
					+  wxString::Format("|%i|", SJ_PL_LEFT) + _("Left channel")
					+  wxString::Format("|%i|", SJ_PL_RIGHT) + _("Right channel")
					+  wxString::Format("|%i|", SJ_PL_OWNOUTPUT) + _("Explicit output");
	lo.Add(new SjLittleEnumLong(_("Prelisten"), options, &g_mainFrame->m_player.m_plDest, SJ_PL_DEFAULT, "player/prelistenDest", SJ_ICON_MODULE));

	// backend settings
	if( g_mainFrame->m_player.m_backend )
	{
		SjLittleOption::SetSection(_("Output"));
		g_mainFrame->m_player.m_backend->GetLittleOptions(lo);
	}

	if( g_mainFrame->m_player.m_plBackend )
	{
		SjLittleOption::SetSection(_("Prelisten"));
		g_mainFrame->m_player.m_plBackend->GetLittleOptions(lo);
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

	m_plBackend             = NULL;
	m_plDest                = SJ_PL_DEFAULT;
}


void SjPlayer::Init()
{
	wxASSERT( wxThread::IsMain() );
	if( m_isInitialized ) return;

	// init/create objects
	m_isInitialized = true;
	m_queue.Init();

	m_backend = new BACKEND_CLASSNAME(SJBE_ID_STDOUTPUT);
	m_plBackend = new BACKEND_CLASSNAME(SJBE_ID_PRELISTEN);

	// load settings
	wxConfigBase* c = g_tools->m_config;
	SetMainVol                  (c->Read("player/volume",              SJ_DEF_VOLUME));
	m_queue.SetShuffle          (c->Read("player/shuffle",             SJ_DEF_SHUFFLE_STATE? 1L : 0L)!=0);
	m_queue.SetShuffleIntensity (c->Read("player/shuffleIntensity",    SJ_DEF_SHUFFLE_INTENSITY));
	m_queue.SetRepeat ((SjRepeat)c->Read("player/repeat",              0L));
	m_queue.SetQueueFlags       (c->Read("player/avoidBoredom",        SJ_QUEUEF_DEFAULT), c->Read("player/boredomMinutes", SJ_DEF_BOREDOM_TRACK_MINUTES), c->Read("player/boredomArtistMinutes", SJ_DEF_BOREDOM_ARTIST_MINUTES));
	SetAutoCrossfade            (c->Read("player/crossfadeActive",     SJ_DEF_AUTO_CROSSFADE_ENABLED? 1L : 0L)!=0);
	SetAutoCrossfadeSubseqDetect(c->Read("player/crossfadeSubseqDetect",1L/*always recommended*/)!=0);
	m_autoCrossfadeMs           =c->Read("player/crossfadeMs",         SJ_DEF_CROSSFADE_MS);
	m_manCrossfadeMs            =c->Read("player/crossfadeManMs",      SJ_DEF_CROSSFADE_MS);
	SetSkipSilence              (c->Read("player/crossfadeSkipSilence",1L/*always recommended*/)!=0);
	SetOnlyFadeOut              (c->Read("player/onlyFadeOut",         0L/*defaults to off*/)!=0);

	StopAfterEachTrack          (c->Read("player/stopAfterEachTrack",  0L)!=0);

	m_ffPause2PlayMs            =c->Read("player/ffPause2Play",        SJ_FF_DEF_PAUSE2PLAY_MS);
	m_ffPlay2PauseMs            =c->Read("player/ffPlay2Pause",        SJ_FF_DEF_PLAY2PAUSE_MS);
	m_ffGotoMs                  =c->Read("player/ffGoto",              SJ_FF_DEF_GOTO_MS);

	AvEnable                    (c->Read("player/autovol",             SJ_AV_DEF_STATE? 1L : 0L)!=0);
	AvSetUseAlbumVol            (c->Read("player/usealbumvol",         SJ_AV_DEF_USE_ALBUM_VOL? 1L : 0L)!=0);
	m_avDesiredVolume           = (float)c->Read("player/autovoldes",  (long)(SJ_AV_DEF_DESIRED_VOLUME*1000.0F)) / 1000.0F;
	m_avMaxGain                 = (float)c->Read("player/autovolmax",  (long)(SJ_AV_DEF_MAX_GAIN*1000.0F)) / 1000.0F;

	m_plDest                    =c->Read("player/prelistenDest",       SJ_PL_DEFAULT);
}


void SjPlayer::SaveSettings() const
{
	// SaveSettings() is only called by the client, if needed. SjPlayer does not call SaveSettings()
	wxASSERT( wxThread::IsMain() );

	wxConfigBase* c = g_tools->m_config;

	// save base - mute is not saved by design

	c->Write("player/volume", m_mainBackupVol==-1? m_mainVol : m_mainBackupVol);

	// save misc.
	c->Write("player/shuffle",             m_queue.GetShuffle()? 1L : 0L);
	c->Write("player/shuffleIntensity",    m_queue.GetShuffleIntensity());

	{
		long queueFlags, boredomTrackMinutes, boredomArtistMinutes;
		m_queue.GetQueueFlags(queueFlags, boredomTrackMinutes, boredomArtistMinutes);
		c->Write("player/avoidBoredom",        queueFlags); // the name "avoidBoredom" is historical ...
		c->Write("player/boredomMinutes",      boredomTrackMinutes);
		c->Write("player/boredomArtistMinutes",boredomArtistMinutes);
	}

	c->Write("player/crossfadeActive",     GetAutoCrossfade()? 1L : 0L);
	c->Write("player/crossfadeMs",         m_autoCrossfadeMs);
	c->Write("player/crossfadeManMs",      m_manCrossfadeMs);
	c->Write("player/crossfadeSkipSilence",GetSkipSilence()? 1L : 0L);
	c->Write("player/onlyFadeOut",         GetOnlyFadeOut()? 1L : 0L);
	c->Write("player/stopAfterEachTrack",  StopAfterEachTrack()? 1L : 0L);
	c->Write("player/crossfadeSubseqDetect",GetAutoCrossfadeSubseqDetect()? 1L : 0L);

	c->Write("player/ffPause2Play",        m_ffPause2PlayMs);
	c->Write("player/ffPlay2Pause",        m_ffPlay2PauseMs);
	c->Write("player/ffGoto",              m_ffGotoMs);

	c->Write("player/autovol",             AvIsEnabled()? 1L : 0L);
	c->Write("player/usealbumvol",         AvGetUseAlbumVol()? 1L : 0L);
	c->Write("player/autovoldes",   (long)(m_avDesiredVolume*1000.0F));
	c->Write("player/autovolmax",   (long)(m_avMaxGain*1000.0F));

	c->Write("player/prelistenDest",       m_plDest);

	// save repeat - repeating a single track is not remembered by design

	if( m_queue.GetRepeat()!=1 )
	{
		c->Write("player/repeat", (long)m_queue.GetRepeat());
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
			delete m_streamA;
			m_streamA = NULL;
		}

		if( m_backend )
		{
			delete m_backend;
			m_backend = NULL;
		}

		if( m_plBackend )
		{
			delete m_plBackend;
			m_plBackend = NULL;
		}

		m_queue.Exit();
	}
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


void SjPlayer::SaveGatheredInfo(const wxString& url, unsigned long startingTime, SjVolumeCalc* volumeCalc, long realContinuousDecodedMs)
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
		    realContinuousDecodedMs);

		#if SJ_USE_SCRIPTS
		SjSee::Player_onPlaybackDone(url);
		#endif
	}
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


/*******************************************************************************
 * DSP- and StreamInfo-Callback
 ******************************************************************************/


class SjBackendUserdata
{
public:
	SjBackendUserdata(SjPlayer* player, long onCreateFadeMs = 0)
	{
		m_player         = player;
		m_onCreateFadeMs = onCreateFadeMs;
		m_isVideo        = false;
		m_realMs         = 0;
	}
	long          m_onCreateFadeMs;
	SjPlayer*     m_player;
	bool          m_isVideo;
	long          m_realMs;
	SjVolumeCalc  m_volumeCalc;
	SjVolumeFade  m_volumeFade;
};


long SjPlayer_BackendCallback(SjBackendCallbackParam* cbp)
{
	// TAKE CARE: this function is called while processing the audio data,
	// just before the output.  So please, do not do weird things here.

	SjBackendStream*   stream     = cbp->stream;        if( stream == NULL )   { return 0; }
	SjBackendUserdata* userdata   = stream->m_userdata; if( userdata == NULL ) { return 0; }
	SjPlayer*          player     = userdata->m_player; if( player == NULL )   { return 0; }
	float*             buffer     = cbp->buffer; // may be NULL
	long               bytes      = cbp->bytes;
	int                samplerate = cbp->samplerate;
	int                channels   = cbp->channels;

	if( cbp->msg == SJBE_MSG_DSP )
	{
		/* DSP
		***********************************************************************/

		if( buffer == NULL || bytes <= 0 )
		{
			return 1; // bad data, message handled
		}

		// calculate the volume - we do this ALWAYS, if autovol is enabled or not
		userdata->m_volumeCalc.AddBuffer(buffer, bytes, samplerate, channels);

		// apply the calulated gain, if desired
		if( player->m_avEnabled )
		{
			userdata->m_volumeCalc.AdjustBuffer(buffer, bytes, player->m_avDesiredVolume, player->m_avMaxGain);

			if( stream == player->m_streamA ) {
				player->m_avCalculatedGain = userdata->m_volumeCalc.GetGain();
			}
		}

		// apply optional fadings, eg. for crossfading
		userdata->m_volumeFade.AdjustBuffer(buffer, bytes, samplerate, channels);

		// forward the data to the visualisation -
		// we do this last, so autovol, equalizers etc. is visible eg. in the spectrum analyzer
		if( g_visModule->IsVisStarted() )
		{
			g_visModule->AddVisData(buffer, bytes);
		}

		return 1; // done, message handled
	}
	else if( cbp->msg == SJBE_MSG_CREATE )
	{
		/* Stream just created - send before any SJBE_MSG_DSP
		***********************************************************************/

		wxASSERT( wxThread::IsMain() );

		userdata->m_volumeCalc.SetPrecalculatedGain(
			g_mainFrame->m_libraryModule->GetAutoVol(stream->GetUrl(), player->AvGetUseAlbumVol())
		);

		if( userdata->m_onCreateFadeMs ) {
			userdata->m_volumeFade.SetVolume(0.0);
			userdata->m_volumeFade.SlideVolume(1.0, userdata->m_onCreateFadeMs);
		}

		return 1; // done, message handled
	}
	else if( cbp->msg == SJBE_MSG_VIDEO_DETECTED )
	{
		/* Video detected
		***********************************************************************/

		if( !userdata->m_isVideo )
		{
			userdata->m_isVideo = true;
			player->SendSignalToMainThread(IDMODMSG_VIDEO_DETECTED);
		}

		return 1; // done, message handled
	}
	else if( cbp->msg == SJBE_MSG_END_OF_STREAM )
	{
		/* End of stream
		***********************************************************************/

		player->SendSignalToMainThread(THREAD_END_OF_STREAM);

		return 1; // done, message handled
	}
	else if( cbp->msg == SJBE_MSG_DESTROY_USERDATA )
	{
		/* destroy
		***********************************************************************/

		wxASSERT( wxThread::IsMain() );

		delete userdata;
		stream->m_userdata = NULL;

		return 1; // done, message handled
	}

	return 0; // message not handled
}


/*******************************************************************************
 * Volume and AutoVol
 ******************************************************************************/


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


SjBackendStream* SjPlayer::CreateStream(const wxString& url, long seekMs, long fadeMs)
{
	SjBackendUserdata* userdata = new SjBackendUserdata(this, fadeMs);
	if( userdata == NULL ) {
		return NULL;
	}

	SjBackendStream* stream = m_backend->CreateStream(url, seekMs, SjPlayer_BackendCallback, userdata);
	if( stream == NULL ) {
		return NULL; // userdata should be deleted via SJBE_MSG_DESTROY_USERDATA
	}

	return stream;
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

		long fadeMs = 0;
		     if( seekMs )     { fadeMs = m_ffGotoMs; }
		else if( fadeToPlay ) { fadeMs = m_manCrossfadeMs; }

		m_streamA = CreateStream(url, seekMs, fadeMs);
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
		SaveGatheredInfo(m_streamA->GetUrl(), m_streamA->GetStartingTime(), &m_streamA->m_userdata->m_volumeCalc, m_streamA->m_userdata->m_realMs);
		delete m_streamA;
		m_streamA = NULL;
	}

	if( m_backend )
	{
		m_backend->SetDeviceState(SJBE_STATE_CLOSED);
	}

	m_paused = false;
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
				SaveGatheredInfo(m_streamA->GetUrl(), m_streamA->GetStartingTime(), &m_streamA->m_userdata->m_volumeCalc, m_streamA->m_userdata->m_realMs);
				delete m_streamA;
				m_streamA = NULL;
            }

			if( m_backend )
			{
				wxString url = m_queue.GetUrlByPos(queuePos);
				if( !url.IsEmpty() )
				{
					bool deviceOpendedBefore = m_backend->IsDeviceOpened();
					m_streamA = CreateStream(url, 0, 0); // may be NULL, we send the signal anyway!
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


void SjPlayer::SeekAbs(long seekMs)
{
	if( !m_isInitialized ) {
		return;
	}

	if( m_streamA ) {
		m_streamA->SeekAbs(seekMs);
	}
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
	if( totalMs > 0 ) {
		m_streamA->m_userdata->m_realMs = totalMs;
	}

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


wxString SjPlayer::GetUrlOnAir()
{
	if( !m_isInitialized || !m_backend || !m_backend->IsDeviceOpened() || !m_streamA ) {
		return wxEmptyString;
	}

	return m_streamA->GetUrl();
}


bool SjPlayer::IsVideoOnAir()
{
	if( m_streamA && m_streamA->m_userdata ) {
		return m_streamA->m_userdata->m_isVideo;
	}
	return false;
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


void SjPlayer::ReceiveSignal(int signal, uintptr_t extraLong)
{
	if( !m_isInitialized || m_backend == NULL ) { return; }

	if( signal == THREAD_END_OF_STREAM )
	{
		// just stop after this track?
		if( m_stopAfterThisTrack || m_stopAfterEachTrack )
		{
			g_mainFrame->Stop(); // Stop() clears the m_stopAfterThisTrack flag, and, as we go over g_mainFrame, this also sets haltedManually and stops autoPlay
			if( HasNextIgnoreAP() ) // make sure, the next hit on play goes to the next track
			{
				GotoNextIgnoreAP(false);
				g_mainFrame->UpdateDisplay();
			}
			return; // done
		}

		// find out the next url to play
		wxString	newUrl;
		long newQueuePos = m_queue.GetNextPos(SJ_PREVNEXT_REGARD_REPEAT);
		if( newQueuePos == -1 )
		{
			// try to enqueue auto-play url
			g_mainFrame->m_autoCtrl.DoAutoPlayIfEnabled(false /*ignoreTimeouts*/);
			newQueuePos = m_queue.GetNextPos(SJ_PREVNEXT_REGARD_REPEAT);

			if( newQueuePos == -1 )
			{
				// no chance, there is nothing more to play ...
				wxLogDebug(" ... receiving THREAD_END_OF_STREAM, stopping and sending IDMODMSG_PLAYER_STOPPED_BY_EOQ");
				Stop();
				SendSignalToMainThread(IDMODMSG_PLAYER_STOPPED_BY_EOQ);
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
			SaveGatheredInfo(m_streamA->GetUrl(), m_streamA->GetStartingTime(), &m_streamA->m_userdata->m_volumeCalc, m_streamA->m_userdata->m_realMs);
			delete m_streamA;
			m_streamA = NULL;
		}

		m_streamA = CreateStream(newUrl, 0, 0); // may be NULL, we send the signal anyway!

		// realize the new position in the UI
		m_queue.SetCurrPos(newQueuePos);
		SendSignalToMainThread(IDMODMSG_TRACK_ON_AIR_CHANGED);
	}
}

