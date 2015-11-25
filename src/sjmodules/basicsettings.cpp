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
 * File:    basicsettings.cpp
 * Authors: Björn Petersen
 * Purpose: The "Basic settings" module
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/msgbox.h>
#include <sjmodules/basicsettings.h>
#include <sjmodules/help/help.h>
#include <sjtools/imgthread.h>
#include <see_dom/sj_see.h>
#include <wx/notebook.h>

#define IDC_REGISTERWWW             (IDM_LASTPRIVATE-1)   // to avoid conflicts with embedded pages, we use "high" IDs
#define IDC_REGISTER                (IDM_LASTPRIVATE-2)
#define IDC_SEARCHPATHS             (IDM_LASTPRIVATE-3)
#define IDC_SEARCHPATHADD           (IDM_LASTPRIVATE-4)
#define IDC_SEARCHPATHREMOVE        (IDM_LASTPRIVATE-5)
#define IDC_LANGSEL                 (IDM_LASTPRIVATE-6)
#define IDC_SHOWSEARCHPATH          (IDM_LASTPRIVATE-16)
#define IDC_OPTIONLAST              (IDM_LASTPRIVATE-18)  // range end (remember, we're subtracting the offsets)
#define IDC_OPTIONFIRST             (IDM_LASTPRIVATE-180) // range start (remember, we're subtracting the offsets)
#define IDC_LOADMODULESMENU         (IDM_LASTPRIVATE-182)
#define IDC_LITTLELIST              (IDM_LASTPRIVATE-183)
#define IDC_LITTLEMENUBUTTON        (IDM_LASTPRIVATE-184)
#define IDC_OPTIONRESET             (IDM_LASTPRIVATE-185)
#define IDC_DUMMYINTERFACE          (IDM_LASTPRIVATE-186)

#define PAGE_SHORTCUTS      0 // the IDs 666, 667 and 668 may also be used for this
#define PAGE_FURTHEROPTIONS 1


/*******************************************************************************
 * SjSearchPathDlg
 ******************************************************************************/


class SjSearchPathDlg : public SjDialog
{
public:
	                SjSearchPathDlg     (wxWindow* parent, const wxArrayString& paths);
	wxArrayString   GetPaths            () const;

private:
	wxListCtrl*     m_searchPathListCtrl;
	wxButton*       m_removeButton;
	void            UpdateButtons       ();
	void            OnSearchPathAdd     (wxCommandEvent&);
	void            OnSearchPathRemove  (wxCommandEvent&);
	void            OnSearchPathSelectionChange (wxListEvent&) { UpdateButtons(); }
	void            OnContextMenu       (wxListEvent&);
	void            OnShowSearchPath    (wxCommandEvent&);
	void            OnSize              (wxSizeEvent&);
	                DECLARE_EVENT_TABLE ()
};


BEGIN_EVENT_TABLE(SjSearchPathDlg, SjDialog)
	EVT_BUTTON                  (IDC_SEARCHPATHADD,      SjSearchPathDlg::OnSearchPathAdd               )
	EVT_MENU                    (IDC_SEARCHPATHADD,      SjSearchPathDlg::OnSearchPathAdd               )
	EVT_BUTTON                  (IDC_SEARCHPATHREMOVE,   SjSearchPathDlg::OnSearchPathRemove            )
	EVT_MENU                    (IDC_SEARCHPATHREMOVE,   SjSearchPathDlg::OnSearchPathRemove            )
	EVT_SIZE                    (                        SjSearchPathDlg::OnSize                        )
	EVT_LIST_ITEM_SELECTED      (IDC_SEARCHPATHS,        SjSearchPathDlg::OnSearchPathSelectionChange   )
	EVT_LIST_ITEM_DESELECTED    (IDC_SEARCHPATHS,        SjSearchPathDlg::OnSearchPathSelectionChange   )
	EVT_LIST_ITEM_RIGHT_CLICK   (IDC_SEARCHPATHS,        SjSearchPathDlg::OnContextMenu                 )
	EVT_MENU                    (IDC_SHOWSEARCHPATH,     SjSearchPathDlg::OnShowSearchPath              )
END_EVENT_TABLE()


SjSearchPathDlg::SjSearchPathDlg(wxWindow* parent, const wxArrayString& paths)
	: SjDialog(parent, _("Search paths"), SJ_MODAL, SJ_RESIZEABLE_IF_POSSIBLE)
{
	// init dialog
	m_removeButton = NULL;

	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer1);

	wxSizer* sizer2 = new wxStaticBoxSizer(new wxStaticBox(this, -1, _("The search paths are used for skins, language files and modules.")), wxVERTICAL);
	sizer1->Add(sizer2, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	wxSizer* sizer3 = new wxBoxSizer(wxHORIZONTAL);
	sizer2->Add(sizer3, 1, wxALL|wxGROW, SJ_DLG_SPACE);

	m_searchPathListCtrl = new wxListCtrl(this, IDC_SEARCHPATHS, wxPoint(-1, -1), wxSize(500, 100),
	                                      wxLC_REPORT | wxLC_SINGLE_SEL | wxSUNKEN_BORDER | wxLC_NO_HEADER
	                                     );
	m_searchPathListCtrl->SetImageList(g_tools->GetIconlist(FALSE), wxIMAGE_LIST_SMALL);
	m_searchPathListCtrl->InsertColumn(0, _("Search paths"));

	int new_i;
	for( size_t i = 0; i < paths.GetCount(); i++ )
	{
		wxListItem listitem;
		listitem.m_mask     = wxLIST_MASK_IMAGE | wxLIST_MASK_TEXT;
		listitem.m_itemId   = i;
		listitem.m_text     = paths[i];
		listitem.m_image    = SJ_ICON_ANYFOLDER;
		new_i = m_searchPathListCtrl->InsertItem(listitem);
		if( i == 0 )
		{
			m_searchPathListCtrl->SetItemState(new_i, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
		}
	}
	sizer3->Add(m_searchPathListCtrl, 1, wxGROW|wxTOP|wxRIGHT, SJ_DLG_SPACE);

	wxSizer* sizer4 = new wxBoxSizer(wxVERTICAL);
	sizer3->Add(sizer4, 0, wxGROW|wxTOP, SJ_DLG_SPACE);

	wxButton* button = new wxButton(this, IDC_SEARCHPATHADD, _("Add..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer4->Add(button, 0, wxGROW|wxBOTTOM, SJ_DLG_SPACE);

	m_removeButton = new wxButton(this, IDC_SEARCHPATHREMOVE, _("Remove"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer4->Add(m_removeButton, 0, wxGROW|wxBOTTOM, SJ_DLG_SPACE);

	sizer2->Add(1, SJ_DLG_SPACE); // some space

	sizer1->Add(CreateButtons(SJ_DLG_OK_CANCEL), 0, wxGROW|wxLEFT|wxTOP|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	sizer1->SetSizeHints(this);
	CentreOnParent();

	UpdateButtons();

	// set accelerators
	SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_SETTINGS));
}


void SjSearchPathDlg::UpdateButtons()
{
	if( m_removeButton )
	{
		bool enable = FALSE;
		int index = GetSelListCtrlItem(m_searchPathListCtrl);
		if( index >= 0 && index < m_searchPathListCtrl->GetItemCount() )
		{
			wxString path = m_searchPathListCtrl->GetItemText(index);
			if( !g_tools->IsStaticSearchPath(path) )
			{
				enable = TRUE;
			}
		}

		m_removeButton->Enable(enable);
	}
}


void SjSearchPathDlg::OnSize(wxSizeEvent& event)
{
	wxSize size;
	SjDialog::OnSize(event);

	size = m_searchPathListCtrl->GetClientSize();
	m_searchPathListCtrl->SetColumnWidth(0, size.x);
	EnsureSelListCtrlItemVisible(m_searchPathListCtrl);
}


void SjSearchPathDlg::OnContextMenu(wxListEvent&)
{
	SjMenu m(0);

	m.Append(IDC_SEARCHPATHADD, _("Add..."));
	m.Append(IDC_SEARCHPATHREMOVE, _("Remove"));
	m.Enable(IDC_SEARCHPATHREMOVE, m_removeButton->IsEnabled());
	m.AppendSeparator();
	m.Append(IDC_SHOWSEARCHPATH, _("Show file"));

	PopupMenu(&m, ScreenToClient(::wxGetMousePosition()));
}


void SjSearchPathDlg::OnShowSearchPath(wxCommandEvent& event)
{
	wxASSERT(m_searchPathListCtrl);
	int index = GetSelListCtrlItem(m_searchPathListCtrl);
	if( index >= 0 && index < m_searchPathListCtrl->GetItemCount() )
	{
		wxString path = m_searchPathListCtrl->GetItemText(index);
		g_tools->ExploreUrl(path);
	}
}


void SjSearchPathDlg::OnSearchPathAdd(wxCommandEvent& event)
{
	wxDirDialog dirDialog(FindTopLevel(this), _("Please select the folder to add to the search paths."));
	if( dirDialog.ShowModal() != wxID_OK )
	{
		return; // nothing added
	}

	wxFileName newSearchPath(dirDialog.GetPath());
	newSearchPath.Normalize();
	if( !::wxDirExists(newSearchPath.GetFullPath()) )
	{
		return; // nothing added
	}

	int newIndex = m_searchPathListCtrl->GetItemCount();
	wxListItem      item;
	item.m_mask     = wxLIST_MASK_IMAGE | wxLIST_MASK_TEXT;
	item.m_itemId   = m_searchPathListCtrl->GetItemCount();
	item.m_text     = newSearchPath.GetFullPath();
	item.m_image    = SJ_ICON_ANYFOLDER;
	m_searchPathListCtrl->InsertItem(item);
	m_searchPathListCtrl->SetItemState(newIndex, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
	EnsureSelListCtrlItemVisible(m_searchPathListCtrl);
}


void SjSearchPathDlg::OnSearchPathRemove(wxCommandEvent& event)
{
	wxASSERT(m_searchPathListCtrl);
	int index = GetSelListCtrlItem(m_searchPathListCtrl);
	if( index >= 0 && index < m_searchPathListCtrl->GetItemCount() )
	{
		wxString path = m_searchPathListCtrl->GetItemText(index);
		if( !g_tools->IsStaticSearchPath(path) )
		{
			m_searchPathListCtrl->DeleteItem(index);
		}
	}
}


wxArrayString SjSearchPathDlg::GetPaths() const
{
	wxArrayString ret;

	int lbIndex, lbCount = m_searchPathListCtrl->GetItemCount();
	for( lbIndex = 0; lbIndex < lbCount; lbIndex++ )
	{
		ret.Add(m_searchPathListCtrl->GetItemText(lbIndex));
	}

	return ret;
}


class SjLittlePathList : public SjLittleOption
{
public:
	SjLittlePathList(const wxString& name, const wxArrayString& paths, const wxArrayString& defPaths, const wxString& ini, SjIcon icon=SJ_ICON_LITTLEDEFAULT)
		: SjLittleOption(name, icon)
	{
		m_paths = paths; m_defPaths = defPaths; m_ini = ini;
	}

	wxString GetDisplayValue() const
	{
		return SjTools::Implode(m_paths, wxT("; "));
	}

	bool IsModified() const
	{
		return SjTools::Implode(m_paths, wxT("; "))!=SjTools::Implode(m_defPaths, wxT("; "));
	}

	long GetOptionCount() const { return 1; }
	wxString GetOption(long i) const { return _("Edit..."); }
	bool OnOption(wxWindow* parent, long i) { return OnDoubleClick(parent); }
	bool OnDoubleClick(wxWindow* parent)
	{
		wxWindowDisabler disabler(SjDialog::FindTopLevel(parent));
		SjSearchPathDlg dlg(SjDialog::FindTopLevel(parent), m_paths);
		if( dlg.ShowModal() == wxID_OK )
		{
			m_paths = dlg.GetPaths();

			long userCount = 0;
			for( size_t i = 0; i < m_paths.GetCount(); i++ )
			{
				if( ::wxDirExists(m_paths[i])
				 && !g_tools->IsStaticSearchPath(m_paths[i]) )
				{
					g_tools->m_config->Write(wxString::Format(wxT("main/searchPath%i"), (int)userCount++), m_paths[i]);
				}
			}
			g_tools->m_config->Write(wxT("main/searchPathCount"), userCount);

			// reload search paths
			g_tools->InitSearchPaths();
		}
		return false; // other options are not affected
	}

	bool OnDefault(wxWindow* parent)
	{
		m_paths = m_defPaths;
		return false; // other options are not affected
	}

private:
	wxArrayString   m_paths, m_defPaths;
	wxString        m_ini;
};


/*******************************************************************************
 * SjFutherOptPanel
 ******************************************************************************/


enum SjPreselect
{
    SJ_PRESELECT_NONE,
    SJ_PRESELECT_NUMPAD,
    SJ_PRESELECT_ADDCREDIT
};



class SjFurtherOptPanel : public wxPanel
{
public:
					SjFurtherOptPanel   (wxWindow* parent, SjArrayLittleOption&, int page, SjPreselect);

private:

	wxArrayLong     GetSelectedOptions  ();
	SjLittleOption* GetSelectedOption   ();

	void            OnLittleDoubleClick (wxListEvent&);
	void            OnLittleOptionsMenu (wxCommandEvent&) { ShowLittleContextMenu(m_littleCustomizeButton, wxPoint(0, 0)); }
	void            OnLittleContextMenu (wxListEvent&) { ShowLittleContextMenu(this, ScreenToClient(::wxGetMousePosition())); }
	void            OnLittleOption      (wxCommandEvent&);
	void            OnLittleReset       (wxCommandEvent&);

	void            UpdateLittleOption  (SjLittleOption*);
	void            ShowLittleContextMenu (wxWindow*, const wxPoint&);
	wxListCtrl*     m_littleListCtrl;
	wxButton*       m_littleCustomizeButton;
	                DECLARE_EVENT_TABLE ()
};


BEGIN_EVENT_TABLE(SjFurtherOptPanel, wxPanel)
	EVT_LIST_ITEM_RIGHT_CLICK   (IDC_LITTLELIST,        SjFurtherOptPanel::OnLittleContextMenu )
	EVT_LIST_ITEM_ACTIVATED     (IDC_LITTLELIST,        SjFurtherOptPanel::OnLittleDoubleClick )
	EVT_BUTTON                  (IDC_LITTLEMENUBUTTON,  SjFurtherOptPanel::OnLittleOptionsMenu )
	EVT_COMMAND_RANGE           (IDC_OPTIONFIRST, IDC_OPTIONLAST, wxEVT_COMMAND_MENU_SELECTED, SjFurtherOptPanel::OnLittleOption      )
	EVT_MENU                    (IDC_OPTIONRESET,       SjFurtherOptPanel::OnLittleReset       )
END_EVENT_TABLE()


static int sortableIcon(SjIcon icon)
{
	switch( icon )
	{
		case SJ_ICON_MODULE:                                return -2000;
		default:                                            return  1000;
		case SJ_ICON_NORMALKEY: case SJ_ICON_SYSTEMKEY:     return  2000;
	}
}


static int wxCALLBACK LittleCompareFunction(long item1__, long item2__, long sortData)
{
	int             ret = 0;
	SjLittleOption* option1 = (SjLittleOption*)item1__;
	SjLittleOption* option2 = (SjLittleOption*)item2__;
	wxString        str1, str2;

	if( option1 && option2 )
	{
		// sort by type (by icon)
		ret = sortableIcon(option1->GetIcon()) - sortableIcon(option2->GetIcon());
		if( ret == 0 )
		{
			// sort by name
			str1 = option1->GetName();
			str1.Replace(wxT(":"), wxT("aaa"));
			str1 = SjNormaliseString(str1, SJ_NUM_SORTABLE);

			str2 = option2->GetName();
			str2.Replace(wxT(":"), wxT("aaa"));
			str2 = SjNormaliseString(str2, SJ_NUM_SORTABLE);

			ret = str1.Cmp(str2);
		}
	}

	return ret;
}


SjFurtherOptPanel::SjFurtherOptPanel(wxWindow* parent, SjArrayLittleOption& littleOptions, int page, SjPreselect preselectKeys)
	: wxPanel(parent, -1)
{
	//m_littleOptions = littleOptions;

	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	this->SetSizer(sizer1);

	sizer1->Add(1, SJ_DLG_SPACE); // some space

	if( page == PAGE_FURTHEROPTIONS )
	{
		wxStaticText* staticText = new wxStaticText(this, -1,
				_("The following settings are for experienced users only."));
		sizer1->Add(staticText, 0, wxALL, SJ_DLG_SPACE);
	}

	m_littleListCtrl = new wxListCtrl(this, IDC_LITTLELIST, wxDefaultPosition, wxSize(550, 180),
									  wxLC_REPORT | wxSUNKEN_BORDER);
	m_littleListCtrl->InsertColumn(0, _("Command or option"));
	m_littleListCtrl->InsertColumn(1, _("Setting"));
	m_littleListCtrl->SetImageList(g_tools->GetIconlist(FALSE), wxIMAGE_LIST_SMALL);
	sizer1->Add(m_littleListCtrl, 1/*grow*/, wxGROW|wxALL, SJ_DLG_SPACE);

	m_littleCustomizeButton = new wxButton(this, IDC_LITTLEMENUBUTTON, _("Customize")+wxString(SJ_BUTTON_MENU_ARROW));
	sizer1->Add(m_littleCustomizeButton, 0, wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	// ... add all options to the list control

	int i, iCount = littleOptions.GetCount();
	{
		wxListItem listitem;
		listitem.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_DATA;
		for( i = 0; i < iCount; i++ )
		{
			SjIcon icon = littleOptions[i].GetIcon();
			if( ( page == PAGE_SHORTCUTS      && (icon==SJ_ICON_NORMALKEY||icon==SJ_ICON_SYSTEMKEY) )
			 || ( page == PAGE_FURTHEROPTIONS && (icon!=SJ_ICON_NORMALKEY&&icon!=SJ_ICON_SYSTEMKEY) ) )
			{
				listitem.m_itemId   = i;
				listitem.m_data     = (long)&littleOptions[i];
				listitem.m_text     = littleOptions[i].GetName();
				m_littleListCtrl->InsertItem(listitem);
			}
		}

		UpdateLittleOption(NULL);
	}

	// ... sort listbox, init listbox selection
	m_littleListCtrl->SortItems(LittleCompareFunction, 0);
	if( preselectKeys != SJ_PRESELECT_NONE )
	{
		wxString searchFor = preselectKeys==SJ_PRESELECT_NUMPAD? _("Numpad") : _("Add credit");
		for( i = 0; i < iCount; i++ )
		{
			if( littleOptions[i].GetName().StartsWith(searchFor) )
			{
				long littleListSelection = m_littleListCtrl->FindItem(-1, (long)&littleOptions[i]);
				m_littleListCtrl->SetItemState(littleListSelection, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
				break;
			}
		}
	}

	m_littleListCtrl->SetColumnWidth(0, wxLIST_AUTOSIZE);
	m_littleListCtrl->SetColumnWidth(1, wxLIST_AUTOSIZE);
	SjDialog::EnsureSelListCtrlItemVisible(m_littleListCtrl);
}


wxArrayLong SjFurtherOptPanel::GetSelectedOptions()
{
	wxArrayLong ret;
	long i, iCount = m_littleListCtrl->GetItemCount();
	for( i = 0; i < iCount; i++ )
	{
		if( m_littleListCtrl->GetItemState(i, wxLIST_STATE_SELECTED) &  wxLIST_STATE_SELECTED )
		{
			ret.Add(m_littleListCtrl->GetItemData(i));
		}
	}
	return ret;
}


SjLittleOption* SjFurtherOptPanel::GetSelectedOption()
{
	wxArrayLong selectedOptions = GetSelectedOptions();
	return selectedOptions.GetCount()==1? (SjLittleOption*)(selectedOptions[0]) : NULL;
}


void SjFurtherOptPanel::UpdateLittleOption(SjLittleOption* updatePtr)
{
	int             i, iCount = m_littleListCtrl->GetItemCount();
	SjLittleOption* o;

	wxColour    colourWhite = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	wxColour    colourBlack = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	wxFont      normalFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
	if( g_accelModule->m_flags&SJ_ACCEL_USE_VIEW_FONT_IN_DLG )
	{
		normalFont = g_mainFrame->m_baseStdFont;
	}
	wxFont      boldFont = normalFont; if(boldFont.GetWeight()==wxBOLD) {boldFont.SetStyle(wxITALIC);} else {boldFont.SetWeight(wxBOLD);}

	for( i = 0; i < iCount; i++ )
	{
		o = (SjLittleOption*)m_littleListCtrl->GetItemData(i);
		if( updatePtr == NULL || updatePtr == o )
		{
			wxListItem li;
			bool isModified = o->IsModified();

			li.SetId(i);

			li.SetFont(isModified? boldFont : normalFont);
			li.SetBackgroundColour(colourWhite);
			li.SetTextColour(colourBlack);

			li.m_mask   = wxLIST_MASK_IMAGE;
			li.m_col    = 0;
			li.m_image  = o->GetIcon();
			m_littleListCtrl->SetItem(li);
			m_littleListCtrl->SetItem(i, 1, o->GetDisplayValue());
		}
	}
}


void SjFurtherOptPanel::ShowLittleContextMenu(wxWindow* window, const wxPoint& pt)
{
	SjMenu          m(0);
	wxArrayLong     selectedOptions = GetSelectedOptions();
	SjLittleOption* o;
	int             i;

	// options for the selected item
	if( selectedOptions.GetCount()==1 )
	{
		o = (SjLittleOption*)(selectedOptions[0]);
		if( o->GetOptionCount() )
		{
			wxString optStr;
			for( i=0; i<(int)o->GetOptionCount(); i++ )
			{
				optStr = o->GetOption(i);
				if( optStr.IsEmpty() )
				{
					m.AppendSeparator();
				}
				else
				{
					long checkable = o->IsOptionCheckable(i);
					     if( checkable==2 ) { m.AppendRadioItem (IDC_OPTIONFIRST+i, optStr); }
					else if( checkable==1 ) { m.AppendCheckItem (IDC_OPTIONFIRST+i, optStr); }
					else                    { m.Append          (IDC_OPTIONFIRST+i, optStr); }

					m.Enable(IDC_OPTIONFIRST+i, o->IsOptionEnabled(i));

					if( checkable )
						m.Check(IDC_OPTIONFIRST+i, o->IsOptionChecked(i));
				}
			}

			m.AppendSeparator();
		}
	}

	// reset option
	bool anythingResetable = FALSE;
	for( i = 0; i < (int)selectedOptions.GetCount(); i++ )
	{
		o = (SjLittleOption*)(selectedOptions[i]);
		if(  o->IsModified()
		        && !o->IsReadOnly() )  // even read-only options may be modified, eg. by the command-line
		{
			anythingResetable = TRUE;
			break;
		}
	}

	m.Append(IDC_OPTIONRESET,  wxString(_("Reset selection")) + (selectedOptions.GetCount()>1? wxT("...") : wxT("")));
	m.Enable(IDC_OPTIONRESET, anythingResetable);

	// show menu
	window->PopupMenu(&m, pt);
}


void SjFurtherOptPanel::OnLittleDoubleClick(wxListEvent&)
{
	SjLittleOption* o = GetSelectedOption();
	if( o )
	{
		if( o->ShowMenuOnDoubleClick() )
		{
			ShowLittleContextMenu(this, ScreenToClient(::wxGetMousePosition()));
		}
		else
		{
			bool othersAffected = o->OnDoubleClick(SjDialog::FindTopLevel(this));
			UpdateLittleOption(othersAffected? NULL : o);
		}
	}
}


void SjFurtherOptPanel::OnLittleOption(wxCommandEvent& event)
{
	SjLittleOption* o = GetSelectedOption();
	if( o )
	{
		bool othersAffected = o->OnOption(SjDialog::FindTopLevel(this), event.GetId()-IDC_OPTIONFIRST);
		UpdateLittleOption(othersAffected? NULL : o);
	}
}


void SjFurtherOptPanel::OnLittleReset(wxCommandEvent&)
{
	wxWindow*   topLevelWindow = SjDialog::FindTopLevel(this);
	wxArrayLong selectedOptions = GetSelectedOptions();
	int         selectedOptionsCount = selectedOptions.GetCount(), i;

	if( selectedOptionsCount > 1 )
	{
		wxWindowDisabler disabler(topLevelWindow);

		if( SjMessageBox(
		            wxString::Format(_("Do you really want to reset all %i selected commands and options to their default values?"), (int)selectedOptionsCount),
		            SJ_PROGRAM_NAME,
		            wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION, topLevelWindow) == wxNO )
		{
			return;
		}

		topLevelWindow = NULL; // will be given to OnDefault, NULL means, there is no feedback allowed
	}

	if( selectedOptionsCount > 0 )
	{
		wxBusyCursor    busy;
		bool            othersAffected = FALSE;
		SjLittleOption  *option, *singleOptionToUpdate = NULL;
		int             modifiedOptionsCount = 0;

		for( i = 0; i < selectedOptionsCount; i++ )
		{
			option = (SjLittleOption*)(selectedOptions[i]);
			if( option->IsModified() )
			{
				if( option->OnDefault(topLevelWindow) )
				{
					othersAffected = TRUE;
				}

				if( singleOptionToUpdate == NULL )
				{
					singleOptionToUpdate = option;
				}

				modifiedOptionsCount++;

				wxASSERT( !option->IsModified() );
			}
		}

		if( modifiedOptionsCount > 1 || othersAffected )
		{
			singleOptionToUpdate = NULL; // update all
		}

		UpdateLittleOption(singleOptionToUpdate);
	}
}


/*******************************************************************************
 * SjBasicSettingsConfigPage
 ******************************************************************************/


class SjBasicSettingsConfigPage : public wxPanel
{
public:
	                SjBasicSettingsConfigPage (SjBasicSettingsModule* basicSettingsModule, wxWindow* parent, int selectedPage);
	                ~SjBasicSettingsConfigPage();

private:
	// further options
	SjArrayLittleOption m_littleOptions; // needed by compare function

	// little "Misc" settings
	void            GetLittleMiscOptions(SjArrayLittleOption&);
	void            ApplyOrCancelLittleMisc (bool apply, bool& wantsToBeRestarted);
	long            m_miscOldIdxCacheIndex, m_miscNewIdxCacheIndex,
	                m_miscOldIdxSync, m_miscNewIdxSync,
	                m_miscIndexImgDiskCache,
	                m_miscIndexImgRamCache,
	                m_miscIndexImgRegardTimestamp;
	wxString		m_oldLanguageValue, m_newLanguageValue;

	// other
	SjBasicSettingsModule* m_basicSettingsModule;
	wxNotebook*     m_notebook;
	bool            m_constructorDone;
	                DECLARE_EVENT_TABLE ()

	friend class    SjBasicSettingsModule;
};


BEGIN_EVENT_TABLE(SjBasicSettingsConfigPage, wxPanel)
END_EVENT_TABLE()


static SjBasicSettingsConfigPage* g_configPage = NULL;


SjBasicSettingsConfigPage::SjBasicSettingsConfigPage(SjBasicSettingsModule* basicSettingsModule, wxWindow* parent, int selectedPage)
	: wxPanel(parent) // wxCLIP_CHILDREN has problems with wxListCtrl
{
	wxBusyCursor busy;

	// save given objects
	m_basicSettingsModule   = basicSettingsModule;
	m_constructorDone       = FALSE;


	// init the little options array
	{
		SjModuleList* listOfModules = g_mainFrame->m_moduleSystem.GetModules(SJ_MODULETYPE_ALL);
		SjModuleList::Node* moduleNode = listOfModules->GetFirst();
		while( moduleNode )
		{
			SjModule* module = moduleNode->GetData();

			SjLittleOption::ClearSection();
			module->GetLittleOptions(m_littleOptions);

			moduleNode = moduleNode->GetNext();
		}
	}

	GetLittleMiscOptions(m_littleOptions);

	// create notebook
	wxSizer* dialogSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(dialogSizer);

	m_notebook = new wxNotebook(this, IDC_NOTEBOOK, wxDefaultPosition, wxDefaultSize, 0/*wxCLIP_CHILDREN - problems with wxChoice/wxComboBox*/);

	wxNotebook* notebookSizer = m_notebook; // wxNotebookSizer is no longer needed in 2.6.0

	SjPreselect preselectKeys= SJ_PRESELECT_NONE;
	if( selectedPage == 666 /*further options*/ )
	{
		selectedPage = PAGE_SHORTCUTS;
	}
	else if( selectedPage == 667 /*further options*/ )
	{
		selectedPage = PAGE_SHORTCUTS;
		preselectKeys = SJ_PRESELECT_NUMPAD;
	}
	else if( selectedPage == 668 /*further options*/ )
	{
		selectedPage = PAGE_SHORTCUTS;
		preselectKeys = SJ_PRESELECT_ADDCREDIT;
	}

	m_notebook->AddPage(new SjFurtherOptPanel(m_notebook, m_littleOptions, PAGE_SHORTCUTS, preselectKeys), _("Shortcuts"));
	m_notebook->AddPage(new SjFurtherOptPanel(m_notebook, m_littleOptions, PAGE_FURTHEROPTIONS, preselectKeys), _("Further options"));

	if( selectedPage < 0
	 || selectedPage >= (int)m_notebook->GetPageCount() )
	{
		selectedPage = 0;
	}

	m_notebook->SetSelection(selectedPage);

	dialogSizer->Add(notebookSizer, 1, wxALL|wxGROW, SJ_DLG_SPACE);

	// init done, center dialog
	dialogSizer->SetSizeHints(this);
	m_constructorDone = TRUE;
	g_configPage = this;
}


SjBasicSettingsConfigPage::~SjBasicSettingsConfigPage()
{
	g_configPage = NULL;
}


/*******************************************************************************
 * SjBasicSettingsConfigPage - Little "Misc" Options
 ******************************************************************************/


#if SJ_USE_SCRIPTS
class SjLittleScript : public SjLittleOption
{
public:
	SjLittleScript(const wxString& name, int index)
		: SjLittleOption(name, SJ_ICON_MODULE)
	{
		m_scriptIndex = index;
	}

	wxString GetDisplayValue() const { return wxT("..."); }
	long GetOptionCount() const { return 1; }
	wxString GetOption(long i) const { return _("Edit..."); }
	bool OnOption(wxWindow* parent, long i) { return OnDoubleClick(parent); }
	bool OnDoubleClick(wxWindow* parent)
	{
		SjSee::OnGlobalEmbedding(SJ_PERSISTENT_CONFIG_BUTTON, m_scriptIndex);
		return false; // other options are not affected
	}

private:
	int m_scriptIndex;
};
#endif


void SjBasicSettingsConfigPage::GetLittleMiscOptions(SjArrayLittleOption& lo)
{
	// init special commands...
	#define SEP wxString(wxT("|"))

	// ...scripts
	SjLittleOption::ClearSection();
	#if SJ_USE_SCRIPTS
	{
		wxArrayString arr = SjSee::GetGlobalEmbeddings(SJ_PERSISTENT_CONFIG_BUTTON);
		int i, iCount = arr.GetCount();
		for( i = 0; i < iCount; i++ )
			lo.Add(new SjLittleScript(arr[i], i));
	}
	#endif

	// ...language
	{
		const wxLanguageInfo* info = wxLocale::GetLanguageInfo(wxLocale::GetSystemLanguage());
		wxString defaultLanguageValue = info? info->CanonicalName : wxString(wxT("en_GB"));

		m_oldLanguageValue = g_tools->m_config->Read(wxT("main/language"), defaultLanguageValue);
		m_newLanguageValue = m_oldLanguageValue;

		lo.Add(new SjLittleStringSel(_("Language"), &m_newLanguageValue, defaultLanguageValue));
	}

	lo.Add(new SjLittleExploreSetting(_("Show files with")));

	if( !g_tools->m_instance.IsEmpty() )
	{
		lo.Add(new SjLittleReadOnly(_("Instance"),
		                            g_tools->m_instance,
		                            TRUE,
		                            wxString::Format(_("Use the command-line option --%s=<file> to change this."), wxT("instance"))));
	}

	// ...files: index files
	SjLittleOption::SetSection(_("Index file"));

	m_miscOldIdxCacheIndex = g_tools->m_config->Read(wxT("main/idxCacheBytes"), SJ_DEF_SQLITE_CACHE_BYTES) / SJ_ONE_MB;
	long tempLong2 = SJ_DEF_SQLITE_CACHE_BYTES/SJ_ONE_MB;
	wxString tempStr = SjLittleBit::GetMbOptions(32, m_miscOldIdxCacheIndex, tempLong2);
	m_miscNewIdxCacheIndex = m_miscOldIdxCacheIndex;
	SjLittleBit* littleFlag = new SjLittleBit (_("RAM cache"), tempStr,
	        &m_miscNewIdxCacheIndex, tempLong2, 0, wxT(""));
	tempStr.Printf(_("avg. query time: %s"), wxString::Format(wxT("%i %s"), (int)g_mainFrame->GetAvgSearchMs(), _("ms")).c_str());
	littleFlag->SetComment(tempStr);
	lo.Add(littleFlag);

	m_miscOldIdxSync = m_miscNewIdxSync =  g_tools->m_config->Read(wxT("main/idxCacheSync"), SJ_DEF_SQLITE_SYNC);
	lo.Add(new SjLittleBit (_("Synchronity"),
	                        _("Fast")+SEP+_("Save but slower")+SEP+_("Very save and slow"),
	                        &m_miscNewIdxSync, 0L, 0L, wxT(""), SJ_ICON_LITTLEDEFAULT, FALSE));


	// ...files: temp. directory
	g_tools->m_cache.GetLittleOptions(lo);

	// ...files: image files
	tempLong2 = g_mainFrame->m_imgThread->GetDiskCache();
	SjLittleOption::SetSection(_("Image cache"));

	m_miscIndexImgDiskCache = lo.GetCount();
	lo.Add(new SjLittleBit (_("Use temporary directory"),
	                        _("No")+SEP+
	                        _("Yes, load images asynchrony")+SEP+
	                        _("Yes, load images directly and avoid flickering"),
	                        &tempLong2, 0L, 0L, wxT(""), SJ_ICON_LITTLEDEFAULT, FALSE));

	long tempLong1 = g_mainFrame->m_imgThread->GetMaxRamCacheBytes()/SJ_ONE_MB;
	tempLong2 = SJ_DEF_IMGTHREAD_CACHE_BYTES/SJ_ONE_MB;
	tempStr = SjLittleBit::GetMbOptions(256, tempLong1, tempLong2);
	littleFlag = new SjLittleBit (_("RAM cache"), tempStr, &tempLong1, tempLong2, 0L, wxT(""), SJ_ICON_LITTLEDEFAULT, FALSE);
	tempStr.Printf(_("%i%% used for %i images"), (int)g_mainFrame->m_imgThread->GetRamCacheUsage('%'), (int)g_mainFrame->m_imgThread->GetRamCacheUsage('i'));
	littleFlag->SetComment(tempStr);

	m_miscIndexImgRamCache = lo.GetCount();
	lo.Add(littleFlag);

	m_miscIndexImgRegardTimestamp = lo.GetCount();
	tempLong2 = g_mainFrame->m_imgThread->GetRegardTimestamp()? 1L : 0L;
	lo.Add(new SjLittleBit (_("Regard file changes"), wxT("yn"), &tempLong2, 0L, 0L, wxT(""), SJ_ICON_LITTLEDEFAULT, FALSE));


	// ...files: search paths
	SjLittleOption::ClearSection();
	{
		wxArrayString paths, defPaths;
		for( int i = 0; i < g_tools->GetSearchPathCount(); i++ )
		{
			paths.Add(g_tools->GetSearchPath(i));
			if( g_tools->IsStaticSearchPath(i) ) defPaths.Add(g_tools->GetSearchPath(i));
		}
		lo.Add(new SjLittlePathList(_("Search paths"), paths, defPaths, wxT("main/searchPath")));
	}
}


void SjBasicSettingsConfigPage::ApplyOrCancelLittleMisc(bool apply, bool& wantsToBeRestarted)
{
	if( apply )
	{
		// apply image cache
		SjLittleBit* littleFlag = (SjLittleBit*)&m_littleOptions[m_miscIndexImgRamCache];
		long ramCacheBytes = SjLittleBit::Index2Mb(littleFlag->GetLong_())*SJ_ONE_MB;

		littleFlag = (SjLittleBit*)&m_littleOptions[m_miscIndexImgDiskCache];
		int useDiskCache = littleFlag->GetLong_();

		littleFlag = (SjLittleBit*)&m_littleOptions[m_miscIndexImgRegardTimestamp];
		bool regardTimestamp = littleFlag->GetLong_()!=0;

		g_mainFrame->m_imgThread->SetCacheSettings(ramCacheBytes, useDiskCache, regardTimestamp);

		// apply index cache
		if( m_miscOldIdxCacheIndex != m_miscNewIdxCacheIndex )
		{
			long newIdxCacheBytes = SjLittleBit::Index2Mb(m_miscNewIdxCacheIndex)*SJ_ONE_MB;
			wxSqlt sql;
			sql.GetDb()->SetCache(newIdxCacheBytes);
			g_tools->m_config->Write(wxT("main/idxCacheBytes"), newIdxCacheBytes);
		}

		if( m_miscOldIdxSync != m_miscNewIdxSync )
		{
			wxSqlt sql;
			sql.GetDb()->SetSync(m_miscNewIdxSync);
			g_tools->m_config->Write(wxT("main/idxCacheSync"), m_miscNewIdxSync);
		}

		// apply language
		if( m_oldLanguageValue != m_newLanguageValue )
		{
			g_tools->m_config->Write(wxT("main/language"), m_newLanguageValue);
			wantsToBeRestarted = TRUE;
			m_oldLanguageValue = m_newLanguageValue; // avoid recursion
		}
	}
}


/*******************************************************************************
 * SjBasicSettingsModule
 ******************************************************************************/


SjBasicSettingsModule::SjBasicSettingsModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	m_file       = wxT("memory:basicsettings.lib");
	m_name       = _("Advanced");
	m_guiIcon    = SJ_ICON_ADVANCED;
	m_sort       = 20000;
}


wxWindow* SjBasicSettingsModule::GetConfigPage(wxWindow* parent, int selectedPage)
{
	return new SjBasicSettingsConfigPage(this, parent, selectedPage);
}


void SjBasicSettingsModule::DoneConfigPage(wxWindow* configPage__, int doneCode__)
{
	SjBasicSettingsConfigPage*  configPage = (SjBasicSettingsConfigPage*)configPage__;
	bool                        wantsToBeRestarted = FALSE;
	bool                        apply = (doneCode__!=SJ_CONFIGPAGE_DONE_CANCEL_CLICKED);

	// cancel / apply embedded "little" options
	{
		int i, iCount = configPage->m_littleOptions.GetCount();
		if( apply )
		{
			for( i = 0; i < iCount; i++ ) { configPage->m_littleOptions[i].OnApply(); }
		}
		else
		{
			for( i = 0; i < iCount; i++ ) { configPage->m_littleOptions[i].OnCancel(); }
		}
		configPage->ApplyOrCancelLittleMisc(apply, wantsToBeRestarted);
	}

	// restart?
	if( wantsToBeRestarted )
	{
		wxWindowDisabler disabler(SjDialog::FindTopLevel(configPage__));
		SjMessageBox(wxString::Format(_("Please restart %s so that the changes can take effect."), SJ_PROGRAM_NAME),
		             SJ_PROGRAM_NAME, wxOK|wxICON_INFORMATION, SjDialog::FindTopLevel(configPage__));
	}
}


