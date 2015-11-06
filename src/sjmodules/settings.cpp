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
 * File:    settings.cpp
 * Authors: Björn Petersen
 * Purpose: The settings module
 *
 *******************************************************************************
 *
 * Note: There is a Bug in the MSW (other?) radio-button implementation of
 * wxToolBar in wxWidgets 2.4.2 (previously enabled tools stay enabled if a
 * new tool gets enabled), therefore I use check-buttons to simulate
 * radio-buttons.
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/notebook.h>
#include <sjbase/browser.h>
#include <sjmodules/settings.h>


/*******************************************************************************
 * SjSettingsDlg
 ******************************************************************************/


// don't use IDs with IDM_FIRSTPRIVATE here as they will conflict
// with the loaded pages!


class SjSettingsDlg : public SjDialog
{
public:
	// Constructor and destructor
	                SjSettingsDlg       (SjSettingsModule* settingsModule, wxWindow* parent);
	                ~SjSettingsDlg      (void);

	// for async opening / goto page
	wxString        m_asyncModule;
	int             m_asyncModuleIndex;
	int             m_asyncPageIndex;

private:
	// the list of modules
	wxToolBar*      m_toolBar;

	void            SetSelToDialog      (const wxString& selFile, int selIndex, int selPage);
	SjCommonModule* GetSelFromDialog    ();

	// the current page
	void            LoadPage            (const wxString& file, int index, int page);
	void            UnloadPage          ();
	long            GetNotebookPage     ();

	wxSizer         *m_dialogSizer,
	                *m_pageSizer,
	                *m_buttonSizer;

	SjCommonModule* m_currPageModule;
	wxWindow*       m_currPageWindow;
	wxString        m_currPageFile;
	int             m_currPageIndex;

	// events
	void            OnSelectionChange   (wxCommandEvent&);


	// common
	SjSettingsModule* m_settingsModule;
	bool            m_trackSelectionChanges;
	void            OnCancel            (wxCommandEvent&);
	void            OnOK                (wxCommandEvent&);
	void            OnClose             (wxCloseEvent&);
	void            OnOpenSettingsAsync (wxCommandEvent&);
	                DECLARE_EVENT_TABLE ();
	friend class    SjSettingsModule;
};


BEGIN_EVENT_TABLE(SjSettingsDlg, SjDialog)
	EVT_TOOL_RANGE              (1000, 2000,            SjSettingsDlg::OnSelectionChange    )
	EVT_BUTTON                  (wxID_OK,               SjSettingsDlg::OnOK                 )
	EVT_BUTTON                  (wxID_CANCEL,           SjSettingsDlg::OnCancel             )
	EVT_CLOSE                   (                       SjSettingsDlg::OnClose              )
	EVT_MENU                    (IDO_OPENSETTINGSASYNC, SjSettingsDlg::OnOpenSettingsAsync  )
END_EVENT_TABLE()


static SjSettingsModule* g_settingsModule = NULL;
static SjSettingsDlg*    g_settingsDlg = NULL;


/*******************************************************************************
 * SjSettingsDlg - Dialog Creation
 ******************************************************************************/


SjSettingsDlg::SjSettingsDlg(SjSettingsModule* settingsModule, wxWindow* parent)
	: SjDialog(parent, settingsModule->m_name, SJ_MODELESS, SJ_RESIZEABLE_IF_POSSIBLE)
{
	wxBusyCursor busyCursor;

	g_settingsDlg       = this;
	m_settingsModule    = settingsModule;

	m_currPageModule    = NULL;
	m_currPageWindow    = NULL;
	m_currPageIndex     = 0;

	m_trackSelectionChanges = FALSE;

	// set accelerators
	SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_SETTINGS));

	// create dialog
	m_dialogSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_dialogSizer);

	// toolbar
	m_pageSizer = new wxBoxSizer(wxVERTICAL);
	m_dialogSizer->Add(m_pageSizer, 1, wxGROW);

	m_toolBar = new wxToolBar(this, IDC_MODLIST, wxDefaultPosition, wxDefaultSize,
	                          wxTB_HORIZONTAL | wxNO_BORDER | wxTB_FLAT | wxTB_TEXT | wxTB_NODIVIDER);
	long LABEL_MAXCHAR = 20;
	m_toolBar->SetToolBitmapSize(wxSize(32, 32));
	if( g_accelModule->m_flags&SJ_ACCEL_USE_VIEW_FONT_IN_DLG )
	{
		m_toolBar->SetFont(g_mainFrame->m_baseStdFont);
		LABEL_MAXCHAR = 12;
	}

	// go through all modules
	int currModuleIndex = 0;
	SjModuleList* moduleList = g_mainFrame->m_moduleSystem.GetModules(SJ_MODULETYPE_COMMON);
	SjModuleList::Node* moduleNode = moduleList->GetFirst();
	wxString label;
	while( moduleNode )
	{
		SjCommonModule* module = (SjCommonModule*)moduleNode->GetData();
		if( module->EmbedTo() == SJ_EMBED_TO_MAIN )
		{
			label = (long)module->m_name.Len()>LABEL_MAXCHAR? (module->m_name.Left(LABEL_MAXCHAR-2)+wxT("..")) : module->m_name;

			m_toolBar->AddCheckTool(1000+currModuleIndex, label, g_tools->GetIconBitmap(module->GetGuiIcon(), TRUE), wxNullBitmap, wxT(""), wxT(""), (wxObject*)module);

			currModuleIndex++;
		}

		// next
		moduleNode = moduleNode->GetNext();
	}

	m_toolBar->Realize();

	m_pageSizer->Add(m_toolBar, 0, wxGROW);

	m_buttonSizer = SjDialog::CreateButtons(this, SJ_DLG_OK_CANCEL);
	m_dialogSizer->Add(m_buttonSizer,
	                   0, wxGROW|wxLEFT|wxRIGHT, SJ_DLG_SPACE*2);

	m_dialogSizer->Add(SJ_DLG_SPACE, SJ_DLG_SPACE);

	m_dialogSizer->SetSizeHints(this);

	m_trackSelectionChanges = TRUE;
}


SjSettingsDlg::~SjSettingsDlg(void)
{
	if( g_mainFrame )
	{
		g_mainFrame->m_moduleSystem.BroadcastMsg(IDMODMSG_SETTINGS_CLOSE);
	}
	g_settingsDlg = NULL;
}


/*******************************************************************************
 *  SjSettingsDlg - Loading and unloading pages
 ******************************************************************************/


void SjSettingsDlg::LoadPage(const wxString& file, int index, int page__)
{
	if( file != m_currPageFile || index != m_currPageIndex )
	{
		#ifdef __WXMSW__
		Freeze();
		#endif

		// remove old page
		if( m_currPageModule )
		{
			m_currPageModule->m_notebookPage = GetNotebookPage();
			m_currPageModule->DoneConfigPage(m_currPageWindow, SJ_CONFIGPAGE_DONE_GENERIC);
			m_currPageModule->Unload();
			m_currPageModule = NULL;
		}

		if( m_currPageWindow )
		{
			delete m_currPageWindow; // wxWindow::RemoveChild() is called automatically
			m_currPageWindow = NULL;
		}

		// add new page
		m_currPageModule = (SjCommonModule*)g_mainFrame->m_moduleSystem.FindModuleByFile(file, index);
		if( m_currPageModule )
		{
			if( m_currPageModule->Load() )
			{
				m_currPageWindow = m_currPageModule->GetConfigPage(this, page__==-1? m_currPageModule->m_notebookPage : page__);
				if( m_currPageWindow )
				{
					wxString title(m_settingsModule->m_name);
					title.Append(wxT(" - "));
					title.Append(m_currPageModule->m_name);
					SetTitle(title);

					m_pageSizer->Add(m_currPageWindow, 1, wxGROW|wxALL, SJ_DLG_SPACE);
					m_pageSizer->Layout();

					// calculate the border size
					wxSize clientSize = GetClientSize();
					wxSize windowSize = GetSize();
					wxSize borderSize;
					borderSize.x = windowSize.x - clientSize.x;
					borderSize.y = windowSize.y - clientSize.y;

					// enlarge the window, if needed
					wxSize toolbarSize = m_toolBar->GetEffectiveMinSize();
					wxSize minSize = m_dialogSizer->CalcMin();
					if( minSize.x < toolbarSize.x )
						minSize.x = toolbarSize.x;

					if( minSize.x > clientSize.x
					 || minSize.y > clientSize.y )
					{
						clientSize.x = clientSize.x > minSize.x? clientSize.x : minSize.x;
						clientSize.y = clientSize.y > minSize.y? clientSize.y : minSize.y;
						SetClientSize(clientSize);
					}
					SetSizeHints(minSize.x+borderSize.x, minSize.y+borderSize.y);

					// set button texts
					wxString okText, cancelText;
					bool okBold;
					m_currPageModule->GetConfigButtons(okText, cancelText, okBold);

					wxButton* okButton = (wxButton*)FindWindow(wxID_OK);
					if( okButton )
					{
						okButton->SetLabel(okText);

						wxFont font = GetFont();
						if( okBold ) font.SetWeight(wxBOLD);
						okButton->SetFont(font);
					}

					wxButton* cancelButton = (wxButton*)FindWindow(wxID_CANCEL);
					if( cancelButton )
					{
						cancelButton->SetLabel(cancelText);
					}

					// add a size event explicitly, the pages rely on this and it is not always send by wxWidgets (send on 2.x, not send on 3.x)
					// note, that this event should be send _after_ the controls are layouted; normally, this should be the case here.
					wxSizeEvent e;
					m_currPageWindow->GetEventHandler()->AddPendingEvent(e);
				}
				else
				{
					m_currPageModule->Unload();
					m_currPageModule = NULL;
				}
			}
			else
			{
				m_currPageModule = NULL;
			}
		}

		#ifdef __WXMSW__
		Thaw();
		#endif

		Refresh();
	}
}


void SjSettingsDlg::UnloadPage()
{
	SjModule* selModule = GetSelFromDialog();
	if( selModule )
	{
		g_tools->m_config->Write(wxT("settings/selModule"), selModule->PackFileName());
		g_tools->m_config->Write(wxT("settings/selPage"), GetNotebookPage());
	}

	g_settingsModule->m_dlgPos.Save(this);

	if( m_currPageModule )
	{
		m_currPageModule->Unload();
		m_currPageModule = NULL;
	}
}


long SjSettingsDlg::GetNotebookPage()
{
	wxNotebook* notebook = (wxNotebook*)FindWindow(IDC_NOTEBOOK);
	if( notebook )
	{
		return notebook->GetSelection();
	}
	return 0;
}


void SjSettingsDlg::SetSelToDialog(const wxString& selFile, int selIndex, int selPage)
{
	m_trackSelectionChanges = FALSE;

	//wxBusyCursor busy;

	bool anythingSelected = false;
	for( int i = 0; i < (int)m_toolBar->GetToolsCount(); i++ )
	{
		SjModule* m = (SjModule*)m_toolBar->GetToolClientData(1000+i);
		if( m )
		{
			bool select = ( m->m_file.CmpNoCase(selFile)==0 && m->m_fileIndex==selIndex );
			if( select ) { anythingSelected = true; }
			m_toolBar->ToggleTool(1000+i, select);
		}
	}

	if( !anythingSelected )
	{
		m_toolBar->ToggleTool(1000+0, true);
		selPage = 0;
	}

	m_trackSelectionChanges = TRUE;

	// update page
	SjModule* m = GetSelFromDialog();
	if( m )
	{
		LoadPage(m->m_file, m->m_fileIndex, selPage);
	}
}


SjCommonModule* SjSettingsDlg::GetSelFromDialog()
{
	int i;
	for( i = 0; i < (int)m_toolBar->GetToolsCount(); i++ )
	{
		if( m_toolBar->GetToolState(1000+i) )
		{
			return (SjCommonModule*)m_toolBar->GetToolClientData(1000+i);
		}
	}

	return NULL;
}


/*******************************************************************************
 * SjSettingsDlg - Events
 ******************************************************************************/


void SjSettingsDlg::OnSelectionChange(wxCommandEvent& event)
{
	static bool s_inHere = FALSE;
	if( !s_inHere )
	{
		s_inHere = TRUE;
		if( m_trackSelectionChanges )
		{
			int i;
			for( i = 0; i < (int)m_toolBar->GetToolsCount(); i++ )
			{
				m_toolBar->ToggleTool(1000+i, (i==event.GetId()-1000));
			}

			SjModule* m = GetSelFromDialog();
			if( m )
			{
				LoadPage(m->m_file, m->m_fileIndex, -1);
			}
		}
		s_inHere = FALSE;
	}
}


void SjSettingsDlg::OnCancel(wxCommandEvent&)
{
	if( m_currPageWindow && m_currPageModule )
	{
		m_currPageModule->DoneConfigPage(m_currPageWindow, SJ_CONFIGPAGE_DONE_CANCEL_CLICKED);
	}

	UnloadPage();
	Destroy(); // do not used delete!
}


void SjSettingsDlg::OnOK(wxCommandEvent&)
{
	if( m_currPageWindow && m_currPageModule )
	{
		m_currPageModule->DoneConfigPage(m_currPageWindow, SJ_CONFIGPAGE_DONE_OK_CLICKED);
	}

	UnloadPage();
	Destroy(); // do not used delete!
}



void SjSettingsDlg::OnClose(wxCloseEvent&)
{
	wxCommandEvent fwd;
	OnCancel(fwd);
}


/*******************************************************************************
 * SjSettingsModule
 ******************************************************************************/


SjSettingsModule::SjSettingsModule(SjInterfaceBase* interf)
	: SjCommonModule(interf),
	  m_dlgPos(wxT("settings/pos"))
{
	m_file              = wxT("memory:settings.lib");
	m_name              = _("Settings");
}


bool SjSettingsModule::FirstLoad()
{
	g_settingsModule = this;
	return TRUE;
}


void SjSettingsModule::LastUnload()
{
	if( g_settingsDlg )
	{
		g_settingsDlg->Destroy();
	}
	g_settingsModule = NULL;
}


void SjSettingsModule::OpenSettings(const wxString& selFile__, int selIndex__, int selPage__)
{
	wxASSERT(g_settingsModule);

	// create dialog
	bool dlgCreatedHere = false;
	if( !g_settingsDlg )
	{
		g_settingsDlg = new SjSettingsDlg(g_settingsModule, g_mainFrame);
		dlgCreatedHere = true;
	}

	// select correct module and page
	if( !selFile__.IsEmpty() || dlgCreatedHere )
	{
		wxString selFile(selFile__);
		int      selIndex = selIndex__;
		int      selPage = selPage__;

		if( selFile.IsEmpty() )
		{
			SjModule::UnpackFileName(g_tools->m_config->Read(wxT("settings/selModule"), wxT("memory:mymusic.lib")), selFile, selIndex);
			selPage = g_tools->m_config->Read(wxT("settings/selPage"), 0L);
		}

		g_settingsDlg->SetSelToDialog(selFile, selIndex, selPage);
	}


	// show dialog
	if( dlgCreatedHere )
	{
		g_settingsModule->m_dlgPos.Restore(g_settingsDlg);
		g_settingsDlg->Show();
	}

	// set focus to dialog
	g_settingsDlg->Raise();
	g_settingsDlg->SetFocus();

	// set focus to the loaded page (there seems to be a bug in the wxWindows
	// automatic focus initialisation - without the following lines
	// the focus is on the help-button!?
	if( g_settingsDlg->m_currPageWindow )
	{
		g_settingsDlg->m_currPageWindow->SetFocus();
	}
}


bool SjSettingsModule::IsDialogOpen()
{
	return (g_settingsDlg!=NULL);
}


void SjSettingsModule::RaiseIfOpen()
{
	if( g_settingsDlg )
	{
		g_settingsDlg->SetFocus();
	}
}


void SjSettingsModule::ReceiveMsg(int msg)
{
	if( g_settingsDlg )
	{
		if( msg == IDMODMSG_KIOSK_STARTING
		 || msg == IDMODMSG_KIOSK_ENDED )
		{
			// Close() will call Cancel() for the page which may get into an infinite loop if this
			// function is used in the kiosk module somehow. So take care!
			g_settingsDlg->Close();
		}
	}
}


void SjSettingsModule::OpenSettingsAsync(const wxString& module, int moduleIndex, int pageIndex)
{
	// this function should only be called by a settings page to switch to another page
	wxASSERT( g_settingsDlg );
	wxASSERT( wxThread::IsMain() );

	if( g_settingsDlg )
	{
		wxCommandEvent fwd(wxEVT_COMMAND_MENU_SELECTED, IDO_OPENSETTINGSASYNC);
		g_settingsDlg->m_asyncModule = module;
		g_settingsDlg->m_asyncModuleIndex = moduleIndex;
		g_settingsDlg->m_asyncPageIndex = pageIndex;
		g_settingsDlg->GetEventHandler()->AddPendingEvent(fwd);
	}
}
void SjSettingsDlg::OnOpenSettingsAsync(wxCommandEvent& e)
{
	SetSelToDialog(m_asyncModule, m_asyncModuleIndex, m_asyncPageIndex);
}
