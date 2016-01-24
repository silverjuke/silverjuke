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
 * File:    obj_Dialog.cpp
 * Authors: Björn Petersen
 * Purpose: The Dialog class for SEE
 *
 ******************************************************************************/


#include <sjbase/base.h>


#if SJ_USE_SCRIPTS


#include <sjtools/console.h>
#include <sjtools/msgbox.h>
#include <sjmodules/advsearch.h>
#include <sjmodules/openfiles.h>
#include <see_dom/sj_see.h>
#include <see_dom/sj_see_helpers.h>


// data used by our object
class SjApiDlg : public SjDialog
{
public:
	                        SjApiDlg                (struct dialog_object* dlo, wxWindow* parent, SjDialogMode mode);
	void                    DoReallyClose           ();
	SjDialogMode            m_mode;
	long                    m_lastButtonIndex;

private:
	struct dialog_object*   m_dlo;
	void                    OnAnyButton             (wxCommandEvent&);
	void                    OnClose                 (wxCloseEvent&) { wxCommandEvent fwd(wxEVT_COMMAND_BUTTON_CLICKED, wxID_CANCEL); OnAnyButton(fwd); }
	                        DECLARE_EVENT_TABLE     ()
};


struct dialog_object
{
	SEE_native      m_native; // MUST be first!!!

	SjSee*          m_see;
	SjDlgControls*  m_controls;
	SjApiDlg*       m_window;

#define         MAX_CONTROLS 1024
	SEE_object*     m_callbacks[MAX_CONTROLS]; // here they are handled by the garbage collection :-)
};


static void dialog_finalize_(SEE_interpreter* interpr, void* ptr, void* closure)
{
	// this function is also used as the "unexpected exit" for persistent objects;
	// as this is followed by the "normal finalizer", set all pointers to zero
	dialog_object* obj = ((dialog_object*)ptr);

	if( obj->m_window && obj->m_window->m_mode == SJ_MODELESS )
	{
		obj->m_window->Destroy();
		obj->m_window = NULL;
	}

	if( obj->m_controls )
	{
		delete obj->m_controls;
		obj->m_controls = NULL;
	}
}


static dialog_object* alloc_dialog_object(SEE_interpreter* interpr)
{
	dialog_object* dlo = (dialog_object*)SEE_malloc_finalize(interpr, sizeof(dialog_object), dialog_finalize_, NULL);
	memset(dlo, 0, sizeof(dialog_object));
	dlo->m_controls = new SjDlgControls();
	dlo->m_see = (SjSee*)interpr->host_data;
	return dlo;
}


static dialog_object* get_dialog_object(SEE_interpreter* interpr, SEE_object* o);


/*******************************************************************************
 * Constructing
 ******************************************************************************/


IMPLEMENT_FUNCTION(dialog, construct)
{
	RETURN_OBJECT( HOST_DATA->Dialog_new() );
}


IMPLEMENT_FUNCTION(dialog, addTextCtrl)
{
	dialog_object* dlo = get_dialog_object(interpr_, this_);
	dlo->m_controls->AddTextCtrl(ARG_STRING(0), ARG_STRING(1), ARG_STRING(2), wxT(""), 0);
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(dialog, addMultilineCtrl)
{
	dialog_object* dlo = get_dialog_object(interpr_, this_);
	dlo->m_controls->AddTextCtrl(ARG_STRING(0), ARG_STRING(1), ARG_STRING(2), wxT(""), wxTE_MULTILINE);
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(dialog, addPasswordCtrl)
{
	dialog_object* dlo = get_dialog_object(interpr_, this_);
	dlo->m_controls->AddTextCtrl(ARG_STRING(0), ARG_STRING(1), ARG_STRING(2), wxT(""), wxTE_PASSWORD);
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(dialog, addSelectCtrl)
{
	dialog_object* dlo = get_dialog_object(interpr_, this_);
	wxArrayString options;
	for( int o = 3; o < argc_; o++ )
		options.Add(ARG_STRING(o));
	dlo->m_controls->AddSelectCtrl(ARG_STRING(0), ARG_STRING(1), ARG_LONG(2), 0, options);
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(dialog, addCheckCtrl)
{
	dialog_object* dlo = get_dialog_object(interpr_, this_);
	dlo->m_controls->AddCheckCtrl(ARG_STRING(0), ARG_STRING(1), ARG_LONG(2), 0);
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(dialog, addStaticText)
{
	dialog_object* dlo = get_dialog_object(interpr_, this_);
	wxString id, label;
	if( argc_>=2 )
	{
		id = ARG_STRING(0);
		label = ARG_STRING(1);
	}
	else
	{
		label = ARG_STRING(0);
	}
	dlo->m_controls->AddStatic(id, label);
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(dialog, addButton)
{
	dialog_object* dlo = get_dialog_object(interpr_, this_);
	SjDlgCtrl* ctrl = dlo->m_controls->AddButton(ARG_STRING(0), ARG_STRING(1));
	if( ctrl->m_index < MAX_CONTROLS )
		dlo->m_callbacks[ctrl->m_index] = ARG_CALLBACK(2);
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(dialog, getValue)
{
	dialog_object* dlo = get_dialog_object(interpr_, this_);
	wxString value = dlo->m_controls->GetValue(dlo->m_controls->Id2Index(ARG_STRING(0)));
	RETURN_STRING( value );
}


IMPLEMENT_FUNCTION(dialog, setValue)
{
	dialog_object* dlo = get_dialog_object(interpr_, this_);
	dlo->m_controls->SetValue(dlo->m_controls->Id2Index(ARG_STRING(0)), ARG_STRING(1));
	RETURN_UNDEFINED;
}


/*******************************************************************************
 * Dialog Handling
 ******************************************************************************/


#define IDC_USER_BUTTON     (IDM_FIRSTPRIVATE+3)


BEGIN_EVENT_TABLE(SjApiDlg, SjDialog)
	EVT_BUTTON  (IDC_USER_BUTTON,       SjApiDlg::OnAnyButton       )
	EVT_BUTTON  (wxID_OK,               SjApiDlg::OnAnyButton       )
	EVT_BUTTON  (wxID_CANCEL,           SjApiDlg::OnAnyButton       )
	EVT_CLOSE   (                       SjApiDlg::OnClose           )
END_EVENT_TABLE()


SjApiDlg::SjApiDlg(dialog_object* dlo, wxWindow* parent, SjDialogMode mode)
	: SjDialog(parent, dlo->m_see->GetFineName(), mode, SJ_NEVER_RESIZEABLE)
{
	dlo->m_see->AddPersistentObject((SEE_object*)dlo, SJ_PERSISTENT_OTHER, NULL, dialog_finalize_);
	m_dlo = dlo;
	m_mode = mode;
	m_lastButtonIndex = -1;

	dlo->m_window = this;

	// if there is no "ok" button, add "ok" and "cancel"
	if( dlo->m_controls->Id2Index(wxT("ok")) == -1 )
	{
		dlo->m_controls->AddButton(wxT("ok"), wxT(""));
		if( dlo->m_controls->Id2Index(wxT("cancel")) == -1 )
			dlo->m_controls->AddButton(wxT("cancel"), wxT(""));
	}

	// create the dialog ...
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	// ... the controls
	wxSizer* sizer2 = new wxStaticBoxSizer(new wxStaticBox(this, -1, wxT("")), wxVERTICAL);
	sizer1->Add(sizer2, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	wxString defOkTitle, defCancelTitle;
	long     defButtons = dlo->m_controls->Render(this, sizer2, IDC_USER_BUTTON, &defOkTitle, &defCancelTitle);

	// ... the default buttons
	sizer1->Add(CreateButtons(defButtons, defOkTitle, defCancelTitle),
	            0, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	sizer1->SetSizeHints(this);
	CentreOnParent();
}


void SjApiDlg::OnAnyButton(wxCommandEvent& event)
{
	switch( event.GetId() )
	{
		case wxID_OK:       m_lastButtonIndex = m_dlo->m_controls->Id2Index(wxT("ok"));     break;
		case wxID_CANCEL:   m_lastButtonIndex = m_dlo->m_controls->Id2Index(wxT("cancel")); break;
		default:            { wxWindow* button = (wxWindow*)event.GetEventObject(); m_lastButtonIndex = (long)button->GetClientData(); } break;
	}

	if( m_lastButtonIndex >= 0
	 && m_lastButtonIndex < MAX_CONTROLS
	 && m_dlo->m_callbacks[m_lastButtonIndex] )
	{
		// the script part may call close() which will close the dialog then
		SeeCallCallback(m_dlo->m_see->m_interpr,
		                m_dlo->m_callbacks[m_lastButtonIndex],      // function
		                (SEE_object*)m_dlo                          // context
		               );
	}
	else
	{
		DoReallyClose();
		// this may destroy the object through the garbage collection!
	}
}


void SjApiDlg::DoReallyClose()
{
	// REALLY close the dialog
	wxASSERT( m_dlo->m_window == this );
	m_dlo->m_controls->Unrender();
	if( m_mode == SJ_MODELESS )
	{
		Hide();
		Destroy();
	}
	else
	{
		EndModal(m_lastButtonIndex);
	}
	m_dlo->m_window = NULL;
	m_dlo->m_see->RemovePersistentObject((SEE_object*)m_dlo);
}


IMPLEMENT_FUNCTION(dialog, showModal)
{
	dialog_object* dlo = get_dialog_object(interpr_, this_);

	if( dlo->m_window == NULL )
	{
		wxWindow* parent =  SjDialog::GetSuitableDlgParent();

		SJ_WINDOW_DISABLER(parent);
		SjApiDlg dlg(dlo, parent, SJ_MODAL);
		long index = dlg.ShowModal();
		wxASSERT( dlo->m_window == NULL );

		SjDlgCtrl* ctrl = dlo->m_controls->GetCtrl(index);
		if( ctrl )
		{
			RETURN_STRING( ctrl->m_id );
			return;
		}
	}

	RETURN_UNDEFINED;
}


extern SEE_objectclass dialog_constructor_class;


IMPLEMENT_FUNCTION(dialog, show)
{
	if( this_->objectclass == &dialog_constructor_class )
	{
		// use as static function
		wxString dlgName = ARG_STRING(0);
		dlgName.MakeLower();
		if( dlgName == wxT("console") )
		{
			SjLogGui::OpenManually();
		}
		else if( dlgName == wxT("settings") )
		{
			g_mainFrame->OpenSettings(ARG_STRING(1), ARG_LONG(2), ARG_LONG(3));
		}
		else if( dlgName == wxT("musicsel") )
		{
			g_advSearchModule->OpenDialog();
		}
		else if( dlgName == wxT("openfiles") )
		{
			g_openFilesModule->OpenDialog(ARG_BOOL(1));
		}
		else if( dlgName == wxT("saveplaylist") )
		{
			g_mainFrame->m_player.m_queue.SaveAsDlg(g_mainFrame);
		}
	}
	else
	{
		// use as instance function
		dialog_object* dlo = get_dialog_object(interpr_, this_);

		if( dlo->m_window == NULL )
		{
			dlo->m_window = new SjApiDlg(dlo, g_mainFrame, SJ_MODELESS);
			dlo->m_window->Show();
		}

		dlo->m_window->Raise();
	}
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(dialog, close)
{
	dialog_object* dlo = get_dialog_object(interpr_, this_);

	if( dlo->m_window )
	{
		dlo->m_window->DoReallyClose();
	}

	RETURN_UNDEFINED;
}


/*******************************************************************************
 * Globals
 ******************************************************************************/


IMPLEMENT_FUNCTION(dialog, alert)
{
	wxWindow* parent = SjDialog::GetSuitableDlgParent();

	SjMessageBox(ARG_STRING(0), HOST_DATA->GetFineName(), wxOK, parent);

	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(dialog, confirm)
{
	wxWindow* parent = SjDialog::GetSuitableDlgParent();

	bool ret = SjMessageBox(ARG_STRING(0), HOST_DATA->GetFineName(), wxYES_NO, parent)==wxYES;

	RETURN_BOOL( ret );
}


IMPLEMENT_FUNCTION(dialog, prompt)
{
	wxWindow* parent = SjDialog::GetSuitableDlgParent();

	SJ_WINDOW_DISABLER(parent);
	wxTextEntryDialog textEntry(parent, ARG_STRING(0), HOST_DATA->GetFineName(), ARG_STRING(1));

	if( textEntry.ShowModal() == wxID_OK )
		RETURN_STRING( textEntry.GetValue() );
	else
		RETURN_FALSE;
}


IMPLEMENT_FUNCTION(dialog, fileSel)
{
	wxWindow* parent = SjDialog::GetSuitableDlgParent();

	SJ_WINDOW_DISABLER(parent);
	wxFileDialog fileDlg(parent,
	                     HOST_DATA->GetFineName(ARG_STRING(0)),
	                     wxT(""), ARG_STRING(1),
	                     wxString(_("All files")) + wxT(" (*.*)|*.*"),
	                     wxFD_CHANGE_DIR | (ARG_LONG(2)? wxFD_SAVE : wxFD_OPEN));

	if( fileDlg.ShowModal() == wxID_OK )
		RETURN_STRING( fileDlg.GetPath() );
	else
		RETURN_FALSE;
}


IMPLEMENT_FUNCTION(dialog, logError)
{
	wxString msg = ARG_STRING(0);
	wxLogError(wxT("%s [%s]"), msg.c_str(), HOST_DATA->m_executionScope.c_str());
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(dialog, logWarning)
{
	wxString msg = ARG_STRING(0);
	wxLogWarning(wxT("%s [%s]"), msg.c_str(), HOST_DATA->m_executionScope.c_str());
	RETURN_UNDEFINED;
}


IMPLEMENT_FUNCTION(dialog, print)
{
	wxString msg = ARG_STRING(0);
	wxLogInfo(wxT("%s [%s]"), msg.c_str(), HOST_DATA->m_executionScope.c_str());
	RETURN_UNDEFINED;
}


/*******************************************************************************
 * Let SEE know about this class (this part is a little more complicated)
 ******************************************************************************/


/* object class for Dialog.prototype and dialog instances */
static SEE_objectclass dialog_inst_class = {
	"Dialog",                   /* Class */
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


/* object class for Dialog constructor */
SEE_objectclass dialog_constructor_class = {
	"DialogConstructor",        /* Class */
	SEE_native_get,             /* Get */
	SEE_native_put,             /* Put */
	SEE_native_canput,          /* CanPut */
	SEE_native_hasproperty,     /* HasProperty */
	SEE_native_delete,          /* Delete */
	SEE_native_defaultvalue,    /* DefaultValue */
	SEE_native_enumerator,      /* DefaultValue */
	dialog_construct,           /* Construct */
	NULL                        /* Call */
};


void SjSee::Dialog_init()
{
	SEE_value temp;

	// Create the "Dialog.prototype" object, this is used as a template for the instances
	m_Dialog_prototype = (SEE_object *)alloc_dialog_object(m_interpr);

	SEE_native_init((SEE_native *)m_Dialog_prototype, m_interpr,
	                &dialog_inst_class, m_interpr->Object_prototype);

	PUT_FUNC(m_Dialog_prototype, dialog, addTextCtrl, 0);
	PUT_FUNC(m_Dialog_prototype, dialog, addMultilineCtrl, 0);
	PUT_FUNC(m_Dialog_prototype, dialog, addPasswordCtrl, 0);
	PUT_FUNC(m_Dialog_prototype, dialog, addSelectCtrl, 0);
	PUT_FUNC(m_Dialog_prototype, dialog, addCheckCtrl, 0);
	PUT_FUNC(m_Dialog_prototype, dialog, addStaticText, 0);
	PUT_FUNC(m_Dialog_prototype, dialog, addButton, 0);
	PUT_FUNC(m_Dialog_prototype, dialog, getValue, 0);
	PUT_FUNC(m_Dialog_prototype, dialog, setValue, 0);
	PUT_FUNC(m_Dialog_prototype, dialog, showModal, 0);
	PUT_FUNC(m_Dialog_prototype, dialog, show, 0); // as instance function
	PUT_FUNC(m_Dialog_prototype, dialog, close, 0);

	// create the "Dialog" object
	SEE_object* Dialog = (SEE_object *)SEE_malloc(m_interpr, sizeof(SEE_native));

	SEE_native_init((SEE_native *)Dialog, m_interpr,
	                &dialog_constructor_class, m_interpr->Object_prototype);

	PUT_OBJECT(Dialog, str_prototype, m_Dialog_prototype)
	PUT_FUNC(Dialog, dialog, show, 0); // as static function

	// now we can add the globals
	PUT_OBJECT(m_interpr->Global, str_Dialog,    Dialog);
	PUT_FUNC(m_interpr->Global, dialog, alert, 0);
	PUT_FUNC(m_interpr->Global, dialog, confirm, 0);
	PUT_FUNC(m_interpr->Global, dialog, prompt, 0);
	PUT_FUNC(m_interpr->Global, dialog, fileSel, 0);
	PUT_FUNC(m_interpr->Global, dialog, logError, 0);
	PUT_FUNC(m_interpr->Global, dialog, logWarning, 0);
	PUT_FUNC(m_interpr->Global, dialog, print, 0);
}


SEE_object* SjSee::Dialog_new()
{
	dialog_object* obj = alloc_dialog_object(m_interpr);

	SEE_native_init(&obj->m_native, m_interpr,
	                &dialog_inst_class, m_Dialog_prototype);

	return (SEE_object *)obj;
}


static dialog_object* get_dialog_object(SEE_interpreter* interpr, SEE_object* o)
{
	if( o->objectclass != &dialog_inst_class )
		SEE_error_throw(interpr, interpr->TypeError, NULL);

	return (dialog_object *)o;
}


#endif // SJ_USE_SCRIPTS
