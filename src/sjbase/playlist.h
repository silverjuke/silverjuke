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
 * File:    playlist.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke playlist handling
 *
 ******************************************************************************/



#ifndef __SJ_PLAYLIST_H__
#define __SJ_PLAYLIST_H__



class SjPlaylist;



class SjPlaylistAddInfo
{
public:
	// SjPlaylistAddInfo is used for add. information about a
	// SjPlaylistEntry object.  For performance reasons in huge playlists,
	// the data are loaded "just in time".
	SjPlaylistAddInfo   ()
	{
		m_what              = 0;
		m_playtimeMs        = -1;
		m_playCount         = 0;
		m_flags             = 0;
	}

	// what add. information are set
	// misc = track, artist, album and playtime
#define         SJ_ADDINFO_PLAYCOUNT            0x04
#define         SJ_ADDINFO_MISC                 0x08
	long            m_what;

	// the add. information
	wxString        m_trackName;
	wxString        m_leadArtistName;
	wxString        m_albumName;
	long            m_playtimeMs;           // -1 for unknown
	long            m_playCount;

#define         SJ_PLAYLISTENTRY_ERRONEOUS  0x01
#define         SJ_PLAYLISTENTRY_AUTOPLAY   0x02
#define         SJ_PLAYLISTENTRY_PLAYNEXT   0x04 // mainly for the queue
#define         SJ_PLAYLISTENTRY_MOVED_DOWN 0x20 // delayed due to avoid boredom
	long            m_flags;
};



class SjPlaylistEntry
{
public:
	// constructor / destructor
	SjPlaylistEntry     (SjPlaylist* playlist, const wxString& url, bool verified, long flags)
	{
		m_playlist      = playlist,
		m_url           = url;
		m_urlVerified   = verified;
		m_urlOk         = verified;
		m_addInfo       = NULL;
		m_id            = s_nextId++;
		if( flags )     SetFlags(flags);
	}

	~SjPlaylistEntry    () { if(m_addInfo) { delete m_addInfo; } }

	// get various URL information
	bool            IsUrlOk             () { if(!m_urlVerified) { VerifyUrl(); } return m_urlOk; }
	wxString        GetUrl              () { if(!m_urlVerified) { VerifyUrl(); } return m_url; }
	wxString        GetUnverifiedUrl    () { return m_url; }
	void            RenameUrl           (const wxString& oldUrl, const wxString& newUrl) { if(m_url==oldUrl) m_url=newUrl; }
	void            UrlChanged          () { if(m_addInfo) { delete m_addInfo; m_addInfo=NULL; } }
	wxString        GetLocalUrl         (const wxString& containerUrl);

	// get the ID, the ID is unique even for different URLs that are several times in the playlist
	// and they stay equal if the playlist is modified
	long            GetId               () const { return m_id; }

	// get/set the playcount
	long            GetPlayCount        () const { return m_addInfo? m_addInfo->m_playCount : 0; }
	void            SetPlayCount        (long cnt) { CheckAddInfo(SJ_ADDINFO_PLAYCOUNT); m_addInfo->m_playCount = cnt; }

	// get/set the flags
	long            GetFlags            () const { return m_addInfo? m_addInfo->m_flags : 0; }
	void            SetFlags            (long f) { CheckAddInfo(SJ_ADDINFO_PLAYCOUNT); m_addInfo->m_flags = f; }
	void            SetFlag             (long f) { SetFlags(GetFlags()|f); }

	// get information about the track
	wxString        GetLeadArtistName   () { CheckAddInfo(SJ_ADDINFO_MISC); return m_addInfo->m_leadArtistName; }
	wxString        GetTrackName        () { CheckAddInfo(SJ_ADDINFO_MISC); return m_addInfo->m_trackName; }
	wxString        GetAlbumName        () { CheckAddInfo(SJ_ADDINFO_MISC); return m_addInfo->m_albumName; }
	long            GetPlaytimeMs       () { CheckAddInfo(SJ_ADDINFO_MISC); return m_addInfo->m_playtimeMs; }

	// update some information
	void            SetPlaytimeMs       (long ms) { CheckAddInfo(SJ_ADDINFO_MISC); if(ms>0)m_addInfo->m_playtimeMs=ms; }
	void            SetRealtimeInfo     (const wxString& info);

	// make sure, the operator = is NOT used,
	// always use references for speed reasons instead
#ifdef __WXDEBUG__
	SjPlaylistEntry&  operator =            (const SjPlaylistEntry& o) { wxASSERT(0); return *this; }
#endif

private:
	// private stuff
	SjPlaylist*     m_playlist;
	wxString        m_url;
	bool            m_urlVerified;
	bool            m_urlOk;

	// For synchronizing eg. with the ringbuffer,
	// we need unique Queue IDs that
	// - do not change if the playlist is modified.
	// - differ if the same url is added twice
	long            m_id;
	static long     s_nextId;

	// additional information are loaded as needed
	SjPlaylistAddInfo*
	m_addInfo;
	void            CheckAddInfo        (long what) { if(m_addInfo==NULL||!(m_addInfo->m_what&what)) { LoadAddInfo(what); } }
	void            LoadAddInfo         (long what);
	void            VerifyUrl           ();
};

WX_DECLARE_OBJARRAY(SjPlaylistEntry, SjArrayPlaylistEntry);



class SjPlaylist
{
public:
	SjPlaylist          () { m_cacheFlags=0; }

	// clear playlist
	void            Clear               () { m_cacheFlags=0; m_array.Clear(); m_urlCounts.Clear(); };

	// adding URLs to playlist
	void            Add                 (const wxArrayString& urls, bool urlsVerified);
	void            Add                 (const wxString& url, bool urlVerified, long flags)
	{
		m_cacheFlags=0;
		m_array.Add(new SjPlaylistEntry(this, url, urlVerified, flags));
		m_urlCounts.Insert(url, m_urlCounts.Lookup(url)+1);
	}
	void            Insert              (const wxString& url, long addBeforeThisIndex, bool urlVerified, long flags)
	{
		m_cacheFlags=0;
		m_array.Insert(new SjPlaylistEntry(this, url, urlVerified, flags), addBeforeThisIndex);
		m_urlCounts.Insert(url, m_urlCounts.Lookup(url)+1);
	}

	// Update some information, urlVerified should normally be TRUE as
	// this function is normally called after an continious playback.
	void            UpdateUrl           (const wxString& url, bool urlVerified, long playtimeMs);

	// removing URLs from playlist
	// RemoveAt() returns the number of the same URLs still in the playlist
	long            RemoveAt            (long index);
	void            Remove              (const wxArrayString&);

	// search a given URL and return the first match
	long            GetPosByUrl         (const wxString& url) const;
	long            GetPosById          (long id) const;

	// getting playlist information
	long             GetCount           () const { return m_array.GetCount(); }
	SjPlaylistEntry& Item               (size_t index) const { return m_array.Item(index); }
	SjPlaylistEntry& operator[]         (size_t index) const { return m_array.Item(index); }

	bool            IsInPlaylist        (const wxString& url) const { return m_urlCounts.Lookup(url)!=0; }
	long            GetCountInPlaylist  (const wxString& url) const { return m_urlCounts.Lookup(url); }

	// get the number of unplayed titles; if you just want to
	// check for a given border, you can set a border at which counting is aborted.
	long            GetUnplayedCount    (long currPos=-1, long maxCnt=-1) const;

	// import / export
#define         SJ_M3U_NO_EXT       0x01
#define         SJ_CUE_SHORTPATHS   0x02
	bool            AddFromFileDlg      (wxWindow* parent);
	bool            AddFromFile         (const wxString& path, long addMax=0, long flags=0);
	bool            AddFromFile         (wxFSFile*, long addMax=0, long flags=0);

	bool            SaveAsDlg           (wxWindow* parent);
	bool            SaveAsFile          (const wxString& path, const wxString& type, long flags=0);
	wxString        SaveAsM3u           (const wxString& containerUrl, long flags=0);
	wxString        SaveAsPls           (const wxString& containerUrl, long flags=0);
	wxString        SaveAsCue           (const wxString& containerUrl, long flags=0);
	wxString        SaveAsXspf          (const wxString& containerUrl, long flags=0);

	// handling meta data, on merging, the meta data of the
	// given playlist have a higher priority.
	void            MergeMetaData       (const SjPlaylist&);

	wxString        GetLeadArtistName   () { LoadOverallNames(); return m_overallLeadArtistName; }
	wxString        GetAlbumName        () { LoadOverallNames(); return m_overallAlbumName; }

	wxString        SuggestPlaylistName ();
	wxString        SuggestPlaylistFileName
	();

	// Handling relative paths and container URLs.
	// Container URLs are URLs of files referencing tracks such as *.m3u files.
	void            AddContainerUrl     (const wxString& b) { if(m_containerUrls.Index(b)==wxNOT_FOUND) { m_containerUrls.Add(b); } }
	wxArrayString   GetContainerUrls    () const { return m_containerUrls; }
	void            RehashUrl           (const wxString& oldUrl, const wxString& newUrl);

	// OnUrlChanged() checks if the old url is in the playlist. If so,
	// all references are modified to use the new url.
	void            OnUrlChanged        (const wxString& oldUrl, const wxString& newUrl);

	// Move
	void            MovePos             (long srcPos, long destPos);

private:
	// The playlist data - we hold the data as an sorted array
	// and as a hash for fast lookup a specific URL
	SjArrayPlaylistEntry
	m_array;
	SjSLHash        m_urlCounts;

	// meta data
	wxString        m_playlistName;
	wxString        m_playlistUrl;

	// cache and overall names
#define         SJ_CACHE_OVERALL_LOADED     0x01
#define         SJ_CACHE_LEAD_ARTIST_FINE   0x02
#define         SJ_CACHE_ALBUM_FINE         0x04
	int             m_cacheFlags;
	wxString        m_overallLeadArtistName;
	wxString        m_overallAlbumName;
	void            LoadOverallNames    ();

	// import / export
	bool            AddFromM3u              (wxFSFile*, long addMax, long flags);
	bool            AddFromPls              (wxFSFile*, long addMax, long flags);
	bool            AddFromCue              (wxFSFile*, long addMax, long flags);
	bool            AddFromXspfOrXmlOrWpl   (wxFSFile*, long addMax, long flags);

	// container urls
	wxArrayString   m_containerUrls;
};



#endif // __SJ_PLAYLIST_H__
