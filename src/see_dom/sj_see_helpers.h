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
 * File:    sj_see_helpers.h
 * Authors: Björn Petersen
 * Purpose: Global strings and tool for our interpreters
 *
 ******************************************************************************/


#ifndef __SJ_SEE_HELPERS_H__
#define __SJ_SEE_HELPERS_H__


#include "../tools/gcalloc.h"
#include "../see/see_see.h"
extern "C" {
#include "../see/see_array.h"
#include "../see/see_function.h"
};


/*******************************************************************************
 * tools
 ******************************************************************************/


wxString    SeeValueToWxString      (SEE_interpreter*, SEE_value*);
wxArrayLong SeeValueToWxArrayLong   (SEE_interpreter*, SEE_value*);
double      SeeValueToDouble        (SEE_interpreter*, SEE_value*);
SEE_object* SeeValueToSeeCallback   (SEE_interpreter*, SEE_value*);

SEE_string* WxStringToSeeString     (SEE_interpreter*, const wxString&);
SEE_object* WxArrayStringToSeeObject(SEE_interpreter*, const wxArrayString&);
SEE_object* WxArrayLongToSeeObject  (SEE_interpreter*, const wxArrayLong&);

void        SeeCallCallback         (SEE_interpreter*, SEE_object* fn, SEE_object* context, SEE_value* arg0=NULL);



/*******************************************************************************
 *  macros
 ******************************************************************************/


// class member functions, use as follows
//
//		IMPLEMENT_FUNCTION(myClass, multBy2) {
//			long l = ARG(0);
//			RETURN_LONG( l * 2 );
//		}

#define IMPLEMENT_FUNCTION(objname, funcname) \
    static void objname##_##funcname(SEE_interpreter* interpr_, SEE_object* self_, \
        SEE_object* this_, int argc_, SEE_value** argv_, SEE_value* res_)

#define HOST_DATA               ((SjSee*)interpr_->host_data)

#define ARG_LONG(i)             (argc_>i? SEE_ToInt32(interpr_, argv_[i]) : 0)
#define ARG_LONG_OR_DEF(i, d)   (argc_>i? SEE_ToInt32(interpr_, argv_[i]) : d)

#define ARG_DOUBLE(i)           SeeValueToDouble(interpr_, argc_>i? argv_[i] : NULL)

#define ARG_IS_BOOL(i)          (argc_>i && SEE_VALUE_GET_TYPE(argv_[i])==SEE_BOOLEAN)
#define ARG_BOOL(i)             (argc_>i? SEE_ToInt32(interpr_, argv_[i]) : 0)!=0

#define ARG_IS_STRING(i)        (argc_>i && SEE_VALUE_GET_TYPE(argv_[i])==SEE_STRING)
#define ARG_STRING(i)           SeeValueToWxString(interpr_, argc_>i? argv_[i] : NULL)

#define ARG_CALLBACK(i)         SeeValueToSeeCallback(interpr_, argc_>i? argv_[i] : NULL)

#define RETURN_STRING(s)        SEE_SET_STRING      (res_, WxStringToSeeString(interpr_, s))
#define RETURN_ARRAY_STRING(a)  SEE_SET_OBJECT      (res_, WxArrayStringToSeeObject(interpr_, a))
#define RETURN_ARRAY_LONG(a)    SEE_SET_OBJECT      (res_, WxArrayLongToSeeObject(interpr_, a))
#define RETURN_LONG(v)          SEE_SET_NUMBER      (res_, (double)v)
#define RETURN_DOUBLE(v)        SEE_SET_NUMBER      (res_, v)
#define RETURN_BOOL(b)          SEE_SET_BOOLEAN     (res_, b)
#define RETURN_TRUE             SEE_SET_BOOLEAN     (res_, true)
#define RETURN_FALSE            SEE_SET_BOOLEAN     (res_, false)
#define RETURN_OBJECT(o)        SEE_SET_OBJECT      (res_, o)
#define RETURN_VALUE(v)         SEE_VALUE_COPY      (res_, v)
#define RETURN_UNDEFINED        SEE_SET_UNDEFINED   (res_)



// class properties, use as follow
//
//		IMPLEMTENT_HASPROPERTY(myClass)
//		{
//			if( ARG_PROPERTY(isImportant) )
//				RETURN_HAS;
//			else
//				RETURN_HASNOT;
//		}
//
//		IMPLEMENT_GET(myClass) {
//			if( ARG_PROPERTY(isImportant) )
//				RETURN_TRUE; // or whatever using the RETURN_* macros from above
//			else
//				RETURN_GET_DEFAULTS;
//		}
//
//		IMPLEMENT_PUT(myClass) {
//			if( ARG_PROPERTY(isImportant) )
//				// do what to do ...
//			else
//				DO_PUT_DEFAULTS;
//		}

#define IMPLEMENT_HASPROPERTY(objname) \
    static int objname##_hasproperty(SEE_interpreter* interpr_, \
                SEE_object *this_, SEE_string *prop_)

#define IMPLEMENT_GET(objname) \
    static void objname##_get(SEE_interpreter *interpr_, \
                SEE_object *this_, SEE_string *prop_, SEE_value *res_)

#define IMPLEMENT_PUT(objname) \
    static void objname##_put(SEE_interpreter *interpr_, \
                SEE_object *this_, SEE_string *prop_, SEE_value *val_, int flags_)

#define VAL_PROPERTY(s)     (SEE_string_cmp(prop_, str_##s)==0)
#define VAL_LONG            (SEE_ToInt32(interpr_, val_))
#define VAL_BOOL            (SEE_ToInt32(interpr_, val_)!=0)
#define VAL_CALLBACK        SeeValueToSeeCallback(interpr_, val_)
#define VAL_STRING          SeeValueToWxString(interpr_, val_)
#define VAL_ARRAY_LONG      SeeValueToWxArrayLong(interpr_, val_)


#define RETURN_HAS          return 1
#define RETURN_HASNOT       return SEE_native_hasproperty(interpr_, this_, prop_)
#define RETURN_GET_DEFAULTS SEE_native_get(interpr_, this_, prop_, res_)
#define DO_PUT_DEFAULTS     SEE_native_put(interpr_, this_, prop_, val_, flags_)



// Class construction helpers

#define PUT_OBJECT(parent, name, obj) \
    SEE_SET_OBJECT(&temp, obj); \
    SEE_OBJECT_PUT(m_interpr, parent, name, &temp, SEE_ATTR_DEFAULT);

#define PUT_FUNC(parent, namePrefix, name, len) \
    SEE_SET_OBJECT(&temp, SEE_cfunction_make(m_interpr, namePrefix##_##name, str_##name, len)); \
    SEE_OBJECT_PUT(m_interpr, parent, str_##name, &temp, SEE_ATTR_DEFAULT);

#define PUT_CONSTANT(parent, name, val) \
    SEE_SET_NUMBER(&temp, val); \
    SEE_OBJECT_PUT(m_interpr, parent, str_##name, &temp, SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY);


/*******************************************************************************
 *  global strings
 ******************************************************************************/



// We add our well-known strings to the global string hash of SEE.
// This has the advantage, that there are no need of multiple instances of
// these strings. moreover, SEE can (and will) do a very fast comparison
// of such strings (just a pointer comparison)

// declare the string object - the strings itself are placed in sj_see_strings.inc
#define STR(a) , *str_##a
#ifndef SJ_IMPLEMENT_HELPERS
extern
#endif
SEE_string *str_prototype
#include "sj_see_strings.inc"
;

// define the strings (on modifications, just copy the block from above)
#ifdef SJ_IMPLEMENT_HELPERS
#undef STR
#define STR(a) str_##a = SEE_intern_global(#a);
static void SEE_system_add_my_strings() {
	STR( prototype )
#include "sj_see_strings.inc"
}
#endif
#undef STR



#endif // __SJ_SEE_HELPERS_H__
