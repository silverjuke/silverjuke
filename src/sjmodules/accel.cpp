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
 * File:    accel.cpp
 * Authors: Björn Petersen
 * Purpose: Accelerator handling
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/tokenzr.h>
#include <sjmodules/accel.h>
#include <sjmodules/kiosk/kiosk.h>
#include <see_dom/sj_see.h>
#include <sjtools/msgbox.h>


/*******************************************************************************
 * SjAccelModule - Load/Unload
 ******************************************************************************/


bool SjAccelModule::FirstLoad()
{
	// already initialized?
	if( m_cmdCount != 0 ) return TRUE;

	// init accelerator table (user settings loaded later)

	// first key is null to avoid index #0
	OrgCmd(wxT(""), 0, SJA_UNEDITABLE);

	// some common keys
	OrgCmd(_("Esc"),
	       IDO_ESC,
	       SJA_MAIN | SJA_UNEDITABLE);
	OrgKey(0, WXK_ESCAPE);
	OrgCmd(_("Tab"),
	       IDO_TAB,
	       SJA_MAIN | SJA_UNEDITABLE);
	OrgKey(0, WXK_TAB);

	// Numpad Keys (should be first as they may overwrite other keys)
	{
		int numpadKeyIndex;
		for( numpadKeyIndex = 0; numpadKeyIndex <= 9; numpadKeyIndex++ )
		{
			OrgCmd(wxString::Format(wxT("%i"), numpadKeyIndex),
			       IDT_NUMPAD_0+numpadKeyIndex,
			       SJA_MAINNUMPAD);
			OrgKey(0, WXK_NUMPAD0+numpadKeyIndex);
		}
		OrgCmd(_("Cancel"),
		       IDT_NUMPAD_CANCEL,
		       SJA_MAINNUMPAD);
		OrgCmd(_("Page left"),
		       IDT_NUMPAD_PAGE_LEFT,
		       SJA_MAINNUMPAD);
		OrgKey(0, WXK_NUMPAD_DIVIDE);
		OrgCmd(_("Page right"),
		       IDT_NUMPAD_PAGE_RIGHT,
		       SJA_MAINNUMPAD);
		OrgKey(0, WXK_NUMPAD_MULTIPLY);
		OrgCmd(_("Page up"),
		       IDT_NUMPAD_PAGE_UP,
		       SJA_MAINNUMPAD);
		OrgCmd(_("Page down"),
		       IDT_NUMPAD_PAGE_DOWN,
		       SJA_MAINNUMPAD);
		OrgKey(0, WXK_NUMPAD_DECIMAL);
		OrgCmd(_("Menu"),
		       IDT_NUMPAD_MENU,
		       SJA_MAINNUMPAD);
	}

	// workspace navigation

	OrgCmd(_("Cursor left"),
	       IDT_WORKSPACE_KEY_LEFT,
	       SJA_MAIN);
	OrgKey(0, WXK_LEFT);
	OrgCmd(_("Cursor right"),
	       IDT_WORKSPACE_KEY_RIGHT,
	       SJA_MAIN);
	OrgKey(0, WXK_RIGHT);
	OrgCmd(_("Cursor up"),
	       IDT_WORKSPACE_KEY_UP,
	       SJA_MAIN);
	OrgKey(0, WXK_UP);
	OrgCmd(_("Cursor down"),
	       IDT_WORKSPACE_KEY_DOWN,
	       SJA_MAIN);
	OrgKey(0, WXK_DOWN);

	OrgCmd(_("Line left"),
	       IDT_WORKSPACE_LINE_LEFT,
	       SJA_MAIN);
	OrgCmd(_("Line right"),
	       IDT_WORKSPACE_LINE_RIGHT,
	       SJA_MAIN);
	OrgCmd(_("Line up"),
	       IDT_WORKSPACE_LINE_UP,
	       SJA_MAIN);
	OrgCmd(_("Line down"),
	       IDT_WORKSPACE_LINE_DOWN,
	       SJA_MAIN);

	OrgCmd(_("Page left"),
	       IDT_WORKSPACE_PAGE_LEFT,
	       SJA_MAIN);
	OrgCmd(_("Page right"),
	       IDT_WORKSPACE_PAGE_RIGHT,
	       SJA_MAIN);
	OrgCmd(_("Page up"),
	       IDT_WORKSPACE_PAGE_UP,
	       SJA_MAIN);
	OrgKey(0, WXK_PAGEUP);
	OrgCmd(_("Page down"),
	       IDT_WORKSPACE_PAGE_DOWN,
	       SJA_MAIN);
	OrgKey(0, WXK_PAGEDOWN);

	OrgCmd(_("Home"),
	       IDT_WORKSPACE_HOME,
	       SJA_MAIN);
	OrgKey(0, WXK_HOME);
	OrgCmd(_("End"),
	       IDT_WORKSPACE_END,
	       SJA_MAIN);
	OrgKey(0, WXK_END);

	OrgCmd(_("Enter"),
	       IDT_WORKSPACE_ENTER,
	       SJA_MAIN);
	OrgKey(0, WXK_RETURN);

	OrgCmd(_("Insert"),
	       IDT_WORKSPACE_INSERT,
	       SJA_MAIN);
	OrgKey(0, WXK_INSERT);

	OrgCmd(_("Delete"),
	       IDT_WORKSPACE_DELETE,
	       SJA_MAIN);
	OrgKey(0, WXK_DELETE);

	OrgCmd(_("Album view"),
	       IDT_WORKSPACE_ALBUM_VIEW,
	       SJA_MAIN);

	OrgCmd(_("Cover view"),
	       IDT_WORKSPACE_COVER_VIEW,
	       SJA_MAIN);

	OrgCmd(_("List view"),
	       IDT_WORKSPACE_LIST_VIEW,
	       SJA_MAIN);

	OrgCmd(_("Toggle view"),
	       IDT_WORKSPACE_TOGGLE_VIEW,
	       SJA_MAIN);
	OrgKey(0, WXK_F3);

	OrgCmd(_("Show covers"),
	       IDT_WORKSPACE_SHOW_COVERS,
	       SJA_MAIN);
	OrgKey(wxACCEL_ALT, 'C');

	// common commands - not all commands have predefined shortcuts!

	OrgCmd(_("Open playlist"),
	       IDT_OPEN_FILES,
	       SJA_MAIN | SJA_3P, SJ_ICON_MUSIC_FILE);
	OrgKey(wxACCEL_CTRL, 'O');
	OrgCmd(_("Unqueue marked tracks"),
	       IDO_UNQUEUE_MARKED,
	       SJA_MAIN);
	OrgCmd(_("Unqueue all but marked tracks"),
	       IDO_UNQUEUE_ALL_BUT_MARKED,
	       SJA_MAIN);
	OrgCmd(_("Clear playlist"),
	       IDT_UNQUEUE_ALL,
	       SJA_MAIN);
	OrgCmd(_("Save playlist"),
	       IDT_SAVE_PLAYLIST,
	       SJA_MAIN | SJA_3P);
	OrgKey(wxACCEL_CTRL, 'S');
	OrgCmd(_("Paste"),
	       IDO_PASTE,
	       SJA_MAIN);
	OrgKey(wxACCEL_CTRL, 'V');
	OrgKey(wxACCEL_SHIFT, WXK_INSERT);
	OrgCmd(_("Paste"),
	       IDO_PASTE_USING_COORD,
	       0, SJ_ICON_EMPTY, IDO_PASTE);
	OrgCmd(_("Music selection"),
	       IDT_ADV_SEARCH,
	       SJA_MAIN | SJA_3P, SJ_ICON_ADVSEARCH);
	OrgKey(wxACCEL_CTRL, 'F');
	OrgCmd(_("Jukebox settings"),
	       IDT_SETTINGS_JUKEBOX,
	       SJA_MAIN | SJA_3P, SJ_ICON_SETTINGSDIALOG);
	OrgKey(wxACCEL_ALT, 'P');
	OrgKey(wxACCEL_ALT, WXK_RETURN);
	OrgCmd(_("Advanced settings"),
	       IDO_SETTINGS_ADV,
	       SJA_MAIN | SJA_3P, SJ_ICON_EMPTY);
	OrgCmd(wxString::Format(_("Exit %s"), SJ_PROGRAM_NAME),
	       IDT_QUIT,
	       SJA_MAIN);
	OrgKey(wxACCEL_CTRL, 'Q');
	OrgKey(wxACCEL_ALT, 'X');
	OrgCmd(_("Update music library"),
	       IDT_UPDATE_INDEX,
	       SJA_MAIN | SJA_MYMUSIC);
	OrgKey(0, WXK_F5);
	OrgCmd(_("Recreate music library"),
	       IDT_DEEP_UPDATE_INDEX,
	       SJA_MAIN | SJA_MYMUSIC);
	OrgKey(wxACCEL_SHIFT, WXK_F5);
	OrgCmd(_("Enqueue tracks"),
	       IDT_ENQUEUE_LAST,
	       SJA_MAIN | SJA_ADVSEARCH, SJ_ICON_PLAY);
	OrgKey(wxACCEL_CTRL, 'E');
	OrgCmd(_("Play tracks next"),
	       IDT_ENQUEUE_NEXT,
	       SJA_MAIN);
	OrgKey(wxACCEL_CTRL, 'N');
	OrgCmd(_("Play tracks now"),
	       IDT_ENQUEUE_NOW,
	       SJA_MAIN | SJA_ADVSEARCH);
	OrgKey(wxACCEL_CTRL, 'P');
	OrgCmd(_("Play tracks at once on double click"),
	       IDT_PLAY_NOW_ON_DBL_CLICK,
	       SJA_MAIN);
	OrgKey(wxACCEL_CTRL, 'D');
	OrgCmd(_("Remove played tracks from queue"),
	       IDT_TOGGLE_REMOVE_PLAYED,
	       SJA_MAIN);
	OrgCmd(_("Unqueue tracks"),
	       IDT_UNQUEUE,
	       SJA_MAIN);
	OrgKey(wxACCEL_CTRL, 'U');
	OrgCmd(_("More from current album"),
	       IDT_MORE_FROM_CURR_ALBUM,
	       SJA_MAIN);
	OrgKey(wxACCEL_CTRL, 'J');
	OrgCmd(_("More from current artist"),
	       IDT_MORE_FROM_CURR_ARTIST,
	       SJA_MAIN);
	OrgKey(wxACCEL_CTRL, 'K');
	OrgCmd(_("Edit tracks/Get info"),
	       IDM_EDITSELECTION,
	       SJA_MAIN | SJA_EDIT | SJA_3P, SJ_ICON_EDIT);
	OrgKey(wxACCEL_CTRL, 'I');
	OrgCmd(_("Play"),
	       IDT_PLAY,
	       SJA_MAIN);
	OrgKey(0, WXK_PAUSE);
	OrgKey(0, WXK_SPACE); // if we move this above WXK_PAUSE, entering spaced in the search field will fail - why?
	OrgCmd(_("Pause"),
	       IDT_PAUSE,
	       SJA_UNEDITABLE); // same shortcut as IDT_PLAY
	OrgCmd(_("Stop"),
	       IDT_STOP,
	       SJA_MAIN);
	OrgKey(wxACCEL_SHIFT, WXK_SPACE);
	OrgCmd(_("Stop after this track"),
	       IDT_STOP_AFTER_THIS_TRACK,
	       SJA_MAIN);
	OrgCmd(_("Stop after each track"),
	       IDT_STOP_AFTER_EACH_TRACK,
	       SJA_MAIN);
	OrgCmd(_("Previous track"),
	       IDT_PREV,
	       SJA_MAIN);
	OrgKey(wxACCEL_CTRL, WXK_LEFT);
	OrgCmd(_("Next track"),
	       IDT_NEXT,
	       SJA_MAIN);
	OrgKey(wxACCEL_CTRL, WXK_RIGHT);
	OrgCmd(_("Seek backward"),
	       IDT_SEEK_BWD,
	       SJA_MAIN);
	OrgCmd(_("Seek forward"),
	       IDT_SEEK_FWD,
	       SJA_MAIN);
	OrgCmd(_("Fade to next"),
	       IDT_FADE_TO_NEXT,
	       SJA_MAIN);
	OrgCmd(_("Volume up"),
	       IDT_MAIN_VOL_UP,
	       SJA_MAIN);
	OrgKey(wxACCEL_CTRL, WXK_UP);
	OrgCmd(_("Volume down"),
	       IDT_MAIN_VOL_DOWN,
	       SJA_MAIN);
	OrgKey(wxACCEL_CTRL, WXK_DOWN);
	OrgCmd(_("Mute"),
	       IDT_MAIN_VOL_MUTE,
	       SJA_MAIN);
	OrgCmd(_("Shuffle"),
	       IDT_SHUFFLE,
	       SJA_MAIN);
	OrgCmd(_("Repeat playlist"),
	       IDT_REPEAT,
	       SJA_MAIN);
	OrgCmd(_("Reaload skin"),
	       IDO_DEBUGSKIN_RELOAD,
	       SJA_MAIN);
	OrgKey(wxACCEL_CTRL|wxACCEL_SHIFT, 'R');
	OrgCmd(_("Video screen"),
	       IDT_START_VIS,
	       SJA_MAIN);
	OrgKey(0, WXK_F2);
	OrgCmd(_("Smooth"),
	       IDO_SMOOTH,
	       SJA_MAIN | SJA_ART );
	OrgCmd(_("Toggle time mode"),
	       IDT_TOGGLE_TIME_MODE,
	       SJA_MAIN);
	OrgCmd(_("Select all"),
	       IDO_SELECTALL,
	       SJA_MAIN);
	OrgKey(wxACCEL_CTRL, 'A');
	OrgCmd(_("Zoom in"),
	       IDT_ZOOM_IN,
	       SJA_MAIN);
	OrgKey(0, WXK_ADD);
	OrgCmd(_("Zoom out"),
	       IDT_ZOOM_OUT,
	       SJA_MAIN);
	OrgKey(0, WXK_SUBTRACT);
	OrgCmd(_("Normal zoom"),
	       IDT_ZOOM_NORMAL,
	       SJA_MAIN);
	OrgKey(0, WXK_MULTIPLY);
	OrgCmd(_("Always on top"),
	       IDT_ALWAYS_ON_TOP,
	       SJA_MAIN);
	OrgCmd(_("Show file"),
	       IDM_EXPLORE,
	       SJA_MAIN | SJA_MYMUSIC | SJA_ART);
	OrgCmd(_("Kiosk mode"),
	       IDT_TOGGLE_KIOSK,
	       SJA_MAIN | SJA_KIOSKSETTINGS);
	OrgKey(0, WXK_F11);

	OrgCmd(_("Go to current track"),
	       IDT_GOTO_CURR,
	       SJA_MAIN);
	OrgKey(wxACCEL_CTRL, 'T');
	OrgCmd(_("Go to marked track"),
	       IDO_GOTO_CURR_MARK,
	       SJA_MAIN | SJA_UNEDITABLE); // edit may be possible but does not make much sense as marking tracks in the display is very temporary and mostly only possible using the mouse+context menu
	OrgCmd(_("Go to random album"),
	       IDT_WORKSPACE_GOTO_RANDOM,
	       SJA_MAIN);
	OrgKey(wxACCEL_CTRL, 'R');

	{
		wxString descr = _("Go to \"%s\"");
		int c;
		for( c = IDT_WORKSPACE_GOTO_A; c <= IDT_WORKSPACE_GOTO_0_9; c++ )
		{
			OrgCmd(wxString::Format(descr, SjColumnMixer::GetAzDescr(c).c_str()),
			       c,
			       SJA_MAIN);
			OrgKey(0, c==IDT_WORKSPACE_GOTO_0_9? '0' : 'A'+(c-IDT_WORKSPACE_GOTO_A));
		}
	}

	OrgCmd(_("Go to previous letter"),
	       IDT_WORKSPACE_GOTO_PREV_AZ,
	       SJA_MAIN);

	OrgCmd(_("Go to next letter"),
	       IDT_WORKSPACE_GOTO_NEXT_AZ,
	       SJA_MAIN);


	{
		int idtCredit;
		for( idtCredit = IDT_ADD_CREDIT_01; idtCredit <= IDT_ADD_CREDIT_16; idtCredit++ )
		{
			OrgCmd(_("Add credit")+wxString::Format(wxT(" (+%i)"), (idtCredit-IDT_ADD_CREDIT_01)+1),
			       idtCredit,
			       SJA_MAIN);
		}
	}

	// tools shortcuts

	{
		OrgCmd(_("Console"),
		       IDO_CONSOLE,
		       SJA_MAIN | SJA_3P);

		// shortcuts for the scripting menu -
		// all this is a little hack at the moment and has the following limitations:
		//  - max. 100 menu entries / shortcuts allowed for ALL scripts together
		//  - all menu entries of ALL scripts are defined by a zero-based index
		//  - therefore program.removeMenuEntry() or sth. like that will disturb the shortcuts and need a re-assign
		// see also http://www.silverjuke.net/forum/topic-1726.html
		int i, iCount = (IDO_EXTRAS_MENU99-IDO_EXTRAS_MENU00)+1, id;
		for( i = 0; i < iCount; i++ )
		{
			id = IDO_EXTRAS_MENU00+i;
			wxASSERT(id>=IDO_EXTRAS_MENU00 && id<=IDO_EXTRAS_MENU99);
			OrgCmd(_("Tools")+wxString::Format(wxT(" #%i"), i+1),
			       id,
			       SJA_MAIN);

		}
	}

	// tag editor shortcuts

	OrgCmd(_("Previous track"),
	       IDC_PREVDLGPAGE,
	       SJA_EDIT);
	OrgKey(0, WXK_PAGEUP);

	OrgCmd(_("Next track"),
	       IDC_NEXTDLGPAGE,
	       SJA_EDIT);
	OrgKey(0, WXK_PAGEDOWN);

	OrgCmd(_("Replace"),
	       IDC_PLUGIN_REPLACE,
	       SJA_EDIT | SJA_3P);
	OrgKey(wxACCEL_CTRL, 'R');

	OrgCmd(_("Split field"),
	       IDC_PLUGIN_SPLIT,
	       SJA_EDIT | SJA_3P);

	OrgCmd(_("Rename files"),
	       IDC_PLUGIN_RENAME,
	       SJA_EDIT | SJA_3P);

	OrgCmd(_("Query online database"),
	       IDC_PLUGIN_FREEDB,
	       SJA_EDIT | SJA_3P);
	OrgKey(wxACCEL_CTRL, 'F');

	// adv. search shortcuts

	OrgCmd(_("New music selection"),
	       IDC_NEWSEARCH,
	       SJA_ADVSEARCH | SJA_3P);
	OrgKey(wxACCEL_CTRL, 'N');

	OrgCmd(_("Save"),
	       IDC_SAVESEARCH,
	       SJA_ADVSEARCH);
	OrgKey(wxACCEL_CTRL, 'S');

	OrgCmd(_("Save as"),
	       IDC_SAVESEARCHAS,
	       SJA_ADVSEARCH | SJA_3P);
	OrgKey(wxACCEL_CTRL, 'M');

	OrgCmd(_("Rename"),
	       IDC_RENAMESEARCH,
	       SJA_ADVSEARCH | SJA_3P);
	OrgKey(wxACCEL_CTRL, 'R');

	OrgCmd(_("Delete"),
	       IDC_DELETESEARCH,
	       SJA_ADVSEARCH);
	OrgKey(wxACCEL_CTRL, 'D');

	OrgCmd(_("Revert to saved"),
	       IDC_REVERTTOSAVEDSEARCH,
	       SJA_ADVSEARCH);

	OrgCmd(_("End search"),
	       IDC_ENDADVSEARCH,
	       SJA_ADVSEARCH);

	// art editor shortcuts

	OrgCmd(_("Open file")/*yeah, "File", not "Cover" as this appears in a submenu as "Open cover -> Open file" submenu*/,
	       IDM_OPENCOVER,
	       SJA_ART | SJA_3P, SJ_ICON_EMPTY, IDT_OPEN_FILES);
	OrgCmd(_("Save cover"),
	       IDM_SAVECOVER,
	       SJA_ART | SJA_3P, SJ_ICON_EMPTY, IDT_SAVE_PLAYLIST);
	OrgCmd(_("Rotate left"),
	       IDM_ROTATELEFT,
	       SJA_ART);
	OrgKey(0, 'L');
	OrgCmd(_("Rotate right"),
	       IDM_ROTATERIGHT,
	       SJA_ART);
	OrgKey(0, 'R');
	OrgCmd(_("Flip horizontally"),
	       IDM_FLIPHORZ,
	       SJA_ART);
	OrgKey(0, 'H');
	OrgCmd(_("Flip vertically"),
	       IDM_FLIPVERT,
	       SJA_ART);
	OrgKey(0, 'V');
	OrgCmd(_("Grayscale"),
	       IDM_GRAYSCALE,
	       SJA_ART);
	OrgKey(0, 'G');
	OrgCmd(_("Negative"),
	       IDM_NEGATIVE,
	       SJA_ART);
	OrgKey(0, 'N');
	OrgCmd(_("Fit to screen"),
	       IDM_FULLSCREEN,
	       SJA_ART | SJA_VIS);
	OrgKey(0, WXK_F11);
	OrgCmd(_("Decrease brightness"),
	       IDM_BRIGHTMINUS,
	       SJA_ART);
	OrgKey(0, '1');
	OrgCmd(_("Increase brightness"),
	       IDM_BRIGHTPLUS,
	       SJA_ART);
	OrgKey(0, '2');
	OrgCmd(_("Decrease contrast"),
	       IDM_CONTRASTMINUS,
	       SJA_ART);
	OrgKey(0, '3');
	OrgCmd(_("Increase contrast"),
	       IDM_CONTRASTPLUS,
	       SJA_ART);
	OrgKey(0, '4');
	OrgCmd(_("Normal brightness/contrast"),
	       IDM_CONTRASTBRIGHTNULL,
	       SJA_ART);
	OrgKey(0, '0');
	OrgCmd(_("Crop"),
	       IDM_CROP,
	       SJA_ART);
	OrgKey(0, 'C');

	// create hashes (a), m_cmdId2CmdIndex is is static all the time,
	// m_cmdId2CmdIndex is needed to load the user settings

	{
		int cmdIndex;
		for( cmdIndex = 1/*first is void*/; cmdIndex < m_cmdCount; cmdIndex++ )
		{
			wxASSERT( m_cmdId2CmdIndex.Lookup(m_cmd[cmdIndex].m_id) == 0 );
			m_cmdId2CmdIndex.Insert(m_cmd[cmdIndex].m_id, cmdIndex);
		}
	}

	// load user settings
	// user settings are stored as a simple list of hex numbers as:
	// <id1>,<keyCount>,<key1>,<key2>,...,<id2>,...
	// however, instead of commas, the caracter "g" is used as a separator

	{
		m_flags = g_tools->m_config->Read(wxT("main/accelFlags"), SJ_ACCEL_DEFAULT);
		m_selDragNDrop = g_tools->m_config->Read(wxT("main/selDragNDrop"), 1L);
		m_middleMouseAction = g_tools->m_config->Read(wxT("main/middleMouseAction"), SJ_ACCEL_MID_OPENS_TAG_EDITOR);
		wxString iniShortcuts = g_tools->m_config->Read(wxT("main/shortcuts"), wxT(""));
		if( !iniShortcuts.IsEmpty() )
		{
			long val, cmdIndex = 0, keyCount = -1;
			wxStringTokenizer tkz(iniShortcuts, wxT(","));
			while( tkz.HasMoreTokens() )
			{
				if( tkz.GetNextToken().ToLong(&val, 16) && val >= 0 )
				{
					if( cmdIndex == 0 )
					{
						cmdIndex = m_cmdId2CmdIndex.Lookup(val);
						if( cmdIndex <= 0 || cmdIndex >= m_cmdCount ) break; // error
						keyCount = -1;
					}
					else if( keyCount == -1 )
					{
						keyCount = val;
						m_cmd[cmdIndex].m_userKeyCount = 0;
						if( keyCount < 0 || keyCount > SJ_ACCEL_MAX_KEYS ) break; // error
						if( keyCount == 0 ) cmdIndex = 0; // next command
					}
					else if( keyCount > 0 )
					{
						if( val == 0 ) break; // error
						m_cmd[cmdIndex].m_userKey[m_cmd[cmdIndex].m_userKeyCount++] = val;
						keyCount--;
						if( keyCount == 0 ) cmdIndex = 0; // next command
					}
					else
					{
						break; // error
					}
				}
				else
				{
					break; // error
				}
			}
		}
	}

	// create hashes (b)

	BuildHashes();

	// init system shortcuts

	InitSystemAccel();
	UpdateSystemAccel(TRUE/*set*/);

	// done

	return TRUE;
}


void SjAccelModule::LastUnload()
{
	ExitSystemAccel();
	g_accelModule = NULL;
}


/*******************************************************************************
 * SjAccelConfigWindow
 ******************************************************************************/


#define IDC_NEWKEY             (IDM_LASTPRIVATE-1)


class SjWaitForKeyCtrl : public wxWindow
{
public:
	SjWaitForKeyCtrl(wxWindow* parent)
		: wxWindow(parent, -1, wxDefaultPosition, wxDefaultSize, 0)
	{
		m_key = 0;
	}

	long GetKey()
	{
		return m_key;
	}

private:
	long m_key;
	void OnKey (wxKeyEvent& event)
	{
		// get the keycode.  modifiers as keycodes are not accepted, in this case,
		// wait for the next key
		long keycode = event.GetKeyCode();

		if( keycode == WXK_LBUTTON
				|| keycode == WXK_RBUTTON
				|| keycode == WXK_MBUTTON
				|| keycode == WXK_SHIFT
				|| keycode == WXK_ALT
				|| keycode == WXK_CONTROL
				|| keycode == WXK_MENU
				|| keycode == WXK_CAPITAL )
		{
			return;
		}

		// get the modifiers
		long modifiers = 0;
		if( event.AltDown() ) modifiers |= wxACCEL_ALT;
		if( event.ShiftDown() ) modifiers |= wxACCEL_SHIFT;
		if( event.ControlDown() )  modifiers |= wxACCEL_CTRL;
		wxLogDebug(wxT("meta.%i"), event.MetaDown());

		// save the key
		m_key = (modifiers << 16) | keycode;

		wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, IDC_NEWKEY);
		GetParent()->AddPendingEvent(evt);
	}
	DECLARE_EVENT_TABLE ();
};


BEGIN_EVENT_TABLE(SjWaitForKeyCtrl, wxWindow)
	EVT_KEY_UP              (SjWaitForKeyCtrl::OnKey          )
END_EVENT_TABLE()


class SjWaitForKeyDlg : public SjDialog
{
public:
	SjWaitForKeyDlg   (wxWindow* parent, const wxString& title)
		: SjDialog(parent, title, SJ_MODAL, SJ_NEVER_RESIZEABLE)
	{
		wxSizer* sizer1 = new wxBoxSizer(wxVERTICAL);
		SetSizer(sizer1);

		sizer1->Add(new wxStaticText(this, -1, _("Please press the shortcut to add.")), 0, wxGROW|wxLEFT|wxTOP|wxRIGHT, SJ_DLG_SPACE);

		m_keyCtrl = new SjWaitForKeyCtrl(this);
		sizer1->Add(m_keyCtrl);

		sizer1->Add(CreateButtons(SJ_DLG_CANCEL), 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, SJ_DLG_SPACE);

		sizer1->SetSizeHints(this);
		CentreOnParent();
		m_keyCtrl->SetFocus();
	}

	long GetKey()
	{
		return m_keyCtrl->GetKey();
	}

private:
	SjWaitForKeyCtrl* m_keyCtrl;
	void NewKey(wxCommandEvent&)
	{
		EndModal(wxID_OK);
	}
	DECLARE_EVENT_TABLE ();
};


BEGIN_EVENT_TABLE(SjWaitForKeyDlg, SjDialog)
	EVT_MENU            (IDC_NEWKEY, SjWaitForKeyDlg::NewKey          )
END_EVENT_TABLE()


/*******************************************************************************
 * SjAccelModule - Configuration
 ******************************************************************************/


class SjLittleAccelOption : public SjLittleOption
{
public:
	SjLittleAccelOption (const wxString& name, SjAccelCmd* cmdPtr, bool lastOption);
	SjIcon          GetIcon             () const;
	wxString        GetDisplayValue     () const;
	bool            IsModified          () const { return !AreTempAndOrgEqual(); }
	long            GetOptionCount      () const {
		return 1/*add*/
#if SJ_CAN_USE_GLOBAL_HOTKEYS
		       +1/*add system wide*/
#endif
		       +m_cmdPtr->m_tempKeyCount/*remove*/;
	}
	wxString        GetOption           (long i) const;
	bool            OnOption            (wxWindow* parent, long i);
	bool            OnDoubleClick       (wxWindow* parent) { return OnOption(parent, 0); }
	bool            OnDefault           (wxWindow* parent);
	void            OnApply             ();

private:
	SjAccelCmd*     m_cmdPtr;
	bool            m_lastOption;

	bool            AreTempAndOrgEqual  () const;
	void            RemoveLongFromArray (long array[], int& arrayCount, int indexToRemove);
	wxString        GetShortName        (const wxString& str) const { return str.AfterLast(':').Trim(FALSE); }
	wxString        GetShortName        () const { return GetShortName(GetName()); }
	bool            AddKey              (long newKey, wxWindow* parent);
};


SjLittleAccelOption::SjLittleAccelOption(const wxString& name, SjAccelCmd* cmdPtr, bool lastOption)
	: SjLittleOption(name, SJ_ICON_EMPTY/*we overwrite GetIcon()*/)
{
	m_cmdPtr        = cmdPtr;
	m_lastOption    = lastOption;

	cmdPtr->m_tempKeyCount = cmdPtr->m_userKeyCount;
	if( cmdPtr->m_userKeyCount )
	{
		wxASSERT( m_cmdPtr->m_tempKeyCount>0 && m_cmdPtr->m_tempKeyCount<SJ_ACCEL_MAX_KEYS );
		memcpy(cmdPtr->m_tempKey, cmdPtr->m_userKey, cmdPtr->m_userKeyCount*sizeof(long));
	}
}


bool SjLittleAccelOption::AreTempAndOrgEqual() const
{
	wxASSERT( m_cmdPtr->m_tempKeyCount >= 0 && m_cmdPtr->m_tempKeyCount < SJ_ACCEL_MAX_KEYS );
	wxASSERT( m_cmdPtr->m_orgKeyCount >= 0 && m_cmdPtr->m_orgKeyCount < SJ_ACCEL_MAX_KEYS );

	if( m_cmdPtr->m_tempKeyCount==m_cmdPtr->m_orgKeyCount )
	{
		if( m_cmdPtr->m_tempKeyCount == 0
		        || memcmp(m_cmdPtr->m_tempKey, m_cmdPtr->m_orgKey, m_cmdPtr->m_tempKeyCount*sizeof(long))==0 )
		{
			return TRUE;
		}
	}
	return FALSE;
}


void SjLittleAccelOption::RemoveLongFromArray(long array[], int& arrayCount, int indexToRemove)
{
	wxASSERT( arrayCount > 0 );
	wxASSERT( indexToRemove < arrayCount );
	int i;
	for( i = indexToRemove+1; i < arrayCount; i++ )
	{
		array[i-1] = array[i];
	}
	arrayCount--;
}


wxString SjLittleAccelOption::GetDisplayValue() const
{
	wxString ret = g_accelModule->GetReadableShortcutsByPtr(m_cmdPtr->m_tempKey, m_cmdPtr->m_tempKeyCount, SJ_SHORTCUTS_LOCAL|SJ_SHORTCUTS_ADDALL);
	wxString add = g_accelModule->GetReadableShortcutsByPtr(m_cmdPtr->m_tempKey, m_cmdPtr->m_tempKeyCount, SJ_SHORTCUTS_SYSTEMWIDE|SJ_SHORTCUTS_ADDALL);
	if( !add.IsEmpty() )
	{
		if( !ret.IsEmpty() ) ret += wxT(", ");
		ret += _("System-wide:");
		ret += wxT(" ");
		ret += add;
	}

	if( m_cmdPtr->m_id == IDT_NUMPAD_MENU
	        && ret.IsEmpty()
	        && g_kioskModule )
	{
		// if no numpad-"menu" key is defined, the menu can be opened
		// by pressing "0" several times (more exact: the number of album digits)
		ret = wxString(wxT('0'), g_kioskModule->m_numpadInput.GetNumAlbumDigits());
	}

	return ret;
}


SjIcon SjLittleAccelOption::GetIcon() const
{
	int i;
	for( i = 0; i < m_cmdPtr->m_tempKeyCount; i++ )
	{
		if( m_cmdPtr->m_tempKey[i] & (wxACCEL_SYSTEMWIDE<<16) )
		{
			return SJ_ICON_SYSTEMKEY;
		}
	}

	return SJ_ICON_NORMALKEY;
}


wxString SjLittleAccelOption::GetOption(long i) const
{
#if SJ_CAN_USE_GLOBAL_HOTKEYS
#define OPTION_ADD              0
#define OPTION_ADDSYSTEMWIDE    1
#define OPTION_REMOVEFIRST      2
#else
#define OPTION_ADD              0
#define OPTION_REMOVEFIRST      1
#endif

	if( i>=OPTION_REMOVEFIRST && i<OPTION_REMOVEFIRST+SJ_ACCEL_MAX_KEYS )
	{
		return wxString::Format(_("Remove shortcut \"%s\""), g_accelModule->GetReadableShortcutByComprKey(m_cmdPtr->m_tempKey[i-OPTION_REMOVEFIRST]).c_str());
	}
#if SJ_CAN_USE_GLOBAL_HOTKEYS
	else if( i == OPTION_ADDSYSTEMWIDE )
	{
		return _("Add system-wide shortcut...");
	}
#endif
	else
	{
		return wxString::Format(_("Add shortcut to \"%s\"..."), GetShortName().c_str());
	}
}


bool SjLittleAccelOption::AddKey(long newKey, wxWindow* parent)
{
	bool othersAffected = FALSE;

	// check that the key is not yet used
	// and what to do IF it is used
	long cmdIndex, keyIndex;
	for( cmdIndex = 0; cmdIndex < g_accelModule->m_cmdCount; cmdIndex++ )
	{
		wxASSERT( cmdIndex >= 0 && cmdIndex < SJ_ACCEL_MAX_CMDS );

		SjAccelCmd* cmd = &g_accelModule->m_cmd[cmdIndex];

		wxASSERT( cmd->m_tempKeyCount >= 0 && cmd->m_tempKeyCount < SJ_ACCEL_MAX_KEYS );

		for( keyIndex = 0; keyIndex < cmd->m_tempKeyCount; keyIndex++ )
		{
			if(  (cmd->m_tempKey[keyIndex]&(~(wxACCEL_SYSTEMWIDE<<16))) == (newKey&(~(wxACCEL_SYSTEMWIDE<<16)))
			        && ((cmd->m_flags&SJA_MASK) & (m_cmdPtr->m_flags&SJA_MASK)) ) // is the key used in the same part?
			{
				if( cmd == m_cmdPtr )
				{
					m_cmdPtr->m_tempKey[keyIndex] = newKey; // maybe the system-wide flag gets set/removed
					return othersAffected; // shortcut is already assigned to this command
				}

				if( parent )
				{
					// if the parent is NOT set, this is a global reset action
					// wheras we do NOT modify other keys
					wxWindowDisabler disabler(parent);
					if( ::SjMessageBox(
					            wxString::Format(_("The shortcut \"%s\" is currently assigned to the command \"%s\".\n\nDo you want to assign it to the shortcut \"%s\" now?"),
					                             g_accelModule->GetReadableShortcutByComprKey(newKey).c_str(),
					                             GetShortName(g_accelModule->GetCmdNameByIndex(cmdIndex)).c_str(),
					                             GetShortName().c_str()),
					            SJ_PROGRAM_NAME,
					            wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION, parent) == wxNO )
					{
						return othersAffected; // the shortcut should stay along with the old command
					}

					// remove old key
					RemoveLongFromArray(cmd->m_tempKey, cmd->m_tempKeyCount, keyIndex);
					othersAffected = TRUE;
				}
			}
		}
	}

	// add new key
	// we won't resort the list, i think this is too confusing to the user
	wxASSERT( m_cmdPtr->m_tempKeyCount >= 0 && m_cmdPtr->m_tempKeyCount <= SJ_ACCEL_MAX_KEYS );
	if( m_cmdPtr->m_tempKeyCount >= SJ_ACCEL_MAX_KEYS )
	{
		m_cmdPtr->m_tempKeyCount = SJ_ACCEL_MAX_KEYS-1; // overwrite last key
	}

	m_cmdPtr->m_tempKey[m_cmdPtr->m_tempKeyCount++] = newKey;

	return othersAffected;
}


bool SjLittleAccelOption::OnOption(wxWindow* parent, long optionIndex)
{
	bool othersAffected = FALSE;

	if( optionIndex>=OPTION_REMOVEFIRST && optionIndex<OPTION_REMOVEFIRST+SJ_ACCEL_MAX_KEYS )
	{
		// remove shortcut
		RemoveLongFromArray(m_cmdPtr->m_tempKey, m_cmdPtr->m_tempKeyCount, optionIndex-OPTION_REMOVEFIRST);
	}
	else
	{
		// add (system-wide) shortcut...

		// ...unset system hotkeys to receive all commands
		g_accelModule->UpdateSystemAccel(FALSE/*clear*/);

		// ...wait for a key
		long newKey = 0;
		{
			wxWindowDisabler disabler(SjDialog::FindTopLevel(parent));
			SjWaitForKeyDlg dlg(parent, GetName());
			if( dlg.ShowModal() == wxID_OK )
			{
				newKey = dlg.GetKey();
			}
		}

		// ...re-activate system hotkeys
		g_accelModule->UpdateSystemAccel(TRUE/*set*/);

		// ...let's see what to do with the new key
		if( newKey && newKey != WXK_ESCAPE )
		{
#if SJ_CAN_USE_GLOBAL_HOTKEYS
			if( optionIndex == OPTION_ADDSYSTEMWIDE )
			{
				newKey |= (wxACCEL_SYSTEMWIDE<<16);
			}
#endif

			if( AddKey(newKey, parent) )
			{
				othersAffected = TRUE;
			}
		}
	}

	return othersAffected;
}


bool SjLittleAccelOption::OnDefault(wxWindow* parent)
{
	bool othersAffected = FALSE;

	// remove all keys
	m_cmdPtr->m_tempKeyCount = 0;

	// copy original keys to temp
	int keyIndex;
	for( keyIndex = 0; keyIndex < m_cmdPtr->m_orgKeyCount; keyIndex++ )
	{
		if( AddKey(m_cmdPtr->m_orgKey[keyIndex], parent) )
		{
			othersAffected = TRUE;
		}
	}

	return othersAffected;
}


void SjLittleAccelOption::OnApply()
{
	m_cmdPtr->m_userKeyCount = m_cmdPtr->m_tempKeyCount;
	if( m_cmdPtr->m_tempKeyCount )
	{
		wxASSERT( m_cmdPtr->m_tempKeyCount>0 && m_cmdPtr->m_tempKeyCount<SJ_ACCEL_MAX_KEYS );
		memcpy(m_cmdPtr->m_userKey, m_cmdPtr->m_tempKey, m_cmdPtr->m_tempKeyCount*sizeof(long));
	}

	if( m_lastOption )
	{
		// save settings
		wxString iniShortcuts;
		int i, j;
		for( i = 0; i < g_accelModule->m_cmdCount; i++ )
		{
			SjAccelCmd& cmd = g_accelModule->m_cmd[i];

			if( !cmd.AreUserAndOrgEqual() ) // only modified shortcuts are written
			{
				iniShortcuts += wxString::Format(wxT("%x,%x,"), (int)cmd.m_id, (int)cmd.m_userKeyCount);
				for( j = 0; j < cmd.m_userKeyCount; j++ )
				{
					iniShortcuts += wxString::Format(wxT("%x,"), (int)cmd.m_userKey[j]);
				}
			}
		}

		if( !iniShortcuts.IsEmpty() ) {
			iniShortcuts.Truncate(iniShortcuts.Len()-1); // remove the last comma
		}

		g_accelModule->BuildHashes();

		g_tools->m_config->Write(wxT("main/shortcuts"), iniShortcuts);

		// apply new shortcuts
		g_mainFrame->SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_MAIN));

		g_accelModule->UpdateSystemAccel(TRUE/*set*/);

		g_mainFrame->InitMainMenu();
	}
}


void SjAccelModule::GetLittleOptions(SjArrayLittleOption& lo)
{
	#define SEP wxString(wxT("|"))

	// mouse
	SjLittleOption::SetSection(_("Mouse"));

	lo.Add(new SjLittleBit (_("Drag window content"), _("Off")+SEP+_("Left mouse button"),
	                        &m_flags, 1L, SJ_ACCEL_CONTENT_DRAG, wxT("main/accelFlags")));
	lo.Add(new SjLittleBit (_("Drag selected tracks"), _("Off")+SEP+_("Left mouse button")+SEP+_("Second click with the left mouse button"),
	                        &m_selDragNDrop, 1L, 0, wxT("main/selDragNDrop")));
	lo.Add(new SjLittleBit (_("Show icons beside mouse cursor"), wxT("yn"),
	                        &m_flags, 1L, SJ_ACCEL_USE_DND_IMAGES, wxT("main/accelFlags")));
	lo.Add(new SjLittleBit (_("Middle mouse button"), _("Off")+SEP+
	                        _("Edit tracks")+wxString(wxT("/"))+_("Cover editor"),
	                        &m_middleMouseAction, SJ_ACCEL_MID_OPENS_TAG_EDITOR, 0, wxT("main/middleMouseAction")));
	#if SJ_USE_TOOLTIPS
	lo.Add(new SjLittleBit (_("Tooltips"), wxT("oo"),
	                        &m_flags, 1L, SJ_ACCEL_TOOLTIPS, wxT("main/accelFlags")));
	#endif

	// mouse wheel
	SjLittleOption::ClearSection();
	lo.Add(new SjLittleBit (_("Mouse wheel"), _("Scroll vertically")+SEP+_("Scroll horizontally"),
	                        &m_flags, 0L, SJ_ACCEL_VERT_WHEEL_HORZ, wxT("main/accelFlags")));
	SjLittleOption::SetSection(_("Mouse wheel"));
	lo.Add(new SjLittleBit (_("Context sensitive"), wxT("yn"), //n/t
	                        &m_flags, 1L, SJ_ACCEL_CONTEXT_SENSITIVE_WHEEL, wxT("main/accelFlags")));

	// start playback on enqueue?
	SjLittleOption::ClearSection();
	lo.Add(new SjLittleBit (_("Start playback on enqueue"), wxT("yn"), //n/t
	                        &m_flags, 1L, SJ_ACCEL_START_PLAYBACK_ON_ENQUEUE, wxT("main/accelFlags")));

	// find out the extra menu items
	#if SJ_USE_SCRIPTS
	wxArrayString extrasArr = SjSee::GetGlobalEmbeddings(SJ_PERSISTENT_MENU_ENTRY);
	int           extrasCount = extrasArr.GetCount();
	#endif

	// shortcuts (should be last as the last option will save all the stuff)
	SjLittleOption::SetSection(_("Shortcut"));
	int cmdIndex;
	SjAccelCmd* cmd;
	bool addOption;
	for( cmdIndex = 0; cmdIndex < m_cmdCount; cmdIndex++ )
	{
		cmd = &g_accelModule->m_cmd[cmdIndex];
		cmd->m_tempKeyCount = 0;    // as we check all other keys for collusions when adding new keys,
		// make sure, the key count is zero for uneditable keys.
		// the SjLittleAccelOption constructor will change m_tempKeyCount
		// if needed.

		addOption = true;

		if( (cmd->m_flags&SJA_UNEDITABLE) || cmd->m_linkId )
		{
			wxASSERT( cmdIndex!=m_cmdCount-1 ); // ensure, the "last" flag gets set
			addOption = false;
		}

		if(  (cmd->m_id >= IDT_NUMPAD_FIRST && cmd->m_id <= IDT_NUMPAD_LAST)
		        && !(g_accelModule->m_flags&SJ_ACCEL_USE_NUMPAD_IN_KIOSK) )
		{
			wxASSERT( cmdIndex!=m_cmdCount-1 ); // ensure, the "last" flag gets set
			addOption = false;
		}

		if( (cmd->m_id >= IDT_ADD_CREDIT_01 && cmd->m_id <= IDT_ADD_CREDIT_16)
		        && (!g_kioskModule->m_creditBase.IsCreditSystemEnabled() || !g_kioskModule->m_creditBase.ListenToShortcuts()) )
		{
			wxASSERT( cmdIndex!=m_cmdCount-1 ); // ensure, the "last" flag gets set
			addOption = false;
		}

		if( cmd->m_id >= IDO_EXTRAS_MENU00 && cmd->m_id <= IDO_EXTRAS_MENU99 )
		{
			wxASSERT( cmdIndex!=m_cmdCount-1 ); // ensure, the "last" flag gets set
			addOption = false;
			#if SJ_USE_SCRIPTS
			if( cmd->m_id-IDO_EXTRAS_MENU00 < extrasCount )
			{
				cmd->m_name = _("Tools") + wxString(wxT(": ")) + extrasArr[cmd->m_id-IDO_EXTRAS_MENU00];
				addOption = true;
			}
			#endif
		}

		if( addOption )
		{
			lo.Add(new SjLittleAccelOption(GetCmdNameByIndex(cmdIndex), cmd, (cmdIndex==m_cmdCount-1)));
		}
	}

	// we do not add the context menu option "Play tracks at once on double click" here;
	// as the settings in the further options should be of the type "set once and forget"
	// the the double click option may be changed frequently by the user.

	// multimedia keys
	SjLittleOption::ClearSection();

	#if SJ_CAN_USE_MM_KEYBD
	lo.Add(new SjLittleBit (_("Use multimedia keyboard keys"), wxT("yn"),
	                        &m_flags, 0L, SJ_ACCEL_USE_MM_KEYBD, wxT("main/accelFlags")));
	#endif

	// confirmation stuff
	lo.Add(new SjLittleBit(_("Ask on close if playing"), wxT("yn"),
	                       &m_flags, 1L, SJ_ACCEL_ASK_ON_CLOSE_IF_PLAYING, wxT("main/accelFlags")));

	lo.Add(new SjLittleBit(_("Ask before enqueuing multiple tracks"), wxT("yn"),
	                       &m_flags, 1L, SJ_ACCEL_ASK_ON_MULTI_ENQUEUE, wxT("main/accelFlags")));

	lo.Add(new SjLittleBit(_("Ask before clearing the playlist"), wxT("yn"),
	                       &m_flags, 1L, SJ_ACCEL_ASK_ON_CLEAR_PLAYLIST, wxT("main/accelFlags")));

	lo.Add(new SjLittleBit(_("Ask before changing the rating"), wxT("yn"),
	                       &m_flags, 1L, SJ_ACCEL_ASK_ON_RATING_CHANGE, wxT("main/accelFlags")));
}


/*******************************************************************************
 * SjAccelModule - Misc.
 ******************************************************************************/


SjAccelModule* g_accelModule = NULL;


SjAccelModule::SjAccelModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	m_file              = wxT("memory:accel.lib");
	m_name              = _("Further options");
	m_cmdCount          = 0;
	m_totalUserKeyCount = 0;
	m_sort              = 2000; // show at end (default is 1000)
	g_accelModule       = this;
}


void SjAccelModule::OrgCmd(const wxString& name, int id, long flags, SjIcon icon, int linkId)
{
	wxASSERT(m_cmdCount<SJ_ACCEL_MAX_CMDS);

	m_cmd[m_cmdCount].m_name        = name;
	m_cmd[m_cmdCount].m_id          = id;
	m_cmd[m_cmdCount].m_flags       = flags;
	m_cmd[m_cmdCount].m_icon        = icon;
	m_cmd[m_cmdCount].m_linkId      = linkId;
	m_cmd[m_cmdCount].m_userKeyCount= 0;
	m_cmd[m_cmdCount].m_orgKeyCount = 0;

	m_cmdCount++;
}


void SjAccelModule::OrgKey(long orgModifier, long orgKeycode)
{
	wxASSERT(m_cmdCount>0 && m_cmdCount<SJ_ACCEL_MAX_CMDS);
	SjAccelCmd* cmd = &m_cmd[m_cmdCount-1];
	if( cmd->m_orgKeyCount < SJ_ACCEL_MAX_KEYS )
	{
		wxASSERT(cmd->m_orgKeyCount<=SJ_ACCEL_MAX_KEYS);
		wxASSERT(!cmd->m_linkId);

		cmd->m_orgKey [cmd->m_orgKeyCount] =
		    cmd->m_userKey[cmd->m_orgKeyCount] = (orgModifier<<16)|orgKeycode;
		cmd->m_orgKeyCount++;
		cmd->m_userKeyCount++;
	}
}


static bool addToAccelTable(long key)
{
	// some common keys are never added to the global accelerator tables as
	// they are used eg. in child controls

	if( !(key & 0x7FFF0000L) )
	{
		if(  (key >= 'A' && key <= 'Z')
		  || (key >= 'a' && key <= 'z')
		  || (key >= '0' && key <= '9')
		/*||  key == WXK_TAB  !!!tab IS an accelerator key!!! */
		  ||  key == WXK_INSERT|| key == WXK_DELETE
		  ||  key == WXK_LEFT  || key == WXK_RIGHT    || key == WXK_UP    || key == WXK_DOWN
		  ||  key == WXK_SPACE || key == WXK_RETURN   || key == WXK_BACK
		  ||  key == WXK_HOME  || key == WXK_END
		  ||  key == WXK_ADD   || key == WXK_SUBTRACT || key == WXK_MULTIPLY )
		{
			return false;
		}
	}

	return true;
}


wxAcceleratorTable SjAccelModule::GetAccelTable(long flags, wxAcceleratorEntry* addEntries, int addEntriesCount)
{
	if( m_cmdCount )
	{
		// add numpad keys?
		if( (flags & SJA_MAIN)
		        &&  UseNumpad() )
		{
			flags |= SJA_MAINNUMPAD;
		}

		// allocate memory for the table
		wxAcceleratorEntry* entries = new wxAcceleratorEntry[m_totalUserKeyCount+addEntriesCount];

		// add our entries to the table
		long cmdIndex, keyIndex, entriesCount = 0, key, cmdId;
		SjAccelCmd* cmd;
		SjLLHash keysAdded;
		for( cmdIndex = 0; cmdIndex < m_cmdCount; cmdIndex++ )
		{
			cmd = &m_cmd[cmdIndex];
			if( cmd->m_flags & flags )
			{
				cmdId = cmd->m_id;
				if( cmd->m_linkId )
				{
					cmd = &m_cmd[CmdId2CmdIndex(cmd->m_linkId)];
					// leave command id!
				}

				for( keyIndex = 0; keyIndex < cmd->m_userKeyCount; keyIndex++ )
				{
					key = cmd->m_userKey[keyIndex];
					if( !(key & 0x7FFF0000L) )
					{
						// without modifiers, the command should be lower case...
						if( key >= 'A' && key <= 'Z' )
						{
							key += ('a'-'A');
						}
					}

					if(  addToAccelTable(key)
					        || (cmd->m_flags&SJA_MAINNUMPAD)
					        || (flags&SJA_ADDNOMODIFIERKEYS) )
					{
						if( !keysAdded.Lookup(key) )
						{
							entries[entriesCount++].Set((key>>16)&~wxACCEL_SYSTEMWIDE, key&0xFFFFL, cmdId);
							keysAdded.Insert(key, 1);
						}
					}
				}
			}
		}

		// add additional entries to the table
		for( cmdIndex = 0; cmdIndex < addEntriesCount; cmdIndex++ )
		{
			wxASSERT(addEntries);
			entries[entriesCount++].Set(addEntries[cmdIndex].GetFlags(), addEntries[cmdIndex].GetKeyCode(), addEntries[cmdIndex].GetCommand());
		}

		// done so far
		wxAcceleratorTable ret(entriesCount, entries);
		delete [] entries;
		return ret;
	}
	else
	{
		return wxAcceleratorTable();
	}
}


wxString SjAccelModule::GetReadableShortcutByKey(long modifier, long keycode)
{
	wxString ret;

	if( keycode )
	{
		if( (modifier & wxACCEL_CTRL) )
		{
			ret += wxT("Ctrl+");
		}

		if( (modifier & wxACCEL_ALT) )
		{
			ret += wxT("Alt+");
		}

		if( (modifier & wxACCEL_SHIFT) )
		{
			ret += wxT("Shift+");
		}

		wxString keyStr;

		if( keycode >= 33 && keycode <= 126 ) //if( (keycode>='A' && keycode<='Z') || (keycode>='0' && keycode<='9') )
		{
			keyStr.Printf(wxT("%c"), (wxChar)keycode);
		}
		else if( keycode>=WXK_F1 && keycode<=WXK_F24 )
		{
			keyStr.Printf(wxT("F%i"), (int)keycode-WXK_F1+1);
		}
		else if( keycode>=WXK_NUMPAD0 && keycode<=WXK_NUMPAD9 )
		{
			keyStr.Printf(_("Num%s"), wxString::Format(wxT("%i"), (int)keycode-WXK_NUMPAD0).c_str());
		}
		else
		{
			switch( keycode )
			{
				case WXK_BACK:              keyStr = _("Backspace");                    break;
				case WXK_TAB:               keyStr = _("Tab");                          break;
				case WXK_RETURN:            keyStr = _("Enter");                        break;
				case WXK_ESCAPE:            keyStr = _("Esc");                          break;
				case WXK_SPACE:             keyStr = _("Space");                        break;
				case WXK_DELETE:            keyStr = _("Del");                          break;
				case WXK_CANCEL:            // = CTRL+Pause
				case WXK_PAUSE:             keyStr = _("Pause");                        break;
				case WXK_PAGEUP:            keyStr = _("Page up");                      break;
				case WXK_PAGEDOWN:          keyStr = _("Page down");                    break;
				case WXK_END:               keyStr = _("End");                          break;
				case WXK_HOME:              keyStr = _("Home");                         break;
				case WXK_UP:                keyStr = _("Up");                           break;
				case WXK_LEFT:              keyStr = _("Left");                         break;
				case WXK_RIGHT:             keyStr = _("Right");                        break;
				case WXK_DOWN:              keyStr = _("Down");                         break;
				case WXK_PRINT:             keyStr = _("Print");                        break;
				case WXK_INSERT:            keyStr = _("Ins");                          break;
				case WXK_HELP:              keyStr = _("Help");                         break;
				case WXK_NUMPAD_DIVIDE:     keyStr.Printf(_("Num%s"), wxT("/"));        break;
				case WXK_DIVIDE:
				case wxT('/'):              keyStr = wxT("/");                          break;
				case WXK_NUMPAD_MULTIPLY:   keyStr.Printf(_("Num%s"), wxT("*"));        break;
				case WXK_MULTIPLY:
				case wxT('*'):              keyStr = wxT("*");                          break;
				case WXK_NUMPAD_SUBTRACT:   keyStr.Printf(_("Num%s"), wxT("-"));        break;
				case WXK_SUBTRACT:
				case wxT('-'):              keyStr = wxT("-");                          break;
				case WXK_NUMPAD_ADD:        keyStr.Printf(_("Num%s"), wxT("+"));        break;
				case WXK_ADD:
				case wxT('+'):              keyStr = wxT("+");                          break;
				case WXK_NUMPAD_DECIMAL:    keyStr.Printf(_("Num%s"), wxT("."));        break;
				case WXK_DECIMAL:
				case wxT('.'):              keyStr = wxT(".");                          break;
				default:                    keyStr.Printf(wxT("#%x"), (int)keycode);    break;
			}
		}

		ret += keyStr;
	}

	return ret;
}


wxString SjAccelModule::GetReadableShortcutsByPtr(const long userKey[], int userKeyCount, long flags) const
{
	wxString    ret;
	long        key, modifier;

	int keyIndex;
	for( keyIndex = 0; keyIndex < userKeyCount; keyIndex++ )
	{
		key = userKey[keyIndex];
		modifier = key >> 16;

		if( ( (modifier&wxACCEL_SYSTEMWIDE) && (flags&SJ_SHORTCUTS_SYSTEMWIDE))
		        || (!(modifier&wxACCEL_SYSTEMWIDE) && (flags&SJ_SHORTCUTS_LOCAL)) )
		{
			if( !ret.IsEmpty() ) ret += wxT(", ");
			ret += GetReadableShortcutByComprKey(key);

			if( !(flags&SJ_SHORTCUTS_ADDALL) ) break; // done
		}
	}

	return ret;
}


wxString SjAccelModule::GetCmdNameByIndex(int cmdIndex, bool shortName)
{
	wxASSERT(cmdIndex>=0&&cmdIndex<m_cmdCount);
	SjAccelCmd& cmd = m_cmd[cmdIndex];

	wxString ret = cmd.m_name;

	wxASSERT( IDT_WORKSPACE_LINE_LEFT   == IDT_WORKSPACE_LINE_LEFT+0 );
	wxASSERT( IDT_WORKSPACE_LINE_RIGHT  == IDT_WORKSPACE_LINE_LEFT+1 );
	wxASSERT( IDT_WORKSPACE_LINE_UP     == IDT_WORKSPACE_LINE_LEFT+2 );
	wxASSERT( IDT_WORKSPACE_LINE_DOWN   == IDT_WORKSPACE_LINE_LEFT+3 );

	wxASSERT( IDT_WORKSPACE_PAGE_LEFT   == IDT_WORKSPACE_PAGE_LEFT+0 );
	wxASSERT( IDT_WORKSPACE_PAGE_RIGHT  == IDT_WORKSPACE_PAGE_LEFT+1 );
	wxASSERT( IDT_WORKSPACE_PAGE_UP     == IDT_WORKSPACE_PAGE_LEFT+2 );
	wxASSERT( IDT_WORKSPACE_PAGE_DOWN   == IDT_WORKSPACE_PAGE_LEFT+3 );
	wxASSERT( IDT_WORKSPACE_INSERT      == IDT_WORKSPACE_PAGE_LEFT+4 );
	wxASSERT( IDT_WORKSPACE_DELETE      == IDT_WORKSPACE_PAGE_LEFT+5 );
	wxASSERT( IDT_WORKSPACE_KEY_LEFT    == IDT_WORKSPACE_PAGE_LEFT+6 );
	wxASSERT( IDT_WORKSPACE_KEY_RIGHT   == IDT_WORKSPACE_PAGE_LEFT+7 );
	wxASSERT( IDT_WORKSPACE_KEY_UP      == IDT_WORKSPACE_PAGE_LEFT+8 );
	wxASSERT( IDT_WORKSPACE_KEY_DOWN    == IDT_WORKSPACE_PAGE_LEFT+9 );
	wxASSERT( IDT_WORKSPACE_MINOR_HOME  == IDT_WORKSPACE_PAGE_LEFT+10 );
	wxASSERT( IDT_WORKSPACE_MINOR_END   == IDT_WORKSPACE_PAGE_LEFT+11 );
	wxASSERT( IDT_WORKSPACE_ALBUM_VIEW  == IDT_WORKSPACE_PAGE_LEFT+12 );
	wxASSERT( IDT_WORKSPACE_COVER_VIEW  == IDT_WORKSPACE_PAGE_LEFT+13 );
	wxASSERT( IDT_WORKSPACE_LIST_VIEW   == IDT_WORKSPACE_PAGE_LEFT+14 );
	wxASSERT( IDT_WORKSPACE_TOGGLE_VIEW == IDT_WORKSPACE_PAGE_LEFT+15 );
	wxASSERT( IDT_WORKSPACE_ENTER       == IDT_WORKSPACE_PAGE_LEFT+16 );

	if( !shortName )
	{
		if( (cmd.m_id >= IDT_WORKSPACE_LINE_LEFT && cmd.m_id <= IDT_WORKSPACE_LINE_DOWN)
		        || (cmd.m_id >= IDT_WORKSPACE_PAGE_LEFT && cmd.m_id <= IDT_WORKSPACE_ENTER)
		        ||  cmd.m_id == IDT_WORKSPACE_HOME
		        ||  cmd.m_id == IDT_WORKSPACE_END
		        ||  cmd.m_id == IDT_WORKSPACE_SHOW_COVERS )
		{
			ret = SjLittleOption::ApplySection(_("Workspace"), ret);
		}
		else if( cmd.m_id == IDO_SMOOTH /*is a global command, but shown at the cover editor options*/ )
		{
			ret = SjLittleOption::ApplySection(_("Cover editor"), ret);
		}
		else if( !(cmd.m_flags&SJA_MAIN) )
		{
			     if( cmd.m_flags&SJA_ART )          { ret = SjLittleOption::ApplySection(_("Cover editor"), ret); }
			else if( cmd.m_flags&SJA_EDIT )         { ret = SjLittleOption::ApplySection(_("Edit tracks/Get info"), ret); }
			else if( cmd.m_flags&SJA_ADVSEARCH )    { ret = SjLittleOption::ApplySection(_("Music selection"), ret); }
			else if( cmd.m_flags&SJA_MAINNUMPAD )   { ret = SjLittleOption::ApplySection(_("Numpad"), ret); }
			else                                    { ret = SjLittleOption::ApplySection(wxT("?"), ret); }
		}
	}

	return ret;
}


int SjAccelModule::KeyEvent2CmdIndex(wxKeyEvent& event, long flags)
{
	// add numpad keys?
	if( (flags & SJA_MAIN)
	        &&  UseNumpad() )
	{
		flags |= SJA_MAINNUMPAD;
	}

	// get normalized key code
	long key = event.GetKeyCode();

	if( key == WXK_NUMPAD_ADD      || key == '+' )  key = WXK_ADD;
	if( key == WXK_NUMPAD_SUBTRACT || key == '-' )  key = WXK_SUBTRACT;
	if( key == WXK_NUMPAD_MULTIPLY || key == '*' )  key = WXK_MULTIPLY;

	// add accelerators to key code
	if( event.AltDown() ) key |= wxACCEL_ALT<<16;
	if( event.ControlDown() ) key |= wxACCEL_CTRL<<16;
	if( event.ShiftDown() ) key |= wxACCEL_SHIFT<<16;

	// don't return the command if the key is handled by the accelerator
	// (normally we should never receive such keys; however wxWidgets returns
	// them in some cases and we will handle the command twice without this check)
	if( !(flags&SJA_NOACCELTABLE) && addToAccelTable(key) )
	{
		return 0;
	}

	// search key - we have to go through the whole field as the keys
	// may be used for different commands in different scopes
	long cmdIndex = 0;
	if( m_keyPresent.Lookup(key) )
	{
		long c, keyIndex;

		for( c = 1/*first is void*/; c < m_cmdCount; c++ )
		{
			SjAccelCmd& cmd = m_cmd[c];

			for( keyIndex = 0; keyIndex < cmd.m_userKeyCount && cmdIndex == 0; keyIndex++ )
			{
				if( (cmd.m_userKey[keyIndex]&(~(wxACCEL_SYSTEMWIDE<<16))) == key
				        && (cmd.m_flags & flags) )
				{
					cmdIndex = c;
					break;
				}
			}

			if( cmdIndex )
			{
				break;
			}
		}
	}

	if( cmdIndex == 0 )
	{
		return 0;
	}

	if( m_cmd[cmdIndex].m_linkId )
	{
		cmdIndex = CmdId2CmdIndex(m_cmd[cmdIndex].m_linkId);
	}

	return cmdIndex;
}


void SjAccelModule::BuildHashes()
{
	long cmdIndex, keyIndex;

	m_totalUserKeyCount = 0;
	m_keyPresent.Clear();

	for( cmdIndex = 1/*first is void*/; cmdIndex < m_cmdCount; cmdIndex++ )
	{
		SjAccelCmd& cmd = m_cmd[cmdIndex];
		for( keyIndex = 0; keyIndex < cmd.m_userKeyCount; keyIndex++ )
		{
			m_totalUserKeyCount++;
			m_keyPresent.Insert(cmd.m_userKey[keyIndex]&(~(wxACCEL_SYSTEMWIDE<<16)), 1);
		}
	}
}


/*******************************************************************************
 * SjMenu
 ******************************************************************************/


SjMenu::SjMenu(long showShortcuts)
	: wxMenu()
{
	memset(m_underscores, 0, 256*sizeof(bool));
	m_showShortcuts = showShortcuts;
}


wxMenuItem* SjMenu::CreateMenuItem(int id, const wxString& text__, wxItemKind kind, wxMenu* subMenu,
                                   SjIcon icon, bool useDefaultIcon)
{
	// get information about this menu item from the accelerator table
	wxString text(text__);
	long key = 0;
	long cmdIndex = g_accelModule->CmdId2CmdIndex(id);
	if( cmdIndex )
	{
		SjAccelCmd* cmd = &g_accelModule->m_cmd[cmdIndex];
		if( icon == SJ_ICON_EMPTY && useDefaultIcon )
		{
			icon = cmd->m_icon;
		}

		if( text.IsEmpty() )
		{
			text = cmd->m_name;
			if( cmd->m_flags & SJA_3P )
			{
				text += wxT("...");
			}
		}

		if( cmd->m_linkId )
		{
			cmdIndex = g_accelModule->CmdId2CmdIndex(cmd->m_linkId);
			wxASSERT(cmdIndex);
			cmd = &g_accelModule->m_cmd[cmdIndex]; // leave bitmap and text "as is"
		}

		if( cmd->m_userKeyCount )
		{
			if( m_showShortcuts & SJ_SHORTCUTS_SYSTEMWIDE )
			{
				int keyIndex;
				for( keyIndex=0; keyIndex<cmd->m_userKeyCount; keyIndex++ )
				{
					if( cmd->m_userKey[keyIndex] & (wxACCEL_SYSTEMWIDE<<16) )
					{
						key = cmd->m_userKey[keyIndex];
						break;
					}
				}
			}
			else
			{
				key = cmd->m_userKey[0];
			}
		}
	}

	// add accelerator to the text ("Open..." becomes "&Open..." etc.)
	{
		wxString        temp(text); // needed as we overwrite "text"
		const wxChar*   s = temp.c_str();
		wxChar*         d = text.GetWriteBuf((text.Len()*2+2)*sizeof(wxChar)/*worst case, string has only "&&&&&" */);
		wxChar          c;
		bool            accelAdded = false;
		while( *s )
		{
			if( !accelAdded )
			{
				// get current char if a-z
				c = 0;
				if( *s >= wxT('A') && *s <= wxT('Z') )
				{
					c = *s;
				}
				else if( *s >= wxT('a') && *s <= wxT('z') )
				{
					c = *s - (wxT('a')-wxT('A'));
				}

				// character a-z found?
				if( c )
				{
					if( !m_underscores[(int)c] )
					{
						// add accelerator
						m_underscores[(int)c] = true;
						accelAdded = true;
						*d = wxT('&');
						d++;
					}
				}
			}

			// mask & in source string
			if( *s == wxT('&') )
			{
				*d = wxT('&');
				d++;
			}

			// add character
			*d = *s;

			// next
			s++;
			d++;
		}
		*d = 0;
		text.UngetWriteBuf();
	}

	// add shortcut to the text ("&Open..." becomes "&Open...\tCtrl+O")
	if( m_showShortcuts )
	{
		wxString shortcut = g_accelModule->GetReadableShortcutByComprKey(key);
		if( !shortcut.IsEmpty() )
		{
			text << wxT("\t") << shortcut;
		}
	}

	// create menu item
	wxMenuItem* mi = new wxMenuItem(this, id, text, wxT(""), kind==wxITEM_RADIO? wxITEM_CHECK : kind, subMenu);

	// set font (wxMenuItem::SetFont() is not available in GTK)
    #if !defined(__WXGTK__)
	if( g_accelModule->m_flags&SJ_ACCEL_USE_VIEW_FONT_IN_DLG )
	{
		mi->SetFont(g_mainFrame->m_baseStdFont);
	}
    #endif

	// done
	return mi;
}


void SjMenu::SetLabel(int id, const wxString& newLabel__)
{
	wxMenuItem* mi = FindItem(id);
	if( mi )
	{
		wxString newLabel(SjTools::Menuencode(newLabel__));

		wxString oldShortcut = GetLabel(id).AfterFirst('\t');
		if( !oldShortcut.IsEmpty() )
		{
			newLabel += wxT("\t") + oldShortcut;
		}

		wxMenu::SetLabel(id, newLabel);
	}
}


void SjMenu::Enable(int id, bool enable)
{
	wxMenuItem* mi = FindItem(id);
	if( mi )
	{
		mi->Enable(enable);
	}
}


void SjMenu::Check(int id, bool check)
{
	if( FindItem(id) )
	{
		wxMenu::Check(id, check);
	}
}


void SjMenu::Clear()
{
	while( 1 )
	{
		wxMenuItemList& list = GetMenuItems();
		if( list.GetCount() <= 0 )
		{
			break;
		}
		Destroy(list.GetFirst()->GetData());
	}
}


/*******************************************************************************
 * Message box that may be switched off
 ******************************************************************************/


int SjAccelModule::YesNo    (const wxString& message,
                             const wxString& yesTitle,
                             wxWindow *parent,
                             long flagToCheck)
{
	// if "flagToCheck" is NOT set in the "flagField", the message box is not shown and
	// simply wxYES is returned.
	if( !(m_flags & flagToCheck) )
	{
		return wxYES;
	}

	// get the title of the option, that may be switched off
	wxArrayString options;
	switch( flagToCheck )
	{
		case SJ_ACCEL_ASK_ON_MULTI_ENQUEUE:    options.Add(_("Ask before enqueuing multiple tracks")); break;
		case SJ_ACCEL_ASK_ON_CLEAR_PLAYLIST:   options.Add(_("Ask before clearing the playlist"));     break;
		case SJ_ACCEL_ASK_ON_CLOSE_IF_PLAYING: options.Add(_("Ask on close if playing"));              break;
		case SJ_ACCEL_ASK_ON_RATING_CHANGE:    options.Add(_("Ask before changing the rating"));       break;
	}

	// show the message box
	int selOption = 1;
	int userChoice = SjMessageBox(message, SJ_PROGRAM_NAME,
	                              wxYES_NO|wxICON_QUESTION,
	                              parent,
	                              &options, &selOption,
	                              yesTitle, _("Cancel") );

	if( selOption == 0 )
	{
		m_flags &= ~flagToCheck;
		g_tools->m_config->Write(wxT("main/accelFlags"), m_flags);
	}

	return userChoice;
}

