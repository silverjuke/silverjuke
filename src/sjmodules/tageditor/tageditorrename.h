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
 * File:    tageditorrename.h
 * Authors: Björn Petersen
 * Purpose: Tag editor rename plugin
 *
 ******************************************************************************/


#ifndef __SJ_TAGEDITOR_RENAME_H__
#define __SJ_TAGEDITOR_RENAME_H__


class SjTrackInfoReplacer : public SjPlaceholdReplacer
{
public:
	                SjTrackInfoReplacer (SjTrackInfo* ti=NULL);
	void            ReplacePath         (wxString& text, SjTrackInfo* ti=NULL);

protected:
	virtual bool    GetReplacement      (const wxString& placeholder, long flags, wxString& replacement);

private:
	SjTrackInfo*    m_ti;
	wxRegEx         m_slashReplacer;
};



class SjRenamePlugin : public SjTagEditorPlugin
{
public:
	                SjRenamePlugin      (wxWindow* parent, SjTrackInfo* exampleTrackInfo);
	bool            PrepareModify       ();
	void            ModifyTrackInfo     (SjTrackInfo&, int index, SjModifyInfo&);

private:
	// configuration
	wxArrayString   m_pattern;
	SjTrackInfoReplacer m_tiReplacer;

	// controls
	SjInsertButton* m_insertButton;
	wxStaticText*   m_exampleCtrl;
	void            UpdateExample       ();

	SjHistoryComboBox* m_patternCtrl;
	void            OnInsert            (wxCommandEvent& e) { m_insertButton->OnOption(e, m_patternCtrl); UpdateExample(); }
	void            OnPatternChange     (wxCommandEvent& e) { UpdateExample(); }
	DECLARE_EVENT_TABLE ()
};



#endif // __SJ_TAGEDITOR_REPLACER_H__
