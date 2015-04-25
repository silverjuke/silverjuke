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
 * File:    library.h
 * Authors: Björn Petersen
 * Purpose: The "Album generator" module
 *
 ******************************************************************************/


#ifndef __SJ_LIBRARY_H__
#define __SJ_LIBRARY_H__


enum SjLibrarySort
{
	SJ_LIBSORT_ARTIST_YEAR_ALBUM = 0,
	SJ_LIBSORT_ARTIST_ALBUM_YEAR = 1,
	SJ_LIBSORT_ALBUM_YEAR_ARTIST = 2,
	SJ_LIBSORT_ALBUM_ARTIST_YEAR = 3,
	SJ_LIBSORT_COUNT
};


class SjIdCollector
{
public:
	void            Clear       () { m_ids.Clear(); }
	void            Add         (long id) { m_ids.Append(wxString::Format(wxT("%i,"), (int)id)); }
	wxString        GetAsString () { m_ids.Truncate(m_ids.Len()-1); return m_ids; }

private:
	wxString        m_ids;
};


// do not change the flag values are they're saved to disk!
#define SJ_LIB_SHOWDISKNR             0x00000001L
#define SJ_LIB_SHOWTRACKNR            0x00000002L
#define SJ_LIB_AUTOMTRACKNR           0x00000004L
#define SJ_LIB_SHOWDIFFLEADARTISTNAME 0x00000008L
#define SJ_LIB_SHOWORGARTISTNAME      0x00000010L
#define SJ_LIB_SHOWCOMPOSERNAME       0x00000020L
#define SJ_LIB_SHOWTIME               0x00000040L
#define SJ_LIB_SHOWYEAR               0x00000080L
#define SJ_LIB_OMITTOEND              0x00000100L
#define SJ_LIB_CREATEALBUMSBY_GENRE   0x00000200L
#define SJ_LIB_SHOWALBUMNAME          0x00000400L
#define SJ_LIB_SHOWLEADARTISTNAME     0x00000800L
#define SJ_LIB_SHOWDOUBLETRACKS       0x00100000L
#define SJ_LIB_SHOWDIFFALBUMNAME      0x00200000L
#define SJ_LIB_PLAYONCOVERDBLCLICK    0x00400000L
#define SJ_LIB_SHOWCOMMENT            0x00800000L
#define SJ_LIB_CREATEALBUMSBY_DIR     0x01000000L
#define SJ_LIB_SHOWGENRE              0x02000000L
#define SJ_LIB_SHOWRATING             0x04000000L
#define SJ_LIB_DEFAULT                0x000FFFFFL
#define SJ_LIB_CREATEALBUMSBY_MASK    (SJ_LIB_CREATEALBUMSBY_GENRE|SJ_LIB_CREATEALBUMSBY_DIR)

#define SJ_SHORTENED_TRACKNAME_LEN  16 // defaults for ShortenName()
#define SJ_SHORTENED_ARTISTNAME_LEN 24


class SjLibraryModule : public SjColModule
{
public:
	SjLibraryModule     (SjInterfaceBase*);

	SjSearchStat    SetSearch           (const SjSearch&, bool deepSearch);

	void            GetIdsInView        (SjLLHash* ret, bool ignoreSimpleSearchIfNull=FALSE, bool ignoreAdvSearchIfNull=FALSE);

	SjEmbedTo       EmbedTo             () { return SJ_EMBED_TO_MUSICLIB; }
	wxWindow*       GetConfigPage       (wxWindow* parent, int selectedPage);
	void            DoneConfigPage      (wxWindow* configPage, int action);
	void            GetLittleOptions    (SjArrayLittleOption&);

	long            GetUnmaskedTrackCount();

	long            GetUnmaskedColCount ();
	long            GetMaskedColCount   ();
	long            GetMaskedColIndexByAz(int targetId);
	long            GetMaskedColIndexByColUrl(const wxString& colUrl);
	wxString        GetUrl              (const wxString& leadArtistName, const wxString& albumName, const wxString& trackName);
	wxString        GetUrl              (long id);
	long            GetId               (const wxString& colUrl);

	SjCol*          GetMaskedCol        (long index) { return GetCol__(HasSearch()? m_searchOffsets[index] : index, index, TRUE/*regardSearch*/); }
	SjCol*          GetUnmaskedCol      (long index) { return GetCol__(index, index, FALSE/*regardSearch*/); }
	SjCol*          GetMaskedCol        (const wxString& trackUrl, long& retIndex /*-1 if currently hidden eg. by search*/);

	bool            UpdateAllCol        (wxWindow* parent, bool deepUpdate);

	// List view depending stuff
	SjListView*     CreateListView      (long order, bool desc, long minAlbumRows);

	// selection handling
	void            SelectAllRows       (bool select);
	long            GetSelectedUrlCount () { return m_selectedTrackIds.GetCount(); }
	void            GetSelectedUrls     (wxArrayString& urls);
	void            GetOrderedUrlsFromIDs
	(SjLLHash& ids, wxArrayString& urls);
	long            DelInsSelection     (bool del);

	// misc.
	wxArrayString   GetUniqueValues     (long what);

	bool            GetAutoComplete     (long what, const wxString& in, wxString& out, bool internalCall = FALSE);
	void            DoAutoComplete      (long what, int& oldTypedLen, wxWindow*);


	bool            GetTrackInfo        (const wxString& url, SjTrackInfo&, long flags, bool logErrors);
	void            PlaybackDone        (const wxString& url, unsigned long startingTime, double newGain, long realDecodedBytes);
	void            GetAutoVol          (const wxString& url, double* trackGain, double* albumGain) const; // set to < 0 if unknown
	double          GetAutoVol          (const wxString& url, bool useAlbumGainIfPossible);
	long            GetAutoVolCount     ();
	bool            AreTracksSubsequent (const wxString& url1, const wxString& url2);

	// Get more tracks from an artist or album,
	// targetId is one of IDT_MORE_FROM_CURR_ALBUM or IDT_MORE_FROM_CURR_ARTIST
	// if alreadyEnqueued is set, URLs already in the given queue are not returned.
	wxArrayString   GetMoreFrom         (const wxString& url, int targetId, SjQueue* alreadyEnqueued);

	// my first idea was not to save the autovol and the playcount directly, but
	// flush it sometime using SavePendingData(). however, PlaybackDone() seems to
	// be fast enough (we call it at the end of a track); so we save many certain
	// circumstances where we would check if there are data pending... in short,
	// currently SavePendingData() is not needed but called at places where it
	// may be needed.
	void            SavePendingData     () {}

	// add an art image to use as cover to an album
#define         SJ_DUMMY_COVER_ID   0x7FFFFFFFL
	void            GetPossibleAlbumArts(long albumId, wxArrayLong& albumArtIds, wxArrayString* albumArtUrls, bool addAutoCover);
	bool            AddArt__            (SjDataObject* data, long albumId, bool ask);

	// adding folders directly by drag'n'drop
	bool            AddFolders          (const wxArrayString& folders);

	// rating/artist info handling
	void            CreateMenu          (SjMenu* enqueueMenu, SjMenu* editMenu, bool createMainMenu=FALSE);
	void            UpdateMenuBar       ();
	void            CreateRatingMenu    (SjMenu&, int baseId, const wxArrayString& urls, SjIcon icon=SJ_ICON_EMPTY);
	void            CreateRatingMenu    (SjMenu&, int baseId, SjIcon icon=SJ_ICON_EMPTY);
	void            UpdateRatingMenu    (SjMenu&, int baseId, long rating, long trackCount);

	bool            SetRating           (const wxArrayString& urls, long rating);

	void            CreateArtistInfoMenu(SjMenu&, int baseId);
	void            UpdateArtistInfoMenu(SjMenu&, int baseId);
	void            CreateArtistInfoMenu(SjMenu&, int baseId, const wxArrayString& urls);
	void            ShowArtistInfo      (int baseId, int clickedId);

	static wxString ShortenName         (const wxString&, int maxLen);

	// the vertical text shown normally aleft, for the album view,
	// you should use m_textup
	wxString        GetUpText();

	// get omitting information, used by ::SjNormaliseString()
	const SjOmitWords* GetOmitArtist   () const {return &m_omitArtist;}
	const SjOmitWords* GetOmitAlbum    () const {return &m_omitAlbum;}

	// returns the number of titles used for a "normal" album -
	// some albums, however, may have eg. no album name and/or no artist name.
	long            DefTitleCount       () const { long r=0; if(m_flags&SJ_LIB_SHOWLEADARTISTNAME)r++; if(m_flags&SJ_LIB_SHOWALBUMNAME)r++; return r; }

	// misc.
	bool            PlayOnDblClick      () const { return (m_flags&SJ_LIB_PLAYONCOVERDBLCLICK) != 0; }

protected:
	bool            FirstLoad           ();
	void            LastUnload          ();

private:
	// search stuff
	long*           m_searchOffsets;
	long            m_searchOffsetsMax;   // if m_searchOffsets is set, this is typically the number of phys. albums
	long            m_searchOffsetsCount; // -1 indicated "no search", use HasSearch() for testing
	SjLLHash        m_searchTracksHash;
	SjSearch        m_search;
	bool            HasSearch           () {return m_searchOffsetsCount==-1? FALSE : TRUE;}
	bool            IsInSearch          (long trackId) {return m_searchOffsetsCount==-1? TRUE : (m_searchTracksHash.Lookup(trackId)!=0); }
	bool            ModifySearch        (int keyCode, bool modifiersPressed);
	bool            HiliteSearchWords   (wxString&);
	SjCol*          GetCol__            (long dbAlbumIndex, long virtualAlbumIndex, bool regardSearch);

	// filter stuff
	SjLLHash        m_filterHash;
	long            m_filterAzFirst[27]; // a, b, c, ... z, 0-9 -> log. offsets
	bool            m_filterAzFirstHidden;
	wxString        m_filterCond;

	// selection stuff
	void            SelectByQuery       (bool select, const wxString& formattedQuery);
	void            AskToAddSelection   ();
	SjLLHash        m_selectedTrackIds;

	// remembered values - use eg. GetUnmaskedTrackCount() and GetMaskedColCount() instead
	long            m_rememberedUnmaskedTrackCount;
	long            m_rememberedUnmaskedColCount;
	void            ForgetRememberedValues() { m_rememberedUnmaskedTrackCount=-1; m_rememberedUnmaskedColCount=-1; }

	// other
	SjOmitWords     m_omitArtist;
	SjOmitWords     m_omitAlbum;

	long            m_flags;

	bool            m_deepUpdate;
	unsigned long   m_updateStartingTime; // the DOS timestamp the update process stated
	SjIdCollector   m_updatedTracks;

	SjCoverFinder   m_coverFinder;

	static wxString GetDummyCoverUrl    (long albumId);

	void            LoadSettings        ();
	void            SaveSettings        ();

	bool            Callback_MarkAsUpdated
	(const wxString& urlBegin, long checkTrackCount);
	bool            Callback_CheckTrackInfo
	(const wxString& url, unsigned long actualTimestamp);
	bool            Callback_ReceiveTrackInfo
	(SjTrackInfo*);

	bool            WriteTrackInfo      (SjTrackInfo*, long trackId, bool writeArtIds=TRUE);

	bool            CombineTracksToAlbums();
	bool            UpdateUniqueValues  (const wxString& name);

	SjLibrarySort   m_sort;
	long            m_n1, m_n2; /* see top of library.cpp */

	void            UpdateMenu          (SjMenu* enqueueMenu, SjMenu* editMenu, bool updateMainMenu=FALSE);
	void            HandleMenu          (int id);
	void            HandleMiddleMouse   ();

	wxString        m_lastLeadArtistName,
	                m_lastTrackName;
	long            m_autoVolCount;

	SjLLHash        m_addedArtIds;

	wxRegEx         m_hiliteRegEx;
	bool            m_hiliteRegExOk;
	wxString        m_hiliteRegExFor;

	friend class    SjLibraryConfigPage;
	friend class    SjLibraryEditDlg;
	friend class    SjAlbumCoverRow;
	friend class    SjAlbumArtistRow;
	friend class    SjAlbumAlbumRow;
	friend class    SjAlbumDiskRow;
	friend class    SjAlbumTrackRow;
	friend class    SjLittleLibBit;
	friend class    SjTagEditorDlg;
	friend class    SjUpdateAlbum;
	friend class    SjLibraryListView;
};


class SjAlbumCoverRow : public SjRow
{
public:
	SjAlbumCoverRow     (long albumId)
		: SjRow(SJ_RRTYPE_COVER)
/*m_isPlaylistCover(false)*/
	{ m_albumId = albumId; }
	void            CreateContextMenu   (SjMenu&);
	int             IsSelectable        () {return PlayOnDblClick()?1:0;}
	void            Select              (bool select);
	void            OnContextMenu       (int id);
	wxString        GetToolTip          (long& flags);
	void            OnDoubleClick       (bool modifiersPressed);
	int             UsesMiddleClick     () { return 1; }

	bool            OnMiddleClick       (bool modifiersPressed);
	bool            OnDropData          (SjDataObject*);

	/*
	void            IsPlaylistCover_    (bool state) {m_isPlaylistCover=state;}
	bool            IsPlaylistCover     () const  {return m_isPlaylistCover;}
	*/

private:
	long            m_albumId;
	/*
	bool            m_isPlaylistCover;
	*/
	SjLibraryModule* GetLibrary         () const { return (SjLibraryModule*)m_col->m_module; }
	bool            PlayOnDblClick      () const { return (GetLibrary()->m_flags & SJ_LIB_PLAYONCOVERDBLCLICK) != 0; }
};


class SjAlbumAlbumRow : public SjRow
{
public:
	SjAlbumAlbumRow     (int roughType, long albumId, const wxString& album)
		: SjRow(roughType)
	{ m_albumId = albumId; m_textm = album; }
	void            CreateContextMenu   (SjMenu&);
	int             IsSelectable        () {return 1;}
	void            Select              (bool select);
	void            OnContextMenu       (int id);
	wxString        GetToolTip          (long& flags);
	void            OnDoubleClick       (bool modifiersPressed);
	bool            OnDropData          (SjDataObject*);

	int             UsesMiddleClick     () { return 2; }
	bool            OnMiddleClick       (bool modifiersPressed);

private:
	long            m_albumId;
};


class SjAlbumArtistRow : public SjRow
{
public:
	SjAlbumArtistRow    (int roughType, long albumId, const wxString& artist)
		: SjRow(roughType)
	{ m_albumId = albumId; m_textm = artist; }
	void            CreateContextMenu   (SjMenu&);
	int             IsSelectable        () { return 1; }
	void            Select              (bool select);
	void            OnContextMenu       (int id);
	wxString        GetToolTip          (long& flags);
	void            OnDoubleClick       (bool modifiersPressed);
	bool            OnDropData          (SjDataObject*);

	int             UsesMiddleClick     () { return 2; }
	bool            OnMiddleClick       (bool modifiersPressed);

private:
	long            m_albumId;
};


class SjAlbumDiskRow : public SjRow
{
public:
	SjAlbumDiskRow      (long albumId, long diskNr)
		: SjRow(SJ_RRTYPE_TITLE3)
	{ m_albumId = albumId; m_diskNr = diskNr; }
	void            CreateContextMenu   (SjMenu&);
	int             IsSelectable        () {return 1;}
	void            Select              (bool select);
	void            OnContextMenu       (int id);
	wxString        GetToolTip          (long& flags);
	void            OnDoubleClick       (bool modifiersPressed);
	bool            OnDropData          (SjDataObject*);

	int             UsesMiddleClick     () { return 2; }
	bool            OnMiddleClick       (bool modifiersPressed);

private:
	long            m_albumId,
	                m_diskNr;
};


class SjAlbumTrackRow : public SjRow
{
public:
	SjAlbumTrackRow     (SjLibraryModule* libraryModule, long trackId, const wxString& url)
		: SjRow(SJ_RRTYPE_NORMAL)
	{ m_libraryModule = libraryModule; m_trackId = trackId; m_url = url; }
	void            CreateContextMenu   (SjMenu&);
	void            OnContextMenu       (int id);
	wxString        GetToolTip          (long& flags);
	void            OnDoubleClick       (bool modifiersPressed);
	bool            OnDropData          (SjDataObject*);

	int             IsSelectable        () {return 2;}
	bool            IsSelected          () { return m_libraryModule->m_selectedTrackIds.Lookup(m_trackId)!=0; }
	void            Select              (bool select);

	int             UsesMiddleClick     () { return 2; }
	bool            OnMiddleClick       (bool modifiersPressed);

private:
	long            m_trackId;
	SjLibraryModule*
	m_libraryModule;
};


#endif // __SJ_LIBRARY_H__
