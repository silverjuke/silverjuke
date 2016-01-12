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
 * File:    tools.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke tools
 *
 *******************************************************************************
 *
 * Notes:
 * 11.10.2004   Improved SjOmitWords::Apply(): Spaces after omitted words are
 *              requiered now (avoid omitting "Dieter" to "ter, die"), some
 *              other minor improvements.
 * 14.01.2005   Using different subdirectories in temp. directory for different
 *              configurations/different databases.  This is needed as files
 *              are stored in temp eg. using IDs from the database.
 * 17.01.2005   Forgot to add the %-decoded character to the output buffer in
 *              SjTools::Urldecode()... added.
 * 18.01.2005   SjTools::FormatDate() returned the wrong time (-1h) because of
 *              a bug in wxDateTime::SetFromDOS() (InitTm() was missing, I
 *              added this; wxWidgets after 2.4.2 should not have this bug)
 * 10.07.2005   ExploreUrl() modified so that filenames with a '#' in the name
 *              are supported; also see my changes in "wxWidgets/src/common/
 *              filesys.cpp" as documented in "msw/README.TXT"
 * 14.08.2005   Surrounded the crash precaution writing process by a critical
 *              section (the crash precaution may be used from different
 *              threads; so opening the file failed from time to time)
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/stdpaths.h>
#include <wx/cmdline.h>
#include <wx/tokenzr.h>
#include <wx/fontenum.h>
#include <wx/fileconf.h>
#if wxCHECK_VERSION(2, 9, 2)
#include <wx/numformatter.h>
#endif
#include <sjtools/tools.h>
#include <sjtools/csv_tokenizer.h>
#include <tagger/tg_bytevector.h>
#include <sjmodules/help/help.h>


/*******************************************************************************
 *  SjTools: Constructor and Destructor
 ******************************************************************************/


SjTools*    g_tools = NULL;
int         g_debug = 0; // use debug output as wxASSERT()? can be enabled by `debug=1` in section `[main]` in `globals.ini`


SjTools::SjTools()
	: m_uptime(wxT("main/upTime"))
{
	// init some pointer
	wxASSERT( g_tools == NULL );
	g_tools = this;
	m_iconlistLoaded = FALSE;

	// create configuration object
	m_configIsDefault = TRUE;
	if( SjMainApp::s_cmdLine->Found(wxT("instance"), &m_configDescr) )
	{
		m_instance = SjNormaliseString(m_configDescr, 0);
		m_config = new wxFileConfig(SJ_PROGRAM_NAME, SJ_PROGRAM_NAME, m_configDescr);
		m_configIsDefault = FALSE;
	}
	else if( SjMainApp::s_cmdLine->Found(wxT("ini"), &m_configDescr) )
	{
		m_config = new wxFileConfig(SJ_PROGRAM_NAME, SJ_PROGRAM_NAME, m_configDescr);
		m_configIsDefault = FALSE;
	}
	#ifdef __WXMSW__
	else if( ::wxFileExists( GetSilverjukeProgramDir() + wxT("globals.ini")) ) // don't use "silverjuke.ini" which will be created by some (winamp) plugins
	{
		m_configDescr = GetSilverjukeProgramDir() + wxT("globals.ini");
		m_config = new wxFileConfig(SJ_PROGRAM_NAME, SJ_PROGRAM_NAME, m_configDescr);
		m_configIsDefault = FALSE;
	}
	#endif
	else
	{
		wxFileName fn(GetUserAppDataDir(), wxT("globals.ini"));
		fn.Normalize();
		m_configDescr = fn.GetFullPath();
		m_config = new wxFileConfig(SJ_PROGRAM_NAME, SJ_PROGRAM_NAME, m_configDescr);
	}

	wxLogInfo(wxT("Loading %s"), m_configDescr.c_str());

	if( m_config == NULL )
	{
		wxLogError(wxT("Cannot open configuration file \"%s\".")/*n/t*/, m_configDescr.c_str());
		SjMainApp::FatalError();
	}

	if( wxConfigBase::Get(FALSE) != m_config )
	{
		m_oldConfig = wxConfigBase::Set(m_config);
	}
	else
	{
		m_oldConfig = NULL;
	}

	// enable/disable debug
	g_debug = m_config->Read("main/debug", 0L);
	if( g_debug )
	{
		wxLogInfo(wxT("Debug enabled by globals.ini"));
		#if wxCHECK_VERSION(2, 9, 1)
			wxSetDefaultAssertHandler();
			if( g_debug&0x02 )
			{
				wxASSERT_MSG(0, "Just a Test assert");
			}
		#else
			wxLogWarning("Assert messages cannot be enabled, please use a more recent version of wxWidgets");
		#endif
	}
	else
	{
		#if wxCHECK_VERSION(2, 9, 1)
			wxSetAssertHandler(NULL);
		#endif
	}

	// set the desired instance name; the instance name is empty for the default instance
	// or defaults to the INI-file name
	{
		wxString explicitInstanceName = m_config->Read(wxT("main/instance"), wxT(""));
		if( !explicitInstanceName.IsEmpty() )
		{
			m_instance = explicitInstanceName;
		}
	}

	// init cache (should be initialized AFTER config)
	m_cache.Init();

	// get search paths (should be initialized BEFORE db)
	InitSearchPaths();

	// get db file (should be initialized AFTER search paths)
	m_dbFileIsDefault = FALSE;
	if( !SjMainApp::s_cmdLine->Found(wxT("jukebox"), &m_dbFile) )
	{
		m_dbFile = m_config->Read(wxT("main/jukebox"), wxT(""));
		if( m_dbFile.IsEmpty() )
		{
			wxFileName fn(GetSearchPath(0), wxT("default.jukebox"));
			fn.Normalize();
			m_dbFile = fn.GetFullPath();
			m_dbFileIsDefault = TRUE;
		}
	}

	// misc
	InitCrashPrecaution(); // relies on the search paths, ini and temp. files
	LoadStaticObjects();
	InitExplore();
	m_uptime.StartWatching();
}


SjTools::~SjTools()
{
	// exit possibly window shading routines
	UnloadWindowTransparencyLibs();

	// write uptime (must be done BEFORE deletin' m_config)
	m_uptime.StopWatching();

	// cleanup temp.
	// CleanupTempDir();

	// remove global pointer to the tools
	wxASSERT( g_tools );
	g_tools = NULL;

	// delete configutation object
	m_config->Flush();
	wxConfigBase::Set(m_oldConfig);
	delete m_config;
	m_config = NULL;

	// :-)
	NotCrashed(TRUE/*stopLogging*/);
}


/*******************************************************************************
 * SjTools: Crash Precaution
 ******************************************************************************/


void SjTools::InitCrashPrecaution()
{
	uint32_t crc = Crc32AddString(Crc32Init(), wxGetUserId()+m_instance);

	m_crashInfoFileName = m_cache.AddToUnmanagedTemp(
	                          wxString::Format(SJ_TEMP_PREFIX wxT("%08x-cp")
								#ifdef __WXDEBUG__
	                                  wxT("-db")
								#endif
	                                  , (int)crc)
	                      );

	if( ::wxFileExists(m_crashInfoFileName) )
	{
		wxFile file(m_crashInfoFileName, wxFile::read);

		char buffer[4096+1];
		buffer[file.Read(buffer, 4096)] = 0;

		#if wxUSE_UNICODE
			wxString info = wxString(buffer, wxConvUTF8);
		#else
			wxString info = buffer;
		#endif

		m_lastCrashModule = info.BeforeFirst('\n');
		m_lastCrashFunc   = info.AfterFirst('\n').BeforeLast('\n');
		m_lastCrashObject = info.AfterLast('\n');
	}
}


static bool s_doCrashLogging = TRUE;
void SjTools::NotCrashed(bool stopLogging)
{
	if( stopLogging )
	{
		s_doCrashLogging = FALSE;
	}

	m_crashPrecautionLocker.Enter();
	if( wxFileExists(m_crashInfoFileName) )
	{
		wxLogNull null;
		wxRemoveFile(m_crashInfoFileName);
	}
	m_crashPrecautionLocker.Leave();
}


void SjTools::ShowPossibleCrash()
{
	if( !m_lastCrashModule.IsEmpty()
	        || !m_lastCrashFunc.IsEmpty()
	        || !m_lastCrashObject.IsEmpty() )
	{
		// get readable object string
		wxString obj, info;

		obj.Printf(wxT("%s (%s)"), m_lastCrashModule.c_str(), m_lastCrashFunc.c_str());
		if( !m_lastCrashObject.IsEmpty() )
		{
			obj << wxT("\n") << m_lastCrashObject;
		}

		// get message
		info.Printf(_("Last time %s did not terminate normally.\nThe following - maybe errorous - objects were in use just before the abnormal termination:\n\n%s\n\nOn continuous problems, try to avoid using these objects.\nDo you want to use the objects this time?"),
		            SJ_PROGRAM_NAME, obj.c_str());

		info.Replace(wxT("\n\n\n"), wxT("\n\n"));

		// show message box -- don't use SjMessageBox, prefer a lower level
		if( ::wxMessageBox(info, _("Use maybe errorous objects?"),
		                   wxYES_NO | wxNO_DEFAULT | wxICON_WARNING) == wxYES )
		{
			m_lastCrashModule.Clear();
			m_lastCrashFunc.Clear();
			m_lastCrashObject.Clear();
		}
	}
}


bool SjTools::CrashPrecaution(const wxString& module, const wxString& func, const wxString& object)
{
	bool ret = TRUE;

	wxASSERT(module.IsEmpty()==FALSE);
	wxASSERT(func.IsEmpty()==FALSE);

	if( module == m_lastCrashModule
	 && func == m_lastCrashFunc
	 && object == m_lastCrashObject )
	{
		ret = FALSE; // the given objects are the possible reason for the last crash
	}

	#ifndef __WXDEBUG__
	if(  s_doCrashLogging
	 && !SjMainApp::IsInShutdown() ) // on shutdown, NotCrashed() may not be called in time if Windows kills us - so no crash precaution on shutdown
	#endif
	{
		wxString info = module;
		info << wxT("\n") << func << wxT("\n") << object;

		m_crashPrecautionLocker.Enter();

		{
			wxFile file(m_crashInfoFileName, wxFile::write);
			if( file.IsOpened() )
			{
				file.Write(info);
			}
		} // m_crashInfoFileName may be re-used from here on, note the "}"

		m_crashPrecautionLocker.Leave();
	}

	return ret;
}


/*******************************************************************************
 * SjTools: CRC and Mathemetical Stuff
 ******************************************************************************/


bool          SjTools::m_crc32InitDone = FALSE;
uint32_t      SjTools::m_crc32Table[256];


static unsigned long Crc32Reflect(uint32_t ref, unsigned char ch)
{
	uint32_t value = 0;
	int i;

	// Swap bit 0 for bit 7, bit 1 for bit 6, etc.
	for(i = 1; i < (ch + 1); i++)
	{
		if(ref & 1)
		{
			value |= 1 << (ch - i);
		}
		ref >>= 1;
	}
	return value;
}


uint32_t SjTools::Crc32Init()
{
	wxASSERT( sizeof(uint32_t) == 4 );

	if( !m_crc32InitDone )
	{
		// This is the official polynomial used by CRC-32 in PKZip, WinZip and Ethernet.
		#define CRC32POLYNOMIAL 0x04c11db7
		int i, j;

		// 256 values representing ASCII character codes.
		for(i = 0; i <= 0xFF; i++)
		{
			m_crc32Table[i]=Crc32Reflect(i, 8) << 24;
			for(j = 0; j < 8; j++)
			{
				m_crc32Table[i] = (m_crc32Table[i] << 1) ^ (m_crc32Table[i] & (1 << 31) ? CRC32POLYNOMIAL : 0);
			}
			m_crc32Table[i] = Crc32Reflect(m_crc32Table[i], 32);
		}

		m_crc32InitDone = TRUE;
	}
	return 0xFFFFFFFFL; // starting crc value
}


uint32_t SjTools::Crc32Add(uint32_t crc32, const char* buffer__, int bufferBytes)
{
	// add bytes to add?
	if( bufferBytes <= 0 )
	{
		return crc32;
	}

	// make sure, the buffer is unsigned
	wxASSERT(buffer__);
	const unsigned char* buffer = (const unsigned char*)buffer__;

	// Perform the algorithm on each character
	// in the string, using the lookup table values.
	wxASSERT(m_crc32InitDone);
	while( bufferBytes-- )
	{
		crc32 = (crc32 >> 8) ^ m_crc32Table[(crc32 & 0xFF) ^ *buffer++];
	}

	// Exclusive OR the result with the beginning value
	return crc32 ^ 0xFFFFFFFFL;
}


long SjTools::Rand(long n) // returns a value between 0 and n-1
{
	static long g_rand_initialized = 0;
	static long g_holdrand;

	if( g_rand_initialized == 0 )
	{
		g_holdrand = ::wxGetLocalTimeMillis().GetLo();
		g_rand_initialized = 1000;
	}

	g_rand_initialized--;

	return SjTools::PrivateRand(g_holdrand, n);
}


long SjTools::PrivateRand(long& holdrand, long n)
{
	if( n <= 1 )
	{
		return 0;
	}
	else if( n >= 0x7fff )
	{
		long t1 = (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
		long t2 = (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
		return (t1*t2) % n;
	}
	else
	{
		long t = (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
		return t % n;
	}
}


unsigned long SjTools::GetMsTicks()
{
	static wxLongLong s_firstCallMs = 0;
	if( s_firstCallMs == 0 )
	{
		s_firstCallMs = ::wxGetLocalTimeMillis();
	}

	wxLongLong currMs = ::wxGetLocalTimeMillis() - s_firstCallMs;

	unsigned long ret = (unsigned long)currMs.ToLong();
	if( ret == 0 )
	{
		ret = 1; // avoid timestamps of null!
	}

	return ret;
}


/*******************************************************************************
 * SjTools: Configuration
 ******************************************************************************/


// the following "#if 0" part is needed by poEdit
// to get the strings needed by LocaleConfigRead() into the database
#if 0
_("__DATE_TIME_LONG__")
_("__DATE_TIME_EDITABLE__")
_("__DATE_LONG__")
_("__DATE_EDITABLE__")
_("__DATE_SUNDAY_FIRST__")
_("__THIS_LANG__")
_("__COVER_SEARCH_URLS__")
_("__ARTIST_INFO_URLS__")
_("__STOP_ARTISTS__")
_("__STOP_ALBUMS__")
_("__COVER_KEYWORDS__")
_("__VIRT_KEYBD__")
#endif


wxString SjTools::LocaleConfigRead(const wxString& keyname, const wxString& def)
{
	wxString test = ::wxGetTranslation(keyname);
	return (test==keyname || test==wxT("0"))? def : test;
}


long SjTools::LocaleConfigRead(const wxString& keyname, long def)
{
	wxString test = ::wxGetTranslation(keyname);
	if( test==keyname )
	{
		return def;
	}
	else
	{
		long ret;
		if( test.ToLong(&ret, 10) ) // 'ret' is only valid if TRUE is returned
		{
			return ret;
		}
		else
		{
			return def;
		}
	}
}


wxRect SjTools::ReadRect(const wxString& key)
{
	return ParseRect(m_config->Read(key, wxT("")));
}


void SjTools::WriteRect(const wxString& key, const wxRect& r)
{
	m_config->Write(key, FormatRect(r));
}


wxArrayString SjTools::ReadArray(const wxString& key)
{
	wxArrayString ret;
	SjStringSerializer ser(m_config->Read(key, wxT("")));
	long i, count = ser.GetLong();
	if( count )
	{
		for( i = 0; i < count; i++ )
		{
			ret.Add(ser.GetString());
		}

		if( ser.HasErrors() )
		{
			ret.Clear();
		}
	}
	return ret;
}


void SjTools::WriteArray(const wxString& key, const wxArrayString& a)
{
	SjStringSerializer ser;
	long i, count = (long)a.GetCount();
	ser.AddLong(count);
	for( i = 0; i < count; i++ )
	{
		ser.AddString(a[i]);
	}
	m_config->Write(key, ser.GetResult());
}


int SjTools::ReadFromCmdLineOrIni(const wxString& key, wxString& ret)
{
	if( SjMainApp::s_cmdLine->Found(key, &ret) )
	{
		return 1; // got from command line
	}
	else
	{
		ret = m_config->Read(wxT("main/")+key, wxT(""));
		if( !ret.IsEmpty() )
		{
			return 2; // got from INI
		}
	}

	return 0; // can't read
}


/*******************************************************************************
 *  SjTools: Search Paths
 ******************************************************************************/


#ifndef __WXMSW__
wxString SjTools::GetUserAppDataDir()
{
	#ifdef __WXMAC__
		wxString userAppDataDir = wxStandardPaths::Get().GetUserDataDir();
		if( userAppDataDir.Last() != wxT('/') )
			userAppDataDir.Append(wxT('/'));
		if( !::wxDirExists(userAppDataDir) )
			::wxMkdir(userAppDataDir);
		return userAppDataDir;
	#else
		wxFileName tempFileName;
		if( wxGetenv(wxT("XDG_CONFIG_HOME")) ) // see http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
		{
			tempFileName = wxGetenv(wxT("XDG_CONFIG_HOME"));
		}
		else
		{
			tempFileName.AssignHomeDir();
			tempFileName.AppendDir(wxT(".config"));
		}
		tempFileName.AppendDir(wxT("silverjuke"));
		tempFileName.Normalize();
		if( !::wxDirExists(tempFileName.GetFullPath()) )
		{
			if( !tempFileName.Mkdir() )
			{
				wxLogError(wxT("Cannot create directory \"%s\"."), tempFileName.GetFullPath().c_str());
			}
		}

		return tempFileName.GetFullPath();
	#endif
}
#endif


wxString SjTools::GetGlobalAppDataDir()
{
	#if defined(__WXMSW__)
		wxString str = GetSilverjukeProgramDir();
		if( !str.IsEmpty() )
		{
			wxFileName fn(str);
			fn.Normalize();
			str = fn.GetFullPath();
		}
		return str;
	#elif defined(__WXMAC__)
		return SjTools::EnsureTrailingSlash(wxStandardPaths::Get().GetResourcesDir());
	#else
		// The constant PKGDATADIR gets defined on the
		// command line while running make. See Makefile.am
		return wxString(wxT(PKGDATADIR));
	#endif
}


void SjTools::InitSearchPaths()
{
	m_searchPaths.Clear();

	// load default search directories...
	//////////////////////////////////////////////////////////////////////

	// ...user home directory
	wxString str = GetUserAppDataDir();
	if( m_searchPaths.Index(str) == wxNOT_FOUND )
	{
		m_searchPaths.Add(str);
	}

	// ...directory shared by all users (eg. /usr/share/silverjuke or the program directory on windows)
	str = GetGlobalAppDataDir();
	if( m_searchPaths.Index(str) == wxNOT_FOUND )
	{
		m_searchPaths.Add(str);
	}

	// load user-defined search directories...
	//////////////////////////////////////////////////////////////////////

	m_searchPathsFirstUser = m_searchPaths.GetCount();

	int userIndex, userCount = m_config->Read(wxT("main/searchPathCount"), 0L);
	for( userIndex = 0; userIndex < userCount; userIndex++ )
	{
		wxString pathStr = m_config->Read(wxString::Format(wxT("main/searchPath%i"), userIndex));
		if( !pathStr.IsEmpty() )
		{
			wxFileName path(pathStr);
			path.Normalize();
			if( ::wxDirExists(path.GetFullPath())
			 && m_searchPaths.Index(path.GetFullPath()) == wxNOT_FOUND )
			{
				m_searchPaths.Add(path.GetFullPath());
			}
		}
	}
}


int SjTools::GetSearchPathIndex(const wxString& path) const
{
	int i, c = GetSearchPathCount();
	for( i = 0; i < c; i++ )
	{
		if( path.CmpNoCase(GetSearchPath(i)) == 0 )
		{
			return i;
		}
	}

	return -1;
}


/*******************************************************************************
 * SjTools: Drawing Objects that never change
 ******************************************************************************/


void SjTools::LoadStaticObjects()
{
	/* create resize cursors
	 */
	m_staticResizeNWSECursor = wxCursor(wxCURSOR_SIZENWSE);
	if( !m_staticResizeNWSECursor.IsOk() )
	{
		m_staticResizeNWSECursor = *wxSTANDARD_CURSOR;
	}

	m_staticResizeWECursor = wxCursor(wxCURSOR_SIZEWE);
	if( !m_staticResizeWECursor.IsOk() )
	{
		m_staticResizeWECursor = *wxSTANDARD_CURSOR;
	}

	m_staticNoEntryCursor = wxCursor(wxCURSOR_NO_ENTRY);
	if( !m_staticNoEntryCursor.IsOk() )
	{
		m_staticNoEntryCursor = *wxSTANDARD_CURSOR;
	}
}


/*******************************************************************************
 *  Other File Tools
 ******************************************************************************/


unsigned long SjTools::GetFileSize(const wxString& name)
{
	wxLogNull null;
	#if 0
		wxFile file(name);
		if( file.IsOpened() )
		{
			file.SeekEnd();
			return file.Tell();
		}
	#else
		wxStructStat buf;
		if( wxStat(name, &buf) == 0 )
		{
			return buf.st_size;
		}
	#endif
	return 0;
}


wxString SjTools::GetFileContent(wxInputStream* inputStream, wxMBConv* mbConv)
{
	wxString ret;

	long bytes = inputStream->GetSize();
	if( bytes <= 0 )
		bytes = 0x200000; // 2 MByte - just try this ... it may be an HTTP-stream

	if( bytes > 0 )
	{
		char* /*not: wxChar! */ buf__ = (char*)malloc(bytes + 1);
		char* buf = buf__;
		if( buf )
		{
			inputStream->Read(buf, bytes);
			bytes = inputStream->LastRead();
			buf[bytes  ] = 0;

			#if wxUSE_UNICODE
				if( ((unsigned char)buf[0]==0xFF && (unsigned char)buf[1]==0xFE)
						|| ((unsigned char)buf[0]==0xFE && (unsigned char)buf[1]==0xFF) )
				{
					// UTF-16LE or UTF-16BE BOM (Byte order mark) detected: Force UTF-16 decoding.
					// see http://www.silverjuke.net/forum/topic-3118.html
					SjByteVector v((const unsigned char*)buf, bytes);
					ret = v.toString(SJ_UTF16);
				}
				else
				{
					if( (unsigned char)buf[0]==0xEF && (unsigned char)buf[1]==0xBB && (unsigned char)buf[2]==0xBF )
					{
						// UTF-8 BOM (Byte order mark) detected: Remove the mark and force UTF-8 decoding
						buf += 3;
						bytes -= 3;
						mbConv = &wxConvUTF8;
					}

					ret = wxString(buf, *mbConv);
					if( ret.IsEmpty() )
					{
						// corrupted UTF-8? try again with Latin-1/ISO8859-1
						ret = wxString(buf, wxConvISO8859_1);
					}
				}
			#else
				ret = buf;
			#endif

			free(buf__);
		}
	}
	return ret;
}


wxString SjTools::GetFileNameFromUrl(const wxString& url, wxString* retPath, bool stripExtension, bool removeSepFromPath)
{
	// CAVE: the given URL may be a real URL, with protocol and escaped, _or_ a normal file name.
	// you should no longer use this function for new stuff, use eg. wxFileSystem::URLToFileName().GetFullName()
	// or wxFileSystem::URLToFileName().GetName() instead

	// to be compatible with different path seperators,
	// use the separator found last
	int i1  = url.Find(':', TRUE),
	    i2  = url.Find('/', TRUE),
	    i3  = url.Find('\\', TRUE);
	if( i2 > i1 ) i1 = i2;
	if( i3 > i1 ) i1 = i3;

	// also return the path?
	if( retPath )
	{
		wxASSERT(&url!=retPath);
		*retPath = url.Left(i1 + (removeSepFromPath? 0 : 1));
	}

	// strip the extension?
	if( stripExtension )
	{
		wxString fileName = url.Mid(i1+1);
		if( fileName.Find('.', TRUE) != -1 )
		{
			fileName = fileName.BeforeLast('.');
		}
		return fileName;
	}
	else
	{
		return url.Mid(i1+1);
	}
}


bool SjTools::AreFilesSame(const wxString& src__, const wxString& dest__)
{
	// rough check for urls
	if( src__ == dest__ )
		return true;

	// more complicated check for files that may be relative etc.
	wxFileName srcName(src__);
	wxFileName destName(dest__);

	srcName.Normalize();
	destName.Normalize();

	return ( srcName.GetFullPath()==destName.GetFullPath());
}


bool SjTools::CopyFile(const wxString& srcName, const wxString& destName)
{
	wxFileSystem    fs;
	wxFSFile*       srcFile = fs.OpenFile(srcName);
	if( !srcFile )
	{
		wxLogError(_("Cannot open \"%s\"."), srcName.c_str());
		return FALSE;
	}

	wxFile          destFile;
	destFile.Create(destName, TRUE/*overwrite*/);
	if( !destFile.IsOpened() )
	{
		wxLogError(_("Cannot write \"%s\"."), destName.c_str());
		return FALSE;
	}

	bool ret = CopyStreamToFile(*(srcFile->GetStream()), destFile);
	delete srcFile;
	return ret;
}


bool SjTools::CopyStreamToFile(wxInputStream& inputStream, wxFile& outputFile)
{
	bool            ret = FALSE;
	char*           buffer = NULL;
	unsigned long   endMs;

	// allocate memory
	#define TEMP_FILE_SLICE_BYTES   4*65536
	#define TEMP_FILE_MAX_COPY_MS   6000 // this should be enough -- streams may never end...
	buffer = (char*)malloc(TEMP_FILE_SLICE_BYTES);
	if( buffer == NULL )
	{
		wxLogError(wxT("CopyStreamToFile: Out of memory."));
		goto Cleanup;
	}

	if( !outputFile.IsOpened() )
	{
		wxLogError(wxT("CopyStreamToFile: Output file not opened."));
		goto Cleanup;
	}

	// copy!
	endMs = SjTools::GetMsTicks() + TEMP_FILE_MAX_COPY_MS;
	if( wxThread::IsMain() )
	{
		::wxBeginBusyCursor();
	}

	while( !inputStream.Eof()
	        && SjTools::GetMsTicks() < endMs )
	{
		inputStream.Read(buffer, TEMP_FILE_SLICE_BYTES);
		size_t lastRead = inputStream.LastRead();
		if( lastRead <= 0
		        || outputFile.Write(buffer, lastRead) != lastRead )
		{
			break;
		}
	}

	if( wxThread::IsMain() )
	{
		::wxEndBusyCursor();
	}

	// success
	ret = TRUE;

	// Cleanup
Cleanup:
	if( buffer ) free(buffer);
	return ret;
}


wxString SjTools::GetExt(const wxString& str)
{
	if( str.Find('.', TRUE/*from end*/) == -1 )
	{
		return wxEmptyString;
	}

	wxString ext = str.AfterLast(wxT('.')).Lower();
	if( ext.Len() > 6 )
	{
		ext.Clear();
	}

	return ext;
}


wxString SjTools::EnsureValidFileNameChars(const wxString& name__)
{
	wxString ret = name__;

	// replace some characters
	{
		static const wxChar forbiddenChars[] = SJ_FILENAME_FORBIDDEN;
		static const wxChar replacementChars[] = SJ_FILENAME_FORBIDDEN_REPLACE;
		wxASSERT( wxStrlen(forbiddenChars) == wxStrlen(replacementChars) );

		wxChar  currSearch[4];
		wxChar  currReplace[4];
		int     i;
		for( i = wxStrlen(forbiddenChars)-1; i >= 0; i-- )
		{
			currSearch[0]   = forbiddenChars[i];
			currSearch[1]   = 0;
			currReplace[0]  = replacementChars[i];
			currReplace[1]  = 0;
			ret.Replace(currSearch,  currReplace);
		}

		// remove sequences of the replacement characters
		// -- as most replacements should be "_", we skip this part as
		// -- the user might to know where and how many characters are skipped.
		/*for( i = wxStrlen(replacementChars)-1; i >= 0; i-- )
		{
		    currSearch[0]   = replacementChars[i];
		    currSearch[1]   = replacementChars[i];
		    currSearch[2]   = 0;
		    currReplace[0]  = replacementChars[i];
		    currReplace[1]  = 0;
		    while( ret.Find(currSearch) != -1 )
		    {
		        ret.Replace(currSearch, currReplace);
		    }
		}*/
	}

	// truncate too long names, but preserve the extension
	if( ret.Len() > SJ_FILENAME_MAX_LEN )
	{
		wxString ext = GetExt(ret);
		if( !ext.IsEmpty() )
		{
			ret = ret.BeforeLast(wxT('.'));
			ext.Prepend(wxT('.'));
		}

		ret.Truncate(SJ_FILENAME_MAX_LEN - ext.Length());

		if( !ext.IsEmpty() )
		{
			ret += ext;
		}
	}

	return ret;
}


wxString SjTools::EnsureValidPathChars(const wxString& path)
{
	// to be compatible with different path seperators,
	// allow any separators of "/" or "\\"
	wxString ret, currPart, currSep;
	wxStringTokenizer tkz(path, wxT("/\\"), wxTOKEN_RET_DELIMS);
	int      pathCount = 0;

	while( tkz.HasMoreTokens() )
	{
		currPart = tkz.GetNextToken();

		currSep.Empty();
		if( currPart.Len() )
		{
			currSep = currPart.Right(1);
			if( currSep == wxT("/") || currSep == wxT("\\") )
			{
				currPart.Truncate(currPart.Len()-1);
			}
			else
			{
				currSep.Empty();
			}
		}

		if( currPart.Len() && currPart.Right(1)==wxT(":") && pathCount==0 )
		{
			currPart.Truncate(currPart.Len()-1);
			currSep.Prepend(wxT(':'));
		}

		ret.Append(EnsureValidFileNameChars(currPart));
		ret.Append(currSep);

		pathCount++;
	}

	return ret;
}


wxString SjTools::EnsureTrailingSlash(const wxString& path)
{
	// is there already a trailing slash?
	if( path.Last() == '/'
	#if __WXMSW__
	 || path.Last() == '\\'
	#endif
	)
	{
		return path;
	}

	// add a slash, for MSW, we add a forward slash if the path already contains one
	#if __WXMSW__
		if( path.Find('/') != wxNOT_FOUND )
		{
			return path + "/";
		}
		return path + "\\";
	#else
		return path + "/";
	#endif
}


/*******************************************************************************
 * SjTools: String Tools
 ******************************************************************************/


wxString SjTools::Capitalize(const wxString& src)
{
	wxString dest;
	int i, iCount = (int)src.Len();
	wxChar c;
	bool nextLower = false;
	for( i = 0; i < iCount; i++ )
	{
		c = src[i];
		if( wxIsalpha(c) )
		{
			c = nextLower? wxTolower(c) : wxToupper(c);
			dest.Append(c);
			nextLower = true;
		}
		else
		{
			dest.Append(c);
			nextLower = false;
		}
	}
	return dest;
}


wxString SjTools::GetLineBreak()
{
	return wxT("\r\n");
}


wxString SjTools::FormatNumber(long number)
{
#if wxCHECK_VERSION(2, 9, 2)
	// There's a handy class since 2.9.2
	return wxNumberFormatter::ToString(number, wxNumberFormatter::Style_WithThousandsSep);
#else
	// negative number?
	bool isNegative = number<0? TRUE : FALSE;
	if( isNegative )
	{
		number = number * -1;
	}

	// get number as string
	wxString ret;
	wxString rest = wxString::Format(wxT("%i"), (int)number);
	wxString right;

	// format number, eg. "10000" becomes "10,000" or "10.000"
	int iteration = 0;
	while( !rest.IsEmpty() )
	{
		right = rest.Right(3);
		rest = rest.Left(rest.Len()-right.Len());
		// TRANSLATORS: This is the thousands separator, used for e.g. 10,000
		if( !ret.IsEmpty() ) right += _(",");
		ret = right + ret;
		iteration++;
		if( iteration > 20 )
		{
			return wxT("**iteration error**");
		}
	}

	// apply negation
	if( isNegative )
	{
		ret = wxString::Format(wxT("-%s"), ret.c_str());
	}

	// done
	return ret;
#endif // wxCHECK_VERSION
}


wxString SjTools::FormatNumbers(const wxArrayLong& numbers, long addToAllNumbers)
{
	wxString ret;
	long i, iCount = (long)numbers.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		if( !ret.IsEmpty() ) ret += wxT(", ");
		ret += wxString::Format(wxT("%i"), (int)(numbers[i]+addToAllNumbers));
	}
	return ret;
}


bool SjTools::ParseNumber(const wxString& str__, long* retNumber)
{
	wxString str(str__);
	str.Replace(wxT(" "), wxT(""));
	str.Replace(wxT("."), wxT(""));
	str.Replace(wxT(","), wxT(""));

	long dummy;
	if( retNumber == NULL ) retNumber = &dummy;

	if( str.ToLong(retNumber, 10) )
	{
		return TRUE;
	}
	else
	{
		*retNumber = 0;
		return FALSE;
	}
}


wxString SjTools::FormatStdFloat(float f)
{
	// returns an non-localized string, ready to save to INI etc.
	wxString ret = wxString::Format(wxT("%f"), f);
	ret.Replace(wxT(","), wxT("."));

	// remove trailing zeros
	if( ret.Find('.') != -1 )
	{
		int retLen;
		while( 1 )
		{
			retLen = ret.Len();
			if( retLen < 2 )
				break;

			if( ret[retLen-1]==wxT('0') && ret[retLen-2]!=wxT('.') )
				ret = ret.Left(retLen-1);
			else
				break;
		}
	}

	return ret;
}


float SjTools::ParseFloat(const wxString& sOrg)
{
	double f;
	if( !sOrg.ToDouble(&f) )
	{
		wxString sTemp(sOrg);
		sTemp.Replace(wxT("."), wxT(","));  // internally, we use "." as a decimal point, however, the loaded localed
		if( !sTemp.ToDouble(&f) )   // may expect a ","
		{
			f = 1.0;
		}
	}
	return (float)f;
}


wxRect SjTools::ParseRect(const wxString& str__)
{
	wxRect r;
	wxString str(str__);
	if( !str.IsEmpty() )
	{
		long l;
		if( str.BeforeFirst(wxT(',')).ToLong(&l) ) { r.x = l; }     str = str.AfterFirst(wxT(','));
		if( str.BeforeFirst(wxT(',')).ToLong(&l) ) { r.y = l; }     str = str.AfterFirst(wxT(','));
		if( str.BeforeFirst(wxT(',')).ToLong(&l) ) { r.width = l; } str = str.AfterFirst(wxT(','));
		if( str.ToLong(&l) )                       { r.height = l; }
	}
	return r;
}


wxString SjTools::FormatRect(const wxRect& r)
{
	return wxString::Format(wxT("%i,%i,%i,%i"),
	                        (int)r.x, (int)r.y, (int)r.width, (int)r.height);
}


wxString SjTools::FormatBytes(long val__, int flags)
{
	wxString    ret;

	if( flags & SJ_FORMAT_MB )
	{
		// the given value are already MEGABYTES ...

		long mbytes = val__;
		long gbytes = mbytes / 1024;
		if( gbytes )
		{
			mbytes -= gbytes * 1024;
			ret = FormatNumber(gbytes);
			if( mbytes )
			{
				// TRANSLATORS: This is the decimal point, used for e.g. 3.1415926
				ret += _(".");

				long temp = (mbytes+51)/103;
				if( temp > 9 ) temp = 9;

				ret += wxString::Format(wxT("%i"), (int)temp);
			}

			ret += wxT(" GB");
		}
		else
		{
			ret = wxString::Format(wxT("%s MB"), FormatNumber(mbytes).c_str());
		}
	}
	else
	{
		// the given value are BYTES ...

		long bytes = val__, mbytes, kbytes;

		if( bytes >= 0x80000L /*1/2 MB*/ )
		{
			mbytes = bytes / 0x100000L;
			bytes -= mbytes * 0x100000L;
			kbytes = (bytes+512)/1024;

			ret = FormatNumber(mbytes);
			if( kbytes || (flags & SJ_FORMAT_ADDEXACT /*"1.0" is more exact than just "1"!*/) )
			{
				// TRANSLATORS: This is the decimal point, used for e.g. 3.1415926
				ret += _(".");

				long temp = (kbytes+51)/103;
				if( temp > 9 ) temp = 9;

				ret += wxString::Format(wxT("%i"), (int)temp);
			}

			ret += wxT(" MB");
		}
		else if( bytes >= 1024 /*1 KB*/ )
		{
			kbytes = (bytes+512)/1024;
			ret  = FormatNumber(kbytes);
			ret += wxT(" KB");
		}
		else
		{
			flags &= ~SJ_FORMAT_ADDEXACT;
			ret  = FormatNumber(bytes);
			ret += wxT(" Byte");
		}

		if( flags & SJ_FORMAT_ADDEXACT )
		{
			ret += wxString::Format(wxT(" (%s Byte)"), FormatNumber(val__).c_str());
		}
	}

	return ret;
}


wxString SjTools::FormatTime(long seconds, long flags)
{
	long        minutes = seconds / 60;
	long        hours, days;
	wxString    ret;

	if( seconds < 0 )
	{
		// unknown time
		ret = wxT("?:??");
	}
	else if( seconds == 0 )
	{
		// unknown or zero time
		ret = (flags&SJ_FT_ALLOW_ZERO)? wxT("0:00") : wxT("?:??");
	}
	else if( minutes > (48*60 + 59) )
	{
		// format time as "DDd:HH:MM:SS" (no maximum)
		seconds -= minutes * 60;

		hours = minutes / 60;
		minutes -= hours * 60;

		days = hours / 24;
		hours -= days * 24;

		wxASSERT( days >= 2 && hours >= 0 && hours <= 23 && minutes >= 0 && minutes <= 59 && seconds >= 0 && seconds <= 59 );
		// TRANSLATORS: %i will be replaced by a number
		ret.Printf(wxPLURAL("%i day", "%i days", days), (int)days);
		ret += wxString::Format(wxT("+%i:%02i:%02i"), (int)hours, (int)minutes, (int)seconds);
	}
	else if( seconds > (99*60 + 59) )
	{
		// format time as "HH:MM:SS" (max. 48:59:00)
		seconds -= minutes * 60;

		hours = minutes / 60;
		minutes -= hours * 60;

		wxASSERT( hours >= 1 && hours <= 48 && minutes >= 0 && minutes <= 59 && seconds >= 0 && seconds <= 59 );
		ret = wxString::Format(wxT("%i:%02i:%02i"), (int)hours, (int)minutes, (int)seconds);
	}
	else
	{
		// format time as "MM:SS" (max. 99:59)
		seconds -= minutes * 60;
		wxASSERT( minutes >= 0 && minutes <= 99 && seconds >= 0 && seconds <= 59 );
		ret = wxString::Format(wxT("%i:%02i"), (int)minutes, (int)seconds);

		if( flags&SJ_FT_MIN_5_CHARS )
		{
			// try to return only 5 characters
			if( ret.Length() < 5 && !(flags&SJ_FT_PREPEND_MINUS) )
				ret.Prepend(wxT("0"));
		}
	}

	// prepend a minus
	if( (flags&SJ_FT_PREPEND_MINUS) )
	{
		ret.Prepend(wxT("-"));
	}

	return ret;
}


bool SjTools::ParseTime(const wxString& str__, long* retSeconds)
{
	long minutes, seconds = 0;
	wxString str(str__);
	str.Replace(wxT("'"), wxT(":"));
	str.Replace(wxT(" "), wxT(""));

	if( str.Find(':')==-1
	 && str.ToLong(&minutes, 10)
	 && minutes >= 0 )
	{
		if( retSeconds ) { *retSeconds = minutes*60; }
		return TRUE;
	}

	if( str.BeforeFirst(wxT(':')).ToLong(&minutes, 10)
	 && minutes >= 0
	 && str.AfterFirst (wxT(':')).ToLong(&seconds, 10)
	 && seconds >= 0 && seconds <= 59 )
	{
		if( retSeconds ) { *retSeconds = minutes*60 + seconds; }
		return TRUE;
	}

	if( retSeconds ) { *retSeconds = 0; }
	return FALSE;
}


wxString SjTools::FormatMs(unsigned long ms)
{
	if( ms < 30000 )
	{
		return FormatNumber(ms) + wxString(wxT(" ")) + _("ms");
	}
	else
	{
		return FormatTime(ms/1000);
	}
}


wxString SjTools::FormatDecibel(double db, bool addPercent)
{
	if( addPercent )
	{
		double gain = ::SjDecibel2Gain(db);
		return wxString::Format(wxT("%+.1f dB (%i%%)"), db, (int)(gain*100.0F+0.5));
	}
	else
	{
		return wxString::Format(wxT("%+.1f dB"), db);
	}
}


wxString SjTools::FormatGain(double gain, bool addPercent)
{
	double db = ::SjGain2Decibel(gain);
	if( addPercent )
	{
		return wxString::Format(wxT("%+.1f dB (%i%%)"), db, (int)(gain*100.0F+0.5));
	}
	else
	{
		return wxString::Format(wxT("%+.1f dB"), db);
	}
}


bool SjTools::ParseDecibel(const wxString& s__, double& ret)
{
	wxString s(s__);
	s = s.BeforeFirst(wxT(' '));
	s = s.BeforeFirst(wxT('d') /*the "d" of "dB"*/);
	if( !s.ToDouble(&ret) )
	{
		s.Replace(wxT("."), wxT(","));  // internally, we use "." as a decimal point, however, the loaded localed
		if( !s.ToDouble(&ret) ) // may expect a ","
		{
			return false;
		}
	}

	if( ret < -12.0F || ret > +12.0F )
	{
		return false;
	}

	return true;
}


wxString SjTools::FormatDate(unsigned long timestamp, long flags)
{
	if( timestamp == 0 )
	{
		return _("n/a");
	}
	else
	{
		wxDateTime dateTime;
		dateTime.SetFromDOS(timestamp);

		wxString mask;

		if( flags & SJ_FORMAT_ADDTIME )
		{
			if( flags & SJ_FORMAT_EDITABLE )
			{
				mask = LocaleConfigRead(wxT("__DATE_TIME_EDITABLE__"), wxT("%d.%m.%Y %H:%M:%S"));
			}
			else
			{
				mask = LocaleConfigRead(wxT("__DATE_TIME_LONG__"), wxT("%a %b %d %Y, %I:%M:%S %p"));
			}
		}
		else
		{
			if( flags & SJ_FORMAT_EDITABLE )
			{
				mask = LocaleConfigRead(wxT("__DATE_EDITABLE__"), wxT("%d.%m.%Y"));
			}
			else
			{
				mask = LocaleConfigRead(wxT("__DATE_LONG__"), wxT("%a %b %d %Y"));
			}
		}

		return dateTime.Format(mask);
	}
}


bool SjTools::ParseYear(const wxString& str__, long* retYear)
{
	wxString str(str__);
	str.Replace(wxT(" "), wxT(""));
	if( retYear ) *retYear = 0;
	if( str.Len()==2 || str.Len()==4 )
	{
		long year;
		if( ParseNumber(str, &year) )
		{
			return ParseYear(year, retYear);
		}
	}
	return FALSE;
}
bool SjTools::ParseYear(long year, long* retYear)
{
	if( year < 0 )
	{
		return FALSE;
	}

	if( year <= 99 )
	{
		long thisYear = wxDateTime::Today().GetYear();
		year = (2000+year<=thisYear)? (2000+year) : (1900+year);
	}

	if( year > 2100 || year < 1000 )
	{
		return FALSE;
	}

	if( retYear )
	{
		*retYear = year;
	}

	return TRUE;
}


bool SjTools::ParseDate_(const wxString& str__, bool keepItSimple,
                         wxDateTime* retDateTime, bool* retTimeSet)
{
	// the function parses a date in the form:
	//
	//      dd.mm.yyyy [hh:mm:ss] [oo]
	//
	// where "oo" is an offset in days (if time is not given) or seconds (if time is given).
	// Moreover, instead of "dd.mm.yyyy" the special values "today", "yesterday" or "now"
	// can be used.
	//
	// if "keepItSimple" is set, offsets and special dates plus time are NOT valid

	// get all given values into an array
	wxString str = str__.Lower().Trim(TRUE).Trim(FALSE);
	wxStringTokenizer tkz(str, wxT(".:/' "), wxTOKEN_STRTOK/*no empty tokens*/);
	long values[7], valueCount = 0;
	while( tkz.HasMoreTokens() && valueCount < 7)
	{
		if( !tkz.GetNextToken().ToLong(&values[valueCount], 10) ) { values[valueCount] = 0; }
		valueCount++;
	}

	// read the values and create the date, the optional time and the optional offset
	wxDateTime              dateTime;
	bool                    dateSet = FALSE, timeSet = FALSE, isSpecial = FALSE;
	long                    i = 0;

	// read special relative dates and times
	if( values[i] == 0 )
	{
		if( str.StartsWith(wxT("today")) )
		{
			dateTime = wxDateTime::Today();
		}
		else if( str.StartsWith(wxT("yesterday")) )
		{
			dateTime = wxDateTime::Today();
			dateTime.Subtract(wxTimeSpan::Days(1));
		}
		else if( str.StartsWith(wxT("now")) && !keepItSimple )
		{
			dateTime = wxDateTime::Now();
			timeSet = TRUE;
		}
		else
		{
			return FALSE;
		}

		dateSet = TRUE;
		isSpecial = TRUE;
		i++;
	}

	// read absolute date
	if( !dateSet )
	{
		long day, month, year;
		if( str__.Find('/')==-1 )
		{
			day     = values[i];    // format as "dd.mm.yyyy"
			month   = values[i+1];
		}
		else
		{
			month   = values[i];    // format as "mm/dd/yyyy"
			day     = values[i+1];
		}

		if( day < 1 || day > 31 || month < 1 || month > 12
		        || !ParseYear(values[i+2], &year) )
		{
			return FALSE;
		}

		dateTime = wxDateTime(day, (wxDateTime::Month)(month-1), year);
		if( !dateTime.IsValid() )
		{
			return FALSE;
		}

		dateSet = TRUE;
		i += 3;
	}

	// read the time
	if( !timeSet && valueCount-i >= 3 )
	{
		long hour, minute, second;
		hour        = values[i];
		minute      = values[i+1];
		second      = values[i+2];

		if( hour < 0 || hour > 23
		        || minute < 0 || minute > 59
		        || second < 0 || second > 59
		        || (keepItSimple && isSpecial) )
		{
			return FALSE;
		}

		dateTime.SetHour(hour);
		dateTime.SetMinute(minute);
		dateTime.SetSecond(second);
		timeSet = TRUE;
		i += 3;
	}

	// apply offset
	if( !keepItSimple && valueCount-i >= 1 )
	{
		if( timeSet )
		{
			dateTime.Add(wxTimeSpan::Minutes(values[i]));
		}
		else
		{
			dateTime.Add(wxTimeSpan::Days(values[i]));
		}

		i += 1;
	}

	// any values left -> error
	if( valueCount-i != 0 )
	{
		return FALSE;
	}

	// success
	if( retDateTime )   *retDateTime = dateTime;
	if( retTimeSet  )   *retTimeSet  = timeSet;

	return TRUE;
}


wxString SjTools::Urlencode(const wxString& in, bool encodeAsUtf8)
{
	const wxChar*   i = static_cast<const wxChar*>(in.c_str());
	wxChar*         o_base = (wxChar* )malloc((in.Len()+1)*3*10*sizeof(wxChar) /*worst case*/); if( o_base == NULL ) { return wxEmptyString; }
	wxChar*         o = o_base;
	wxChar          buffer2[32];
	wxString        temp;

	while( *i )
	{
		if( (*i>=wxT('a') && *i<=wxT('z'))
		        || (*i>=wxT('A') && *i<=wxT('Z'))
		        || (*i>=wxT('0') && *i<=wxT('9'))
		        ||  *i==wxT('-')
		        ||  *i==wxT('_')
		        ||  *i==wxT('.') )
		{
			*o++ = *i;
		}
		else if( *i == wxT(' ') )
		{
			*o++ = wxT('+');
		}
		else if( *i > wxT(' ') )
		{
			if( encodeAsUtf8 )
			{
				temp.Printf(wxT("%c"), *i);
				const wxCharBuffer      utf8Buf = temp.mb_str(wxConvUTF8);
				const unsigned char*    utf8Ptr = (unsigned char*)utf8Buf.data();
				while( *utf8Ptr )
				{
					wxSprintf(buffer2, wxT("%02X"), (int)*utf8Ptr);
					*o++ = wxT('%');
					*o++ = buffer2[0];
					*o++ = buffer2[1];
					utf8Ptr ++;
				}
			}
			else
			{
				wxSprintf(buffer2, wxT("%02X"), (int)*i);
				*o++ = wxT('%');
				*o++ = buffer2[0];
				*o++ = buffer2[1];
			}
		}

		i++;
	}

	*o = 0;
	wxString out(o_base);
	free(o_base);
	return out;
}


static int hexChar2Int(wxChar c)
{
	     if( c >= wxT('0') && c<=wxT('9') ) { return c-wxT('0');    }
	else if( c >= wxT('a') && c<=wxT('f') ) { return c-wxT('a')+10; }
	else if( c >= wxT('A') && c<=wxT('F') ) { return c-wxT('A')+10; }
	else                                    { return 0;             }
}


wxString SjTools::Urldecode(const wxString& in) // always expects UTF-8
{
	const wxChar*   i = static_cast<const wxChar*>(in.c_str());
	unsigned char*  o_base = (unsigned char*)malloc((in.Len()+1)/*worst case*/); if( o_base == NULL ) { return wxEmptyString; }
	unsigned char*  o = o_base;
	int             v;

	while( *i )
	{
		if( *i == wxT('%') )
		{
			if( *++i )
			{
				v = hexChar2Int(*i)<<4;
				if( *++i )
				{
					v |= hexChar2Int(*i++);
					*o++ = (unsigned char)v;
				}
			}
		}
		else if( *i == wxT('+') )
		{
			*o++ = ' ';
			i++;
		}
		else
		{
			*o++ = (unsigned char)*i++;
		}

	}

	*o = 0;
	wxString out((const char*)o_base, wxConvUTF8);
	free(o_base);
	return out;
}


wxString SjTools::Htmlentities(const wxString& str__)
{
	wxString str(str__);

	str.Replace(wxT("&"), wxT("&amp;")); // must be first
	str.Replace(wxT("<"), wxT("&lt;"));
	str.Replace(wxT(">"), wxT("&gt;"));
	str.Replace(wxT("\""), wxT("&quot;"));

	return str;
}


wxString SjTools::Menuencode(const wxString& str__)
{
	#ifdef __WXMSW__
		// In menus on Windows, a simple ampersand is used to underline the next character
		// (eg. "&File" or "&Open..."). To avoid this and to display a single "&", the
		// ampersand must be escaped by another ampersand.
		wxString str(str__);

		str.Replace(wxT("&"), wxT("&&"));

		return str;
	#else
		return str__;
	#endif
}


wxString SjTools::GetLastDir(const wxString& path)
{
	wxFileName fn(path);
	return fn.GetFullName();
}


wxString SjTools::ShortenUrl(const wxString& url, long maxChars)
{
	wxString ret = url;

	// convert file:-URLs to local filename
	if( ret.Left(5)=="file:" )
	{
		ret = wxFileSystem::URLToFileName(ret).GetFullPath();
	}

	// preprocess
	bool hasBackslashes = FALSE;
	if( ret.Find('\\')!=-1 )
	{
		hasBackslashes = TRUE;
	}
	ret.Replace(wxT("\\"), wxT("/"));

	// tokenize the string
	wxArrayString dirs;
	wxStringTokenizer tkz(ret, wxT("/"));
	while (tkz.HasMoreTokens() )
	{
		dirs.Add(tkz.GetNextToken());
	}
	long dirsCount = dirs.GetCount();

	// get the result
	if( dirs.GetCount() > 3 && maxChars < (long)url.Len() )
	{
		#if 0
			ret = dirs.Item(0) << wxT("/") << dirs.Item(1) << wxT("/.../") << dirs.Item(dirsCount-1);
		#else
			wxString left        = dirs.Item(0) << wxT("/") << dirs.Item(1);
			long     nextLeft    = 2;
			wxString right       = dirs.Item(dirsCount-1);
			long     nextRight   = dirsCount-2;
			bool     nextIsRight = TRUE;
			while( nextLeft  <  dirsCount
					&& nextRight >= 0 )
			{
				bool sthAdded = FALSE;
				long test = 0, newLen;
				while( !sthAdded && test++ < 2 )
				{
					if( nextIsRight )
					{
						newLen = left.Len()+right.Len()+dirs.Item(nextRight).Len();
						if( newLen <= maxChars )
						{
							right.Prepend(wxT("/"));
							right.Prepend(dirs.Item(nextRight));
							nextRight--;
							sthAdded = TRUE;
						}

					}
					else
					{
						newLen = left.Len()+right.Len()+dirs.Item(nextLeft).Len();
						if( newLen <= maxChars )
						{
							left.Append(wxT("/"));
							left.Append(dirs.Item(nextLeft));
							nextLeft++;
							sthAdded = TRUE;
						}
					}
					nextIsRight = !nextIsRight;
				}

				if( !sthAdded )
				{
					break;
				}
			}

			ret = left + (nextLeft <= nextRight? wxT("/.../") : wxT("/")) + right;
		#endif
	}

	// postprocess
	if( hasBackslashes )
	{
		ret.Replace(wxT("/"), wxT("\\"));
	}
	return ret;
}


bool SjTools::ReplaceNonISO88591Characters(wxString& in, wxChar replacement)
{
	if( replacement > 0xFF ) {
		replacement = wxT('?');
	}

	bool            sthReplaced = false;

	const wxChar*   i = static_cast<const wxChar*>(in.c_str());
	wxChar*         o_base = (wxChar*)malloc((in.Len()+1)*sizeof(wxChar)); if( o_base == NULL ) { in = replacement; return true; }
	wxChar*         o = o_base;
	while( *i )
	{
		if( *i > 0xFF )
		{
			*o = replacement;
			sthReplaced = true;
		}
		else
		{
			*o = *i;
		}
		i++;
		o++;
	}
	*o = 0;
	wxString out(o_base);
	free(o_base);
	in = out;
	return sthReplaced;
}


#if 0
wxArrayString SjTools::CreateArrayString(const char* query, ...)
{
	wxArrayString ret;
	va_list args;
	char* curr;
	va_start(args, query);

	ret.Add(query);

	while( (curr=va_arg(args, char*)) != NULL )
	{
		ret.Add(curr);
	}

	va_end(args);
	return ret;
}
#endif


wxArrayString SjTools::Explode(const wxString& str, wxChar delims, long minRetItems, long maxRetItems)
{
	wxArrayString       ret;
	wxStringTokenizer   tkz(str, delims, wxTOKEN_RET_EMPTY_ALL);

	while( tkz.HasMoreTokens() )
	{
		if( (long)ret.GetCount() == maxRetItems )
		{
			return ret;
		}

		ret.Add(tkz.GetNextToken());
	}

	while( (long)ret.GetCount() < minRetItems )
	{
		ret.Add(wxT(""));
	}

	return ret;
}


wxArrayLong SjTools::ExplodeLong(const wxString& str, wxChar delims, long minRetItems, long maxRetItems)
{
	wxArrayString arrStr = Explode(str, delims, minRetItems, maxRetItems);
	wxArrayLong arrLong;
	long i, iCount = arrStr.GetCount(), curr;
	for( i = 0; i < iCount; i++ )
	{
		if( !arrStr[i].ToLong(&curr) )
		{
			curr = 0;
		}

		arrLong.Add(curr);
	}
	return arrLong;
}


wxString SjTools::Implode(const wxArrayString& a, const wxString& delim)
{
	int i, iCount = a.GetCount();
	wxString ret;
	for( i = 0; i < iCount; i++ )
	{
		if( i ) ret += delim;
		ret += a.Item(i);
	}
	return ret;
}


wxString SjTools::Implode(const wxArrayLong& a, const wxString& delim)
{
	int i, iCount = a.GetCount();
	wxString ret;
	for( i = 0; i < iCount; i++ )
	{
		if( i ) ret += delim;
		ret += wxString::Format(wxT("%i"), (int)a.Item(i));
	}
	return ret;
}


/*******************************************************************************
 *  SjTools: Drawing
 ******************************************************************************/


void SjTools::DrawRubberbox(wxDC& dc, const wxPoint& start, const wxPoint& end)
{
	wxRasterOperationMode oldLogicalFunction = dc.GetLogicalFunction();
	dc.SetLogicalFunction(wxINVERT);

	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxTRANSPARENT_BRUSH);

	wxRect r(start, end);
	if( r.width < 4 ) r.width = 4;
	if( r.height < 4 ) r.height = 4;

	dc.DrawRectangle(r.x, r.y, r.width, r.height);

	r.Deflate(1);
	dc.DrawRectangle(r.x, r.y, r.width, r.height);

	dc.SetLogicalFunction(oldLogicalFunction);
}


void SjTools::DrawCross(wxDC& dc, int x, int y, int w, int h)
{
	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxWHITE_BRUSH);
	dc.DrawRectangle(x, y, w, h);
	dc.DrawLine(x, y, x+w-1, y+h-1);
	dc.DrawLine(x, y+h-1, x+w-1, y);
}


void SjTools::DrawBitmap(wxDC& dc, const wxBitmap* bitmap, int x, int y, int w, int h)
{
	/* if width and height are given, the bitmap is centerd in the image;
	 * otherwise it is drawn at the given position.
	 */

	if( !bitmap )
	{
		DrawCross(dc, x, y, w>=0? w : 16, h>=0? h : 16);
		return;
	}

	if( w > 0 && h> 0 )
	{
		int bitmapW = bitmap->GetWidth();
		int bitmapH = bitmap->GetHeight();

		dc.DrawBitmap(*bitmap, x + w/2 - bitmapW/2, y + h/2 - bitmapH/2, TRUE);
	}
	else
	{
		dc.DrawBitmap(*bitmap, x, y, TRUE);
	}
}


void SjTools::DrawBitmapHBg(wxDC& dc,
                            const wxBitmap* bitmapLeft,
                            const wxBitmap* bitmapMid,
                            const wxBitmap* bitmapRight,
                            const wxRect& drawRect,
                            bool alignMToR)
{
	// get bitmap height
	int bitmapWidth, bitmapHeight = 0;

	if( bitmapLeft )
	{
		bitmapHeight = bitmapLeft->GetHeight();
	}
	else if( bitmapRight )
	{
		bitmapHeight = bitmapRight->GetHeight();
	}
	else if( bitmapMid )
	{
		bitmapHeight = bitmapMid->GetHeight();
	}

	if( bitmapHeight == 0 )
	{
		DrawCross(dc, drawRect.x, drawRect.y, drawRect.width, drawRect.height);
		return;
	}

	// draw!
	bool   clippingSet = FALSE;
	wxRect currRect = drawRect;

	while( currRect.height > 0 )
	{
		// start drawing one line of bitmaps
		currRect.x      = drawRect.x;
		currRect.width  = drawRect.width;

		// draw left bitmap
		if( bitmapLeft && currRect.width > 0 )
		{
			bitmapWidth = bitmapLeft->GetWidth();
			if( bitmapWidth > currRect.width || bitmapHeight > currRect.height )
			{
				dc.SetClippingRegion(currRect);
				clippingSet = TRUE;
			}

			dc.DrawBitmap(*bitmapLeft, currRect.x, currRect.y, TRUE);
			currRect.x += bitmapWidth;
			currRect.width -= bitmapWidth;

			if( clippingSet )
			{
				dc.DestroyClippingRegion();
				clippingSet = FALSE;
			}
		}

		// draw right bitmap
		if( bitmapRight && currRect.width > 0 )
		{
			bitmapWidth = bitmapRight->GetWidth();
			if( bitmapWidth > currRect.width || bitmapHeight > currRect.height )
			{
				dc.SetClippingRegion(currRect);
				clippingSet = TRUE;
			}

			dc.DrawBitmap(*bitmapRight, currRect.x + currRect.width - bitmapWidth, currRect.y, TRUE);
			currRect.width -= bitmapWidth;

			if( clippingSet )
			{
				dc.DestroyClippingRegion();
				clippingSet = FALSE;
			}
		}

		// draw middle bitmap
		if( bitmapMid )
		{
			bitmapWidth = bitmapMid->GetWidth();
			while( currRect.width > 0 )
			{
				if( bitmapWidth > currRect.width || bitmapHeight > currRect.height )
				{
					dc.SetClippingRegion(currRect);
					clippingSet = TRUE;
				}

				dc.DrawBitmap(*bitmapMid, alignMToR? (currRect.x + currRect.width - bitmapWidth) : currRect.x, currRect.y, TRUE);
				if( !alignMToR ) currRect.x += bitmapWidth;
				currRect.width -= bitmapWidth;

				if( clippingSet )
				{
					dc.DestroyClippingRegion(); // on wxMac, this may not work as expected on wxClientDC, see remarks in SjSkinScrollbarItem::OnPaint()
					clippingSet = FALSE;
				}
			}
		}

		// prepare for next line
		currRect.y += bitmapHeight;
		currRect.height -= bitmapHeight;
	}
}


void SjTools::DrawBitmapVBg(wxDC& dc,
                            const wxBitmap* bitmapTop,
                            const wxBitmap* bitmapMid,
                            const wxBitmap* bitmapBottom,
                            const wxRect& drawRect,
                            bool alignMToB)
{
	// get bitmap width
	int bitmapWidth = 0, bitmapHeight;

	if( bitmapTop )
	{
		bitmapWidth = bitmapTop->GetWidth();
	}
	else if( bitmapBottom )
	{
		bitmapWidth = bitmapBottom->GetWidth();
	}
	else if( bitmapMid )
	{
		bitmapWidth = bitmapMid->GetWidth();
	}

	if( bitmapWidth == 0 )
	{
		DrawCross(dc, drawRect.x, drawRect.y, drawRect.width, drawRect.height);
		return;
	}

	// draw!
	bool   clippingSet = FALSE;
	wxRect currRect = drawRect;

	while( currRect.width > 0 )
	{
		// start drawing one column of bitmaps
		currRect.y      = drawRect.y;
		currRect.height = drawRect.height;

		// draw top bitmap
		if( bitmapTop && currRect.height > 0 )
		{
			bitmapHeight = bitmapTop->GetHeight();
			if( bitmapWidth > currRect.width || bitmapHeight > currRect.height )
			{
				dc.SetClippingRegion(currRect);
				clippingSet = TRUE;
			}

			dc.DrawBitmap(*bitmapTop, currRect.x, currRect.y, TRUE);
			currRect.y += bitmapHeight;
			currRect.height -= bitmapHeight;

			if( clippingSet )
			{
				dc.DestroyClippingRegion();
				clippingSet = FALSE;
			}
		}

		// draw bottom bitmap
		if( bitmapBottom && currRect.height > 0 )
		{
			bitmapHeight = bitmapBottom->GetHeight();
			if( bitmapWidth > currRect.width || bitmapHeight > currRect.height )
			{
				dc.SetClippingRegion(currRect);
				clippingSet = TRUE;
			}

			dc.DrawBitmap(*bitmapBottom, currRect.x, currRect.y + currRect.height - bitmapHeight, TRUE);
			currRect.height -= bitmapHeight;

			if( clippingSet )
			{
				dc.DestroyClippingRegion();
				clippingSet = FALSE;
			}
		}

		// draw middle bitmap
		if( bitmapMid )
		{
			bitmapHeight = bitmapMid->GetHeight();
			while( currRect.height > 0 )
			{
				if( bitmapWidth > currRect.width || bitmapHeight > currRect.height )
				{
					dc.SetClippingRegion(currRect);
					clippingSet = TRUE;
				}

				dc.DrawBitmap(*bitmapMid, currRect.x, alignMToB? (currRect.y + currRect.height - bitmapHeight) : currRect.y, TRUE);
				if( !alignMToB ) currRect.y += bitmapHeight;
				currRect.height -= bitmapHeight;

				if( clippingSet )
				{
					dc.DestroyClippingRegion();
					clippingSet = FALSE;
				}
			}
		}

		// prepare for next column
		currRect.x += bitmapWidth;
		currRect.width -= bitmapWidth;
	}
}


static wxString removeTabs(const wxString& s)
{
	wxString ret = s;
	ret.Replace(wxT("\t"), wxT(""));
	return ret;
}


void SjTools::DrawText( wxDC&           dc,
                        const wxString& text,
                        wxRect&         rect,
                        const wxFont&   font1,
                        const wxFont&   font2,
                        const wxColour& hiliteColour,
                        bool            doDraw )
{
	wxStringTokenizer   tokenizer(text, wxT(" "));
	wxString            token;
	wxCoord             tokenW, tokenH;
	wxCoord             lineX = 0, lineY = 0, lineH = 0, maxLineW = 0;
	wxCoord             spaceW;
	int                 lineTokenCount = 0;
	int                 totalTokenCount = 0;
	int                 firstTokenChar;
	bool                anyTabs = text.Find(wxT("\t"))>=0, hiliteState = false;
	wxColour            normalColour;

	dc.SetFont(font1);
	dc.GetTextExtent(wxT(" "), &spaceW, &tokenH);


	while( tokenizer.HasMoreTokens() )
	{
		/* get token
		 */
		token = tokenizer.GetNextToken();
		if( !token.IsEmpty() )
		{
			/* swith to font2? (usually a smaller font)
			 */
			firstTokenChar = token.GetChar(0);
			if( totalTokenCount
			        && (firstTokenChar=='(' || firstTokenChar=='[' || firstTokenChar=='{') )
			{
				dc.SetFont(font2);
				dc.GetTextExtent(wxT(" "), &spaceW, &tokenH);
			}

			/* calculate the width and the height of the token
			 */
			dc.GetTextExtent(anyTabs? removeTabs(token) : token, &tokenW, &tokenH);

			/* switch to the next line?
			 */
			if( lineX + tokenW > rect.width
			        && lineTokenCount > 0 )
			{
				if( lineX > maxLineW )
				{
					maxLineW = lineX;
				}

				lineX =  0;
				lineY += lineH;
				lineH =  0;
				lineTokenCount = 0;
			}

			if( tokenH > lineH )
			{
				lineH = tokenH;
			}

			if( doDraw )
			{
				if( lineX+tokenW > rect.width )
				{
					/* trim token string and append ".."
					 */
					wxString    test = token;
					wxCoord     testW, testH;
					long        tokenLen = token.Len(), i;
					for( i=tokenLen; i>=0; i-- )
					{
						test = token.Left(i);
						test.Trim();
						test.Append(wxT(".."));
						dc.GetTextExtent(anyTabs? removeTabs(test) : test, &testW, &testH);
						if( lineX + testW <= rect.width )
						{
							break;
						}
					}

					/* if the rest contains odd tabs, add a tab
					*/
					int tabCount = token.Mid(i).Replace(wxT("\t"), wxT("?"));
					if( tabCount%2 == 1 ) test.Append(wxT("\t"));

					token = test;
				}

				/* draw the text
				 */
				if( anyTabs && token.Find(wxT("\t"))!=-1 )
				{
					if( !normalColour.IsOk() ) normalColour = dc.GetTextForeground();

					wxStringTokenizer subtkz(token, wxT("\t"), wxTOKEN_RET_EMPTY_ALL);
					int               subtokenX = rect.x+lineX,
					                  subtokenY = rect.y+lineY+(lineH-tokenH);
					wxCoord           subtokenW, subtokenH;
					while( subtkz.HasMoreTokens() )
					{
						wxString subtoken(subtkz.GetNextToken());
						if( !subtoken.IsEmpty() )
						{
							dc.DrawText(subtoken, subtokenX, subtokenY);
							dc.GetTextExtent(subtoken, &subtokenW, &subtokenH);
							subtokenX += subtokenW;
						}

						if( subtkz.HasMoreTokens() )
						{
							hiliteState = !hiliteState;
							dc.SetTextForeground(hiliteState? hiliteColour : normalColour);
						}
					}
				}
				else
				{
					dc.DrawText(token, rect.x+lineX, rect.y+lineY+(lineH-tokenH));
				}
			}

			lineX += tokenW + spaceW;
			if( lineX > rect.width )
			{
				lineX = rect.width;
			}
			lineTokenCount++;
			totalTokenCount++;
		}
	}

	if( lineX > maxLineW )
	{
		maxLineW = lineX;
	}

	rect.width  = maxLineW;
	rect.height = lineY + lineH;

	if( hiliteState )
	{
		dc.SetTextForeground(normalColour);
	}
}


bool SjTools::DrawSingleLineText(wxDC& dc, const wxString& text, wxRect& rect,
                                 const wxFont& font1, const wxFont& smallFont, const wxFont* firstCharFont,
                                 const wxColour& hiliteColour)
{
	dc.SetFont(font1);
	bool hiliteState = false, truncated = false;
	wxColour normalColour = dc.GetTextForeground();

	wxString part = text;
	part.Replace(wxT("["), wxT("("));
	part.Replace(wxT("{"), wxT("("));
	long p1 = text.Find('(');
	if( p1 > 0 /*-1=not found, 0=first character (we do not want this)*/ )
	{
		part = text.Left(p1);
		wxRect partRect = rect;
		if( !DrawSingleLineText(dc, part, partRect, normalColour, hiliteColour, hiliteState) )
		{
			long p1width = partRect.width;

			dc.SetFont(smallFont);
			part = text.Mid(p1);
			partRect = rect;
			partRect.x += p1width;
			partRect.width -= p1width;
			truncated = DrawSingleLineText(dc, part, partRect, normalColour, hiliteColour, hiliteState);

			rect.width = p1width + partRect.width;
		}
	}
	else
	{
		truncated = DrawSingleLineText(dc, text, rect, normalColour, hiliteColour, hiliteState);
	}

	if( hiliteState )
	{
		dc.SetTextForeground(normalColour);
	}

	return truncated;
}
bool SjTools::DrawSingleLineText(wxDC& dc, const wxString& givenText__, wxRect& rect, const wxColour& normalColour, const wxColour& hiliteColour, bool& hiliteState)
{
	bool     anyTabs = givenText__.Find(wxT("\t"))>=0, truncated = false;
	wxCoord  textW, textH;
	wxString textToPrint = givenText__;

	dc.GetTextExtent(anyTabs? removeTabs(textToPrint) : textToPrint, &textW, &textH);
	if( textW > rect.width )
	{
		#define TWO_POINTS wxT("..")
		long tokenLen = textToPrint.Len(), i;
		for( i=tokenLen; i>=0; i-- )
		{
			textToPrint = givenText__.Left(i);
			textToPrint.Trim();
			textToPrint.Append(TWO_POINTS);
			dc.GetTextExtent(anyTabs? removeTabs(textToPrint) : textToPrint, &textW, &textH);
			if( textW <= rect.width )
			{
				break;
			}
		}

		if( textToPrint == TWO_POINTS && tokenLen > 0 )
		{
			// well, try again with the first character and one point
			for( i = 0; i <= 1; i++ )
			{
				wxCoord tstW;
				wxString tst = givenText__.Left(1) + (i==0? wxT(".") : wxT(""));
				dc.GetTextExtent(anyTabs? removeTabs(tst) : tst, &tstW, &textH);
				if( tstW <= rect.width )
				{
					textW = tstW;
					textToPrint = tst;
					break;
				}
			}
		}

		/* if the rest contains odd tabs, add a tab
		*/
		int tabCount = givenText__.Mid(i).Replace(wxT("\t"), wxT("?"));
		if( tabCount%2 == 1 ) textToPrint.Append(wxT("\t"));

		if( textW > rect.width )
			return true; // truncated

		truncated = true;
	}

	/* draw the text
	 */
	wxCoord y = rect.y + (rect.height-textH);
	if( anyTabs && textToPrint.Find(wxT("\t"))!=-1 )
	{
		wxStringTokenizer subtkz(textToPrint, wxT("\t"), wxTOKEN_RET_EMPTY_ALL);
		int               subtokenX = rect.x;
		wxCoord           subtokenW, subtokenH;
		while( subtkz.HasMoreTokens() )
		{
			wxString subtoken(subtkz.GetNextToken());
			if( !subtoken.IsEmpty() )
			{
				dc.DrawText(subtoken, subtokenX, y);
				dc.GetTextExtent(subtoken, &subtokenW, &subtokenH);
				subtokenX += subtokenW;
			}

			if( subtkz.HasMoreTokens() )
			{
				hiliteState = !hiliteState;
				dc.SetTextForeground(hiliteState? hiliteColour : normalColour);
			}
		}
	}
	else
	{
		dc.DrawText(textToPrint, rect.x, y);
	}

	rect.width = textW;
	return truncated;
}


wxString SjTools::wxColourToHtml(const wxColour& colour)
{
	wxString str;
	str.Printf(wxT("#%02x%02x%02x"), colour.Red(), colour.Green(), colour.Blue());
	return str;
}


long SjTools::wxColourToLong(const wxColour& colour)
{
	return  colour.Red()<<16
	    |   colour.Green()<<8
	    |   colour.Blue();
}


void SjTools::wxColourFromLong(wxColour& colour, long l)
{
	colour.Set((l&0xFF0000L)>>16, (l&0x00FF00L)>>8, (l&0x0000FFL));
}


void SjTools::DrawIcon(wxDC& dc, const wxRect& rect, long flags)
{
	int x = rect.x, y = rect.y;

	if( flags & SJ_DRAWICON_PLAY )
	{
		// draw PLAY icon
		int dx, diff = rect.height/2;
		for( dx = 0; dx < diff; dx++ )
		{
			dc.DrawLine(x+dx, y+dx, x+dx, y+diff*2-dx);
		}
	}
	else if( flags & (SJ_DRAWICON_TRIANGLE_DOWN|SJ_DRAWICON_TRIANGLE_UP) )
	{
		// draw MENU / TRIANGLE UP/DOWN icon
		int dy, diff = rect.width/2, thisY;
		int addy = (rect.height-diff)/2;
		for( dy = 0; dy < diff; dy++ )
		{
			thisY = y + addy + ((flags&SJ_DRAWICON_TRIANGLE_UP)? (diff-dy-1) : dy);
			dc.DrawLine(x+dy, thisY, x+diff*2-dy, thisY);
		}
	}
	else if( flags & (SJ_DRAWICON_TRIANGLE_RIGHT|SJ_DRAWICON_TRIANGLE_LEFT) )
	{
		// draw MENU / TRIANGLE RIGHT icon
		int dx, diff = rect.height/2, thisX;
		int addx = (rect.width-diff)/2;
		for( dx = 0; dx < diff; dx++ )
		{
			thisX = x + addx + ((flags&SJ_DRAWICON_TRIANGLE_LEFT)? (diff-dx-1) : dx);
			dc.DrawLine(thisX, y+dx, thisX, y+diff*2-dx);
		}
	}
	else if( flags & SJ_DRAWICON_PAUSE )
	{
		// draw PAUSE icon
		int dx, diffx = rect.height / 4, diffy = rect.height/4;
		for( dx = 0; dx < diffx; dx++ )
		{
			dc.DrawLine(x+dx, y+diffy/2, x+dx, y+rect.height-diffy);
		}

		for( dx = 0; dx < diffx; dx++ )
		{
			dc.DrawLine(x+diffx*2+dx, y+diffy/2, x+diffx*2+dx, y+rect.height-diffy);
		}
	}
	else if( flags & SJ_DRAWICON_STOP )
	{
		// draw STOP icon
		int dx, diffx = rect.height / 4, diffy = rect.height/4;
		for( dx = 0; dx < diffx*3; dx++ )
		{
			dc.DrawLine(x+dx, y+diffy/2, x+dx, y+rect.height-diffy);
		}
	}
	else if( flags & SJ_DRAWICON_DELETE )
	{
		// draw DELETE icon (a cross)
		wxRect cross(rect);
		cross.x++;
		cross.y++;
		cross.width -= 2;
		cross.height -= 2;

		dc.DrawLine(cross.x,                cross.y, cross.x+cross.width,   cross.y+cross.height);
		dc.DrawLine(cross.x+cross.width-1,  cross.y, cross.x-1,             cross.y+cross.width);

		dc.DrawLine(cross.x-1,              cross.y, cross.x+cross.width-1, cross.y+cross.height);
		dc.DrawLine(cross.x+cross.width-2,  cross.y, cross.x-2,             cross.y+cross.width);
	}
	else if( flags & (SJ_DRAWICON_VOLUP|SJ_DRAWICON_VOLDOWN) )
	{
		wxRect vol(rect);
		vol.height /= 2;
		vol.y += vol.height/2;
		int i, x;
		for( i = 0; i < vol.width; i++ )
		{
			x = (flags&SJ_DRAWICON_VOLUP) ? (vol.x+i) : (vol.x+vol.width-i-1);
			dc.DrawLine(x, vol.y+vol.height, x, vol.y+vol.height-i/2-1);
		}
	}
	else if( flags & SJ_DRAWICON_CHECK )
	{
		// draw CHECK icon (always 7x7 pixels)
		y += rect.height/2 - 4;
		static const char ypoints[7] = { 3, 4, 5, 4, 3, 2, 1 };
		int i;
		for( i = 0; i < 7; i++ )
		{
			dc.DrawLine(x+i, y+ypoints[i], x+i, y+ypoints[i]+2);
		}
	}
	else if( flags & SJ_DRAWICON_MOVED_DOWN )
	{
		// draw MOVED DOWN icon (always 7x7 pixels)
		y += rect.height/2 - 3;
		static const char yspoints[7] = { 3, 4, 5, 0, 5, 4, 3 };
		static const char yepoints[7] = { 4, 5, 6, 7, 6, 5, 4 };
		int i;
		for( i = 0; i < 7; i++ )
		{
			dc.DrawLine(x+i, y+yspoints[i], x+i, y+yepoints[i]);
		}
	}
	else
	{
		dc.DrawRectangle(rect);
	}
}


void SjTools::UpdateFacenames()
{
	// calling this will update the list used by GetFacenames() and HasFacename()
	wxASSERT( wxThread::IsMain()  );
	m_facenames.Clear();
}


const wxArrayString& SjTools::GetFacenames()
{
	wxASSERT( wxThread::IsMain()  );
	if( m_facenames.IsEmpty() )
	{
		// (re-)load list of facenames
		wxFontEnumerator fontEnumerator;
		fontEnumerator.EnumerateFacenames();
		m_facenames = fontEnumerator.GetFacenames();

		// make sure, there is at least one element
		if( m_facenames.IsEmpty() )
			m_facenames.Add(SJ_DEF_FONT_FACE);

		// sort the list
		m_facenames.Sort();
	}

	return m_facenames;
}


bool SjTools::HasFacename(const wxString& facename)
{
	const wxArrayString& facenames = GetFacenames();
	return (facenames.Index(facename, false /*case?*/) != wxNOT_FOUND);
}


/*******************************************************************************
 * SjTools: Icons
 ******************************************************************************/


wxImageList* SjTools::GetIconlist(bool large)
{
	if( !m_iconlistLoaded )
	{
		int iconSize, iconIndex;
		for( iconSize = 0; iconSize < 2; iconSize++ )
		{
			wxFileSystem fs;
			wxFSFile* fsFile = fs.OpenFile((iconSize==0? wxT("memory:icons16.png") : wxT("memory:icons32.png")));
			if( fsFile )
			{
				wxImage image(*(fsFile->GetStream()));
				if( image.IsOk() )
				{
					int h = image.GetHeight(),
					    w = image.GetWidth();
					wxColour maskColour(image.GetRed(0,0), image.GetGreen(0,0), image.GetBlue(0,0));
					if( h > 0 )
					{
						wxImageList* list = iconSize==0? &m_iconlistSmall : &m_iconlistLarge;
						list->RemoveAll();
						list->Create(h, h, TRUE, 16);
						for( iconIndex = 0; iconIndex < w/h; iconIndex++ )
						{
							wxImage subimage = image.GetSubImage(wxRect(iconIndex*h, 0, h, h));
							wxBitmap bitmap(subimage);
							list->Add(bitmap, maskColour);
						}
					}
				}

				delete fsFile;
			}
		}

		m_iconlistLoaded = TRUE;
	}

	return large? &m_iconlistLarge : &m_iconlistSmall;
}


wxBitmap SjTools::GetIconBitmap(SjIcon index, bool large)
{
	wxBitmap& bitmaps = large? m_iconBitmapsLarge : m_iconBitmapsSmall;

	if( !bitmaps.IsOk() )
	{
		wxFileSystem fs;
		wxFSFile* fsFile = fs.OpenFile(large? wxT("memory:icons32.png") : wxT("memory:icons16.png"));
		if( fsFile )
		{
			wxImage image(*(fsFile->GetStream()));
			if( image.IsOk() )
			{
				image.SetMaskColour(image.GetRed(0,0), image.GetGreen(0,0), image.GetBlue(0,0));
				bitmaps = wxBitmap(image);
			}

			delete fsFile;
		}
	}

	wxASSERT( index >= 0 && index < SJ_ICON_COUNT );

	if( bitmaps.IsOk() )
	{
		int bitmapH = bitmaps.GetHeight();
		return bitmaps.GetSubBitmap(wxRect(index*bitmapH, 0, bitmapH, bitmapH));
	}
	else
	{
		return wxBitmap();
	}
}


wxIcon SjTools::GetIconIcon(SjIcon index, bool large)
{
	wxIcon icon;
	icon.CopyFromBitmap(GetIconBitmap(index, large));
	return icon;
}


/*******************************************************************************
 * SjOmitWords Class
 ******************************************************************************/


static int SjOmitWords__Cmp(const wxString& s1, const wxString& s2)
{
	return s2.Len() - s1.Len(); // longer strings first
}


void SjOmitWords::Init(const wxString& words)
{
	wxStringTokenizer   tkz(words, wxT(","));
	wxString            curr;

	m_words = words;
	m_array.Clear();

	while( tkz.HasMoreTokens() )
	{
		curr = tkz.GetNextToken();
		curr.Trim(TRUE/*from right*/);
		curr.Trim(FALSE/*from left*/);
		if( !curr.IsEmpty() )
		{
			m_array.Add(curr);
		}
	}

	m_array.Sort(SjOmitWords__Cmp);
}


wxString SjOmitWords::Apply(const wxString& str__) const
{
	if( str__.IsEmpty() || m_array.IsEmpty() )
	{
		return str__;
	}

	/* remove tabs - tabs are used eg. for hiliting, they are added again if needed.
	 * we do not trim the string - this should be done before, if wanted!
	 */
	wxString str = str__;
	int tabCount = str.Replace(wxT("\t"), wxT(""));

	/* test all words to omit in the given order
	 */
	wxString left;
	size_t i;
	for( i = 0; i < m_array.Count(); i++ )
	{
		const wxString& curr = m_array.Item(i);
		left = str.Left(curr.Len());
		if( left.CmpNoCase(curr)==0 )
		{
			/* found a matching word-beginnig; move this beginning
			 * to the end so that "The Rolling Stones" becomes
			 * "Rolling Stones, The".  However, there MUST be something
			 * behind the word beginning for this purpose. In any case,
			 * we're done here as we check all beginnings ordered by
			 * their length. (eg. if you want to omit the german
			 * article "Die" but leave the Band "Die Happy" as is
			 * the rules are "die happy, die"
			 */
			if( str.Len() > /*not >= - sth. must follow the prefix!*/ curr.Len()
			        && str.GetChar(curr.Len()) == ' ' )
			{
				/* the comparison is case-insensitive, but moving the
				 * beginning to the end is case-sensitive:
				 */
				wxString right(str.Mid(left.Len()));

				/* re-add tabs in the new order if they were removed above
				 */
				int leftTabs = 0, rightTabs = 0;
				if( tabCount )
				{
					size_t i;
					for( i=0; i<str__.Len(); i++ )
					{
						if( str__[i]=='\t' )
						{
							if( i < left.Len()  )
							{
								leftTabs++;
								left.insert(i, wxT('\t'));
							}
							else if( i-left.Len()<=right.Len() )
							{
								rightTabs++;
								right.insert(i-left.Len(), wxT('\t'));
							}
						}
					}

					if( leftTabs == 1 && rightTabs == 1 && left[0u] == wxT('\t') && right.Last() == '\t' )
					{
						// mark the whole word
						left = left.Mid(1) + wxT("\t"); right = wxT("\t") + right.RemoveLast();
						leftTabs = rightTabs = 0; // avoid adding tabs after truncate
					}
				}

				/* add additional tabs at the end of left/the beginning of right;
				 * remove spaces in front of the right part
				 */
				if( leftTabs%2 == 1 )
				{
					if( right[0u] == wxT('\t') )
					{
						right = right.Mid(1);
						rightTabs--;
					}

					left.Append(wxT("\t"));
					leftTabs++;
				}

				while( right.Len() && right[0u]==wxT(' ') )
				{
					right.Remove(0, 1); // don't use Trim() to preserve tabs
				}

				if( rightTabs%2 == 1 )
				{
					right.Prepend(wxT("\t"));
					rightTabs++;
				}

				/* done, swap the parts
				 */
				str = right + wxT(", ") + left;
				return str;
			}

			/* noting more to do, exit for()
			 */
			break;
		}
	}

	/* nothing to omit
	 */
	return str__;
}


/*******************************************************************************
 * SjCoverFinder Class
 ******************************************************************************/


void SjCoverFinder::Init(const wxString& words)
{
	wxStringTokenizer   tkz(words, wxT(","));
	wxString            curr;

	m_keywords = words;
	m_array.Clear();

	while( tkz.HasMoreTokens() )
	{
		curr = tkz.GetNextToken();
		curr.Trim(TRUE/*from right*/);
		curr.Trim(FALSE/*from left*/);
		if( !curr.IsEmpty() )
		{
			m_array.Add(curr.Lower());
		}
	}
}


long SjCoverFinder::Apply(const wxArrayString& inPaths__, const wxString& inAlbumName__)
{
	// normalize the album name
	wxString inAlbumName = SjNormaliseString(inAlbumName__, 0);

	// get all input names
	long            uniqueAlbumNameIndex = 0, uniqueAlbumNameIndexCount = 0;
	wxArrayString   inNames;
	wxString        currInName;
	int i, inPathsCount = inPaths__.GetCount();
	for( i = 0; i < inPathsCount; i++ )
	{
		currInName = inPaths__.Item(i);

		currInName.Replace(wxT("\\"), wxT("/"));
		currInName.Replace(wxT(":"), wxT("/")); // replacing ":" by "/" is not only needed for Mac,
		// but also for nested file system paths eg.
		//      bla.mp3#id3:cover.jpg
		// or   bla.zip#zip:blub.mp3#id3:cover.jpg


		currInName = currInName.AfterLast(wxT('/'));
		currInName = currInName.BeforeLast(wxT('.'));
		currInName.MakeLower();
		inNames.Add(currInName);

		// check if the image name is equal to the album name
		// remember the index, if so.
		// (we only regard album names with at least 3 characters, remember
		// album names as "Lenny Kravitz - 5" or "H. Groenemeyer - Oe"
		if( inAlbumName.Len() >= 3
		 && SjNormaliseString(currInName, 0).Find(inAlbumName) != -1 )
		{
			uniqueAlbumNameIndex = i;
			uniqueAlbumNameIndexCount++;
		}
	}

	// if we've found _exactly_ one image name equal to
	// the album name, use it
	if( uniqueAlbumNameIndexCount == 1 )
	{
		return uniqueAlbumNameIndex;
	}

	// go through all keywords
	int w, wordCount = m_array.GetCount();
	for( w = 0; w < wordCount; w++ )
	{
		for( i = 0; i < inPathsCount; i++ )
		{
			if( inNames.Item(i).Find(m_array.Item(w)) != -1 )
			{
				return i;
			}
		}
	}

	return inPathsCount? 0 : -1;
}


/*******************************************************************************
 * String stuff
 ******************************************************************************/


#define s_scrambleStringUnicodeMark wxT('u')
static const wxUChar s_scrambleStringChars[16] =
{
	wxT('2'), wxT('f'), wxT('0'), wxT('3'),
	wxT('c'), wxT('4'), wxT('g'), wxT('1'),
	wxT('6'), wxT('8'), wxT('7'), wxT('b'),
	wxT('9'), wxT('a'), wxT('d'), wxT('e')
};
static int s_scrambleStringVal(wxUChar c)
{
	int i;
	for( i = 0; i < 16; i++ )
	{
		wxASSERT( s_scrambleStringChars[i] != s_scrambleStringUnicodeMark );
		if( s_scrambleStringChars[i] == c )
		{
			return i;
		}
	}
	return 0;
}


wxString SjTools::ScrambleString(const wxString& str)
{
	wxString ret;

	for( int i = 0; i < (int)str.Len(); i++ )
	{
		wxUChar c = str[i];
		if( c > 255 )
		{
			ret.Append(wxString::Format(wxT("%c%08x"), s_scrambleStringUnicodeMark, (int)c));
		}
		else
		{
			ret.Append(s_scrambleStringChars[c>>4 ]);
			ret.Append(s_scrambleStringChars[c&0xF]);
		}
	}

	return ret;
}


wxString SjTools::UnscrambleString(const wxString& str)
{
	wxString ret;
	wxUChar  c;
	long     l;
	int      i = 0;
	while( i < (int)str.Len() )
	{
		if( str[i] == s_scrambleStringUnicodeMark )
		{
			if( str.Mid(i+1, 8).ToLong(&l, 16) )
			{
				c = (wxUChar)l;
			}
			else
			{
				c = '?';
			}

			i += 9;
		}
		else
		{
			c = s_scrambleStringVal(str[i  ])<<4
			  | s_scrambleStringVal(str[i+1]);
			i += 2;
		}

		ret.Append(c);
	}

	return ret;
}


long SjTools::VersionString2Long(const wxString& versionStr)
{
	long j_major = 0, n_minor = 0, r_revision = 0;
	wxArrayString arr = SjTools::Explode(versionStr, '.', 3, 3);
	if(!arr[0].IsEmpty() ) { if( !arr[0].ToLong(&j_major) ) { j_major = 0; } }
	if(!arr[1].IsEmpty() ) { if( !arr[1].ToLong(&n_minor) ) { n_minor = 0; } }
	if(!arr[2].IsEmpty() ) { if( !arr[2].ToLong(&r_revision) ) { r_revision = 0; } }

	return (j_major<<24) | (n_minor<<16) | (r_revision<<8); // returned version is 0xjjnnrr00
}


/*******************************************************************************
 * SjLineTokenizer Class
 ******************************************************************************/


SjLineTokenizer::SjLineTokenizer(const wxString& str)
{
	long charsInclNull = str.Len()+1;

	m_freeData = TRUE;

	m_data = (wxChar*)malloc(charsInclNull * sizeof(wxChar));
	if( m_data == NULL ) return;

	memcpy(m_data, static_cast<const wxChar*>(str.c_str()), charsInclNull * sizeof(wxChar));
	m_data[charsInclNull-1] = 0;

	m_nextLineStart = m_data;
}


SjLineTokenizer::~SjLineTokenizer()
{
	if( m_freeData && m_data ) free(m_data);
}


wxChar* SjLineTokenizer::GetNextLine(bool trimAtBeg)
{
	wxChar* p1 = m_nextLineStart;
	wxChar* p2;

	if( m_data == NULL
	        || *p1 == 0 )
	{
		return NULL; // error in constructor or no more lines
	}

	// read over spaces and tabs at line beginning
	if( trimAtBeg )
	{
		while(  *p1 != 0
		        && (*p1 == ' ' || *p1 == wxT('\t')) )
		{
			p1++;
		}

		if( *p1 == 0 )
		{
			m_nextLineStart = p1;
			return p1;
		}
	}

	// find line end
	p2 = p1;
	while(  *p2 != 0
	        &&  *p2 != wxT('\n') && *p2 != wxT('\r') )
	{
		p2++;
	}

	if( *p2 == 0 )
	{
		m_nextLineStart = p2;
	}
	else
	{
		m_nextLineStart = p2 + 1;
		*p2 = 0;
	}

	// go back from p2 and read over spaces and tabs
	p2--;
	while(  p2 >= p1
	        && (*p2 == wxT(' ') || *p2 == wxT('\t')) )
	{
		*p2 = 0;
		p2--;
	}

	// done
	return p1;
}


void SjCfgTokenizer::AddFromString(const wxString& content__)
{
	wxString content(content__);

	content.Replace(wxT("\t"), wxT(" "));

	SjLineTokenizer tkz(content);
	wxChar*         linePtr;
	wxString        line, key, value;
	while( (linePtr=tkz.GetNextLine()) != NULL )
	{
		// get key'n'value pair (currLine is already trimmed aleft and aright)
		if( *linePtr && linePtr[0] != wxT(';') )
		{
			line = linePtr;
			if( line.Find(wxT('=')) != -1 )
			{
				key   = line.BeforeFirst(wxT('=')).Trim().Lower();
				value = line.AfterFirst(wxT('=')).Trim(FALSE);
				if( !key.IsEmpty() && !value.IsEmpty() )
				{
					m_hash.Insert(key, value);
					m_keys.Add(key);
					m_values.Add(value);
				}
			}
		}
	}
}


/*******************************************************************************
 * SjStringSerializer - Serialize Strings
 ******************************************************************************/


void SjStringSerializer::AddString(const wxString& s)
{
	wxASSERT( m_arr.IsEmpty() );

	if( !m_str.IsEmpty() ) {
		m_str += wxT(",");
	}

	wxString escaped_s(s);
	escaped_s.Replace(wxT("\\"), wxT("\\\\"));
	escaped_s.Replace(wxT("\""), wxT("\\\""));
	m_str += wxT("\"") + escaped_s + wxT("\"");
}


void SjStringSerializer::AddLong(long l)
{
	wxASSERT( m_arr.IsEmpty() );

	if( !m_str.IsEmpty() ) {
		m_str += wxT(",");
	}

	m_str += wxString::Format(wxT("%i"), (int)l); // longs are written decimal (instead of hex) to avoid problems with negative numbers
}


void SjStringSerializer::AddFloat(float f)
{
	wxASSERT( m_arr.IsEmpty() );

	if( !m_str.IsEmpty() ) {
		m_str += wxT(",");
	}

	wxString s = wxString::Format(wxT("%f"), f);
	s.Replace(wxT(","), wxT(".")); // we want "." as a decimal point
	m_str += s;
}


/*******************************************************************************
 * SjStringSerializer - Unserialize Strings
 ******************************************************************************/


SjStringSerializer::SjStringSerializer(const wxString& str)
{
	m_hasErrors = false;
	const wxCharBuffer cb = str.mb_str(wxConvUTF8);

	SjCsvTokenizer tknzr(wxT(","), wxT("\""), wxT("\\"));
	tknzr.AddData((const unsigned char*)cb.data(), strlen(cb.data()));
	tknzr.AddData((const unsigned char*)"\n", 1);

	wxArrayString* record = tknzr.GetRecord();
	if( record ) {
		m_arr = *record;
	}
}


wxString SjStringSerializer::GetString()
{
	wxASSERT( m_str.IsEmpty() );

	wxString ret;

	if( m_arr.GetCount() >= 1 ) {
		ret = m_arr.Item(0);
		m_arr.RemoveAt(0);
	}
	else {
		m_hasErrors = true;
		// end of list, no more items, this is treated as an error the caller can add
	}

	return ret;
}


long SjStringSerializer::GetLong()
{
	long l;
	wxString s = GetString();
	if( !s.ToLong(&l, 10) )
	{
		m_hasErrors = TRUE;
		l = 0;
	}
	return l;
}


float SjStringSerializer::GetFloat()
{
	double f;
	wxString s = GetString();
	if( !s.ToDouble(&f) )
	{
		s.Replace(wxT("."), wxT(","));  // internally, we use "." as a decimal point, however, the loaded localed
		if( !s.ToDouble(&f) )   // may expect a ","
		{
			m_hasErrors = TRUE;
			f = 0.0;
		}
	}
	return (float)f;
}


/*******************************************************************************
 * SjStrReplacer Class
 ******************************************************************************/


wxString SjStrReplacer::PrepareReplacement(const wxString& replacement) const
{
	wxString ret(replacement);

	// we have to replace all backslashes as we do not want the user to use
	// back-references (regex and Silverjuke crash for errors in back-references)
	ret.Replace(wxT("\\"), wxT("\\\\"));

	return ret;
}


bool SjStrReplacer::Compile(const wxString& pattern__, const wxString& replacement, long flags)
{
	wxString pattern(pattern__);
	m_replacement = PrepareReplacement(replacement);

	if( flags & wxRE_REGEX )
	{
		// the user wants to use regular expressions --
		// also allow PERL-compatible escapements and convert them to POSIX
		// however, this does not work as this would require recursive character
		// classes ("[\s\d]" would be converted to "[[[:digit:]][[:space:]]]" which is errorous), the user should use POSIX

		/* pattern.Replace("\\d", "[[:digit:]]");
		pattern.Replace("\\D", "[^[:digit:]]");
		pattern.Replace("\\s", "[[:space:]]");
		pattern.Replace("\\S", "[^[:space:]]");
		pattern.Replace("\\w", "[[:alnum:]_]");
		pattern.Replace("\\W", "[^[:alnum:]_]");

		bool wordStart = TRUE;
		while( pattern.Replace("\\b", wordStart? "[[:<:]]" : "[[:>:]]", FALSE) )
		{
		    wordStart = !wordStart;
		} */
	}
	else
	{
		// the user does not want to use regular expressions --
		// escape the meta characters used by our regular expression

		pattern.Replace(wxT("\\"), wxT("\\\\"));    // first escape the backslash
		pattern.Replace(wxT("["), wxT("\\["));  // the closing square bracket must not be escaped -- http://www.regular-expressions.info/characters.html
		pattern.Replace(wxT("^"), wxT("\\^"));
		pattern.Replace(wxT("$"), wxT("\\$"));
		pattern.Replace(wxT("."), wxT("\\."));
		pattern.Replace(wxT("|"), wxT("\\|"));
		pattern.Replace(wxT("?"), wxT("\\?"));
		pattern.Replace(wxT("*"), wxT("\\*"));
		pattern.Replace(wxT("+"), wxT("\\+"));
		pattern.Replace(wxT("("), wxT("\\("));
		pattern.Replace(wxT(")"), wxT("\\)"));
	}

	// the user wanto to match whole words only,
	// add the POSIX word-boundary sequenced around the pattern

	if( flags & wxRE_WHOLEWORDS )
	{
		pattern = wxT("[[:<:]]") + pattern + wxT("[[:>:]]");
	}

	// done so far -- compile the pattern

	m_compiled = m_regEx.Compile(pattern, (flags&0x0FFFFFFFL)|wxRE_EXTENDED);
	if( !m_compiled )
	{
		wxLogError(_("Invalid regular expression \"%s\"."), pattern.c_str());
	}

	return m_compiled;
}


int SjStrReplacer::ReplaceAll(wxString& text, const wxString* replacement)
{
	if( !IsValid() )
	{
		return -1;
	}

	if( replacement )
	{
		return m_regEx.ReplaceAll(&text, PrepareReplacement(*replacement));
	}
	else
	{
		return m_regEx.ReplaceAll(&text, m_replacement);
	}
}


/*******************************************************************************
 * SjPlaceholdReplacer Classes
 ******************************************************************************/


void SjPlaceholdReplacer::ReplaceAll(wxString& text)
{
	#define PLACEHOLDER_START '<'
	#define PLACEHOLDER_END   '>'

	wxString currPlaceholder, temp;
	long     currFlags;
	int      i1, i2 = 0;
	bool     hasFinalPlaceholder = FALSE;
	while( 1 )
	{
		// get the start/ending positions of the next placeholder
		// and the placeholder without the surrounding characters
		i1 = text.find(PLACEHOLDER_START, i2);
		if( i1 < 0 )
		{
			break;
		}

		i2 = text.find(PLACEHOLDER_END, i1);
		if( i2 < 0 )
		{
			break;
		}

		wxASSERT( i2 > i1 );

		currPlaceholder = text.Mid(i1+1, (i2-i1)-1).Trim(TRUE).Trim(FALSE);

		// check, if this is the final placeholder (if the string
		// does not end with a placeholder, there is no final placeholder)
		if( i2 == (long)text.Len()-1 )
		{
			hasFinalPlaceholder = TRUE;
		}

		// get length information about the placeholder
		currFlags = 0;
		if( currPlaceholder.Find('(') != -1 )
		{
			temp = currPlaceholder.AfterLast('(').BeforeFirst(')');
			if( !temp.ToLong(&currFlags) ) currFlags = 0;
			if( currFlags > SJ_PLR_WIDTH ) currFlags = SJ_PLR_WIDTH;

			currPlaceholder = currPlaceholder.BeforeLast('(').Trim();
		}

		// get the case information about the placeholder
		if( currPlaceholder.Upper() == currPlaceholder )
		{
			currFlags |= SJ_PLR_MAKEUPPER;
		}
		else if( currPlaceholder.Lower() == currPlaceholder )
		{
			currFlags |= SJ_PLR_MAKELOWER;
		}

		currPlaceholder.MakeLower();

		// try to replace the placeholder calling a virtual function
		// that should be implemented in derived classes
		if( GetReplacement(currPlaceholder, currFlags, temp) )
		{
			text = text.Left(i1) + temp + text.Mid(i2+1);
			i2 = i1 + temp.Len();
		}
	}

	// remember the final placeholder
	m_finalPlaceholder.Empty();
	if( hasFinalPlaceholder )
	{
		m_finalPlaceholder = currPlaceholder;
	}
}


wxString SjPlaceholdReplacer::ApplyFlags(const wxString& replacement, long flags)
{
	wxString ret(replacement);

	long width = flags & SJ_PLR_WIDTH;
	if( width )
	{
		ret = ret.Left(width);
	}

	if( flags & SJ_PLR_MAKEUPPER )
	{
		ret.MakeUpper();
	}
	else if( flags & SJ_PLR_MAKELOWER )
	{
		ret.MakeLower();
	}

	return ret;
}


wxString SjPlaceholdReplacer::ApplyFlags(long replacement, long flags)
{
	wxString ret = wxString::Format(wxT("%i"), (int)replacement);

	size_t width = flags & SJ_PLR_WIDTH;
	if( width )
	{
		if( width < ret.Len() )
		{
			ret = ret.Right(width);
		}
		else while( width > ret.Len() )
			{
				ret.Prepend(wxT('0'));
			}
	}

	return ret;
}


/*******************************************************************************
 * SjPlaceholdMatcher Classes
 ******************************************************************************/


#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArrayPlaceholdPart);


void SjPlaceholdMatcher::Compile(const wxString& pattern, const wxString& highPriorityDelim)
{
	// example pattern /<artist>/<album>/<nr> <title>.<ext>

	#ifdef __WXDEBUG__
		m_pattern = pattern;
	#endif

	m_parts.Clear();
	m_highPriorityDelim = highPriorityDelim;

	SjPlaceholdPart *currPart = NULL, *nextPart;
	int patternLen = pattern.Len();
	int i1, i2;
	for( i1 = 0; i1 < patternLen; i1++ )
	{
		// add placeholder?
		if( pattern[i1] == PLACEHOLDER_START )
		{
			i2 = pattern.find(PLACEHOLDER_END, i1);
			if( i2 >= 0 )
			{
				nextPart = new SjPlaceholdPart;
				nextPart->m_delimWidth = 0;
				nextPart->m_placeholder = pattern.Mid(i1+1, (i2-i1)-1).Lower().Trim(TRUE).Trim(FALSE);
				if( nextPart->m_placeholder.Find('(') >= 0 )
				{
					if( !nextPart->m_placeholder.AfterLast('(').BeforeFirst(')').ToLong(&nextPart->m_delimWidth) )
					{
						nextPart->m_delimWidth = 0;
					}

					nextPart->m_placeholder = nextPart->m_placeholder.BeforeLast('(').Trim();
				}

				if( !nextPart->m_placeholder.IsEmpty() )
				{
					currPart = nextPart;
					m_parts.Add(currPart);

					i1 = i2;
					continue;
				}
				else
				{
					delete nextPart;
				}
			}
		}

		// add separator
		if( currPart == NULL )
		{
			currPart = new SjPlaceholdPart;
			currPart->m_delimWidth = 0;
			m_parts.Add(currPart);
		}

		currPart->m_delimSepAfter.Append(pattern[i1]);
	}

	// set up the "go back" strings
	// (only used if the first placeholder is empty as we match from the right in this
	// case)
	m_goBack.Clear();
	m_goForward = 0;
	if( m_parts.GetCount() > 0 // Bug fixed, s. http://www.silverjuke.net/forum/topic-2669.html
	        && m_parts[0].m_placeholder.IsEmpty() )
	{
		int currPartIndex;
		for( currPartIndex = (int)m_parts.GetCount()-1; currPartIndex >= 0; currPartIndex-- )
		{
			currPart = &m_parts[currPartIndex];
			if( !currPart->m_delimSepAfter.IsEmpty() )
			{
				if( highPriorityDelim.IsEmpty() )
				{
					m_goBack.Add(currPart->m_delimSepAfter);
					m_goForward = m_parts[0].m_delimSepAfter.Len();
				}
				else
				{
					wxString temp = currPart->m_delimSepAfter;
					int highPriorityDelimCount = temp.Replace(highPriorityDelim, wxT("")), i;
					for( i = 0; i < highPriorityDelimCount; i++ )
					{
						m_goBack.Add(highPriorityDelim);
					}
					m_goForward = highPriorityDelim.Len()*highPriorityDelimCount;
				}
			}
		}
	}
}


bool SjPlaceholdMatcher::HasPlaceholder(const wxString& placeholder)
{
	int i, iCount = m_parts.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		if( m_parts[i].m_placeholder == placeholder )
		{
			return TRUE;
		}
	}
	return FALSE;
}


void SjPlaceholdMatcher::Match(const wxString& haystack, bool shortExt)
{
	int                 currPartIndex; // sign is important as we may count backwards
	SjPlaceholdPart     *currPart;//, *prevPart;
	wxString            currMatch;
	int                 startPos;

	if( m_parts.GetCount()==0 )
	{
		// no parts - nothing to do
		return;
	}

	#ifdef __WXDEBUG__
		wxLogDebug(wxT("----------------"));
		wxLogDebug(wxT("pattern := %s"), m_pattern.c_str());
		for( currPartIndex = 0; currPartIndex < (int)m_parts.GetCount(); currPartIndex++ )
		{
			currPart = &m_parts[currPartIndex  ];
			wxLogDebug(wxT("%s := read %i characters until \"%s\""),
					   currPart->m_placeholder.c_str(),
					   (int)currPart->m_delimWidth,
					   currPart->m_delimSepAfter.c_str());
		}
		wxLogDebug(wxT("----------------"));
	#endif

	// find out the starting position and save it to "startPos"

	if( m_goBack.GetCount() )
	{
		startPos = haystack.Len()+1;
		int i, iCount = m_goBack.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			if( startPos > 0 )
			{
				int goBackward = haystack.rfind(m_goBack[i], startPos-1);
				if( goBackward >= 0 )
				{
					startPos = goBackward;
				}
			}
		}
		startPos += m_goForward;
		currPartIndex = 1;
		wxASSERT( m_parts[0].m_placeholder.IsEmpty() );
	}
	else
	{
		startPos = 0;
		currPartIndex = 0;
	}

	#ifdef __WXDEBUG__
		wxLogDebug(wxT("interesting part starting at pos. %i := %s"), (int)startPos, haystack.Mid(startPos).c_str());
	#endif

	// okay, "startPos" is the starting position of our first placeholder now
	int endPos, nextPos;
	for( /*starting part index set above*/; currPartIndex < (int)m_parts.GetCount(); currPartIndex++ )
	{
		currPart = &m_parts[currPartIndex];
		wxASSERT( !currPart->m_placeholder.IsEmpty() );

		// get ending position to "i2"
		// (i2 will be the character after the placeholder)
		if( currPart->m_delimWidth )
		{
			endPos = startPos + currPart->m_delimWidth;
			if( endPos > (int)haystack.Len() )
			{
				continue;
			}

			nextPos = endPos;
			if( !currPart->m_delimSepAfter.IsEmpty() && startPos < (int)haystack.Len() )
			{
				int testPos = haystack.find(currPart->m_delimSepAfter, startPos);
				if( testPos >= 0 )
				{
					nextPos = testPos;
				}
			}
		}
		else if( !currPart->m_delimSepAfter.IsEmpty() && startPos < (int)haystack.Len() )
		{
			endPos = haystack.find(currPart->m_delimSepAfter, startPos);
			if( endPos < 0 )
			{
				continue;
			}

			// if the delimiter is a point and the next is the extension, look for the last
			// point -- see the forum entry http://www.silverjuke.net/forum/post.php?p=1849#1849
			if( shortExt
			        && currPart->m_delimSepAfter == wxT(".")
			        && currPartIndex == (int)m_parts.GetCount()-2
			        && m_parts[currPartIndex+1].m_placeholder == wxT("ext") )
			{
				int tryPos;
				while( endPos+1<(int)haystack.Len()
				        && (tryPos = haystack.find(currPart->m_delimSepAfter, endPos+1))>endPos  )
				{
					endPos = tryPos;
				}
			}

			nextPos = endPos;
		}
		else
		{
			endPos = haystack.Len();
			nextPos = endPos;
		}

		// got match!
		currMatch = haystack.Mid(startPos, (endPos-startPos)).Trim(TRUE).Trim(FALSE);
		#ifdef __WXDEBUG__
			wxLogDebug(wxT("%s := %s"), currPart->m_placeholder.c_str(), currMatch.c_str());
		#endif

		if(  (GotMatch(currPart->m_placeholder, currMatch) || currPart->m_delimWidth)
		        && (m_highPriorityDelim.IsEmpty() || !haystack.Mid(startPos, (nextPos-startPos)).Contains(m_highPriorityDelim)) )
		{
			// set next starting position
			startPos = nextPos + currPart->m_delimSepAfter.Len();
		}
	}
}


/*******************************************************************************
 * SjTrackInfoMatcher Classes
 ******************************************************************************/


SjTrackInfoMatcher::SjTrackInfoMatcher()
{
	m_tiDest = NULL;
	m_isCompiled = FALSE;
	m_slashReplacer.Compile(wxT("[[:space:]]*[\\/:]+[[:space:]]*"));
}


wxString SjTrackInfoMatcher::GetDefaultPattern()
{
	return wxT("<Artist>/<Year> <Album>/<Nr> <Title>");
}


wxString SjTrackInfoMatcher::NormalizeUrl(const wxString& url)
{
	wxString ret(url);

	if( m_tiFieldToSplit == SJ_TI_URL )
	{
		if( ret.Left(5)=="file:" )
		{
			ret = wxFileSystem::URLToFileName(ret).GetFullPath();
		}

		ret.Prepend(wxT('/'));
		m_slashReplacer.ReplaceAll(&ret, wxT("/"));
	}

	if( m_underscoresToSpaces )
	{
		ret.Replace(wxT("_"), wxT(" "));
	}

	return ret;
}


void SjTrackInfoMatcher::Compile(long tiFieldToSplit, const wxString& pattern__)
{
	// prepare pattern
	wxString pattern(pattern__);

	pattern.Replace(wxT("*"), wxT("<void>"));

	m_tiFieldToSplit = tiFieldToSplit;
	m_rawPattern = pattern__;

	m_underscoresToSpaces = FALSE;
	if( m_tiFieldToSplit == SJ_TI_URL )
	{
		m_underscoresToSpaces = (pattern.Find(wxT('_'))==-1);
		pattern += wxT(".<ext>");
	}

	// compile pattern
	SjPlaceholdMatcher::Compile(NormalizeUrl(pattern), m_tiFieldToSplit == SJ_TI_URL? wxT("/") : wxT(""));

	// use smart disk numbers?
	m_smartDiskNr = !HasPlaceholder(wxT("disknr"));

	// mark matcher as compiled
	m_isCompiled = TRUE;
}


void SjTrackInfoMatcher::Match(SjTrackInfo& tiSrc, SjTrackInfo& tiDest)
{
	if( !m_isCompiled )
	{
		Compile(SJ_TI_URL, GetDefaultPattern());
		wxASSERT( m_isCompiled );
	}

	m_tiDest = &tiDest;
	SjPlaceholdMatcher::Match(NormalizeUrl(tiSrc.GetValue(m_tiFieldToSplit)), true /*short extension!*/);
}


bool SjTrackInfoMatcher::GotMatch(const wxString& placeholder, const wxString& text)
{
	if( m_tiDest )
	{
		if( placeholder == wxT("title") )
		{
			m_tiDest->m_trackName = text;
			m_tiDest->m_validFields |= SJ_TI_TRACKNAME;
		}
		else if( placeholder == wxT("artist") )
		{
			m_tiDest->m_leadArtistName = text;
			m_tiDest->m_validFields |= SJ_TI_LEADARTISTNAME;
		}
		else if( placeholder == wxT("orgartist") )
		{
			m_tiDest->m_orgArtistName = text;
			m_tiDest->m_validFields |= SJ_TI_ORGARTISTNAME;
		}
		else if( placeholder == wxT("composer") )
		{
			m_tiDest->m_composerName = text;
			m_tiDest->m_validFields |= SJ_TI_COMPOSERNAME;
		}
		else if( placeholder == wxT("album") )
		{
			m_tiDest->m_albumName = text;
			m_tiDest->m_validFields |= SJ_TI_ALBUMNAME;
		}
		else if( placeholder == wxT("genre") )
		{
			m_tiDest->m_genreName = text;
			m_tiDest->m_validFields |= SJ_TI_GENRENAME;
		}
		else if( placeholder == wxT("group") )
		{
			m_tiDest->m_groupName = text;
			m_tiDest->m_validFields |= SJ_TI_GROUPNAME;
		}
		else if( placeholder == wxT("comment") )
		{
			m_tiDest->m_comment = text;
			m_tiDest->m_validFields |= SJ_TI_COMMENT;
		}
		else
		{
			long val;
			if( !text.ToLong(&val, 10) ) val = -1;

			if( placeholder == wxT("year") )
			{
				if( !SjTools::ParseYear(val, &val) ) return FALSE; // unexpected data - add text to the next placeholder

				m_tiDest->m_year = val;
				m_tiDest->m_validFields |= SJ_TI_YEAR;
			}
			else if( placeholder == wxT("disknr") )
			{
				if( val <= 0 || val > 999 ) return FALSE; // unexpected data - add text to the next placeholder

				m_tiDest->m_diskNr = val;
				m_tiDest->m_validFields |= SJ_TI_DISKNR;
			}
			else if( placeholder == wxT("nr") )
			{
				if( m_smartDiskNr && val >= 101 && val <= 999 )
				{
					m_tiDest->m_diskNr = val/100;
					m_tiDest->m_trackNr = val - m_tiDest->m_diskNr*100;
					m_tiDest->m_validFields |= SJ_TI_DISKNR | SJ_TI_TRACKNR;
				}
				else
				{
					if( val <= 0 || val > 999 ) return FALSE; // unexpected data - add text to the next placeholder

					m_tiDest->m_trackNr = val;
					m_tiDest->m_validFields |= SJ_TI_TRACKNR;
				}
			}
		}
	}

	return TRUE; // data read; we also return TRUE if the tag is not understood by
	// us (remember, the user can skip any information using "<void>")
}


/*******************************************************************************
 * SjProp Class
 ******************************************************************************/


void SjProp::Init()
{
	m_names.Clear();
	m_values.Clear();
	m_flags.Clear();
}


void SjProp::Add(const wxString& name, const wxString& value, long flags)
{
	m_names.Add(name);
	m_values.Add(value);
	m_flags.Add(flags);
}


void SjProp::AddHex(const wxString& name, long value, long flags)
{
	Add(name, wxString::Format(wxT("0x%08X (%i)"), (int)value, (int)value), flags);
}


void SjProp::Add(const wxString& name, long value, long flags)
{
	if( value == 0 && flags&SJ_PROP_EMPTYIFEMPTY )
	{
		Add(name, wxT(""), flags);
	}
	else
	{
		Add(name, wxString::Format(wxT("%i"), (int)value), flags);
	}
}


void SjProp::AddBytes(const wxString& name, long value, long flags)
{
	if( value == 0 && flags&SJ_PROP_EMPTYIFEMPTY )
	{
		Add(name, wxT(""), flags);
	}
	else
	{
		Add(name, SjTools::FormatBytes(value, SJ_FORMAT_ADDEXACT), flags);
	}
}


/*******************************************************************************
 * SjWheelHelper Class
 ******************************************************************************/


SjWheelHelper::SjWheelHelper()
{
	m_availRotation = 0;
}


void SjWheelHelper::PushRotationNPopAction(wxMouseEvent& e, long& actions, long& dir)
{
	// collect rotations ...
	long rotation = e.GetWheelRotation();
	if( (rotation < 0 && m_availRotation > 0) || (rotation > 0 && m_availRotation < 0) )
	{
		m_availRotation = 0; // discard saved scrolling for the wrong direction
	}
	m_availRotation += rotation;

	// ... use rotation, return the number of actions to perform
	long delta = e.GetWheelDelta();
	if( delta <= 0 ) delta = 120;

	actions = m_availRotation/delta;
	m_availRotation -= actions*delta;

	// separate sign from action
	dir = 1;
	if( actions < 0 )
	{
		actions *= -1;
		dir = -1;
	}

	if( actions > 2000 ) { actions = 2000; } // shdould not happen, added for safety
}


/*******************************************************************************
 * SjLLHash - Hash long => long
 ******************************************************************************/


wxString SjLLHash::GetKeysAsString()
{
	wxString ret;
	long     key;
	SjHashIterator iterator;
	while( Iterate(iterator, &key) )
	{
		ret.Append(wxString::Format(wxT("%i,"), (int)key));
	}
	ret.Truncate(ret.Len()-1);
	return ret;
}


void SjLLHash::CopyFrom(SjLLHash* o)
{
	Clear();
	SjHashIterator iterator;
	long value, key;
	while( (value=o->Iterate(iterator, &key)) )
	{
		Insert(key, value);
	}
}


/*******************************************************************************
 * SjSSHash - Hash string => string
 ******************************************************************************/


void SjSSHash::Clear()
{
	SjHashIterator iterator;
	wxString key, *value;
	while( (value=Iterate(iterator, key)) )
	{
		delete value;
	}
	sjhashClear(&m_hash);
}


wxString SjSSHash::Serialize()
{
	SjStringSerializer ser;
	ser.AddLong(GetCount());

	SjHashIterator iterator;
	wxString *value, key;
	while( (value=Iterate(iterator, key))!=NULL )
	{
		ser.AddString(key);
		ser.AddString(*value);
	}

	return ser.GetResult();
}


void SjSSHash::Unserialize(const wxString& str)
{
	Clear();

	SjStringSerializer ser(str);
	long i, iCount = ser.GetLong();

	wxString key, value;
	for( i = 0; i < iCount; i++ )
	{
		key = ser.GetString();
		value = ser.GetString();
		Insert(key, value);
	}

	if( ser.HasErrors() )
	{
		Clear();
	}
}


/*******************************************************************************
 * Hardware Graphic Stuff, Misc.
 ******************************************************************************/


long SjMs2Bytes(long ms)
{
	// calculate the exact number of bytes using 64 bit to avoid overflows
	wxLongLong ll(0, ms);
	ll *= (SJ_WW_FREQ * SJ_WW_CH * SJ_WW_BYTERES);
	ll /= 1000;
	long l = ll.ToLong();

	// make sure, the number of bytes are sample-aligned
	if( l % (SJ_WW_BYTERES*SJ_WW_CH) != 0 )
	{
		l += SJ_WW_BYTERES * SJ_WW_CH - (l % (SJ_WW_BYTERES * SJ_WW_CH));
	}

	// done
	wxASSERT( l % (SJ_WW_BYTERES*SJ_WW_CH) == 0 );
	return l;
}


/*******************************************************************************
 * Strings used by wx/Silverjuke that should be localizable;
 * There is no need to include them into the project, only needed by poEdit.
 ******************************************************************************/


#ifdef __ANY_LABEL_THAT_SHOULDNT_BE_DEFINED__

// the logging dialog

_("Fatal error");

// accelerator stuff

_("Ctrl");
_("Alt");
_("Shift");

// skin: layout switching targets

_("Enlarge window")
_("Shrink window")
_("Enlarge display")
_("Shrink display")

// Mac OS X menu entries as used in src/osx/menu_osx.cpp

_("Services")
_("Hide %s")
_("Hide Others")
_("Show All")
_("Quit %s")

// misc.

_("Full screen");
_("Info...");
_("State")
_("Modules on the web...")

#endif


wxString _os(const wxString& str__)
{
	// the function _os() created OS-specific strings from generic strings,
	// eg. most OS use "Exit" but the Mac uses "Quit"
	wxString str(str__);
	
	#if defined(__WXMSW__)
		if( str==_("Show file") ) { str = _("Explore"); }
	#elif defined(__WXMAC__)
		str.Replace("Exit ",              "Quit ");
		str.Replace("Settings",           "Preferences");
		if( str=="Datei" )        { str = "Ablage"; }
		if( str=="Ansicht" )      { str = "Darstellung"; }
		if( str==_("Show file") ) { str = _("Reveal in Finder"); }
	#endif
	
	return str;
}

