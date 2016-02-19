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
 * File:    tageditor.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke main application, "edit tags" dialog
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/browser.h>
#include <sjtools/msgbox.h>
#include <sjmodules/tageditor/tageditor.h>
#include <sjmodules/tageditor/tageditorplugin.h>
#include <sjmodules/tageditor/tageditorrename.h>
#include <sjmodules/tageditor/tageditorsplit.h>
#include <sjmodules/tageditor/tageditorreplace.h>
#include <sjmodules/tageditor/tageditorfreedb.h>
#include <sjmodules/help/htmlwindow.h>
#include <tagger/tg_a_tagger_frontend.h>
#include <tagger/tg_bytefile.h>
#include <wx/notebook.h>


/*******************************************************************************
 * SjTagEditorDlg - Constructor / Destructor
 ******************************************************************************/


#define HTML_WINDOW_W 450

#define IDC_SHOWMOREINFO        (IDM_FIRSTPRIVATE+3)
#define IDC_WRITEID3TAGS        (IDM_FIRSTPRIVATE+6)

#define IDC_TRACKNAME           (IDM_FIRSTPRIVATE+110)
#define IDC_TRACKCHECK          (IDM_FIRSTPRIVATE+111) /* should be IDC_TRACKNAME + 1 */
#define IDC_LEADARTISTNAME      (IDM_FIRSTPRIVATE+112)
#define IDC_LEADARTISTCHECK     (IDM_FIRSTPRIVATE+113) /* should be IDC_LEADARTISTNAME + 1 */
#define IDC_ORGARTISTNAME       (IDM_FIRSTPRIVATE+114)
#define IDC_ORGARTISTCHECK      (IDM_FIRSTPRIVATE+115) /* should be IDC_ORGARTISTNAME + 1 */
#define IDC_COMPOSERNAME        (IDM_FIRSTPRIVATE+116)
#define IDC_COMPOSERCHECK       (IDM_FIRSTPRIVATE+117) /* should be IDC_COMPOSERNAME + 1 */
#define IDC_ALBUMNAME           (IDM_FIRSTPRIVATE+118)
#define IDC_ALBUMCHECK          (IDM_FIRSTPRIVATE+119) /* should be IDC_ALBUMNAME + 1 */
#define IDC_COMMENT             (IDM_FIRSTPRIVATE+120)
#define IDC_COMMENTCHECK        (IDM_FIRSTPRIVATE+121) /* should be IDC_COMMENT + 1 */
#define IDC_TRACKNR             (IDM_FIRSTPRIVATE+122)
#define IDC_TRACKNRCHECK        (IDM_FIRSTPRIVATE+123) /* should be IDC_TRACKNR + 1 */
#define IDC_TRACKCOUNT          (IDM_FIRSTPRIVATE+124)
#define IDC_TRACKCOUNTCHECK     (IDM_FIRSTPRIVATE+125) /* should be IDC_TRACKCOUNT + 1 */
#define IDC_DISKNR              (IDM_FIRSTPRIVATE+126)
#define IDC_DISKNRCHECK         (IDM_FIRSTPRIVATE+127) /* should be IDC_DISKNR + 1 */
#define IDC_DISKCOUNT           (IDM_FIRSTPRIVATE+128)
#define IDC_DISKCOUNTCHECK      (IDM_FIRSTPRIVATE+129) /* should be IDC_DISKCOUNT + 1 */
#define IDC_GENRENAME           (IDM_FIRSTPRIVATE+130)
#define IDC_GENRECHECK          (IDM_FIRSTPRIVATE+131) /* should be IDC_GENRENAME + 1 */
#define IDC_GROUPNAME           (IDM_FIRSTPRIVATE+132)
#define IDC_GROUPCHECK          (IDM_FIRSTPRIVATE+133) /* should be IDC_GROUPNAME + 1 */
#define IDC_YEAR                (IDM_FIRSTPRIVATE+134)
#define IDC_YEARCHECK           (IDM_FIRSTPRIVATE+135) /* should be IDC_YEAR + 1 */
#define IDC_BPM                 (IDM_FIRSTPRIVATE+136)
#define IDC_BPMCHECK            (IDM_FIRSTPRIVATE+137) /* should be IDC_BPM + 1 */
#define IDC_RATING              (IDM_FIRSTPRIVATE+138)
#define IDC_RATINGCHECK         (IDM_FIRSTPRIVATE+139) /* should be IDC_RATING + 1 */


BEGIN_EVENT_TABLE(SjTagEditorDlg, SjDialog)
	EVT_BUTTON                  (IDC_BUTTONBARMENU,     SjTagEditorDlg::OnButtonBarMenu     )
	EVT_MENU_RANGE              (IDC_PLUGIN_FIRST,
	                             IDC_PLUGIN_LAST,       SjTagEditorDlg::OnPlugin            )
	EVT_MENU                    (IDC_WRITEID3TAGS,      SjTagEditorDlg::OnWriteId3Tags      )
	EVT_MENU                    (IDC_SHOWMOREINFO,      SjTagEditorDlg::OnShowMoreInfo      )
	EVT_MENU                    (IDM_EDITSELECTION,     SjTagEditorDlg::OnTogglePage        )
	EVT_TEXT                    (IDC_TRACKNAME,         SjTagEditorDlg::OnDataInput         )
	EVT_TEXT                    (IDC_LEADARTISTNAME,    SjTagEditorDlg::OnDataInput         )
	EVT_TEXT                    (IDC_ORGARTISTNAME,     SjTagEditorDlg::OnDataInput         )
	EVT_TEXT                    (IDC_COMPOSERNAME,      SjTagEditorDlg::OnDataInput         )
	EVT_TEXT                    (IDC_ALBUMNAME,         SjTagEditorDlg::OnDataInput         )
	EVT_TEXT                    (IDC_COMMENT,           SjTagEditorDlg::OnDataInput         )
	EVT_TEXT                    (IDC_TRACKNR,           SjTagEditorDlg::OnDataInput         )
	EVT_TEXT                    (IDC_TRACKCOUNT,        SjTagEditorDlg::OnDataInput         )
	EVT_TEXT                    (IDC_DISKNR,            SjTagEditorDlg::OnDataInput         )
	EVT_TEXT                    (IDC_DISKCOUNT,         SjTagEditorDlg::OnDataInput         )
	EVT_TEXT                    (IDC_GENRENAME,         SjTagEditorDlg::OnDataInput         )
	EVT_TEXT                    (IDC_GROUPNAME,         SjTagEditorDlg::OnDataInput         )
	EVT_TEXT                    (IDC_YEAR,              SjTagEditorDlg::OnDataInput         )
	EVT_TEXT                    (IDC_BPM,               SjTagEditorDlg::OnDataInput         )
	EVT_CHOICE                  (IDC_RATING,            SjTagEditorDlg::OnDataInput         )
	EVT_BUTTON                  (wxID_OK,               SjTagEditorDlg::OnOK                )
	EVT_BUTTON                  (wxID_CANCEL,           SjTagEditorDlg::OnCancel            )
	EVT_BUTTON                  (IDC_PREVDLGPAGE,       SjTagEditorDlg::OnPrevOrNext        )
	EVT_MENU                    (IDC_PREVDLGPAGE,       SjTagEditorDlg::OnPrevOrNext        )
	EVT_BUTTON                  (IDC_NEXTDLGPAGE,       SjTagEditorDlg::OnPrevOrNext        )
	EVT_MENU                    (IDC_NEXTDLGPAGE,       SjTagEditorDlg::OnPrevOrNext        )
	EVT_CLOSE                   (                       SjTagEditorDlg::OnClose             )
END_EVENT_TABLE()


SjTagEditorDlg::SjTagEditorDlg(wxWindow* parent, bool multiEdit)
	: SjDialog(parent, wxEmptyString, SJ_MODELESS, SJ_RESIZEABLE_IF_POSSIBLE, wxCLIP_CHILDREN)
{
	wxBusyCursor busy;

	// save given objects
	m_dataMultiEdit = multiEdit;

	// get line height
	m_dlgLineHeight = 0;
	wxFont font( GetFont() );
	if (!font.IsOk())
	{
		font = *wxSWISS_FONT;
	}
	GetTextExtent( wxT("H"), (int*)NULL, &m_dlgLineHeight, (int*)NULL, (int*)NULL, &font);

	// create dialog
	m_dlgInputFromUser  = FALSE;

	wxBoxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	m_dlgNotebook = new wxNotebook(this, IDC_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxCLIP_CHILDREN);

	wxNotebook* notebookSizer = m_dlgNotebook;

	m_dlgNotebook->AddPage(CreateTitlePage(m_dlgNotebook), _("Edit track"));
	m_dlgNotebook->AddPage(CreateInfoPage(m_dlgNotebook),  _("Info"));

	sizer1->Add(notebookSizer, 1, wxALL|wxGROW, SJ_DLG_SPACE);

	wxString prevTitle, nextTitle;
	if( !m_dataMultiEdit )
	{
		prevTitle = _("Previous track");
		wxString shortcut = g_accelModule->GetReadableShortcutsByCmd(IDC_PREVDLGPAGE, SJ_SHORTCUTS_LOCAL|SJ_SHORTCUTS_SYSTEMWIDE);
		if( !shortcut.IsEmpty() ) { prevTitle.Append(wxString::Format(wxT(" [%s]"), shortcut.c_str())); }

		nextTitle = _("Next track");
		shortcut = g_accelModule->GetReadableShortcutsByCmd(IDC_NEXTDLGPAGE, SJ_SHORTCUTS_LOCAL|SJ_SHORTCUTS_SYSTEMWIDE);
		if( !shortcut.IsEmpty() ) { nextTitle.Append(wxString::Format(wxT(" [%s]"), shortcut.c_str())); }
	}

	sizer1->Add(CreateButtons(SJ_DLG_OK_CANCEL | SJ_DLG_MENU | (m_dataMultiEdit? 0 : SJ_DLG_PREV_NEXT), wxT(""), wxT(""), prevTitle, nextTitle),
	            0, wxGROW|wxALL, SJ_DLG_SPACE);

	sizer1->SetSizeHints(this);

	// set accelerators
	SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_EDIT));

	// done so far
	m_dlgInputFromUser  = TRUE;
}


bool SjTagEditorDlg::Init__()
{
	// load data, we're doing this here and not in the
	// constructor, as the constructor cannot call virtual functions properly.
	wxBusyCursor busy;

	// Load Data, Init Dialog
	if( !Dsk2Data_CopyAllData(0, TRUE/*log errros*/) )
	{
		return FALSE;
	}

	Data2Dlg_CopyAllData();
	return TRUE;
}


SjTagEditorDlg::~SjTagEditorDlg()
{
}


/*******************************************************************************
 * SjTagEditorDlg - Create the dialog
 ******************************************************************************/


wxTextCtrl* SjTagEditorDlg::CreateTextCtrl(wxWindow* parent, wxSizer* sizer1, int id,
        const wxSize& size, int borderTop, bool multiLine)
{
	wxTextCtrl* textCtrl;

	if( multiLine )
	{
		textCtrl = new wxTextCtrl(parent, id, wxEmptyString, wxDefaultPosition, size, wxTE_MULTILINE);
	}
	else
	{
		textCtrl = new wxTextCtrl(parent, id, wxEmptyString, wxDefaultPosition, size);
	}

	if( m_dataMultiEdit )
	{
		wxBoxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);
		sizer1->Add(sizer2, 0, wxGROW|wxTOP, borderTop);

		CreateCheckBox(parent, sizer2, id+1,
		               multiLine? wxALIGN_TOP : wxALIGN_CENTER_VERTICAL);

		sizer2->Add(textCtrl, 1, wxGROW, 0);
	}
	else
	{
		sizer1->Add(textCtrl, 0, wxGROW|wxTOP, borderTop);
	}

	return textCtrl;
}


void SjTagEditorDlg::CreateCheckBox(wxWindow* parent, wxSizer* sizer, int id, long align)
{
	wxCheckBox* checkBox = NULL;

	if( m_dataMultiEdit )
	{
		checkBox = new wxCheckBox(parent, id, wxEmptyString);
		sizer->Add(checkBox, 0, align|wxRIGHT, 2);
	}
}


void SjTagEditorDlg::CreateComboboxItems(int id, long what)
{
	wxArrayString   data = g_mainFrame->m_libraryModule->GetUniqueValues(what);
	long            i;

	wxComboBox* comboBox = (wxComboBox*)FindWindow(id);
	wxASSERT(comboBox);
	if( comboBox )
	{
		comboBox->Clear();
		for( i =0; i < (long)data.GetCount(); i++ )
		{
			comboBox->Append(data.Item(i));
		}
	}
}


wxString SjTagEditorDlg::CreateTitle(const wxString& title__, int prependSpaces)
{
	wxString title;

	if( m_dataMultiEdit && prependSpaces )
	{
		title.Printf(wxT("     %s:"), title__.c_str());
	}
	else
	{
		title.Printf(wxT("%s:"), title__.c_str());
	}

	return title;
}


wxPanel* SjTagEditorDlg::CreateTitlePage(wxWindow* parent)
{
	wxSize  numberSize      (m_dlgLineHeight*3, -1);
	wxSize  multilineSize   (-1,             (m_dlgLineHeight*2)+6);
	#define ADD_SPACE       ((SJ_DLG_SPACE/2)+1) // this additional space is added between a title and a control

	// create page and main page sizer
	wxPanel* page = new wxPanel(parent, -1);
	wxBoxSizer* sizer1 = new wxBoxSizer(wxHORIZONTAL);
	page->SetSizer(sizer1);

	// create left part...
	wxFlexGridSizer* sizer2f = new wxFlexGridSizer(5, 2, 0, SJ_DLG_SPACE);
	sizer2f->AddGrowableCol(1);
	sizer2f->AddGrowableRow(4);
	sizer1->Add(sizer2f, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	// ...track name
	sizer2f->Add(new wxStaticText(page, -1, CreateTitle(_("Title"))), 0,
	             wxALIGN_CENTER_VERTICAL|wxTOP, m_dlgLineHeight);
	CreateTextCtrl(page, sizer2f, IDC_TRACKNAME, wxDefaultSize, m_dlgLineHeight);

	// ...lead artist name
	sizer2f->Add(new wxStaticText(page, -1, CreateTitle(_("Artist"))), 0, wxALIGN_CENTER_VERTICAL|wxTOP, m_dlgLineHeight+ADD_SPACE);
	CreateTextCtrl(page, sizer2f, IDC_LEADARTISTNAME, wxDefaultSize, m_dlgLineHeight+ADD_SPACE);

	// ...original artist and composer
	sizer2f->Add(5, 5, 0, wxTOP, ADD_SPACE);

	wxFlexGridSizer* sizer3f = new wxFlexGridSizer(2, 2, 0, SJ_DLG_SPACE);
	sizer3f->AddGrowableCol(0);
	sizer3f->AddGrowableCol(1);
	sizer2f->Add(sizer3f, 0, wxGROW|wxTOP, ADD_SPACE);

	wxStaticText* staticText = new wxStaticText(page, -1, CreateTitle(_("Original artist"), TRUE));
	sizer3f->Add(staticText, 0, 0, 0);

	staticText =  new wxStaticText(page, -1, CreateTitle(_("Composer"), TRUE));
	sizer3f->Add(staticText, 0, 0, 0);

	CreateTextCtrl(page, sizer3f, IDC_ORGARTISTNAME, wxDefaultSize, 0);
	CreateTextCtrl(page, sizer3f, IDC_COMPOSERNAME, wxDefaultSize, 0);

	// ...album name
	sizer2f->Add(new wxStaticText(page, -1, CreateTitle(_("Album"))), 0, wxALIGN_CENTER_VERTICAL|wxTOP, m_dlgLineHeight+ADD_SPACE);
	CreateTextCtrl(page, sizer2f, IDC_ALBUMNAME, wxDefaultSize, m_dlgLineHeight+ADD_SPACE);

	// ...comment
	sizer2f->Add(new wxStaticText(page, -1, CreateTitle(_("Comment"))), 0, wxALIGN_TOP|wxTOP, m_dlgLineHeight+ADD_SPACE);
	CreateTextCtrl(page, sizer2f, IDC_COMMENT, multilineSize, m_dlgLineHeight+ADD_SPACE, TRUE);

	// some space between left and right parts
	sizer1->Add(SJ_DLG_SPACE*2, SJ_DLG_SPACE, 0, 0, 0);

	// create right part...
	wxBoxSizer* sizer2 = new wxBoxSizer(wxVERTICAL);
	sizer1->Add(sizer2, 0, wxALL|wxGROW, SJ_DLG_SPACE);

	// ...track number
	sizer2->Add(new wxStaticText(page, -1, CreateTitle(_("Track number"), TRUE)), 0, 0, 0);

	wxBoxSizer* sizer3 = new wxBoxSizer(wxHORIZONTAL);
	sizer2->Add(sizer3, 0, wxGROW, 0);

	CreateCheckBox(page, sizer3, IDC_TRACKNRCHECK);
	sizer3->Add(new wxTextCtrl(page, IDC_TRACKNR, wxEmptyString, wxDefaultPosition, numberSize), 1, 0, 0);

	staticText = new wxStaticText(page, -1, _("of"));
	sizer3->Add(staticText, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, SJ_DLG_SPACE/2);

	CreateCheckBox(page, sizer3, IDC_TRACKCOUNTCHECK);
	sizer3->Add(new wxTextCtrl(page, IDC_TRACKCOUNT, wxEmptyString, wxDefaultPosition, numberSize), 1, 0, 0);

	// ...disk number
	sizer2->Add(new wxStaticText(page, -1, CreateTitle(_("Disk number"), TRUE)), 0, wxTOP, ADD_SPACE);

	sizer3 = new wxBoxSizer(wxHORIZONTAL);
	sizer2->Add(sizer3, 0, wxGROW, 0);

	CreateCheckBox(page, sizer3, IDC_DISKNRCHECK);
	sizer3->Add(new wxTextCtrl(page, IDC_DISKNR, wxEmptyString, wxDefaultPosition, numberSize), 1, 0, 0);

	staticText = new wxStaticText(page, -1, _("of"));
	sizer3->Add(staticText, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, SJ_DLG_SPACE/2);

	CreateCheckBox(page, sizer3, IDC_DISKCOUNTCHECK);
	sizer3->Add(new wxTextCtrl(page, IDC_DISKCOUNT, wxEmptyString, wxDefaultPosition, numberSize), 1, 0, 0);


	// ...genre
	sizer2->Add(new wxStaticText(page, -1, CreateTitle(_("Genre"), TRUE)), 0, wxTOP, ADD_SPACE);

	sizer3 = new wxBoxSizer(wxHORIZONTAL);
	sizer2->Add(sizer3, 0, wxGROW, 0);

	CreateCheckBox(page, sizer3, IDC_GENRECHECK);
	sizer3->Add(new wxComboBox(page, IDC_GENRENAME, wxEmptyString, wxDefaultPosition, numberSize), 1, 0, 0);

	CreateComboboxItems(IDC_GENRENAME, SJ_TI_GENRENAME);

	// content group
	sizer2->Add(new wxStaticText(page, -1, CreateTitle(_("Group"), TRUE)), 0, wxTOP, ADD_SPACE);

	sizer3 = new wxBoxSizer(wxHORIZONTAL);
	sizer2->Add(sizer3, 0, wxGROW, 0);

	CreateCheckBox(page, sizer3, IDC_GROUPCHECK);
	sizer3->Add(new wxComboBox(page, IDC_GROUPNAME, wxEmptyString, wxDefaultPosition, numberSize), 1, 0, 0);

	CreateComboboxItems(IDC_GROUPNAME, SJ_TI_GROUPNAME);

	// year / bpm
	sizer3f = new wxFlexGridSizer(2, 2, 0, SJ_DLG_SPACE);
	sizer3f->AddGrowableCol(0);
	sizer3f->AddGrowableCol(1);
	sizer2->Add(sizer3f, 0, wxGROW|wxTOP, ADD_SPACE);

	sizer3f->Add(new wxStaticText(page, -1, CreateTitle(_("Year"), TRUE)), 0, 0, 0);
	sizer3f->Add(new wxStaticText(page, -1, CreateTitle(_("BPM"), TRUE)), 0, 0, 0);
	CreateTextCtrl(page, sizer3f, IDC_YEAR, numberSize, 0);
	CreateTextCtrl(page, sizer3f, IDC_BPM, numberSize, 0);


	// some space
	sizer2->Add(ADD_SPACE, ADD_SPACE*3, 1, 0, 0);

	// rating
	sizer2->Add(new wxStaticText(page, -1, CreateTitle(_("Rating"), TRUE)), 0, 0, 0);

	sizer3 = new wxBoxSizer(wxHORIZONTAL);
	sizer2->Add(sizer3, 0, wxGROW, 0);

	CreateCheckBox(page, sizer3, IDC_RATINGCHECK);
	m_dlgRatingChoice = new wxChoice(page, IDC_RATING, wxDefaultPosition, numberSize);
	if( m_dataMultiEdit )
	{
		m_dlgRatingChoice->Append(wxT(""));
	}
	m_dlgRatingChoice->Append(SJ_RATING_CHARS_DLG);
	m_dlgRatingChoice->Append(SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG);
	m_dlgRatingChoice->Append(SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG);
	m_dlgRatingChoice->Append(SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG);
	m_dlgRatingChoice->Append(SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG SJ_RATING_CHARS_DLG);
	m_dlgRatingChoice->Append(_("No rating"));
	sizer3->Add(m_dlgRatingChoice, 1, 0, 0);
	// done
	return page;
}


wxPanel* SjTagEditorDlg::CreateInfoPage(wxWindow* parent)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	wxPanel*    page = new wxPanel(parent, -1);
	page->SetSizer(sizer);

	SjHtmlWindow* htmlWindow = new SjHtmlWindow(page, IDC_HTMLWINDOW, wxPoint(-1, -1), wxSize(HTML_WINDOW_W, 100), wxHW_SCROLLBAR_AUTO | wxSUNKEN_BORDER);

	sizer->Add(m_dlgLineHeight, m_dlgLineHeight + SJ_DLG_SPACE/2, 0, 0, 0); /*just a spacer */
	sizer->Add(htmlWindow, 1, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	return page;
}


/*******************************************************************************
 * SjTagEditorDlg - Events
 ******************************************************************************/


void SjTagEditorDlg::OnButtonBarMenu(wxCommandEvent& event)
{
	wxButton*   wnd = (wxButton*)FindWindow(IDC_BUTTONBARMENU);
	SjMenu      menu(SJ_SHORTCUTS_LOCAL);

	if( wnd )
	{
		menu.Append(IDC_PLUGIN_REPLACE);
		menu.Append(IDC_PLUGIN_SPLIT);
		menu.Append(IDC_PLUGIN_RENAME);
		menu.Append(IDC_PLUGIN_FREEDB);
		menu.AppendSeparator();
		menu.AppendCheckItem(IDC_WRITEID3TAGS, _("Write (ID3-)tags"));
		menu.Check(IDC_WRITEID3TAGS, g_tagEditorModule->GetWriteId3Tags());
		wnd->PopupMenu(&menu, 0, 0);
	}
}


void SjTagEditorDlg::OnWriteId3Tags(wxCommandEvent& event)
{
	g_tagEditorModule->SetWriteId3Tags(!g_tagEditorModule->GetWriteId3Tags());
}


void SjTagEditorDlg::OnPlugin(wxCommandEvent& event)
{
	// show the plugin dialog
	SjTagEditorPlugin* plugin = NULL;
	int modalResult;
	{
		SJ_WINDOW_DISABLER(this);

		SjTrackInfo exampleTrackInfo;
		g_mainFrame->m_libraryModule->GetTrackInfo(m_dataUrls[0], exampleTrackInfo, SJ_TI_FULLINFO, FALSE);

		switch( event.GetId() )
		{
			case IDC_PLUGIN_REPLACE:    plugin = new SjReplacePlugin(this, &exampleTrackInfo);              break;
			case IDC_PLUGIN_SPLIT:      plugin = new SjSplitPlugin(this, &exampleTrackInfo);                break;
			case IDC_PLUGIN_RENAME:     plugin = new SjRenamePlugin(this, &exampleTrackInfo);               break;
			case IDC_PLUGIN_FREEDB:     plugin = new SjFreedbPlugin(this, &exampleTrackInfo, m_dataUrls);   break;
		}

		if( plugin == NULL )
		{
			wxASSERT( plugin );
			return; // error
		}

		plugin->AfterConstructor();
		plugin->SetTitle(plugin->GetTitle() + wxString(wxT(" - ")) + m_dataTitle);

		modalResult = plugin->ShowModal();
	}

	// see what to do
	if( modalResult == wxID_OK )
	{
		bool reloadData, canceled;
		Dlg2Data_CopyAll(plugin, reloadData, canceled);
		if( reloadData )
		{
			wxBusyCursor busy;
			Dsk2Data_CopyAllData(0);
			Data2Dlg_CopyAllData();
		}
	}

	// done
	delete plugin;
}


void SjTagEditorDlg::OnDataInput(wxCommandEvent& event)
{
	// Is the data input really from the user? we check this flag
	// as this event is also emmitted on SetValue().
	// Moreover, we're avoiding recursive calls to this function using "inHere".
	static bool inHere = FALSE;
	if( m_dlgInputFromUser && !inHere  )
	{
		inHere = TRUE;

		// Get ID and Window
		int id = event.GetId();
		wxWindow* window = FindWindow(id);
		if( window )
		{
			// Check the checkbox if not yet checked
			if( m_dataMultiEdit )
			{
				wxCheckBox* checkbox = (wxCheckBox*)FindWindow(id+1);
				if( checkbox )
				{
					checkbox->SetValue(id!=IDC_RATING || ((wxChoice*)window)->GetSelection()!=0);
				}
			}

			// auto-completion stuff
			SjLibraryModule* library = g_mainFrame->m_libraryModule;
			if( id == IDC_LEADARTISTNAME )
			{
				library->DoAutoComplete(SJ_TI_LEADARTISTNAME, m_autoCompleteLeadArtistLen, window);
			}
			else if( id == IDC_ORGARTISTNAME )
			{
				library->DoAutoComplete(SJ_TI_ORGARTISTNAME, m_autoCompleteOrgArtistLen, window);
			}
			else if( id == IDC_COMPOSERNAME )
			{
				library->DoAutoComplete(SJ_TI_COMPOSERNAME, m_autoCompleteComposerLen, window);
			}
			else if( id == IDC_ALBUMNAME )
			{
				library->DoAutoComplete(SJ_TI_ALBUMNAME, m_autoCompleteAlbumLen, window);
			}
			else if( id == IDC_GENRENAME )
			{
				library->DoAutoComplete(SJ_TI_GENRENAME, m_autoCompleteGenreLen, window);
			}
			else if( id == IDC_GROUPNAME )
			{
				library->DoAutoComplete(SJ_TI_GROUPNAME, m_autoCompleteGroupLen, window);
			}
			/*else if( id == IDC_TRACKNAME )
			{
			    library->DoAutoComplete(SJ_TI_TRACKNAME, m_autoCompleteTrackLen, window);
			}*/
			/*else if( id == IDC_COMMENT )
			{
			    library->DoAutoComplete(SJ_TI_COMMENT, m_autoCompleteCommentLen, window);
			}*/
		}

		inHere = FALSE;
	}
}


void SjTagEditorDlg::OnTogglePage(wxCommandEvent&)
{
	// toggle between the edit- and the info-page,
	// called when pressing the shortcut "Ctrl-I"

	if( m_dlgNotebook )
	{
		m_dlgNotebook->SetSelection(m_dlgNotebook->GetSelection()? 0 : 1);
	}
}


void SjTagEditorDlg::OnCancel(wxCommandEvent&)
{
	g_tagEditorModule->CloseTagEditor(); // Calls Destroy(), do not used delete!
}


void SjTagEditorDlg::OnOK(wxCommandEvent&)
{
	bool reloadData, canceled;
	Dlg2Data_CopyAll(NULL, reloadData, canceled);
	if( !canceled )
	{
		g_tagEditorModule->CloseTagEditor(); // Calls Destroy(), do not used delete!
	}
	else if( reloadData )
	{
		wxBusyCursor busy;
		Dsk2Data_CopyAllData(0);
		Data2Dlg_CopyAllData();
	}
}


void SjTagEditorDlg::OnPrevOrNext(wxCommandEvent& event)
{
	bool reloadData, canceled;
	Dlg2Data_CopyAll(NULL, reloadData, canceled); // ignore the return value, goto prev/next in any case

	{
		wxBusyCursor busy;
		if( Dsk2Data_CopyAllData(event.GetId()==IDC_PREVDLGPAGE? -1 : 1, TRUE) )
		{
			Data2Dlg_CopyAllData();
		}
	}
}


/*******************************************************************************
 * SjTagEditorDlg - Transferring disk -> data
 ******************************************************************************/


bool SjTagEditorDlg::Dsk2Data_CopyAllData(int what, bool logErrors)
{
	// get the library module
	SjLibraryModule* lib = g_mainFrame->m_libraryModule;
	if( lib == NULL )
	{
		return FALSE;
	}

	// get the urls
	size_t urlCount;
	{
		// the derived calls must supply the urls, get the urls
		wxArrayString urls;
		if( !GetUrls(urls, what) )
		{
			return FALSE;
		}

		// check the number of urls, multiple editing is only
		// supported if "multiEdit" was given to the constructor
		urlCount = urls.GetCount();
		if(  urlCount <= 0
		 || (urlCount >  1 && !m_dataMultiEdit) )
		{
			return FALSE;
		}

		m_dataUrls = urls;
	}

	// init stats for single edit - that's easy
	if( !m_dataMultiEdit )
	{
		m_dataStat.Clear();
		m_dataTitle = SjTools::ShortenUrl(m_dataUrls[0], 45/*tested on MSW*/);
		return lib->GetTrackInfo(m_dataUrls[0], m_dataStat, SJ_TI_FULLINFO, logErrors);
	}


	// init stats for multiple edit - gather averages, sums etc.
	m_dataTitle.Printf(_("%s tracks"), SjTools::FormatNumber(urlCount).c_str());

	long            validPlaytimes = 0, invalidPlaytimes = 0;
	long            dataMB = 0,
	                bitrateKbyte = 0,
	                bitrateCount = 0;
	wxLongLong      samplerate;
	long            samplerateCount = 0;
	wxLongLong      gain;
	long            gainCount = 0;

	// get the first track information
	m_dataStat.Clear();
	if( !lib->GetTrackInfo(m_dataUrls[0], m_dataStat, SJ_TI_FULLINFO, logErrors) )
	{
		return FALSE;
	}
	m_dataStat.m_validFields = 0x7FFFFFFFL & ~SJ_TI_URL;

	// init playtime
	m_dataStat.m_playtimeMs = m_dataStat.m_playtimeMs/1000;
	if( m_dataStat.m_playtimeMs ) { validPlaytimes++; } else { invalidPlaytimes++; }

	// init bitrate / samplerate / gain
	bitrateKbyte = m_dataStat.m_bitrate / 1024;
	if( bitrateKbyte ) bitrateCount++;

	samplerate = m_dataStat.m_samplerate;
	if( samplerate != 0 ) samplerateCount++;

	gain = m_dataStat.m_autoVol;
	if( gain > 0 ) gainCount++;

	// go through all data
	size_t i;
	SjTrackInfo currTrackInfo;
	for( i = 1; i < urlCount; i++ )
	{
		// get subsequent track information
		currTrackInfo.Clear();
		if( !lib->GetTrackInfo(m_dataUrls[i], currTrackInfo, SJ_TI_FULLINFO, logErrors) )
		{
			return FALSE;
		}

		// adapt playtime
		if( currTrackInfo.m_playtimeMs )
		{
			m_dataStat.m_playtimeMs += currTrackInfo.m_playtimeMs/1000;
			validPlaytimes++;
		}
		else
		{
			invalidPlaytimes++;
		}

		// adapt bitrate / samplerate / gain
		if( currTrackInfo.m_bitrate )
		{
			bitrateKbyte += currTrackInfo.m_bitrate / 1024;
			bitrateCount++;
		}

		if( currTrackInfo.m_samplerate )
		{
			samplerate += currTrackInfo.m_samplerate;
			samplerateCount++;
		}

		if( currTrackInfo.m_autoVol )
		{
			gain += currTrackInfo.m_autoVol;
			gainCount++;
		}

		// adapt channels
		if( currTrackInfo.m_channels > m_dataStat.m_channels )
		{
			m_dataStat.m_channels = currTrackInfo.m_channels;
		}

		// adapt data bytes
		m_dataStat.m_dataBytes += currTrackInfo.m_dataBytes;
		if( m_dataStat.m_dataBytes >= SJ_ONE_GB )
		{
			dataMB += 1024;
			m_dataStat.m_dataBytes -= SJ_ONE_GB;
		}

		// adapt play count and dates
		m_dataStat.m_timesPlayed += currTrackInfo.m_timesPlayed;

		if( currTrackInfo.m_lastPlayed > m_dataStat.m_lastPlayed )
		{
			m_dataStat.m_lastPlayed = currTrackInfo.m_lastPlayed;
		}

		if( currTrackInfo.m_timeAdded < m_dataStat.m_timeAdded )
		{
			m_dataStat.m_timeAdded = currTrackInfo.m_timeAdded;
		}

		if( currTrackInfo.m_timeModified > m_dataStat.m_timeModified )
		{
			m_dataStat.m_timeModified = currTrackInfo.m_timeModified;
		}

		// adapt title etc.
		Dsk2Data_CheckUnique(m_dataStat.m_trackName,        currTrackInfo.m_trackName,          SJ_TI_TRACKNAME,        m_dataStat.m_validFields);
		Dsk2Data_CheckUnique(m_dataStat.m_leadArtistName,   currTrackInfo.m_leadArtistName,     SJ_TI_LEADARTISTNAME,   m_dataStat.m_validFields);
		Dsk2Data_CheckUnique(m_dataStat.m_orgArtistName,    currTrackInfo.m_orgArtistName,      SJ_TI_ORGARTISTNAME,    m_dataStat.m_validFields);
		Dsk2Data_CheckUnique(m_dataStat.m_composerName,     currTrackInfo.m_composerName,       SJ_TI_COMPOSERNAME,     m_dataStat.m_validFields);
		Dsk2Data_CheckUnique(m_dataStat.m_albumName,        currTrackInfo.m_albumName,          SJ_TI_ALBUMNAME,        m_dataStat.m_validFields);
		Dsk2Data_CheckUnique(m_dataStat.m_comment,          currTrackInfo.m_comment,            SJ_TI_COMMENT,          m_dataStat.m_validFields);
		Dsk2Data_CheckUnique(m_dataStat.m_trackNr,          currTrackInfo.m_trackNr,            SJ_TI_TRACKNR,          m_dataStat.m_validFields);
		Dsk2Data_CheckUnique(m_dataStat.m_trackCount,       currTrackInfo.m_trackCount,         SJ_TI_TRACKCOUNT,       m_dataStat.m_validFields);
		Dsk2Data_CheckUnique(m_dataStat.m_diskNr,           currTrackInfo.m_diskNr,             SJ_TI_DISKNR,           m_dataStat.m_validFields);
		Dsk2Data_CheckUnique(m_dataStat.m_diskCount,        currTrackInfo.m_diskCount,          SJ_TI_DISKCOUNT,        m_dataStat.m_validFields);
		Dsk2Data_CheckUnique(m_dataStat.m_genreName,        currTrackInfo.m_genreName,          SJ_TI_GENRENAME,        m_dataStat.m_validFields);
		Dsk2Data_CheckUnique(m_dataStat.m_groupName,        currTrackInfo.m_groupName,          SJ_TI_GROUPNAME,        m_dataStat.m_validFields);
		Dsk2Data_CheckUnique(m_dataStat.m_year,             currTrackInfo.m_year,               SJ_TI_YEAR,             m_dataStat.m_validFields);
		Dsk2Data_CheckUnique(m_dataStat.m_beatsPerMinute,   currTrackInfo.m_beatsPerMinute,     SJ_TI_BEATSPERMINUTE,   m_dataStat.m_validFields);
		Dsk2Data_CheckUnique(m_dataStat.m_rating,           currTrackInfo.m_rating,             SJ_TI_RATING,           m_dataStat.m_validFields);
	}

	// finish playing time
	// (we're using seconds instead of ms -> negative number)
	if( validPlaytimes && invalidPlaytimes )
	{
		m_dataStat.m_playtimeMs += (m_dataStat.m_playtimeMs/validPlaytimes) * invalidPlaytimes;
	}

	m_dataStat.m_playtimeMs *= -1;

	// finish data bytes
	// (we may be using MB instead of bytes -> negative number)
	if( dataMB )
	{
		dataMB += m_dataStat.m_dataBytes/SJ_ONE_MB;
		m_dataStat.m_dataBytes = dataMB * -1;
	}

	// finish bitrate / samplerate
	m_dataStat.m_bitrate = 0;
	if( bitrateCount )
	{
		m_dataStat.m_bitrate = (bitrateKbyte / bitrateCount) * 1024;
	}

	m_dataStat.m_samplerate = 0;
	if( samplerateCount )
	{
		samplerate /= samplerateCount;
		m_dataStat.m_samplerate = samplerate.ToLong();
	}

	m_dataStat.m_autoVol = 0;
	if( gainCount )
	{
		gain /= gainCount;
		m_dataStat.m_autoVol = gain.ToLong();
	}

	return TRUE;
}


void SjTagEditorDlg::Dsk2Data_CheckUnique(wxString& str1, const wxString& str2, long flag, long& uniqueState)
{
	if( uniqueState & flag )
	{
		if( str1 != str2 )
		{
			str1.Clear();
			uniqueState &= ~flag;
		}
	}
}


void SjTagEditorDlg::Dsk2Data_CheckUnique(long& val1, long val2, long flag, long& uniqueState)
{
	if( uniqueState & flag )
	{
		if( val1 != val2 )
		{
			val1 = 0;
			uniqueState &= ~flag;
		}
	}
}


/*******************************************************************************
 * SjTagEditorDlg - Transferring data -> dialog
 ******************************************************************************/


void SjTagEditorDlg::Data2Dlg_CopySingleData(int idEdit, const wxString& strVal)
{
	wxTextCtrl* textCtrl = (wxTextCtrl*)FindWindow(idEdit);
	wxASSERT(textCtrl);
	if( textCtrl )
	{
		textCtrl->SetValue(strVal);
		textCtrl->SetSelection(-1, -1);
	}
}


void SjTagEditorDlg::Data2Dlg_CopyAllData()
{
	m_dlgInputFromUser = FALSE;

	// genre / content group
	wxComboBox* comboBox = (wxComboBox*)FindWindow(IDC_GENRENAME);
	wxASSERT(comboBox);
	if( comboBox )
	{
		comboBox->SetValue(m_dataStat.m_genreName);
	}

	comboBox = (wxComboBox*)FindWindow(IDC_GROUPNAME);
	wxASSERT(comboBox);
	if( comboBox )
	{
		comboBox->SetValue(m_dataStat.m_groupName);
	}

	// rest
	Data2Dlg_CopySingleData(IDC_TRACKNAME,      m_dataStat.m_trackName      );
	Data2Dlg_CopySingleData(IDC_TRACKNR,        m_dataStat.m_trackNr        );
	Data2Dlg_CopySingleData(IDC_TRACKCOUNT,     m_dataStat.m_trackCount     );
	Data2Dlg_CopySingleData(IDC_LEADARTISTNAME, m_dataStat.m_leadArtistName );
	Data2Dlg_CopySingleData(IDC_ORGARTISTNAME,  m_dataStat.m_orgArtistName  );
	Data2Dlg_CopySingleData(IDC_COMPOSERNAME,   m_dataStat.m_composerName   );
	Data2Dlg_CopySingleData(IDC_ALBUMNAME,      m_dataStat.m_albumName      );
	Data2Dlg_CopySingleData(IDC_YEAR,           m_dataStat.m_year           );
	Data2Dlg_CopySingleData(IDC_BPM,            m_dataStat.m_beatsPerMinute );
	Data2Dlg_CopySingleData(IDC_DISKNR,         m_dataStat.m_diskNr         );
	Data2Dlg_CopySingleData(IDC_DISKCOUNT,      m_dataStat.m_diskCount      );
	Data2Dlg_CopySingleData(IDC_COMMENT,        m_dataStat.m_comment        );

	// Rating
	if( m_dataMultiEdit )
	{
		m_dlgRatingChoice->SetSelection((m_dataStat.m_validFields&SJ_TI_RATING && m_dataStat.m_rating==0)? 6 : m_dataStat.m_rating);
	}
	else
	{
		m_dlgRatingChoice->SetSelection(m_dataStat.m_rating==0? 5 : m_dataStat.m_rating-1);
	}

	// Set title
	SetTitle(_("Edit tracks") + wxString(wxT(" - ")) + m_dataTitle);

	// Set Focus
	wxTextCtrl* textCtrl = (wxTextCtrl*)FindWindow((m_dataMultiEdit && m_dataStat.m_trackName.IsEmpty())? IDC_LEADARTISTNAME : IDC_TRACKNAME);
	if( textCtrl )
	{
		textCtrl->SetFocus();
	}

	// init information window
	Data2Dlg_CopyInfo(FALSE);

	// init auto-complete stuff
	m_autoCompleteTrackLen = 0;
	m_autoCompleteLeadArtistLen = 0;
	m_autoCompleteOrgArtistLen = 0;
	m_autoCompleteComposerLen = 0;
	m_autoCompleteAlbumLen = 0;
	m_autoCompleteGenreLen = 0;
	m_autoCompleteGroupLen = 0;
	m_autoCompleteCommentLen = 0;

	m_dlgInputFromUser = TRUE;
}


void SjTagEditorDlg::Data2Dlg_CopyInfo(bool showMoreInfo)
{
	wxBusyCursor busy;

	SjHtmlWindow* htmlWindow = (SjHtmlWindow*)FindWindow(IDC_HTMLWINDOW);
	wxASSERT(htmlWindow);
	if( htmlWindow == NULL )
	{
		return;
	}

	SjProp          p;
	wxArrayString   names;
	wxArrayString   values;
	long            i, urlCount;
	wxString        temp;
	wxString        finalHtml;
	wxString        cacheName;

	// urls
	urlCount = m_dataUrls.GetCount();
	wxASSERT( (urlCount == 1 && !m_dataMultiEdit) || (urlCount > 1 && m_dataMultiEdit) );
	if( !m_dataMultiEdit )
	{
		wxString filename = m_dataStat.m_url;
		if( filename.StartsWith("file:") )
		{
			filename = wxFileSystem::URLToFileName(filename).GetFullPath();
		}

		p.Add(_("File name"), filename, SJ_PROP_BOLD);

		cacheName = g_tools->m_cache.LookupCache(m_dataStat.m_url);
		if( !cacheName.IsEmpty() )
		{
			m_dataStat.m_dataBytes = g_tools->GetFileSize(cacheName);
		}
	}
	else
	{
		wxString filename;
		for( i = 0; i < urlCount; i++ )
		{
			filename = m_dataUrls[i];
			if( filename.StartsWith("file:") )
			{
				filename = wxFileSystem::URLToFileName(filename).GetFullPath();
			}

			p.Add(i==0? _("File names") : wxT(""), filename, SJ_PROP_BOLD);

			if( !showMoreInfo && i >= 2 && urlCount > 4 )
			{
				p.Add(wxT(""), wxString::Format(_("Show all %s file names..."),
				                                SjTools::FormatNumber(urlCount).c_str()), SJ_PROP_ID|IDC_SHOWMOREINFO);
				break;
			}

			finalHtml = wxT("<p>") + SjTools::Htmlentities(_("The shown values are the sum or the average of all single values in the given files.")) + wxT("</p>");
		}
	}

	// size
	if( m_dataStat.m_dataBytes == 0 )
	{
		temp.Empty();
	}
	else if( m_dataStat.m_dataBytes >= 0 )
	{
		temp = SjTools::FormatBytes(m_dataStat.m_dataBytes, SJ_FORMAT_ADDEXACT);
	}
	else
	{
		temp = SjTools::FormatBytes(m_dataStat.m_dataBytes*-1, SJ_FORMAT_MB|SJ_FORMAT_ADDEXACT);
	}
	p.Add(_("File size"), temp, SJ_PROP_EMPTYIFEMPTY);

	// date added
	p.Add(_("Date added"), SjTools::FormatDate(m_dataStat.m_timeAdded, SJ_FORMAT_ADDTIME), SJ_PROP_EMPTYIFEMPTY);

	// date modified
	p.Add(_("Date modified"), SjTools::FormatDate(m_dataStat.m_timeModified, SJ_FORMAT_ADDTIME), SJ_PROP_EMPTYIFEMPTY);

	// last played, play count
	temp = SjTools::FormatDate(m_dataStat.m_lastPlayed, SJ_FORMAT_ADDTIME);
	if( m_dataStat.m_timesPlayed > 0 )
	{
		temp += wxT(", ") + wxString(_("Play count")) + wxT(": ") + SjTools::FormatNumber(m_dataStat.m_timesPlayed);
	}
	p.Add(_("Last played"), temp, SJ_PROP_EMPTYIFEMPTY);

	// queue position
	if( urlCount == 1 )
	{
		wxArrayLong allQueuePositions;
		if( g_mainFrame->GetAllQueuePosByUrl(m_dataStat.m_url, allQueuePositions) > 0 )
		{
			p.Add(_("Queue position"), SjTools::FormatNumbers(allQueuePositions, 1/*we want to show numbers starting at 1*/));

			if( g_debug )
			{
				long playCount = g_mainFrame->m_player.m_queue.GetPlayCount(allQueuePositions[0]);
				long repeatRound = g_mainFrame->m_player.m_queue.GetRepeatRound();
				p.Add(wxT("Queue play count"), playCount); // this has nothing to do with the "times played" from above
				p.Add(wxT("Queue repeat round"), repeatRound);
			}
		}
	}

	// playtime
	temp = m_dataStat.m_playtimeMs? SjTools::FormatTime(m_dataStat.m_playtimeMs>0? m_dataStat.m_playtimeMs/1000 : m_dataStat.m_playtimeMs*-1) : wxString();
	p.Add(_("Duration"), temp, SJ_PROP_BOLD|SJ_PROP_EMPTYIFEMPTY);

	// volume
	temp.Empty();
	if( urlCount == 1 )
	{
		double trackGain, albumGain;
		g_mainFrame->m_libraryModule->GetAutoVol(m_dataStat.m_url, &trackGain, &albumGain);
		if( trackGain > 0.0F )
		{
			temp += _("Track") + wxString(wxT(": ")) + SjTools::FormatGain(trackGain);
		}
		else if( albumGain > 0.0F )
		{
			temp += _("Track") + wxString(wxT(": ")) + _("n/a");
		}

		if( albumGain > 0.0F )
		{
			temp += wxString(wxT(", ")) + _("Album") + wxString(wxT(": ")) + SjTools::FormatGain(albumGain);
		}
	}
	else if( m_dataStat.m_autoVol )
	{
		temp = SjTools::FormatGain(::SjLong2Gain(m_dataStat.m_autoVol));
	}

	p.Add(_("Gain"), temp, SJ_PROP_EMPTYIFEMPTY);

	// bitrate, samplerate, channels
	p.Add(_("Channels"), m_dataStat.m_channels, SJ_PROP_EMPTYIFEMPTY);

	if( m_dataStat.m_samplerate )
	{
		p.Add(_("Samplerate"), SjTools::FormatNumber(m_dataStat.m_samplerate)+wxT(" ")+
		// TRANSLATORS: Abbreviation of "Hertz"
		_("Hz"));
	}
	else
	{
		p.Add(_("Samplerate"), 0L, SJ_PROP_EMPTYIFEMPTY);
	}

	if( m_dataStat.m_bitrate )
	{
		p.Add(_("Bitrate"), SjTools::FormatNumber(m_dataStat.m_bitrate)+wxT(" ")+
		// TRANSLATORS: Abbreviation of "Bits per second"
		_("bit/s"));
	}
	else
	{
		p.Add(_("Bitrate"), 0L, SJ_PROP_EMPTYIFEMPTY);
	}

	// ext. info
	bool scrollDown = FALSE;
	if( urlCount == 1 )
	{
		if( showMoreInfo )
		{
			p.Add(_("Further information"), wxT(""), SJ_PROP_HEADLINE);

			wxString ext = SjTools::GetExt(m_dataStat.m_url);
			p.Add(_("File type"), ext);

			p.Add(_("URL"), m_dataStat.m_url);

			{
				wxFileSystem fs;
				wxFSFile* fsFile = fs.OpenFile(m_dataStat.m_url, wxFS_READ|wxFS_SEEKABLE);
				if( fsFile )
				{
					SjGetMoreInfoFromID3Etc(fsFile, p);
					delete fsFile;
				}
			}

			if( !cacheName.IsEmpty() )
			{
				p.Add(_("Temporary directory"), cacheName);
			}

			scrollDown = TRUE;
		}
		else
		{
			finalHtml = wxString::Format(wxT("<p><a href=\"id:%i\">%s</a></p>"), (int)IDC_SHOWMOREINFO, _("Further information..."));
		}
	}

	// done so far
	htmlWindow->InitPage();
	htmlWindow->AddPropToPage(p);

	if( !finalHtml.IsEmpty() )
	{
		htmlWindow->AppendToPage(   finalHtml   );
	}

	if( scrollDown )
	{
		htmlWindow->Scroll(-1, 8L);
	}
}


/*******************************************************************************
 * SjTagEditorDlg - Transferring dialog -> data
 ******************************************************************************/


bool SjTagEditorDlg::Dlg2Data_IsChecked(int id)
{
	bool isChecked = TRUE;

	if( m_dataMultiEdit )
	{
		wxCheckBox* checkBox = (wxCheckBox*)FindWindow(id+1);
		if( checkBox == NULL || !checkBox->GetValue() )
		{
			isChecked = FALSE;
		}
	}

	return isChecked;
}


wxString SjTagEditorDlg::Dlg2Data_GetValue(int id)
{
	wxWindow* window = FindWindow(id);
	if( window )
	{
		if( id == IDC_GENRENAME || id == IDC_GROUPNAME )
		{
			return ((wxComboBox*)window)->GetValue();
		}
		else if( id == IDC_RATING )
		{
			int index = ((wxChoice*)window)->GetSelection();
			if( m_dataMultiEdit )
			{
				return wxString::Format(wxT("%i"), index==6? 0 : index);
			}
			else
			{
				return wxString::Format(wxT("%i"), index==5? 0 : index+1);
			}
		}
		else
		{
			return ((wxTextCtrl*)window)->GetValue();
		}
	}

	return wxEmptyString;
}


long SjTagEditorDlg::Dlg2Data_GetString(int id, long fieldFlag, wxString& oldAndRetValue)
{
	if( Dlg2Data_IsChecked(id) )
	{
		wxString newValue = Dlg2Data_GetValue(id);
		newValue.Trim(TRUE);
		newValue.Trim(FALSE);

		if( newValue != oldAndRetValue
		 || m_dataMultiEdit )
		{
			oldAndRetValue = newValue;
			return fieldFlag;
		}
	}

	return 0;
}


long SjTagEditorDlg::Dlg2Data_GetLong(int id, long fieldFlag, long& oldAndRetValue)
{
	if( Dlg2Data_IsChecked(id) )
	{
		wxString newValueString = Dlg2Data_GetValue(id);
		newValueString.Trim(TRUE);
		newValueString.Trim(FALSE);

		long newValue = 0;
		if( !newValueString.ToLong(&newValue) )
		{
			newValue = 0;
		}

		if( newValue != oldAndRetValue
		 || m_dataMultiEdit )
		{
			oldAndRetValue = newValue;
			return fieldFlag;
		}
	}

	return 0;
}


bool SjTagEditorDlg::Dlg2Data_PrepareModify()
{
	long changes = 0;

	changes |= Dlg2Data_GetString   (IDC_TRACKNAME,         SJ_TI_TRACKNAME,        m_dataStat.m_trackName);
	changes |= Dlg2Data_GetString   (IDC_LEADARTISTNAME,    SJ_TI_LEADARTISTNAME,   m_dataStat.m_leadArtistName);
	changes |= Dlg2Data_GetString   (IDC_ORGARTISTNAME,     SJ_TI_ORGARTISTNAME,    m_dataStat.m_orgArtistName);
	changes |= Dlg2Data_GetString   (IDC_COMPOSERNAME,      SJ_TI_COMPOSERNAME,     m_dataStat.m_composerName);
	changes |= Dlg2Data_GetString   (IDC_ALBUMNAME,         SJ_TI_ALBUMNAME,        m_dataStat.m_albumName);
	changes |= Dlg2Data_GetString   (IDC_COMMENT,           SJ_TI_COMMENT,          m_dataStat.m_comment);
	changes |= Dlg2Data_GetLong     (IDC_TRACKNR,           SJ_TI_TRACKNR,          m_dataStat.m_trackNr);
	changes |= Dlg2Data_GetLong     (IDC_TRACKCOUNT,        SJ_TI_TRACKCOUNT,       m_dataStat.m_trackCount);
	changes |= Dlg2Data_GetLong     (IDC_DISKNR,            SJ_TI_DISKNR,           m_dataStat.m_diskNr);
	changes |= Dlg2Data_GetLong     (IDC_DISKCOUNT,         SJ_TI_DISKCOUNT,        m_dataStat.m_diskCount);
	changes |= Dlg2Data_GetString   (IDC_GENRENAME,         SJ_TI_GENRENAME,        m_dataStat.m_genreName);
	changes |= Dlg2Data_GetString   (IDC_GROUPNAME,         SJ_TI_GROUPNAME,        m_dataStat.m_groupName);
	changes |= Dlg2Data_GetLong     (IDC_YEAR,              SJ_TI_YEAR,             m_dataStat.m_year);
	changes |= Dlg2Data_GetLong     (IDC_BPM,               SJ_TI_BEATSPERMINUTE,   m_dataStat.m_beatsPerMinute);
	changes |= Dlg2Data_GetLong     (IDC_RATING,            SJ_TI_RATING,           m_dataStat.m_rating);

	m_dataStat.m_validFields = changes;

	return TRUE;
}


void SjTagEditorDlg::Dlg2Data_ModifyTrackInfo(SjTrackInfo& ti, int index, SjModifyInfo& mod)
{
	int i;
	for( i = 0; i < 31; i++ )
	{
		if( m_dataStat.m_validFields & (1<<i) )
		{
			mod.Add(1<<i, m_dataStat.GetValue(1<<i));
		}
	}
}


void SjTagEditorDlg::Dlg2Data_CopyAll(SjTagEditorPlugin* plugin, bool& reloadData, bool& canceled)
{
	// prepare modifiy

	SjTrackInfo backupedStat = m_dataStat;
	reloadData               = FALSE;
	canceled                 = FALSE;

	SjLibraryModule* lib = g_mainFrame->m_libraryModule;
	if( lib == NULL
	 || !Dlg2Data_PrepareModify()
	 || (plugin && !plugin->PrepareModify()) )
	{
		m_dataStat = backupedStat;
		canceled = TRUE;
		return;
	}


	// go through all data and collect the modification information

	SjModifyInfo mod;

	long i, urlCount = m_dataUrls.GetCount();

	{
		wxBusyCursor busy;
		SjTrackInfo ti;
		for( i = 0; i < urlCount; i++ )
		{
			ti.Clear();
			if( lib->GetTrackInfo(m_dataUrls[i], ti, SJ_TI_FULLINFO, FALSE) )
			{
				//  EVTL.       Wenn die Trackinformationen aus dem Dateinamen und nicht aus den
				//  TODO:       ID3-Tags gelesen wurden, ID3 aber geschrieben werden soll,
				//              sollten wir überlegen, hier auch mit den ID3-Tags vergleichen.
				//
				//              Ansonsten wird im Zweifelsfalle nämlich nichts in die ID3-Tags
				//              da in der Datenbank ja schon die richtigen Informationen aus dem
				//              Dateinamen stehen ...
				//
				//              Für Silverjuke selbst ist das nicht wirklich tragisch,
				//              da sich die Informationen aus dem Dateinamen im Kombination mit ID3
				//              rekonstruieren lassen.  Nur der Tag ist halt im Zweifelsfalle
				//              unvollständig.
				//
				//              Aufgefallen ist mir dieses Verhalten, als ich von last.fm
				//              Dateien aus dem dem Browsercache gerippt haben, die komplett keine
				//              Tags enthalten (ungewöhnlich).  Das erste Einlesen in Silverjuke
				//              klappte dann über den Dateinamen - nur änderungen wollten nicht
				//              in den Tags landen, da aus sich dieses Vergleiches ja alles schon
				//              "schön" war ...
				//
				//  CONTRA:     Ein ziemlicher Overhead, wenn z.B. 2000 Titel gleichzeitig
				//              bearbeitet werden sollen ...
				//
				//  WORKAROUND: Man muß in diesen Fällen nur dafür sorgen, daß irgendein _anderes_
				//              Feld sich ändert, z.B. die Anzahl der Titel, dann werden die Tags
				//              _komplett_ neu erzeugt.
				//              NEIN: Das stimmt so nicht, es werden tatsächlich nur die geänderten
				//              Informationen geschrieben

				mod.SetScope(ti);

				Dlg2Data_ModifyTrackInfo(ti, i, mod);

				if( plugin )
				{
					plugin->ModifyTrackInfo(ti, i, mod);
				}
			}
		}

		wxString postModifyMsg;
		if( plugin )
		{
			postModifyMsg = plugin->PostModify();
		}

		// any modifications?
		// if there are no modifications, we return TRUE as everything is in sync.
		// (same as if we have written the data successfully)

		if( mod.GetCount() <= 0 )
		{
			if( plugin && !postModifyMsg.IsEmpty() )
			{
				SjMessageBox(postModifyMsg, plugin->GetTitle(), wxOK|wxICON_INFORMATION, this);
				canceled = TRUE;
			}

			m_dataStat = backupedStat;
			return;
		}
	}

	// let the user confirm the modification

	{
		bool askWriteId3, askDelEmptyDir, onlyUrlsModified;
		if( mod.LetConfirm(askWriteId3, askDelEmptyDir, onlyUrlsModified)
		 || urlCount > 1
		 || plugin )
		{
			SJ_WINDOW_DISABLER(this);
			SjConfirmDlg dlg(this, mod, false /*was: askWriteId3, but this is confusing, as the option is already in the menu*/, askDelEmptyDir, onlyUrlsModified);
			if( dlg.ShowModal() != wxID_OK )
			{
				m_dataStat = backupedStat;
				canceled = TRUE;
				return;
			}
		}
	}

	// any modifications left?
	// (the user may modify and delete modifications in the confirmation dialog)
	// this is no "cancel" as the is the same as if the user hasn't initiated any modifications at
	// all.

	if( mod.GetCount() <= 0 )
	{
		m_dataStat = backupedStat;
		return;
	}

	// write the modifications

	m_possiblyEmptyDirUrls.Clear();

	{
		long                modCount = mod.GetCount(); // get again as it may be changed by the user in the confirmation dialog
		SjModifyItem*       modItem;
		wxString            lastUrl;
		SjTrackInfo         ti;

		long                updateStartingTime = wxDateTime::Now().GetAsDOS();

		wxBusyCursor*       busyCursor = new wxBusyCursor;
		SjBusyInfo*         busyInfo = NULL;
		bool                updateGenres = FALSE,
		                    updateGroups = FALSE,
		                    updateAlbums = FALSE;

		// write all modifications
		wxArrayString oldGenres = lib->GetUniqueValues(SJ_TI_GENRENAME);
		wxArrayString oldGroups = lib->GetUniqueValues(SJ_TI_GROUPNAME);
		long changedFields = 0;
		for( i = 0; i < modCount; i++ )
		{
			// get mod. item
			modItem = mod.GetItem(i);

			// get track information
			if( modItem->GetUrl() != lastUrl )
			{
				if( !lastUrl.IsEmpty() )
				{
					Data2Dsk_Write(lastUrl, ti, updateAlbums);
					lastUrl.Empty();
				}

				ti.Clear();
				if( !lib->GetTrackInfo(modItem->GetUrl(), ti, SJ_TI_FULLINFO, FALSE) )
				{
					continue;
				}

				ti.m_timeModified = updateStartingTime;
				lastUrl = modItem->GetUrl();

				if( !SjBusyInfo::Set(lastUrl, i==0) )
				{
					canceled = TRUE;
					break; // user abort
				}
			}

			// apply the modification
			ti.SetValue(modItem->GetField(), modItem->GetNewVal());
			ti.m_validFields |= modItem->GetField();
			changedFields |= modItem->GetField();

			// check what was modified
			if( !updateGenres )
			{
				if( changedFields&SJ_TI_GENRENAME )
				{
					if( lib->m_flags&(SJ_LIB_CREATEALBUMSBY_GENRE|SJ_LIB_CREATEALBUMSBY_DIR) )
					{
						updateAlbums = TRUE;
					}

					if( oldGenres.Index(ti.m_genreName) == wxNOT_FOUND )
					{
						updateGenres = TRUE;
					}
				}
			}

			if( !updateGroups )
			{
				if( changedFields&SJ_TI_GROUPNAME )
				{
					if( oldGroups.Index(ti.m_groupName) == wxNOT_FOUND )
					{
						updateGroups = TRUE;
					}
				}
			}

			if( !updateAlbums )
			{
				if( changedFields&(SJ_TI_LEADARTISTNAME|SJ_TI_ALBUMNAME|SJ_TI_YEAR) )
				{
					updateAlbums = TRUE;
				}
			}

			// switch from busy cursor to busy info
			// if the update process seems to take a while
			if( busyInfo == NULL )
			{
				if( updateAlbums || updateGroups || updateGenres || urlCount > 1 )
				{
					delete busyCursor;
					busyCursor = NULL;
					busyInfo = new SjBusyInfo(this,
					                          _("Updating music library"),
					                          TRUE,
					                          _("If you cancel the update process, your music library may not be up to date.\n\nCancel the update process?"));
				}
			}
		}

		if( !lastUrl.IsEmpty() )
		{
			Data2Dsk_Write(lastUrl, ti, updateAlbums);
		}

		// update the rest

		if( updateAlbums )
		{
			// if we do not end the search and the music selection,
			// the search will be inconsistent after the update --
			// it may be possible to reset if afterwards,
			// but this is more complicated.
			g_mainFrame->EndAllSearch();

			lib->CombineTracksToAlbums();
		}

		if( updateGenres )
		{
			lib->UpdateUniqueValues(wxT("genrename"));
		}

		if( updateGroups )
		{
			lib->UpdateUniqueValues(wxT("groupname"));
		}

		// update view

		if( updateAlbums || updateGenres )
		{
			g_mainFrame->m_browser->ReloadColumnMixer(); // reload needed as there may be more or less albums in the view now!
		}
		else
		{
			g_mainFrame->m_browser->RefreshAll();
		}

		g_mainFrame->OnUrlChangingDone();

		// delete possibly empty directories,
		if( g_tagEditorModule->GetDelEmptyDir() )
		{
			wxString dirUrl;
			SjHashIterator iterator;
			while( m_possiblyEmptyDirUrls.Iterate(iterator, dirUrl) )
			{
				dirUrl = SjTools::EnsureTrailingSlash(dirUrl)+"dummy.file";
				wxString dir = wxFileSystem::URLToFileName(dirUrl).GetPath(wxPATH_GET_VOLUME|wxPATH_GET_SEPARATOR), file;

				{
					wxLogNull null;
					file = ::wxFindFirstFile(dir+"*.*");
				}

				if( file.IsEmpty() )
				{
					::wxRmdir(dir);
				}
			}
		}

		// cleanup

		if( busyInfo )
		{
			delete busyInfo;
		}
		else if( busyCursor )
		{
			delete busyCursor;
		}
	}

	// done
	reloadData = TRUE;
	return;
}


/*******************************************************************************
 *  SjTagEditorDlg - Transferring data -> disk
 ******************************************************************************/


void SjTagEditorDlg::RenameDone__(const wxString& oldUrl, const wxString& newUrl)
{
	// "real" rename= (if newUrl is empty, this function just informs the
	// belonging instances about the modified data)
	if( !newUrl.IsEmpty() )
	{
		wxSqlt sql;

		wxString oldPathUrl, newPathUrl;
		SjTools::GetFileNameFromUrl(oldUrl, &oldPathUrl);
		SjTools::GetFileNameFromUrl(newUrl, &newPathUrl);

		if( oldPathUrl != newPathUrl )
		{
			// move all belonging covers if they're are also in the old directory
			sql.Query(wxT("SELECT artids FROM tracks WHERE url='") + sql.QParam(oldUrl) + wxT("';"));
			if( sql.Next() )
			{
				const char* p = sql.GetAsciiPtr(0);
				while( p && *p )
				{
					long currArtId = atol(p);
					sql.Query(wxString::Format(wxT("SELECT url FROM arts WHERE id=%lu;"), currArtId));
					if( sql.Next() )
					{
						wxString oldArtUrl = sql.GetString(0);
						wxString oldArtPath; SjTools::GetFileNameFromUrl(oldArtUrl, &oldArtPath);
						if( oldArtPath == oldPathUrl )
						{
							wxString newArtUrl = newPathUrl + SjTools::GetFileNameFromUrl(oldArtUrl);

							// rename or copy the art file
							if( wxFileExists(oldArtUrl) )
							{
								if( !wxRename(oldArtUrl, newArtUrl) )
								{
									wxCopyFile(oldArtUrl, newArtUrl, FALSE);
								}
							}

							// if the new file exists, update the database (we do the rename
							// and the existance check in two steps as the file may be renames
							// the the beloning to another track)
							if( wxFileExists(newArtUrl) )
							{
								sql.Query(wxT("UPDATE arts SET url='") + sql.QParam(newArtUrl) + wxT("' WHERE id=") + sql.UParam(currArtId) + wxT(";"));
							}
						}
					}

					// next art
					p = strchr(p, ' ');
					if( p ) p++;
				}
			}
		}

		// mark the old directory for deletion
		m_possiblyEmptyDirUrls.Insert(oldPathUrl, 1);

		// update database
		sql.Query(wxT("UPDATE tracks SET url='") + sql.QParam(newUrl) + wxT("' WHERE url='") + sql.QParam(oldUrl) + wxT("';"));
	}

	// inform the main frame about the change --
	// the main frame will call all other instances that should be informed
	// (esp. the player)
	g_mainFrame->OnUrlChanged(oldUrl, newUrl);

	// inform the derived class
	// (OnUrlChanged() is a virtual function)
	OnUrlChanged(oldUrl, newUrl);
}


bool SjTagEditorDlg::RenameFile__(const wxString& oldUrl, const wxString& newUrl)
{
	// The caller should print the error
	wxLogNull null;

	// convert the given URLs to filenames
	wxFileName oldFileName = wxFileSystem::URLToFileName(oldUrl);
	wxFileName newFileName = wxFileSystem::URLToFileName(newUrl);

	// If the dest. url exists, there is currently nothing we can do
	if( newFileName.FileExists() )
	{
		return FALSE;
	}

	// Normal system call
	if( wxRename(oldFileName.GetFullPath(), newFileName.GetFullPath()) == 0 )
	{
		RenameDone__(oldUrl, newUrl);
		return TRUE;  // success
	}

	// check if all needed directories exist; if not, create them
	{
		wxFileName fn2(newFileName.GetPath(wxPATH_GET_VOLUME|wxPATH_GET_SEPARATOR));
		fn2.Mkdir(0777, wxPATH_MKDIR_FULL);
	}

	// Normal system call, second try with dirs created
	if( wxRename(oldFileName.GetFullPath(), newFileName.GetFullPath()) == 0 )
	{
		RenameDone__(oldUrl, newUrl);
		return TRUE;  // success
	}

	// Try to copy
	if (wxCopyFile(oldFileName.GetFullPath(), newFileName.GetFullPath()))
	{
		if( wxRemoveFile(oldFileName.GetFullPath()) )
		{
			RenameDone__(oldUrl, newUrl);
			return TRUE;  // success
		}
		else
		{
			wxRemoveFile(newFileName.GetFullPath());
		}
	}

	// Give up.
	return FALSE;
}


void SjTagEditorDlg::WaitForWriteAccess__(const wxString& url)
{
	unsigned long   startWaiting = SjTools::GetMsTicks();
	bool            hasWriteAccess = false;

	while( !hasWriteAccess && SjTools::GetMsTicks()-startWaiting < 4000 )
	{
		// sleep a little moment
		wxThread::Sleep(150);

		// try to access the file
		SjByteFile byteFile(url, NULL /*force write access*/);
		if( !byteFile.ReadOnly() )
		{
			hasWriteAccess = true;
		}
	}
}


bool SjTagEditorDlg::Data2Dsk_Write(const wxString& orgUrl, SjTrackInfo& ti, bool& updateAlbums)
{
	SjLibraryModule* lib = g_mainFrame->m_libraryModule;

	// Stop the player?
	// This is needed if the file is renamed or ID3 tags should be written.
	bool stopped = FALSE;
	long stoppedPos = 0;
	if( !g_mainFrame->IsStopped()
	 &&  g_mainFrame->GetQueueUrl(-1) == orgUrl
	 && (orgUrl != ti.m_url || g_tagEditorModule->GetWriteId3Tags()) )
	{
		stoppedPos = g_mainFrame->GetElapsedTime();
		g_mainFrame->Stop(false);
		stopped = TRUE;

		// wait a little moment until the track is faded out and stopped
		WaitForWriteAccess__(orgUrl);
	}

	// rename the file?
	if( orgUrl != ti.m_url )
	{
		// rename the file.
		wxASSERT( ti.m_validFields & SJ_TI_URL );

		if( !RenameFile__(orgUrl, ti.m_url) )
		{
			wxString from = orgUrl;
			if( from.StartsWith("file:") ) { from = wxFileSystem::URLToFileName(from).GetFullPath(); }

			wxString to = ti.m_url;
			if( to.StartsWith("file:") ) { to = wxFileSystem::URLToFileName(to).GetFullPath(); }

			wxLogError(_("Cannot rename \"%s\" to \"%s\"."),
			           from.c_str(), to.c_str());
			ti.m_url = orgUrl;
		}
		ti.m_validFields &= ~SJ_TI_URL;

		if( !updateAlbums )
		{
			if( lib->m_flags&SJ_LIB_CREATEALBUMSBY_DIR )
			{
				updateAlbums = TRUE;
			}
		}

		// anything more to do?
		if( ti.m_validFields == 0 )
		{
			goto Data2Dsk_Write_Done; // no, we're succeeded
		}
	}

	// write the data
	lib->WriteTrackInfo(&ti, ti.m_id, FALSE/*don't write art ids (not loaded)*/);

	if( g_tagEditorModule->GetWriteId3Tags() )
	{
		SjScannerModule* scannerModule = g_mainFrame->m_moduleSystem.FindScannerModuleByUrl(ti.m_url);
		if( scannerModule )
		{
			wxASSERT( scannerModule->IsLoaded() );

			scannerModule->SetTrackInfo(ti.m_url, ti);
		}
	}

	// RenameDone__() with no new url set will
	// just inform the needed instances about the modified data.
	RenameDone__(ti.m_url, wxT(""));

	// done so far -- restart player, if stopped
Data2Dsk_Write_Done:

	if( stopped )
	{
		g_mainFrame->Play(stoppedPos);
	}

	return TRUE;
}


/*******************************************************************************
 *  SjTagEditorModule
 ******************************************************************************/


SjTagEditorModule* g_tagEditorModule = NULL;


SjTagEditorModule::SjTagEditorModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	m_file                  = wxT("memory:tageditor.lib");
	m_name                  = _("Edit track");
	m_selPage               = 0;
	m_dlg                   = NULL;
}


bool SjTagEditorModule::FirstLoad()
{
	g_tagEditorModule = this;
	return TRUE;
}


void SjTagEditorModule::LastUnload()
{
	CloseTagEditor();
}


void SjTagEditorModule::SetWriteId3Tags(bool writeId3Tags)
{
	if( writeId3Tags != m_writeId3Tags )
	{
		m_writeId3Tags = writeId3Tags;
		g_tools->m_config->Write(wxT("tageditor/writeId3"), m_writeId3Tags? 1L : 0L);
	}
}


void SjTagEditorModule::SetDelEmptyDir(bool delEmptyDir)
{
	if( delEmptyDir != m_delEmptyDir )
	{
		m_delEmptyDir = delEmptyDir;
		g_tools->m_config->Write(wxT("tageditor/delEmptyDir"), m_delEmptyDir? 1L : 0L);
	}
}


void SjTagEditorModule::OpenTagEditor(SjTagEditorDlg* newTagEditorDlg)
{
	// load settings
	m_writeId3Tags  = (g_tools->m_config->Read(wxT("tageditor/writeId3"), 1L)!=0);
	m_delEmptyDir   = (g_tools->m_config->Read(wxT("tageditor/delEmptyDir"), 1L)!=0);

	// delete old editor
	CloseTagEditor();

	// show new editor
	m_dlg = newTagEditorDlg;

	if( m_dlg->m_dlgNotebook
	 && m_selPage >= 0
	 && m_selPage < (int)m_dlg->m_dlgNotebook->GetPageCount() )
	{
		m_dlg->m_dlgNotebook->SetSelection(m_selPage);
	}

	m_dlg->CenterOnParent();

	if( m_dlg->Init__() )
	{
		m_dlg->Show();
	}
	else
	{
		// error should be logged in Init__()
		m_dlg->Destroy();
		m_dlg = NULL;
	}
}


void SjTagEditorModule::CloseTagEditor()
{
	if( m_dlg )
	{
		// hide
		m_dlg->Hide();

		// save position
		if( m_dlg->m_dlgNotebook )
		{
			m_selPage = m_dlg->m_dlgNotebook->GetSelection();
		}

		m_dlg->Destroy();
		m_dlg = NULL;
	}
}


void SjTagEditorModule::GetLittleOptions(SjArrayLittleOption& lo)
{
	SjLittleOption::SetSection(wxT("Freedb"));
	lo.Add(new SjLittleStringSel(_("Server name"), NULL, SJ_FREEDB_DEF_HOST, wxT("tageditor/freedbHost"), SJ_ICON_LITTLEDEFAULT, FALSE));
}


void SjTagEditorModule::ReceiveMsg(int msg)
{
	if( msg == IDMODMSG_KIOSK_STARTING
	 || msg == IDMODMSG_WINDOW_CLOSE )
	{
		CloseTagEditor();
	}
}

