/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2016 Björn Petersen Software Design and Development
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
 * File:    eq_panel.cpp
 * Authors: Björn Petersen
 * Purpose: Equalizer control panel
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/msgbox.h>
#include <sjmodules/fx/eq_panel.h>


#define SLIDER_RES	1000

#define IDM_SWITCH_ON_OFF           (IDM_FIRSTPRIVATE+100)
#define IDM_PRESET_CHOICE           (IDM_FIRSTPRIVATE+101)
#define IDM_SAVE_AS                 (IDM_FIRSTPRIVATE+102)
#define IDM_DELETE                  (IDM_FIRSTPRIVATE+103)
#define IDM_IMPORT                  (IDM_FIRSTPRIVATE+104)
#define IDM_EXPORT                  (IDM_FIRSTPRIVATE+105)
#define IDM_RESET_STD_PRESETS       (IDM_FIRSTPRIVATE+106)
#define IDM_SHIFT_UP                (IDM_FIRSTPRIVATE+107)
#define IDM_SHIFT_DOWN              (IDM_FIRSTPRIVATE+108)
#define IDM_SHIFT_AUTO_LEVEL        (IDM_FIRSTPRIVATE+109)
#define IDM_UNDO                    (IDM_FIRSTPRIVATE+110)
#define IDM_PARAMSLIDER000			(IDM_FIRSTPRIVATE+130)
#define IDM_LABEL_FIRST				(IDM_FIRSTPRIVATE+170)


BEGIN_EVENT_TABLE(SjEqPanel, wxPanel)
	EVT_BUTTON   (IDC_BUTTONBARMENU,     SjEqPanel::OnMenu            )
	EVT_CHECKBOX (IDM_SWITCH_ON_OFF,     SjEqPanel::OnSwitchOnOff     )
	EVT_CHOICE   (IDM_PRESET_CHOICE,     SjEqPanel::OnPresetChoice    )
	EVT_MENU     (IDM_SAVE_AS,           SjEqPanel::OnSaveAs          )
	EVT_MENU     (IDM_DELETE,            SjEqPanel::OnDelete          )
	EVT_MENU     (IDM_IMPORT,            SjEqPanel::OnImport          )
	EVT_MENU     (IDM_EXPORT,            SjEqPanel::OnExport          )
	EVT_MENU     (IDM_RESET_STD_PRESETS, SjEqPanel::OnResetStdPresets )
	EVT_MENU     (IDM_SHIFT_UP,          SjEqPanel::OnShift           )
	EVT_MENU     (IDM_SHIFT_DOWN,        SjEqPanel::OnShift           )
	EVT_MENU     (IDM_SHIFT_AUTO_LEVEL,  SjEqPanel::OnShift           )
	EVT_MENU     (IDM_UNDO,              SjEqPanel::OnUndo            )
	EVT_COMMAND_SCROLL_RANGE(IDM_PARAMSLIDER000, IDM_PARAMSLIDER000+SJ_EQ_BANDS, SjEqPanel::OnSlider)
END_EVENT_TABLE()


SjEqPanel::SjEqPanel(wxWindow* parent)
	: wxPanel(parent, -1)
{
	wxBoxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	bool eqEnabled;
	g_mainFrame->m_player.EqGetParam(&eqEnabled, &m_currParam);
	m_backupParam = m_currParam;

	// on/off
	sizer1->Add(1, SJ_DLG_SPACE*2); // some space

	m_onOffSwitch = new wxCheckBox(this, IDM_SWITCH_ON_OFF, _("Use Equalizer"));
	m_onOffSwitch->SetValue(eqEnabled);
	sizer1->Add(m_onOffSwitch, 0, wxLEFT, SJ_DLG_SPACE);

	// create the sliders
	wxBoxSizer* allSliderSizer = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(allSliderSizer, 1, wxGROW|wxLEFT|wxRIGHT|wxTOP, SJ_DLG_SPACE);

	wxStaticText* staticText;
	for( int p = 0; p < SJ_EQ_BANDS; p++ )
	{
		// create the parameter's sizer
		wxSizer* paramSizer = new wxBoxSizer(wxVERTICAL);
		allSliderSizer->Add(paramSizer, 1, wxGROW, SJ_DLG_SPACE);

		// create the label (wxALIGN_CENTRE_HORIZONTAL does not work on wxGTK 3.0.2, wxMSW is fine)
		staticText = new wxStaticText(this, wxID_ANY, p==0? "Hz" : (p==9?"KHz" : " ") , wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE|wxALIGN_CENTRE_HORIZONTAL);
		paramSizer->Add(staticText, 0, wxGROW);

		wxString label = SjEqParam::s_bandNames[p];
		staticText = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE|wxALIGN_CENTRE_HORIZONTAL);
		paramSizer->Add(staticText, 0, wxGROW);

		// create the slider
		wxSlider* slider = new wxSlider(this,
			IDM_PARAMSLIDER000+p,
			SLIDER_RES/2, 0, SLIDER_RES, // curr/min/max - we calculate the stuff ourselfs to allow small values abottom
			wxDefaultPosition,
			wxSize(-1, 120),
			wxSL_VERTICAL | wxSL_INVERSE);
		paramSizer->Add(slider, 1, wxALIGN_CENTER);

		// create the current value field
		staticText = new wxStaticText(this, IDM_LABEL_FIRST+p, "", wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE|wxALIGN_CENTRE_HORIZONTAL);
		paramSizer->Add(staticText, 0, wxGROW);

		staticText = new wxStaticText(this, wxID_ANY, p==0?"dB":" " , wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE|wxALIGN_CENTRE_HORIZONTAL);
		paramSizer->Add(staticText, 0, wxGROW);
	}

	UpdateSliders();

	// controls below the sliders
	wxBoxSizer* slider2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(slider2, 0, wxGROW|wxALL, SJ_DLG_SPACE);

	slider2->Add(new wxStaticText(this, wxID_ANY, _("Preset:")), 0, wxALIGN_CENTER|wxRIGHT, SJ_DLG_SPACE);

	m_presetChoice = new wxChoice(this, IDM_PRESET_CHOICE);
	slider2->Add(m_presetChoice, 0, wxALIGN_CENTER|wxRIGHT, SJ_DLG_SPACE);
	UpdatePresetChoice(true /*create list*/);

	m_backupPresetName = GetPresetNameFromChoice();

	wxButton* b = new wxButton(this, IDC_BUTTONBARMENU, _("Menu") + wxString(SJ_BUTTON_MENU_ARROW));
	slider2->Add(b, 0, wxALIGN_CENTER|wxRIGHT, SJ_DLG_SPACE);
}


wxString SjEqPanel::FormatParam(float db)
{
	// we do not have much space for each value; so use a comma only for small values that to longen the string
	int rounded = (int)(db+0.5F);
	if( rounded <= 0 ) {
		return wxString::Format("%i", rounded);
	}
	else {
		return wxString::Format("+%i", rounded);
	}
}


wxString SjEqPanel::GetPresetNameFromChoice()
{
	wxString ret;
	int selectedIndex = m_presetChoice->GetSelection();
	if( selectedIndex != wxNOT_FOUND ) {
		ret = m_presetChoice->GetString(selectedIndex);
	}
	return ret;
}

void SjEqPanel::UpdateSlider(int paramIndex)
{
	wxSlider*			slider = (wxSlider*)FindWindowById(IDM_PARAMSLIDER000+paramIndex, this);
	wxStaticText*		label = (wxStaticText*)FindWindowById(IDM_LABEL_FIRST+paramIndex, this);

	// convert the parameter floating point range to our
	// slider range of 0..1000
	float range = SJ_EQ_BAND_MAX - SJ_EQ_BAND_MIN;
	float zeroBasedValue = m_currParam.m_bandDb[paramIndex] - SJ_EQ_BAND_MIN;
	int value = (int) (zeroBasedValue / range * (float)SLIDER_RES);
	wxASSERT( value >= 0 && value <= 1000 );

	// update the slider and the label
	slider->SetValue(value);
	label->SetLabel(FormatParam(m_currParam.m_bandDb[paramIndex]));
}


void SjEqPanel::UpdateSliders()
{
	for( int i = 0; i < SJ_EQ_BANDS; i++ ) {
		UpdateSlider(i);
	}
}


void SjEqPanel::UpdatePresetChoice(bool createItems)
{
	// create items
	if( createItems ) {
		m_presetChoice->Clear();
		wxArrayString presetNames = g_mainFrame->m_player.m_eqPresetFactory.GetNames();
		for( size_t i = 0; i < presetNames.GetCount(); i++ ) {
			m_presetChoice->Append(presetNames[i]);
		}
	}

	// select item by eq-parameters
	SjEqPreset currPreset = g_mainFrame->m_player.m_eqPresetFactory.GetPresetByParam(m_currParam);
	if( currPreset.m_name.IsEmpty() ) {
		m_presetChoice->SetSelection(wxNOT_FOUND);
	}
	else {
		m_presetChoice->SetSelection(m_presetChoice->FindString(currPreset.m_name)); // FindString() may return wxNOT_FOUND which is accepted by SetSelection()
	}
}


void SjEqPanel::UpdatePlayer()
{
	g_mainFrame->m_player.EqSetParam(NULL, &m_currParam);
}


void SjEqPanel::OnSwitchOnOff(wxCommandEvent&)
{
	bool newState = m_onOffSwitch->GetValue();
	g_mainFrame->m_player.EqSetParam(&newState, NULL);
}


void SjEqPanel::OnSlider(wxScrollEvent& event)
{
	int i_ = event.GetId();
	wxSlider* slider = (wxSlider*)FindWindowById(i_, this);
	wxStaticText* label = (wxStaticText*)FindWindowById(i_+(IDM_LABEL_FIRST-IDM_PARAMSLIDER000), this);
	if( slider && label )
	{
		// which effect and which parameter was modified?
		int paramIndex = (i_-IDM_PARAMSLIDER000);

		wxASSERT( paramIndex >= 0 && paramIndex < SJ_EQ_BANDS );

		// convert the 0..1000 value of the slider to the
		// ranged value of the parameter
		int value = slider->GetValue();
		float range = SJ_EQ_BAND_MAX - SJ_EQ_BAND_MIN;
		float newValue = SJ_EQ_BAND_MIN + ((float)value * range / SLIDER_RES);

		if( value>490 && value<510 )
		{
			newValue = 0.0; // snap to zero
		}

		if( newValue != m_currParam.m_bandDb[paramIndex] )
		{
			m_currParam.m_bandDb[paramIndex] = newValue;
			label->SetLabel(FormatParam(newValue));

			UpdatePresetChoice();
			UpdatePlayer();
		}
	}
}


void SjEqPanel::OnPresetChoice(wxCommandEvent& e)
{
	m_backupParam = m_currParam;

    int selectedIndex   = m_presetChoice->GetSelection(); if( selectedIndex == wxNOT_FOUND ) { return; } // nothing selected
    wxString presetName = m_presetChoice->GetString(selectedIndex); if( presetName == "" ) { return; } // error

    SjEqPreset preset = g_mainFrame->m_player.m_eqPresetFactory.GetPresetByName(presetName);
    if( preset.m_name.IsEmpty() ) { return; } // not found

    // realize the new preset
    m_backupPresetName = presetName;
    m_currParam = preset.m_param;
    UpdateSliders();
    UpdatePlayer();
}


void SjEqPanel::OnMenu(wxCommandEvent& event)
{
	wxButton*   wnd = (wxButton*)FindWindow(IDC_BUTTONBARMENU);
	if( wnd )
	{
		SjMenu menu(SJ_SHORTCUTS_LOCAL);
		bool   isPreset = !GetPresetNameFromChoice().IsEmpty();

		menu.Append(IDM_SAVE_AS, _("Save as...")); // we do not provide a "Rename" here - the name is used as the key
		menu.Append(IDM_DELETE, _("Delete"));      // and should not be changed.  If _really_ required, you can use "Save as..." and delete the old preset
		menu.Enable(IDM_DELETE, isPreset);
		SjMenu* submenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
			submenu->Append(IDM_IMPORT, _("Import preset")+"...");
			submenu->Append(IDM_EXPORT, _("Export preset")+"...");
			submenu->Append(IDM_RESET_STD_PRESETS, _("Reset standard presets"));
		menu.Append(0, _("Import/Export"), submenu);

		menu.AppendSeparator();
		menu.Append(IDM_SHIFT_UP, _("Shift up"));
		menu.Append(IDM_SHIFT_DOWN, _("Shift down"));
		menu.Append(IDM_SHIFT_AUTO_LEVEL, _("Auto Level"));
		menu.Enable(IDM_SHIFT_AUTO_LEVEL, (m_currParam.GetAutoLevelShift()!=0.0F)? true : false);
		menu.Append(IDM_UNDO, _("Undo"));

		wnd->PopupMenu(&menu, 0, 0);
	}
}


void SjEqPanel::OnSaveAs(wxCommandEvent&)
{
	wxString presetName = GetPresetNameFromChoice();
	if( presetName.IsEmpty() ) { presetName = m_backupPresetName; }

	wxTextEntryDialog textEntry(this, _("Enter a preset name:"), _("Equalizer"), presetName);
	if( textEntry.ShowModal() != wxID_OK ) { return; } // cancelled
	presetName = textEntry.GetValue();
	m_backupPresetName = presetName;

	g_mainFrame->m_player.m_eqPresetFactory.AddPreset(presetName, m_currParam);

	UpdatePresetChoice(true /*recreate list*/);
}


void SjEqPanel::OnDelete(wxCommandEvent&)
{
    int selectedIndex   = m_presetChoice->GetSelection(); if( selectedIndex == wxNOT_FOUND ) { return; } // nothing selected
    wxString presetName = m_presetChoice->GetString(selectedIndex); if( presetName == "" ) { return; } // error

	if( SjMessageBox(wxString::Format(_("Delete preset \"%s\"?"), presetName.c_str()), _("Equalizer"), wxYES_NO, this)!=wxYES ) { return; } // aborted

	g_mainFrame->m_player.m_eqPresetFactory.DeletePreset(presetName);

	UpdatePresetChoice(true /*recreate list*/);
}


void SjEqPanel::OnImport(wxCommandEvent&)
{
	m_backupParam = m_currParam;

	// let user select a file
	SjExtList extList("feq");
	wxFileDialog dlg(this, _("Import preset"), "", "", extList.GetFileDlgStr(), wxFD_OPEN|wxFD_CHANGE_DIR);
	if( dlg.ShowModal() != wxID_OK ) { return; }
	wxString selPath = dlg.GetPath();
	wxString selExt = SjTools::GetExt(selPath);

    // read file
	wxFileSystem fileSystem;
	wxFSFile* fsFile = fileSystem.OpenFile(selPath, wxFS_READ|wxFS_SEEKABLE);
		if( fsFile == NULL ) { wxLogError(_("Cannot read \"%s\"."), selPath.c_str()); return; }
		wxString content = SjTools::GetFileContent(fsFile->GetStream(), &wxConvISO8859_1 /*file is a windows file, however, we only use ASCII*/);
	delete fsFile;

	// set silders to imported data
    m_currParam.FromString(content);

	// also add a preset
	g_mainFrame->m_player.m_eqPresetFactory.AddPreset(wxFileName(selPath).GetName(), m_currParam);

	// update controls
	UpdateSliders();
	UpdatePresetChoice(true);
	UpdatePlayer();
}


void SjEqPanel::OnExport(wxCommandEvent&)
{
	// suggested file name
	wxString suggestedFileName = GetPresetNameFromChoice();
	if( suggestedFileName.IsEmpty() ) { suggestedFileName = m_backupPresetName; }
	if( suggestedFileName.IsEmpty() ) { suggestedFileName = _("Equalizer"); }
	suggestedFileName =  SjTools::EnsureValidFileNameChars(suggestedFileName);

	// let user select a file
	SjExtList extList("feq");
	wxFileDialog dlg(this, _("Export preset"), "", suggestedFileName+".feq", extList.GetFileDlgStr(wxFD_SAVE), wxFD_SAVE|wxFD_OVERWRITE_PROMPT|wxFD_CHANGE_DIR);
	dlg.SetFilterIndex(extList.GetFileDlgIndex("feq"));
	if( dlg.ShowModal() != wxID_OK ) { return; }
	wxString selPath, selExt;
	extList.GetFileDlgPath(dlg, selPath, selExt);

	// save the file
	wxString fileContent = m_currParam.ToString("\r\n") /*.feq files come from windows and have a DOS line ends*/;

	wxFile file(selPath, wxFile::write);
	if( !file.Write(fileContent, wxConvISO8859_1/*file is a windows file, however, we only use ASCII*/) )
	{
		wxLogError(_("Cannot write \"%s\"."), selPath.c_str());
		return;
	}
}


void SjEqPanel::OnResetStdPresets(wxCommandEvent&)
{
	g_mainFrame->m_player.m_eqPresetFactory.AddDefaultPresets();

	UpdatePresetChoice(true /*recreate list*/);
}


void SjEqPanel::OnShift(wxCommandEvent& e)
{
	m_backupParam = m_currParam;

	switch( e.GetId() )
	{
		case IDM_SHIFT_DOWN:       m_currParam.Shift(-1.0F); break;
		case IDM_SHIFT_UP:         m_currParam.Shift( 1.0F); break;
		case IDM_SHIFT_AUTO_LEVEL: m_currParam.Shift(m_currParam.GetAutoLevelShift()); break;
	}

	UpdateSliders();
	UpdatePresetChoice();
	UpdatePlayer();
}


void SjEqPanel::OnUndo(wxCommandEvent&)
{
	SjEqParam temp = m_currParam;
	m_currParam = m_backupParam;
	m_backupParam = temp;

	UpdateSliders();
	UpdatePresetChoice();
	UpdatePlayer();
}
