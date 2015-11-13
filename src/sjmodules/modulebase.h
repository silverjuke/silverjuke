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
 * File:    modulebase.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke modules
 *
 ******************************************************************************/


#ifndef __SJ_MODULEBASE_H__
#define __SJ_MODULEBASE_H__


class SjModuleSystem;
class SjInterfaceBase;
class SjScannerModule;
class SjMenu;
class SjRow;
class SjCol;
class SjListView;


/*******************************************************************************
 * Track Info Object, used for communication between Silverjuke and Modules
 ******************************************************************************/


enum SjResult
{
	 SJ_ERROR = 0
	,SJ_SUCCESS = 1
	,SJ_SUCCESS_BUT_NO_DATA = 2
};


class SjTrackInfo
{
public:
	                SjTrackInfo         () { ClearLongs(); }
	void            Clear               () { ClearLongs(); ClearStrings(); }

	// Flags that may be used by functions taking SjTrackInfo as arguments.
	// They are not used by SjTrackInfo itself.

	#define         SJ_TI_QUICKINFO         0x01 // set only m_trackName, m_leadArtistName, m_albumName and m_playtimeMs
	#define         SJ_TI_ALBUMCOVERURL__   0x02 // set only m_arts to the TRACK cover url (by default, this is the ALBUM cover url)
	#define         SJ_TI_TRACKCOVERURL     0x04 // set only m_arts to the TRACK cover url (by default, this is the ALBUM cover url)
	#define         SJ_TI_FULLINFO          0x08 // set all but m_arts

	// information that should be set by the Music Track Scanner modules
	// null or an empty string indicates missing information

	wxString        m_url;          // eg. a file
	long            m_id;           // internal, optional use, normally the URL should be used to identify a track
	uint32_t        m_updatecrc;    // any number that changes if the track was modified
	unsigned long   m_timeAdded;
	unsigned long   m_timeModified;
	unsigned long   m_timesPlayed;
	unsigned long   m_lastPlayed;
	long            m_dataBytes;    // eg. the file size, includes headers and ID3 tags
	long            m_bitrate;
	long            m_samplerate;
	long            m_channels;
	long            m_playtimeMs;
	long            m_autoVol;      // we store the gain * 1000 here, eg. 4321 for 4.321, 0 indicates no information

	wxString        m_trackName;
	long            m_trackNr;
	long            m_trackCount;
	long            m_diskNr;
	long            m_diskCount;

	wxString        m_leadArtistName;
	wxString        m_orgArtistName;
	wxString        m_composerName;

	wxString        m_albumName;

	wxString        m_genreName;
	wxString        m_groupName;
	wxString        m_comment;
	long            m_beatsPerMinute;
	long            m_rating;
	long            m_year;

	// arts, several files are separated by "\n"
	wxString        m_arts;
	void            AddArt              (const wxString& lines) { if( !m_arts.IsEmpty() ) m_arts += wxT("\n"); m_arts += lines; }

	// some functions use the m_validFields to determinate which fields are valid
	#define         SJ_TI_URL               0x00000001L
	#define         SJ_TI_TRACKNAME         0x00000002L
	#define         SJ_TI_TRACKNR           0x00000004L
	#define         SJ_TI_TRACKCOUNT        0x00000008L
	#define         SJ_TI_DISKNR            0x00000010L
	#define         SJ_TI_DISKCOUNT         0x00000020L
	#define         SJ_TI_LEADARTISTNAME    0x00000040L
	#define         SJ_TI_ORGARTISTNAME     0x00000080L
	#define         SJ_TI_COMPOSERNAME      0x00000100L
	#define         SJ_TI_ALBUMNAME         0x00000200L
	#define         SJ_TI_GENRENAME         0x00000400L
	#define         SJ_TI_GROUPNAME         0x00000800L
	#define         SJ_TI_COMMENT           0x00001000L
	#define         SJ_TI_BEATSPERMINUTE    0x00002000L
	#define         SJ_TI_RATING            0x00004000L
	#define         SJ_TI_YEAR              0x00008000L
	#define         SJ_TI_PLAYTIMEMS        0x00010000L
	#define         SJ_TI_X_TIMEADDED       0x0000001FL // the SJ_TI_X_* values are only used by the list view, but they're stored in the db, no bit-tests are possible!
	#define         SJ_TI_X_TIMEMODIFIED    0x0000002FL
	#define         SJ_TI_X_LASTPLAYED      0x0000003FL
	#define         SJ_TI_X_TIMESPLAYED     0x0000004FL
	#define         SJ_TI_X_DATABYTES       0x0000005FL
	#define         SJ_TI_X_BITRATE         0x0000006FL
	#define         SJ_TI_X_SAMPLERATE      0x0000007FL
	#define         SJ_TI_X_CHANNELS        0x0000008FL
	#define         SJ_TI_X_AUTOVOL         0x0000009FL
	#define         SJ_TI_Y_FILETYPE        0x000000AFL // the SJ_TI_Y_* are calculated by other values, no bit-tests are possible!
	#define         SJ_TI_Y_QUEUEPOS        0x000000BFL
	long            m_validFields;

	SjTrackInfo&    operator =          (const SjTrackInfo& o);
	bool            AddMerge            (const SjTrackInfo& o); // returns TRUE if sth. was modified

	// get / set values by known SJ_TI_* values,
	wxString        GetFormattedValue   (long ti) const;
	wxString        GetValue            (long ti) const;
	void            SetValue            (long ti, const wxString&);
	static wxString GetFieldDescr       (long ti);
	static wxString GetFieldDbName      (long ti);

	// compare two track information objects; currently only needed for debugging purposes
	#ifdef __WXDEBUG__
	bool            operator ==         (const SjTrackInfo& o) const { return  IsEqualTo(o); }
	bool            operator !=         (const SjTrackInfo& o) const { return !IsEqualTo(o); }
	bool            IsEqualTo           (const SjTrackInfo& o) const;
	wxString        Diff                (const SjTrackInfo& o) const;
	#endif

private:
	void            ClearLongs          ();
	void            ClearStrings        ();
};


/*******************************************************************************
 * Common Module Base Class
 ******************************************************************************/


enum SjModuleType
{
     SJ_MODULETYPE_ALL = 0
	,SJ_MODULETYPE_COMMON       // common module
    ,SJ_MODULETYPE_COL          // COLUMN GENERATOR module
    ,SJ_MODULETYPE_SCANNER      // SCANNER module (normally for internal use only)
    ,SJ_MODULETYPE_VIS_RENDERER // VISUALIZATION module
    ,SJ_MODULETYPE_COUNT
};


class SjModule
{
public:
	// Constructor / destructor.
	// The destructor has no real implementation, but "virtual" is important
	                SjModule            (SjInterfaceBase*, SjModuleType type);
	virtual         ~SjModule           () { wxASSERT(m_usage==0); }

	// pointer back to the interface with this module was found
	SjInterfaceBase* m_interface;

	// the module type
	SjModuleType    m_type;

	// only a hint that may be used to sort the modules in the settings dialog
	long            m_sort;


	wxString        m_name;                 // name and (optional) version
	wxString        m_file;                 // full path to the module, internal modules should use "memory:module_name.lib"
	int             m_fileIndex;            // if a module file has several modules, this defines the zero-based index

	// Load/unload module.  You have to load a module before using
	// any other module function (beside some exceptions which are
	// defined separatly).
	bool            Load                ();
	void            Unload              ();
	bool            IsLoaded            () const { return (m_usage>0); }
	long            GetUsage            () const { return m_usage; }

	// As an alternative to the normal destructor, you use UnloadAndDestroy()
	// (this is needed eg. for some winamp-dlls who crash if unloaded "in time")
	void            UnloadAndDestroy    ();

	// Simple "little" configutation.  GetLittleOptions() should also work for
	// unloaded modules.
	virtual void    GetLittleOptions    (SjArrayLittleOption&) { }

	// Receiving IDMODMSG_* messages (Silverjuke -> Module)
	virtual void    ReceiveMsg          (int) { };

	// (Un-)packing the filename/fileindex.  This also works for unloaded modules.
	wxString        PackFileName        ()       { return PackFileName(m_file,m_fileIndex); }
	static wxString PackFileName        (const wxString& file, int fileIndex);
	static void     UnpackFileName      (const wxString& inFileNIndex, wxString& retFile, int& retIndex);

	// Get Unique String IDs for identifying a module.  This also works for unloaded modules.
	#if 0
	wxString        GetUniqueStrId      () const { return GetUniqueStrId(m_file, m_fileIndex); }
	static wxString GetUniqueStrId      (const wxString& file, int fileIndex);
	#endif

	// Is this module a plugin?
	virtual bool    IsPlugin            () { return false; }

protected:
	virtual bool    FirstLoad           () { return TRUE; }
	virtual void    LastUnload          () { }

private:
	long            m_usage;

	unsigned long   m_pushPendingTimestamp;
	bool            m_destroyPending;

	friend class    SjModuleSystem;
};


WX_DECLARE_LIST(SjModule, SjModuleList);


/*******************************************************************************
 * Common Module Base Class
 ******************************************************************************/


enum SjDragNDropAction
{
    // a normal sequence is  ENTER -> MOVE -> MOVE ... -> LEAVE
    // or                    ENTER -> MOVE -> MOVE ... -> DROP
    // either "LEAVE" or "DROP" are send, noth both
    //
    // for "MOVE", SjCommonModule::DragNDrop() should return if drag'n'drop
    // at the given position is acceptable (TRUE) or not (FALSE).
    // for "ENTER", "LEAVE" and "DROP", return TRUE if d'n'd is okay in common.
    SJ_DND_ENTER
    ,SJ_DND_MOVE
    ,SJ_DND_LEAVE
    ,SJ_DND_DROP
};


class SjDataObject : public wxDataObjectComposite
{
public:
	                SjDataObject        (wxDragResult);
	void            AddFileData         ();
	void            AddFile             (const wxString&);
	void            AddFiles            (const wxArrayString&);
	void            AddBitmapData       ();

	wxFileDataObject*   m_fileData;
	wxBitmapDataObject* m_bitmapData;
	wxDragResult        m_dragResult;
};


enum SjEmbedTo
{
    SJ_EMBED_NONE=0,
    SJ_EMBED_TO_MAIN=1,
    SJ_EMBED_TO_BASICSETTINGS=2,
    SJ_EMBED_TO_MUSICLIB=3
};


class SjCommonModule : public SjModule
{
public:
	SjCommonModule(SjInterfaceBase* interf, SjModuleType type=SJ_MODULETYPE_COMMON)
		: SjModule(interf, type)
	{
		m_notebookPage  = 0;
		m_guiIcon       = SJ_ICON_EMPTY;
	}

	virtual SjIcon  GetGuiIcon          () const { return m_guiIcon; }

	// The configuration.  EmbedTo() also works for unloaded modules and
	// should return one of SJ_EMBED_*
	virtual SjEmbedTo EmbedTo     () { return SJ_EMBED_NONE; }

	// Still configuration.  Load the modules before using the following functions.
	virtual wxWindow* GetConfigPage       (wxWindow* parent, int selectedPage) { return NULL; }
	virtual void    GetConfigButtons    (wxString& okText, wxString& cancelText, bool& okBold) { okText=_("OK"); cancelText=_("Cancel"); okBold=FALSE; }
	#define         SJ_CONFIGPAGE_DONE_GENERIC          0
	#define         SJ_CONFIGPAGE_DONE_OK_CLICKED       1
	#define         SJ_CONFIGPAGE_DONE_CANCEL_CLICKED   2
	virtual void    DoneConfigPage      (wxWindow* configPage, int action) { }
	int             m_notebookPage;     // used by the settings dialog to remember the current page if a notebook is present

	// Drag'n'drop
	// the data on drop are found in SjMainFrame::m_dataObject
	virtual bool    DragNDrop           (SjDragNDropAction, wxWindow*, const wxPoint&, SjDataObject*) { return FALSE; }

protected:
	// The icon.
	SjIcon          m_guiIcon;
};


/*******************************************************************************
 * Music library generator module class
 ******************************************************************************/


class SjColModule : public SjCommonModule
{
public:
	SjColModule(SjInterfaceBase* interf)
		: SjCommonModule(interf, SJ_MODULETYPE_COL)
	{
	}

	// Search handling, the function should return the number of
	// tracks/rows (NOT: albums/columns) found; if there is no
	// search set, a previous search should be canceled and the
	// function should return null.
	// If "deepSearch" is set, the module should not use anything
	// from its cache but really search again.  If the module does
	// not cache any searches, this flag can be ignored.
	virtual SjSearchStat SetSearch      (const SjSearch&, bool deepSearch) = 0;

	// called if "del" or "ins" is hit, the function should return the number
	// of DELETED tracks/rows, if any
	virtual long    DelInsSelection     (bool del) = 0;

	// Column information for album and cover view - "MaskedCol" refers to the current search set
	// while "UnmaskedCol" ignores all search settings
	virtual long    GetUnmaskedColCount () = 0;
	virtual SjCol*  GetUnmaskedCol      (long index) = 0;

	virtual long    GetMaskedColCount   () = 0;
	virtual long    GetMaskedColIndexByAz(int targetId) { return 0; };
	virtual long    GetMaskedColIndexByColUrl(const wxString& colUrl) = 0;
	virtual SjCol*  GetMaskedCol        (long index) = 0;
	virtual SjCol*  GetMaskedCol        (const wxString& trackUrl, long& retIndex /*-1 if currently hidden eg. by search*/) = 0;
	virtual bool    UpdateAllCol        (wxWindow* parent, bool deepUpdate) { return TRUE; };

	// List view depending stuff
	virtual SjListView* CreateListView  (long order, bool desc, long minAlbumRows) = 0;

	// Selection handling (see also SjRow)
	virtual void    SelectAllRows       (bool select)         { }
	virtual long    GetSelectedUrlCount ()                    { return 0; }
	virtual void    GetSelectedUrls     (wxArrayString& urls) { }

	// Get Track Information
	virtual bool    GetTrackInfo        (const wxString& url, SjTrackInfo&, long flags, bool logErrors) = 0;

	// the following functions may be called by a SjScannerModule object...

	// ...CheckTrackInfo() should check if the given URL exists in the
	// database and if it is up to date. return TRUE if so and FALSE if
	// it does not exist or is out of date.
	// if you return TRUE, you should mark the URL as updated as you
	// ReceiveTrackInfo() won't be called.
	virtual bool    Callback_CheckTrackInfo(const wxString& url, uint32_t actualCrc32) = 0;

	// ...receiving track information, the given object is owned by
	// the function and should be deleted from it when no longer needed
	virtual bool    Callback_ReceiveTrackInfo(SjTrackInfo* trackInfo) = 0;

	virtual bool    Callback_MarkAsUpdated(const wxString& urlBegin, long checkTrackCount) = 0;
};


class SjListView
{
public:
	                SjListView          (long order, bool desc, long minAlbumRows) { m_currOrderField=order; m_currOrderDesc=desc; m_minAlbumRows = minAlbumRows; }
	virtual         ~SjListView         () {}

	// all stuff is always "masked" (regards music selections and searched)
	virtual void    ChangeOrder         (long order, bool desc) = 0;
	virtual long    GetTrackCount       () = 0;
	virtual bool    IsTrackSelected     (long offset) = 0;
	virtual void    SelectTrack         (long offset, bool select) = 0;
	virtual long    Url2TrackOffset     (const wxString& url) = 0;  // return -1 if not in view
	long            Az2TrackOffset      (int az);
	int             GetAzFromOffset     (long offset);

	// get information about a row/a track
	#define         SJ_LISTVIEW_SPECIAL_FIRST_TRACK     0x01
	#define         SJ_LISTVIEW_SPECIAL_GAP             0x02
	#define         SJ_LISTVIEW_SPECIAL_LAST_GAP        0x04
	#define         SJ_LISTVIEW_SPECIAL_DARK            0x08
	virtual void    GetTrack            (long offset, SjTrackInfo&, long& albumId, long& special) = 0;
	virtual long    GetTrackSpecial     (long offset) = 0;


	virtual wxString GetUpText          () = 0;

	virtual void    CreateContextMenu   (long offset, SjMenu&) = 0;
	virtual void    OnContextMenu       (long offset, int id) = 0;
	virtual void    OnDoubleClick       (long offset, bool modifiersPressed) = 0;
	virtual void    OnMiddleClick       (long offset, bool modifiersPressed) = 0;

	// get the corresponding SjCol object for an offset;
	// the albumId is the value returned from GetTrack() and the returned object
	// must be freed using "delete" if it is no longer needed
	virtual SjCol*  GetCol              (long albumId) = 0;

protected:
	long            m_currOrderField;
	bool            m_currOrderDesc;
	long            m_minAlbumRows; // cannot be changed!
};


class SjCol
{
public:
	SjCol(SjColModule* module)              // constructor/destructor
	{
		m_rows          = NULL;
		m_rowCount      = 0;
		m_rowMaxCount   = 0;
		m_az            = 0;
		m_module        = module;
	}

	virtual ~SjCol()
	{
		DestroyRows();
	}

	wxString        m_url;                      // unique URL identifing the column
	SjColModule*    m_module;                   // pointer back to the module
	char            m_az;                       // sorting information
	wxString        m_textUp;                   // text that should be displayed vertically left of the column
	long            m_rowCount;
	SjRow**         m_rows;
	int             m_textlLeft, m_textlRight,  // the OS can save some
	                m_textmLeft, m_textmRight,  // calculated positions
	                m_textrLeft, m_textrRight,  // in these fields
	                m_top, m_bottom;

	void            AddRow              (SjRow*);
	SjRow*          RemoveRow           (SjRow*); // the row object is not deleted
	SjRow*          RemoveRow           (long index);
	SjRow*          RemoveLastRow       ()       { m_rowCount--; return m_rows[m_rowCount]; }
	void            DestroyRows         ();
	SjRow*          GetLastRow          ()       { return m_rowCount>0? m_rows[m_rowCount-1] : NULL; }
	SjRow*          FindSelectableRow   (long startIndex=0, long inc=+1);
	bool            IsFirstSelectableRow(SjRow* row) { return FindSelectableRow()==row; }
	SjRow*          FindFirstSelectedRow();
	void            SelectAllRows       (bool select);


private:
	long            m_rowMaxCount;
};


class SjRow
{
public:
	#define         SJ_RRTYPE_NORMAL    0
	#define         SJ_RRTYPE_TITLE1    1
	#define         SJ_RRTYPE_TITLE2    2
	#define         SJ_RRTYPE_TITLE3    3
	#define         SJ_RRTYPE_COVER     4

	SjRow(int roughType = SJ_RRTYPE_NORMAL)     // constructor/destructor
	{
		m_roughType = roughType;
	}

	virtual ~SjRow()
	{
	}

	SjCol* m_col;       // pointer back to the column, may be NULL

	virtual int     IsSelectable        () { return 0; }            // may the row may get into the selected state (2) or is it just clickable (1)?
	virtual bool    IsSelected          () { return FALSE; }        // is the row currently selected?
	virtual void    Select              (bool select) {}            // (de-)select the row
	virtual void    OnDoubleClick       (bool modifiersPressed) {}  // called on a double click for selectable and non-selectable rows
	virtual bool    OnDropData          (SjDataObject*) = 0;

	virtual int     UsesMiddleClick     () { return 0; }            // 0=no, 1=yes 2=yes and select first
	virtual bool    OnMiddleClick       (bool modifiersPressed) { return FALSE; }   // called on a click with the mid mouse button for selectable and non-selectable rows

	virtual void    CreateContextMenu   (SjMenu&) {}
	virtual void    OnContextMenu       (int id) {};                // should be called for context menus, ID is in the range IDM_MODULEUSER00-IDM_MODULEUSER99
	virtual wxString GetToolTip         (long& flags/*width in the lower bits*/) = 0;

	int             m_roughType;                // the rough type of the row

	wxString        m_textl, m_textm, m_textr;  // left/mid/right text, highlited text is remarked as
	                                            // "normal text \thighlited text\t normal text"

	int             m_top, m_bottom;            // the OS can save some calculated positions in these fields

	wxString        m_url;                      // may be used by the module,
	// nearly undefined for the user!
};


/*******************************************************************************
 * Music library reader module class
 ******************************************************************************/


class SjScannerModule : public SjModule
{
public:
	SjScannerModule(SjInterfaceBase* interf)
		: SjModule(interf, SJ_MODULETYPE_SCANNER)
	{
	}

	// An array of source types that can be add, the index is given to
	// AddSources().  These array must not change after construction!
	wxArrayString       m_addSourceTypes_;
	wxArrayLong         m_addSourceIcons_;

	// get/set sources (eg. directories or servers)
	virtual long        GetSourceCount      () = 0;
	virtual wxString    GetSourceUrl        (long index) = 0;
	virtual wxString    GetSourceNotes      (long index) = 0;
	virtual SjIcon      GetSourceIcon       (long index) = 0;
	virtual long        AddSources          (int sourceType, wxWindow* parent) = 0; // the function returns the first new index or -1 for errors or abort
	virtual bool        DeleteSource        (long index, wxWindow* parent) = 0;

	// ConfigSource() should return TRUE if an index update is required.
	virtual bool        ConfigSource        (long index, wxWindow* parent) = 0;

	// Interate track information,
	// The function should call SjColModule::Callback_ReceiveTrackInfo() for each source
	virtual bool        IterateTrackInfo    (SjColModule* receiver)=0;

	// Set track information, SjTrackInfo::m_url should be ignored
	virtual bool        SetTrackInfo        (const wxString& url, SjTrackInfo&)=0;

	// Check if a given URL belongs to a given source
	virtual bool        IsMyUrl             (const wxString& url)=0;

	// The follwing function is mainly are for the folder scanner
	// and is used when dropping folders to silverjuke. Other scanners should get
	// their sources by AddSources()
	virtual bool        AddUrl              (const wxString& url) { return FALSE; }
};


/*******************************************************************************
 *  Visialization Module Base Class
 ******************************************************************************/


class SjVisImpl;


class SjVisRendererModule : public SjModule
{
public:
	SjVisRendererModule(SjInterfaceBase* interf)
		: SjModule(interf, SJ_MODULETYPE_VIS_RENDERER)
	{
	}

	// Start the visualisation. The module should use the
	// public interface of SjVisModule and SjVisImpl
	// "justContinue" ist just a hint, set if the vis. window was attached/detached and is started therefore.
	virtual bool    Start               (SjVisImpl*, bool justContinue) = 0;

	// Stop the visualisation. After Stop() is called, the
	// player object given to Start() is no longer valid.
	virtual void    Stop                () = 0;

	// option handling
	virtual void    AddMenuOptions      (SjMenu&) {}
	virtual void    OnMenuOption        (int i) {}

	// when the size has changed ... the module should call SjVisImpl::GetRendererClientRect() or SjVisImpl::GetRendererScreenRect()
	virtual void    PleaseUpdateSize    (SjVisImpl*) = 0;
};


/*******************************************************************************
 *  Module System, Interface Class
 ******************************************************************************/


class SjInterfaceBase
{
public:
	// constructor
	                SjInterfaceBase     (const wxString& name);

	// the destructor has no implementation, but "virtual" is important
	virtual         ~SjInterfaceBase    () {};

	// pointers back to the main frame and to the module system
	SjModuleSystem* m_moduleSystem;

	// query interface information
	wxString        GetName             () const { return m_name; }

	// get modules. the returned modules are owned by the caller,
	// the function returns TRUE if any modules are added or deleted from the list.
	virtual void    LoadModules         (SjModuleList&) = 0;

	// message handling
	virtual void    ReceiveMsg          (int) { }

	// the cache, the module is only identified by m_file and NOT by m_fileIndex,
	// so if a file contains several modules and should be written to the cache, all
	// information about all modules should be placed in addInfo
	#if 0
	void            WriteToCache        (const wxString& file, const wxString& info, unsigned long fileTimestamp=0);
	bool            ReadFromCache       (const wxString& file, wxArrayString& info, unsigned long fileTimestamp=0);
	wxString        ReadFromCache       (const wxString& file, unsigned long fileTimestamp=0);
	#endif

protected:

	#if 0
	// Some tools that may be useful for derived classes:
	//
	// AddModulesFromDir() searches the given directory for *.dll files
	// and calls AddModulesFromFile() for each file found. Moreover,
	// AddModulesFromDir() recurses into some special directories as
	// "plugins" or "modules".
	void            AddModulesFromDir   (SjModuleList&, const wxString& dirName, bool suppressNoAccessErrors=FALSE);
	virtual void    AddModulesFromFile  (SjModuleList&, const wxString& file, bool suppressNoAccessErrors) {  }

	// AddModulesFromSearchPaths() calls AddModulesFromDir() for each search path
	// and returns TRUE if sth. was changed.
	void            AddModulesFromSearchPaths(SjModuleList&, bool suppressNoAccessErrors=FALSE);

	bool            IsModuleAdded       (SjModuleList&, const wxString& file, int fileIndex=0, const wxString& name=wxEmptyString);
	#endif

private:
	wxString        m_name;
	#if 0
	wxString        GetUniqueStrId      () const { return SjNormaliseString(m_name, 0); }
	#endif

	friend class    SjModuleSystem;
};


WX_DECLARE_LIST(SjInterfaceBase, SjInterfaceList);


/*******************************************************************************
 *  The Module System
 ******************************************************************************/


class SjModuleSystem
{
public:
	// constructor / destructor / init / exit
	                SjModuleSystem      ();
	                ~SjModuleSystem     ();
	void            Init                ();
	void            Exit                ();

	// adding interfaces, get interface information
	void            AddInterface        (SjInterfaceBase*);

	// Load modules. This function may only be called once on startup.
	void            LoadModules         ();

	// GetModules() returns modules of the given type or all modules if
	// type=0. the result must not be deleted. The list and the
	// module pointers are valid until the next call of
	// UpdateModules().
	SjModuleList*   GetModules          (SjModuleType type);

	// get assigned extensions
	#define         SJ_EXT_MUSICFILES           0x001
	#define         SJ_EXT_IMAGEFILES           0x002
	#define         SJ_EXT_ARCHIVES             0x004
	#define         SJ_EXT_SKINFILES            0x008
	#define         SJ_EXT_LANGUAGEFILES        0x010
	#define         SJ_EXT_DATABASES            0x020
	#define         SJ_EXT_PLAYLISTS_READ       0x040
	#define         SJ_EXT_PLAYLISTS_WRITE      0x080
	#define         SJ_EXT_KARAOKE              0x100
	SjExtList       GetAssignedExt              (long flags);

	// searching modules
	SjModule*        FindModuleByFile            (const wxString& file, int fileIndex = 0);
	SjScannerModule* FindScannerModuleByUrl      (const wxString& url);
	bool             FindImageHandlerByExt       (const wxString& ext) { wxASSERT( ext.Cmp(ext.Lower())==0 ); return m_imageExtList.LookupExt(ext); }
	bool             FindPlaylistHandlerByExt    (const wxString& ext) { wxASSERT( ext.Cmp(ext.Lower())==0 ); return m_playlistExtListRead.LookupExt(ext); }

	// get the list of interfaces
	SjInterfaceList* GetInterfaces      () { return &m_listOfInterfaces; }

	// BroadcastMsg() send a IDMODMSG_* message to all modules and
	// to all module interfaces
	void            BroadcastMsg        (int);

	wxArrayString   m_scripts;
	#if SJ_USE_SCRIPTS
	wxArrayPtrVoid  m_sees;
	#endif

	// needed for shutdown as the garbage may contain references to the default database
	static wxSqltDb* s_delayedDbDelete;

private:
	SjInterfaceList m_listOfInterfaces;
	SjModuleList    m_listOfModules;
	SjModuleList    m_listOfModules2[SJ_MODULETYPE_COUNT];
	SjSLHash        m_hashOfExt;
	SjExtList       m_imageExtList;
	SjExtList       m_playlistExtListRead;
	SjExtList       m_playlistExtListWrite;

	wxArrayString   GetStubImageExt     () const;
	void            InitMemoryFS        ();

	// when modules are unloaded, they normally stay
	// in memory for a little while - this is to avoid
	// too much loading/unloading of modules.
	// UnloadPending() unloads theses pending modules physically.
	void            PushPending         (SjModule*, bool destroyModulePointer=FALSE);
	bool            PopPending          (SjModule*);
	void            UnloadPending       (bool ignoreTimeouts=FALSE);
	SjModuleList    m_listOfPendingModules;

	friend class SjInterfaceBase;
	friend class SjModule;
};


#endif // __SJ_MODULEBASE_H__
