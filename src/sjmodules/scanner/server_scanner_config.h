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
 * File:    server_scanner_config.h
 * Authors: Björn Petersen
 * Purpose: The "server scanner" module
 *
 ******************************************************************************/


#ifndef __SJ_SERVER_SCANNER_CONFIG_H__
#define __SJ_SERVER_SCANNER_CONFIG_H__


class SjServerScannerConfigDialog : public SjDialog
{
public:
	SjServerScannerConfigDialog (wxWindow* parent, const SjServerScannerSource&);

	// function returns TRUE is sth. was changed, otherwise FALSE is returned
	bool GetChanges(SjServerScannerSource&);

private:
	bool            m_isNew;

	wxCheckBox*     m_enabledCheckBox;
	wxTextCtrl*     m_serverNameTextCtrl;
	wxChoice*       m_serverTypeChoice;
	wxTextCtrl*     m_loginNameTextCtrl;
	wxTextCtrl*     m_loginPasswordTextCtrl;
	wxCheckBox*     m_doUpdateCheckBox;

	void            EnableDisable       ();

	void            OnEnableCheck       (wxCommandEvent&) { EnableDisable(); }
	void            OnHelp              (wxCommandEvent&);
	DECLARE_EVENT_TABLE ()
};


#endif // __SJ_SERVER_SCANNER_CONFIG_H__

