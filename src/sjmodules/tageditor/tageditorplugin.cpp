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
 * File:    tageditorplugin.cpp
 * Authors: Björn Petersen
 * Purpose: Tag editor plugin base
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/tageditor/tageditor.h>
#include <sjmodules/tageditor/tageditorplugin.h>

#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArrayModifyItem);


/*******************************************************************************
 * SjModifyInfo
 ******************************************************************************/


void SjModifyItem::SetNewVal(const wxString& newVal)
{
	if( m_field == SJ_TI_URL )
	{
		m_newVal = SjTools::EnsureValidPathChars(newVal);
	}
	else
	{
		m_newVal = newVal;
	}
}


void SjModifyInfo::Add(  const wxString& url,
                         long field,
                         const wxString& oldVal, const wxString& newVal__ )
{
	// correct the new value if it is an URL
	wxString newVal(newVal__);
	if( field == SJ_TI_URL )
	{
		newVal = SjTools::EnsureValidPathChars(newVal);
	}

	// add the modification, if it does not exist yet
	wxString key = wxString::Format(wxT("%s-%lu"), url.c_str(), field);
	SjModifyItem* item = (SjModifyItem*)m_hash.Lookup(key);
	if( item == NULL )
	{
		item = new SjModifyItem(field, url, oldVal);
		m_items.Add(item);
		m_hash.Insert(key, (long)item);
	}

	// set the new value
	item->SetNewVal(newVal);

	// check if the modification item is still modified; if not, remove it
	// (may happen if SetNewVal() changes the new value eg. due to invalid chars in a path name
	if( !item->IsModified() )
	{
		//m_items.Remove(item); <-- this does not work any longer, use the line below
		m_items.RemoveAt(m_items.Index(*item));

		m_hash.Remove(key);
	}
}


void SjModifyInfo::Delete(int i)
{
	SjModifyItem* item = GetItem(i);
	if( item )
	{
		wxString key = wxString::Format(wxT("%s-%lu"), item->GetUrl().c_str(), item->GetField());

		//m_items.Remove(item); <-- this does not work any longer, use the line below
		m_items.RemoveAt(m_items.Index(*item));

		m_hash.Remove(key);
	}
}


bool SjModifyInfo::LetConfirm(bool& askWriteId3, bool& askDelEmptyDir, bool& onlyUrlsModified)
{
	bool letConfirm = FALSE;

	askWriteId3 = FALSE;
	askDelEmptyDir = FALSE;
	onlyUrlsModified = TRUE;

	long i, modCount = GetCount();
	SjModifyItem* modItem;
	for( i = 0; i < modCount; i++ )
	{
		modItem = GetItem(i);

		if( modItem->GetField() == SJ_TI_URL )
		{
			// if the url us modified, ask if to del. empty dir; this implies confirmation
			if( !askDelEmptyDir )
			{
				wxString oldPath = wxFileSystem::URLToFileName(modItem->GetOldVal()).GetPath(),
				         newPath = wxFileSystem::URLToFileName(modItem->GetNewVal()).GetPath();
				if( oldPath != newPath )
				{
					askDelEmptyDir = TRUE;
				}
			}

			letConfirm = TRUE;
		}
		else
		{
			// if another field is modified, ask if to update the ID3 tags (only if confirmation
			// is needed by other reasons, we do not want to pop up the confirmation window too often)
			askWriteId3 = TRUE;
			onlyUrlsModified = FALSE;
		}

		if( !letConfirm )
		{
			// if the new value is empty or null, let the user confirm this
			// (the "!letConfirm" is just for speed reasons)
			if( modItem->GetNewVal().IsEmpty() || modItem->GetNewVal() == wxT("0") )
			{
				letConfirm = TRUE;
			}
		}
	}

	return letConfirm;
}


/*******************************************************************************
 * SjModifyListCtrl
 ******************************************************************************/


SjModifyListCtrl::SjModifyListCtrl(wxWindow* parent, wxWindowID id, SjModifyInfo& items, bool showUrlsOnly)
	: wxListCtrl(parent, id, wxDefaultPosition, wxSize(550, 200), wxLC_REPORT | wxLC_VIRTUAL)
{
	m_items = &items;
	m_showUrlsOnly = showUrlsOnly;

	if( m_showUrlsOnly )
	{
		InsertColumn(CONFIRM_COL1_OLDURL,   _("Old file name"));
		InsertColumn(CONFIRM_COL1_NEWURL,   _("New file name"));
	}
	else
	{
		InsertColumn(CONFIRM_COL2_URL,      _("File"));
		InsertColumn(CONFIRM_COL2_FIELD,    _("Field"));
		InsertColumn(CONFIRM_COL2_OLDVAL,   _("Old value"));
		InsertColumn(CONFIRM_COL2_NEWVAL,   _("New value"));
	}

	SetImageList(g_tools->GetIconlist(FALSE), wxIMAGE_LIST_SMALL);

	SetItemCount(m_items->GetCount());
}


void SjModifyListCtrl::SizeColumns()
{
	wxSize size = GetClientSize();

	if( m_showUrlsOnly )
	{
		int w1 = size.x/2;
		SetColumnWidth(CONFIRM_COL1_OLDURL, w1);
		SetColumnWidth(CONFIRM_COL1_NEWURL, size.x-w1);
	}
	else
	{
		#define FIELD_W 80
		#define VAL_W   140
		SetColumnWidth(CONFIRM_COL2_URL,    size.x-FIELD_W-VAL_W*2);
		SetColumnWidth(CONFIRM_COL2_FIELD,  FIELD_W);
		SetColumnWidth(CONFIRM_COL2_OLDVAL, VAL_W);
		SetColumnWidth(CONFIRM_COL2_NEWVAL, VAL_W);
	}
}


wxString SjModifyListCtrl::GetValDescr(SjModifyItem* item, bool newVal) const
{
	wxString valDescr = newVal? item->GetNewVal() : item->GetOldVal();

	if( valDescr.IsEmpty() || valDescr == wxT("0") )
	{
		return _("<empty>");
	}
	else if( item->GetField() == SJ_TI_URL )
	{
		wxString oldPath, newPath;
		SjTools::GetFileNameFromUrl(item->GetOldVal(), &oldPath);
		SjTools::GetFileNameFromUrl(item->GetNewVal(), &newPath);
		if( oldPath == newPath )
		{
			return SjTools::GetFileNameFromUrl(valDescr);
		}
	}

	return valDescr;
}


wxString SjModifyListCtrl::OnGetItemText(long index, long column) const
{
	SjModifyItem* item = m_items->GetItem(index);
	if( item == NULL ) return wxEmptyString;

	if( m_showUrlsOnly )
	{
		wxString text = column==CONFIRM_COL1_OLDURL? item->GetOldVal() : item->GetNewVal();
		if( text.StartsWith("file:") )
		{
			text = wxFileSystem::URLToFileName(text).GetFullPath();
		}

		return text;
	}
	else
	{
		switch( column )
		{
			case CONFIRM_COL2_FIELD:
				return SjTrackInfo::GetFieldDescr(item->GetField());

			case CONFIRM_COL2_OLDVAL:
				return GetValDescr(item, FALSE);

			case CONFIRM_COL2_NEWVAL:
				return GetValDescr(item, TRUE);

			default:
				if( m_items->IsFirstUrl(index) )
				{
					wxString text = item->GetUrl();
					if( text.StartsWith("file:") )
					{
						text = wxFileSystem::URLToFileName(text).GetFullPath();
					}

					return text;
				}
				else
				{
					return wxEmptyString;
				}
		}
	}
}


int SjModifyListCtrl::OnGetItemImage(long index) const
{
	if( m_items->IsFirstUrl(index) )
	{
		return SJ_ICON_ANYFILE;
	}
	else
	{
		return SJ_ICON_EMPTY;
	}
}


/*******************************************************************************
 * SjConfirmDlg
 ******************************************************************************/


BEGIN_EVENT_TABLE(SjConfirmDlg, SjTagEditorPlugin)
	EVT_LIST_ITEM_ACTIVATED     (IDC_CONFIRM_LIST,      SjConfirmDlg::OnDoubleClick     )
	EVT_LIST_ITEM_RIGHT_CLICK   (IDC_CONFIRM_LIST,      SjConfirmDlg::OnContextMenu     )
	EVT_LIST_KEY_DOWN           (IDC_CONFIRM_LIST,      SjConfirmDlg::OnKeyDown         )
	EVT_MENU                    (IDC_CONFIRM_EDIT,      SjConfirmDlg::OnEdit            )
	EVT_MENU                    (IDC_CONFIRM_DELETE,    SjConfirmDlg::OnDelete          )
	EVT_SIZE                    (                       SjConfirmDlg::OnSize            )
END_EVENT_TABLE()


SjConfirmDlg::SjConfirmDlg(wxWindow* parent, SjModifyInfo& items, bool askWriteId3, bool askDelEmptyDir, bool onlyUrlsModified)
	: SjTagEditorPlugin(parent, wxT("confirm"), _("Confirm modifications"), NULL)
{
	m_items = &items;
	m_itemsEdited = FALSE;

	m_writeId3CheckBox = NULL;
	m_delEmptyDirCheckBox = NULL;

	m_sizer1->Add(SJ_DLG_SPACE, SJ_DLG_SPACE); // some space

	m_msgTextCtrl = new wxStaticText(this, -1, GetMsg());
	m_sizer1->Add(m_msgTextCtrl, 0, wxALL|wxGROW, SJ_DLG_SPACE);

	m_listCtrl = new SjModifyListCtrl(this, IDC_CONFIRM_LIST, items, onlyUrlsModified);
	m_sizer1->Add(m_listCtrl, 1, wxALL|wxGROW, SJ_DLG_SPACE);

	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	m_sizer1->Add(buttonSizer, 0, wxGROW|wxALL, SJ_DLG_SPACE);

	if( askWriteId3 )
	{
		m_writeId3CheckBox = new wxCheckBox(this, -1, _("Write (ID3-)tags"));
		m_writeId3CheckBox->SetValue(g_tagEditorModule->GetWriteId3Tags());
		buttonSizer->Add(m_writeId3CheckBox, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);
	}

	if( askDelEmptyDir )
	{
		m_delEmptyDirCheckBox = new wxCheckBox(this, -1, _("Delete empty folders"));
		m_delEmptyDirCheckBox->SetValue(g_tagEditorModule->GetDelEmptyDir());
		buttonSizer->Add(m_delEmptyDirCheckBox, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);
	}

	buttonSizer->Add(SJ_DLG_SPACE, SJ_DLG_SPACE, 1/*grow*/);

	wxButton* okButton = new wxButton(this, wxID_OK, _("OK"));
	buttonSizer->Add(okButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, SJ_DLG_SPACE);

	buttonSizer->Add(new wxButton(this, wxID_CANCEL,  _("Cancel")), 0, wxALIGN_CENTER_VERTICAL);

	// finally, set the default button. do it last to avoid
	// miscalculated button sizes eg. under Motif
	okButton->SetDefault();

	m_sizer1->SetSizeHints(this);
	m_listCtrl->SizeColumns();
	CenterOnParent();
}


wxString SjConfirmDlg::GetMsg() const
{
	if( m_items->GetCount() == 1 )
	{
		return _("Please confirm or edit the following modification:");
	}
	else
	{
		return wxString::Format(_("Please confirm or edit the following %s modifications:"), SjTools::FormatNumber(m_items->GetCount()).c_str());
	}
}


void SjConfirmDlg::OnKeyDown(wxListEvent& event)
{
	switch( event.GetKeyCode() )
	{
		case WXK_DELETE:
		case WXK_BACK:
		{
			wxCommandEvent fwd(wxEVT_COMMAND_MENU_SELECTED, IDC_CONFIRM_DELETE);
			AddPendingEvent(fwd);
		}
		break;
	}
}


void SjConfirmDlg::OnContextMenu(wxListEvent&)
{
	int selItemCount = m_listCtrl->GetSelectedItemCount();
	SjMenu m(0);

	m.Append(IDC_CONFIRM_EDIT, _("Edit..."));
	m.Enable(IDC_CONFIRM_EDIT, selItemCount==1);

	m.Append(IDC_CONFIRM_DELETE, _("Delete"));
	m.Enable(IDC_CONFIRM_DELETE, selItemCount>0);

	PopupMenu(&m, ScreenToClient(::wxGetMousePosition()));
}


void SjConfirmDlg::OnEdit(wxCommandEvent&)
{
	long selItemCount = m_listCtrl->GetSelectedItemCount();
	if( selItemCount > 0 )
	{
		// get the selected item
		long i, itemCount = m_items->GetCount(), index = -1;
		for( i = 0; i < itemCount; i++ )
		{
			if( m_listCtrl->GetItemState(i, wxLIST_STATE_SELECTED) &  wxLIST_STATE_SELECTED )
			{
				index = i;
				break;
			}
		}

		// edit this item
		if( index != -1 )
		{
			SjModifyItem* item = m_items->GetItem(index);
			if( item )
			{
				wxTextEntryDialog textEntry(this, _("New value")+wxString(wxT(":")), SjTrackInfo::GetFieldDescr(item->GetField()), item->GetNewVal());
				if( textEntry.ShowModal() == wxID_OK )
				{
					item->SetNewVal(textEntry.GetValue());
					m_listCtrl->Refresh();

					m_itemsEdited = TRUE; // we have to check for void modifications on close
				}
			}
		}
	}
}


void SjConfirmDlg::OnDelete(wxCommandEvent&)
{
	long selItemCount = m_listCtrl->GetSelectedItemCount();
	if( selItemCount > 0 )
	{
		wxBusyCursor busy;

		// delete all seleted items and deselect them,
		// remember the index of the first selected item
		long firstSelItem = -1;
		long deletedItems = 0;
		long i, itemCount = m_items->GetCount();
		for( i = 0; i < itemCount; i++ )
		{
			if( m_listCtrl->GetItemState(i, wxLIST_STATE_SELECTED) & wxLIST_STATE_SELECTED )
			{
				m_listCtrl->SetItemState(i, 0, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
				if( firstSelItem == -1 ) firstSelItem = i;

				m_items->Delete(i-deletedItems);
				deletedItems++;
			}
		}

		// set the new item count
		itemCount = m_items->GetCount();
		m_listCtrl->SetItemCount(itemCount);

		// select the item at the index of the first selected item before deletion
		if( itemCount )
		{
			if( firstSelItem >= itemCount ) firstSelItem = itemCount-1;
			m_listCtrl->SetItemState(firstSelItem, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
		}

		// update the message text
		m_msgTextCtrl->SetLabel(GetMsg());
	}
}


bool SjConfirmDlg::TransferDataFromWindow()
{
	// call parent class
	if( !SjTagEditorPlugin::TransferDataFromWindow() )
	{
		return FALSE;
	}

	// write id3 tags? delete empty directories?
	if( m_writeId3CheckBox )
	{
		g_tagEditorModule->SetWriteId3Tags(m_writeId3CheckBox->IsChecked());
	}

	if( m_delEmptyDirCheckBox )
	{
		g_tagEditorModule->SetDelEmptyDir(m_delEmptyDirCheckBox->IsChecked());
	}

	// check modifications?
	// (if the user edited some modifications, these modidications may get void;
	// if so, remove them from the item list)
	if( m_itemsEdited )
	{
		wxBusyCursor busy;
		long i, iCount = m_items->GetCount();
		SjModifyItem* item;
		for( i = 0; i < iCount; i++ )
		{
			item = m_items->GetItem(i);
			if(  item
			        && !item->IsModified() )
			{
				m_items->Delete(i);
				i--; // same index again
				iCount--;
			}
		}
	}

	return TRUE;
}


/*******************************************************************************
 * SjInsertButton
 ******************************************************************************/


SjInsertButton::SjInsertButton(wxWindow* parent, int idButton, int idFirstOption)
	: wxButton(parent, idButton, _("Insert")+wxString(SJ_BUTTON_MENU_ARROW), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT)
{
	m_idButton      = idButton;
	m_idFirstOption = idFirstOption;
}


void SjInsertButton::AddOption(const wxString& name, const wxString& descr)
{
	m_optName.Add(name);
	m_optDescr.Add(descr);
}


void SjInsertButton::AddCaseOption(const wxString& name, const wxString& descr)
{
	AddOption(name, descr);
	AddOption(name.Lower(), descr+wxString(wxT(" "))+_("(lower case)"));
	AddOption(name.Upper(), descr+wxString(wxT(" "))+_("(upper case)"));
}


void SjInsertButton::AddWidthOption(const wxString& name, const wxString& descr)
{
	AddOption(name, descr);
	AddOption(name.Left(name.Len()-1) + wxT("(10)>"),
	          descr + wxString(wxT(" ")) + _("(n characters)"));
}


void SjInsertButton::OnOption(wxCommandEvent& event, wxComboBox* cb)
{
	int id = event.GetId();

	if( id == m_idButton )
	{
		// popup menu
		SjMenu menu(SJ_SHORTCUTS_LOCAL);

		size_t i;
		for( i = 0; i < m_optDescr.GetCount(); i++ )
		{
			if( m_optDescr[i].IsEmpty() )
			{
				menu.AppendSeparator();
			}
			else
			{
				menu.Append(m_idFirstOption+i, m_optDescr[i]);
			}
		}

		PopupMenu(&menu, 0, 0);
	}
	else if( id >= m_idFirstOption )
	{
		// insert text into dest. window
		int index = id - m_idFirstOption;
		if( index >= (int)m_optDescr.GetCount() ) return;

		int i = cb->GetInsertionPoint();
		wxLogDebug(wxT("%i"), i);

		wxString value = cb->GetValue().Trim();

		value += wxT(" ") + m_optName[index];
		value.Trim(FALSE);

		cb->SetValue(value);

	}
}


/*******************************************************************************
 * SjFieldChoice
 ******************************************************************************/


SjTrackInfoFieldChoice::SjTrackInfoFieldChoice(wxWindow* parent, int id)
	: wxChoice(parent, id)
{
}


void SjTrackInfoFieldChoice::AppendFlags(long tiFlags)
{
	#define ADD_TO_FIELD_CHOICE(f, t) \
     if( tiFlags & (f) ) \
     { \
         Append((t), (void*)(f)); \
     }

	ADD_TO_FIELD_CHOICE(SJ_TI_TRACKNR,          _("Track number"))
	ADD_TO_FIELD_CHOICE(SJ_TI_TRACKCOUNT,       _("Track count"))
	ADD_TO_FIELD_CHOICE(SJ_TI_DISKNR,           _("Disk number"))
	ADD_TO_FIELD_CHOICE(SJ_TI_DISKCOUNT,        _("Disk count"))
	ADD_TO_FIELD_CHOICE(SJ_TI_TRACKNAME,        _("Title"))
	ADD_TO_FIELD_CHOICE(SJ_TI_LEADARTISTNAME,   _("Artist"))
	ADD_TO_FIELD_CHOICE(SJ_TI_ORGARTISTNAME,    _("Original artist"))
	ADD_TO_FIELD_CHOICE(SJ_TI_ALBUMNAME,        _("Album"))
	ADD_TO_FIELD_CHOICE(SJ_TI_GENRENAME,        _("Genre"))
	ADD_TO_FIELD_CHOICE(SJ_TI_GROUPNAME,        _("Group"))
	ADD_TO_FIELD_CHOICE(SJ_TI_COMMENT,          _("Comment"))
	ADD_TO_FIELD_CHOICE(SJ_TI_YEAR,             _("Year"))
	ADD_TO_FIELD_CHOICE(SJ_TI_PLAYTIMEMS,       _("Duration"))
}


/*******************************************************************************
 * SjTagEditorPlugin - the base for all edit plugins
 ******************************************************************************/


SjTagEditorPlugin::SjTagEditorPlugin(wxWindow* parent, const wxString& name, const wxString& title, SjTrackInfo* exampleTrackInfo)
	: SjDialog(parent, title, SJ_MODAL, SJ_RESIZEABLE_IF_POSSIBLE )
{
	m_exampleTrackInfo = exampleTrackInfo;
	m_name = name;
	m_sizer1 = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_sizer1);

	// set accelerators
	SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_EDIT));
}


void SjTagEditorPlugin::AfterConstructor()
{
	m_sizer1->Add(CreateButtons(SJ_DLG_OK_CANCEL),
	              0, wxGROW|wxALL, SJ_DLG_SPACE);

	m_sizer1->SetSizeHints(this);
	CenterOnParent();
}

