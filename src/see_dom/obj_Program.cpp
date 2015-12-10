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
 * File:    obj_Program.cpp
 * Authors: Björn Petersen
 * Purpose: The Program class for SEE
 *
 ******************************************************************************/


#include <sjbase/base.h>


#if SJ_USE_SCRIPTS


#include <sjbase/browser.h>
#include <sjbase/browser_list.h>
#include <sjmodules/kiosk/kiosk.h>
#include <sjmodules/advsearch.h>
#include <sjmodules/vis/vis_module.h>
#include <see_dom/sj_see.h>
#include <see_dom/sj_see_helpers.h>


// data used by our object
struct program_object
{
	SEE_native      m_native; // MUST be first!

	SEE_object*     m_onLoad;
	SEE_object*     m_onUnload;

	SEE_object*     m_onKioskStarting;
	SEE_object*     m_onKioskStarted;
	SEE_object*     m_onKioskEnding;
	SEE_object*     m_onKioskEnded;
	SEE_object*     m_onTimeout;
};


static program_object* alloc_program_object(SEE_interpreter* interpr)
{
	program_object* po = (program_object*)SEE_malloc(interpr, sizeof(program_object));
	memset(po, 0, sizeof(program_object));
	return po;
}


/*******************************************************************************
 * Constructor
 ******************************************************************************/


IMPLEMENT_FUNCTION(program, construct)
{
	RETURN_OBJECT( HOST_DATA->Program_new() );
}


/*******************************************************************************
 * Embedding
 ******************************************************************************/


void SjSee::Program_getGlobalEmbed(SjSeePersistent scope, wxArrayString& ret) const
{
	persistent_object* cur = m_persistentAnchor->m_next;
	while( cur )
	{
		if( cur->m_scope == scope )
		{
			ret.Add(SeeValueToWxString(m_interpr, cur->m_param2));
		}
		cur = cur->m_next;
	}
}


bool SjSee::Program_onGlobalEmbed(SjSeePersistent scope, int index)
{
	persistent_object* cur = m_persistentAnchor->m_next;
	while( cur )
	{
		if( cur->m_scope == scope )
		{
			if( index == 0 )
			{
				SeeCallCallback(m_interpr, cur->m_object, cur->m_object);
				return true;
			}
			index--;
		}
		cur = cur->m_next;
	}
	return false;
}


static void addEntry(SEE_interpreter* interpr_, SjSeePersistent scope, int argc_,
                     SEE_value** argv_, SEE_value* res_)
{
	bool success = false;
	SEE_object* callback = ARG_CALLBACK(1);
	if( argc_ >= 2 && callback )
	{
		// add entry
		SEE_value* param2 = (SEE_value*)SEE_malloc(interpr_, sizeof(SEE_value));
		SEE_VALUE_COPY(param2, argv_[0]);
		HOST_DATA->AddPersistentObject(callback, scope, param2);
		success = true;
	}
	else if( argc_ >= 1 && SEE_VALUE_GET_TYPE(argv_[0]) == SEE_STRING )
	{
		// remove entry by name
		persistent_object* cur = HOST_DATA->m_persistentAnchor->m_next;
		while( cur )
		{
			if( cur->m_scope == scope
			 && cur->m_param2
			 && SEE_VALUE_GET_TYPE(cur->m_param2) == SEE_STRING
			 && SEE_string_cmp(cur->m_param2->u.string, argv_[0]->u.string)==0 )
			{
				HOST_DATA->RemovePersistentObject(cur->m_object);
				success = true;
				break;
			}
			cur = cur->m_next;
		}
	}
	else
	{
		// remove entry by type
		persistent_object *cur = HOST_DATA->m_persistentAnchor->m_next, *next;
		while( cur )
		{
			next = cur->m_next;
			if( cur->m_scope == scope )
			{
				HOST_DATA->RemovePersistentObject(cur->m_object);
				success = true;
				// no break - there may be several entries!
			}
			cur = next;
		}
	}
	RETURN_BOOL( success );
}


IMPLEMENT_FUNCTION(program, addMenuEntry)
{
	addEntry(interpr_, SJ_PERSISTENT_MENU_ENTRY, argc_, argv_, res_);
	g_mainFrame->InitMainMenu();
}


IMPLEMENT_FUNCTION(program, addConfigButton)
{
	addEntry(interpr_, SJ_PERSISTENT_CONFIG_BUTTON, argc_, argv_, res_);
}


IMPLEMENT_FUNCTION(program, addSkinsButton)
{
	addEntry(interpr_, SJ_PERSISTENT_SKINS_BUTTON, argc_, argv_, res_);
}


IMPLEMENT_FUNCTION(program, addExitOption)
{
	addEntry(interpr_, SJ_PERSISTENT_EXIT_OPTION, argc_, argv_, res_);
}


/*******************************************************************************
 * Timers
 ******************************************************************************/


SjProgramTimer::SjProgramTimer(SEE_interpreter* interpr, program_object* po)
{
	interpr_ = interpr;
	m_po = po;
	m_inTimer = false;
}


void SjProgramTimer::Notify()
{
	wxASSERT( (SEE_object*)m_po == HOST_DATA->m_program );
	wxASSERT( wxThread::IsMain() );

	if( !m_inTimer )
	{
		m_inTimer = true;
		SEE_object* callback = m_po->m_onTimeout;

		if( IsOneShot() )
		{
			HOST_DATA->RemovePersistentObject( m_po->m_onTimeout );
			m_po->m_onTimeout = NULL;
		}

		if( callback )
		{
			SeeCallCallback(interpr_, callback, (SEE_object*)m_po);
		}
		m_inTimer = false;
	}
}


IMPLEMENT_FUNCTION(program, setTimeout)
{
	program_object* po = (program_object*)HOST_DATA->m_program;
	SEE_object*     callback = ARG_CALLBACK(0);
	long            timeout = ARG_LONG(1);

	// stop any timer
	if( HOST_DATA->m_timer )
	{
		HOST_DATA->m_timer->Stop();
	}

	// remove any previous callbacks
	if( po->m_onTimeout )
	{
		HOST_DATA->RemovePersistentObject( po->m_onTimeout );
		po->m_onTimeout = NULL;
	}

	// set and start new timer (if any - you can also assign "undefined" as the new callback, AddPersistenObject handles this)
	if( callback && timeout > 0 )
	{
		po->m_onTimeout = callback;
		HOST_DATA->AddPersistentObject(po->m_onTimeout, SJ_PERSISTENT_OTHER, NULL);

		if( HOST_DATA->m_timer == NULL )
			HOST_DATA->m_timer = new SjProgramTimer(interpr_, po);
		HOST_DATA->m_timer->Start(timeout, ARG_LONG(2)? wxTIMER_CONTINUOUS : wxTIMER_ONE_SHOT);
	}
}


/*******************************************************************************
 * Import / Export
 ******************************************************************************/


IMPLEMENT_FUNCTION(program, exportFunction)
{
	SEE_object* function = ARG_CALLBACK(0);
	if( function == NULL )
		SEE_error_throw(interpr_, interpr_->TypeError, "no function");

	HOST_DATA->AddPersistentObject(function, SJ_PERSISTENT_EXPORT, NULL);
}


static void copyVal(SEE_interpreter* srcInterpr, const SEE_value* srcVal,
                    SEE_interpreter* destInterpr, SEE_value* destVal)
{
	switch( SEE_VALUE_GET_TYPE(srcVal) )
	{
		case SEE_STRING:
			SEE_SET_STRING(destVal, SEE_string_dup(destInterpr, srcVal->u.string));
			break;

		case SEE_BOOLEAN:
			SEE_SET_BOOLEAN(destVal, srcVal->u.boolean);
			break;

		case SEE_NUMBER:
			SEE_SET_NUMBER(destVal, srcVal->u.number);
			break;

		case SEE_NULL:
			SEE_SET_NULL(destVal);
			break;

		default: // objects cannot be copied this way!
			SEE_SET_UNDEFINED(destVal);
			break;
	}
}


IMPLEMENT_FUNCTION(program, callExported)
{
	if( argc_ < 1
	        || SEE_VALUE_GET_TYPE(argv_[0]) != SEE_STRING )
		SEE_error_throw(interpr_, interpr_->TypeError, "no function");

	SEE_string *wantedFuncName = argv_[0]->u.string, *otherFuncName;
	SjSee* otherSee = SjSee::s_first;
	while( otherSee )
	{
		persistent_object* otherPer = otherSee->m_persistentAnchor->m_next;
		while( otherPer )
		{
			if( otherPer->m_scope == SJ_PERSISTENT_EXPORT )
			{
				otherFuncName = SEE_function_getname(otherSee->m_interpr, otherPer->m_object);
				if( SEE_string_cmp(wantedFuncName, otherFuncName) == 0 )
				{
					// make a copy of all arguments
					int         otherArgc = argc_-1, i;
					SEE_value** otherArgv = (SEE_value**)SEE_malloc(otherSee->m_interpr, sizeof(SEE_value*)*(otherArgc+1));
					SEE_value   otherResult;
					for( i = 0; i < otherArgc; i++ )
					{
						otherArgv[i] = (SEE_value*)SEE_malloc(otherSee->m_interpr, sizeof(SEE_value));
						copyVal(interpr_, argv_[i+1], otherSee->m_interpr, otherArgv[i]);
					}

					// call the exported function
					SEE_OBJECT_CALL(otherSee->m_interpr, otherPer->m_object, otherPer->m_object,
					                otherArgc, otherArgv, &otherResult);

					// copy back the result to our scope
					copyVal(otherSee->m_interpr, &otherResult, interpr_, res_);
					return;
				}
			}

			otherPer = otherPer->m_next;
		}

		otherSee = otherSee->m_next;
	}

	SEE_error_throw(interpr_, interpr_->Error, "export not found");
}


IMPLEMENT_FUNCTION(program, callPlugin)
{
	/* -- cplugins are currently not supported --
	SjCPlugin* p = HOST_DATA->m_plugin;
	if( p == NULL )
		SEE_error_throw(interpr_, interpr_->Error, "no plugin");

	LPARAM ret = p->CallPlugin(SJ_PLUGIN_CALL, p->EncodeString(ARG_STRING(0)), p->EncodeString(ARG_STRING(1)), p->EncodeString(ARG_STRING(2)) );
	if( ret == 0 || ret == 1 )
		RETURN_BOOL( ret );
	else
		RETURN_STRING( p->DecodeString(ret) );
	*/
	RETURN_BOOL( FALSE );
}


/*******************************************************************************
 * Misc.
 ******************************************************************************/


IMPLEMENT_FUNCTION(program, selectAll)
{
	bool doSelect = argc_>0? ARG_BOOL(0) : true;

	g_mainFrame->m_columnMixer.SelectAll(doSelect);
	g_mainFrame->m_browser->RefreshSelection();

	if( !doSelect )
	{	                                                // as getSelection also returns URLs selected in the display,
		g_mainFrame->m_display.ClearDisplaySelection(); // they should be cleared as well. For "select all" this is not needed as
	}                                                   // they are included in the workspace.

	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(program, getSelection)
{
	wxArrayString selUrls;
	SjSelectedUrlsScope scope;
	switch( ARG_LONG(0) )
	{
		default: /*skipped or 0*/   scope = SJ_SEL_URLS_SMART;      break;
		case 1:                     scope = SJ_SEL_URLS_WORKSPACE;  break;
		case 2:                     scope = SJ_SEL_URLS_DISPLAY;    break;
	}
	g_mainFrame->m_columnMixer.GetSelectedUrls(selUrls, scope);
	RETURN_ARRAY_STRING( selUrls );
}


IMPLEMENT_FUNCTION(program, getMusicSels)
{
	wxArrayString musicSels;
	if( g_advSearchModule )
	{
		long i, iCount = g_advSearchModule->GetSearchCount();
		for( i = 0; i < iCount; i++ )
			musicSels.Add(g_advSearchModule->GetSearchByIndex(i, false).GetName());
	}
	RETURN_ARRAY_STRING( musicSels );
}


IMPLEMENT_FUNCTION(program, run)
{
	if( !::wxExecute(ARG_STRING(0), wxEXEC_ASYNC) )
		SEE_error_throw(interpr_, interpr_->Error, "cannot run");
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(program, launchBrowser)
{
	g_tools->ExploreUrl(ARG_STRING(0));
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(program, iniRead)
{
	wxString ret;
	if( !ARG_IS_STRING(0) )
	{
		RETURN_UNDEFINED;
		return;
	}

	wxString                key = ARG_STRING(0);
	wxConfigBase::EntryType keyType = g_tools->m_config->GetEntryType(key);
	double                  valDouble;
	switch( keyType )
	{
		case wxConfigBase::Type_Boolean:
			RETURN_BOOL( g_tools->m_config->Read(key, ARG_BOOL(1)) );
			break;

		case wxConfigBase::Type_Integer:
			RETURN_LONG( g_tools->m_config->Read(key, ARG_LONG(1)) );
			break;

		case wxConfigBase::Type_Float:
			valDouble = ARG_DOUBLE(1);
			g_tools->m_config->Read(key, &valDouble);
			RETURN_DOUBLE( valDouble );
			break;

		default:
			RETURN_STRING( g_tools->m_config->Read(key, ARG_STRING(1)) );
			break;
	}
}


IMPLEMENT_FUNCTION(program, iniWrite)
{
	if( !ARG_IS_STRING(0) )
	{
		RETURN_UNDEFINED;
		return;
	}

	wxString                key = ARG_STRING(0);
	wxConfigBase::EntryType keyType = g_tools->m_config->GetEntryType(key);
	if( ( keyType == wxConfigBase::Type_Integer || keyType == wxConfigBase::Type_Boolean || keyType == wxConfigBase::Type_Unknown )
	 && ( ARG_IS_BOOL(1) || ARG_STRING(1) == wxString::Format(wxT("%i"), (int)ARG_LONG(1)) ) )
	{
		// keep/write long
		g_tools->m_config->Write(key, ARG_LONG(1));
	}
	else
	{
		// convert to string
		g_tools->m_config->Write(key, ARG_STRING(1));
	}

	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(program, setDisplayMsg)
{
	g_mainFrame->SetDisplayMsg(ARG_STRING(0), ARG_LONG_OR_DEF(1, 10000));
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(program, setSkinText)
{
	g_mainFrame->SetSkinText(ARG_STRING(0), ARG_STRING(1));
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(program, refreshWindows)
{
	if( g_mainFrame )
	{
		// what to update (this is a bitfield, bit #0: update display, bit #1: update workspace,
		// if this parameter is skipped, update the display for historical reasons)
		long what = 0x01;
		if( argc_ >= 1 ) what = ARG_LONG(0);

		//  do the update
		if( what & 0x01 )
		{
			g_mainFrame->UpdateDisplay();
		}

		if( what & 0x02 )
		{
			g_mainFrame->m_browser->ReloadColumnMixer();
		}

		g_mainFrame->Refresh();
	}
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(program, shutdown)
{
	SjMainApp::DoShutdownEtc((SjShutdownEtc)ARG_LONG(0));
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(program, gc)
{
	SjGcDoCleanup();
	RETURN_UNDEFINED;
}


/*******************************************************************************
 * Properties
 ******************************************************************************/


IMPLEMENT_HASPROPERTY(program)
{
	if(
	    VAL_PROPERTY( version )
	 || VAL_PROPERTY( os )
	 || VAL_PROPERTY( locale )
	 || VAL_PROPERTY( loaded )
	 || VAL_PROPERTY( zoom )
	 || VAL_PROPERTY( viewMode )
	 || VAL_PROPERTY( listModeColumns )
	 || VAL_PROPERTY( listModeOrder )
	 || VAL_PROPERTY( memory )
	 || VAL_PROPERTY( memoryPeak )
	 || VAL_PROPERTY( hwnd )
	 || VAL_PROPERTY( onLoad )
	 || VAL_PROPERTY( onUnload )
	 || VAL_PROPERTY( onKioskStarting )
	 || VAL_PROPERTY( onKioskStarted )
	 || VAL_PROPERTY( onKioskEnding )
	 || VAL_PROPERTY( onKioskEnded )
	 || VAL_PROPERTY( kioskMode )
	 || VAL_PROPERTY( visMode )
	 || VAL_PROPERTY( sleepMode )
	 || VAL_PROPERTY( sleepMinutes )
	 || VAL_PROPERTY( autoPlay )
	 || VAL_PROPERTY( musicSel )
	 || VAL_PROPERTY( search )
	 || VAL_PROPERTY( layout )
	 || VAL_PROPERTY( lastUserInput )
	)
	{
		RETURN_HAS;
	}
	else
	{
		RETURN_HASNOT;
	}
}



IMPLEMENT_GET(program)
{
	program_object* po = (program_object*)HOST_DATA->m_program;

	if( VAL_PROPERTY( loaded ) )
	{
		RETURN_BOOL( !g_mainFrame->InConstruction() );
	}
	else if( VAL_PROPERTY( version ) )
	{
		RETURN_LONG( ((SJ_VERSION_MAJOR<<24)|(SJ_VERSION_MINOR<<16)|(SJ_VERSION_REVISION<<8)) );
	}
	else if( VAL_PROPERTY( os ) )
	{
		#if defined(__WXMSW__)
			RETURN_STRING(wxT("win"));
		#elif defined(__WXMAC__)
			RETURN_STRING(wxT("mac"));
		#elif defined(__WXGTK__)
			RETURN_STRING(wxT("gtk"));
		#else
			#error Your OS here!
		#endif
	}
	else if( VAL_PROPERTY( locale ) )
	{
		RETURN_STRING( g_mainFrame->m_locale.GetCanonicalName() );
	}
	else if( VAL_PROPERTY( zoom ) )
	{
		RETURN_LONG( g_mainFrame->GetZoom() );
	}
	else if( VAL_PROPERTY( viewMode ) )
	{
		RETURN_LONG( g_mainFrame->m_browser->GetView() );
	}
	else if( VAL_PROPERTY( listModeColumns ) )
	{
		SjListBrowser* listBrowser = g_mainFrame->m_browser->GetListBrowser();
		RETURN_ARRAY_LONG( listBrowser->GetColumns() );
	}
	else if( VAL_PROPERTY( listModeOrder ) )
	{
		SjListBrowser* listBrowser = g_mainFrame->m_browser->GetListBrowser();
		long sortField; bool desc;
		listBrowser->GetOrder(sortField, desc);
		if( desc ) sortField = sortField*-1;
		RETURN_LONG( sortField );
	}
	else if( VAL_PROPERTY( memory ) )
	{
		RETURN_LONG( g_gc_system.curSize );
	}
	else if( VAL_PROPERTY( memoryPeak ) )
	{
		RETURN_LONG( g_gc_system.peakSize );
	}
	else if( VAL_PROPERTY( hwnd ) )
	{
		RETURN_LONG( (long)g_mainFrame->GetHandle() );
	}
	else if( VAL_PROPERTY( onLoad ) )
	{
		if( po->m_onLoad ) RETURN_OBJECT(po->m_onLoad); else RETURN_UNDEFINED;
	}
	else if( VAL_PROPERTY( onUnload ) )
	{
		if( po->m_onUnload ) RETURN_OBJECT(po->m_onUnload); else RETURN_UNDEFINED;
	}
	// kiosk
	else if( VAL_PROPERTY( onKioskStarting ) )
	{
		if( po->m_onKioskStarting ) RETURN_OBJECT(po->m_onKioskStarting); else RETURN_UNDEFINED;
	}
	else if( VAL_PROPERTY( onKioskStarted ) )
	{
		if( po->m_onKioskStarted ) RETURN_OBJECT(po->m_onKioskStarted); else RETURN_UNDEFINED;
	}
	else if( VAL_PROPERTY( onKioskEnding ) )
	{
		if( po->m_onKioskEnding ) RETURN_OBJECT(po->m_onKioskEnding); else RETURN_UNDEFINED;
	}
	else if( VAL_PROPERTY( onKioskEnded ) )
	{
		if( po->m_onKioskEnded ) RETURN_OBJECT(po->m_onKioskEnded); else RETURN_UNDEFINED;
	}
	else if( VAL_PROPERTY( kioskMode ) )
	{
		RETURN_BOOL( g_mainFrame->IsKioskStarted() );
	}
	// get vis. mode
	else if( VAL_PROPERTY( visMode ) )
	{
		if( g_visModule )
			RETURN_BOOL(  g_visModule->IsVisStarted() );
		else
			RETURN_FALSE;
	}
	// get sleep mode
	else if( VAL_PROPERTY( sleepMode ) || VAL_PROPERTY( sleepMinutes ) )
	{
		bool enabled, doFade;
		SjShutdownEtc action;
		long timemode, minutes, fadeSeconds;
		g_mainFrame->m_autoCtrl.GetSleepSettings(enabled, action, timemode, minutes, doFade, fadeSeconds);
		if( VAL_PROPERTY( sleepMode ) )
		{
			RETURN_BOOL( enabled );
		}
		else if( VAL_PROPERTY( sleepMinutes )
		         && timemode!=SJ_SLEEPMODE_TIMEMODE_AT && timemode!=SJ_SLEEPMODE_TIMEMODE_ALWAYS_AT )
		{
			RETURN_LONG( minutes );
		}
		else
		{
			RETURN_UNDEFINED;
		}
	}
	// get auto play
	else if( VAL_PROPERTY( autoPlay ) )
	{
		bool enabled;
		enabled = (g_mainFrame->m_autoCtrl.m_flags&SJ_AUTOCTRL_AUTOPLAY_ENABLED) != 0;
		RETURN_BOOL( enabled );
	}
	// search
	else if( VAL_PROPERTY( search ) )
	{
		if( g_advSearchModule )
		{
			const SjSearch* search = g_mainFrame->GetSearch();
			if( search->m_simple.IsSet() )
			{
				RETURN_STRING(search->m_simple.GetWords());
				return;
			}

		}
		RETURN_UNDEFINED;
	}
	// music sel
	else if( VAL_PROPERTY( musicSel ) )
	{
		if( g_advSearchModule )
		{
			const SjSearch* search = g_mainFrame->GetSearch();
			if( search->m_adv.IsSet() )
			{
				RETURN_STRING(search->m_adv.GetName());
				return;
			}

		}
		RETURN_UNDEFINED;
	}
	// get the current layout
	else if( VAL_PROPERTY( layout ) )
	{
		if( g_mainFrame )
		{
			SjSkinLayout* currLayout = g_mainFrame->GetLayout();
			if( currLayout )
			{
				RETURN_STRING( currLayout->GetName() );
				return;
			}
		}
		RETURN_UNDEFINED;
	}
	// get the last user input timestamp
	else if( VAL_PROPERTY( lastUserInput ) )
	{
		if( g_mainFrame )
		{
			RETURN_LONG( SjTimestampDiff(g_mainFrame->LastInputFromUser(), SjTools::GetMsTicks()) );
		}
		else
		{
			RETURN_LONG( 0 );
		}
	}
	else
	{
		RETURN_GET_DEFAULTS;
	}
}


IMPLEMENT_PUT(program)
{
	program_object* po = (program_object*)HOST_DATA->m_program;

	if( VAL_PROPERTY( zoom ) )
	{
		g_mainFrame->m_browser->SetZoom_(VAL_LONG);
	}
	else if( VAL_PROPERTY( viewMode ) )
	{
		g_mainFrame->m_browser->SetView_(VAL_LONG, true, true);
	}
	else if( VAL_PROPERTY( listModeColumns ) )
	{
		SjListBrowser* listBrowser = g_mainFrame->m_browser->GetListBrowser();
		listBrowser->SetColumns(VAL_ARRAY_LONG);
	}
	else if( VAL_PROPERTY( listModeOrder ) )
	{
		SjListBrowser* listBrowser = g_mainFrame->m_browser->GetListBrowser();
		long sortField = VAL_LONG; bool desc = false;
		if( sortField < 0 ) {sortField*=-1; desc=true;}
		listBrowser->SetOrder(sortField, desc);
	}
	else if( VAL_PROPERTY( onLoad ) )
	{
		SEE_object* newCallback = VAL_CALLBACK;
		HOST_DATA->RemovePersistentObject(po->m_onLoad);
		if( g_mainFrame->InConstruction() )
		{
			HOST_DATA->AddPersistentObject(newCallback, SJ_PERSISTENT_OTHER);
			po->m_onLoad = newCallback;
		}
		else
		{
			SeeCallCallback(interpr_, newCallback, (SEE_object*)po);
		}
	}
	else if( VAL_PROPERTY( onUnload ) )
	{
		SEE_object* newCallback = VAL_CALLBACK;
		HOST_DATA->RemovePersistentObject(po->m_onUnload);
		HOST_DATA->AddPersistentObject(newCallback, SJ_PERSISTENT_OTHER); // you can also assign "undefined" as the new callback, AddPersistenObject handles this
		po->m_onUnload = newCallback;
	}
	// kiosk
	else if( VAL_PROPERTY( onKioskStarting ) )
	{
		SEE_object* newCallback = VAL_CALLBACK;
		HOST_DATA->RemovePersistentObject(po->m_onKioskStarting);
		HOST_DATA->AddPersistentObject(newCallback, SJ_PERSISTENT_OTHER); // you can also assign "undefined" as the new callback, AddPersistenObject handles this
		po->m_onKioskStarting = newCallback;
	}
	else if( VAL_PROPERTY( onKioskStarted ) )
	{
		SEE_object* newCallback = VAL_CALLBACK;
		HOST_DATA->RemovePersistentObject(po->m_onKioskStarted);
		HOST_DATA->AddPersistentObject(newCallback, SJ_PERSISTENT_OTHER); // you can also assign "undefined" as the new callback, AddPersistenObject handles this
		po->m_onKioskStarted = newCallback;
	}
	else if( VAL_PROPERTY( onKioskEnding ) )
	{
		SEE_object* newCallback = VAL_CALLBACK;
		HOST_DATA->RemovePersistentObject(po->m_onKioskEnding);
		HOST_DATA->AddPersistentObject(newCallback, SJ_PERSISTENT_OTHER); // you can also assign "undefined" as the new callback, AddPersistenObject handles this
		po->m_onKioskEnding = newCallback;
	}
	else if( VAL_PROPERTY( onKioskEnded ) )
	{
		SEE_object* newCallback = VAL_CALLBACK;
		HOST_DATA->RemovePersistentObject(po->m_onKioskEnding);
		HOST_DATA->AddPersistentObject(newCallback, SJ_PERSISTENT_OTHER); // you can also assign "undefined" as the new callback, AddPersistenObject handles this
		po->m_onKioskEnded = newCallback;
	}
	else if( VAL_PROPERTY( kioskMode ) )
	{
		if( !g_mainFrame->InConstruction() )
		{
			if( VAL_LONG )
				g_kioskModule->StartRequest(true/*forceNoQueryConfig*/);
			else
				g_kioskModule->ExitRequest(0, NULL, true/*forceSimpleExit*/);
		}
	}
	// set vis. mode
	else if( VAL_PROPERTY( visMode ) )
	{
		if( g_visModule )
		{
			if( VAL_BOOL )
				g_visModule->StartVis();
			else
				g_visModule->StopVis();
		}
	}
	// set sleep mode
	else if( VAL_PROPERTY( sleepMode ) || VAL_PROPERTY( sleepMinutes ) )
	{
		bool enabled, doFade, putBack = false;
		SjShutdownEtc action;
		long timemode, minutes, fadeSeconds;
		g_mainFrame->m_autoCtrl.GetSleepSettings(enabled, action, timemode, minutes, doFade, fadeSeconds);
		if( VAL_PROPERTY( sleepMode ) )
		{
			enabled = VAL_BOOL;
			putBack = true;
		}
		else if( VAL_PROPERTY( sleepMinutes )
		         && timemode!=SJ_SLEEPMODE_TIMEMODE_AT && timemode!=SJ_SLEEPMODE_TIMEMODE_ALWAYS_AT )
		{
			minutes = VAL_LONG;
			putBack = true;
		}

		if( putBack )
		{
			g_mainFrame->m_autoCtrl.SetSleepSettings(enabled, action, timemode, minutes, doFade, fadeSeconds);
			g_mainFrame->m_autoCtrl.SaveAutoCtrlSettings();
		}
	}
	// set auto play
	else if( VAL_PROPERTY( autoPlay ) )
	{
		bool enabled = VAL_BOOL;
		long old = g_mainFrame->m_autoCtrl.m_flags;
		SjTools::SetFlag(g_mainFrame->m_autoCtrl.m_flags, SJ_AUTOCTRL_AUTOPLAY_ENABLED, enabled);
		if( g_mainFrame->m_autoCtrl.m_flags != old )
			g_mainFrame->m_autoCtrl.SaveAutoCtrlSettings();
	}
	// search
	else if( VAL_PROPERTY( search ) )
	{
		if( g_advSearchModule && !g_mainFrame->InConstruction() )
		{
			wxString words = VAL_STRING;
			if( !words.IsEmpty() )
			{
				g_mainFrame->SetSearch(SJ_SETSEARCH_SETSIMPLE, words);
			}
			else
			{
				g_mainFrame->SetSearch(SJ_SETSEARCH_CLEARSIMPLE);
			}
		}
	}
	// music sel
	else if( VAL_PROPERTY( musicSel ) )
	{
		if( g_advSearchModule && !g_mainFrame->InConstruction() )
		{
			wxString name = VAL_STRING;
			if( !name.IsEmpty() )
			{
				SjAdvSearch advSearch = g_advSearchModule->GetSearchByName(name);
				g_mainFrame->SetSearch(SJ_SETSEARCH_CLEARSIMPLE|SJ_SETSEARCH_SETADV, wxT(""), &advSearch);
			}
			else
			{
				g_mainFrame->SetSearch(SJ_SETSEARCH_CLEARSIMPLE|SJ_SETSEARCH_CLEARADV);
			}
		}
	}
	// set the current layout
	else if( VAL_PROPERTY( layout ) )
	{
		if( g_mainFrame )
		{
			wxString newLayoutName = VAL_STRING;

			SjSkinLayout* currLayout = g_mainFrame->GetLayout();
			SjSkinLayout* newLayout = g_mainFrame->GetLayout(newLayoutName);

			if( currLayout != newLayout )
			{
				g_mainFrame->LoadLayout(newLayout);
			}
		}
	}
	// set the last user input timestamp
	else if( VAL_PROPERTY( lastUserInput ) )
	{
		if( g_mainFrame )
		{
			g_mainFrame->GotInputFromUser();
		}
	}
	else
	{
		DO_PUT_DEFAULTS;
	}
}


/*******************************************************************************
 * Let SEE know about this class (this part is a little more complicated)
 ******************************************************************************/


/* object class for Program.prototype and program instances */
static SEE_objectclass program_inst_class = {
	"Program",                  /* Class */
	program_get,                /* Get */
	program_put,                /* Put */
	SEE_native_canput,          /* CanPut */
	program_hasproperty,        /* HasProperty */
	SEE_native_delete,          /* Delete */
	SEE_native_defaultvalue,    /* DefaultValue */
	SEE_native_enumerator,      /* DefaultValue */
	NULL,                       /* Construct */
	NULL,                       /* Call */
	NULL                        /* HasInstance */
};


/* object class for Program constructor */
static SEE_objectclass program_constructor_class = {
	"ProgramConstructor",       /* Class */
	SEE_native_get,             /* Get */
	SEE_native_put,             /* Put */
	SEE_native_canput,          /* CanPut */
	SEE_native_hasproperty,     /* HasProperty */
	SEE_native_delete,          /* Delete */
	SEE_native_defaultvalue,    /* DefaultValue */
	SEE_native_enumerator,      /* DefaultValue */
	program_construct,          /* Construct */
	NULL                        /* Call */
};


void SjSee::Program_init()
{
	SEE_value temp;

	// Create the "Program.prototype" object, this is used as a template for the instances
	m_Program_prototype = (SEE_object *)alloc_program_object(m_interpr);

	SEE_native_init((SEE_native *)m_Program_prototype, m_interpr,
	                &program_inst_class, m_interpr->Object_prototype);

	PUT_FUNC(m_Program_prototype, program, addMenuEntry, 0);
	PUT_FUNC(m_Program_prototype, program, addConfigButton, 0);
	PUT_FUNC(m_Program_prototype, program, addSkinsButton, 0);
	PUT_FUNC(m_Program_prototype, program, addExitOption, 0);
	PUT_FUNC(m_Program_prototype, program, setTimeout, 0);
	PUT_FUNC(m_Program_prototype, program, setDisplayMsg, 0);
	PUT_FUNC(m_Program_prototype, program, setSkinText, 0);
	PUT_FUNC(m_Program_prototype, program, refreshWindows, 0);
	PUT_FUNC(m_Program_prototype, program, selectAll, 0);
	PUT_FUNC(m_Program_prototype, program, getSelection, 0);
	PUT_FUNC(m_Program_prototype, program, getMusicSels, 0);
	PUT_FUNC(m_Program_prototype, program, run, 0);
	PUT_FUNC(m_Program_prototype, program, launchBrowser, 0);
	PUT_FUNC(m_Program_prototype, program, iniRead, 0);
	PUT_FUNC(m_Program_prototype, program, iniWrite, 0);
	PUT_FUNC(m_Program_prototype, program, exportFunction, 0);
	PUT_FUNC(m_Program_prototype, program, callExported, 0);
	PUT_FUNC(m_Program_prototype, program, callPlugin, 0);
	PUT_FUNC(m_Program_prototype, program, shutdown, 0);
	PUT_FUNC(m_Program_prototype, program, gc, 0);

	// create the "Program" object
	SEE_object* Program = (SEE_object *)SEE_malloc(m_interpr, sizeof(SEE_native));

	SEE_native_init((SEE_native *)Program, m_interpr,
	                &program_constructor_class, m_interpr->Object_prototype);

	PUT_OBJECT(Program, str_prototype, m_Program_prototype)

	// now we can add the globals
	PUT_OBJECT(m_interpr->Global, str_Program,   Program);

	m_program = Program_new();
	PUT_OBJECT(m_interpr->Global, str_program,   m_program);
}


SEE_object* SjSee::Program_new()
{
	program_object* obj = alloc_program_object(m_interpr);

	SEE_native_init(&obj->m_native, m_interpr,
	                &program_inst_class, m_Program_prototype);

	return (SEE_object *)obj;
}


void SjSee::Program_receiveMsg(int msg)
{
	if( m_interprInitialized )
	{
		program_object* po = (program_object*)m_program;

		switch( msg )
		{
			case IDMODMSG_KIOSK_STARTING:
				SeeCallCallback(m_interpr, po->m_onKioskStarting, m_program);
				break;

			case IDMODMSG_KIOSK_STARTED:
				SeeCallCallback(m_interpr, po->m_onKioskStarted, m_program);
				break;

			case IDMODMSG_KIOSK_ENDING:
				SeeCallCallback(m_interpr, po->m_onKioskEnding, m_program);
				break;

			case IDMODMSG_KIOSK_ENDED:
				SeeCallCallback(m_interpr, po->m_onKioskEnded, m_program);
				break;

			case IDMODMSG__SEE_PROGRAM_LOADED:
				SeeCallCallback(m_interpr, po->m_onLoad, m_program);
				RemovePersistentObject(po->m_onLoad);
				po->m_onLoad = NULL;
				break;

			case IDMODMSG_WINDOW_BEFORE_CLOSE_HIDE_N_STOP:
				SeeCallCallback(m_interpr, po->m_onUnload, m_program);
				RemovePersistentObject(po->m_onUnload);
				po->m_onUnload = NULL;
				break;

			case IDMODMSG_TRACK_ON_AIR_CHANGED:
				Player_receiveMsg(msg);
				break;
		}
	}
}


#endif // SJ_USE_SCRIPTS

