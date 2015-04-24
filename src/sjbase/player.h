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
 * File:    player.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke player
 *
 ******************************************************************************/


#ifndef __SJ_PLAYER_H__
#define __SJ_PLAYER_H__


class SjPlayerModule : public SjCommonModule
{
public:
	SjPlayerModule      (SjInterfaceBase*);
	void            GetLittleOptions    (SjArrayLittleOption&);

protected:
	bool            m_settingsLoaded;
	bool            FirstLoad           ();
	void            LastUnload          ();
};


extern SjPlayerModule* g_playerModule;
class SjPlayerImpl;


class SjPlayer
{
public:
	// Constructor/Destructor, Init/Exit, Load/Save settings.
	// for complete contruction call SjPlayer() AND Init(), same for destruction.
	// LoadSettings() and SaveSettings() always saves the settings to the m_player
	// instance, so you should use this only for the main player
	SjPlayer            ();
	~SjPlayer           ();
	void            Init                ();
	void            Exit                ();
	SjQueue         m_queue;

	// Load the player settings - note that you can use these functions
	// only for the main player instance
	void            LoadSettings        ();
	void            SaveSettings        () const;

	// Check if a given file is playable
	bool            TestUrl             (const wxString& url);
	const SjExtList* GetExtList         ();

	// Basic Player Control.
	// The playback is NOT started implicit by Goto() or Enqueue() functions.
	// To check if the operations were successfull, use the Is*()/Get*() functions.
	void            Play                (long ms=0, bool fadeToPlay=false);
	void            Pause               (bool fadeToPause=false);
	void            PlayOrPause         (bool fadeToPlayOrPause=false)      { if(IsPlaying()) {Pause(fadeToPlayOrPause);} else {Play(0, fadeToPlayOrPause);} }
	void            Stop                (bool stopVisIfPlaying=true);
	void            StopAfterThisTrack  (bool s)                            { m_stopAfterThisTrack=s; }
	bool            StopAfterThisTrack  () const                            { return m_stopAfterThisTrack; }
	void            StopAfterEachTrack  (bool s)                            { m_stopAfterEachTrack=s; }
	bool            StopAfterEachTrack  () const                            { return m_stopAfterEachTrack; }
	bool            IsPlaying           ()                                  { return !GetUrlOnAir().IsEmpty() && !m_paused; }
	bool            IsPaused            ()                                  { return !GetUrlOnAir().IsEmpty() && m_paused; }
	bool            IsStopped           ()                                  { return GetUrlOnAir().IsEmpty(); }
	wxString        GetUrlOnAir         ();
	bool            IsAutoPlayOnAir     ();

	// Handling the main volume slider and "mute", volumes between 0..255 are valid.
	// If you mute the volume, the returned main volume is 0.
	void            SetMainVol          (int v);
	int             GetMainVol          () const                            { return m_mainVol; }
	void            SetMainVolMute      (bool);
	bool            GetMainVolMute      () const                            { return (GetMainVol()==0); }
	void            ToggleMainVolMute   ()                                  { SetMainVolMute(!GetMainVolMute()); }

	// Goto specific tracks in the queue; if playing, this will stop the currently
	// playing song and start the new one. If pause, this will stop the player.
	// Playback is never started by any Goto() function.
	// GotoPrev()/GotoNextIgnoreAP() are NOT the same as GotoRelPos(-1/+1) as the former
	// regards shuffle settings and the latter does not.
	bool            HasPrev             ()                      { return m_queue.GetPrevPos(SJ_PREVNEXT_LOOKUP_ONLY)!=-1; }
	void            GotoPrev            (bool fadeToPrev)       { long p=m_queue.GetPrevPos(0); if(p!=-1) { GotoAbsPos(p, fadeToPrev); } }
	bool            HasNextIgnoreAP     ()                      { return m_queue.GetNextPos(SJ_PREVNEXT_LOOKUP_ONLY)!=-1; }
	void            GotoNextIgnoreAP    (bool fadeToNext)       { long p=m_queue.GetNextPos(0); if(p!=-1) { GotoAbsPos(p, fadeToNext); } }
	void            GotoRelPos          (long p)                { GotoAbsPos(m_queue.GetCurrPos()+p); }
	void            GotoUrl             (const wxString& url)   { long p=m_queue.GetClosestPosByUrl(url); if(p!=-1) { GotoAbsPos(p); } }
	void            GotoAbsPos          (long, bool fadeToPos=false);

	// Get Time Information, Seek inside the track
	void            GetTime             (long& totalMs, long& elapsedMs, long& remainingMs);
	long            GetTotalTime        ()          { long t1, t2, t3; GetTime(t1, t2, t3); return t1; }
	long            GetElapsedTime      ()          { long t1, t2, t3; GetTime(t1, t2, t3); return t2; }
	long            GetRemainingTime    ()          { long t1, t2, t3; GetTime(t1, t2, t3); return t3; }
	long            GetEnqueueTime      ();
	void            SeekAbs             (long ms);
	void            SeekRel             (long ms)   { SeekAbs(GetElapsedTime()+ms); }

	// Automatic volume
#define         SJ_AV_DEF_STATE             TRUE
#define         SJ_AV_DEF_DESIRED_VOLUME    1.0F
#define         SJ_AV_DEF_MAX_GAIN          5.0F
#define         SJ_AV_DEF_USE_ALBUM_VOL     FALSE
	void            AvEnable            (bool s) {  m_avEnabled = s; }
	bool            AvIsEnabled         () const { return m_avEnabled; }
	void            AvSetUseAlbumVol    (bool);
	bool            AvGetUseAlbumVol    () const { return m_avUseAlbumVol; }
	void            AvSetDesiredVolume  (float v) { m_avDesiredVolume = v; }
	float           AvGetDesiredVolume  () const { return m_avDesiredVolume; }
	void            AvSetMaxGain        (float m) { m_avMaxGain = m; }
	float           AvGetMaxGain        () const { return m_avMaxGain; }
	float           AvGetCalculatedGain () const { return m_avCalculatedGain; } // this does not include the desired volume nor the max. gain!

	// Crossfade & Other fadings
	void            SetAutoCrossfade    (bool e) { m_autoCrossfade=e; }
	bool            GetAutoCrossfade    () const { return m_autoCrossfade; }
	void            SetAutoCrossfadeSubseqDetect(bool r) { m_autoCrossfadeSubseqDetect=r; }
	bool            GetAutoCrossfadeSubseqDetect() const { return m_autoCrossfadeSubseqDetect; }
	void            SetOnlyFadeOut      (bool e) { m_onlyFadeOut = e; }
	bool            GetOnlyFadeOut      () const { return m_onlyFadeOut; }
	void            SetSkipSilence      (bool r) { m_skipSilence=r; }
	bool            GetSkipSilence      () const { return m_skipSilence; }

	// Crossfade & Other fadings: Time settings
#define         SJ_DEF_CROSSFADE_MS 10000L
	long            m_autoCrossfadeMs;
	long            m_manCrossfadeMs;

#define         SJ_FF_DEF_PAUSE2PLAY_MS 150
#define         SJ_FF_DEF_PLAY2PAUSE_MS 300
#define         SJ_FF_DEF_PLAY2EXIT_MS  500 // cannot be edited, we will always be "smart"
#define         SJ_FF_DEF_GOTO_MS       1000
	long            m_ffPause2PlayMs;
	long            m_ffPlay2PauseMs;
	long            m_ffGotoMs;

	// GetVisData() is only for visualisation modules for retrieving
	// data to visualisize. latencyBytes is the number of bytes
	// between the call of this function and really drawing the data
	// (the number of bytes reflect the latency time in sample size).
	void            GetVisData          (unsigned char* buffer, long bytes, long latencyBytes);

	// The IDP_* messages posted to the main module should
	// be given to ReceiveSignal().  This is to avoid using a separate
	// thread for this purpose.
	void            ReceiveSignal       (int id, uintptr_t extraLong);

	// the following can be treated as private, but for easier implementations it is declared as public
	// (m_impl is known only to the private implementation, which, however, may define objects that may need access to m_impl;
	// SendSignalToMainThread() is called by the implementation part)

private:
	int             m_mainBackupVol;
	bool            m_stopAfterThisTrack;
	bool            m_stopAfterEachTrack;

public:
	// the following Do* function must be implemented by the player implementations (they can use m_impl for this),
	// which can rely on parameter checking and, if appropriate, all status are set fine before they're called.
	// this part is "private", however, due to easier implementations and less header dependencies, it is declared as public
	void            DoInit              ();
	void            DoExit              ();
	const SjExtList* DoGetExtList       ();
	void            DoPlay              (long ms, bool fadeToPlay);
	void            DoPause             (bool fadeToPause);
	void            DoStop              ();
	void            DoGotoAbsPos        (long, bool fadeToPos);
	wxString        DoGetUrlOnAir       ();
	void            DoGetTime           (long& totalMs, long& elapsedMs);
	void            DoSetMainVol        ();
	void            DoSeekAbs           (long ms);
	void            DoGetVisData        (unsigned char* pcmBuffer, long bytes, long visLatencyBytes);
	void            DoReceiveSignal     (int id, uintptr_t extraLong);
	void            DoGetLittleOptions  (SjArrayLittleOption&);
	SjPlayerImpl*   m_impl;

	// tools for the implementation
	void            SendSignalToMainThread(int id, uintptr_t extraLong=0) const;
	void            SaveGatheredInfo    (const wxString& url, unsigned long startingTime, SjVolumeCalc*, long realDecodedBytes);

	// main volume stuff
	int             m_mainVol;
	double          m_mainGain;

	// auto volume stuff
	bool            m_avEnabled;
	bool            m_avUseAlbumVol;
	float           m_avDesiredVolume;
	float           m_avMaxGain;
	float           m_avCalculatedGain;

	// crossfading etc.
#define         SJ_DEF_AUTO_CROSSFADE_ENABLED   true
	bool            m_autoCrossfade;
	bool            m_autoCrossfadeSubseqDetect;
	bool            m_skipSilence;
	bool            m_onlyFadeOut;

	// Misc
	bool            m_isInitialized;
	bool            m_paused;           // as we use sliding to/from pause, BASS_IsPaused() does not always reflect the correct state
	bool            m_inOpening;
	void*           m_visBuffer;
	long            m_visBufferBytes;
	wxArrayString   m_failedUrls;
};


// internal messages
#define THREAD_PREPARE_NEXT         (IDPLAYER_FIRST+0)
#define THREAD_OUT_OF_DATA          (IDPLAYER_FIRST+1)
#define THREAD_DELETE_STREAM        (IDPLAYER_FIRST+2)
#define THREAD_RECALL_TRASH         (IDPLAYER_FIRST+5)
#define THREAD_NEW_TRACK_ON_AIR     (IDPLAYER_FIRST+6)
#define THREAD_NEW_META_DATA        (IDPLAYER_FIRST+7)


#endif // __SJ_PLAYER_H__
