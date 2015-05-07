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
 * File:    sj_see.cpp
 * Authors: Björn Petersen
 * Purpose: Our Interpreter
 *
 ******************************************************************************/



#include "../basesj.h"

#include "sj_see.h"
#define SJ_IMPLEMENT_HELPERS
#include "sj_see_helpers.h"

#if SJ_USE_SCRIPTS


/*******************************************************************************
 *  Callbacks for SEE_system
 ******************************************************************************/



static void SjSee_abort(SEE_interpreter* interpr_, const char* msgPtr)
{
	wxString msgStr(msgPtr, wxConvUTF8);
	wxLogError(wxT("Fatal: %s [%s]"), msgStr.c_str(), HOST_DATA->m_executionScope.c_str());
	SjMainApp::FatalError();
}



static void* SjSee_malloc(SEE_interpreter* interpr, SEE_size_t size)
{
	return ::SjGcAlloc(size,
	                   (interpr?0:SJ_GC_ALLOC_STATIC));
}



static void* SjSee_malloc_string(SEE_interpreter* interpr, SEE_size_t size)
{
	return ::SjGcAlloc(size,
	                   (interpr?0:SJ_GC_ALLOC_STATIC) | SJ_GC_ALLOC_STRING);
}




static void* SjSee_malloc_finalize(SEE_interpreter* interpr, SEE_size_t size,
                                   void (*finalizefn)(SEE_interpreter*, void*, void*), void* closure)
{
	return ::SjGcAlloc(size,
	                   (interpr?0:SJ_GC_ALLOC_STATIC),
	                   (SJ_GC_PROC)finalizefn, interpr, closure);
}



static void SjSee_free(SEE_interpreter* interpr, void* ptr)
{
	::SjGcUnref(ptr);
}



/*******************************************************************************
 *  SjSee constructor / destructor
 ******************************************************************************/


SjSee* SjSee::s_first = NULL;



SjSee::SjSee()
{
	SjGcLocker gclocker;

	// allocate interpreter structure
	// (initialization is delayed to the first call to Execute() - this allows us
	// to create SjSee objects very fast even if they may be not needed.
	m_plugin                    = NULL;
	m_interpr                   = (SEE_interpreter*)::SjGcAlloc(sizeof(SEE_interpreter), SJ_GC_ALLOC_STATIC|SJ_GC_ZERO);
	m_interpr->host_data        = this;
	m_interprInitialized        = false;
	m_executeResult             = (SEE_value*)::SjGcAlloc(sizeof(SEE_value), SJ_GC_ALLOC_STATIC|SJ_GC_ZERO);
	m_persistentAnchor          = (persistent_object*)::SjGcAlloc(sizeof(persistent_object), SJ_GC_ALLOC_STATIC|SJ_GC_ZERO);
	m_persistentAnchor->m_object= (SEE_object*)m_persistentAnchor;

	m_timer                     = NULL;

	wxASSERT( m_executeResult->_type == SEE_UNDEFINED );

	// add to list
	m_next = s_first;
	s_first = this;
}



SjSee::~SjSee()
{
	SjGcLocker gclocker;

	// stop and free the timer
	if( m_timer )
	{
		wxASSERT( wxThread::IsMain() );

		m_timer->m_inTimer = true;
		m_timer->Stop();
		delete m_timer;
		m_timer = NULL;
	}

	// call all "unexpected unpersistent" functions
	{
		persistent_object *cur = m_persistentAnchor->m_next;
		while( cur )
		{
			if( cur->m_unexpectedUnpersistent )
				cur->m_unexpectedUnpersistent(m_interpr, cur->m_object, NULL);
			cur = cur->m_next;
		}
	}

	// free interpreter structure
	::SjGcUnref(m_interpr);
	::SjGcUnref(m_executeResult);
	::SjGcUnref(m_persistentAnchor);

	// remove from list
	{
#ifdef __WXDEBUG__
		bool dbg_removed = false;
		long dbg_oldCnt = 0;
		{
			SjSee* dbg_cur = s_first;
			while( dbg_cur )
			{
				dbg_oldCnt++;
				dbg_cur = dbg_cur->m_next;
			}
		}
#endif

		SjSee *cur = s_first, **prev = &s_first;
		while( cur )
		{
			if( cur==this )
			{
#ifdef __WXDEBUG__
				dbg_removed = true;
#endif
				*prev = m_next;
				break;
			}
			prev = &cur->m_next;
			cur = cur->m_next;
		}

#ifdef __WXDEBUG__
		wxASSERT( dbg_removed );
		{
			long dbg_newCnt = 0;
			SjSee* dbg_cur = s_first;
			while( dbg_cur )
			{
				dbg_newCnt++;
				dbg_cur = dbg_cur->m_next;
			}
			wxASSERT( dbg_newCnt == dbg_oldCnt - 1 );
		}
#endif
	}
}



/*******************************************************************************
 *  SjSee - embedding and events
 ******************************************************************************/



void SjSee::ReceiveMsg(int msg)
{
	SjGcLocker gclocker;

	SjSee* cur = s_first;
	while( cur )
	{
		cur->Program_receiveMsg(msg);
		cur = cur->m_next;
	}
}


bool SjSee::HasGlobalEmbeddings(SjSeePersistent embedId) const
{
	wxArrayString strings;
	Program_getGlobalEmbed(embedId, strings);
	return !strings.IsEmpty();
}



wxArrayString SjSee::GetGlobalEmbeddings(SjSeePersistent embedId)
{
	wxArrayString strings;
	SjSee* cur = s_first;
	while( cur )
	{
		cur->Program_getGlobalEmbed(embedId, strings);
		cur = cur->m_next;
	}
	return strings;
}



bool SjSee::OnGlobalEmbedding(SjSeePersistent embedId, int globalIndex, SjSee* doDefaultAction)
{
	SjGcLocker gclocker;

	int localIndexStart = 0, localCount;
	wxArrayString strings;
	SjSee* cur = s_first;
	while( cur )
	{
		strings.Empty();
		cur->Program_getGlobalEmbed(embedId, strings);
		localCount = strings.GetCount();

		if( (globalIndex >= localIndexStart && globalIndex < localIndexStart+localCount)
		        ||  doDefaultAction == cur )
		{
			return cur->Program_onGlobalEmbed(embedId, doDefaultAction? 0 : globalIndex-localIndexStart);
		}

		cur = cur->m_next;
		localIndexStart += localCount;
	}

	return false;
}




/*******************************************************************************
 *  SjSee - execute scripts
 ******************************************************************************/


static void SeeLogErrorObj(SEE_interpreter* interpr_, SEE_value* errorValue)
{
	// is the value an object?
	if( errorValue==NULL || SEE_VALUE_GET_TYPE(errorValue) != SEE_OBJECT )
		return;

	// is the object an array?
	SEE_object* errorObj = errorValue->u.object;
	if( errorObj == NULL )
		return;

	// get what to get ...
	wxString errorName, errorMsg, tempStr;
	long lineno = -1;

	// ... get the error name
	SEE_value tempVal;
	SEE_OBJECT_GET(interpr_, errorObj, str_name, &tempVal);
	if( SEE_VALUE_GET_TYPE(&tempVal) == SEE_STRING )
		errorName = SeeValueToWxString(interpr_, &tempVal);
	errorName.Trim(true);
	errorName.Trim(false);
	if( errorName.IsEmpty() )
		errorName = wxT("Error");

	// ... get the error message, this is formatted as "<string>:<lineno>:msg"
	SEE_OBJECT_GET(interpr_, errorObj, str_message, &tempVal);
	if( SEE_VALUE_GET_TYPE(&tempVal) == SEE_STRING )
	{
		errorMsg = SeeValueToWxString(interpr_, &tempVal);
		if( errorMsg.Find(wxT(":")) >= 0 )
		{
			tempStr = errorMsg.BeforeFirst(wxT(':'));
			errorMsg = errorMsg.AfterFirst(wxT(':'));
		}

		if( errorMsg.Find(wxT(":")) >= 0 )
		{
			tempStr = errorMsg.BeforeFirst(wxT(':'));
			if( !tempStr.ToLong(&lineno, 10) )
				lineno = -1;
			errorMsg = errorMsg.AfterFirst(wxT(':'));
		}

		errorMsg.Trim(true);
		errorMsg.Trim(false);
	}

	if( errorMsg.IsEmpty() )
		errorMsg = wxT("Unknown error");

	// ... get the line number
	if( lineno >= 1 )
		tempStr.Printf(wxT("%i"), (int)lineno);
	else
		tempStr = wxT("?");

	// ... format the line
	wxLogError(wxT("Line %s: %s: %s [%s]"),
	           tempStr.c_str(),
	           errorName.c_str(),
	           errorMsg.c_str(),
	           HOST_DATA->m_executionScope.c_str()
	          );

	// old stuff
#if 0
	{
		SEE_value errorObjAsStr;
		SEE_ToString(interpr_, errorValue, &errorObjAsStr);

		wxString error = ::SeeValueToWxString(interpr_, &errorObjAsStr);
		if( error.IsEmpty() )
			error = wxT("Unknown error.");

		wxLogError(wxT("%s [%s]"), error.c_str(), HOST_DATA->m_executionScope.c_str());
	}
#endif
}



bool SjSee::Execute(const wxString& script__)
{
	wxASSERT( !m_executionScope.IsEmpty() );

	// very first, do some garbarge collection (if needed)
	// this must be done _before_ we create SjGcLocker
	/*if( ::SjGcNeedsCleanup() )
	{
	    ::SjGcCleanup(); -- done in SjGcLocker
	}*/

	// lock the garbage collector
	SjGcLocker gclocker;

	// init the interpreter, if not yet done
	if( !m_interprInitialized )
	{
		// init some global function pointers
		static bool SEE_system_initialized = false;
		if( !SEE_system_initialized )
		{
			SEE_system.abort            = SjSee_abort;
			SEE_system.malloc           = SjSee_malloc;
			SEE_system.malloc_string    = SjSee_malloc_string;
			SEE_system.malloc_finalize  = SjSee_malloc_finalize;
			SEE_system.free             = SjSee_free;
			SEE_system_initialized      = true;
			SEE_system_add_my_strings   ();
		}

		// init the interpreter instance and our objects
		SEE_interpreter_init(m_interpr);
		Player_init();
		Program_init();
		Rights_init();
		Dialog_init();
		Database_init();
		File_init();
		HttpRequest_init();
		m_interprInitialized = true;
	}

	// do what to do
	bool success = true;

	SEE_input*          input;
	SEE_try_context_t   tryContext;

	/* Create an input stream that provides program text */
	{
		wxString script(script__);
		if( script.Find(wxT("\r")) != -1 ) // no "\r" - otherwise, the line numbers get out of order
		{
			if( script.Find(wxT("\n")) != -1 )
				script.Replace(wxT("\r"), wxT(" "));
			else
				script.Replace(wxT("\r"), wxT("\n"));
		}
		input = SEE_input_string(m_interpr, WxStringToSeeString(m_interpr, script));
	}

	/* Establish an exception context */
	SEE_TRY(m_interpr, tryContext)
	{
		/* Call the program evaluator */
		SEE_Global_eval(m_interpr, input, m_executeResult);
	}

	/* Finally: */
	SEE_INPUT_CLOSE(input);

	/* Catch any exceptions */
	SEE_value* errorObj;
	if( (errorObj=SEE_CAUGHT(tryContext)) )
	{
		SeeLogErrorObj(m_interpr, errorObj);
		success = false;
	}

	return success;
}



bool SjSee::ExecuteAsFunction(const wxString& script)
{
	wxASSERT( !m_executionScope.IsEmpty() );

	// add the function, if not yet done
	// this is a little hack as long as we have no real DOM for the skinning tree
	static long s_cnt = 29275837;
	long l = m_dynFunc.Lookup(script);
	if( l == 0 )
	{
		l = s_cnt++;
		wxString declare = wxString::Format(wxT("function dynfunc%i(){%s}"), (int)l, script.c_str());
		Execute(declare);
		m_dynFunc.Insert(script, l);
	}

	return Execute(wxString::Format(wxT("dynfunc%i()"), (int)l));
}



bool SjSee::IsResultDefined()
{
	return SEE_VALUE_GET_TYPE(m_executeResult)!=SEE_UNDEFINED;
}



wxString SjSee::GetResultString()
{
	SjGcLocker gcLocker;

	wxASSERT( wxThread::IsMain() );

	wxString            ret;
	SEE_try_context_t   tryContext;

	if( m_interprInitialized )
	{
		// Establish an exception context
		SEE_TRY(m_interpr, tryContext)
		{
			ret = ::SeeValueToWxString(m_interpr, m_executeResult);
		}

		// Catch any exceptions
		if( SEE_CAUGHT(tryContext) )
		{
			ret.Empty();
		}
	}

	// done
	return ret;
}



double SjSee::GetResultDouble()
{
	SjGcLocker gcLocker;

	wxASSERT( wxThread::IsMain() );

	double              ret = 0.0;
	SEE_value           tempValue;
	SEE_try_context_t   tryContext;

	if( m_interprInitialized )
	{
		// Establish an exception context
		SEE_TRY(m_interpr, tryContext)
		{
			SEE_ToNumber(m_interpr, m_executeResult, &tempValue);
			ret = tempValue.u.number;
		}

		// Catch any exceptions
		if( SEE_CAUGHT(tryContext) )
		{
			ret = 0.0;
		}
	}

	// done
	return ret;
}



long SjSee::GetResultLong()
{
	SjGcLocker gcLocker;

	wxASSERT( wxThread::IsMain() );

	long                ret = 0;
	SEE_try_context_t   tryContext;

	if( m_interprInitialized )
	{
		// Establish an exception context
		SEE_TRY(m_interpr, tryContext)
		{
			ret = SEE_ToInt32(m_interpr, m_executeResult);
		}

		// Catch any exceptions
		if( SEE_CAUGHT(tryContext) )
		{
			ret = 0;
		}
	}

	// done
	return ret;
}



wxString SjSee::GetFineName(const wxString& scope, const wxString& append)
{
	wxString fineName(scope);


	if( fineName.Find(wxT('/')) != -1
	        || fineName.Find(wxT('\\')) != -1
	        || fineName.Find(wxT(':')) != -1 )
	{
		// strip path
		fineName = SjTools::GetFileNameFromUrl(fineName, NULL, true);

		// replace some special characters
		fineName.Replace(wxT("_"), wxT(" "));
		fineName.Replace(wxT("-"), wxT(" "));

		// make first letter uppercase
		int p = -1;
		fineName = fineName.Mid(p+1, 1).Upper() + fineName.Mid(p+2);
	}

	// append extra string
	if( !append.IsEmpty() )
	{
		fineName += wxT(" - ") + append;
	}

	return fineName;
}


/*******************************************************************************
 *  Some Tools as declared in sj_see_helper.h
 ******************************************************************************/



wxString SeeValueToWxString(SEE_interpreter* interpr, SEE_value* value)
{
	SEE_string* ret = NULL;
	SEE_value   tempValue;

	if( value != NULL
	        && SEE_VALUE_GET_TYPE(value) != SEE_UNDEFINED )
	{
		SEE_ToString(interpr, value, &tempValue);
		ret = tempValue.u.string;
	}

	if( ret )
	{
#ifdef __WXMSW__
		wxASSERT( sizeof(SEE_char_t) == sizeof(wxChar) );
		return wxString((const wxChar *) ret->data, ret->length); // (const wxChar *) added on VC2008 migration
#else
		SEE_char_t* retPtr = ret->data;
		long i, iCount = (long)ret->length;
		wxChar* wxBase = (wxChar*)malloc((iCount+1)*sizeof(wxChar)); if( wxBase == NULL ) { return wxEmptyString; }
		wxChar* wxPtr = wxBase;
		for( i = 0; i < iCount; i++ )
			wxPtr[i] = (wxChar)retPtr[i];
		wxPtr[iCount] = 0;
		wxString wx(wxBase);
		free(wxBase);
		return wx;
#endif
	}
	else
	{
		return wxT("");
	}
}


wxArrayLong SeeValueToWxArrayLong(SEE_interpreter* interpr, SEE_value* value)
{
	wxArrayLong ret;

	// is the value an object?
	if( value==NULL || SEE_VALUE_GET_TYPE(value) != SEE_OBJECT )
		return ret;

	// is the object an array?
	SEE_object* object = value->u.object;
	if( object == NULL || !SEE_is_Array(object) )
		return ret;

	// is the array length > 0?
	SEE_uint32_t i, length = SEE_Array_length(interpr, object);
	if( length <= 0 )
		return ret;

	// initialize the return value to the length
	ret.Add(0, length);

	// go through all properties and add them to the correct index
	SEE_enum *e;
	SEE_string *s;
	SEE_value item;
	int flags;
	e = SEE_OBJECT_ENUMERATOR(interpr, object);
	while ((s = SEE_ENUM_NEXT(interpr, e, &flags)))
	{
		if( SEE_to_array_index(s, &i) && i >= 0 && i < length )
		{
			SEE_OBJECT_GET(interpr, object, s, &item);
			ret[i] = SEE_ToInt32(interpr, &item);
		}
	}

	// success
	return ret;
}


double SeeValueToDouble(SEE_interpreter* interpr, SEE_value* value)
{
	if( value != NULL )
	{
		if( SEE_VALUE_GET_TYPE(value) == SEE_NUMBER )
			return value->u.number;

		SEE_value tempValue;
		SEE_ToNumber(interpr, value, &tempValue);
		return tempValue.u.number;
	}

	return 0.0;
}


SEE_string* WxStringToSeeString(SEE_interpreter* interpr, const wxString& src)
{
	long            srcLen = src.Len(), i;
	const wxChar*   srcPtr = src.c_str();
	SEE_string*     dest = SEE_string_new(interpr, srcLen); // the object is freed in the garbage collection
	SEE_char_t*     destPtr = dest->data;

	for( i = 0; i < srcLen; i++ )
		destPtr[i] = srcPtr[i];

	dest->length = srcLen;
	return dest;
}



SEE_object* WxArrayStringToSeeObject(SEE_interpreter* interpr, const wxArrayString& wx)
{
	// construct an array to A
	SEE_value temp;
	_SEE_OBJECT_CONSTRUCT(interpr, interpr->Array, interpr->Array, 0, NULL, &temp);
	SEE_object* A = temp.u.object;

	// add all strings to A
	int i, iCount = wx.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		SEE_SET_STRING(&temp, WxStringToSeeString(interpr, wx[i]));
		SEE_Array_push(interpr, A, &temp);
	}

	return A;
}



SEE_object* WxArrayLongToSeeObject(SEE_interpreter* interpr, const wxArrayLong& wx)
{
	// construct an array to A
	SEE_value temp;
	_SEE_OBJECT_CONSTRUCT(interpr, interpr->Array, interpr->Array, 0, NULL, &temp);
	SEE_object* A = temp.u.object;

	// add all longs to A
	int i, iCount = wx.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		SEE_SET_NUMBER(&temp, (double)wx[i]);
		SEE_Array_push(interpr, A, &temp);
	}

	return A;
}


SEE_object* SeeValueToSeeCallback(SEE_interpreter* interpr, SEE_value* value)
{
	wxASSERT( SjGcIsValidPtr(interpr) );
	if( value )
	{
		if( SEE_VALUE_GET_TYPE(value) == SEE_OBJECT )
		{
			SEE_object* fnObj = value->u.object;
			if( SEE_OBJECT_HAS_CALL(fnObj) )
			{
				wxASSERT( SjGcIsValidPtr(fnObj) );
				return fnObj;
			}
		}
	}

	return NULL;
}


void SeeCallCallback(SEE_interpreter* interpr_, SEE_object* fnObj, SEE_object* fnContext, SEE_value* arg0)
{
	SjGcLocker gcLocker;

	wxASSERT( interpr_ );
	wxASSERT( SjGcIsValidPtr(interpr_) );

	if( fnObj )
	{
		wxASSERT( SEE_OBJECT_HAS_CALL(fnObj) );

		int                 argc = 0;
		SEE_value*          argv[16];
		SEE_value           result;
		SEE_try_context_t   tryContext;

		// set parameters
		if( arg0 )
		{
			argv[0] = arg0;
			argc++;
		}

		// call
		SEE_TRY(interpr_, tryContext)
		{
			SEE_OBJECT_CALL(interpr_, fnObj, fnContext, argc, argv, &result);
		}

		SEE_value* errorObj;
		if( (errorObj=SEE_CAUGHT(tryContext)) )
		{
			SeeLogErrorObj(interpr_, errorObj);
		}
	}
}



void SjSee::AddPersistentObject(SEE_object* newObj, SjSeePersistent scope, SEE_value* param2,
                                SJ_UNEXPECTED_UNPERSISTENT_PROC unexpectedUnpersistent)
{
	if( newObj )
	{
		persistent_object* cur = (persistent_object*)::SjGcAlloc(sizeof(persistent_object), SJ_GC_ZERO);
		cur->m_object                   = newObj;
		cur->m_scope                    = scope;
		cur->m_param2                   = param2;
		cur->m_unexpectedUnpersistent   = unexpectedUnpersistent;

#if 0
		// add as first object, very easy and fast
		cur->m_next = m_persistentAnchor->m_next;
		m_persistentAnchor->m_next = cur;
#else
		// add as last object, little slower, but we need this eg. for proper menu ordering
		persistent_object* test = m_persistentAnchor;
		while( test->m_next )
			test = test->m_next;
		test->m_next = cur;
#endif
	}
}


void SjSee::RemovePersistentObject(SEE_object* oldObj)
{
	if( oldObj )
	{
#ifdef __WXDEBUG__
		bool dbg_removed = false;
#endif
		persistent_object *cur = m_persistentAnchor, *prev = NULL;
		while( cur )
		{
			if( cur->m_object == oldObj )
			{
				wxASSERT( prev ); // as we have a "dummy" anchor, prev should always be valid here
				prev->m_next = cur->m_next;
#ifdef __WXDEBUG__
				dbg_removed = true;
#endif
				break;
			}

			prev = cur;
			cur = cur->m_next;
		}

		wxASSERT( dbg_removed );
	}
}

#endif // SJ_USE_SCRIPTS

