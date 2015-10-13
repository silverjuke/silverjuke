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
 * File:    tageditorsplit.cpp
 * Authors: Björn Petersen
 * Purpose: Tag editor split plugin
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/tageditor/tageditor.h>
#include <sjmodules/tageditor/tageditorplugin.h>
#include <sjmodules/tageditor/tageditorsplit.h>


#define IDC_PATTERNCB       (IDM_FIRSTPRIVATE+1)
#define IDC_INSERT_BUTTON   (IDM_FIRSTPRIVATE+2)
#define IDC_SPLITIN         (IDM_FIRSTPRIVATE+3)
#define IDC_INSERT_FIRST    (IDM_FIRSTPRIVATE+100)
#define IDC_INSERT_LAST     (IDM_FIRSTPRIVATE+199)


BEGIN_EVENT_TABLE(SjSplitPlugin, SjTagEditorPlugin)
	EVT_BUTTON                  (IDC_INSERT_BUTTON,     SjSplitPlugin::OnInsert             )
	EVT_MENU_RANGE              (IDC_INSERT_FIRST,
	                             IDC_INSERT_LAST,       SjSplitPlugin::OnInsert             )
	EVT_TEXT                    (IDC_PATTERNCB,         SjSplitPlugin::OnPatternChange      )
	EVT_COMBOBOX                (IDC_PATTERNCB,         SjSplitPlugin::OnPatternChange      )
	EVT_CHOICE                  (IDC_SPLITIN,           SjSplitPlugin::OnFieldToSplitChange )
END_EVENT_TABLE()


SjSplitPlugin::SjSplitPlugin(wxWindow* parent, SjTrackInfo* exampleTrackInfo)
	: SjTagEditorPlugin(parent, wxT("split"), _("Split field"), exampleTrackInfo)
{
	// load configuration
	m_pattern = g_tools->ReadArray(wxT("tageditor/splitPattern"));
	if( m_pattern.IsEmpty() )
	{
		m_pattern.Add(wxT("<Artist>/<Year> <Album>/<Nr> <Title>"));
		m_pattern.Add(wxT("<Year> <Album>/<Nr> <Artist> - <Title>"));
		m_pattern.Add(wxT("<Nr> <Artist> - <Album> - <Title>"));
	}

	m_splitIn = g_tools->m_config->Read(wxT("tageditor/splitIn"), SJ_TI_URL);

	// create dialog
	wxSizer* sizer2 = new wxStaticBoxSizer(new wxStaticBox(this, -1, wxT("")), wxVERTICAL);
	m_sizer1->Add(sizer2, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	wxFlexGridSizer* sizer3 = new wxFlexGridSizer(2, SJ_DLG_SPACE, SJ_DLG_SPACE);
	sizer3->AddGrowableCol(1);
	sizer3->AddGrowableRow(2);
	sizer2->Add(sizer3, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	// field to split
	sizer3->Add(new wxStaticText(this, -1, _("Field to split:")), 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);

	m_splitInChoice = new SjTrackInfoFieldChoice(this, IDC_SPLITIN);
	m_splitInChoice->AppendFlags(SJ_TI_TRACKNAME
	                             |SJ_TI_LEADARTISTNAME
	                             |SJ_TI_ORGARTISTNAME
	                             |SJ_TI_COMPOSERNAME
	                             |SJ_TI_ALBUMNAME
	                             |SJ_TI_GENRENAME
	                             |SJ_TI_GROUPNAME
	                             |SJ_TI_COMMENT);
	m_splitInChoice->Append(SJ_CHOICE_SEP, (void*)0);
	m_splitInChoice->Append(_("Path and file name"), (void*)SJ_TI_URL);
	SjDialog::SetCbSelection(m_splitInChoice, m_splitIn);
	sizer3->Add(m_splitInChoice, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, SJ_DLG_SPACE/2);

	// pattern
	sizer3->Add(new wxStaticText(this, -1, _("Destination fields and pattern:")), 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);

	wxSizer* sizer4 = new wxBoxSizer(wxHORIZONTAL);
	sizer3->Add(sizer4, 0, wxALIGN_CENTER_VERTICAL|wxGROW);

	m_patternCtrl = new SjHistoryComboBox(this, IDC_PATTERNCB, 360, m_pattern);
	sizer4->Add(m_patternCtrl, 1/*grow*/, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

	m_insertButton = new SjInsertButton(this, IDC_INSERT_BUTTON, IDC_INSERT_FIRST);
	m_insertButton->AddWidthOption  (wxT("<Title>"),    _("Title"));
	m_insertButton->AddSep          ();
	m_insertButton->AddOption       (wxT("<Nr>"),       _("Track number"));
	m_insertButton->AddOption       (wxT("<Nr(1)>"),    _("Track number (1 character)"));
	m_insertButton->AddOption       (wxT("<Nr(2)>"),    _("Track number (2 characters)"));
	m_insertButton->AddOption       (wxT("<DiskNr>"),   _("Disk number"));
	m_insertButton->AddOption       (wxT("<DiskNr(1)>"),_("Disk number (1 character)"));
	m_insertButton->AddOption       (wxT("<DiskNr(2)>"),_("Disk number (2 characters)"));
	m_insertButton->AddSep          ();
	m_insertButton->AddWidthOption  (wxT("<Artist>"),   _("Artist"));
	m_insertButton->AddWidthOption  (wxT("<OrgArtist>"),_("Original artist"));
	m_insertButton->AddWidthOption  (wxT("<Composer>"), _("Composer"));
	m_insertButton->AddSep          ();
	m_insertButton->AddWidthOption  (wxT("<Album>"),    _("Album"));
	m_insertButton->AddSep          ();
	m_insertButton->AddWidthOption  (wxT("<Genre>"),    _("Genre"));
	m_insertButton->AddWidthOption  (wxT("<Group>"),    _("Group"));
	m_insertButton->AddWidthOption  (wxT("<Comment>"),  _("Comment"));
	m_insertButton->AddSep          ();
	m_insertButton->AddOption       (wxT("<Year>"),     _("Year"));
	m_insertButton->AddOption       (wxT("<Year(2)>"),  _("Year (2 characters)"));
	m_insertButton->AddOption       (wxT("<Year(4)>"),  _("Year (4 characters)"));
	m_insertButton->AddSep          ();
	m_insertButton->AddOption       (wxT("/"),          _("Directory change"));
	m_insertButton->AddOption       (wxT("*"),          _("Void information"));
	sizer4->Add(m_insertButton, 0, wxALIGN_CENTER_VERTICAL);

	// example
	wxStaticText* staticText = new wxStaticText(this, -1, _("Example:"));
	sizer3->Add(staticText, 0, wxALIGN_TOP|wxALIGN_RIGHT);

	m_exampleCtrl = new wxStaticText(this, -1, wxT("\n\n\n\n\n\n\n\n\n"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
	sizer3->Add(m_exampleCtrl, 0, wxALIGN_TOP|wxGROW);

	UpdateExample();
}


void SjSplitPlugin::UpdateExample()
{
	if( m_exampleCtrl && m_exampleTrackInfo )
	{
		// perform the example split
		long fieldToSplit = SjDialog::GetCbSelection(m_splitInChoice);
		if( fieldToSplit == 0 ) fieldToSplit = m_splitIn;
		m_matcher.Compile(fieldToSplit, m_patternCtrl->GetValue());

		SjTrackInfo dest;
		m_matcher.Match(*m_exampleTrackInfo, dest);

		// build example string
		wxString summary;
		int i;
		for( i = 0; i <= 31; i++ )
		{
			if( dest.m_validFields & (1<<i) )
			{
				summary += wxT("\n") + SjTrackInfo::GetFieldDescr(1<<i) + wxT(": ") + dest.GetValue(1<<i);
			}
		}

		if( summary.IsEmpty() )
		{
			summary = wxT("\n") + SjTrackInfo::GetFieldDescr(SJ_TI_TRACKNAME) + wxString(wxT(": ")) + _("n/a");
		}

		wxString fieldToSplitValue = m_exampleTrackInfo->GetValue(fieldToSplit);
		if( fieldToSplitValue.IsEmpty() )
		{
			fieldToSplitValue = _("n/a");
		}

		summary.Prepend(SjTrackInfo::GetFieldDescr(fieldToSplit) + wxString(wxT(": ")) + fieldToSplitValue + wxT("\n"));

		m_exampleCtrl->SetLabel(summary);
	}
}


bool SjSplitPlugin::PrepareModify()
{
	// save settings
	m_pattern = m_patternCtrl->GetHistory();
	g_tools->WriteArray(wxT("tageditor/splitPattern"), m_pattern);

	long newSplitIn = SjDialog::GetCbSelection(m_splitInChoice);
	if( newSplitIn /*avoid sep. selection*/)
	{
		m_splitIn = newSplitIn;
		g_tools->m_config->Write(wxT("tageditor/splitIn"), m_splitIn);
	}

	m_matcher.Compile(m_splitIn, m_pattern[0]);

	return TRUE;
}


void SjSplitPlugin::ModifyTrackInfo(SjTrackInfo& retTi, int index, SjModifyInfo& mod)
{
	SjTrackInfo destTi;

	m_matcher.Match(retTi, destTi);

	int i;
	for( i = 0; i < 31; i++ )
	{
		if( destTi.m_validFields & (1<<i) )
		{
			mod.Add(1<<i, destTi.GetValue(1<<i));
		}
	}
}
