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
 * File:    library.cpp
 * Authors: Björn Petersen
 * Purpose: The "Album generator" module
 *
 *******************************************************************************
 *
 * We're using the following strategy to build the albums out of a number
 * of unordered tracks:
 *
 * (1)  sort and group the tracks by "leadArtistName, albumName"
 * (2)  all groups with equal or more than N1 tracks are treated as an album
 * (3)  repeat steps (1) and (2) for sorting and grouping by "albumName" (3b),
 *      "leadArtistName" (3c) and (optionally) "genreName" (3d).
 *      all groups from (3b) are so called "compilations" aka "samplers"
 * (4)  the rest is also treated as a single album with the name
 *      "Unsorted Tracks"
 * (5)  check for identical albumNames from step (3b), merge "small"
 *      albums of less than N2 tracks into bigger ones.
 *
 * N1 and N2 default values below.
 *
 *******************************************************************************
 *
 * Some words about scanning speed:
 *
 * Massive Speed Improvement (30s -> less than 2s for 8000 tracks) in
 * UpdateAlbums() by loading all needed track data into memory.
 * Further Speed Improvements on all Updating by tracking the "updated" state
 * in memory instead of the database (esp. the "UPDATE tracks SET updated=1;"
 * was very time expensive as we use a transaction. Checking for updates in
 * 8000 tracks is done in about 5s instead of 35s; (re-)creating the library
 * "from scratch" is about 20s faster.
 *
 * Some measured times for reading about 7000 files from the folderscanner:
 *
 * - Silverjuke:              2'40 (although using slower wxFileSystem for
 *                                 archive support; compared to (*), the
 *                                 speed comes from using 4 KB cache and
 *                                 wxID3Reader  instead of the local file
 *                                 system; other improvements from
 *                                 forwarding wxInputStream to all modules
 *                                 instead of using only the URLs)
 * - Silverjuke
 *   without ID3 reading:     0'17
 *
 * - Silverjuke with
 *   ":memory:" database:     2'00 (okay, little faster, but the database
 *                                 also needs up to 20 MB extra RAM which
 *                                 are not read/written back in this test)
 * - Silverjuke with
 *   ":memory:" database,
 *   without ID3 reading:     0'07 (so there is room for further ID3
 *                                 reader optimizations, maybe we should
 *                                 replace id3lib (which is also very
 *                                 large, see mainframe.cpp) with sth. else)
 *  Compared to other Systems:
 *
 *  - Old Silverjuke:          4'10 (*) see above
 *  - iTunes:                  6'45
 *  - Winamp:                  3'10
 *  - wxMusic:                 3'30
 *
 *  Searching Times:
 *
 *  - Avg. Search time:        60 ms
 *  - Avg. Search time with
 *    ":memory:" database:     25 ms
 *
 *  Silverjuke times measured with 2 MB sqlite cache. The database was
 *  initially empty. The 7000 files do not contain errorous files.
 *
 *  Test System: Windows 2000, 490 MB RAM, AMD Athlon XP 2400 MHz
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/tokenzr.h>
#include <wx/spinctrl.h>
#include <sjtools/imgthread.h>
#include <sjbase/browser.h>
#include <sjbase/columnmixer.h>
#include <sjtools/msgbox.h>
#include <sjmodules/library.h>
#include <sjmodules/kiosk/kiosk.h>
#include <sjmodules/advsearch.h>
#include <sjmodules/arteditor.h>
#include <sjmodules/tageditor/tageditor.h>
#include <sjmodules/weblinks.h>


#define DEFAULT_OMIT_ARTISTS    SjTools::LocaleConfigRead(wxT("__STOP_ARTISTS__"), wxT("the, der, die, die happy, das"))
#define DEFAULT_OMIT_ALBUMS     SjTools::LocaleConfigRead(wxT("__STOP_ALBUMS__"), wxT(""))

#define DEFAULT_COVER_KEYWORDS  SjTools::LocaleConfigRead(wxT("__COVER_KEYWORDS__"), wxT("front, vorn, outside, cover"))

#define DEFAULT_N1              2
#define MIN_N1                  1
#define MAX_N1                  99

#define DEFAULT_N2              4
#define MIN_N2                  2
#define MAX_N2                  99


#define IDM_DEFAULT             (IDM_FIRSTPRIVATE+16)


/*******************************************************************************
 * SjLibraryConfigDlg Class
 ******************************************************************************/


class SjLibraryConfigPage : public wxPanel
{
public:
	SjLibraryConfigPage (SjLibraryModule* module, wxWindow* parent);

private:
	SjLibraryModule*
	m_module;
	wxSpinCtrl*     m_n1SpinCtrl;
	wxSpinCtrl*     m_n2SpinCtrl;
	wxChoice*       m_createAlbumsByChoice;
	wxChoice*       m_sortByChoice;
	wxTextCtrl*     m_omitArtistWordsTextCtrl;
	wxTextCtrl*     m_omitAlbumWordsTextCtrl;
	wxCheckBox*     m_omitToEndCheck;
	wxTextCtrl*     m_coverKeywordsTextCtrl;

	void            OnDefault           (wxCommandEvent&);
	DECLARE_EVENT_TABLE ()

	friend class    SjLibraryModule;

};


BEGIN_EVENT_TABLE(SjLibraryConfigPage, wxPanel)
	EVT_BUTTON  (IDM_DEFAULT,   SjLibraryConfigPage::OnDefault  )
END_EVENT_TABLE()


SjLibraryConfigPage::SjLibraryConfigPage(SjLibraryModule* module, wxWindow* parent)
	: wxPanel(parent, -1)
{
	// save given objects
	m_module = module;

	// init dialog
	wxBoxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	sizer1->Add(1, SJ_DLG_SPACE); // some space

	wxStaticText* staticText = new wxStaticText(this, -1, _("The following settings are for experienced users only."));
	sizer1->Add(staticText, 0, wxALL, SJ_DLG_SPACE);

	// n1
	#define N1_N2_CHOICE_W 50
	wxBoxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2, 0/*grow*/, wxLEFT|wxTOP|wxRIGHT|wxGROW, SJ_DLG_SPACE);

	wxString temp = _("An album must have at least %s tracks with corresponding %s.");
	sizer2->Add(new wxStaticText(this, -1, temp.BeforeFirst('%')), 0, wxALIGN_CENTER_VERTICAL, 0);
	temp = temp.AfterFirst('%').Mid(1);

	m_n1SpinCtrl = new wxSpinCtrl(this, -1, wxString::Format(wxT("%i"), (int)module->m_n1), wxDefaultPosition, wxSize(N1_N2_CHOICE_W, -1), wxSP_ARROW_KEYS,
	                              MIN_N1, MAX_N1, module->m_n1);
	sizer2->Add(m_n1SpinCtrl, 0/*grow*/, wxALIGN_CENTER_VERTICAL, 0);

	sizer2->Add(new wxStaticText(this, -1, temp.BeforeFirst('%')), 0, wxALIGN_CENTER_VERTICAL, 0);

	m_createAlbumsByChoice = new wxChoice(this, -1, wxDefaultPosition, wxSize(130, -1));
	m_createAlbumsByChoice->Append(_("artist/album"),           (void*)0);
	m_createAlbumsByChoice->Append(_("artist/album or genre"),  (void*)SJ_LIB_CREATEALBUMSBY_GENRE);
	m_createAlbumsByChoice->Append(_("directory"),              (void*)SJ_LIB_CREATEALBUMSBY_DIR);
	SjDialog::SetCbSelection(m_createAlbumsByChoice, module->m_flags&SJ_LIB_CREATEALBUMSBY_MASK);
	sizer2->Add(m_createAlbumsByChoice, 0/*1 - looks bad on large windows*/, wxALIGN_CENTER_VERTICAL);

	sizer2->Add(new wxStaticText(this, -1, temp.AfterFirst('%').Mid(1)), 0, wxALIGN_CENTER_VERTICAL, 0);

	// some space
	sizer1->Add(2, 2);

	// n2
	sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer2, 0/*grow*/, wxLEFT|wxRIGHT|wxBOTTOM|wxGROW, SJ_DLG_SPACE);

	temp = _("Compilations may contain up to %s tracks of the same artist.");
	sizer2->Add(new wxStaticText(this, -1, temp.BeforeFirst(wxT('%'))), 0/*grow*/, wxALIGN_CENTER_VERTICAL, 0);

	m_n2SpinCtrl = new wxSpinCtrl(this, -1, wxString::Format(wxT("%i"), (int)module->m_n2), wxDefaultPosition, wxSize(N1_N2_CHOICE_W, -1), wxSP_ARROW_KEYS,
	                              MIN_N2, MAX_N2, module->m_n2);

	sizer2->Add(m_n2SpinCtrl, 0/*grow*/, wxALIGN_CENTER_VERTICAL, 0);

	sizer2->Add(new wxStaticText(this, -1, temp.AfterFirst(wxT('%')).Mid(1)), 0, wxALIGN_CENTER_VERTICAL, 0);

	// order, omit etc.
	wxFlexGridSizer* sizer2b = new wxFlexGridSizer(2, SJ_DLG_SPACE/2, SJ_DLG_SPACE);
	sizer2b->AddGrowableCol(1);
	sizer1->Add(sizer2b, 0, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);
	wxString tooltip(_("Separate the words using commas, case is ignored"));

	// order by
	sizer2b->Add(new wxStaticText(this, -1, _("Sort by:")), 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);
	m_sortByChoice = new wxChoice(this, -1);
	m_sortByChoice->Append(wxString::Format(wxT("%s - %s - %s"), _("Artist"), _("Year"), _("Album")));
	m_sortByChoice->Append(wxString::Format(wxT("%s - %s - %s"), _("Artist"), _("Album"), _("Year")));
	m_sortByChoice->Append(wxString::Format(wxT("%s - %s - %s"), _("Album"), _("Year"), _("Artist")));
	m_sortByChoice->Append(wxString::Format(wxT("%s - %s - %s"), _("Album"), _("Artist"), _("Year")));
	m_sortByChoice->SetSelection(module->m_sort);
	SjDialog::SetCbWidth(m_sortByChoice);
	sizer2b->Add(m_sortByChoice, 1, wxALIGN_CENTER_VERTICAL);


	// omit artist words combobox
	sizer2b->Add(new wxStaticText(this, -1, _("Stop-words for sorting artists:")), 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);
	m_omitArtistWordsTextCtrl = new wxTextCtrl(this, -1, module->m_omitArtist.GetWords());
	m_omitArtistWordsTextCtrl->SetToolTip(tooltip);
	sizer2b->Add(m_omitArtistWordsTextCtrl, 0, wxALIGN_CENTER_VERTICAL|wxGROW);

	// omit album words combobox
	sizer2b->Add(new wxStaticText(this, -1, _("Stop-words for sorting albums:")), 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);
	m_omitAlbumWordsTextCtrl = new wxTextCtrl(this, -1, module->m_omitAlbum.GetWords());
	m_omitAlbumWordsTextCtrl->SetToolTip(tooltip);
	sizer2b->Add(m_omitAlbumWordsTextCtrl, 0, wxALIGN_CENTER_VERTICAL|wxGROW);

	// omit to end
	m_omitToEndCheck = new wxCheckBox(this, -1,
	                                  _("Show stop-words words at end"));
	m_omitToEndCheck->SetValue((module->m_flags&SJ_LIB_OMITTOEND)!=0);
	m_omitToEndCheck->SetToolTip(_("If selected, eg. \"The Rolling Stones\" becomes \"Rolling Stones, The\";\nsorting itself is not affected by this option"));
	sizer2b->Add(SJ_DLG_SPACE, SJ_DLG_SPACE);
	sizer2b->Add(m_omitToEndCheck);

	sizer2b->Add(SJ_DLG_SPACE, SJ_DLG_SPACE);
	sizer2b->Add(SJ_DLG_SPACE, SJ_DLG_SPACE);

	sizer2b->Add(new wxStaticText(this, -1, _("Keywords to identify cover image for an album:")), 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);
	m_coverKeywordsTextCtrl = new wxTextCtrl(this, -1, module->m_coverFinder.GetWords());
	m_coverKeywordsTextCtrl->SetToolTip(tooltip);
	sizer2b->Add(m_coverKeywordsTextCtrl, 1, wxALIGN_CENTER_VERTICAL|wxGROW, 0);

	wxButton* button = new wxButton(this, IDM_DEFAULT, _("Reset to default values"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer1->Add(button, 0, wxLEFT|wxBOTTOM|wxRIGHT, SJ_DLG_SPACE);

	// init done, center dialog
	sizer1->SetSizeHints(this);
}


void SjLibraryConfigPage::OnDefault(wxCommandEvent&)
{
	m_n1SpinCtrl->SetValue(DEFAULT_N1);
	m_n2SpinCtrl->SetValue(DEFAULT_N2);
	SjDialog::SetCbSelection(m_createAlbumsByChoice, SJ_LIB_CREATEALBUMSBY_GENRE);
	m_sortByChoice->SetSelection(0);
	m_omitArtistWordsTextCtrl->SetValue(DEFAULT_OMIT_ARTISTS);
	m_omitAlbumWordsTextCtrl->SetValue(DEFAULT_OMIT_ALBUMS);
	m_omitToEndCheck->SetValue((SJ_LIB_DEFAULT&SJ_LIB_OMITTOEND)? TRUE : FALSE);
	m_coverKeywordsTextCtrl->SetValue(DEFAULT_COVER_KEYWORDS);
}


wxWindow* SjLibraryModule::GetConfigPage(wxWindow* parent, int selectedPage)
{
	return new SjLibraryConfigPage(this, parent);
}


void SjLibraryModule::DoneConfigPage(wxWindow* configPage__, int doneAction__)
{
	if( doneAction__ == SJ_CONFIGPAGE_DONE_CANCEL_CLICKED )
		return;

	SjLibraryConfigPage* configPage = (SjLibraryConfigPage*)configPage__;

	bool settingsChanged = FALSE,
	     needsAlbumUpdate = FALSE;

	wxString newOmitArtistWords     = configPage->m_omitArtistWordsTextCtrl->GetValue();
	wxString newOmitAlbumWords      = configPage->m_omitAlbumWordsTextCtrl->GetValue();
	long     newOmitToEnd           = configPage->m_omitToEndCheck->GetValue()? SJ_LIB_OMITTOEND : 0;
	wxString newCoverKeywords       = configPage->m_coverKeywordsTextCtrl->GetValue();
	long     newN1                  = configPage->m_n1SpinCtrl->GetValue();
	long     newN2                  = configPage->m_n2SpinCtrl->GetValue();
	long     newCreateAlbumsBy      = SjDialog::GetCbSelection(configPage->m_createAlbumsByChoice);
	SjLibrarySort newSort           = (SjLibrarySort)configPage->m_sortByChoice->GetSelection();

	if( newN1 < MIN_N1 || newN1 > MAX_N1 ) newN1 = m_n1;
	if( newN2 < MIN_N2 || newN2 > MAX_N2 ) newN2 = m_n2;

	if( newOmitArtistWords      != m_omitArtist.GetWords()
	        || newOmitAlbumWords       != m_omitAlbum.GetWords()
	        || newCoverKeywords        != m_coverFinder.GetWords()
	        || newN1                   != m_n1
	        || newN2                   != m_n2
	        || newCreateAlbumsBy       != (m_flags&SJ_LIB_CREATEALBUMSBY_MASK)
	        || newSort                 != m_sort )
	{
		settingsChanged = TRUE;
		needsAlbumUpdate = TRUE;
	}

	if( newOmitToEnd != (m_flags&SJ_LIB_OMITTOEND) )
	{
		settingsChanged = TRUE;
	}

	if( settingsChanged )
	{
		m_omitArtist.Init(newOmitArtistWords);
		m_omitAlbum.Init(newOmitAlbumWords);
		m_coverFinder.Init(newCoverKeywords);
		m_n1                    = newN1;
		m_n2                    = newN2;
		m_sort                  = newSort;
		m_flags                 = (m_flags&~SJ_LIB_CREATEALBUMSBY_MASK) | newCreateAlbumsBy;
		SjTools::SetFlag(m_flags, SJ_LIB_OMITTOEND, newOmitToEnd!=0);
		SaveSettings();

		if( needsAlbumUpdate )
		{
			SjBusyInfo busy(SjDialog::FindTopLevel(configPage),
			                _("Updating music library"),
			                TRUE,
			                _("If you cancel the update process, your music library may not be up to date.\n\nCancel the update process?"));

			// if we do not end the search and the music selection,
			// the search will be inconsistent after the update --
			// it may be possible to reset if afterwards,
			// but this is more complicated.
			g_mainFrame->EndAllSearch();

			CombineTracksToAlbums();

			g_mainFrame->m_columnMixer.ReloadColumns();
			g_mainFrame->m_browser->ReloadColumnMixer();
		}
		else
		{
			g_mainFrame->m_browser->RefreshAll();
		}
	}
}


/*******************************************************************************
 * SjLibraryModule: Handle little options
 ******************************************************************************/


class SjLittleLibBit : public SjLittleBit
{
public:
	SjLittleLibBit(SjLibraryModule*, const wxString& name, long bitInBitfield, const wxString& options=wxT("yn"), SjIcon icon=SJ_ICON_LITTLEDEFAULT);
	void OnFeedback()
	{
		g_mainFrame->m_browser->RefreshAll();
	}
};


SjLittleLibBit::SjLittleLibBit( SjLibraryModule* module,
                                const wxString& name,
                                long bitInBitfield,
                                const wxString& options,
                                SjIcon icon)
	: SjLittleBit(name, options, &module->m_flags, SJ_LIB_DEFAULT&bitInBitfield, bitInBitfield, wxT("library/flags"), icon)
{
}


void SjLibraryModule::GetLittleOptions(SjArrayLittleOption& lo)
{
	SjLittleOption::SetSection(_("Mouse"));
	lo.Add(new SjLittleLibBit(this, _("Double click on covers"),    SJ_LIB_PLAYONCOVERDBLCLICK,
	                          _("Cover editor")+wxString(wxT("/"))+_("Toggle view")+wxString(wxT("|"))
	                          +_("Select/play album")));
}


/*******************************************************************************
 * SjLibraryModule Constructor etc.
 ******************************************************************************/


SjLibraryModule::SjLibraryModule(SjInterfaceBase* interf)
	: SjColModule(interf)
{
	m_file  = wxT("memory:library.lib");
	m_name  = _("Combine tracks to albums");
	m_searchOffsets = NULL;
	m_searchOffsetsCount = -1; // no search
	m_filterAzFirstHidden = FALSE;
	m_autoVolCount = -1;
	m_hiliteRegExOk = false;

	ForgetRememberedValues();
}


void SjLibraryModule::LoadSettings()
{
	wxConfigBase* config = g_tools->m_config;

	m_n1                    = config->Read(wxT("library/n1"), DEFAULT_N1);
	m_n2                    = config->Read(wxT("library/n2"), DEFAULT_N2);
	m_sort                  = (SjLibrarySort)config->Read(wxT("library/sort"), (long)SJ_LIBSORT_ARTIST_YEAR_ALBUM);
	m_flags                 = config->Read(wxT("library/flags"), SJ_LIB_DEFAULT);
	m_omitArtist.Init       ( config->Read(wxT("library/omitArtistWords"), DEFAULT_OMIT_ARTISTS) );
	m_omitAlbum.Init        ( config->Read(wxT("library/omitAlbumWords"), DEFAULT_OMIT_ALBUMS) );
	m_coverFinder.Init      ( config->Read(wxT("library/coverKeywords"), DEFAULT_COVER_KEYWORDS) );

	if( m_sort < 0 || m_sort >= SJ_LIBSORT_COUNT ) m_sort = SJ_LIBSORT_ARTIST_YEAR_ALBUM;
}


void SjLibraryModule::SaveSettings()
{
	wxConfigBase* config = g_tools->m_config;

	config->Write(wxT("library/n1"),                  m_n1);
	config->Write(wxT("library/n2"),                  m_n2);
	config->Write(wxT("library/sort"),                (long)m_sort);
	config->Write(wxT("library/flags"),               m_flags);
	config->Write(wxT("library/omitArtistWords"),     m_omitArtist.GetWords());
	config->Write(wxT("library/omitAlbumWords"),      m_omitAlbum.GetWords());
	config->Write(wxT("library/coverKeywords"),       m_coverFinder.GetWords());
}


bool SjLibraryModule::FirstLoad()
{
	bool needsRecombiningAlbums = FALSE;

	{
		wxSqlt sql;

		// load settings
		LoadSettings();

		// create track table, if not exists
		if( !sql.TableExists(wxT("tracks")) )
		{
			sql.Query(
			    wxT("CREATE TABLE tracks (")
			    wxT("id INTEGER PRIMARY KEY AUTOINCREMENT, ")// AUTOINCREMENT makes sure, there are
			    wxT("url TEXT, ")                            // NEVER two records with the same ID,
			    wxT("updatecrc INTEGER, ")                   // even if the highes record ID is deleted
			    wxT("timeadded INTEGER, ")
			    wxT("timemodified INTEGER, ")
			    wxT("timesplayed INTEGER, ")
			    wxT("lastplayed INTEGER, ")
			    wxT("databytes INTEGER, ")
			    wxT("bitrate INTEGER, ")
			    wxT("samplerate INTEGER, ")
			    wxT("channels INTEGER, ")
			    wxT("playtimems INTEGER, ")
			    wxT("autovol INTEGER DEFAULT 0, ")
			    wxT("trackname TEXT, ")
			    wxT("tracknr INTEGER, ")
			    wxT("trackcount INTEGER, ")
			    wxT("disknr INTEGER, ")
			    wxT("diskcount INTEGER, ")
			    wxT("leadartistname TEXT, ")
			    wxT("orgartistname TEXT, ")
			    wxT("composername TEXT, ")
			    wxT("albumname TEXT, ")
			    wxT("albumid INTEGER, ")
			    wxT("genrename TEXT, ")
			    wxT("groupname TEXT, ")
			    wxT("comment TEXT, ")
			    wxT("beatsperminute INTEGER, ")
			    wxT("rating INTEGER, ") // 0=unset, 1-5=1-5 stars
			    wxT("year INTEGER, ")
			    wxT("vis INTEGER, ")
			    wxT("artids TEXT);")
			);

			sql.Query(wxT("CREATE INDEX tracksindex01 ON tracks (url);"));
			sql.Query(wxT("CREATE INDEX tracksindex03 ON tracks (trackname);"));
			sql.Query(wxT("CREATE INDEX tracksindex04 ON tracks (leadartistname);"));
			sql.Query(wxT("CREATE INDEX tracksindex05 ON tracks (orgartistname);")); // needed when editing tracks
			sql.Query(wxT("CREATE INDEX tracksindex06 ON tracks (composername);"));     // needed when editing tracks
			sql.Query(wxT("CREATE INDEX tracksindex07 ON tracks (albumname);"));
			sql.Query(wxT("CREATE INDEX tracksindex08 ON tracks (genrename);"));
			sql.Query(wxT("CREATE INDEX tracksindex09 ON tracks (groupname);"));
			if( !sql.Query(wxT("CREATE INDEX tracksindex10 ON tracks (albumid);")) )
			{
				return FALSE;
			}
		}

		// any fields to add?
		// this is only needed to ensure backward compatibility if new fields are needed
		// in newer versions of Silverjuke.
		if( !sql.ColumnExists(wxT("tracks"), wxT("vis")) )
		{
			sql.AddColumn(wxT("tracks"), wxT("vis INTEGER DEFAULT 0")); // added in 15.1beta, may be removed soon
		}

		// create album table, if not exists
		if( !sql.TableExists(wxT("albums")) )
		{
			sql.Query(
			    wxT("CREATE TABLE albums (")
			    wxT("id INTEGER PRIMARY KEY, ")
			    wxT("url TEXT, ")
			    wxT("leadartistname TEXT, ")
			    wxT("albumname TEXT, ")
			    wxT("albumindex INTEGER, ")
			    wxT("az INTEGER, ")
			    wxT("azfirst INTEGER, ")
			    wxT("span INTEGER, ")
			    wxT("spanfirst INTEGER, ")
			    wxT("artidauto INTEGER, ")
			    wxT("artiduser INTEGER);")
			);

			sql.Query(wxT("CREATE INDEX albumsindex01 ON albums (url);"));
			sql.Query(wxT("CREATE INDEX albumsindex04 ON albums (azfirst);"));
			if( !sql.Query(wxT("CREATE INDEX albumsindex05 ON albums (albumindex);")) )
			{
				return FALSE;
			}
		}
	}

	// currently not needed, however, this may be useful for future updates of the library
	if( needsRecombiningAlbums )
	{
		CombineTracksToAlbums();
	}

	return TRUE;
}


void SjLibraryModule::LastUnload()
{
	if( m_searchOffsets )
	{
		free(m_searchOffsets);
		m_searchOffsets = NULL;
	}

	SavePendingData();
}



/*******************************************************************************
 * SjLibraryModule (re-)loading Sources
 ******************************************************************************/


bool SjLibraryModule::WriteTrackInfo(SjTrackInfo* t, long trackId, bool writeArtIds)
{
	wxSqlt sql;

	wxASSERT(trackId>0);

	wxString artIds;
	if( writeArtIds )
	{
		// get art IDs
		wxStringTokenizer tkz(t->m_arts, wxT("\n"));
		wxString currArt;
		long     currArtId;
		while( tkz.HasMoreTokens() )
		{
			currArt = tkz.GetNextToken();
			if( !currArt.IsEmpty() )
			{
				currArtId = 0;
				sql.Query(wxT("SELECT id FROM arts WHERE url='") + sql.QParam(currArt) + wxT("';"));
				if( sql.Next() )
				{
					currArtId = sql.GetLong(0);
				}
				else
				{
					sql.Query(wxT("INSERT INTO arts (url) VALUES ('") + sql.QParam(currArt) + wxT("');"));
					currArtId = sql.GetInsertId();
				}

				artIds << wxString::Format(wxT("%i "), (int)currArtId);
			}
		}

		artIds.Trim();
	}
	else
	{
		// preserve art IDs
		sql.Query(wxString::Format(wxT("SELECT artids FROM tracks WHERE id=%lu;"), trackId));
		if( sql.Next() )
		{
			artIds = sql.GetString(0);
		}
	}

	// write track data
	// we're not writing autovol here; this is done in PlaybackDone()

	if( !sql.Query(
	            wxT("UPDATE tracks SET ")
	            wxT("updatecrc=")       + sql.UParam(t->m_updatecrc)        + wxT(", ")
	            wxT("timemodified=")    + sql.UParam(t->m_timeModified)     + wxT(", ")
	            wxT("lastplayed=")      + sql.UParam(t->m_lastPlayed)       + wxT(", ")
	            wxT("timesplayed=")     + sql.UParam(t->m_timesPlayed)      + wxT(", ")
	            wxT("databytes=")       + sql.UParam(t->m_dataBytes)        + wxT(", ")
	            wxT("bitrate=")         + sql.UParam(t->m_bitrate)          + wxT(", ")
	            wxT("samplerate=")      + sql.UParam(t->m_samplerate)       + wxT(", ")
	            wxT("channels=")        + sql.UParam(t->m_channels)         + wxT(", ")
	            wxT("playtimems=")      + sql.UParam(t->m_playtimeMs)       + wxT(", ")
	            wxT("trackname='")      + sql.QParam(t->m_trackName)        + wxT("', ")
	            wxT("tracknr=")         + sql.UParam(t->m_trackNr)          + wxT(", ")
	            wxT("trackcount=")      + sql.UParam(t->m_trackCount)       + wxT(", ")
	            wxT("disknr=")          + sql.UParam(t->m_diskNr)           + wxT(", ")
	            wxT("diskcount=")       + sql.UParam(t->m_diskCount)        + wxT(", ")
	            wxT("leadartistname='") + sql.QParam(t->m_leadArtistName)   + wxT("', ")
	            wxT("orgartistname='")  + sql.QParam(t->m_orgArtistName)    + wxT("', ")
	            wxT("composername='")   + sql.QParam(t->m_composerName)     + wxT("', ")
	            wxT("albumname='")      + sql.QParam(t->m_albumName)        + wxT("', ")
	            wxT("genrename='")      + sql.QParam(t->m_genreName)        + wxT("', ")
	            wxT("groupname='")      + sql.QParam(t->m_groupName)        + wxT("', ")
	            wxT("comment='")        + sql.QParam(t->m_comment)          + wxT("', ")
	            wxT("beatsperminute=")  + sql.UParam(t->m_beatsPerMinute)   + wxT(", ")
	            wxT("rating=")          + sql.UParam(t->m_rating)           + wxT(", ")
	            wxT("year=")            + sql.UParam(t->m_year)             + wxT(", ")
	            wxT("artids='")         + artIds                            + wxT("' ")
	            wxT(" WHERE id=") + sql.UParam(trackId) + wxT(";")) )
	{
		return FALSE;
	}

	// update the URL?
	if( t->m_validFields & SJ_TI_URL )
	{
		sql.Query(wxT("UPDATE tracks SET url='") + sql.QParam(t->m_url) + wxT("' WHERE id=") + sql.UParam(trackId) + wxT(";"));
	}

	return TRUE;
}


bool SjLibraryModule::Callback_MarkAsUpdated(const wxString& urlBegin, long checkTrackCount)
{
	if( !m_deepUpdate && checkTrackCount > 0 )
	{
		wxSqlt sql;

		sql.Query(wxT("SELECT COUNT(*) FROM tracks WHERE url LIKE '") + sql.QParam(urlBegin) + wxT("%'"));
		if( sql.GetLong(0) == checkTrackCount )
		{
			sql.Query(wxT("SELECT id FROM tracks WHERE url LIKE '") + sql.QParam(urlBegin) + wxT("%'"));
			while( sql.Next() )
			{
				m_updatedTracks.Add(sql.GetLong(0));
			}

			return TRUE;
		}
	}
	return FALSE;
}


bool SjLibraryModule::Callback_CheckTrackInfo(const wxString& url, uint32_t actualCrc)
{
	if( !m_deepUpdate )
	{
		wxSqlt sql;

		sql.Query(wxT("SELECT id, updatecrc FROM tracks WHERE url='") + sql.QParam(url) + wxT("'"));
		if( sql.Next() )
		{
			if( (uint32_t)sql.GetLong(1) == actualCrc )
			{
				m_updatedTracks.Add(sql.GetLong(0));
				return TRUE;
			}
		}
	}

	return FALSE;
}


bool SjLibraryModule::Callback_ReceiveTrackInfo(SjTrackInfo* trackInfo)
{
	// insert track info if needed, wxSqlt is local for early destructor call
	long trackId;
	{
		wxSqlt sql;
		sql.Query(  wxT("SELECT id, rating, groupName, timesplayed, lastplayed, timemodified, autovol, playtimems, genrename FROM tracks WHERE url='") + sql.QParam(trackInfo->m_url) + wxT("';"));
		if( sql.Next() )
		{
			trackId = sql.GetLong(0);

			// preserve rating, if new is unset
			if( trackInfo->m_rating == 0 )
			{
				trackInfo->m_rating = sql.GetLong(1);
			}

			// preserve old group name, if new is unset
			if( trackInfo->m_groupName.IsEmpty() )
			{
				trackInfo->m_groupName = sql.GetString(2);
			}

			// preserve old genre name, if new is unset, new in 2.52beta2, see http://www.silverjuke.net/forum/privmsg.php?mode=quote&p=202
			if( trackInfo->m_genreName.IsEmpty() )
			{
				trackInfo->m_genreName = sql.GetString(8);
			}

			// preserve old times played, if larger
			unsigned long old_timesPlayed = sql.GetLong(3);
			if( trackInfo->m_timesPlayed < old_timesPlayed )
			{
				trackInfo->m_timesPlayed =  old_timesPlayed;
			}

			// preserve old last played, if it is larger
			unsigned long old_lastPlayed = sql.GetLong(4);
			if( trackInfo->m_lastPlayed < old_lastPlayed )
			{
				trackInfo->m_lastPlayed = old_lastPlayed;
			}

			// always preserve old modification date
			trackInfo->m_timeModified = sql.GetLong(5);

			// preserve old volume information, always use the old value if set
			// (I think we make it better ;-)
			long old_autoVol = sql.GetLong(6);
			if( old_autoVol )
			{
				trackInfo->m_autoVol = old_autoVol;
			}

			// preserved old playing time, if new is unset
			if( trackInfo->m_playtimeMs <= 0 )
			{
				trackInfo->m_playtimeMs = sql.GetLong(7);
			}
		}
		else
		{
			if( !sql.Query(wxT("INSERT INTO tracks (url, timeadded, timemodified) VALUES ('") + sql.QParam(trackInfo->m_url) + wxT("', ") + sql.UParam(m_updateStartingTime) + wxT(", 0);")) )
			{
				delete trackInfo;
				return TRUE; // error, but continue
			}

			trackId = sql.GetInsertId();
		}
	}   /* sql deleted */

	m_updatedTracks.Add(trackId);

	// update track info
	if( !WriteTrackInfo(trackInfo, trackId) )
	{
		delete trackInfo;
		return TRUE; // error, but continue
	}

	// success
	delete trackInfo;
	return TRUE;
}


bool SjLibraryModule::UpdateAllCol(wxWindow* parent, bool deepUpdate)
{
	wxSqlt               sql;
	wxSqltTransaction    transaction;

	m_deepUpdate            = deepUpdate;
	m_updateStartingTime    = wxDateTime::Now().GetAsDOS();

	m_updatedTracks.Clear();
	SavePendingData();
	ForgetRememberedValues();

	// go through all music library scanner modules
	// and receive the track information by SjLibraryModule::ReceiveTrackInfo()
	{
		SjModuleList* moduleList = g_mainFrame->m_moduleSystem.GetModules(SJ_MODULETYPE_SCANNER);
		wxASSERT(moduleList);
		SjModuleList::Node* moduleNode = moduleList->GetFirst();
		while( moduleNode )
		{
			SjScannerModule* scannerModule = (SjScannerModule*)moduleNode->GetData();
			wxASSERT(scannerModule);

			if( !scannerModule->IterateTrackInfo(this) )
			{
				return FALSE; // user abort
			}

			moduleNode = moduleNode->GetNext();
		}
	}

	// remove non-updated tracks
	SjBusyInfo::Set(_("Updating music library")+wxString(wxT("...")), TRUE);

	{
		wxString updatedTracksStr = m_updatedTracks.GetAsString();
		if( !updatedTracksStr.IsEmpty() )
		{
			if( !sql.Query(wxT("DELETE FROM tracks WHERE NOT (id IN (") + updatedTracksStr + wxT("));")) )
			{
				return FALSE;
			}

			if( sql.GetChangedRows() >= 1000 )
			{
				transaction.Vacuum();
			}
		}
		else
		{
			if( !sql.Query(wxT("DELETE FROM tracks;")) )
			{
				return FALSE;
			}

			transaction.Vacuum(); // GetChangedRows() won't work as DELETE FROM without WHERE recreates the table in sqlite
		}
	}

	m_updatedTracks.Clear();

	// update albums, genres and groups
	if( !CombineTracksToAlbums()
	        || !UpdateUniqueValues(wxT("genrename"))
	        || !UpdateUniqueValues(wxT("groupname")) )
	{
		return FALSE;
	}

	// success
	transaction.Commit();
	m_autoVolCount = -1;
	ForgetRememberedValues();
	return TRUE;
}


bool SjLibraryModule::UpdateUniqueValues(const wxString& name)
{
	wxASSERT( name == wxT("genrename") || name == wxT("groupname") );

	wxSqlt          sql;
	wxString        currValue;
	wxArrayString   allValuesArray;
	wxString        allValuesString;

	SjBusyInfo::Set(wxString::Format(name==wxT("genrename")? _("Updating genres...") : _("Updating groups...")), TRUE);

	sql.Query(wxString::Format(wxT("SELECT %s FROM tracks GROUP BY %s ORDER BY %s;"), name.c_str(), name.c_str(), name.c_str()));
	while( sql.Next() )
	{
		currValue = sql.GetString(0);
		currValue.Trim(TRUE);
		currValue.Trim(FALSE);
		if( !currValue.IsEmpty()
		        && allValuesArray.Index(currValue)==wxNOT_FOUND )
		{
			allValuesArray.Add(currValue);

			allValuesString += currValue + wxT("\n");
		}
	}

	sql.ConfigWrite(wxT("library/") + name, allValuesString);

	return TRUE;
}


/*******************************************************************************
 * SjLibraryModule (re-)creating albums from tracks table
 ******************************************************************************/


class SjUpdateAlbumTrack
{
public:
	SjUpdateAlbumTrack(const wxString& artist, const wxString& album, const wxString& genre, long year, long albumId, const wxString& url)
	{
		m_leadArtistName    = artist;
		m_albumName         = album;
		m_genreName         = genre;
		m_updated           = FALSE;
		m_year              = year;
		m_albumId           = albumId;
		m_url               = url;
	}

	long        m_id;
	wxString    m_leadArtistName;
	wxString    m_albumName;
	wxString    m_genreName;
	long        m_year;
	long        m_albumId;
	wxString    m_url;
	bool        m_updated;
};


class SjUpdateAlbum
{
public:
	SjUpdateAlbum(long step)
	{
		m_trackIds = NULL;
		m_step     = step;
	}

	~SjUpdateAlbum()
	{
		if( m_trackIds )
		{
			delete m_trackIds;
		}
	}

	wxString        GetSortStr          (const wxString& year) const;
	static wxString GetUrlAsHash        (const wxString& url);

	long            m_step;
	wxString        m_leadArtistName;
	wxString        m_albumName;
	wxString        m_url, m_sort;
	wxArrayLong*    m_trackIds;
};


wxString SjUpdateAlbum::GetSortStr(const wxString& year) const
{
	SjLibraryModule* lib = g_mainFrame->m_libraryModule;

	#define SORT_SEP wxT("0z")
	// SORT_SEP is needed to separate the artist- from the albumname to allow
	// differing eg. between "Queen" and "Queen Ida And Her Zydeco Band", see
	// http://www.silverjuke.net/forum/topic-1573.html
	//
	// Some notes: The "0" in the sort string is needed for a correct order,
	// the "z" is  needed to seperate the "0" from numbers in the artist-/albumname

	if( m_leadArtistName.IsEmpty() || m_albumName.IsEmpty() )
	{
		if( !m_leadArtistName.IsEmpty() )
		{
			return SjNormaliseString
			       (
			           lib->m_omitArtist.Apply(m_leadArtistName)
			           +   year
			           ,   SJ_NUM_SORTABLE|SJ_NUM_TO_END
			       );
		}
		else if( !m_albumName.IsEmpty() )
		{
			return SjNormaliseString
			       (
			           lib->m_omitAlbum.Apply(m_albumName)
			           +   year
			           ,   SJ_NUM_SORTABLE|SJ_NUM_TO_END
			       );
		}
		else
		{
			return wxT("zzzzzzzzzzzzzzzzzzzzzzzzzzz"); // very last
		}
	}
	else if( lib->m_sort == SJ_LIBSORT_ARTIST_ALBUM_YEAR )
	{
		return SjNormaliseString
		       (
		           lib->m_omitArtist.Apply(m_leadArtistName)
		           +   SORT_SEP
		           +   SjNormaliseString(lib->m_omitAlbum.Apply(m_albumName), SJ_NUM_SORTABLE|SJ_NUM_TO_END)
		           +   year
		           ,   SJ_NUM_SORTABLE|SJ_NUM_TO_END
		       );
	}
	else if( lib->m_sort == SJ_LIBSORT_ALBUM_YEAR_ARTIST )
	{
		return SjNormaliseString
		       (
		           lib->m_omitAlbum.Apply(m_albumName)
		           +   year
		           +   SjNormaliseString(lib->m_omitArtist.Apply(m_leadArtistName), SJ_NUM_SORTABLE|SJ_NUM_TO_END)
		           ,   SJ_NUM_SORTABLE|SJ_NUM_TO_END
		       );
	}
	else if( lib->m_sort == SJ_LIBSORT_ALBUM_ARTIST_YEAR )
	{
		return SjNormaliseString
		       (
		           lib->m_omitAlbum.Apply(m_albumName)
		           +   SORT_SEP
		           +   SjNormaliseString(lib->m_omitArtist.Apply(m_leadArtistName), SJ_NUM_SORTABLE|SJ_NUM_TO_END)
		           +   year
		           ,   SJ_NUM_SORTABLE|SJ_NUM_TO_END
		       );
	}
	else // lib->m_sort == SJ_LIBSORT_ARTIST_YEAR_ALBUM
	{
		return SjNormaliseString
		       (
		           lib->m_omitArtist.Apply(m_leadArtistName)
		           +   year
		           +   SjNormaliseString(lib->m_omitAlbum.Apply(m_albumName), SJ_NUM_SORTABLE|SJ_NUM_TO_END)
		           ,   SJ_NUM_SORTABLE|SJ_NUM_TO_END
		       );
	}
}


wxString SjUpdateAlbum::GetUrlAsHash(const wxString& url)
{
	wxString ret;
	SjTools::GetFileNameFromUrl(url, &ret, FALSE, TRUE);
	ret.Replace(wxT(":"), wxT("x"));
	ret.Replace(wxT("/"), wxT("x"));
	ret.Replace(wxT("\\"), wxT("x"));
	return SjNormaliseString(ret, SJ_NUM_SORTABLE);
}


WX_DECLARE_LIST(SjUpdateAlbum, SjUpdateAlbumList);
#include <wx/listimpl.cpp>
WX_DEFINE_LIST(SjUpdateAlbumList);


WX_DECLARE_STRING_HASH_MAP(wxArrayLong*, SjUpdateAlbumHash);


class SjUpdateAlbumsTempData
{
public:
	SjUpdateAlbumsTempData()
		: m_hash1(1024/*just a size hint*/)
	{
	}

	~SjUpdateAlbumsTempData()
	{
		ClearHash1();
	}

	void ClearHash1();

	SjUpdateAlbumHash m_hash1;
};


void SjUpdateAlbumsTempData::ClearHash1()
{
	SjUpdateAlbumHash::iterator hashNode;
	for( hashNode = m_hash1.begin(); hashNode != m_hash1.end(); hashNode++ )
	{
		if( hashNode->second )
		{
			delete hashNode->second;
		}
	}
	m_hash1.clear();
}


int SjLibraryModule_CmpAlbums(const SjUpdateAlbum** i1, const SjUpdateAlbum** i2)
{
	wxASSERT(i1 && *i1 && i2 && *i2);

	return (*i1)->m_sort.Cmp((*i2)->m_sort);
}


void SjLibraryModule::GetPossibleAlbumArts(long albumId, wxArrayLong& albumArtIds,
        wxArrayString* albumArtUrls, bool addAutoCover)
{
	wxSqlt      sql;

	long        currArtId;

	// add all arts of all tracks belonging to the album
	sql.Query(wxString::Format(wxT("SELECT artids FROM tracks WHERE albumid=%lu;"), albumId));
	while( sql.Next() )
	{
		const char* p = sql.GetAsciiPtr(0);
		while( p && *p )
		{
			currArtId = atol(p);
			if( albumArtIds.Index(currArtId)==wxNOT_FOUND )
			{
				albumArtIds.Add(currArtId);
			}

			p = strchr(p, ' ');
			if( p ) p++;
		}
	}

	// add the art selected (by the file selector) by the user for this album
	// (if there is one, we add it to a temp. hash to allow the user to
	// select it again if another art is set to albums.artiduser in between)
	sql.Query(wxString::Format(wxT("SELECT artiduser FROM albums WHERE id=%lu;"), albumId));
	if( sql.Next() )
	{
		currArtId = sql.GetLong(0);
		if( currArtId && albumArtIds.Index(currArtId)==wxNOT_FOUND )
		{
			albumArtIds.Add(currArtId);
			m_addedArtIds.Insert(albumId, currArtId);
		}
	}

	// sort the ids now (should be done before albumArtUrls are set, see http://www.silverjuke.net/forum/topic-2746.html )
	if( !albumArtIds.IsEmpty() )
	{
		sql.Query(wxString::Format(wxT("SELECT id FROM arts WHERE id IN(%s) ORDER BY url;"), SjTools::Implode(albumArtIds, wxT(",")).c_str()));
		albumArtIds.Clear();
		while( sql.Next() )
		{
			albumArtIds.Add(sql.GetLong(0));
		}
	}

	if( addAutoCover )
	{
		// we also add the temp. art ID only if the caller wants everything
		// (indicated by "addAutoCover" in this case)
		currArtId = m_addedArtIds.Lookup(albumId);
		if( currArtId && albumArtIds.Index(currArtId)==wxNOT_FOUND )
		{
			sql.Query(wxString::Format(wxT("SELECT id FROM arts WHERE id=%lu;"), currArtId)); // make sure, the record still exists
			if( sql.Next() )
			{
				albumArtIds.Add(currArtId);
			}
		}

		// add auto cover (should be last for cosmetic reasons when using a list in menus)
		if( albumArtIds.Index(SJ_DUMMY_COVER_ID)==wxNOT_FOUND )
		{
			albumArtIds.Add(SJ_DUMMY_COVER_ID);
		}
	}

	// get the URLs if wanted
	if( albumArtUrls )
	{
		int i, idsCount = (int)albumArtIds.GetCount();
		for( i = 0; i < idsCount; i++ )
		{
			currArtId = albumArtIds.Item(i);
			if( currArtId == SJ_DUMMY_COVER_ID )
			{
				albumArtUrls->Add(GetDummyCoverUrl(albumId));
			}
			else
			{
				sql.Query(wxString::Format(wxT("SELECT url FROM arts WHERE id=%lu;"), currArtId));
				albumArtUrls->Add(sql.Next()? sql.GetString(0) : wxString(wxT("")));
				// also add empty string, the returned array must
				// have the same size as the input array
			}
		}
	}
}


bool SjLibraryModule::CombineTracksToAlbums()
{
	// init
	bool                    ret = FALSE;

	wxSqlt                  sql;

	SjLLHash                allTracks;
	long                    currTrackId;
	SjUpdateAlbumTrack*     currTrack;

	SjUpdateAlbumList       allAlbums;
	allAlbums.DeleteContents(TRUE);
	SjUpdateAlbum*          currAlbum;

	wxArrayLong*            trackIds;
	long                    trackIdsCount;

	ForgetRememberedValues();

	SjBusyInfo::Set(_("Combining tracks to albums..."), TRUE);

	// read all tracks
	if( (m_flags&SJ_LIB_CREATEALBUMSBY_DIR) )
	{
		sql.Query(wxT("SELECT id, leadartistname, albumname, genrename, year, albumid, url FROM tracks;"));
		while( sql.Next() )
		{
			allTracks.Insert(sql.GetLong(0), (long)new SjUpdateAlbumTrack(sql.GetString(1), sql.GetString(2), sql.GetString(3), sql.GetLong(4), sql.GetLong(5), sql.GetString(6)));
		}
	}
	else
	{
		sql.Query(wxT("SELECT id, leadartistname, albumname, genrename, year, albumid FROM tracks;"));
		while( sql.Next() )
		{
			allTracks.Insert(sql.GetLong(0), (long)new SjUpdateAlbumTrack(sql.GetString(1), sql.GetString(2), sql.GetString(3), sql.GetLong(4), sql.GetLong(5), wxT("")));
		}
	}

	// collecting albums
	{
		int                         step, i;
		SjUpdateAlbumsTempData      albumsThisStep;
		SjUpdateAlbumHash::iterator albumsThisStepNode;
		wxString                    year;
		int                         FIRST_STEP = (m_flags&SJ_LIB_CREATEALBUMSBY_DIR)? -1 : 0;
		#define                     LAST_STEP 4

		#ifdef __WXDEBUG__
			SjSLHash usedAlbumUrls;
		#endif

		for( step = FIRST_STEP; step <= LAST_STEP; step++ )
		{
			albumsThisStep.ClearHash1();

			// build hash with keys as "artist-album", "album" or "artist"
			{
				wxString currTrackHash, currTrackHash2;

				SjHashIterator iterator1;
				while( (currTrack=(SjUpdateAlbumTrack*)allTracks.Iterate(iterator1, &currTrackId)) )
				{
					if( currTrack->m_updated )
					{
						continue;
					}

					if( step == -1 )
					{
						// directory
						currTrackHash = SjUpdateAlbum::GetUrlAsHash(currTrack->m_url);
						if( currTrackHash.IsEmpty() )
						{
							continue;
						}
					}
					else if( step == 0 )
					{
						// "artist-album"
						currTrackHash  = SjNormaliseString(m_omitArtist.Apply(currTrack->m_leadArtistName), SJ_NUM_SORTABLE|SJ_NUM_TO_END);
						currTrackHash2 = SjNormaliseString(m_omitAlbum.Apply(currTrack->m_albumName), SJ_NUM_SORTABLE|SJ_NUM_TO_END);
						if( currTrackHash.IsEmpty() || currTrackHash2.IsEmpty() )
						{
							continue;
						}
						currTrackHash += wxT("-") + currTrackHash2;
					}
					else if( step == 1 )
					{
						// "album" - compilations
						currTrackHash = SjNormaliseString(m_omitAlbum.Apply(currTrack->m_albumName), SJ_NUM_SORTABLE|SJ_NUM_TO_END);
						if( currTrackHash.IsEmpty() )
						{
							continue;
						}
					}
					else if( step == 2 )
					{
						// "artist"
						currTrackHash = SjNormaliseString(m_omitArtist.Apply(currTrack->m_leadArtistName), SJ_NUM_SORTABLE|SJ_NUM_TO_END);
						if( currTrackHash.IsEmpty() )
						{
							continue;
						}
					}
					else if( step == 3 )
					{
						// "genre"
						currTrackHash = SjNormaliseString(m_omitArtist.Apply(currTrack->m_genreName), SJ_NUM_SORTABLE|SJ_NUM_TO_END);
						if( !(m_flags&SJ_LIB_CREATEALBUMSBY_GENRE) || currTrackHash.IsEmpty() )
						{
							continue;
						}
					}
					else
					{
						// rest
						currTrackHash = wxT("-");
					}

					albumsThisStepNode = albumsThisStep.m_hash1.find(currTrackHash);
					if( albumsThisStepNode == albumsThisStep.m_hash1.end() )
					{
						trackIds = new wxArrayLong;
						if( !trackIds )
						{
							wxLogError(wxT("Out of memory."));
							goto Cleanup;
						}
						albumsThisStep.m_hash1[currTrackHash] = trackIds;
					}
					else
					{
						trackIds = albumsThisStepNode->second;
					}
					trackIds->Add(currTrackId);
				}
			}

			// yield
			if( !SjBusyInfo::Set() ) { goto Cleanup; }

			// go through hash and assume all items with equal or
			// more than N1 tracks to be an album
			{
				long largestYear;

				for( albumsThisStepNode = albumsThisStep.m_hash1.begin(); albumsThisStepNode != albumsThisStep.m_hash1.end(); albumsThisStepNode++ )
				{
					trackIds      = albumsThisStepNode->second;
					trackIdsCount = (int)trackIds->GetCount();
					largestYear   = 0;
					if( trackIdsCount >= m_n1 || step == LAST_STEP )
					{
						// there are equal or more than N1 items, assume these
						// tracks to be an album and skip them in the next step
						for( i = 0; i < trackIdsCount; i++ )
						{
							currTrack = (SjUpdateAlbumTrack*)allTracks.Lookup(trackIds->Item(i));
							wxASSERT( currTrack );

							currTrack->m_updated = TRUE;

							if( currTrack->m_year < 9999
							        && currTrack->m_year > largestYear )
							{
								largestYear = currTrack->m_year;
							}
						}

						// add this album to album list
						wxASSERT( currTrack );

						currAlbum = new SjUpdateAlbum(step);
						if( !currAlbum )
						{
							wxLogError(wxT("Out of memory."));
							goto Cleanup;
						}

						albumsThisStepNode->second  = NULL;
						currAlbum->m_trackIds = trackIds; // the track IDs array now belongs to the list

						if( step == -1 )
						{
							currTrack = (SjUpdateAlbumTrack*)allTracks.Lookup(trackIds->Item(0));
							currAlbum->m_leadArtistName = currTrack->m_leadArtistName;
							currAlbum->m_albumName = currTrack->m_albumName;
							for( i = 1; i < trackIdsCount; i++ )
							{
								currTrack = (SjUpdateAlbumTrack*)allTracks.Lookup(trackIds->Item(i));
								if( SjNormaliseString(currAlbum->m_leadArtistName, 0)
								 != SjNormaliseString(currTrack->m_leadArtistName, 0) )
								{
									currAlbum->m_leadArtistName.Clear();
								}

								if( currAlbum->m_albumName.IsEmpty() )
								{
									currAlbum->m_albumName = currTrack->m_albumName;
								}
							}
						}
						else if( step == 0 )
						{
							currAlbum->m_leadArtistName = currTrack->m_leadArtistName;
							currAlbum->m_albumName = currTrack->m_albumName;
							wxASSERT(!currAlbum->m_leadArtistName.IsEmpty());
							wxASSERT(!currAlbum->m_albumName.IsEmpty());
						}
						else if( step == 1 )
						{
							currAlbum->m_albumName = currTrack->m_albumName;
							wxASSERT(!currAlbum->m_albumName.IsEmpty());
						}
						else if( step == 2 )
						{
							currAlbum->m_leadArtistName = currTrack->m_leadArtistName;
							wxASSERT(!currAlbum->m_leadArtistName.IsEmpty());
						}
						else if( step == 3 )
						{
							currAlbum->m_albumName = currTrack->m_genreName;
							wxASSERT( !currAlbum->m_albumName.IsEmpty() );
						}

						// get sorting
						if( largestYear <= 0 ) largestYear = 9999;
						year.Printf(wxT("%i"), (int)largestYear);
						currAlbum->m_sort = currAlbum->GetSortStr(year);

						// get url...
						if( step == -1 )
						{
							currAlbum->m_url  = wxT("album:")
							                    + SjUpdateAlbum::GetUrlAsHash(currTrack->m_url);
						}
						else if( step == 0 )
						{
							wxASSERT(!currAlbum->m_leadArtistName.IsEmpty());
							wxASSERT(!currAlbum->m_albumName.IsEmpty());
							currAlbum->m_url  = wxT("album:")
							                    + SjNormaliseString(m_omitArtist.Apply(currAlbum->m_leadArtistName), SJ_NUM_SORTABLE|SJ_NUM_TO_END)
							                    + wxT("/")
							                    + (currAlbum->m_albumName.IsEmpty()? wxString(wxT("-")) : SjNormaliseString(m_omitAlbum.Apply(currAlbum->m_albumName), SJ_NUM_SORTABLE|SJ_NUM_TO_END));
						}
						else if( step == 1 )
						{
							wxASSERT(!currAlbum->m_albumName.IsEmpty());
							currAlbum->m_url  = wxT("album:-/")
							                    + SjNormaliseString(m_omitAlbum.Apply(currAlbum->m_albumName), SJ_NUM_SORTABLE|SJ_NUM_TO_END);
						}
						else if( step == 2 )
						{
							wxASSERT(!currAlbum->m_leadArtistName.IsEmpty());
							currAlbum->m_url  = wxT("album:")
							                    + SjNormaliseString(m_omitAlbum.Apply(currAlbum->m_leadArtistName), SJ_NUM_SORTABLE|SJ_NUM_TO_END)
							                    + wxT("/-");
						}
						else if( step == 3 )
						{
							currAlbum->m_url  = wxT("album:-/")
							                    + SjNormaliseString(currAlbum->m_albumName, SJ_NUM_SORTABLE|SJ_NUM_TO_END)
							                    + wxT("-genre");
							// (***) hier koennen die URLs doppeldeutig sein, wenn ein albumname dem eines genrenamens entspricht!
							// daher wird "genre-" vorangestellt!
						}
						else
						{
							currAlbum->m_url  = wxT("album:-/-");
						}

						// add album
						allAlbums.Append(currAlbum);
						#ifdef __WXDEBUG__
							wxASSERT( usedAlbumUrls.Lookup(currAlbum->m_url) == 0 );
							usedAlbumUrls.Insert(currAlbum->m_url, 1);
						#endif
					}
				}
			}

			// yield
			if( !SjBusyInfo::Set() ) { goto Cleanup; }

		} // next step
	}

	// implement step (5) from the header
	{
		SjSLHash                    allStep1Albums;
		SjUpdateAlbum*              currStep1Album;

		SjUpdateAlbumList::Node*    currAlbumNode;
		SjUpdateAlbumList::Node*    nextAlbumNode;
		long                        currTrackCount, i;

		// collect all compilation albums from step #1
		currAlbumNode = allAlbums.GetFirst();
		while( currAlbumNode )
		{
			currStep1Album = currAlbumNode->GetData();
			wxASSERT(currStep1Album);

			if( currStep1Album->m_step == 1 )
			{
				wxASSERT(  currStep1Album->m_leadArtistName.IsEmpty() );
				wxASSERT( !currStep1Album->m_albumName.IsEmpty() );
				wxASSERT(  allStep1Albums.Lookup(currStep1Album->m_albumName) == 0 );

				allStep1Albums.Insert(currStep1Album->m_albumName, (long)currStep1Album);
			}

			currAlbumNode = currAlbumNode->GetNext();
		}

		// check which albums from step #0 should be merged with the compilations
		// from step #1
		currAlbumNode = allAlbums.GetFirst();
		while( currAlbumNode )
		{
			nextAlbumNode = currAlbumNode->GetNext();
			currAlbum = currAlbumNode->GetData();
			wxASSERT(currAlbum);

			if( currAlbum->m_step == 0
			        && (long)currAlbum->m_trackIds->GetCount() <= m_n2 )
			{
				currStep1Album = (SjUpdateAlbum*)allStep1Albums.Lookup(currAlbum->m_albumName);
				if( currStep1Album )
				{
					currTrackCount = currAlbum->m_trackIds->GetCount();
					for( i = 0; i < currTrackCount; i++ )
					{
						currStep1Album->m_trackIds->Add(currAlbum->m_trackIds->Item(i));
					}
					allAlbums.DeleteNode(currAlbumNode);
				}
			}

			currAlbumNode = nextAlbumNode;
		}
	}


	// okay, now we have all albums, sort the albums
	allAlbums.Sort(SjLibraryModule_CmpAlbums);

	// update album table
	{
		wxSqltTransaction           transaction;

		SjUpdateAlbumList::Node*    currAlbumNode;
		long                        currAlbumIndex = 0;

		wxArrayLong                 artIds;
		wxArrayString               artUrls;
		long                        albumId, i, artId;
		int                         lastAz=0, thisAz, azFirst;

		SjIdCollector               updatedAlbums;

		currAlbumNode = allAlbums.GetFirst();
		while( currAlbumNode )
		{
			currAlbum = currAlbumNode->GetData();
			wxASSERT( currAlbum );

			// insert album into album table if not yet there, get album ID
			sql.Query(wxT("SELECT id FROM albums WHERE url='") + sql.QParam(currAlbum->m_url) + wxT("';"));
			if( sql.Next() )
			{
				albumId = sql.GetLong(0);
			}
			else
			{
				sql.Query(wxT("INSERT INTO albums (url) VALUES ('") + sql.QParam(currAlbum->m_url) + wxT("');"));
				albumId = sql.GetInsertId();
			}

			// get a-z
			thisAz = (int)(currAlbum->m_sort[0]);
			if( thisAz < 'a' )
			{
				thisAz = (int)'a';
			}
			else if( thisAz >= 'z' )
			{
				thisAz = (int)'z';
				if( currAlbum->m_sort.StartsWith(SJ_NUM_TO_END_ZZZZZZZZZZ) )
				{
					thisAz++;
				}
			}

			if( thisAz == lastAz )
			{
				azFirst = FALSE;
			}
			else
			{
				azFirst = TRUE;
				lastAz = thisAz;
			}

			// store album ID in track table, get all arts
			trackIds      = currAlbum->m_trackIds;
			trackIdsCount = (int)trackIds->GetCount();
			for( i = 0; i < trackIdsCount; i++ )
			{
				currTrackId = trackIds->Item(i);
				currTrack = (SjUpdateAlbumTrack*)allTracks.Lookup(currTrackId);
				wxASSERT(currTrack);
				if( currTrack->m_albumId != albumId )
				{
					sql.Query(wxString::Format(wxT("UPDATE tracks SET albumid=%lu WHERE id=%lu;"),
					                           albumId, currTrackId));
				}
			}

			// get art to use
			artIds.Clear();
			artUrls.Clear();
			GetPossibleAlbumArts(albumId, artIds, &artUrls, FALSE/*addAutoCover*/);
			artId/*Index*/ = m_coverFinder.Apply(artUrls, currAlbum->m_albumName);
			artId/*Convert back to Id*/ = artId == -1? 0 : artIds.Item(artId);

			// save album data
			sql.Query(wxT("UPDATE albums SET ")
			          wxT("albumindex=")        + sql.UParam(currAlbumIndex)                + wxT(", ")
			          wxT("leadartistname='")   + sql.QParam(currAlbum->m_leadArtistName)   + wxT("', ")
			          wxT("albumname='")        + sql.QParam(currAlbum->m_albumName)        + wxT("', ")
			          wxT("az=")                + sql.LParam(thisAz)                        + wxT(", ")
			          wxT("azfirst=")           + sql.LParam(azFirst? thisAz : 0)           + wxT(", ")
			          wxT("artidauto=")         + sql.UParam(artId)                         + wxT(" ")
			          wxT("WHERE id=") + sql.UParam(albumId) + wxT(";"));

			updatedAlbums.Add(albumId);

			// yield
			if( (currAlbumIndex % 10) == 0 )
			{
				if( !SjBusyInfo::Set() ) { goto Cleanup; }
			}

			// next album
			currAlbumNode = currAlbumNode->GetNext();
			currAlbumIndex++;
		}

		// remove some albums
		{
			wxString updatedAlbumsStr = updatedAlbums.GetAsString();
			if( !updatedAlbumsStr.IsEmpty() )
			{
				#ifdef __WXDEBUG__
					sql.Query(wxString::Format(wxT("SELECT url FROM albums WHERE NOT (id IN (%s));"), updatedAlbumsStr.c_str()));
					while( sql.Next() )
					{
						wxLogDebug(wxT("unused album: %s"), sql.GetString(0).c_str());
					}
				#endif
				sql.Query(wxT("DELETE FROM albums WHERE NOT (id IN (") + updatedAlbumsStr + wxT("));"));
			}
			else
			{
				sql.Query(wxT("DELETE FROM albums;"));
			}
		}

		// success
		transaction.Commit();
		ret = TRUE;
	}

	// cleanup
Cleanup:

	if( m_searchOffsets )
	{
		free(m_searchOffsets); // free as the number of columns may increase, re-set on next search
		m_searchOffsets = NULL;
	}

	m_searchOffsetsCount = -1; // no search
	m_searchTracksHash.Clear();
	m_selectedTrackIds.Clear();
	UpdateMenuBar();

	SjHashIterator iterator2;
	while( (currTrack=(SjUpdateAlbumTrack*)allTracks.Iterate(iterator2, &currTrackId)) )
	{
		delete currTrack;
	}

	ForgetRememberedValues();
	return ret;
}


/*******************************************************************************
 * SjLibraryModule: Adding folders directly
 ******************************************************************************/


bool SjLibraryModule::AddFolders(const wxArrayString& folders)
{
	// ask the user
	wxString msg = _("Do you want to add the following folder(s) to your music library?");
	msg += wxT("\n");
	int i;
	for( i = 0; i < (int)folders.GetCount(); i++ )
	{
		msg += wxT("\n");
		msg += folders[i];
		if( i == 8 && folders.GetCount()>10 )
		{
			msg += wxT("\n...");
			break;
		}
	}

	{
		wxWindowDisabler disabler(g_mainFrame);
		if( SjMessageBox(msg, SJ_PROGRAM_NAME, wxYES_NO|wxICON_QUESTION, g_mainFrame)!=wxYES )
		{
			return FALSE; // no, the user canceled
		}
	}

	// get the folder scanner module
	SjScannerModule* folderScanner = (SjScannerModule*)m_interface->m_moduleSystem->FindModuleByFile(wxT("memory:folderscanner.lib"));
	if( folderScanner == NULL )
	{
		wxASSERT( folderScanner );
		return FALSE; // no folder scanner module!?
	}
	wxASSERT( folderScanner->m_type == SJ_MODULETYPE_SCANNER );

	// add all folders
	bool sthAdded = FALSE;
	for( i = 0; i < (int)folders.GetCount(); i++ )
	{
		if( folderScanner->AddUrl(folders[i]) )
		{
			sthAdded = TRUE;
		}
	}

	return sthAdded;
}


/*******************************************************************************
 * SjLibraryModule: Dropping files to a row (normally covers)
 ******************************************************************************/


static wxString getMyCoverName(const wxString& path, const wxString& baseName__, const wxString& ext)
{
	if( !::wxDirExists(path) )
	{
		if( !::wxMkdir(path) )
		{
			wxLogError(_("Cannot write \"%s\"."), path.c_str());
		}
	}

	wxFileName destFN(path, wxT("no.file"));

	wxString baseName = SjTools::EnsureValidFileNameChars(baseName__);
	int number = 1;
	while( 1 )
	{
		wxString name = baseName;
		if( number > 1 ) name += wxString::Format(wxT(" (%i)"), number);
		name += (wxT(".") + ext);

		destFN.SetFullName(name);
		if( !destFN.FileExists()
		        || number > 200 )
		{
			break;
		}

		number++;
	}

	return destFN.GetFullPath();
}


bool SjLibraryModule::AddArt__(SjDataObject* srcData, long albumId, bool ask)
{
	wxSqlt      sql;
	wxString    albumName;
	wxString    srcFilename;
	wxString    destDir;
	bool        copyImage = FALSE;

	#define     THUMBNAIL_SIZE 64
	wxBitmap    thumbnailBitmap;

	wxWindowDisabler disabler1(g_mainFrame);

	{
		wxBusyCursor busy;

		// make sure, the album exists
		// ---------------------------

		sql.Query(wxString::Format(wxT("SELECT id FROM albums WHERE id=%lu;"), albumId));
		if( !sql.Next() )
		{
			wxLogError(wxT("AddArt(): Album not found.")/*n/t*/);
			return FALSE; // album does not exist
		}

		// get a string as "Artist - Album"
		// --------------------------------

		sql.Query(wxString::Format(wxT("SELECT leadartistname, albumname FROM tracks WHERE albumid=%lu;"), albumId));
		if( !sql.Next() )
		{
			wxLogError(wxT("AddArt(): Dir not found.")/*n/t*/);
			return FALSE; // error
		}

		wxString artistAlbumName = sql.GetString(0);
		albumName = sql.GetString(1);
		if( !albumName.IsEmpty() ) artistAlbumName += wxT(" - ") + albumName;

		// get the possible destination directory
		// --------------------------------------

		sql.Query(wxString::Format(wxT("SELECT url FROM tracks WHERE albumid=%lu ORDER BY tracknr;"), albumId));
		if( sql.Next() )
		{
			// ...use the dir. of the first track
			wxFileName albumFN;
			albumFN.Assign(sql.GetString(0));  // path+track
			if( albumFN.DirExists() )
			{
				destDir = albumFN.GetPath();
			}
		}

		if( destDir.IsEmpty() )
		{
			// ...use the database dir
			wxFileName dbFN;
			dbFN.Assign(g_tools->m_dbFile);     // path+file
			dbFN.AppendDir(wxT("mycovers"));    // add a dir
			dbFN.SetFullName(wxT("anyfile"));   // add a dummy file
			if( !dbFN.DirExists() )
			{
				dbFN.Mkdir();
			}

			destDir = dbFN.GetPath();
		}

		// get the source file name (if any),
		// get a thumbnail of the image
		// ----------------------------------

		if( srcData->m_fileData )
		{
			wxArrayString allSrcFilenames = srcData->m_fileData->GetFilenames();
			if( allSrcFilenames.GetCount() < 1 )
			{
				wxLogError(wxT("AddArt(): No data.")/*n/t*/);
				return FALSE; // no data, however, don't log an error
			}

			#ifdef __WXMAC__ // there seems to be a bug in the drag'n'drop handler of wx - the filenames contain lineend characters!?
				for( int i = allSrcFilenames.GetCount()-1; i >= 0; i-- )
				{
					allSrcFilenames[i].Replace(wxT("\n"), wxT(""));
					allSrcFilenames[i].Replace(wxT("\r"), wxT(""));
				}
			#endif

			srcFilename = allSrcFilenames[0];

			if( !g_mainFrame->m_moduleSystem.FindImageHandlerByExt(SjTools::GetExt(srcFilename)) )
			{
				wxLogError(_("Cannot open \"%s\"."), srcFilename.c_str());
				return FALSE;
			}

			if( srcData->m_dragResult==wxDragCopy )
			{
				copyImage = TRUE;
			}

			// create thumbnail
			if( ask )
			{
				wxLogNull       null;
				wxFileSystem    fs;
				wxImage         tempImage;
				int             tries;

				for( tries = 0; tries <= 1; tries++ )
				{
					wxFSFile* fsFile = fs.OpenFile(srcFilename, wxFS_READ|wxFS_SEEKABLE); // i think, seeking is needed by at least one format ...
					if( fsFile )
					{
						wxInputStream* inputStream = fsFile->GetStream();
						if( inputStream->GetSize() > 0 )
						{
							tempImage.LoadFile(*inputStream, wxBITMAP_TYPE_ANY);
							tries = 1000; // = break plus delete fsFile
						}

						delete fsFile;
					}

					if( tries == 0 )
					{
						// hack: wait a second - this is needed for some apps to close
						// their exclusive access, eg. Mozilla Firefox :-(
						wxWindowDisabler disabler2;
						::wxSleep(1);
					}
				}

				if( tempImage.IsOk() )
				{
					SjImgOp::DoResize(tempImage, THUMBNAIL_SIZE, THUMBNAIL_SIZE, SJ_IMGOP_SMOOTH);
					thumbnailBitmap = wxBitmap(tempImage);
				}
			}
		}
		else if( srcData->m_bitmapData )
		{
			copyImage = TRUE;

			// create thumbnail
			if( ask )
			{
				thumbnailBitmap = srcData->m_bitmapData->GetBitmap();
				if( !thumbnailBitmap.IsOk() )
				{
					wxLogError(wxT("AddArt(): Cannot create thumbnail.")/*n/t*/);
					return FALSE;
				}
				wxImage tempImage = thumbnailBitmap.ConvertToImage();
				SjImgOp::DoResize(tempImage, THUMBNAIL_SIZE, THUMBNAIL_SIZE, SJ_IMGOP_SMOOTH);
				thumbnailBitmap = wxBitmap(tempImage);
			}
		}
		else
		{
			wxLogError(wxT("AddArt(): Unsupported data type.")/*n/t*/);
			return FALSE; // unsupported data type, however, don't log an error
		}
	}


	// ask the user if he want to use the given image as a cover
	// and wheter to copy it
	// ---------------------------------------------------------

	if( ask )
	{
		wxArrayString   destOptions;

		if( !srcFilename.IsEmpty() )
		{
			#define SHORTEN_LEN 50
			wxString srcDir;
			SjTools::GetFileNameFromUrl(srcFilename, &srcDir, FALSE, TRUE);
			destOptions.Add(wxString::Format(_("Yes, leave the image in \"%s\""), SjTools::ShortenUrl(srcDir, SHORTEN_LEN).c_str()));
		}

		destOptions.Add(wxString::Format(_("Yes, copy the image to \"%s\""), SjTools::ShortenUrl(destDir, SHORTEN_LEN).c_str()));

		int destOptionSelIndex = copyImage? 1 : 0;
		if( SjMessageBox(wxString::Format(_("Do you want to use the given image as the cover for \"%s\"?"), albumName.c_str()),
		                   SJ_PROGRAM_NAME,
		                   wxOK|wxCANCEL|wxICON_QUESTION,
		                   g_mainFrame,
		                   &destOptions,
		                   &destOptionSelIndex,
		                   wxT(""), wxT(""),
		                   &thumbnailBitmap)!=wxOK )
		{
			return FALSE; // no, the user canceled
		}

		if( !srcFilename.IsEmpty() )
		{
			copyImage = (destOptionSelIndex==1);
		}
		else if( !destOptionSelIndex )
		{
			return FALSE; // no, the user disabled the only "copy" option
		}
	}

	{
		wxBusyCursor  busy;

		// get the destination file
		// ------------------------

		wxString destFilename;
		if( srcData->m_fileData )
		{
			if( copyImage )
			{
				destFilename = getMyCoverName(destDir, albumName, SjTools::GetExt(srcFilename));
				::wxCopyFile(srcFilename, destFilename);
			}
			else
			{
				destFilename = srcFilename;
			}
		}
		else if( srcData->m_bitmapData )
		{
			wxBitmap bitmap = srcData->m_bitmapData->GetBitmap();
			if( !bitmap.IsOk() )
			{
				wxLogError(wxT("AddArt(): Cannot create bitmap.")/*n/t*/);
				return FALSE;
			}
			wxImage image = bitmap.ConvertToImage();
			destFilename = getMyCoverName(destDir, albumName, wxT("jpg"));
			image.SaveFile(destFilename, wxBITMAP_TYPE_JPEG);
		}
		else
		{
			// should not happen as this condition is already checked before the used is asked
			return FALSE;
		}

		// add the image to the list of arts or use the existing art ID
		sql.Query(wxT("SELECT id FROM arts WHERE url='") + sql.QParam(destFilename) + wxT("';"));
		if( sql.Next() )
		{
			sql.Query(wxString::Format(wxT("DELETE FROM arts WHERE id=%lu;"), sql.GetLong(0)));
		}

		if( !sql.Query(wxT("INSERT INTO arts (url) VALUES ('") + sql.QParam(destFilename) + wxT("');")) )
		{
			wxLogError(wxT("AddArt(): Cannot write database.")/*n/t*/);
			return FALSE; // error
		}
		long artId = sql.GetInsertId();

		// set the image as the user selection
		sql.Query(wxString::Format(wxT("UPDATE albums SET artiduser=%lu WHERE id=%lu;"),
		                           artId, albumId));
	}

	return TRUE;
}


/*******************************************************************************
 * SjLibraryEditDlg
 ******************************************************************************/


class SjLibraryEditDlg : public SjTagEditorDlg
{
public:
	SjLibraryEditDlg    ();
	bool            GetUrls             (wxArrayString&, int what);
	void            OnUrlChanged        (const wxString& oldUrl, const wxString& newUrl);

private:
	wxArrayString   m_urls;
};


SjLibraryEditDlg::SjLibraryEditDlg()
	:   SjTagEditorDlg(g_mainFrame, (g_mainFrame->m_libraryModule->m_selectedTrackIds.GetCount()>1))
{
	g_mainFrame->m_libraryModule->SavePendingData();
	g_mainFrame->m_libraryModule->GetSelectedUrls(m_urls);
}


bool SjLibraryEditDlg::GetUrls(wxArrayString& retUrls, int what)
{
	if( what != 0 )
	{
		// get prev/next track
		if( !g_mainFrame->m_browser->ChangeSelection(what==1? SJ_SEL_NEXT : SJ_SEL_PREV) )
		{
			return FALSE;
		}

		m_urls.Clear();
		g_mainFrame->m_libraryModule->GetSelectedUrls(m_urls);
	}

	// return the urls in scope
	retUrls = m_urls;
	return TRUE;
}


void SjLibraryEditDlg::OnUrlChanged(const wxString& oldUrl, const wxString& newUrl)
{
	if( !newUrl.IsEmpty() )
	{
		int index = m_urls.Index(oldUrl);
		if( index != wxNOT_FOUND )
		{
			m_urls[index] = newUrl;
		}
	}
}


/*******************************************************************************
 * SjLibraryModule User Actions
 ******************************************************************************/


#define LARGE_ALBUM 2 // albums with at least 2 tracks should have
// the change to appear early in the list; this should be
// true for almost all albums. however, we compare not the
// number of REAL tracks but the FOUND tracks.
static long* g_tracksPerAlbum = NULL;
static int compareOffsets(const void *arg1, const void *arg2)
{
	wxASSERT( g_tracksPerAlbum );

	long o1 = *((long*)arg1), o2 = *((long*)arg2);
	long t1 = g_tracksPerAlbum[o1], t2 = g_tracksPerAlbum[o2];

	if( t1 >= LARGE_ALBUM && t2 >= LARGE_ALBUM )
	{
		return o1-o2;
	}
	else if( t1 >= LARGE_ALBUM )
	{
		return -1;
	}
	else if( t2 >= LARGE_ALBUM )
	{
		return 1;
	}

	return o1-o2;
}


extern "C"
{
	static SjLLHash* g_filterHash = NULL;
	static void sqlite_infilter(sqlite3_context* context, int argc, sqlite3_value** argv)
	{
		sqlite3_result_int(context, g_filterHash->Lookup(sqlite3_value_int(argv[0])));
	}
};


SjSearchStat SjLibraryModule::SetSearch(const SjSearch& search, bool deepSearch)
{
	wxASSERT( wxThread::IsMain() );

	wxSqlt          sql;
	SjSearchStat    retStat;

	// create the filter IDs, if needed
	if( deepSearch || m_search.m_adv!=search.m_adv )
	{
		if( g_filterHash == NULL )
		{
			g_filterHash = &m_filterHash;
			sqlite3_create_function(sql.GetDb()->GetDb(), "infilter", 1, SQLITE_ANY, NULL, sqlite_infilter, NULL, NULL);
		}

		retStat = search.m_adv.GetAsSql(&m_filterHash, m_filterCond);
	}
	m_search = search;

	// cancel search?
	if( !search.IsSet() )
	{
		m_searchOffsetsCount = -1;
		m_searchTracksHash.Clear();
		return retStat; // search canceled - no tracks found
	}

	// allocate memory for the search offsets if not yet done
	if( m_searchOffsets == NULL )
	{
		sql.Query(wxT("SELECT COUNT(*) FROM albums;")); // get the max. numbers of albums
		m_searchOffsetsMax = sql.GetLong(0);
		if( m_searchOffsetsMax <= 0 )
		{
			return retStat; // no error but nothing to search for
		}

		m_searchOffsets = (long*)malloc(sizeof(long)*m_searchOffsetsMax);
		if( m_searchOffsets == NULL )
		{
			return retStat; /* error */
		}
	}

	// allocate temp. memory for the number of tracks per
	// album and for moving "large" albums to the beginning
	wxASSERT( m_searchOffsetsMax > 0 );
	g_tracksPerAlbum = (long*)malloc(sizeof(long)*m_searchOffsetsMax);
	if( g_tracksPerAlbum == NULL ) { return retStat; /* error */ }
	memset(g_tracksPerAlbum, 0, sizeof(long)*m_searchOffsetsMax);

	// build query string
	wxString    query;
	bool        gatherAzFirst = !m_search.m_simple.IsSet();

	query =  wxT("SELECT tracks.id,albumindex");
	if( gatherAzFirst ) { query += wxT(",albums.az"); }
	query += wxT(" FROM tracks, albums WHERE");

	if( m_search.m_simple.IsSet() )
	{
		wxString simpleSearchWords = search.m_simple.GetWords();
		wxString simpleCond;

		// search the search string in trackname, leadartistname, albumname
		if( g_advSearchModule->m_flags&SJ_SEARCHFLAG_SEARCHSINGLEWORDS )
		{
			// ... space == AND (default, added in version 3.x)
			wxArrayString simpleSearchWordsArray = SjTools::Explode(simpleSearchWords, wxT(' '), 1);
			for( size_t w = 0; w < simpleSearchWordsArray.GetCount(); w++ )
			{
				wxString wWord =simpleSearchWordsArray[w];
				if( !wWord.IsEmpty() )
				{
					wxString wCond = wxT(" (trackname LIKE '%?%' OR tracks.leadartistname LIKE '%?%' OR tracks.albumname LIKE '%?%') ");
					wCond.Replace(wxT("?"), wxSqlt::QParam(wWord));

					simpleCond += simpleCond.IsEmpty()? wxT("") : wxT(" AND ");
					simpleCond += wCond;
				}
			}

			if( simpleCond.IsEmpty() )
				simpleCond = wxT(" (0) ");
		}
		else
		{
			// ... phrase search
			simpleCond = wxT(" trackname LIKE '%?%' OR tracks.leadartistname LIKE '%?%' OR tracks.albumname LIKE '%?%' ");
			simpleCond.Replace(wxT("?"), wxSqlt::QParam(simpleSearchWords));
		}


		// create genre condition (deprecated, since 3.0, this is off by default)
		if( g_advSearchModule->m_flags&SJ_SEARCHFLAG_SIMPLEGENRELOOKUP )
		{
			wxArrayString allGenres = GetUniqueValues(SJ_TI_GENRENAME);
			long          genreIndex = allGenres.Index(simpleSearchWords, FALSE/*not case-sensitive*/);
			if( genreIndex != wxNOT_FOUND )
			{
				simpleCond = wxString::Format(wxT(" (%s) OR genrename='%s' "), simpleCond.c_str(), wxSqlt::QParam(allGenres.Item(genreIndex)).c_str());
			}
		}

		// add to query
		query += wxT(" (") + simpleCond + wxT(") ");
	}
	else if( gatherAzFirst )
	{
		int i; for(i=0; i<27; i++) m_filterAzFirst[i] = -1;
		m_filterAzFirstHidden = FALSE;
	}

	if( search.m_adv.IsSet() )
	{
		query += search.m_simple.IsSet()? wxT(" AND ") : wxT(" ");
		query += m_filterCond;
	}

	query += wxT(" AND albums.id=albumid ") // <-- should be the last condition as this won't eliminate any row itself
	         wxT("ORDER BY albumindex;");

	// query database
	{
		wxSqlt  sql;
		#if 0//def __WXDEBUG__
				wxString queryDebug__(query);
				queryDebug__.Replace(wxT("%"), wxT("%%"));
				wxLogDebug(queryDebug__);
		#endif
		sql.Query(query);

		// go through result:
		long lastAlbumIndex = -1, thisAlbumIndex;
		long thisAlbumAz, lastAlbumAz = -1;
		m_searchOffsetsCount = 0;
		m_searchTracksHash.Clear();
		while( sql.Next() )
		{
			thisAlbumIndex = sql.GetLong(1);
			wxASSERT( thisAlbumIndex >= lastAlbumIndex );

			if( lastAlbumIndex != thisAlbumIndex )
			{
				wxASSERT( m_searchOffsetsCount < m_searchOffsetsMax );
				m_searchOffsets[m_searchOffsetsCount] = thisAlbumIndex;
				lastAlbumIndex = thisAlbumIndex;

				if( gatherAzFirst )
				{
					thisAlbumAz = sql.GetLong(2);
					wxASSERT( thisAlbumAz >= lastAlbumAz );
					wxASSERT( thisAlbumAz >= 'a' && thisAlbumAz <= ('z'+1) );
					if( thisAlbumAz != lastAlbumAz
					        && thisAlbumAz >= 'a' && thisAlbumAz <= ('z'+1) )
					{
						m_filterAzFirst[thisAlbumAz-'a'] = m_searchOffsetsCount;
						lastAlbumAz = thisAlbumAz;
					}
				}

				m_searchOffsetsCount++;
			}

			wxASSERT( thisAlbumIndex < m_searchOffsetsMax );
			if( thisAlbumIndex < m_searchOffsetsMax /*proofe anyway for corrupted databases*/ )
			{
				g_tracksPerAlbum[thisAlbumIndex]++;
			}

			m_searchTracksHash.Insert(sql.GetLong(0), 1);
		}
	}

	// check, if the selected tracks are still in search
	{
		long selTrackId;
		bool updateMenu = FALSE;
		SjHashIterator iterator3;
		while( m_selectedTrackIds.Iterate(iterator3, &selTrackId) )
		{
			if( m_searchTracksHash.Lookup(selTrackId)==0 )
			{
				// the iteration functionality allows us to remove the
				// current element
				m_selectedTrackIds.Remove(selTrackId);
				updateMenu = TRUE;
			}
		}

		if( updateMenu )
		{
			UpdateMenuBar();
		}
	}

	// Move albums with equal or more than LARGE_ALBUM tracks found to the beginning of the list
	// (normally these are full albums with matching artist or album names)
	// However, we do this only if the simple search ist set
	if( m_search.m_simple.IsSet() )
	{
		qsort(m_searchOffsets, m_searchOffsetsCount, sizeof(long), compareOffsets);
	}
	else if( m_searchOffsetsCount < 20 )
	{
		// for very small albums view, hide the display "Albums - A" etc.
		m_filterAzFirstHidden = TRUE;
	}

	// done
	free(g_tracksPerAlbum);
	retStat.m_totalResultCount = m_searchTracksHash.GetCount();
	return retStat;
}


void SjLibraryModule::GetIdsInView(SjLLHash* ret,
                                   bool ignoreSimpleSearchIfNull,
                                   bool ignoreAdvSearchIfNull)
{
	wxASSERT( wxThread::IsMain() );
	wxASSERT( ret );

	if( m_searchTracksHash.GetCount() )
	{
		long currId;
		SjHashIterator iterator4;
		while( m_searchTracksHash.Iterate(iterator4, &currId) )
		{
			ret->Insert(currId, 1);
		}

		return; // done - tracks in view found
	}

	bool returnAll = FALSE;
	if( ignoreSimpleSearchIfNull
	        && m_search.m_simple.IsSet() )
	{
		if( m_search.m_adv.IsSet() )
		{
			// advanced search set, try to find tracks in the adv. search without the simple search
			SjAdvSearch tempSearch = m_search.m_adv;
			wxString dummySql;
			tempSearch.GetAsSql(ret, dummySql);

			if( ret->GetCount() )
			{
				return; // done - tracks in view found
			}

			// else continue and check ignoreAdvSearchIfNull
		}
		else
		{
			// adv. search not set - ignoring the simple search equals to return all
			returnAll = TRUE;
		}
	}

	if( (m_search.m_adv.IsSet() && (g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL)||g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL_GENRE)) && ignoreAdvSearchIfNull)
	        || !m_search.m_adv.IsSet() )
	{
		returnAll = TRUE;
	}

	if( returnAll )
	{
		wxSqlt sql;
		sql.Query(wxT("SELECT id FROM tracks;"));
		while( sql.Next() )
		{
			ret->Insert(sql.GetLong(0), 1);
		}
	}
}


long SjLibraryModule::DelInsSelection(bool del)
{
	if( g_advSearchModule->IncludeExclude(&m_selectedTrackIds, del) )
	{
		// the adv. search says, the tracks were deleted;
		// if there is a search set, remove the tracks from view.
		// otherwise the tracks are added or unchanged; in these cases, we have nothing to do
		if( m_search.m_adv.IsSet() )
		{
			// remove the selection from the filtered tracks
			long numDeletedTracks = m_selectedTrackIds.GetCount();
			long selectedId;
			SjHashIterator iterator5;
			while( m_selectedTrackIds.Iterate(iterator5, &selectedId) )
			{
				m_filterHash.Remove(selectedId);
			}

			// make sure, the hash is always regarded
			// (SjAdvSearch::GetAsSql() may have returned some other, optimized query)
			m_filterCond = wxT("INFILTER(tracks.id)");

			// recalculate the search
			SetSearch(m_search, FALSE);

			// done, view changed by deletion of tracks
			return numDeletedTracks;
		}
	}

	// no deleted tracks
	return 0;
}


bool SjLibraryModule::HiliteSearchWords(wxString& in)
{
	bool ret = false; // nothing hilited

	if( m_search.m_simple.IsSet() )
	{
		if( m_hiliteRegExFor != m_search.m_simple.GetHiliteWords() )
		{
			wxString cond = m_search.m_simple.GetHiliteWords();
			cond.Trim(true).Trim(false);
			if( cond.IsEmpty() )
			{
				m_hiliteRegExOk = false;
			}
			else
			{
				cond.Replace(wxT("\\"), wxT("\\\\"));   // do not for get to escape all special characters in the words ...
				cond.Replace(wxT("$"), wxT("\\$"));     // see http://www.silverjuke.net/forum/post.php?p=14723
				cond.Replace(wxT("+"), wxT("\\+"));
				cond.Replace(wxT("-"), wxT("\\-"));
				cond.Replace(wxT("*"), wxT("\\*"));
				cond.Replace(wxT("^"), wxT("\\^"));
				cond.Replace(wxT("."), wxT("\\."));
				cond.Replace(wxT(","), wxT("\\,"));
				cond.Replace(wxT("?"), wxT("\\?"));
				cond.Replace(wxT("|"), wxT("\\|"));
				cond.Replace(wxT("["), wxT("\\["));
				cond.Replace(wxT("]"), wxT("\\]"));
				cond.Replace(wxT("("), wxT("\\("));
				cond.Replace(wxT(")"), wxT("\\)"));
				cond.Replace(wxT("{"), wxT("\\{"));
				cond.Replace(wxT("}"), wxT("\\}"));

				cond.Replace(wxT(" "), wxT("|"));
				cond = wxT("(") + cond + wxT(")");

				m_hiliteRegExOk  = m_hiliteRegEx.Compile(cond, wxRE_ICASE);
			}

			m_hiliteRegExFor = m_search.m_simple.GetHiliteWords();
		}

		if( m_hiliteRegExOk )
		{
			if( m_hiliteRegEx.Replace(&in, wxT("\t\\1\t")) > 1 )
				ret = true;
		}
	}

	return ret;
}


long SjLibraryModule::GetMaskedColCount()
{
	if( HasSearch() )
	{
		return m_searchOffsetsCount;
	}
	else
	{
		return GetUnmaskedColCount();
	}
}


long SjLibraryModule::GetUnmaskedColCount()
{
	if( m_rememberedUnmaskedColCount == -1 )
	{
		wxSqlt sql;
		sql.Query(wxT("SELECT COUNT(*) FROM albums;"));
		m_rememberedUnmaskedColCount = sql.Next()? sql.GetLong(0) : 0;
	}

	return m_rememberedUnmaskedColCount;
}


long SjLibraryModule::GetUnmaskedTrackCount()
{
	if( m_rememberedUnmaskedTrackCount == -1 )
	{
		wxSqlt sql;
		sql.Query(wxT("SELECT COUNT(*) FROM tracks;"));
		m_rememberedUnmaskedTrackCount = sql.Next()? sql.GetLong(0) : 0;
	}

	return m_rememberedUnmaskedTrackCount;
}


bool SjLibraryModule::GetTrackInfo(const wxString& url, SjTrackInfo& trackInfo, long flags, bool logErrors)
{
	bool    ret = true;
	wxSqlt  sql;

	if( flags & SJ_TI_QUICKINFO )
	{
		sql.Query(wxT("SELECT trackName, leadArtistName, playtimeMs, albumName FROM tracks WHERE url='") + sql.QParam(url) + wxT("';"));
		if( sql.Next() )
		{
			trackInfo.m_trackName = sql.GetString(0);
			trackInfo.m_leadArtistName = sql.GetString(1);
			trackInfo.m_playtimeMs = sql.GetLong(2);
			trackInfo.m_albumName = sql.GetString(3);
		}
		else
		{
			ret = false;
		}
	}
	else if( flags & SJ_TI_FULLINFO )
	{
		sql.Query(wxT("SELECT id, trackName, ")
		          wxT("leadArtistName, orgArtistName, composerName, ")
		          wxT("albumName, comment, ")
		          wxT("trackNr, trackCount, diskNr, diskCount, ")
		          wxT("genreName, groupName, ")
		          wxT("year, beatsperminute, ")
		          wxT("rating, playtimeMs, autovol, ")
		          wxT("bitrate, samplerate, channels, databytes, ")
		          wxT("lastplayed, timesplayed, timeadded, timemodified ")
		          wxT("FROM tracks WHERE url='") + sql.QParam(url) + wxT("';"));
		if( sql.Next() )
		{
			trackInfo.m_url             = url;
			trackInfo.m_id              = sql.GetLong  (0);
			trackInfo.m_trackName       = sql.GetString(1);
			trackInfo.m_leadArtistName  = sql.GetString(2);
			trackInfo.m_orgArtistName   = sql.GetString(3);
			trackInfo.m_composerName    = sql.GetString(4);
			trackInfo.m_albumName       = sql.GetString(5);
			trackInfo.m_comment         = sql.GetString(6);
			trackInfo.m_trackNr         = sql.GetLong  (7);
			trackInfo.m_trackCount      = sql.GetLong  (8);
			trackInfo.m_diskNr          = sql.GetLong  (9);
			trackInfo.m_diskCount       = sql.GetLong  (10);
			trackInfo.m_genreName       = sql.GetString(11);
			trackInfo.m_groupName       = sql.GetString(12);
			trackInfo.m_year            = sql.GetLong  (13);
			trackInfo.m_beatsPerMinute  = sql.GetLong  (14);
			trackInfo.m_rating          = sql.GetLong  (15);
			trackInfo.m_playtimeMs      = sql.GetLong  (16);
			trackInfo.m_autoVol         = sql.GetLong  (17);
			trackInfo.m_bitrate         = sql.GetLong  (18);
			trackInfo.m_samplerate      = sql.GetLong  (19);
			trackInfo.m_channels        = sql.GetLong  (20);
			trackInfo.m_dataBytes       = sql.GetLong  (21);
			trackInfo.m_lastPlayed      = sql.GetLong  (22);
			trackInfo.m_timesPlayed     = sql.GetLong  (23);
			trackInfo.m_timeAdded       = sql.GetLong  (24);
			trackInfo.m_timeModified    = sql.GetLong  (25);
		}
		else
		{
			ret = false;
		}
	}

	if( flags & (SJ_TI_TRACKCOVERURL|SJ_TI_ALBUMCOVERURL__) )
	{
		// find out the album ID
		long albumId = 0;
		sql.Query(wxT("SELECT albumid FROM tracks WHERE url='") + sql.QParam(url) + wxT("';"));
		if( sql.Next() )
			albumId = sql.GetLong(0);

		if( flags & SJ_TI_TRACKCOVERURL )
		{
			// try to set TRACK cover - this is the first embedded cover; else the ALBUM cover will be returned
			if( g_mainFrame->m_skinFlags&SJ_SKIN_PREFER_TRACK_COVER )
			{
				wxArrayLong artIds;
				wxArrayString artUrls;
				GetPossibleAlbumArts(albumId, artIds, &artUrls, false);

				wxString soll = url, ist;
				soll.Replace(wxT("\\"), wxT("/"));
				long sollLen = soll.Len();

				size_t i;
				for( i = 0; i < artUrls.GetCount(); i++ )
				{
					ist = artUrls.Item(i);
					ist.Replace(wxT("\\"), wxT("/"));
					if( ist.Left(sollLen) == soll )
					{
						trackInfo.AddArt(artUrls.Item(i));
						break;
					}
				}
			}

			if( trackInfo.m_arts.IsEmpty() )
				flags |= SJ_TI_ALBUMCOVERURL__;
		}

		if( flags & SJ_TI_ALBUMCOVERURL__ )
		{
			// set the ALBUM cover
			sql.Query(wxString::Format(wxT("SELECT artiduser, artidauto FROM albums WHERE id=%lu;"), albumId));
			if( sql.Next() )
			{
				long artIdUser = sql.GetLong(0);
				long artIdAuto = sql.GetLong(1);
				if( (artIdAuto || artIdUser) && artIdUser!=SJ_DUMMY_COVER_ID )
				{
					sql.Query(wxString::Format(wxT("SELECT url FROM arts WHERE id=%lu;"), artIdUser? artIdUser : artIdAuto));
					if( sql.Next() )
					{
						// use real cover
						trackInfo.AddArt(sql.GetString(0));
					}
				}
				else
				{
					// use dummy cover
					trackInfo.AddArt(GetDummyCoverUrl(albumId));
				}
			}
		}
	}

	if( !ret && logErrors )
	{
		// TRANSLATORS: %s will be replaced by a filename
		wxLogWarning(_("\"%s\" is not in your music library and cannot be edited therefore."), url.c_str());
	}

	return ret;
}


wxArrayString SjLibraryModule::GetUniqueValues(long what)
{
	wxString        name = what==SJ_TI_GENRENAME? wxT("genrename") : wxT("groupname");

	wxSqlt          sql;
	wxString        allValuesString = sql.ConfigRead(wxT("library/") + name, wxEmptyString);
	wxArrayString   allValuesArray;

	wxString        currValue;

	wxStringTokenizer tkz(allValuesString, wxT("\n\r"));
	while( tkz.HasMoreTokens() )
	{
		currValue = tkz.GetNextToken();
		if( !currValue.IsEmpty() )
		{
			allValuesArray.Add(currValue);
		}
	}

	return allValuesArray;
}


bool SjLibraryModule::GetAutoComplete(long what, const wxString& in, wxString& out, bool internalCall)
{
	wxString    name;
	wxSqlt      sql;

	if( what == SJ_TI_ALBUMNAME )      { name = wxT("albumname");      }
	else if( what == SJ_TI_GENRENAME )      { name = wxT("genrename");      }
	else if( what == SJ_TI_GROUPNAME )      { name = wxT("groupname");      }
	else if( what == SJ_TI_LEADARTISTNAME ) { name = wxT("leadartistname"); }
	else if( what == SJ_TI_ORGARTISTNAME )  { name = wxT("orgartistname");  }
	else if( what == SJ_TI_COMPOSERNAME )   { name = wxT("composername");   }
	else if( what == SJ_TI_COMMENT )        { name = wxT("comment");        }
	else if( what == SJ_TI_TRACKNAME )      { name = wxT("trackname");      }
	else if( what == SJ_TI_URL )            { name = wxT("url");            }


	sql.Query(wxT("SELECT ") + name + wxT(" FROM tracks WHERE ") + name + wxT(" LIKE '") + sql.QParam(in) + wxT("%' ORDER BY ") + name + wxT(" LIMIT 1;"));
	if( sql.Next() )
	{
		out = sql.GetString(0);
		if( out.Len() > in.Len() )
		{
			return TRUE;
		}
	}

	if( !internalCall )
	{
		if( what == SJ_TI_LEADARTISTNAME )
		{
			if( GetAutoComplete(SJ_TI_ORGARTISTNAME, in, out, TRUE)
			        || GetAutoComplete(SJ_TI_COMPOSERNAME, in, out, TRUE) )
			{
				return TRUE;
			}
		}
		else if( what == SJ_TI_ORGARTISTNAME )
		{
			if( GetAutoComplete(SJ_TI_LEADARTISTNAME, in, out, TRUE)
			        || GetAutoComplete(SJ_TI_COMPOSERNAME, in, out, TRUE) )
			{
				return TRUE;
			}
		}
		else if( what == SJ_TI_COMPOSERNAME )
		{
			if( GetAutoComplete(SJ_TI_LEADARTISTNAME, in, out, TRUE)
			        || GetAutoComplete(SJ_TI_ORGARTISTNAME, in, out, TRUE) )
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}


void SjLibraryModule::DoAutoComplete(long what, int& oldTypedLen, wxWindow* window)
{
	// get window pointer
	wxTextCtrl* textCtrl = NULL;
	wxComboBox* comboBox = NULL;

	wxString in;
	long     inSelStart, inSelEnd;
	wxString className = window->GetClassInfo()->GetClassName();
	if( className == wxT("wxTextCtrl") )
	{
		textCtrl = (wxTextCtrl*)window;
		in = textCtrl->GetValue();
		textCtrl->GetSelection(&inSelStart, &inSelEnd);
	}
	else if( className == wxT("wxComboBox") )
	{
		comboBox = (wxComboBox*)window;
		in = comboBox->GetValue();
		inSelStart = inSelEnd = in.Len();
	}
	else
	{
		return;
	}

	if( inSelStart == inSelEnd
	        && inSelStart == (int)in.Len()
	        && oldTypedLen < (int)in.Len() )
	{
		wxString out;
		if( GetAutoComplete(what, in, out) )
		{
			if( textCtrl )
			{
				textCtrl->SetValue(out);
				textCtrl->SetSelection(in.Len(), out.Len());
			}
			else
			{
				comboBox->SetValue(out);
				comboBox->SetSelection(in.Len(), out.Len());
			}
		}
	}

	oldTypedLen = (int)in.Len();
}


long SjLibraryModule::GetMaskedColIndexByAz(int targetId)
{
	char az = 'a' + (targetId-IDT_WORKSPACE_GOTO_A);
	wxSqlt sql;

	if( !m_search.m_simple.IsSet() )
	{
		while( az >= 'a' )
		{
			sql.Query(wxString::Format(wxT("SELECT albumindex FROM albums WHERE azfirst=%i;"), (int)az));
			if( sql.Next() )
			{
				long orgIndex = sql.GetLong(0);

				if( m_search.m_adv.IsSet() )
				{
					// search correct index in adv. search
					long i;
					for( i = m_searchOffsetsCount-1; i >= 0; i-- )
					{
						if( m_searchOffsets[i] <= orgIndex )
						{
							// here is our offset -- but there may be a better one when looking forward
							sql.Query(wxString::Format(wxT("SELECT az FROM albums WHERE albumindex=%i;"), (int)m_searchOffsets[i]));
							if( sql.Next() && sql.GetLong(0) != az )
							{
								sql.Query(wxString::Format(wxT("SELECT az FROM albums WHERE albumindex=%i;"), (int)m_searchOffsets[i+1]));
								if( sql.Next() && sql.GetLong(0) == az )
								{
									return i+1;
								}
							}
							return i;
						}
					}

					return 0; // first column
				}
				else
				{
					// no search -- thats simple
					return orgIndex;
				}
			}
			az--;
		}
	}

	return 0; // first column
}


long SjLibraryModule::GetMaskedColIndexByColUrl(const wxString& colUrl)
{
	long   index = -1, i;
	wxSqlt sql;

	sql.Query(wxT("SELECT albumindex FROM albums WHERE url='") + sql.QParam(colUrl) + wxT("';"));
	if( sql.Next() )
	{
		index = sql.GetLong(0);
		if( HasSearch() )
		{
			for( i = 0; i < m_searchOffsetsCount; i++ )
			{
				if( m_searchOffsets[i] == index )
				{
					index = i;
					break;
				}
			}

			if( i >= m_searchOffsetsCount )
			{
				index = -1; // column does not exist in the current search
			}
		}
	}

	return index;
}


wxString SjLibraryModule::GetUrl(long id)
{
	wxSqlt sql;
	sql.Query(wxString::Format(wxT("SELECT url FROM tracks WHERE id=%i;"), (int)id));
	if( sql.Next() )
	{
		return sql.GetString(0);
	}
	else
	{
		return wxEmptyString;
	}
}


wxString SjLibraryModule::GetUrl(const wxString& leadArtistName__, const wxString& albumName__, const wxString& trackName__)
{
	wxString leadArtistName(leadArtistName__); leadArtistName.Trim(true).Trim(false);
	wxString albumName     (albumName__     ); albumName.     Trim(true).Trim(false);
	wxString trackName     (trackName__     ); trackName.     Trim(true).Trim(false);
	wxSqlt   sql;

	if( !albumName.IsEmpty() )
	{
		// try artist/album/track, regard case
		sql.Query(wxT("SELECT url FROM tracks WHERE leadartistname='") + sql.QParam(leadArtistName) + wxT("' AND albumname='") + sql.QParam(albumName) + wxT("' AND trackname='") + sql.QParam(trackName) + wxT("';"));
		if( sql.Next() )
			return sql.GetString(0);

		// try artist/album/track, ignore case
		sql.Query(wxT("SELECT url FROM tracks WHERE leadartistname LIKE '") + sql.QParam(leadArtistName) + wxT("' AND albumname LIKE '") + sql.QParam(albumName) + wxT("' AND trackname LIKE '") + sql.QParam(trackName) + wxT("';"));
		if( sql.Next() )
			return sql.GetString(0);
	}

	// try artist/track, regard case
	sql.Query(wxT("SELECT url FROM tracks WHERE leadartistname='") + sql.QParam(leadArtistName) + wxT("' AND trackname='") + sql.QParam(trackName) + wxT("';"));
	if( sql.Next() )
		return sql.GetString(0);

	// try artist/track, ignore case
	sql.Query(wxT("SELECT url FROM tracks WHERE leadartistname LIKE '") + sql.QParam(leadArtistName) + wxT("' AND trackname LIKE '") + sql.QParam(trackName) + wxT("';"));
	if( sql.Next() )
		return sql.GetString(0);

	// finally, try a artist/track reverse search
	sql.Query(wxT("SELECT url FROM tracks WHERE leadartistname LIKE '") + sql.QParam(trackName) + wxT("' AND trackname LIKE '") + sql.QParam(leadArtistName) + wxT("';"));
	if( sql.Next() )
		return sql.GetString(0);

	// nothing found
	return wxEmptyString;
}


long SjLibraryModule::GetId(const wxString& colUrl)
{
	wxSqlt sql;
	sql.Query(wxT("SELECT id FROM tracks WHERE url='") + sql.QParam(colUrl) + wxT("';"));
	if( sql.Next() )
	{
		return sql.GetLong(0);
	}
	else
	{
		return 0;
	}
}


wxString SjLibraryModule::GetDummyCoverUrl(long albumId)
{
	wxString ret;
	wxSqlt   sql;
	sql.Query(wxString::Format(wxT("SELECT leadartistname, albumname FROM albums WHERE id=%lu;"), albumId));
	if( sql.Next() )
	{
		return SjImgOp::GetDummyCoverUrl(sql.GetString(0), sql.GetString(1));
	}
	else
	{
		return SjImgOp::GetDummyCoverUrl(wxT(""), wxT(""));
	}
}


wxString SjLibraryModule::GetUpText()
{
	wxString ret;
	if( m_search.m_simple.IsSet() )
	{
		if(  m_search.m_adv.IsSet()
		        && (g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL) || g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL_GENRE) ) )
		{
			ret = m_search.m_adv.GetName() + wxT(" - ");
		}

		ret += wxString::Format(_("%s tracks on %s albums found for \"%s\""),
		                        SjTools::FormatNumber(m_searchTracksHash.GetCount()).c_str(),
		                        SjTools::FormatNumber(m_searchOffsetsCount).c_str(),
		                        m_search.m_simple.GetWords().c_str());
	}
	else if( m_search.m_adv.IsSet() )
	{
		if( g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL) || g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL_GENRE) )
		{
			ret = m_search.m_adv.GetName() + wxString(wxT(" - "));
		}

		ret += wxString::Format(_("%s tracks on %s albums"),
		                        SjTools::FormatNumber(m_searchTracksHash.GetCount()).c_str(),
		                        SjTools::FormatNumber(m_searchOffsetsCount).c_str());
	}
	else
	{
		ret.Printf(_("%s tracks on %s albums"),
		           SjTools::FormatNumber(GetUnmaskedTrackCount()).c_str(),
		           SjTools::FormatNumber(GetMaskedColCount()).c_str());
	}

	return ret;
}


SjCol* SjLibraryModule::GetCol__(long dbAlbumIndex, long virtualAlbumIndex, bool regardSearch)
{
	wxSqlt sql;

	// get album information
	sql.Query(wxString::Format(wxT("SELECT id, leadartistname, albumname, az, azfirst, artidauto, artiduser, url FROM albums WHERE albumindex=%lu"), dbAlbumIndex));
	if( !sql.Next() )
	{
		wxLogDebug(wxT("SELECT id, leadartistname, albumname, az, azfirst, artidauto, artiduser, url FROM albums WHERE albumindex=%lu"), dbAlbumIndex);
		return NULL;
	}
	long     albumId        = sql.GetLong(0);
	wxString albumArtistName= sql.GetString(1);
	wxString albumAlbumName = sql.GetString(2);
	long     albumYearMax   = 0, albumYearMin = 9999; // calculated on track iterating
	int      az             = sql.GetLong(3);
	int      azfirst        = sql.GetLong(4);
	long     artIdAuto      = sql.GetLong(5);
	long     artIdUser      = sql.GetLong(6);
	wxString url            = sql.GetString(7);

	// hilite omit words on album
	if( HasSearch() )
	{
		HiliteSearchWords(albumArtistName);
		HiliteSearchWords(albumAlbumName);
	}

	if( m_flags&SJ_LIB_OMITTOEND )
	{
		albumArtistName = m_omitArtist.Apply(albumArtistName);
		albumAlbumName  = m_omitAlbum.Apply(albumAlbumName);
	}

	// create column object...
	SjCol* col = new SjCol(this);
	col->m_url = url;

	// ...set a-z "up" text
	if( m_search.m_simple.IsSet() )
	{
		col->m_az = 0;

		if( virtualAlbumIndex == 0 )
		{
			if(  m_search.m_adv.IsSet()
			        && (g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL) || g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL_GENRE)) )
			{
				col->m_textUp =  m_search.m_adv.GetName() + wxT(" - ");
			}

			col->m_textUp += wxString::Format(_("%s tracks on %s albums found for \"%s\""),
			                                  SjTools::FormatNumber(m_searchTracksHash.GetCount()).c_str(),
			                                  SjTools::FormatNumber(m_searchOffsetsCount).c_str(),
			                                  m_search.m_simple.GetWords().c_str());
		}
	}
	else if( m_search.m_adv.IsSet() )
	{
		col->m_az = az;

		if( virtualAlbumIndex == 0 )
		{
			if( g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL) || g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL_GENRE) )
			{
				col->m_textUp = m_search.m_adv.GetName() + wxString(wxT(" - "));
			}

			col->m_textUp += wxString::Format(_("%s tracks on %s albums"),
			                                  SjTools::FormatNumber(m_searchTracksHash.GetCount()).c_str(),
			                                  SjTools::FormatNumber(m_searchOffsetsCount).c_str());

			if( !m_filterAzFirstHidden ) // for very small albums view, hide the display "Albums - A" etc.
			{
				col->m_textUp += wxT(" - ") + SjColumnMixer::GetAzDescr(az);
			}
		}
		else if( !m_filterAzFirstHidden )
		{
			for( int i=0; i<27; i++ )
			{
				if( m_filterAzFirst[i] == virtualAlbumIndex )
				{
					if( g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL) || g_mainFrame->IsOpAvailable(SJ_OP_MUSIC_SEL_GENRE) )
					{
						col->m_textUp = m_search.m_adv.GetName() + wxT(" - ") + SjColumnMixer::GetAzDescr(az);
					}
					else
					{
						col->m_textUp = _("Albums") + wxString(wxT(" - ")) + SjColumnMixer::GetAzDescr(az);
					}
					break;
				}
			}
		}
	}
	else
	{
		col->m_az = az;

		if( azfirst )
		{
			wxASSERT(azfirst==az);

			if( dbAlbumIndex == 0 )
			{
				col->m_textUp.Printf(_("%s tracks on %s albums"),
				                     SjTools::FormatNumber(GetUnmaskedTrackCount()).c_str(),
				                     SjTools::FormatNumber(GetMaskedColCount()).c_str());
			}
			else
			{
				col->m_textUp = _("All albums");
			}

			col->m_textUp += wxT(" - ") + SjColumnMixer::GetAzDescr(az);
		}
	}

	// ...set covers
	SjAlbumCoverRow* coverRow = new SjAlbumCoverRow(albumId);
	if( (artIdAuto || artIdUser) && artIdUser!=SJ_DUMMY_COVER_ID )
	{
		sql.Query(wxString::Format(wxT("SELECT url FROM arts WHERE id=%lu;"), artIdUser? artIdUser : artIdAuto));
		if( sql.Next() )
		{
			// use real cover
			coverRow->m_textm = sql.GetString(0);
		}
	}
	else
	{
		// use dummy cover
		coverRow->m_textm = GetDummyCoverUrl(albumId);
	}

	col->AddRow(coverRow);

	// ...set titles
	SjRow* addYearRow = NULL;
	SjRow* addNumpadNumRow = NULL;
	bool showArtistName = (m_flags&SJ_LIB_SHOWLEADARTISTNAME)!=0;
	bool showAlbumName = (m_flags&SJ_LIB_SHOWALBUMNAME)!=0;
	bool showNumpadNumbers = (g_kioskModule && g_accelModule && g_accelModule->UseNumpad());

	if( !albumArtistName.IsEmpty() && !albumAlbumName.IsEmpty() )
	{
		// get the rows,
		// set the numpad row to the artist row
		SjAlbumArtistRow* artistRow_ = NULL;
		SjAlbumAlbumRow*  albumRow_ = NULL;

		if( showArtistName )
		{
			artistRow_ = new SjAlbumArtistRow(SJ_RRTYPE_TITLE1, albumId, albumArtistName);
			addYearRow = artistRow_;
			if( showNumpadNumbers )
			{
				addNumpadNumRow = artistRow_;
			}
		}

		if( showAlbumName || (showNumpadNumbers && addNumpadNumRow==NULL) )
		{
			albumRow_ = new SjAlbumAlbumRow(showArtistName? SJ_RRTYPE_TITLE2 : SJ_RRTYPE_TITLE1, albumId, albumAlbumName);
			addYearRow = albumRow_;
			if( showNumpadNumbers && addNumpadNumRow==NULL )
			{
				addNumpadNumRow = albumRow_;
			}
		}

		// add the rows regarding the sorting criteria,
		// adapt the numpad row if sorting by album first
		if( m_sort == SJ_LIBSORT_ALBUM_YEAR_ARTIST || m_sort == SJ_LIBSORT_ALBUM_ARTIST_YEAR )
		{
			if( albumRow_ )
			{
				albumRow_->m_roughType = SJ_RRTYPE_TITLE1;
				col->AddRow(albumRow_);
				if( showNumpadNumbers )
				{
					addNumpadNumRow = albumRow_;
				}
			}

			if( artistRow_ )
			{
				artistRow_->m_roughType = albumRow_? SJ_RRTYPE_TITLE2 : SJ_RRTYPE_TITLE1;
				col->AddRow(artistRow_);
			}
		}
		else
		{
			if( artistRow_ ) col->AddRow(artistRow_);
			if( albumRow_ )  col->AddRow(albumRow_);
		}
	}
	else if( !albumArtistName.IsEmpty() )
	{
		if( showArtistName || showAlbumName || showNumpadNumbers /*assume the album name be be equal to the artist name in this case*/ )
		{
			addYearRow = new SjAlbumArtistRow(SJ_RRTYPE_TITLE1, albumId, albumArtistName);
			if( showNumpadNumbers )
			{
				addNumpadNumRow = addYearRow;
			}
			col->AddRow(addYearRow);
		}
	}
	else if( !albumAlbumName.IsEmpty() )
	{
		if( showAlbumName || showNumpadNumbers )
		{
			addYearRow = new SjAlbumAlbumRow(SJ_RRTYPE_TITLE1, albumId, albumAlbumName);
			if( showNumpadNumbers )
			{
				addNumpadNumRow = addYearRow;
			}
			col->AddRow(addYearRow);
		}
	}
	else
	{
		if( showAlbumName || showNumpadNumbers )
		{
			addYearRow = new SjAlbumArtistRow(SJ_RRTYPE_TITLE1, albumId, _("Unsorted tracks"));
			if( showNumpadNumbers )
			{
				addNumpadNumRow = addYearRow;
			}
			col->AddRow(addYearRow);
		}
	}

	// ...add numpad album number
	if( addNumpadNumRow )
	{
		addNumpadNumRow->m_textl = g_kioskModule->m_numpadInput.DbAlbumIndex2String(dbAlbumIndex);
	}

	// go through all tracks
	bool        showDiffLeadArtistName, showDiffAlbumName;

	long        trackId;
	wxString    trackAlbumName;
	wxString    trackName, lastTrackName;
	wxString    trackLeadArtistName;
	wxString    trackOrgArtistName;
	wxString    trackComposerName;
	long        trackYear;
	long        trackNr, automTrackNr = 0;
	long        trackPlaytimeMs, lastPlaytimeMs = 0, trackRating = 0;
	SjRow*      trackRow;
	wxString    trackUrl, trackComment, trackGenre;

	long        diskNr, albumDiskNr = 0, albumDiskCount = 0;
	SjRow*      diskNrRow = NULL;

	sql.Query(wxString::Format(wxT("SELECT id, albumname, trackname, leadartistname, orgartistname, composername, ")
	                           wxT("year, tracknr, playtimems, url, disknr, comment, genrename, rating FROM tracks WHERE albumid=%lu ORDER BY disknr, tracknr, trackname, id;"), albumId));
	while( sql.Next() )
	{
		showDiffLeadArtistName
		    = (m_flags&SJ_LIB_SHOWDIFFLEADARTISTNAME)!=0;
		showDiffAlbumName   = (m_flags&SJ_LIB_SHOWDIFFALBUMNAME)!=0;

		trackId             = sql.GetLong(0);
		trackAlbumName      = sql.GetString(1);
		trackName           = sql.GetString(2);
		trackLeadArtistName = sql.GetString(3);
		trackOrgArtistName  = sql.GetString(4);
		trackComposerName   = sql.GetString(5);
		trackYear           = sql.GetLong(6);
		trackNr             = sql.GetLong(7);
		trackPlaytimeMs     = sql.GetLong(8);
		trackUrl            = sql.GetString(9);
		diskNr              = sql.GetLong(10);
		trackComment        = sql.GetString(11);
		trackGenre          = sql.GetString(12);
		trackRating         = sql.GetLong(13);

		// Avoid double tracks.
		// The comparison implies the same artis- and albumname
		// and the same disk- and tracknumber
		#define MS_TOLERANCE 1200
		if( !(m_flags&SJ_LIB_SHOWDOUBLETRACKS)
		        && (lastPlaytimeMs==0 || trackPlaytimeMs==0 || (lastPlaytimeMs>trackPlaytimeMs-MS_TOLERANCE && lastPlaytimeMs<trackPlaytimeMs+MS_TOLERANCE))
		        && lastTrackName == trackName )
		{
			continue;
		}

		lastPlaytimeMs= trackPlaytimeMs;
		lastTrackName = trackName;

		// add disk-nr. row (may be removed later)
		if( diskNr < 1 ) diskNr = 1;
		if( (m_flags&SJ_LIB_SHOWDISKNR)
		        && diskNr
		        && diskNr != albumDiskNr )
		{
			// remove last row if this is a disk-nr. row
			if( (diskNrRow=col->GetLastRow())!=NULL
			        && diskNrRow->m_roughType == SJ_RRTYPE_TITLE3 )
			{
				delete col->RemoveLastRow();
			}

			// add new disk-nr. row
			diskNrRow = new SjAlbumDiskRow(albumId, diskNr);
			col->AddRow(diskNrRow);

			diskNrRow->m_textm = wxString::Format(_("Disk %i"), (int)diskNr);
			albumDiskNr = diskNr;
			albumDiskCount++;

			if( !showNumpadNumbers )
			{
				automTrackNr = 0;
			}
		}

		// track in search?
		if( regardSearch && HasSearch() && m_searchTracksHash.Lookup(trackId)==0 )
		{
			automTrackNr++;
			continue;
		}

		// highlite words
		// don't hilite original artist or composer, as we won't search for it
		if( HasSearch() )
		{
			HiliteSearchWords(trackName);
			if( HiliteSearchWords(trackAlbumName)      ) { showDiffAlbumName      = TRUE; }
			if( HiliteSearchWords(trackLeadArtistName) ) { showDiffLeadArtistName = TRUE; }
		}

		// omit words
		if( m_flags&SJ_LIB_OMITTOEND )
		{	trackLeadArtistName = m_omitArtist.Apply(trackLeadArtistName);
			if( m_flags&SJ_LIB_SHOWORGARTISTNAME )  { trackOrgArtistName  = m_omitArtist.Apply(trackOrgArtistName); }
			if( m_flags&SJ_LIB_SHOWCOMPOSERNAME )   { trackComposerName   = m_omitArtist.Apply(trackComposerName);  }
			if( showDiffAlbumName )                 { trackAlbumName      = m_omitAlbum.Apply(trackAlbumName);      }
		}

		// add track row...
		trackRow = new SjAlbumTrackRow(this, trackId, trackUrl);
		col->AddRow(trackRow);

		// ...track-nr.
		automTrackNr++;
		if( showNumpadNumbers )
		{
			trackRow->m_textl.Printf(wxT("%i"), (int)automTrackNr);
		}
		else if( m_flags&SJ_LIB_SHOWTRACKNR )
		{
			if( m_flags&SJ_LIB_AUTOMTRACKNR )
			{
				trackRow->m_textl.Printf(wxT("%i."), (int)automTrackNr);
			}
			else if( trackNr )
			{
				trackRow->m_textl.Printf(wxT("%i."), (int)trackNr);
			}
			else
			{
				trackRow->m_textl = wxT("- "); // make sure, the left text is not emtpty
			}                             // as it is also used for status information
		}
		else
		{
			trackRow->m_textl = wxT("- ");     //    - " -
		}

		// ...title
		trackRow->m_textm = trackName;
		if( showDiffLeadArtistName  )
		{
			if( !trackLeadArtistName.IsEmpty()
			        &&  trackLeadArtistName.CmpNoCase(albumArtistName)!=0 )
			{
				trackRow->m_textm += wxT(" (") + trackLeadArtistName + wxT(")");
			}

			if(  (m_flags&SJ_LIB_SHOWORGARTISTNAME)
			        && !trackOrgArtistName.IsEmpty()
			        &&  trackOrgArtistName.CmpNoCase(albumArtistName)!=0
			        &&  trackOrgArtistName.CmpNoCase(trackLeadArtistName)!=0 )
			{
				trackRow->m_textm += wxT(" (") + wxString(_("O:"))/*abbreviation for "Original artist:"*/ + wxT(" ") + trackOrgArtistName + wxT(")");
			}

			if(  (m_flags&SJ_LIB_SHOWCOMPOSERNAME)
			        && !trackComposerName.IsEmpty()
			        &&  trackComposerName.CmpNoCase(albumArtistName)!=0
			        &&  trackComposerName.CmpNoCase(trackLeadArtistName)!=0
			        &&  trackComposerName.CmpNoCase(trackOrgArtistName)!=0 )
			{
				trackRow->m_textm += wxT(" (") + wxString(_("C:"))/*abbreviation for "Composer:"*/ + wxT(" ") + trackComposerName + wxT(")");
			}

			if(  (m_flags&SJ_LIB_SHOWGENRE)
			        && !trackGenre.IsEmpty() )
			{
				trackRow->m_textm += wxT(" (") + trackGenre + wxT(")");
			}

			if(  (m_flags&SJ_LIB_SHOWCOMMENT)
			        && !trackComment.IsEmpty() )
			{
				#define SJ_LIB_SHOWCOMMENT_MAX_CHARS 40

				trackComment.Replace(wxT("\n"), wxT(" "));
				trackComment.Replace(wxT("\r"), wxT(" "));
				while( trackComment.Find(wxT("  ")) != -1 )
					trackComment.Replace(wxT("  "), wxT(" "));

				trackComment.Trim(true);
				trackComment.Trim(false);
				if( trackComment.Len() > SJ_LIB_SHOWCOMMENT_MAX_CHARS )
				{
					trackComment = trackComment.Left(SJ_LIB_SHOWCOMMENT_MAX_CHARS-3).Trim() + wxT("..");
				}

				if( !trackComment.IsEmpty() )
				{
					trackRow->m_textm += wxT(" (") + trackComment + wxT(")");
				}
			}

			if( (m_flags&SJ_LIB_SHOWRATING)
			        && trackRating > 0 )
			{
				long r = trackRating; if( r>5 ) r = 5;
				trackRow->m_textm += wxT(" ");
				while(r--) { trackRow->m_textm += SJ_RATING_CHARS_ELSEWHERE; }
			}
		}

		if( showDiffAlbumName )
		{
			if( !trackAlbumName.IsEmpty()
			        &&  trackAlbumName.CmpNoCase(albumAlbumName)!=0 )
			{
				trackRow->m_textm += wxString::Format(wxT(" (A: %s)"), trackAlbumName.c_str());
			}
		}

		// ...playing time
		if( (m_flags&SJ_LIB_SHOWTIME) && trackPlaytimeMs )
		{
			trackRow->m_textr = SjTools::FormatTime(trackPlaytimeMs/1000);
		}

		// calculate year
		if( trackYear > albumYearMax ) { albumYearMax = trackYear; }
		if( trackYear > 1000 && trackYear < albumYearMin ) { albumYearMin = trackYear; }
	}

	// remove single or last disk-nr. row
	if( albumDiskCount == 1 )
	{
		wxASSERT(diskNrRow);
		col->RemoveRow(diskNrRow);
		delete diskNrRow;
	}

	if( (diskNrRow=col->GetLastRow())!=NULL
	        && diskNrRow->m_roughType == SJ_RRTYPE_TITLE3 )
	{
		delete col->RemoveLastRow();
	}

	// add year to album name
	if( (m_flags&SJ_LIB_SHOWYEAR)
	        && albumYearMax
	        && addYearRow )
	{
		if( albumYearMin != 9999 && albumYearMin != albumYearMax )
		{
			wxASSERT(albumYearMin < albumYearMax);
			addYearRow->m_textm += wxString::Format(wxT(" (%i-%i)"), (int)albumYearMin, (int)albumYearMax);
		}
		else
		{
			addYearRow->m_textm += wxString::Format(wxT(" (%i)"), (int)albumYearMax);
		}
	}

	// finalize the Numpad track numbers regading the largest number
	if( showNumpadNumbers )
	{
		int i;
		for( i = 0; i < col->m_rowCount; i++ )
		{
			trackRow = col->m_rows[i];
			wxASSERT( trackRow );
			if(  trackRow
			        && (trackRow->m_roughType<SJ_RRTYPE_TITLE1 || trackRow->m_roughType>SJ_RRTYPE_TITLE3)
			        && !trackRow->m_textl.IsEmpty() )
			{
				g_kioskModule->m_numpadInput.FinalizeTrackNumber(trackRow->m_textl, automTrackNr);
			}
		}
	}

	return col;
}


SjCol* SjLibraryModule::GetMaskedCol(const wxString& trackUrl, long& retIndex /*-1 if currently hidden eg. by search*/)
{
	wxSqlt sql;
	sql.Query(wxT("SELECT albumid FROM tracks WHERE url='") + sql.QParam(trackUrl) + wxT("';"));
	if( sql.Next() )
	{
		long albumId = sql.GetLong(0);
		sql.Query(wxString::Format(wxT("SELECT albumindex FROM albums WHERE id=%lu;"), albumId));
		if( sql.Next() )
		{
			retIndex = sql.GetLong(0);
			SjCol* ret = GetCol__(retIndex, retIndex, TRUE/*regardSearch*/);
			if( ret )
			{
				if( HasSearch() )
				{
					int i; bool indexFound = FALSE;
					for( i = 0; i < m_searchOffsetsCount; i++ )
					{
						if( m_searchOffsets[i] == retIndex )
						{
							retIndex = i;
							indexFound = TRUE;
							break;
						}
					}
					if( !indexFound ) retIndex = -1;
				}
				return ret;
			}
		}
	}

	return NULL;
}


wxString SjLibraryModule::ShortenName(const wxString& name__, int maxLen)
{
	wxString name(name__);

	if( (int)name.Len() > maxLen )
	{
		name = name.Left(maxLen-2).Trim() + wxT("..");
	}

	return name;
}


void SjLibraryModule::CreateMenu(SjMenu* enqueueMenu, SjMenu* editMenu, bool createMainMenu)
{
	enqueueMenu->Append(IDT_ENQUEUE_LAST);

	if( createMainMenu || g_mainFrame->IsOpAvailable(SJ_OP_EDIT_QUEUE) )
	{
		enqueueMenu->Append(IDT_ENQUEUE_NOW);
		enqueueMenu->Append(IDT_ENQUEUE_NEXT);
	}

	if( createMainMenu || g_mainFrame->IsOpAvailable(SJ_OP_UNQUEUE) )
	{
		enqueueMenu->Append(IDT_UNQUEUE);
	}

	if( createMainMenu || g_mainFrame->IsAllAvailable() )
	{
		if( editMenu == enqueueMenu )
		{
			editMenu->AppendSeparator();
		}

		editMenu->Append(IDM_EDITSELECTION);

		CreateRatingMenu(*editMenu, IDM_RATINGSELECTION00);

		CreateArtistInfoMenu(*editMenu, IDM_SELECTARTISTINF00);

		editMenu->Append(IDM_EXPLORE);
	}

	g_mainFrame->AddScriptMenuEntries(*editMenu);
}


void SjLibraryModule::UpdateMenu(SjMenu* enqueueMenu, SjMenu* editMenu, bool updateMainMenu)
{
	// get some info...
	wxString        shortenedTrackName;
	long            rating = 0,
	                trackCount = m_selectedTrackIds.GetCount();
	long            enqueuedCount = 0;
	wxString        currUrl;
	wxArrayString   allUrls;

	m_lastLeadArtistName.Clear();
	m_lastTrackName.Clear();

	if( trackCount == 1 )
	{
		// ...get single track information
		wxSqlt sql;
		long trackId = 0;
		SjHashIterator iterator6;
		m_selectedTrackIds.Iterate(iterator6, &trackId);
		sql.Query(wxString::Format(wxT("SELECT rating, trackname, url, leadartistname FROM tracks WHERE id=%lu;"), trackId));
		if( sql.Next() )
		{
			rating = sql.GetLong(0); if(rating<0)rating=0; if(rating>5)rating=5;
			currUrl = sql.GetString(2);
			allUrls.Add(currUrl);

			shortenedTrackName = ShortenName(sql.GetString(1), SJ_SHORTENED_TRACKNAME_LEN);

			m_lastLeadArtistName = sql.GetString(3);
			m_lastTrackName = sql.GetString(1);

			if( g_mainFrame->IsEnqueued(currUrl) )
			{
				enqueuedCount = 1;
			}
		}
	}
	else if( trackCount > 1 )
	{
		// get multiple track information
		if( trackCount>250)
		{
			::wxBeginBusyCursor();
		}

		wxSqlt sql;
		long currRating, currRatingCount = 0;
		bool artistNameUnique = TRUE;

		sql.Query(wxT("SELECT rating, url, leadartistname, trackname FROM tracks WHERE id IN(") + m_selectedTrackIds.GetKeysAsString() + wxT(");"));
		while( sql.Next() )
		{
			currRating = sql.GetLong(0);
			currUrl = sql.GetString(1);
			allUrls.Add(currUrl);
			if( allUrls.GetCount() == 1 )
			{
				shortenedTrackName = ShortenName(sql.GetString(3), SJ_SHORTENED_TRACKNAME_LEN);
			}

			if( currRating )
			{
				rating += currRating;
				currRatingCount++;
			}

			if( g_mainFrame->IsEnqueued(currUrl) )
			{
				enqueuedCount++;
			}

			if( artistNameUnique )
			{
				if( m_lastLeadArtistName.IsEmpty() )
				{
					m_lastLeadArtistName = sql.GetString(2);
				}
				else if( sql.GetString(2) != m_lastLeadArtistName )
				{
					artistNameUnique = FALSE;
					m_lastLeadArtistName.Clear();
				}
			}
		}

		if( currRatingCount > 0 )
		{
			rating /= currRatingCount;
		}

		if( trackCount>250)
		{
			::wxEndBusyCursor();
		}
	}

	// (un)queue menu items
	enqueueMenu->Enable(IDT_ENQUEUE_LAST, FALSE);
	enqueueMenu->SetLabel(IDT_ENQUEUE_LAST, _("Enqueue tracks"));

	enqueueMenu->Enable(IDT_ENQUEUE_NOW, FALSE);
	enqueueMenu->SetLabel(IDT_ENQUEUE_NOW,_("Play tracks now"));

	enqueueMenu->Enable(IDT_ENQUEUE_NEXT, FALSE);
	enqueueMenu->SetLabel(IDT_ENQUEUE_NEXT, _("Play tracks next"));

	enqueueMenu->Enable(IDT_UNQUEUE, FALSE);
	enqueueMenu->SetLabel(IDT_UNQUEUE, _("Unqueue tracks"));

	if( trackCount == 1 )
	{
		enqueueMenu->Enable(IDT_ENQUEUE_LAST, TRUE);
		enqueueMenu->SetLabel(IDT_ENQUEUE_LAST, wxString::Format(_("Enqueue \"%s\""), shortenedTrackName.c_str()));

		if( g_mainFrame->IsOpAvailable(SJ_OP_EDIT_QUEUE) )
		{
			enqueueMenu->Enable(IDT_ENQUEUE_NOW, TRUE);
			enqueueMenu->SetLabel(IDT_ENQUEUE_NOW, wxString::Format(_("Play \"%s\" now"), shortenedTrackName.c_str()));

			enqueueMenu->Enable(IDT_ENQUEUE_NEXT, TRUE);
			enqueueMenu->SetLabel(IDT_ENQUEUE_NEXT, wxString::Format(_("Play \"%s\" next"), shortenedTrackName.c_str()));
		}

		if( enqueuedCount>0 && g_mainFrame->IsOpAvailable(SJ_OP_UNQUEUE) )
		{
			enqueueMenu->Enable(IDT_UNQUEUE, TRUE);
			enqueueMenu->SetLabel(IDT_UNQUEUE, wxString::Format(_("Unqueue \"%s\""), shortenedTrackName.c_str()));
		}
	}
	else if( trackCount > 1 )
	{
		enqueueMenu->Enable(IDT_ENQUEUE_LAST, TRUE);
		enqueueMenu->SetLabel(IDT_ENQUEUE_LAST, wxString::Format(
		  // TRANSLATORS: %i will be replaced by the number of tracks
		  wxPLURAL("Enqueue %i track", "Enqueue %i tracks", trackCount),
		  (int)trackCount));

		if( g_mainFrame->IsOpAvailable(SJ_OP_EDIT_QUEUE) )
		{
			enqueueMenu->Enable(IDT_ENQUEUE_NOW, TRUE);
			enqueueMenu->SetLabel(IDT_ENQUEUE_NOW, wxString::Format(
			  // TRANSLATORS: %i will be replaced by the number of tracks
			  wxPLURAL("Play %i track now", "Play %i tracks now", trackCount),
			  (int)trackCount));

			enqueueMenu->Enable(IDT_ENQUEUE_NEXT, TRUE);
			enqueueMenu->SetLabel(IDT_ENQUEUE_NEXT, wxString::Format(
			  // TRANSLATORS: %i will be replaced by the number of tracks
			  wxPLURAL("Play %i track next", "Play %i tracks next", trackCount),
			  (int)trackCount));
		}

		if( enqueuedCount>0 && g_mainFrame->IsOpAvailable(SJ_OP_UNQUEUE) )
		{
			enqueueMenu->Enable(IDT_UNQUEUE, TRUE);
			enqueueMenu->SetLabel(IDT_UNQUEUE, wxString::Format(
			  // TRANSLATORS: %i will be replaced by the number of tracks
			  wxPLURAL("Unqueue %i track", "Unqueue %i tracks", enqueuedCount),
			  (int)enqueuedCount));
		}
	}

	if( !updateMainMenu && enqueueMenu->FindItem(IDT_UNQUEUE) && !enqueueMenu->IsEnabled(IDT_UNQUEUE) )
	{
		wxMenuItem* toDel = enqueueMenu->Remove(IDT_UNQUEUE);
		if( toDel )
			delete toDel; // this is not done by Remove()!
	}

	if( trackCount >= 1 && g_mainFrame->IsAllAvailable() )
	{
		editMenu->Enable(IDM_EDITSELECTION, TRUE);
		editMenu->Enable(IDM_EXPLORE, trackCount == 1);

		UpdateRatingMenu(*editMenu, IDM_RATINGSELECTION00, rating, trackCount);
	}
	else
	{
		editMenu->Enable(IDM_EDITSELECTION, FALSE);
		editMenu->Enable(IDM_EXPLORE, FALSE);

		UpdateRatingMenu(*editMenu, IDM_RATINGSELECTION00, 0, 0);
	}

	UpdateArtistInfoMenu(*editMenu, IDM_SELECTARTISTINF00);
}


void SjLibraryModule::UpdateMenuBar()
{
	if( g_mainFrame->m_playbackMenu
	        && g_mainFrame->m_editMenu )
	{
		UpdateMenu(g_mainFrame->m_playbackMenu, g_mainFrame->m_editMenu, TRUE);
	}
}


void SjLibraryModule::CreateArtistInfoMenu(SjMenu& m, int baseId, const wxArrayString& urls)
{
	wxSqlt sql;
	long trackCount = urls.GetCount(), i;
	m_lastLeadArtistName.Clear();
	m_lastTrackName.Clear();
	for( i = 0; i < trackCount; i++ )
	{
		sql.Query(wxT("SELECT leadArtistName, trackName FROM tracks WHERE url='") + sql.QParam(urls[i]) + wxT("';"));
		if( sql.Next() )
		{
			if( i == 0 )
			{
				m_lastLeadArtistName = sql.GetString(0);
				if( trackCount == 1 )
				{
					m_lastTrackName = sql.GetString(1);
				}
			}
			else if( m_lastLeadArtistName != sql.GetString(0) )
			{
				m_lastLeadArtistName.Clear();
				break;
			}
		}
	}

	CreateArtistInfoMenu(m, baseId);
	UpdateArtistInfoMenu(m, baseId);
}


void SjLibraryModule::CreateArtistInfoMenu(SjMenu& m, int baseId)
{
	SjMenu* artistInfoMenu = new SjMenu(m.ShowShortcuts());
	SjWebLinks weblinks(SJ_WEBLINK_ARTISTINFO);
	int i;
	for( i = 0; i < weblinks.GetCount(); i++ )
	{
		artistInfoMenu->Append(baseId+i, weblinks.GetName(i));
		if( weblinks.BreakAfter(i) ) artistInfoMenu->AppendSeparator();
	}

	m.Append(baseId+42, wxString::Format(_("\"%s\" on the web"), wxT("...")), artistInfoMenu);
}
void SjLibraryModule::UpdateArtistInfoMenu(SjMenu& m, int baseId)
{
	bool enable = !m_lastLeadArtistName.IsEmpty();

	SjWebLinks weblinks(SJ_WEBLINK_ARTISTINFO);
	int i;
	for( i = 0; i < weblinks.GetCount(); i++ )
	{
		m.Enable(baseId+i, enable);
	}

	m.Enable(baseId+42, enable);

	if( enable )
	{
		wxString artistName = ShortenName(m_lastLeadArtistName, SJ_SHORTENED_ARTISTNAME_LEN);
		m.SetLabel(baseId+42, wxString::Format(_("\"%s\" on the web"), artistName.c_str()));
	}
	else
	{
		m.SetLabel(baseId+42, wxString::Format(_("\"%s\" on the web"), wxT("...")));
	}
}


void SjLibraryModule::ShowArtistInfo(int baseId, int clickedId)
{
	SjWebLinks weblinks(SJ_WEBLINK_ARTISTINFO);
	weblinks.Explore(clickedId-baseId, m_lastLeadArtistName, wxT(""), m_lastTrackName);
}


void SjLibraryModule::CreateRatingMenu(SjMenu& m, int baseId, const wxArrayString& urls)
{
	wxSqlt sql;

	long i, trackCount = urls.GetCount(), rating = 0, ratingCount = 0;
	for( i = 0; i < trackCount; i++ )
	{
		sql.Query(wxT("SELECT rating FROM tracks WHERE url='") + sql.QParam(urls[i]) + wxT("';"));
		if( sql.Next() )
		{
			long currRating = sql.GetLong(0); if(rating<0)rating=0; if(rating>5)rating=5;
			if( currRating )
			{
				rating += currRating;
				ratingCount++;
			}
		}
	}

	if( ratingCount > 0 )
	{
		rating /= ratingCount;
	}

	CreateRatingMenu(m, baseId);
	UpdateRatingMenu(m, baseId, rating, trackCount);
}


void SjLibraryModule::CreateRatingMenu(SjMenu& m, int baseId)
{
	SjMenu* ratingMenu = new SjMenu(m.ShowShortcuts());
	ratingMenu->AppendRadioItem(baseId+1, SJ_RATING_CHARS_DLG);
	ratingMenu->AppendRadioItem(baseId+2, SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG);
	ratingMenu->AppendRadioItem(baseId+3, SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG);
	ratingMenu->AppendRadioItem(baseId+4, SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG);
	ratingMenu->AppendRadioItem(baseId+5, SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG);
	ratingMenu->AppendRadioItem(baseId+0, _("No rating"));

	m.Append(baseId+6,
	         _("Rating"),
	         ratingMenu);
}


void SjLibraryModule::UpdateRatingMenu(SjMenu& m, int baseId, long rating, long trackCount)
{
	bool enable = (trackCount > 0);
	int i;

	for( i = baseId; i <= baseId+5; i++ )
	{
		m.Enable(i, enable);
		m.Check(i, FALSE);
	}

	m.Enable(baseId+6, enable);

	if( enable )
	{
		m.Check(baseId+rating, TRUE);

		wxString chars;
		for(i = 0; i < rating; i++)
		{
			chars += SJ_RATING_CHARS_DLG;
		}

		m.SetLabel(baseId+6,    rating?
		           wxString::Format(trackCount>1?_("Average rating: %s"):_("Rating: %s"), chars.c_str())
			           :   wxString(_("Rating")) );
	}
	else
	{
		m.SetLabel(baseId+6,    _("Rating") );
	}
}


bool SjLibraryModule::SetRating(const wxArrayString& urls, long rating)
{
	bool ret = TRUE;
	long trackCount = urls.GetCount();

	if( trackCount > 1 )
	{
		// wxWindowDisabler disabler(g_mainFrame); -- done in YesNo()
		if( g_accelModule->YesNo(
		            wxString::Format(
		              // TRANSLATORS: %i will be replaced by the number of tracks
		              wxPLURAL("Do you want to change the rating for %i selected track?", "Do you want to change the rating for %i selected tracks?", trackCount),
		              (int)trackCount),
		            _("Change rating"),
		            g_mainFrame,
		            SJ_ACCEL_ASK_ON_RATING_CHANGE)!=wxYES )
		{
			return FALSE;
		}
	}

	{
		wxBusyCursor busy;
		wxSqlt sql;

		long i, setRatingCount = 0;
		for( i = 0; i < trackCount; i++ )
		{
			sql.Query(wxT("SELECT id FROM tracks WHERE url='") + sql.QParam(urls[i]) + wxT("';"));
			if( sql.Next() )
			{
				sql.Query(wxT("UPDATE tracks SET rating=") + sql.LParam(rating) + wxT(" WHERE url='") + sql.QParam(urls[i]) + wxT("';"));
				setRatingCount ++;
			}
			else
			{
				wxLogWarning(_("\"%s\" is not in your music library and cannot be edited therefore."), urls[i].c_str());
				ret = FALSE;
			}
		}

		if( setRatingCount > 0 && g_mainFrame && g_mainFrame->m_browser )
		{
			g_mainFrame->m_browser->RefreshAll();
		}
	}

	return ret;
}


void SjLibraryModule::HandleMenu(int id)
{
	if( id >= IDM_SELECTARTISTINF00 && id <= IDM_SELECTARTISTINF49 )
	{
		ShowArtistInfo(IDM_SELECTARTISTINF00, id);
	}
	else switch( id )
		{
			case IDM_RATINGSELECTION00:
			case IDM_RATINGSELECTION01:
			case IDM_RATINGSELECTION02:
			case IDM_RATINGSELECTION03:
			case IDM_RATINGSELECTION04:
			case IDM_RATINGSELECTION05:
			{
				long trackCount = m_selectedTrackIds.GetCount();
				if( trackCount > 1 )
				{
					// wxWindowDisabler disabler(g_mainFrame); -- done in YesNo()
					if( g_accelModule->YesNo(
					            wxString::Format(
					              // TRANSLATORS: %i will be replaced by the number of tracks
					              wxPLURAL("Do you want to change the rating for %i selected track?", "Do you want to change the rating for %i selected tracks?", trackCount),
					              (int)trackCount),
					            _("Change rating"),
					            g_mainFrame,
					            SJ_ACCEL_ASK_ON_RATING_CHANGE)!=wxYES )
					{
						return;
					}
				}

				{
					wxBusyCursor busy;
					wxSqlt sql;

					long trackId;
					SjHashIterator iterator7;
					while( m_selectedTrackIds.Iterate(iterator7, &trackId) )
					{
						sql.Query(wxString::Format(wxT("UPDATE tracks SET rating=%i WHERE id=%i;"),
						                           (int)(id-IDM_RATINGSELECTION00), (int)trackId));
					}

					if( g_tagEditorModule->GetWriteId3Tags() )
					{
						SjHashIterator iterator8;
						while( m_selectedTrackIds.Iterate(iterator7, &trackId) )
						{
							SjTrackInfo ti;
							ti.m_validFields |= SJ_TI_RATING|SJ_TI_URL;
							ti.m_url = GetUrl(trackId);
							ti.m_rating = (long)(id-IDM_RATINGSELECTION00);
							SjScannerModule* scannerModule = g_mainFrame->m_moduleSystem.FindScannerModuleByUrl(ti.m_url);
							if( scannerModule )
							{
								wxASSERT( scannerModule->IsLoaded() );
								scannerModule->SetTrackInfo(ti.m_url, ti);
							}
						}
					}
				}

				g_mainFrame->m_browser->RefreshAll();
			}
			break;

			case IDM_EDITSELECTION:
				if( g_mainFrame->IsAllAvailable() )
				{
					g_tagEditorModule->OpenTagEditor(new SjLibraryEditDlg());
				}
				break;

			case IDM_EXPLORE:
				if( m_selectedTrackIds.GetCount() == 1 )
				{
					wxSqlt sql;
					long trackId = 0;

					SjHashIterator iterator8;
					m_selectedTrackIds.Iterate(iterator8, &trackId);

					sql.Query(wxString::Format(wxT("SELECT url FROM tracks WHERE id=%lu;"), trackId));
					if( sql.Next() )
					{
						g_tools->ExploreUrl(sql.GetString(0));
					}
				}
				break;
		}

	UpdateMenuBar();
}


void SjLibraryModule::HandleMiddleMouse()
{
	switch( g_accelModule->m_middleMouseAction )
	{
		case SJ_ACCEL_MID_OPENS_TAG_EDITOR:
			HandleMenu(IDM_EDITSELECTION);
			break;
	}
}


/*******************************************************************************
 * Data that are modified by playback and are pended for performance reasons
 ******************************************************************************/


#define VALID_GAIN_MIN  0.1
#define VALID_GAIN_MAX 10.0


void SjLibraryModule::PlaybackDone(const wxString& url, unsigned long newStartingTime,
                                   double newGainDbl, long realContinuousDecodedMs)
{
	// hint: if the track was not played completely continuously, realContinuousDecodedMs
	// should be set to 0!

	wxSqlt          sql;
	unsigned long   id;
	long            oldGainLong, newGainLong = ::SjGain2Long(newGainDbl);
	unsigned long   oldStartingTime;
	unsigned long   oldTimesPlayed;
	long            oldPlaytimeMs, newPlaytimeMs;

	sql.Query(wxT("SELECT id, timesplayed, lastplayed, autovol, playtimems FROM tracks WHERE url='") + sql.QParam(url) + wxT("';"));
	if( !sql.Next() )
		return; // not in database library

	id              = sql.GetLong(0);
	oldTimesPlayed  = sql.GetLong(1);
	oldStartingTime = sql.GetLong(2);
	oldGainLong     = sql.GetLong(3);
	oldPlaytimeMs   = sql.GetLong(4);

	// old gain valid? due to a bug in older versions, it may be out of range, see http://www.silverjuke.net/forum/viewtopic.php?t=1007
	double testGain = ::SjLong2Gain(oldGainLong);
	if( testGain < VALID_GAIN_MIN || testGain > VALID_GAIN_MAX )
		oldGainLong = 0;

	// check if we should use the new or the old values
	if( oldStartingTime > newStartingTime )
	{
		newStartingTime = oldStartingTime;
	}

	if(  newGainLong <= 0
	        || (oldGainLong < newGainLong && oldGainLong > 0) )
	{
		newGainLong = oldGainLong;
	}

	newPlaytimeMs = oldPlaytimeMs;
	if( realContinuousDecodedMs > 0 )
	{
		newPlaytimeMs = realContinuousDecodedMs;
		if( (newPlaytimeMs/1000) != (oldPlaytimeMs/1000) )
		{
			if( !SjMainApp::IsInShutdown() /*the main frame may already be destructed, check this!*/ )
			{
				g_mainFrame->UpdateEnqueuedUrl(url, TRUE, newPlaytimeMs);

				wxCommandEvent pendEvt(wxEVT_COMMAND_MENU_SELECTED, IDO_BROWSER_RELOAD_VIEW);
				g_mainFrame->GetEventHandler()->AddPendingEvent(pendEvt);
			}
		}
	}

	sql.Query(wxString::Format(wxT("UPDATE tracks SET timesplayed=%lu, lastplayed=%lu, autovol=%i, playtimems=%i WHERE id=%lu;"),
	                           oldTimesPlayed+1, newStartingTime, (int)newGainLong, (int)newPlaytimeMs,
	                           id));
}


void SjLibraryModule::GetAutoVol(const wxString& url, double* trackGain, double* albumGain) const
{
	wxASSERT( wxThread::IsMain() );
	wxASSERT( trackGain );

	// get the gain of the given track
	*trackGain = -1.0F;

	wxSqlt        sql;
	unsigned long albumId = 0;
	long          lng;
	sql.Query(wxT("SELECT autovol, albumid FROM tracks WHERE url='") + sql.QParam(url) + wxT("';"));
	if( sql.Next() )
	{
		lng = sql.GetLong(0);
		if( lng > 0 )
		{
			*trackGain = ::SjLong2Gain(lng);
			if( *trackGain < VALID_GAIN_MIN || *trackGain > VALID_GAIN_MAX )
				*trackGain = -1.0F; // do not set to min/max - the gain is just boken!
		}
		albumId = sql.GetLong(1);
	}

	// if requested, get the gain of the album the track belongs to
	if( albumGain )
	{
		*albumGain = -1.0F;

		// get the min. gain for all tracks of the album the track belongs to
		wxString      firstArtistName;
		double        minGain = 1000.0F;
		sql.Query(wxString::Format(wxT("SELECT autovol, leadArtistName FROM tracks WHERE albumid=%lu;"), albumId));
		while( sql.Next() )
		{
			lng = sql.GetLong(0);
			if( lng > 0 )
			{
				double testGain = ::SjLong2Gain(lng);
				if( testGain >= VALID_GAIN_MIN && testGain <= VALID_GAIN_MAX
				        && testGain < minGain )
				{
					minGain = testGain;
				}
			}

			// check if the all artist names of the given tracks are equal
			// (we assume albums with different artists not to have the same gain)
			if( firstArtistName.IsEmpty() )
			{
				firstArtistName = SjNormaliseString(sql.GetString(1), 0);
				if( firstArtistName.IsEmpty() )
				{
					return;
				}
			}
			else if( firstArtistName != SjNormaliseString(sql.GetString(1), 0) )
			{
				return;
			}
		}

		if( minGain < 1000.0F )
		{
			*albumGain = minGain;
		}
	}
}


double SjLibraryModule::GetAutoVol(const wxString& url, bool useAlbumGainIfPossible)
{
	double trackGain, albumGain;
	if( useAlbumGainIfPossible )
	{
		GetAutoVol(url, &trackGain, &albumGain);
		return albumGain > 0.0F? albumGain : trackGain;
	}
	else
	{
		GetAutoVol(url, &trackGain, NULL /*speed up loading*/);
		return trackGain;
	}
}


long SjLibraryModule::GetAutoVolCount()
{
	if( m_autoVolCount == -1 )
	{
		wxBusyCursor busy;

		m_autoVolCount = 0;
		wxSqlt sql;
		sql.Query(wxT("SELECT COUNT(*) FROM tracks WHERE autovol!=0;"));
		if( sql.Next() )
		{
			m_autoVolCount = sql.GetLong(0);
		}
	}

	return m_autoVolCount;
}


bool SjLibraryModule::AreTracksSubsequent(const wxString& url1, const wxString& url2)
{
	wxSqlt sql;

	sql.Query(wxT("SELECT albumid FROM tracks WHERE url='") + sql.QParam(url1) + wxT("' OR url='") + sql.QParam(url2) + wxT("';"));
	if( sql.Next() )
	{
		long albumId1 = sql.GetLong(0);
		if( sql.Next() )
		{
			long albumId2 = sql.GetLong(0);
			if( albumId1 == albumId2 )
			{
				sql.Query(wxString::Format(wxT("SELECT url FROM tracks WHERE albumid=%lu ORDER BY disknr, tracknr, trackname, id;"), albumId1));
				while( sql.Next() )
				{
					if( sql.GetString(0) == url1 )
					{
						if( sql.Next() )
						{
							if( sql.GetString(0) == url2 )
							{
								return TRUE;
							}
						}

						break;
					}
				}
			}
		}
	}

	return FALSE;
}


/*******************************************************************************
 * Get more tracks from a known url
 ******************************************************************************/


wxArrayString SjLibraryModule::GetMoreFrom(const wxString& url, int targetId, SjQueue* alreadyEnqueued)
{
	wxArrayString ret;

	wxSqlt sql;
	if( targetId == IDT_MORE_FROM_CURR_ALBUM )
	{
		sql.Query(wxT("SELECT albumid FROM tracks WHERE url='") + sql.QParam(url) + wxT("';"));
		if( sql.Next() )
		{
			long albumId = sql.GetLong(0);
			sql.Query(wxString::Format(wxT("SELECT url FROM tracks WHERE albumid=%lu;"), albumId));
			// we go through the query below
		}
	}
	else if( targetId == IDT_MORE_FROM_CURR_ARTIST )
	{
		sql.Query(wxT("SELECT leadartistname FROM tracks WHERE url='") + sql.QParam(url) + wxT("';"));
		if( sql.Next() )
		{
			wxString leadArtistName = sql.GetString(0);
			sql.Query(wxT("SELECT url FROM tracks WHERE leadartistname='") + sql.QParam(leadArtistName) + wxT("';"));
			// we go through the query below
		}
	}

	// go through the query
	while( sql.Next() )
	{
		wxString url = sql.GetString(0);
		if( alreadyEnqueued == NULL
		        || alreadyEnqueued->GetClosestPosByUrl(url) == wxNOT_FOUND )
		{
			ret.Add(url);
		}
	}

	return ret;
}


/*******************************************************************************
 * SjAlbumCoverRow Class
 ******************************************************************************/


void SjAlbumCoverRow::CreateContextMenu(SjMenu& m)
{
	SjLibraryModule* library = GetLibrary();

	if( PlayOnDblClick() )
	{
		library->CreateMenu(&m, &m);
		library->UpdateMenu(&m, &m);
		if( m.GetMenuItemCount() )
		{
			m.AppendSeparator();
		}
	}

	// get current art IDs in use
	wxSqlt sql;
	long artidauto = 0;
	sql.Query(wxString::Format(wxT("SELECT artidauto FROM albums WHERE id=%lu;"), m_albumId));
	if( sql.Next() )
	{
		artidauto = sql.GetLong(0);
	}

	// create art menu
	wxArrayLong   artIds;
	wxArrayString artUrls;
	g_mainFrame->m_libraryModule->GetPossibleAlbumArts(m_albumId, artIds, &artUrls, TRUE/*addAutoCover*/);

	if( g_artEditorModule )
	{
		wxString artUrlAuto;
		sql.Query(wxString::Format(wxT("SELECT url FROM arts WHERE id=%lu;"), artidauto));
		if( sql.Next() )
		{
			artUrlAuto = sql.GetString(0);
		}

		g_mainFrame->SetPasteCoord(g_mainFrame->ScreenToClient(::wxGetMousePosition()));
		g_artEditorModule->CreateArtMenu(m, artUrls,
		                                 m_textm, artUrlAuto);
	}
}


wxString SjAlbumCoverRow::GetToolTip(long& flags)
{
	if( PlayOnDblClick() )
	{
		return _("Click to select all tracks of this album, double-click to play them");
	}
	else
	{
		return _("Double-click to enlarge the cover");
	}
}


void SjAlbumCoverRow::Select(bool select)
{
	if( PlayOnDblClick() )
	{
		SjLibraryModule* library = GetLibrary();

		library->SelectByQuery(select,
		                       wxString::Format(wxT("SELECT id FROM tracks WHERE albumid=%lu;"), m_albumId));
	}
}


void SjAlbumCoverRow::OnContextMenu(int id)
{
	SjLibraryModule* library = GetLibrary();
	if( g_artEditorModule )
	{
		wxArrayLong artIds;
		wxArrayString artUrls;
		g_mainFrame->m_libraryModule->GetPossibleAlbumArts(m_albumId, artIds, &artUrls, true/*addAutoCover*/);

		if( !g_artEditorModule->OnArtMenu(g_mainFrame, artUrls, m_textm, m_albumId, id, NULL, false /*IsPlaylistCover*/) )
		{
			if( PlayOnDblClick() )
			{
				library->HandleMenu(id);
			}
		}
	}
}


void SjAlbumCoverRow::OnDoubleClick(bool modifiersPressed)
{
	if( PlayOnDblClick() )
	{
		GetLibrary()->AskToAddSelection();
	}
	else
	{
		OnContextMenu(IDM_OPENARTEDITOR);
	}
}


bool SjAlbumCoverRow::OnMiddleClick(bool modifiersPressed)
{
	if( g_accelModule->m_middleMouseAction == SJ_ACCEL_MID_OPENS_TAG_EDITOR )
	{
		OnContextMenu(IDM_OPENARTEDITOR);
	}
	return TRUE;
}


bool SjAlbumCoverRow::OnDropData(SjDataObject* data)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	return library->AddArt__(data, m_albumId, TRUE/*do ask*/);
}


/*******************************************************************************
 * SjAlbumAlbumRow Class
 ******************************************************************************/


void SjAlbumAlbumRow::CreateContextMenu(SjMenu& m)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;
	library->CreateMenu(&m, &m);
	library->UpdateMenu(&m, &m);
}


wxString SjAlbumAlbumRow::GetToolTip(long& flags)
{
	return _("Click to select all tracks of this album, double-click to play them");
}


void SjAlbumAlbumRow::OnContextMenu(int id)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	library->HandleMenu(id);
}


void SjAlbumAlbumRow::OnDoubleClick(bool select)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	library->AskToAddSelection();
}


bool SjAlbumAlbumRow::OnMiddleClick(bool modifiersPressed)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	library->HandleMiddleMouse();

	return TRUE;
}


bool SjAlbumAlbumRow::OnDropData(SjDataObject* data)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	return library->AddArt__(data, m_albumId, TRUE/*do ask*/);
}


void SjAlbumAlbumRow::Select(bool select)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	library->SelectByQuery(select,
	                       wxString::Format(wxT("SELECT id FROM tracks WHERE albumid=%lu;"), m_albumId));
}


/*******************************************************************************
 * SjAlbumArtistRow Class
 ******************************************************************************/


void SjAlbumArtistRow::CreateContextMenu(SjMenu& m)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;
	library->CreateMenu(&m, &m);
	library->UpdateMenu(&m, &m);
}


wxString SjAlbumArtistRow::GetToolTip(long& flags)
{
	return _("Click to select all tracks of this artists, double-click to play them");
}


void SjAlbumArtistRow::OnContextMenu(int id)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	library->HandleMenu(id);
}


void SjAlbumArtistRow::OnDoubleClick(bool select)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	library->AskToAddSelection();
}


bool SjAlbumArtistRow::OnMiddleClick(bool modifiersPressed)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	library->HandleMiddleMouse();

	return TRUE;
}


bool SjAlbumArtistRow::OnDropData(SjDataObject* data)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	return library->AddArt__(data, m_albumId, TRUE/*do ask*/);
}


void SjAlbumArtistRow::Select(bool select)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;
	wxString albumArtistName;

	{
		wxSqlt sql;
		sql.Query(wxString::Format(wxT("SELECT leadartistname FROM albums WHERE id=%lu;"), m_albumId));
		if( sql.Next() )
		{
			albumArtistName = sql.GetString(0);
		}
	}


	library->SelectByQuery(select,
	                       wxT("SELECT id FROM tracks WHERE leadartistname='") + wxSqlt::QParam(albumArtistName) + wxT("';"));
}


/*******************************************************************************
 * SjAlbumDiskRow Class
 ******************************************************************************/


void SjAlbumDiskRow::CreateContextMenu(SjMenu& m)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;
	library->CreateMenu(&m, &m);
	library->UpdateMenu(&m, &m);
}


wxString SjAlbumDiskRow::GetToolTip(long& flags)
{
	return _("Click to select all tracks of this disk, double-click to play them");
}


void SjAlbumDiskRow::OnContextMenu(int id)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	library->HandleMenu(id);
}


void SjAlbumDiskRow::OnDoubleClick(bool select)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	library->AskToAddSelection();
}


bool SjAlbumDiskRow::OnMiddleClick(bool modifiersPressed)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	library->HandleMiddleMouse();

	return TRUE;
}


bool SjAlbumDiskRow::OnDropData(SjDataObject* data)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	return library->AddArt__(data, m_albumId, TRUE/*do ask*/);
}


void SjAlbumDiskRow::Select(bool select)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	library->SelectByQuery(select,
	                       wxString::Format(wxT("SELECT id FROM tracks WHERE albumid=%lu AND disknr=%lu;"), m_albumId, m_diskNr));
}


/*******************************************************************************
 * SjAlbumTrackRow Class and SjLibraryModule Selection handling
 ******************************************************************************/


void SjAlbumTrackRow::CreateContextMenu(SjMenu& m)
{
	m_libraryModule->CreateMenu(&m, &m);
	m_libraryModule->UpdateMenu(&m, &m);
}


wxString SjAlbumTrackRow::GetToolTip(long& flags)
{
	wxString        ret;

	wxString        trackName = wxT("?");
	wxString        url;
	int             rating = 0;
	long            lastplayed = 0;
	long            timesplayed = 0;

	{
		wxSqlt sql;
		sql.Query(wxString::Format(wxT("SELECT trackname, rating, lastplayed, timesplayed, url FROM tracks WHERE id=%lu;"), m_trackId));
		if( sql.Next() )
		{
			trackName   = sql.GetString(0);
			rating      = sql.GetLong(1);
			lastplayed  = sql.GetLong(2);
			timesplayed = sql.GetLong(3);
			url         = sql.GetString(4);
		}
	}

	if( rating )
	{
		wxString chars;
		while(rating--) { chars += SJ_RATING_CHARS_ELSEWHERE; }

		if( !ret.IsEmpty() ) ret += wxT("\n");
		ret += wxString::Format(_("Rating: %s"), chars.c_str());
	}

	if( lastplayed )
	{
		if( !ret.IsEmpty() ) ret += wxT("\n");
		ret += _("Last played");
		ret += wxT(": ");
		ret += SjTools::FormatDate(lastplayed, SJ_FORMAT_ADDTIME);
	}

	if( timesplayed )
	{
		if( !ret.IsEmpty() ) ret += wxT("\n");
		ret += _("Play count");
		ret += wxT(": ");
		ret += SjTools::FormatNumber(timesplayed);
	}

	wxArrayLong allQueuePositions;
	if( g_mainFrame->GetAllQueuePosByUrl(url, allQueuePositions) > 0 )
	{
		if( !ret.IsEmpty() ) ret += wxT("\n");
		ret += _("Queue position");
		ret += wxT(": ");
		ret += SjTools::FormatNumbers(allQueuePositions, 1 /*we want to display values starting at 1*/);
	}

	if( ret.IsEmpty() )
	{
		ret = wxString::Format(_("Double-click to enqueue \"%s\""), trackName.c_str());
	}
	else
	{
		flags = 400; // width in the lower bits
	}

	return ret;
}


void SjAlbumTrackRow::OnContextMenu(int id)
{
	m_libraryModule->HandleMenu(id);
}


void SjAlbumTrackRow::OnDoubleClick(bool modifiersPressed)
{
	// former versions of Silverjuke had unqueued songs that were
	// already enqueued.  However, this is a little bit confusing some
	// time - what if the user wants to hear the song twice?
	// what in kiosk mode?
	// Use the display to unqueue songs.

	// enqueue!
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	library->AskToAddSelection();
}


bool SjAlbumTrackRow::OnMiddleClick(bool modifiersPressed)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	library->HandleMiddleMouse();

	return TRUE;
}


bool SjAlbumTrackRow::OnDropData(SjDataObject* data)
{
	SjLibraryModule* library = (SjLibraryModule*)m_col->m_module;

	wxSqlt sql;
	sql.Query(wxString::Format(wxT("SELECT albumid FROM tracks WHERE id=%lu;"), m_trackId));
	if( sql.Next() )
	{
		return library->AddArt__(data, sql.GetLong(0), TRUE/*do ask*/);
	}
	else
	{
		return FALSE;
	}
}


void SjAlbumTrackRow::Select(bool select)
{
	if( select )
	{
		m_libraryModule->m_selectedTrackIds.Insert(m_trackId, 1);
	}
	else
	{
		m_libraryModule->m_selectedTrackIds.Remove(m_trackId);
	}

	m_libraryModule->UpdateMenuBar();
}


void SjLibraryModule::SelectAllRows(bool select)
{
	m_selectedTrackIds.Clear();

	if( select )
	{
		wxBusyCursor busy;

		if( HasSearch() )
		{
			long trackId;
			SjHashIterator iterator9;
			while( m_searchTracksHash.Iterate(iterator9, &trackId) )
			{
				m_selectedTrackIds.Insert(trackId, 1);
			}
		}
		else
		{
			wxSqlt sql;
			sql.Query(wxT("SELECT id FROM tracks;"));
			while( sql.Next() )
			{
				m_selectedTrackIds.Insert(sql.GetLong(0), 1);
			}
		}
	}

	UpdateMenuBar();
}


void SjLibraryModule::SelectByQuery(bool select, const wxString& formattedQuery)
{
	if( !g_mainFrame->IsOpAvailable(SJ_OP_MULTI_ENQUEUE) ) return;

	wxSqlt  sql;
	long    trackId;

	sql.Query(formattedQuery);
	while( sql.Next() )
	{
		trackId = sql.GetLong(0);
		if( IsInSearch(trackId) )
		{
			if( select )
			{
				m_selectedTrackIds.Insert(trackId, 1);
			}
			else
			{
				m_selectedTrackIds.Remove(trackId);
			}
		}
	}

	g_mainFrame->m_browser->RefreshSelection();

	UpdateMenuBar();
}


void SjLibraryModule::GetOrderedUrlsFromIDs(SjLLHash& ids, wxArrayString& urls)
{
	long trackCount = ids.GetCount();
	if( trackCount >= 1 )
	{
		// get all track IDs as a string for sql command IN()
		if( trackCount>250) { ::wxBeginBusyCursor(); }

		wxSqlt sql;
		sql.Query(wxString::Format(wxT("SELECT tracks.url FROM tracks, albums WHERE tracks.id IN(%s) AND albums.id=albumid ")
		                           wxT("ORDER BY albums.albumindex, disknr, tracknr, tracks.id;"),
		                           ids.GetKeysAsString().c_str()));
		while( sql.Next() )
		{
			urls.Add(sql.GetString(0));
		}

		if( trackCount>250) { ::wxEndBusyCursor(); }
	}
}


void SjLibraryModule::GetSelectedUrls(wxArrayString& urls)
{
	GetOrderedUrlsFromIDs(m_selectedTrackIds, urls);
}


void SjLibraryModule::AskToAddSelection()
{
	long trackCount = m_selectedTrackIds.GetCount();

	if( trackCount > 1 )
	{
		// wxWindowDisabler disabler(this); -- done by YesNo()
		if( g_accelModule->YesNo(
		            wxString::Format(
		              // TRANSLATORS: %i will be replaced by the number of tracks
		              wxPLURAL("Do you want to enqueue %i selected track?", "Do you want to enqueue %i selected tracks?", trackCount),
		              (int)trackCount),
		            wxString::Format(
		              // TRANSLATORS: %i will be replaced by the number of tracks
		              wxPLURAL("Enqueue %i track", "Enqueue %i tracks", trackCount),
		              (int)trackCount),
		            g_mainFrame,
		            SJ_ACCEL_ASK_ON_MULTI_ENQUEUE) != wxYES )
		{
			return;
		}
	}

	int action = IDT_ENQUEUE_LAST;
	if(  g_mainFrame->IsOpAvailable(SJ_OP_EDIT_QUEUE)
	        && (g_accelModule->m_flags&SJ_ACCEL_PLAY_NOW_ON_DBL_CLICK) )
	{
		action = IDT_ENQUEUE_NOW;
	}

	wxCommandEvent fwdEvent(wxEVT_COMMAND_MENU_SELECTED, action);
	g_mainFrame->GetEventHandler()->AddPendingEvent(fwdEvent);
}


/*******************************************************************************
 * ListView stuff
 ******************************************************************************/


struct SjId
{
	long        id;
	long        special;
};

class SjLibraryListView : public SjListView
{
public:
	SjLibraryListView   (long order, bool desc, long minAlbumRows, SjLibraryModule* m);
	~SjLibraryListView  ();
	void            ChangeOrder         (long orderField, bool orderDesc);
	long            GetTrackCount       () { return m_idsCount; }
	void            GetTrack            (long offset, SjTrackInfo&, long& albumId, long& special);
	long            GetTrackSpecial     (long offset);
	bool            IsTrackSelected     (long offset) { return m_module->m_selectedTrackIds.Lookup(m_ids[offset].id)!=0; }
	void            SelectTrack         (long offset, bool select) { m_module->m_selectedTrackIds.InsertOrRemove(m_ids[offset].id, select? 1L : 0L); }
	long            Url2TrackOffset     (const wxString& url); // return -1 if not in view

	wxString        GetUpText           ();

	void            CreateContextMenu   (long offset, SjMenu&);
	void            OnContextMenu       (long offset, int id);
	void            OnDoubleClick       (long offset, bool modifiersPressed);
	void            OnMiddleClick       (long offset, bool modifiersPressed);

	SjCol*          GetCol              (long albumId);

private:
	SjLibraryModule* m_module;
	SjId*           m_ids;
	long            m_idsCount;
};


SjListView* SjLibraryModule::CreateListView(long order, bool desc, long minAlbumRows)
{
	return new SjLibraryListView(order, desc, minAlbumRows, this);
}


SjLibraryListView::SjLibraryListView(long orderField, bool orderDesc, long minAlbumRows, SjLibraryModule* m)
	: SjListView(orderField, orderDesc, minAlbumRows)
{
	m_module = m;
	m_ids = NULL;
	m_currOrderField = -1;
	ChangeOrder(orderField, orderDesc);
}


void SjLibraryListView::ChangeOrder(long orderField, bool orderDesc)
{
	// can we re-use an existing ordering?
	if( m_ids
	        && m_currOrderField==orderField
	        && m_currOrderDesc ==orderDesc
	        && m_currOrderField!=SJ_TI_Y_QUEUEPOS ) // the queue position is always subject to change
	{
		return;
	}

	#ifdef __WXDEBUG__
		unsigned long ms = SjTools::GetMsTicks();
	#endif

	// get ordering -- we do not add secondary sort criterias as this will slow down the whole stuff
	// as there is no - combined - index available
	wxSqlt sql;
	wxString order;
	{
		switch( orderField )
		{
			case SJ_TI_LEADARTISTNAME:
			case SJ_TI_ORGARTISTNAME:
			case SJ_TI_COMPOSERNAME:
				order = wxString::Format(wxT("sortable(%s,15) DIR, albumName, trackNr"), SjTrackInfo::GetFieldDbName(orderField).c_str());
				break;                          //       ^^^ 15 = SJ_NUM_SORTABLE|SJ_NUM_TO_END|SJ_EMPTY_TO_END|SJ_OMIT_ARTIST

			case SJ_TI_ALBUMNAME:
				order = wxString::Format(wxT("sortable(%s,23) DIR, albumId, trackNr"), SjTrackInfo::GetFieldDbName(orderField).c_str());
				break;                          //       ^^^ 23 = SJ_NUM_SORTABLE|SJ_NUM_TO_END|SJ_EMPTY_TO_END|SJ_OMIT_ALBUM

			case SJ_TI_TRACKNAME:
			case SJ_TI_GENRENAME:
			case SJ_TI_GROUPNAME:
			case SJ_TI_COMMENT:
				order = wxString::Format(wxT("sortable(%s,7) DIR, albumName, trackNr"), SjTrackInfo::GetFieldDbName(orderField).c_str());
				break;                          //       ^^^ 7 = SJ_NUM_SORTABLE|SJ_NUM_TO_END|SJ_EMPTY_TO_END

			case SJ_TI_URL:
				order = wxT("url DIR, albumName");
				break;

			case SJ_TI_X_TIMEADDED:
			case SJ_TI_X_TIMEMODIFIED:
			case SJ_TI_X_LASTPLAYED:
			case SJ_TI_YEAR:
			case SJ_TI_X_TIMESPLAYED:
			case SJ_TI_PLAYTIMEMS:
			case SJ_TI_RATING:
				// dates, do not use nulltolast() as this uses the full 32 bit (I believe?!)
				// or desc. ordering is the default
				order= wxString::Format(wxT("%s DIR, albumName"), SjTrackInfo::GetFieldDbName(orderField).c_str());
				break;

			case SJ_TI_BEATSPERMINUTE:
			case SJ_TI_TRACKNR:
			case SJ_TI_DISKNR:
			case SJ_TI_TRACKCOUNT:
			case SJ_TI_DISKCOUNT:
			case SJ_TI_X_DATABYTES:
			case SJ_TI_X_BITRATE:
			case SJ_TI_X_SAMPLERATE:
			case SJ_TI_X_CHANNELS:
			case SJ_TI_X_AUTOVOL:
				// numbers where <=0 is the last
				order= wxString::Format(wxT("nulltoend(%s) DIR, albumName"), SjTrackInfo::GetFieldDbName(orderField).c_str());
				break;

			case SJ_TI_Y_FILETYPE:
				order = wxT("filetype(url) DIR, albumName");
				break;

			case SJ_TI_Y_QUEUEPOS:
				order = wxT("queuepos(url) DIR, albumName");
				break;

			default:
				wxASSERT( 0 );
				order = wxT("id DIR");
				break;
		}

		wxASSERT( !order.IsEmpty() );
	}

	// get order direction
	order.Replace(wxT("DIR"), orderDesc? wxT("DESC") : wxT(""));

	// get base query
	m_idsCount = m_module->m_searchTracksHash.GetCount();
	if( m_idsCount )
	{
		sql.Query(wxString::Format(wxT("SELECT id, albumId FROM tracks WHERE id IN (%s) ORDER BY %s"),  m_module->m_searchTracksHash.GetKeysAsString().c_str(), order.c_str()));
	}
	else
	{
		m_idsCount = m_module->GetUnmaskedTrackCount();
		sql.Query(wxString::Format(wxT("SELECT id, albumId FROM tracks ORDER BY %s"), order.c_str()));
	}

	// free preciously allocated IDs
	if( m_ids )
		free(m_ids);

	// get all IDs
	wxASSERT( m_minAlbumRows >= 0 );

	long i = 0, j, trackId, albumId, lastAlbumId = -1, lastTrackId = -1, albumRows = 0;
	bool dark = true;
	long maxPossibleIds = m_idsCount * (m_minAlbumRows + 1/*for the gap*/);
	SjId* ids = (SjId*)malloc(maxPossibleIds * sizeof(SjId));
	if( ids  )
	{
		while( sql.Next() && i < maxPossibleIds )
		{
			trackId = sql.GetLong(0);
			albumId = sql.GetLong(1);

			if( albumId != lastAlbumId )
			{
				if( i )
				{
					// add a gap
					wxASSERT( lastTrackId != - 1);
					if( m_minAlbumRows )
					{
						for( j = albumRows+1; j < m_minAlbumRows; j++ )
						{
							ids[i].special = SJ_LISTVIEW_SPECIAL_GAP;
							ids[i].id = lastTrackId;
							i++;
						}

						ids[i].special = SJ_LISTVIEW_SPECIAL_GAP | SJ_LISTVIEW_SPECIAL_LAST_GAP;
						ids[i].id = lastTrackId;
						i++;

						dark = true;
					}

					albumRows = 0;
				}

				ids[i].special = SJ_LISTVIEW_SPECIAL_FIRST_TRACK | (dark? SJ_LISTVIEW_SPECIAL_DARK : 0);
			}
			else
			{
				ids[i].special = dark? SJ_LISTVIEW_SPECIAL_DARK : 0;
			}

			ids[i].id = trackId;
			i++;

			dark = !dark;
			lastAlbumId = albumId;
			lastTrackId = trackId;
			albumRows ++;

			wxASSERT( i <= maxPossibleIds );
		}
	}

	m_ids = ids;
	m_idsCount = i;
	m_currOrderField = orderField;
	m_currOrderDesc = orderDesc;

	#ifdef __WXDEBUG__
		wxLogDebug(wxT("%lu ms needed to create the view"), (SjTools::GetMsTicks()-ms));
	#endif
}


SjLibraryListView::~SjLibraryListView()
{
	free(m_ids);
}


long SjLibraryListView::GetTrackSpecial(long offset)
{
	return m_ids[offset].special;
}


void SjLibraryListView::GetTrack(long offset, SjTrackInfo& trackInfo, long& retAlbumId, long& retSpecial)
{
	wxSqlt sql;
	sql.Query(wxString::Format(wxT("SELECT id, trackName, ")
	                           wxT("leadArtistName, orgArtistName, composerName, ")
	                           wxT("albumName, comment, ")
	                           wxT("trackNr, trackCount, diskNr, diskCount, ")
	                           wxT("genreName, groupName, ")
	                           wxT("year, beatsperminute, ")
	                           wxT("rating, playtimeMs, autovol, ")
	                           wxT("bitrate, samplerate, channels, databytes, ")
	                           wxT("lastplayed, timesplayed, timeadded, timemodified, url, albumid ")
	                           wxT("FROM tracks WHERE id=%lu;"), m_ids[offset].id));
	if( sql.Next() )
	{
		trackInfo.m_id              = sql.GetLong  (0);
		trackInfo.m_trackName       = sql.GetString(1);
		trackInfo.m_leadArtistName  = sql.GetString(2);
		trackInfo.m_orgArtistName   = sql.GetString(3);
		trackInfo.m_composerName    = sql.GetString(4);
		trackInfo.m_albumName       = sql.GetString(5);
		trackInfo.m_comment         = sql.GetString(6);
		trackInfo.m_trackNr         = sql.GetLong  (7);
		trackInfo.m_trackCount      = sql.GetLong  (8);
		trackInfo.m_diskNr          = sql.GetLong  (9);
		trackInfo.m_diskCount       = sql.GetLong  (10);
		trackInfo.m_genreName       = sql.GetString(11);
		trackInfo.m_groupName       = sql.GetString(12);
		trackInfo.m_year            = sql.GetLong  (13);
		trackInfo.m_beatsPerMinute  = sql.GetLong  (14);
		trackInfo.m_rating          = sql.GetLong  (15);
		trackInfo.m_playtimeMs      = sql.GetLong  (16);
		trackInfo.m_autoVol         = sql.GetLong  (17);
		trackInfo.m_bitrate         = sql.GetLong  (18);
		trackInfo.m_samplerate      = sql.GetLong  (19);
		trackInfo.m_channels        = sql.GetLong  (20);
		trackInfo.m_dataBytes       = sql.GetLong  (21);
		trackInfo.m_lastPlayed      = sql.GetLong  (22);
		trackInfo.m_timesPlayed     = sql.GetLong  (23);
		trackInfo.m_timeAdded       = sql.GetLong  (24);
		trackInfo.m_timeModified    = sql.GetLong  (25);
		trackInfo.m_url             = sql.GetString(26);
		retAlbumId                  = sql.GetLong  (27);
		retSpecial                  = m_ids[offset].special;

		m_module->HiliteSearchWords(trackInfo.m_trackName);
		m_module->HiliteSearchWords(trackInfo.m_leadArtistName);
		m_module->HiliteSearchWords(trackInfo.m_albumName);

		if( m_module->m_flags&SJ_LIB_OMITTOEND )
		{
			trackInfo.m_leadArtistName  =   m_module->m_omitArtist.Apply(trackInfo.m_leadArtistName);
			trackInfo.m_orgArtistName   =   m_module->m_omitArtist.Apply(trackInfo.m_orgArtistName);
			trackInfo.m_composerName    =   m_module->m_omitArtist.Apply(trackInfo.m_composerName);
			trackInfo.m_albumName       =   m_module->m_omitAlbum .Apply(trackInfo.m_albumName);
		}
	}

	retSpecial = m_ids[offset].special;
}


void SjLibraryListView::CreateContextMenu(long offset, SjMenu& m)
{
	m_module->CreateMenu(&m, &m);
	m_module->UpdateMenu(&m, &m);
}


void SjLibraryListView::OnContextMenu(long offset, int id)
{
	m_module->HandleMenu(id);
}


void SjLibraryListView::OnDoubleClick(long offset, bool modifiersPressed)
{
	m_module->AskToAddSelection();
}


void SjLibraryListView::OnMiddleClick(long offset, bool modifiersPressed)
{
	m_module->HandleMiddleMouse();
}


SjCol* SjLibraryListView::GetCol(long albumId)
{
	wxSqlt sql;
	sql.Query(wxString::Format(wxT("SELECT albumindex FROM albums WHERE id=%lu;"), albumId));
	if( sql.Next() )
	{
		long albumIndex = sql.GetLong(0);
		return m_module->GetUnmaskedCol(albumIndex);
	}
	return NULL;
}


long SjLibraryListView::Url2TrackOffset(const wxString& url)
{
	long        wantedId = -1;
	long        i, iCount = m_idsCount;
	const SjId* ids = m_ids;

	{
		wxSqlt sql;
		sql.Query(wxT("SELECT id FROM tracks WHERE url='") + sql.QParam(url) + wxT("';"));
		if( sql.Next() )
		{
			wantedId = sql.GetLong(0);
		}
	}

	if( wantedId != -1 )
	{
		for( i = 0; i < iCount; i++ )
		{
			if( ids[i].id == wantedId )
			{
				if( !(ids[i].special&SJ_LISTVIEW_SPECIAL_GAP) )
					return i;
			}
		}
	}

	return -1;
}


wxString SjLibraryListView::GetUpText()
{
	return m_module->GetUpText();
}


int SjListView::GetAzFromOffset(long offset)
{
	SjTrackInfo ti;
	long dummyAlbumId, dummySpecial;
	GetTrack(offset, ti, dummyAlbumId, dummySpecial);
	wxString str = SjNormaliseString(ti.GetFormattedValue(m_currOrderField), 0);
	int azCurr = str[0];
	if( azCurr < 'a' || azCurr > 'z' )
		azCurr = 'z'+1;
	return azCurr;
}


long SjListView::Az2TrackOffset(int azWanted)
{
	// this belongs to the generic SjListView implementation,
	// however, we've put it here ...

	long p_left = 0,
	     p_right = GetTrackCount()-1,
	     p_test = -1;
	int azCurr;
	while( p_right > p_left )
	{
		// calcualte the test offset
		p_test = p_left + (p_right-p_left)/2;

		// get the AZ-scope for the test offset
		azCurr = GetAzFromOffset(p_test);

		// AZ found? -- Otherwise correct the left and right borders
		if( azCurr == azWanted )
		{
			break;
		}
		else if( m_currOrderDesc )
		{
			if( azCurr > azWanted )
				p_left = p_test+1;
			else
				p_right = p_test-1;
		}
		else
		{
			if( azCurr < azWanted )
				p_left = p_test+1;
			else
				p_right = p_test-1;
		}
	}

	// seek backward until the azCurr is really the first offset;
	// we ignore the ASC/DESC setting here by design
	long diff;
	for( int fastFwd = 2; fastFwd >= 0; fastFwd -- )
	{
		if( fastFwd == 2 ) { diff = 256; }
		else if( fastFwd == 1 ) { diff = 16;  }
		else                    { diff = 1; }

		long p_beforeAfter = p_test - diff;
		while( p_beforeAfter >= 0 )
		{
			azCurr = GetAzFromOffset(p_beforeAfter);
			if( azCurr != azWanted )
				break; // out of while

			p_test = p_beforeAfter;
			p_beforeAfter -= diff;
		}
	}

	return p_test;
}
