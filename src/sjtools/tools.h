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
 * File:    tools.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke tools
 *
 ******************************************************************************/


#ifndef __SJ_TOOLS_H__
#define __SJ_TOOLS_H__


class SjMainFrame;


/*******************************************************************************
 *  Sj*Hash Classes
 ******************************************************************************/


#include "hash.h"
#include "levensthein.h"
#include "homepageids.h"
#include "timeout.h"


class SjHashIterator
{
public:
	            SjHashIterator          () { m_hashElem = NULL; m_hashElemNext = NULL; }
	void        Rewind                  () { m_hashElem = NULL; }

private:
	sjhashElem*     m_hashElem;
	sjhashElem*     m_hashElemNext;
	friend class    SjLLHash;
	friend class    SjLPHash;
	friend class    SjSLHash;
	friend class    SjSPHash;
	friend class    SjSSHash;
};


class SjLLHash
{
public:
	// constructs a long => long hash
	                SjLLHash            () { sjhashInit(&m_hash, SJHASH_INT, 0); }
					~SjLLHash           () { Clear(); }
	SjLLHash&       operator =          (const SjLLHash& o) { CopyFrom((SjLLHash*)&o); return *this; }

	// Insert into hash. Returns the old element or 0 if there is no
	// old element so Null-values cannot be inserted. Null-keys are okay.
	long            Insert              (long key, long value)
	{
		wxASSERT(value != 0);
		return (long)sjhashInsert(&m_hash, NULL, /*pKey, not needed*/ (int)key, /*nKey*/ (void*)value/*pData*/);
	}

	// Remove from hash. Returns the old element or 0 if there is no
	// old element.
	long            Remove              (long key)
	{
		return (long)sjhashInsert(&m_hash, NULL,/*pKey, not needed*/ (int)key,/*nKey*/ NULL/*pData*/);
	}

	// Insert if value != 0, remove otherwise
	long            InsertOrRemove      (long key, long value)
	{
		return (long)sjhashInsert(&m_hash, NULL, /*pKey, not needed*/ (int)key, /*nKey*/ (void*)value/*pData*/);
	}

	// Search a key and return its the value or 0 if there is no such
	// key.
	long             Lookup              (long key) const
	{
		return (long)sjhashFind(&m_hash, NULL, /*pKey, not needed*/ (int)key/*nKey*/);
	}

	// Iterate the elements, Iterate() returns one value after the other,
	// if there are no more values, 0 is returned. Optionally, the key
	// is copied into the given buffer.
	long             Iterate             (SjHashIterator& i, long* key) const
	{
		wxASSERT(key);
		i.m_hashElem = i.m_hashElem? i.m_hashElemNext : m_hash.first;
		if( i.m_hashElem )
		{
			*key = i.m_hashElem->nKey;
			i.m_hashElemNext = i.m_hashElem->next; // saved to allow the returned element to be deleted
			return (long)i.m_hashElem->data;
		}
		return 0;
	}

	// get all keys as a string in the format "12,2,23" etc.
	wxString        GetKeysAsString     ();

	// retrieve the number of elemets in the hash
	long            GetCount            () const { return m_hash.count; }

	// remove all elements from the hash
	void            Clear               () { sjhashClear(&m_hash); }

private:
	sjhash          m_hash;
	void            CopyFrom            (SjLLHash* o);
};


class SjLPHash
{
public:
	// constructs a long => void* hash
	                SjLPHash            () { sjhashInit(&m_hash, SJHASH_INT, /*keytype*/ 0/*copyKey*/); }
	                ~SjLPHash           () { Clear(); }

	// Insert into hash. Returns the old element or 0 if there is no
	// old element so Null-values cannot be inserted. Null-keys are okay.
	void*           Insert              (long key, void* value)
	{
		wxASSERT(value != 0);
		return (void*)sjhashInsert(&m_hash, NULL, /*pKey, not needed*/ (int)key, /*nKey*/ value/*pData*/);
	}

	// Remove from hash. Returns the old element or 0 if there is no
	// old element.
	void*           Remove              (long key)
	{
		return (void*)sjhashInsert(&m_hash, NULL,/*pKey, not needed*/ (int)key,/*nKey*/ NULL/*pData*/);
	}

	// Search a key and return its the value or 0 if there is no such
	// key.
	void*           Lookup              (long key) const
	{
		return (void*)sjhashFind(&m_hash, NULL, /*pKey, not needed*/ (int)key/*nKey*/);
	}

	// Iterate the elements, Iterate() returns one value after the other,
	// if there are no more values, 0 is returned. Optionally, the key
	// is copied into the given buffer.
	void*           Iterate             (SjHashIterator& i, long* key) const
	{
		wxASSERT(key);
		i.m_hashElem = i.m_hashElem? i.m_hashElemNext : m_hash.first;
		if( i.m_hashElem )
		{
			*key = i.m_hashElem->nKey;
			i.m_hashElemNext = i.m_hashElem->next; // saved to allow the returned element to be deleted
			return (void*)i.m_hashElem->data;
		}
		return 0;
	}

	// retrieve the number of elemets in the hash
	long            GetCount            () const { return m_hash.count; }

	// remove all elements from the hash
	void            Clear               () { sjhashClear(&m_hash); }

private:
	sjhash          m_hash;
};


class SjSLHash
{
public:
	// constructs a string => long hash
	               SjSLHash            () { sjhashInit(&m_hash,
											#if wxUSE_UNICODE
												SJHASH_BINARY,
											#else
												SJHASH_STRING,
											#endif
											1 /* copyKey*/);
										}
	               ~SjSLHash           () { Clear(); }

	// Insert into hash. Returns the old element or 0 if there is no
	// old element so Null-values cannot be inserted.
	long           Insert              (const wxString& key, long value)
	{
		wxASSERT(value != 0);
		return (long)sjhashInsert(&m_hash, key.c_str(), (int)(key.Len()+1)
								#if wxUSE_UNICODE
		                          *sizeof(wxChar)
								#endif
		                          , (void*)value/*pData*/);
	}

	// Remove from hash. Returns the old element or 0 if there is no
	// old element.
	long            Remove              (const wxString& key)
	{
		return (long)sjhashInsert(&m_hash, key.c_str(), (int)(key.Len()+1)
								#if wxUSE_UNICODE
		                          *sizeof(wxChar)
								#endif
		                          , NULL/*pData*/);
	}

	// Search a key and return its the value or 0 if there is no such
	// key.
	long            Lookup              (const wxString& key) const
	{
		return (long)sjhashFind(&m_hash, key.c_str(), (int)(key.Len()+1)
								#if wxUSE_UNICODE
		                        *sizeof(wxChar)
								#endif
		                       );
	}

	// Iterate the elements, Iterate() returns one value after the other,
	// if there are no more values, 0 is returned. Optionally, the key
	// is copied into the given buffer.
	long            Iterate             (SjHashIterator& i, wxString& key)
	{
		i.m_hashElem = i.m_hashElem? i.m_hashElemNext : m_hash.first;
		if( i.m_hashElem )
		{
			key = (wxChar*)i.m_hashElem->pKey;
			i.m_hashElemNext = i.m_hashElem->next; // saved to allow the returned element to be deleted
			return (long)i.m_hashElem->data;
		}
		return 0;
	}

	// retrieve the number of elemets in the hash
	long            GetCount            () { return m_hash.count; }

	// remove all elements from the hash
	void            Clear               () { sjhashClear(&m_hash); }

private:
	sjhash          m_hash;
};


class SjSPHash
{
public:
	// constructs a string => void* hash
	                 SjSPHash            () {
												sjhashInit(&m_hash,
													#if wxUSE_UNICODE
														   SJHASH_BINARY,
													#else
														   SJHASH_STRING,
													#endif
														   1 /* copyKey*/);
											}
	                 ~SjSPHash           () { Clear(); }

	// Insert into hash. Returns the old element or 0 if there is no
	// old element so Null-values cannot be inserted.
	void*           Insert              (const wxString& key, void* value)
	{
		wxASSERT(value != 0);
		return sjhashInsert(&m_hash, key.c_str(), (int)(key.Len()+1)
							#if wxUSE_UNICODE
		                    *sizeof(wxChar)
							#endif
		                    , value/*pData*/);
	}

	// Remove from hash. Returns the old element or 0 if there is no
	// old element.
	void*           Remove              (const wxString& key)
	{
		return sjhashInsert(&m_hash, key.c_str(), (int)(key.Len()+1)
							#if wxUSE_UNICODE
		                    *sizeof(wxChar)
							#endif
		                    , NULL/*pData*/);
	}

	// Search a key and return its the value or 0 if there is no such
	// key.
	void*           Lookup              (const wxString& key) const
	{
		return sjhashFind(&m_hash, key.c_str(), (int)(key.Len()+1)
							#if wxUSE_UNICODE
							*sizeof(wxChar)
							#endif
		                 );
	}

	// Iterate the elements, Iterate() returns one value after the other,
	// if there are no more values, 0 is returned. Optionally, the key
	// is copied into the given buffer.
	void*           Iterate             (SjHashIterator& i, wxString& key) const
	{
		i.m_hashElem = i.m_hashElem? i.m_hashElemNext : m_hash.first;
		if( i.m_hashElem )
		{
			key = (wxChar*)i.m_hashElem->pKey;
			i.m_hashElemNext = i.m_hashElem->next; // saved to allow the returned element to be deleted
			return i.m_hashElem->data;
		}
		return NULL;
	}

	// retrieve the number of elemets in the hash
	long            GetCount            () { return m_hash.count; }

	// remove all elements from the hash
	void            Clear               () { sjhashClear(&m_hash); }

private:
	sjhash          m_hash;
};


class SjSSHash
{
public:
	// constructs a string => long hash
	                 SjSSHash            () {
		sjhashInit (&m_hash,
                    #if wxUSE_UNICODE
		            SJHASH_BINARY,
                    #else
		            SJHASH_STRING,
					#endif
		            1/* copyKey*/);
	}
	                 ~SjSSHash           () { Clear(); }

	// Insert into hash. Returns the old element or 0 if there is no
	// old element so Null-values cannot be inserted.
	void            Insert              (const wxString& key, const wxString& value)
	{
		wxString* oldValue = (wxString*)sjhashInsert(&m_hash, key.c_str(), (int)(key.Len()+1)
							#if wxUSE_UNICODE
		                     *sizeof(wxChar)
							#endif
		                     , (void*)new wxString(value));
		if( oldValue )
		{
			delete oldValue;
		}
	}
	void            Remove              (const wxString& key)
	{
		wxString* oldValue = (wxString*)sjhashInsert(&m_hash, key.c_str(), (int)(key.Len()+1)
						#if wxUSE_UNICODE
		                     *sizeof(wxChar)
						#endif
		                     , NULL);
		if( oldValue )
		{
			delete oldValue;
		}
	}

	// Search a key and return its the value or 0 if there is no such
	// key.
	wxString*        Lookup             (const wxString& key) const
	{
		return (wxString*)sjhashFind(&m_hash, key.c_str(), (int)(key.Len()+1)
								#if wxUSE_UNICODE
		                             *sizeof(wxChar)
								#endif
		                            );
	}
	/*wxString        Lookup            (const wxString& key, const wxString& defVal) const
	                                    {
	                                        wxString* val = Lookup(key);
	                                        if( val && !val->IsEmpty() )
	                                        {
	                                            return *val;
	                                        }
	                                        return defVal;
	                                    }*/

	// Iterate the elements, Iterate() returns one value after the other,
	// if there are no more values, 0 is returned. Optionally, the key
	// is copied into the given buffer.
	wxString*        Iterate            (SjHashIterator& i, wxString& key) const
	{
		i.m_hashElem = i.m_hashElem? i.m_hashElemNext : m_hash.first;
		if( i.m_hashElem )
		{
			key = (wxChar*)i.m_hashElem->pKey;
			i.m_hashElemNext = i.m_hashElem->next; // saved to allow the returned element to be deleted
			return (wxString*)i.m_hashElem->data;
		}
		return NULL;
	}

	// retrieve the number of elemets in the hash
	long            GetCount            () { return m_hash.count; }

	// remove all elements from the hash
	void            Clear               ();

	// (un)serialize as string
	wxString        Serialize           ();
	void            Unserialize         (const wxString&);

private:
	sjhash          m_hash;
};


#include "temp_n_cache.h"


/*******************************************************************************
 *  SjTools Class
 ******************************************************************************/


enum SjIcon
{
    // the order reflects the order in the bitmap!
     SJ_ICON_EMPTY = 0
    // large icons
	,SJ_ICON_MUSICLIB
    ,SJ_ICON_PLAYBACK_SETTINGS
    ,SJ_ICON_FX
    ,SJ_ICON_SKIN
    ,SJ_ICON_KIOSK
    ,SJ_ICON_ADVANCED
    // small icons:
    ,SJ_ICON_SETTINGSDIALOG
    ,SJ_ICON_ANYFOLDER
    ,SJ_ICON_MUSIC_FOLDER
    ,SJ_ICON_ANYFILE
    ,SJ_ICON_MUSIC_FILE
    ,SJ_ICON_SKIN_FILE
    ,SJ_ICON_PLAY
    ,SJ_ICON_DEPRECATED_PRELSTN
    ,SJ_ICON_SILVERJUKE
    ,SJ_ICON_COVER
    ,SJ_ICON_SWITCH
    ,SJ_ICON_INTERNET_SERVER
    ,SJ_ICON_MODULE
    ,SJ_ICON_ADVSEARCH
    ,SJ_ICON_ADVSEARCH_SEL
    ,SJ_ICON_NORMALKEY
    ,SJ_ICON_SYSTEMKEY
    ,SJ_ICON_EDIT
    ,SJ_ICON_PRIORITY
    ,SJ_ICON_UNCHECKED
    ,SJ_ICON_CHECKED
    ,SJ_ICON_UNRADIOED
    ,SJ_ICON_RADIOED
    // end of list
    ,SJ_ICON_COUNT
};


/*******************************************************************************
 *  SjTools Class
 ******************************************************************************/


class SjTools
{
public:
	SjTools();
	~SjTools();

	/********************************************************************
	 *  Crash Precaution
	 ********************************************************************/

	// CrashPrecaution() writes the given information in the file
	// .crashprecaution.  On silverjuke termination or when calling
	// NotCrashed(), this file is deleted. If the program hangs,
	// the information is printed on next startup.
	//
	// Modules using CrashPrecaution() should have a look
	// at the return value.  If FALSE is returned, the given objects
	// are possibly the reason for the last crash... If there was
	// no crash, TRUE is returned.  Alternativly, the m_lastCrash*
	// members may be used directly.
	bool            CrashPrecaution     (const wxString& module, const wxString& func, const wxString& object=wxT(""));
	void            NotCrashed          (bool stopLogging = false);
	void            ShowPossibleCrash();
	wxString        m_lastCrashModule;
	wxString        m_lastCrashFunc;
	wxString        m_lastCrashObject;

private:
	void            InitCrashPrecaution ();
	wxString        m_crashInfoFileName;
	wxCriticalSection
	m_crashPrecautionLocker;


	/********************************************************************
	 *  Exploring
	 ********************************************************************/

public:
	// the constructor - in the "real" constructor, eg. g_tools may be invalid!
	void            InitExplore         ();

	// explore any URL or check if it is exporable
	bool            IsUrlExplorable     (const wxString& uri);
	void            ExploreUrl          (const wxString& uri);

	// ExploreFile_() is called by ExploreUrl()
	// depending on the given url. This function should be implemented
	// OS-Specific.
	void            ExploreFile_        (const wxString& url, const wxString& programToUse);

	// Explore the Silverjuke homepage.
	// ExploreHelp() should be called if only the help should be opened,
	// which may also be available offline (but currently isn't). ExploreHomepage()
	// should be called when www.silverjuke.net is REALLY needed, eg. for
	// downloading skins or modules.
	void            ExploreHomepage     (SjHomepageId, const wxString& param=wxT(""));

	// the default program to use for exploring files
	wxString        m_exploreProgram;

	// uptime
	SjUptimeWatcher m_uptime;


	/********************************************************************
	 *  CRC and Mathemetical Stuff
	 ********************************************************************/

public:
	static unsigned long	Crc32Init           ();
	static unsigned long	Crc32Add            (unsigned long crc32, const char* buffer, int bufferBytes);
	static unsigned long	Crc32AddLong        (unsigned long crc32, long lng)	{ return Crc32Add(crc32, (const char*)&lng, sizeof(long)); }
	static unsigned long	Crc32AddString      (unsigned long crc32, const wxString& string) { wxCharBuffer stringBuffer = string.mb_str(wxConvUTF8); const char* stringPtr = stringBuffer.data();	return Crc32Add(crc32, stringPtr, strlen(stringPtr)); }
	static void     ToggleFlag          (long& bitfield, long flag) { if(bitfield&flag) {bitfield&=~flag;} else {bitfield|=flag;} }
	static void     SetFlag             (long& bitfield, long flag, bool set) { if(set) {bitfield|=flag;} else {bitfield&=~flag;} }
	static bool     SetFlagRetChanged   (long& bitfield, long flag, bool set) { long old=bitfield; SetFlag(bitfield, flag, set); return (old!=bitfield)/*return TRUE if changed*/; }
private:
	static bool     m_crc32InitDone;
	static unsigned long
	m_crc32Table[256];

public:
	static long     Rand                (long n); // returns a value between 0 and n-1
	static long     PrivateRand         (long& holdrand, long n); // returns a value between 0 and n-1
	static wxString ScrambleString      (const wxString&); // used to obscurify passwords, so that they are not directly readable in the INI-files
	static wxString UnscrambleString    (const wxString&);
	static long     VersionString2Long  (const wxString&);
	static unsigned long GetMsTicks     (); // return an integer that increases by 1 every millisecond.

	/********************************************************************
	 *  Configuration
	 ********************************************************************/

public:
	// using rgistry or file configuration (this is the normal case)
	wxString        m_instance;
	wxConfigBase*   m_config;
	bool            m_configIsDefault;
	wxString        m_configDescr;
	wxString        m_dbFile;
	bool            m_dbFileIsDefault;
	wxRect          ReadRect            (const wxString& key);
	void            WriteRect           (const wxString& key, const wxRect&);
	wxArrayString   ReadArray           (const wxString& key);
	void            WriteArray          (const wxString& key, const wxArrayString&);
	int             ReadFromCmdLineOrIni(const wxString& key, wxString& ret);

	// using locale-specific configuration
	static wxString LocaleConfigRead    (const wxString& keyname, const wxString& def);
	static long     LocaleConfigRead    (const wxString& keyname, long def);

	// language stuff
	static const wxLanguageInfo*
	FindLanguageInfo    (const wxString& canonicalName);
	wxArrayString   GetAvailLanguages   (wxArrayString* files=NULL);
	static bool     IsLangGerman        ();

private:
	wxConfigBase*   m_oldConfig;

	/********************************************************************
	 *  Search Paths
	 ********************************************************************/

public:
	int             GetSearchPathCount  () const        { return m_searchPaths.GetCount(); }
	wxString        GetSearchPath       (int index) const  { return m_searchPaths.Item(index); }
	int             GetSearchPathIndex  (const wxString& path) const;
	bool            IsStaticSearchPath  (int index) const { return (index>=0&&index<m_searchPathsFirstUser)? TRUE : FALSE; }
	bool            IsStaticSearchPath  (const wxString& path) const { return IsStaticSearchPath(GetSearchPathIndex(path)); }
	void            InitSearchPaths     ();
	static wxString GetUserAppDataDir   (); // eg. c:/dokumente und einstellungen/benutzer/meinname/
	static wxString GetSystemProgramDir (); // eg. c:/programs/
	static wxString GetSilverjukeProgramDir
	(); // eg. c:/programs/silverjuke/

private:
	wxArrayString   m_searchPaths;
	int             m_searchPathsFirstUser;

	/********************************************************************
	 *  Drawing Objects that never change
	 ********************************************************************/

public:
	wxCursor        m_staticMovehandCursor;
	wxCursor        m_staticResizeNWSECursor;
	wxCursor        m_staticResizeWECursor;
	wxCursor        m_staticNoEntryCursor;

	void            LoadStaticObjects   ();


	/********************************************************************
	 *  Drawing Objects changeable by the System
	 ********************************************************************/

public:
	static wxString wxColourToHtml      (const wxColour&);
	static long     wxColourToLong      (const wxColour&);
	static void     wxColourFromLong    (wxColour&, long l);
	static wxColour wxColourFromLong    (long l) { wxColour c; wxColourFromLong(c, l); return c; }



	/********************************************************************
	 *  Handling Temporary Files
	 ********************************************************************/

public:
	SjTempNCache    m_cache;

	/********************************************************************
	 *  Other File Tools
	 ********************************************************************/

public:
	static unsigned long    GetFileSize         (const wxString& name);
	static wxString         GetFileContent      (wxInputStream* inputStream, wxMBConv*);
	static wxString         GetFileNameFromUrl  (const wxString& url, wxString* retPath=NULL, bool stripExtension=FALSE, bool removeSepFromPath=FALSE);
	static wxString         SetFileNameToUrl    (const wxString& url, const wxString& fileName, bool leaveExtension);
	static bool             IsAbsUrl            (const wxString& url);
	static bool             AreFilesSame        (const wxString& src, const wxString& dest);
	static bool             CopyFile            (const wxString& src, const wxString& dest);
	static bool             CopyStreamToFile    (wxInputStream&, wxFile&);
	static wxString         EnsureValidFileNameChars
	(const wxString&);
	static wxString         EnsureValidPathChars(const wxString&);


	/********************************************************************
	 *  String Tools
	 ********************************************************************/

public:
	static wxString GetLineBreak        ();

	static wxString FormatNumber        (long number);
	static wxString FormatNumbers       (const wxArrayLong& numbers, long addToAllNumbers=0);
	static bool     ParseNumber         (const wxString& str, long* retNumber=NULL);

	static wxString FormatStdFloat      (float f); // returns an non-localized string, ready to save to INI etc.
	static float    ParseFloat          (const wxString& str);

	static wxString FormatRect          (const wxRect&);
	static wxRect   ParseRect           (const wxString&);

	#define         SJ_FT_ALLOW_ZERO    0x01L
	#define         SJ_FT_MIN_5_CHARS   0x02L
	#define         SJ_FT_PREPEND_MINUS 0x04L
	static wxString FormatTime          (long seconds, long flags=0);
	static bool     ParseTime           (const wxString& str, long* retSeconds=NULL);

	static wxString FormatMs            (unsigned long ms);

	static wxString FormatDecibel       (double db, bool addPercent=TRUE);
	static wxString FormatGain          (double gain, bool addPercent=TRUE);
	static bool     ParseDecibel        (const wxString&, double& ret);

	#define         SJ_FORMAT_ADDTIME   0x00000001L
	#define         SJ_FORMAT_EDITABLE  0x00000002L
	static wxString FormatDate          (unsigned long timestamp, long flags);
	static wxString FormatDate          (const wxDateTime& dt, long flags) { return FormatDate(dt.GetAsDOS(), flags); }
	static bool     ParseDate_          (const wxString& str, bool keepItSimple=TRUE, wxDateTime* retDateTime=NULL, bool* retTimeSet=NULL);
	static bool     ParseYear           (long year, long* retYear=NULL);
	static bool     ParseYear           (const wxString& str, long* retYear=NULL);

	#define         SJ_FORMAT_ADDEXACT  0x00000001L
	#define         SJ_FORMAT_MB        0x00000004L
	static wxString FormatBytes         (long bytes, int flags = 0);

	static wxString ShortenUrl          (const wxString&, long maxChars=0);

	static wxString Urlencode           (const wxString&, bool encodeAsUtf8=false);
	static wxString Urldecode           (const wxString&);
	static wxString Htmlentities        (const wxString&);
	static wxString Menuencode          (const wxString&);



	static wxString GetExt              (const wxString& str);
	static wxString GetLastDir          (const wxString& str);

	static wxArrayString Explode        (const wxString& str, wxChar delims, long minRetItems, long maxRetItems=-1);
	static wxArrayLong   ExplodeLong    (const wxString& str, wxChar delims, long minRetItems, long maxRetItems=-1);
	static wxString Implode             (const wxArrayString&, const wxString& delim);
	static wxString Implode             (const wxArrayLong&, const wxString& delim);

	static bool     StripPrefix         (wxString& s, const wxString& p) { if(s.StartsWith(p)) {s=s.Mid(p.Len()); return TRUE;} else {return FALSE;} }

	static wxString Capitalize          (const wxString&);

	// returns true if sth was removed
	static bool ReplaceNonISO88591Characters(wxString&, wxChar replacement=wxT('?'));


	/********************************************************************
	 *  Drawing
	 ********************************************************************/

public:
	// drawing text:  On calling DrawText(), you should set the width of
	// the rectangle to the max. wanted width.  On return, the width and
	// the height are set to the text extent.
	// font1 is the standard font, font2 ist the font used after the
	// first opening bracket. Hilite colour is used to hilite characters
	// between two tabs
	void            DrawText            (wxDC&, const wxString&, wxRect&, const wxFont&, const wxFont&, const wxColour& hiliteColour, bool doDraw=true);
	void            CalcTextWidthAndHeight(wxDC& dc, const wxString& text, const wxFont& font1, const wxFont& font2, int maxW, int& retW, int& retH)	{ wxRect rect(0,0,maxW,0); DrawText(dc, text, rect, font1, font2, *wxRED, FALSE); retW = rect.width; retH = rect.height; }
	int             CalcTextHeight      (wxDC& dc, const wxString& text, const wxFont& font1, const wxFont& font2, int maxW) { wxRect rect(0,0,maxW,0); DrawText(dc, text, rect, font1, font2, *wxRED, FALSE); return rect.height; }

	// same as DrawText() but only for one line of text; returns true if the text was truncated
	bool            DrawSingleLineText  (wxDC&, const wxString&, wxRect&, const wxFont&, const wxFont& smallFont, const wxFont* firstChar, const wxColour& hiliteColour);
	bool            DrawSingleLineText  (wxDC&, const wxString&, wxRect&, const wxColour& normalColour, const wxColour& hiliteColour, bool& hiliteState);

	// drawing bitmaps
	void            DrawBitmap          (wxDC&, const wxBitmap*, int x, int y, int w = 0, int h = 0);
	void            DrawBitmapHBg       (wxDC&, const wxBitmap*, const wxBitmap*, const wxBitmap*, const wxRect&, bool alignMToR = FALSE);
	void            DrawBitmapVBg       (wxDC&, const wxBitmap*, const wxBitmap*, const wxBitmap*, const wxRect&, bool alignMToB = FALSE);

	// drawing a cross indicating missing bitmaps
	static void     DrawCross           (wxDC&, int x, int y, int w, int h);
	static void     DrawCross           (wxDC& dc, const wxRect& r) { DrawCross(dc, r.x, r.y, r.width, r.height); }

	// draw a XOR rubberbox
	static void     DrawRubberbox       (wxDC&, const wxPoint&, const wxPoint&);

	// draw "icons"
	#define         SJ_DRAWICON_PLAY            0x001
	#define         SJ_DRAWICON_PAUSE           0x002
	#define         SJ_DRAWICON_STOP            0x004
	#define         SJ_DRAWICON_DELETE          0x008
	#define         SJ_DRAWICON_CHECK           0x010
	#define         SJ_DRAWICON_TRIANGLE_DOWN   0x020
	#define         SJ_DRAWICON_VOLUP           0x040
	#define         SJ_DRAWICON_VOLDOWN         0x080
	#define         SJ_DRAWICON_TRIANGLE_UP     0x100
	#define         SJ_DRAWICON_TRIANGLE_RIGHT  0x200
	#define         SJ_DRAWICON_TRIANGLE_LEFT   0x400
	#define         SJ_DRAWICON_MOVED_DOWN      0x800
	static void     DrawIcon            (wxDC&, const wxRect&, long flags);

	// find out some font face names
	void                    UpdateFacenames     (); // calling this will update the list used by GetFacenames() and HasFacename()
	const wxArrayString&    GetFacenames        ();
	bool                    HasFacename         (const wxString& facename);
private:
	wxArrayString   m_facenames;
public:

	/********************************************************************
	 *  Icons and more
	 ********************************************************************/

public:
	wxImageList*    GetIconlist         (bool large=FALSE);
	wxBitmap        GetIconBitmap       (SjIcon index, bool large=false);
	wxIcon          GetIconIcon         (SjIcon index, bool large=false);

	#if SJ_USE_TOOLTIPS
	SjToolTipManager m_toolTipManager;
	#endif

	// transparent (alpah-blended or see-through windows)
	bool            CanSetWindowTransparency    ();
	void            SetWindowTransparency       (wxWindow*, int transparency/*0=fully opaque, 100=fully transparent*/);
	void            UnloadWindowTransparencyLibs();

private:
	wxBitmap        m_iconBitmapsSmall;
	wxBitmap        m_iconBitmapsLarge;
	bool            m_iconlistLoaded;
	wxImageList     m_iconlistSmall; // leave the returned icons on SjSkinSkin destruction
	wxImageList     m_iconlistLarge;
};


extern SjTools* g_tools;


/*******************************************************************************
 * SjOmitWords Class
 ******************************************************************************/


class SjOmitWords
{
public:
	                SjOmitWords         (const wxString& words=wxEmptyString) {Init(words);}
	void            Init                (const wxString& words);
	wxString        Apply               (const wxString& str) const;
	wxString        GetWords            () {return m_words;};

private:
	wxString        m_words;
	wxArrayString   m_array;
};


/*******************************************************************************
 *  SjInHere Class
 ******************************************************************************/


class SjInHereHelper
{
public:
	SjInHereHelper() {
		m_inHere = NULL;
	}

	bool InHere(bool* v) {
		if(*v) {
			return true;
		}
		m_inHere=v;
		*m_inHere=true;
		return false;
	}

	~SjInHereHelper() {
		if(m_inHere) {
			*m_inHere = false;
		}
	}
private:
	bool*       m_inHere;
};

#define SJ_FORCE_IN_HERE_ONLY_ONCE      static bool inHereState = false; \
                                        SjInHereHelper inHereHelper; \
                                        if( inHereHelper.InHere(&inHereState) ) \
                                            return;


/*******************************************************************************
 * SjCoverFinder Class
 ******************************************************************************/


class SjCoverFinder
{
public:
	                SjCoverFinder(const wxString& keywords=wxEmptyString) { Init(keywords); }
	void            Init(const wxString& keywords);
	long            Apply(const wxArrayString& paths, const wxString& albumName); // returns the index in the given array or -1
	wxString        GetWords() { return m_keywords;} ;

private:
	wxString        m_keywords;
	wxArrayString   m_array;
};



/*******************************************************************************
 *  SjLineTokenizer Class
 ******************************************************************************/


class SjLineTokenizer
{
public:
	// constructor
	// if you give a char-buffer to the constructor, this buffer is NOT freed,
	// but MODIFIED (line ends to zeros)
	SjLineTokenizer     (const wxString&);
	//SjLineTokenizer   (wxChar* data, long dataBytes);
	~SjLineTokenizer    ();

	// get next line, line will be null-terminated.
	// as SjLineTokenizer treats both, "\n" and "\r" as lineends,
	// there may be MANY empty lines.
	// the returned line is trimmed at both ends by " " and "\t".
	// if there are no more lines, the function returns NULL.
	wxChar*         GetNextLine         (bool trimAtBeg=TRUE);

	// GetRest() returns all data up to the end of the buffer;
	// GetRest() may return NULL.
	wxChar*         GetRest             () { return m_nextLineStart; }

private:
	wxChar*         m_data;
	bool            m_freeData;
	wxChar*         m_nextLineStart;
};


class SjCfgTokenizer
{
public:
	// constructor - SjCfgTokenizer takes a string/a file content
	// and treats every line as a key=value pair; lines starting with a semicolon are comments.
					SjCfgTokenizer      () { InitIterator(); }

	// add keys
	void            AddFromString       (const wxString&);
	void            AddFromStream       (wxInputStream* s) { AddFromString(SjTools::GetFileContent(s, &wxConvUTF8)); }

	// clear
	void            Clear               () { m_hash.Clear(); m_keys.Clear(); m_values.Clear(); m_i = 0; }

	// lookup a key
	wxString Lookup              (const wxString& key, const wxString& defVal) const
	{
		wxString* val = m_hash.Lookup(key);
		if( val && !val->IsEmpty() )
		{
			return *val;
		}
		return defVal;
	}

	// iterate the keys in the correct order
	int             GetCount            () const { return m_keys.GetCount(); }
	void            InitIterator        () { m_i = 0; }
	bool            Iterate             (wxString& key, wxString& value) { if(m_i>=GetCount()) return false; key=m_keys[m_i]; value=m_values[m_i]; m_i++; return true; }


private:
	SjSSHash        m_hash;
	wxArrayString   m_keys;
	wxArrayString   m_values;
	int             m_i;
};


/*******************************************************************************
 * SjStringSerializer Class
 ******************************************************************************/


class SjStringSerializer
{
public:
	// serialize strings
	SjStringSerializer  () { m_hasErrors=FALSE; }
	void            AddString           (const wxString&);
	void            AddLong             (long);
	void            AddFloat            (float);
	wxString        GetResult           () const { return m_str; }

	// unserialize string
	SjStringSerializer  (const wxString& str);
	wxString        GetString           ();
	long            GetLong             ();
	float           GetFloat            ();
	// misc.
	bool            HasErrors           () const { return m_hasErrors; }

private:
	// private stuff
	wxString        m_str;
	wxArrayString	m_arr;
	bool            m_hasErrors;
};


/*******************************************************************************
 *  SjStrReplacer Class
 ******************************************************************************/


class SjStrReplacer
{
public:
	SjStrReplacer       () { m_compiled = FALSE; }
	SjStrReplacer       (const wxString& pattern, const wxString& replacement, long flags=0) { Compile(pattern, replacement, flags); }

	#define         wxRE_WHOLEWORDS     0x10000000L
	#define         wxRE_REGEX          0x20000000L
	//also defined: wxRE_ICASE          and others
	bool            Compile             (const wxString& pattern, const wxString& replacement, long flags=0);
	bool            IsValid             () const { return m_compiled; }
	int             ReplaceAll          (wxString& text, const wxString* replacement=NULL);

private:
	wxRegEx         m_regEx;
	wxString        m_replacement;
	bool            m_compiled;
	wxString        PrepareReplacement  (const wxString&) const;
};


/*******************************************************************************
 *  SjPlaceholdReplacer / SjPlaceholdMatcher Classes
 ******************************************************************************/


class SjPlaceholdReplacer
{
public:
	SjPlaceholdReplacer () { }
	virtual         ~SjPlaceholdReplacer() { }
	void            ReplaceAll          (wxString& text);

	// GetLastPlaceholder() returns the last, final placeholder,
	// if the text given to ReplaceAll() ends with a placeholder.
	// Otherwise the empty string is returned.
	wxString        GetFinalPlaceholder () const { return m_finalPlaceholder; }

protected:
	#define         SJ_PLR_WIDTH        0x000000FFL
	#define         SJ_PLR_MAKEUPPER    0x00000100L
	#define         SJ_PLR_MAKELOWER    0x00000200L
	virtual bool    GetReplacement      (const wxString& placeholder, long flags, wxString& replacement) = 0;
	wxString        ApplyFlags          (const wxString& replacement, long flags);
	wxString        ApplyFlags          (long replacement, long flags);

private:
	wxString        m_finalPlaceholder;
};


class SjPlaceholdPart
{
public:
	wxString    m_placeholder; // may be empty for the first part
	long        m_delimWidth;
	wxString    m_delimSepAfter;
};
WX_DECLARE_OBJARRAY(SjPlaceholdPart, SjArrayPlaceholdPart);


class SjPlaceholdMatcher
{
public:
	SjPlaceholdMatcher  () {}
	virtual         ~SjPlaceholdMatcher () {}
	void            Compile             (const wxString& pattern, const wxString& highPriorityDelim);
	bool            HasPlaceholder      (const wxString& placeholder);
	void            Match               (const wxString& haystack, bool shortExt);

protected:
	// GotMatch() should be implemented by derived classes,
	// the function should return TRUE if the text found is acceptable for the
	// given placeholder and FALSE otherwise (eg. if a number was expected and sth.
	// else is returned).  If FALSE is returned, the text will be added to the next
	// placeholder found.
	virtual bool    GotMatch            (const wxString& placeholder, const wxString& text) = 0;

private:
	SjArrayPlaceholdPart
	m_parts;
	wxString        m_highPriorityDelim;
	wxArrayString   m_goBack;
	int             m_goForward;
#ifdef __WXDEBUG__
	wxString       m_pattern;
#endif
};


class SjTrackInfo;


class SjTrackInfoMatcher : public SjPlaceholdMatcher
{
public:
	SjTrackInfoMatcher  ();
	void            Compile             (long tiFieldToSplit, const wxString& pattern);
	wxString        GetPattern          () const { return m_rawPattern; }
	static wxString GetDefaultPattern   ();

	// Match() sets the m_validFields bits for the information read from
	// the filename.
	void            Match               (SjTrackInfo& src, SjTrackInfo& dest);

protected:
	bool            GotMatch            (const wxString& placeholder, const wxString& text);

private:
	wxString        m_rawPattern;
	wxString        NormalizeUrl        (const wxString&);
	wxRegEx         m_slashReplacer;
	long            m_tiFieldToSplit;
	SjTrackInfo*    m_tiDest;
	bool            m_smartDiskNr;
	bool            m_underscoresToSpaces;
	bool            m_isCompiled;
};


/*******************************************************************************
 * SjProp Class
 ******************************************************************************/


class SjProp
{
public:
	                SjProp              () { Init(); }
	void            Init                ();

	#define         SJ_PROP_BOLD            0x10000000L
	#define         SJ_PROP_HEADLINE        0x20000000L
	#define         SJ_PROP_ID              0x40000000L // the lower 16 bit contain the ID
	#define         SJ_PROP_HTML            0x01000000L
	#define         SJ_PROP_EMPTYIFEMPTY    0x02000000L // show the grayed string <empty> if the value is empty; for longs, use the value 0 for "empty"
	void            Add                 (const wxString& name, const wxString& value, long flags=0);
	void            Add                 (const wxString& name, long value, long flags=0);
	void            AddHex              (const wxString& name, long value, long flags=0);
	void            AddBytes            (const wxString& name, long value, long flags=0);

	int             GetCount            () const     { return m_names.GetCount(); }
	void            InitIterator        ()           { m_iterator=-1; }
	bool            Next                ()           { m_iterator++; return m_iterator<GetCount(); }

	wxString        GetName             (int i=-1) const  { return m_names.Item(i==-1?m_iterator:i);  }
	wxString        GetValue            (int i=-1) const  { return m_values.Item(i==-1?m_iterator:i); }
	long            GetFlags            (int i=-1) const  { return m_flags.Item(i==-1?m_iterator:i);  }

private:
	wxArrayString   m_names;
	wxArrayString   m_values;
	wxArrayLong     m_flags;
	int             m_iterator;
};


/*******************************************************************************
 *  Hardware Graphic Stuff, Misc.
 ******************************************************************************/


// just to make calculations easier
#define SJ_ONE_GB 0x40000000L
#define SJ_ONE_MB 0x00100000L

// more mono-channels are not supported; these are 8 stereo channels which should be enough
// the max. is used eg. by the volume normalizer
#define SJ_WW_MAX_CH            16

// Do not change the constants if you don't REALLY know what you are doing!
// Internally, Silverjuke always calculates with these audio settings.
#define SJ_WW_FREQ              44100
#define SJ_WW_BYTERES           2
#define SJ_WW_CH                2
long    SjMs2Bytes          (long ms);


#endif /* __SJ_TOOLS_H__ */
