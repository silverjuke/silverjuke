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
 * File:    tageditorreplace.h
 * Authors: Björn Petersen
 * Purpose: Tag editor replace plugin
 *
 ******************************************************************************/


#ifndef __SJ_TAGEDITOR_REPLACE_H__
#define __SJ_TAGEDITOR_REPLACE_H__


class SjReplacePlugin : public SjTagEditorPlugin
{
public:
	                SjReplacePlugin     (wxWindow* parent, SjTrackInfo* exampleTrackInfo);

	bool            PrepareModify       ();
	void            ModifyTrackInfo     (SjTrackInfo&, int index, SjModifyInfo&);
	wxString        PostModify          ();

private:
	// settings
	wxArrayString   m_searchFor;
	long            m_searchIn;
	wxArrayString   m_replaceWith;
	long            m_searchFlags;

	// modify a single field
	long            m_replacementCount;
	void            ModifyField         (SjTrackInfo& trackInfo, long field, SjModifyInfo& mod);

	// controls
	SjHistoryComboBox* m_searchForTextCtrl;
	SjTrackInfoFieldChoice* m_searchInChoice;
	SjHistoryComboBox* m_replaceWithTextCtrl;
	wxCheckBox*     m_matchWholeWordsOnly;
	wxCheckBox*     m_matchCase;
	wxCheckBox*     m_matchRegEx;
	SjStrReplacer   m_replacer;
	#define         REPL_FIELDS     (SJ_TI_TRACKNAME \
									|SJ_TI_LEADARTISTNAME \
                                    |SJ_TI_ORGARTISTNAME \
                                    |SJ_TI_COMPOSERNAME \
                                    |SJ_TI_ALBUMNAME \
                                    |SJ_TI_GENRENAME \
                                    |SJ_TI_GROUPNAME \
                                    |SJ_TI_COMMENT)
	#define         REPL_FILENAME   0x101 // 2 bits set!
};


#endif // __SJ_TAGEDITOR_REPLACE_H__
