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
 * File:    obj_Database.cpp
 * Authors: Björn Petersen
 * Purpose: The Database class for SEE
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include "sj_see.h"
#include "sj_see_helpers.h"

// data used by our object
struct database_object
{
	SEE_native  native; // MUST be first!
	wxSqltDb*   db;
	wxSqlt*     sql;
};
static void database_finalize(SEE_interpreter* interpr, void* ptr, void* closure)
{
	database_object* dbo = (database_object*)ptr;
	delete dbo->sql;
	delete dbo->db;
}
static database_object* alloc_database_object(SEE_interpreter* interpr)
{
	database_object* dbo = (database_object*)SEE_malloc_finalize(interpr, sizeof(database_object), database_finalize, NULL);
	memset(dbo, 0, sizeof(database_object));
	return dbo;
}
static database_object* toDatabase(SEE_interpreter* interpr, SEE_object* o);



/*******************************************************************************
 *  Database methods
 ******************************************************************************/



IMPLEMENT_FUNCTION(database, construct)
{
	RETURN_OBJECT( HOST_DATA->Database_new(ARG_STRING(0)) );
}

IMPLEMENT_FUNCTION(database, openQuery)
{
	database_object* dbo = toDatabase(interpr_, this_);
	RETURN_BOOL( dbo->sql->Query(ARG_STRING(0)) );
}

IMPLEMENT_FUNCTION(database, nextRecord)
{
	database_object* dbo = toDatabase(interpr_, this_);
	RETURN_BOOL( dbo->sql->Next() );
}

IMPLEMENT_FUNCTION(database, getFieldCount)
{
	database_object* dbo = toDatabase(interpr_, this_);
	RETURN_LONG( dbo->sql->GetFieldCount() );
}

IMPLEMENT_FUNCTION(database, getField)
{
	database_object* dbo = toDatabase(interpr_, this_);
	long n = ARG_LONG(0);

	if( n == -2 )
	{
		RETURN_LONG( dbo->sql->GetChangedRows() );
	}
	else if( n == -1 )
	{
		RETURN_LONG( dbo->sql->GetInsertId() );
	}
	else if( n < 0 || n >= dbo->sql->GetFieldCount() )
	{
		SEE_error_throw(interpr_, interpr_->RangeError, "bad index");
	}
	else
	{
		if( dbo->sql->IsSet( n ) )
			RETURN_STRING( dbo->sql->GetString( n ) );
		else
			RETURN_UNDEFINED;
	}
}

IMPLEMENT_FUNCTION(database, closeQuery)
{
	database_object* dbo = toDatabase(interpr_, this_);
	dbo->sql->CloseQuery();
	RETURN_UNDEFINED;
}



IMPLEMENT_FUNCTION(database, getFile)
{
	database_object* dbo = toDatabase(interpr_, this_);

	wxSqltDb* db = dbo->db? dbo->db : wxSqltDb::GetDefault();
	if( db )
		RETURN_STRING( db->GetFile() );
	else
		RETURN_UNDEFINED;
}


/*******************************************************************************
 *  Database statics
 ******************************************************************************/



IMPLEMENT_FUNCTION(database, update)
{
	if( g_mainFrame )
		RETURN_BOOL( g_mainFrame->UpdateIndex(g_mainFrame, ARG_BOOL(0)) );
	else
		RETURN_FALSE;
}


/*******************************************************************************
 *  Let SEE know about this class (this part is a little more complicated)
 ******************************************************************************/



/* object class for Database.prototype and database instances */
static SEE_objectclass database_inst_class = {
	"Database",                 /* Class */
	SEE_native_get,             /* Get */
	SEE_native_put,             /* Put */
	SEE_native_canput,          /* CanPut */
	SEE_native_hasproperty,     /* HasProperty */
	SEE_native_delete,          /* Delete */
	SEE_native_defaultvalue,    /* DefaultValue */
	SEE_native_enumerator,      /* DefaultValue */
	NULL,                       /* Construct */
	NULL,                       /* Call */
	NULL                        /* HasInstance */
};



/* object class for Database constructor */
static SEE_objectclass database_constructor_class = {
	"DatabaseConstructor",      /* Class */
	SEE_native_get,             /* Get */
	SEE_native_put,             /* Put */
	SEE_native_canput,          /* CanPut */
	SEE_native_hasproperty,     /* HasProperty */
	SEE_native_delete,          /* Delete */
	SEE_native_defaultvalue,    /* DefaultValue */
	SEE_native_enumerator,      /* DefaultValue */
	database_construct,         /* Construct */
	NULL                        /* Call */
};



void SjSee::Database_init()
{
	SEE_value temp;

	// Create the "Database.prototype" object, this is used as a template for the instances
	m_Database_prototype = (SEE_object *)alloc_database_object(m_interpr);

	SEE_native_init((SEE_native *)m_Database_prototype, m_interpr,
	                &database_inst_class, m_interpr->Object_prototype);

	PUT_FUNC(m_Database_prototype, database, openQuery, 0);
	PUT_FUNC(m_Database_prototype, database, nextRecord, 0);
	PUT_FUNC(m_Database_prototype, database, getFieldCount, 0);
	PUT_FUNC(m_Database_prototype, database, getField, 0);
	PUT_FUNC(m_Database_prototype, database, closeQuery, 0);
	PUT_FUNC(m_Database_prototype, database, getFile, 0);

	// create the "Database" object
	SEE_object* Database = (SEE_object *)SEE_malloc(m_interpr, sizeof(SEE_native));

	SEE_native_init((SEE_native *)Database, m_interpr,
	                &database_constructor_class, m_interpr->Object_prototype);

	PUT_OBJECT(Database, str_prototype, m_Database_prototype)
	PUT_FUNC(Database, database, update, 0); // static function

	// now we can add the globals
	PUT_OBJECT(m_interpr->Global, str_Database,  Database);
}



SEE_object* SjSee::Database_new(const wxString& name)
{
	database_object* obj = alloc_database_object(m_interpr);

	SEE_native_init(&obj->native, m_interpr,
	                &database_inst_class, m_Database_prototype);

	if( !name.IsEmpty() )
	{
		obj->db = new wxSqltDb(name);
	}

	obj->sql = new wxSqlt(obj->db);

	return (SEE_object *)obj;
}



static database_object* toDatabase(SEE_interpreter* interpr, SEE_object* o)
{
	if( o->objectclass != &database_inst_class )
		SEE_error_throw(interpr, interpr->TypeError, NULL);

	return (database_object *)o;
}



