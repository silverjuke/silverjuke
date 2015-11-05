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
 * File:    autoctrl.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke automatic control routines
 *
 ******************************************************************************/



#ifndef __SJ_AUTOCTRL_H__
#define __SJ_AUTOCTRL_H__



enum SjShutdownEtc;
class SjAutoFader;



class SjAutoCtrl
{
public:
	// settings
	               ~SjAutoCtrl         ();
	#define         SJ_AUTOCTRL_FOLLOW_PLAYLIST             0x00010000L
	#define         SJ_AUTOCTRL_RESET_VIEW                  0x00020000L
	#define         SJ_AUTOCTRL_LIMIT_PLAY_TIME             0x00040000L
	#define         SJ_AUTOCTRL_START_VIS                   0x00100000L
	#define         SJ_AUTOCTRL_STOP_VIS                    0x00200000L
	#define         SJ_AUTOCTRL_AUTOPLAY_ENABLED            0x01000000L
	#define         SJ_AUTOCTRL_SLEEP                       0x02000000L
	#define         SJ_AUTOCTRL_SLEEP_FADE                  0x00000001L
	#define         SJ_AUTOCTRL_AUTOPLAY_MAN_ENQ_INTERRUPT  0x08000000L
	#define         SJ_AUTOCTRL_AUTOPLAY_IGNORE             0x10000000L
	#define         SJ_AUTOCTRL_DEF_FLAGS                   0x0000FFFFL
	long            m_flags;

	#define         SJ_AUTOCTRL_DEF_LIMITPLAYTIMESECONDS    180L
	#define         SJ_AUTOCTRL_MIN_LIMITPLAYTIMESECONDS    10L
	#define         SJ_AUTOCTRL_MAX_LIMITPLAYTIMESECONDS    3600L
	long			m_limitPlayTimeSeconds;

	#define         SJ_AUTOCTRL_DEF_FOLLOWPLAYLISTMINUTES   5L
	#define         SJ_AUTOCTRL_MIN_FOLLOWPLAYLISTMINUTES   1L
	#define         SJ_AUTOCTRL_MAX_FOLLOWPLAYLISTMINUTES   999L
	long            m_followPlaylistMinutes;


	#define         SJ_AUTOCTRL_DEF_RESETVIEWMINUTES        6L
	#define         SJ_AUTOCTRL_MIN_RESETVIEWMINUTES        1L
	#define         SJ_AUTOCTRL_MAX_RESETVIEWMINUTES        999L
	long            m_resetViewMinutes;
	long            m_resetViewTo;

	#define         SJ_AUTOCTRL_DEF_STARTVISMINUTES         10L
	#define         SJ_AUTOCTRL_MIN_STARTVISMINUTES         1L
	#define         SJ_AUTOCTRL_MAX_STARTVISMINUTES         999L
	long            m_startVisMinutes;

	#define         SJ_AUTOCTRL_DEF_STOPVISMINUTES          10L
	#define         SJ_AUTOCTRL_MIN_STOPVISMINUTES          1L
	#define         SJ_AUTOCTRL_MAX_STOPVISMINUTES          999L
	long            m_stopVisMinutes;

	#define         SJ_AUTOCTRL_DEF_AUTOPLAYWAITMINUTES     3L
	#define         SJ_AUTOCTRL_MIN_AUTOPLAYWAITMINUTES     0L
	#define         SJ_AUTOCTRL_MAX_AUTOPLAYWAITMINUTES     999L
	long            m_autoPlayWaitMinutes;

	#define         SJ_AUTOCTRL_DEF_AUTOPLAYNUMTRACKS       2L
	#define         SJ_AUTOCTRL_MIN_AUTOPLAYNUMTRACKS       1L
	#define         SJ_AUTOCTRL_MAX_AUTOPLAYNUMTRACKS       999L
	long            m_autoPlayNumTracks;
	long            m_autoPlayMusicSelId;
	long            m_autoPlayMusicSelIgnoreId;
	bool            HasInterruptibleAutoPlay    () const { return (m_flags&(SJ_AUTOCTRL_AUTOPLAY_ENABLED|SJ_AUTOCTRL_AUTOPLAY_MAN_ENQ_INTERRUPT)) == (SJ_AUTOCTRL_AUTOPLAY_ENABLED|SJ_AUTOCTRL_AUTOPLAY_MAN_ENQ_INTERRUPT); }

	// pending events: events are added to this array for some events that
	// happen eg. while an database update. They should be delayed until
	// there is no longer a BusyInfo dialog present
	wxArrayPtrVoid  m_pendingEvents;

	// sleepin'
public:
	void            GetSleepSettings    (bool& enabled, SjShutdownEtc& action, long& timemode, long& minutes, bool& doFade, long& fadeSeconds);
	void            SetSleepSettings    (bool enabled, SjShutdownEtc action, long timemode, long minutes, bool doFade, long fadeSeconds);
	bool            m_stateTriggerSleep;// public to SjAutoFader
private:
	#define         SJ_SLEEPMODE_ACTION_MASK                0x000000FFL // see SJ_SHUTDOWN_*
	#define         SJ_SLEEPMODE_TIMEMODE_MASK              0x00000F00L
	#define         SJ_SLEEPMODE_TIMEMODE_IN                0x00000000L
	#define         SJ_SLEEPMODE_TIMEMODE_AFTER             0x00000100L
	#define         SJ_SLEEPMODE_TIMEMODE_AT                0x00000200L
	#define         SJ_SLEEPMODE_TIMEMODE_ALWAYS_IN         0x00000300L
	#define         SJ_SLEEPMODE_TIMEMODE_ALWAYS_AFTER      0x00000400L
	#define         SJ_SLEEPMODE_TIMEMODE_ALWAYS_AT         0x00000500L
	#define         SJ_SLEEPMODE_DEF_FLAGS                  0x00000000L
	#define         SJ_SLEEPMODE_TIMEMODE_BASIC_COUNT       3
	SjShutdownEtc   m_sleepAction;
	long            m_sleepTimemode;
	#define         SJ_SLEEPMODE_DEF_MINUTES                60 // 60 minutes or 01:00 o'clock
	#define         SJ_SLEEPMODE_MIN_MINUTES                1
	#define         SJ_SLEEPMODE_MAX_MINUTES                99999
	long            m_sleepMinutes;

	#define         SJ_SLEEPMODE_DEF_FADE_SECONDS           60L
	#define         SJ_SLEEPMODE_MIN_FADE_SECONDS           1L
	#define         SJ_SLEEPMODE_MAX_FADE_SECONDS           999L
	long            m_sleepFadeSeconds;

	unsigned long   m_stateSleepWaitTimestamp;
	SjAutoFader*    m_stateSleepAutoFader;
	bool            m_stateHaltedBySleep;

	// misc
public:
	void            LoadAutoCtrlSettings();
	void            SaveAutoCtrlSettings();
	void            ValidateSettings    (); // validation is done automatically on load/save

	// performing actions
	void            OnOneSecondTimer    ();
	bool            DoAutoPlayIfEnabled (bool ignoreTimeouts);
	void            SetAutoPlayUnqueueId(long id) { m_stateLastUnqueueId = id; }

	// public states
	unsigned long   m_stateStartVisTimestamp;
	unsigned long   m_stateStopVisTimestamp;
	unsigned long   m_stateGotoCurrClickedTimestamp;

private:
	// switch to default kiosk layout, if need
	void            SwitchToDefLayout   ();

	// private states
	wxString        m_stateFollowPlaylistUrlFollowed;
	unsigned long   m_stateOpenDialogTimestamp;
	unsigned long   m_lastCleanupTimestamp;

	// auto-play
	wxArrayLong     m_autoPlayedTrackIds;

	long            m_stateAutoPlayTracksLeft;
	unsigned long   m_stateAutoPlayLastPlaybackTimestamp;
	long            m_stateLastAutoPlayQueueId;

	long            m_stateLastUnqueueId;

	bool            EnqueueAutoPlayUrl  ();
	wxString        GetAutoPlayUrl      ();
	bool            LastTrackWasAutoPlay();
	bool            IsCurrTrackCloseToEnd();

	// auto play is only performed if m_haltAutoPlay == 0
	int             m_haltAutoPlay;
	friend class    SjHaltAutoPlay;
	friend class    SjAutoFader;
};



class SjHaltAutoPlay
{
public:
	// helper for setting m_haltAutoPlay
	SjHaltAutoPlay();
	~SjHaltAutoPlay();
};



#endif // __SJ_AUTOCTRL_H__
