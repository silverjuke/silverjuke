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
 * File:    mainapp.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke main application class
 *
 ******************************************************************************/


#ifndef __SJ_MAINAPP_H__
#define __SJ_MAINAPP_H__


enum SjShutdownEtc
{
    SJ_SHUTDOWN_STOP_PLAYBACK       = 0,
    SJ_SHUTDOWN_CLEAR_PLAYLIST      = 10,
    SJ_SHUTDOWN_EXIT_KIOSK_MODE     = 20,
    SJ_SHUTDOWN_EXIT_SILVERJUKE     = 30,
    SJ_SHUTDOWN_POWEROFF_COMPUTER   = 40,
    SJ_SHUTDOWN_REBOOT_COMPUTER     = 50,
    SJ_SHUTDOWN_USER_DEF_SCRIPT     = 60,
    SJ_SHUTDOWN_USER_DEF_SCRIPT_LAST= 159, // must all be less than 255, also note, the values are saved to disk!
    SJ_SHUTDOWN_ASK_FOR_ACTION      = 1000,
};


class SjMainApp : public wxApp
{
public:
	bool            OnInit              ();
	int             OnExit              ();
	void            OnFatalException    ();
	static void     FatalError          ();
	void            SetIsInShutdown     ();
	static bool     IsInShutdown        () { return s_isInShutdown; }
	static void     DoShutdownEtc       (SjShutdownEtc, long restoreOrgVol=-1);
	static wxCmdLineParser* s_cmdLine;

private:
	static bool     s_isInShutdown;
	void            OnActivate          (wxActivateEvent&);
	void            OnQueryEndSession   (wxCloseEvent&);
	void            OnEndSession        (wxCloseEvent&);
	DECLARE_EVENT_TABLE ();
};

DECLARE_APP(SjMainApp);


#endif /* __SJ_MAINAPP_H__ */
