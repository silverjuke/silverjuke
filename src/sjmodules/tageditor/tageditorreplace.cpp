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
 * File:    tageditorreplace.cpp
 * Authors: Björn Petersen
 * Purpose: Tag editor replace plugin
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/tageditor/tageditor.h>
#include <sjmodules/tageditor/tageditorplugin.h>
#include <sjmodules/tageditor/tageditorreplace.h>


SjReplacePlugin::SjReplacePlugin(wxWindow* parent, SjTrackInfo* exampleTrackInfo)
	: SjTagEditorPlugin(parent, wxT("replace"), _("Replace"), exampleTrackInfo)
{
	// read configuration
	m_searchFor = g_tools->ReadArray(wxT("tageditor/searchFor"));
	if( m_searchFor.IsEmpty() ) m_searchFor.Add(wxT(""));

	m_replaceWith = g_tools->ReadArray(wxT("tageditor/replaceWith"));
	if( m_replaceWith.IsEmpty() ) m_replaceWith.Add(wxT(""));

	m_searchIn = g_tools->m_config->Read(wxT("tageditor/searchIn"), SJ_TI_TRACKNAME);
	m_searchFlags = g_tools->m_config->Read(wxT("tageditor/searchFlags"), wxRE_ICASE);

	// create dialog
	wxSizer* sizer2 = new wxStaticBoxSizer(new wxStaticBox(this, -1, wxEmptyString), wxVERTICAL);
	m_sizer1->Add(sizer2, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	wxFlexGridSizer* sizer3 = new wxFlexGridSizer(4, 0, SJ_DLG_SPACE);
	sizer3->AddGrowableCol(1);
	sizer2->Add(sizer3, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	// search for
	sizer3->Add(new wxStaticText(this, -1, _("Search for:")), 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM, SJ_DLG_SPACE/2);

	m_searchForTextCtrl = new SjHistoryComboBox(this, -1, 200, m_searchFor);
	sizer3->Add(m_searchForTextCtrl, 0, wxALIGN_CENTER_VERTICAL|wxGROW|wxBOTTOM, SJ_DLG_SPACE/2);

	// search in
	sizer3->Add(new wxStaticText(this, -1, _("in:")), 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, SJ_DLG_SPACE/2);

	m_searchInChoice = new SjTrackInfoFieldChoice(this, -1);
	m_searchInChoice->AppendFlags(REPL_FIELDS);
	m_searchInChoice->Append(_("All fields"), (void*)REPL_FIELDS);
	m_searchInChoice->Append(SJ_CHOICE_SEP, (void*)0);
	m_searchInChoice->Append(_("File name"), (void*)REPL_FILENAME);
	m_searchInChoice->Append(_("Path and file name"), (void*)SJ_TI_URL);
	SjDialog::SetCbSelection(m_searchInChoice, m_searchIn);
	sizer3->Add(m_searchInChoice, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, SJ_DLG_SPACE/2);

	// replace with
	sizer3->Add(new wxStaticText(this, -1, _("Replace with:")), 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxBOTTOM, SJ_DLG_SPACE/2);

	m_replaceWithTextCtrl = new SjHistoryComboBox(this, -1, -1, m_replaceWith);
	sizer3->Add(m_replaceWithTextCtrl, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, SJ_DLG_SPACE/2);

	// match whole words only?
	sizer3->Add(2, 2);

	m_matchWholeWordsOnly = new wxCheckBox(this, -1, _("Match whole words only"));
	m_matchWholeWordsOnly->SetValue(m_searchFlags&wxRE_WHOLEWORDS? TRUE : FALSE);
	sizer3->Add(m_matchWholeWordsOnly, 0, wxALIGN_BOTTOM|wxBOTTOM, SJ_DLG_SPACE/2);

	// match case?
	sizer3->Add(2, 2);
	sizer3->Add(2, 2);
	sizer3->Add(2, 2);

	m_matchCase = new wxCheckBox(this, -1, _("Match case"));
	m_matchCase->SetValue(m_searchFlags&wxRE_ICASE? FALSE : TRUE);
	sizer3->Add(m_matchCase, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, SJ_DLG_SPACE/2);

	// match regex?
	sizer3->Add(2, 2);
	sizer3->Add(2, 2);
	sizer3->Add(2, 2);

	m_matchRegEx = new wxCheckBox(this, -1, _("Regular expression"));
	m_matchRegEx->SetValue(m_searchFlags&wxRE_REGEX? TRUE : FALSE);
	sizer3->Add(m_matchRegEx, 0, wxALIGN_CENTER_VERTICAL);
}


bool SjReplacePlugin::PrepareModify()
{
	// save settings
	m_searchFor = m_searchForTextCtrl->GetHistory();
	g_tools->WriteArray(wxT("tageditor/searchFor"), m_searchFor);

	m_replaceWith = m_replaceWithTextCtrl->GetHistory();
	g_tools->WriteArray(wxT("tageditor/replaceWith"), m_replaceWith);

	long newSearchIn = SjDialog::GetCbSelection(m_searchInChoice);
	if( newSearchIn /*avoid sep. selection*/)
	{
		m_searchIn = newSearchIn;
		g_tools->m_config->Write(wxT("tageditor/searchIn"), m_searchIn);
	}

	SjTools::SetFlag(m_searchFlags, wxRE_WHOLEWORDS, m_matchWholeWordsOnly->IsChecked());
	SjTools::SetFlag(m_searchFlags, wxRE_ICASE, m_matchCase->IsChecked()? FALSE : TRUE);
	SjTools::SetFlag(m_searchFlags, wxRE_REGEX, m_matchRegEx->IsChecked());
	g_tools->m_config->Write(wxT("tageditor/searchFlags"), m_searchFlags);

	// anything to search for?
	m_replacementCount = 0;
	if(  m_searchFor[0].IsEmpty()
	        || !m_replacer.Compile(m_searchFor[0], m_replaceWith[0], m_searchFlags) )
	{
		return FALSE;
	}

	// done
	return TRUE;
}


void SjReplacePlugin::ModifyField(SjTrackInfo& trackInfo, long field, SjModifyInfo& mod)
{
	wxString scope;
	if( field == REPL_FILENAME )
	{
		scope = SjTools::GetFileNameFromUrl(trackInfo.m_url, NULL, TRUE/*strip extension*/);
		if( m_replacer.ReplaceAll(scope) > 0 )
		{
			scope = SjTools::SetFileNameToUrl(trackInfo.m_url, scope, TRUE/*leave extension*/);
			mod.Add(SJ_TI_URL, scope);
			m_replacementCount++;
		}
	}
	else
	{
		scope = trackInfo.GetValue(field);
		if( m_replacer.ReplaceAll(scope) > 0 )
		{
			mod.Add(field, scope);
			m_replacementCount++;
		}
	}
}


void SjReplacePlugin::ModifyTrackInfo(SjTrackInfo& trackInfo, int index, SjModifyInfo& mod)
{
	if( m_searchIn == REPL_FIELDS /*all*/ )
	{
		int i;
		for( i = 0; i < 31; i++ )
		{
			if( REPL_FIELDS & (1<<i) )
			{
				ModifyField(trackInfo, 1<<i, mod);
			}
		}
	}
	else
	{
		ModifyField(trackInfo, m_searchIn, mod);
	}
}


wxString SjReplacePlugin::PostModify()
{
	if( m_replacementCount == 0 )
	{
		return wxString::Format(_("\"%s\" not found."), m_searchFor[0].c_str());
	}

	return wxT("");
}

