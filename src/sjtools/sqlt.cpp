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
 * File:    sqlt.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke SQLite wrapper for sqlite 3.*
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/sqlt.h>
#include <sjtools/levensthein.h>
#include <wx/datetime.h>
#ifdef __WXMSW__
#include <windows.h>
#include <wx/msw/winundef.h> // undef symbols conflicting with wxWidgets
#endif


/*******************************************************************************
 * user-defined functions for sqlite
 ******************************************************************************/


extern "C"
{
	#ifdef __WXDEBUG__
	long g_sortableCalls = 0;
	#endif

	static void sqlite_timestamp(sqlite3_context* context, int argc, sqlite3_value** argv)
	{
		// convert a given date in the format accepted by SjTools::ParseDate
		// to a timestamp
		const char* arg0 = (const char*)sqlite3_value_text(argv[0]);
		if( arg0 )
		{
			wxDateTime dateTime;

			SQLITE3_TO_WXSTRING(arg0)
			if( SjTools::ParseDate_( arg0WxStr, FALSE, &dateTime) )
			{
				sqlite3_result_int(context, dateTime.GetAsDOS());
			}
			else
			{
				sqlite3_result_error(context, "Invalid date.", -1);
			}
		}
	}

	static void sqlite_filetype(sqlite3_context* context, int argc, sqlite3_value** argv)
	{
		// extract the extension (without the point) from a filename or URL
		const char *arg0 = (const char*)sqlite3_value_text(argv[0]);
		if( arg0 )
		{
			SQLITE3_TO_WXSTRING(arg0);
			wxString fileType = SjTools::GetExt( arg0WxStr );

			WXSTRING_TO_SQLITE3(fileType)
			sqlite3_result_text(context, fileTypeSqlite3Str, -1, SQLITE_TRANSIENT);
		}
	}

	static void sqlite_levensthein(sqlite3_context* context, int argc, sqlite3_value** argv)
	{
		// levensthein(str, pattern, max_dist) returns the distance
		// levensthein(str, pattern) chooses a distance defined by the pattern length
		// and returns TRUE (1) or FALSE (0)
		if( argc >= 2 )
		{
			const unsigned char *zA = sqlite3_value_text(argv[0]);
			const unsigned char *zB = sqlite3_value_text(argv[1]);
			if( zA && zB )
			{
				sqlite3_result_int(context, levensthein(zA, zB, argc==2? 0 : sqlite3_value_int(argv[2])));
				return;
			}
		}

		sqlite3_result_error(context, "wrong arguments given to function levensthein()", -1);
	}

	static void sqlite_queuepos(sqlite3_context* context, int argc, sqlite3_value** argv)
	{
		// queuepos(url) returns the queue position of the given url or NULL if none
		// queuepos() returns the current position or NULL for none; in this special case,
		// we return NULL instead of 0 to let fail all operations with an empty queue (normally,
		// we use 0 or an empty string as "NULL" to save space in data structures (wxArrayLong is an
		// example))
		//
		// the returned positions start at index #1 (while internally, we start at #0, as usual)
		if( argc >= 1 )
		{
			const char *arg0 = (const char*)sqlite3_value_text(argv[0]);
			long queuePos;
			if( arg0 && g_mainFrame )
			{
				SQLITE3_TO_WXSTRING(arg0)
				if( (queuePos=g_mainFrame->GetClosestQueuePosByUrl( arg0WxStr )) != wxNOT_FOUND )
				{
					sqlite3_result_int(context, queuePos+1);
					return;
				}
			}
		}
		else
		{
			long queuePos = g_mainFrame->GetQueuePos();
			if( queuePos != wxNOT_FOUND )
			{
				sqlite3_result_int(context, queuePos+1);
				return;
			}
		}

		sqlite3_result_null(context);
	}

	static void sqlite_sortable(sqlite3_context* context, int argc, sqlite3_value** argv)
	{
		#ifdef __WXDEBUG__
		g_sortableCalls++;
		#endif
		if( argc > 0 )
		{
			const char *arg0 = (const char*)sqlite3_value_text(argv[0]);
			if( arg0 )
			{
				// find out the flags to use
				long flags = SJ_NUM_SORTABLE;
				if( argc > 1 )
				{
					flags = sqlite3_value_int(argv[1]);
				}

				// convert the sqlite string to wxString
				SQLITE3_TO_WXSTRING(arg0)

				// normalize the string regarding the given flags
				wxString sortable = ::SjNormaliseString( arg0WxStr, flags);

				// convert the wxString back to a sqlite string and set the result
				WXSTRING_TO_SQLITE3(sortable)
				sqlite3_result_text(context, sortableSqlite3Str, -1, SQLITE_TRANSIENT);
			}
		}
	}

	static void sqlite_nulltoend(sqlite3_context* context, int argc, sqlite3_value** argv)
	{
		long v = sqlite3_value_int(argv[0]);
		sqlite3_result_int(context, v<=0? 0x7FFFFFFFL : v);
	}

};


/*******************************************************************************
 * wxSqltDb
 ******************************************************************************/


wxSqltDb* wxSqltDb::s_defaultDb = NULL;


wxSqltDb::wxSqltDb(const wxString& file)
{
	m_transactionCount          = 0;
	m_transactionVacuumPending  = FALSE;
	m_file                      = file;
	m_dbExistsBeforeOpening     = ::wxFileExists(file);
	m_sqlite                    = NULL;
	#ifdef __WXDEBUG__
	m_instanceCount             = 0;
	#endif

	// open database - this expects the filename to be UTF-8 ...
	wxMBConv* mbConvFilename = &wxConvUTF8;
	#ifdef __WXMSW__
	{
		// ... however, for windows < NT, sqlite
		// forwards the given pointer to just to CreateFileA() without any conversions (is this a bug? see sqlite-3.2.8\src\os_win.h)
		OSVERSIONINFO sInfo;
		sInfo.dwOSVersionInfoSize = sizeof(sInfo);
		GetVersionEx(&sInfo);
		if( sInfo.dwPlatformId != VER_PLATFORM_WIN32_NT ) // do the same test as sqlite does in os_win.h::isNT()
		{
			mbConvFilename = &wxConvISO8859_1;
		}
	}
	#endif

	#if wxUSE_UNICODE
	const wxCharBuffer fileCharBuf = file.mb_str(*mbConvFilename);
	#else
	const wxCharBuffer fileCharBuf = mbConvFilename->cWC2MB(file.wc_str(wxConvLocal));;
	#endif
	const char* fileSqlite3Str = fileCharBuf.data();

	if( sqlite3_open(fileSqlite3Str, &m_sqlite) != SQLITE_OK )
	{
		if( m_sqlite )
		{
			const char* err = sqlite3_errmsg(m_sqlite);
			SQLITE3_TO_WXSTRING(err)
			wxLogError(errWxStr);

			sqlite3_close(m_sqlite);
			m_sqlite = NULL;
		}
		wxLogError(_("Cannot open \"%s\"."), file.c_str());
		return;
	}

	// some initialisations ... database strings are _always_ encoded using UTF-8.
	{
		wxSqlt sql(this);

		// ... create the configuration table
		if( !m_dbExistsBeforeOpening )
		{
			sql.Query(wxT("CREATE TABLE config (id INTEGER PRIMARY KEY, keyname TEXT, value TEXT);"));
			sql.Query(wxT("CREATE INDEX configindex01 ON config (keyname);"));
		}
	}

	// set user defined functions
	sqlite3_create_function(m_sqlite, "timestamp",    1/*number of arguments*/,     SQLITE_ANY, this, sqlite_timestamp,   NULL, NULL);
	sqlite3_create_function(m_sqlite, "filetype",     1/*number of arguments*/,     SQLITE_ANY, this, sqlite_filetype,    NULL, NULL);
	sqlite3_create_function(m_sqlite, "levensthein", -1/*any number of arguments*/, SQLITE_ANY, this, sqlite_levensthein, NULL, NULL);
	sqlite3_create_function(m_sqlite, "queuepos",    -1/*any number of arguments*/, SQLITE_ANY, this, sqlite_queuepos,    NULL, NULL);
	sqlite3_create_function(m_sqlite, "sortable",    -1/*any number of arguments*/, SQLITE_ANY, this, sqlite_sortable,    NULL, NULL);
	sqlite3_create_function(m_sqlite, "nulltoend",    1/*any number of arguments*/, SQLITE_ANY, this, sqlite_nulltoend,   NULL, NULL);
}


wxSqltDb::~wxSqltDb()
{
	wxASSERT(m_transactionCount == 0);

	if( s_defaultDb == this )
	{
		s_defaultDb = NULL;
	}

	if( m_sqlite )
	{
		#ifdef __WXDEBUG__
		if( m_instanceCount != 0 )
		{
			wxLogError(wxT("%i instances left open for the database."), m_instanceCount);
		}
		#endif

		if( sqlite3_close(m_sqlite) != SQLITE_OK )
		{
			#ifndef __WXDEBUG__
			if( !SjMainApp::IsInShutdown() )
			#endif
			{
				const char* err = sqlite3_errmsg(m_sqlite);
				SQLITE3_TO_WXSTRING(err)
				wxLogError(errWxStr);

				wxLogError(wxT("Cannot close database \"%s\".")/*n/t*/, m_file.c_str());
			}
		}
	}
}


void wxSqltDb::SetDefault()
{
	if( !IsOk() )
	{
		wxLogError(wxT("Cannot set default database.")/*n/t*/);
		return;
	}

	s_defaultDb = this;
}


wxSqltDb* wxSqltDb::GetDefault()
{
	return s_defaultDb;
}


long wxSqltDb::SetCache(long bytes, bool saveInDb)
{
	wxSqlt sql(this);
	long pages = Bytes2Pages(bytes);
	sql.Query(wxString::Format(wxT("PRAGMA cache_size=%lu;"), pages));
	if( saveInDb )
	{
		sql.Query(wxString::Format(wxT("PRAGMA default_cache_size=%lu;"), pages));
	}
	return Pages2Bytes(pages);
}


long wxSqltDb::GetCache()
{
	wxSqlt sql(this);
	sql.Query(wxT("PRAGMA cache_size;"));
	if( sql.Next() )
	{
		return Pages2Bytes(sql.GetLong(0));
	}
	return 0; // don't know
}


void wxSqltDb::SetSync(long state, bool saveInDb)
{
	if( state >= 0 && state <= 2 )
	{
		wxSqlt sql(this);
		sql.Query(wxString::Format(wxT("PRAGMA synchronous=%i;"), state));
		if( saveInDb )
		{
			sql.Query(wxString::Format(wxT("PRAGMA default_synchronous=%i;"), state));
		}
	}
}


long wxSqltDb::GetSync()
{
	wxSqlt sql(this);
	sql.Query(wxT("PRAGMA synchronous;"));
	if( sql.Next() )
	{
		return sql.GetLong(0);
	}
	return 1; // NORMAL
}



long wxSqltDb::Bytes2Pages(long bytes)
{
	// each page needs about 1 KB on disk and 1.5 KB in memory,
	// this calculation is for the memory setting
	long pages = bytes / 1536;
	if( pages < 64 ) { pages = 64; }
	if( pages > 16000 ) { pages = 16000; }
	return pages;
}



long wxSqltDb::Pages2Bytes(long pages)
{
	return pages * 1536;
}



/*wxString wxSqltDb::GetLibVersion()
{
    return sqlite3_version;
}*/



/*******************************************************************************
 *  wxSqlt - low level query interface
 ******************************************************************************/


wxString wxSqlt::QParam(const wxString& str)
{
	if( str.Find('\'') != -1 )
	{
		wxString ret(str);
		ret.Replace(wxT("'"), wxT("''"));
		return ret;
	}
	else
	{
		return str;
	}
}


bool wxSqlt::Query(const wxString& query)
{
	int         sqlState;
	const char* sqlTail = NULL;  // OUT: Part of zSQL not compiled

	// close any open query
	CloseQuery();

	// compile the complete SQL string
	WXSTRING_TO_SQLITE3(query)
	if( sqlite3_prepare(m_db->m_sqlite, querySqlite3Str, -1, &m_stmt, &sqlTail) != SQLITE_OK )
	{
		const char* err = sqlite3_errmsg(m_db->m_sqlite);
		SQLITE3_TO_WXSTRING(err)
		wxLogError(errWxStr);

		CloseQuery();
		wxLogError(wxT("Cannot compile SQL statement \"%s\".")/*n/t*/, query.c_str());
		return FALSE;
	}

	if( sqlTail && sqlTail[0] )
	{
		CloseQuery();
		wxLogError(wxT("Only a single SQL Statement can be queried, multiple statements as \"%s\" are not allowed.")/*n/t*/, query.c_str());
		return FALSE;
	}

	// fetch query
	sqlState = FetchQuery_();
	if( sqlState == SQLITE_ERROR )
	{
		// error - message already displayed in FetchQuery_()
		CloseQuery();
		return FALSE;
	}
	else if( sqlState == SQLITE_ROW )
	{
		// success - you can get the rows using Next()
		m_fetchState = 'f'; // [f]irst
		return TRUE;
	}
	else
	{
		// success - however, there is no result, the virtual machine
		// stays in memory for reuse
		CloseQuery();
		return TRUE;
	}
}


int wxSqlt::FetchQuery_()
{
	//int nc = 0;

	wxASSERT(m_stmt);

	int sqlState = sqlite3_step(m_stmt);
	m_fieldCount = sqlite3_data_count(m_stmt);
	if( sqlState == SQLITE_ROW )
	{
		// next row fetched
		return SQLITE_ROW;
	}
	else if( sqlState == SQLITE_DONE )
	{
		// there are no more rows
		return SQLITE_DONE;
	}
	else
	{
		// error - we do not log this as this error may occur under mysterious circumstances - see
		// http://www.silverjuke.net/forum/viewtopic.php?t=900
		#ifdef __WXDEBUG__
		const char* err = sqlite3_errmsg(m_db->m_sqlite);
		SQLITE3_TO_WXSTRING(err)
		wxLogError(errWxStr);

		wxLogError(wxT("Cannot get SQL row.")/*n/t*/);
		#endif
		return SQLITE_ERROR;
	}
}


void wxSqlt::CloseQuery()
{
	if( m_stmt )
	{
		if( sqlite3_finalize(m_stmt) != SQLITE_OK )
		{
			const char* err = sqlite3_errmsg(m_db->m_sqlite);
			SQLITE3_TO_WXSTRING(err)
			wxLogError(errWxStr);

			wxLogError(wxT("Cannot close SQL query.")/*n/t*/);
		}

		m_stmt = NULL;
		m_fieldCount = 0; // needed eg. for save scripting
		m_fetchState = 'd'; // [d]one
	}
}


bool wxSqlt::Next()
{
	if( m_stmt == NULL || m_fetchState == 'd' /* [d]one */ )
	{
		// the virtual machine may already be deleted in Query()
		return FALSE;
	}

	if( m_fetchState == 'f' /* [f]irst */ )
	{
		// FetchQuery() already called in Query()
		m_fetchState = 0;
	}
	else
	{
		// fetch next row
		if( FetchQuery_()!=SQLITE_ROW )
		{
			CloseQuery();
			return FALSE;
		}
	}

	// success
	return TRUE;
}


int wxSqlt::GetFieldIndex(const wxString& fieldName) const
{
	int field;

	wxASSERT(m_stmt);

	for( field = 0; field < m_fieldCount; field++ )
	{
		const char* colName = sqlite3_column_name(m_stmt, field);
		SQLITE3_TO_WXSTRING(colName)
		if( fieldName.CmpNoCase(colNameWxStr) == 0 )
		{
			return field;
		}
	}

	wxLogError(wxT("Invalid SQL field \"%s\".")/*n/t*/, fieldName.c_str());
	return 0;
}


wxString wxSqlt::GetFieldName(int fieldIndex) const
{
	wxASSERT(m_stmt);

	if(fieldIndex < 0 || fieldIndex >= m_fieldCount)
	{
		wxLogError(wxT("Invalid SQL field %i, range is 0..%i")/*n/t*/, fieldIndex, m_fieldCount);
		return wxT("");
	}

	const char* colName = sqlite3_column_name(m_stmt, fieldIndex);
	SQLITE3_TO_WXSTRING(colName)
	colNameWxStr.MakeLower();
	return colNameWxStr;
}


long wxSqlt::GetInsertId()
{
	return (long)sqlite3_last_insert_rowid(m_db->m_sqlite); // internally, the id has 64 bits
}


long wxSqlt::GetChangedRows()
{
	return (long)sqlite3_changes(m_db->m_sqlite); // internally, the id has 64 bits
}


/*******************************************************************************
 * wxSqlt - Query table information and other
 ******************************************************************************/


bool wxSqlt::TableExists(const wxString& tablename)
{
	Query(wxString::Format(wxT("PRAGMA table_info(%s);"), tablename.c_str()));
	return Next();
}


bool wxSqlt::ColumnExists(const wxString& tablename, const wxString& rowname__)
{
	wxString rowname = rowname__.Lower();
	Query(wxString::Format(wxT("PRAGMA table_info(%s);"), tablename.c_str()));
	while( Next() )
	{
		if( GetString(1).Lower() == rowname )
		{
			return TRUE;
		}
	}
	return FALSE;
}


void wxSqlt::AddColumn(const wxString& tablename, const wxString& column_def)
{
	// you should ensure, the column does not exist when calling this function!
	Query(wxString::Format(wxT("ALTER TABLE %s ADD COLUMN %s;"), tablename.c_str(), column_def.c_str()));
}


/*******************************************************************************
 * wxSqlt - Configuration handling
 ******************************************************************************/


void wxSqlt::ConfigWrite(const wxString& keyname, const wxString& value)
{
	Query(wxT("SELECT value FROM config WHERE keyname='") +  QParam(keyname) + wxT("';"));
	if( !Next() )
	{
		Query(wxT("INSERT INTO config (keyname, value) VALUES ('") + QParam(keyname) + wxT("', '") + QParam(value) + wxT("')"));
	}
	else
	{
		Query(wxT("UPDATE config SET value='") + QParam(value) + wxT("' WHERE keyname='") + QParam(keyname) + wxT("';"));
	}
}


wxString wxSqlt::ConfigRead(const wxString& keyname, const wxString& def)
{
	Query(wxT("SELECT value FROM config WHERE keyname='") + QParam(keyname) + wxT("';"));
	return Next()? GetString(0) : def;
}


long wxSqlt::ConfigRead(const wxString& keyname, long def)
{
	Query(wxT("SELECT value FROM config WHERE keyname='") + QParam(keyname) + wxT("';"));
	return Next()? GetLong(0) : def;
}


void wxSqlt::ConfigDeleteEntry(const wxString& keyname)
{
	Query(wxT("DELETE FROM config WHERE keyname='") + QParam(keyname) + wxT("';"));
}


/*******************************************************************************
 * wxSqltTransaction
 ******************************************************************************/


wxSqltTransaction::wxSqltTransaction(wxSqltDb* db)
{
	m_db = db? db : wxSqltDb::GetDefault();
	if( m_db == NULL )
	{
		wxLogFatalError(wxT("No database given to wxSqltTransaction()."));
	}

	wxASSERT( m_db->m_transactionCount >= 0 );
	m_db->m_transactionCount++;
	m_commited = FALSE;
	if( m_db->m_transactionCount == 1 )
	{
		m_db->OnTransactionBegin();
		{
			wxBusyCursor busy;
			wxSqlt sql(m_db);
			sql.Query(wxT("BEGIN;"));
		}
	}
}


wxSqltTransaction::~wxSqltTransaction()
{
	wxASSERT( m_db );
	wxASSERT( m_db->m_transactionCount >= 1 );

	if( m_db->m_transactionCount == 1 )
	{
		if( !m_commited )
		{
			m_db->OnTransactionRollback();
			{
				wxBusyCursor busy;
				wxSqlt sql(m_db);
				sql.Query(wxT("ROLLBACK;"));
			}
		}
	}

	m_db->m_transactionCount--;
}


bool wxSqltTransaction::Commit()
{
	wxASSERT( m_db );
	wxASSERT( m_db->m_transactionCount >= 1 );
	wxASSERT( m_commited == FALSE );

	if( m_db->m_transactionCount == 1 )
	{
		bool ret;
		m_db->OnTransactionCommit();
		{
			wxBusyCursor busy;
			wxSqlt sql(m_db);
			ret = sql.Query(wxT("COMMIT;"));

			if( m_db->m_transactionVacuumPending )
			{
				if( wxThread::IsMain() )
				{
					SjBusyInfo::Set(_("Cleanup index..."), TRUE);
				}

				m_db->m_transactionVacuumPending = FALSE;
				sql.Query(wxT("VACUUM;"));
			}

			m_commited = TRUE;
		}

		return ret;
	}
	else
	{
		m_commited = TRUE;
		return TRUE; // committed later
	}
}



