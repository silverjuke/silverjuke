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
 * File:    sqlt.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke SQLite wrapper for sqlite 3.*
 *
 ******************************************************************************/


#ifndef __WX_SQLT_H__
#define __WX_SQLT_H__


#include <wx/wx.h>
#include <sqlite3.h>



// String conversions sqlite <-> wxString:
// - use this ONLY for the content string (Query etc.), the filenames are ALWAYS UTF-8
// - sqlite uses UTF-8 internally, however, we have stored ISO 8859-1 strings in older versions :-(
// - sqlite also has an UTF-16 interface, which, however is only mapped to the UTF-8 functions
//   and needs one more level of conversion
//
// Usage of the macros:
//   wxString str;
//   WXSTRING_TO_SQLITE3(str)
//   sqlite3...(strSqlite3Str);
//
// or:
//   const char* str = sqlite3...
//   SQLITE3_TO_WXSTRING(str)
//   return strWxStr;
#if wxUSE_UNICODE

	#define SQLITE3_TO_WXSTRING(a) \
		 wxString a##WxStr = wxString((a), wxConvUTF8);

	#define WXSTRING_TO_SQLITE3(a) \
		 const wxCharBuffer a##__tempCharBuf = (a).mb_str(wxConvUTF8); \
		 const char* a##Sqlite3Str = a##__tempCharBuf.data();

#else

	#define SQLITE3_TO_WXSTRING(a) \
		 wxString a##WxStr = wxString(wxConvUTF8.cMB2WC((a)));

	#define WXSTRING_TO_SQLITE3(a) \
		 const char* a##Sqlite3Str; \
		 wxCharBuffer a##__tempCharBuf((size_t)0L); \
		 a##__tempCharBuf = wxConvUTF8.cWC2MB((a).wc_str(wxConvLocal)); \
		 a##Sqlite3Str = a##__tempCharBuf.data();

#endif



class wxSqlt;



class wxSqltDb
{
public:
						wxSqltDb                (const wxString& file);
	virtual             ~wxSqltDb               ();

	bool                IsOk                    () const {return m_sqlite? TRUE : FALSE; }
	bool                ExistsBeforeOpening     () const {return m_dbExistsBeforeOpening;}
	wxString            GetFile                 () const {return m_file;}
	void                SetDefault              ();
	static wxSqltDb*    GetDefault              ();
	static void         ClearDefault            () {s_defaultDb=NULL;}

	// cache settings
	// sync is one of NORMAL (1, default), OFF (0) or FULL (2)
	long                SetCache                (long bytes, bool saveInDb=TRUE); // the real number of cache bytes are returned
	void                SetSync                 (long state, bool saveInDb=TRUE);
	long                GetCache                ();
	long                GetSync                 ();
	sqlite3*            GetDb                   () { return m_sqlite; }

	// some events that may be used by derived classes.
	// the event are placed here and not in wxSqltTransaction as calling
	// virtual functions in the constructor/destructor is not straight-forward
	virtual void        OnTransactionBegin      () {}
	virtual void        OnTransactionRollback   () {}
	virtual void        OnTransactionCommit     () {}

	// misc.
	static wxString		GetLibVersion			();

private:
	wxString            m_file;
	bool                m_dbExistsBeforeOpening;
	sqlite3*            m_sqlite;
	int                 m_transactionCount;
	bool                m_transactionVacuumPending;
	#ifdef __WXDEBUG__
	int                 m_instanceCount;
	#endif

	long                Bytes2Pages         (long bytes);
	long                Pages2Bytes         (long pages);

	static wxSqltDb*    s_defaultDb;

	friend class        wxSqlt;
	friend class        wxSqltTransaction;

	bool                RecodeToUtf8        (wxSqlt& sql1);
};



class wxSqlt
{
public:
	// constructor / destructor
	wxSqlt              (wxSqltDb* db=NULL)
	{
		m_db = db? db : wxSqltDb::s_defaultDb;
		m_stmt = NULL;
		wxASSERT(m_db);
		#ifdef __WXDEBUG__
			m_db->m_instanceCount++;
		#endif
	}
	~wxSqlt             ()
	{
		CloseQuery();
		#ifdef __WXDEBUG__
			m_db->m_instanceCount--;
		#endif
	}

	wxSqltDb*       GetDb               () const { return m_db; }

	// simply for preparing some strings
	static wxString QParam              (const wxString&);
	static wxString LParam              (long v) { return wxString::Format(wxT("%i"), (int)v); }
	static wxString UParam              (unsigned long v) { return wxString::Format(wxT("%lu"), v); }

	// high level query interface, use as:
	//  sql.Query("SELECT id, name FROM table");
	//  while( sql.Next() )
	//  {
	//      id = sql.GetLong(0);
	//      name = sql.GetString(1);
	//      // or more readable but slower:
	//      id = sql.GetLong("id");
	//      name = sql.GetString("name");
	//  }
	bool            Query               (const wxString& query);
	bool            Next                ();

	// query the result using the field index
	long            GetFieldCount       () const { return m_fieldCount; }
	bool            IsSet               (int fieldIndex) const
	{
		wxASSERT(fieldIndex>=0 && fieldIndex<m_fieldCount);
		wxASSERT(m_stmt);
		return (sqlite3_column_type(m_stmt, fieldIndex)!=SQLITE_NULL);
	}
	wxString        GetString           (int fieldIndex) const
	{
		wxASSERT(fieldIndex>=0 && fieldIndex<m_fieldCount);
		wxASSERT(m_stmt);
		const char* str = (const char*)sqlite3_column_text(m_stmt, fieldIndex);
		SQLITE3_TO_WXSTRING(str)
		return strWxStr;
	}
	const char*     GetAsciiPtr      (int fieldIndex) const
	{
		// use this function ONLY if you are sure, the column does NOT contain anything above bit #6 (128-256 or larger)
		wxASSERT(fieldIndex>=0 && fieldIndex<m_fieldCount);
		return (const char*)sqlite3_column_text(m_stmt, fieldIndex);
	}
	long            GetLong             (int fieldIndex) const { wxASSERT(fieldIndex>=0 && fieldIndex<m_fieldCount); return sqlite3_column_int(m_stmt, fieldIndex); }

	// same as the functions above, but for names fields;
	// note that this is some slower!
	bool            IsSet               (const wxString& fieldName) const { return IsSet(GetFieldIndex(fieldName)); }
	wxString        GetString           (const wxString& fieldName) const { return GetString(GetFieldIndex(fieldName)); }
	long            GetLong             (const wxString& fieldName) const { return GetLong(GetFieldIndex(fieldName)); }
	int             GetFieldIndex       (const wxString& fieldName) const;
	wxString        GetFieldName        (int fieldIndex) const;

	// get the last insert id
	long            GetInsertId         ();
	long            GetChangedRows      ();

	// queries are automatically closed if a new query is started or
	// if the object is destructed. however, if you use different wxSqlt
	// objects you may want to close queries explicit to avoid leaving
	// tables locked.
	void            CloseQuery          ();

	// Query table information or add missing files, fieldsNTypes
	// is a null-separated list of fieldnames/types in this case
	bool            TableExists         (const wxString& tablename);
	bool            ColumnExists        (const wxString& tablename, const wxString& rowname);
	void            AddColumn           (const wxString& tablename, const wxString& column_def);

	// Configuration handling
	void            ConfigWrite         (const wxString& keyname, const wxString& value);
	void            ConfigWrite         (const wxString& keyname, long value) {ConfigWrite(keyname,wxString::Format(wxT("%lu"),value));}
	wxString        ConfigRead          (const wxString& keyname, const wxString& def);
	long            ConfigRead          (const wxString& keyname, long def);
	void            ConfigDeleteEntry   (const wxString& keyname);

private:
	// private stuff, use as less an as "easy" objects as possible
	// for fast wxSqlt creation on the stack
	int             FetchQuery_         ();
	wxSqltDb*       m_db;
	sqlite3_stmt*   m_stmt;
	int             m_fetchState; // [d]one, [f]irst or 0

	int             m_fieldCount;
};



class wxSqltTransaction
{
	// Transaction objects may be nested, however, only the outest
	// object is really used. You may subclass wxSqltDb and
	// catch the OnTransaction*() events
public:
	wxSqltTransaction   (wxSqltDb* db = NULL);
	~wxSqltTransaction  ();
	void            Vacuum              () { m_db->m_transactionVacuumPending = TRUE; }
	bool            Commit              ();

private:
	wxSqltDb*       m_db;
	bool            m_commited;
};


#ifdef __WXDEBUG__
extern "C"
{
	extern long g_sortableCalls;
}
#endif


#endif // __WX_SQLT_H__
