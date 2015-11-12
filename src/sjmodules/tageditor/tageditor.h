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
 * File:    tageditor.h
 * Authors: Björn Petersen
 * Purpose: The tag editor module
 *
 ******************************************************************************/


#ifndef __SJ_TAGEDITOR_H__
#define __SJ_TAGEDITOR_H__


class wxNotebook;


/*******************************************************************************
 *  Tracking modifications
 ******************************************************************************/


class SjModifyItem
{
public:
	                SjModifyItem        (long field, const wxString& url, const wxString& oldVal) { m_field = field; m_url = url; m_oldVal = oldVal; }

	wxString        GetUrl              () const { return m_url; }
	long            GetField            () const { return m_field; }
	wxString        GetOldVal           () const { return m_oldVal; }

	void            SetNewVal           (const wxString& newVal);
	wxString        GetNewVal           () const { return m_newVal; }

	bool            IsModified          () const { return (m_newVal!=m_oldVal); }

private:
	wxString        m_url;
	long            m_field;
	wxString        m_oldVal;
	wxString        m_newVal;
};


WX_DECLARE_OBJARRAY(SjModifyItem, SjArrayModifyItem);


class SjModifyInfo
{
public:
	SjModifyInfo        ()
	{	m_scope=NULL;
	}

	long            GetCount            ()
	{	return m_hash.GetCount();
	}

	SjModifyItem*   GetItem             (int i)
	{	if(i<0||i>=GetCount()) return NULL;
		return &(m_items.Item(i));
	}

	bool            IsFirstUrl          (int i)
	{	if(i==0) return TRUE;
		if(i<0||i>=GetCount()) return FALSE;
		return (GetItem(i-1)->GetUrl()!=GetItem(i)->GetUrl());
	}

	void            SetScope            (SjTrackInfo& ti)
	{	m_scope=&ti;
		m_orgUrl=ti.m_url;
	}

	void            Add                 (const wxString& url, long field, const wxString& oldVal, const wxString& newVal);
	void            Add                 (long field, const wxString& newVal)
	{	wxASSERT(m_scope);
		Add(m_orgUrl, field, m_scope->GetValue(field), newVal);
		m_scope->SetValue(field, newVal);
	}
	void            Add                 (long field, long newVal)
	{	Add(field, wxString::Format(wxT("%i"), (int)newVal));
	}
	void            Delete              (int i);

	bool            LetConfirm          (bool& askWriteId3, bool& askDelEmptyDir, bool& onlyUrlsModified);

private:
	SjArrayModifyItem
	m_items;
	SjSLHash        m_hash;
	SjTrackInfo*    m_scope;
	wxString        m_orgUrl;
};


/*******************************************************************************
 *  The edit dialog
 ******************************************************************************/


class SjTagEditorPlugin;


class SjTagEditorDlg : public SjDialog
{
public:
	// Constructor and destructor.
	// You should derive from SjTagEditorDlg() and overwrite GetUrls().
	// "what" is: 0=get current selection, 1=get next or -1=get previous.
	// Use Init__() just after construction.
	SjTagEditorDlg      (wxWindow* parent, bool multiEdit);
	virtual bool    GetUrls             (wxArrayString&, int what) = 0;
	bool            Init__              ();
	virtual         ~SjTagEditorDlg     ();

	// this function is called if a url is renamed (old+new were set)
	// or if the url information are changed (new is empty).
	// you should reload whatever needed when this function is called...
	virtual void    OnUrlChanged        (const wxString& oldUrl, const wxString& newUrl) = 0;

private:
	// Data in edit
	bool            m_dataMultiEdit;
	wxArrayString   m_dataUrls;
	SjTrackInfo     m_dataStat;
	wxString        m_dataTitle;

	// Transferring disk -> data
	bool            Dsk2Data_CopyAllData(int what, bool logErrors=FALSE);
	void            Dsk2Data_CheckUnique(wxString& str1, const wxString& str2, long flag, long& uniqueState);
	void            Dsk2Data_CheckUnique(long& val1, long val2, long flag, long& uniqueState);

	// Transferring data -> dialog
	void            Data2Dlg_CopyAllData();
	void            Data2Dlg_CopyInfo   (bool allUrls);
	void            Data2Dlg_CopySingleData (int idEdit, const wxString&);
	void            Data2Dlg_CopySingleData (int idEdit, long v) { Data2Dlg_CopySingleData(idEdit, v>0? wxString::Format(wxT("%lu"), v) : wxString()); }

	// Transferring dialog -> data
	void            Dlg2Data_CopyAll    (SjTagEditorPlugin*, bool& reloadData, bool& canceled);
	bool            Dlg2Data_PrepareModify
	();
	void            Dlg2Data_ModifyTrackInfo
	(SjTrackInfo&, int index, SjModifyInfo&);
	long            Dlg2Data_GetString  (int id, long fieldFlag, wxString& oldAndRetValue);
	long            Dlg2Data_GetLong    (int id, long fieldFlag, long& oldAndRetValue);
	wxString        Dlg2Data_GetValue   (int id);
	bool            Dlg2Data_IsChecked  (int id);

	// Transferring data -> disk
	bool            Data2Dsk_Write      (const wxString& orgUrl, SjTrackInfo&, bool& updateAlbums);

	// Creating the dialog
	wxTextCtrl*     CreateTextCtrl      (wxWindow* parent, wxSizer*, int id, const wxSize&, int borderTop, bool multiLine = FALSE);
	void            CreateCheckBox      (wxWindow* parent, wxSizer*, int id, long align = wxALIGN_CENTER_VERTICAL);
	void            CreateComboboxItems (int id, long what);
	wxString        CreateTitle         (const wxString& title, int prependSpaces = FALSE);
	wxPanel*        CreateTitlePage     (wxWindow* parent);
	wxPanel*        CreateInfoPage      (wxWindow* parent);

	// Dialog vars
	wxNotebook*     m_dlgNotebook;
	int             m_dlgLineHeight;
	bool            m_dlgInputFromUser;
	wxChoice*       m_dlgRatingChoice;

	// Auto-complete
	int             m_autoCompleteTrackLen,
	                m_autoCompleteLeadArtistLen,
	                m_autoCompleteOrgArtistLen,
	                m_autoCompleteComposerLen,
	                m_autoCompleteAlbumLen,
	                m_autoCompleteGenreLen,
	                m_autoCompleteGroupLen,
	                m_autoCompleteCommentLen;
	void            DoAutoComplete      (int id, long what, int& oldTypedLen);


	// misc
	void            WaitForWriteAccess__(const wxString& url);
	bool            RenameFile__        (const wxString& file1, const wxString& file2);
	void            RenameDone__        (const wxString& oldUrl, const wxString& newUrl);
	SjSLHash        m_possiblyEmptyDirUrls;

	// Handle events
	void            OnButtonBarMenu     (wxCommandEvent&);
	void            OnWriteId3Tags      (wxCommandEvent&);
	void            OnPlugin            (wxCommandEvent&);
	void            OnShowMoreInfo      (wxCommandEvent&) { Data2Dlg_CopyInfo(TRUE); }
	void            OnDataInput         (wxCommandEvent&);
	void            OnTogglePage        (wxCommandEvent&);
	void            OnCancel            (wxCommandEvent&);
	void            OnOK                (wxCommandEvent&);
	void            OnPrevOrNext        (wxCommandEvent&);
	void            OnClose             (wxCloseEvent&) { wxCommandEvent fwd; OnCancel(fwd); }
	DECLARE_EVENT_TABLE ()

	friend class    SjTagEditorModule;
};


/*******************************************************************************
 *  The edit module
 ******************************************************************************/


class SjTagEditorModule : public SjCommonModule
{
public:
	SjTagEditorModule   (SjInterfaceBase*);

	void            OpenTagEditor       (SjTagEditorDlg*);
	void            CloseTagEditor      ();
	bool            IsTagEditorOpened   () const { return (m_dlg!=NULL); }

	bool            GetWriteId3Tags     () const { return m_writeId3Tags; }
	void            SetWriteId3Tags     (bool);

	bool            GetDelEmptyDir      () const { return m_delEmptyDir; }
	void            SetDelEmptyDir      (bool);

protected:
	bool            FirstLoad           ();
	void            LastUnload          ();
	void            ReceiveMsg          (int msg);
	void            GetLittleOptions    (SjArrayLittleOption& lo);

private:
	SjTagEditorDlg* m_dlg;
	long            m_selPage;
	bool            m_writeId3Tags;
	bool            m_delEmptyDir;
};


extern SjTagEditorModule* g_tagEditorModule;


#endif // __SJ_TAGEDITOR_H__
