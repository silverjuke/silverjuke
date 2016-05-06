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
 * File:    autoctrl.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/autoctrl.h>
#include <sjbase/browser.h>
#include <sjmodules/advsearch.h>
#include <sjmodules/vis/vis_module.h>


/*******************************************************************************
 * SjAutoFader - fade the main volume
 ******************************************************************************/


enum SjFadeDir
{
     SJ_FADEDIR_IN
    ,SJ_FADEDIR_OUT
};


class SjAutoFader : public wxTimer
{
public:
	                SjAutoFader     (long secondsToFade, SjFadeDir dir);
	void            Notify              ();
	virtual void    OnFadingDone        ();
	long            GetOrgVolume        () const { return m_orgVol; }
	bool            WasAborted          () const { return m_aborted; }

private:
	void            FadingDone          (bool aborted);
	unsigned long   m_startFadingTimestamp;
	unsigned long   m_endFadingTimestamp;
	long            m_orgVol, m_startFadingVol, m_lastVol;
	bool            m_inNotify;
	bool            m_aborted;
};


SjAutoFader::SjAutoFader(long secondsToFade, SjFadeDir dir)
	: wxTimer()
{
	m_aborted               = FALSE;
	m_inNotify              = FALSE;
	m_startFadingTimestamp  = SjTools::GetMsTicks();
	m_endFadingTimestamp    = m_startFadingTimestamp + secondsToFade*1000;
	m_orgVol                =
	m_startFadingVol        =
	m_lastVol               = g_mainFrame->GetMainVol();

	Start(100/*ms*/, wxTIMER_CONTINUOUS);
}


void SjAutoFader::Notify()
{
	if( !m_inNotify && g_mainFrame && !SjMainApp::IsInShutdown() )
	{
		m_inNotify = TRUE;

		unsigned long thisTimestamp = SjTools::GetMsTicks();

		// volume changed by the user in between?
		if( m_lastVol != g_mainFrame->GetMainVol() )
		{
			m_startFadingTimestamp = thisTimestamp;
			m_startFadingVol       = g_mainFrame->GetMainVol();
			if( g_mainFrame->IsAllAvailable() )
			{
				FadingDone(TRUE/*aborted*/);
				return;
			}
		}
		else if(  g_mainFrame->m_lastUserInputTimestamp > m_startFadingTimestamp
		      && (g_mainFrame->m_autoCtrl.m_sleepTimemode==SJ_SLEEPMODE_TIMEMODE_AFTER || g_mainFrame->m_autoCtrl.m_sleepTimemode==SJ_SLEEPMODE_TIMEMODE_ALWAYS_AFTER ) )
		{
			if( !g_mainFrame->IsOpAvailable(SJ_OP_MAIN_VOL) )
			{
				g_mainFrame->SetAbsMainVol(m_orgVol);
			}
			FadingDone(TRUE/*aborted*/);
			return;
		}

		// calculate the total fading time
		unsigned long totalFadingMs = SjTimestampDiff(m_startFadingTimestamp, m_endFadingTimestamp);
		unsigned long doneFadingMs = SjTimestampDiff(m_startFadingTimestamp, thisTimestamp);

		// calculate the available volume steps
		long totalSteps = m_startFadingVol;
		if( totalSteps <= 0
		 || thisTimestamp >= m_endFadingTimestamp )
		{
			FadingDone(FALSE/*aborted*/);
			return;
		}

		// calculate the ms for each step
		double msPerVolStep = ((double)totalFadingMs) / ((double)totalSteps);

		// calculate the new volume
		if( msPerVolStep > 0 )
		{
			long newVol = (long)((double)(totalFadingMs-doneFadingMs) / msPerVolStep);
			g_mainFrame->SetAbsMainVol(newVol);
			wxLogDebug(wxT("new vol: %i"), (int)newVol);
		}


		m_lastVol = g_mainFrame->GetMainVol();

		m_inNotify = FALSE;
	}
}


void SjAutoFader::FadingDone(bool aborted)
{
	m_aborted = aborted;
	Stop();
	OnFadingDone();
}


void SjAutoFader::OnFadingDone()
{
	g_mainFrame->m_autoCtrl.m_stateTriggerSleep = TRUE;
}


/*******************************************************************************
 *  SjAutoCtrl - sleep
 ******************************************************************************/


void SjAutoCtrl::GetSleepSettings(bool& enabled, SjShutdownEtc& action, long& timemode, long& minutes,
                                  bool& doFade, long& fadeSeconds)
{
	enabled     = (m_flags&SJ_AUTOCTRL_SLEEP)!=0;
	action      = m_sleepAction;
	timemode    = m_sleepTimemode;
	minutes     = m_sleepMinutes;
	doFade      = (m_flags&SJ_AUTOCTRL_SLEEP_FADE)!=0;
	fadeSeconds = m_sleepFadeSeconds;
}


void SjAutoCtrl::SetSleepSettings(bool enabled, SjShutdownEtc action, long timemode, long minutes,
                                  bool doFade, long fadeSeconds)
{
	// if the sleep settings were changed, reset the "sleep enabled" timestamp
	bool setDisplayMsg = FALSE;

	if( (m_flags&SJ_AUTOCTRL_SLEEP) != (enabled? SJ_AUTOCTRL_SLEEP : 0)
	 || m_sleepAction  != action
	 || m_sleepTimemode!= timemode
	 || m_sleepMinutes != minutes )
	{
		setDisplayMsg = TRUE;
		m_stateTriggerSleep = FALSE;
		m_stateSleepWaitTimestamp = enabled? SjTools::GetMsTicks() : 0;
		if( m_stateSleepAutoFader )
		{
			delete m_stateSleepAutoFader;
			m_stateSleepAutoFader = NULL;
		}
	}

	bool editDoFade = FALSE;
	if( enabled ) // else leave the flag "as is"
	{
		editDoFade = TRUE;
	}

	if( (editDoFade && ((m_flags&SJ_AUTOCTRL_SLEEP_FADE) != (doFade? SJ_AUTOCTRL_SLEEP_FADE : 0)))
	 || m_sleepFadeSeconds != fadeSeconds )
	{
		setDisplayMsg = TRUE;
	}

	// save the settings
	SjTools::SetFlag(m_flags, SJ_AUTOCTRL_SLEEP, enabled);
	m_sleepAction = action;
	m_sleepTimemode = timemode;
	m_sleepMinutes = minutes;
	if( editDoFade ) SjTools::SetFlag(m_flags, SJ_AUTOCTRL_SLEEP_FADE, doFade);
	m_sleepFadeSeconds = fadeSeconds;

	// update display?
	if( setDisplayMsg )
	{
		g_mainFrame->SetDisplayMsg(enabled? _("Sleep mode activated.") : _("Sleep mode deactivated."), 5000);
	}
}


/*******************************************************************************
 * SjAutoCtrl
 ******************************************************************************/


void SjAutoCtrl::LoadAutoCtrlSettings()
{
	// load the settings, as this may reset some sleep flags,
	// this function should only be called once at startups
	#ifdef __WXDEBUG__
		static bool s_debugLoaded = FALSE;
		wxASSERT( s_debugLoaded == FALSE );
		s_debugLoaded = TRUE;
	#endif

	wxConfigBase* c = g_tools->m_config;

	m_flags                             = c->Read(wxT("autoctrl/flags"),            SJ_AUTOCTRL_DEF_FLAGS);
	m_followPlaylistMinutes             = c->Read(wxT("autoctrl/followPlaylist"),   SJ_AUTOCTRL_DEF_FOLLOWPLAYLISTMINUTES);
	m_resetViewMinutes                  = c->Read(wxT("autoctrl/resetView"),        SJ_AUTOCTRL_DEF_RESETVIEWMINUTES);
	m_resetViewTo                       = c->Read(wxT("autoctrl/resetViewTo"),      SJ_BROWSER_ALBUM_VIEW);
	m_startVisMinutes                   = c->Read(wxT("autoctrl/startVis"),         SJ_AUTOCTRL_DEF_STARTVISMINUTES);
	m_stopVisMinutes                    = c->Read(wxT("autoctrl/stopVis"),          SJ_AUTOCTRL_DEF_STOPVISMINUTES);
	m_autoPlayWaitMinutes               = c->Read(wxT("autoctrl/autoPlay"),         SJ_AUTOCTRL_DEF_AUTOPLAYWAITMINUTES);
	m_autoPlayNumTracks                 = c->Read(wxT("autoctrl/autoPlayTracks"),   SJ_AUTOCTRL_DEF_AUTOPLAYNUMTRACKS);
	m_autoPlayMusicSelId                = c->Read(wxT("autoctrl/autoPlayId"),       0L/*=current view*/);
	m_autoPlayMusicSelIgnoreId          = c->Read(wxT("autoctrl/autoPlayIgnoreId"), 0L/*=none*/);

	m_limitPlayTimeSeconds              = c->Read(wxT("autoctrl/limitPlayTime"),    SJ_AUTOCTRL_DEF_LIMITPLAYTIMESECONDS);

	m_sleepMinutes                      = c->Read(wxT("autoctrl/sleepMinutes"),     SJ_SLEEPMODE_DEF_MINUTES);
	m_sleepFadeSeconds                  = c->Read(wxT("autoctrl/sleepFadeSeconds"), SJ_SLEEPMODE_DEF_FADE_SECONDS);
	long sleepFlags                     = c->Read(wxT("autoctrl/sleepFlags"),       SJ_SLEEPMODE_DEF_FLAGS);
	m_sleepAction                       = (SjShutdownEtc)(sleepFlags&SJ_SLEEPMODE_ACTION_MASK);
	m_sleepTimemode                     = sleepFlags&SJ_SLEEPMODE_TIMEMODE_MASK;

	m_jingleFlags                       = c->Read(wxT("autoctrl/jingleFlags"),     0L);
	m_jingleEvery                       = c->Read(wxT("autoctrl/jingleEvery"),     SJ_JINGLE_DEF_MINUTES);
	m_jingleEveryMusicSelId             = c->Read(wxT("autoctrl/jingleEveryId"),   0L/*=none*/);
	m_jingleEveryTriggerTimestamp       = SjTools::GetMsTicks(); // do as if a jingle was played on program startup
	for( int i = 0; i < SJ_JINGLE_AT_CNT; i++ )
	{
		m_jingleAt[i]                   = c->Read(wxString::Format("autoctrl/jingleAt%i", i),    0L);
		m_jingleAtMusicSelId[i]         = c->Read(wxString::Format("autoctrl/jingleAtId%i", i),  0L/*=none*/);
		m_jingleAtTriggerTimestamp[i]   = 0; // not: SjTools::GetMsTicks() - this will force the first trigger without any timeouts
	}

	m_stateStartVisTimestamp            = 0;
	m_stateStopVisTimestamp             = 0;
	m_stateGotoCurrClickedTimestamp     = 0;

	m_stateAutoPlayTracksLeft           = 0;
	m_stateAutoPlayLastPlaybackTimestamp= SjTools::GetMsTicks(); // force the timeout also at program start#

	m_stateLastAutoPlayQueueId          = 0;
	m_stateOpenDialogTimestamp          = 0;
	m_stateLastUnqueueId                = 0;
	m_stateHaltedBySleep                = FALSE;
	m_haltAutoPlay                      = 0;

	m_lastCleanupTimestamp              = SjTools::GetMsTicks();

	// validate the settings
	ValidateSettings();

	// check if we have to reset some sleep flags
	if( m_flags & SJ_AUTOCTRL_SLEEP )
	{
		if( m_sleepTimemode < SJ_SLEEPMODE_TIMEMODE_ALWAYS_IN )
		{
			m_flags &= ~SJ_AUTOCTRL_SLEEP;
		}
	}

	m_stateTriggerSleep         = FALSE;
	m_stateSleepWaitTimestamp   = (m_flags&SJ_AUTOCTRL_SLEEP)? SjTools::GetMsTicks() : 0;
	m_stateSleepAutoFader       = NULL;
}


SjAutoCtrl::~SjAutoCtrl()
{
	if( m_stateSleepAutoFader )
	{
		delete m_stateSleepAutoFader;
		m_stateSleepAutoFader = NULL;
	}
}


void SjAutoCtrl::SaveAutoCtrlSettings()
{
	ValidateSettings();

	wxConfigBase* c = g_tools->m_config;

	c->Write(wxT("autoctrl/flags"),             m_flags);
	c->Write(wxT("autoctrl/followPlaylist"),    m_followPlaylistMinutes);
	c->Write(wxT("autoctrl/resetView"),         m_resetViewMinutes);
	c->Write(wxT("autoctrl/resetViewTo"),       m_resetViewTo);
	c->Write(wxT("autoctrl/startVis"),          m_startVisMinutes);
	c->Write(wxT("autoctrl/stopVis"),           m_stopVisMinutes);
	c->Write(wxT("autoctrl/autoPlay"),          m_autoPlayWaitMinutes);
	c->Write(wxT("autoctrl/autoPlayTracks"),    m_autoPlayNumTracks);
	c->Write(wxT("autoctrl/autoPlayId"),        m_autoPlayMusicSelId);
	c->Write(wxT("autoctrl/autoPlayIgnoreId"),  m_autoPlayMusicSelIgnoreId);

	c->Write(wxT("autoctrl/limitPlayTime"),     m_limitPlayTimeSeconds);

	c->Write(wxT("autoctrl/sleepMinutes"),      m_sleepMinutes);
	c->Write(wxT("autoctrl/sleepFlags"),        m_sleepAction|m_sleepTimemode);
	c->Write(wxT("autoctrl/sleepFadeSeconds"),  m_sleepFadeSeconds);

	c->Write(wxT("autoctrl/jingleFlags"),       m_jingleFlags);
	c->Write(wxT("autoctrl/jingleEvery"),       m_jingleEvery);
	c->Write(wxT("autoctrl/jingleEveryId"),     m_jingleEveryMusicSelId);
	for( int i = 0; i < SJ_JINGLE_AT_CNT; i++ )
	{
		c->Write(wxString::Format("autoctrl/jingleAt%i", i),   m_jingleAt[i]);
		c->Write(wxString::Format("autoctrl/jingleAtId%i", i), m_jingleAtMusicSelId[i]);
	}

	// if the previous number of tracks to play was larger than the new one,
	// align the number of left tracks
	if( m_stateAutoPlayTracksLeft > m_autoPlayNumTracks )
	{
		m_stateAutoPlayTracksLeft = m_autoPlayNumTracks;
	}

	// wait from now on
	m_stateAutoPlayLastPlaybackTimestamp = SjTools::GetMsTicks();
}


void SjAutoCtrl::ValidateSettings()
{
	// these values are set to defaults if out of range
	// (loading errors, there should be no need to use very large or very small values)
	if( m_followPlaylistMinutes < SJ_AUTOCTRL_MIN_FOLLOWPLAYLISTMINUTES
	 || m_followPlaylistMinutes > SJ_AUTOCTRL_MAX_FOLLOWPLAYLISTMINUTES )
	{
		m_followPlaylistMinutes = SJ_AUTOCTRL_DEF_FOLLOWPLAYLISTMINUTES;
	}

	if( m_resetViewMinutes < SJ_AUTOCTRL_MIN_RESETVIEWMINUTES
	 || m_resetViewMinutes > SJ_AUTOCTRL_MAX_RESETVIEWMINUTES )
	{
		m_resetViewMinutes = SJ_AUTOCTRL_DEF_RESETVIEWMINUTES;
	}

	if( m_resetViewTo < 0 || m_resetViewTo >= SJ_BROWSER_VIEW_COUNT )
	{
		m_resetViewTo = SJ_BROWSER_ALBUM_VIEW;
	}

	if( m_startVisMinutes < SJ_AUTOCTRL_MIN_STARTVISMINUTES
	 || m_startVisMinutes > SJ_AUTOCTRL_MAX_STARTVISMINUTES )
	{
		m_startVisMinutes = SJ_AUTOCTRL_DEF_STARTVISMINUTES;
	}

	if( m_stopVisMinutes < SJ_AUTOCTRL_MIN_STOPVISMINUTES
	 || m_stopVisMinutes > SJ_AUTOCTRL_MAX_STOPVISMINUTES )
	{
		m_stopVisMinutes = SJ_AUTOCTRL_DEF_STOPVISMINUTES;
	}

	if( m_limitPlayTimeSeconds < SJ_AUTOCTRL_MIN_LIMITPLAYTIMESECONDS
	 || m_limitPlayTimeSeconds > SJ_AUTOCTRL_MAX_LIMITPLAYTIMESECONDS )
	{
		m_limitPlayTimeSeconds = SJ_AUTOCTRL_DEF_LIMITPLAYTIMESECONDS;
	}

	// for these values, set to min/max as the user may enter very large or
	// very slow values to get a special effect
	if( m_autoPlayWaitMinutes < SJ_AUTOCTRL_MIN_AUTOPLAYWAITMINUTES )   m_autoPlayWaitMinutes = SJ_AUTOCTRL_MIN_AUTOPLAYWAITMINUTES;
	if( m_autoPlayWaitMinutes > SJ_AUTOCTRL_MAX_AUTOPLAYWAITMINUTES )   m_autoPlayWaitMinutes = SJ_AUTOCTRL_MAX_AUTOPLAYWAITMINUTES;

	if( m_autoPlayNumTracks < SJ_AUTOCTRL_MIN_AUTOPLAYNUMTRACKS )   m_autoPlayNumTracks = SJ_AUTOCTRL_MIN_AUTOPLAYNUMTRACKS;
	if( m_autoPlayNumTracks > SJ_AUTOCTRL_MAX_AUTOPLAYNUMTRACKS )   m_autoPlayNumTracks = SJ_AUTOCTRL_MAX_AUTOPLAYNUMTRACKS;

	// sleep mode
	if( m_sleepMinutes < SJ_SLEEPMODE_MIN_MINUTES ) m_sleepMinutes = SJ_SLEEPMODE_MIN_MINUTES;
	if( m_sleepMinutes > SJ_SLEEPMODE_MAX_MINUTES ) m_sleepMinutes = SJ_SLEEPMODE_MAX_MINUTES;

	if( m_sleepFadeSeconds < SJ_SLEEPMODE_MIN_FADE_SECONDS ) m_sleepFadeSeconds = SJ_SLEEPMODE_MIN_FADE_SECONDS;
	if( m_sleepFadeSeconds > SJ_SLEEPMODE_MAX_FADE_SECONDS ) m_sleepFadeSeconds = SJ_SLEEPMODE_MAX_FADE_SECONDS;

	// jingles
	if( m_jingleEvery < SJ_JINGLE_MIN_MINUTES || m_jingleEvery > SJ_JINGLE_MAX_MINUTES ) { m_jingleEvery = SJ_JINGLE_DEF_MINUTES; }
	for( int i = 0; i < SJ_JINGLE_AT_CNT; i++ ) {
		if( m_jingleAt[i] < 0 || m_jingleAt[i] >= 24*60 ) { m_jingleAt[i] = 0; }
	}
}



void SjAutoCtrl::OnOneSecondTimer()
{
	static bool s_inHere = FALSE;
	if( !s_inHere )
	{
		s_inHere = TRUE;

		unsigned long thisTimestamp = SjTools::GetMsTicks();

		g_mainFrame->m_player.OneSecondTimer();

		// automatic control: do cleanup temp?
		//////////////////////////////////////////////////////////////////////////////////////////////


		{
			unsigned long refTimestamp = g_mainFrame->m_lastUserInputTimestamp;

			#define IDLE_MINUTES 2
			#define MIN_IN_BETWEEN_MINUTES 15
			#define MAX_IN_BETWEEN_MINUTES 120
			if(     (       thisTimestamp > refTimestamp+IDLE_MINUTES*60*1000
			             && thisTimestamp > m_lastCleanupTimestamp+MIN_IN_BETWEEN_MINUTES*60*1000
			        )
			        ||      thisTimestamp > m_lastCleanupTimestamp+MAX_IN_BETWEEN_MINUTES*60*1000
			  )
			{
				if( !g_mainFrame->m_mainApp->IsInShutdown()
				 &&  g_mainFrame->IsEnabled()
				 && !SjBusyInfo::InYield() )
				{
					g_tools->m_cache.CleanupFiles();
				}

				m_lastCleanupTimestamp = thisTimestamp;
			}
		}

		// automatic control: do follow playlist?
		//////////////////////////////////////////////////////////////////////////////////////////////

		if( (m_flags & SJ_AUTOCTRL_FOLLOW_PLAYLIST)
		 &&  g_mainFrame->IsPlaying() )
		{
			unsigned long refTimestamp = g_mainFrame->m_lastUserBrowserInputTimestamp;
			if( m_stateOpenDialogTimestamp > refTimestamp ) refTimestamp = m_stateOpenDialogTimestamp;
			if( m_stateGotoCurrClickedTimestamp > refTimestamp ) refTimestamp = 0;

			if( thisTimestamp > (refTimestamp+m_followPlaylistMinutes*60*1000) )
			{
				wxString currUrl = g_mainFrame->m_player.m_queue.GetUrlByPos(-1);
				if( currUrl != m_stateFollowPlaylistUrlFollowed )
				{
					if( g_mainFrame->IsAnyDialogOpened() ) // IsAnyDialogOpened() may be rather slow, so this is checked last
					{
						m_stateOpenDialogTimestamp = thisTimestamp;
					}
					else
					{
						SwitchToDefLayout();

						m_stateFollowPlaylistUrlFollowed = currUrl;

						g_mainFrame->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDO_GOTOCURRAUTO));
					}
				}
			}
		}

		// automatic control: do reset the view?
		//////////////////////////////////////////////////////////////////////////////////////////////

		if( (m_flags & SJ_AUTOCTRL_RESET_VIEW)
		 && g_mainFrame->m_browser != NULL
		 && g_mainFrame->m_browser->GetView() != m_resetViewTo )
		{
			unsigned long refTimestamp = g_mainFrame->m_lastUserBrowserInputTimestamp;
			if( m_stateOpenDialogTimestamp > refTimestamp ) refTimestamp = m_stateOpenDialogTimestamp;
			if( m_stateGotoCurrClickedTimestamp > refTimestamp ) refTimestamp = 0;

			if( thisTimestamp > (refTimestamp+m_resetViewMinutes*60*1000) )
			{
				g_mainFrame->m_browser->SetView_(m_resetViewTo, TRUE, TRUE);
			}
		}

		// automatic control: do start vis?
		//////////////////////////////////////////////////////////////////////////////////////////////

		if( (m_flags & SJ_AUTOCTRL_START_VIS)
		 &&  g_mainFrame->IsPlaying()
		 && !g_visModule->IsVisStarted() )
		{
			unsigned long refTimestamp = g_mainFrame->m_lastUserInputTimestamp;
			if( m_stateOpenDialogTimestamp > refTimestamp ) refTimestamp = m_stateOpenDialogTimestamp;
			if( m_stateStopVisTimestamp > refTimestamp ) refTimestamp = m_stateStopVisTimestamp;

			if( thisTimestamp > (refTimestamp+m_startVisMinutes*60*1000) )
			{
				wxWindow* focusWindow = wxWindow::FindFocus();
				if( focusWindow )
				{
					focusWindow = SjDialog::FindTopLevel(focusWindow);
					if( focusWindow == g_mainFrame )
					{
						if( g_mainFrame->IsAnyDialogOpened() ) // IsAnyDialogOpened() may be rather slow, so this is checked last
						{
							m_stateOpenDialogTimestamp = thisTimestamp;
						}
						else
						{
							if( IsCurrTrackCloseToEnd() == 0 )
							{
								SwitchToDefLayout();

								g_visModule->StartVis();
							}
						}
					}
					else
					{
						m_stateStopVisTimestamp = thisTimestamp; // reset timeout as the Silverjuke window is not the top-level window
					}
				}
				else
				{
					m_stateStopVisTimestamp = thisTimestamp; // reset timeout as the Silverjuke window is not the top-level window
				}
			}
		}


		// automatic control: do stop vis?
		//////////////////////////////////////////////////////////////////////////////////////////////


		if( (m_flags & SJ_AUTOCTRL_STOP_VIS)
		 &&  g_visModule->IsVisStarted() )
		{
			if( thisTimestamp > (m_stateStartVisTimestamp+m_stopVisMinutes*60*1000) )
			{
				g_visModule->StopVis();
			}
		}

		// automatic control: jingles
		//////////////////////////////////////////////////////////////////////////////////////////////


		if( m_jingleFlags&(SJ_JINGLE_EVERY|SJ_JINGLE_AT0|SJ_JINGLE_AT1)
		 && !g_mainFrame->m_inConstruction
		 && !g_mainFrame->m_mainApp->IsInShutdown() )
		{
			if( m_jingleFlags&SJ_JINGLE_EVERY // the normal jingles are only played if the jukebox is playing.
			 && g_mainFrame->IsPlaying() )
			{
				unsigned long nextTimestamp = m_jingleEveryTriggerTimestamp+m_jingleEvery*60*1000;
				if( thisTimestamp > nextTimestamp )
				{
					m_jingleEveryTriggerTimestamp = thisTimestamp;
					SjAdvSearch advSearch = g_advSearchModule->GetSearchById(m_jingleEveryMusicSelId);
					if( advSearch.GetId()==0 )
					{
						SjTools::SetFlag(m_jingleFlags, SJ_JINGLE_EVERY, false);
						SaveAutoCtrlSettings();
					}
					wxString url = advSearch.GetRandomUrl();
					if( !url.IsEmpty() ) {
						g_mainFrame->Enqueue(url, -2/*enqueue next*/, TRUE/*verified*/);
					}
				}
			}

			for( int i = 0; i < SJ_JINGLE_AT_CNT; i++ )
			{
				if( m_jingleFlags&(SJ_JINGLE_AT0<<i) ) // the jingles at conctrete times are played even if the jukebox is paused/stopped.
				{
					wxDateTime now = wxDateTime::Now();
					long thisMinuteOfToday = now.GetHour()*60 + now.GetMinute();
					if( m_jingleAt[i] == thisMinuteOfToday
					 && thisTimestamp > m_jingleAtTriggerTimestamp[i]+70*1000 )
					{
						m_jingleAtTriggerTimestamp[i] = thisTimestamp;
						SjAdvSearch advSearch = g_advSearchModule->GetSearchById(m_jingleAtMusicSelId[i]);
						if( advSearch.GetId()==0 || (m_jingleFlags&(SJ_JINGLE_AT0_DAILY<<i))==0 )
						{
							SjTools::SetFlag(m_jingleFlags, SJ_JINGLE_AT0<<i, false);
							SaveAutoCtrlSettings();
						}
						wxString url = advSearch.GetRandomUrl();
						if( !url.IsEmpty() ) {
							g_mainFrame->Enqueue(url, (m_jingleFlags&(SJ_JINGLE_AT0_WAIT<<i))? -2 : -3, TRUE/*verified*/);
						}
					}
				}
			}
		}

		// automatic control: limit play time
		//////////////////////////////////////////////////////////////////////////////////////////////

		if( (m_flags & SJ_AUTOCTRL_LIMIT_PLAY_TIME)
		 && !g_mainFrame->m_inConstruction
		 && !g_mainFrame->m_mainApp->IsInShutdown()
		 && !SjBusyInfo::InYield()
		 &&  IsCurrTrackCloseToEnd() != 1 /*limit if we're "not close to end" or if "we don't know"*/
		 &&  g_mainFrame->IsPlaying() )
		{
			long elapsedMs = g_mainFrame->GetElapsedTime();
			if( elapsedMs > m_limitPlayTimeSeconds*1000 )
			{
				g_mainFrame->SetDisplayMsg(_("Play time exceeded"), 5000); // should be set before GotoNextRegardAP() to avoid flickering
				if( !g_mainFrame->GotoNextRegardAP(true /*fade to next*/, false /*do not ignore timeouts (leave a gap of some minutes if desired*/) )
				{
					g_mainFrame->Stop();
					g_mainFrame->m_haltedManually = false; /*set to "true" in Stop() - but this was no manual stop, we want the autoplay to start over*/
				}
			}
		}


		// automatic control: check auto-play
		//////////////////////////////////////////////////////////////////////////////////////////////

		if( (m_flags & SJ_AUTOCTRL_AUTOPLAY_ENABLED)
		 && !g_mainFrame->m_inConstruction
		 && !g_mainFrame->m_mainApp->IsInShutdown()
		 && !SjBusyInfo::InYield()
		 && m_haltAutoPlay == 0 )
		{
			if( g_mainFrame->IsStopped() )
			{
				// the player is currently stopped...

				// ...see if we have to add an auto-play URL
				if( !g_mainFrame->m_haltedManually
				 && !m_stateHaltedBySleep )
				{
					if( m_stateAutoPlayTracksLeft > 0 && LastTrackWasAutoPlay() )
					{
						EnqueueAutoPlayUrl();
						m_stateAutoPlayTracksLeft--;
					}
					else if( (thisTimestamp > (m_stateAutoPlayLastPlaybackTimestamp+m_autoPlayWaitMinutes*60*1000))
					      || (m_stateLastUnqueueId!=0 && m_stateLastUnqueueId==m_stateLastAutoPlayQueueId) )
					{
						wxASSERT( m_autoPlayNumTracks > 0 );

						m_stateAutoPlayTracksLeft = m_autoPlayNumTracks;
						EnqueueAutoPlayUrl();
						m_stateAutoPlayTracksLeft--;
					}
				}
			}
			else
			{
				// the player is currently playing or pause... - auto-enqueing is done by SjPlayer by calling DoAutoPlayIfEnabled()
				m_stateHaltedBySleep = FALSE;

				// ...remember the last time sth. was played
				m_stateAutoPlayLastPlaybackTimestamp = thisTimestamp;

				/*// ...see if we have to add an auto-play URL,
				// we do this only if the player is not paused
				if(  IsCurrTrackCloseToEnd() != 0
				 && !g_mainFrame->IsPaused()
				 && !g_mainFrame->HasNextIgnoreAP() ) // HasNext() seems only to work exactly if playing, so better check m_haltedManually in the stopped-condtion above
				{
				    if( m_stateAutoPlayTracksLeft > 0 && LastTrackWasAutoPlay() )
				    {
				        EnqueueAutoPlayUrl();
				        m_stateAutoPlayTracksLeft--;
				    }
				    else if( m_autoPlayWaitMinutes == 0 )
				    {
				        wxASSERT( m_autoPlayNumTracks > 0 );

				        m_stateAutoPlayTracksLeft = m_autoPlayNumTracks;
				        EnqueueAutoPlayUrl();
				        m_stateAutoPlayTracksLeft--;
				    }
				}*/
			}
		}

		// automatic control: sleep mode
		//////////////////////////////////////////////////////////////////////////////////////////////


		if( (m_flags & SJ_AUTOCTRL_SLEEP)
		 && !g_mainFrame->m_inConstruction
		 && !g_mainFrame->m_mainApp->IsInShutdown() )
		{
			#ifdef __WXDEBUG__
				if( m_stateTriggerSleep )
				{
					wxLogDebug(wxT("Sleep triggered..."));
				}
			#endif

			if( m_stateSleepWaitTimestamp )
			{
				// sleep mode is enabled by not yet triggered: check the timeouts ...

				switch( m_sleepTimemode )
				{
					case SJ_SLEEPMODE_TIMEMODE_IN:
					case SJ_SLEEPMODE_TIMEMODE_ALWAYS_IN:
						if( thisTimestamp > (m_stateSleepWaitTimestamp+m_sleepMinutes*60*1000) )
						{
							m_stateTriggerSleep = TRUE;
						}
						break;

					case SJ_SLEEPMODE_TIMEMODE_AFTER:
					case SJ_SLEEPMODE_TIMEMODE_ALWAYS_AFTER:
						if( thisTimestamp > (g_mainFrame->m_lastUserInputTimestamp+m_sleepMinutes*60*1000) )
						{
							m_stateTriggerSleep = TRUE;
						}
						break;

					case SJ_SLEEPMODE_TIMEMODE_AT:
					case SJ_SLEEPMODE_TIMEMODE_ALWAYS_AT:
					{
						static unsigned long s_thisTriggerTimestamp = 0;
						wxDateTime now = wxDateTime::Now();
						long thisMinuteOfToday = now.GetHour()*60 + now.GetMinute();
						if(  m_sleepMinutes == thisMinuteOfToday
						 && (s_thisTriggerTimestamp == 0 || (thisTimestamp>s_thisTriggerTimestamp+70*1000)) )
						{
							m_stateTriggerSleep = TRUE;
							s_thisTriggerTimestamp = thisTimestamp;
						}
					}
					break;
				}

				if( m_stateTriggerSleep )
				{
					g_mainFrame->SetDisplayMsg(_("Sleep mode activated."), 5000);

					m_stateSleepWaitTimestamp = 0;
					if( m_flags&SJ_AUTOCTRL_SLEEP_FADE
					 && g_mainFrame->IsPlaying() )
					{
						m_stateTriggerSleep = FALSE;
						if( m_stateSleepAutoFader )
						{
							delete m_stateSleepAutoFader;
							m_stateSleepAutoFader = NULL;
						}
						m_stateSleepAutoFader = new SjAutoFader(m_sleepFadeSeconds, SJ_FADEDIR_OUT);
					}
				}
			}
			else if( m_stateTriggerSleep )
			{
				// sleep mode is triggered and an (optional) fadeout is done ... do what to do
				long orgVol = -1;
				bool aborted = FALSE;

				m_stateTriggerSleep = FALSE;
				if( m_stateSleepAutoFader )
				{
					orgVol = m_stateSleepAutoFader->GetOrgVolume();
					aborted = m_stateSleepAutoFader->WasAborted();
					delete m_stateSleepAutoFader;
					m_stateSleepAutoFader = NULL;
				}

				if( m_sleepTimemode < SJ_SLEEPMODE_TIMEMODE_ALWAYS_IN )
				{
					m_flags &= ~SJ_AUTOCTRL_SLEEP; // no need to save this setting as it is also changed when Silverjuke is loaded
				}

				if( !aborted )
				{
					SjMainApp::DoShutdownEtc(m_sleepAction, orgVol);

					g_mainFrame->m_haltedManually = FALSE;
					m_stateHaltedBySleep = TRUE;
				}
				else
				{
					g_mainFrame->SetDisplayMsg(_("Sleep mode deactivated."), 3000);
				}

				// start over if SJ_AUTOCTRL_SLEEPMODE is not cleared above
				m_stateSleepWaitTimestamp = SjTools::GetMsTicks();
			}
		}

		// any pending events?
		//////////////////////////////////////////////////////////////////////////////////////////////

		if(  m_pendingEvents.GetCount()
		 && !SjBusyInfo::InYield() )
		{
			int i, iCount = m_pendingEvents.GetCount();
			for( i = 0; i < iCount; i++ )
			{
				wxEvent* event = (wxEvent*)m_pendingEvents[i];
				if( event )
				{
					g_mainFrame->GetEventHandler()->QueueEvent(event);
				}
			}
			m_pendingEvents.Clear();
		}

		s_inHere = FALSE;
	}
}


bool SjAutoCtrl::DoAutoPlayIfEnabled(bool ignoreTimeouts)
{
	bool ret = false;

	if( (m_flags & SJ_AUTOCTRL_AUTOPLAY_ENABLED) )
	{
		if( ignoreTimeouts || m_autoPlayWaitMinutes == 0 )
		{
			m_stateAutoPlayTracksLeft = m_autoPlayNumTracks;
		}
		else if( m_stateAutoPlayTracksLeft <= 0 )
		{
			wxASSERT( m_stateAutoPlayTracksLeft == 0 );
			return false;
		}

		ret = EnqueueAutoPlayUrl();
		m_stateAutoPlayTracksLeft--;
	}

	return ret;
}


bool SjAutoCtrl::EnqueueAutoPlayUrl()
{
	wxASSERT( SJ_AUTOCTRL_MIN_AUTOPLAYNUMTRACKS > 0 );
	wxASSERT( m_stateAutoPlayTracksLeft > 0 );

	// get the URL to play
	wxString url = GetAutoPlayUrl();
	if( url.IsEmpty() )
	{
		return false;
	}

	// enqueue this URL
	long oldQueueCount = g_mainFrame->GetQueueCount();
	g_mainFrame->Enqueue(url, -1, TRUE/*verified*/, TRUE/*autoplay*/);

	// remember the ID of the URL just enqueued
	long newQueueCount = g_mainFrame->GetQueueCount();
	wxASSERT( oldQueueCount == newQueueCount-1 );

	if( oldQueueCount == newQueueCount-1 )
	{
		m_stateLastAutoPlayQueueId = g_mainFrame->m_player.m_queue.GetIdByPos(newQueueCount-1);
		return true;
	}
	else
	{
		return false;
	}
}


bool SjAutoCtrl::LastTrackWasAutoPlay()
{
	long queueCount = g_mainFrame->GetQueueCount();
	if( queueCount > 0
	 && g_mainFrame->m_player.m_queue.GetIdByPos(queueCount-1) == m_stateLastAutoPlayQueueId )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


int SjAutoCtrl::IsCurrTrackCloseToEnd(long headroomMs) // returns 0, 1 or -1 for unknown
{
	// headroomMs defaults to 10 seconds: before anythings starts; the player itself prepares a new stream 5 seconds before the end of the previous one
	long remainingMs = g_mainFrame->GetRemainingTime();
	wxASSERT( remainingMs >= -1 );
	if( remainingMs < 0 )
	{
		return -1; // dont'know
	}

	long crossfadeMs = g_mainFrame->m_player.GetAutoCrossfade()?
		(g_mainFrame->m_player.m_crossfadeOffsetEndMs+g_mainFrame->m_player.m_autoCrossfadeMs) : 0;
	wxASSERT( remainingMs >= 0 );
	wxASSERT( crossfadeMs >= 0 );
	if( remainingMs < (crossfadeMs+headroomMs) )
	{
		return 1; // we're close to end
	}

	return 0; // we're not close to end
}


wxString SjAutoCtrl::GetAutoPlayUrl()
{
	wxArrayLong trackIdsArray;
	long        trackIdsCount = 0;

	// get an array of available track IDs
	//////////////////////////////////////

	{
		// collect the track IDs to ignore
		SjLLHash ignoreIdsHash;
		long     ignoreIdsCount = 0;
		if( m_flags & SJ_AUTOCTRL_AUTOPLAY_IGNORE )
		{
			SjAdvSearch advSearch = g_advSearchModule->GetSearchById(m_autoPlayMusicSelIgnoreId);
			if( advSearch.GetId()==0 )
			{
				// the adv. search to ignore was deleted; disable the ignore function
				m_flags &= ~SJ_AUTOCTRL_AUTOPLAY_IGNORE;
				SaveAutoCtrlSettings();
			}

			wxString dummySelectSql;
			advSearch.GetAsSql(&ignoreIdsHash, dummySelectSql);
			ignoreIdsCount = ignoreIdsHash.GetCount();
		}

		// collect the possible track IDs
		SjLLHash trackIdsHash;
		if( m_autoPlayMusicSelId == 0 )
		{
			// get the IDs currently in view - this is
			//      - an adv. search
			//      - a simple search,
			//      - an adv. plus a simple search
			//      - all tracks
			g_mainFrame->m_libraryModule->GetIdsInView(&trackIdsHash, TRUE/*ignoreSimpleSearchIfNull*/,
			        TRUE/*ignoreAdvSearchIfNull*/);
		}
		else
		{
			// get the adv. search to use
			SjAdvSearch advSearch = g_advSearchModule->GetSearchById(m_autoPlayMusicSelId);
			if( advSearch.GetId()==0 )
			{
				// the adv. search to use was deleted; disable the auto-play
				// functionality until the user selects a valid adv. search to use
				m_autoPlayMusicSelId = 0; // reset to "current view"
				m_flags &= ~SJ_AUTOCTRL_AUTOPLAY_ENABLED;
				SaveAutoCtrlSettings();
				return wxT("");
			}

			// get all track IDs in this adv. search
			wxString dummySelectSql;
			advSearch.GetAsSql(&trackIdsHash, dummySelectSql);
		}

		// convert hash to array
		long tempTrackId;
		long maxTrackIdsCount = trackIdsHash.GetCount(); if( maxTrackIdsCount < 16 ) maxTrackIdsCount = 16;
		trackIdsArray.Alloc(maxTrackIdsCount);
		SjHashIterator iterator;
		while( (trackIdsHash.Iterate(iterator, &tempTrackId)) )
		{
			if( ignoreIdsCount==0L || ignoreIdsHash.Lookup(tempTrackId)==0L )
			{
				trackIdsArray.Add(tempTrackId);
				trackIdsCount++;
			}
		}

		if( trackIdsCount <= 0 )
		{
			// there are no tracks in this music selection; however, DO NOT
			// disable auto-play therefore as the music selection may be dynamic
			// eg. sth. like "tracks played today"
			return wxEmptyString;
		}
	}

	// select a random track ID from the available track IDs
	////////////////////////////////////////////////////////

	#define MAX_REMEMBER_IDS 32
	#define MAX_ITERATIONS   1000

	wxSqlt sql;
	unsigned long now = SjTools::GetMsTicks();
	long selectedTrackId = 0;
	for( int iterations = 0; iterations < MAX_ITERATIONS; iterations++ )
	{
		long testIndex = SjTools::Rand(trackIdsCount);
		wxASSERT( testIndex >= 0 && testIndex < trackIdsCount);
		wxASSERT( trackIdsCount <= (long)trackIdsArray.GetCount() );

		selectedTrackId = trackIdsArray[testIndex];

		if( m_autoPlayedTrackIds.Index(selectedTrackId)==wxNOT_FOUND )
		{
			// not in internal cache,
			// also check agains the "avoid boredom" settings, see http://www.silverjuke.net/forum/topic-2998.html
			sql.Query(wxString::Format(wxT("SELECT leadartistname, trackname FROM tracks WHERE id=%lu;"), selectedTrackId));
			if( !sql.Next() )
				break; // okay, fine track found

			if( !g_mainFrame->m_player.m_queue.IsBoring(sql.GetString(0), sql.GetString(1), now) )
				break; // okay, fine track found
		}

		// remove the test index from trackIdsArray
		trackIdsCount--;
		if( trackIdsCount <= 0 )
			break; // nothing found, nevertheless, use selectedTrackId
		trackIdsArray[testIndex] = trackIdsArray[trackIdsCount /*one substracted above!*/];
	}

	// done, add the track to the internal cache that avoids playing the same tracks too often
	// (this cache is used in addition to "avoid boredom")
	m_autoPlayedTrackIds.Add(selectedTrackId);
	if( m_autoPlayedTrackIds.GetCount() > MAX_REMEMBER_IDS )
	{
		m_autoPlayedTrackIds.RemoveAt(0);
	}

	// done, get the URL from the track id
	return g_mainFrame->m_libraryModule->GetUrl(selectedTrackId);
}


void SjAutoCtrl::SwitchToDefLayout()
{
	if( g_mainFrame && g_mainFrame->IsKioskStarted() )
	{
		SjSkinLayout* currLayout = g_mainFrame->GetLayout();
		SjSkinLayout* kioskLayout = g_mainFrame->GetLayout(wxT("kiosk"));
		if( currLayout != kioskLayout && kioskLayout != NULL )
		{
			g_mainFrame->LoadLayout(kioskLayout);
		}
	}
}


/*******************************************************************************
 * help class for setting m_haltAutoPlay
 ******************************************************************************/


SjHaltAutoPlay::SjHaltAutoPlay()
{
	g_mainFrame->m_autoCtrl.m_haltAutoPlay++;
}


SjHaltAutoPlay::~SjHaltAutoPlay()
{
	g_mainFrame->m_autoCtrl.m_haltAutoPlay--;
}
