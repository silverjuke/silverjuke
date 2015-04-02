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
 * File:    obj_HttpRequest.cpp
 * Authors: Björn Petersen
 * Purpose: The HttpRequest class for SEE
 *
 ******************************************************************************/




#include "../basesj.h"
#include "sj_see.h"
#include "sj_see_helpers.h"


// data used by our object
class SjHttpEvtHandler;

struct httprequest_object
{
	SEE_native          m_native; // MUST be first!!!
	SjSee*              m_see;
	SjSSHash*           m_requestHeader;

	// objects involved into processing and responsing
	SjHttp*             m_sjHttp;
	SjHttpEvtHandler*   m_sjHttpEvtHandler;
	SEE_object*         m_persistentCallback;
};


static void httprequest_freeAll_(httprequest_object* hro, bool removePersistance);

static void httprequest_finalize_(SEE_interpreter* interpr, void* ptr, void* closure)
{
	httprequest_object* hro = ((httprequest_object*)ptr);
	httprequest_freeAll_(hro, true /*remove persistance*/);
}

static void httprequest_unexpected_unpersistent_(SEE_interpreter* interpr, void* ptr, void* closure)
{
	httprequest_object* hro = ((httprequest_object*)ptr);
	httprequest_freeAll_(hro, false /*remove persistance*/);
}


static httprequest_object* alloc_httprequest_object_(SEE_interpreter* interpr)
{
	httprequest_object* hro = (httprequest_object*)SEE_malloc_finalize(interpr, sizeof(httprequest_object), httprequest_finalize_, NULL);
	memset(hro, 0, sizeof(httprequest_object));

	hro->m_see              = (SjSee*)interpr->host_data;

	return hro;
}

static httprequest_object* get_httprequest_object_(SEE_interpreter* interpr, SEE_object* o);



/*******************************************************************************
 *  The Little Event Handler
 ******************************************************************************/



class SjHttpEvtHandler : public wxEvtHandler
{
public:
	SjHttpEvtHandler(httprequest_object* hro)
	{
		m_hro = hro;
	}

	void OnSocketDone(wxSocketEvent& e)
	{
		wxASSERT( wxThread::IsMain() );

		if( m_hro && m_hro->m_see && m_hro->m_sjHttp )
		{
			SjGcLocker gclocker;
			if( m_hro->m_persistentCallback )
			{
				SEE_object* cb = m_hro->m_persistentCallback;
				m_hro->m_persistentCallback = NULL;
				m_hro->m_see->RemovePersistentObject((SEE_object*)m_hro);

				SeeCallCallback(m_hro->m_see->m_interpr,
				                cb,                         // function, SeeCallCallback() checks for NULL
				                (SEE_object*)m_hro          // context
				               );
			}
		}
	}

	httprequest_object* m_hro;

	DECLARE_EVENT_TABLE()
};



#define IDO_SEESOCKET (IDM_FIRSTPRIVATE+1)

BEGIN_EVENT_TABLE(SjHttpEvtHandler, wxEvtHandler)
	EVT_SOCKET(IDO_SEESOCKET, SjHttpEvtHandler::OnSocketDone)
END_EVENT_TABLE()



static void httprequest_freeResponse_(httprequest_object* hro, bool removePersistance)
{
	// this function is also used as the "unexpected exit" for persistent objects;
	// as this is followed by the "normal finalizer", set all pointers to zero

	wxASSERT( wxThread::IsMain() );

	if( hro->m_sjHttpEvtHandler )
	{
		hro->m_sjHttpEvtHandler->m_hro = NULL;
		delete hro->m_sjHttpEvtHandler;
		hro->m_sjHttpEvtHandler = NULL;
	}

	if( hro->m_sjHttp )
	{
		delete hro->m_sjHttp;
		hro->m_sjHttp = NULL;
	}

	if( hro->m_persistentCallback )
	{
		hro->m_persistentCallback = NULL;
		if( removePersistance )
			hro->m_see->RemovePersistentObject((SEE_object*)hro);
	}
}



static void httprequest_freeAll_(httprequest_object* hro, bool removePersistance)
{
	httprequest_freeResponse_(hro, removePersistance);

	if(  hro->m_requestHeader )
	{
		delete hro->m_requestHeader;
		hro->m_requestHeader = NULL;
	}
}



/*******************************************************************************
 *  Functions
 ******************************************************************************/



IMPLEMENT_FUNCTION(httprequest, construct)
{
	RETURN_OBJECT( HOST_DATA->HttpRequest_new() );
}



IMPLEMENT_FUNCTION(httprequest, setRequestHeader)
{
	httprequest_object* hro = get_httprequest_object_(interpr_, this_);

	if( hro->m_requestHeader == NULL )
		hro->m_requestHeader = new SjSSHash();

	if( argc_ == 0 )
		hro->m_requestHeader->Clear();
	else
		hro->m_requestHeader->Insert(ARG_STRING(0), ARG_STRING(1));

	RETURN_UNDEFINED;
}



IMPLEMENT_FUNCTION(httprequest, getResponseHeader)
{
	httprequest_object* hro = get_httprequest_object_(interpr_, this_);
	wxString ret;
	if( hro->m_sjHttp )
		ret = hro->m_sjHttp->GetHttpResponseHeader(ARG_STRING(0));
	RETURN_STRING(ret);
}



IMPLEMENT_FUNCTION(httprequest, request)
{
	// free previous HTTP object (if any)
	httprequest_object* hro = get_httprequest_object_(interpr_, this_);
	httprequest_freeResponse_(hro, true);

	// get the callback
	if( argc_ < 2 )
		SEE_error_throw(interpr_, interpr_->Error, "too few parameters");
	if( argc_ > 3 )
		SEE_error_throw(interpr_, interpr_->Error, "too many parameters");
	SEE_object* callback  = ARG_CALLBACK(argc_-1);
	if( callback == NULL )
		SEE_error_throw(interpr_, interpr_->TypeError, "no callback given");

	// get the post buffer (if any)
	wxString postBuffer;
	if( argc_ > 2 )
		postBuffer = ARG_STRING(1);

	// set object persistent, remember the callback (always the last parameter)
	hro->m_see->AddPersistentObject((SEE_object*)hro, SJ_PERSISTENT_OTHER, NULL, httprequest_unexpected_unpersistent_);
	hro->m_persistentCallback = callback;

	// create a new HTTP object
	hro->m_sjHttp = new SjHttp();
	hro->m_sjHttpEvtHandler = new SjHttpEvtHandler(hro);

	hro->m_sjHttp->Init(hro->m_sjHttpEvtHandler, IDO_SEESOCKET);
	hro->m_sjHttp->OpenFile(ARG_STRING(0), &wxConvUTF8, hro->m_requestHeader, postBuffer);

	RETURN_UNDEFINED;
}



IMPLEMENT_FUNCTION(httprequest, abort)
{
	httprequest_object* hro = get_httprequest_object_(interpr_, this_);
	httprequest_freeResponse_(hro, true);

	RETURN_UNDEFINED;
}



/*******************************************************************************
 *  Properties
 ******************************************************************************/



IMPLEMENT_HASPROPERTY(httprequest)
{
	if(
	    VAL_PROPERTY( status )
	    || VAL_PROPERTY( responseText )
	)
	{
		RETURN_HAS;
	}
	else
	{
		RETURN_HASNOT;
	}
}



IMPLEMENT_GET(httprequest)
{
	httprequest_object* hro = get_httprequest_object_(interpr_, this_);

	if( VAL_PROPERTY( status ) )
	{
		long status = 400; // bad request/nothing loaded yet
		if( hro->m_sjHttp )
			status = hro->m_sjHttp->GetHttpStatusCode();
		RETURN_LONG( status );
	}
	else if( VAL_PROPERTY( responseText ) )
	{
		wxString text;
		if( hro->m_sjHttp )
			text = hro->m_sjHttp->GetString();
		RETURN_STRING( text );
	}
	else
	{
		RETURN_GET_DEFAULTS;
	}
}



/*******************************************************************************
 *  Let SEE know about this class (this part is a little more complicated)
 ******************************************************************************/



/* object class for HttpRequest.prototype and httprequest instances */
static SEE_objectclass httprequest_inst_class = {
	"HttpRequest",              /* Class */
	httprequest_get,            /* Get */
	SEE_native_put,             /* Put */
	SEE_native_canput,          /* CanPut */
	httprequest_hasproperty,    /* HasProperty */
	SEE_native_delete,          /* Delete */
	SEE_native_defaultvalue,    /* DefaultValue */
	SEE_native_enumerator,      /* DefaultValue */
	NULL,                       /* Construct */
	NULL,                       /* Call */
	NULL                        /* HasInstance */
};



/* object class for HttpRequest constructor */
SEE_objectclass httprequest_constructor_class = {
	"HttpRequestConstructor",   /* Class */
	SEE_native_get,             /* Get */
	SEE_native_put,             /* Put */
	SEE_native_canput,          /* CanPut */
	SEE_native_hasproperty,     /* HasProperty */
	SEE_native_delete,          /* Delete */
	SEE_native_defaultvalue,    /* DefaultValue */
	SEE_native_enumerator,      /* DefaultValue */
	httprequest_construct,      /* Construct */
	NULL                        /* Call */
};



void SjSee::HttpRequest_init()
{
	SEE_value temp;

	// Create the "HttpRequest.prototype" object, this is used as a template for the instances
	m_HttpRequest_prototype = (SEE_object *)alloc_httprequest_object_(m_interpr);

	SEE_native_init((SEE_native *)m_HttpRequest_prototype, m_interpr,
	                &httprequest_inst_class, m_interpr->Object_prototype);

	PUT_FUNC(m_HttpRequest_prototype, httprequest, setRequestHeader, 0);
	PUT_FUNC(m_HttpRequest_prototype, httprequest, getResponseHeader, 0);
	PUT_FUNC(m_HttpRequest_prototype, httprequest, request, 0);
	PUT_FUNC(m_HttpRequest_prototype, httprequest, abort, 0);

	// create the "HttpRequest" object
	SEE_object* HttpRequest = (SEE_object *)SEE_malloc(m_interpr, sizeof(SEE_native));

	SEE_native_init((SEE_native *)HttpRequest, m_interpr,
	                &httprequest_constructor_class, m_interpr->Object_prototype);

	PUT_OBJECT(HttpRequest, str_prototype, m_HttpRequest_prototype);

	// now we can add the globals
	PUT_OBJECT(m_interpr->Global, str_HttpRequest,   HttpRequest);
}



SEE_object* SjSee::HttpRequest_new()
{
	httprequest_object* obj = alloc_httprequest_object_(m_interpr);

	SEE_native_init(&obj->m_native, m_interpr,
	                &httprequest_inst_class, m_HttpRequest_prototype);

	return (SEE_object *)obj;
}



static httprequest_object* get_httprequest_object_(SEE_interpreter* interpr, SEE_object* o)
{
	wxASSERT( wxThread::IsMain() );

	if( o->objectclass != &httprequest_inst_class )
		SEE_error_throw(interpr, interpr->TypeError, NULL);

	return (httprequest_object *)o;
}

