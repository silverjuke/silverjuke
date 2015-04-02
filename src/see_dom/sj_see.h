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
 * File:    sj_see.h
 * Authors: Björn Petersen
 * Purpose: Our Interpreter
 *
 ******************************************************************************/



#ifndef __SJ_SEE_H__
#define __SJ_SEE_H__

#if SJ_USE_SCRIPTS

struct SEE_object;
struct SEE_interpreter;
struct SEE_value;
class SjCPlugin;



enum SjSeePersistent
{
    SJ_PERSISTENT_OTHER
    ,   SJ_PERSISTENT_MENU_ENTRY
    ,   SJ_PERSISTENT_CONFIG_BUTTON
    ,   SJ_PERSISTENT_SKINS_BUTTON
    ,   SJ_PERSISTENT_EXIT_OPTION
    ,   SJ_PERSISTENT_EXPORT
};



typedef void (*SJ_UNEXPECTED_UNPERSISTENT_PROC)(SEE_interpreter* interpr, void* ptr, void* userData2);



struct persistent_object
{
	persistent_object*              m_next;
	SEE_object*                     m_object;
	SjSeePersistent                 m_scope;
	SEE_value*                      m_param2;
	SJ_UNEXPECTED_UNPERSISTENT_PROC m_unexpectedUnpersistent;
};




// our little timer class - internal use only!
class SjProgramTimer : public wxTimer
{
public:
	SjProgramTimer(SEE_interpreter* interpr, struct program_object* po);
	void Notify();
	SEE_interpreter*        interpr_;
	struct program_object*  m_po;
	bool                    m_inTimer;
};



class SjSee
{
public:
	// constructor / destructor
	SjSee                   ();
	~SjSee                  ();

	// very simple interface to execute a script
	void                    SetExecutionScope       (const wxString& scope) {m_executionScope=scope;}
	bool                    Execute                 (const wxString& script);
	bool                    ExecuteAsFunction       (const wxString& script);
	bool                    IsResultDefined         ();
	wxString                GetResultString         ();
	double                  GetResultDouble         ();
	long                    GetResultLong           ();

	// process events and callbacks
	// ----------------------------

	bool                    HasGlobalEmbeddings     (SjSeePersistent) const;
	static wxArrayString    GetGlobalEmbeddings     (SjSeePersistent);
	static bool             OnGlobalEmbedding       (SjSeePersistent, int index, SjSee* doDefaultAction=NULL);
	static void             ReceiveMsg              (int msg);

	// from here on, public only to SEE!
	// ---------------------------------

	// the interpreter object
	SEE_interpreter*        m_interpr;
	bool                    m_interprInitialized;

	// persistent objects are objects that are not necessarily global
	// but may be used through the GUI - eg. event functions
	persistent_object*      m_persistentAnchor;
	void                    AddPersistentObject     (SEE_object* newObj, SjSeePersistent param1, SEE_value* param2=NULL, SJ_UNEXPECTED_UNPERSISTENT_PROC unexpectedUnpersistent=NULL);
	void                    RemovePersistentObject  (SEE_object* oldObj);


	// some predefined objects and classes
	void                    Player_init             ();
	SEE_object*             Player_new              ();
	void                    Player_receiveMsg       (int msg);
	static void             Player_onPlaybackDone   (const wxString& url);
	SEE_object*             m_Player_prototype;
	SEE_object*             m_player;

	void                    Program_init            ();
	SEE_object*             Program_new             ();
	void                    Program_receiveMsg      (int msg);
	void                    Program_getGlobalEmbed  (SjSeePersistent, wxArrayString& ret) const;
	bool                    Program_onGlobalEmbed   (SjSeePersistent, int index);
	SEE_object*             m_Program_prototype;
	SEE_object*             m_program;

	void                    Rights_init             ();
	SEE_object*             Rights_new              ();
	SEE_object*             m_Rights_prototype;

	void                    Dialog_init             ();
	SEE_object*             Dialog_new              ();
	SEE_object*             m_Dialog_prototype;

	void                    Database_init           ();
	SEE_object*             Database_new            (const wxString& name);
	SEE_object*             m_Database_prototype;

	void                    File_init               ();
	SEE_object*             File_new                (SEE_value* name, bool binary);
	SEE_object*             m_File_prototype;

	void                    HttpRequest_init        ();
	SEE_object*             HttpRequest_new         ();
	SEE_object*             m_HttpRequest_prototype;


	// only for the "script to C" bridge
	SjCPlugin*          m_plugin;

	// misc
	wxString                m_executionScope;
	SEE_value*              m_executeResult;
	SjSLHash                m_dynFunc;
	wxString                GetFineName             (const wxString& append=wxT("")) const {return GetFineName(m_executionScope, append);}
	static wxString         GetFineName             (const wxString& path, const wxString& append);

	// single-chainged list
	static SjSee*           s_first;
	SjSee*                  m_next;

	// handling the timer
	SjProgramTimer*         m_timer;
};


#endif // SJ_USE_SCRIPTS

#endif // __SJ_SEE_H__

