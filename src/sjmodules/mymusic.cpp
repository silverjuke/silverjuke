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
 * File:    mymusic.cpp
 * Authors: Björn Petersen
 * Purpose: "Music library" settings
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/notebook.h>
#include <sjmodules/mymusic.h>
#include <sjmodules/help/help.h>


// don't use IDs with IDM_FIRSTPRIVATE here as they will conflict
// with the loaded pages!


/*******************************************************************************
 * SjMyMusicConfigPage
 ******************************************************************************/


class SjSettingsSourceItem
{
public:
	SjSettingsSourceItem(SjScannerModule* m, long index, const wxString& url, SjIcon icon)
	{
		m_scannerModule = m;
		m_index = index;
		m_url = url;
		m_icon = icon;
	}

	SjScannerModule*    GetScannerModule() const { return m_scannerModule; };
	long                GetIndex        () const { return m_index; }
	wxString            GetUrl          () const { return m_url; }
	SjIcon              GetIcon         () const { return m_icon; }

private:
	SjScannerModule*    m_scannerModule;
	long                m_index;
	wxString            m_url;
	SjIcon              m_icon;
};


WX_DECLARE_LIST(SjSettingsSourceItem, SjSettingsSourceItemList);
#include <wx/listimpl.cpp>
WX_DEFINE_LIST(SjSettingsSourceItemList);


class SjMyMusicConfigPage : public wxPanel
{
public:
	                SjMyMusicConfigPage (SjMyMusicModule* module, wxWindow* parent, int selectedPage);

	bool            m_idxChanged;

private:
	wxPanel*        CreatePage          (wxWindow* parent);
	void            InitPage            (const wxString& selSourceName);
	SjSettingsSourceItem* GetSelFromDialog ();
	void            OnColClick          (wxListEvent&);
	void            OnSelectionChange   (wxListEvent&) { UpdateButtons(); }
	void            OnDoubleClick       (wxListEvent&);
	void            OnContextMenu       (wxListEvent&) { ShowContextMenu(this, ScreenToClient(::wxGetMousePosition())); }
	void            OnListKeyDown       (wxListEvent&);
	void            OnUpdateContextMenu (wxCommandEvent&) { ShowContextMenu(m_updateButton, wxPoint(0,0)); }
	void            OnOptionsContextMenu(wxCommandEvent&) { ShowContextMenu(m_configMenuButton, wxPoint(0,0)); }
	void            OnAddSourceContextMenu(wxCommandEvent&) { ShowContextMenu(m_addButton, wxPoint(0,0)); }
	void            ShowContextMenu     (wxWindow*, const wxPoint&);
	void            OnAddSources        (wxCommandEvent&);
	void            OnDelSource         (wxCommandEvent&);
	void            OnConfigSource      (wxCommandEvent&);
	void            OnExplore           (wxCommandEvent&);
	void            OnUpdate            (wxCommandEvent&);
	void            OnSize              (wxSizeEvent&);

	SjSettingsSourceItemList m_listOfSources;
	int             m_currSortCol;

	wxNotebook*     m_notebook;
	wxListCtrl*     m_listCtrl;
	wxButton*       m_updateButton;

	wxButton*       m_configMenuButton;
	wxButton*       m_addButton;
	wxButton*       m_removeButton;
	void            UpdateButtons       ();

	DECLARE_EVENT_TABLE ();

	friend class    SjMyMusicModule;
};


BEGIN_EVENT_TABLE(SjMyMusicConfigPage, wxPanel)
	EVT_SIZE                    (                       SjMyMusicConfigPage::OnSize             )
	EVT_LIST_COL_CLICK          (IDC_IDXLIST,           SjMyMusicConfigPage::OnColClick         )
	EVT_LIST_ITEM_SELECTED      (IDC_IDXLIST,           SjMyMusicConfigPage::OnSelectionChange  )
	EVT_LIST_ITEM_DESELECTED    (IDC_IDXLIST,           SjMyMusicConfigPage::OnSelectionChange  )
	EVT_LIST_ITEM_ACTIVATED     (IDC_IDXLIST,           SjMyMusicConfigPage::OnDoubleClick      )
	EVT_LIST_ITEM_RIGHT_CLICK   (IDC_IDXLIST,           SjMyMusicConfigPage::OnContextMenu      )
	EVT_LIST_KEY_DOWN           (IDC_IDXLIST,           SjMyMusicConfigPage::OnListKeyDown      )
	EVT_BUTTON                  (IDC_IDXUPDATEMENU,     SjMyMusicConfigPage::OnUpdateContextMenu)
	EVT_BUTTON                  (IDC_IDXADDSOURCES,     SjMyMusicConfigPage::OnAddSourceContextMenu )
	EVT_MENU_RANGE              (IDC_MODULE00, IDC_MODULE99, SjMyMusicConfigPage::OnAddSources  )
	EVT_BUTTON                  (IDC_IDXDELSOURCE,      SjMyMusicConfigPage::OnDelSource        )
	EVT_MENU                    (IDC_IDXDELSOURCE,      SjMyMusicConfigPage::OnDelSource        )
	EVT_BUTTON                  (IDC_IDXCONFIGSOURCEMENU,SjMyMusicConfigPage::OnOptionsContextMenu)
	EVT_MENU                    (IDC_IDXCONFIGSOURCE,   SjMyMusicConfigPage::OnConfigSource     )
	EVT_MENU                    (IDM_EXPLORE,           SjMyMusicConfigPage::OnExplore          )
	EVT_BUTTON                  (IDT_UPDATE_INDEX,      SjMyMusicConfigPage::OnUpdate           )
	EVT_MENU                    (IDT_UPDATE_INDEX,      SjMyMusicConfigPage::OnUpdate           )
	EVT_MENU                    (IDT_DEEP_UPDATE_INDEX, SjMyMusicConfigPage::OnUpdate           )
END_EVENT_TABLE()


static int wxCALLBACK ListCtrlCompareFunction(long item1__, long item2__, long sortData)
{
	int                         ret = 0, toggleSort = sortData&0x00001000? 1 : 0;
	SjSettingsSourceItem        *source1 = (SjSettingsSourceItem*)item1__,
	                             *source2 = (SjSettingsSourceItem*)item2__;

	switch( sortData & ~0x00001000 )
	{
		case 0:
			ret = ::wxStricmp(source1->GetUrl(), source2->GetUrl());
			break;
	}

	if( toggleSort )
	{
		ret *= -1;
	}

	return ret;
}


wxPanel* SjMyMusicConfigPage::CreatePage(wxWindow* parent)
{
	wxPanel* page = new wxPanel(parent, -1);
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	page->SetSizer(sizer1);

	sizer1->Add(1, SJ_DLG_SPACE); // some space

	wxStaticText* staticText = new wxStaticText(page, -1, _("Read music-files from the following folders and sources:"));
	sizer1->Add(staticText, 0, wxALL, SJ_DLG_SPACE);

	m_listCtrl = new wxListCtrl(page, IDC_IDXLIST, wxPoint(-1, -1), wxSize(200, 80),
	                            wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxSUNKEN_BORDER
	                           );
	m_listCtrl->SetImageList(g_tools->GetIconlist(FALSE), wxIMAGE_LIST_SMALL);
	//m_listCtrl->SetImageList(g_tools->GetIconlist(TRUE), wxIMAGE_LIST_NORMAL);
	m_listCtrl->InsertColumn(0, _("Read music-files from the following sources:"));
	sizer1->Add(m_listCtrl, 1, wxGROW|wxLEFT|wxRIGHT|wxTOP, SJ_DLG_SPACE);

	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(buttonSizer, 0, wxRIGHT|wxGROW, SJ_DLG_SPACE);

	m_addButton = new wxButton(page, IDC_IDXADDSOURCES, _("Add source")+wxString(SJ_BUTTON_MENU_ARROW), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	buttonSizer->Add(m_addButton, 0, wxLEFT|wxTOP|wxBOTTOM, SJ_DLG_SPACE);

	m_removeButton = new wxButton(page, IDC_IDXDELSOURCE, _("Remove source"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	buttonSizer->Add(m_removeButton, 0, wxLEFT|wxTOP|wxBOTTOM, SJ_DLG_SPACE);

	m_configMenuButton = new wxButton(page, IDC_IDXCONFIGSOURCEMENU, _("Options")+wxString(SJ_BUTTON_MENU_ARROW), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	buttonSizer->Add(m_configMenuButton, 0, wxLEFT|wxTOP|wxBOTTOM, SJ_DLG_SPACE);

	m_updateButton = new wxButton(page, IDC_IDXUPDATEMENU, wxString::Format(wxT(" %s%s "), _("Update music library"), SJ_BUTTON_MENU_ARROW), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	buttonSizer->Add(m_updateButton, 0, wxALL, SJ_DLG_SPACE);

	// add supported files hint
	SjExtList supportedExt = g_mainFrame->m_moduleSystem.GetAssignedExt(SJ_EXT_MUSICFILES); // equals to `g_mainFrame->m_player.GetExtList()`

	wxArrayString arrayStr;
	for( int i = supportedExt.GetCount()-1; i >= 0; i-- ) {
		arrayStr.Add(supportedExt.GetExt(i));
	}
	arrayStr.Sort();

	wxString line = _("Supported file types") + ": ", text, ext;
	size_t i, i_cnt = arrayStr.GetCount();
	bool hasMp3 = false, hasOgg = false;
	for( i = 0; i < i_cnt; i++ )
	{
		ext = arrayStr.Item(i);
		line += ext;
		if( i != i_cnt-1 ) { line += ", "; }
		if( line.Len() > 75 ) { if( text.Len() ) { text += "\n"; } text += line; line = ""; }
		if( ext == "mp3" ) { hasMp3 = true; }
		if( ext == "ogg" ) { hasOgg = true; }
        if( hasMp3 && hasOgg && i != i_cnt-1 && i >= 70 ) { line += wxString::Format("%i", (int)(i_cnt-i-1)) + "  more"; break; }
	}
	if( line.Len() ) { if( text.Len() ) { text += "\n"; } text += line; }

	staticText = new wxStaticText(page, -1, text);
	sizer1->Add(staticText, 0, wxALL, SJ_DLG_SPACE);

	sizer1->Add(1, SJ_DLG_SPACE); // some space

	return page;
}


SjMyMusicConfigPage::SjMyMusicConfigPage(SjMyMusicModule* myMusicModule, wxWindow* parent, int selectedPage)
	: wxPanel(parent)
{
	// save given objects
	m_idxChanged        = FALSE;
	m_configMenuButton  = NULL;
	m_addButton         = NULL;
	m_removeButton      = NULL;
	m_currSortCol       = 0;

	// init dialog
	wxSizer* dialogSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(dialogSizer);

	m_notebook = new wxNotebook(this, IDC_NOTEBOOK, wxDefaultPosition, wxDefaultSize, 0/*wxCLIP_CHILDREN - problems with wxChoice/wxComboBox*/);
	wxNotebook* notebookSizer = m_notebook;

	m_notebook->AddPage(CreatePage(m_notebook),  _("Music library"));

	for( int type=0; type<=1; type++ )
	{
		SjModuleList* moduleList = g_mainFrame->m_moduleSystem.GetModules(type==0? SJ_MODULETYPE_COL : SJ_MODULETYPE_COMMON);
		SjModuleList::Node* moduleNode = moduleList->GetFirst();
		while( moduleNode )
		{
			SjCommonModule* commonModule = (SjCommonModule*)moduleNode->GetData();
			if( commonModule->EmbedTo() == SJ_EMBED_TO_MUSICLIB )
			{
				m_notebook->AddPage(commonModule->GetConfigPage(m_notebook, 0), commonModule->m_name);
			}

			moduleNode = moduleNode->GetNext();
		}
	}

	if( selectedPage < 0
	 || selectedPage >= (int)m_notebook->GetPageCount() )
	{
		selectedPage = 0;
	}

	m_notebook->SetSelection(selectedPage);

	dialogSizer->Add(notebookSizer, 1, wxALL|wxGROW, SJ_DLG_SPACE);

	dialogSizer->SetSizeHints(this);

	// init data
	InitPage(g_tools->m_config->Read(wxT("settings/selSource"), wxT("")));
}


void SjMyMusicConfigPage::OnSize(wxSizeEvent& event)
{
	wxSize size;

	size = m_listCtrl->GetClientSize();
	m_listCtrl->SetColumnWidth(0, size.x-8);
	SjDialog::EnsureSelListCtrlItemVisible(m_listCtrl);

	event.Skip(); // forward event to the next handler
}


void SjMyMusicConfigPage::UpdateButtons()
{
	bool enable = GetSelFromDialog()!=NULL;

	if( m_configMenuButton )
	{
		m_configMenuButton->Enable(enable);
	}

	if( m_removeButton )
	{
		m_removeButton->Enable(enable);
	}

	if( m_updateButton )
	{
		m_updateButton->SetLabel(wxString::Format(m_idxChanged? wxT("* %s%s") : wxT(" %s%s "), _("Update music library"), SJ_BUTTON_MENU_ARROW));
	}
}


SjSettingsSourceItem* SjMyMusicConfigPage::GetSelFromDialog()
{
	long i = SjDialog::GetSelListCtrlItem(m_listCtrl);
	if( i >= 0 )
	{
		return (SjSettingsSourceItem*)m_listCtrl->GetItemData(i);
	}

	return NULL;
}


void SjMyMusicConfigPage::OnColClick(wxListEvent& event)
{
	int newCol = event.GetColumn();

	if( m_currSortCol == newCol )
	{
		m_currSortCol = newCol | 0x00001000; /* toggle! */;
	}
	else
	{
		m_currSortCol = newCol;
	}

	m_listCtrl->SortItems(ListCtrlCompareFunction, m_currSortCol); // this may unselect Items on GTK, however, this should be fixed in wxWidgets/GTK
}


void SjMyMusicConfigPage::InitPage(const wxString& selSourceUrl)
{
	// create index list
	m_listOfSources.Clear();
	m_listOfSources.DeleteContents(TRUE);

	SjModuleList* list = g_mainFrame->m_moduleSystem.GetModules(SJ_MODULETYPE_SCANNER);
	wxASSERT(list);

	SjModuleList::Node* moduleNode = list->GetFirst();
	SjScannerModule*    scannerModule;
	while( moduleNode )
	{
		scannerModule = (SjScannerModule*)moduleNode->GetData();
		wxASSERT(scannerModule);
		wxASSERT(scannerModule->IsLoaded());

		long sourceCount = scannerModule->GetSourceCount();
		long currSourceIndex;
		for( currSourceIndex = 0; currSourceIndex < sourceCount; currSourceIndex++ )
		{
			SjSettingsSourceItem* item = new SjSettingsSourceItem(scannerModule, currSourceIndex, scannerModule->GetSourceUrl(currSourceIndex), scannerModule->GetSourceIcon(currSourceIndex));
			m_listOfSources.Append(item);
		}

		// next
		moduleNode = moduleNode->GetNext();
	}

	// get list control
	m_listCtrl->Freeze();
	m_listCtrl->DeleteAllItems();

	// go through all search directories
	SjSettingsSourceItem*           item;
	SjSettingsSourceItemList::Node* itemnode = m_listOfSources.GetFirst();
	int                             i = 0, new_i;
	bool                            anythingSelected = FALSE;
	wxString                        sourceNotes;
	while( itemnode )
	{
		item = itemnode->GetData();
		wxASSERT(item);

		wxListItem listitem;
		listitem.m_mask     = wxLIST_MASK_IMAGE | wxLIST_MASK_TEXT | wxLIST_MASK_DATA;
		listitem.m_itemId   = i;
		listitem.m_text     = item->GetUrl();
		listitem.m_data     = (long)item;
		listitem.m_image    = item->GetScannerModule()->GetSourceIcon(item->GetIndex());

		sourceNotes = item->GetScannerModule()->GetSourceNotes(item->GetIndex());
		if( !sourceNotes.IsEmpty() )
		{
			listitem.m_text.Append(wxT(" ("));
			listitem.m_text.Append(sourceNotes);
			listitem.m_text.Append(wxT(')'));
		}

		new_i = m_listCtrl->InsertItem(listitem);

		if( item->GetUrl() == selSourceUrl )
		{
			m_listCtrl->SetItemState(new_i, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
			anythingSelected = TRUE;
		}

		itemnode = itemnode->GetNext();
		i++;
	}

	if( !anythingSelected && i > 0 )
	{
		m_listCtrl->SetItemState(0, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED,
		                         wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
	}

	m_listCtrl->SortItems(ListCtrlCompareFunction, m_currSortCol); // this may unselect Items on GTK, however, this should be fixed in wxWidgets/GTK

	m_listCtrl->Thaw();

	UpdateButtons();
}


void SjMyMusicConfigPage::OnAddSources(wxCommandEvent& event)
{
	SjModuleList* list = g_mainFrame->m_moduleSystem.GetModules(SJ_MODULETYPE_SCANNER);
	wxASSERT(list);

	// find the correct module
	SjModuleList::Node* moduleNode = list->GetFirst();
	SjScannerModule*    scannerModule = NULL;
	int                 typeCount = 0, typeIndex = -1;
	while( moduleNode && typeIndex == -1 )
	{
		scannerModule = (SjScannerModule*)moduleNode->GetData();
		wxASSERT(scannerModule);
		wxASSERT(scannerModule->IsLoaded());

		size_t i;
		for( i = 0; i < scannerModule->m_addSourceTypes_.GetCount(); i++ )
		{
			if( IDC_MODULE00+typeCount == event.GetId() )
			{
				typeIndex = i; // exit outer loop
				break;
			}

			typeCount++;
		}

		// next
		moduleNode = moduleNode->GetNext();
	}

	// add by module
	if( typeIndex != -1 && scannerModule )
	{
		long newIndex;

		{
			wxWindow* topLevelWindow = SjDialog::FindTopLevel(this);
			wxWindowDisabler disabler(topLevelWindow);
			newIndex = scannerModule->AddSources(typeIndex, topLevelWindow);
		}

		if( newIndex != -1 )
		{
			InitPage(scannerModule->GetSourceUrl(newIndex));
			m_idxChanged = TRUE;
			UpdateButtons();
		}
	}
}


void SjMyMusicConfigPage::OnDelSource(wxCommandEvent& event)
{
	SjSettingsSourceItem* source = GetSelFromDialog();
	if( source )
	{
		wxWindowDisabler disabler(SjDialog::FindTopLevel(this));
		if( source->GetScannerModule()->DeleteSource(source->GetIndex(), SjDialog::FindTopLevel(this)) )
		{
			InitPage(wxT(""));
			m_idxChanged = TRUE;
			UpdateButtons();
		}
	}
}


void SjMyMusicConfigPage::OnConfigSource(wxCommandEvent& event)
{
	SjSettingsSourceItem* source = GetSelFromDialog();
	if( source )
	{
		wxWindowDisabler disabler(SjDialog::FindTopLevel(this));

		wxString sourceUrl = source->GetUrl(); // save as source gets invalid on InitPage()

		bool needsUpdate = source->GetScannerModule()->ConfigSource(source->GetIndex(), SjDialog::FindTopLevel(this));

		InitPage(sourceUrl);

		if( needsUpdate )
		{
			m_idxChanged = TRUE;
			UpdateButtons();
		}
	}
}


void SjMyMusicConfigPage::OnExplore(wxCommandEvent& event)
{
	SjSettingsSourceItem* source = GetSelFromDialog();
	if( source )
	{
		g_tools->ExploreUrl(source->GetUrl());
	}
}


void SjMyMusicConfigPage::OnDoubleClick(wxListEvent& event)
{
	wxCommandEvent fwd;
	OnConfigSource(fwd);
}


void SjMyMusicConfigPage::OnListKeyDown(wxListEvent& event)
{
	switch( event.GetKeyCode() )
	{
		case WXK_DELETE:
		case WXK_BACK:
		{
			QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDC_IDXDELSOURCE));
		}
		break;
	}
}


void SjMyMusicConfigPage::ShowContextMenu(wxWindow* window, const wxPoint& pt)
{
	SjMenu                  m(0);
	SjSettingsSourceItem*   source = GetSelFromDialog(); // may be NULL!

	if( window == this || window == m_addButton )
	{
		SjModuleList* list = g_mainFrame->m_moduleSystem.GetModules(SJ_MODULETYPE_SCANNER);
		wxASSERT(list);

		SjModuleList::Node* moduleNode = list->GetFirst();
		SjScannerModule*    scannerModule;
		int                 typeCount = 0;

		while( moduleNode )
		{
			scannerModule = (SjScannerModule*)moduleNode->GetData();
			wxASSERT(scannerModule);
			wxASSERT(scannerModule->IsLoaded());

			size_t i;
			wxASSERT( scannerModule->m_addSourceTypes_.GetCount() == scannerModule->m_addSourceIcons_.GetCount() );
			for( i = 0; i < scannerModule->m_addSourceTypes_.GetCount(); i++ )
			{
				m.Append(IDC_MODULE00+typeCount, scannerModule->m_addSourceTypes_.Item(i)+wxT("..."));

				typeCount++;
			}

			// next
			moduleNode = moduleNode->GetNext();
		}
	}

	if( window == this )
	{
		m.Append(IDC_IDXDELSOURCE, _("Remove source"));
		m.Enable(IDC_IDXDELSOURCE, source!=NULL);
	}

	if( window == this || window == m_configMenuButton )
	{
		if( m.GetMenuItemCount() )
		{
			m.AppendSeparator();
		}

		if( source )
		{
			m.Append(IDC_IDXCONFIGSOURCE, wxString::Format(_("Options for \"%s\""), SjTools::ShortenUrl(source->GetUrl()).c_str())+wxT("..."));
		}
		else
		{
			m.Append(IDC_IDXCONFIGSOURCE, _("Options..."));
			m.Enable(IDC_IDXCONFIGSOURCE, FALSE);
		}

		m.Append(IDM_EXPLORE);
		m.Enable(IDM_EXPLORE, source!=NULL);
	}

	if( window == this || window == m_updateButton )
	{
		if( m.GetMenuItemCount() )
		{
			m.AppendSeparator();
		}

		m.Append(IDT_UPDATE_INDEX);
		m.Append(IDT_DEEP_UPDATE_INDEX);
	}

	window->PopupMenu(&m, pt);
}


void SjMyMusicConfigPage::OnUpdate(wxCommandEvent& event)
{
	if( g_mainFrame->UpdateIndex(SjDialog::FindTopLevel(this), event.GetId()==IDT_UPDATE_INDEX? FALSE : TRUE) )
	{
		m_idxChanged = FALSE; // the index is up-to-date now
		UpdateButtons();
	}
}


/*******************************************************************************
 * SjMyMusicModule
 ******************************************************************************/


SjMyMusicModule::SjMyMusicModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	m_file          = wxT("memory:mymusic.lib");
	m_name          = _("Jukebox");
	m_guiIcon          = SJ_ICON_MUSICLIB;
	m_sort          = 10;
}


wxWindow* SjMyMusicModule::GetConfigPage(wxWindow* parent, int selectedPage)
{
	return new SjMyMusicConfigPage(this, parent, selectedPage);
}


void SjMyMusicModule::DoneConfigPage(wxWindow* configPage__, int doneAction__)
{
	SjMyMusicConfigPage* configPage = (SjMyMusicConfigPage*)configPage__;

	// apply/cancel this module
	if( configPage->m_idxChanged )
	{
		g_mainFrame->UpdateIndex(SjDialog::FindTopLevel(configPage), FALSE);
	}

	// apply/cancel all embedded modules
	int moduleIndex = 1;
	for( int type=0; type<=1; type++ )
	{
		SjModuleList* moduleList = g_mainFrame->m_moduleSystem.GetModules(type==0? SJ_MODULETYPE_COL : SJ_MODULETYPE_COMMON);
		SjModuleList::Node* moduleNode = moduleList->GetFirst();
		while( moduleNode )
		{
			SjCommonModule* commonModule = (SjCommonModule*)moduleNode->GetData();
			if( commonModule->EmbedTo() == SJ_EMBED_TO_MUSICLIB )
			{
				wxWindow* commonPage = configPage->m_notebook->GetPage(moduleIndex);
				if( commonPage )
				{
					commonModule->DoneConfigPage(commonPage, doneAction__);
				}
				moduleIndex++;
			}

			// next
			moduleNode = moduleNode->GetNext();
		}
	}
}
