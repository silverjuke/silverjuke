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
 * File:    openfiles.cpp
 * Authors: Björn Petersen
 * Purpose: The open files & more dialog
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/openfiles.h>
#include <see_dom/sj_see.h>


/*******************************************************************************
 * SjOpenFilesDlg
 ******************************************************************************/


class SjOpenFilesDialog : public SjDialog
{
public:
	SjOpenFilesDialog   ();

	SjHistoryComboBox* m_inputCtrl;
	wxCheckBox*     m_appendCheck;
	wxArrayString   m_multipleFiles;

private:
	void            OnBrowse            (wxCommandEvent&);
	void            OnOK                (wxCommandEvent&);
	void            OnCancel            (wxCommandEvent&);
	void            OnClose             (wxCloseEvent&) { wxCommandEvent fwd; OnCancel(fwd); }
	DECLARE_EVENT_TABLE ()
};


#define IDC_INPUTCTRL   (IDM_FIRSTPRIVATE+4)
#define IDC_BROWSE      (IDT_OPEN_FILES) // we use the same accelerator
#define IDC_APPEND      (IDM_FIRSTPRIVATE+6)

#define MULTI_FILE_POSTFIX wxT(", ...")


BEGIN_EVENT_TABLE(SjOpenFilesDialog, SjDialog)
	EVT_BUTTON          (IDC_BROWSE,            SjOpenFilesDialog::OnBrowse     )
	EVT_MENU            (IDC_BROWSE,            SjOpenFilesDialog::OnBrowse     )
	EVT_BUTTON          (wxID_OK,               SjOpenFilesDialog::OnOK         )
	EVT_BUTTON          (wxID_CANCEL,           SjOpenFilesDialog::OnCancel     )
	EVT_CLOSE           (                       SjOpenFilesDialog::OnClose      )
END_EVENT_TABLE()


SjOpenFilesDialog::SjOpenFilesDialog()
	: SjDialog(g_mainFrame, g_openFilesModule->m_name, SJ_MODELESS, SJ_RESIZEABLE_IF_POSSIBLE)
{
	// create the dialog
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	wxSizer* sizer2 = new wxStaticBoxSizer(new wxStaticBox(this, -1, _("Please select a playlist, some files or enter a streaming URL")), wxVERTICAL);
	sizer1->Add(sizer2, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	wxSizer* sizer3 = new wxBoxSizer(wxHORIZONTAL);
	sizer2->Add(sizer3, 0, wxGROW|wxALL, SJ_DLG_SPACE);

	wxArrayString history = g_openFilesModule->m_settingsHistory;
	history.Insert(wxT(""), 0);
	m_inputCtrl = new SjHistoryComboBox(this, IDC_INPUTCTRL, 400, history);
	sizer3->Add(m_inputCtrl, 1, wxRIGHT, SJ_DLG_SPACE);

	wxButton* browseButton = new wxButton(this, IDC_BROWSE, _("Browse")+wxString(wxT("...")), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer3->Add(browseButton, 0, 0, SJ_DLG_SPACE);

	m_appendCheck = new wxCheckBox(this, IDC_APPEND, _("Append to current playlist"));
	sizer2->Add(m_appendCheck, 0, wxLEFT|wxBOTTOM|wxRIGHT, SJ_DLG_SPACE);

	sizer1->Add(CreateButtons(SJ_DLG_OK_CANCEL), 0, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	sizer1->SetSizeHints(this);
}


void SjOpenFilesDialog::OnBrowse(wxCommandEvent&)
{
	// get the files
	wxArrayString files;
	{
		wxWindowDisabler disabler(this);
		wxFileDialog dlg(this,
		                 _("Browse"),
		                 wxT(""), wxT(""),
		                 g_mainFrame->m_moduleSystem.GetAssignedExt(SJ_EXT_MUSICFILES|SJ_EXT_PLAYLISTS_READ).GetFileDlgStr(),
		                 wxFD_OPEN|wxFD_CHANGE_DIR|wxFD_MULTIPLE);
		if( dlg.ShowModal() != wxID_OK )
		{
			return;
		}

		dlg.GetPaths(files);
	}

	// see what to do
	m_multipleFiles.Clear();

	int count = files.GetCount();
	if( count == 1 )
	{
		m_inputCtrl->SetValue(files[0]);
	}
	else if( count > 1 )
	{
		m_inputCtrl->SetValue(files[0] + MULTI_FILE_POSTFIX);
		m_multipleFiles = files;
	}
}


void SjOpenFilesDialog::OnOK(wxCommandEvent&)
{
	// save settings
	g_openFilesModule->m_settingsAppend = m_appendCheck->GetValue();
	g_openFilesModule->m_settingsHistory = m_inputCtrl->GetHistory();

	g_tools->m_config->Write(wxT("openfiles/append"), g_openFilesModule->m_settingsAppend? 1L : 0L);
	g_tools->WriteArray(wxT("openfiles/history"), g_openFilesModule->m_settingsHistory);

	// get the file(s) to open
	wxArrayString files;
	{
		wxString value = m_inputCtrl->GetValue();
		value.Trim(true);
		value.Trim(false);

		if( value.Right(wxStrlen(MULTI_FILE_POSTFIX)) == MULTI_FILE_POSTFIX
		        && m_multipleFiles.GetCount() > 1 )
		{
			files = m_multipleFiles;
		}
		else if( !value.IsEmpty() )
		{
			files.Add(value);
		}
	}

	// anything given? open the file(s).
	if( !files.IsEmpty() )
	{
		g_mainFrame->OpenFiles(files, g_openFilesModule->m_settingsAppend? SJ_OPENFILES_ENQUEUE : SJ_OPENFILES_DEFCMD);
	}

	// close the dialog
	g_openFilesModule->CloseDialog();
}


void SjOpenFilesDialog::OnCancel(wxCommandEvent&)
{
	g_openFilesModule->CloseDialog();
}


/*******************************************************************************
 * SjOpenFilesModule
 ******************************************************************************/


SjOpenFilesModule* g_openFilesModule = NULL;


SjOpenFilesModule::SjOpenFilesModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	m_file                  = wxT("memory:openfiles.lib");
	m_name                  = _("Open playlist");
	m_dlg                   = NULL;
	m_settingsLoaded        = false;
}


bool SjOpenFilesModule::FirstLoad()
{
	g_openFilesModule = this;
	return TRUE;
}


void SjOpenFilesModule::LastUnload()
{
	CloseDialog();
}


void SjOpenFilesModule::OpenDialog(bool checkAppend /*if set to FALSE, the checkbox stays as it is*/)
{
	// load the settings, if not yet done
	if( !m_settingsLoaded )
	{
		m_settingsHistory = g_tools->ReadArray(wxT("openfiles/history"));
		m_settingsAppend = g_tools->m_config->Read(wxT("openfiles/append"), 0L) != 0;

		m_settingsLoaded = true;
	}

	if( checkAppend )
	{
		m_settingsAppend = true;
	}

	// open the dialog, if it is not yet opended
	if( !IsDialogOpened() )
	{
		wxASSERT( m_dlg == NULL );
		m_dlg = new SjOpenFilesDialog();

		m_dlg->CentreOnParent();

		m_dlg->Show();
	}

	// append? (do this independingly of dialog creation as the dialog is modeless
	// and the user may select "append" from the main menu while the dialog is already opened)
	m_dlg->m_appendCheck->SetValue(m_settingsAppend);

	// set focus to dialog
	m_dlg->Raise();
	m_dlg->SetFocus();
}


void SjOpenFilesModule::CloseDialog()
{
	if( m_dlg )
	{
		// hide
		m_dlg->Hide();

		// destroy
		m_dlg->Destroy();
		m_dlg = NULL;
	}
}


void SjOpenFilesModule::ReceiveMsg(int msg)
{
	if( msg == IDMODMSG_KIOSK_STARTING
	        || msg == IDMODMSG_WINDOW_CLOSE )
	{
		CloseDialog();
	}

	// hack: forwarding to SjSee ...
	#if SJ_USE_SCRIPTS
		SjSee::ReceiveMsg(msg);
	#endif
}

