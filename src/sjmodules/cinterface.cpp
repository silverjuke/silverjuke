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
 * File:    cinterface.cpp
 * Authors: Björn Petersen
 * Purpose: Handling external C-Modules
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/dynlib.h>
#include <sjmodules/cinterface.h>
#include <see_dom/sj_see.h>


/*******************************************************************************
 * SjCPlugin
 ******************************************************************************/


extern "C"
{
	SJPARAM SJCALLBACK SjCPlugin__CallMaster__(struct SjInterface* cinterf, SJPARAM msg, SJPARAM param1, SJPARAM param2, SJPARAM param3)
	{
		if( cinterf && cinterf->rsvd )
		{
			return ((SjCPlugin*)cinterf->rsvd)->CallMaster(msg, param1, param2, param3);
		}
		return 0;
	}
}


SJPARAM SjCPlugin::CallMaster(SJPARAM msg, SJPARAM param1, SJPARAM param2, SJPARAM param3)
{
	wxASSERT( msg >= 20000 && msg <= 29999 );

	if( g_mainFrame == NULL || g_tools == NULL )
		return 0;

	// thread-safe mesages
	switch( msg )
	{
		case SJ_GET_VERSION:
			return ((SJ_VERSION_MAJOR<<24)|(SJ_VERSION_MINOR<<16)|(SJ_VERSION_REVISION<<8));
	}

	if( ! wxThread::IsMain() )
		return 0;

	// no thread-save message
	switch( msg )
	{
		case SJ_EXECUTE:
			#if SJ_USE_SCRIPTS
				if( m_see == NULL )
				{
					m_see = new SjSee();
					m_see->m_plugin = this;
					m_see->SetExecutionScope(m_file);
				}

				m_see->Execute(DecodeString(param1));   // seems to work recursively ;-)
				// ExecuteAsFunction() is no good idea - this avoids to define _any_ globals!
				if( param2 )
					return EncodeString(m_see->GetResultString());
				else
					return m_see->GetResultLong();
			#else
				return 0;
			#endif
	}

	// not supported
	return 0;
}


SJPARAM SjCPlugin::CallPlugin(SJPARAM msg, SJPARAM param1, SJPARAM param2, SJPARAM param3)
{
	wxASSERT( msg >= 10000 && msg <= 19999 );

	if( !m_initError )
	{
		return m_cinterf->CallPlugin(m_cinterf, msg, param1, param2, param3);
	}

	return 0;
}


SjCPlugin::SjCPlugin(   SjInterfaceBase* interf,
                        const wxString& file,
                        wxDynamicLibrary* dynlib, SjInterface* cinterf)
	:   SjCommonModule(interf)
{
	wxASSERT( interf && dynlib && cinterf );

	// init structures
	m_returnStringStack     = 0;
	m_file                  = file;
	m_name                  = SjTools::GetFileNameFromUrl(file, NULL, true/*strip ext.*/);
	m_dynlib                = dynlib;

	m_cinterf               = cinterf;
	m_cinterf->rsvd         = (SJPARAM)this;
	m_cinterf->CallMaster   = SjCPlugin__CallMaster__;

	m_initDone              = false;
	m_initError             = false;

	m_see                   = NULL;
}


bool SjCPlugin::FirstLoad()
{
	// call PL_INIT
	m_initError = false;
	if( CallPlugin(SJ_PLUGIN_INIT) == 0 )
	{
		wxLogError(wxT("SJ_PLUGIN_INIT failed."));
		wxLogError(_("Cannot open \"%s\"."), m_file.c_str());
		m_initError = true;
		return false;
	}
	m_initDone = true;

	return true;
}


void SjCPlugin::LastUnload()
{
	// call PL_EXIT (only if PL_INIT succeeded before)
	CallPlugin(SJ_PLUGIN_EXIT);
	m_initDone = false;
	#if SJ_USE_SCRIPTS
	if( m_see )
	{
		delete m_see;
		m_see = NULL;
	}
	#endif
}


SjCPlugin::~SjCPlugin()
{
	delete m_dynlib;
}


SJPARAM SjCPlugin::EncodeString(const wxString& str)
{
	m_returnStringStack++;
	if( m_returnStringStack >= ENCODE_MAX_STACK )
		m_returnStringStack = 0;

	m_returnString[m_returnStringStack].clear();
	m_returnString[m_returnStringStack].appendString(str, SJ_UTF8);
	m_returnString[m_returnStringStack].appendChar((unsigned char)0, 1); // append null-byte
	return (SJPARAM)m_returnString[m_returnStringStack].getReadableData();
}


wxString SjCPlugin::DecodeString(SJPARAM ptr)
{
	return SjByteVector::toString(SJ_UTF8, (void*)ptr, -1);
}


/*******************************************************************************
 * SjCInterface
 ******************************************************************************/


SjCInterface* g_cInterface = NULL;


SjCInterface::SjCInterface()
	: SjInterfaceBase(wxT("C-Plugin"))
{
	g_cInterface    = this;
}


typedef SjInterface *(*dllMainEntryFuncType) (void);


void SjCInterface::AddModulesFromFile(SjModuleList& list, const wxFileName& fn, bool suppressNoAccessErrors)
{
	#if SJ_USE_C_INTERFACE
		wxDynamicLibrary*       dynlib;
		dllMainEntryFuncType    entryPoint;
		SjInterface*            cinterf;

		// rough filename check
		{
			wxString name = fn.GetName().Lower();
			if( name.StartsWith("bass")
			 || name.StartsWith("dwlgina")
			 || name.StartsWith("msvc")
			 || name.StartsWith("sjmmkeybd") )
			{
				return; // these are libraries used internally
			}
		}

		// load a DLL
		{
			wxLogNull null;

			dynlib = new wxDynamicLibrary(fn.GetFullPath());
			if( dynlib == NULL )
				return; // nothing to log - this is no valid library

			if( !dynlib->IsLoaded() )
			{
				delete dynlib;
				return; // nothing to log - this is no valid library
			}

			entryPoint = (dllMainEntryFuncType)dynlib->GetSymbol(wxT("SjGetInterface"));
			if( entryPoint == NULL )
			{
				delete dynlib;
				return; // nothing to log - this is no valid library
			}
		}

		wxLogInfo(wxT("Loading %s"), fn.GetFullPath().c_str());

		cinterf = entryPoint();
		if( cinterf == NULL || cinterf->CallPlugin == NULL )
		{
			wxLogError(wxT("SjGetInterface returns 0 or CallPlugin set to 0."));
			wxLogError(_("Cannot open \"%s\"."), fn.GetFullPath().c_str());
			delete dynlib;
			return; // error
		}

		// success so far - create the plugin and add it to the list
		list.Append(new SjCPlugin(this, fn.GetFullPath(), dynlib, cinterf));
	#endif
}


void SjCInterface::LoadModules(SjModuleList& list)
{
	// this also adds the scripts for speed reasons (otherwise we would scan the directory twice) (execteud later in mainframe.cpp)
	AddModulesFromSearchPaths(list, true);
}


