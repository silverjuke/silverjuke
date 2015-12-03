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
 * File:    dialogsj.h
 * Authors: Björn Petersen
 * Purpose: Dialog base-classes
 *
 ******************************************************************************/


#ifndef __SJ_DIALOG_H__
#define __SJ_DIALOG_H__


#ifndef SJ_DLG_SPACE
#define SJ_DLG_SPACE 7L
#endif

#ifndef SJ_CHOICE_SEP
#define SJ_CHOICE_SEP wxT("--------------------------")
#endif

#ifndef SJ_BUTTON_MENU_ARROW
# if wxUSE_UNICODE
#  define SJ_BUTTON_MENU_ARROW wxT(" \u25BE") // ="BLACK DOWN-POINTING SMALL TRIANGLE" (\u25BC="BLACK DOWN-POINTING TRIANGLE" is too large)
# else
#  define SJ_BUTTON_MENU_ARROW wxT(" v")
# endif
#endif


class wxSpinCtrl;


enum SjDialogMode
{
    SJ_MODAL,
    SJ_MODELESS
};


enum SjDialogResizeType
{
    SJ_ALWAYS_RESIZEABLE,
    SJ_NEVER_RESIZEABLE,
    SJ_RESIZEABLE_IF_POSSIBLE
};


class SjDialog : public wxDialog
{
public:
	// constructor
	                SjDialog            (wxWindow* parent, const wxString& title, SjDialogMode, SjDialogResizeType, long addStyle=0);

	// tools
	#define         SJ_DLG_OK           0x01
	#define         SJ_DLG_CANCEL       0x02
	#define         SJ_DLG_PREV_NEXT    0x10
	#define         SJ_DLG_MENU         0x40
	#define         SJ_DLG_OK_CANCEL    (SJ_DLG_OK|SJ_DLG_CANCEL)
	wxSizer*        CreateButtons       (long flags, const wxString& okTitle=wxT(""), const wxString& cancelTitle=wxT(""), const wxString& prevTitle=wxT(""), const wxString& nextTitle=wxT("")) { return CreateButtons(this, flags, okTitle, cancelTitle, prevTitle, nextTitle); }
	static wxSizer* CreateButtons       (wxWindow*, long flags, const wxString& okTitle=wxT(""), const wxString& cancelTitle=wxT(""), const wxString& prevTitle=wxT(""), const wxString& nextTitle=wxT(""));

	static void     AppendToCb          (wxComboBox*, const wxString&); // append if not yet in list

	static void     SetCbWidth          (wxWindow*, long maxPixel=160);
	static void     SetCbDroppedWidth   (wxWindow*, long pixel);
	static bool     SetCbSelection      (wxChoice*, long userData);
	static long     GetCbSelection      (wxChoice*, long defVal = 0);

	static bool     ApplyToBitfield     (wxCheckBox*, long& bitfield, long flagToApply);
	static long     GetSelListCtrlItem  (wxListCtrl* listCtrl);
	static long     EnsureSelListCtrlItemVisible (wxListCtrl* listCtrl);
	static wxWindow*    FindTopLevel        (wxWindow*);
	static void         PleaseRestartMsg    (wxWindow* parent);

	static wxWindow*    FindWindowFromHandle(void*);
	static wxWindow*    GetSuitableDlgParent();


	///////////////////////////////////////////////
	// centering etc. regarding the correct display
	///////////////////////////////////////////////

	// the function finds out the workspace rectangle of the display the
	// given window belongs to. If NULL is given as the belonging window,
	// the primaries' display workspace is used.
	//
	// the workspace rect is the display geometry minus any toolbars etc.
	static wxRect   GetDisplayWorkspaceRect(wxWindow* belongingWindow, wxPoint* p=NULL);
	static wxRect   GetDisplayWorkspaceRect(int x, int y) { wxPoint p(x,y); return GetDisplayWorkspaceRect(NULL, &p); }

	// check against the available displays
	// at least a 128 x 128 pixel area should fit on any display!
	//
	// otherwise we align the rect to the primary display, in this case the
	// skin may be misaligned, however, there is no more space
	static void     EnsureRectDisplayVisibility (wxRect&);

	void            CenterOnDisplay     (wxWindow* parent) { CenterOnDisplay(this, parent); }
	static void     CenterOnDisplay     (wxWindow*, wxWindow* parent);

	// add a simple box showing some states; normally, this box is placed at the end of the dialog
	void            AddStateBox         (wxSizer* parentSizer);
	void            AddState            (const wxString& label, const wxString& value) {AddState(label, -1, value);}
	void            AddState            (const wxString& label, int id, const wxString& value=wxT(""));

private:
	wxFlexGridSizer* m_stateSizer;
};


class SjDlgCheckCtrl
{
public:
	// SjDlgCheckCtrl class - a check box plus a control
	SjDlgCheckCtrl      ();
	void            Create              (wxWindow* parentWindow, wxSizer* parentSizer,
	                                     const wxString& text, long sizerBorder,
	                                     int idCheckBox, bool checkVal,
	                                     int idCtrl, int ctrlVal, int ctrlMin, int ctrlMax);

	void            Enable              (bool enable);
	bool            IsEnabled           () { wxASSERT(m_checkBox); return m_checkBox->IsEnabled();  }

	void            Update              ();

	void            SetChecked          (bool state);
	void            SetValue            (int value);

	bool            IsChecked           ();
	int             GetValue            ();

	#define     SJ_3DIG_SPINCTRL_W  58
	#define     SJ_4DIG_SPINCTRL_W  74
	#define     SJ_8DIG_SPINCTRL_W  94

	int             GetSpinW            (int ctrlMin, int ctrlMax);
	wxSizer*        m_sizer;
	wxCheckBox*     m_checkBox;
	wxSpinCtrl*     m_spinCtrl;
	wxStaticText*   m_staticText;

private:
	bool            m_backupedCheckValValid;
	bool            m_backupedCheckVal;
};


class SjDlgSlider
{
public:
	// a slider plus a label
	                SjDlgSlider         ();

	#define         SJ_SLIDER_MS        0x10000000L
	#define         SJ_SLIDER_MS_SEC    0x20000000L // we use milliseconds internally but display seconds
	#define         SJ_SLIDER_SNAP      0x0000FFFFL
	#define         SJ_SLIDER_SNAP10            10L
	void            Create              (wxWindow* parentWindow, wxSizer* parentSizer,
	                                     int idSlider,
	                                     long type,
	                                     long curr, long min, long max);
	long            GetValue            () { return m_slider->GetValue(); }
	void            SetValue            (long v) { m_slider->SetValue(v); Update(); }
	void            Enable              (bool e) { m_slider->Enable(e); m_label->Enable(e); }

	// Update() should be called on slider events
	void            Update              () { m_label->SetLabel(RenderLabel(GetValue())); }
	wxString        RenderLabel         (long val);
	wxString        RenderLabel         (long val, long min, long max); // adds the max. number of spaces to the string

	// mainly for internal use ...
	long            m_type;
	wxSlider*       m_slider;
	wxStaticText*   m_label;
};


class SjHistoryComboBox : public wxComboBox
{
public:
	                SjHistoryComboBox   (wxWindow* parent, int id, int w, const wxArrayString& history);

	#define         HISTORY_MAX         16
	void            Init                (const wxArrayString& history);
	wxArrayString   GetHistory          () const;
};


enum SjDlgCtrlType
{
    SJ_DLG_TEXTCTRL_TYPE,
    SJ_DLG_SELECTCTRL_TYPE,
    SJ_DLG_CHECKCTRL_TYPE,
    SJ_DLG_STATICCTRL_TYPE,
    SJ_DLG_BUTTON_TYPE,
};


class SjDlgCtrl
{
public:
	                SjDlgCtrl() { m_style = 0; m_wndLabel = NULL; m_wndCtrl = NULL; }

	SjDlgCtrlType   m_type;
	wxString        m_id;
	wxString        m_label;
	wxString        m_value;
	wxArrayString   m_options;
	wxString        m_defaultValue;
	long            m_style;
	wxWindow        *m_wndLabel, *m_wndCtrl;
	long            m_index;
};
WX_DECLARE_OBJARRAY(SjDlgCtrl, SjArrayDlgCtrl);


class SjDlgControls
{
public:
	                SjDlgControls       () {}
	SjDlgCtrl*      AddTextCtrl         (const wxString& id, const wxString& label, const wxString& value, const wxString& defaultValue, long style);
	SjDlgCtrl*      AddSelectCtrl       (const wxString& id, const wxString& label, long value, long defaultValue, const wxArrayString& options);
	SjDlgCtrl*      AddSelectCtrl       (const wxString& id, const wxString& label, long value, long defaultValue, const wxString& options /*use "|" as separator*/);
	SjDlgCtrl*      AddCheckCtrl        (const wxString& id, const wxString& label, long value, long defaultValue);
	SjDlgCtrl*      AddStatic           (const wxString& id, const wxString& label);
	SjDlgCtrl*      AddButton           (const wxString& id, const wxString& label);

	// for the user, who does not want to hazzle around with dialog creation ...
	long            Id2Index            (const wxString& id) const;
	long            GetCount            () const {return m_ctrl.GetCount();}
	wxString        GetValue            (long index) const;
	long            GetValueLong        (long index) const;
	SjDlgCtrl*      GetCtrl             (long index) const;
	void            SetValue            (long index, const wxString& value);

	// for the dialog owner ...
	long            Render              (wxWindow* parent, wxSizer*, int idButtons=-1, wxString* retDefOkTitle=NULL, wxString* retDefCancelTitle=NULL);
	void            Unrender            ();
	void            Enable              (bool enable); // this function needs the controls to be already rendered!
	void            SetToDefaults       ();

private:
	SjDlgCtrl*      AddCtrl             (SjDlgCtrlType type, const wxString& id, const wxString& label);
	SjArrayDlgCtrl  m_ctrl;
};


#endif // __SJ_DIALOG_H__
