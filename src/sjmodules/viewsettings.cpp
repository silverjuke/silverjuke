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
 * File:    viewsettings.cpp
 * Authors: Björn Petersen
 * Purpose: The view settings module
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/notebook.h>
#include <sjbase/skinenum.h>
#include <sjbase/browser.h>
#include <sjmodules/viewsettings.h>
#include <sjmodules/basicsettings.h>
#include <sjmodules/help/help.h>
#include <see_dom/sj_see.h>
#include <sjtools/msgbox.h>

#define IDC_SKINLIST                (IDM_FIRSTPRIVATE+0)
#define IDC_SKINEXPLORE             (IDM_FIRSTPRIVATE+2)
#define IDC_SKININFO                (IDM_FIRSTPRIVATE+3)
#define IDC_SKINWWW                 (IDM_FIRSTPRIVATE+4)
#define IDC_FONTFACECHOICE          (IDM_FIRSTPRIVATE+11)
#define IDC_FONTPTSLIDER            (IDM_FIRSTPRIVATE+12)
#define IDC_COLUMNWIDTHSLIDER       (IDM_FIRSTPRIVATE+13)
#define IDC_COVERHEIGHTSLIDER       (IDM_FIRSTPRIVATE+14)
#define IDC_FONTDEFAULT             (IDM_FIRSTPRIVATE+23)
#define IDC_LOADSKINSMENUBUTTON     (IDM_FIRSTPRIVATE+24)
#define IDC_CANCELSKINSELECTION     (IDM_FIRSTPRIVATE+25)


/*******************************************************************************
 * SjViewSettingsPage
 ******************************************************************************/


class SjViewSettingsPage : public wxPanel
{
public:
	SjViewSettingsPage(wxWindow* parent, int selectedPage);
	~SjViewSettingsPage();

private:
	// common
	wxNotebook*     m_notebook;

	// skin settings
	wxPanel*        CreateSkinPage      (wxWindow* parent);

	SjSkinEnumeratorItem* GetSkinFromDialog   ();
	void            UpdateSkinList      (const wxString& selSkin);

	void            OnUpdateSkinList    (wxCommandEvent& event);
	void            OnCancelSkinSelection(wxCommandEvent& event);
	void            OnSkinContextMenu   (wxListEvent&) { ShowContextMenu(this, ScreenToClient(::wxGetMousePosition())); }
	void            OnLoadSkinsContextMenu (wxCommandEvent&) { if(m_loadSkinsMenuButton) { ShowContextMenu(m_loadSkinsMenuButton, wxPoint(0,0)); } }
	void            OnChangeSkin        (wxListEvent&);
	void            OnDoubleClick       (wxListEvent&);
	void            OnSkinInfo          (wxCommandEvent&) {SkinInfo();}
	void            SkinInfo            ();
	void            OnSkinExplore       (wxCommandEvent&);
	void            OnSkinWWW           (wxCommandEvent&);
	void            OnSize              (wxSizeEvent&);
	void            OnSearchPaths       (wxCommandEvent&);
	void            ShowContextMenu     (wxWindow* window, const wxPoint& pt);

	int             m_skinChangeFromMe;
	wxListCtrl*     m_listCtrl;
	wxButton*       m_loadSkinsMenuButton;
	SjSkinEnumerator* m_skinEnumerator;
	wxString        m_orgSkinPath;

	// font settings
	wxPanel*        CreateFontPage      (wxWindow* parent);

	void            UpdateFontPtText    (bool addSpaces=FALSE);
	void            UpdateColumnWidthText
	();
	void            UpdateCoverHeightText
	();
	void            GetFontFromDialog   (wxString& fontFace, long& fontSize, long& columnWidth, long& coverHeight);

	void            OnFontFaceChoice    (wxCommandEvent&);
	void            OnFontDefault       (wxCommandEvent&);
	void            OnFontPtSlider      (wxScrollEvent&);
	void            OnColumnWidthSlider (wxScrollEvent&);
	void            OnCoverHeightSlider (wxScrollEvent&);

	wxString        m_orgFace;
	long            m_orgSize;
	long            m_orgColumnWidth;
	long            m_orgCoverHeight;
	wxChoice*       m_fontFaceChoice;
	wxSlider*       m_fontPtSlider;
	wxStaticText*   m_fontPtText;
	wxCheckBox*     m_useViewFontInDlgCheckBox;
	#define         COLUMNWIDTH_DIVISOR 5
	wxSlider*       m_columnWidthSlider;
	wxStaticText*   m_columnWidthText;
	wxSlider*       m_coverHeightSlider;
	wxStaticText*   m_coverHeightText;

	DECLARE_EVENT_TABLE ();

	friend class    SjViewSettingsModule;
};


BEGIN_EVENT_TABLE(SjViewSettingsPage, wxPanel)
	EVT_LIST_ITEM_SELECTED      (IDC_SKINLIST,          SjViewSettingsPage::OnChangeSkin            )
	EVT_LIST_ITEM_RIGHT_CLICK   (IDC_SKINLIST,          SjViewSettingsPage::OnSkinContextMenu       )
	EVT_LIST_ITEM_ACTIVATED     (IDC_SKINLIST,          SjViewSettingsPage::OnDoubleClick           )
	EVT_MENU                    (IDC_SKININFO,          SjViewSettingsPage::OnSkinInfo              )
	EVT_MENU                    (IDC_SKINEXPLORE,       SjViewSettingsPage::OnSkinExplore           )
	EVT_MENU                    (IDC_SKINUPDATELIST,    SjViewSettingsPage::OnUpdateSkinList        )
	EVT_MENU                    (IDC_CANCELSKINSELECTION, SjViewSettingsPage::OnCancelSkinSelection   )
	EVT_MENU                    (IDC_SKINWWW,           SjViewSettingsPage::OnSkinWWW               )
	EVT_BUTTON                  (IDC_SKINWWW,           SjViewSettingsPage::OnSkinWWW               )
	EVT_BUTTON                  (IDC_LOADSKINSMENUBUTTON, SjViewSettingsPage::OnLoadSkinsContextMenu  )
	EVT_CHOICE                  (IDC_FONTFACECHOICE,    SjViewSettingsPage::OnFontFaceChoice        )
	EVT_COMMAND_SCROLL          (IDC_FONTPTSLIDER,      SjViewSettingsPage::OnFontPtSlider          )
	EVT_BUTTON                  (IDC_FONTDEFAULT,       SjViewSettingsPage::OnFontDefault           )
	EVT_COMMAND_SCROLL          (IDC_COLUMNWIDTHSLIDER, SjViewSettingsPage::OnColumnWidthSlider     )
	EVT_COMMAND_SCROLL          (IDC_COVERHEIGHTSLIDER, SjViewSettingsPage::OnCoverHeightSlider     )
	EVT_SIZE                    (                       SjViewSettingsPage::OnSize                  )
END_EVENT_TABLE()


SjViewSettingsPage::SjViewSettingsPage(wxWindow* parent, int selectedPage)
	: wxPanel(parent)
{
	wxBusyCursor busy;

	// reset the list of facenames, this allows the user to install fonts without exit Silverjuke
	g_tools->UpdateFacenames();

	// create notebook
	wxSizer* dialogSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(dialogSizer);

	m_notebook = new wxNotebook(this, IDC_NOTEBOOK, wxDefaultPosition, wxDefaultSize, 0/*wxCLIP_CHILDREN - problems with wxChoice/wxComboBox*/);

	wxNotebook* notebookSizer = m_notebook;

	m_notebook->AddPage(CreateSkinPage(m_notebook),  _("Skins"));
	m_notebook->AddPage(CreateFontPage(m_notebook),  _("Fonts and covers"));

	if( selectedPage<0 || selectedPage >= (int)m_notebook->GetPageCount() )
	{
		selectedPage = 0;
	}

	m_notebook->SetSelection(selectedPage);

	dialogSizer->Add(notebookSizer, 1, wxALL|wxGROW, SJ_DLG_SPACE);

	// init done, center dialog
	dialogSizer->SetSizeHints(this);

	wxSizeEvent ev;
	OnSize(ev);
}


/*******************************************************************************
 * SjViewSettingsPage - Skin Settings
 ******************************************************************************/


wxPanel* SjViewSettingsPage::CreateSkinPage(wxWindow* parent)
{
	// save given objects
	m_skinEnumerator        = NULL;
	m_skinChangeFromMe      = 0;
	m_loadSkinsMenuButton   = NULL;

	// init dialog
	wxPanel* page = new wxPanel(parent, -1);
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	page->SetSizer(sizer1);

	sizer1->Add(SJ_DLG_SPACE, SJ_DLG_SPACE); // some space

	wxStaticText* staticText = new wxStaticText(page, -1,
	        wxString::Format(_("With different skins you can change the \"look and feel\" of %s. Just\nselect the skin to use from the list above. You'll find more skins on the web."), SJ_PROGRAM_NAME));
	sizer1->Add(staticText,
	            0, wxALL, SJ_DLG_SPACE);

	/* add list control
	 */
	m_listCtrl = new wxListCtrl(page, IDC_SKINLIST, wxDefaultPosition, wxSize(200, 100),
	                            wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxSUNKEN_BORDER);
	m_listCtrl->SetImageList(g_tools->GetIconlist(FALSE), wxIMAGE_LIST_SMALL);
	//m_listCtrl->SetImageList(g_tools->GetIconlist(TRUE), wxIMAGE_LIST_NORMAL);
	m_listCtrl->InsertColumn(0, _("Name"));
	sizer1->Add(m_listCtrl, 1, wxGROW|wxALL, SJ_DLG_SPACE);

	wxSizer* sizer3 = new wxBoxSizer(wxHORIZONTAL);
	sizer1->Add(sizer3, 0, wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

	m_loadSkinsMenuButton = new wxButton(page, IDC_LOADSKINSMENUBUTTON, _("Options")+wxString(SJ_BUTTON_MENU_ARROW));
	sizer3->Add(m_loadSkinsMenuButton, 0, wxRIGHT, SJ_DLG_SPACE);

	// init data
	m_orgSkinPath = g_mainFrame->GetSkinUrl();
	UpdateSkinList(m_orgSkinPath);

	return page;
}


SjViewSettingsPage::~SjViewSettingsPage()
{
	if( m_skinEnumerator )
	{
		delete m_skinEnumerator;
		m_skinEnumerator = NULL;
	}
}


void SjViewSettingsPage::OnSize(wxSizeEvent& event)
{
	wxSize size = m_listCtrl->GetClientSize();
	m_listCtrl->SetColumnWidth(0, size.x-8);
	SjDialog::EnsureSelListCtrlItemVisible(m_listCtrl);

	event.Skip(); // forward event to the next handler
}


void SjViewSettingsPage::OnUpdateSkinList(wxCommandEvent& event)
{
	g_mainFrame->Enable(FALSE);

	wxBusyCursor busy;

	g_tools->UpdateFacenames();

	wxString selSkin = m_orgSkinPath;
	SjSkinEnumeratorItem* skin = GetSkinFromDialog();
	if( skin )
	{
		selSkin = skin->m_url;
	}

	UpdateSkinList(selSkin);

	g_mainFrame->Enable(TRUE);
}



void SjViewSettingsPage::UpdateSkinList(const wxString& selSkin)
{
	m_skinChangeFromMe++;

	m_listCtrl->Freeze();
	m_listCtrl->DeleteAllItems();

	if( m_skinEnumerator )
	{
		delete m_skinEnumerator;
		m_skinEnumerator = NULL;
	}

	m_skinEnumerator = new SjSkinEnumerator();
	if( m_skinEnumerator )
	{
		int currSkinIndex, new_i;
		for( currSkinIndex = 0; currSkinIndex < m_skinEnumerator->GetCount(); currSkinIndex++ )
		{
			SjSkinEnumeratorItem* currSkin = m_skinEnumerator->GetSkin(currSkinIndex);
			wxListItem      item;
			item.m_mask     = wxLIST_MASK_IMAGE | wxLIST_MASK_TEXT | wxLIST_MASK_DATA;
			item.m_itemId   = currSkinIndex;
			item.m_text     = currSkin->m_name;
			item.m_data     = currSkinIndex;
			item.m_image    = SJ_ICON_SKIN_FILE;
			new_i = m_listCtrl->InsertItem(item);

			if( currSkin->m_url == selSkin )
			{
				m_listCtrl->SetItemState(new_i, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
			}
		}

		wxSize size = m_listCtrl->GetClientSize();
		m_listCtrl->SetColumnWidth(0, size.x-8);
	}

	m_listCtrl->Thaw();

	m_skinChangeFromMe--;
}


SjSkinEnumeratorItem* SjViewSettingsPage::GetSkinFromDialog()
{
	long i = SjDialog::GetSelListCtrlItem(m_listCtrl);
	if( i >= 0 )
	{
		i = m_listCtrl->GetItemData(i);

		wxASSERT(m_skinEnumerator);
		SjSkinEnumeratorItem* currSkin = m_skinEnumerator->GetSkin(i);
		wxASSERT(currSkin);

		return currSkin;
	}

	return NULL;
}


void SjViewSettingsPage::OnSkinExplore(wxCommandEvent& event)
{
	g_mainFrame->Enable(FALSE);

	SjSkinEnumeratorItem* currSkin = GetSkinFromDialog();
	if( currSkin )
	{
		g_tools->ExploreUrl(currSkin->m_url);
	}

	g_mainFrame->Enable(TRUE);
}


void SjViewSettingsPage::OnDoubleClick(wxListEvent& event)
{
	SkinInfo();
}


void SjViewSettingsPage::SkinInfo()
{
	wxWindow* parent = SjDialog::FindTopLevel(this);
	wxWindowDisabler disabled(parent);

	SjMessageBox(g_mainFrame->GetSkinName()
	               + wxT("\n(") + g_mainFrame->GetSkinUrl() + wxT(")\n\n") + g_mainFrame->GetSkinAbout(),
	               g_mainFrame->GetSkinName(), wxOK, parent);
}


void SjViewSettingsPage::OnChangeSkin(wxListEvent& event)
{
	if( !m_skinChangeFromMe )
	{
		SjSkinEnumeratorItem* currSkin = GetSkinFromDialog();
		if( currSkin )
		{
			wxBusyCursor busy;

			if( g_mainFrame->LoadSkin(currSkin->m_url, SJ_OP_DEF_NONKIOSK, wxEmptyString, true, this) )
			{
				g_tools->m_config->Write(wxT("main/skinFile"), g_mainFrame->GetSkinUrl());
				g_tools->m_config->Flush();
			}
			else
			{
				wxCommandEvent fwd(wxEVT_COMMAND_MENU_SELECTED, IDC_CANCELSKINSELECTION);
				AddPendingEvent(fwd);
			}

			g_mainFrame->Refresh();
			g_mainFrame->InitMainMenu(); // skins menu entries may be deleted
		}
	}
}
void SjViewSettingsPage::OnCancelSkinSelection(wxCommandEvent& event)
{
	UpdateSkinList(g_mainFrame->GetSkinUrl());
	SjDialog::EnsureSelListCtrlItemVisible(m_listCtrl);
}


void SjViewSettingsPage::ShowContextMenu(wxWindow* window, const wxPoint& pt)
{
	SjMenu menu(0);

	menu.Append(IDC_SKININFO, _("Info..."));

	menu.Append(IDC_SKINEXPLORE, _("Show file"));
	menu.AppendSeparator();
	menu.Append(IDC_SKINUPDATELIST, _("Update list"));
	menu.Append(IDC_SKINWWW, _("More skins on the web..."));

	window->PopupMenu(&menu, pt);
}


void SjViewSettingsPage::OnSkinWWW(wxCommandEvent& event)
{
	g_tools->ExploreHomepage(SJ_HOMEPAGE_MODULES_SKINS);
}


/*******************************************************************************
 * SjViewSettingsPage - Font Settings
 ******************************************************************************/


wxPanel* SjViewSettingsPage::CreateFontPage(wxWindow* parent)
{
	// init dialog
	wxPanel* page = new wxPanel(parent, -1);
	wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
	page->SetSizer(sizer1);

	sizer1->Add(SJ_DLG_SPACE, SJ_DLG_SPACE); // some space

	wxStaticText* staticText = new wxStaticText(page, -1,
	        _("You can set independent font and cover sizes to use in the main window. The sizes\nrefer to a zoom of 100%. If you zoom in or out (eg. by using the \"+\" or \"-\" keys), both\nsizes will grow or shrink proportionally."));
	sizer1->Add(staticText,
	            0, wxALL, SJ_DLG_SPACE);

	m_orgFace           = g_mainFrame->GetBaseFontFace();
	m_orgSize           = g_mainFrame->GetBaseFontSize();
	m_orgColumnWidth    = g_mainFrame->GetBaseColumnWidth();
	m_orgCoverHeight    = g_mainFrame->GetBaseCoverHeight();

	wxFlexGridSizer* sizer3 = new wxFlexGridSizer(2, SJ_DLG_SPACE/2, SJ_DLG_SPACE);
	sizer1->Add(sizer3, 0, wxGROW|wxALL, SJ_DLG_SPACE);

	#define SLIDER_W 240

	// font face
	sizer3->Add(new wxStaticText(page, -1, _("Font:")), 0, wxALIGN_CENTER_VERTICAL);

	m_fontFaceChoice = new wxChoice(page, IDC_FONTFACECHOICE/*, wxDefaultPosition, wxSize(FONTFACE_W, -1)*/);

	const wxArrayString& fontFaces = g_tools->GetFacenames();

	wxString currFace;
	int facesIndex, facesCount = fontFaces.GetCount();
	for( facesIndex = 0; facesIndex < facesCount; facesIndex++ )
	{
		currFace = fontFaces.Item(facesIndex);
		if( currFace[0] != '?' && currFace[0] != '#' ) // ignore funny font names as "?234" eg. on os x/wx2.6.3
		{
			m_fontFaceChoice->Append(currFace);
			if( currFace.CmpNoCase(m_orgFace)==0 )
			{
				m_fontFaceChoice->SetSelection(m_fontFaceChoice->GetCount()-1);
			}
		}
	}

	SjDialog::SetCbWidth(m_fontFaceChoice);
	sizer3->Add(m_fontFaceChoice, 0, wxALIGN_CENTER_VERTICAL);

	// font size
	sizer3->Add(new wxStaticText(page, -1, _("Font size:")), 0, wxALIGN_CENTER_VERTICAL);

	wxBoxSizer* sizer4 = new wxBoxSizer(wxHORIZONTAL);
	sizer3->Add(sizer4);

	m_fontPtSlider = new wxSlider(page, IDC_FONTPTSLIDER, m_orgSize, SJ_MIN_FONT_SIZE, SJ_MAX_FONT_SIZE, wxDefaultPosition, wxSize(SLIDER_W, -1), wxSL_HORIZONTAL);
	sizer4->Add(m_fontPtSlider, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

	m_fontPtText = new wxStaticText(page, -1, wxEmptyString);
	UpdateFontPtText(TRUE);
	sizer4->Add(m_fontPtText, 0, wxALIGN_CENTER_VERTICAL, 0);


	// use font in dialogs?
	sizer3->Add(SJ_DLG_SPACE, SJ_DLG_SPACE);

	m_useViewFontInDlgCheckBox = new wxCheckBox(page, -1, _("Use this font for dialogs, too"));
	m_useViewFontInDlgCheckBox->SetValue((g_accelModule->m_flags&SJ_ACCEL_USE_VIEW_FONT_IN_DLG)!=0);
	sizer3->Add(m_useViewFontInDlgCheckBox, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM, SJ_DLG_SPACE);

	// column width
	sizer3->Add(new wxStaticText(page, -1, _("Column width:")), 0, wxALIGN_CENTER_VERTICAL);

	sizer4 = new wxBoxSizer(wxHORIZONTAL);
	sizer3->Add(sizer4);

	m_columnWidthSlider = new wxSlider(page, IDC_COLUMNWIDTHSLIDER, m_orgColumnWidth/COLUMNWIDTH_DIVISOR, SJ_MIN_COLUMN_WIDTH/COLUMNWIDTH_DIVISOR, SJ_MAX_COLUMN_WIDTH/COLUMNWIDTH_DIVISOR, wxDefaultPosition, wxSize(SLIDER_W, -1), wxSL_HORIZONTAL);
	sizer4->Add(m_columnWidthSlider, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

	m_columnWidthText = new wxStaticText(page, -1, wxEmptyString);
	UpdateColumnWidthText();
	sizer4->Add(m_columnWidthText, 0, wxALIGN_CENTER_VERTICAL);


	// cover height (= width = size)
	sizer3->Add(new wxStaticText(page, -1, _("Cover size:")), 0, wxALIGN_CENTER_VERTICAL);

	sizer4 = new wxBoxSizer(wxHORIZONTAL);
	sizer3->Add(sizer4);

	m_coverHeightSlider = new wxSlider(page, IDC_COVERHEIGHTSLIDER, m_orgCoverHeight, 1, 100, wxDefaultPosition, wxSize(SLIDER_W, -1), wxSL_HORIZONTAL);
	sizer4->Add(m_coverHeightSlider, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

	m_coverHeightText = new wxStaticText(page, -1, wxEmptyString);
	UpdateCoverHeightText();
	sizer4->Add(m_coverHeightText, 0, wxALIGN_CENTER_VERTICAL);

	wxButton* button = new wxButton(page, IDC_FONTDEFAULT, _("Reset to default values"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	sizer1->Add(button, 0, wxALL, SJ_DLG_SPACE);

	return page;
}


void SjViewSettingsPage::UpdateFontPtText(bool addSpaces)
{
	long v = m_fontPtSlider->GetValue();
	m_fontPtText->SetLabel(wxString::Format(wxT("%i Pt%s"), (int)v, addSpaces? wxT("    ") : wxT("")));
}


void SjViewSettingsPage::UpdateColumnWidthText()
{
	long v = m_columnWidthSlider->GetValue() * COLUMNWIDTH_DIVISOR;
	m_columnWidthText->SetLabel(wxString::Format(wxT("%i"), (int)v));
}


void SjViewSettingsPage::UpdateCoverHeightText()
{
	long v = m_coverHeightSlider->GetValue();
	m_coverHeightText->SetLabel(wxString::Format(wxT("%i%%"), (int)v));
}


void SjViewSettingsPage::GetFontFromDialog(wxString& fontFace, long& fontSize, long& columnWidth, long& coverHeight)
{
	long i = m_fontFaceChoice->GetSelection();
	if( i >= 0 )
	{
		fontFace = m_fontFaceChoice->GetString(i);
	}
	else
	{
		fontFace = g_mainFrame->GetBaseFontFace();
	}

	fontSize = m_fontPtSlider->GetValue();
	columnWidth = m_columnWidthSlider->GetValue() * COLUMNWIDTH_DIVISOR;
	coverHeight = m_coverHeightSlider->GetValue();
}


void SjViewSettingsPage::OnFontFaceChoice(wxCommandEvent&)
{
	if( m_columnWidthText /*init done?*/ )
	{
		wxString fontFace;
		long     fontSize, columnWidth, coverHeight;
		GetFontFromDialog(fontFace, fontSize, columnWidth, coverHeight);
		g_mainFrame->SetFontNCoverBase(fontFace, fontSize, columnWidth, coverHeight);
	}
}


void SjViewSettingsPage::OnFontDefault(wxCommandEvent&)
{
	// use default values
	long i = m_fontFaceChoice->FindString(SJ_DEF_FONT_FACE);
	if( i == -1 ) i = 0;
	m_fontFaceChoice->SetSelection(i);

	m_fontPtSlider->SetValue(SJ_DEF_FONT_SIZE);
	UpdateFontPtText();
	m_columnWidthSlider->SetValue(SJ_DEF_COLUMN_WIDTH / COLUMNWIDTH_DIVISOR);
	UpdateColumnWidthText();
	m_coverHeightSlider->SetValue(SJ_DEF_COVER_HEIGHT);
	UpdateCoverHeightText();

	m_useViewFontInDlgCheckBox->SetValue(false);

	g_mainFrame->SetFontNCoverBase(SJ_DEF_FONT_FACE, SJ_DEF_FONT_SIZE, SJ_DEF_COLUMN_WIDTH, SJ_DEF_COVER_HEIGHT);
}


void SjViewSettingsPage::OnFontPtSlider(wxScrollEvent&)
{
	if( m_columnWidthText /*init done?*/ )
	{
		UpdateFontPtText();

		wxString fontFace;
		long     fontSize, columnWidth, coverHeight;
		GetFontFromDialog(fontFace, fontSize, columnWidth, coverHeight);
		g_mainFrame->SetFontNCoverBase(fontFace, fontSize, columnWidth, coverHeight);
	}
}


void SjViewSettingsPage::OnColumnWidthSlider(wxScrollEvent&)
{
	if( m_columnWidthText /*init done?*/ )
	{
		UpdateColumnWidthText();

		wxString fontFace;
		long     fontSize, columnWidth, coverHeight;
		GetFontFromDialog(fontFace, fontSize, columnWidth, coverHeight);
		g_mainFrame->SetFontNCoverBase(fontFace, fontSize, columnWidth, coverHeight);
	}
}


void SjViewSettingsPage::OnCoverHeightSlider(wxScrollEvent&)
{
	if( m_columnWidthText /*init done?*/ )
	{
		UpdateCoverHeightText();

		wxString fontFace;
		long     fontSize, columnWidth, coverHeight;
		GetFontFromDialog(fontFace, fontSize, columnWidth, coverHeight);
		g_mainFrame->SetFontNCoverBase(fontFace, fontSize, columnWidth, coverHeight);
	}
}


/*******************************************************************************
 * SjViewSettingsModule
 ******************************************************************************/


SjViewSettingsModule::SjViewSettingsModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	m_file       = wxT("memory:viewsettings.lib");
	m_name       = _("Skins");
	m_guiIcon    = SJ_ICON_SKIN;
	m_sort       = 30;
}


wxWindow* SjViewSettingsModule::GetConfigPage(wxWindow* parent, int selectedPage)
{
	return new SjViewSettingsPage(parent, selectedPage);
}


void SjViewSettingsModule::DoneConfigPage(wxWindow* configPage__, int doneAction__)
{
	SjViewSettingsPage* configPage = (SjViewSettingsPage*)configPage__;

	if( doneAction__ != SJ_CONFIGPAGE_DONE_CANCEL_CLICKED )
	{
		// apply settings
		SjTools::SetFlag(g_accelModule->m_flags, SJ_ACCEL_USE_VIEW_FONT_IN_DLG, configPage->m_useViewFontInDlgCheckBox->GetValue());
		g_tools->m_config->Write(wxT("main/accelFlags"), g_accelModule->m_flags);
	}
	else
	{
		// cancel skin settings
		if( g_mainFrame->GetSkinUrl() != configPage->m_orgSkinPath )
		{
			wxBusyCursor busy;

			g_mainFrame->LoadSkin(configPage->m_orgSkinPath, SJ_OP_DEF_NONKIOSK);
			g_mainFrame->Refresh();
			g_mainFrame->InitMainMenu(); // skins menu entries may be deleted

			wxASSERT(g_tools);
			g_tools->m_config->Write(wxT("main/skinFile"), g_mainFrame->GetSkinUrl());
			g_tools->m_config->Flush();
		}

		// cancel font settings
		g_mainFrame->m_browser->Freeze();
		g_mainFrame->SetFontNCoverBase(configPage->m_orgFace, configPage->m_orgSize, configPage->m_orgColumnWidth, configPage->m_orgCoverHeight);
		g_mainFrame->m_browser->Thaw();
	}
}

