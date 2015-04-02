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
 * File:    kiosk.h
 * Authors: Björn Petersen
 * Purpose: Kiosk mode handling
 *
 ******************************************************************************/



#ifndef __SJ_KIOSK_H__
#define __SJ_KIOSK_H__



#include "numpad.h"
#include "creditbase.h"



#define         SJ_KIOSKF_EXIT_KEY              0x00000001L
#define         SJ_KIOSKF_DISABLE_AT            0x00000002L // disable ALT-TAB
#define         SJ_KIOSKF_MAX_TRACKS_IN_QUEUE   0x00000008L
#define         SJ_KIOSKF_EXIT_CORNER           0x00010000L
#define         SJ_KIOSKF_USE_PASSWORD          0x00020000L
#define         SJ_KIOSKF_LIMIT_TO_ADV_SEARCH   0x00040000L
#define         SJ_KIOSKF_SHUFFLE               0x00080000L
#define         SJ_KIOSKF_SWITCHRES_OLD         0x00100000L
#define         SJ_KIOSKF_DISABLE_SHUTDOWN      0x00200000L
#define         SJ_KIOSKF_SWITCHRES_0           0x00400000L
#define         SJ_KIOSKF_SWITCHRES_1           0x00800000L
#define         SJ_KIOSKF_NO_DBL_TRACKS         0x01000000L
#define         SJ_KIOSKF_DISABLE_SCRSAVER      0x02000000L
#define         SJ_KIOSKF_DISABLE_POWERMAN      0x04000000L
#define         SJ_KIOSKF_DISABLE_CAD           0x08000004L // disable CTRL-ALT-DEL, in Silverjuke<3.00, this was the flag 0x00000004L; changed to force a re-enabling for the driver installation
#define         SJ_KIOSKF_DEFAULT               0x0001FFF1L
#define         SJ_KIOSKF_RESETABLE         (SJ_KIOSKF_NO_DBL_TRACKS|SJ_KIOSKF_SHUFFLE|SJ_KIOSKF_MAX_TRACKS_IN_QUEUE|SJ_KIOSKF_LIMIT_TO_ADV_SEARCH)



class SjKioskModule : public SjCommonModule
{
public:

	// Common
	////////////////////////////////////////////////////////////////////////////////

public:
	SjKioskModule       (SjInterfaceBase*);
	void            LastUnload          ();

	SjEmbedTo       EmbedTo             () { return SJ_EMBED_TO_MAIN; }
	wxWindow*       GetConfigPage       (wxWindow* parent, int selectedPage);
	void            GetConfigButtons    (wxString& okText, wxString& cancelText, bool& okBold);
	void            DoneConfigPage      (wxWindow* configPage, int action);
	void            GetLittleOptions    (SjArrayLittleOption& lo);

	SjHomepageId    GetHelpTopic        ();

	void            StartRequest        (bool forceNoQuerySettings=false);
	bool            ExitRequest         (long flag, const wxString* password=NULL, bool forceSimpleExit=false);
	void            DoExit              (bool restoreWindow);
	void            ToggleRequest       (long flag) { if(KioskStarted()) {ExitRequest(flag);} else {StartRequest();} }
	bool            KioskStarted        () const { return m_isStarted; }

	bool            IsPasswordDlgOpened () const { return m_passwordDlgOpened; }
	bool            IsPasswordInUse     ();
	bool            IsShutdownDisabled  () const { return (m_configKioskf&SJ_KIOSKF_DISABLE_SHUTDOWN)!=0; }

	long            GetLimitToId        () const { return (m_configKioskf&SJ_KIOSKF_LIMIT_TO_ADV_SEARCH)? m_configLimitToAdvSearch : 0; }

	SjUptimeWatcher m_uptime;

	SjNumpadInput   m_numpadInput;


	// CanEnqueue() should be called before enqueing titles.
	// It returns TRUE if adding the number of given titles is okay; in this case you
	// should really add the titles to the queue as the credit counter is decreased.
	// If FALSE is returned, adding is not possible at the moment, an error is already
	// printed by CanEnqueue() eg. using g_mainFrame->SetDisplayMsg()
	bool            CanEnqueue          (const wxArrayString& requestedUrls, bool urlsVerified);
	SjCreditBase    m_creditBase;
	void            UpdateCreditSpinCtrl();

protected:
	void            ReceiveMsg          (int msg);

private:
	// configuration
	long            m_configKioskf;
	long            m_configOp;
	wxString        m_configUserPassword;
	wxString        m_configMaintenancePassword;
	SjShutdownEtc   m_configDefExitAction;

#define         SJ_DEF_MAX_TRACKS_IN_QUEUE  16
	long            m_configMaxTracksInQueue;

	long            m_configLimitToAdvSearch;
	long            m_configMonitor[2];
	wxString        m_configDispRes[2];

	void            LoadConfig          ();
	void            SaveConfig          ();
	bool            m_configLoaded;

	// running kiosk mode
	void            DoStart             ();
	bool            m_isStarted;
	bool            m_passwordDlgOpened;

	// backuped settings that are restored if the kiosk mode exits
	long            m_backupOp;
	bool            m_backupFullScreen;
	bool            m_backupAlwaysOnTop;
	bool            m_backupVideoModeValid[2];
	bool            m_backupResetAccel;
	wxVideoMode     m_backupVideoMode[2];
	wxString        m_backupLayout;
	wxRect          m_backupMainFrameRect;
	wxArrayPtrVoid  m_blackFrames;

	// CanDisableCtrlAltDel() checks if there are any additional conditions to disable
	// CTRL+ALT+DEL ...
	bool            CanDisableCtrlAltDel    (wxWindow* parent);

	// show the kiosk real exclusive - this should be implemented by the OS
	void            SetExclusive        (bool);
	static void     ClipMouse           (const wxRect*);

	friend class    SjKioskConfigPage;
	friend class    SjLittleMaintenancePw;
	friend class    SjCreditBase;
	friend class    SjPlaybackSettingsConfigPage;
};



extern SjKioskModule* g_kioskModule;



#endif // __SJ_KIOSK_H__
