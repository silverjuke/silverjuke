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
 * File:    playbacksettings.cpp
 * Authors: Björn Petersen
 * Purpose: The "Playback settings" module
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/statline.h>
#include <wx/spinctrl.h>
#include <wx/notebook.h>
#include <sjmodules/playbacksettings.h>
#include <sjmodules/advsearch.h>
#include <sjmodules/settings.h>
#include <sjmodules/kiosk/kiosk.h>
#include <sjmodules/kiosk/password_dlg.h>
#include <sjbase/browser.h>


/*******************************************************************************
 * SjEffectDlg - the base for some common dialogs
 ******************************************************************************/


class SjEffectDlg : public SjDialog
{
public:
	                SjEffectDlg         (wxWindow* parent, const wxString& title);
	void            StartStateTimer     (long ms=200);
	void            UpdateStateText     (int id, const wxString&);
	virtual void    UpdateState         () { }

private:
	void            OnStateTimer           (wxTimerEvent&) { UpdateState(); }
	wxTimer         m_stateTimer;
	DECLARE_EVENT_TABLE ()
};


BEGIN_EVENT_TABLE(SjEffectDlg, SjDialog)
	EVT_TIMER   (IDTIMER_EFFECTINFO,    SjEffectDlg::OnStateTimer   )
END_EVENT_TABLE()


SjEffectDlg::SjEffectDlg(wxWindow* parent, const wxString& title)
	: SjDialog(parent, title, SJ_MODELESS, SJ_NEVER_RESIZEABLE)
{
}


void SjEffectDlg::StartStateTimer(long ms)
{
	m_stateTimer.SetOwner(this, IDTIMER_EFFECTINFO);
	m_stateTimer.Start(ms);
}


void SjEffectDlg::UpdateStateText(int id, const wxString& state)
{
	wxStaticText* s = (wxStaticText*)FindWindow(id);
	if( s )
	{
		s->SetLabel(state);
	}
}


/*******************************************************************************
 * SjAutovolDlg
 ******************************************************************************/


class SjAutovolDlg : public SjEffectDlg
{
public:
	                SjAutovolDlg        (wxWindow* parent);
	static void     OpenDialog          (wxWindow* alignToWindow);
	static void     CloseDialog         ();
	static bool     IsDialogOpen        () { return s_dialog!=NULL; }

private:
	// enabled?
	bool            m_orgEnabled;
	wxCheckBox*     m_enabledCheck;
	void            OnEnable            (wxCommandEvent&);
	void            EnableDisable       ();

	// desired volume
	#define         DESVOL_MULTIPLIER   10.0F
	double          m_orgDesVol;
	wxSlider*       m_desVolSlider;
	wxStaticText*   m_desVolText;
	void            UpdateDesVol        (bool saveToGainTool);
	void            OnDesVolSlider      (wxScrollEvent&) { UpdateDesVol(TRUE); }

	// max. gain
	#define         MAXGAIN_MULTIPLIER  10.0F
	double          m_orgMaxGain;
	wxSlider*       m_maxGainSlider;
	wxStaticText*   m_maxGainText;
	void            UpdateMaxGain       (bool saveToGainTool);
	void            OnMaxGainSlider     (wxScrollEvent&) { UpdateMaxGain(TRUE); }

	// use album volume?
	bool            m_orgUseAlbumVol;
	wxCheckBox*     m_useAlbumVolCheck;
	void            OnUseAlbumVol       (wxCommandEvent&);

	// misc
	wxButton*       m_resetButton;
	static SjAutovolDlg*
	s_dialog;
	void            OnReset             (wxCommandEvent&);
	void            UpdateState         ();
	void            OnMyOk              (wxCommandEvent&) { SjAutovolDlg::CloseDialog(); }
	void            OnMyCancel          (wxCommandEvent&);
	void            OnMyClose           (wxCloseEvent&) { SjAutovolDlg::CloseDialog(); }
	DECLARE_EVENT_TABLE ()
};


#define IDC_DESVOLSLIDER            (IDM_FIRSTPRIVATE+1)
#define IDC_MAXGAINSLIDER           (IDM_FIRSTPRIVATE+2)
#define IDC_AUTOVOLRESET            (IDM_FIRSTPRIVATE+4)
#define IDC_STATE_CURR_TRACK        (IDM_FIRSTPRIVATE+5)
#define IDC_USE_ALBUM_VOL           (IDM_FIRSTPRIVATE+10)
#define IDC_AUTO_VOL_ENABLED        (IDM_FIRSTPRIVATE+11)


BEGIN_EVENT_TABLE(SjAutovolDlg, SjEffectDlg)
	EVT_CHECKBOX        (IDC_AUTO_VOL_ENABLED,  SjAutovolDlg::OnEnable              )
	EVT_COMMAND_SCROLL  (IDC_DESVOLSLIDER,      SjAutovolDlg::OnDesVolSlider        )
	EVT_COMMAND_SCROLL  (IDC_MAXGAINSLIDER,     SjAutovolDlg::OnMaxGainSlider       )
	EVT_CHECKBOX        (IDC_USE_ALBUM_VOL,     SjAutovolDlg::OnUseAlbumVol         )
	EVT_BUTTON          (IDC_AUTOVOLRESET,      SjAutovolDlg::OnReset               )
	EVT_BUTTON          (wxID_OK,               SjAutovolDlg::OnMyOk                )
	EVT_BUTTON          (wxID_CANCEL,           SjAutovolDlg::OnMyCancel            )
	EVT_CLOSE           (                       SjAutovolDlg::OnMyClose             )
END_EVENT_TABLE()


SjAutovolDlg* SjAutovolDlg::s_dialog = NULL;


void SjAutovolDlg::OpenDialog(wxWindow* alignToWindow)
{
	if( IsDialogOpen() )
	{
		s_dialog->Raise();
	}
	else
	{
		s_dialog = new SjAutovolDlg(g_mainFrame);

		if( alignToWindow )
		{
			wxRect alignRect = alignToWindow->GetRect();
			s_dialog->SetSize(alignRect.x+32, alignRect.y+32, -1, -1);
		}
		else
		{
			s_dialog->CentreOnParent();
		}

		s_dialog->Show();
	}
}


void SjAutovolDlg::CloseDialog()
{
	if( IsDialogOpen() )
	{
		// destroy (not: delete) dialog
		s_dialog->Hide();
		s_dialog->Destroy();
		s_dialog = NULL;

		SjSettingsModule::RaiseIfOpen();
	}
}


SjAutovolDlg::SjAutovolDlg(wxWindow* parent)
	: SjEffectDlg(parent, _("Volume control"))
{
	// backup settings
	SjPlayer* player = &g_mainFrame->m_player;

	m_orgEnabled    = player->AvIsEnabled();
	m_orgDesVol     = player->AvGetDesiredVolume();
	m_orgMaxGain    = player->AvGetMaxGain();
	m_orgUseAlbumVol= player->AvGetUseAlbumVol();

	// create dialog
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	wxSizer* sizer2 = new wxStaticBoxSizer(new wxStaticBox(this, -1, _("Options")), wxVERTICAL);
	sizer1->Add(sizer2, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	// some space
	sizer2->Add(SJ_DLG_SPACE, SJ_DLG_SPACE);

	// enabled?
	m_enabledCheck = new wxCheckBox(this, IDC_AUTO_VOL_ENABLED, _("Volume control"));
	m_enabledCheck->SetValue(m_orgEnabled);
	sizer2->Add(m_enabledCheck, 0, wxALL, SJ_DLG_SPACE);

	// use album vol?
	m_useAlbumVolCheck = new wxCheckBox(this, IDC_USE_ALBUM_VOL, _("Play all tracks of an album with the same volume"));
	m_useAlbumVolCheck->SetValue(m_orgUseAlbumVol);
	sizer2->Add(m_useAlbumVolCheck, 0, wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	wxFlexGridSizer* sizer3 = new wxFlexGridSizer(3, SJ_DLG_SPACE/2, SJ_DLG_SPACE);
	sizer3->AddGrowableCol(1);
	sizer2->Add(sizer3, 0, wxGROW|wxALL, SJ_DLG_SPACE);

	// desired volume
	sizer3->Add(new wxStaticText(this, -1, _("Desired volume:")), 0, wxALIGN_CENTER_VERTICAL);

	long desVol = (long)(SjGain2Decibel(m_orgDesVol)*DESVOL_MULTIPLIER);
	m_desVolSlider = new wxSlider(this, IDC_DESVOLSLIDER,
	                              desVol,
	                              (int)( -12.0F*DESVOL_MULTIPLIER ),
	                              (int)( +12.0F*DESVOL_MULTIPLIER ),
	                              wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL);
	sizer3->Add(m_desVolSlider, 0, wxGROW|wxALIGN_CENTER_VERTICAL);

	m_desVolText = new wxStaticText(this, -1, wxEmptyString);
	UpdateDesVol(FALSE);
	sizer3->Add(m_desVolText, 0, wxALIGN_CENTER_VERTICAL);

	// max gain
	sizer3->Add(new wxStaticText(this, -1, _("Max. gain:")), 0, wxALIGN_CENTER_VERTICAL);

	long maxGain = (long)(SjGain2Decibel(m_orgMaxGain)*MAXGAIN_MULTIPLIER);
	m_maxGainSlider = new wxSlider(this, IDC_MAXGAINSLIDER,
	                               maxGain,
	                               (int)( + 1.0F*MAXGAIN_MULTIPLIER ),
	                               (int)( +27.0F*MAXGAIN_MULTIPLIER ),
	                               wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL);
	sizer3->Add(m_maxGainSlider, 0, wxGROW|wxALIGN_CENTER_VERTICAL);

	m_maxGainText = new wxStaticText(this, -1, wxEmptyString);
	UpdateMaxGain(FALSE);
	sizer3->Add(m_maxGainText, 0, wxALIGN_CENTER_VERTICAL);

	// reset button
	m_resetButton = new wxButton(this, IDC_AUTOVOLRESET, _("Reset to default values"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer2->Add(m_resetButton,
	            0, wxALL, SJ_DLG_SPACE);

	AddStateBox(sizer1);

	AddState(_("Current volume:"),  IDC_STATE_CURR_TRACK);

	UpdateState();
	StartStateTimer();

	sizer1->Add(CreateButtons(SJ_DLG_OK_CANCEL), 0, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	sizer1->SetSizeHints(this);

	EnableDisable();
}


void SjAutovolDlg::UpdateState()
{
	// curr track
	wxString str;
	if( g_mainFrame->IsStopped() || !g_mainFrame->m_player.AvIsEnabled() )
	{
		str = _("n/a");
	}
	else
	{
		str = SjTools::FormatGain(g_mainFrame->m_player.AvGetCalculatedGain());
	}
	UpdateStateText(IDC_STATE_CURR_TRACK, str);
}


void SjAutovolDlg::OnReset(wxCommandEvent&)
{
	m_desVolSlider->SetValue((int)( ::SjGain2Decibel(SJ_AV_DEF_DESIRED_VOLUME)*DESVOL_MULTIPLIER) );
	UpdateDesVol(TRUE);

	m_maxGainSlider->SetValue((int)( ::SjGain2Decibel(SJ_AV_DEF_MAX_GAIN)*MAXGAIN_MULTIPLIER ));
	UpdateMaxGain(TRUE);

	m_useAlbumVolCheck->SetValue(SJ_AV_DEF_USE_ALBUM_VOL);
	g_mainFrame->m_player.AvSetUseAlbumVol(SJ_AV_DEF_USE_ALBUM_VOL);

	g_mainFrame->ReplayIfPlaying();
}


void SjAutovolDlg::UpdateDesVol(bool saveToGainTool)
{
	double db   = (double)(m_desVolSlider->GetValue()) / DESVOL_MULTIPLIER;
	double gain = ::SjDecibel2Gain(db);

	m_desVolText->SetLabel(SjTools::FormatDecibel(db)+wxT("  "));

	if( saveToGainTool )
	{
		g_mainFrame->m_player.AvSetDesiredVolume(gain);
	}
}


void SjAutovolDlg::UpdateMaxGain(bool saveToGainTool)
{
	double db = (double)(m_maxGainSlider->GetValue()) / MAXGAIN_MULTIPLIER;
	double gain = ::SjDecibel2Gain(db);

	m_maxGainText->SetLabel(SjTools::FormatDecibel(db)+wxT("  "));

	if( saveToGainTool )
	{
		g_mainFrame->m_player.AvSetMaxGain(gain);
	}
}


void SjAutovolDlg::OnEnable(wxCommandEvent&)
{
	bool e = m_enabledCheck->IsChecked();
	g_mainFrame->m_player.AvEnable(e);
	EnableDisable();
}
void SjAutovolDlg::EnableDisable()
{
	bool e = m_enabledCheck->IsChecked();
	m_useAlbumVolCheck->Enable(e);
	m_maxGainSlider->Enable(e);
	m_desVolSlider->Enable(e);
	m_resetButton->Enable(e);
}


void SjAutovolDlg::OnUseAlbumVol(wxCommandEvent&)
{
	bool useAlbumVol = m_useAlbumVolCheck->IsChecked();

	g_mainFrame->m_player.AvSetUseAlbumVol(useAlbumVol);

	g_mainFrame->ReplayIfPlaying();
}


void SjAutovolDlg::OnMyCancel(wxCommandEvent&)
{
	// restore original settings
	SjPlayer* player = &g_mainFrame->m_player;
	player->AvEnable(m_orgEnabled);
	player->AvSetDesiredVolume(m_orgDesVol);
	player->AvSetMaxGain(m_orgMaxGain);
	player->AvSetUseAlbumVol(m_orgUseAlbumVol);

	g_mainFrame->ReplayIfPlaying();

	// Close dialog, this calls Destroy(), do not used delete!
	SjAutovolDlg::CloseDialog();
}


/*******************************************************************************
 * SjCrossfadeDlg
 ******************************************************************************/


#define IDC_MAN_CROSSFADE_SLIDER        (IDM_FIRSTPRIVATE+1)
#define IDC_AUTO_CROSSFADE_ENABLED      (IDM_FIRSTPRIVATE+2)
#define IDC_AUTO_CROSSFADE_SLIDER       (IDM_FIRSTPRIVATE+3)
#define IDC_CROSSFADE_OFFSET_END_SLIDER (IDM_FIRSTPRIVATE+4)
#define IDC_SKIP_SILENCE                (IDM_FIRSTPRIVATE+5)
#define IDC_SUBSEQ_DETECT               (IDM_FIRSTPRIVATE+6)
#define IDC_ONLY_FADE_OUT               (IDM_FIRSTPRIVATE+7)

#define IDC_RESET_CROSSFADE             (IDM_FIRSTPRIVATE+10)


class SjCrossfadeDlg : public SjEffectDlg
{
public:
	                SjCrossfadeDlg      (wxWindow* parent);
	static void     OpenDialog          (wxWindow* alignToWindow);
	static void     CloseDialog         ();
	static bool     IsDialogOpen        () { return s_dialog!=NULL; }

private:
	// auto and manual crossfades
	SjDlgSlider     m_manCrossfadeMsSlider;
	long            m_manCrossfadeMsOrg;
	void            OnManCrossfadeMsSlider  (wxScrollEvent&) { m_manCrossfadeMsSlider.Update(); g_mainFrame->m_player.m_manCrossfadeMs = m_manCrossfadeMsSlider.GetValue(); }

	wxCheckBox*     m_autoCrossfadeCheck;
	bool            m_autoCrossfadeOrgEnabled;
	void            OnAutoCrossfadeEnable(wxCommandEvent&);

	SjDlgSlider     m_autoCrossfadeMsSlider;
	long            m_autoCrossfadeMsOrg;
	void            OnAutoCrossfadeMsSlider (wxScrollEvent&) { m_autoCrossfadeMsSlider.Update(); g_mainFrame->m_player.m_autoCrossfadeMs = m_autoCrossfadeMsSlider.GetValue(); }

	SjDlgSlider     m_crossfadeOffsetEndMsSlider;
	long            m_crossfadeOffsetEndMsOrg;
	void            OnCrossfadeOffsetEndMsSlider (wxScrollEvent&) { m_crossfadeOffsetEndMsSlider.Update(); g_mainFrame->m_player.m_crossfadeOffsetEndMs = m_crossfadeOffsetEndMsSlider.GetValue(); }

	// crossfade options
	wxCheckBox*     m_subseqDetectCheck;
	bool            m_subseqDetectOrg;
	void            OnSubseqDetect(wxCommandEvent&);

	wxCheckBox*     m_onlyFadeOutCheck;
	bool            m_orgOnlyFadeOut;
	void            OnOnlyFadeOut(wxCommandEvent&);

	// misc
	static SjCrossfadeDlg* s_dialog;
	void            EnableDisable       ();
	void            OnCrossfadeReset    (wxCommandEvent&);
	void            OnFastfadeReset     (wxCommandEvent&);
	void            OnMyOk              (wxCommandEvent&) { SjCrossfadeDlg::CloseDialog(); }
	void            OnMyCancel          (wxCommandEvent&);
	void            OnMyClose           (wxCloseEvent&) { SjCrossfadeDlg::CloseDialog(); }
	                DECLARE_EVENT_TABLE ()
};


BEGIN_EVENT_TABLE(SjCrossfadeDlg, SjEffectDlg)
	// crossfade
	EVT_COMMAND_SCROLL      (IDC_MAN_CROSSFADE_SLIDER,      SjCrossfadeDlg::OnManCrossfadeMsSlider      )
	EVT_CHECKBOX            (IDC_AUTO_CROSSFADE_ENABLED,    SjCrossfadeDlg::OnAutoCrossfadeEnable       )
	EVT_COMMAND_SCROLL      (IDC_AUTO_CROSSFADE_SLIDER,     SjCrossfadeDlg::OnAutoCrossfadeMsSlider     )
	EVT_COMMAND_SCROLL      (IDC_CROSSFADE_OFFSET_END_SLIDER, SjCrossfadeDlg::OnCrossfadeOffsetEndMsSlider )
	EVT_CHECKBOX            (IDC_SUBSEQ_DETECT,             SjCrossfadeDlg::OnSubseqDetect              )
	EVT_CHECKBOX            (IDC_ONLY_FADE_OUT,             SjCrossfadeDlg::OnOnlyFadeOut               )
	EVT_BUTTON              (IDC_RESET_CROSSFADE,           SjCrossfadeDlg::OnCrossfadeReset            )

	// misc.
	EVT_BUTTON              (wxID_OK,                       SjCrossfadeDlg::OnMyOk                      )
	EVT_BUTTON              (wxID_CANCEL,                   SjCrossfadeDlg::OnMyCancel                  )
	EVT_CLOSE               (                               SjCrossfadeDlg::OnMyClose                   )
END_EVENT_TABLE()


SjCrossfadeDlg* SjCrossfadeDlg::s_dialog = NULL;


void SjCrossfadeDlg::OpenDialog(wxWindow* alignToWindow)
{
	if( IsDialogOpen() )
	{
		s_dialog->Raise();
	}
	else
	{
		s_dialog = new SjCrossfadeDlg(g_mainFrame);

		if( alignToWindow )
		{
			wxRect alignRect = alignToWindow->GetRect();
			s_dialog->SetSize(alignRect.x+32, alignRect.y+32, -1, -1);
		}
		else
		{
			s_dialog->CentreOnParent();
		}

		s_dialog->Show();
	}
}


void SjCrossfadeDlg::CloseDialog()
{
	if( IsDialogOpen() )
	{
		// destroy (not: delete) dialog
		s_dialog->Hide();
		s_dialog->Destroy();
		s_dialog = NULL;

		SjSettingsModule::RaiseIfOpen();
	}
}


SjCrossfadeDlg::SjCrossfadeDlg(wxWindow* parent)
	: SjEffectDlg(parent, _("Fading"))
{
	SjPlayer* player = &g_mainFrame->m_player;

	// save settings
	m_manCrossfadeMsOrg             = player->m_manCrossfadeMs;
	m_autoCrossfadeOrgEnabled       = player->GetAutoCrossfade();
	m_autoCrossfadeMsOrg            = player->m_autoCrossfadeMs;
	m_crossfadeOffsetEndMsOrg       = player->m_crossfadeOffsetEndMs;

	m_subseqDetectOrg               = player->GetAutoCrossfadeSubseqDetect();
	m_orgOnlyFadeOut                = player->GetOnlyFadeOut();

	// create dialog
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	wxSizer* sizer2 = new wxStaticBoxSizer(new wxStaticBox(this, -1, _("Crossfading")), wxVERTICAL);
	sizer1->Add(sizer2, 0, wxGROW|wxALL, SJ_DLG_SPACE);

	sizer2->Add(SJ_DLG_SPACE, SJ_DLG_SPACE);

	wxFlexGridSizer* sizer3 = new wxFlexGridSizer(3, SJ_DLG_SPACE/2, SJ_DLG_SPACE);
	sizer3->AddGrowableCol(1);
	sizer2->Add(sizer3, 0, wxLEFT|wxTOP|wxRIGHT, SJ_DLG_SPACE);

	// crossfade lengths
	sizer3->Add(new wxStaticText(this, -1, _("Manual crossfades:")), 0, wxALIGN_CENTER_VERTICAL);

	m_manCrossfadeMsSlider.Create(this, sizer3, IDC_MAN_CROSSFADE_SLIDER, SJ_SLIDER_MS_SEC,
	                              m_manCrossfadeMsOrg, 0, 30000);

	m_autoCrossfadeCheck = new wxCheckBox(this, IDC_AUTO_CROSSFADE_ENABLED, _("Automatic crossfades:"));
	m_autoCrossfadeCheck->SetValue(m_autoCrossfadeOrgEnabled);
	sizer3->Add(m_autoCrossfadeCheck, 0, wxALIGN_CENTER_VERTICAL);

	m_autoCrossfadeMsSlider.Create(this, sizer3, IDC_AUTO_CROSSFADE_SLIDER, SJ_SLIDER_MS_SEC,
	                               m_autoCrossfadeMsOrg, 0, 30000);

	// offsets
	sizer3->Add(new wxStaticText(this, -1, _("Offset to end:")), 0, wxALIGN_CENTER_VERTICAL);

	m_crossfadeOffsetEndMsSlider.Create(this, sizer3, IDC_CROSSFADE_OFFSET_END_SLIDER, SJ_SLIDER_MS_SEC,
	                              m_crossfadeOffsetEndMsOrg, 0, 30000);

	sizer2->Add(SJ_DLG_SPACE/2, SJ_DLG_SPACE/2);

	// subsequent detect
	m_subseqDetectCheck = new wxCheckBox(this, IDC_SUBSEQ_DETECT, _("No crossfades between subsequent tracks of the same album"));
	m_subseqDetectCheck->SetValue(m_subseqDetectOrg);
	sizer2->Add(m_subseqDetectCheck, 0, wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	// only fade out?
	m_onlyFadeOutCheck = new wxCheckBox(this, IDC_ONLY_FADE_OUT, _("Only fade out the old track, the new track starts with full volume"));
	m_onlyFadeOutCheck->SetValue(m_orgOnlyFadeOut);
	sizer2->Add(m_onlyFadeOutCheck, 0, wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	wxButton* button = new wxButton(this, IDC_RESET_CROSSFADE, _("Reset to default values"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer2->Add(button, 0, wxALL, SJ_DLG_SPACE);

	sizer1->Add(CreateButtons(SJ_DLG_OK_CANCEL), 0, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	sizer1->SetSizeHints(this);

	EnableDisable();
}


void SjCrossfadeDlg::OnCrossfadeReset(wxCommandEvent&)
{
	// reset auto-enabled
	m_autoCrossfadeCheck->SetValue(true);
	g_mainFrame->m_player.SetAutoCrossfade(true);

	// reset ms
	m_manCrossfadeMsSlider.SetValue(SJ_DEF_CROSSFADE_MS);
	g_mainFrame->m_player.m_manCrossfadeMs = SJ_DEF_CROSSFADE_MS;

	m_autoCrossfadeMsSlider.SetValue(SJ_DEF_CROSSFADE_MS);
	g_mainFrame->m_player.m_autoCrossfadeMs = SJ_DEF_CROSSFADE_MS;

	// reset offset
	m_crossfadeOffsetEndMsSlider.SetValue(SJ_DEF_CROSSFADE_OFFSET_END_MS);
	g_mainFrame->m_player.m_crossfadeOffsetEndMs = SJ_DEF_CROSSFADE_OFFSET_END_MS;

	// reset "only fade out"
	m_onlyFadeOutCheck->SetValue(false);
	g_mainFrame->m_player.SetOnlyFadeOut(false);

	// reset "no subsequent fadings"
	m_subseqDetectCheck->SetValue(false);
	g_mainFrame->m_player.SetAutoCrossfadeSubseqDetect(false);

	EnableDisable();
}


void SjCrossfadeDlg::EnableDisable()
{
	bool e = m_autoCrossfadeCheck->IsChecked();
	m_autoCrossfadeMsSlider.Enable(e);
	m_crossfadeOffsetEndMsSlider.Enable(e);
	m_subseqDetectCheck->Enable(e);
}


void SjCrossfadeDlg::OnAutoCrossfadeEnable(wxCommandEvent&)
{
	bool e = m_autoCrossfadeCheck->IsChecked();
	g_mainFrame->m_player.SetAutoCrossfade(e);
	EnableDisable();
}


void SjCrossfadeDlg::OnOnlyFadeOut(wxCommandEvent&)
{
	bool e = m_onlyFadeOutCheck->IsChecked();
	g_mainFrame->m_player.SetOnlyFadeOut(e);
}


void SjCrossfadeDlg::OnSubseqDetect(wxCommandEvent&)
{
	bool subseqDetect = m_subseqDetectCheck->IsChecked();
	g_mainFrame->m_player.SetAutoCrossfadeSubseqDetect(subseqDetect);
}


void SjCrossfadeDlg::OnMyCancel(wxCommandEvent&)
{
	// restore original settings
	g_mainFrame->m_player.SetAutoCrossfade(m_autoCrossfadeOrgEnabled);
	g_mainFrame->m_player.SetAutoCrossfadeSubseqDetect(m_subseqDetectOrg);
	g_mainFrame->m_player.m_manCrossfadeMs = m_manCrossfadeMsOrg;
	g_mainFrame->m_player.m_autoCrossfadeMs = m_autoCrossfadeMsOrg;
	g_mainFrame->m_player.m_crossfadeOffsetEndMs = m_crossfadeOffsetEndMsOrg;
	g_mainFrame->m_player.SetOnlyFadeOut(m_orgOnlyFadeOut);

	// Close dialog, this calls Destroy(), do not used delete!
	SjCrossfadeDlg::CloseDialog();
}


/*******************************************************************************
 * SjPlaybackSettingsConfigPage - Common
 ******************************************************************************/


class SjModuleNames : public wxArrayString
{
public:
	SjModule*       GetModule           (int i);
	wxString        GetName             (int i);
	int             Index               (SjModule*);
	void            SortNames           ();
};


SjModule* SjModuleNames::GetModule(int i)
{
	if( i < 0 || i >= (int)GetCount() ) return NULL;
	wxString moduleName;
	int      moduleIndex;
	SjModule::UnpackFileName(Item(i), moduleName, moduleIndex);
	return g_mainFrame->m_moduleSystem.FindModuleByFile(moduleName, moduleIndex); // may be NULL!
}


wxString SjModuleNames::GetName(int i)
{
	SjModule* m = GetModule(i);
	if( m == NULL )
	{
		return wxT("");
	}
	return m->m_name;
}


int SjModuleNames::Index(SjModule* m)
{
	int i, iCount = (int)GetCount();
	for( i = 0; i < iCount; i++ )
	{
		if( GetModule(i)==m )
		{
			return i;
		}
	}
	return -1;
}


static wxString SjModuleNames_GetName(const wxString& packedFileName)
{
	wxString moduleName;
	int      moduleIndex;
	SjModule::UnpackFileName(packedFileName, moduleName, moduleIndex);
	SjModule* m = g_mainFrame->m_moduleSystem.FindModuleByFile(moduleName, moduleIndex); // may be NULL!
	if( m )
	{
		return m->m_name;
	}
	else
	{
		return wxT("");
	}
}
static int SjModuleNames_SortNames(const wxString& s1, const wxString& s2)
{
	return SjNormaliseString(SjModuleNames_GetName(s1), SJ_NUM_SORTABLE)
	       .Cmp(SjNormaliseString(SjModuleNames_GetName(s2), SJ_NUM_SORTABLE));
}
void SjModuleNames::SortNames()
{
	Sort(SjModuleNames_SortNames);
}


class SjPlaybackSettingsConfigPage : public wxPanel
{
public:
	                SjPlaybackSettingsConfigPage (SjPlaybackSettingsModule*, wxWindow* parent, int selectedPage);
					~SjPlaybackSettingsConfigPage();

private:
	// Queue page
	wxPanel*        CreateQueuePage     (wxWindow* parent);
	int             m_queuePage;
	void            CloseQueueNResumePage(bool apply, bool& needsReplay);
	SjDlgCheckCtrl  m_maxTracks;
	SjDlgCheckCtrl  m_boredomTracks;
	SjDlgCheckCtrl  m_boredomArtists;
	wxSlider*       m_shuffleSlider;
	wxStaticText*   m_shuffleText;
	wxCheckBox*     m_noDblTracksCheck;
	wxCheckBox*     m_removePlayedCheck;
	void            OnEnableDisableCheck(wxCommandEvent&) { EnableDisable(); }
	void            OnShuffleSlider     (wxScrollEvent&) { UpdateShuffleText(); }
	void            OnQueueReset        (wxCommandEvent&);
	void            UpdateShuffleText   ();
	void            EnableDisable       ();

	// Resume page
	wxPanel*        CreateResumePage    (wxWindow* parent);
	int             m_resumePage;
	wxCheckBox*     m_resumeCheck;
	wxCheckBox*     m_resumeLoadPlayedCheck;
	wxCheckBox*     m_resumeStartPlaybackCheck;
	void            EnableDisableResume ();
	void            OnEnableDisableResumeCheck(wxCommandEvent&) { EnableDisableResume(); }

	// AutoCtrl page
	wxPanel*        CreateAutoCtrlPage  (wxWindow* parent);
	int             m_autoCtrlPage;
	void            CloseAutoCtrlPage   (bool apply, bool& needsReplay);
	SjDlgCheckCtrl  m_autoPlayWait;
	wxSpinCtrl*     m_autoPlayTracks;
	wxChoice*       m_autoPlayMusicSelChoice;
	wxButton*       m_autoPlayMusicSelButton;
	wxCheckBox*     m_autoPlayMusicSelIgnoreCheck;
	wxChoice*       m_autoPlayMusicSelIgnoreChoice;
	wxButton*       m_autoPlayMusicSelIgnoreButton;
	wxCheckBox*     m_autoPlayManEnqInterrupt;
	wxCheckBox*     m_sleepCheckBox;
	wxChoice*       m_sleepActionChoice;
	wxChoice*       m_sleepTimemodeChoice;
	long            m_sleepTimemodeCurr;
	wxTextCtrl*     m_sleepMinutesTextCtrl;
	wxStaticText*   m_sleepMinUnitStatic;
	wxArrayString   m_sleepMinUnitStrings;
	SjDlgCheckCtrl  m_sleepFade;
	void            OnAutoCtrlCheckE    (wxCommandEvent&) { UpdateAutoCtrlChecksE(); }
	void            OnMusicSelButton    (wxCommandEvent&);
	void            OnAutoCtrlSleepTimemode(wxCommandEvent&);
	void            UpdateAutoCtrlChecksE();
	static wxString FormatMinutes       (long minutes, long mode);
	static long     ParseMinutes        (const wxString&);

	void            OnTransitionButton  (wxCommandEvent&);
	void            OnAutoVolButton     (wxCommandEvent&);

	// Jingle page
	wxPanel*        CreateJinglePage    (wxWindow* parent);
	int             m_jinglePage;
	void            CloseJinglePage     (bool apply, bool& needsReplay);
	wxCheckBox*     m_jingleEveryCheck;
	wxSpinCtrl*     m_jingleEveryMinutesSpin;
	wxChoice*       m_jingleEveryMusicSelChoice;
	wxButton*       m_jingleEveryMusicSelButton;
	wxCheckBox*     m_jingleAtCheck[SJ_JINGLE_AT_CNT];
	wxTextCtrl*     m_jingleAtTextCtrl[SJ_JINGLE_AT_CNT];
	wxChoice*       m_jingleAtMusicSelChoice[SJ_JINGLE_AT_CNT];
	wxButton*       m_jingleAtMusicSelButton[SJ_JINGLE_AT_CNT];
	wxCheckBox*     m_jingleAtDailyCheck[SJ_JINGLE_AT_CNT];
	wxCheckBox*     m_jingleAtWaitCheck[SJ_JINGLE_AT_CNT];
	void            EnableDisableJingle();
	void            OnEnableDisableJngl (wxCommandEvent&) { EnableDisableJingle(); }

	// Further options page
	wxPanel*        CreateFurtherOptPage(wxWindow* parent);
	int             m_furtherOptPage;
	void            CloseFurtherOptPage (bool apply, bool& needsReplay);
	SjDlgCheckCtrl  m_limitPlayTime;
	SjDlgCheckCtrl  m_autoFollowPlaylist;
	SjDlgCheckCtrl  m_autoResetView;
	wxChoice*       m_autoResetViewTo;
	SjDlgCheckCtrl  m_autoStartVis;
	SjDlgCheckCtrl  m_autoStopVis;
	void			UpdateFurtherOptChecksM();
	void            OnFurtherOptCheckM  (wxCommandEvent&) { UpdateFurtherOptChecksM(); }
	void            OnFurtherOptReset   (wxCommandEvent&);

	// Common
	SjPlaybackSettingsModule* m_playbackSettingsModule;
	wxNotebook*     m_notebook;

	void            CloseAll            (bool apply);
	void            Cancel              ();

	wxString        ShortenModuleName   (const wxString& str);

	DECLARE_EVENT_TABLE ()

	friend class    SjPlaybackSettingsModule;
};


#define IDC_TRANSITION_BUTTON           (IDM_FIRSTPRIVATE+2)
#define IDC_AUTOVOLBUTTON               (IDM_FIRSTPRIVATE+3)
#define IDC_SHUFFLESLIDER               (IDM_FIRSTPRIVATE+10)
#define IDC_QUEUERESET                  (IDM_FIRSTPRIVATE+11)
#define IDC_TRACKSINQUEUECHECK          (IDM_FIRSTPRIVATE+12)
#define IDC_BOREDOMTRACKSCHECK          (IDM_FIRSTPRIVATE+13)
#define IDC_BOREDOMARTISTSCHECK         (IDM_FIRSTPRIVATE+14)

#define IDC_RESUME                      (IDM_FIRSTPRIVATE+18)

#define IDC_AUTOPLAY                    (IDM_FIRSTPRIVATE+23)
#define IDC_AUTOPLAYMUSICSELBUT         (IDM_FIRSTPRIVATE+24)
#define IDC_AUTOPLAYFXCHECK             (IDM_FIRSTPRIVATE+25)
#define IDC_AUTOPLAYFXBUTTON            (IDM_FIRSTPRIVATE+26)
#define IDC_AUTOPLAYIGNORECHECK         (IDM_FIRSTPRIVATE+27)
#define IDC_AUTOPLAYIGNOREBUTTON        (IDM_FIRSTPRIVATE+28)
#define IDC_AUTOPLAY_MAN_ENQ_INTERRUPT  (IDM_FIRSTPRIVATE+29)
#define IDC_SLEEPCHECKBOX               (IDM_FIRSTPRIVATE+30)
#define IDC_SLEEPTIMEMODECHOICE         (IDM_FIRSTPRIVATE+31)
#define IDC_SLEEPFADE                   (IDM_FIRSTPRIVATE+33)

#define IDC_JINGLEEVERYCHECK            (IDM_FIRSTPRIVATE+40)
#define IDC_JINGLEEVERYMUSICSELBUTTON   (IDM_FIRSTPRIVATE+41)
#define IDC_JINGLEAT0CHECK              (IDM_FIRSTPRIVATE+42)
#define IDC_JINGLEAT1CHECK              (IDM_FIRSTPRIVATE+43)
#define IDC_JINGLEAT0MUSICSELBUTTON     (IDM_FIRSTPRIVATE+44)
#define IDC_JINGLEAT1MUSICSELBUTTON     (IDM_FIRSTPRIVATE+45)

#define IDC_LIMIT_PLAY_TIME             (IDM_FIRSTPRIVATE+49)
#define IDC_FOLLOW_PLAYLIST             (IDM_FIRSTPRIVATE+50)
#define IDC_AUTO_START_VIS              (IDM_FIRSTPRIVATE+51)
#define IDC_AUTO_STOP_VIS               (IDM_FIRSTPRIVATE+52)
#define IDC_RESET_VIEW                  (IDM_FIRSTPRIVATE+53)
#define IDC_RESET_VIEW_TO               (IDM_FIRSTPRIVATE+54)
#define IDC_FURTHEROPTRESET             (IDM_FIRSTPRIVATE+55)


BEGIN_EVENT_TABLE(SjPlaybackSettingsConfigPage, wxPanel)
	EVT_COMMAND_SCROLL          (IDC_SHUFFLESLIDER,             SjPlaybackSettingsConfigPage::OnShuffleSlider           )
	EVT_BUTTON                  (IDC_QUEUERESET,                SjPlaybackSettingsConfigPage::OnQueueReset              )
	EVT_CHECKBOX                (IDC_TRACKSINQUEUECHECK,        SjPlaybackSettingsConfigPage::OnEnableDisableCheck      )
	EVT_CHECKBOX                (IDC_BOREDOMTRACKSCHECK,        SjPlaybackSettingsConfigPage::OnEnableDisableCheck      )
	EVT_CHECKBOX                (IDC_BOREDOMARTISTSCHECK,       SjPlaybackSettingsConfigPage::OnEnableDisableCheck      )

	EVT_CHECKBOX                (IDC_RESUME,                    SjPlaybackSettingsConfigPage::OnEnableDisableResumeCheck)

	EVT_CHECKBOX                (IDC_AUTOPLAY,                  SjPlaybackSettingsConfigPage::OnAutoCtrlCheckE          )
	EVT_BUTTON                  (IDC_AUTOPLAYMUSICSELBUT,       SjPlaybackSettingsConfigPage::OnMusicSelButton          )
	EVT_CHECKBOX                (IDC_AUTOPLAYIGNORECHECK,       SjPlaybackSettingsConfigPage::OnAutoCtrlCheckE          )
	EVT_BUTTON                  (IDC_AUTOPLAYIGNOREBUTTON,      SjPlaybackSettingsConfigPage::OnMusicSelButton          )
	EVT_CHECKBOX                (IDC_AUTOPLAY_MAN_ENQ_INTERRUPT,SjPlaybackSettingsConfigPage::OnAutoCtrlCheckE          )

	EVT_CHECKBOX                (IDC_SLEEPCHECKBOX,             SjPlaybackSettingsConfigPage::OnAutoCtrlCheckE          )
	EVT_CHOICE                  (IDC_SLEEPTIMEMODECHOICE,       SjPlaybackSettingsConfigPage::OnAutoCtrlSleepTimemode   )
	EVT_CHECKBOX                (IDC_SLEEPFADE,                 SjPlaybackSettingsConfigPage::OnAutoCtrlCheckE          )

	EVT_CHECKBOX                (IDC_JINGLEEVERYCHECK,          SjPlaybackSettingsConfigPage::OnEnableDisableJngl       )
	EVT_CHECKBOX                (IDC_JINGLEAT0CHECK,            SjPlaybackSettingsConfigPage::OnEnableDisableJngl       )
	EVT_CHECKBOX                (IDC_JINGLEAT1CHECK,            SjPlaybackSettingsConfigPage::OnEnableDisableJngl       )
	EVT_BUTTON                  (IDC_JINGLEEVERYMUSICSELBUTTON, SjPlaybackSettingsConfigPage::OnMusicSelButton          )
	EVT_BUTTON                  (IDC_JINGLEAT0MUSICSELBUTTON,   SjPlaybackSettingsConfigPage::OnMusicSelButton          )
	EVT_BUTTON                  (IDC_JINGLEAT1MUSICSELBUTTON,   SjPlaybackSettingsConfigPage::OnMusicSelButton          )

	EVT_BUTTON                  (IDC_TRANSITION_BUTTON,         SjPlaybackSettingsConfigPage::OnTransitionButton        )
	EVT_BUTTON                  (IDC_AUTOVOLBUTTON,             SjPlaybackSettingsConfigPage::OnAutoVolButton           )

	EVT_CHECKBOX                (IDC_LIMIT_PLAY_TIME,           SjPlaybackSettingsConfigPage::OnFurtherOptCheckM        )
	EVT_CHECKBOX                (IDC_FOLLOW_PLAYLIST,           SjPlaybackSettingsConfigPage::OnFurtherOptCheckM        )
	EVT_CHECKBOX                (IDC_RESET_VIEW,                SjPlaybackSettingsConfigPage::OnFurtherOptCheckM        )
	EVT_CHECKBOX                (IDC_AUTO_START_VIS,            SjPlaybackSettingsConfigPage::OnFurtherOptCheckM        )
	EVT_CHECKBOX                (IDC_AUTO_STOP_VIS,             SjPlaybackSettingsConfigPage::OnFurtherOptCheckM        )
	EVT_BUTTON                  (IDC_FURTHEROPTRESET,           SjPlaybackSettingsConfigPage::OnFurtherOptReset         )

END_EVENT_TABLE()


static SjPlaybackSettingsConfigPage* g_playbackConfigPage = NULL;


SjPlaybackSettingsConfigPage::SjPlaybackSettingsConfigPage(SjPlaybackSettingsModule* module, wxWindow* parent, int selectedPage)
	: wxPanel(parent)
{
	// save given objects
	m_playbackSettingsModule= module;

	// create notebook
	wxSizer* dialogSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(dialogSizer);

	m_notebook = new wxNotebook(this, IDC_NOTEBOOK, wxDefaultPosition, wxDefaultSize, 0/*wxCLIP_CHILDREN - problems with wxChoice/wxComboBox*/);

	wxNotebook* notebookSizer = m_notebook;

	m_notebook->AddPage(CreateQueuePage(m_notebook),  _("Queue"));
	m_queuePage = m_notebook->GetPageCount();

	m_notebook->AddPage(CreateResumePage(m_notebook),  _("Resume"));
	m_resumePage = m_notebook->GetPageCount();

	m_notebook->AddPage(CreateAutoCtrlPage(m_notebook),  _("Automatic control"));
	m_autoCtrlPage = m_notebook->GetPageCount();

	m_notebook->AddPage(CreateJinglePage(m_notebook),  _("Jingles"));
	m_jinglePage = m_notebook->GetPageCount();

	m_notebook->AddPage(CreateFurtherOptPage(m_notebook),  _("Further options"));
	m_furtherOptPage = m_notebook->GetPageCount();

	if( selectedPage<0 || selectedPage >= (int)m_notebook->GetPageCount() ) selectedPage = 0;
	m_notebook->SetSelection(selectedPage);

	dialogSizer->Add(notebookSizer, 1, wxALL|wxGROW, SJ_DLG_SPACE);

	// done
	dialogSizer->SetSizeHints(this);
	g_playbackConfigPage = this;

	// set accelerators
	SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_PLAYBACKSETTINGS));
}


SjPlaybackSettingsConfigPage::~SjPlaybackSettingsConfigPage()
{
	g_playbackConfigPage = NULL;
}


wxString SjPlaybackSettingsConfigPage::ShortenModuleName(const wxString& str__)
{
	wxString str(str__);

	if( str.Len() > 32 )
	{
		str = str.Left(30).Trim();
		if( str.Find(' ', TRUE/*from end*/)>10 ) str = str.BeforeLast(' ');
		str += wxT("..");
	}

	return str;
}


void SjPlaybackSettingsConfigPage::CloseAll(bool apply)
{
	bool needsReplay = FALSE;

	// apply settings
	CloseQueueNResumePage(apply, needsReplay);
	CloseAutoCtrlPage    (apply, needsReplay);
	CloseJinglePage      (apply, needsReplay);
	CloseFurtherOptPage  (apply, needsReplay);

	// save settings to disk
	if( apply )
	{
		g_mainFrame->m_autoCtrl.SaveAutoCtrlSettings();
	}
	g_mainFrame->m_player.SaveSettings();

	// auto play settings change may need an update of the prev/next/display ...
	if( apply )
	{
		g_mainFrame->UpdateDisplay();
	}

	// restart playback?
	if( needsReplay )
	{
		g_mainFrame->ReplayIfPlaying();
	}

	g_tools->m_config->Flush();
}


/*******************************************************************************
 * SjPlaybackSettingsConfigPage - Queue Page
 ******************************************************************************/


wxPanel* SjPlaybackSettingsConfigPage::CreateQueuePage(wxWindow* parent)
{
	// create dialog
	wxPanel* page = new wxPanel(parent, -1);
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);

	sizer1->Add(1, SJ_DLG_SPACE); // some space

	// top hints
	wxStaticText* staticText = new wxStaticText(page, -1, _("The following options define the behaviour of the queue. For the \"shuffle intensity\",\nsmall values create only slight variations of the queue order, whereas large values\ncause major variations."));
	sizer1->Add(staticText,
	            0, wxALL, SJ_DLG_SPACE);

	// max. tracks in queue

	g_kioskModule->LoadConfig();

	m_maxTracks.Create(page, sizer1,
	                   // TRANSLATORS: %i will be replaced by a number
	                   wxPLURAL("Kiosk mode: Allow max. %i track waiting in queue", "Kiosk mode: Allow max. %i tracks waiting in queue", g_kioskModule->m_configMaxTracksInQueue),
	                   wxLEFT|wxRIGHT|wxTOP,
	                   IDC_TRACKSINQUEUECHECK, (g_kioskModule->m_configKioskf&SJ_KIOSKF_MAX_TRACKS_IN_QUEUE)!=0,
	                   -1, g_kioskModule->m_configMaxTracksInQueue, 1, 999);

	sizer1->Add(2, 2); // some min. space

	// avoid double tracks

	m_noDblTracksCheck = new wxCheckBox(page, -1, _("Kiosk mode: Avoid double tracks waiting in queue"));
	m_noDblTracksCheck->SetValue((g_kioskModule->m_configKioskf&SJ_KIOSKF_NO_DBL_TRACKS)!=0);
	sizer1->Add(m_noDblTracksCheck, 0, wxLEFT|wxRIGHT, SJ_DLG_SPACE);

	// remove played tracks

	long queueFlags, boredomTrackMinutes, boredomArtistMinutes;
	g_mainFrame->m_player.m_queue.GetQueueFlags(queueFlags, boredomTrackMinutes, boredomArtistMinutes);

	m_removePlayedCheck = new wxCheckBox(page, -1, _("Remove played tracks from queue"));
	m_removePlayedCheck->SetValue((queueFlags&SJ_QUEUEF_REMOVE_PLAYED)!=0);
	sizer1->Add(m_removePlayedCheck, 0, wxLEFT|wxRIGHT|wxTOP, SJ_DLG_SPACE);

	// avoid boredom

	sizer1->Add(2, 2); // some min. space

	m_boredomTracks.Create(page, sizer1,
	                       _("Avoid boredom: No track repetition within %i minutes"), wxLEFT|wxRIGHT,
	                       IDC_BOREDOMTRACKSCHECK, (queueFlags&SJ_QUEUEF_BOREDOM_TRACKS)!=0,
	                       -1, boredomTrackMinutes, 1, 999);

	sizer1->Add(2, 2); // some min. space

	m_boredomArtists.Create(page, sizer1,
	                        _("Avoid boredom: No artist repetition within %i minutes"), wxLEFT|wxRIGHT,
	                        IDC_BOREDOMARTISTSCHECK, (queueFlags&SJ_QUEUEF_BOREDOM_ARTISTS)!=0,
	                        -1, boredomArtistMinutes, 1, 999);

	sizer1->Add(2, 2); // some min. space

	// shuffle morph rate

	wxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2, 0, wxLEFT|wxRIGHT|wxGROW, SJ_DLG_SPACE);

	sizer2->Add(new wxStaticText(page, -1, _("Shuffle intensity:")), 0, wxALIGN_CENTER_VERTICAL);

	m_shuffleSlider = new wxSlider(page, IDC_SHUFFLESLIDER,
	                               g_mainFrame->m_player.m_queue.GetShuffleIntensity(),
	                               1,
	                               100,
	                               wxDefaultPosition, wxSize(180, -1), wxSL_HORIZONTAL);
	sizer2->Add(m_shuffleSlider, 0/*grow*/, wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);

	m_shuffleText = new wxStaticText(page, -1, wxT("100%"));
	sizer2->Add(m_shuffleText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);


	// reset button
	wxButton* button = new wxButton(page, IDC_QUEUERESET, _("Reset to default values"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer1->Add(button,
	            0, wxALL, SJ_DLG_SPACE);

	page->SetSizer(sizer1);

	EnableDisable();
	UpdateShuffleText();

	return page;
}


void SjPlaybackSettingsConfigPage::UpdateShuffleText()
{
	long v = m_shuffleSlider->GetValue();
	m_shuffleText->SetLabel(wxString::Format(wxT("%i%%"), (int)v));
}


void SjPlaybackSettingsConfigPage::EnableDisable()
{
	m_maxTracks.Update();
	m_boredomTracks.Update();
	m_boredomArtists.Update();
}


void SjPlaybackSettingsConfigPage::CloseQueueNResumePage(bool apply, bool& needsReplay)
{
	if( apply )
	{
		long queueFlags, boredomTrackMinutes, boredomArtistMinutes;
		g_mainFrame->m_player.m_queue.GetQueueFlags(queueFlags, boredomTrackMinutes, boredomArtistMinutes);

		// max. tracks / avoid double tracks
		SjTools::SetFlag(g_kioskModule->m_configKioskf, SJ_KIOSKF_MAX_TRACKS_IN_QUEUE, m_maxTracks.IsChecked());
		g_kioskModule->m_configMaxTracksInQueue = m_maxTracks.GetValue();

		SjTools::SetFlag(g_kioskModule->m_configKioskf, SJ_KIOSKF_NO_DBL_TRACKS, m_noDblTracksCheck->IsChecked());
		SjTools::SetFlag(queueFlags, SJ_QUEUEF_REMOVE_PLAYED, m_removePlayedCheck->IsChecked());

		// queue flags / avoid boredom
		SjTools::SetFlag(queueFlags, SJ_QUEUEF_BOREDOM_TRACKS, m_boredomTracks.IsChecked());
		boredomTrackMinutes = m_boredomTracks.GetValue();
		SjTools::SetFlag(queueFlags, SJ_QUEUEF_BOREDOM_ARTISTS, m_boredomArtists.IsChecked());
		boredomArtistMinutes = m_boredomArtists.GetValue();

		// shuffle intensity
		g_mainFrame->m_player.m_queue.SetShuffleIntensity(m_shuffleSlider->GetValue());

		// stuff from the "Resume" page (also saved via SetQueueFlags() and we do not want to call the function twice)
		SjTools::SetFlag(queueFlags, SJ_QUEUEF_RESUME, m_resumeCheck->IsChecked());
		SjTools::SetFlag(queueFlags, SJ_QUEUEF_RESUME_LOAD_PLAYED, m_resumeLoadPlayedCheck->IsChecked());
		SjTools::SetFlag(queueFlags, SJ_QUEUEF_RESUME_START_PLAYBACK, m_resumeStartPlaybackCheck->IsChecked());

		// save stuff now
		g_mainFrame->m_player.m_queue.SetQueueFlags(queueFlags, boredomTrackMinutes, boredomArtistMinutes);
		g_kioskModule->SaveConfig();
	}
}


void SjPlaybackSettingsConfigPage::OnQueueReset(wxCommandEvent&)
{
	// max. tracks / avoid double tracks
	m_maxTracks.SetChecked((SJ_KIOSKF_DEFAULT&SJ_KIOSKF_MAX_TRACKS_IN_QUEUE)!=0);
	m_maxTracks.SetValue(SJ_DEF_MAX_TRACKS_IN_QUEUE);

	m_noDblTracksCheck->SetValue((SJ_KIOSKF_DEFAULT&SJ_KIOSKF_NO_DBL_TRACKS)!=0);
	m_removePlayedCheck->SetValue((SJ_QUEUEF_DEFAULT&SJ_QUEUEF_REMOVE_PLAYED)!=0);

	// avoid boredom
	m_boredomTracks.SetChecked((SJ_QUEUEF_DEFAULT&SJ_QUEUEF_BOREDOM_TRACKS)!=0);
	m_boredomTracks.SetValue(SJ_DEF_BOREDOM_TRACK_MINUTES);

	m_boredomArtists.SetChecked((SJ_QUEUEF_DEFAULT&SJ_QUEUEF_BOREDOM_ARTISTS)!=0);
	m_boredomArtists.SetValue(SJ_DEF_BOREDOM_ARTIST_MINUTES);

	// shuffle intensity
	m_shuffleSlider->SetValue(SJ_DEF_SHUFFLE_INTENSITY);
	UpdateShuffleText();
}


/*******************************************************************************
 * SjPlaybackSettingsConfigPage - Resume
 ******************************************************************************/


wxPanel* SjPlaybackSettingsConfigPage::CreateResumePage(wxWindow* parent)
{
	long queueFlags, boredomTrackMinutes, boredomArtistMinutes;
	g_mainFrame->m_player.m_queue.GetQueueFlags(queueFlags, boredomTrackMinutes, boredomArtistMinutes);

	// create dialog
	wxPanel* page = new wxPanel(parent, -1);
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);

	sizer1->Add(1, SJ_DLG_SPACE); // some space

	// hints
	wxStaticText* staticText = new wxStaticText(page, -1,
	        _(  "Here you can decide whether to restore the queue after a restart of Silverjuke.\nYou can restore all or only unplayed titles."
	         ));
	sizer1->Add(staticText,
	            0, wxALL, SJ_DLG_SPACE);

	m_resumeCheck = new wxCheckBox(page, IDC_RESUME, _("Restore queue"));
	m_resumeCheck->SetValue((queueFlags&SJ_QUEUEF_RESUME)!=0);
	sizer1->Add(m_resumeCheck, 0, wxLEFT|wxRIGHT|wxTOP, SJ_DLG_SPACE);

	sizer1->Add(SJ_DLG_SPACE, SJ_DLG_SPACE);

	m_resumeLoadPlayedCheck = new wxCheckBox(page, -1, _("Restore already played tracks"));
	m_resumeLoadPlayedCheck->SetValue((queueFlags&SJ_QUEUEF_RESUME_LOAD_PLAYED)!=0);
	sizer1->Add(m_resumeLoadPlayedCheck, 0, wxLEFT, SJ_DLG_SPACE*6);

	sizer1->Add(SJ_DLG_SPACE, SJ_DLG_SPACE);

	m_resumeStartPlaybackCheck = new wxCheckBox(page, -1, _("Start playback from last position"));
	m_resumeStartPlaybackCheck->SetValue((queueFlags&SJ_QUEUEF_RESUME_START_PLAYBACK)!=0);
	sizer1->Add(m_resumeStartPlaybackCheck, 0, wxLEFT, SJ_DLG_SPACE*6);

	EnableDisableResume();

	page->SetSizer(sizer1);
	return page;
}


void SjPlaybackSettingsConfigPage::EnableDisableResume()
{
	bool e = m_resumeCheck->IsChecked();
	m_resumeLoadPlayedCheck->Enable(e);
	m_resumeStartPlaybackCheck->Enable(e);
}


/*******************************************************************************
 * SjPlaybackSettingsConfigPage - AutoCtrl
 ******************************************************************************/


wxPanel* SjPlaybackSettingsConfigPage::CreateAutoCtrlPage(wxWindow* parent)
{
	// make sure, the "ignore" music selection is just fine (otherweise UpdateAdvSearchChoice() would select the "most recent" music selection)
	if( g_advSearchModule && g_advSearchModule->GetSearchById(g_mainFrame->m_autoCtrl.m_autoPlayMusicSelIgnoreId, false).GetId() == 0 )
	{
		g_mainFrame->m_autoCtrl.m_autoPlayMusicSelIgnoreId = g_advSearchModule->GetSearchByName(_("Worst rated")).GetId();
	}

	// create dialog
	long flags = g_mainFrame->m_autoCtrl.m_flags;
	wxPanel* page = new wxPanel(parent, -1);
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);

	sizer1->Add(1, SJ_DLG_SPACE); // some space

	// hints
	wxStaticText* staticText = new wxStaticText(page, -1,
	        _(  "Use the following options to define which actions should be done automatically\nafter given timeouts."
	         ));
	sizer1->Add(staticText,
	            0, wxALL, SJ_DLG_SPACE);

	// auto play...

	m_autoPlayWait.Create(page, sizer1,
	                      _("AutoPlay: If the playlist is empty, wait %i minutes;"), wxLEFT|wxTOP|wxRIGHT,
	                      IDC_AUTOPLAY, (flags&SJ_AUTOCTRL_AUTOPLAY_ENABLED)!=0,
	                      -1, g_mainFrame->m_autoCtrl.m_autoPlayWaitMinutes, SJ_AUTOCTRL_MIN_AUTOPLAYWAITMINUTES, SJ_AUTOCTRL_MAX_AUTOPLAYWAITMINUTES);

	sizer1->Add(2, 2); // some min. space

	// auto play: num tracks / music selection

	wxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2, 0, wxLEFT|wxRIGHT, SJ_DLG_SPACE);


	wxString optionText = _("then, play %i tracks from the %s");

	sizer2->Add(SJ_DLG_SPACE*6, SJ_DLG_SPACE);

	sizer2->Add(new wxStaticText(page, -1, optionText.BeforeFirst('%').Trim()),
	            0, wxALIGN_CENTER_VERTICAL);
	optionText = optionText.AfterFirst('%').Mid(1).Trim(FALSE);

	// auto play: num tracks

	int numTracks = g_mainFrame->m_autoCtrl.m_autoPlayNumTracks;
	m_autoPlayTracks = new wxSpinCtrl(page, -1, wxString::Format(wxT("%i"), (int)numTracks),
	                                  wxDefaultPosition, wxSize(SJ_3DIG_SPINCTRL_W, -1), wxSP_ARROW_KEYS,
	                                  SJ_AUTOCTRL_MIN_AUTOPLAYNUMTRACKS, SJ_AUTOCTRL_MAX_AUTOPLAYNUMTRACKS, numTracks);
	sizer2->Add(m_autoPlayTracks, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, SJ_DLG_SPACE);

	sizer2->Add(new wxStaticText(page, -1, optionText.BeforeFirst('%').Trim()),
	            0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

	// auto play: music selection

	m_autoPlayMusicSelChoice = new wxChoice(page, -1, wxDefaultPosition, wxSize(200, -1));
	sizer2->Add(m_autoPlayMusicSelChoice, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE/2);

	SjAdvSearchModule::UpdateAdvSearchChoice(m_autoPlayMusicSelChoice,
	        g_mainFrame->m_autoCtrl.m_autoPlayMusicSelId,
	        _("Music selection")+wxString(wxT(" \"%s\"")), _("Current view"));
	SjDialog::SetCbWidth(m_autoPlayMusicSelChoice, 200);

	m_autoPlayMusicSelButton = new wxButton(page, IDC_AUTOPLAYMUSICSELBUT, wxT("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer2->Add(m_autoPlayMusicSelButton, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);

	sizer2->Add(new wxStaticText(page, -1, optionText.AfterFirst('%').Mid(1).Trim(FALSE)),
	            0, wxALIGN_CENTER_VERTICAL);

	sizer1->Add(2, 2); // some min. space

	// auto play: ignore list

	sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2, 0, wxLEFT|wxRIGHT, SJ_DLG_SPACE);

	sizer2->Add(SJ_DLG_SPACE*6, SJ_DLG_SPACE);

	m_autoPlayMusicSelIgnoreCheck = new wxCheckBox(page, IDC_AUTOPLAYIGNORECHECK,
	        _("Ignore tracks from the music selection"));
	m_autoPlayMusicSelIgnoreCheck->SetValue((flags & SJ_AUTOCTRL_AUTOPLAY_IGNORE)!=0);
	sizer2->Add(m_autoPlayMusicSelIgnoreCheck, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

	m_autoPlayMusicSelIgnoreChoice = new wxChoice(page, -1, wxDefaultPosition, wxSize(200, -1));
	sizer2->Add(m_autoPlayMusicSelIgnoreChoice, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE/2);

	SjAdvSearchModule::UpdateAdvSearchChoice(m_autoPlayMusicSelIgnoreChoice,
	        g_mainFrame->m_autoCtrl.m_autoPlayMusicSelIgnoreId);
	SjDialog::SetCbWidth(m_autoPlayMusicSelIgnoreChoice, 200);

	m_autoPlayMusicSelIgnoreButton = new wxButton(page, IDC_AUTOPLAYIGNOREBUTTON, wxT("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer2->Add(m_autoPlayMusicSelIgnoreButton, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);

	sizer1->Add(6, 6); // some min. space (little more as the next line is taller)

	// auto play: manual enqueued tracks will interrupt

	sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2, 0, wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	sizer2->Add(SJ_DLG_SPACE*6, SJ_DLG_SPACE);

	m_autoPlayManEnqInterrupt = new wxCheckBox(page, IDC_AUTOPLAY_MAN_ENQ_INTERRUPT,
	        _("Manually enqueued tracks interrupt AutoPlay immediately"));
	m_autoPlayManEnqInterrupt->SetValue((flags & SJ_AUTOCTRL_AUTOPLAY_MAN_ENQ_INTERRUPT)!=0);
	sizer2->Add(m_autoPlayManEnqInterrupt, 0, wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);

	sizer1->Add(new wxStaticLine(page, -1), 0, wxGROW|wxLEFT|wxRIGHT|wxTOP|wxBOTTOM, SJ_DLG_SPACE);

	// auto play: sleep mode

	{
		bool sleepEnabled; SjShutdownEtc sleepAction; bool sleepDoFade; long sleepFadeSeconds, sleepMinutesCurr;
		g_mainFrame->m_autoCtrl.GetSleepSettings(sleepEnabled, sleepAction, m_sleepTimemodeCurr, sleepMinutesCurr, sleepDoFade, sleepFadeSeconds);


		sizer2 = new wxBoxSizer(wxHORIZONTAL);
		sizer1->Add(sizer2, 0, wxLEFT|wxTOP|wxRIGHT|wxGROW, SJ_DLG_SPACE);

		m_sleepCheckBox = new wxCheckBox(page, IDC_SLEEPCHECKBOX, _("Sleep mode:"));
		m_sleepCheckBox->SetValue(sleepEnabled);
		sizer2->Add(m_sleepCheckBox, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

		m_sleepActionChoice = new wxChoice(page, -1);
		m_sleepActionChoice->Append(_("Stop playback"), (void*)SJ_SHUTDOWN_STOP_PLAYBACK);
		m_sleepActionChoice->Append(_("Clear playlist"), (void*)SJ_SHUTDOWN_CLEAR_PLAYLIST);
		SjPasswordDlg::InitExitActionChoice(m_sleepActionChoice);
		SjDialog::SetCbSelection(m_sleepActionChoice, sleepAction);
		SjDialog::SetCbWidth(m_sleepActionChoice, 140);
		sizer2->Add(m_sleepActionChoice, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

		m_sleepTimemodeChoice = new wxChoice(page, IDC_SLEEPTIMEMODECHOICE, wxDefaultPosition, wxSize(120, -1));
		wxArrayString modes = SjTools::Explode(_("in|after inactivity of|at|always in|always after inactivity of|always at"), '|', SJ_SLEEPMODE_TIMEMODE_BASIC_COUNT*2, SJ_SLEEPMODE_TIMEMODE_BASIC_COUNT*2);
		m_sleepTimemodeChoice->Append(modes[0], (void*)SJ_SLEEPMODE_TIMEMODE_IN);
		m_sleepTimemodeChoice->Append(modes[1], (void*)SJ_SLEEPMODE_TIMEMODE_AFTER);
		m_sleepTimemodeChoice->Append(modes[2], (void*)SJ_SLEEPMODE_TIMEMODE_AT);
		m_sleepTimemodeChoice->Append(modes[3], (void*)SJ_SLEEPMODE_TIMEMODE_ALWAYS_IN);
		m_sleepTimemodeChoice->Append(modes[4], (void*)SJ_SLEEPMODE_TIMEMODE_ALWAYS_AFTER);
		m_sleepTimemodeChoice->Append(modes[5], (void*)SJ_SLEEPMODE_TIMEMODE_ALWAYS_AT);
		SjDialog::SetCbSelection(m_sleepTimemodeChoice, m_sleepTimemodeCurr);
		sizer2->Add(m_sleepTimemodeChoice, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

		m_sleepMinutesTextCtrl = new wxTextCtrl(page, -1, FormatMinutes(sleepMinutesCurr, m_sleepTimemodeCurr),
		                                        wxDefaultPosition, wxSize(SJ_4DIG_SPINCTRL_W, -1));
		sizer2->Add(m_sleepMinutesTextCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

		wxArrayString temp = SjTools::Explode(_("minutes|o'clock"), '|', 2, 2);
		m_sleepMinUnitStrings.Clear();
		m_sleepMinUnitStrings.Add(temp[0]);
		m_sleepMinUnitStrings.Add(temp[0]);
		m_sleepMinUnitStrings.Add(temp[1]);
		m_sleepMinUnitStrings.Add(temp[0]);
		m_sleepMinUnitStrings.Add(temp[0]);
		m_sleepMinUnitStrings.Add(temp[1]);

		m_sleepMinUnitStatic = new wxStaticText(page, -1, m_sleepMinUnitStrings[m_sleepTimemodeChoice->GetSelection()],
		                                        wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
		sizer2->Add(m_sleepMinUnitStatic, 1, wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);


		sizer1->Add(2, 2); // some min. space

		sizer2 = new wxBoxSizer(wxHORIZONTAL);
		sizer1->Add(sizer2, 0, wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

		sizer2->Add(SJ_DLG_SPACE*6, SJ_DLG_SPACE);

		m_sleepFade.Create(page, sizer2,
		                   _("Before this, fade out %i seconds"), 0,
		                   IDC_SLEEPFADE, sleepDoFade,
		                   -1, sleepFadeSeconds, SJ_SLEEPMODE_MIN_FADE_SECONDS, SJ_SLEEPMODE_MAX_FADE_SECONDS);

	}

	sizer1->Add(SJ_DLG_SPACE, SJ_DLG_SPACE);

	// misc.

	sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2, 0, wxALIGN_CENTER_VERTICAL|wxALL, SJ_DLG_SPACE);

	sizer2->Add(new wxButton(page, IDC_TRANSITION_BUTTON, _("Fading")+wxString(wxT("...")), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT),
	            0, 0, SJ_DLG_SPACE);

	sizer2->Add(new wxButton(page, IDC_AUTOVOLBUTTON, _("Volume control")+wxString(wxT("...")), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT),
	            0, wxLEFT, SJ_DLG_SPACE);

	page->SetSizer(sizer1);

	UpdateAutoCtrlChecksE();

	return page;
}


wxString SjPlaybackSettingsConfigPage::FormatMinutes(long mm, long timemode)
{
	if( timemode == SJ_SLEEPMODE_TIMEMODE_AT
	 || timemode == SJ_SLEEPMODE_TIMEMODE_ALWAYS_AT  )
	{
		long hh = mm / 60;
		mm -= hh*60;
		return wxString::Format(wxT("%02i:%02i"), (int)hh, (int)mm);
	}
	else
	{
		return wxString::Format(wxT("%i"), (int)mm);
	}
}
long SjPlaybackSettingsConfigPage::ParseMinutes(const wxString& str)
{
	long hh, mm;
	if( str.Find(':') > 0 )
	{
		if( !str.BeforeFirst(':').ToLong(&hh, 10) ) { hh = 0; }
		if( hh < 0 ) hh = 0; if (hh > 23) hh = 23;

		if( !str.AfterFirst(':').ToLong(&mm, 10) ) { mm = 0; }
		if( mm < 0 ) mm = 0; if (mm > 59) mm = 59;

		return hh*60 + mm;
	}
	else
	{
		if( !str.ToLong(&mm, 10) ) { mm = 0; }
		if( mm < 0 ) mm = 0;

		return mm;
	}
}


void SjPlaybackSettingsConfigPage::UpdateAutoCtrlChecksE()
{
	m_autoPlayWait.Update();

	bool autoPlayWaitChecked = m_autoPlayWait.IsChecked();
	m_autoPlayTracks->Enable(autoPlayWaitChecked);
	m_autoPlayMusicSelChoice->Enable(autoPlayWaitChecked);
	m_autoPlayMusicSelButton->Enable(autoPlayWaitChecked);

	bool ignoreChecked = m_autoPlayMusicSelIgnoreCheck->IsChecked();
	m_autoPlayMusicSelIgnoreCheck->Enable(autoPlayWaitChecked);
	m_autoPlayMusicSelIgnoreChoice->Enable(autoPlayWaitChecked && ignoreChecked);
	m_autoPlayMusicSelIgnoreButton->Enable(autoPlayWaitChecked && ignoreChecked);

	m_autoPlayManEnqInterrupt->Enable(autoPlayWaitChecked);

	bool sleepChecked = m_sleepCheckBox->IsChecked();
	m_sleepActionChoice->Enable(sleepChecked);
	m_sleepTimemodeChoice->Enable(sleepChecked);
	m_sleepMinutesTextCtrl->Enable(sleepChecked);
	m_sleepMinUnitStatic->SetLabel(m_sleepMinUnitStrings [ m_sleepTimemodeChoice->GetSelection() ] /*index!*/);

	if( (m_sleepFade.IsEnabled()!=FALSE) != (sleepChecked!=FALSE) )
		m_sleepFade.Enable(sleepChecked);

	if( sleepChecked )
		m_sleepFade.Update();
}


void SjPlaybackSettingsConfigPage::OnMusicSelButton(wxCommandEvent& evt)
{
	if( g_advSearchModule )
	{
		long preselectId = 0;
		switch( evt.GetId() )
		{
			case IDC_AUTOPLAYMUSICSELBUT:       preselectId = SjDialog::GetCbSelection(m_autoPlayMusicSelChoice);       break; // may be 0 (for "Current view"), however, OpenDialog() below accepts this and uses the most recent music selection
			case IDC_AUTOPLAYIGNOREBUTTON:      preselectId = SjDialog::GetCbSelection(m_autoPlayMusicSelIgnoreChoice); break;
			case IDC_JINGLEEVERYMUSICSELBUTTON: preselectId = SjDialog::GetCbSelection(m_jingleEveryMusicSelChoice);    break;
			case IDC_JINGLEAT0MUSICSELBUTTON:   preselectId = SjDialog::GetCbSelection(m_jingleAtMusicSelChoice[0]);    break;
			case IDC_JINGLEAT1MUSICSELBUTTON:   preselectId = SjDialog::GetCbSelection(m_jingleAtMusicSelChoice[1]);    break;
		}

		g_advSearchModule->OpenDialog(preselectId);
	}
}


void SjPlaybackSettingsConfigPage::OnAutoCtrlSleepTimemode(wxCommandEvent&)
{
	m_sleepMinUnitStatic->SetLabel(m_sleepMinUnitStrings [ m_sleepTimemodeChoice->GetSelection() ] /*index!*/);

	long newTimemode = SjDialog::GetCbSelection(m_sleepTimemodeChoice);
	if( newTimemode%SJ_SLEEPMODE_TIMEMODE_BASIC_COUNT != m_sleepTimemodeCurr%SJ_SLEEPMODE_TIMEMODE_BASIC_COUNT /*we've three timemodes, that are repeated or not*/ )
	{
		wxSpinEvent fwd;
		m_sleepMinutesTextCtrl->SetValue(FormatMinutes(SJ_SLEEPMODE_DEF_MINUTES, newTimemode));
	}
	m_sleepTimemodeCurr = newTimemode;
}


void SjPlaybackSettingsConfigPage::CloseAutoCtrlPage(bool apply, bool& needsReplay)
{
	if( apply )
	{
		// save from dialog to g_mainFrame->m_autoCtrl
		SjAutoCtrl* a = &(g_mainFrame->m_autoCtrl);

		SjTools::SetFlag(a->m_flags, SJ_AUTOCTRL_AUTOPLAY_ENABLED, m_autoPlayWait.IsChecked());
		SjTools::SetFlag(a->m_flags, SJ_AUTOCTRL_AUTOPLAY_IGNORE, m_autoPlayMusicSelIgnoreCheck->IsChecked());
		SjTools::SetFlag(a->m_flags, SJ_AUTOCTRL_AUTOPLAY_MAN_ENQ_INTERRUPT, m_autoPlayManEnqInterrupt->IsChecked());
		a->m_autoPlayWaitMinutes = m_autoPlayWait.GetValue();
		a->m_autoPlayNumTracks = m_autoPlayTracks->GetValue();
		int index = m_autoPlayMusicSelChoice->GetSelection();
		if( index != -1 )
		{
			a->m_autoPlayMusicSelId = (long)m_autoPlayMusicSelChoice->GetClientData(index);
			// ^^ may be 0 for "Current view"
		}

		index = m_autoPlayMusicSelIgnoreChoice->GetSelection();
		if( index != -1 )
		{
			a->m_autoPlayMusicSelIgnoreId = (long)m_autoPlayMusicSelIgnoreChoice->GetClientData(index);
			// ^^ 0 should not happen here
		}


		wxString temp = m_sleepMinutesTextCtrl->GetValue();
		long sleepMinutesCurr = ParseMinutes(temp);

		a->SetSleepSettings(m_sleepCheckBox->IsChecked(),
		                    (SjShutdownEtc)SjDialog::GetCbSelection(m_sleepActionChoice),
		                    m_sleepTimemodeCurr,
		                    sleepMinutesCurr,
		                    m_sleepFade.IsChecked(),
		                    m_sleepFade.GetValue());

	}
}


void SjPlaybackSettingsConfigPage::OnTransitionButton(wxCommandEvent& event)
{
	SjCrossfadeDlg::OpenDialog(SjDialog::FindTopLevel(this));
}


void SjPlaybackSettingsConfigPage::OnAutoVolButton(wxCommandEvent& event)
{
	SjAutovolDlg::OpenDialog(SjDialog::FindTopLevel(this));
}


/*******************************************************************************
 * SjPlaybackSettingsConfigPage - Jingle
 ******************************************************************************/


wxPanel* SjPlaybackSettingsConfigPage::CreateJinglePage(wxWindow* parent)
{
	SjAutoCtrl* a = &(g_mainFrame->m_autoCtrl);

	// make sure, the "jingle" music selection is just fine (otherweise disable the option)
	if( g_advSearchModule )
	{
		if( g_advSearchModule->GetSearchById(a->m_jingleEveryMusicSelId, false).GetId() == 0 )
		{
			SjTools::SetFlag(a->m_jingleFlags, SJ_JINGLE_EVERY, false);
			a->SaveAutoCtrlSettings();
		}

		for( int i = 0; i < SJ_JINGLE_AT_CNT; i++ )
		{
			if( g_advSearchModule->GetSearchById(a->m_jingleAtMusicSelId[i], false).GetId() == 0 )
			{
				SjTools::SetFlag(a->m_jingleFlags, (SJ_JINGLE_AT0<<i), false);
				a->SaveAutoCtrlSettings();
			}
		}
	}

	// create dialog
	wxPanel* page = new wxPanel(parent, -1);
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);

	sizer1->Add(1, SJ_DLG_SPACE); // some space

	// hints
	wxStaticText* staticText = new wxStaticText(page, -1, _("Jingles are normal tracks from your music library that are played\nunder certain conditions."));
	sizer1->Add(staticText, 0, wxALL, SJ_DLG_SPACE);

	// Add the following controls: [X] If the jukebox is playing, about every [_____] minutes
	wxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2, 0, wxLEFT|wxTOP|wxRIGHT|wxGROW, SJ_DLG_SPACE);

		m_jingleEveryCheck = new wxCheckBox(page, IDC_JINGLEEVERYCHECK, _("If the jukebox is playing, about every"));
		m_jingleEveryCheck->SetValue((a->m_jingleFlags&SJ_JINGLE_EVERY)!=0);
		sizer2->Add(m_jingleEveryCheck, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

		m_jingleEveryMinutesSpin = new wxSpinCtrl(page, -1, wxString::Format(wxT("%i"), (int)a->m_jingleEvery),
		                                        wxDefaultPosition, wxSize(SJ_3DIG_SPINCTRL_W, -1), wxSP_ARROW_KEYS ,
		                                        SJ_JINGLE_MIN_MINUTES, SJ_JINGLE_MAX_MINUTES, a->m_jingleEvery);
		sizer2->Add(m_jingleEveryMinutesSpin, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

		sizer2->Add(new wxStaticText(page, -1, _("minutes,")), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

	// Add the following controls: [X] play a jingle from the music selection [Jingles  ][v]
	sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2, 0, wxLEFT|wxRIGHT, SJ_DLG_SPACE);

		sizer2->Add(SJ_DLG_SPACE*6, SJ_DLG_SPACE);

		sizer2->Add(new wxStaticText(page, -1, _("play a track from the music selection")), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

		m_jingleEveryMusicSelChoice = new wxChoice(page, -1, wxDefaultPosition, wxSize(200, -1));
		sizer2->Add(m_jingleEveryMusicSelChoice, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE/2);

		SjAdvSearchModule::UpdateAdvSearchChoice(m_jingleEveryMusicSelChoice, a->m_jingleEveryMusicSelId);
		SjDialog::SetCbWidth(m_jingleEveryMusicSelChoice, 200);

		m_jingleEveryMusicSelButton = new wxButton(page, IDC_JINGLEEVERYMUSICSELBUTTON, wxT("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
		sizer2->Add(m_jingleEveryMusicSelButton, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);

	for( int i = 0; i < SJ_JINGLE_AT_CNT; i++ )
	{
		// Add the following controls: [X] At [_____] o'clock,
		sizer2 = new wxBoxSizer(wxHORIZONTAL);
		sizer1->Add(sizer2, 0, wxLEFT|wxTOP|wxRIGHT|wxGROW, SJ_DLG_SPACE);

			m_jingleAtCheck[i] = new wxCheckBox(page, IDC_JINGLEAT0CHECK+i, _("At"));
			m_jingleAtCheck[i]->SetValue((a->m_jingleFlags&(SJ_JINGLE_AT0<<i))!=0);
			sizer2->Add(m_jingleAtCheck[i], 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

			m_jingleAtTextCtrl[i] = new wxTextCtrl(page, -1, FormatMinutes(a->m_jingleAt[i], SJ_SLEEPMODE_TIMEMODE_AT), wxDefaultPosition, wxSize(SJ_4DIG_SPINCTRL_W, -1));
			sizer2->Add(m_jingleAtTextCtrl[i], 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

			sizer2->Add(new wxStaticText(page, -1, _("o'clock,")), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

		// Add the following controls: [X] play a jingle from the music selection [Jingle  ][v]
		sizer2 = new wxBoxSizer(wxHORIZONTAL);
		sizer1->Add(sizer2, 0, wxLEFT|wxRIGHT, SJ_DLG_SPACE);

			sizer2->Add(SJ_DLG_SPACE*6, SJ_DLG_SPACE);

			sizer2->Add(new wxStaticText(page, -1, _("play a track from the music selection")), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

			m_jingleAtMusicSelChoice[i] = new wxChoice(page, -1, wxDefaultPosition, wxSize(200, -1));
			sizer2->Add(m_jingleAtMusicSelChoice[i], 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE/2);

			SjAdvSearchModule::UpdateAdvSearchChoice(m_jingleAtMusicSelChoice[i], a->m_jingleAtMusicSelId[i]);
			SjDialog::SetCbWidth(m_jingleAtMusicSelChoice[i], 200);

			m_jingleAtMusicSelButton[i] = new wxButton(page, IDC_JINGLEAT0MUSICSELBUTTON+i, wxT("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
			sizer2->Add(m_jingleAtMusicSelButton[i], 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);

		// add the following controls: [X] Daily  [X] Wait for end of playing track
		sizer2 = new wxBoxSizer(wxHORIZONTAL);
		sizer1->Add(sizer2, 0, wxLEFT|wxRIGHT, SJ_DLG_SPACE);

			sizer2->Add(SJ_DLG_SPACE*6, SJ_DLG_SPACE);

			m_jingleAtDailyCheck[i] = new wxCheckBox(page, -1, _("Daily"));
			m_jingleAtDailyCheck[i]->SetValue((a->m_jingleFlags&(SJ_JINGLE_AT0_DAILY<<i))!=0);
			sizer2->Add(m_jingleAtDailyCheck[i], 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

			m_jingleAtWaitCheck[i] = new wxCheckBox(page, -1, _("Wait for end of playing track"));
			m_jingleAtWaitCheck[i]->SetValue((a->m_jingleFlags&(SJ_JINGLE_AT0_WAIT<<i))!=0);
			sizer2->Add(m_jingleAtWaitCheck[i], 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);
	}

	EnableDisableJingle();
	page->SetSizer(sizer1);
	return page;
}


void SjPlaybackSettingsConfigPage::EnableDisableJingle()
{
	bool enabled = m_jingleEveryCheck->IsChecked();

	m_jingleEveryMinutesSpin->Enable(enabled);

	m_jingleEveryMusicSelChoice->Enable(enabled);
	m_jingleEveryMusicSelButton->Enable(enabled);

	for( int i = 0; i < SJ_JINGLE_AT_CNT; i++ )
	{
		enabled = m_jingleAtCheck[i]->IsChecked();

		m_jingleAtTextCtrl[i]->Enable(enabled);

		m_jingleAtMusicSelChoice[i]->Enable(enabled);
		m_jingleAtMusicSelButton[i]->Enable(enabled);

		m_jingleAtDailyCheck[i]->Enable(enabled);
		m_jingleAtWaitCheck[i]->Enable(enabled);
	}
}


void SjPlaybackSettingsConfigPage::CloseJinglePage(bool apply, bool& needsReplay)
{
	if( apply )
	{
		SjAutoCtrl* a = &(g_mainFrame->m_autoCtrl);

		SjTools::SetFlag(a->m_jingleFlags, SJ_JINGLE_EVERY, m_jingleEveryCheck->IsChecked());
		a->m_jingleEvery = m_jingleEveryMinutesSpin->GetValue();
		a->m_jingleEveryMusicSelId = SjDialog::GetCbSelection(m_jingleEveryMusicSelChoice);

		for( int i = 0; i < SJ_JINGLE_AT_CNT; i++ )
		{
			SjTools::SetFlag(a->m_jingleFlags, (SJ_JINGLE_AT0<<i), m_jingleAtCheck[i]->IsChecked());
			SjTools::SetFlag(a->m_jingleFlags, (SJ_JINGLE_AT0_DAILY<<i), m_jingleAtDailyCheck[i]->IsChecked());
			SjTools::SetFlag(a->m_jingleFlags, (SJ_JINGLE_AT0_WAIT<<i), m_jingleAtWaitCheck[i]->IsChecked());
			a->m_jingleAt[i] = ParseMinutes(m_jingleAtTextCtrl[i]->GetValue());
			a->m_jingleAtMusicSelId[i] = SjDialog::GetCbSelection(m_jingleAtMusicSelChoice[i]);
		}
	}
}


/*******************************************************************************
 * SjPlaybackSettingsConfigPage - Further Options
 ******************************************************************************/


wxPanel* SjPlaybackSettingsConfigPage::CreateFurtherOptPage(wxWindow* parent)
{
	long flags = g_mainFrame->m_autoCtrl.m_flags;

	// create dialog
	wxPanel* page = new wxPanel(parent, -1);
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);

	sizer1->Add(1, SJ_DLG_SPACE); // some space

	// hints
	wxStaticText* staticText = new wxStaticText(page, -1,
	        _(  "Use the following options to define which actions should be done automatically\nafter given timeouts."
	         ));
	sizer1->Add(staticText,
	            0, wxALL, SJ_DLG_SPACE);

	// some space

	m_autoFollowPlaylist.Create(page, sizer1,
	                            _("Go to current track after %i minutes of inactivity"), wxLEFT|wxTOP|wxRIGHT|wxBOTTOM,
	                            IDC_FOLLOW_PLAYLIST, (flags&SJ_AUTOCTRL_FOLLOW_PLAYLIST)!=0,
	                            -1, g_mainFrame->m_autoCtrl.m_followPlaylistMinutes, SJ_AUTOCTRL_MIN_FOLLOWPLAYLISTMINUTES, SJ_AUTOCTRL_MAX_FOLLOWPLAYLISTMINUTES);

	m_autoResetView.Create(page, sizer1,
	                       _("Reset view after %i minutes of inactivity to"), wxLEFT|wxRIGHT|wxBOTTOM,
	                       IDC_RESET_VIEW, (flags&SJ_AUTOCTRL_RESET_VIEW)!=0,
	                       -1, g_mainFrame->m_autoCtrl.m_resetViewMinutes, SJ_AUTOCTRL_MIN_RESETVIEWMINUTES, SJ_AUTOCTRL_MAX_RESETVIEWMINUTES);

	m_autoResetViewTo = new wxChoice(page, IDC_RESET_VIEW_TO);
	m_autoResetViewTo->Append(_("Album view"), (void*)SJ_BROWSER_ALBUM_VIEW);
	m_autoResetViewTo->Append(_("Cover view"), (void*)SJ_BROWSER_COVER_VIEW);
	m_autoResetViewTo->Append(_("List view"), (void*)SJ_BROWSER_LIST_VIEW);
	m_autoResetView.m_sizer->Add(m_autoResetViewTo, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);
	SjDialog::SetCbSelection(m_autoResetViewTo, g_mainFrame->m_autoCtrl.m_resetViewTo);

	wxString str(_("Open video screen after %i minutes of inactivity"));
	m_autoStartVis.Create(page, sizer1,
	                      str, wxLEFT|wxRIGHT|wxBOTTOM,
	                      IDC_AUTO_START_VIS, (flags&SJ_AUTOCTRL_START_VIS)!=0,
	                      -1, g_mainFrame->m_autoCtrl.m_startVisMinutes, SJ_AUTOCTRL_MIN_STARTVISMINUTES, SJ_AUTOCTRL_MAX_STARTVISMINUTES);

	str  = _("Close video screen after %i minutes");
	m_autoStopVis.Create(page, sizer1,
	                     str, wxLEFT|wxRIGHT|wxBOTTOM,
	                     IDC_AUTO_STOP_VIS, (flags&SJ_AUTOCTRL_STOP_VIS)!=0,
	                     -1, g_mainFrame->m_autoCtrl.m_stopVisMinutes, SJ_AUTOCTRL_MIN_STOPVISMINUTES, SJ_AUTOCTRL_MAX_STOPVISMINUTES);

	m_limitPlayTime.Create(page, sizer1,
	                            _("Limit play time to %i seconds"), wxLEFT|wxRIGHT|wxBOTTOM,
	                            IDC_LIMIT_PLAY_TIME, (flags&SJ_AUTOCTRL_LIMIT_PLAY_TIME)!=0,
	                            -1, g_mainFrame->m_autoCtrl.m_limitPlayTimeSeconds, SJ_AUTOCTRL_MIN_LIMITPLAYTIMESECONDS, SJ_AUTOCTRL_MAX_LIMITPLAYTIMESECONDS);
	// reset button
	wxButton* button = new wxButton(page, IDC_FURTHEROPTRESET, _("Reset to default values"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer1->Add(button,
	            0, wxALL, SJ_DLG_SPACE);

	UpdateFurtherOptChecksM();

	page->SetSizer(sizer1);
	return page;
}


void SjPlaybackSettingsConfigPage::UpdateFurtherOptChecksM()
{
	m_autoFollowPlaylist.Update();
	m_autoResetView.Update();
	m_autoResetViewTo->Enable(m_autoResetView.IsChecked());
	m_autoStartVis.Update();
	m_autoStopVis.Update();
	m_limitPlayTime.Update();
}


void SjPlaybackSettingsConfigPage::OnFurtherOptReset(wxCommandEvent&)
{
	m_autoFollowPlaylist.SetChecked((SJ_AUTOCTRL_DEF_FLAGS&SJ_AUTOCTRL_FOLLOW_PLAYLIST)!=0);
	m_autoFollowPlaylist.SetValue(SJ_AUTOCTRL_DEF_FOLLOWPLAYLISTMINUTES);

	m_autoResetView.SetChecked((SJ_AUTOCTRL_DEF_FLAGS&SJ_AUTOCTRL_RESET_VIEW)!=0);
	m_autoResetView.SetValue(SJ_AUTOCTRL_DEF_RESETVIEWMINUTES);
	SjDialog::SetCbSelection(m_autoResetViewTo, SJ_BROWSER_ALBUM_VIEW);

	m_autoStartVis.SetChecked((SJ_AUTOCTRL_DEF_FLAGS&SJ_AUTOCTRL_START_VIS)!=0);
	m_autoStartVis.SetValue(SJ_AUTOCTRL_DEF_STARTVISMINUTES);

	m_autoStopVis.SetChecked((SJ_AUTOCTRL_DEF_FLAGS&SJ_AUTOCTRL_STOP_VIS)!=0);
	m_autoStopVis.SetValue(SJ_AUTOCTRL_DEF_STOPVISMINUTES);

	m_limitPlayTime.SetChecked((SJ_AUTOCTRL_DEF_FLAGS&SJ_AUTOCTRL_LIMIT_PLAY_TIME)!=0);
	m_limitPlayTime.SetValue(SJ_AUTOCTRL_DEF_LIMITPLAYTIMESECONDS);

	UpdateFurtherOptChecksM();
}


void SjPlaybackSettingsConfigPage::CloseFurtherOptPage(bool apply, bool& needsReplay)
{
	if( apply )
	{
		SjAutoCtrl* a = &(g_mainFrame->m_autoCtrl);

		SjTools::SetFlag(a->m_flags, SJ_AUTOCTRL_FOLLOW_PLAYLIST, m_autoFollowPlaylist.IsChecked());
		a->m_followPlaylistMinutes = m_autoFollowPlaylist.GetValue();

		SjTools::SetFlag(a->m_flags, SJ_AUTOCTRL_RESET_VIEW, m_autoResetView.IsChecked());
		a->m_resetViewMinutes = m_autoResetView.GetValue();
		a->m_resetViewTo = SjDialog::GetCbSelection(m_autoResetViewTo, SJ_BROWSER_ALBUM_VIEW);

		SjTools::SetFlag(a->m_flags, SJ_AUTOCTRL_START_VIS, m_autoStartVis.IsChecked());
		a->m_startVisMinutes = m_autoStartVis.GetValue();

		SjTools::SetFlag(a->m_flags, SJ_AUTOCTRL_STOP_VIS, m_autoStopVis.IsChecked());
		a->m_stopVisMinutes = m_autoStopVis.GetValue();

		SjTools::SetFlag(a->m_flags, SJ_AUTOCTRL_LIMIT_PLAY_TIME, m_limitPlayTime.IsChecked());
		a->m_limitPlayTimeSeconds = m_limitPlayTime.GetValue();
	}
}


/*******************************************************************************
 * SjPlaybackSettingsModule
 ******************************************************************************/


SjPlaybackSettingsModule::SjPlaybackSettingsModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	m_file       = wxT("memory:playbacksettings.lib");
	m_name       = _("Playback");
	m_guiIcon    = SJ_ICON_PLAYBACK_SETTINGS;
	m_sort       = 40;
}


wxWindow* SjPlaybackSettingsModule::GetConfigPage(wxWindow* parent, int selectedPage)
{
	return new SjPlaybackSettingsConfigPage(this, parent, selectedPage);
}


void SjPlaybackSettingsModule::DoneConfigPage (wxWindow* configPage, int doneAction)
{
	((SjPlaybackSettingsConfigPage*)configPage)->CloseAll(doneAction!=SJ_CONFIGPAGE_DONE_CANCEL_CLICKED);
}


void SjPlaybackSettingsModule::CloseDependingDialogs()
{
	SjAutovolDlg::CloseDialog();
	SjCrossfadeDlg::CloseDialog();
}


void SjPlaybackSettingsModule::ReceiveMsg(int msg)
{
	if( msg == IDMODMSG_KIOSK_STARTING
	 || msg == IDMODMSG_WINDOW_CLOSE )
	{
		CloseDependingDialogs();
	}
	else if( msg == IDMODMSG_ADV_SEARCH_CONFIG_CHANGED )
	{
		if( g_playbackConfigPage )
		{
			SjAdvSearchModule::UpdateAdvSearchChoice(g_playbackConfigPage->m_autoPlayMusicSelChoice,
			        0, // ignored, the current selection is used
			        _("Music selection")+wxString(wxT(" \"%s\"")), _("Current view"));
			SjAdvSearchModule::UpdateAdvSearchChoice(g_playbackConfigPage->m_autoPlayMusicSelIgnoreChoice,
			        0 // ignored, the current selection is used
			                                        );
			SjAdvSearchModule::UpdateAdvSearchChoice(g_playbackConfigPage->m_jingleEveryMusicSelChoice,
			        0 // ignored, the current selection is used
			                                        );
			for( int i = 0; i < SJ_JINGLE_AT_CNT; i++ )
			{
				SjAdvSearchModule::UpdateAdvSearchChoice(g_playbackConfigPage->m_jingleAtMusicSelChoice[i],
					0 // ignored, the current selection is used
													);
			}
		}
	}
}


