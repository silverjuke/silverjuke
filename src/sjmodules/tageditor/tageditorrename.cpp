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
 * File:    tageditorrename.cpp
 * Authors: Björn Petersen
 * Purpose: Tag editor rename plugin
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/tageditor/tageditor.h>
#include <sjmodules/tageditor/tageditorplugin.h>
#include <sjmodules/tageditor/tageditorrename.h>


SjTrackInfoReplacer::SjTrackInfoReplacer(SjTrackInfo* ti)
{
	m_ti = ti;
	m_slashReplacer.Compile(wxT("[[:space:]]*[\\/]+[[:space:]]*"));
}


bool SjTrackInfoReplacer::GetReplacement(const wxString& placeholder, long flags, wxString& replacement)
{
	if( placeholder == wxT("title") )       { replacement = SjTools::EnsureValidFileNameChars(ApplyFlags(m_ti->m_trackName,     flags));    return TRUE; }
	if( placeholder == wxT("disknr") )      { replacement =                                   ApplyFlags(m_ti->m_diskNr,        flags);     return TRUE; }
	if( placeholder == wxT("diskcount") )   { replacement =                                   ApplyFlags(m_ti->m_diskCount,     flags);     return TRUE; }
	if( placeholder == wxT("artist") )      { replacement = SjTools::EnsureValidFileNameChars(ApplyFlags(m_ti->m_leadArtistName,flags));    return TRUE; }
	if( placeholder == wxT("orgartist") )   { replacement = SjTools::EnsureValidFileNameChars(ApplyFlags(m_ti->m_orgArtistName, flags));    return TRUE; }
	if( placeholder == wxT("composer") )    { replacement = SjTools::EnsureValidFileNameChars(ApplyFlags(m_ti->m_composerName,  flags));    return TRUE; }
	if( placeholder == wxT("album") )       { replacement = SjTools::EnsureValidFileNameChars(ApplyFlags(m_ti->m_albumName,     flags));    return TRUE; }
	if( placeholder == wxT("genre") )       { replacement = SjTools::EnsureValidFileNameChars(ApplyFlags(m_ti->m_genreName,     flags));    return TRUE; }
	if( placeholder == wxT("group") )       { replacement = SjTools::EnsureValidFileNameChars(ApplyFlags(m_ti->m_groupName,     flags));    return TRUE; }
	if( placeholder == wxT("year") )        { replacement =                                   ApplyFlags(m_ti->m_year,          flags);     return TRUE; }

	size_t width = flags & SJ_PLR_WIDTH;

	if( placeholder == wxT("nr") )
	{
		if( width==0 ) { flags|=2; }
		replacement = ApplyFlags(m_ti->m_trackNr, flags);
		return TRUE;
	}

	if( placeholder == wxT("count") )
	{
		if( width==0 ) { flags|=2; }

		replacement = ApplyFlags(m_ti->m_trackCount, flags);
		return TRUE;
	}

	if( placeholder == wxT("time") )
	{
		replacement = SjTools::FormatTime(m_ti->m_playtimeMs/1000, SJ_FT_ALLOW_ZERO);
		replacement.Replace(wxT(":"), wxT("'"));
		while ( width > replacement.Len() ) { replacement.Prepend(wxT('0')); }

		replacement = SjTools::EnsureValidFileNameChars(replacement);
		return TRUE;
	}

	if( placeholder == wxT("filename") )
	{
		replacement = ApplyFlags(SjTools::GetFileNameFromUrl(m_ti->m_url, NULL, TRUE), flags);
		return TRUE;
	}

	if( placeholder == wxT("ext") )
	{
		replacement = ApplyFlags(SjTools::GetExt(m_ti->m_url), flags);
		return TRUE;
	}

	return FALSE;
}


void SjTrackInfoReplacer::ReplacePath(wxString& text, SjTrackInfo* ti)
{
	// use the given track info object, if any
	if( ti )
	{
		m_ti = ti;
	}

	// remove spaces before/after path seperators and at the
	// beginning/the end of the pattern
	int numPathSep = m_slashReplacer.ReplaceAll(&text, wxT("/"));

	text.Trim(TRUE);
	text.Trim(FALSE);

	// replace the placeholders themselves
	ReplaceAll(text);

	if( text.IsEmpty() )
	{
		// make sure, the new name is not empty, just use the
		// old name in this case
		text = SjTools::GetFileNameFromUrl(m_ti->m_url, NULL, FALSE);
	}
	else if( GetFinalPlaceholder()!=wxT("ext") )
	{
		// make sure, the text ends with an extension
		if( text.Last() != '.' ) text.Append('.');
		text.Append(SjTools::GetExt(m_ti->m_url));
	}

	// make sure, the result is a valid path
	if( numPathSep )
	{
		text = SjTools::EnsureValidPathChars(text);
	}
	else
	{
		text = SjTools::EnsureValidFileNameChars(text);
	}

	// prepend (parts) of the original URL, if the current path is not yet absolute
	if( !SjTools::IsAbsUrl(text) )
	{
		wxString path;
		SjTools::GetFileNameFromUrl(ti->m_url, &path, FALSE, TRUE);
		for( ; numPathSep > 0; numPathSep-- )
		{
			wxString oldPath(path);
			SjTools::GetFileNameFromUrl(oldPath, &path, FALSE, TRUE);
		}

		if( text[0u] != wxT('/') )
		{
			text.Prepend(wxT('/'));
		}

		text.Prepend(path);
	}
}


#define IDC_PATTERNCB       (IDM_FIRSTPRIVATE+1)
#define IDC_INSERT_BUTTON   (IDM_FIRSTPRIVATE+2)
#define IDC_SPLITIN         (IDM_FIRSTPRIVATE+3)
#define IDC_INSERT_FIRST    (IDM_FIRSTPRIVATE+100)
#define IDC_INSERT_LAST     (IDM_FIRSTPRIVATE+199)


BEGIN_EVENT_TABLE(SjRenamePlugin, SjTagEditorPlugin)
	EVT_BUTTON                  (IDC_INSERT_BUTTON,     SjRenamePlugin::OnInsert        )
	EVT_MENU_RANGE              (IDC_INSERT_FIRST,
	                             IDC_INSERT_LAST,       SjRenamePlugin::OnInsert        )
	EVT_TEXT                    (IDC_PATTERNCB,         SjRenamePlugin::OnPatternChange )
	EVT_COMBOBOX                (IDC_PATTERNCB,         SjRenamePlugin::OnPatternChange )
END_EVENT_TABLE()


SjRenamePlugin::SjRenamePlugin(wxWindow* parent, SjTrackInfo* exampleTrackInfo)
	: SjTagEditorPlugin(parent, wxT("rename"), _("Rename files"), exampleTrackInfo)
{
	// load configuration
	m_pattern = g_tools->ReadArray(wxT("tageditor/renamePattern"));
	if( m_pattern.IsEmpty() )
	{
		m_pattern.Add(wxT("<Nr> <Artist> - <Title>"));
	}

	// create dialog
	wxSizer* sizer2 = new wxStaticBoxSizer(new wxStaticBox(this, -1, wxT("")), wxVERTICAL);
	m_sizer1->Add(sizer2, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	wxFlexGridSizer* sizer3 = new wxFlexGridSizer(2, SJ_DLG_SPACE, SJ_DLG_SPACE);
	sizer3->AddGrowableCol(1);
	sizer3->AddGrowableRow(1);
	sizer2->Add(sizer3, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	// pattern
	sizer3->Add(new wxStaticText(this, -1, _("Pattern:")), 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);

	wxSizer* sizer4 = new wxBoxSizer(wxHORIZONTAL);
	sizer3->Add(sizer4, 0, wxALIGN_CENTER_VERTICAL|wxGROW);

	m_patternCtrl = new SjHistoryComboBox(this, IDC_PATTERNCB, 360, m_pattern);
	sizer4->Add(m_patternCtrl, 1/*grow*/, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

	m_insertButton = new SjInsertButton(this, IDC_INSERT_BUTTON, IDC_INSERT_FIRST);
	m_insertButton->AddCaseOption(wxT("<Title>"),       _("Title"));
	m_insertButton->AddSep       ();
	m_insertButton->AddOption    (wxT("<Nr>"),          _("Track number"));
	m_insertButton->AddOption    (wxT("<Count>"),       _("Track count"));
	m_insertButton->AddOption    (wxT("<DiskNr>"),      _("Disk number"));
	m_insertButton->AddOption    (wxT("<DiskCount>"),   _("Disk count"));
	m_insertButton->AddSep       ();
	m_insertButton->AddCaseOption(wxT("<Artist>"),      _("Artist"));
	m_insertButton->AddOption    (wxT("<ARTIST(1)>"),   _("Artist")+wxString(wxT(" "))+_("(first character)"));
	m_insertButton->AddCaseOption(wxT("<OrgArtist>"),   _("Original artist"));
	m_insertButton->AddCaseOption(wxT("<Composer>"),    _("Composer"));
	m_insertButton->AddSep       ();
	m_insertButton->AddCaseOption(wxT("<Album>"),       _("Album"));
	m_insertButton->AddOption    (wxT("<ALBUM(1)>"),    _("Album")+wxString(wxT(" "))+_("(first character)"));
	m_insertButton->AddSep       ();
	m_insertButton->AddCaseOption(wxT("<Genre>"),       _("Genre"));
	m_insertButton->AddOption    (wxT("<GENRE(1)>"),    _("Genre")+wxString(wxT(" "))+_("(first character)"));
	m_insertButton->AddCaseOption(wxT("<Group>"),       _("Group"));
	m_insertButton->AddOption    (wxT("<GROUP(1)>"),    _("Group")+wxString(wxT(" "))+_("(first character)"));
	m_insertButton->AddSep       ();
	m_insertButton->AddOption    (wxT("<Year>"),        _("Year")+wxString(wxT(" "))+wxString::Format(_("(%i characters)"), 4));
	m_insertButton->AddOption    (wxT("<Year(2)>"),     _("Year")+wxString(wxT(" "))+wxString::Format(_("(%i characters)"), 2));
	m_insertButton->AddSep       ();
	m_insertButton->AddOption    (wxT("<Time>"),        _("Duration"));
	m_insertButton->AddOption    (wxT("<Time(5)>"),     _("Duration")+wxString(wxT(" "))+wxString::Format(_("(%i characters)"), 5));
	m_insertButton->AddSep       ();
	m_insertButton->AddOption    (wxT("/"),             _("Directory change"));
	m_insertButton->AddCaseOption(wxT("<Filename>"),    _("File name"));
	m_insertButton->AddOption    (wxT("<FILENAME(8)>"), _("File name")+wxString(wxT(" "))+wxString::Format(_("(%i characters)"), 8));
	m_insertButton->AddCaseOption(wxT("<Ext>"),         _("File extension"));
	sizer4->Add(m_insertButton, 0, wxALIGN_CENTER_VERTICAL);

	// example
	wxStaticText* staticText = new wxStaticText(this, -1, _("Example:"));
	sizer3->Add(staticText, 0, wxALIGN_TOP|wxALIGN_RIGHT);

	m_exampleCtrl = new wxStaticText(this, -1, wxT("\n\n"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
	sizer3->Add(m_exampleCtrl, 0, wxALIGN_TOP|wxGROW);

	UpdateExample();
}


void SjRenamePlugin::UpdateExample()
{
	if( m_exampleCtrl && m_exampleTrackInfo )
	{
		wxString pathPartNFileName = m_patternCtrl->GetValue();
		m_tiReplacer.ReplacePath(pathPartNFileName, m_exampleTrackInfo);

#ifdef __WXMSW__
		pathPartNFileName.Replace(wxT("&"), wxT("&&")); // "&" is converted to an underscore for the next character on MSW
#endif

		m_exampleCtrl->SetLabel(pathPartNFileName);
	}
}


bool SjRenamePlugin::PrepareModify()
{
	// save settings
	m_pattern = m_patternCtrl->GetHistory();
	g_tools->WriteArray(wxT("tageditor/renamePattern"), m_pattern);

	return TRUE;
}


void SjRenamePlugin::ModifyTrackInfo(SjTrackInfo& trackInfo, int index, SjModifyInfo& mod)
{
	wxString newPath(m_pattern[0]);
	m_tiReplacer.ReplacePath(newPath, &trackInfo);

	mod.Add(SJ_TI_URL, newPath);
}
