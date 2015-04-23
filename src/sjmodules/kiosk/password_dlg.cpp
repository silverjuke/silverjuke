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
 * File:    password_dlg.cpp
 * Authors: Björn Petersen
 * Purpose: The password-entry dialog for the kiosk mode
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/kiosk/password_dlg.h>
#include <see_dom/sj_see.h>

#define IDC_PASSWORD_INPUT      (IDM_FIRSTPRIVATE+1)
#define IDC_CLOSE_TIMER         (IDM_FIRSTPRIVATE+2)


BEGIN_EVENT_TABLE(SjPasswordDlg, SjDialog)
	EVT_TEXT    (IDC_PASSWORD_INPUT,SjPasswordDlg::OnPasswordInput  )
	EVT_TIMER   (IDC_CLOSE_TIMER,   SjPasswordDlg::OnCloseByTimer   )
END_EVENT_TABLE()


SjPasswordDlg::SjPasswordDlg(wxWindow*          parent,
                             long               flags,
                             const wxString&    title,
                             const wxString&    passwordHint,
                             const wxString&    correctPassword1,
                             const wxString&    correctPassword2)
	:   SjDialog(parent, title, SJ_MODAL, SJ_NEVER_RESIZEABLE),
	    m_timeoutWatcher(5*60*1000/*close after 5 minutes*/)
{
	m_correctPassword1 = correctPassword1;
	m_correctPassword2 = correctPassword2;
	m_okClicked = FALSE;
	m_passwordOk = 0;
	m_passwordTextCtrl = NULL;
	m_passwordAction = NULL;
	m_flags = flags;

	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	wxSizer* sizer2 = new wxStaticBoxSizer(new wxStaticBox(this, -1, wxEmptyString), wxVERTICAL);
	sizer1->Add(sizer2, 0, wxGROW|wxALL, SJ_DLG_SPACE);

	if( flags & SJ_PASSWORDDLG_ASK_FOR_PASSWORD && !passwordHint.IsEmpty() )
	{
		sizer2->Add(new wxStaticText(this, -1, passwordHint), 0, wxALL, SJ_DLG_SPACE);
	}

	wxFlexGridSizer* sizer3 = new wxFlexGridSizer(2, SJ_DLG_SPACE, SJ_DLG_SPACE);
	sizer3->AddGrowableCol(1);
	sizer2->Add(sizer3, 0, wxGROW|wxALL, SJ_DLG_SPACE);

#define CTRL_W 180
	if( flags & SJ_PASSWORDDLG_ASK_FOR_PASSWORD )
	{
		sizer3->Add(new wxStaticText(this, -1, _("Password:")), 0, wxALIGN_CENTER_VERTICAL);

		m_passwordTextCtrl = new SjVirtKeybdTextCtrl(this, IDC_PASSWORD_INPUT, wxT(""), wxDefaultPosition, wxSize(CTRL_W, -1), wxTE_PASSWORD);
		sizer3->Add(m_passwordTextCtrl, 0, wxGROW|wxALIGN_CENTER_VERTICAL);
	}

	if( flags & SJ_PASSWORDDLG_ASK_FOR_ACTION )
	{
		sizer3->Add(new wxStaticText(this, -1, _("Action:")), 0, wxALIGN_CENTER_VERTICAL);

		m_passwordAction = CreateExitActionChoice(this,
		                   (SjShutdownEtc)g_tools->m_config->Read(wxT("kiosk/exitAction"), (long)SJ_SHUTDOWN_EXIT_KIOSK_MODE), FALSE);
		sizer3->Add(m_passwordAction, 0, wxGROW|wxALIGN_CENTER_VERTICAL);
	}

	sizer1->Add(CreateButtons(SJ_DLG_OK_CANCEL), 0, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	sizer1->SetSizeHints(this);
	CentreOnParent();

	if( flags & SJ_PASSWORDDLG_AUTOCLOSE )
	{
		m_closeTimer.SetOwner(this, IDC_CLOSE_TIMER);
		m_closeTimer.Start(1000, wxTIMER_CONTINUOUS);
	}
}


void SjPasswordDlg::InitExitActionChoice(wxChoice* c)
{
	c->Append(wxString::Format(_("Exit %s"), SJ_PROGRAM_NAME), (void*)SJ_SHUTDOWN_EXIT_SILVERJUKE);
	c->Append(_("Shutdown computer"), (void*)SJ_SHUTDOWN_POWEROFF_COMPUTER);
	c->Append(_("Reboot computer"), (void*)SJ_SHUTDOWN_REBOOT_COMPUTER);

	#if SJ_USE_SCRIPTS
	wxArrayString arr = SjSee::GetGlobalEmbeddings(SJ_PERSISTENT_EXIT_OPTION);
	for( int i = 0; i < (int)arr.GetCount(); i++ )
		c->Append(arr[i], (void*)((uintptr_t)SJ_SHUTDOWN_USER_DEF_SCRIPT+i));
	#endif
}


wxChoice* SjPasswordDlg::CreateExitActionChoice(wxWindow* parent, SjShutdownEtc sel, bool addAsk)
{
	wxChoice* c = new wxChoice(parent, -1);

	c->Append(_("Exit kiosk mode"), (void*)SJ_SHUTDOWN_EXIT_KIOSK_MODE);
	InitExitActionChoice(c);
	if( addAsk )
		c->Append(_("Ask"), (void*)SJ_SHUTDOWN_ASK_FOR_ACTION);

	SetCbSelection(c, sel);
	SetCbWidth(c, CTRL_W);

	return c;
}


bool SjPasswordDlg::TransferDataFromWindow()
{
	m_okClicked = TRUE;
	if( m_passwordTextCtrl )
	{
		wxString enteredPassword = m_passwordTextCtrl->GetValue();
		if( m_flags&SJ_PASSWORDDLG_ACCEPT_ANY_PASSWORD )
		{
			m_passwordOk = 1;;
		}
		else if( m_correctPassword1 == enteredPassword )
		{
			m_passwordOk = 1;
		}
		else if( m_correctPassword2 == enteredPassword && !m_correctPassword2.IsEmpty() )
		{
			m_passwordOk = 2;
		}
		else
		{
			SetTitle(_("Invalid password. Please try again.")); // do not use wxMessageBox() which seems to force a context-switch and makes other programs available in kiosk mode!
			m_passwordTextCtrl->SetValue(wxT(""));
			m_passwordTextCtrl->SetFocus();
			return FALSE; // keep dialog open
		}
	}

	return TRUE; // close dialog
}


void SjPasswordDlg::OnCloseByTimer(wxTimerEvent&)
{
	if( m_timeoutWatcher.IsExpired() )
	{
		m_timeoutWatcher.Clear();
		EndModal(wxID_CANCEL);
	}
}


SjShutdownEtc SjPasswordDlg::GetExitAction() const
{
	SjShutdownEtc what = SJ_SHUTDOWN_EXIT_KIOSK_MODE;
	if( m_passwordAction )
	{
		what = (SjShutdownEtc)GetCbSelection(m_passwordAction);
		g_tools->m_config->Write(wxT("kiosk/exitAction"), (long)what);
	}
	return what;
}
