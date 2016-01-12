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
 * File:    accelsj.h
 * Authors: Björn Petersen
 * Purpose: Accelerator handling
 *
 ******************************************************************************/


#ifndef __SJ_ACCEL_H__
#define __SJ_ACCEL_H__


/*******************************************************************************
 * SjAccelCmd
 ******************************************************************************/


// if the wxACCEL_SYSTEMWIDE is set for a shourtcut, the shortcut should be
// available system-wide. may be used in combination with the other
// wxACCEL_* flags as wxACCEL_ALT etc. from wxWindows.
#define wxACCEL_SYSTEMWIDE 0x0100


class SjAccelCmd
{
public:
	// no constructor, call Init()
	wxString        m_name;
	int             m_id;

	// command flages
	#define         SJA_MAIN                    0x0001
	#define         SJA_SETTINGS                0x0002
	#define         SJA_ART                     0x0008
	#define         SJA_EDIT                    0x0020
	#define         SJA_KIOSKSETTINGS           0x0040
	#define         SJA_VIS                     0x0080
	#define         SJA_ADVSEARCH               0x0100
	#define         SJA_PLAYBACKSETTINGS        0x0200
	#define         SJA_MAINNUMPAD              0x0400
	#define         SJA_MASK                    0xFFFF
	#define         SJA_ADDNOMODIFIERKEYS   0x00010000  // even add simple keys as ENTER or SPACE when calling GetAccelTable() - this is only useful if there are not other (child-)controls taking keys in the window
	#define         SJA_NOACCELTABLE        0x00020000  // may be used with KeyEvent2*() to return keys that are normally handled by the accelerator table
	#define         SJA_UNEDITABLE          0x00080000
	long            m_flags;

	// is this command just a link to another command?
	int             m_linkId;

	// the shortcuts for this command, up to SJ_ACCEL_MAX_KEYS
	// shortcuts may be defined per command (should be enough for
	// all purposes)
	// we remember three keyboard layouts:
	//   -  the original keys in m_orgKey[] as defined by me
	//   -  keys as definded by the user in m_userKey[], default to m_orgKey[]
	//   -  finally some temp. keys that are used only while editing
	#define         SJ_ACCEL_MAX_KEYS 8
	long            m_orgKey[SJ_ACCEL_MAX_KEYS];
	int             m_orgKeyCount;
	long            m_userKey[SJ_ACCEL_MAX_KEYS];
	int             m_userKeyCount;
	long            m_tempKey[SJ_ACCEL_MAX_KEYS];
	int             m_tempKeyCount;
	bool            AreUserAndOrgEqual  () const { return (m_userKeyCount==m_orgKeyCount && memcmp(m_userKey, m_orgKey, m_userKeyCount*sizeof(long))==0); }
};


/*******************************************************************************
 * SjAccelModule
 ******************************************************************************/


class SjAccelModule : public SjCommonModule
{
public:
	                SjAccelModule       (SjInterfaceBase*);

	wxAcceleratorTable GetAccelTable    (long flags, wxAcceleratorEntry* addEntries=NULL, int addEntriesCount=0);

	int             CmdId2CmdIndex      (int id) { return m_cmdId2CmdIndex.Lookup(id); }
	int             KeyEvent2CmdIndex   (wxKeyEvent&, long flags);
	int             KeyEvent2CmdId      (wxKeyEvent& e, long flags) { return m_cmd[KeyEvent2CmdIndex(e, flags)].m_id; }

	static wxString GetReadableShortcutByKey (long modifier, long keycode, bool useNativeSymbols=true);
	static wxString GetReadableShortcutByComprKey (long key, bool useNativeSymbols=true) { return GetReadableShortcutByKey(key>>16, key&0xFFFFL, useNativeSymbols); }

	#define         SJ_SHORTCUTS_ADDALL     0x01
	#define         SJ_SHORTCUTS_LOCAL      0x04
	#define         SJ_SHORTCUTS_SYSTEMWIDE 0x08
	wxString        GetReadableShortcutsByCmd(int cmdId, long flags)	{ SjAccelCmd* cmd=&m_cmd[CmdId2CmdIndex(cmdId)]; return GetReadableShortcutsByPtr(cmd->m_userKey, cmd->m_userKeyCount, flags); }

	bool            IsShortcutDefined   (int cmdId) { return (m_cmd[CmdId2CmdIndex(cmdId)].m_userKeyCount>0); }

	wxString        GetCmdNameByIndex   (int index, bool shortName=FALSE);
	wxString        GetCmdNameById      (int id, bool shortName=FALSE) { int index=CmdId2CmdIndex(id); if(index) { return GetCmdNameByIndex(index, shortName); } else { return wxEmptyString; } }

	// misc. settings, public read
	#define         SJ_ACCEL_CONTENT_DRAG               0x00000004L
	#define         SJ_ACCEL_ASK_ON_MULTI_ENQUEUE       0x00000008L
	#define         SJ_ACCEL_ASK_ON_CLEAR_PLAYLIST      0x00000010L
	#define         SJ_ACCEL_TOOLTIPS                   0x00000020L
	#define         SJ_ACCEL_ASK_ON_CLOSE_IF_PLAYING    0x00000040L
	#define         SJ_ACCEL_USE_DND_IMAGES             0x00000080L
	#define         SJ_ACCEL_ASK_ON_RATING_CHANGE       0x00000200L
	#define         SJ_ACCEL_USE_NUMPAD_OUT_KIOSK       0x00000400L
	#define         SJ_ACCEL_START_PLAYBACK_ON_ENQUEUE  0x00008000L
	#define         SJ_ACCEL_USE_MM_KEYBD               0x00010000L
	#define         SJ_ACCEL_WHEEL_HORZ_IN_ALBUMVIEW    0x00020000L
	#define         SJ_ACCEL_USE_NUMPAD_IN_KIOSK        0x00040000L
	#define         SJ_ACCEL_PLAY_NOW_ON_DBL_CLICK      0x00080000L
	#define         SJ_ACCEL_USE_VIEW_FONT_IN_DLG       0x00100000L
	#define         SJ_ACCEL_SAME_ZOOM_IN_ALL_VIEWS     0x00200000L
	#define         SJ_ACCEL_WHEEL_MODIFIER_AXIS_TOGGLE 0x00400000L
	#define         SJ_ACCEL_WHEEL_RMOUSE_AXIS_TOGGLE   0x00800000L
	#define         SJ_ACCEL_WHEEL_VALUE_INPUT          0x01000000L
	#define         SJ_ACCEL_DEFAULT                    0x0000FFFFL
	long            m_flags;
	long            m_selDragNDrop; // 0=off, 1=first click, 2=second click

	#define         SJ_ACCEL_MID_OFF                0L
	#define         SJ_ACCEL_MID_OPENS_TAG_EDITOR   1L
	long            m_middleMouseAction; // values of SJ_ACCEL_MID* (this cannot be an enum to avoid type conflicts)

	bool     UseNumpad           () const
	{
		long test = g_mainFrame->IsKioskStarted()? SJ_ACCEL_USE_NUMPAD_IN_KIOSK : (SJ_ACCEL_USE_NUMPAD_IN_KIOSK|SJ_ACCEL_USE_NUMPAD_OUT_KIOSK);
		return (m_flags&test)==test;
	}

	// configuration
	void            GetLittleOptions    (SjArrayLittleOption&);

	// message box, that may be switched of,
	// equal to wxMessageBox
	int             YesNo               (const wxString& message, const wxString& yesTitle, wxWindow *parent, long flagToCheck);

protected:
	bool            FirstLoad           ();
	void            LastUnload          ();

private:
	#define         SJ_ACCEL_MAX_CMDS 280
	SjAccelCmd      m_cmd[SJ_ACCEL_MAX_CMDS];
	int             m_cmdCount;
	int             m_totalUserKeyCount; // all keys in all commands

	void            OrgCmd              (const wxString& name, int id, long flags, int linkId=0);
	void            OrgKey              (long orgModifier, long orgKeycode);
	wxString        GetReadableShortcutsByPtr (const long userKey[], int userKeyCount, long flags) const;
	SjLLHash        m_cmdId2CmdIndex,
	                m_keyPresent;
	void            BuildHashes         ();

	friend class    SjMenu;
	friend class    SjAccelConfigPage;
	friend class    SjMmKeybd;

	// system-wide shortcuts -- this should be implemented in the OS-depending part
	// if "set" is TRUE, accelerators should be set from m_cmd - otherwise all
	// accelerators should be cleared.
	void            InitSystemAccel     ();
	void            UpdateSystemAccel   (bool set);
	void            ExitSystemAccel     ();

	friend class    SjLittleAccelOption;
};


extern SjAccelModule* g_accelModule;


/*******************************************************************************
 * SjMenu
 ******************************************************************************/


class SjMenu : public wxMenu
{
public:
			SjMenu          (long showShortcuts);

	void    Append          (int id, const wxString& s=wxEmptyString) {	wxMenu::Append(CreateMenuItem(id, s, wxITEM_NORMAL, NULL));	}
	void    Append          (int id, const wxString& s, wxMenu* m)    { wxMenu::Append(CreateMenuItem(id, s, wxITEM_NORMAL, m)); }
	void    AppendCheckItem (int id, const wxString& s=wxEmptyString) { wxMenu::Append(CreateMenuItem(id, s, wxITEM_CHECK, NULL)); }
	void    AppendRadioItem (int id, const wxString& s=wxEmptyString) { wxMenu::Append(CreateMenuItem(id, s, wxITEM_RADIO, NULL)); }
	void    Insert          (size_t pos, int id, const wxString& s=wxEmptyString) { wxMenu::Insert(pos, CreateMenuItem(id, s, wxITEM_NORMAL, NULL)); }
	void    Insert          (size_t pos, int id, const wxString& s, wxMenu* m)	{ wxMenu::Insert(pos, CreateMenuItem(id, s, wxITEM_NORMAL, m)); }
	void    InsertCheckItem (size_t pos, int id, const wxString& s=wxEmptyString) { wxMenu::Insert(pos, CreateMenuItem(id, s, wxITEM_CHECK, NULL)); }
	long    ShowShortcuts   (long showShortcuts) { long old = m_showShortcuts; m_showShortcuts = showShortcuts; return old; }
	long    ShowShortcuts   () const { return m_showShortcuts; }
	void    SetLabel        (int id, const wxString& newLabel);
	void    Enable          (int id, bool enable=TRUE);
	void    Check           (int id, bool check=TRUE);
	void    Clear           ();

private:
	wxMenuItem*     CreateMenuItem      (int id, const wxString& str, wxItemKind kind, wxMenu* subMenu);
	long            m_showShortcuts;
};


#endif // __SJ_ACCEL_H__

