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
	// SjPlayerModule is needed for the configuration only
	                SjPlayerModule      (SjInterfaceBase*);
	void            GetLittleOptions    (SjArrayLittleOption&);
};


class SjBackend;
class SjBackendStream;
class SjBackendCallbackParam;
class SjVolumeCalc;


class SjPlayer
{
public:
	// Constructor/Destructor, Init/Exit, Load/Save settings.
	// for complete contruction call SjPlayer() _and_ Init().
	                SjPlayer            ();
	void            Init                ();
	void            Exit                ();
	void            SaveSettings        () const;
	SjQueue         m_queue;

	// resume
	void            SaveToResumeFile    ();
	void            LoadFromResumeFile  ();
	wxString        GetResumeFile       () const;

	// Basic Player Control.
	// The playback is NOT started implicit by Goto() or Enqueue() functions.
	// To check if the operations were successfull, use the Is*()/Get*() functions.
	void            Play                (long ms=0);
	void            Pause               ();
	void            PlayOrPause         ()                                  { if(IsPlaying()) {Pause();} else {Play(0);} }
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
	bool            IsVideoOnAir        (); // has the running title a video stream?
	bool            AreTheseUrlsCurrentlyPrelistening(const wxArrayString&) { return false; }
	void            TogglePrelisten     (const wxArrayString& urls) {}

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
	void            GotoPrev            ()                      { long p=m_queue.GetPrevPos(0); if(p!=-1) { GotoAbsPos(p); } }
	bool            HasNextIgnoreAP     ()                      { return m_queue.GetNextPos(SJ_PREVNEXT_LOOKUP_ONLY)!=-1; }
	bool            GotoNextIgnoreAP    (bool fadeToNext)       { long p=m_queue.GetNextPos(0); if(p!=-1) { GotoAbsPos(p, fadeToNext); return true; } else { return false; } }
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

	// Crossfade & Other fadings: Time settings
	#define         SJ_DEF_CROSSFADE_MS 10000L
	long            m_autoCrossfadeMs;
	#define         SJ_DEF_CROSSFADE_OFFSET_END_MS 3000L
	long            m_crossfadeOffsetEndMs;
	long            m_manCrossfadeMs;

	void            OneSecondTimer      ();

	// The IDP_* messages posted to the main module should
	// be given to ReceiveSignal().  This is to avoid using a separate
	// thread for this purpose.
	void            ReceiveSignal       (int id, uintptr_t extraLong);

private:
	// The player's backend, normally selected by a define as SJ_USE_GSTREAMER or SJ_USE_XINE
	SjBackend*       m_backend;
	SjBackendStream* m_streamA;
	SjBackendStream* CreateStream       (const wxString& url, long seekMs, long fadeMs);
	void             DeleteStream       (SjBackendStream**, long fadeMs); // the given pointer must not be used by the caller after using this function!

	#define          SJ_SYSVOL_DONTUSE  0
	#define          SJ_SYSVOL_USE      1
	#define          SJ_SYSVOL_ONLYINIT 2
	#define          SJ_SYSVOL_DEFAULT  SJ_SYSVOL_USE
	long             m_useSysVol;

	// prelisten
	#define          SJ_PL_MIX       1
	#define          SJ_PL_LEFT      2
	#define          SJ_PL_RIGHT     3
	#define          SJ_PL_OWNOUTPUT 4
	#define          SJ_PL_DEFAULT   SJ_PL_MIX
	long             m_prelistenDest;
	SjBackend*       m_prelistenBackend;
	long             m_prelistenUseSysVol;

	// the trash
	wxArrayPtrVoid   m_trashedStreams;

	// tools
	void            SendSignalToMainThread(int id, uintptr_t extraLong=0) const;
	void            SaveGatheredInfo    (const wxString& url, unsigned long startingTime, SjVolumeCalc*, long realDecodedBytes);

	// main volume stuff
	int             m_mainVol;
	double          m_mainGain; // 0.0 - 1.0

	int             m_mainBackupVol;
	bool            m_stopAfterThisTrack;
	bool            m_stopAfterEachTrack;



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
	bool            m_onlyFadeOut;

	// Misc
	bool            m_isInitialized;
	bool            m_paused;           // as we use sliding to/from pause, BASS_IsPaused() does not always reflect the correct state
	bool            m_inOpening;
	void*           m_visBuffer;
	long            m_visBufferBytes;
	wxArrayString   m_failedUrls;

	friend class    SjPlayerModule;
	friend void     SjPlayer_BackendCallback(SjBackendCallbackParam*);
};


#endif // __SJ_PLAYER_H__
