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
 * File:    displaysj.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke main frame
 *
 ******************************************************************************/



#ifndef __SJ_DISPLAYSJ_H__
#define __SJ_DISPLAYSJ_H__



#include <sjmodules/tageditor/tageditor.h>



class SjDisplay
{
public:
	                SjDisplay           ();

	#define         SJ_DISPLAY_SCROLL_LOCK_MS          20000 /*20 seconds*/
	#define         SJ_DISPLAY_SELECTION_LOCK_MS       20000 /*20 seconds*/
	#define         SJ_DISPLAY_ENQUEUE_LOCK_MS          4000 /* 4 seconds*/

	long            m_scrollPos;
	long            m_firstLineQueuePos;

	bool            m_tagEditorJustOpened;

	wxString        m_msg1;
	wxString        m_msg2;
	unsigned long   m_msgCancelAtTimestamp;

	wxString        m_backupedCurrTitle;
	long            m_backupedCurrLine;
	bool            m_backupedCurrBold;

	SjLLHash        m_selectedIds;
	long            m_selectedAnchorId;
	unsigned long   m_selectedIdsTimestamp;
	unsigned long   m_selectedIdsLockMs;
	long            m_selectionDoneOnMouseDown;
	void            ClearDisplaySelection();
};



class SjDisplayEditDlg : public SjTagEditorDlg
{
public:
	                SjDisplayEditDlg    ();
	bool            GetUrls             (wxArrayString&, int what);
	void            OnUrlChanged        (const wxString& oldUrl, const wxString& newUrl);

private:
	wxArrayString   m_urls;
	long            m_id; // for single edit only
	void            MarkId              (long changeScrollPosDir);
};



#endif // __SJ_DISPLAYSJ_H__
