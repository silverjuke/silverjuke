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
 * File:    obj_Rights.cpp
 * Authors: Björn Petersen
 * Purpose: The Rights class for SEE
 *
 ******************************************************************************/


#include <sjbase/base.h>


#if SJ_USE_SCRIPTS


#include <sjmodules/kiosk/kiosk.h>
#include <sjbase/browser.h>
#include <see_dom/sj_see.h>
#include <see_dom/sj_see_helpers.h>


// data used by our object
struct rights_object
{
	SEE_native  native;
};


/*******************************************************************************
 * Misc.
 ******************************************************************************/


IMPLEMENT_FUNCTION(rights, construct)
{
	RETURN_OBJECT( HOST_DATA->Rights_new() );
}


IMPLEMENT_FUNCTION(rights, viewMode)
{
	RETURN_BOOL( g_mainFrame->m_browser->IsViewAvailable(ARG_LONG(0)) );
}


/*******************************************************************************
 * Properties
 ******************************************************************************/


IMPLEMENT_HASPROPERTY(rights)
{
	if(
	    VAL_PROPERTY( all )
	 || VAL_PROPERTY( credits )
	 || VAL_PROPERTY( useCredits )
	 || VAL_PROPERTY( play )
	 || VAL_PROPERTY( pause )
	 || VAL_PROPERTY( stop )
	 || VAL_PROPERTY( editQueue )
	 || VAL_PROPERTY( multiEnqueue )
	 || VAL_PROPERTY( search )
	 || VAL_PROPERTY( startVis )
	 || VAL_PROPERTY( volume )
	 || VAL_PROPERTY( prelisten )
	 || VAL_PROPERTY( zoom )
	)
	{
		RETURN_HAS;
	}
	else
	{
		RETURN_HASNOT;
	}
}


IMPLEMENT_GET(rights)
{
	if( VAL_PROPERTY( all ) )
	{
		RETURN_BOOL( g_mainFrame->IsAllAvailable() );
	}
	else if( VAL_PROPERTY( credits ) )
	{
		RETURN_LONG( g_kioskModule->m_creditBase.GetCredit() );
	}
	else if( VAL_PROPERTY( useCredits ) )
	{
		RETURN_BOOL( g_kioskModule->m_creditBase.IsCreditSystemEnabled() );
	}
	else if( VAL_PROPERTY( play ) || VAL_PROPERTY( pause ) || VAL_PROPERTY( stop ) )
	{
		RETURN_BOOL( g_mainFrame->IsOpAvailable(SJ_OP_PLAYPAUSE) );
	}
	else if( VAL_PROPERTY( editQueue ) )
	{
		RETURN_BOOL( g_mainFrame->IsOpAvailable(SJ_OP_EDIT_QUEUE) );
	}
	else if( VAL_PROPERTY( unqueue ) )
	{
		RETURN_BOOL( g_mainFrame->IsOpAvailable(SJ_OP_UNQUEUE) );
	}
	else if( VAL_PROPERTY( multiEnqueue ) )
	{
		RETURN_BOOL( g_mainFrame->IsOpAvailable(SJ_OP_MULTI_ENQUEUE) );
	}
	else if( VAL_PROPERTY( search ) )
	{
		RETURN_BOOL( g_mainFrame->IsOpAvailable(SJ_OP_SEARCH) );
	}
	else if( VAL_PROPERTY( startVis ) )
	{
		RETURN_BOOL( g_mainFrame->IsOpAvailable(SJ_OP_STARTVIS) );
	}
	else if( VAL_PROPERTY( volume ) )
	{
		RETURN_BOOL( g_mainFrame->IsOpAvailable(SJ_OP_MAIN_VOL) );
	}
	else if( VAL_PROPERTY( prelisten ) )
	{
		RETURN_BOOL( g_mainFrame->IsOpAvailable(SJ_OP_PRELISTEN) );
	}
	else if( VAL_PROPERTY( zoom ) )
	{
		RETURN_BOOL( g_mainFrame->IsOpAvailable(SJ_OP_ZOOM) );
	}
	else if( VAL_PROPERTY( repeat ) )
	{
		RETURN_BOOL( g_mainFrame->IsOpAvailable(SJ_OP_REPEAT) );
	}
	else
	{
		RETURN_GET_DEFAULTS;
	}
}


IMPLEMENT_PUT(rights)
{
	long opToChange = 0;

	if( VAL_PROPERTY( credits ) )
	{
		g_kioskModule->m_creditBase.AddCredit(VAL_LONG, SJ_ADDCREDIT_SET_TO_NULL_BEFORE_ADD);
	}
	else if( VAL_PROPERTY( editQueue ) )
	{
		opToChange = SJ_OP_EDIT_QUEUE;
	}
	else if( VAL_PROPERTY( unqueue ) )
	{
		opToChange = SJ_OP_UNQUEUE;
	}
	else if( VAL_PROPERTY( multiEnqueue ) )
	{
		opToChange = SJ_OP_MULTI_ENQUEUE;
	}
	else
	{
		DO_PUT_DEFAULTS;
	}

	// change a flag, added in 2.52beta2, see also http://www.silverjuke.net/forum/topic-2204.html
	if( opToChange != 0 && !g_mainFrame->IsAllAvailable() )
	{
		SjTools::SetFlag(g_mainFrame->m_availOp, opToChange, VAL_BOOL);

		g_mainFrame->UpdateDisplay();
	}
}


/*******************************************************************************
 * Let SEE know about this class (this part is a little more complicated)
 ******************************************************************************/


/* object class for Rights.prototype and rights instances */
static SEE_objectclass rights_inst_class = {
	"Rights",                   /* Class */
	rights_get,                 /* Get */
	rights_put,                 /* Put */
	SEE_native_canput,          /* CanPut */
	rights_hasproperty,         /* HasProperty */
	SEE_native_delete,          /* Delete */
	SEE_native_defaultvalue,    /* DefaultValue */
	SEE_native_enumerator,      /* DefaultValue */
	NULL,                       /* Construct */
	NULL,                       /* Call */
	NULL                        /* HasInstance */
};


/* object class for Rights constructor */
static SEE_objectclass rights_constructor_class = {
	"RightsConstructor",        /* Class */
	SEE_native_get,             /* Get */
	SEE_native_put,             /* Put */
	SEE_native_canput,          /* CanPut */
	SEE_native_hasproperty,     /* HasProperty */
	SEE_native_delete,          /* Delete */
	SEE_native_defaultvalue,    /* DefaultValue */
	SEE_native_enumerator,      /* DefaultValue */
	rights_construct,           /* Construct */
	NULL                        /* Call */
};


void SjSee::Rights_init()
{
	SEE_value temp;

	// Create the "Rights.prototype" object, this is used as a template for the instances
	m_Rights_prototype = (SEE_object *)SEE_malloc(m_interpr, sizeof(rights_object));

	SEE_native_init((SEE_native *)m_Rights_prototype, m_interpr,
	                &rights_inst_class, m_interpr->Object_prototype);

	PUT_FUNC(m_Rights_prototype, rights, viewMode, 0);

	// create the "Rights" object
	SEE_object* Rights = (SEE_object *)SEE_malloc(m_interpr, sizeof(SEE_native));

	SEE_native_init((SEE_native *)Rights, m_interpr,
	                &rights_constructor_class, m_interpr->Object_prototype);

	PUT_OBJECT(Rights, str_prototype, m_Rights_prototype)

	// now we can add the globals
	PUT_OBJECT(m_interpr->Global, str_Rights,    Rights);
	PUT_OBJECT(m_interpr->Global, str_rights,    Rights_new());
}


SEE_object* SjSee::Rights_new()
{
	rights_object* obj = (rights_object*)SEE_malloc(m_interpr, sizeof(rights_object));

	SEE_native_init(&obj->native, m_interpr,
	                &rights_inst_class, m_Rights_prototype);

	return (SEE_object *)obj;
}


#endif // SJ_USE_SCRIPTS
