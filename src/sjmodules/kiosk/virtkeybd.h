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
 * File:    virtkeybd.h
 * Authors: Björn Petersen
 * Purpose: The virtual keyboard
 *
 ******************************************************************************/


#ifndef __SJ_VIRTKEYBD_H__
#define __SJ_VIRTKEYBD_H__


class SjVirtKeybdTextCtrl : public wxTextCtrl
{
	// if you use SjVirtKeybdTextCtrl instead of wxTextCtrl, the user may
	// use the virtual keyboard for this text control
public:
					SjVirtKeybdTextCtrl  (wxWindow*, wxWindowID,
	                                      const wxString& value,
	                                      const wxPoint&, const wxSize&,
	                                      long style);
	                ~SjVirtKeybdTextCtrl ();
	void            SetForceForTesting   (bool forceForTesting=TRUE) { m_forceForTesting = forceForTesting; }
	void            DontCloseOn          (wxWindow* w) { m_dontCloseOn.Add(w); }
	const wxArrayPtrVoid* GetDontCloseOn () const { return &m_dontCloseOn; }

private:
	bool            m_forceForTesting;
	wxArrayPtrVoid  m_dontCloseOn;

	void            OnMyMouseDown       (wxMouseEvent&);
	void            OnMyMouseDownDo     (wxCommandEvent&);
	                DECLARE_EVENT_TABLE ();
};


class SjVirtKeybdKey
{
public:
	// define a key
	                SjVirtKeybdKey      (const wxString& keyNormal);
	void            SetKey              (const wxString& type, const wxString& key);
	void            SetRelWidth         (const wxString& s) { m_relWidth = SjTools::ParseFloat(s); }
	void            SetRelXPos          (float x) { m_relXPos = x; }

	// get key information
	wxString        GetKey              (int shifted, int alt) const;
	bool            IsSpacer            () const { return (m_keys[0][0]==wxT("spacer")); }
	bool            IsNextLine          () const { return (m_keys[0][0]==wxT("nextline")); }
	int             IsShift             () const { return (m_keys[0][0]==wxT("shift"))? 1 : 0; }
	int             IsEnter             () const { return (m_keys[0][0]==wxT("enter"))? 1 : 0; }
	int             IsEnterCont         () const { return (m_keys[0][0]==wxT("entercont"))? 1 : 0; }
	int             IsAlt               (int shifted, int alt) const;
	wxString        GetKeyTitle         (int shifted, int alt) const;
	int             GetKeyFlags         (int shifted, int alt) const { return m_keyFlags[shifted][alt]; }
	float           GetRelWidth         () const { return m_relWidth; }
	float           GetRelXPos          () const { return m_relXPos; }

	wxRect          m_rect;

private:
	#define         SJ_VK_MAX_SHIFT     2
	#define         SJ_VK_MAX_ALT       33 /* 32 keys -- 1..32 */
	wxString        m_keys[SJ_VK_MAX_SHIFT][SJ_VK_MAX_ALT];
	wxString        m_keyTitles[SJ_VK_MAX_SHIFT][SJ_VK_MAX_ALT];

	#define         SJ_VK_LOCK          0x01
	long            m_keyFlags[SJ_VK_MAX_SHIFT][SJ_VK_MAX_ALT];

	float           m_relXPos;
	float           m_relWidth;


	static wxString ParseKey            (const wxString& key, const wxString& defaultKey, wxString& retKeyTitle, long& retKeyFlags);

	wxString        TranslTitle         (int index) const;

	bool            FindKey             (int inShifted, int inAlt, int& outShifted, int& outAlt) const;
};

WX_DECLARE_OBJARRAY(SjVirtKeybdKey, SjArrayVirtKeybdKey);


class SjVirtKeybdLayout
{
public:
	                SjVirtKeybdLayout   ();
	bool            LoadLayoutFromFile  (const wxString& file, wxArrayString* allNames);
	wxString        m_name;
	wxString        m_file;
	SjArrayVirtKeybdKey m_keys;

	// the "relative width" is - more or less - the number of buttons in
	// a single line of the keyboard; however, the buttons may have
	// different widths.
	float           GetRelWidth         () const { return m_totalRelWidth; }
	float           GetAspectRatio      () const { return (m_lineCount>0)? (m_totalRelWidth/(float)m_lineCount) : 1.0; }
	void            CalcRects           (const wxRect& totalRect);
	SjVirtKeybdKey* FindKey             (int x, int y);

	// the "enter" button is little special as it may go over two rows;
	// howver, both pointers may be NULL!
	SjVirtKeybdKey* m_enterKey;
	SjVirtKeybdKey* m_enterCont;

private:
	long            m_lineCount;
	float           m_totalRelWidth;
};

WX_DECLARE_OBJARRAY(SjVirtKeybdLayout, SjArrayVirtKeybdLayout);


class SjVirtKeybdFrame;
class SjVirtKeybdModule : public SjCommonModule
{
public:
	                SjVirtKeybdModule   (SjInterfaceBase*);

	// open / close the keyboard
	void            OpenKeybd           (wxWindow* inputReceiver, bool forceForTesting=FALSE);
	void            CloseKeybd          ();
	bool            IsKeybdOpened       ();

	// select a keyboard layout
	void            GetAvailKeybdLayouts(SjArrayVirtKeybdLayout&);
	wxString        GetKeybdLayout      ();
	void            SetKeybdLayout      (const wxString& file);

	// transparency
	#define         SJ_DEF_VIRTKEYBD_TRANSP 26L  // 26 is a nice value close to black (0=fully opaque, 100=fully transparent)
	int             GetTransparency     () ;
	void            SetTransparency     (int transp);

	// misc. settings
	#define         SJ_VIRTKEYBD_BLACK                  0x00000010L
	#define         SJ_VIRTKEYBD_USE_IN_KIOSK           0x00010000L
	#define         SJ_VIRTKEYBD_USE_OUTSIDE_KIOSK      0x00020000L
	#define         SJ_VIRTKEYBD_HIDE_CURSOR            0x00040000L
	#define         SJ_VIRTKEYBD_DEFAULT                0x0000FFFFL
	long            GetKeybdFlags       () const { return m_flags; }
	void            SetFlag             (long flag, bool set);
	bool            GetForceForTesting  () const { return m_forceForTesting; };
	wxWindow*       GetInputReceiver    () const { return m_inputReceiver; }

	// the cursor
	static wxCursor GetStandardCursor   ();

protected:
	bool            FirstLoad           ();
	void            LastUnload          ();
	void            ReceiveMsg          (int msg);

private:
	// the frame window
	SjVirtKeybdFrame* GetVirtKeybdFrame   ();
	wxWindow*       m_inputReceiver; // may be NULL at any time!

	// the settings
	bool            m_forceForTesting;
	long            m_flags;
	wxString        m_layoutFile_dontUse;   // use GetKeybdLayout() instead
	int             m_transp_dontUse;       // use GetTransparency() instead
	void            SaveSettings        ();
	void            GetAvailKeybdLayoutsFromDir (SjArrayVirtKeybdLayout& list, const wxString& dir);
	void            GetAvailKeybdLayoutsFromFile (SjArrayVirtKeybdLayout& list, const wxString& file);

};


extern SjVirtKeybdModule* g_virtKeybd;


#endif // __SJ_VIRTKEYBD_H__
