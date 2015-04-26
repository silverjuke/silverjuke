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
 * File:    password_dlg.h
 * Authors: Björn Petersen
 * Purpose: The password-entry dialog for the kiosk mode
 *
 ******************************************************************************/


#ifndef __SJ_PASSWORD_DLG_H__
#define __SJ_PASSWORD_DLG_H__


#include "../kiosk/virtkeybd.h"


#define SJ_PASSWORDDLG_AUTOCLOSE            0x01
#define SJ_PASSWORDDLG_ASK_FOR_PASSWORD     0x02
#define SJ_PASSWORDDLG_ASK_FOR_ACTION       0x04
#define SJ_PASSWORDDLG_ACCEPT_ANY_PASSWORD  0x10


class SjPasswordDlg : public SjDialog
{
public:
	                 SjPasswordDlg          (wxWindow* parent,
	                                         long flags /*SJ_PASSWORDDLG_*/,
	                                         const wxString& title,
	                                         const wxString& passwordHint,
	                                         const wxString& correctPassword1,
	                                         const wxString& correctPassword2=wxT(""));

	bool             IsOkClicked            () const { return m_okClicked; }
	int              IsPasswordOk           () const { return m_passwordOk; }

	SjShutdownEtc    GetExitAction          () const;
	wxString         GetEnteredPassword     () const { if(m_passwordTextCtrl) {return m_passwordTextCtrl->GetValue();} return wxT(""); }

	static void      InitExitActionChoice   (wxChoice*);
	static wxChoice* CreateExitActionChoice (wxWindow* parent, SjShutdownEtc sel, bool addAsk);

private:
	wxString             m_correctPassword1, m_correctPassword2;
	bool                 m_okClicked;
	int                  m_passwordOk; // 0=none, 1=password1, 2=password2
	wxChoice*            m_passwordAction;
	SjVirtKeybdTextCtrl* m_passwordTextCtrl;
	SjTimeoutWatcher     m_timeoutWatcher;
	wxTimer              m_closeTimer;
	long                 m_flags;
	void                 OnPasswordInput        (wxCommandEvent&) { m_timeoutWatcher.Reset(); }
	void                 OnCloseByTimer         (wxTimerEvent&);
	bool                 TransferDataFromWindow ();
	DECLARE_EVENT_TABLE ()
};


#endif // __SJ_PASSWORD_DLG_H__
