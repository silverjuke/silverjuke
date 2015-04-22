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
 * File:    kiosk.cpp
 * Authors: Björn Petersen
 * Purpose: Kiosk mode handling
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/browser.h>
#include <sjmodules/kiosk/kiosk.h>
#include <sjmodules/kiosk/virtkeybd.h>
#include <sjmodules/kiosk/monitor_overview.h>
#include <sjmodules/kiosk/password_dlg.h>
#include <sjmodules/vis/vis_module.h>
#include <sjmodules/advsearch.h>
#include <sjmodules/settings.h>
#include <see_dom/sj_see.h>
#include <wx/artprov.h>
#include <wx/display.h>
#include <wx/spinctrl.h>
#include <wx/notebook.h>


/*******************************************************************************
 * A simple, black frame
 ******************************************************************************/


class SjBlackFrame : public wxFrame
{
public:
	SjBlackFrame        (wxWindow* parent, wxWindowID id, const wxString& title,
	                     const wxPoint& pos, const wxSize& size,
	                     long style)
		: wxFrame(parent, id, title, pos, size, style)
	{
		SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_MAIN));
	}

private:
	void            OnMyClose           (wxCloseEvent& e)
	{
		if( e.CanVeto() )
			e.Veto();
	}
	void            OnEraseBackground   (wxEraseEvent& e) { }
	void            OnPaint             (wxPaintEvent& e)
	{
		wxPaintDC dc(this);
		wxSize size = GetClientSize();
		dc.SetBrush(*wxBLACK_BRUSH);
		dc.SetPen(*wxBLACK_PEN);
		dc.DrawRectangle(0, 0, size.x, size.y);
	}
	void            OnFwdToMainFrame    (wxCommandEvent& e)
	{
		g_mainFrame->ProcessEvent(e);
	}

	void            OnKeyUp             (wxKeyEvent& e)
	{
		int targetId = g_accelModule->KeyEvent2CmdId(e, SJA_MAIN);
		if( targetId )
		{
			wxCommandEvent fwd(wxEVT_COMMAND_MENU_SELECTED, targetId);
			g_mainFrame->ProcessEvent(fwd);
		}
		else
		{
			e.Skip();
		}
	}
	DECLARE_EVENT_TABLE ();
};


BEGIN_EVENT_TABLE(SjBlackFrame, wxFrame)
	EVT_CLOSE           (                       SjBlackFrame::OnMyClose             )
	EVT_ERASE_BACKGROUND(                       SjBlackFrame::OnEraseBackground     )
	EVT_PAINT           (                       SjBlackFrame::OnPaint               )
	EVT_MENU_RANGE      (IDT_FIRST, IDT_LAST,   SjBlackFrame::OnFwdToMainFrame      )
	EVT_KEY_UP          (                       SjBlackFrame::OnKeyUp               )
END_EVENT_TABLE()



/*******************************************************************************
 * SjKioskConfigPage - Common
 ******************************************************************************/


class SjKioskConfigPage : public wxPanel
{
public:
	SjKioskConfigPage
	(wxWindow* parent, int selectedPage);
	~SjKioskConfigPage  ();

private:
	// start page
	wxPanel*        CreateStartPage     (wxWindow* parent);
	wxChoice*       m_defExitActionChoice;
	void            OnMyKioskStart      (wxCommandEvent&); // even needed if the button is not present (for the hotkey)

	// function page
	wxPanel*        CreateFunctionPage  (wxWindow* parent);
	wxArrayLong     m_operationIds;
	wxArrayLong     m_operationFields;
	wxArrayLong     m_operationFlags;
	wxButton*       m_changePasswordButton;
	bool            ChoosePassword      ();
	wxChoice*       m_limitToAdvSearchChoice;
	wxButton*       m_limitToAdvSearchButton;

#define         ADD_TRUNCATE_TEXT   0x01
	wxSpinCtrl*     AddSpinCtrl         (wxSizer* sizer, long value, long min, long max, int id=-1);
	void            AddStaticText       (wxSizer* sizer, const wxString&);
	void            AddSpace            (wxSizer* sizer) { sizer->Add(SJ_DLG_SPACE, SJ_DLG_SPACE); }
	wxCheckBox*     AddOp               (wxSizer* sizer, int field, long flag, const wxString& text, long flags=0);
	void            AddOp               (wxSizer* sizer, int id, wxCheckBox**, const wxString& text, long flags=0);

	void            SetOp               (int field, long flag, bool state) { wxCheckBox* cb=FindOp(field, flag); if(cb) { cb->SetValue(state); } }
	bool            GetOp               (int field, long flag) { wxCheckBox* cb=FindOp(field, flag); if(cb) { return cb->GetValue(); } else { return FALSE; } }
	wxCheckBox*     FindOp              (int field, long flag);
	void            CheckDependencies   ();
	void            OnOpCheck           (wxCommandEvent&);
	void            OnOpDefault         (wxCommandEvent&);
	void            OnChoosePassword    (wxCommandEvent&) { ChoosePassword(); }
	void            OnLimitToMusicSelBut(wxCommandEvent&);

	// "monitor" page
	wxPanel*        CreateMonitorPage   (wxWindow* parent);
	void            InitMonitorPage     ();
	void            UpdateMonitorPage   ();
	void            UpdateMonitorResChoice(wxChoice*, size_t displayIndex, wxString& selRes);
	wxString        m_monitorInitialization;
	SjMonitorOverview*
	m_monitorOverview;
	wxChoice*       m_monitorChoice[2];
	wxChoice*       m_monitorResChoice[2];
	wxTimer         m_monitorTimer;
	void            OnMonitorTimer      (wxTimerEvent&);
	void            OnMonitorChoice     (wxCommandEvent&);
	void            OnMonitorResChoice  (wxCommandEvent&);
	void            OnMonitorSysSettings
	(wxCommandEvent&);

	// "virtual keyboard" page
	wxPanel*        CreateVirtKeybdPage (wxWindow* parent);
	void            UpdateVirtKeybdCtrls();
	wxCheckBox*     m_virtKeybdInKiosk;
	wxCheckBox*     m_virtKeybdOutsideKiosk;
	wxStaticText*   m_virtKeybdTestLabel1;
	wxStaticText*   m_virtKeybdTestLabel2;
	wxStaticText*   m_virtKeybdLayoutLabel;
	wxChoice*       m_virtKeybdLayout;
	wxStaticText*   m_virtKeybdColourLabel;
	wxChoice*       m_virtKeybdColour;
	wxSlider*       m_virtKeybdTransp;
	wxStaticText*   m_virtKeybdTranspLabel1;
	wxStaticText*   m_virtKeybdTranspLabel2;
	wxCheckBox*     m_virtKeybdHideCursor;
	SjVirtKeybdTextCtrl*
	m_virtKeybdTest;
	void            OnVirtKeybdCheck    (wxCommandEvent&);
	void            OnVirtKeybdLayout   (wxCommandEvent&);
	void            OnVirtKeybdColour   (wxCommandEvent&);
	void            OnVirtKeybdTransp   (wxScrollEvent&);
	void            OnVirtKeybdDefault  (wxCommandEvent&);
	void            UpdateVirtKeybdTranspText
	();

	// Numpad page
	wxPanel*        CreateNumpadPage    (wxWindow* parent);
	void            UpdateNumpadCtrls   ();
	void            OnNumpadUseInKioskCheck(wxCommandEvent&);
	void            OnNumpadUseOutsideKioskCheck(wxCommandEvent&);
	void            OnNumpadEditKeys    (wxCommandEvent&);
	wxCheckBox*     m_numpadUseInKioskCheckBox;
	wxCheckBox*     m_numpadUseOutsideKioskCheckBox;
	wxStaticText*   m_numpadKeysLabel;
	wxStaticText*   m_numpadKeysList;
	wxButton*       m_numpadEditKeysButton;

	// Credit page
	wxPanel*        CreateCreditPage    (wxWindow* parent);
	void            UpdateCreditCtrls   ();
	void            OnCreditUseCheck    (wxCommandEvent&);
	void            OnCreditSaveCheck   (wxCommandEvent&);
	void            OnCreditDDECheck    (wxCommandEvent&);
	void            OnCreditShortcutEditButton(wxCommandEvent&);
	void            OnCreditCurrSpinCtrl(wxSpinEvent&);
	void            OnCreditCurrText    (wxCommandEvent&) { wxSpinEvent fwd; OnCreditCurrSpinCtrl(fwd); }
	wxCheckBox*     m_creditUseCheckBox;
	wxCheckBox*     m_creditSaveCheckBox;
	wxCheckBox*     m_creditDDECheckBox;
	wxButton*   m_creditShortcutEditButton;
	wxStaticText*   m_creditCurrLabel;
	wxSpinCtrl*     m_creditCurrSpinCtrl;

	// Common
	bool            m_optionsAvailable; // if the kiosk mode is already started, the kiosk mode options are not available.
	wxNotebook*     m_notebook;
	void            OnNotebookChange    (wxNotebookEvent& event);
	wxPanel*        m_tempPanel;
	bool            m_constructorDone;
	bool            Apply               ();
	DECLARE_EVENT_TABLE ()
	friend class    SjKioskModule;
};


#define IDC_CHANGE_PASSWORD_BUTTON      (IDM_FIRSTPRIVATE+3)
#define IDC_OPERATION_FIRST             (IDM_FIRSTPRIVATE+4)   // range start
#define IDC_OPERATION_LAST              (IDM_FIRSTPRIVATE+104) // range end
#define IDC_OPERATION_DEFAULT           (IDM_FIRSTPRIVATE+105)
#define IDC_LIMITTOMUSICSELBUT          (IDM_FIRSTPRIVATE+106)
#define IDC_VIRTKEYBD_LAYOUT            (IDM_FIRSTPRIVATE+107)
#define IDC_VIRTKEYBD_COLOUR            (IDM_FIRSTPRIVATE+108)
#define IDC_VIRTKEYBD_TRANSP            (IDM_FIRSTPRIVATE+109)
#define IDC_VIRTKEYBD_INKIOSK           (IDM_FIRSTPRIVATE+110)
#define IDC_VIRTKEYBD_OUTKIOSK          (IDM_FIRSTPRIVATE+111)
#define IDC_VIRTKEYBD_HIDECURSOR        (IDM_FIRSTPRIVATE+112)
#define IDC_VIRTKEYBD_DEFAULT           (IDM_FIRSTPRIVATE+113)
#define IDC_NUMPAD_USE_IN_KIOSK         (IDM_FIRSTPRIVATE+114)
#define IDC_NUMPAD_USE_OUTSIDE_KIOSK    (IDM_FIRSTPRIVATE+115)
#define IDC_NUMPAD_EDITKEYS             (IDM_FIRSTPRIVATE+116)
#define IDC_CREDIT_USE                  (IDM_FIRSTPRIVATE+117)
#define IDC_CREDIT_DDE                  (IDM_FIRSTPRIVATE+118)
#define IDC_CREDIT_SHORTCUT_EDIT        (IDM_FIRSTPRIVATE+119)
#define IDC_CREDIT_SAVE                 (IDM_FIRSTPRIVATE+120)
#define IDC_CREDIT_CURR                 (IDM_FIRSTPRIVATE+121)
#define IDC_MONITOR_CHOICE_0            (IDM_FIRSTPRIVATE+123) // range start
#define IDC_MONITOR_CHOICE_1            (IDM_FIRSTPRIVATE+124) // range end
#define IDC_MONITOR_RES_CHOICE_0        (IDM_FIRSTPRIVATE+127) // range start
#define IDC_MONITOR_RES_CHOICE_1        (IDM_FIRSTPRIVATE+128) // range end
#define IDC_MONITOR_SYS_SETTINGS        (IDM_FIRSTPRIVATE+129)
#define IDC_MONITOR_TIMER               (IDM_FIRSTPRIVATE+130)


BEGIN_EVENT_TABLE(SjKioskConfigPage, wxPanel)
	EVT_NOTEBOOK_PAGE_CHANGED   (IDC_NOTEBOOK,              SjKioskConfigPage::OnNotebookChange             )

	EVT_BUTTON                  (IDT_TOGGLE_KIOSK,          SjKioskConfigPage::OnMyKioskStart               )
	EVT_MENU                    (IDT_TOGGLE_KIOSK,          SjKioskConfigPage::OnMyKioskStart               )
	EVT_COMMAND_RANGE           (IDC_OPERATION_FIRST,
	                             IDC_OPERATION_LAST,
	                             wxEVT_COMMAND_CHECKBOX_CLICKED,
	                             SjKioskConfigPage::OnOpCheck                    )
	EVT_BUTTON                  (IDC_OPERATION_DEFAULT,     SjKioskConfigPage::OnOpDefault                  )
	EVT_BUTTON                  (IDC_CHANGE_PASSWORD_BUTTON,SjKioskConfigPage::OnChoosePassword             )
	EVT_BUTTON                  (IDC_LIMITTOMUSICSELBUT,    SjKioskConfigPage::OnLimitToMusicSelBut         )

	EVT_CHOICE                  (IDC_MONITOR_CHOICE_0,      SjKioskConfigPage::OnMonitorChoice              )
	EVT_CHOICE                  (IDC_MONITOR_CHOICE_1,      SjKioskConfigPage::OnMonitorChoice              )
	EVT_CHOICE                  (IDC_MONITOR_RES_CHOICE_0,  SjKioskConfigPage::OnMonitorResChoice           )
	EVT_CHOICE                  (IDC_MONITOR_RES_CHOICE_1,  SjKioskConfigPage::OnMonitorResChoice           )
	EVT_MENU                    (IDC_MONITOR_SYS_SETTINGS,  SjKioskConfigPage::OnMonitorSysSettings         )
	EVT_TIMER                   (IDC_MONITOR_TIMER,         SjKioskConfigPage::OnMonitorTimer               )

	EVT_CHECKBOX                (IDC_VIRTKEYBD_INKIOSK,     SjKioskConfigPage::OnVirtKeybdCheck             )
	EVT_CHECKBOX                (IDC_VIRTKEYBD_OUTKIOSK,    SjKioskConfigPage::OnVirtKeybdCheck             )
	EVT_CHECKBOX                (IDC_VIRTKEYBD_HIDECURSOR,  SjKioskConfigPage::OnVirtKeybdCheck             )
	EVT_CHOICE                  (IDC_VIRTKEYBD_LAYOUT,      SjKioskConfigPage::OnVirtKeybdLayout            )
	EVT_CHOICE                  (IDC_VIRTKEYBD_COLOUR,      SjKioskConfigPage::OnVirtKeybdColour            )
	EVT_COMMAND_SCROLL          (IDC_VIRTKEYBD_TRANSP,      SjKioskConfigPage::OnVirtKeybdTransp            )
	EVT_BUTTON                  (IDC_VIRTKEYBD_DEFAULT,     SjKioskConfigPage::OnVirtKeybdDefault           )

	EVT_CHECKBOX                (IDC_NUMPAD_USE_IN_KIOSK,   SjKioskConfigPage::OnNumpadUseInKioskCheck      )
	EVT_CHECKBOX                (IDC_NUMPAD_USE_OUTSIDE_KIOSK,
	                             SjKioskConfigPage::OnNumpadUseOutsideKioskCheck )
	EVT_BUTTON                  (IDC_NUMPAD_EDITKEYS,       SjKioskConfigPage::OnNumpadEditKeys             )

	EVT_CHECKBOX                (IDC_CREDIT_USE,            SjKioskConfigPage::OnCreditUseCheck             )
	EVT_CHECKBOX                (IDC_CREDIT_DDE,            SjKioskConfigPage::OnCreditDDECheck             )
	EVT_BUTTON                  (IDC_CREDIT_SHORTCUT_EDIT,  SjKioskConfigPage::OnCreditShortcutEditButton   )
	EVT_CHECKBOX                (IDC_CREDIT_SAVE,           SjKioskConfigPage::OnCreditSaveCheck            )
	EVT_SPINCTRL                (IDC_CREDIT_CURR,           SjKioskConfigPage::OnCreditCurrSpinCtrl         )
	EVT_TEXT                    (IDC_CREDIT_CURR,           SjKioskConfigPage::OnCreditCurrText             )

END_EVENT_TABLE()


static SjKioskConfigPage* g_kioskConfigPage = NULL;


SjKioskConfigPage::SjKioskConfigPage(wxWindow* parent, int selectedPage)
	: wxPanel(parent)
{
	wxBusyCursor    busy;
	m_constructorDone = false,

	// save given objects
	m_creditCurrSpinCtrl = NULL;
	m_optionsAvailable = !g_kioskModule->KioskStarted();

	g_kioskModule->LoadConfig();

	// create notebook
	wxSizer* dialogSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(dialogSizer);

	m_notebook = new wxNotebook(this, IDC_NOTEBOOK, wxDefaultPosition, wxDefaultSize, 0/*wxCLIP_CHILDREN - problems with wxChoice/wxComboBox*/);

	wxNotebook* notebookSizer = m_notebook;

#define PAGE_START 0
	m_notebook->AddPage(CreateStartPage(m_notebook),  _("Start kiosk mode"));

#define PAGE_FUNCTIONS 1
	m_notebook->AddPage(CreateFunctionPage(m_notebook),  _("Functionality"));

#define PAGE_MONITOR 2
	m_notebook->AddPage(CreateMonitorPage(m_notebook),  _("Monitors"));

#define PAGE_VIRTKEYBD 3
	m_notebook->AddPage(CreateVirtKeybdPage(m_notebook),  _("Virtual keyboard"));

#define PAGE_NUMPAD 4
	m_notebook->AddPage(CreateNumpadPage(m_notebook),  _("Numpad"));

#define PAGE_CREDIT 5
	m_notebook->AddPage(CreateCreditPage(m_notebook),  _("Credit system"));

	CheckDependencies();

	if( selectedPage<0 || selectedPage >= (int)m_notebook->GetPageCount() )
		selectedPage = 0;

	if( selectedPage == PAGE_MONITOR )
	{
		InitMonitorPage();
	}

	m_notebook->SetSelection(selectedPage);

	dialogSizer->Add(notebookSizer, 1, wxALL|wxGROW, SJ_DLG_SPACE);

	// set accelerators
	SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_KIOSKSETTINGS));

	// done
	dialogSizer->SetSizeHints(this);
	g_kioskConfigPage = this;
	m_constructorDone = true;
}


SjKioskConfigPage::~SjKioskConfigPage()
{
	g_kioskConfigPage = NULL;
}


void SjKioskConfigPage::OnNotebookChange(wxNotebookEvent& event)
{
	if( m_constructorDone )
	{
		if( event.GetSelection() == PAGE_MONITOR )
		{
			InitMonitorPage();
		}
	}
}


bool SjKioskConfigPage::Apply()
{
	bool ret = TRUE;

	if(  g_kioskModule // may be NULL on exit if the close event comes delayed ...
	        && !g_kioskModule->KioskStarted() )
	{
		// init flags flags
		g_kioskModule->m_configOp       = SJ_OP_KIOSKON;
		g_kioskModule->m_configKioskf   &= (SJ_KIOSKF_MAX_TRACKS_IN_QUEUE|SJ_KIOSKF_NO_DBL_TRACKS); // set all to 0 beside these two flags; they're are changed elsewhere

		// get operation flags
		int i, iCount = m_operationIds.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			wxCheckBox* checkBox = (wxCheckBox*)FindWindow(m_operationIds[i]);
			wxASSERT( checkBox );
			if( checkBox /*test anyway*/ && checkBox->GetValue() )
			{
				if( m_operationFields[i] == 1 )
				{
					g_kioskModule->m_configOp |= m_operationFlags[i];
				}
				else
				{
					g_kioskModule->m_configKioskf |= m_operationFlags[i];
				}
			}
		}

		// make sure either album or list view is enabled
		if( (g_kioskModule->m_configOp&(SJ_OP_ALBUM_VIEW|SJ_OP_COVER_VIEW|SJ_OP_LIST_VIEW)) == 0 )
			g_kioskModule->m_configOp |= SJ_OP_ALBUM_VIEW;

		// get the exit action
		g_kioskModule->m_configDefExitAction = (SjShutdownEtc)SjDialog::GetCbSelection(m_defExitActionChoice);

		// get the advanced search that should be used to limit the user's choice
		{
			int sel = m_limitToAdvSearchChoice->GetSelection();
			if( sel != -1 )
			{
				g_kioskModule->m_configLimitToAdvSearch = (long)m_limitToAdvSearchChoice->GetClientData(sel);
			}
		}

		// save config
		g_kioskModule->SaveConfig();
	}

	// done
	return ret;
}


/*******************************************************************************
 * SjKioskConfigPage - Start Page
 ******************************************************************************/


wxPanel* SjKioskConfigPage::CreateStartPage(wxWindow* parent)
{
	m_tempPanel = new wxPanel(parent, -1);
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	m_tempPanel->SetSizer(sizer1);

	sizer1->Add(1, SJ_DLG_SPACE); // some space

	wxStaticText* staticText = new wxStaticText(m_tempPanel, -1,
	        _("Silverjuke provides a so called \"kiosk mode\" which allows you to run the program\nfull screen with a defined functionality."));
	staticText->Enable(m_optionsAvailable);
	sizer1->Add(staticText, 0, wxALL, SJ_DLG_SPACE);

	staticText = new wxStaticText(m_tempPanel, -1,
	                              _("Before you enter the kiosk mode by clicking on \"Start\", make sure, you know how\nto exit again:"));
	staticText->Enable(m_optionsAvailable);
	staticText->SetForegroundColour(wxColour(0xEE, 0x1A, 0x24));
	sizer1->Add(staticText, 0, wxALL, SJ_DLG_SPACE);

	wxSizer* sizer2f = new wxFlexGridSizer(1, SJ_DLG_SPACE/2, SJ_DLG_SPACE*3);
	sizer1->Add(sizer2f, 0, wxLEFT|wxTOP|wxRIGHT, SJ_DLG_SPACE);

	wxString startKey = g_accelModule->GetReadableShortcutsByCmd(IDT_TOGGLE_KIOSK, SJ_SHORTCUTS_LOCAL|SJ_SHORTCUTS_SYSTEMWIDE);
	AddOp(sizer2f, 2, SJ_KIOSKF_EXIT_KEY, wxString::Format(_("Exit by hitting %s"), startKey.c_str()));

	AddOp(sizer2f, 2, SJ_KIOSKF_EXIT_CORNER, _("Exit by clicking into two different corners"));

	wxString disableKey;
#ifdef __WXMAC__
	disableKey = SjAccelModule::GetReadableShortcutByKey(wxACCEL_CTRL, WXK_TAB, true) + wxT(" etc.");
#else
	disableKey = SjAccelModule::GetReadableShortcutByKey(wxACCEL_ALT, WXK_TAB);
#endif
	AddOp(sizer2f, 2, SJ_KIOSKF_DISABLE_AT, wxString::Format(_("Disable %s"), disableKey.c_str()));

	disableKey = SjAccelModule::GetReadableShortcutByKey(wxACCEL_ALT|wxACCEL_CTRL, WXK_DELETE);
	AddOp(sizer2f, 2, SJ_KIOSKF_DISABLE_CAD, wxString::Format(_("Disable %s"), disableKey.c_str())
#ifdef __WXDEBUG__
	      + wxT(" (not in debug-mode)")
#endif
	     );

	AddOp(sizer2f, 2, SJ_KIOSKF_DISABLE_SHUTDOWN, _("Disable shutdown"));

	wxBoxSizer* sizer3 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer3, 0, wxLEFT|wxTOP|wxRIGHT, SJ_DLG_SPACE);

	AddOp(sizer3, 2, SJ_KIOSKF_USE_PASSWORD, _("Ask for a password on exit"));

	m_changePasswordButton = new wxButton(m_tempPanel, IDC_CHANGE_PASSWORD_BUTTON, _("Change password..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	m_changePasswordButton->Enable((g_kioskModule->m_configKioskf&SJ_KIOSKF_USE_PASSWORD)? true : false);
	sizer3->Add(m_changePasswordButton, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE/2);

	sizer3 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer3, 0, wxALL, SJ_DLG_SPACE);

	AddStaticText(sizer3, _("Exit action:"));

	m_defExitActionChoice = SjPasswordDlg::CreateExitActionChoice(m_tempPanel, g_kioskModule->m_configDefExitAction, TRUE);
	m_defExitActionChoice->Enable(m_optionsAvailable);
	sizer3->Add(m_defExitActionChoice, 0, wxLEFT, SJ_DLG_SPACE);

	return m_tempPanel;
}


void SjKioskConfigPage::OnMyKioskStart(wxCommandEvent&)
{
	if( Apply() )
	{
		// Older versions of Silverjuke just called SjTools::FindTopLevel(this)->Close()
		// (=g_settingsDlg->Close()) here - however, this was no good idea as the settings
		// dialog was NOT closed if the kiosk mode was started eg. using the menu or a key
		// in the main window...
		//
		// The settings dialog is now closed in SjSettingsModule::ReceiveMsg() when receiving
		// IDMODMSG_BEFORE_KIOSK_START or IDMODMSG_AFTER_KIOSK_END.  However, as the page also
		// belongs to the settings dialog, we must not use CancelConfigPage() here, or, if we
		// do, we have to make sure not to enter an infinite loop!!!

		if( g_kioskModule->KioskStarted() )
		{
			g_kioskModule->ExitRequest(0);
		}
		else
		{
			g_kioskModule->DoStart();
		}
	}
}


/*******************************************************************************
 * SjKioskConfigPage - Function Page
 ******************************************************************************/


wxPanel* SjKioskConfigPage::CreateFunctionPage(wxWindow* parent)
{
	m_tempPanel = new wxPanel(parent, -1);
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	m_tempPanel->SetSizer(sizer1);

	sizer1->Add(1, SJ_DLG_SPACE*2); // some space

	wxStaticText* staticText = new wxStaticText(m_tempPanel, -1,
	        _("Start the kiosk mode with the following functionality:"));
	staticText->Enable(m_optionsAvailable);
	sizer1->Add(staticText, 0, wxLEFT|wxBOTTOM|wxRIGHT, SJ_DLG_SPACE);

	// common settings

	wxSizer* sizer2f = new wxFlexGridSizer(3, SJ_DLG_SPACE/2, SJ_DLG_SPACE*3);
	sizer1->Add(sizer2f, 0, wxALL, SJ_DLG_SPACE);


	AddOp(sizer2f, 1, SJ_OP_ALBUM_VIEW,         _("Album view"), ADD_TRUNCATE_TEXT);
	AddOp(sizer2f, 1, SJ_OP_SEARCH,                                 _("Search"), ADD_TRUNCATE_TEXT);
	AddOp(sizer2f, 1, SJ_OP_PLAYPAUSE,                                                  _("Play/pause"), ADD_TRUNCATE_TEXT);


	AddOp(sizer2f, 1, SJ_OP_COVER_VIEW,         _("Cover view"), ADD_TRUNCATE_TEXT);
	AddOp(sizer2f, 1, SJ_OP_MUSIC_SEL,                              _("Search for music selection"), ADD_TRUNCATE_TEXT);
	AddOp(sizer2f, 1, SJ_OP_EDIT_QUEUE,                                                 _("Edit queue"), ADD_TRUNCATE_TEXT);

	AddOp(sizer2f, 1, SJ_OP_LIST_VIEW,          _("List view"), ADD_TRUNCATE_TEXT);
	AddOp(sizer2f, 1, SJ_OP_MUSIC_SEL_GENRE,                        _("Search for genre"), ADD_TRUNCATE_TEXT);
	AddOp(sizer2f, 1, SJ_OP_REPEAT,                                                     _("Repeat playlist"), ADD_TRUNCATE_TEXT);

	AddOp(sizer2f, 1, SJ_OP_TOGGLE_ELEMENTS,    _("Toggle elements"), ADD_TRUNCATE_TEXT);
	AddOp(sizer2f, 1, SJ_OP_MAIN_VOL,                               _("Volume"), ADD_TRUNCATE_TEXT);
	AddOp(sizer2f, 1, SJ_OP_UNQUEUE,                                                    _("Unqueue tracks"), ADD_TRUNCATE_TEXT);


	AddOp(sizer2f, 1, SJ_OP_TOGGLE_TIME_MODE,   _("Toggle time mode"), ADD_TRUNCATE_TEXT);
	AddOp(sizer2f, 1, SJ_OP_MULTI_ENQUEUE,                          _("Enqueue multiple tracks"), ADD_TRUNCATE_TEXT);
	AddOp(sizer2f, 1, SJ_OP_ZOOM,                                                       _("Zoom in")+wxString(wxT("/"))+_("Zoom out"), ADD_TRUNCATE_TEXT);

	AddOp(sizer2f, 1, SJ_OP_STARTVIS,                               _("Open video screen"), ADD_TRUNCATE_TEXT);
	AddOp(sizer2f, 1, SJ_OP_ALL,                                                        _("All functions"), ADD_TRUNCATE_TEXT);

	sizer1->Add(SJ_DLG_SPACE/2, SJ_DLG_SPACE/2); // some space

	// adv. search

	wxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2, 0, wxLEFT|wxRIGHT, SJ_DLG_SPACE);

	wxString optionText = _("Limit tracks to the music selection %s");

	AddOp(sizer2, 2, SJ_KIOSKF_LIMIT_TO_ADV_SEARCH, optionText.BeforeFirst('%').Trim());

	m_limitToAdvSearchChoice = new wxChoice(m_tempPanel, -1, wxDefaultPosition, wxSize(-1, -1));
	sizer2->Add(m_limitToAdvSearchChoice, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);
	sizer2->Add(SJ_DLG_SPACE/2, SJ_DLG_SPACE);

	SjAdvSearchModule::UpdateAdvSearchChoice(m_limitToAdvSearchChoice, g_kioskModule->m_configLimitToAdvSearch);
	SjDialog::SetCbWidth(m_limitToAdvSearchChoice, 100);

	m_limitToAdvSearchButton = new wxButton(m_tempPanel, IDC_LIMITTOMUSICSELBUT, wxT("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer2->Add(m_limitToAdvSearchButton, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);

	AddStaticText(sizer2, optionText.AfterFirst('%').Mid(1).Trim(FALSE));

	sizer1->Add(SJ_DLG_SPACE/2, SJ_DLG_SPACE/2); // some space

	// shuffle - SJ_OP_SHUFFLE is included in SJ_OP_EDIT_QUEUE

	sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2, 0, wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	AddOp(sizer2, 2, SJ_KIOSKF_SHUFFLE, _("Shuffle"));

	// reset button

	wxButton* resetButton = new wxButton(m_tempPanel, IDC_OPERATION_DEFAULT, _("Reset to default values"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	resetButton->Enable(m_optionsAvailable);
	sizer1->Add(resetButton, 0, wxALL, SJ_DLG_SPACE);

	return m_tempPanel;
}


wxCheckBox* SjKioskConfigPage::AddOp(wxSizer* sizer, int field, long flag, const wxString& text, long textFlags)
{
	int     id = IDC_OPERATION_FIRST+m_operationFlags.GetCount();
	long    flags = field==1? g_kioskModule->m_configOp : g_kioskModule->m_configKioskf;

	wxCheckBox* checkBox;
	AddOp(sizer, id, &checkBox, text, textFlags);
	checkBox->SetValue((flags&flag)!=0);

	m_operationIds.Add(id);
	m_operationFields.Add(field);
	m_operationFlags.Add(flag);

	return checkBox;
}


void SjKioskConfigPage::AddOp(wxSizer* sizer, int id, wxCheckBox** retCheckBox, const wxString& text__, long textFlags)
{
#define MAX_OP_TEXT_LEN 30
	wxString text(text__);
	bool addTooltip = false;
	if( (textFlags&ADD_TRUNCATE_TEXT) && text.Length() > MAX_OP_TEXT_LEN )
	{
		text = text.Left(MAX_OP_TEXT_LEN-2).Trim() + wxT("..");
		addTooltip = true;
	}

	wxCheckBox* checkBox = new wxCheckBox(m_tempPanel, id, text);
	checkBox->Enable(m_optionsAvailable);
	sizer->Add(checkBox, 0, wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE*2);

	if( addTooltip )
		checkBox->SetToolTip(text__);

	if( retCheckBox ) *retCheckBox = checkBox;
}


wxCheckBox* SjKioskConfigPage::FindOp(int field, long flag)
{
	int i, iCount = m_operationIds.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		if( m_operationFields[i] == field
		        && m_operationFlags[i] == flag )
		{
			return (wxCheckBox*)FindWindow(m_operationIds[i]);
		}
	}
	return NULL;
}


void SjKioskConfigPage::OnOpCheck(wxCommandEvent& event)
{
	long    clickedIndex    = m_operationIds.Index(event.GetId()); if( clickedIndex==wxNOT_FOUND ) return;
	long    clickedField    = m_operationFields[clickedIndex];
	long    clickedFlag     = m_operationFlags[clickedIndex];
	bool    clickedValue    = GetOp(clickedField, clickedFlag);

	if( clickedField == 1 )
	{
		// if "all" is enabled, also enable all other values;
		// if one of the other values is disabled, disable "all"
		if( clickedFlag == SJ_OP_ALL && clickedValue )
		{
			int i, iCount = m_operationFlags.GetCount();
			for( i = 0; i < iCount; i++ )
			{
				if( m_operationFields[i]==1 ) SetOp(1, m_operationFlags[i], TRUE);
			}
		}
		else if( clickedFlag != SJ_OP_ALL && !clickedValue )
		{
			SetOp(1, SJ_OP_ALL, FALSE);
		}
	}
	else if( clickedField == 2 )
	{
		if( clickedFlag == SJ_KIOSKF_DISABLE_CAD && clickedValue )
		{
			// if "disable Ctrl+Alt+Del" is enabled, also enable "disable Alt+Tab"; moreover, CanDisableCtrlAltDel() may result in a reboot; therefore, save the all settings
			SetOp(2, SJ_KIOSKF_DISABLE_AT, TRUE);
			Apply();
			if( !g_kioskModule->CanDisableCtrlAltDel(this) )
			{
				SetOp(2, SJ_KIOSKF_DISABLE_CAD, FALSE);
			}
		}
		else if( clickedFlag == SJ_KIOSKF_DISABLE_AT && !clickedValue )
		{
			// if "disable Alt+Tab" is disabled, also disable "disable Ctrl+Alt+Del"
			SetOp(2, SJ_KIOSKF_DISABLE_CAD, FALSE);
		}
		else if( clickedFlag == SJ_KIOSKF_USE_PASSWORD)
		{
			// if "ask for a password" is enabled, let the user choose the password
			m_changePasswordButton->Enable(clickedValue);
			if( clickedValue )
			{
				if( !ChoosePassword() )
				{
					SetOp(2, SJ_KIOSKF_USE_PASSWORD, false);
					m_changePasswordButton->Enable(false);
				}
			}
		}
	}

	CheckDependencies();
}


bool SjKioskConfigPage::ChoosePassword()
{
	wxWindow* topLevelWindow = SjDialog::FindTopLevel(this);
	wxWindowDisabler disabler(topLevelWindow);
	SjPasswordDlg mdlg1(topLevelWindow, SJ_PASSWORDDLG_ASK_FOR_PASSWORD | SJ_PASSWORDDLG_ACCEPT_ANY_PASSWORD, _("Kiosk mode"), wxT(""), wxT(""));
	mdlg1.ShowModal();
	if( mdlg1.IsPasswordOk() )
	{
		wxString newPassword = mdlg1.GetEnteredPassword();
		SjPasswordDlg mdlg2(topLevelWindow, SJ_PASSWORDDLG_ASK_FOR_PASSWORD, _("Kiosk mode"), _("Please enter the password again for verification."), newPassword);
		mdlg2.ShowModal();
		if( mdlg2.IsPasswordOk() )
		{
			g_kioskModule->m_configUserPassword = newPassword;
			return true;
		}
	}

	return false;
}


void SjKioskConfigPage::OnOpDefault(wxCommandEvent& event)
{
	int i, iCount = m_operationFlags.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		long defFlags;
		if( m_operationFields[i]==1 )
		{
			defFlags = SJ_OP_DEF_KIOSK;
		}
		else
		{
			defFlags = SJ_KIOSKF_DEFAULT;
			if( !(m_operationFlags[i] & SJ_KIOSKF_RESETABLE) ) defFlags = 0;
		}

		if( defFlags )
		{
			SetOp(m_operationFields[i], m_operationFlags[i], (defFlags & m_operationFlags[i])!=0);
		}
	}

	CheckDependencies();
}


void SjKioskConfigPage::OnLimitToMusicSelBut(wxCommandEvent&)
{
	if( g_advSearchModule )
	{
		int preselectId = m_limitToAdvSearchChoice->GetSelection();
		if( preselectId >= 0 )
		{
			preselectId = (long)m_limitToAdvSearchChoice->GetClientData(preselectId);
		}

		g_advSearchModule->OpenDialog(preselectId);
	}
}


void SjKioskConfigPage::CheckDependencies()
{
	m_limitToAdvSearchChoice->Enable(m_optionsAvailable && GetOp(2, SJ_KIOSKF_LIMIT_TO_ADV_SEARCH));
	m_limitToAdvSearchButton->Enable(m_optionsAvailable && GetOp(2, SJ_KIOSKF_LIMIT_TO_ADV_SEARCH));

	m_monitorResChoice[0]->Enable(m_optionsAvailable && GetOp(2, SJ_KIOSKF_SWITCHRES_0));
	m_monitorResChoice[1]->Enable(m_optionsAvailable && GetOp(2, SJ_KIOSKF_SWITCHRES_1));
}


wxSpinCtrl* SjKioskConfigPage::AddSpinCtrl(wxSizer* sizer, long value, long min, long max, int id)
{
	wxSpinCtrl* spinCtrl = new wxSpinCtrl(m_tempPanel, id, wxString::Format(wxT("%i"), (int)value),
	                                      wxDefaultPosition, wxSize(SJ_3DIG_SPINCTRL_W, -1),
	                                      wxSP_ARROW_KEYS, min, max, value);

	sizer->Add(spinCtrl, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, SJ_DLG_SPACE);

	return spinCtrl;
}


void SjKioskConfigPage::AddStaticText(wxSizer* sizer, const wxString& text)
{
	wxStaticText* staticCtrl = new wxStaticText(m_tempPanel, -1, text);

	staticCtrl->Enable(m_optionsAvailable);

	sizer->Add(staticCtrl, 0, wxALIGN_CENTER_VERTICAL);
}


/*******************************************************************************
 * SjKioskConfigPage - "Monitor" Page
 ******************************************************************************/


static wxString getVideoModeName(const wxVideoMode& mode)
{
	wxString name;

	name = wxString::Format(wxT("%i x %i, %i bpp"), mode.GetWidth(), mode.GetHeight(), mode.GetDepth());

	if( mode.refresh > 0 )
	{
		name += wxString::Format(wxT(", %i Hz"), mode.refresh);
	}

	return name;
}


static wxString getDisplayUniqueId()
{
	wxString ret;
	size_t d, dCount = wxDisplay::GetCount();
	for( d = 0; d < dCount; d++ )
	{
		wxDisplay   display(d);
		int         isPrimary = display.IsPrimary()? 1 : 0;
		wxRect      geom = display.GetGeometry();
		wxVideoMode vm = display.GetCurrentMode();

		ret += wxString::Format(wxT("%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,"),
		                        (int)d,
		                        (int)isPrimary,
		                        (int)geom.x, (int)geom.y, (int)geom.width, (int)geom.height,
		                        (int)vm.GetWidth(), (int)vm.GetHeight(), (int)vm.GetDepth(), (int)vm.refresh);
	}
	return ret;
}


wxPanel* SjKioskConfigPage::CreateMonitorPage(wxWindow* parent)
{
	m_tempPanel = new wxPanel(parent, -1);
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	m_tempPanel->SetSizer(sizer1);

	sizer1->Add(1, SJ_DLG_SPACE*2); // some space

	wxStaticText* staticText = new wxStaticText(m_tempPanel, -1,
	        _("If you have more than one monitor connected to your computer,\nyou can assign the different Silverjuke windows to different monitors here."));
	staticText->Enable(m_optionsAvailable);
	sizer1->Add(staticText, 0, wxLEFT|wxBOTTOM|wxRIGHT, SJ_DLG_SPACE);

	staticText = new wxStaticText(m_tempPanel, -1,
	                              _("Overview:"));
	staticText->Enable(m_optionsAvailable);
	sizer1->Add(staticText, 0, wxLEFT|wxRIGHT, SJ_DLG_SPACE);

	// create the monitor controls
	wxASSERT( IDC_MONITOR_CHOICE_1 == IDC_MONITOR_CHOICE_0+1 );
	wxASSERT( IDC_MONITOR_RES_CHOICE_1 == IDC_MONITOR_RES_CHOICE_0+1 );

	m_monitorOverview = new SjMonitorOverview(m_tempPanel, IDC_MONITOR_SYS_SETTINGS, wxSUNKEN_BORDER);
	m_monitorOverview->Enable(m_optionsAvailable);
	sizer1->Add(m_monitorOverview, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	wxFlexGridSizer* sizer2 = new wxFlexGridSizer(5, SJ_DLG_SPACE/2, SJ_DLG_SPACE);
	sizer2->AddGrowableCol(3);
	sizer1->Add(sizer2, 0, wxGROW|wxALL, SJ_DLG_SPACE);

	AddStaticText(sizer2, _("Monitor for") + wxString(wxT(" ...")));
	sizer2->Add(2,2);
	sizer2->Add(2,2);
	sizer2->Add(2,2);
	sizer2->Add(2,2);

	int monitorSetting;
	for( monitorSetting = 0; monitorSetting < 2; monitorSetting++ )
	{
		// label
		wxString label(wxT("... "));
		label += monitorSetting==0? _("Main window") : _("Video screen");
		label += wxT(":");
		AddStaticText(sizer2, label);

		// monitor select
		m_monitorChoice[monitorSetting] = new wxChoice(m_tempPanel, IDC_MONITOR_CHOICE_0+monitorSetting, wxDefaultPosition, wxSize(-1, -1));
		m_monitorChoice[monitorSetting]->Enable(m_optionsAvailable);
		m_monitorChoice[monitorSetting]->Append(_("Monitor")+wxString(wxT(" 1")));
		sizer2->Add(m_monitorChoice[monitorSetting], 0, wxGROW|wxALIGN_CENTER_VERTICAL);

		// monitor resolution check
		wxString optionText = _("Switch resolution to %s");

		AddOp(sizer2, 2, monitorSetting==0? SJ_KIOSKF_SWITCHRES_0 : SJ_KIOSKF_SWITCHRES_1,
		      optionText.BeforeFirst('%').Trim(), false);

		// monitor resolution
		m_monitorResChoice[monitorSetting] = new wxChoice(m_tempPanel, IDC_MONITOR_RES_CHOICE_0+monitorSetting, wxDefaultPosition, wxSize(-1, -1));
		m_monitorResChoice[monitorSetting]->Enable(m_optionsAvailable);
		sizer2->Add(m_monitorResChoice[monitorSetting], 0, wxGROW|wxALIGN_CENTER_VERTICAL);

		// rest of option label
		AddStaticText(sizer2, optionText.AfterFirst('%').Mid(1).Trim(FALSE));
	}

	// diable screensaver?

#if SJ_CAN_DISABLE_SCRSAVER || SJ_CAN_DISABLE_POWERMAN
	sizer2 = new wxFlexGridSizer(2, SJ_DLG_SPACE, SJ_DLG_SPACE);
	sizer1->Add(sizer2, 0, wxALL, SJ_DLG_SPACE);

#if SJ_CAN_DISABLE_SCRSAVER
	AddOp(sizer2, 2, SJ_KIOSKF_DISABLE_SCRSAVER, _("Disable screensaver"), false);
#endif
#if SJ_CAN_DISABLE_POWERMAN
	AddOp(sizer2, 2, SJ_KIOSKF_DISABLE_POWERMAN, _("Disable power management"), false);
#endif
#endif

	return m_tempPanel;
}


void SjKioskConfigPage::InitMonitorPage()
{
	if( m_monitorInitialization.IsEmpty() )
	{
		wxBusyCursor busy;

		Freeze();

		UpdateMonitorPage();
		m_monitorInitialization = getDisplayUniqueId();

		m_monitorTimer.SetOwner(this, IDC_MONITOR_TIMER);
		m_monitorTimer.Start(1000, wxTIMER_CONTINUOUS);

		Thaw();
	}
}


void SjKioskConfigPage::OnMonitorTimer(wxTimerEvent&)
{
	static bool inHere = false;
	if( !inHere )
	{
		inHere = true;
		wxString thisUniqueId = getDisplayUniqueId();
		if( m_monitorInitialization != thisUniqueId )
		{
			UpdateMonitorPage();
			m_monitorInitialization = thisUniqueId;
		}
		inHere = false;
	}
}


void SjKioskConfigPage::UpdateMonitorPage()
{
	size_t displayCount = wxDisplay::GetCount(), displayIndex;

	m_monitorOverview->Clear();

	int monitorSetting;
	for( monitorSetting = 0; monitorSetting < 2; monitorSetting++ )
	{
		// set up the monitor choice control and
		// find out the selected display index or the best display index for this monitor setting
		m_monitorChoice[monitorSetting]->Clear();
		size_t displayPrimaryIndex = 0, displaySelectedIndex = 0x7fffffffL;
		for( displayIndex = 0; displayIndex < displayCount; displayIndex++ )
		{
			wxDisplay curr(displayIndex);
			wxString name = _("Monitor") + wxString::Format(wxT(" %i"), (int)displayIndex+1);
			m_monitorChoice[monitorSetting]->Append(name);

			if( curr.IsPrimary() )
			{
				displayPrimaryIndex = displayIndex;
			}

			if( g_kioskModule->m_configMonitor[monitorSetting] == (long)displayIndex )
			{
				displaySelectedIndex = displayIndex;
			}

			if( monitorSetting == 0 )
				m_monitorOverview->AddMonitor(curr.GetGeometry());
		}

		if( displaySelectedIndex == 0x7fffffffL )
			displaySelectedIndex = displayPrimaryIndex;

		// set the selction in the monitor choice control;
		// as this might have changed, also remember this setting in the config
		m_monitorChoice[monitorSetting]->SetSelection(displaySelectedIndex);
		g_kioskModule->m_configMonitor[monitorSetting] = displaySelectedIndex;

		m_monitorOverview->SetMonitorUsage(displaySelectedIndex,
		                                   monitorSetting == 0? MONITOR_USAGE_MAIN : MONITOR_USAGE_VIS, true);

		// update monitor resolution
		UpdateMonitorResChoice(m_monitorResChoice[monitorSetting], displaySelectedIndex,
		                       g_kioskModule->m_configDispRes[monitorSetting]);
	}

	if( displayCount <= 1 )
		m_monitorOverview->AddDummy();

	m_monitorOverview->Refresh();
}


void SjKioskConfigPage::UpdateMonitorResChoice( wxChoice* monitorResChoice, size_t displayIndex,
        wxString& selRes)
{
	wxDisplay display(displayIndex);

	// collect all strings
	wxArrayString modeNames;
	{
		wxArrayVideoModes   modes = display.GetModes();
		size_t              m, mCount = modes.GetCount();
		for( m = 0; m < mCount; m++ )
		{
			if( modes[m].IsOk() && modes[m].GetDepth()>8 )
			{
				wxString modeName( getVideoModeName(modes[m]) );
				if( modeNames.Index(modeName) == wxNOT_FOUND )
				{
					modeNames.Add(modeName);
				}
			}
		}
	}

	// sort all strings
	modeNames.Sort();

	// update choice control
	{
		bool anythingSelected = false;
		size_t m, mCount = modeNames.GetCount();
		monitorResChoice->Clear();
		for( m = 0; m < mCount; m++ )
		{
			monitorResChoice->Append(modeNames[m]);
			if( modeNames[m] == selRes )
			{
				monitorResChoice->SetSelection(m);
				anythingSelected = true;
			}
		}

		if( !anythingSelected )
		{
			// try to select the default mode
			wxString newSelRes = getVideoModeName(display.GetCurrentMode());
			int i = modeNames.Index(newSelRes);
			if( i == wxNOT_FOUND && !modeNames.IsEmpty() )
			{
				newSelRes = modeNames[0];
				i = 0;
			}

			if( i != wxNOT_FOUND )
			{
				monitorResChoice->SetSelection(i);
				selRes = newSelRes;
			}
		}
	}
}


void SjKioskConfigPage::OnMonitorChoice(wxCommandEvent& e)
{
	int       monitorSetting = e.GetId() - IDC_MONITOR_CHOICE_0;
	wxChoice* monitorChoice = (wxChoice*)FindWindow(IDC_MONITOR_CHOICE_0+monitorSetting);
	if( monitorChoice )
	{
		wxBusyCursor busy;

		size_t displayCount = wxDisplay::GetCount();
		size_t newDisplayIndex = monitorChoice->GetSelection();
		if( newDisplayIndex >= 0 && newDisplayIndex < displayCount )
		{
			long flag = monitorSetting == 0? MONITOR_USAGE_MAIN : MONITOR_USAGE_VIS;
			m_monitorOverview->SetMonitorUsage(g_kioskModule->m_configMonitor[monitorSetting], flag, false);
			m_monitorOverview->SetMonitorUsage(newDisplayIndex, flag, true);
			m_monitorOverview->Refresh();
			//m_monitorOverview->Update();

			g_kioskModule->m_configMonitor[monitorSetting] = newDisplayIndex;
			g_kioskModule->m_configDispRes[monitorSetting].Empty();

			UpdateMonitorResChoice(m_monitorResChoice[monitorSetting], newDisplayIndex, g_kioskModule->m_configDispRes[monitorSetting]);
		}
	}
}


void SjKioskConfigPage::OnMonitorResChoice(wxCommandEvent& e)
{
	int       monitorSetting = e.GetId() - IDC_MONITOR_RES_CHOICE_0;
	wxChoice* monitorResChoice = (wxChoice*)FindWindow(IDC_MONITOR_RES_CHOICE_0+monitorSetting);
	if( monitorResChoice )
	{
		g_kioskModule->m_configDispRes[monitorSetting]
		    = monitorResChoice->GetStringSelection();
	}
}


void SjKioskConfigPage::OnMonitorSysSettings(wxCommandEvent& e)
{
	long flag = e.GetExtraLong();
	if( flag == MONITOR_USAGE_MAIN || flag == MONITOR_USAGE_VIS )
	{
		long monitorSettingIndex = flag == MONITOR_USAGE_MAIN? 0 : 1;
		long newDisplayIndex = e.GetInt();

		if( newDisplayIndex >= 0 && newDisplayIndex < (long)m_monitorChoice[monitorSettingIndex]->GetCount() )
		{
			m_monitorChoice[monitorSettingIndex]->SetSelection(newDisplayIndex);
			wxCommandEvent fwd(wxEVT_COMMAND_CHOICE_SELECTED, IDC_MONITOR_CHOICE_0+monitorSettingIndex);
			OnMonitorChoice(fwd);
		}
	}
}


/*******************************************************************************
 * SjKioskConfigPage - "Virtual keyboard" Page
 ******************************************************************************/


wxPanel* SjKioskConfigPage::CreateVirtKeybdPage(wxWindow* parent)
{
	long flags = g_virtKeybd->GetKeybdFlags();

	m_tempPanel = new wxPanel(parent, -1);
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	m_tempPanel->SetSizer(sizer1);

	sizer1->Add(1, SJ_DLG_SPACE*2); // some space

	wxStaticText* staticText = new wxStaticText(m_tempPanel, -1,
	        _("If no physical keyboard is present, you can use our virtual keyboard;\nthe virtual keyboard then is shown whenever you click into a text control."));
	sizer1->Add(staticText, 0, wxLEFT|wxBOTTOM|wxRIGHT, SJ_DLG_SPACE);

	// use?
	m_virtKeybdInKiosk = new wxCheckBox(m_tempPanel, IDC_VIRTKEYBD_INKIOSK,
	                                    _("Use the virtual keyboard"));
	sizer1->Add(m_virtKeybdInKiosk, 0, wxALL, SJ_DLG_SPACE);

	// use outside kiosk?
	m_virtKeybdOutsideKiosk = new wxCheckBox(m_tempPanel, IDC_VIRTKEYBD_OUTKIOSK,
	        _("Also use the virtual keyboard outside the kiosk mode"));
	sizer1->Add(m_virtKeybdOutsideKiosk, 0, wxLEFT|wxBOTTOM|wxRIGHT, SJ_DLG_SPACE);

	wxFlexGridSizer* sizer2 = new wxFlexGridSizer(2, (long)((float)SJ_DLG_SPACE*0.7), SJ_DLG_SPACE);
	sizer1->Add(sizer2, 0, wxLEFT|wxBOTTOM|wxRIGHT, SJ_DLG_SPACE);

	// test field
	m_virtKeybdTestLabel1 = new wxStaticText(m_tempPanel, -1,
	        _("Test:"));
	sizer2->Add(m_virtKeybdTestLabel1, 0, wxALIGN_CENTER_VERTICAL);

	wxSizer* sizer3 = new wxBoxSizer(wxHORIZONTAL);
	sizer2->Add(sizer3, 0, wxALIGN_CENTER_VERTICAL);

	m_virtKeybdTest = new SjVirtKeybdTextCtrl(m_tempPanel, -1, wxT(""), wxDefaultPosition, wxSize(100, -1), 0);
	m_virtKeybdTest->SetForceForTesting();
	sizer3->Add(m_virtKeybdTest, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);

	m_virtKeybdTestLabel2 = new wxStaticText(m_tempPanel, -1,
	        _("(please click into the text control)"));
	sizer3->Add(m_virtKeybdTestLabel2, 0, wxALIGN_CENTER_VERTICAL);


	// layout selection
	m_virtKeybdLayoutLabel = new wxStaticText(m_tempPanel, -1,
	        _("Virtual keyboard layout:"));
	sizer2->Add(m_virtKeybdLayoutLabel, 0, wxALIGN_CENTER_VERTICAL);

	m_virtKeybdLayout = new wxChoice(m_tempPanel, IDC_VIRTKEYBD_LAYOUT);
	SjArrayVirtKeybdLayout layouts;
	wxString currKeybdLayout = g_virtKeybd->GetKeybdLayout();
	g_virtKeybd->GetAvailKeybdLayouts(layouts);
	long i, iCount = layouts.GetCount(), sthSelected = 0;
	for( i = 0; i < iCount; i++ )
	{
		m_virtKeybdLayout->Append(layouts[i].m_name, (void*)i);
		if( currKeybdLayout == layouts[i].m_file )
		{
			m_virtKeybdLayout->SetSelection(i);
			sthSelected = 1;
		}
	}
	if( !sthSelected )
	{
		m_virtKeybdLayout->SetSelection(0); // = US International
	}
	m_virtKeybdLayout->Append(_("More layouts on the web..."), (void*)0x0000FFFFL);
	SjDialog::SetCbWidth(m_virtKeybdLayout);
	sizer2->Add(m_virtKeybdLayout, 0, wxALIGN_CENTER_VERTICAL);

	// colour setting
	m_virtKeybdColourLabel = new wxStaticText(m_tempPanel, -1,
	        _("Colour:"));
	sizer2->Add(m_virtKeybdColourLabel, 0, wxALIGN_CENTER_VERTICAL);

	m_virtKeybdColour = new wxChoice(m_tempPanel, IDC_VIRTKEYBD_COLOUR);
	m_virtKeybdColour->Append(_("Black"));
	m_virtKeybdColour->Append(_("White"));
	m_virtKeybdColour->SetSelection((flags&SJ_VIRTKEYBD_BLACK)? 0 : 1);
	SjDialog::SetCbWidth(m_virtKeybdColour);
	sizer2->Add(m_virtKeybdColour, 0, wxALIGN_CENTER_VERTICAL);

	// transparency setting
	if( g_tools->CanSetWindowTransparency() )
	{
		m_virtKeybdTranspLabel1 = new wxStaticText(m_tempPanel, -1, _("Transparency:"));
		sizer2->Add(m_virtKeybdTranspLabel1, 0, wxALIGN_CENTER_VERTICAL);

		sizer3 = new wxBoxSizer(wxHORIZONTAL);
		sizer2->Add(sizer3, 0, wxALIGN_CENTER_VERTICAL);

		m_virtKeybdTransp = new wxSlider(m_tempPanel, IDC_VIRTKEYBD_TRANSP,
		                                 g_virtKeybd->GetTransparency(), 0, 80/*not: 100 - avoid the keyboard to disappear completely*/,
		                                 wxDefaultPosition, wxSize(120, -1), wxSL_HORIZONTAL);
		sizer3->Add(m_virtKeybdTransp, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

		m_virtKeybdTranspLabel2 = new wxStaticText(m_tempPanel, -1,  wxEmptyString);
		UpdateVirtKeybdTranspText();
		sizer3->Add(m_virtKeybdTranspLabel2, 0, wxALIGN_CENTER_VERTICAL);
	}
	else
	{
		m_virtKeybdTransp = NULL;
		m_virtKeybdTranspLabel1 = NULL;
		m_virtKeybdTranspLabel2 = NULL;
	}

	// hide cursor
	m_virtKeybdHideCursor = new wxCheckBox(m_tempPanel, IDC_VIRTKEYBD_HIDECURSOR,
	                                       _("Hide cursor"));
	sizer1->Add(m_virtKeybdHideCursor, 0, wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);
	m_virtKeybdHideCursor->SetValue((flags&SJ_VIRTKEYBD_HIDE_CURSOR)!=0);

	// default
	wxButton* virtKeybdDefault = new wxButton(m_tempPanel, IDC_VIRTKEYBD_DEFAULT, _("Reset to default values"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer1->Add(virtKeybdDefault, 0, wxLEFT|wxRIGHT|wxTOP|wxBOTTOM, SJ_DLG_SPACE);

	m_virtKeybdTest->DontCloseOn(m_virtKeybdOutsideKiosk);
	m_virtKeybdTest->DontCloseOn(m_virtKeybdLayout);
	m_virtKeybdTest->DontCloseOn(m_virtKeybdColour);
	m_virtKeybdTest->DontCloseOn(virtKeybdDefault);
	if( m_virtKeybdTransp )
		m_virtKeybdTest->DontCloseOn(m_virtKeybdTransp);

	UpdateVirtKeybdCtrls();

	return m_tempPanel;
}


void SjKioskConfigPage::UpdateVirtKeybdCtrls()
{
	long flags = g_virtKeybd->GetKeybdFlags();

	// virtual keyboard on/off

	bool useInKiosk = (flags&SJ_VIRTKEYBD_USE_IN_KIOSK)? TRUE : FALSE;
	bool useOutsideKiosk = (flags&SJ_VIRTKEYBD_USE_OUTSIDE_KIOSK)? TRUE : FALSE;

	m_virtKeybdInKiosk->SetValue(useInKiosk? TRUE : FALSE);

	m_virtKeybdOutsideKiosk->Enable(useInKiosk);
	m_virtKeybdOutsideKiosk->SetValue((useInKiosk && useOutsideKiosk)? TRUE : FALSE);

	// virtual keyboard settings

	m_virtKeybdLayoutLabel->Enable(useInKiosk);
	m_virtKeybdLayout->Enable(useInKiosk);

	m_virtKeybdColourLabel->Enable(useInKiosk);
	m_virtKeybdColour->Enable(useInKiosk);

	if( m_virtKeybdTransp )
	{
		m_virtKeybdTranspLabel1->Enable(useInKiosk);
		m_virtKeybdTranspLabel2->Enable(useInKiosk);
		m_virtKeybdTransp->Enable(useInKiosk);
	}

	m_virtKeybdTestLabel1->Enable(useInKiosk);
	m_virtKeybdTest->Enable(useInKiosk);

	m_virtKeybdTestLabel2->Enable(useInKiosk);
}


void SjKioskConfigPage::UpdateVirtKeybdTranspText()
{
	if( m_virtKeybdTransp )
	{
		int v = m_virtKeybdTransp->GetValue();
		m_virtKeybdTranspLabel2->SetLabel(wxString::Format(wxT("%i%%  "), v));
	}
}


void SjKioskConfigPage::OnVirtKeybdCheck(wxCommandEvent& e)
{
	int id = e.GetId();

	if( id == IDC_VIRTKEYBD_INKIOSK )
	{
		g_virtKeybd->SetFlag(SJ_VIRTKEYBD_USE_IN_KIOSK, m_virtKeybdInKiosk->IsChecked());
	}
	else if( id == IDC_VIRTKEYBD_OUTKIOSK )
	{
		g_virtKeybd->SetFlag(SJ_VIRTKEYBD_USE_OUTSIDE_KIOSK, m_virtKeybdOutsideKiosk->IsChecked());
	}
	else if( id == IDC_VIRTKEYBD_HIDECURSOR )
	{
		g_virtKeybd->SetFlag(SJ_VIRTKEYBD_HIDE_CURSOR, m_virtKeybdHideCursor->IsChecked());
		g_mainFrame->SetCursor(SjVirtKeybdModule::GetStandardCursor());
	}

	UpdateVirtKeybdCtrls();
}


void SjKioskConfigPage::OnVirtKeybdLayout(wxCommandEvent&)
{
	SjArrayVirtKeybdLayout layouts;
	wxString currKeybdLayout = g_virtKeybd->GetKeybdLayout();
	g_virtKeybd->GetAvailKeybdLayouts(layouts);
	long i, iCount = layouts.GetCount(), sthSelected = 0;

	long selIndex = m_virtKeybdLayout->GetSelection();
	if( selIndex == -1 )
	{
		return;
	}

	selIndex = (long)m_virtKeybdLayout->GetClientData(selIndex);
	if( selIndex == 0x0000FFFFL )
	{
		// open browser to get more layouts from the web
		for( i = 0; i < iCount; i++ )
		{
			if( currKeybdLayout == layouts[i].m_file )
			{
				m_virtKeybdLayout->SetSelection(i);
				sthSelected = 1;
			}
		}
		if( !sthSelected )
		{
			m_virtKeybdLayout->SetSelection(0); // = US International
		}

		g_tools->ExploreHomepage(SJ_HOMEPAGE_MODULES_VIRTKEYB);
	}
	else
	{
		// select layout
		if( selIndex >= 0 && selIndex < (long)layouts.GetCount() )
		{
			g_virtKeybd->SetKeybdLayout(layouts[selIndex].m_file);
		}
	}
}


void SjKioskConfigPage::OnVirtKeybdColour(wxCommandEvent&)
{
	long selIndex = m_virtKeybdColour->GetSelection();
	g_virtKeybd->SetFlag(SJ_VIRTKEYBD_BLACK, (selIndex==0));
}


void SjKioskConfigPage::OnVirtKeybdTransp(wxScrollEvent&)
{
	if( m_virtKeybdTransp )
	{
		int v = m_virtKeybdTransp->GetValue();
		g_virtKeybd->SetTransparency(v);

		UpdateVirtKeybdTranspText();
	}
}


void SjKioskConfigPage::OnVirtKeybdDefault(wxCommandEvent&)
{
	g_virtKeybd->SetFlag(SJ_VIRTKEYBD_BLACK, TRUE);
	m_virtKeybdColour->SetSelection(0);

	if( m_virtKeybdTransp )
	{
		g_virtKeybd->SetTransparency(SJ_DEF_VIRTKEYBD_TRANSP);
		m_virtKeybdTransp->SetValue(SJ_DEF_VIRTKEYBD_TRANSP);
		UpdateVirtKeybdTranspText();
	}

	m_virtKeybdHideCursor->SetValue(false);
	g_virtKeybd->SetFlag(SJ_VIRTKEYBD_HIDE_CURSOR, false);
}


/*******************************************************************************
 * SjKioskModule - Numpad Controls
 ******************************************************************************/


wxPanel* SjKioskConfigPage::CreateNumpadPage(wxWindow* parent)
{
	m_tempPanel = new wxPanel(parent, -1);
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	m_tempPanel->SetSizer(sizer1);

	sizer1->Add(1, SJ_DLG_SPACE*2); // some space

	wxStaticText* staticText = new wxStaticText(m_tempPanel, -1,
	        _("Using the Numpad plus some other special keys, you can control Silverjuke completely\nby the keyboard or some other hardware buttons. In this case, you'll enqueue new\ntracks by entering the unique numbers shown beside the album and track names."));
	sizer1->Add(staticText, 0, wxLEFT|wxBOTTOM|wxRIGHT, SJ_DLG_SPACE);

	// use?
	m_numpadUseInKioskCheckBox = new wxCheckBox(m_tempPanel, IDC_NUMPAD_USE_IN_KIOSK,
	        _("Use Numpad controls"));
	m_numpadUseInKioskCheckBox->SetValue((g_accelModule->m_flags&SJ_ACCEL_USE_NUMPAD_IN_KIOSK)!=0);
	sizer1->Add(m_numpadUseInKioskCheckBox, 0, wxALL, SJ_DLG_SPACE);

	m_numpadUseOutsideKioskCheckBox = new wxCheckBox(m_tempPanel, IDC_NUMPAD_USE_OUTSIDE_KIOSK,
	        _("Also use Numpad controls outside the kiosk mode"));
	if( (g_accelModule->m_flags&SJ_ACCEL_USE_NUMPAD_IN_KIOSK) )
	{
		m_numpadUseOutsideKioskCheckBox->SetValue((g_accelModule->m_flags&SJ_ACCEL_USE_NUMPAD_OUT_KIOSK)!=0);
	}
	sizer1->Add(m_numpadUseOutsideKioskCheckBox, 0, wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	wxFlexGridSizer* sizer2 = new wxFlexGridSizer(3, (long)((float)SJ_DLG_SPACE*0.7), SJ_DLG_SPACE);
	sizer1->Add(sizer2, 0, wxLEFT|wxBOTTOM|wxRIGHT, SJ_DLG_SPACE);

	// keys overview
	m_numpadKeysLabel = new wxStaticText(m_tempPanel, -1, _("Keys:"));
	sizer2->Add(m_numpadKeysLabel, 0, wxALIGN_CENTER_VERTICAL);

	int idt;
	wxString allKeys;

	size_t wrapAt = (70-wxString(_("Keys:")).Len())-wxString(_("Edit...")).Len();
	if( wrapAt < 40 ) wrapAt = 40;

	for( idt = IDT_NUMPAD_FIRST; idt <= IDT_NUMPAD_LAST; idt++ )
	{
		wxString currKeys(g_accelModule->GetReadableShortcutsByCmd(idt, SJ_SHORTCUTS_LOCAL|SJ_SHORTCUTS_SYSTEMWIDE|SJ_SHORTCUTS_ADDALL));

		if( idt == IDT_NUMPAD_MENU && currKeys.IsEmpty() )
		{
			// if no numpad-"menu" key is defined, the menu can be opened
			// by pressing "0" several times (more exact: the number of album digits)
			currKeys = wxString(wxT('0'), g_kioskModule->m_numpadInput.GetNumAlbumDigits());
		}

		if( !currKeys.IsEmpty() )
		{
			if( idt<IDT_NUMPAD_0 || idt>IDT_NUMPAD_9 )
			{
				// prepend title to the keys (not done for obvious keys as 0-9)
				wxString currName(g_accelModule->GetCmdNameById(idt, TRUE/*shortName*/));
				if( !currName.IsEmpty() )
				{
					currKeys.Prepend(currName + wxT("="));
				}
			}

			if( !allKeys.IsEmpty() )
			{
				allKeys += wxT(", ");
				if( allKeys.AfterLast(wxT('\n')).Len() > wrapAt )
				{
					allKeys += wxT("\n");
				}
			}
			allKeys += currKeys;
		}
	}

	if( allKeys.Find('\n') == -1 && allKeys.Find(',') != -1 )
	{
		// force at least two lines (looks better)
		allKeys = allKeys.BeforeLast(',') + wxT(",\n") + allKeys.AfterLast(wxT(',')).Trim(FALSE);
	}

	m_numpadKeysList = new wxStaticText(m_tempPanel, -1, allKeys);
	sizer2->Add(m_numpadKeysList, 0, wxALIGN_CENTER_VERTICAL);


	// "edit keys" button
	m_numpadEditKeysButton = new wxButton(m_tempPanel, IDC_NUMPAD_EDITKEYS, _("Edit..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer2->Add(m_numpadEditKeysButton, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);

	UpdateNumpadCtrls();

	return m_tempPanel;
}


void SjKioskConfigPage::UpdateNumpadCtrls()
{
	bool useNumpad = m_numpadUseInKioskCheckBox->IsChecked();

	m_numpadUseOutsideKioskCheckBox->Enable(useNumpad);
	m_numpadKeysLabel->Enable(useNumpad);
	m_numpadKeysList->Enable(useNumpad);
	m_numpadEditKeysButton->Enable(useNumpad);
}


void SjKioskConfigPage::OnNumpadUseInKioskCheck(wxCommandEvent&)
{
	SjTools::SetFlag(g_accelModule->m_flags, SJ_ACCEL_USE_NUMPAD_IN_KIOSK, m_numpadUseInKioskCheckBox->IsChecked());

	g_tools->m_config->Write(wxT("main/accelFlags"), g_accelModule->m_flags);

	g_mainFrame->ClearDisplayMsg();
	g_mainFrame->SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_MAIN));
	g_mainFrame->m_browser->RefreshAll();

	UpdateNumpadCtrls();

	if( !(g_accelModule->m_flags&SJ_ACCEL_USE_NUMPAD_IN_KIOSK) )
	{
		m_numpadUseOutsideKioskCheckBox->SetValue(FALSE);
	}
	else
	{
		m_numpadUseOutsideKioskCheckBox->SetValue((g_accelModule->m_flags&SJ_ACCEL_USE_NUMPAD_OUT_KIOSK)!=0);
	}
}


void SjKioskConfigPage::OnNumpadUseOutsideKioskCheck(wxCommandEvent&)
{
	SjTools::SetFlag(g_accelModule->m_flags, SJ_ACCEL_USE_NUMPAD_OUT_KIOSK, m_numpadUseOutsideKioskCheckBox->IsChecked());

	g_tools->m_config->Write(wxT("main/accelFlags"), g_accelModule->m_flags);

	g_mainFrame->ClearDisplayMsg();
	g_mainFrame->SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_MAIN));
	g_mainFrame->m_browser->RefreshAll();

	UpdateNumpadCtrls();
}


void SjKioskConfigPage::OnNumpadEditKeys(wxCommandEvent&)
{
	SjSettingsModule::OpenSettingsAsync(wxT("memory:basicsettings.lib"), 0, 667);
}


/*******************************************************************************
 * SjKioskModule - Credit system
 ******************************************************************************/


wxPanel* SjKioskConfigPage::CreateCreditPage(wxWindow* parent)
{
	m_tempPanel = new wxPanel(parent, -1);
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	m_tempPanel->SetSizer(sizer1);

	sizer1->Add(1, SJ_DLG_SPACE*2); // some space

	wxStaticText* staticText = new wxStaticText(m_tempPanel, -1,
	        _("With the credit system, users must eg. add coins before tracks can be enqueued. Please\nrefer to the online help for details about the possibilities and the supported hardware."));
	sizer1->Add(staticText, 0, wxLEFT|wxBOTTOM|wxRIGHT, SJ_DLG_SPACE);

	// use?
	m_creditUseCheckBox = new wxCheckBox(m_tempPanel, IDC_CREDIT_USE,
	                                     _("Use credit system"));
	m_creditUseCheckBox->SetValue(g_kioskModule->m_creditBase.IsCreditSystemEnabled());
	sizer1->Add(m_creditUseCheckBox, 0, wxLEFT|wxRIGHT/*|wxBOTTOM*/, SJ_DLG_SPACE);

	// listen to DDE / command line / shortcuts
	wxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2, 0, wxLEFT|wxRIGHT/*|wxBOTTOM*/, SJ_DLG_SPACE);

	wxString ddeString = _("Credits may be added by DDE, command line or shortcuts");
	m_creditDDECheckBox = new wxCheckBox(m_tempPanel, IDC_CREDIT_DDE, ddeString);
	sizer2->Add(m_creditDDECheckBox, 0, wxALIGN_CENTER_VERTICAL);

	m_creditShortcutEditButton = new wxButton(m_tempPanel, IDC_CREDIT_SHORTCUT_EDIT, _("Keys..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer2->Add(m_creditShortcutEditButton, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);

	// save?
	m_creditSaveCheckBox = new wxCheckBox(m_tempPanel, IDC_CREDIT_SAVE,
	                                      _("Remember credits between program starts"));
	sizer1->Add(m_creditSaveCheckBox, 0, wxLEFT|wxBOTTOM|wxRIGHT, SJ_DLG_SPACE);

	// current credit
	sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2, 0, wxLEFT|wxBOTTOM|wxRIGHT, SJ_DLG_SPACE);

	m_creditCurrLabel = new wxStaticText(m_tempPanel, -1,
	                                     _("Current credit:"));
	sizer2->Add(m_creditCurrLabel, 0, wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);

	m_creditCurrSpinCtrl = AddSpinCtrl(sizer2, g_kioskModule->m_creditBase.GetCredit(), 0, 9999, IDC_CREDIT_CURR);

	UpdateCreditCtrls();

	return m_tempPanel;
}


void SjKioskConfigPage::UpdateCreditCtrls()
{
	bool creditSystemEnabled = m_creditUseCheckBox->IsChecked();
	bool creditDDEEnabled = (g_kioskModule->m_creditBase.GetFlags()&SJ_CREDITBASEF_LISTEN_TO_DDE)!=0;

	m_creditSaveCheckBox->Enable(creditSystemEnabled);
	m_creditSaveCheckBox->SetValue(creditSystemEnabled && (g_kioskModule->m_creditBase.GetFlags()&SJ_CREDITBASEF_SAVE_CREDITS)!=0);

	m_creditDDECheckBox->Enable(creditSystemEnabled);
	m_creditDDECheckBox->SetValue(creditSystemEnabled && creditDDEEnabled);

	m_creditShortcutEditButton->Enable(creditSystemEnabled && creditDDEEnabled);

	m_creditCurrLabel->Enable(creditSystemEnabled);
	m_creditCurrSpinCtrl->Enable(creditSystemEnabled);
}


void SjKioskConfigPage::OnCreditUseCheck(wxCommandEvent&)
{
	g_kioskModule->m_creditBase.SetFlag(SJ_CREDITBASEF_ENABLED, m_creditUseCheckBox->IsChecked());

	UpdateCreditCtrls();
}


void SjKioskConfigPage::OnCreditDDECheck(wxCommandEvent&)
{
	g_kioskModule->m_creditBase.SetFlag(SJ_CREDITBASEF_LISTEN_TO_DDE, m_creditDDECheckBox->IsChecked());

	UpdateCreditCtrls();
}


void SjKioskConfigPage::OnCreditShortcutEditButton(wxCommandEvent&)
{
	SjSettingsModule::OpenSettingsAsync(wxT("memory:basicsettings.lib"), 0, 668);
}


void SjKioskConfigPage::OnCreditSaveCheck(wxCommandEvent&)
{
	g_kioskModule->m_creditBase.SetFlag(SJ_CREDITBASEF_SAVE_CREDITS, m_creditSaveCheckBox->IsChecked());
}


void SjKioskConfigPage::OnCreditCurrSpinCtrl(wxSpinEvent&)
{
	int newCredit = m_creditCurrSpinCtrl->GetValue();
	g_kioskModule->m_creditBase.AddCredit(newCredit, SJ_ADDCREDIT_SET_TO_NULL_BEFORE_ADD);
}


/*******************************************************************************
 * SjKioskModule - Common
 ******************************************************************************/


SjKioskModule* g_kioskModule = NULL;


SjKioskModule::SjKioskModule(SjInterfaceBase* interf)
	: SjCommonModule(interf), m_uptime(wxT("kiosk/upTime"))
{
	m_file              = wxT("memory:kiosk.lib");
	m_name              = _("Kiosk mode");
	m_guiIcon           = SJ_ICON_KIOSK;
	m_sort              = 50;
	m_isStarted         = FALSE;
	m_configLoaded      = FALSE;
	g_kioskModule       = this;
	m_passwordDlgOpened = FALSE;
}


void SjKioskModule::LastUnload()
{
	g_kioskModule = NULL;
}


void SjKioskModule::LoadConfig()
{
	if( !m_configLoaded )
	{
		m_configKioskf          = g_tools->m_config->Read(wxT("kiosk/kioskf"), SJ_KIOSKF_DEFAULT);
		m_configOp              = g_tools->m_config->Read(wxT("kiosk/opf"), SJ_OP_DEF_KIOSK);
		m_configMaxTracksInQueue= g_tools->m_config->Read(wxT("kiosk/maxTracksInQueue"), SJ_DEF_MAX_TRACKS_IN_QUEUE);
		m_configLimitToAdvSearch= g_tools->m_config->Read(wxT("kiosk/limitToAdvSearch"), 0L);
		m_configMonitor[0]      = g_tools->m_config->Read(wxT("kiosk/monitor0"), -1L);
		m_configMonitor[1]      = g_tools->m_config->Read(wxT("kiosk/monitor1"), -1L);
		m_configDispRes[0]      = g_tools->m_config->Read(wxT("kiosk/dispRes0"), wxT(""));
		m_configDispRes[1]      = g_tools->m_config->Read(wxT("kiosk/dispRes1"), wxT(""));
		m_configDefExitAction   = (SjShutdownEtc)g_tools->m_config->Read(wxT("kiosk/defExitAction"), (long)SJ_SHUTDOWN_EXIT_KIOSK_MODE);

		m_configUserPassword = g_tools->m_config->Read(wxT("kiosk/password"), wxT(""));
		if( !m_configUserPassword.IsEmpty() )
			m_configUserPassword = SjTools::UnscrambleString(m_configUserPassword);

		m_configMaintenancePassword = g_tools->m_config->Read(wxT("kiosk/maintenancePassword"), wxT(""));
		if( !m_configMaintenancePassword.IsEmpty() )
			m_configMaintenancePassword = SjTools::UnscrambleString(m_configMaintenancePassword);

		// make sure either album or list view is enabled
		if( (m_configOp&(SJ_OP_ALBUM_VIEW|SJ_OP_COVER_VIEW|SJ_OP_LIST_VIEW)) == 0 )
			m_configOp |= SJ_OP_ALBUM_VIEW;

		m_configLoaded = TRUE;
	}
}


void SjKioskModule::SaveConfig()
{
	g_tools->m_config->Write(wxT("kiosk/kioskf"), m_configKioskf);
	g_tools->m_config->Write(wxT("kiosk/opf"), m_configOp);
	g_tools->m_config->Write(wxT("kiosk/maxTracksInQueue"), m_configMaxTracksInQueue);
	g_tools->m_config->Write(wxT("kiosk/limitToAdvSearch"), m_configLimitToAdvSearch);
	g_tools->m_config->Write(wxT("kiosk/monitor0"), m_configMonitor[0]);
	g_tools->m_config->Write(wxT("kiosk/monitor1"), m_configMonitor[1]);
	g_tools->m_config->Write(wxT("kiosk/dispRes0"), m_configDispRes[0]);
	g_tools->m_config->Write(wxT("kiosk/dispRes1"), m_configDispRes[1]);
	g_tools->m_config->Write(wxT("kiosk/defExitAction"), (long)m_configDefExitAction);
	g_tools->m_config->Write(wxT("kiosk/password"), SjTools::ScrambleString(m_configUserPassword));
	g_tools->m_config->Write(wxT("kiosk/maintenancePassword"), SjTools::ScrambleString(m_configMaintenancePassword));

	g_tools->m_config->Flush(); // Silverjuke may be "killed" eg. if the computer is rebooted
	// on the exit of the kiosk mode; make sure, the settings are really saved
}


wxWindow* SjKioskModule::GetConfigPage(wxWindow* parent, int selectedPage)
{
	return new SjKioskConfigPage(parent, selectedPage);
}


void SjKioskModule::GetConfigButtons(wxString& okText, wxString& cancelText, bool& okBold)
{
	okText = KioskStarted()? _("Stop") : _("Start");
	cancelText = _("Close");
	okBold = TRUE;
}


void SjKioskModule::DoneConfigPage(wxWindow* configPage, int doneAction)
{
	((SjKioskConfigPage*)configPage)->Apply();

	if( doneAction == SJ_CONFIGPAGE_DONE_OK_CLICKED )
	{
		if( KioskStarted() )
		{
			ExitRequest(0);
		}
		else
		{
			DoStart();
		}
	}
}


void SjKioskModule::ReceiveMsg(int msg)
{
	if( msg == IDMODMSG_ADV_SEARCH_CONFIG_CHANGED )
	{
		if( g_kioskConfigPage )
		{
			SjAdvSearchModule::UpdateAdvSearchChoice(g_kioskConfigPage->m_limitToAdvSearchChoice, 0);
		}
	}
}


SjHomepageId SjKioskModule::GetHelpTopic()
{
	SjHomepageId ret = SJ_HELP_KIOSK;
	if( g_kioskConfigPage )
	{
		int selPage = g_kioskConfigPage->m_notebook->GetSelection();
		if( selPage == PAGE_FUNCTIONS )
		{
			ret = SJ_HELP_KIOSK_FUNC;
		}
		else if( selPage == PAGE_MONITOR )
		{
			ret = SJ_HELP_KIOSK_MONITORS;
		}
		else if( selPage == PAGE_VIRTKEYBD )
		{
			ret = SJ_HELP_VIRTKEYBD;
		}
		else if( selPage == PAGE_NUMPAD )
		{
			ret = SJ_HELP_NUMPAD;
		}
		else if( selPage == PAGE_CREDIT )
		{
			ret = SJ_HELP_CREDIT_SYSTEM;
		}
	}
	return ret;
}


bool SjKioskModule::IsPasswordInUse()
{
	LoadConfig();
	return (m_configKioskf&SJ_KIOSKF_USE_PASSWORD)!=0;
}


/*******************************************************************************
 * SjKioskModule - Start / Exit
 ******************************************************************************/


void SjKioskModule::StartRequest(bool forceNoQueryConfig)
{
	if( !KioskStarted() )
	{
		LoadConfig();

		if( !forceNoQueryConfig )
		{
			g_mainFrame->OpenSettings(wxT("memory:kiosk.lib"));
		}
		else
		{
			DoStart();
		}
	}
}


static bool s_kioskInShutdown = FALSE;
bool SjKioskModule::ExitRequest(long flag, const wxString* givenPassword, bool forceSimpleExit)
{
	// check if the kiosk is really started
	if( !KioskStarted() )
	{
		return false; // nothing to do
	}

	// a "simple exit" may be forced by a script
	if( forceSimpleExit )
	{
		DoExit(true/*restoreWindow*/);
		return true;
	}

	// check if exiting the kiosk mode this way is okay
	if( flag != 0 && !(m_configKioskf & flag) )
	{
		return false; // this exit way is disabled by the user
	}

	// init the exit action
	SjShutdownEtc   exitAction = m_configDefExitAction;
	long            dlgFlags = 0;

	// password required?
	if( m_configKioskf & SJ_KIOSKF_USE_PASSWORD )
	{
		if( givenPassword == NULL )
		{
			dlgFlags |= SJ_PASSWORDDLG_ASK_FOR_PASSWORD;
		}
		else
		{
			if( m_configUserPassword.Lower() == givenPassword->Lower() )
			{
				;
			}
			else if( m_configMaintenancePassword.Lower() == givenPassword->Lower() && !m_configMaintenancePassword.IsEmpty() )
			{
#define MAINTENANCE_ACTION SJ_SHUTDOWN_EXIT_KIOSK_MODE
				exitAction = MAINTENANCE_ACTION;
			}
			else
			{
				return false; // invalid password, although case was ignored;
				// (we ignore the case as the given password comes
				// eg. from the numpad control text input which
				// does not support to enter upper-case characters)
			}
		}
	}

	// ask for exit action?
	if( exitAction == SJ_SHUTDOWN_ASK_FOR_ACTION )
	{
		dlgFlags |= SJ_PASSWORDDLG_ASK_FOR_ACTION;
	}

	// show dialog?
	if( dlgFlags )
	{
		wxWindowDisabler disabler(g_mainFrame);
		SjPasswordDlg dlg(g_mainFrame,
		                  dlgFlags | SJ_PASSWORDDLG_AUTOCLOSE,
		                  SJ_PROGRAM_NAME + wxString(wxT(" ")) + _("Kiosk mode"),
		                  _("Please enter your password to exit the kiosk mode."),
		                  m_configUserPassword,
		                  m_configMaintenancePassword);

		g_kioskModule->m_passwordDlgOpened = TRUE;
		dlg.ShowModal();
		g_kioskModule->m_passwordDlgOpened = FALSE;

		if( !dlg.IsOkClicked() )
		{
			return false; // cancel clicked
		}

		if( (dlgFlags & SJ_PASSWORDDLG_ASK_FOR_PASSWORD)
		        &&  dlg.IsPasswordOk() == 0 )
		{
			return false; // invalid password
		}

		if( (dlgFlags & SJ_PASSWORDDLG_ASK_FOR_ACTION) )
		{
			exitAction = dlg.GetExitAction();
		}

		if( dlg.IsPasswordOk() == 2 )
		{
			exitAction = MAINTENANCE_ACTION; // maintenance password entered - just exit the kiosk mode
		}
	}

	// okay: really exit
	if( exitAction == SJ_SHUTDOWN_EXIT_SILVERJUKE
	        || exitAction == SJ_SHUTDOWN_POWEROFF_COMPUTER
	        || exitAction == SJ_SHUTDOWN_REBOOT_COMPUTER
	  )
	{
		s_kioskInShutdown = TRUE;
		DoExit(FALSE/*restoreWindow*/);
		SjMainApp::DoShutdownEtc(exitAction);
	}
	else if( exitAction>=SJ_SHUTDOWN_USER_DEF_SCRIPT && exitAction<=SJ_SHUTDOWN_USER_DEF_SCRIPT_LAST )
	{
		SjMainApp::DoShutdownEtc(exitAction);
	}
	else
	{
		DoExit(true/*restoreWindow*/);
	}

	return true;
}


void SjKioskModule::DoStart()
{
	wxBusyCursor    busy;
	bool            doCreateBlackFrames = true;
	wxArrayLong     displaysUsed;

	// (/)  Shutdown request before?
	//      (this additional check is needed as here may be
	//      eventually pending key presses and we do not want to rely on sth. else for this
	//      critical stuff)
	if( s_kioskInShutdown )
	{
		return;
	}

	wxLogInfo(wxT("Starting kiosk mode"));

	wxASSERT( !KioskStarted() );
	wxASSERT( m_configLoaded );

	// (1)  begin starting kiosk mode
	m_interface->m_moduleSystem->BroadcastMsg(IDMODMSG_KIOSK_STARTING);

	// (/)  stop vis. - if needed it is started again later
	g_visModule->UnprepareWindow();

	// (2)  set shuffle / repeat (this is not restored on leaving kiosk mode)
	g_mainFrame->SetShuffle((m_configKioskf&SJ_KIOSKF_SHUFFLE)!=0);

	if( !(m_configOp&SJ_OP_REPEAT) )
		g_mainFrame->SetRepeat(SJ_REPEAT_OFF); // if repeat cannot be changed in kiosk mode, set it off

	// (3a) switch display modes
	size_t displayCount = wxDisplay::GetCount();
	m_backupMainFrameRect = g_mainFrame->GetRect();
	{
		int i;
		long switchDone = -1;
		for( i = 0; i < 2; i++ )
		{
			m_backupVideoModeValid[i] = false;
			if( m_configKioskf & (i==0? SJ_KIOSKF_SWITCHRES_0 : SJ_KIOSKF_SWITCHRES_1) )
			{
				if( m_configMonitor[i] >= 0
				        && m_configMonitor[i] < (long)displayCount
				        && m_configMonitor[i] != switchDone )
				{
					wxDisplay display(m_configMonitor[i]);
					wxArrayVideoModes modes = display.GetModes();
					int modeIndex, mCount = modes.GetCount();
					for( modeIndex = 0; modeIndex < mCount; modeIndex++ )
					{
						if( getVideoModeName(modes[modeIndex]) == m_configDispRes[i] )
						{
							m_backupVideoMode[i] = display.GetCurrentMode();
							if( m_backupVideoMode[i].IsOk() )
							{
								if( display.ChangeMode(modes[modeIndex]) )
								{
									m_backupVideoModeValid[i] = true;
									switchDone = m_configMonitor[i]; // avoid switching the res. of the same monitor twice
								}
							}
							break;
						}
					}
				}
			}
		}
	}

	// (3b) set exclusive / always on top
	if( m_configKioskf&(SJ_KIOSKF_DISABLE_AT|SJ_KIOSKF_DISABLE_CAD) )
	{
		m_backupAlwaysOnTop = g_mainFrame->IsAlwaysOnTop();

#ifndef __WXDEBUG__
		g_mainFrame->SetWindowStyle(g_mainFrame->GetWindowStyle() | wxSTAY_ON_TOP);
		// we don't do this in debug mode as an
#endif                                  // always-on-top windows makes debugging almost impossible
	}

#ifndef __WXMAC__ // on mac, this must be very last !
	SetExclusive(true);
#endif

	// (4)  set search / deselect all rows
	if( g_mainFrame->m_columnMixer.IsAnythingSelected() )
	{
		g_mainFrame->m_columnMixer.SelectAll(FALSE);
		g_mainFrame->m_browser->RefreshSelection();
	}

	{
		bool searchSet = FALSE;
		if( m_configKioskf & SJ_KIOSKF_LIMIT_TO_ADV_SEARCH )
		{
			SjAdvSearch advSearch = g_advSearchModule->GetSearchById(m_configLimitToAdvSearch);
			if( advSearch.GetId() )
			{
				g_mainFrame->SetSearch(SJ_SETSEARCH_SETADV|SJ_SETSEARCH_CLEARSIMPLE, wxT(""), &advSearch);
				searchSet = TRUE;
			}
		}

		if( !searchSet )
		{
			g_mainFrame->SetSearch(SJ_SETSEARCH_CLEARADV|SJ_SETSEARCH_CLEARSIMPLE);
		}
	}

	// (5)  set kiosk operation flags in main frame (last)
	m_backupOp = g_mainFrame->m_availOp;
	g_mainFrame->m_availOp = m_configOp | SJ_OP_KIOSKON;
	if( g_mainFrame->m_availOp&SJ_OP_ALL )
	{
		g_mainFrame->m_availOp = 0x7FFFFFFFL;
	}

	// (-) check if the browser view is still available
	if( !g_mainFrame->m_browser->IsViewAvailable(g_mainFrame->m_browser->GetView()))
	{
		for( int test = 0; test < SJ_BROWSER_VIEW_COUNT; test ++ )
		{
			if( g_mainFrame->m_browser->IsViewAvailable(test) )
			{
				g_mainFrame->m_browser->SetView_(test, true, true);
				break;
			}
		}
	}

	// (6)  set layout
	m_backupLayout.Clear();
	SjSkinLayout* newLayout = g_mainFrame->GetLayout(wxT("kiosk"));
	if( newLayout )
	{
		SjSkinLayout* oldLayout = g_mainFrame->GetLayout();
		if( oldLayout )
		{
			m_backupLayout = oldLayout->GetName();
			g_mainFrame->LoadLayout(newLayout, SJ_NO_SIZE_CHANGE); // size is changed in ReloadSkin() below
		}
	}

	//  (7) set fullscreen / set size
	g_mainFrame->ReloadSkin(g_mainFrame->m_availOp, false/*reloadScripts*/);

	m_backupFullScreen = g_mainFrame->IsFullScreen();

	wxString rectStr;
	if( g_tools->ReadFromCmdLineOrIni(wxT("kioskrect"), rectStr) )
	{
		wxArrayLong arr = SjTools::ExplodeLong(rectStr, ',', 2, 4);
		wxRect kioskRect;
		if( arr.GetCount()==2 || arr.GetCount()==3 /*3 if "clipmouse" is added*/ )
		{
			kioskRect.width = arr[0]; kioskRect.height = arr[1];
		}
		else
		{
			kioskRect.x = arr[0]; kioskRect.y = arr[1];
			kioskRect.width = arr[2]; kioskRect.height = arr[3];
		}
		g_mainFrame->SetSize(kioskRect);

		if( rectStr.AfterLast(',') == wxT("clipmouse") )
			ClipMouse(&kioskRect);

		doCreateBlackFrames = false;
	}
	else
	{
		if( displayCount > 1
		        && m_configMonitor[0]>=0 && m_configMonitor[0]<(long)displayCount )
		{
			// make sure, the window is on the correct display before calling ShowFullScreen()
			wxDisplay displ(m_configMonitor[0]);
			wxRect geom = displ.GetGeometry();
			if( wxDisplay::GetFromWindow(g_mainFrame) != m_configMonitor[0] )
			{
				g_mainFrame->SetSize(geom);
				wxASSERT( wxDisplay::GetFromWindow(g_mainFrame) == m_configMonitor[0] );
			}

			if( !g_mainFrame->IsAllAvailable() )
			{
#ifndef __WXDEBUG__
				ClipMouse(&geom); // makes debugging impossible ...
#endif
			}
		}

		g_mainFrame->ShowFullScreen(true);
		displaysUsed.Add(wxDisplay::GetFromWindow(g_mainFrame));
	}

	// (/) create the vis. window and the black frames
	if( g_tools->ReadFromCmdLineOrIni(wxT("visrect"), rectStr) )
	{
		wxArrayLong arr = SjTools::ExplodeLong(rectStr, ',', 2, 4);
		wxRect visRect;
		if( arr.GetCount()==2 )
		{
			visRect.width = arr[0]; visRect.height = arr[1];
		}
		else
		{
			visRect.x = arr[0]; visRect.y = arr[1];
			visRect.width = arr[2]; visRect.height = arr[3];
		}
		g_visModule->PrepareWindow(0, &visRect);

		doCreateBlackFrames = false;
	}
	else if( m_configMonitor[1] != m_configMonitor[0]
	         && m_configMonitor[1] >= 0 && m_configMonitor[1] < (long)displayCount )
	{
		wxDisplay displ(m_configMonitor[1]);
		if( displ.IsOk() )
		{
			g_visModule->PrepareWindow(m_configMonitor[1], NULL);
			displaysUsed.Add(m_configMonitor[1]);
		}
	}

	if( doCreateBlackFrames
	        && displayCount > 1 )
	{
		for( int d = 0; d < (int)displayCount; d++ )
		{
			if( displaysUsed.Index(d) == wxNOT_FOUND )
			{
				wxDisplay displ(d);
				wxRect rect = displ.GetGeometry();
				SjBlackFrame* blackFrame = new SjBlackFrame(NULL, -1,
				        wxT(""),
				        wxPoint(rect.x,rect.y), wxSize(rect.width,rect.height),
				        wxSTAY_ON_TOP | // <-- without this and with g_mainFrame instead of NULL as parent, the tastbar stays visible!
				        wxFRAME_NO_TASKBAR);
				blackFrame->ShowFullScreen(true);
				m_blackFrames.Add(blackFrame);
			}
		}
	}

	// (8)  done starting kiosk mode
	g_mainFrame->SetFocus();
	m_isStarted = TRUE;
	g_mainFrame->UpdateSearchInfo(0); // we use different descriptions for kiosk and non-kiosk mode

	// (9) start watching the time
	m_uptime.StartWatching();

	// (/) nearly done: start the vis. if it has an explicit window
	{
		bool visAutoStart;
		g_mainFrame->GetVisEmbedRect(NULL, NULL, &visAutoStart);
		if( g_visModule->IsWindowPrepared() || visAutoStart )
		{
			g_visModule->StartVis();
		}
	}

	// (/) Update accelerator table
	// (we need this only if we're *not* using the numpad outside the kiosk - otherwise
	// the accelerator table is already correct)
	m_backupResetAccel = false;
	if(  (g_accelModule->m_flags&SJ_ACCEL_USE_NUMPAD_IN_KIOSK)
	        && !(g_accelModule->m_flags&SJ_ACCEL_USE_NUMPAD_OUT_KIOSK) )
	{
		m_backupResetAccel = true;
		g_mainFrame->SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_MAIN));
	}

	// (/) change cursor
	if( g_virtKeybd->GetKeybdFlags()&SJ_VIRTKEYBD_HIDE_CURSOR )
	{
		g_mainFrame->SetCursor(SjVirtKeybdModule::GetStandardCursor());
	}

	// (/) set exclusive on Mac (must be last, the flags are checked by SetExclusive())
#ifdef __WXMAC__
	SetExclusive(true);
#endif

	m_interface->m_moduleSystem->BroadcastMsg(IDMODMSG_KIOSK_STARTED);
}


void SjKioskModule::DoExit(bool restoreWindow)
{
	wxLogInfo(wxT("Ending kiosk mode"));

	m_interface->m_moduleSystem->BroadcastMsg(IDMODMSG_KIOSK_ENDING);

	wxBusyCursor busy;

	ClipMouse(NULL);

	wxASSERT( KioskStarted() );

	// (X) reset exclusivity on OS X (must be first, the flags are checked by SetExclusive())
#ifdef __WXMAC__
	SetExclusive(false);
#endif

	// (9) stop watching the time
	m_uptime.StopWatching();

	// (5)  restore kiosk operation flags in main frame (first)
	g_mainFrame->m_availOp = m_backupOp & ~SJ_OP_KIOSKON;

	// (/)  stop vis., delete black frames
	g_visModule->UnprepareWindow();
	for( int i = 0; i < (int)m_blackFrames.GetCount(); i++ )
	{
		wxWindowList::Node *node;
		wxWindow* toDestroy = (wxWindow*)m_blackFrames.Item(i);
		for ( node = wxTopLevelWindows.GetFirst(); node; node = node->GetNext() )
		{
			if( node->GetData() == toDestroy )
			{
				toDestroy->Hide();
				toDestroy->Destroy();
				break;
			}
		}
	}
	m_blackFrames.Clear();

	// (3a) restore display modes
	{
		int i;
		for( i = 0; i < 2; i++ )
		{
			if( m_backupVideoModeValid[i] )
			{
				wxDisplay display(m_configMonitor[i]);
				display.ChangeMode(m_backupVideoMode[i]);
			}
		}
	}

	// (3b) restore exclusive / always on top
#ifndef __WXMAC__
	SetExclusive(FALSE);
#endif
	if( m_configKioskf&(SJ_KIOSKF_DISABLE_AT|SJ_KIOSKF_DISABLE_CAD) )
	{
		if( !m_backupAlwaysOnTop )
		{
			g_mainFrame->SetWindowStyle(g_mainFrame->GetWindowStyle() & ~wxSTAY_ON_TOP);
		}
	}

	if( restoreWindow )
	{
		// (7)  restore fullscreen
		g_mainFrame->ShowFullScreen(m_backupFullScreen);
		g_mainFrame->SetSize(m_backupMainFrameRect);

		g_mainFrame->ReloadSkin(SJ_OP_DEF_NONKIOSK, false/*reloadScripts*/, SJ_FORCE_SIZE_CHANGE);


		// (6)  restore layout
		if( !m_backupLayout.IsEmpty() )
		{
			SjSkinLayout* layout = g_mainFrame->GetLayout(m_backupLayout);
			if( layout )
			{
				g_mainFrame->LoadLayout(layout, SJ_FORCE_SIZE_CHANGE);
			}
		}
	}

	// (4)  restore search

	;

	// (2)  restore shuffle / repeat
	//      we do not restore this as otherwise the playlist gets "out of order"
	//      if one simply want to change some settings and go then back to kiosk mode again

	;

	// (1)  done exiting kiosk mode
	m_isStarted = FALSE;
	g_mainFrame->UpdateSearchInfo(0); // we use different descriptions for kiosk and non-kiosk mode
	m_interface->m_moduleSystem->BroadcastMsg(IDMODMSG_KIOSK_ENDED);

	// (/) restore accel
	if( m_backupResetAccel && g_mainFrame && g_accelModule )
	{
		g_mainFrame->SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_MAIN));
	}

	// (/) restore cursor
	if( g_virtKeybd->GetKeybdFlags()&SJ_VIRTKEYBD_HIDE_CURSOR )
	{
		g_mainFrame->SetCursor(SjVirtKeybdModule::GetStandardCursor());
	}
}



/*******************************************************************************
 * Can enqueue?
 ******************************************************************************/


bool SjKioskModule::CanEnqueue(const wxArrayString& requestedUrls, bool urlsVerified)
{
	// kiosk started?
	if( !m_isStarted )
	{
		return true; // kiosk not started, enqueing is always okay
	}

	// check the number of waiting tracks
	long requestedUrlsCount = requestedUrls.GetCount();
	if( m_configKioskf&SJ_KIOSKF_MAX_TRACKS_IN_QUEUE )
	{
		long unplayedCount = g_mainFrame->m_player.m_queue.GetWaitingCount(m_configMaxTracksInQueue);
		if( unplayedCount >= m_configMaxTracksInQueue )
		{
			g_mainFrame->SetDisplayMsg(_("Too many tracks in queue,\nplease try again later."), SDM_KIOSK_CANNOT_ENQUEUE_MS);
			return false; // too many wating tracks
		}
	}

	// check for double tracks
	if( m_configKioskf&SJ_KIOSKF_NO_DBL_TRACKS )
	{
		unsigned long estimatedPlayingTime = SjTools::GetMsTicks() + g_mainFrame->m_player.GetEnqueueTime();

		SjPlaylist tempPlaylist;
		tempPlaylist.Add(requestedUrls, urlsVerified);
		wxASSERT( requestedUrlsCount == tempPlaylist.GetCount() );
		for( long i = 0; i < requestedUrlsCount; i++ )
		{
			SjPlaylistEntry& entry = tempPlaylist.Item(i);
			wxArrayLong allQueuePos;
			if( g_mainFrame->GetAllQueuePosByUrl(entry.GetUrl(), allQueuePos, true/*unplayedOnly*/) > 0 )
			{
				g_mainFrame->SetDisplayMsg(_("This track is already in queue,\nplease try again later."), SDM_KIOSK_CANNOT_ENQUEUE_MS);
				return false;
			}

			if( g_mainFrame->m_player.m_queue.IsBoring(entry.GetLeadArtistName(), entry.GetTrackName(), estimatedPlayingTime) )
			{
				g_mainFrame->SetDisplayMsg(_("This track or artist was just played,\nplease try again later."), SDM_KIOSK_CANNOT_ENQUEUE_MS);
				return false;
			}
		}
	}

	// check the used credit systems
	return m_creditBase.CheckAndDecreaseCredit(requestedUrlsCount);
}


void SjKioskModule::UpdateCreditSpinCtrl()
{
	if( g_kioskConfigPage
	        && g_kioskConfigPage->m_creditCurrSpinCtrl )
	{
		long oldValue = g_kioskConfigPage->m_creditCurrSpinCtrl->GetValue();
		long newValue = m_creditBase.GetCredit();
		if( newValue != oldValue )
		{
			g_kioskConfigPage->m_creditCurrSpinCtrl->SetValue(newValue);
		}
	}
}


/*******************************************************************************
 * Some "little options"
 ******************************************************************************/


class SjLittlePw : public SjLittleOption
{
public:
	SjLittlePw(const wxString& name, const wxString& password)
		: SjLittleOption(name)
	{
		m_password = password;
	}

	wxString GetDisplayValue() const { if(m_password.IsEmpty())return _("Off"); return wxString(wxT('*'), m_password.Len());}

	bool IsModified() const { return !m_password.IsEmpty(); }
	bool OnDefault(wxWindow* parent) { m_password.Clear(); return FALSE; }

	long GetOptionCount() const { return 1; }
	wxString GetOption(long i) const { return _("Edit..."); }
	bool OnOption(wxWindow* parent, long i) { return OnDoubleClick(parent); }
	bool OnDoubleClick(wxWindow* parent)
	{
		wxWindowDisabler disabler(parent);
		SjPasswordDlg mdlg1(parent, SJ_PASSWORDDLG_ASK_FOR_PASSWORD | SJ_PASSWORDDLG_ACCEPT_ANY_PASSWORD, GetName(), GetName(), wxT(""));
		mdlg1.ShowModal();
		if( mdlg1.IsPasswordOk() )
		{
			wxString newPassword = mdlg1.GetEnteredPassword();
			SjPasswordDlg mdlg2(parent, SJ_PASSWORDDLG_ASK_FOR_PASSWORD, GetName(), _("Please enter the password again for verification."), newPassword);
			mdlg2.ShowModal();
			if( mdlg2.IsPasswordOk() )
			{
				m_password = newPassword; // may be empty now!
			}
		}

		return false; // other options are not affected
	}

	virtual wxString GetPassword() const {return m_password;}

private:
	wxString m_password;
};


class SjLittleMaintenancePw : public SjLittlePw
{
public:
	SjLittleMaintenancePw()
		: SjLittlePw(_("Maintenance password"), g_kioskModule->m_configMaintenancePassword)
	{
	}

	void OnApply()
	{
		if( g_kioskModule )
		{
			g_kioskModule->m_configMaintenancePassword = GetPassword();
			g_kioskModule->SaveConfig();
		}
	}
};


void SjKioskModule::GetLittleOptions(SjArrayLittleOption& lo)
{
	SjLittleOption::SetSection(_("Kiosk mode"));

	LoadConfig();
	lo.Add(new SjLittleMaintenancePw());
}
