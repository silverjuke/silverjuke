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
#include <sjmodules/cinterface.h>
#include <sjbase/sj_api.h>
#include <see_dom/sj_see.h>


#if SJ_USE_C_INTERFACE


/*******************************************************************************
 * SjCPlugin
 ******************************************************************************/


extern "C"
{
	long CALLBACK SjCPlugin__CallMaster__(struct SjInterface* cinterf, UINT msg, LPARAM param1, LPARAM param2, LPARAM param3)
	{
		if( cinterf && cinterf->rsvd )
		{
			return ((SjCPlugin*)cinterf->rsvd)->CallMaster(msg, param1, param2, param3);
		}
		return 0;
	}
}


long SjCPlugin::CallMaster(UINT msg, LPARAM param1, LPARAM param2, LPARAM param3)
{
	wxASSERT( msg >= 20000 && msg <= 29999 );

	if( g_mainFrame == NULL || g_tools == NULL )
		return 0;

	// thread-safe mesages
	switch( msg )
	{
		case SJ_GET_VERSION:
			return SjTools::VersionString2Long(wxString::Format(wxT("%i.%i"), SJ_VERSION_MAJOR, SJ_VERSION_MINOR));
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


LPARAM SjCPlugin::CallPlugin(UINT msg, LPARAM param1, LPARAM param2, LPARAM param3)
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
	m_cinterf->rsvd         = (LPARAM)this;
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


LPARAM SjCPlugin::EncodeString(const wxString& str)
{
	m_returnStringStack++;
	if( m_returnStringStack >= ENCODE_MAX_STACK )
		m_returnStringStack = 0;

	m_returnString[m_returnStringStack].clear();
	m_returnString[m_returnStringStack].appendString(str, SJ_UTF8);
	m_returnString[m_returnStringStack].appendChar((unsigned char)0, 1); // append null-byte
	return (LPARAM)m_returnString[m_returnStringStack].getReadableData();
}


wxString SjCPlugin::DecodeString(LPARAM ptr)
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
void SjCInterface::LoadModulesFastHack(SjModuleList& list, const wxArrayString& possibleDlls)
{
	size_t                  i, iCount = possibleDlls.GetCount();
	wxDynamicLibrary*       dynlib;
	dllMainEntryFuncType    entryPoint;
	SjInterface*            cinterf;
	for( i = 0; i < iCount; i++ )
	{
		// load a DLL
		{
			wxLogNull null;

			dynlib = new wxDynamicLibrary(possibleDlls[i]);
			if( dynlib == NULL )
				continue; // nothing to log - this is no SjDll

			if( !dynlib->IsLoaded() )
			{
				delete dynlib;
				continue; // nothing to log - this is no SjDll
			}

			entryPoint = (dllMainEntryFuncType)dynlib->GetSymbol(wxT("SjGetInterface"));
			if( entryPoint == NULL )
			{
				delete dynlib;
				continue; // nothing to log - this is no SjDll
			}
		}

		wxLogInfo(wxT("Loading %s"), possibleDlls.Item(i).c_str());

		cinterf = entryPoint();
		if( cinterf == NULL || cinterf->CallPlugin == NULL )
		{
			wxLogError(wxT("SjGetInterface returns 0 or CallPlugin set to 0."));
			wxLogError(_("Cannot open \"%s\"."), possibleDlls.Item(i).c_str());
			delete dynlib;
			continue; // error
		}

		// success so far - create the plugin and add it to the list
		list.Append(new SjCPlugin(this, possibleDlls[i], dynlib, cinterf));
	}
}


#endif // SJ_USE_C_INTERFACE
