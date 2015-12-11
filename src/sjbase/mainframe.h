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
 * File:    mainframe.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke main frame
 *
 ******************************************************************************/


#ifndef __SJ_MAINFRAME_H__
#define __SJ_MAINFRAME_H__


class SjColumnMixer;
class SjBrowserWindow;
class SjMainApp;
class SjLibraryModule;
class SjMainFrame;
class SjSee;


extern SjMainFrame* g_mainFrame;


#if SJ_USE_TOOLTIPS
class SjMainFrameToolTipProvider : public SjToolTipProvider
{
public:
	wxString        GetText             (long& flags);
	wxWindow*       GetWindow           () { return (wxWindow*)g_mainFrame; }
	wxRect          GetLocalRect        () { return m_rect; }

private:
	long            m_targetId;
	long            m_subitem;
	wxRect          m_rect;
	friend class    SjMainFrame;
};
#endif


class SjMainFrame : public SjSkinWindow
{
public:
	// Constructor and Destructor
	                    SjMainFrame         (SjMainApp*, int id, long skinFlags, const wxPoint& pos, const wxSize& size);
	                    ~SjMainFrame        (void);
	bool                InConstruction      () { return m_inConstruction; }
	long                GetUptimeSeconds    ();

	// Public pointers and objects
	SjModuleSystem  m_moduleSystem;
	SjLibraryModule* m_libraryModule;
	SjColumnMixer   m_columnMixer;
	SjBrowserWindow* m_browser;
	wxIconBundle    m_iconBundle;
	wxLocale        m_locale;

	// Public Player Functions
	void            Play                (long ms=0) { m_player.Play(ms); m_haltedManually=FALSE; UpdateDisplay(); }
	void            Pause               ()       { m_player.Pause(); m_haltedManually=TRUE; UpdateDisplay(); }
	void            Stop                () { m_player.Stop(); m_haltedManually=TRUE; UpdateDisplay(); }
	void            PlayOrPause         (bool fadeToPlayOrPause) { m_player.PlayOrPause(fadeToPlayOrPause); m_haltedManually=m_player.IsPaused(); UpdateDisplay(); }
	void            ReplayIfPlaying     ();
	void            SeekAbs             (long m) { m_player.SeekAbs(m); UpdateDisplay(); }
	void            SeekRel             (long m) { m_player.SeekRel(m); UpdateDisplay(); }
	void            Enqueue             (const wxString& url, long pos, bool verified, bool autoPlay=false, bool uiChecks=true) { wxArrayString a; a.Add(url); Enqueue(a, pos, verified, autoPlay, uiChecks); } /* pos=-1: enqueue last, pos=-2: play next, pos=-3: play now */
	void            Enqueue             (const wxArrayString&, long pos, bool verified, bool autoPlay=false, bool uiChecks=true);
	void            UnqueueAll          ()       { m_player.m_queue.UnqueueAll(&m_player, TRUE); m_display.m_scrollPos=-1; UpdateDisplay(); /*the caller should update the browser as he may enqueue new tracks just after the unqueue!*/ }
	void            UnqueueByPos        (long p);
	void            UpdateEnqueuedUrl   (const wxString& url, bool urlVerified, long playtimeMs) { m_player.m_queue.UpdateUrl(url, urlVerified, playtimeMs); }
	void            GotoAbsPos          (long p) { m_player.GotoAbsPos(p); m_display.m_scrollPos=-1; }
	void            GotoUrl             (const wxString& url) { m_player.GotoUrl(url); m_display.m_scrollPos=-1; if(!IsPlaying()) {UpdateDisplay();} }
	void            GotoPrev            (bool fadeToPrev) { m_player.GotoPrev(fadeToPrev); m_display.m_scrollPos=-1; if(!IsPlaying()) {UpdateDisplay();} }
	bool            GotoNextRegardAP    (bool fadeToNext, bool ignoreTimeouts=true);
	void            SetAbsMainVol       (long v);
	void            SetRelMainVol       (long v) { SetAbsMainVol(m_player.GetMainVol()+v); }
	long            GetMainVol          () const { return m_player.GetMainVol(); }
	void            SetShuffle          (bool v) { m_player.m_queue.SetShuffle(v); UpdateDisplay(); }
	void            ToggleShuffle       ()       { m_player.m_queue.ToggleShuffle(); UpdateDisplay(); }
	void            SetRepeat           (SjRepeat r) { m_player.m_queue.SetRepeat(r); UpdateDisplay(); }
	void            ToggleRepeat        ()       { m_player.m_queue.ToggleRepeat(); UpdateDisplay(); }
	void            StopAfterThisTrack  (bool s) { m_player.StopAfterThisTrack(s); UpdateDisplay(); }
	void            StopAfterEachTrack  (bool s) { m_player.StopAfterEachTrack(s); UpdateDisplay(); }
	void            ShowRemainingTime   (bool s) { m_showRemainingTime=s; UpdateDisplay(); }
	bool            IsStopped           ()    { return m_player.IsStopped(); }
	bool            IsPlaying           ()    { return m_player.IsPlaying(); }
	bool            IsPaused            ()    { return m_player.IsPaused(); }
	bool            IsEnqueued          (const wxString& url) const { return m_player.m_queue.IsEnqueued(url); }
	bool            IsAnyDialogOpened   ();
	bool            HasPrev             ()       { return m_player.HasPrev(); }
	bool            HasNextIgnoreAP     ()       { return m_player.HasNextIgnoreAP(); }
	bool            HasNextRegardAP     ()       { return ( HasNextIgnoreAP()  ||  (IsPlaying() && (m_autoCtrl.m_flags&SJ_AUTOCTRL_AUTOPLAY_ENABLED)) ); }
	wxString        GetNextMenuTitle    ();
	long            GetQueuePos         () const { return m_player.m_queue.GetCurrPos(); }
	long            GetClosestQueuePosByUrl (const wxString& url) const { return m_player.m_queue.GetClosestPosByUrl(url); } // wxNOT_FOUND for "not in queue"
	long            GetAllQueuePosByUrl(const wxString& url, wxArrayLong& ret, bool unplayedOnly=false) const{ return m_player.m_queue.GetAllPosByUrl(url, ret, unplayedOnly); } // returns the count, 0 for "not in queue"
	long            GetQueueCount       () const { return m_player.m_queue.GetCount(); }
	wxString        GetQueueUrl         (long p) const	{ return m_player.m_queue.GetUrlByPos(p); }
	SjPlaylistEntry& GetQueueInfo       (long p) { return m_player.m_queue.GetInfo(p); }
	long            GetElapsedTime      ()       { return m_player.GetElapsedTime(); }
	long            GetRemainingTime    ()       { return m_player.GetRemainingTime(); }
	long            GetTotalTime        ()       { return m_player.GetTotalTime(); }
	bool            GetShuffle          () const { return m_player.m_queue.GetShuffle(); }
	SjRepeat        GetRepeat           () const { return m_player.m_queue.GetRepeat(); }
	bool            StopAfterThisTrack  () const { return m_player.StopAfterThisTrack(); }
	bool            StopAfterEachTrack  () const { return m_player.StopAfterEachTrack(); }
	bool            ShowRemainingTime   () const { return m_showRemainingTime; }
	void            OnUrlChanged        (const wxString& n, const wxString& o) { m_player.m_queue.OnUrlChanged(n, o); }
	void            OnUrlChangingDone   () { UpdateDisplay(); }

	// open files - this function is called eg. on a double click
	// on an associated file or when the user drags a file onto the main frame.
	#define         SJ_OPENFILES_DEFCMD     0
	#define         SJ_OPENFILES_PLAY       1
	#define         SJ_OPENFILES_ENQUEUE    2
	void            OnIDO_DND_ONDATA    (wxCommandEvent&);
	bool            OpenFiles           (const wxArrayString&, int command=SJ_OPENFILES_DEFCMD, int x=0, int y=0);
	bool            OpenData            (SjDataObject*, int command=SJ_OPENFILES_DEFCMD, int x=0, int y=0);
	bool            DragNDrop           (SjDragNDropAction, wxWindow*, const wxPoint&, SjDataObject* data, wxArrayString* files);
	void            UpdateIndexAfterConstruction () { m_updateIndexAfterConstruction = TRUE; }
	void            SetPasteCoord       (const wxPoint& pt) { m_pasteCoord=pt; }

	// Public Search Functions
	#define         SJ_SETSEARCH_CLEARSIMPLE        0x01
	#define         SJ_SETSEARCH_SETSIMPLE          0x02
	#define         SJ_SETSEARCH_CLEARADV           0x04
	#define         SJ_SETSEARCH_SETADV             0x08
	#define         SJ_SETSEARCH_NOTEXTCTRLUPDATE   0x10
	#define         SJ_SETSEARCH_NOAUTOHISTORYADD   0x20
	void            SetSearch           (long flags, const wxString& newSimpleSearch=wxEmptyString, const SjAdvSearch* newAdvSearch=NULL);
	const SjSearch* GetSearch           () const { return &m_search; }
	const SjSearchStat* GetSearchStat   () const { return &m_searchStat; }
	void            EndOneSearch        () { SetSearch(m_search.m_simple.IsSet()? SJ_SETSEARCH_CLEARSIMPLE : SJ_SETSEARCH_CLEARADV); }
	void            EndAllSearch        () { SetSearch(SJ_SETSEARCH_CLEARADV|SJ_SETSEARCH_CLEARSIMPLE); }
	void            EndSimpleSearch     () { SetSearch(SJ_SETSEARCH_CLEARSIMPLE); }
	long            GetAvgSearchMs      () const { return m_allSearchMs/m_allSearchCount; } // m_allSearchCount is at least 1
	bool            HasAnySearch        () const { return m_search.IsSet(); }
	bool            HasSimpleSearch     () const { return m_search.m_simple.IsSet(); }
	void            UpdateSearchInfo    (long numTracksRemoved);

	// Zoom and Font Settings
	wxString        GetBaseFontFace     () const { return m_baseFontFace; }
	long            GetBaseFontSize     () const { return m_baseFontSize; }
	long            GetBaseColumnWidth  () const { return m_baseColumnWidth; }
	long            GetBaseCoverHeight  () const { return m_baseCoverHeight; }
	void            SetFontNCoverBase   (const wxString& fontFace, long fontSize, long columnWidth, long coverHeight);
	void            SetDefaultWindowSize();

	long            m_currColumnWidth,
	                m_currCoverWidth,
	                m_currCoverHeight;
	long            m_currColumnXSpace,
	                m_currColumnYSpace;
	long            m_currFontSize,
	                m_currFontSizeSmall;
	wxFont          m_baseStdFont; // m_baseStdFont should be used in dialogs etc. this is the same as m_currStdFont - without any zoom applied
	wxFont          m_currStdFont;
	wxFont          m_currSmallFont;
	wxFont          m_currSmallBoldFont;
	wxFont          m_currBoldFont;
	wxFont          m_currBoldItalicFont;

	// Misc. Settings
	bool            m_internalDragNDrop;

	// Misc. Functions
	void            OpenSettings        (const wxString& selFile=wxEmptyString, int selIndex=0, int selPage=0);
	bool            UpdateIndex         (wxWindow* parent, bool deepUpdate);
	void            SetSkinAzValues     (int az);
	void            OnSkinTargetEvent   (int targetId, SjSkinValue&, long accelFlags); // overwritten from SjSkinWindow
	bool            QueryEndSession     (bool onShutdown=FALSE);
	void            DoEndSession        ();
	void            LoadLayout          (SjSkinLayout* l, SjLoadLayoutFlag sizeChangeFlag=SJ_AUTO_SIZE_CHANGE) { long oldLines = GetLinesCount(); SjSkinWindow::LoadLayout(l, sizeChangeFlag); if( oldLines != GetLinesCount() ) { m_display.m_scrollPos = -1; } UpdateDisplay(); }

	// Menu Stuff
public:
	void            AllocMainMenu       ();
	void            InitMainMenu        ();
	void            CreateContextMenu_  (SjMenu&, bool embedFastSearch=FALSE);
	void            CreatePlaybackMenu  (SjMenu*);
	void            CreateKioskMenu     (SjMenu*);
	void            CreateViewMenu      (SjMenu*);
	void            CreateUnqueueMenu   (SjMenu&);
	void            CreateSearchMenu    (SjMenu&);
	void            AddScriptMenuEntries(SjMenu&, bool addConfigButtons=false);
	#if SJ_USE_TOOLTIPS
	SjMainFrameToolTipProvider m_toolTipProvider;
	SjToolTipProvider* GetToolTipProvider  (long targetId, long subitem, const wxRect& rect) { m_toolTipProvider.m_targetId = targetId; m_toolTipProvider.m_subitem = subitem; m_toolTipProvider.m_rect = rect; return &m_toolTipProvider; }
	#endif
	SjMenu          *m_fileMenu, *m_editMenu, *m_viewMenu, *m_playbackMenu, *m_kioskMenu, *m_helpMenu;
private:
	wxMenuBar*      m_menuBar;
	bool            m_menuBarComplete;
	void            UpdateMenuBarValue  (int targetId, const SjSkinValue& v);
	void            UpdateMenuBarQueue  ();
	void            UpdateMenuBarView   ();

	// Allowed Operations (in Kiosk Mode), see SJ_OP_* in skin.h
public:
	long            m_availOp;
	bool            IsOpAvailable       (long f) const { return (m_availOp&f)==f; }
	bool            IsAllAvailable      () const { return IsOpAvailable(SJ_OP_ALL); } // if SJ_OP_ALL is set, this should imply all other flags
	bool            IsKioskStarted      () const { return IsOpAvailable(SJ_OP_KIOSKON); }

	// Auto-control struff:
	// Call these functions only if the input REALLY comes from the user
	// (used for timeouts, eg. for the display reset and for following the playlist)
	void     GotInputFromUser    () {
		m_lastUserInputTimestamp = SjTools::GetMsTicks();
		#if SJ_USE_TOOLTIPS
		if(g_tools) {g_tools->m_toolTipManager.CloseToolTipWindow();}
		#endif
	}
	unsigned long LastInputFromUser       () const {return m_lastUserInputTimestamp;}
	void          GotDisplayInputFromUser () { m_lastUserDisplayInputTimestamp = SjTools::GetMsTicks(); GotInputFromUser(); }
	void          GotBrowserInputFromUser () { m_lastUserBrowserInputTimestamp = SjTools::GetMsTicks(); GotInputFromUser(); }

	SjAutoCtrl      m_autoCtrl;

	// Display stuff
	// The message OnSetDisplayMsg() may contain up to two lines separated by "\n"
	void            SetDisplayMsg       (const wxString& text, long holdMs=10000);
	#define         SDM_STATE_CHANGE_MS         2500
	#define         SDM_KIOSK_CANNOT_ENQUEUE_MS 3000
	void            ClearDisplayMsg     () { SetDisplayMsg(wxEmptyString, 0); }

	// you should only use the Get-functions from the player;
	// if SjMainFrame provides its own functins; these should be preferred
	SjPlayer        m_player;

	// Display stuff - Public - UpdateDisplay() should be used with care and is most times not needed!
	void            UpdateDisplay       ();
	SjDisplay       m_display;
private:
	// Display stuff - Private
	void            MarkDisplayTrack    (int targetId, bool mouseDown, long accelFlags);
	void            OnSkinTargetMotion  (int targetId, int motionAmount);
	void            OnSkinDisplayEvent  (int targetId, SjSkinValue&, long accelFlags);
	wxString        CombineArtistNTitle (const wxString& artist, const wxString& title, const wxString& sep=wxT("\t"));
	void            UpdateCurrTime      ();

private:
	// Functions overwritten from SjSkinWindow
	void            OnSkinTargetContextMenu (int, long x, long y);

	// The search stuff
	wxString        NormalizeSearchText (const wxString&);
	void            OnSimpleSearchInput (wxCommandEvent&); // also called on SetValue, so use the flag below
	void            OnSimpleSearchInputTimer (wxTimerEvent&);
	void            OnSearchHistory     (wxCommandEvent&);
	void            OnSearchGenre       (wxCommandEvent&);
	void            OnSearchMusicSel    (wxCommandEvent&);
	wxTextCtrl*     m_simpleSearchInputWindow;
	bool            m_simpleSearchInputFromUser,
	                m_inPerformingSearch;
	wxTimer         m_simpleSearchInputTimer;
	wxString        m_simpleSearchDelayedText;
	long            m_allSearchCount,
	                m_allSearchMs;

	// current search settings
	SjSearch        m_search;
	SjSearchStat    m_searchStat;
	wxString        m_searchLastColumnGuid;
	long            m_searchLastColumnGuidViewOffset;
	SjAdvSearch     m_tempSearch;

	// Zoom and Font Settings
public:
	#define         SJ_ZOOM_MIN     0
	#define         SJ_ZOOM_DEF     3
	#define         SJ_ZOOM_MAX     6
	void            SetZoom__           (long newZoom, bool redraw=true); // Should ONLY be called by SjBrowserWindow::SetZoom() (which should be used from extern)
	long            GetZoom             () const {return m_currZoom;}
	static long     CorrectZoom         (long zoom);
private:
	long            m_currZoom; // 0=smallest..3=normal..6=largest
	long            m_baseColumnWidth;
	long            m_baseCoverHeight;
	wxString        m_baseFontFace;
	long            m_baseFontSize;
	static long     CorrectColumnWidth  (long columnWidth);
	static long     CorrectCoverHeight  (long coverHeight);
	static long     CorrectFontSize     (long fontSize);
	void            CalcCurrFontSizesFromZoom ();
	void            CreateFontObjects   ();

	// Misc.
	unsigned long   m_lastUserInputTimestamp, m_lastUserDisplayInputTimestamp, m_lastUserBrowserInputTimestamp;
	bool            m_showRemainingTime;
	bool            m_blinkBlink;
	wxTimer         m_elapsedTimeTimer;
	SjMainApp*      m_mainApp;
	bool            m_updateIndexAfterConstruction;
	bool            m_haltedManually;
	bool            m_inConstruction;
	wxArrayString   m_contextMenuClickedUrls;
	long            m_contextMenuClickedId;
	#if SJ_USE_SCRIPTS
	SjSee*          m_cmdLineAndDdeSee;
	#endif
public:
	#if SJ_USE_SCRIPTS
	void            CmdLineAndDdeSeeExecute (const wxString&);
	#endif
private:

	// paste handling
	void            OnPaste             (wxCommandEvent&);
	wxPoint         m_pasteCoord;

	// events
	//void          OnIdle              (wxIdleEvent&);
	void            OnOpenBuyDlg        (wxCommandEvent&);
	void            OnFwdToSkin         (wxCommandEvent&);
	void            OnFwdToPlayer       (wxCommandEvent&);
	void            OnFwdToModules      (wxCommandEvent&);
	void            OnEsc               (wxCommandEvent&);
	void            OnTab               (wxCommandEvent&);
	void            OnCloseWindow       (wxCloseEvent&);
	void            OnIconizeWindow     (wxIconizeEvent&);
	void            OnElapsedTimeTimer  (wxTimerEvent&);

	DECLARE_EVENT_TABLE ();

	friend class    SjMainFrameToolTipProvider;
	friend class    SjBasicSettingsConfigPage; // for m_locale
	friend class    SjKioskModule;
	friend class    SjDisplayEditDlg;
	friend class    SjAutoCtrl;
	friend class    SjDisplay;
	friend class    SjAutoFader;
	friend class    SjTagEditorDlg;
};


#endif /* __SJ_MAINFRAME_H__ */
