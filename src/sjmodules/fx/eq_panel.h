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
 * File:    eq_panel.h
 * Authors: Björn Petersen
 * Purpose: Equalizer control panel
 *
 ******************************************************************************/


#ifndef __SJ_EQ_PANEL_H__
#define __SJ_EQ_PANEL_H__


class SjEqPanel : public wxPanel
{
public:
	                SjEqPanel           (wxWindow* parent);

private:
	wxCheckBox*     m_onOffSwitch;
	wxChoice*       m_presetChoice;

	SjEqParam       m_currParam;

	SjEqParam       m_backupParam;
	wxString        m_backupPresetName;

	wxString        FormatParam         (float db);
	wxString        GetPresetNameFromChoice();

	void            UpdateSlider        (int paramIndex);
	void            UpdateSliders       ();
	void            UpdatePresetChoice  (bool createItems=false);
	void            UpdatePlayer        ();

	void            OnSwitchOnOff       (wxCommandEvent&);
	void            OnSlider            (wxScrollEvent&);
	void            OnPresetChoice      (wxCommandEvent&);
	void            OnMenu              (wxCommandEvent&);
	void            OnSaveAs            (wxCommandEvent&);
	void            OnDelete            (wxCommandEvent&);
	void            OnImport            (wxCommandEvent&);
	void            OnExport            (wxCommandEvent&);
	void            OnUndo              (wxCommandEvent&);
	void            OnResetStdPresets   (wxCommandEvent&);
	void            OnShift             (wxCommandEvent&);
	                DECLARE_EVENT_TABLE ();
};


#endif // __SJ_EQ_PANEL_H__
