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
 * File:    dialogsj.cpp
 * Authors: Björn Petersen
 * Purpose: Dialog base-classes
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/spinctrl.h>
#include <wx/display.h>
#include <wx/gbsizer.h>
#include <sjtools/dialog.h>
#include <sjtools/msgbox.h>
#include <sjmodules/kiosk/virtkeybd.h>

#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArrayDlgCtrl);


/*******************************************************************************
 * SjDialog
 ******************************************************************************/


SjDialog::SjDialog(wxWindow* parent, const wxString& title, SjDialogMode mode,
                   SjDialogResizeType resizeType, long addStyle)
	: wxDialog  (parent, -1, title, wxDefaultPosition, wxDefaultSize,
	             (resizeType==SJ_NEVER_RESIZEABLE?
	              (wxDEFAULT_DIALOG_STYLE)
	              : (wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER))
	             | addStyle
	            )
{
	m_stateSizer = NULL;

	if( g_accelModule )
	{
		if( g_accelModule->m_flags&SJ_ACCEL_USE_VIEW_FONT_IN_DLG )
		{
			SetFont(g_mainFrame->m_baseStdFont);
		}
	}

	if( g_virtKeybd )
	{
		if( g_virtKeybd->GetKeybdFlags()&SJ_VIRTKEYBD_HIDE_CURSOR )
		{
			SetCursor(SjVirtKeybdModule::GetStandardCursor());
		}
	}
}


/*******************************************************************************
 * SjDialog static tools
 ******************************************************************************/


wxSizer* SjDialog::CreateButtons(wxWindow* this_, long flags,
                                 const wxString& okTitle,   const wxString& cancelTitle,
                                 const wxString& prevTitle, const wxString& nextTitle)
{
	/* the button order is ...
	 * [menu] [<<] [>>] --- [ok] [cancel]
	 */
	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton*   buttonDefault = 0;
	wxButton*   b;

	/* create the buttons
	 */
	if( flags & SJ_DLG_MENU )
	{
		b = new wxButton(this_, IDC_BUTTONBARMENU, _("Menu") + wxString(SJ_BUTTON_MENU_ARROW));
		buttonSizer->Add(b, 0, wxRIGHT, SJ_DLG_SPACE);
	}

	if( flags & SJ_DLG_PREV_NEXT )
	{
		#define BUTTON_W 32

		b = new wxButton(this_, IDC_PREVDLGPAGE, wxT("<<"), wxDefaultPosition, wxSize(BUTTON_W, -1));
		if( !prevTitle.IsEmpty() ) { b->SetToolTip(prevTitle); }
		buttonSizer->Add(b, 0, wxRIGHT, SJ_DLG_SPACE/2);

		b = new wxButton(this_, IDC_NEXTDLGPAGE, wxT(">>"), wxDefaultPosition,  wxSize(BUTTON_W, -1));
		if( !nextTitle.IsEmpty() ) { b->SetToolTip(nextTitle); }
		buttonSizer->Add(b, 0, wxRIGHT, SJ_DLG_SPACE);
	}

	buttonSizer->Add(1,
	                 1,
	                 1, wxGROW|wxRIGHT, SJ_DLG_SPACE*2);

	if( flags & SJ_DLG_OK )
	{
		buttonDefault = new wxButton(this_, wxID_OK, okTitle.IsEmpty()? wxString(_("OK")) : okTitle);
		buttonSizer->Add(buttonDefault, 0, 0, SJ_DLG_SPACE);
	}

	if( flags & SJ_DLG_CANCEL )
	{
		b = new wxButton(this_, wxID_CANCEL,  cancelTitle.IsEmpty()? wxString(_("Cancel")) : cancelTitle);
		buttonSizer->Add(b, 0, wxLEFT, SJ_DLG_SPACE);
	}

	// finally, set the default button. do it last to avoid
	// miscalculated button sizes eg. under Motif
	if( buttonDefault )
	{
		buttonDefault->SetDefault();
	}

	return buttonSizer;
}


void SjDialog::SetCbWidth(wxWindow* window, long maxW)
{
	wxClientDC dc(window);
	dc.SetFont(FindTopLevel(window)->GetFont());

	wxString className = window->GetClassInfo()->GetClassName();
	if( className == wxT("wxChoice") )
	{
		wxChoice* c = (wxChoice*)window;

		long i, iCount = c->GetCount();
		wxCoord w, h, reqW = 20;
		for( i = 0; i < iCount; i++ )
		{
			dc.GetTextExtent(c->GetString(i), &w, &h);
			if( w > reqW ) { reqW = w; }
		}

		#define OVERHEAD_W 32
		c->SetSize((reqW+OVERHEAD_W)>maxW? maxW : reqW+OVERHEAD_W, -1);
	}
}


bool SjDialog::SetCbSelection(wxChoice* cb, long userData)
{
	int i, iCount = cb->GetCount();
	for( i = 0; i < iCount; i++ )
	{
		if( cb->GetClientData(i)==(void*)userData )
		{
			cb->SetSelection(i);
			return true;
		}
	}
	return false;
}


long SjDialog::GetCbSelection(wxChoice* cb, long defVal)
{
	int index = cb->GetSelection();
	return (index>=0 && index<(int)cb->GetCount())? (long)cb->GetClientData(index) : defVal;
}


void SjDialog::AppendToCb(wxComboBox* cb, const wxString& str)
{
	// append if not yet in list
	// we cannot use FindString() as it returns -1 for empty strings
	wxASSERT(cb);
	int i, cnt = cb->GetCount();
	for( i = 0; i < cnt; i++ )
	{
		if( cb->GetString(i) == str )
		{
			return; // don't append, string is already in list
		}
	}

	// append as the string is not yet in the list
	cb->Append(str);
}


bool SjDialog::ApplyToBitfield(wxCheckBox* cb, long& bitfield, long flagToApply)
{
	bool bitChanged = FALSE;

	if( cb->GetValue() )
	{
		if( !(bitfield & flagToApply) )
		{
			bitfield |= flagToApply;
			bitChanged = TRUE;
		}
	}
	else
	{
		if( bitfield & flagToApply )
		{
			bitfield &= ~flagToApply;
			bitChanged = TRUE;
		}
	}

	return bitChanged;
}


long SjDialog::GetSelListCtrlItem(wxListCtrl* listCtrl)
{
	wxASSERT(listCtrl);

	long count = listCtrl->GetItemCount(), currIndex;

	for( currIndex = 0; currIndex < count; currIndex++ )
	{
		if( listCtrl->GetItemState(currIndex,  wxLIST_STATE_SELECTED) &  wxLIST_STATE_SELECTED )
		{
			return currIndex;
		}
	}

	return -1;
}


long SjDialog::EnsureSelListCtrlItemVisible(wxListCtrl* listCtrl)
{
	long currIndex = GetSelListCtrlItem(listCtrl);

	if( currIndex != -1 )
	{
		listCtrl->EnsureVisible(currIndex);
	}

	return currIndex;
}


wxWindow* SjDialog::FindTopLevel(wxWindow* w__)
{
	wxWindow* w = w__;
	while( w )
	{
		if( w->IsTopLevel() )
		{
			return w;
		}

		w = w->GetParent();
	}
	return w__;
}


wxWindow* SjDialog::FindWindowFromHandle(void* handle)
{
	wxWindowList::Node *node;
	for ( node = wxTopLevelWindows.GetFirst(); node; node = node->GetNext() )
	{
		wxWindow *winTop = node->GetData();
		if ( winTop->GetHandle() == handle )
		{
			return winTop;
		}
	}
	return NULL;
}


wxWindow* SjDialog::GetSuitableDlgParent()
{
	// we're searching for a top-leven window or dialog that can be used as the parent
	// for a modeless dialog.  The desired window or dialog must be shown and must not
	// be disabled; the active window or dialog is preferred.

	// try the top-level application window
	if( g_mainFrame && g_mainFrame->IsShown() && g_mainFrame->IsEnabled() )
		return g_mainFrame;

	// try active (modal) dialogs or windows
	wxWindowList::Node *node;
	for ( node = wxTopLevelWindows.GetFirst(); node; node = node->GetNext() )
	{
		wxTopLevelWindow* winTop = (wxTopLevelWindow*)node->GetData();
		if( winTop->IsActive() && winTop->IsShown() && winTop->IsEnabled() )
			return winTop;
	}

	// try any other (modal) dialogs or windows
	for ( node = wxTopLevelWindows.GetFirst(); node; node = node->GetNext() )
	{
		wxTopLevelWindow* winTop = (wxTopLevelWindow*)node->GetData();
		if( winTop->IsShown() && winTop->IsEnabled() )
			return winTop;
	}

	return NULL;
}


void SjDialog::PleaseRestartMsg(wxWindow* parent)
{
	wxWindowDisabler disabler(FindTopLevel(parent));
	SjMessageBox(wxString::Format(_("Please restart %s so that the changes can take effect."), SJ_PROGRAM_NAME),
	             SJ_PROGRAM_NAME, wxOK|wxICON_INFORMATION, FindTopLevel(parent));
}


/*******************************************************************************
 * Centering etc. regarding the current display
 ******************************************************************************/


wxRect SjDialog::GetDisplayWorkspaceRect(wxWindow* belongingWindow, wxPoint* fromPoint)
{
	wxRect  displWorkspaceRect;
	bool    doDefault = true;

	int displayIndex = wxNOT_FOUND;
	if( belongingWindow )
	{
		displayIndex = wxDisplay::GetFromWindow(belongingWindow);
	}
	else if( fromPoint )
	{
		displayIndex = wxDisplay::GetFromPoint(*fromPoint);
	}

	if( displayIndex != wxNOT_FOUND )
	{
		wxDisplay display(displayIndex);
		if( !display.IsPrimary() )
		{
			displWorkspaceRect = display.GetGeometry();
			doDefault = false;
		}
	}

	if( doDefault )
	{
		displWorkspaceRect = ::wxGetClientDisplayRect();
	}

	return displWorkspaceRect;
}


void SjDialog::EnsureRectDisplayVisibility(wxRect& r)
{
	// check against the available displays
	// at least a 128 x 128 pixel area should fit on any display!
	//
	// otherwise we align the rect to the primary display, in this case the
	// skin may be misaligned, however, there is no more space

	bool alignToPrimaryWorkspace = true;

	size_t displayCount = wxDisplay::GetCount(), d;
	if( displayCount > 1 )
	{
		for( d = 0; d < displayCount; d++ )
		{
			wxDisplay   currDisplay(d);
			wxRect      currDisplayRect = currDisplay.GetGeometry();

			wxRect      intersection = currDisplayRect.Intersect(r);
			if( intersection.width*intersection.height > 128*128 )
			{
				alignToPrimaryWorkspace = false;
				break; // done
			}
		}
	}

	if( alignToPrimaryWorkspace )
	{
		wxRect displRect = wxGetClientDisplayRect();

		if( r.width  > displRect.width ) r.width = displRect.width;
		if( r.height > displRect.height ) r.height = displRect.height;

		if( r.x < displRect.x ) r.x = displRect.x;
		if( r.x > displRect.x + displRect.width - r.width ) r.x = displRect.x + displRect.width - r.width;

		if( r.y < displRect.y ) r.y = displRect.y;
		if( r.y > displRect.y + displRect.height - r.height ) r.y = displRect.y + displRect.height - r.height;
	}
}



// centre the window with respect to its parent in either (or both) directions
void SjDialog::CenterOnDisplay(wxWindow* this_, wxWindow* parent_)
{
	int windowW, windowH;
	this_->GetSize(&windowW, &windowH);

	wxRect displRect = GetDisplayWorkspaceRect(parent_? parent_ : this_->GetParent());

	if( g_mainFrame && g_mainFrame->IsKioskStarted() )
	{
		displRect = g_mainFrame->GetRect();
	}

	int xNew, yNew;
	xNew = displRect.x + displRect.width/2 - windowW/2; if( xNew < displRect.x ) xNew = displRect.x;
	yNew = displRect.y + displRect.height/2 - windowH/2; if( yNew < displRect.y ) yNew = displRect.y;

	this_->SetSize(xNew, yNew, -1, -1, wxSIZE_ALLOW_MINUS_ONE);
}


void SjDialog::AddStateBox(wxSizer* parentSizer)
{
	#if 1
		wxSizer* sizer2 = new wxStaticBoxSizer(new wxStaticBox(this, -1, _("State")), wxVERTICAL);
		parentSizer->Add(sizer2, 0, wxGROW|wxALL, SJ_DLG_SPACE);
	#else
		wxSizer* sizer2 = parentSizer;
	#endif

	m_stateSizer = new wxFlexGridSizer(2, 0, 0);
	m_stateSizer->AddGrowableCol(1);
	sizer2->Add(m_stateSizer, 0, wxALL|wxGROW, SJ_DLG_SPACE);
}


void SjDialog::AddState(const wxString& label, int id, const wxString& value)
{
	wxASSERT( m_stateSizer );

	m_stateSizer->Add(new wxStaticText(this, -1, label),
	                  0, wxRIGHT|wxALIGN_CENTER_VERTICAL, SJ_DLG_SPACE);

	m_stateSizer->Add(new wxStaticText(this, id, value, wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE),
	                  0, wxALIGN_CENTER_VERTICAL|wxGROW);
}


SjDialogPos::SjDialogPos(const wxString& regKey)
{
	m_regKey = regKey;
	m_forDisplay = wxNOT_FOUND;
}


void SjDialogPos::Save(wxTopLevelWindow* dlg)
{
	if( !dlg->IsMaximized() && !dlg->IsIconized() )
	{
		m_rect = dlg->GetRect();

		m_forDisplay = wxDisplay::GetFromWindow(g_mainFrame);
		if( m_forDisplay != wxNOT_FOUND )
		{
			wxDisplay display(m_forDisplay);
			m_forDisplayGeometry = display.GetGeometry();
		}

		g_tools->WriteRect(m_regKey, m_rect);
	}
}


void SjDialogPos::Restore(wxTopLevelWindow* dlg)
{
	wxRect storedRect = g_tools->ReadRect(m_regKey);

	// set width and height
	if( storedRect.width > 0 && storedRect.width < 5000
	        && storedRect.height > 0 && storedRect.height < 5000 )
	{
		dlg->SetSize(0, 0, storedRect.width,  storedRect.height);
	}

	// set the position if the main frame is still on the same display
	int forDisplay = wxDisplay::GetFromWindow(g_mainFrame);
	wxRect forDisplayGeometry;
	if( forDisplay != wxNOT_FOUND )
	{
		wxDisplay display(forDisplay);
		forDisplayGeometry = display.GetGeometry();
	}

	if( forDisplay != wxNOT_FOUND
	        && forDisplay == m_forDisplay
	        && forDisplayGeometry == m_forDisplayGeometry )
	{
		dlg->SetSize(storedRect.x, storedRect.y, -1, -1, wxSIZE_USE_EXISTING);
	}
	else
	{
		dlg->CentreOnParent();
	}
}


/*******************************************************************************
 * SjDlgCheckCtrl class - a check box plus a control
 ******************************************************************************/


SjDlgCheckCtrl::SjDlgCheckCtrl()
{
	m_checkBox  = NULL;
	m_spinCtrl  = NULL;
	m_staticText= NULL;
}


int SjDlgCheckCtrl::GetSpinW(int ctrlMin, int ctrlMax)
{
	int minLen = wxString::Format(wxT("%i"), ctrlMin).Len();
	int maxLen = wxString::Format(wxT("%i"), ctrlMax).Len();

	if( minLen > maxLen /*may be true for negative values*/ )
	{
		maxLen = minLen;
	}

	if( maxLen >= 4 )
	{
		return SJ_4DIG_SPINCTRL_W;
	}
	else
	{
		return SJ_3DIG_SPINCTRL_W;
	}
}


void SjDlgCheckCtrl::Create(wxWindow* parentWindow, wxSizer* parentSizer,
                            const wxString& text, long sizerBorder,
                            int idCheckBox, bool checkVal,
                            int idCtrl, int intCtrlVal, int ctrlMin, int ctrlMax)
{
	m_backupedCheckValValid = FALSE;
	m_sizer = new wxBoxSizer(wxHORIZONTAL);
	parentSizer->Add(m_sizer, 0, sizerBorder, SJ_DLG_SPACE);

	m_checkBox = new wxCheckBox(parentWindow, idCheckBox,
	                            text.BeforeFirst('%').Trim());
	m_checkBox->SetValue(checkVal);
	m_sizer->Add(m_checkBox, 0, wxALIGN_CENTER_VERTICAL);

	m_spinCtrl = new wxSpinCtrl(parentWindow, idCtrl, wxString::Format(wxT("%i"), intCtrlVal),
	                            wxDefaultPosition, wxSize(GetSpinW(ctrlMin, ctrlMax), -1), wxSP_ARROW_KEYS,
	                            ctrlMin, ctrlMax, intCtrlVal);
	m_sizer->Add(m_spinCtrl, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, SJ_DLG_SPACE);

	m_staticText = new wxStaticText(parentWindow, -1,
	                                text.AfterFirst('%').Mid(1).Trim(FALSE));
	m_sizer->Add(m_staticText, 0, wxALIGN_CENTER_VERTICAL);
}


void SjDlgCheckCtrl::Enable(bool enable)
{
	if( m_checkBox )
	{
		wxASSERT( m_spinCtrl && m_checkBox && m_staticText );

		if( enable )
		{
			if( m_backupedCheckValValid )
			{
				m_checkBox->SetValue(m_backupedCheckVal);
				m_backupedCheckValValid = FALSE;
			}
		}
		else
		{
			m_backupedCheckVal = m_checkBox->IsChecked();
			m_backupedCheckValValid = TRUE;
			m_checkBox->SetValue(FALSE);
		}

		m_checkBox->Enable(enable);
		m_spinCtrl->Enable(enable&&m_checkBox->IsChecked());
		m_staticText->Enable(enable);
	}
}


void SjDlgCheckCtrl::Update()
{
	wxASSERT( m_spinCtrl && m_checkBox );

	m_spinCtrl->Enable(m_checkBox->IsChecked());
}


void SjDlgCheckCtrl::SetChecked(bool state)
{
	wxASSERT( m_spinCtrl && m_checkBox );

	m_checkBox->SetValue(state);
	Update();
}


void SjDlgCheckCtrl::SetValue(int value)
{
	wxASSERT(m_spinCtrl);
	m_spinCtrl->SetValue(value);
}


bool SjDlgCheckCtrl::IsChecked()
{
	wxASSERT(m_checkBox);
	return m_checkBox->IsChecked();
}


int SjDlgCheckCtrl::GetValue()
{
	wxASSERT(m_spinCtrl);
	return m_spinCtrl->GetValue();
}


/*******************************************************************************
 * SjDlgSlider
 ******************************************************************************/


SjDlgSlider::SjDlgSlider()
{
	m_slider = NULL;
	m_label = NULL;
}


void SjDlgSlider::Create(wxWindow* parentWindow, wxSizer* parentSizer,
                         int idSlider,
                         long type,
                         long curr, long min, long max)
{
	m_type = type;
	m_slider = new wxSlider(parentWindow, idSlider,
	                        curr, min, max,
	                        wxDefaultPosition, wxSize(180, -1), wxSL_HORIZONTAL);
	parentSizer->Add(m_slider, 0, wxALIGN_CENTER_VERTICAL);

	m_label = new wxStaticText(parentWindow, -1, RenderLabel(curr, min, max), wxDefaultPosition, wxDefaultSize);
	parentSizer->Add(m_label, 0, wxALIGN_CENTER_VERTICAL);
}


wxString SjDlgSlider::RenderLabel(long val)
{
	long snap = m_type & SJ_SLIDER_SNAP;
	if( snap > 0 )
	{
		val = ((val+(snap/2)) / snap) * snap;
	}

	wxString str = SjTools::FormatNumber(val);
	if( m_type & SJ_SLIDER_MS )
	{
		str += wxT(" ms");
	}
	else if( m_type & SJ_SLIDER_MS_SEC )
	{
		str = SjTools::FormatNumber((val+500)/1000);
		str += wxT(" ");
		str += _("seconds");
	}
	return str;
}


wxString SjDlgSlider::RenderLabel(long val, long min, long max)
{
	wxString valStr = RenderLabel(val) + wxT(" ");
	wxString minStr = RenderLabel(min);
	wxString maxStr = RenderLabel(max);
	while( valStr.Len() < minStr.Len()
	    || valStr.Len() < maxStr.Len() )
	{
		valStr += wxT(" ");
	}
	return valStr;
}


/*******************************************************************************
 * SjHistoryCombobox
 ******************************************************************************/


SjHistoryComboBox::SjHistoryComboBox(wxWindow* parent, int id, int w, const wxArrayString& history)
	: wxComboBox(parent, id, wxT(""), wxDefaultPosition, wxSize(w, -1))
{
	Init(history);
}


void SjHistoryComboBox::Init(const wxArrayString& history)
{
	Clear();

	int i, iCount = (int)history.GetCount(), realCount = 0;
	for( i = 0; i < iCount; i++ )
	{
		if( !history[i].Trim().Trim(FALSE).IsEmpty() )
		{
			Append(history[i]);
			realCount++;
			if( realCount >= HISTORY_MAX )
			{
				break;
			}
		}
	}

	SetValue(history[0]);
}


wxArrayString SjHistoryComboBox::GetHistory() const
{
	wxArrayString   ret;
	wxString        curr;

	// the first element is always the current value - even if empty
	ret.Add(GetValue());

	// add the elements from the history
	int i, iCount = GetCount();
	for( i = 0; i < iCount; i++ )
	{
		curr = GetString(i);
		if( ret.Index(curr) == wxNOT_FOUND )
		{
			ret.Add(curr);
			if( ret.GetCount() >= HISTORY_MAX )
			{
				break;
			}
		}
	}

	// done
	return ret;
}


/*******************************************************************************
 *  SjDlgOptions - simple options handling
 ******************************************************************************/


SjDlgCtrl* SjDlgControls::AddCtrl(SjDlgCtrlType type, const wxString& id, const wxString& label)
{
	SjDlgCtrl* dc = new SjDlgCtrl();
	dc->m_type = type;
	dc->m_id = id;
	dc->m_label = label;
	dc->m_index = m_ctrl.GetCount();
	m_ctrl.Add(dc);
	return dc;
}


SjDlgCtrl* SjDlgControls::AddTextCtrl(const wxString& id, const wxString& label, const wxString& value, const wxString& defaultValue, long style)
{
	SjDlgCtrl* dc = AddCtrl(SJ_DLG_TEXTCTRL_TYPE, id, label);
	dc->m_value = value;
	dc->m_defaultValue = defaultValue;
	dc->m_style = style;
	return dc;
}


SjDlgCtrl* SjDlgControls::AddSelectCtrl(const wxString& id, const wxString& label, long value, long defaultValue, const wxArrayString& options)
{
	SjDlgCtrl* dc = AddCtrl(SJ_DLG_SELECTCTRL_TYPE, id, label);
	dc->m_value = wxString::Format(wxT("%i"), (int)value);
	dc->m_defaultValue = wxString::Format(wxT("%i"), (int)defaultValue);
	dc->m_options = options;
	return dc;
}


SjDlgCtrl* SjDlgControls::AddSelectCtrl(const wxString& id, const wxString& label, long value, long defaultValue, const wxString& optionsStr)
{
	wxArrayString optionsArr = SjTools::Explode(optionsStr, '|', 1/*return at least one element*/);
	return AddSelectCtrl(id, label, value, defaultValue, optionsArr);
}


SjDlgCtrl* SjDlgControls::AddCheckCtrl(const wxString& id, const wxString& label, long value, long defaultValue)
{
	SjDlgCtrl* dc = AddCtrl(SJ_DLG_CHECKCTRL_TYPE, id, label);
	dc->m_value = wxString::Format(wxT("%i"), (int)value);
	dc->m_defaultValue = wxString::Format(wxT("%i"), (int)defaultValue);
	return dc;
}


SjDlgCtrl* SjDlgControls::AddStatic(const wxString& id, const wxString& label)
{
	SjDlgCtrl* dc = AddCtrl(SJ_DLG_STATICCTRL_TYPE, id, label);
	return dc;
}


SjDlgCtrl* SjDlgControls::AddButton(const wxString& id, const wxString& label)
{
	SjDlgCtrl* dc = AddCtrl(SJ_DLG_BUTTON_TYPE, id, label);
	return dc;
}


long SjDlgControls::Id2Index(const wxString& id) const
{
	int index, iCount = (int)m_ctrl.GetCount();
	for( index = 0; index < iCount; index++ )
	{
		if( m_ctrl[index].m_id == id )
			return index;
	}
	return -1;
}


wxString SjDlgControls::GetValue(long index) const
{
	if( index < 0 || index >= (int)m_ctrl.GetCount() )
		return wxT("");

	SjDlgCtrl& dc = m_ctrl[index];
	if( dc.m_wndCtrl )
	{
		if( dc.m_type == SJ_DLG_TEXTCTRL_TYPE )
		{
			SjVirtKeybdTextCtrl* textCtrl = (SjVirtKeybdTextCtrl*)dc.m_wndCtrl;
			dc.m_value = textCtrl->GetValue();
		}
		else if( dc.m_type == SJ_DLG_SELECTCTRL_TYPE )
		{
			wxChoice* choice = (wxChoice*)dc.m_wndCtrl;
			int sel = choice->GetSelection();
			if( sel==-1 )
				sel = 0;
			dc.m_value.Printf(wxT("%i"), sel);
		}
		else if( dc.m_type == SJ_DLG_CHECKCTRL_TYPE )
		{
			wxCheckBox* checkBox = (wxCheckBox*)dc.m_wndCtrl;
			dc.m_value.Printf(wxT("%i"), (int)(checkBox->GetValue()?1:0));
		}
	}

	return dc.m_value;
}


long SjDlgControls::GetValueLong(long index)  const
{
	wxString valueStr = GetValue(index);
	long valueLong;
	valueStr.ToLong(&valueLong);
	return valueLong;
}


SjDlgCtrl* SjDlgControls::GetCtrl(long index) const
{
	if( index < 0 || index >= (int)m_ctrl.GetCount() )
		return NULL;

	return &(m_ctrl.Item(index));
}


void SjDlgControls::SetValue(long index, const wxString& value)
{
	if( index < 0 || index >= (int)m_ctrl.GetCount() )
		return;

	SjDlgCtrl& dc = m_ctrl[index];
	dc.m_value = value;

	if( dc.m_wndCtrl )
	{
		long valueLong;
		value.ToLong(&valueLong);

		if( dc.m_type == SJ_DLG_TEXTCTRL_TYPE )
		{
			SjVirtKeybdTextCtrl* textCtrl = (SjVirtKeybdTextCtrl*)dc.m_wndCtrl;
			textCtrl->SetValue(value);
		}
		else if( dc.m_type == SJ_DLG_SELECTCTRL_TYPE )
		{
			wxChoice* choice = (wxChoice*)dc.m_wndCtrl;
			choice->SetSelection(valueLong);
		}
		else if( dc.m_type == SJ_DLG_CHECKCTRL_TYPE )
		{
			wxCheckBox* checkBox = (wxCheckBox*)dc.m_wndCtrl;
			checkBox->SetValue(valueLong!=0);
		}
		else if( dc.m_type == SJ_DLG_STATICCTRL_TYPE )
		{
			// added in 2.52beta15 due to http://www.silverjuke.net/forum/post.php?p=8037#8037
			wxStaticText* staticText = (wxStaticText*)dc.m_wndCtrl;
			staticText->SetLabel(value);
		}
	}
}


long SjDlgControls::Render(wxWindow* parent, wxSizer* parentSizer, int idButtons,
                           wxString* retDefOkTitle, wxString* retDefCancelTitle)
{
	wxString retDefTitleDummy;
	if( retDefOkTitle == NULL )
	{
		retDefOkTitle = &retDefTitleDummy;
	}

	if( retDefCancelTitle == NULL )
	{
		retDefCancelTitle = &retDefTitleDummy;
	}

	long defButtons = 0;
	int index, iCount = (int)m_ctrl.GetCount();
	if( iCount )
	{
		wxGridBagSizer* gbsizer = new wxGridBagSizer(SJ_DLG_SPACE/2, SJ_DLG_SPACE);
		gbsizer->AddGrowableCol(1);
		parentSizer->Add(gbsizer, 0, wxALL|wxGROW, SJ_DLG_SPACE);

		int posY = 0;
		for( index = 0; index < iCount; index++ )
		{
			SjDlgCtrl& dc = m_ctrl[index];

			long valueLong;
			dc.m_value.ToLong(&valueLong);

			// create control
			bool hasLabel = true;
			long controlSizeStyle = 0;
			if( dc.m_type == SJ_DLG_TEXTCTRL_TYPE )
			{
				SjVirtKeybdTextCtrl* textCtrl = new SjVirtKeybdTextCtrl(parent, -1, dc.m_value, wxDefaultPosition, wxDefaultSize,
				        dc.m_style);
				dc.m_wndCtrl = textCtrl;
				controlSizeStyle |= wxGROW;
			}
			else if( dc.m_type == SJ_DLG_SELECTCTRL_TYPE )
			{
				wxChoice* choice = new wxChoice(parent, -1);
				dc.m_wndCtrl = choice;
				int j, jCount = dc.m_options.GetCount();
				for( j = 0; j < jCount; j++ )
				{
					choice->Append(dc.m_options[j]);
				}
				choice->SetSelection(valueLong);
				//SjTools::SetCbWidth(choice); -- look crazy if all choices have different widths
			}
			else if( dc.m_type == SJ_DLG_CHECKCTRL_TYPE )
			{
				wxCheckBox* checkBox = new wxCheckBox(parent, -1, dc.m_label);
				checkBox->SetValue(valueLong!=0);
				dc.m_wndCtrl = checkBox;
				hasLabel = false;
			}
			else if( dc.m_type == SJ_DLG_STATICCTRL_TYPE )
			{
				wxStaticText* staticText = new wxStaticText(parent, -1, dc.m_label);
				dc.m_wndCtrl = staticText;
				hasLabel = false;
			}
			else if( dc.m_type == SJ_DLG_BUTTON_TYPE )
			{
				if( dc.m_id==wxT("ok") )     { defButtons |= SJ_DLG_OK; *retDefOkTitle = dc.m_label; continue; }
				if( dc.m_id==wxT("cancel") ) { defButtons |= SJ_DLG_CANCEL; *retDefCancelTitle = dc.m_label; continue; }

				wxButton* button = new wxButton(parent, idButtons, dc.m_label);
				dc.m_wndCtrl = button;
				hasLabel = false;

			}
			else
			{
				wxASSERT( 0 );
				continue;
			}

			// add label
			if( hasLabel )
			{
				wxStaticText* label = new wxStaticText(parent, -1, dc.m_label);

				gbsizer->Add(label, 0, wxALIGN_CENTER_VERTICAL);
				gbsizer->SetItemPosition(label, wxGBPosition(posY, 0));

				dc.m_wndLabel = label;
			}

			// add control
			wxASSERT( dc.m_wndCtrl );
			dc.m_wndCtrl->SetClientData((void*)((uintptr_t)index));

			gbsizer->Add(dc.m_wndCtrl, 0, wxALIGN_CENTER_VERTICAL|controlSizeStyle);
			gbsizer->SetItemPosition(dc.m_wndCtrl, wxGBPosition(posY, hasLabel?1:0));
			if( !hasLabel )
				gbsizer->SetItemSpan(dc.m_wndCtrl, wxGBSpan(1, 2));

			posY ++;
		}
	}

	return defButtons;
}


void SjDlgControls::Unrender()
{
	int index, iCount = (int)m_ctrl.GetCount();
	for( index = 0; index < iCount; index++ )
	{
		GetValue(index); // force copy from the control
		SjDlgCtrl& dc = m_ctrl[index];
		dc.m_wndCtrl = NULL;
		dc.m_wndLabel = NULL;
	}
}


void SjDlgControls::Enable(bool enable)
{
	int index, iCount = (int)m_ctrl.GetCount();
	for( index = 0; index < iCount; index++ )
	{
		SjDlgCtrl& dc = m_ctrl[index];
		if( dc.m_wndLabel )
			dc.m_wndLabel->Enable(enable);

		if( dc.m_wndCtrl )
			dc.m_wndCtrl->Enable(enable);
	}
}


void SjDlgControls::SetToDefaults()
{
	int index, iCount = (int)m_ctrl.GetCount();
	for( index = 0; index < iCount; index++ )
	{
		SetValue(index, m_ctrl[index].m_defaultValue);
	}
}
