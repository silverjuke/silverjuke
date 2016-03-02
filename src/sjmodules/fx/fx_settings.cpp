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
 * File:    fx_settings.cpp
 * Authors: Björn Petersen
 * Purpose: FX settings pages
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/fx/fx_settings.h>
#include <sjmodules/fx/eq_panel.h>
#include <wx/notebook.h>


/*******************************************************************************
 * SjFxSettingsPage
 ******************************************************************************/


class SjFxSettingsPage : public wxPanel
{
public:
	               SjFxSettingsPage     (wxWindow* parent, int selectedPage);

private:
	wxNotebook*     m_notebook;
	                DECLARE_EVENT_TABLE ();

	friend class    SjViewSettingsModule;
};


BEGIN_EVENT_TABLE(SjFxSettingsPage, wxPanel)
END_EVENT_TABLE()


SjFxSettingsPage::SjFxSettingsPage(wxWindow* parent, int selectedPage)
	: wxPanel(parent)
{
	// create notebook
	wxSizer* dialogSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(dialogSizer);

	m_notebook = new wxNotebook(this, IDC_NOTEBOOK, wxDefaultPosition, wxDefaultSize, 0/*wxCLIP_CHILDREN - problems with wxChoice/wxComboBox*/);

	wxNotebook* notebookSizer = m_notebook;

	m_notebook->AddPage(new SjEqPanel(m_notebook),  _("Equalizer"));

	if( selectedPage<0 || selectedPage >= (int)m_notebook->GetPageCount() ) {
		selectedPage = 0;
	}
	m_notebook->SetSelection(selectedPage);

	dialogSizer->Add(notebookSizer, 1, wxALL|wxGROW, SJ_DLG_SPACE);

	// init done, center dialog
	dialogSizer->SetSizeHints(this);
}


/*******************************************************************************
 * SjFxSettingsModule
 ******************************************************************************/


SjFxSettingsModule::SjFxSettingsModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	m_file       = "memory:fxsettings.lib";
	m_name       = _("FX");
	m_guiIcon    = SJ_ICON_FX_SETTINGS;
	m_sort       = 45;
}


wxWindow* SjFxSettingsModule::GetConfigPage(wxWindow* parent, int selectedPage)
{
	return new SjFxSettingsPage(parent, selectedPage);
}



void SjFxSettingsModule::DoneConfigPage(wxWindow* configPage, int action)
{
	g_mainFrame->m_player.SaveSettings();
}
