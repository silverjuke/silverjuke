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
 * File:    tageditorplugin.h
 * Authors: Björn Petersen
 * Purpose: The tag editor module
 *
 ******************************************************************************/



#ifndef __SJ_TAGEDITOR_PLUGIN_H__
#define __SJ_TAGEDITOR_PLUGIN_H__



class SjInsertButton : public wxButton
{
public:
	                SjInsertButton      (wxWindow* parent, int idButton, int idFirstOption);
	void            AddOption           (const wxString& name, const wxString& descr);
	void            AddCaseOption       (const wxString& name, const wxString& descr);
	void            AddWidthOption      (const wxString& name, const wxString& descr);
	void            AddSep              () { AddOption(wxT(""), wxT("")); }
	void            OnOption            (wxCommandEvent&, wxComboBox* dest);

private:
	int             m_idButton;
	int             m_idFirstOption;
	wxArrayString   m_optName;
	wxArrayString   m_optDescr;
};



class SjTrackInfoFieldChoice : public wxChoice
{
public:
					SjTrackInfoFieldChoice (wxWindow* parent, int id);
	void            AppendFlags            (long tiFlags);
};



class SjTagEditorPlugin : public SjDialog
{
public:
	                SjTagEditorPlugin   (wxWindow* parent, const wxString& name, const wxString& title, SjTrackInfo* exampleTrackInfo);
	virtual         ~SjTagEditorPlugin  () { }
	void            AfterConstructor    ();

	// This function is called once before a serie of ModifyTrackInfo();
	virtual bool    PrepareModify       () { return FALSE; };

	// Change the given track information; any modifications must
	// be added to SjConfirm using Add()
	virtual void    ModifyTrackInfo     (SjTrackInfo&, int index, SjModifyInfo&) { }

	// Called after a serie of ModifyTrackInfo(),
	// you may return a message that should be presented to the user.
	virtual wxString PostModify          () { return wxEmptyString; }

protected:
	// misc.
	void            OnCommand              (wxCommandEvent&);
	bool            TransferDataFromWindow () { return TRUE; }
	wxString        m_name;
	wxSizer*        m_sizer1;
	SjTrackInfo*    m_exampleTrackInfo;
};



class SjModifyListCtrl : public wxListCtrl
{
public:
	                SjModifyListCtrl    (wxWindow* parent, wxWindowID id, SjModifyInfo&, bool showUrlsOnly);
	void            SizeColumns         ();

	#define         CONFIRM_COL1_OLDURL 0
	#define         CONFIRM_COL1_NEWURL 1

	#define         CONFIRM_COL2_URL    0
	#define         CONFIRM_COL2_FIELD  1
	#define         CONFIRM_COL2_OLDVAL 2
	#define         CONFIRM_COL2_NEWVAL 3

	wxString        OnGetItemText       (long item, long column) const;
	int             OnGetItemImage      (long item) const;

private:
	bool            m_showUrlsOnly;
	SjModifyInfo*   m_items;
	wxString        GetValDescr         (SjModifyItem* item, bool newVal) const;
};




class SjConfirmDlg : public SjTagEditorPlugin
{
public:
	                SjConfirmDlg        (wxWindow* parent, SjModifyInfo&, bool askWriteId3, bool askDelEmptyDir, bool onlyUrlsModified);

private:
	// data
	SjModifyInfo*   m_items;
	bool            m_itemsEdited;
	bool            TransferDataFromWindow ();
	// controls
	wxStaticText*   m_msgTextCtrl;
	wxString        GetMsg              () const;

	wxCheckBox*     m_writeId3CheckBox;
	wxCheckBox*     m_delEmptyDirCheckBox;

	SjModifyListCtrl* m_listCtrl;

	// events
	#define         IDC_CONFIRM_LIST    (IDM_FIRSTPRIVATE+1)
	#define         IDC_CONFIRM_EDIT    (IDM_FIRSTPRIVATE+2)
	#define         IDC_CONFIRM_DELETE  (IDM_FIRSTPRIVATE+3)
	void            OnDoubleClick       (wxListEvent&) { QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDC_CONFIRM_EDIT)); }
	void            OnContextMenu       (wxListEvent&);
	void            OnKeyDown           (wxListEvent&);
	void            OnEdit              (wxCommandEvent&);
	void            OnDelete            (wxCommandEvent&);
	void            OnSize              (wxSizeEvent& event) { SjDialog::OnSize(event); if(m_listCtrl) { m_listCtrl->SizeColumns(); } }
	DECLARE_EVENT_TABLE ()
};


#endif // __SJ_TAGEDITOR_PLUGIN_H__
