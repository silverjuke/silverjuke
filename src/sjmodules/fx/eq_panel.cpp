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
 * File:    eq_panel.cpp
 * Authors: Björn Petersen
 * Purpose: Equalizer control panel
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/fx/eq_panel.h>
#include <sjmodules/fx/equalizer.h>


#define SLIDER_RES	1000

#define IDM_SWITCH_ON_OFF           (IDM_FIRSTPRIVATE+100)
#define IDM_PRESET_CHOICE           (IDM_FIRSTPRIVATE+101)
#define IDM_SAVE_AS                 (IDM_FIRSTPRIVATE+102)
#define IDM_RENAME                  (IDM_FIRSTPRIVATE+103)
#define IDM_DELETE                  (IDM_FIRSTPRIVATE+104)
#define IDM_IMPORT                  (IDM_FIRSTPRIVATE+105)
#define IDM_EXPORT                  (IDM_FIRSTPRIVATE+106)
#define IDM_RESET_STD_PRESETS       (IDM_FIRSTPRIVATE+107)
#define IDM_SHIFT_UP                (IDM_FIRSTPRIVATE+108)
#define IDM_SHIFT_DOWN              (IDM_FIRSTPRIVATE+109)
#define IDM_SHIFT_AUTO_LEVEL        (IDM_FIRSTPRIVATE+110)
#define IDM_PARAMSLIDER000			(IDM_FIRSTPRIVATE+130)
#define IDM_LABEL_FIRST				(IDM_FIRSTPRIVATE+170)


BEGIN_EVENT_TABLE(SjEqPanel, wxPanel)
	EVT_BUTTON   (IDC_BUTTONBARMENU,     SjEqPanel::OnMenu            )
	EVT_CHECKBOX (IDM_SWITCH_ON_OFF,     SjEqPanel::OnSwitchOnOff     )
	EVT_MENU     (IDM_SAVE_AS,           SjEqPanel::OnSaveAsRename    )
	EVT_MENU     (IDM_RENAME,            SjEqPanel::OnSaveAsRename    )
	EVT_MENU     (IDM_DELETE,            SjEqPanel::OnDelete          )
	EVT_MENU     (IDM_IMPORT,            SjEqPanel::OnImport          )
	EVT_MENU     (IDM_EXPORT,            SjEqPanel::OnExport          )
	EVT_MENU     (IDM_RESET_STD_PRESETS, SjEqPanel::OnResetStdPresets )
	EVT_MENU     (IDM_SHIFT_UP,          SjEqPanel::OnShift           )
	EVT_MENU     (IDM_SHIFT_DOWN,        SjEqPanel::OnShift           )
	EVT_MENU     (IDM_SHIFT_AUTO_LEVEL,  SjEqPanel::OnShift           )
	EVT_COMMAND_SCROLL_RANGE(IDM_PARAMSLIDER000, IDM_PARAMSLIDER000+SJ_EQ_BANDS, SjEqPanel::OnSlider)
END_EVENT_TABLE()


SjEqPanel::SjEqPanel(wxWindow* parent)
	: wxPanel(parent, -1)
{
	wxBoxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	bool eqEnabled;
	g_mainFrame->m_player.EqGetParam(&eqEnabled, &m_currParam);

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

	Param2Dlg();

	// controls below the sliders
	wxBoxSizer* slider2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(slider2, 0, wxGROW|wxALL, SJ_DLG_SPACE);

	slider2->Add(new wxStaticText(this, wxID_ANY, _("Preset:")), 0, wxALIGN_CENTER|wxRIGHT, SJ_DLG_SPACE);

	m_presetChoice = new wxChoice(this, IDM_PRESET_CHOICE);
	m_presetChoice->Append("Null");
	m_presetChoice->SetSelection(0);
	slider2->Add(m_presetChoice, 0, wxALIGN_CENTER|wxRIGHT, SJ_DLG_SPACE);

	wxButton* b = new wxButton(this, IDC_BUTTONBARMENU, _("Menu") + wxString(SJ_BUTTON_MENU_ARROW));
	slider2->Add(b, 0, wxALIGN_CENTER|wxRIGHT, SJ_DLG_SPACE);
}


wxString SjEqPanel::FormatParam(float db)
{
	// we do not have much space for each value; so use a comma only for small values that to longen the string
	#define PLUS_MINUS_CHAR "\u00B1"
	wxString ret;
	     if( db>-0.1 && db<0.1   ) { ret = "0";          }
	else                           { ret = wxString::Format("%.0f", db); }

	if( db > 0.0 ) { ret = "+" + ret; }

	return ret;
}


void SjEqPanel::Param2Dlg(int paramIndex)
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


void SjEqPanel::Param2Dlg()
{
	for( int i = 0; i < SJ_EQ_BANDS; i++ ) {
		Param2Dlg(i);
	}
}


void SjEqPanel::OnSwitchOnOff(wxCommandEvent&)
{
	wxLogError("TODO: On/Off");
	m_onOffSwitch->SetValue(false);
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

			g_mainFrame->m_player.EqSetParam(NULL, &m_currParam);
		}
	}
}


void SjEqPanel::OnMenu(wxCommandEvent& event)
{
	wxButton*   wnd = (wxButton*)FindWindow(IDC_BUTTONBARMENU);
	if( wnd )
	{
		SjMenu menu(SJ_SHORTCUTS_LOCAL);

		menu.Append(IDM_SAVE_AS, _("Save as..."));
		menu.Append(IDM_RENAME, _("Rename"));
		menu.Append(IDM_DELETE, _("Delete"));

		SjMenu* submenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
			submenu->Append(IDM_IMPORT, _("Import preset..."));
			submenu->Append(IDM_EXPORT, _("Export preset..."));
			submenu->Append(IDM_RESET_STD_PRESETS, _("Reset standard presets"));
		menu.Append(0, _("Import/Export"), submenu);

		menu.AppendSeparator();
		menu.Append(IDM_SHIFT_UP, _("Shift up"));
		menu.Append(IDM_SHIFT_DOWN, _("Shift down"));
		menu.Append(IDM_SHIFT_AUTO_LEVEL, _("Auto Level"));

		wnd->PopupMenu(&menu, 0, 0);
	}
}


void SjEqPanel::OnSaveAsRename(wxCommandEvent&)
{
	wxLogError("TODO: SaveAs/Rename");
}


void SjEqPanel::OnDelete(wxCommandEvent&)
{
	wxLogError("TODO: Delete");
}


void SjEqPanel::OnImport(wxCommandEvent&)
{
	wxLogError("TODO: Import");
}


void SjEqPanel::OnExport(wxCommandEvent&)
{
	wxLogError("TODO: Export");
}


void SjEqPanel::OnResetStdPresets(wxCommandEvent&)
{
	wxLogError("TODO: Rest");
}


void SjEqPanel::OnShift(wxCommandEvent&)
{
	wxLogError("TODO: Shift");
}


