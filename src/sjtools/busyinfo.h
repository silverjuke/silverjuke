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
 * File:    busyinfo.h
 * Authors: Björn Petersen
 * Purpose: Our busy information and progress dialog
 *
 ******************************************************************************/


#ifndef __SJ_BUSYINFO_H__
#define __SJ_BUSYINFO_H__


class SjMainFrame;


class SjBusyInfo : public SjDialog
{
public:
	                SjBusyInfo          (wxWindow *parent, const wxString& title=wxT(""), bool canCancel=TRUE, const wxString& questionOnCancel=wxT(""));
	                ~SjBusyInfo         ();

	static bool     Set                 (const wxString& object=wxT(""), bool forceUpdateForLongOp = FALSE);
	static void     SetMainFrame        (SjMainFrame*);

	static void     Yield               () {
		wxASSERT(s_inYield==0);
		wxASSERT(wxThread::IsMain());
		if(s_inYield==0) {s_inYield++; wxYield(); s_inYield--;}
	}
	static bool     InYield             () { return (s_dlgOpen || (s_inYield!=0)); }

private:
	static SjBusyInfo* s_busyInfo;
	static long     s_busyInfoUsage;
	static SjMainFrame* s_mainFrame;
	static long     s_inYield;
	static bool     s_dlgOpen;

	bool            m_canCancel;
	wxString        m_questionOnCancel;
	bool            m_cancelPressed;
	unsigned long   m_nextUpdateMs;
	unsigned long   m_startTimeMs;
	unsigned long   m_elapsedSeconds;
	unsigned long   m_objectCount;

	wxStaticText*   m_objectNameWindow;
	wxStaticText*   m_objectCountWindow;
	wxStaticText*   m_elapsedTimeWindow;

	bool            Set__               (const wxString& object, bool forceUpdateForLongOp = FALSE);
	void            AskToClose          ();
	void            OnCloseWindow       (wxCloseEvent& event);
	void            OnCancel            (wxCommandEvent& event);
	DECLARE_EVENT_TABLE ();
};


#endif // __SJ_BUSYINFO_H__
