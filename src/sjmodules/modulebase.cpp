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
 * File:    modulebase.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke module system class
 *
 *******************************************************************************
 *
 * Module unloading is delayed 10 seconds. If using "real" modules, this solves
 * a lot of problems of heavy loading/unloading modules eg. on updating the
 * music library. See the *Pending*() functions for details.
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/fs_zip.h>
#include <wx/fs_mem.h>
#include <wx/tokenzr.h>
#include <sjmodules/internalinterface.h>
#include <tagger/tg_a_tagger_frontend.h>
#include <sjtools/console.h>
#include <sjtools/fs_inet.h>
#include <see_dom/sj_see.h>

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(SjModuleList);
WX_DEFINE_LIST(SjInterfaceList);


/*******************************************************************************
 * SjModuleSystem
 ******************************************************************************/


wxSqltDb* SjModuleSystem::s_delayedDbDelete = NULL;


SjModuleSystem::SjModuleSystem()
{
	// init image and filesystem handlers;
	// this should be done very soon, not in Init() as
	// eg. the skin is loaded before SjModuleSystem::Init()
	// and relies on this stuff
	static bool imageHandlersInitialized = FALSE;
	if( !imageHandlersInitialized )
	{
		::wxInitAllImageHandlers();
		wxFileSystem::AddHandler(new wxZipFSHandler);
		wxFileSystem::AddHandler(new wxMemoryFSHandler);
		wxFileSystem::AddHandler(new SjInternetFSHandler); // needed to play http streams
		SjInitID3Etc(TRUE/*init file system handler*/);

		InitMemoryFS(); // in data<os>.cpp, eg. datawin.cpp

		imageHandlersInitialized = TRUE;
	}
}


SjModuleSystem::~SjModuleSystem()
{
	Exit();
}


void SjModuleSystem::Init()
{
	// add extensions read by wxImage
	wxList&         imgHandlerList = wxImage::GetHandlers();
	wxList::Node*   imgNode = imgHandlerList.GetFirst();
	wxImageHandler* imgHandler;
	while( imgNode )
	{
		imgHandler = (wxImageHandler*)imgNode->GetData();
		if( imgHandler )
		{
			const wxString& ext = imgHandler->GetExtension();
			m_imageExtList.AddExt(ext);
		}

		imgNode = imgNode->GetNext();
	}

	// handle some incorrectly names extensions that are still okay for wxImage
	wxArrayString stubExt = GetStubImageExt();
	size_t i;
	for( i = 0; i < stubExt.GetCount(); i++ )
	{
		m_imageExtList.AddExt(stubExt.Item(i));
	}

	// add playlist extensions
	m_playlistExtListRead. AddExt(wxT("m3u,m3u8,pls,cue,wpl,xml,xspf"));
	m_playlistExtListWrite.AddExt(wxT("m3u,m3u8,pls,cue,xspf"));


	// init database...
	{
		wxLogInfo(wxT("Loading %s"), g_tools->m_dbFile.c_str());
		SjBusyInfo::Set(g_tools->m_dbFile, TRUE);

		// ...open database
		wxSqltDb* db = new wxSqltDb(g_tools->m_dbFile);
		if( !db->Ok() ) { SjMainApp::FatalError(); }
		db->SetDefault();

		// ...the database version: if the major version of the database is
		// _larger_, the database cannot be opened.  However, the version
		// normally does not change at all as we can add tables and fields as
		// needed.
		#define CURR_DB_VERSION 1L

		// ...init database...
		if( !db->ExistsBeforeOpening() )
		{

			// first opening...
			wxSqlt sql;

			db->SetCache(g_tools->m_config->Read(wxT("main/idxCacheBytes"), SJ_DEF_SQLITE_CACHE_BYTES));
			db->SetSync(g_tools->m_config->Read(wxT("main/idxCacheSync"), SJ_DEF_SQLITE_SYNC));

			// ...create silverjuke header table
			sql.ConfigWrite(wxT("dbversion"), CURR_DB_VERSION);
			sql.ConfigWrite(wxT("created"), (long)wxDateTime::Now().GetAsDOS());

			// ...create arts tables, this table is used by several modules
			sql.Query(wxT("CREATE TABLE arts (id INTEGER PRIMARY KEY, url TEXT, operations TEXT);"));
			sql.Query(wxT("CREATE INDEX artsindex01 ON arts (url);"));
		}
		else
		{
			// subsequent opening
			wxSqlt sql;
			if( sql.ConfigRead(wxT("dbversion"), -1) != CURR_DB_VERSION ) {
				// normally, this should not happen; the database format should be stable.
				wxMessageBox(wxString::Format(wxT("Bad version of the jukebox file \"%s\". Delete the file and try over; we'll recreate a more correct version then. For now, the program will terminate.")/*n/t*/, g_tools->m_dbFile.c_str()));
				SjMainApp::FatalError();
			}
		}
	}
}


void SjModuleSystem::Exit()
{
	int i;

	// unload all scripts
	#if SJ_USE_SCRIPTS
	for( i = 0; i < (int)m_sees.GetCount(); i++ )
	{
		SjSee* see = (SjSee*)m_sees[i];
		delete see;
	}
	m_sees.Empty();
	#endif

	// unload all modules -- we do this in two steps: first the plugins, then the internal modules
	SjModuleList::Node* moduleNode;
	for( i = 0; i < 2; i++ )
	{
		moduleNode = m_listOfModules.GetFirst();
		while( moduleNode )
		{
			SjModule* module = moduleNode->GetData();
			wxASSERT(module);

			if( (i==0 && module->m_interface != g_internalInterface)
			        || (i==1 && module->m_interface == g_internalInterface) )
			{
				long iteration = 0;
				while( module->IsLoaded() && iteration < 100 )
				{
					module->Unload();
					iteration++;
				}
				wxASSERT( !module->IsLoaded() );
			}

			// next module
			moduleNode = moduleNode->GetNext();
		}
	}

	UnloadPending(TRUE /*ignoreTimeouts*/);
	wxASSERT( m_listOfPendingModules.GetCount() == 0 );

	// delete all modules, should be done after unloading as
	// some modules keep pointers to other loaded modules
	moduleNode = m_listOfModules.GetFirst();
	while( moduleNode )
	{
		delete moduleNode->GetData();
		delete moduleNode;
		moduleNode = m_listOfModules.GetFirst();
	}

	// delete all module interfaces
	SjInterfaceList::Node* interfaceNode = m_listOfInterfaces.GetFirst();
	while( interfaceNode )
	{
		SjInterfaceBase* interf = interfaceNode->GetData();
		wxASSERT(interf);

		// next module
		delete interf;
		delete interfaceNode;
		interfaceNode = m_listOfInterfaces.GetFirst();
	}
	m_listOfInterfaces.Clear();

	// delete module lists
	for( i = 0; i < SJ_MODULETYPE_COUNT; i++ )
	{
		m_listOfModules2[i].Clear();
	}

	m_hashOfExt.Clear();

	// exit database
	wxSqltDb* db = wxSqltDb::GetDefault();
	if( db )
	{
		// NULL the pointer (the default database);
		// real deletion is done after ::GcShutdown() in SjMainApp
		// (this is needed as
		wxSqltDb::ClearDefault();
		s_delayedDbDelete = db;
	}
}


void SjModuleSystem::AddInterface(SjInterfaceBase* interf)
{
	interf->m_moduleSystem = this;
	m_listOfInterfaces.Append(interf);
}


wxArrayString SjModuleSystem::GetStubImageExt() const
{
	// the function retuns a list of "incorrect" extensions
	// which will also be supported by wxImage

	wxArrayString ret;

#if wxUSE_LIBTIFF
	ret.Add(wxT("tiff"));
#endif

#if wxUSE_LIBJPEG
	ret.Add(wxT("jpe"));
	ret.Add(wxT("jpeg"));
	ret.Add(wxT("jif"));
#endif

	return ret;
}


int SjModuleSystem_CmpModulesByAll(const SjModule** m1, const SjModule** m2)
{
	wxASSERT(m1 && *m1 && m2 && *m2);

	// in versions before 1.23beta1, the internal interface was compared by a name of " ".
	// why? to put it up in the list?
	wxString interf1 = (*m1)->m_interface->GetName().c_str();
	wxString interf2 = (*m2)->m_interface->GetName().c_str();

	if( (*m1)->m_type == (*m2)->m_type )
	{
		if( (*m1)->m_sort == (*m2)->m_sort )
		{
			return (*m1)->m_name.CmpNoCase((*m2)->m_name);
		}

		return (*m1)->m_sort - (*m2)->m_sort;
	}

	return (*m1)->m_type - (*m2)->m_type;
}


int SjModuleSystem_CmpModulesByName(const SjModule** m1, const SjModule** m2)
{
	wxASSERT(m1 && *m1 && m2 && *m2);

	// in versions before 1.23beta1, the internal interface was compared by a name of " ".
	// why? to put it up in the list?
	wxString interf1 = (*m1)->m_interface->GetName().c_str();
	wxString interf2 = (*m2)->m_interface->GetName().c_str();

	return (*m1)->m_name.CmpNoCase((*m2)->m_name);
}


void SjModuleSystem::LoadModules()
{
	// This function may only be called from the main thread.
#ifdef __WXDEBUG__
	wxASSERT( wxThread::IsMain() );
	static bool s_functionCalled = false;
	wxASSERT( !s_functionCalled );
	s_functionCalled = true;
#endif

	// get modules
	SjInterfaceList::Node* interfaceNode = m_listOfInterfaces.GetFirst();
	while( interfaceNode )
	{
		SjInterfaceBase* interf = interfaceNode->GetData();
		wxASSERT(interf);

		// get the modules from the interface
		interf->LoadModules(m_listOfModules);

		// continue with next interface
		interfaceNode = interfaceNode->GetNext();
	}

	m_listOfModules.Sort(SjModuleSystem_CmpModulesByAll);

	// create type lists
	int type;
	for( type = 1; type < SJ_MODULETYPE_COUNT; type++ )
	{
		m_listOfModules2[type].Clear();
		SjModuleList::Node* moduleNode = m_listOfModules.GetFirst();
		while( moduleNode )
		{
			SjModule* module = moduleNode->GetData();
			wxASSERT(module);

			if( module->m_type == type )
			{
				m_listOfModules2[type].Append(module);
			}

			moduleNode = moduleNode->GetNext();
		}
	}

	m_listOfModules2[SJ_MODULETYPE_VIS_RENDERER].Sort(SjModuleSystem_CmpModulesByName);
}


SjModuleList* SjModuleSystem::GetModules(SjModuleType type)
{
	wxASSERT(type >= 0 && type < SJ_MODULETYPE_COUNT);

	if( type == 0 )
	{
		// get all modules
		return &m_listOfModules;
	}
	else
	{
		return &m_listOfModules2[type];
	}
}


SjModule* SjModuleSystem::FindModuleByFile(const wxString& file, int fileIndex)
{
	SjModuleList*       list = GetModules(SJ_MODULETYPE_ALL);
	SjModuleList::Node* moduleNode = list->GetFirst();
	SjModule*           module;
	while( moduleNode )
	{
		module = moduleNode->GetData();
		wxASSERT(module);

		if( module->m_file.CmpNoCase(file) == 0
		        && module->m_fileIndex == fileIndex )
		{
			return module;
		}

		moduleNode = moduleNode->GetNext();
	}

	return NULL;
}


SjScannerModule* SjModuleSystem::FindScannerModuleByUrl(const wxString& url)
{
	SjModuleList*       scannerList = GetModules(SJ_MODULETYPE_SCANNER);
	SjModuleList::Node* scannerNode = scannerList->GetFirst();
	SjScannerModule*    scannerModule;
	while( scannerNode )
	{
		scannerModule = (SjScannerModule*)scannerNode->GetData();
		wxASSERT( scannerModule );
		wxASSERT( scannerModule->IsLoaded() );

		if( scannerModule->IsMyUrl(url) )
		{
			return scannerModule;
		}
	}
	return NULL;
}


SjExtList SjModuleSystem::GetAssignedExt(long flags)
{
	SjExtList       ret;

	// directly playable files
	if( flags & SJ_EXT_MUSICFILES )
	{
		wxBusyCursor busy;
		ret.AddExt(g_mainFrame->m_player.GetExtList());
	}

	// files types read along with the playable files
	if( flags & SJ_EXT_ARCHIVES )
	{
		ret.AddExt(wxT("zip"));
	}

	if( flags & SJ_EXT_IMAGEFILES )
	{
		ret.AddExt(m_imageExtList);
	}

	if( flags & SJ_EXT_KARAOKE )
	{
		ret.AddExt(wxT("cdg")); // graphics from from mp3+g
		ret.AddExt(wxT("lrc"));
	}

	// other file types
	if( flags & SJ_EXT_PLAYLISTS_READ )
	{
		ret.AddExt(m_playlistExtListRead);
	}

	if( flags & SJ_EXT_PLAYLISTS_WRITE )
	{
		ret.AddExt(m_playlistExtListWrite);
	}

	if( flags & SJ_EXT_SKINFILES )
	{
		ret.AddExt(wxT("sjs"));
		ret.AddExt(wxT("xml"));
	}

	if( flags & SJ_EXT_LANGUAGEFILES )
	{
		ret.AddExt(wxT("mo"));
	}

	return ret;
}


void SjModuleSystem::BroadcastMsg(int msg)
{
	static int inSendMsg = 0;

	wxASSERT( wxThread::IsMain() );
	wxASSERT( msg!=0 );

	if( inSendMsg == msg )
	{
		wxLogError(wxT("Recursive call of SjModuleSystem::SendMsg (msg=%i).")/*n/t*/, msg);
	}
	else
	{
		inSendMsg = msg;

		SjInterfaceList::Node* interfaceNode = m_listOfInterfaces.GetFirst();
		while( interfaceNode )
		{
			SjInterfaceBase* interf = interfaceNode->GetData();
			wxASSERT(interf);

			interf->ReceiveMsg(msg);

			interfaceNode = interfaceNode->GetNext();
		}

		// send message to all modules
		SjModuleList::Node* moduleNode = m_listOfModules.GetFirst();
		while( moduleNode )
		{
			SjModule* module = moduleNode->GetData();
			wxASSERT(module);
			wxASSERT(module->GetUsage() >= 0);

			if( module->IsLoaded() )
			{
				module->ReceiveMsg(msg);
			}

			moduleNode = moduleNode->GetNext();
		}

		inSendMsg = 0;
	}
}


/*******************************************************************************
 * SjModuleSystem - handling pending modules
 ******************************************************************************/


#define SJ_PENDING_MODULES_MS 20000L /*20 seconds*/


void SjModuleSystem::PushPending(SjModule* m, bool destroyModulePointer)
{
	wxASSERT( wxThread::IsMain() );
	wxASSERT( m );
	wxASSERT( !m->IsLoaded() );
	wxASSERT( m->m_pushPendingTimestamp == 0 );
	wxASSERT( PopPending(m)==FALSE );

	// add to list
	m->m_pushPendingTimestamp = SjTools::GetMsTicks();
	m->m_destroyPending       = destroyModulePointer;
	m_listOfPendingModules.Append(m);
}


bool SjModuleSystem::PopPending(SjModule* m)
{
	wxASSERT( wxThread::IsMain() );
	wxASSERT( m );
	wxASSERT( !m->IsLoaded() );

	// module in list of pending modules?
	SjModuleList::Node* node = m_listOfPendingModules.Find(m);
	if( node )
	{
		// yes: remove from list
		m_listOfPendingModules.DeleteNode(node);
		m->m_pushPendingTimestamp   = 0;
		m->m_destroyPending         = FALSE;
		return TRUE;
	}

	// no.
	return FALSE;
}


void SjModuleSystem::UnloadPending(bool ignoreTimestamp)
{
	wxASSERT( wxThread::IsMain() );

	// physicall unload pending modules
	unsigned long thisTimestamp = SjTools::GetMsTicks();
	SjModuleList::Node* node = m_listOfPendingModules.GetFirst();
	while( node )
	{
		SjModule* m = node->GetData();

		wxASSERT( m );
		wxASSERT( m->m_pushPendingTimestamp != 0 );

		if( ignoreTimestamp
		        || SjTimestampDiff(m->m_pushPendingTimestamp, thisTimestamp) > SJ_PENDING_MODULES_MS )
		{
			m->m_pushPendingTimestamp = 0; // prepare for the next pending
			m->LastUnload();

			// also delete the module pointer itself?
			if( m->m_destroyPending )
			{
				delete m;
			}

			m_listOfPendingModules.DeleteNode(node);

			node = m_listOfPendingModules.GetFirst(); // start over
		}
		else
		{
			node = node->GetNext(); // next in list
		}
	}
}


/*******************************************************************************
 *  SjInterfaceBase
 ******************************************************************************/


SjInterfaceBase::SjInterfaceBase(const wxString& name)
{
	m_name          = name;
	m_moduleSystem  = NULL;
}


static bool s_searchedForScripts = false;
void SjInterfaceBase::AddModulesFromDir(SjModuleList& list, const wxString& dirName, bool suppressNoAccessErrors)
{
	wxFileSystem        fs;
	wxString            entryStr;
	wxArrayString       entryStrings;
	size_t              e;

	fs.ChangePathTo(dirName, TRUE);

	// first, go through some special subdirectories.
	// we don't recurse all directories for speed reasons
	// as the program path is also in the search path and we don't know
	// where it is and how many thousands file we have to scan.

	{
		// Non-privileged users may not have access to the directory.
		// Do not log errors in this case.
		SjLogString logToString(suppressNoAccessErrors);

#ifdef __WXMSW__
		entryStr = fs.FindFirst(wxT("*.*"), wxDIR);
#else
		entryStr = fs.FindFirst(wxT("*"), wxDIR);
#endif
	}

	while( !entryStr.IsEmpty() )
	{
		wxString lastDir = g_tools->GetLastDir(entryStr).Lower();
		if( lastDir == wxT("plugins")           // common
		        || lastDir == wxT("plug-ins")          // common
		        || lastDir == wxT("modules")           // common
		        || lastDir == wxT("input")             // used eg. by XMMS
		        || lastDir == wxT("visualization")     // used eg. by XMMS
		  )
		{
			entryStrings.Add(entryStr);
		}

		entryStr = fs.FindNext();
	}

	for( e = 0; e < entryStrings.GetCount(); e++ )
	{
		AddModulesFromDir(list, entryStrings.Item(e), suppressNoAccessErrors);
	}

	// search for DLLs

	entryStrings.Clear();

	{
		// Non-privileged users may not have access to the directory.
		// Do not log errors in this case.
		SjLogString logToString(suppressNoAccessErrors);

#if defined(__WXMSW__)
		entryStr = fs.FindFirst(wxT("*.dll"), wxFILE);
#elif defined(__WXMAC__)
		entryStr = fs.FindFirst(wxT("*.dylib"), wxFILE);
#else
		entryStr = fs.FindFirst(wxT("*.so"), wxFILE);
#endif
	}

	while( !entryStr.IsEmpty() )
	{
		entryStrings.Add(entryStr);
		entryStr = fs.FindNext();
	}

	for( e = 0; e < entryStrings.GetCount(); e++ )
		AddModulesFromFile(list, entryStrings.Item(e), suppressNoAccessErrors);

	// search for scripts

	if( !s_searchedForScripts )
	{
		entryStrings.Clear();
		entryStr = fs.FindFirst(wxT("*.sj"), wxFILE);
		while( !entryStr.IsEmpty() )
		{
			m_moduleSystem->m_scripts.Add(entryStr);
			entryStr = fs.FindNext();
		}
	}
}


void SjInterfaceBase::AddModulesFromSearchPaths(SjModuleList& list, bool suppressNoAccessErrors)
{
	int searchPathCount = g_tools->GetSearchPathCount();
	int searchPathIndex;
	for( searchPathIndex = 0; searchPathIndex < searchPathCount; searchPathIndex++ )
	{
		AddModulesFromDir(list, g_tools->GetSearchPath(searchPathIndex), suppressNoAccessErrors);
	}

	s_searchedForScripts = true;
}


bool SjInterfaceBase::IsModuleAdded(SjModuleList& list,
                                    const wxString& file, int fileIndex, const wxString& name)
{
	SjModuleList::Node* currModuleNode = list.GetFirst();
	while( currModuleNode )
	{
		SjModule* currModule = currModuleNode->GetData();
		wxASSERT(currModule);

		if( currModule->m_interface == this )
		{
			if( currModule->m_file.CmpNoCase(file)==0
			        && currModule->m_fileIndex == fileIndex )
			{
				return TRUE;
			}

			if( currModule->m_name == name )
			{
				/*wxLogWarning(wxT("The Modules \"%s\" and \"%s\" seems to be identical.") n/t,
				    currModule->m_file.c_str(), file.c_str());
				*/
				return TRUE;
			}
		}

		// next
		currModuleNode = currModuleNode->GetNext();
	}

	return FALSE;
}


void SjInterfaceBase::WriteToCache(const wxString& file, const wxString& info__, unsigned long fileTimestamp)
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	// get key
	wxString key(SjModule::GetUniqueStrId(m_name+file, 0));

	// create file timestamp if not given
	if( fileTimestamp == 0 )
	{
		if( ::wxFileExists(file) )
		{
			size_t fileModTime = ::wxFileModificationTime(file);
			fileTimestamp = (unsigned long)fileModTime;
		}
	}

	// init info with timestamp
	wxString info = wxString::Format(wxT("%lu:"), fileTimestamp);

	// add user stuff to info
	wxStringTokenizer tkz(info__, wxT("\t"), wxTOKEN_RET_EMPTY_ALL);
	wxString currToken;
	while( tkz.HasMoreTokens() )
	{
		currToken = tkz.GetNextToken();
		info += wxString::Format(wxT("%i:%s:"), (int)currToken.Len(), currToken.c_str());
	}

	// write to cache
	g_tools->m_config->Write(wxT("modulecache/") + key, info);
}


bool SjInterfaceBase::ReadFromCache(const wxString& file, wxArrayString& retInfo, unsigned long fileTimestamp)
{
	retInfo.Clear();

	// get key
	wxString key(SjModule::GetUniqueStrId(m_name+file, 0));

	// create file timestamp if not given
	if( fileTimestamp == 0 )
	{
		if( ::wxFileExists(file) )
		{
			size_t fileModTime = ::wxFileModificationTime(file);
			fileTimestamp = (unsigned long)fileModTime;
		}
	}

	// get info from cache as "<timestamp>:<len1>:<val1>:<len2>:<val2>:<len3> ..."
	wxString info(g_tools->m_config->Read(wxT("modulecache/") + key, wxT("")));
	if( info.IsEmpty() )
	{
		return FALSE; // not in cache
	}

	// get cache timestamp
	long l;
	info.BeforeFirst(wxT(':')).ToLong(&l, 10);
	if( (unsigned long)l != fileTimestamp )
	{
		return FALSE; // cache out of date
	}
	info = info.AfterFirst(wxT(':'));

	// get cache data
	long currLen;
	while( info.BeforeFirst(wxT(':')).ToLong(&currLen, 10) )
	{
		info = info.AfterFirst(wxT(':'));

		if( currLen >= (long)info.Len() ) { return FALSE; }
		retInfo.Add(info.Left(currLen));

		info = info.Mid(currLen+1);
	}

	return TRUE;
}


wxString SjInterfaceBase::ReadFromCache(const wxString& file, unsigned long fileTimestamp)
{
	wxArrayString a;
	ReadFromCache(file, a, fileTimestamp);
	return a.GetCount()? a.Item(0).c_str() : wxT("");
}


/*******************************************************************************
 *  SjCol
 ******************************************************************************/


void SjCol::AddRow(SjRow* row)
{
	wxASSERT(m_rowCount <= m_rowMaxCount);

	if( m_rowCount == m_rowMaxCount )
	{
		SjRow** oldRows = m_rows;

		m_rowMaxCount = m_rowMaxCount*2;
		if( m_rowMaxCount < 32 ) m_rowMaxCount = 32;
		m_rows = new SjRow*[m_rowMaxCount];
		if( m_rows == NULL )
		{
			wxLogFatalError(wxT("Out of memory on allocating rows."));
		}

		if( oldRows )
		{
			long i;
			for( i = 0; i < m_rowCount; i++ )
			{
				m_rows[i] = oldRows[i];
			}
			delete [] oldRows;
		}
	}

	row->m_col = this;
	m_rows[m_rowCount] = row;
	m_rowCount++;
}


SjRow* SjCol::RemoveRow(SjRow* row)
{
	long i;
	for( i = 0; i < m_rowCount; i++ )
	{
		if( m_rows[i] == row )
		{
			for( /*leave i*/; i < m_rowCount-1; i++ )
			{
				m_rows[i] = m_rows[i+1];
			}
			m_rowCount--;
			return row;
		}
	}
	return NULL;
}


void SjCol::DestroyRows()
{
	if( m_rows )
	{
		int r;
		for( r = 0; r < m_rowCount; r++ )
		{
			delete m_rows[r];
		}
		delete [] m_rows;
	}

	m_rowCount = 0;
	m_rows = NULL;
}


SjRow* SjCol::FindSelectableRow(long startIndex, long inc)
{
	long r;

	if( inc > 0 )
	{
		if( startIndex < 0 ) startIndex = 0;
		for( r = startIndex; r < m_rowCount; r++ )
		{
			if( m_rows[r]->IsSelectable() == 2 )
			{
				return m_rows[r];
			}
		}
	}
	else
	{
		if( startIndex < 0 ) startIndex = m_rowCount-1;
		for( r = startIndex; r >= 0; r-- )
		{
			if( m_rows[r]->IsSelectable() == 2 )
			{
				return m_rows[r];
			}
		}
	}

	return NULL;
}


SjRow* SjCol::FindFirstSelectedRow()
{
	if( m_rows )
	{
		int r;
		for( r = 0; r < m_rowCount; r++ )
		{
			if( m_rows[r]->IsSelected() )
				return m_rows[r];
		}
	}
	return NULL;
}


void SjCol::SelectAllRows(bool select)
{
	if( m_rows )
	{
		int r;
		for( r = 0; r < m_rowCount; r++ )
		{
			if( m_rows[r]->IsSelectable() == 2 )
				m_rows[r]->Select(select);
		}
	}
}


/*******************************************************************************
 *  SjModule
 ******************************************************************************/


SjModule::SjModule(SjInterfaceBase* interf, SjModuleType type)
{
	m_type                  = type;
	m_interface             = interf;
	m_usage                 = 0;
	m_fileIndex             = 0;
	m_sort                  = 1000;

	m_pushPendingTimestamp  = 0;
	m_destroyPending        = FALSE;
}


bool SjModule::Load()
{
	bool ret = TRUE;

	wxASSERT( m_usage>=0 );
	wxASSERT( wxThread::IsMain() );

	if( m_usage==0 )
	{
		if( !m_interface->m_moduleSystem->PopPending(this) )
		{
			ret = FirstLoad();
		}
	}

	if( ret )
	{
		m_usage++;
	}

	// unload any pending modules if the timeout has expired
	m_interface->m_moduleSystem->UnloadPending();

	return ret;
}


void SjModule::Unload()
{
	wxASSERT( wxThread::IsMain() );

	if( m_usage > 0 ) // invalid calls to Unload() may happen on shutdown
	{
		m_usage--;

		if( m_usage==0 )
		{
			m_interface->m_moduleSystem->PushPending(this);
		}

		// unload any pending modules if the timeout has expired,
		// do not do this if the current module was not loaded as this may indicate a shutdown
		m_interface->m_moduleSystem->UnloadPending();
	}
}


void SjModule::UnloadAndDestroy()
{
	wxASSERT( m_usage );
	wxASSERT( m_interface->m_moduleSystem->m_listOfModules.Find(this) == NULL );

	m_usage = 0;
	m_interface->m_moduleSystem->PushPending(this, TRUE/*destroyModulePointer*/);
}


wxString SjModule::PackFileName(const wxString& file, int fileIndex)
{
	if( fileIndex )
	{
		return wxString::Format(wxT("%s,%i"), file.c_str(), fileIndex);
	}
	else
	{
		return file;
	}
}


void SjModule::UnpackFileName(const wxString& inFileNIndex, wxString& retFile, int& retIndex)
{
	retIndex = 0;
	retFile = inFileNIndex.BeforeLast(',');
	if( retFile.IsEmpty() )
	{
		retFile = inFileNIndex;
	}
	else
	{
		wxString retIndexStr = inFileNIndex.AfterLast(',');
		long l;
		if( retIndexStr.ToLong(&l) )
		{
			retIndex = l;
		}
	}
}


wxString SjModule::GetUniqueStrId(const wxString& file, int fileIndex)
{
	wxString str(PackFileName(file, fileIndex));

	str.Replace(wxT("."),  wxT("X"));
	str.Replace(wxT(":"),  wxT("X"));
	str.Replace(wxT("/"),  wxT("X"));
	str.Replace(wxT("\\"), wxT("X"));

	return ::SjNormaliseString(str, 0);
}


/*******************************************************************************
 *  SjTrackInfo
 ******************************************************************************/


void SjTrackInfo::ClearLongs()
{
	m_id            = 0;
	m_updatecrc     = 0;
	m_timeAdded     = 0;
	m_timeModified  = 0;
	m_timesPlayed   = 0;
	m_lastPlayed    = 0;
	m_dataBytes     = 0;
	m_bitrate       = 0;
	m_samplerate    = 0;
	m_channels      = 0;
	m_playtimeMs    = 0;
	m_autoVol       = 0;
	m_trackNr       = 0;
	m_trackCount    = 0;
	m_diskNr        = 0;
	m_diskCount     = 0;
	m_beatsPerMinute= 0;
	m_rating        = 0;
	m_year          = 0;
	m_validFields   = 0;
}


void SjTrackInfo::ClearStrings()
{
	m_url           .Empty();
	m_trackName     .Empty();
	m_leadArtistName.Empty();
	m_orgArtistName .Empty();
	m_composerName  .Empty();
	m_albumName     .Empty();
	m_genreName     .Empty();
	m_groupName     .Empty();
	m_comment       .Empty();
	m_arts          .Empty();
}


SjTrackInfo& SjTrackInfo::operator = (const SjTrackInfo& o)
{
	m_id            = o.m_id;
	m_updatecrc     = o.m_updatecrc;
	m_timeAdded     = o.m_timeAdded;
	m_timeModified  = o.m_timeModified;
	m_timesPlayed   = o.m_timesPlayed;
	m_lastPlayed    = o.m_lastPlayed;
	m_dataBytes     = o.m_dataBytes;
	m_bitrate       = o.m_bitrate;
	m_samplerate    = o.m_samplerate;
	m_channels      = o.m_channels;
	m_playtimeMs    = o.m_playtimeMs;
	m_autoVol       = o.m_autoVol;
	m_trackNr       = o.m_trackNr;
	m_trackCount    = o.m_trackCount;
	m_diskNr        = o.m_diskNr;
	m_diskCount     = o.m_diskCount;
	m_beatsPerMinute= o.m_beatsPerMinute;
	m_rating        = o.m_rating;
	m_year          = o.m_year;
	m_validFields   = o.m_validFields;

	m_url           = o.m_url;
	m_trackName     = o.m_trackName;
	m_leadArtistName= o.m_leadArtistName;
	m_orgArtistName = o.m_orgArtistName;
	m_composerName  = o.m_composerName;
	m_albumName     = o.m_albumName;
	m_genreName     = o.m_genreName;
	m_groupName     = o.m_groupName;
	m_comment       = o.m_comment;
	m_arts          = o.m_arts;

	return *this;
}


bool SjTrackInfo::AddMerge(const SjTrackInfo& o)
{
	bool sthModified = FALSE;

#define DO_MERGE(f, n) \
     if( (o.m_validFields & (f)) && (n != o.n) ) \
     { \
         n = o.n; \
         sthModified = TRUE; \
     }

	DO_MERGE(SJ_TI_TRACKNAME,       m_trackName)
	DO_MERGE(SJ_TI_TRACKNR,         m_trackNr)
	DO_MERGE(SJ_TI_TRACKCOUNT,      m_trackCount)
	DO_MERGE(SJ_TI_DISKNR,          m_diskNr)
	DO_MERGE(SJ_TI_DISKCOUNT,       m_diskCount)
	DO_MERGE(SJ_TI_LEADARTISTNAME,  m_leadArtistName)
	DO_MERGE(SJ_TI_ORGARTISTNAME,   m_orgArtistName)
	DO_MERGE(SJ_TI_COMPOSERNAME,    m_composerName)
	DO_MERGE(SJ_TI_ALBUMNAME,       m_albumName)
	DO_MERGE(SJ_TI_GENRENAME,       m_genreName)
	DO_MERGE(SJ_TI_GROUPNAME,       m_groupName)
	DO_MERGE(SJ_TI_COMMENT,         m_comment)
	DO_MERGE(SJ_TI_BEATSPERMINUTE,  m_beatsPerMinute)
	DO_MERGE(SJ_TI_RATING,          m_rating)
	DO_MERGE(SJ_TI_YEAR,            m_year)

	return sthModified;
}


wxString SjTrackInfo::GetFormattedValue(long ti) const
{
	switch( ti )
	{
		case SJ_TI_YEAR:            if( m_year <= 0 )           return wxT(""); /*else*/ break;
		case SJ_TI_TRACKNR:         if( m_trackNr <= 0 )        return wxT(""); /*else*/ break;
		case SJ_TI_TRACKCOUNT:      if( m_trackCount <= 0 )     return wxT(""); /*else*/ break;
		case SJ_TI_DISKNR:          if( m_diskNr <= 0 )         return wxT(""); /*else*/ break;
		case SJ_TI_DISKCOUNT:       if( m_diskCount <= 0 )      return wxT(""); /*else*/ break;
		case SJ_TI_BEATSPERMINUTE:  if( m_beatsPerMinute <= 0 ) return wxT(""); /*else*/ break;

		case SJ_TI_PLAYTIMEMS:
			if( m_playtimeMs <= 0 ) return wxT("");
			return SjTools::FormatTime(m_playtimeMs/1000);

		case SJ_TI_X_TIMEADDED:
			return SjTools::FormatDate(m_timeAdded, SJ_FORMAT_ADDTIME);

		case SJ_TI_X_TIMEMODIFIED:
			if( m_timeModified <= 0 ) return wxT("");
			return SjTools::FormatDate(m_timeModified, SJ_FORMAT_ADDTIME);

		case SJ_TI_X_LASTPLAYED:
			if( m_lastPlayed <= 0 ) return wxT("");
			return SjTools::FormatDate(m_lastPlayed, SJ_FORMAT_ADDTIME);

		case SJ_TI_X_SAMPLERATE:
			if( m_samplerate <= 0 ) return wxT("");
			return SjTools::FormatNumber(m_samplerate);

		case SJ_TI_X_BITRATE:
			if( m_bitrate <= 0 ) return wxT("");
			return SjTools::FormatNumber(m_bitrate);

		case SJ_TI_X_DATABYTES:
			return SjTools::FormatBytes(m_dataBytes);

		case SJ_TI_X_AUTOVOL:
			if( m_autoVol <= 0 ) return wxT("");
			return SjTools::FormatGain(SjLong2Gain(m_autoVol), false);

		case SJ_TI_Y_FILETYPE:
			return SjTools::GetExt(m_url);

		case SJ_TI_Y_QUEUEPOS:
		{
			wxArrayLong pos;
			wxString ret;
			long queueCount = g_mainFrame->GetAllQueuePosByUrl(m_url, pos);
			for( long i = 0; i < queueCount; i++ )
			{
				if( i ) ret += wxT(", ");
				ret += wxString::Format(wxT("%i"), pos[i]+1);
			}
			return ret;
		}

		case SJ_TI_RATING:
			if( m_rating > 0 )
			{
				wxString chars;
				for( long i = 0; i < m_rating; i++)
				{
					chars += SJ_RATING_CHARS_ELSEWHERE;
				}
				return chars;
			}
			else
			{
				return wxT("");
			}

	}

	return GetValue(ti);
}


wxString SjTrackInfo::GetValue(long ti) const
{
#define RETURN_STRING(n) \
         return (n);
#define RETURN_LONG(n) \
         return wxString::Format(wxT("%i"), (int)(n));

	switch( ti )
	{
		case SJ_TI_URL:                 RETURN_STRING   (m_url);
		case SJ_TI_TRACKNAME:           RETURN_STRING   (m_trackName);
		case SJ_TI_TRACKNR:             RETURN_LONG     (m_trackNr);
		case SJ_TI_TRACKCOUNT:          RETURN_LONG     (m_trackCount);
		case SJ_TI_DISKNR:              RETURN_LONG     (m_diskNr);
		case SJ_TI_DISKCOUNT:           RETURN_LONG     (m_diskCount);
		case SJ_TI_LEADARTISTNAME:      RETURN_STRING   (m_leadArtistName);
		case SJ_TI_ORGARTISTNAME:       RETURN_STRING   (m_orgArtistName);
		case SJ_TI_COMPOSERNAME:        RETURN_STRING   (m_composerName);
		case SJ_TI_ALBUMNAME:           RETURN_STRING   (m_albumName);
		case SJ_TI_GENRENAME:           RETURN_STRING   (m_genreName);
		case SJ_TI_GROUPNAME:           RETURN_STRING   (m_groupName);
		case SJ_TI_COMMENT:             RETURN_STRING   (m_comment);
		case SJ_TI_BEATSPERMINUTE:      RETURN_LONG     (m_beatsPerMinute);
		case SJ_TI_RATING:              RETURN_LONG     (m_rating);
		case SJ_TI_YEAR:                RETURN_LONG     (m_year);
		case SJ_TI_PLAYTIMEMS:          RETURN_LONG     (m_playtimeMs);
		case SJ_TI_X_TIMESPLAYED:       RETURN_LONG     (m_timesPlayed);
		case SJ_TI_X_CHANNELS:          RETURN_LONG     (m_channels);
	}

	wxASSERT( 0 );
	return wxT("***");
}


void SjTrackInfo::SetValue(long ti, const wxString& value)
{
	long longValue;
#define SET_STRING(n) \
         (n) = value; return;
#define SET_LONG(n) \
         if( value.ToLong(&longValue) ) { (n) = longValue; } return;

	switch( ti )
	{
		case SJ_TI_URL:                 SET_STRING  (m_url);
		case SJ_TI_TRACKNAME:           SET_STRING  (m_trackName);
		case SJ_TI_TRACKNR:             SET_LONG    (m_trackNr);
		case SJ_TI_TRACKCOUNT:          SET_LONG    (m_trackCount);
		case SJ_TI_DISKNR:              SET_LONG    (m_diskNr);
		case SJ_TI_DISKCOUNT:           SET_LONG    (m_diskCount);
		case SJ_TI_LEADARTISTNAME:      SET_STRING  (m_leadArtistName);
		case SJ_TI_ORGARTISTNAME:       SET_STRING  (m_orgArtistName);
		case SJ_TI_COMPOSERNAME:        SET_STRING  (m_composerName);
		case SJ_TI_ALBUMNAME:           SET_STRING  (m_albumName);
		case SJ_TI_GENRENAME:           SET_STRING  (m_genreName);
		case SJ_TI_GROUPNAME:           SET_STRING  (m_groupName);
		case SJ_TI_COMMENT:             SET_STRING  (m_comment);
		case SJ_TI_BEATSPERMINUTE:      SET_LONG    (m_beatsPerMinute);
		case SJ_TI_RATING:              SET_LONG    (m_rating);
		case SJ_TI_YEAR:                SET_LONG    (m_year);
		case SJ_TI_PLAYTIMEMS:          SET_LONG    (m_playtimeMs);
	}

	wxASSERT( 0 );
}


wxString SjTrackInfo::GetFieldDescr(long ti)
{
	switch( ti )
	{
		case SJ_TI_URL:             return _("File name");
		case SJ_TI_TRACKNAME:       return _("Title");
		case SJ_TI_TRACKNR:         return _("Track number");
		case SJ_TI_TRACKCOUNT:      return _("Track count");
		case SJ_TI_DISKNR:          return _("Disk number");
		case SJ_TI_DISKCOUNT:       return _("Disk count");
		case SJ_TI_LEADARTISTNAME:  return _("Artist");
		case SJ_TI_ORGARTISTNAME:   return _("Original artist");
		case SJ_TI_COMPOSERNAME:    return _("Composer");
		case SJ_TI_ALBUMNAME:       return _("Album");
		case SJ_TI_GENRENAME:       return _("Genre");
		case SJ_TI_GROUPNAME:       return _("Group");
		case SJ_TI_COMMENT:         return _("Comment");
		case SJ_TI_BEATSPERMINUTE:  return _("BPM");
		case SJ_TI_RATING:          return _("Rating");
		case SJ_TI_YEAR:            return _("Year");
		case SJ_TI_PLAYTIMEMS:      return _("Duration");

		case SJ_TI_X_TIMEADDED:     return _("Date added");
		case SJ_TI_X_TIMEMODIFIED:  return _("Date modified");
		case SJ_TI_X_LASTPLAYED:    return _("Last played");
		case SJ_TI_X_TIMESPLAYED:   return _("Play count");
		case SJ_TI_X_DATABYTES:     return _("Size");
		case SJ_TI_X_BITRATE:       return _("Bitrate");
		case SJ_TI_X_SAMPLERATE:    return _("Samplerate");
		case SJ_TI_X_CHANNELS:      return _("Channels");
		case SJ_TI_X_AUTOVOL:       return _("Gain");
		case SJ_TI_Y_FILETYPE:      return _("File type");
		case SJ_TI_Y_QUEUEPOS:      return _("Queue position");

		default:                    return wxT("***");
	}
}


wxString SjTrackInfo::GetFieldDbName(long ti)
{
	switch( ti )
	{
		case SJ_TI_URL:             return wxT("url");
		case SJ_TI_TRACKNAME:       return wxT("trackName");
		case SJ_TI_TRACKNR:         return wxT("trackNr");
		case SJ_TI_TRACKCOUNT:      return wxT("trackCount");
		case SJ_TI_DISKNR:          return wxT("diskNr");
		case SJ_TI_DISKCOUNT:       return wxT("diskCount");
		case SJ_TI_LEADARTISTNAME:  return wxT("leadArtistName");
		case SJ_TI_ORGARTISTNAME:   return wxT("orgArtistName");
		case SJ_TI_COMPOSERNAME:    return wxT("composerName");
		case SJ_TI_ALBUMNAME:       return wxT("albumName");
		case SJ_TI_GENRENAME:       return wxT("genreName");
		case SJ_TI_GROUPNAME:       return wxT("groupName");
		case SJ_TI_COMMENT:         return wxT("comment");
		case SJ_TI_BEATSPERMINUTE:  return wxT("beatsperminute");
		case SJ_TI_RATING:          return wxT("rating");
		case SJ_TI_YEAR:            return wxT("year");
		case SJ_TI_PLAYTIMEMS:      return wxT("playtimeMs");

		case SJ_TI_X_TIMEADDED:     return wxT("timeadded");
		case SJ_TI_X_TIMEMODIFIED:  return wxT("timemodified");
		case SJ_TI_X_LASTPLAYED:    return wxT("lastplayed");
		case SJ_TI_X_TIMESPLAYED:   return wxT("timesplayed");
		case SJ_TI_X_DATABYTES:     return wxT("databytes");
		case SJ_TI_X_BITRATE:       return wxT("bitrate");
		case SJ_TI_X_SAMPLERATE:    return wxT("samplerate");
		case SJ_TI_X_CHANNELS:      return wxT("channels");
		case SJ_TI_X_AUTOVOL:       return wxT("autovol");

		default:                    wxASSERT(0); return wxT("id");
	}
}


#ifdef __WXDEBUG__
bool SjTrackInfo::IsEqualTo(const SjTrackInfo& o) const
{
	if( // compare longs
	    m_id            != o.m_id
	    || m_updatecrc     != o.m_updatecrc
	    || m_timeAdded     != o.m_timeAdded
	    || m_timeModified  != o.m_timeModified
	    || m_timesPlayed   != o.m_timesPlayed
	    || m_lastPlayed    != o.m_lastPlayed
	    || m_dataBytes     != o.m_dataBytes
	    || m_bitrate       != o.m_bitrate
	    || m_samplerate    != o.m_samplerate
	    || m_channels      != o.m_channels
	    || m_playtimeMs    != o.m_playtimeMs
	    || m_autoVol       != o.m_autoVol
	    || m_trackNr       != o.m_trackNr
	    || m_trackCount    != o.m_trackCount
	    || m_diskNr        != o.m_diskNr
	    || m_diskCount     != o.m_diskCount
	    || m_beatsPerMinute!= o.m_beatsPerMinute
	    || m_rating        != o.m_rating
	    || m_year          != o.m_year
	    || m_validFields   != o.m_validFields
	    // compare strings
	    || m_url           != o.m_url
	    || m_trackName     != o.m_trackName
	    || m_leadArtistName!= o.m_leadArtistName
	    || m_orgArtistName != o.m_orgArtistName
	    || m_composerName  != o.m_composerName
	    || m_albumName     != o.m_albumName
	    || m_genreName     != o.m_genreName
	    || m_groupName     != o.m_groupName
	    || m_comment       != o.m_comment
	    || m_arts          != o.m_arts )
	{
		// the two track information objects are not equal
		return FALSE;
	}

	// both track information objects are equal
	return TRUE;
}
#endif


#ifdef __WXDEBUG__
wxString SjTrackInfo::Diff(const SjTrackInfo& o) const
{
	wxString ret;

#define DIFF_STRING(a) \
     if( a != o.a ) \
     { \
         wxString t(a), o_t(o.a); \
         t.Replace(wxT(" "), wxT("")); o_t.Replace(wxT(" "), wxT("")); \
         if( t != o_t ) \
         { \
             ret += wxString::Format(wxT("\n%s: %s != %s"), wxT(#a), a.c_str(), o.a.c_str()); \
         } \
         else \
         { \
             wxString t(a), o_t(o.a); \
             t.Replace(wxT(" "), wxT("_")); o_t.Replace(wxT(" "), wxT("_")); \
             ret += wxString::Format(wxT("\n%s: %s != %s"), wxT(#a), t.c_str(), o_t.c_str()); \
         } \
     }

#define DIFF_LONG(a) \
     if( a != o.a ) \
         ret += wxString::Format(wxT("\n%s: %i != %i"), wxT(#a), (int)a, (int)o.a);

	DIFF_LONG   (m_id);
	DIFF_LONG   (m_updatecrc);
	DIFF_LONG   (m_timeAdded);
	DIFF_LONG   (m_timeModified);
	DIFF_LONG   (m_timesPlayed);
	DIFF_LONG   (m_lastPlayed);
	DIFF_LONG   (m_dataBytes);
	DIFF_LONG   (m_bitrate);
	DIFF_LONG   (m_samplerate);
	DIFF_LONG   (m_channels);
	DIFF_LONG   (m_playtimeMs);
	DIFF_LONG   (m_autoVol);
	DIFF_LONG   (m_trackNr);
	DIFF_LONG   (m_trackCount);
	DIFF_LONG   (m_diskNr);
	DIFF_LONG   (m_diskCount);
	DIFF_LONG   (m_beatsPerMinute);
	DIFF_LONG   (m_rating);
	DIFF_LONG   (m_year);
	DIFF_LONG   (m_validFields);

	DIFF_STRING (m_url);
	DIFF_STRING (m_trackName);
	DIFF_STRING (m_leadArtistName);
	DIFF_STRING (m_orgArtistName);
	DIFF_STRING (m_composerName);
	DIFF_STRING (m_albumName);
	DIFF_STRING (m_genreName);
	DIFF_STRING (m_groupName);
	DIFF_STRING (m_comment);
	DIFF_STRING (m_arts);

	if( ret.IsEmpty() )
	{
		ret = wxT("The two track information objects for \"")  + m_url + wxT("\" equal.\n");
	}
	else
	{
		ret.Prepend(wxT("I found the following differences for \"") + m_url +  wxT("\":\n"));
	}

	return ret;
}
#endif


/*******************************************************************************
 *  SjDataObject
 ******************************************************************************/


SjDataObject::SjDataObject(wxDragResult result)
{
	m_fileData = NULL;
	m_bitmapData = NULL;
	m_dragResult = result;
}


void SjDataObject::AddFileData()
{
	m_fileData = new wxFileDataObject();
	Add(m_fileData);
}


void SjDataObject::AddFile(const wxString& file)
{
	AddFileData();
	m_fileData->AddFile(file);
}


void SjDataObject::AddFiles(const wxArrayString& files)
{
	AddFileData();
	size_t i, iCount=files.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		m_fileData->AddFile(files[i]);
	}
}


void SjDataObject::AddBitmapData()
{
	m_bitmapData = new wxBitmapDataObject();
	Add(m_bitmapData);
}
