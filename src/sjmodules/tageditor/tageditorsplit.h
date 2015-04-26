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
 * File:    tageditorsplit.h
 * Authors: Björn Petersen
 * Purpose: Tag editor split plugin
 *
 ******************************************************************************/


#ifndef __SJ_TAGEDITOR_SPLIT_H__
#define __SJ_TAGEDITOR_SPLIT_H__


class SjSplitPlugin : public SjTagEditorPlugin
{
public:
	                SjSplitPlugin       (wxWindow* parent, SjTrackInfo* exampleTrackInfo);
	bool            PrepareModify       ();
	void            ModifyTrackInfo     (SjTrackInfo&, int index, SjModifyInfo&);

private:
	// configuration
	long            m_splitIn;
	wxArrayString   m_pattern;
	SjTrackInfoMatcher m_matcher;

	// controls
	SjTrackInfoFieldChoice* m_splitInChoice;
	SjInsertButton* m_insertButton;
	wxStaticText*   m_exampleCtrl;
	void            UpdateExample       ();

	SjHistoryComboBox* m_patternCtrl;
	void            OnInsert            (wxCommandEvent& e) { m_insertButton->OnOption(e, m_patternCtrl); UpdateExample(); }
	void            OnFieldToSplitChange(wxCommandEvent& e) { UpdateExample(); }
	void            OnPatternChange     (wxCommandEvent& e) { UpdateExample(); }
	DECLARE_EVENT_TABLE ()
};


#endif // __SJ_TAGEDITOR_SPLIT_H__
