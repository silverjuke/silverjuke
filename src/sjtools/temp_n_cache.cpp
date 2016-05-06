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
 * File:    temp_n_cache.cpp
 * Authors: Björn Petersen
 * Purpose: Handling temporary and cached files
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/cmdline.h>
#include <sjtools/temp_n_cache.h>


/*******************************************************************************
 * Constructor / Destructor
 ******************************************************************************/


SjTempNCache::SjTempNCache()
{
	m_filesAdded = 0;
	m_filesAddedMax = SJ_MIN_FILESADDEDMAX;
	m_sqlDb = NULL;
	if( s_removeOnLastCall == NULL )
	{
		s_removeOnLastCall = new SjSLHash;
	}
}


SjTempNCache::~SjTempNCache()
{
	if( m_sqlDb )
	{
		m_sqlDbCritical.Enter();
		if( m_sqlDbProtectedIds.GetCount() )
		{
			// unprotect protected IDs on exit - however, removing is done on
			// the next normal cleanup (when Silverjuke starts next time) - this
			// is also a good idea as other programs may use the file, eg. when
			// starting a burning program by a CUE-file and exit Silverjuke while
			// the burning program is open.
			wxSqlt sql(m_sqlDb);
			sql.Query(wxString::Format(wxT("UPDATE sjcache SET method=0 WHERE id IN (%s);"),
			                           m_sqlDbProtectedIds.GetKeysAsString().c_str()));
		}

		delete m_sqlDb;
		m_sqlDb = NULL;
		m_sqlDbCritical.Leave();
	}
}


void SjTempNCache::Init()
{
	// must be called by the user before using ANY other functions

	GetTempDir();
}


bool SjTempNCache::InitDb(bool lock)
{
	// called internally by functions needing m_sqlDb

	bool ret = TRUE;

	if( m_sqlDb == NULL )
	{
		if(lock) m_sqlDbCritical.Enter();

		if( m_sqlDb == NULL /*check again, as we do not alloc the critsect on the first check*/ )
		{
			m_sqlDb = CreateOrOpenDb(m_tempDir);
			if( m_sqlDb == NULL )
			{
				ret = FALSE;
			}
		}

		if(lock) m_sqlDbCritical.Leave();
	}

	return ret;
}


wxSqltDb* SjTempNCache::CreateOrOpenDb(const wxString& tempDir)
{
	wxString dbName = tempDir + SJ_TEMP_PREFIX wxT("00000000.db");

	wxSqltDb* retDb = new wxSqltDb(dbName);
	if( retDb == NULL )
	{
		wxLogError(_("Cannot open \"%s\"."), dbName.c_str());
		return NULL;
	}

	retDb->SetSync(0);

	wxSqlt sql(retDb);

	if( !retDb->ExistsBeforeOpening() )
	{
		sql.Query(wxT("CREATE TABLE sjcache (id INTEGER PRIMARY KEY, org TEXT, method INTEGER DEFAULT 0);"));
		sql.Query(wxT("CREATE INDEX sjcacheindex01 ON sjcache (org);"));

		// init max. MB, if available, we use the value stored in the INI
		long maxMb = g_tools->m_config->Read(wxT("main/tempMaxMb"), SJ_TEMP_DEF_MB);
		if( maxMb < SJ_TEMP_MIN_MB ) maxMb = SJ_TEMP_DEF_MB;
		sql.ConfigWrite(wxT("maxMb"), maxMb);
	}

	return retDb;
}


/*******************************************************************************
 * Get/set the temp. directory to use
 ******************************************************************************/


wxString SjTempNCache::GetDefaultTempDir()
{
	wxString test;

	test = wxFileName::CreateTempFileName(wxT("sj"));
	{ wxLogNull null; ::wxRemoveFile(test); }

	wxFileName fn(test);
	fn.AppendDir("silverjuke-tmp-" + SjNormaliseString(wxGetUserId(), 0)); // add the user name as otherwise different users may use /tmp/silverjuke-tmp/
	return SjLittleDirSel::NormalizeDir(fn.GetPath(wxPATH_GET_VOLUME|wxPATH_GET_SEPARATOR));
}


wxString SjTempNCache::GetTempDir()
{
	// temp. dir already set?
	if( m_tempDir.IsEmpty() )
	{
		// see the user's preferences
		bool gotFromIni = FALSE;
		if( !SjMainApp::s_cmdLine->Found(wxT("temp"), &m_tempDir) )
		{
			m_tempDir = g_tools->m_config->Read(wxT("main/temp"), wxT(""));
			if( !m_tempDir.IsEmpty() )
			{
				gotFromIni = TRUE;
			}
		}

		if( !m_tempDir.IsEmpty() )
		{
			// check stuff given by the user, on errors,
			// we clear the temp. directory which will force the default.
			if( !::wxDirExists(m_tempDir) )
			{
				// try absolute path
				wxFileName fn;
				fn.AssignDir(m_tempDir);
				fn.MakeAbsolute(g_tools->GetSilverjukeProgramDir());
				m_tempDir = fn.GetPath(wxPATH_GET_VOLUME|wxPATH_GET_SEPARATOR);
			}

			if( ::wxDirExists(m_tempDir) )
			{
				m_tempDir = SjLittleDirSel::NormalizeDir(m_tempDir);
			}
			else
			{
				wxLogError(wxT("Cannot use the directory \"%s\" for temporary files as the directory does not exist.")/*n/t - locale normally not yet loaded*/, m_tempDir.c_str());
				m_tempDir.Clear();
				if( gotFromIni )
				{
					// avoid coming the error up on every program start
					g_tools->m_config->DeleteEntry(wxT("main/temp"));
				}
			}
		}

		if( m_tempDir.IsEmpty() )
		{
			// set default temp. dir (normalization
			// done in GetDefaultTempDir() as values returned by
			// GetTempDir and GetDefaultTempDir() should be comparable
			m_tempDir = GetDefaultTempDir();
		}

		// make sure, the directory exists
		if( !::wxDirExists(m_tempDir) )
		{
			::wxMkdir(m_tempDir);
		}
	}

	// done so far
	return m_tempDir;
}


void SjTempNCache::SetTempDir(const wxString& newTempDir__)
{
	wxString newTempDir = SjLittleDirSel::NormalizeDir(newTempDir__);

	if( !InitDb() || newTempDir == GetTempDir() )
	{
		return;
	}

	if( !wxDirExists(newTempDir) )
	{
		wxLogError(_("Cannot open \"%s\"."), newTempDir.c_str());
		return;
	}

	wxSqltDb* newDb = CreateOrOpenDb(newTempDir);
	if( newDb == NULL )
	{
		return; // error already logged by CreateOrOpenDb()
	}


	m_sqlDbCritical.Enter();

	// just assign the new database object.
	//
	// do NOT delete the old database object as it may be used by other instances.
	// do NOT delete the old cache files as they also may still be used by other
	// instances or even by us.

	delete m_sqlDb;
	m_sqlDb = newDb;

	m_tempDir = newTempDir;

	if( m_tempDir == GetDefaultTempDir() )
	{
		g_tools->m_config->DeleteEntry(wxT("main/temp"));
	}
	else
	{
		g_tools->m_config->Write(wxT("main/temp"), m_tempDir);
	}

	m_sqlDbCritical.Leave();
}


long SjTempNCache::GetMaxMB()
{
	// read the setting from the cache database
	long ret = SJ_TEMP_DEF_MB;
	if( InitDb() )
	{
		m_sqlDbCritical.Enter();
		{
			wxSqlt sql(m_sqlDb);
			ret = sql.ConfigRead(wxT("maxMb"), SJ_TEMP_DEF_MB);
		}
		m_sqlDbCritical.Leave();
	}
	return ret;
}


void SjTempNCache::SetMaxMB(long newMaxMb)
{
	if( newMaxMb >= SJ_TEMP_MIN_MB && InitDb() )
	{
		// save the setting in the cache database as well as to the INI
		long oldMaxMb = GetMaxMB();

		m_sqlDbCritical.Enter();
		{
			wxSqlt sql(m_sqlDb);
			sql.ConfigWrite(wxT("maxMb"), newMaxMb);
		}
		m_sqlDbCritical.Leave();

		g_tools->m_config->Write(wxT("main/tempMaxMb"), newMaxMb);

		if( newMaxMb < oldMaxMb )
		{
			CleanupFiles(SJ_CLEANUP_FORCE);
		}
	}
}


/*******************************************************************************
 * Configuration using "little options"
 ******************************************************************************/


#define _GB 1024
static const long s_mbOptions[] = {
	64,     128,    256,    512,    1*_GB,
	2*_GB,  3*_GB,  5*_GB,  10*_GB, 25*_GB,
	50*_GB
};


class SjLittleTempSize : public SjLittleOption
{
public:
	SjLittleTempSize(SjTempNCache* tempNCache)
		: SjLittleOption(_("max. size"), SJ_ICON_LITTLEDEFAULT)
	{
		m_tempNCache = tempNCache;
		m_mbOnCreate = tempNCache->GetMaxMB();
		m_mbCurr     = m_mbOnCreate;
	}

	wxString GetDisplayValue() const
	{
		return SjTools::FormatBytes(m_mbCurr, SJ_FORMAT_MB);
	}

	long GetOptionCount () const
	{
		return sizeof(s_mbOptions)/sizeof(long);
	}

	wxString GetOption(long i) const
	{
		return SjTools::FormatBytes(s_mbOptions[i], SJ_FORMAT_MB);
	}

	bool IsOptionChecked(long i) const;
	long IsOptionCheckable(long i) const;
	bool ShowMenuOnDoubleClick() { return TRUE; }
	bool IsDefaultOption(long mb) const;
	bool IsModified() const { return m_mbCurr!=SJ_TEMP_DEF_MB; }
	bool OnOption           (wxWindow* parent, long i) { m_mbCurr = s_mbOptions[i]; return FALSE; }
	bool OnDefault          (wxWindow* parent) { m_mbCurr = SJ_TEMP_DEF_MB; return FALSE; }
	void OnApply            () { if(m_mbCurr!=m_mbOnCreate) { wxBusyCursor busy; m_tempNCache->SetMaxMB(m_mbCurr); } }

private:
	SjTempNCache*   m_tempNCache;
	long            m_mbOnCreate, m_mbCurr;
};


bool SjLittleTempSize::IsOptionChecked(long i) const
{
	if(  m_mbCurr >= s_mbOptions[i]
	        && (i==GetOptionCount()-1 || m_mbCurr<s_mbOptions[i+1]) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


long SjLittleTempSize::IsOptionCheckable(long i) const
{
	return 2;
}


class SjLittleTempDirSel : public SjLittleDirSel
{
public:
	SjLittleTempDirSel(SjTempNCache* tempNCache)
		: SjLittleDirSel(wxT(""), tempNCache->GetTempDir(), tempNCache->GetDefaultTempDir(), SJ_ICON_LITTLEDEFAULT)
	{
		m_tempNCache = tempNCache;
	}

	void OnApply()
	{
		m_tempNCache->SetTempDir(m_currDir);
	}

private:
	SjTempNCache* m_tempNCache;
};


void SjTempNCache::GetLittleOptions(SjArrayLittleOption& lo)
{
	SjLittleOption::SetSection(_("Temporary directory"));

	lo.Add(new SjLittleTempSize(this));
	lo.Add(new SjLittleTempDirSel(this));
}


/*******************************************************************************
 * Unmanaged Temporary Files
 ******************************************************************************/


SjSLHash* SjTempNCache::s_removeOnLastCall = NULL;


wxString SjTempNCache::AddToUnmanagedTemp(const wxString& fileName)
{
	wxASSERT( m_tempDir.Last()=='\\' || m_tempDir.Last()=='/' );
	wxString tempPath = m_tempDir+fileName;

	m_removeOnLastCallCritical.Enter();
	s_removeOnLastCall->Remove(tempPath);
	m_removeOnLastCallCritical.Leave();

	return tempPath;
}


wxString SjTempNCache::LookupUnmanagedTemp(const wxString& fileName)
{
	wxString tempPath = m_tempDir+fileName;

	if( ::wxFileExists(tempPath) )
	{
		m_removeOnLastCallCritical.Enter();
		if( s_removeOnLastCall->Lookup(tempPath) )
		{
			tempPath = wxT(""); // the file exists but its deletion is pending
		}
		m_removeOnLastCallCritical.Leave();
	}
	else
	{
		tempPath = wxT(""); // the file does not exist
	}

	return tempPath;
}


void SjTempNCache::RemoveFromUnmanagedTemp(const wxString& tempPath)
{
	wxLogNull null;
	if( !::wxRemoveFile(tempPath) )
	{
		m_removeOnLastCallCritical.Enter();
		s_removeOnLastCall->Insert(tempPath, 1);
		m_removeOnLastCallCritical.Leave();
	}
}


void SjTempNCache::LastCallOnExit()
{
	if( s_removeOnLastCall )
	{
		// a critical section is not needed as this should be called really at the
		// end of the program
		wxASSERT( wxThread::IsMain() );

		wxLogNull null;
		wxString  tempPath;

		SjHashIterator iterator;
		while( s_removeOnLastCall->Iterate(iterator, tempPath) )
		{
			::wxRemoveFile(tempPath);
		}

		delete s_removeOnLastCall;
		s_removeOnLastCall = NULL;
	}
}


wxString SjTempNCache::GetUnusedFileName(const wxString& dir, const wxString& ext__)
{
	static long s_tempIndex = 0;
	wxString name;
	wxString ext(ext__);
	if( ext.IsEmpty() ) ext = wxT("tmp");
	while( 1 )
	{
		wxASSERT( dir.Last() == '/' || dir.Last() == '\\' );
		s_tempIndex++;
		name = wxString::Format(SJ_TEMP_PREFIX wxT("z%07x.%s"), (int)s_tempIndex, ext.c_str());
		//  ^ the "z" is here to avoid collasions with files in the temp. dir.
		if( !::wxFileExists(dir+name) )
		{
			break;
		}
	}
	return name;
}


/*******************************************************************************
 * Managed Temporary Files
 ******************************************************************************/


wxString SjTempNCache::AddToManagedTemp(const wxString& ext__,
                                        bool  protectTilExit)
{
	wxString ext(ext__);
	if( ext.IsEmpty() )
	{
		ext = wxT("tmp");
	}

	wxString orgFileName = wxString::Format(wxT("temp:%lu-%lu.%s"), (unsigned long)SjTools::GetMsTicks(), (unsigned long)SjTools::Rand(0x10000000L), ext.c_str());
	return Add(orgFileName, protectTilExit, TRUE/*do lock*/);
}


void SjTempNCache::RemoveFromManagedTemp(const wxString& tempPath)
{
	// remove the file - do this before the ID check
	// as it is save to delete files with RemoveFromManagedTemp() without
	// allocation by AddToManagedTemp() (this may be useful to get the
	// delayed deletion feature)
	RemoveFromUnmanagedTemp(tempPath);

	// get the ID from the temp. file name -
	wxString idHex = tempPath.AfterLast('-').BeforeFirst('.');
	if( idHex.Len()==8 )
	{
		long id = 0;
		if( !idHex.ToLong(&id, 16) ) { id = 0; }
		if( id
		 && InitDb() )
		{
			// remove the db entry
			m_sqlDbCritical.Enter();
			{
				wxSqlt sql(m_sqlDb);
				sql.Query(wxString::Format(wxT("DELETE FROM sjcache WHERE id=%lu;"), (unsigned long)id));

				m_sqlDbProtectedIds.Remove(id);
			}
			m_sqlDbCritical.Leave();
		}
	}
}


/*******************************************************************************
 * The Cache
 ******************************************************************************/


static wxString GetIDdName(const wxString& orgFileName,
                           long            id)
{
	wxString ext = SjTools::GetExt(orgFileName);
	if( ext.IsEmpty() )
	{
		ext = wxT("tmp");
	}

	return wxString::Format(SJ_TEMP_PREFIX wxT("%08x.%s"), (int)id, ext.c_str());
}


wxString SjTempNCache::Add(const wxString&  orgFileName,
                           bool             protectTilExit,
                           bool             lock)
{
	wxString cachePathNFile = LookupCache(orgFileName, lock);
	if( !cachePathNFile.IsEmpty() )
	{
		// file found on cache
		return cachePathNFile;
	}
	else if( InitDb() )
	{
		// add file to cache (InitDb() is also called by LookupCache() above)
		long id;

		if(lock) m_sqlDbCritical.Enter();
		if( m_sqlDbNeverCache.Lookup(orgFileName) )
		{
			if(lock) m_sqlDbCritical.Leave(); // do not forget this
			return wxT(""); // never cache this file!
		}

		{
			wxSqlt sql(m_sqlDb);
			sql.Query(wxT("INSERT INTO sjcache (org, method) VALUES ('") + sql.QParam(orgFileName) + wxT("', ") + sql.LParam(protectTilExit? 1 : 0) + wxT(")"));
			id = sql.GetInsertId();

			if( protectTilExit )
			{
				m_sqlDbProtectedIds.Insert(id, 1);
			}
			else
			{
				m_sqlDbProtectedIds.Remove(id);
			}

			m_filesAdded++;
		}
		if(lock) m_sqlDbCritical.Leave();

		return AddToUnmanagedTemp(GetIDdName(orgFileName, id));
	}

	// error
	return wxT("");
}


wxString SjTempNCache::AddToCache(const wxString&   orgFileName,
                                  const void*       orgData,
                                  long              orgBytes)
{
	bool lockHere = FALSE;
	if( orgBytes < 1024*1024 )
	{
		// for larger files, we do not lock the thread as writing back may be too time-consuming.
		// on the other hand it is a good idea to write the files
		lockHere = TRUE;
	}

	if( lockHere ) m_sqlDbCritical.Enter();

	wxString cacheFileName = Add(orgFileName, SJ_TEMP_NO_PROTECT, !lockHere);
	if( !cacheFileName.IsEmpty() )
	{
		// write data to the cache file, if needed, we'll overwrite any existing file
		bool fileWritten = FALSE;
		{
			wxFile destFile;
			destFile.Create(cacheFileName, TRUE); // TRUE=overwrite
			if( destFile.IsOpened() )
			{
				if( destFile.Write(orgData, orgBytes) == (size_t)orgBytes )
				{
					fileWritten = TRUE;
				}
			}
		}

		if( !fileWritten )
		{
			// error writing the file, delete the file
			// (the database entry is deleted on the next Lookup())
			RemoveFromUnmanagedTemp(cacheFileName);
			cacheFileName.Clear();
		}
	}

	if( lockHere ) m_sqlDbCritical.Leave();

	return cacheFileName;
}


wxString SjTempNCache::LookupCache(const wxString& orgFileName, bool lock)
{
	if( InitDb() )
	{
		long            id = 0;

		if(lock) m_sqlDbCritical.Enter();
		{
			wxSqlt sql(m_sqlDb);
			sql.Query(wxT("SELECT id FROM sjcache WHERE org='") + sql.QParam(orgFileName) + wxT("';"));
			if( sql.Next() )
			{
				id = sql.GetLong(0);
			}
		}
		if( lock ) m_sqlDbCritical.Leave();

		if( id )
		{
			wxString tempPath = LookupUnmanagedTemp(GetIDdName(orgFileName, id));
			if( !tempPath.IsEmpty() )
			{
				wxFileName fn(tempPath);
				fn.Touch();
				return tempPath;
			}
			else
			{
				if(lock) m_sqlDbCritical.Enter();
				{
					wxSqlt sql(m_sqlDb);
					sql.Query(wxString::Format(wxT("DELETE FROM sjcache WHERE id=%lu;"), id));

					m_sqlDbProtectedIds.Remove(id);
				}
				if( lock ) m_sqlDbCritical.Leave();
			}
		}

	}
	return wxT("");
}


void SjTempNCache::RemoveFromCache(const wxString& orgFileName, bool neverCacheAgain)
{
	if( InitDb() )
	{
		long id = 0;

		m_sqlDbCritical.Enter();
		{
			wxSqlt sql(m_sqlDb);
			sql.Query(wxT("SELECT id FROM sjcache WHERE org='") + sql.QParam(orgFileName) + wxT("';"));
			if( sql.Next() )
			{
				id = sql.GetLong(0);

				wxString tempPath = LookupUnmanagedTemp(GetIDdName(orgFileName, id));
				if( !tempPath.IsEmpty() )
				{
					RemoveFromUnmanagedTemp(tempPath);
				}

				sql.Query(wxString::Format(wxT("DELETE FROM sjcache WHERE id=%lu;"), (unsigned long)id));

				m_sqlDbProtectedIds.Remove(id);
			}

			if( neverCacheAgain )
			{
				m_sqlDbNeverCache.Insert(orgFileName, 1);
			}
		}
		m_sqlDbCritical.Leave();
	}
}


/*******************************************************************************
 * Cleaning up
 ******************************************************************************/


SjDirIterator::SjDirIterator(const wxString& dir)
{
	m_dir = dir;
	m_handle = NULL;
	m_handle2 = NULL;
}


SjDirIterator::~SjDirIterator()
{
}


bool SjDirIterator::Iterate()
{
	if( m_handle == NULL )
	{
		m_fullPath = ::wxFindFirstFile(m_dir+wxT("*.*"), 0);
		m_handle = (void*)1;
	}
	else
	{
		m_fullPath = ::wxFindNextFile();
	}

	if( m_fullPath.IsEmpty() )
	{
		return FALSE;
	}

	wxStructStat buf;
	wxStat(m_fullPath, &buf);
	m_bytes   = buf.st_size;
	m_modTime = wxDateTime(buf.st_mtime);

	return TRUE;
}


class SjTempDirEntry
{
public:
	SjTempDirEntry(const wxString& tempPath, long id, long bytes, wxDateTime modTime)
	{
		m_tempPath  = tempPath;
		m_id        = id;
		m_bytes     = bytes;
		m_modTime   = modTime;
	}
	wxString    m_tempPath;
	long        m_id;
	long        m_bytes;
	wxDateTime  m_modTime;
};


WX_DECLARE_OBJARRAY(SjTempDirEntry, SjArrayTempDirEntry);
#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArrayTempDirEntry);


static int SjTempDirEntry_Sort(SjTempDirEntry** i1__, SjTempDirEntry** i2__)
{
	SjTempDirEntry *i1 = *i1__, *i2 = *i2__;

	if( i1->m_modTime == i2->m_modTime )
	{
		return 0;
	}

	return (i1->m_modTime < i2->m_modTime)? -1 : 1;
}


void SjTempNCache::CleanupFiles(long action)
{
	// for idle cleanup, this function is called from SjAutoCtrl::OnOneSecondTimer()

	// some files added since the last check?
	if( !(action&SJ_CLEANUP_FORCE) )
	{
		wxCriticalSectionLocker locker(m_sqlDbCritical);

		if( m_filesAdded < m_filesAddedMax )
		{
			// not enough files added ;-)
			return;
		}
	}

	// find out the max. size in bytes (leave a little headroom)
	wxLongLong maxBytes;
	{
		long maxMb = GetMaxMB();
		long headroomMb = maxMb / 16;
		if( headroomMb > 128 )
		{
			headroomMb = 128;
		}

		maxBytes = maxMb-headroomMb;
		maxBytes *= SJ_ONE_MB;
	}

	// go through directory and collect all files matching sj-12345678.ext
	SjArrayTempDirEntry dirEntries;
	wxLongLong          usedBytes;
	{
		SjDirIterator       di(m_tempDir);
		wxString            currIdHex;
		long                currId;
		while( di.Iterate() )
		{
			currIdHex = di.m_fullPath.AfterLast('-').BeforeFirst('.');
			if( currIdHex.Len()==8 )
			{
				currId = 0;
				if( !currIdHex.ToLong(&currId, 16) ) { currId = 0; }
				if( currId /*sj-00000000.db must not be deleted (note the headroom)*/ )
				{
					dirEntries.Add(new SjTempDirEntry(di.m_fullPath, currId, di.m_bytes, di.m_modTime));
				}
				usedBytes += di.m_bytes;
			}
		}

		// set up the new files added maximum
		{
			wxLongLong ll = usedBytes; ll /= SJ_ONE_MB; long usedMb = ll.GetLo();
			ll = maxBytes;  ll /= SJ_ONE_MB; long maxMb  = ll.GetLo();
			long freeMb = maxMb-usedMb;

			wxCriticalSectionLocker locker(m_sqlDbCritical);

			m_filesAddedMax = freeMb/10; // an avg. file size of 10 MB is very pessimistic
			if( m_filesAddedMax < SJ_MIN_FILESADDEDMAX ) m_filesAddedMax = SJ_MIN_FILESADDEDMAX;
			if( m_filesAddedMax > SJ_MAX_FILESADDEDMAX ) m_filesAddedMax = SJ_MAX_FILESADDEDMAX;
		}


		// too large?
		if( (usedBytes <= maxBytes && !(action&SJ_CLEANUP_ALL)) || dirEntries.GetCount() <= 0 )
		{
			// no ;-)
			return;
		}
	}

	// sort the array by the modification time
	dirEntries.Sort(SjTempDirEntry_Sort);

	// get all protected IDs
	// (we cannot use m_sqlDbProtectedIds here as the protection may also come from other
	// instancted
	SjLLHash allProtectedIds;
	{
		wxCriticalSectionLocker locker(m_sqlDbCritical);
		wxSqlt sql(m_sqlDb);
		sql.Query(wxT("SELECT id FROM sjcache WHERE method!=0;"));
		while( sql.Next() )
		{
			allProtectedIds.Insert(sql.GetLong(0), 1);
		}
	}

	// go through the array and remove the files physically,
	// collect the IDs to delete
	wxString idsToDel;
	{
		long dirEntriesCount = dirEntries.GetCount(), d;
		SjTempDirEntry* currDirEntry;
		for( d = 0; d < dirEntriesCount; d++ )
		{
			currDirEntry = &(dirEntries[d]);

			if( !allProtectedIds.Lookup(currDirEntry->m_id) )
			{
				RemoveFromUnmanagedTemp(currDirEntry->m_tempPath);
				idsToDel.Append(wxString::Format(wxT("%lu,")/*the last comma is removed below*/, currDirEntry->m_id));

				usedBytes -= currDirEntry->m_bytes;
				if( usedBytes <= maxBytes && !(action&SJ_CLEANUP_ALL) )
				{
					break; // enough deleted for now ;-)
				}
			}
		}
	}

	// remove the entries from the database
	if( idsToDel.Len() )
	{
		wxCriticalSectionLocker locker(m_sqlDbCritical);
		wxSqlt sql(m_sqlDb);
		idsToDel = idsToDel.Left(idsToDel.Len()-1/*remove trailing comma*/);
		sql.Query(wxString::Format(wxT("DELETE FROM sjcache WHERE id IN (%s);"), idsToDel.c_str()));
	}
}


wxString SjTempNCache::GetAsLocalFile(const wxString& url)
{
	wxString file = wxFileSystem::URLToFileName(url).GetFullPath(); // convert file:-URLs
	if( ::wxFileExists(file) )
	{
		// Fine: the URL is a local file
		return file;
	}

	// Bad: the URL must be copied to a local file
	wxFileSystem fs;
	wxFSFile* fsFile = fs.OpenFile(url, wxFS_READ);
	if( fsFile )
	{
		wxInputStream* stream = fsFile->GetStream();
		if( stream )
		{
			wxString tempFileName = g_tools->m_cache.AddToManagedTemp(SjTools::GetExt(url), SJ_TEMP_PROTECT_TIL_EXIT);
			wxFile tempFile(tempFileName, wxFile::write);
			if( tempFile.IsOpened()
			 && SjTools::CopyStreamToFile(*stream, tempFile) )
			{
				return tempFileName; // copy success
			}
		}
	}

	// error, cannot retrieve the URL
	return "";
}

