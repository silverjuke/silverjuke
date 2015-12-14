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
 * File:    c_interface.h
 * Authors: Björn Petersen
 * Purpose: Handling external C-Modules
 *
 ******************************************************************************/


#ifndef __SJ_C_INTERFACE_H__
#define __SJ_C_INTERFACE_H__


#include <sjbase/sj_api.h>
#include <tagger/tg_bytevector.h>


class wxDynamicLibrary;
class SjSee;
class SjCInterface;


class SjCPlugin : public SjCommonModule
{
public:
	// Constructor / Destructor
	                    SjCPlugin           (SjInterfaceBase* interf, const wxString& file, wxDynamicLibrary*, SjInterface*);
	virtual             ~SjCPlugin          ();

	// Reimplementations
	bool                FirstLoad           ();
	void                LastUnload          ();

	// public only to internal callbacks!
	SJPARAM             CallPlugin          (SJPARAM msg, SJPARAM param1=0, SJPARAM param2=0, SJPARAM param3=0);
	SJPARAM             CallMaster          (SJPARAM msg, SJPARAM param1, SJPARAM param2, SJPARAM param3);
	wxString            DecodeString        (SJPARAM ptr);
	SJPARAM             EncodeString        (const wxString&);

private:
	// private stuff
	wxDynamicLibrary*   m_dynlib;
	SjInterface*       m_cinterf;

	// state
	bool                m_initDone;
	bool                m_initError;

	// string from/to plugin (the plugin uses UTF-8)
	#define             ENCODE_MAX_STACK    8
	SjByteVector        m_returnString[ENCODE_MAX_STACK];
	long                m_returnStringStack;

	// the SEE object
	SjSee*              m_see;
};


class SjCInterface : public SjInterfaceBase
{
public:
	                SjCInterface        ();
	void            AddModulesFromFile  (SjModuleList&, const wxFileName&, bool suppressNoAccessErrors);
	void            LoadModules         (SjModuleList&);
};

extern SjCInterface* g_cInterface;


#endif // __SJ_C_INTERFACE_H__
