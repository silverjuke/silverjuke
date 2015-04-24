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
 * File:    advsearch.h
 * Authors: Björn Petersen
 * Purpose: Searching albums and tracks
 *
 ******************************************************************************/


#ifndef __SJ_ADVSEARCH_H__
#define __SJ_ADVSEARCH_H__


/*******************************************************************************
 *  SjRuleControls
 ******************************************************************************/


class SjAdvSearchDialog;


class SjRuleControls
{
public:
	// The rule pointer given to the constructor is only valid as long as the constructor
	// terminates.  Up to 10 IDs may be used for the rule controls
	SjRuleControls      (const SjRule&, SjAdvSearchDialog*, wxFlexGridSizer*, long ruleId, bool setFocus, long focusRepostValue);

	// Children should be removed only when DestroyChildren() is explicit called to
	// avoid double deletion of the controls eg. on dialog exit.
	// The destructor (if any) must NOT destroy the child windows!
	void            DestroyChildren     ();
	static void     DestroyChild        (wxSizer* parentSizer, wxWindow** childToDestroy);
	static void     DestroySizer        (wxSizer* parentSizer, wxSizer** sizerToDestroy);

	// The rule ID is the index by default; however this may change eg.
	// when removing controls
	long            m_ruleId;

	// Get the rule; the returned pointer is owned by the caller.
	SjRule          GetRuleInEdit       () const { return m_rule; }

	// events
	void            OnFieldChanged      ();
	void            OnOpChanged         ();
	void            OnTextChanged       (int index, WXTYPE eventType); // index is 0 or 1
	void            OnChoiceChanged     (int index); // index is 0 or 1

private:
	// current rule data
	SjRule          m_rule;
	int             m_inValueChange;

	// field input type that defines all controls and their meanings.
	// if the input type changes, we reload all rules.
	long            m_inputType;
	long            GetRuleInputType    ();

	// the field choice control
	wxChoice*       m_fieldChoice;
	void            InitFieldChoice     ();
	void            InitFieldChoice     (SjField);

	// the operation choice control
	wxChoice*       m_opChoice;
	void            InitOpChoice        ();
	void            InitOpChoice        (const wxString& name, SjFieldOp op);

	// the value controls
	// either m_textCtrl OR m_comboBox is used
	wxTextCtrl*     m_valueTextCtrl[2];
	wxComboBox*     m_valueComboBox[2];
	wxButton*       m_valueButton[2];
	wxTextCtrl*     CreateValueTextCtrl (long baseId, const wxString& value);
	wxChoice*       CreateValueChoice   (long baseId, wxString& value);
	wxComboBox*     CreateValueComboBox (long baseId, const wxString& value);

	// additional choice controls:
	//  for limits  :   choice[0] = unit,           choice[1] = select bys
	//  for ratings :   choice[0] = value1,         choice[1] = value2 (on range select)
	//  for others  :   choice[0] = optional unit,  choice[1] = unused
	wxChoice*       m_choice[2];
	void            InitUnitChoice      ();

	// misc.
	void            UpdateFieldTooltips ();
	wxColour        m_normalColour;
	wxStaticText*   m_staticText[2];
	wxSizer*        m_sizer1;
	wxButton*       m_addRemoveButton[2];
	SjAdvSearchDialog*
	m_dialog;
	wxFlexGridSizer*
	m_ruleSizer;
};



WX_DECLARE_OBJARRAY(SjRuleControls, SjArrayRuleControls);



/*******************************************************************************
 * SjAdvSearchDialog
 ******************************************************************************/


class SjAdvSearchDialog : public SjDialog
{
public:
	                SjAdvSearchDialog   (long preselectId);
	void            ReloadRule          (long ruleId, long focusRepostValue); // should be called from a rule to enforce recreation of its controls
	void            RuleChanged         (); // should be called if any field (eg. in the rule) is modifued
	void            UpdateIncludeExclude(const SjAdvSearch& s, long changedRule);
	SjAdvSearch     GetSearchInEdit     () const;
	bool            SaveSearchInEditIfNeeded(bool askUser=TRUE);

private:
	// controls
	wxSizer*        m_mainSizer;
	wxListCtrl*     m_searchListCtrl;
	wxStaticBox*    m_staticBox;
	wxChoice*       m_selectScopeChoice;
	wxChoice*       m_selectOpChoice;
	wxFlexGridSizer* m_ruleSizer;
	SjArrayRuleControls m_ruleControls;
	wxButton*       m_menuButton;
	wxButton*       m_startSearchButton;
	wxButton*       m_endSearchButton;

	long            m_ruleTempId,
	                m_ruleTempFocusRepostValue;

	// handling controls
	wxArrayLong     GetSelectionFromSearchListCtrl(long* retSingleSelIndex=NULL); // returns and array of selected search IDs
	void            SetSelectionToSearchListCtrl(long advSearchId);
	void            UpdateSearchList    (const SjAdvSearch& recentSearch);
	void            UpdateSearchListColours(bool init);
	void            UpdateSearchTitle   (const SjAdvSearch& recentSearch);
	void            UpdateSearchButtons ();
	void            UpdateSearchControls(const SjAdvSearch&, long focusIndex=-1, long focusRepostValue=-1);
	void            ShowContextMenu     (wxWindow*, const wxPoint&);
	long            GetRuleIndex        (long ruleId);
	bool            IsSearchInEditEqualToRecentSearch();
	bool            IsSearchInEditEqualToSearchInMain();

	// events
	void            OnSearchDoubleClick (wxListEvent&) { wxCommandEvent fwd; OnSearch(fwd); }
	void            OnSearchContextMenu (wxListEvent&) { ShowContextMenu(this, ScreenToClient(::wxGetMousePosition())); }
	void            OnSearchSelChange   (wxListEvent&);
	void            OnSearchSelChangeDo (wxCommandEvent&);
	void            OnSearchInMainChanged(wxCommandEvent&);
	void            OnSearchEndEditLabel(wxListEvent&);
	void            OnSearchKeyDown     (wxListEvent&);
	void            OnUpdateSearchList  (wxCommandEvent&);

	void            OnMenuButton        (wxCommandEvent&) { ShowContextMenu(m_menuButton, wxPoint(0, 0)); }

	void            OnNewOrSaveAsSearch (wxCommandEvent&);
	void            OnSaveSearch        (wxCommandEvent&) { SaveSearchInEditIfNeeded(FALSE/*don't ask user*/); }
	void            OnRenameSearch      (wxCommandEvent&);
	void            OnRevertToSavedSearch(wxCommandEvent&);
	void            OnDeleteSearch      (wxCommandEvent&);

	void            OnSelectChoice      (wxCommandEvent&) { RuleChanged(); }
	void            OnSelectOpChoice    (wxCommandEvent&) { RuleChanged(); }

	void            OnRuleFieldChanged  (wxCommandEvent&);
	void            OnRuleOpChanged     (wxCommandEvent&);
	void            OnRuleText0Changed  (wxCommandEvent&);
	void            OnRuleText1Changed  (wxCommandEvent&);
	void            OnRuleChoice0Changed(wxCommandEvent&);
	void            OnRuleChoice1Changed(wxCommandEvent&);
	void            OnRemoveRuleRequest (wxCommandEvent&);
	void            OnRemoveRuleDo      (wxCommandEvent&);
	void            OnReloadRuleDo      (wxCommandEvent&);
	void            OnAddRule           (wxCommandEvent&);
	void            OnOpenPlaylistDo    (wxCommandEvent&);
	void            OnSize              (wxSizeEvent&);
	void            OnSearch            (wxCommandEvent&);
	bool            OnSearchCheckObviousErrors (const SjAdvSearch& advSearch, wxString& warnings);
	void            OnSearchShowErrors  (const SjAdvSearch& advSearch, wxString& warnings);
	void            OnEndSearch         (wxCommandEvent&);
	void            OnPlay              (wxCommandEvent&);
	void            OnCloseByButton     (wxCommandEvent&) { OnClose(); }
	void            OnCloseByTitlebar   (wxCloseEvent&) { OnClose(); }
	void            OnClose             ();
	DECLARE_EVENT_TABLE ()
};


/*******************************************************************************
 * SjSearchHistory
 ******************************************************************************/


class SjSearchHistory
{
public:
	SjSearchHistory     (const wxString& simpleSearchWords, long advSearchId, const wxString& advSearchRules);
	SjSearchHistory     (const SjSearch&);

	// for a history item, that is no longer valid, an empty title or
	// any empty search is returned. this may happen when deleting adv. searches
	// which are referenced by the history
	wxString        GetSimpleSearchWords() const { return m_simpleSearchWords; }
	long            GetAdvSearchId      () const { return m_advSearchId; }
	wxString        GetAdvSearchRules   () const { return m_advSearchRules; }
	wxString        GetTitle            () const;
	SjSearch        GetSearch           (bool getRules=TRUE) const;
	bool            operator ==         (const SjSearchHistory& o) const { return  IsEqualTo(o); }
	bool            operator !=         (const SjSearchHistory& o) const { return !IsEqualTo(o); }

private:
	wxString        m_simpleSearchWords;
	long            m_advSearchId;
	wxString        m_advSearchRules;
	bool            IsEqualTo           (const SjSearchHistory& o) const { return (m_advSearchId==o.m_advSearchId && m_advSearchRules==o.m_advSearchRules && m_simpleSearchWords==o.m_simpleSearchWords); }
};


WX_DECLARE_OBJARRAY(SjSearchHistory, SjArraySearchHistory);



/*******************************************************************************
 *  SjAdvSearchModule
 ******************************************************************************/



class SjAdvSearchDialog;



class SjAdvSearchModule : public SjCommonModule
{
public:
	// c'tor
	SjAdvSearchModule   (SjInterfaceBase*);
	~SjAdvSearchModule  ();

	// open/close the search dialog
	void            OpenDialog          (long preselectId=0);
	void            CloseDialog         ();
	bool            IsDialogOpen        () const { return (m_dialog!=NULL); }
	bool            IncludeExclude      (SjLLHash*, bool delPressed); // returns TRUE if tracks were deleted

	// functions mainly for the search dialog
	long            GetSearchCount      () const;
	SjAdvSearch     GetSearchById       (long id, bool getRules=TRUE) const { return GetSearch__(-1, id, wxT(""), getRules); }
	SjAdvSearch     GetSearchByIndex    (long index, bool getRules=TRUE) const { return GetSearch__(index, -1, wxT(""), getRules); }
	SjAdvSearch     GetSearchByName     (const wxString& name, bool getRules=TRUE) const { return GetSearch__(-1, -1, name, getRules); }

	SjAdvSearch     GetRecentSearch     (bool getRules=TRUE) const;
	void            SetRecentSearch     (const SjAdvSearch& s) { m_recentSearchId = s.m_id; }

	SjAdvSearch     NewSearch           (const SjAdvSearch* patternSearch=NULL);
	void            RenameSearch        (SjAdvSearch&, const wxString& newName);
	void            DeleteSearch        (const SjAdvSearch&);
	void            SaveSearch          (const SjAdvSearch&);

	// formatting a search count result
	static wxString FormatResultString  (long count, const wxString& advSearchName=wxT(""));

	// some flags
#define         SJ_SEARCHFLAG_SEARCHWHILETYPING     0x00000001L
#define         SJ_SEARCHFLAG_SIMPLEGENRELOOKUP     0x00100000L // since version 3.0, this is no longer set by default
#define         SJ_SEARCHFLAG_SEARCHSINGLEWORDS     0x00000004L
#define         SJ_SEARCHFLAG_DEFAULTS              0x000FFFFFL
	long            m_flags;

#define         SJ_SAVESEARCHES_ASK         0L
#define         SJ_SAVESEARCHES_SAVE        1L
#define         SJ_SAVESEARCHES_DONTSAVE    2L
	long            m_saveSearches;

#define         SJ_DEFAULT_HISTORY_SIZE 10L
	long            m_maxHistorySize;

	// history stuff
	long            GetHistoryCount     ();
	const SjSearchHistory*
	GetHistory          (long index);
	void            AddToHistory        (const SjSearch&);

	// handling comboboxes with the adv. searches as options
	static void     UpdateAdvSearchChoice
	(wxChoice*, long searchIdToSelect,
	 const wxString& format=wxT("%s"), const wxString& specialForId0=wxT(""));


protected:
	bool            FirstLoad           ();
	void            LastUnload          ();
	void            ReceiveMsg          (int msg);
	void            GetLittleOptions    (SjArrayLittleOption&);
	bool            DragNDrop           (SjDragNDropAction, wxWindow*, const wxPoint&, SjDataObject*);

private:
	SjAdvSearchDialog*
	m_dialog;
	SjDialogPos     m_dlgPos;

	// the most recent search ID, this is normally the ID in edit
	long            m_recentSearchId;

	// instead of saving searches directly, we use one pending object
	// -> what is this for? m_pendingSearch is only set in SjAdvSearchModule::SaveSearch()
	//    there seems to be no significant speed improvements, but there may be problems as
	//    the GetSearch__() may be out of sync with the database if the database is modified eg. by a script ...
	// -> for 2.52beta11, we have undefined SJ_USE_PENDING_MUSIC_SEL,
	//    if there are no problems, we can remove this completely somewhen later
#define SJ_USE_PENDING_MUSIC_SEL 0
#if SJ_USE_PENDING_MUSIC_SEL
	SjAdvSearch m_pendingSearch;
	bool        m_pendingNeedsSave;
public:
	void        Flush               ();
private:
#endif

	// misc.
	wxString        CheckName           (long skipId, const wxString&) const;
	SjAdvSearch     GetSearch__         (long index, long id, const wxString& name, bool getRules=TRUE) const;
	long            GetSearchIndex      (const SjAdvSearch& search) const;

	void            AddPredefinedSearches();

	SjArraySearchHistory
	m_history; // the newest items are found at the end
	bool            m_needsHistorySave,
	                m_historyLoaded;
	void            LoadHistory         ();
	void            CheckHistory        (); // called if a search is deleted
	void            SaveHistory         ();
};


extern SjAdvSearchModule* g_advSearchModule;


#endif // __SJ_ADVSEARCH_H__
