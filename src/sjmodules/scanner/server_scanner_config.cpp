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
 * File:    server_scanner_config.cpp
 * Authors: Björn Petersen
 * Purpose: The "server scanner" module
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/scanner/server_scanner.h>
#include <sjmodules/scanner/server_scanner_config.h>


#define IDC_ENABLECHECK     (IDM_FIRSTPRIVATE+0)
#define IDC_STATICBOX       (IDM_FIRSTPRIVATE+1)
#define IDC_SERVER_NAME     (IDM_FIRSTPRIVATE+2)
#define IDC_SERVER_TYPE     (IDM_FIRSTPRIVATE+3)
#define IDC_LOGIN_NAME      (IDM_FIRSTPRIVATE+4)
#define IDC_LOGIN_PASSWORD  (IDM_FIRSTPRIVATE+5)


BEGIN_EVENT_TABLE(SjServerScannerConfigDialog, SjDialog)
	EVT_CHECKBOX    (   IDC_ENABLECHECK,    SjServerScannerConfigDialog::OnEnableCheck  )
END_EVENT_TABLE()


SjServerScannerConfigDialog::SjServerScannerConfigDialog(wxWindow* parent, const SjServerScannerSource& source)
	: SjDialog(parent, wxT(""), SJ_MODAL, SJ_RESIZEABLE_IF_POSSIBLE)
{
	m_enabledCheckBox = NULL;

	// set title
	m_isNew = FALSE;
	if( source.m_serverName.IsEmpty() )
	{
		SetTitle(_("Add an HTTP server"));
		m_isNew = TRUE;
	}
	else
	{
		SetTitle(wxString::Format(_("Options for \"%s\""), source.m_serverName.c_str()));
	}

	// create dialog
	#define CTRL_W 260

	wxBoxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);


	wxBoxSizer* sizer2 = new wxStaticBoxSizer(new wxStaticBox(this, -1, wxEmptyString), wxVERTICAL);
	sizer1->Add(sizer2, 1/*grow*/, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	// enable checkbox
	wxSizer* sizer3 = new wxBoxSizer(wxHORIZONTAL);
	sizer2->Add(sizer3, 0, wxALL, SJ_DLG_SPACE);

	sizer3->Add(SJ_DLG_SPACE*4, 2); // some space

	if( m_isNew )
	{
		sizer3->Add(new wxStaticText(this, -1, _("Use server:")), 0, wxALIGN_CENTER_VERTICAL);
	}
	else
	{
		m_enabledCheckBox = new wxCheckBox(this, IDC_ENABLECHECK, _("Use server:"));
		m_enabledCheckBox->SetValue((source.m_flags&SJ_SERVERSCANNER_ENABLED)!=0);
		sizer3->Add(m_enabledCheckBox, 0, wxALIGN_CENTER_VERTICAL);
	}

	wxFlexGridSizer* sizer3f = new wxFlexGridSizer(2, SJ_DLG_SPACE, SJ_DLG_SPACE);
	sizer3f->AddGrowableCol(1);
	sizer2->Add(sizer3f, 0, wxGROW|wxALL, SJ_DLG_SPACE);

	// server
	sizer3f->Add(new wxStaticText(this, -1, _("Server:")), 0, wxALIGN_CENTER_VERTICAL);

	m_serverNameTextCtrl = new wxTextCtrl(this, IDC_SERVER_NAME, source.m_serverName, wxDefaultPosition, wxSize(CTRL_W, -1), 0);
	sizer3f->Add(m_serverNameTextCtrl, 0, wxGROW|wxALIGN_CENTER_VERTICAL);

	// server type
	sizer3f->Add(new wxStaticText(this, -1, _("Type")+wxString(wxT(":"))), 0, wxALIGN_CENTER_VERTICAL);

	m_serverTypeChoice = new wxChoice(this, IDC_SERVER_TYPE);
	m_serverTypeChoice->Append(wxT("CSV over HTTP (beta)")/*n/t*/, (void*)SJ_SERVERSCANNER_TYPE_HTTP);
	SjDialog::SetCbSelection(m_serverTypeChoice, (long)source.m_serverType);
	sizer3f->Add(m_serverTypeChoice, 0, wxALIGN_CENTER_VERTICAL);

	// login name
	sizer3f->Add(new wxStaticText(this, -1, _("Login name:")), 0, wxALIGN_CENTER_VERTICAL);

	m_loginNameTextCtrl = new wxTextCtrl(this, IDC_LOGIN_NAME, source.m_loginName, wxDefaultPosition, wxSize(CTRL_W, -1), 0);
	sizer3f->Add(m_loginNameTextCtrl, 0, wxGROW|wxALIGN_CENTER_VERTICAL);

	// password
	sizer3f->Add(new wxStaticText(this, -1, _("Password:")), 0, wxALIGN_CENTER_VERTICAL);

	m_loginPasswordTextCtrl = new wxTextCtrl(this, IDC_LOGIN_PASSWORD, source.m_loginPassword, wxDefaultPosition, wxSize(CTRL_W, -1), wxTE_PASSWORD);
	sizer3f->Add(m_loginPasswordTextCtrl, 0, wxGROW|wxALIGN_CENTER_VERTICAL);

	// options
	sizer3f->Add(SJ_DLG_SPACE, SJ_DLG_SPACE);

	m_doUpdateCheckBox = new wxCheckBox(this, -1, _("Include server to the update process"));
	m_doUpdateCheckBox->SetValue(source.m_flags&SJ_SERVERSCANNER_DO_UPDATE? TRUE : FALSE);
	sizer3f->Add(m_doUpdateCheckBox);

	// state stuff
	if( !m_isNew )
	{
		AddStateBox(sizer1);

		wxString cfg = source.m_lastCfgFile;
		if( cfg.IsEmpty() )
		{
			cfg = _("n/a");
		}
		else if( cfg.Length() > 128 )
		{
			cfg = cfg.Left(120)+wxT("..");
		}

		AddState(_("Configuration file")+wxString(wxT(":")), cfg);
	}


	// buttons
	sizer1->Add(CreateButtons(SJ_DLG_OK_CANCEL), 0, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	// init done, center dialog
	EnableDisable();
	sizer1->SetSizeHints(this);
	CentreOnParent();

	// set accelerators
	SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_SETTINGS));
}


void SjServerScannerConfigDialog::EnableDisable()
{
	if( m_enabledCheckBox )
	{
		bool                enable = m_enabledCheckBox->GetValue();

		wxWindowList&       children = GetChildren();
		wxWindowList::Node* childNode = children.GetFirst();
		while( childNode )
		{
			wxWindow*   child = childNode->GetData();
			int         childId = child->GetId();

			if( childId != wxID_OK
			 && childId != wxID_CANCEL
			 && childId != IDC_ENABLECHECK
			 && childId != IDC_STATICBOX
			 && !wxString(child->GetClassInfo()->GetClassName()).StartsWith("wxStatic") )
			{
				child->Enable(enable);
			}

			childNode = childNode->GetNext();
		}
	}
}


bool SjServerScannerConfigDialog::GetChanges(SjServerScannerSource& source)
{
	// get the new values
	wxString            newServerName       = m_serverNameTextCtrl->GetValue();
	SjServerScannerType newServerType       = (SjServerScannerType)SjDialog::GetCbSelection(m_serverTypeChoice);
	wxString            newLoginName        = m_loginNameTextCtrl->GetValue();
	wxString            newLoginPassword    = m_loginPasswordTextCtrl->GetValue();
	long                newFlags            = source.m_flags;

	if( m_enabledCheckBox )
	{
		SjTools::SetFlag(newFlags, SJ_SERVERSCANNER_ENABLED, m_enabledCheckBox->IsChecked());
	}

	SjTools::SetFlag(newFlags, SJ_SERVERSCANNER_DO_UPDATE, m_doUpdateCheckBox->IsChecked());

	// correct the server name
	newServerName.Trim(TRUE);
	newServerName.Trim(FALSE);

	if( newServerName.Last() == '/' )
	{
		newServerName = newServerName.BeforeLast('/');
	}

	int p = newServerName.Find(wxT("://"));
	if( p != -1 )
	{
		newServerName = newServerName.Mid(p+3);
	}

	// if the server name is not set, use the old one
	if( newServerName.IsEmpty() )
	{
		newServerName = source.m_serverName;
	}

	// sth. changed?
	if( newServerName   != source.m_serverName
	 || newServerType   != source.m_serverType
	 || newLoginName    != source.m_loginName
	 || newLoginPassword!= source.m_loginPassword
	 || newFlags        != source.m_flags )
	{
		// sth. changed!
		source.m_serverName     = newServerName;
		source.m_serverType     = newServerType;
		source.m_loginName      = newLoginName;
		source.m_loginPassword  = newLoginPassword;
		source.m_flags          = newFlags;

		return TRUE;
	}

	// nothing changed
	return FALSE;
}



