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
#include <sjmodules/fx/equalizer.h>
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
#define THREAD_END_OF_STREAM_A       (IDPLAYER_FIRST+0)
#define THREAD_AUTO_DELETE           (IDPLAYER_FIRST+1)
#define THREAD_PRELISTEN_END         (IDPLAYER_FIRST+2)


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
	lo.Add(new SjLittleEnumLong(_("Prelisten"), options, &g_mainFrame->m_player.m_prelistenDest, SJ_PL_DEFAULT, "player/prelistenDest", SJ_ICON_MODULE));

	// backend settings
	if( g_mainFrame->m_player.m_backend )
	{
		SjLittleOption::SetSection(_("Output"));
		wxString sysVolOptions = wxString::Format("%s|%s|%s", _("No") /*=0*/, _("Yes") /*=1=Use+Init*/, _("Initialize") /*=2*/);
		lo.Add(new SjLittleBit(	_("Use system volume"), sysVolOptions, &g_mainFrame->m_player.m_useSysVol, SJ_SYSVOL_DEFAULT, 0, "player/useSysVol", SJ_ICON_MODULE));
		g_mainFrame->m_player.m_backend->GetLittleOptions(lo);
	}

	if( g_mainFrame->m_player.m_prelistenBackend )
	{
		SjLittleOption::SetSection(_("Prelisten"));
		wxString sysVolOptions = wxString::Format("%s|%s", _("No") /*=0*/, _("Initialize") /*=1*/); // for prelistening, we rely on m_prelistenGain - this is much easier for the different destinations; and _if_ we really use an explicit output, its volume is normally not editable by the normal system controls, so an init+Gain should work well
		lo.Add(new SjLittleBit(	_("Use system volume"), sysVolOptions, &g_mainFrame->m_player.m_prelistenUseSysVol, SJ_SYSVOL_DEFAULT, 0, "player/prelistenUseSysVol", SJ_ICON_MODULE));
		g_mainFrame->m_player.m_prelistenBackend->GetLittleOptions(lo);
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

	m_eqEnabled             = SJ_EQ_DEF_ENABLED;

	m_autoCrossfade         = SJ_DEF_AUTO_CROSSFADE_ENABLED;
	m_autoCrossfadeSubseqDetect = false;
	m_autoCrossfadeMs       = SJ_DEF_CROSSFADE_MS;
	m_manCrossfadeMs        = SJ_DEF_CROSSFADE_MS;
	m_crossfadeOffsetEndMs  = SJ_DEF_CROSSFADE_OFFSET_END_MS;
	m_onlyFadeOut           = false; // radio-like: only fade out, the new track starts with the full volume

	m_backend               = NULL;
	m_streamA               = NULL;
	m_useSysVol             = SJ_SYSVOL_DEFAULT;

	m_prelistenBackend      = NULL;
	m_prelistenStream       = NULL;
	m_prelistenDest         = SJ_PL_DEFAULT;
	m_prelistenGain         = 1.0F;
	m_prelistenUseSysVol    = SJ_SYSVOL_DEFAULT;
}


void SjPlayer::Init()
{
	wxASSERT( wxThread::IsMain() );
	if( m_isInitialized ) return;

	// init/create objects
	m_isInitialized = true;
	m_queue.Init();

	m_backend = new BACKEND_CLASSNAME(SJBE_ID_STDOUTPUT);
	m_prelistenBackend = new BACKEND_CLASSNAME(SJBE_ID_PRELISTEN);

	// load settings
	wxConfigBase* c = g_tools->m_config;
	m_useSysVol                 =c->Read("player/useSysVol",           SJ_SYSVOL_DEFAULT); // should be done before SetMainVol()
	SetMainVol                  (c->Read("player/volume",              SJ_DEF_VOLUME));
	m_queue.SetShuffle          (c->Read("player/shuffle",             SJ_DEF_SHUFFLE_STATE? 1L : 0L)!=0);
	m_queue.SetShuffleIntensity (c->Read("player/shuffleIntensity",    SJ_DEF_SHUFFLE_INTENSITY));
	m_queue.SetRepeat ((SjRepeat)c->Read("player/repeat",              0L));
	m_queue.SetQueueFlags       (c->Read("player/avoidBoredom",        SJ_QUEUEF_DEFAULT), c->Read("player/boredomMinutes", SJ_DEF_BOREDOM_TRACK_MINUTES), c->Read("player/boredomArtistMinutes", SJ_DEF_BOREDOM_ARTIST_MINUTES));
	SetAutoCrossfade            (c->Read("player/crossfadeActive",     SJ_DEF_AUTO_CROSSFADE_ENABLED? 1L : 0L)!=0);
	SetAutoCrossfadeSubseqDetect(c->Read("player/crossfadeSubseq",     0L/*defaults to off*/)!=0);
	m_autoCrossfadeMs           =c->Read("player/crossfadeMs",         SJ_DEF_CROSSFADE_MS);
	m_manCrossfadeMs            =c->Read("player/crossfadeManMs",      SJ_DEF_CROSSFADE_MS);
	m_crossfadeOffsetEndMs      =c->Read("player/crossfadeOffsetEndMs",SJ_DEF_CROSSFADE_OFFSET_END_MS);
	SetOnlyFadeOut              (c->Read("player/onlyFadeOut",         0L/*defaults to off*/)!=0);

	StopAfterEachTrack          (c->Read("player/stopAfterEachTrack",  0L)!=0);

	AvEnable                    (c->Read("player/autovol",             SJ_AV_DEF_STATE? 1L : 0L)!=0);
	AvSetUseAlbumVol            (c->Read("player/usealbumvol",         SJ_AV_DEF_USE_ALBUM_VOL? 1L : 0L)!=0);
	m_avDesiredVolume           = (float)c->Read("player/autovoldes",  (long)(SJ_AV_DEF_DESIRED_VOLUME*1000.0F)) / 1000.0F;
	m_avMaxGain                 = (float)c->Read("player/autovolmax",  (long)(SJ_AV_DEF_MAX_GAIN*1000.0F)) / 1000.0F;

	m_eqEnabled                 = (c->Read("player/eqActive",          SJ_EQ_DEF_ENABLED? 1L : 0L))!=0;
	m_eqParam.FromString        (  c->Read("player/eqParam",           ""));

	m_prelistenDest             =c->Read("player/prelistenDest",       SJ_PL_DEFAULT);
	m_prelistenUseSysVol        =c->Read("player/prelistenUseSysVol",  SJ_SYSVOL_DEFAULT);
	m_prelistenMixQuiet         = (float)c->Read("player/prelistenMixQuiet", (long)(SJ_DEF_PL_MIX_QUIET*1000.0F)) / 1000.0F;
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
	c->Write("player/crossfadeOffsetEndMs",m_crossfadeOffsetEndMs);
	c->Write("player/crossfadeManMs",      m_manCrossfadeMs);
	c->Write("player/onlyFadeOut",         GetOnlyFadeOut()? 1L : 0L);
	c->Write("player/stopAfterEachTrack",  StopAfterEachTrack()? 1L : 0L);
	c->Write("player/crossfadeSubseq",     GetAutoCrossfadeSubseqDetect()? 1L : 0L);

	c->Write("player/autovol",             AvIsEnabled()? 1L : 0L);
	c->Write("player/usealbumvol",         AvGetUseAlbumVol()? 1L : 0L);
	c->Write("player/autovoldes",   (long)(m_avDesiredVolume*1000.0F));
	c->Write("player/autovolmax",   (long)(m_avMaxGain*1000.0F));

	c->Write("player/eqActive", m_eqEnabled? 1L : 0L);
	c->Write("player/eqParam", m_eqParam.ToString());

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

		DeleteStream(&m_streamA, 0);

		if( m_backend )
		{
			delete m_backend;
			m_backend = NULL;
		}

		if( m_prelistenBackend )
		{
			delete m_prelistenBackend;
			m_prelistenBackend = NULL;
		}

		m_queue.Exit();
	}
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
	SjBackendUserdata(SjPlayer* player, bool isPrelistenStream, long onCreateFadeMs, float onCreateFadeDestGain)
	{
		m_player               = player;
		m_isPrelistenStream    = isPrelistenStream;
		m_onCreateFadeMs       = onCreateFadeMs;
		m_onCreateFadeDestGain = onCreateFadeDestGain;
		m_isVideo              = false;
		m_realMs               = 0;
		m_autoDelete           = false; // if set, the stream is deleted on EOS or if fading is done
		m_autoDeleteSend       = false;
	}
	long          m_onCreateFadeMs;
	float         m_onCreateFadeDestGain;
	SjPlayer*     m_player;
	bool          m_isPrelistenStream;
	bool          m_isVideo;
	long          m_realMs;
	SjVolumeCalc  m_volumeCalc;
	SjVolumeFade  m_volumeFade;
	SjEqualizer   m_equalizer;

	bool              m_autoDelete;
	bool              m_autoDeleteSend;
	wxCriticalSection m_autoDeleteCritical;
};


void SjPlayer_BackendCallback(SjBackendCallbackParam* cbp)
{
	// TAKE CARE: this function is called while processing the audio data,
	// just before the output.  So please, do not do weird things here.

	// TAKE CARE II: `stream` may lay on different backends!

	SjBackendStream*   stream     = cbp->stream;        if( stream == NULL )   { return; }
	SjBackendUserdata* userdata   = stream->m_userdata; if( userdata == NULL ) { return; }
	SjPlayer*          player     = userdata->m_player; if( player == NULL )   { return; }
	float*             buffer     = cbp->buffer; // may be NULL
	long               bytes      = cbp->bytes;
	int                samplerate = cbp->samplerate;
	int                channels   = cbp->channels;

	if( cbp->msg == SJBE_MSG_DSP )
	{
		/* DSP
		***********************************************************************/

		if( buffer != NULL && bytes > 0 )
		{
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

			// equalizer - after volumeCalc, otherwise, volumeCalc would calculate the volume depending on the eq settings
			if( player->m_eqEnabled )
			{
				userdata->m_equalizer.AdjustBuffer(buffer, bytes, samplerate, channels);
			}

			// forward the data to the visualisation -
			// we do this after autovol, equalizers etc. so that these changes become visible eg. in the spectrum analyzer
			if( g_visModule->IsVisStarted() && stream == player->m_streamA /*this also excludes prelistening*/ )
			{
				g_visModule->AddVisData(buffer, bytes);
			}

			// apply optional fadings, eg. for crossfading
			if( !userdata->m_volumeFade.AdjustBuffer(buffer, bytes, samplerate, channels) )
			{
				userdata->m_autoDeleteCritical.Enter();
					if( userdata->m_autoDelete && !userdata->m_autoDeleteSend ) {
						userdata->m_autoDeleteSend = true;
						player->SendSignalToMainThread(THREAD_AUTO_DELETE, (uintptr_t)stream);
					}
				userdata->m_autoDeleteCritical.Leave();
			}

			// finally, after the visualisation, apply the main volume and mixdown channels, if appropriate ...
			if( !userdata->m_isPrelistenStream )
			{
				// ... normal stream
				if( player->m_prelistenDest == SJ_PL_LEFT || player->m_prelistenDest == SJ_PL_RIGHT ) {
					SjMixdownChannels(buffer, bytes, channels, player->m_prelistenDest==SJ_PL_LEFT? 1 : 0);
				}

				if( player->m_useSysVol != SJ_SYSVOL_USE ) { // = SJ_SYSVOL_DONTUSE || SJ_SYSVOL_ONLYINIT
					SjApplyVolume(buffer, bytes, player->m_mainGain);
				}
			}
			else
			{
				// ... prelisten stream
				if( player->m_prelistenDest == SJ_PL_LEFT || player->m_prelistenDest == SJ_PL_RIGHT ) {
					SjMixdownChannels(buffer, bytes, channels, player->m_prelistenDest==SJ_PL_LEFT? 0 : 1);
				}

				if( player->m_prelistenDest == SJ_PL_MIX && player->m_useSysVol != SJ_SYSVOL_USE ) { // on prelisten "mix", first apply the normal volume to the channel!
					SjApplyVolume(buffer, bytes, player->m_mainGain);
				}

				SjApplyVolume(buffer, bytes, player->m_prelistenGain);
			}
		}
	}
	else if( cbp->msg == SJBE_MSG_CREATE )
	{
		/* Stream just created - send before any SJBE_MSG_DSP
		***********************************************************************/

		wxASSERT( wxThread::IsMain() );

		userdata->m_volumeCalc.SetPrecalculatedGain(
			g_mainFrame->m_libraryModule->GetAutoVol(stream->GetUrl(), player->AvGetUseAlbumVol())
		);

		userdata->m_equalizer.SetParam(player->m_eqParam);

		if( userdata->m_onCreateFadeMs ) {
			userdata->m_volumeFade.SetVolume(0.0);
			userdata->m_volumeFade.SlideVolume(userdata->m_onCreateFadeDestGain, userdata->m_onCreateFadeMs);
		}
	}
	else if( cbp->msg == SJBE_MSG_VIDEO_DETECTED )
	{
		/* Video detected
		***********************************************************************/

		if( !userdata->m_isVideo && !userdata->m_isPrelistenStream )
		{
			userdata->m_isVideo = true;
			player->SendSignalToMainThread(IDMODMSG_VIDEO_DETECTED);
		}
	}
	else if( cbp->msg == SJBE_MSG_END_OF_STREAM )
	{
		/* End of stream
		***********************************************************************/

		// auto delete stream?
		bool sendEos = true;
		userdata->m_autoDeleteCritical.Enter();
			if( userdata->m_autoDelete )
			{
				if( !userdata->m_autoDeleteSend ) {
					userdata->m_autoDeleteSend = true;
					player->SendSignalToMainThread(THREAD_AUTO_DELETE, (uintptr_t)stream);
				}
				sendEos = false;
			}
		userdata->m_autoDeleteCritical.Leave();

		// just send end-of-stream
		if( sendEos )
		{
			if( stream == player->m_streamA /*this also excludes prelistening streams*/ )
			{
				player->SendSignalToMainThread(THREAD_END_OF_STREAM_A, (uintptr_t)stream);
			}
			else if( userdata->m_isPrelistenStream )
			{
				player->SendSignalToMainThread(THREAD_PRELISTEN_END, (uintptr_t)stream);
			}
		}
	}
	else if( cbp->msg == SJBE_MSG_DESTROY_USERDATA )
	{
		/* destroy
		***********************************************************************/

		wxASSERT( wxThread::IsMain() );

		player->SaveGatheredInfo(stream->GetUrl(), stream->GetStartingTime(), &userdata->m_volumeCalc, userdata->m_realMs);

		delete userdata;
		stream->m_userdata = NULL;
	}
}


/*******************************************************************************
 * Common Player Functionality
 ******************************************************************************/


SjBackendStream* SjPlayer::CreateStream(const wxString& url, bool createPrelistenStream, long explicitSeekMs, long fadeMs)
{
	float fadeDest = 1.0;
	if( !createPrelistenStream && m_prelistenStream && m_prelistenDest == SJ_PL_MIX ) {
		if( fadeMs <= 0 ) fadeMs = 10;
		fadeDest = m_prelistenMixQuiet;
	}

	SjBackendUserdata* userdata = new SjBackendUserdata(this, createPrelistenStream, fadeMs, fadeDest);
	if( userdata == NULL ) {
		return NULL;
	}

	long startThisMs = 0;
	if( explicitSeekMs > 0 ) {
		startThisMs = explicitSeekMs;
	}

	// create the stream
	SjBackendStream* stream;
	if( createPrelistenStream && m_prelistenDest == SJ_PL_OWNOUTPUT ) {
		stream = m_prelistenBackend->CreateStream(url, startThisMs, SjPlayer_BackendCallback, userdata);
	}
	else {
		stream = m_backend->CreateStream(url, startThisMs, SjPlayer_BackendCallback, userdata);
	}

	if( stream == NULL ) {
		return NULL; // userdata should be deleted via SJBE_MSG_DESTROY_USERDATA
	}

	return stream;
}


void SjPlayer::DeleteStream(SjBackendStream** streamPtr, long fadeMs)
{
	wxASSERT( wxThread::IsMain() );

	SjBackendStream* stream = *streamPtr;
	if( stream )
	{
		*streamPtr = NULL; // do this very soon, we check eg. against m_streamA on various situations

		if( fadeMs > 0 )
		{
			m_trashedStreams.Add(stream);
			stream->m_userdata->m_autoDeleteCritical.Enter();
				stream->m_userdata->m_volumeFade.SlideVolume(0.0, fadeMs);
				stream->m_userdata->m_autoDelete = true; // set this _after_ SlideVolume() - otherwise the other thread may have called AdjustBuffer() without adjusting and found m_autoDelete=true
			stream->m_userdata->m_autoDeleteCritical.Leave();
		}
		else
		{
			delete stream;
		}
	}
}


void SjPlayer::Play(long seekMs)
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

		m_streamA = CreateStream(url, false, seekMs, 0);
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


void SjPlayer::Pause()
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


void SjPlayer::Stop(bool stopVisIfPlaying, bool keepPrelisten)
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

	// delete all streams on the devices
	// (on all other placed, please use `DeleteStream()` instead of `delete stream` which will allow fading, check pointers etc.)
	if( m_streamA ) {
		delete m_streamA;
		m_streamA = NULL;
	}

	if( m_prelistenStream && !keepPrelisten ) {
		delete m_prelistenStream; // may lay on m_backend or m_prelistenBackend
		m_prelistenStream = NULL;
	}

	while( m_trashedStreams.GetCount() > 0 ) {
		SjBackendStream* stream = (SjBackendStream*)m_trashedStreams.Item(0);
		m_trashedStreams.RemoveAt(0);
		delete stream;
	}

	// CLOSE devices
	if( m_backend ) {
		m_backend->SetDeviceState(SJBE_STATE_CLOSED);
	}

	if( m_prelistenBackend && m_prelistenStream == NULL ) {
		m_prelistenBackend->SetDeviceState(SJBE_STATE_CLOSED);
	}

	m_paused = false;
}


void SjPlayer::GotoAbsPos(long queuePos, bool fadeToPos) // this is the only function, that _starts_ playback!
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
			DeleteStream(&m_streamA, fadeToPos? m_manCrossfadeMs : 0);

			if( m_backend )
			{
				wxString url = m_queue.GetUrlByPos(queuePos);
				if( !url.IsEmpty() )
				{
					bool deviceOpendedBefore = m_backend->IsDeviceOpened();
					m_streamA = CreateStream(url, false, 0, (fadeToPos && !m_onlyFadeOut)? m_manCrossfadeMs : 0); // may be NULL, we send the signal anyway!
					if( m_streamA )
					{
						if( !deviceOpendedBefore ) {
							if( m_useSysVol==SJ_SYSVOL_USE ) {
								m_backend->SetDeviceVol(m_mainGain);
							}
							else if( m_useSysVol==SJ_SYSVOL_ONLYINIT ) {
								m_backend->SetDeviceVol(1.0);
							}
						}
					}
				}
			}
		}

		SendSignalToMainThread(IDMODMSG_TRACK_ON_AIR_CHANGED);

		inHere = false;
	}
}


void SjPlayer::Seek(long seekMs)
{
	if( m_streamA ) {
		m_streamA->SeekAbs(seekMs);
	}
}


void SjPlayer::SeekPrelisten(long seekMs)
{
	if( m_prelistenStream ) {
		m_prelistenStream->SeekAbs(seekMs);
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


bool SjPlayer::IsAutoPlayOnAir()
{
	if( !m_isInitialized ) { return false; }

	wxString urlOnAir = GetUrlOnAir();
	if( !urlOnAir.IsEmpty() ) {
		long urlOnAirPos = m_queue.GetClosestPosByUrl(urlOnAir);
		if( urlOnAirPos != wxNOT_FOUND ) {
			if( m_queue.GetFlags(urlOnAirPos)&SJ_PLAYLISTENTRY_AUTOPLAY ) {
				return true;
			}
		}
	}

	return false;
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
		if( m_useSysVol==SJ_SYSVOL_USE ) {
			m_backend->SetDeviceVol((float)m_mainGain);
		}
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


void SjPlayer::EqSetParam(const bool* newEnabled, const SjEqParam* newParam)
{
    if( newEnabled ) {
		m_eqEnabled = *newEnabled;
    }

    if( newParam ) {
		m_eqParam = *newParam;
		if( m_streamA ) {
			m_streamA->m_userdata->m_equalizer.SetParam(m_eqParam);
		}
    }
}


void SjPlayer::TogglePrelisten(const wxArrayString& urls)
{
	if( m_prelistenStream )
	{
		// stop prelisten
		DeleteStream(&m_prelistenStream, 1000);
		if( m_streamA && m_prelistenDest == SJ_PL_MIX ) {
			m_streamA->m_userdata->m_volumeFade.SlideVolume(1.0, 2000);
		}
	}
	else if( urls.GetCount() >= 1 )
	{
		// start prelisten
		bool deviceOpendedBefore = m_prelistenBackend->IsDeviceOpened();

		m_prelistenStream = CreateStream(urls[0], true, 0, 0);
		if( m_prelistenStream && m_streamA && m_prelistenDest == SJ_PL_MIX ) {
			m_streamA->m_userdata->m_volumeFade.SlideVolume(m_prelistenMixQuiet, 1000);
		}

		if( !deviceOpendedBefore && m_prelistenDest==SJ_PL_OWNOUTPUT && m_prelistenUseSysVol ) {
			m_prelistenBackend->SetDeviceVol(1.0);
		}
	}
}


void SjPlayer::SetPrelistenGain(float newGain)
{
	if( newGain < 0.1F ) newGain = 0.1F;
	if( newGain > 1.5F ) newGain = 1.5F; // allow little overdrive
	m_prelistenGain = newGain;
}


void SjPlayer::GetPrelistenInfo(wxString& url, long& totalMs, long& elapsedMs, long& remainingMs)
{
	url = "";
	totalMs = -1;
	elapsedMs = -1;
	remainingMs = -1;
	if( m_prelistenStream )
	{
		url = m_prelistenStream->GetUrl();
        m_prelistenStream->GetTime(totalMs, elapsedMs);
		if( totalMs != -1 && elapsedMs != -1 ) {
			remainingMs = totalMs - elapsedMs;
			if( remainingMs < 0 ) { remainingMs = 0; }
		}
	}
}


void SjPlayer::OneSecondTimer()
{
	if( !m_autoCrossfade || m_streamA == NULL || m_streamA->m_userdata == NULL ) {
		return; // crossfading and silence detection disabled OR no stream
	}

	long totalMs = -1, elapsedMs = -1;
	m_streamA->GetTime(totalMs, elapsedMs);
	if( totalMs < m_autoCrossfadeMs*2 || elapsedMs < 0 || m_streamA->m_userdata->m_isVideo ) {
		return; // do not crossfade on very short tracks OR if the position is unknwon OR if the current stream is a video (may cause problems if the next stream is also a video)
	}

	#define HEADROOM_MS 50 // assumed time for stream creation
	long startNextMs = totalMs - HEADROOM_MS;
	if( m_autoCrossfade )  { startNextMs -= m_autoCrossfadeMs+m_crossfadeOffsetEndMs; }

	if( elapsedMs < startNextMs ) {
		return; // still waiting for the correct moment
	}

	// the correct moment is reached!
	if( m_stopAfterThisTrack || m_stopAfterEachTrack ) {
		return; // no crossfade, we shall just stop
	}

	// find out the next url to play
	wxString newUrl;
	long newQueuePos = m_queue.GetNextPos(SJ_PREVNEXT_REGARD_REPEAT);
	if( newQueuePos == -1 )
	{
		// try to enqueue auto-play url
		g_mainFrame->m_autoCtrl.DoAutoPlayIfEnabled(false /*ignoreTimeouts*/);
		newQueuePos = m_queue.GetNextPos(SJ_PREVNEXT_REGARD_REPEAT);
		if( newQueuePos == -1 ) {
			return; // nothing more to play, stop when reaching THREAD_END_OF_STREAM_A
		}
	}
	newUrl = m_queue.GetUrlByPos(newQueuePos);
	if( m_failedUrls.Index( newUrl ) != wxNOT_FOUND ) {
		return; // the URL has just failed, handle this in THREAD_END_OF_STREAM_A
	}

	// try to create the next stream
	DeleteStream(&m_streamA, m_autoCrossfadeMs);
	m_streamA = CreateStream(newUrl, false, 0, m_onlyFadeOut? 0 : m_autoCrossfadeMs);
	m_queue.SetCurrPos(newQueuePos); // realize the new position in the UI

	if( m_streamA ) {
		SendSignalToMainThread(IDMODMSG_TRACK_ON_AIR_CHANGED);
	}
	else {
		SendSignalToMainThread(THREAD_END_OF_STREAM_A); // error - start over
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


void SjPlayer::ReceiveSignal(int signal, uintptr_t extraLong)
{
	if( !m_isInitialized || m_backend == NULL ) { return; }
	wxASSERT( wxThread::IsMain() );

	if( signal==THREAD_END_OF_STREAM_A
	&& (((SjBackendStream*)extraLong)==m_streamA || extraLong==0) )
	{
		// just stop after this track? - we're here if the DSP callback sends EOS or if we resend the signal outself
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
		wxString newUrl;
		long newQueuePos = m_queue.GetNextPos(SJ_PREVNEXT_REGARD_REPEAT);
		if( newQueuePos == -1 )
		{
			// try to enqueue auto-play url
			g_mainFrame->m_autoCtrl.DoAutoPlayIfEnabled(false /*ignoreTimeouts*/);
			newQueuePos = m_queue.GetNextPos(SJ_PREVNEXT_REGARD_REPEAT);

			if( newQueuePos == -1 ) {
				// no chance, there is nothing more to play ...
				Stop();
				SendSignalToMainThread(IDMODMSG_PLAYER_STOPPED_BY_EOQ);
				return;
			}
		}
		newUrl = m_queue.GetUrlByPos(newQueuePos);

		// has the URL just failed? try again in the next message loop
		if( m_failedUrls.Index( newUrl ) != wxNOT_FOUND ) {
			m_queue.SetCurrPos(newQueuePos);
			SendSignalToMainThread(THREAD_END_OF_STREAM_A, 0); // start over
			return;
		}

		// try to create the next stream
		DeleteStream(&m_streamA, 0);
		m_streamA = CreateStream(newUrl, false, 0, 0);
		m_queue.SetCurrPos(newQueuePos); // realize the new position in the UI

		if( m_streamA ) {
			SendSignalToMainThread(IDMODMSG_TRACK_ON_AIR_CHANGED);
		}
		else {
			SendSignalToMainThread(THREAD_END_OF_STREAM_A, 0); // start over
		}
	}
	else if( signal == THREAD_PRELISTEN_END )
	{
		// prelisten stream ended - safely delete the steam from the main thread and update display, menus etc.
		if( ((SjBackendStream*)extraLong) == m_prelistenStream )
		{
			DeleteStream(&m_prelistenStream, 0);
			g_mainFrame->m_libraryModule->UpdateMenuBar();
			g_mainFrame->UpdateDisplay();
		}
	}
	else if( signal == THREAD_AUTO_DELETE )
	{
		// just delete the given stream - the message is needed as we cannot do this from a working thread
		// moreover, as we're again the the main thread, we can also safely modify m_trashedStreams
        SjBackendStream* toDel = (SjBackendStream*)extraLong;
        int i = m_trashedStreams.Index(toDel);
        if( toDel && i != wxNOT_FOUND ) {
			m_trashedStreams.RemoveAt(i);
			delete toDel;
		}
	}
}

