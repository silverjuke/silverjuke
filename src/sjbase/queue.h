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
 * File:    queue.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke player
 *
 ******************************************************************************/


#ifndef __SJ_QUEUE_H__
#define __SJ_QUEUE_H__


enum SjRepeat
{
    SJ_REPEAT_OFF = 0,
	SJ_REPEAT_SINGLE = 1,
    SJ_REPEAT_ALL = 2
};


class SjPlayer;


class SjQueue
{
public:
	                SjQueue             ();
	void            Init                ()  { m_isInitialized = true; }
	void            Exit                ()  { m_isInitialized = false; }

	// enqueue / unqueue.  The player object for unqueing is needed to stop the
	// playback if there is nothing left in the queue.
	long            Enqueue             (const wxArrayString& urls, long addBeforeThisPos, bool verified, SjLLHash* addedIds, long flags);
	long            UnqueueByPos        (long pos, SjPlayer*, int* replayRet=NULL); // returns the number of identical URLs still in the queue at other positions
	void            UnqueueByUrl        (const wxString& url, SjPlayer*, int* replayRet=NULL);
	void            UnqueueByUrls       (const wxArrayString& urls, SjPlayer*);
	void            UnqueueByIds        (const SjLLHash& ids, long removeVal, SjPlayer* player);
	void            UnqueueAll          (SjPlayer*, bool stopVisIfPlaying);
	void            UnqueuePlayed       ();

	// Update some information, urlVerified should normally be TRUE as
	// this function is normally called after an continious playback.
	void            UpdateUrl           (const wxString& url, bool urlVerified, long playtimeMs) { m_playlist.UpdateUrl(url, urlVerified, playtimeMs); }

	// queue information
	long            GetCount            () const                    { return m_playlist.GetCount(); }
	long            GetWaitingCount     (long maxCnt=-1) const      { return m_playlist.GetUnplayedCount((m_shuffle||m_repeat==SJ_REPEAT_ALL)? -1/*count all*/ : m_pos/*count from current position*/, maxCnt); }
	long            GetClosestPosByUrl  (const wxString& url) const;
	long            GetAllPosByUrl      (const wxString& url, wxArrayLong& ret, bool unplayedOnly=false) const; // returns the count
	long            GetPosById          (long id) const             { return m_playlist.GetPosById(id); }
	wxArrayLong     GetPosByIds         (const SjLLHash&) const;
	long            GetIdByPos          (long pos) const            { return m_playlist.Item(pos).GetId(); }
	long            GetIdByCurrPos      () const                    { return m_pos<0? -1 : m_playlist.Item(m_pos).GetId(); }
	wxString        GetUrlByPos         (long pos) const;
	wxString        GetUrlById          (long id) const;
	wxArrayString   GetUrlsByIds        (const SjLLHash&) const;
	wxArrayString   GetUrls             () const;
	bool            WasPlayed           (long pos) const            { return m_playlist.Item(pos).GetPlayCount()>0; }
	long            GetPlayCount        (long pos) const            { return m_playlist.Item(pos).GetPlayCount(); }
	void            ResetPlayCount      (long pos)                  { m_playlist.Item(pos).SetPlayCount(0); CleanupNextShufflePos(); }
	long            GetFlags            (long pos) const            { return m_playlist.Item(pos).GetFlags(); }
	void            SetFlags            (long pos, long flags)      { m_playlist.Item(pos).SetFlags(flags); }
	void            SetCurrErroneous    ();
	SjPlaylistEntry& GetInfo            (long pos/*-1 for current*/);
	bool            IsEnqueued          (const wxString& url) const { return m_playlist.IsInPlaylist(url); }
	bool            IsPlaying           (const wxString& url) const { return m_pos>=0? (m_playlist.Item(m_pos).GetUrl()==url) : FALSE; }
	bool            MoveToTopOnEoq      () const;

	// the current queue position
	long            GetCurrPos          () const   { return m_pos; }
	void            SetCurrPos          (long pos);
	void            IncRepeatRound      () { m_repeatRound++; } // should be called after playback stop at end of playlist to allow the user manually restarting the playlist
	long            GetRepeatRound      () const { return m_repeatRound; }
	void            EqualizeRepeatRound (); // should be called if auto-play tracks are added

	// getting the previous / next queue positions
	#define         SJ_PREVNEXT_REGARD_REPEAT   0x01
	#define         SJ_PREVNEXT_LOOKUP_ONLY     0x02
	#define         SJ_PREVNEXT_INIT            0x04
	long            GetPrevPos          (int flags);
	long            GetNextPos          (int flags);

	// shuffle - the intensity is a percentage between 2 and 100
	#define         SJ_DEF_SHUFFLE_STATE FALSE
	#define         SJ_DEF_SHUFFLE_INTENSITY 50L
	void            SetShuffle          (bool s) { m_shuffle = s; CleanupNextShufflePos(); }
	void            ToggleShuffle       ()       { SetShuffle(!GetShuffle()); }
	bool            GetShuffle          () const { return m_shuffle; }
	void            SetShuffleIntensity (int i)  { m_shuffleIntensity = i; CleanupNextShufflePos(); }
	int             GetShuffleIntensity () const { return m_shuffleIntensity; }

	// repeat
	void            SetRepeat           (SjRepeat r)  { if(r<SJ_REPEAT_OFF||r>SJ_REPEAT_ALL)r=SJ_REPEAT_OFF;  m_repeat = r; CleanupNextShufflePos(); };
	void            ToggleRepeat        ();
	SjRepeat        GetRepeat           () const { return m_repeat; }

	// queue flags / boredom
	#define         SJ_QUEUEF_BOREDOM_TRACKS        0x0001L
	#define         SJ_QUEUEF_BOREDOM_ARTISTS       0x0002L
	#define         SJ_QUEUEF_REMOVE_PLAYED         0x0004L
	#define         SJ_QUEUEF_RESUME                0x0010L
	#define         SJ_QUEUEF_RESUME_LOAD_PLAYED    0x0020L
	#define         SJ_QUEUEF_RESUME_START_PLAYBACK 0x0040L
	#define         SJ_QUEUEF_DEFAULT               0x0000L
	#define         SJ_DEF_BOREDOM_TRACK_MINUTES    30L
	#define         SJ_DEF_BOREDOM_ARTIST_MINUTES   20L
	void            SetQueueFlags       (long flags, long t, long a)
	{
		m_queueFlags = flags;
		m_boredomTrackMinutes = t;
		m_boredomArtistMinutes = a;
		CleanupNextShufflePos();
	}

	long            GetQueueFlags       () const
	{
		return m_queueFlags;
	}
	void            GetQueueFlags       (long& flags, long& t, long& a) const
	{
		flags = m_queueFlags;
		t = m_boredomTrackMinutes;
		a = m_boredomArtistMinutes;
	}

	// import / export
	void            SaveAsDlg           (wxWindow* parent) { m_playlist.SaveAsDlg(parent); }
	void            MergeMetaData       (const SjPlaylist& p) { m_playlist.MergeMetaData(p); }

	void            OnUrlChanged        (const wxString& oldUrl, const wxString& newUrl) { m_playlist.OnUrlChanged(oldUrl, newUrl); }

	// move tracks
	long            MoveByIds           (const SjLLHash& idsToMove, long motionAmount);

	// find out if a track is boring
	bool            IsBoring            (const wxString& artistName, const wxString& trackName, unsigned long currTimestamp) const;
	bool            IsBoring            (long pos, unsigned long currTimestamp) const
	{
		SjPlaylistEntry& entry = m_playlist.Item(pos);
		return IsBoring(entry.GetLeadArtistName(), entry.GetTrackName(), currTimestamp);
	}

private:
	bool            m_isInitialized;

	// The queue - SjPlaylist holds the data as an sorted array and as a hash
	// for fast lookup a specific URL
	SjPlaylist      m_playlist;

	// the history holds the played IDs, needed esp. for going back in shuffle mode;
	// however, AddToHistory() and PopFromHistory() use the playlist positions for communication with
	// the caller
	void            AddToHistory        (long pos);
	long            PopFromHistory      (int flags);
	wxArrayLong     m_historyIds;
	SjSLHash        m_historyArtists;
	SjSLHash        m_historyTracks;

	// calculated shuffle positions
	long            m_nextShufflePos;
	long            m_nextShufflePosFor;
	bool            m_nextShuffleIncRepeatRound;

	long            GetNextShufflePos(int flags, unsigned long currTimestamp);
	long            GetNextShufflePos_GetPossibleTrack(bool regardBoredom, long repeatRound, unsigned long currTimestamp);

	void            CleanupNextShufflePos()
	{ m_nextShufflePos=-1; m_nextShufflePosFor=-2;/*-1 is okay*/ m_nextShuffleIncRepeatRound=FALSE; }

	// the current queue position, -1 = nothing in queue
	long            m_pos;

	// shuffle / repeat
	bool            m_shuffle;
	int             m_shuffleIntensity;
	SjRepeat        m_repeat;
	long            m_repeatRound;

	// enqueueing
	long            EnqueueDo__         (const wxString& url, long addBeforeThisPos, bool verified, SjLLHash* addedIds, long flags);
	void            EnqueueFinish__     (long oldPos);
	void            UnqueueReplay__     (SjPlayer* player, int replayRet);
	long            m_playNextId;

	// queue flags / avoid boredom
	long            m_queueFlags;
	int             m_boredomTrackMinutes, m_boredomArtistMinutes;

	// Misc.
	SjPlaylistEntry m_dummyPlaylistInfo;
};



#endif // __SJ_QUEUE_H__

