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
 * File:    vis_module.h
 * Authors: Björn Petersen
 * Purpose: Video, Karaoke, Vis. handling
 *
 ******************************************************************************/


#ifndef __SJ_VIS_MODULE_H__
#define __SJ_VIS_MODULE_H__


enum SjVisState
{
    SJ_VIS_EMBEDDED     = 0,
    SJ_VIS_FLOATING     = 1,
    SJ_VIS_OWNSCREEN    = 2
};


class SjVisModule : public SjCommonModule
{
public:
	                SjVisModule         (SjInterfaceBase*);

	// Open the vis. window; this does _not_ automatically start the vis.
	//
	// The vis. is opened as a fullscreen window on the given display, optionally
	// using only the given rect.
	//
	// If no display is given (fullscreenDisplay=-1), it is embedded to the
	// main window or is floating. Floating is only available in non-kiosk mode
	// and may be toggled by the user in the vis. window (menu or double-click)
	//
	// If the window is already opened, nothing happens.
	void            PrepareWindow       (int fullscreenDisplay, const wxRect* fullscreenVisRect);

	// Close any opened vis. window ans stops any running vis.
	//
	// If no vis. window is opened, nothing happens and false is returned.
	// If the window was really closed by this funcition, true is returned.
	void            UnprepareWindow     ();

	// Start vis. If the window is not opened on start, this is done implicitly.
	#define         SJ_SET_FROM_m_temp1str__    ((SjVisRendererModule*)1)
	#define         SJ_SET_LAST_SELECTED        ((SjVisRendererModule*)2)
	void            StartVis            ();

	// Stop vis. If the window was opened implicitly by StartVis(), this function will
	// also close the window.
	void            StopVis             ();

	// Simelar to StopVis(), but the vis. is only stopped if the vis. lays
	// (at least partly) over the workspace. If the window was opened implicitly by
	// StartVis(), this function will also close the window.
	void            StopVisIfOverWorkspace() { if( IsOverWorkspace() ) { StopVis(); }  }

	// IsVisStarted() should be used to set the state of the vis. icon or of
	// menu entries.
	bool            IsVisStarted        () const { return m_visIsStarted; }

	// More state
	bool            IsOverWorkspace     () const { return (m_visWindow!=NULL && m_visState==SJ_VIS_EMBEDDED && m_visIsOverWorkspace); }
	bool            IsEmbedded          () const { return (m_visState==SJ_VIS_EMBEDDED); }
	bool            IsCloseButtonNeeded () const;
	bool            IsCloseMenuEntryNeeded() const;
	bool            IsWindowPrepared    () const { return (m_visWindow!=NULL); }

	// used by the SjVisImpl class
	void            StopOrCloseRequest  ();
	void            AttachDetachRequest ();

	// some settings - real by all, write please only for SjVisImpl
	#define         SJ_VIS_FLAGS_EMBEDDED                   0x00000001L
	#define         SJ_VIS_FLAGS_SWITCH_OVER_AUTOMATICALLY  0x00000008L
	#define         SJ_VIS_FLAGS_HALF_SIZE                  0x00010000L
	#define         SJ_VIS_FLAGS_DEFAULT                    0x0000FFFFL
	long            m_visFlags;
	void            WriteVisFlags       ();

	// normally not needed beside for SjVisImpl
	void                    SetCurrRenderer         (SjVisRendererModule* m, bool justContinue=false);
	SjVisRendererModule*    GetCurrRenderer         () const;
	void                    SetModal                (bool set) { m_modal += set? 1 : -1; }

	// get the desired type of the action window regarding the currently playing track
	// needed for SJ_VIS_FLAGS_SWITCH_OVER_AUTOMATICALLY and for the blinking icon
	wxString        GetDesiredRenderer  ();

	// get the renderer REALLY to use next. this may also be the sfx or the oscillosope - which are never desired by GetDesiredRenderer()
	// if there is no need to change the renderer, the empty string is returned.
	wxString        GetRealNextRenderer (const wxString& desiredRenderer);

protected:
	// protected stuff
	bool            FirstLoad           ();
	void            LastUnload          ();
	void            ReceiveMsg          (int msg);

private:
	// private stuff
	bool            OpenWindow__        (int           fullscreenDisplay,
	                                     const wxRect* fullscreenVisRect);
	bool            CloseWindow__       ();

	bool            m_explicitlyPrepared;

	wxWindow*       m_visWindow;
	SjVisState      m_visState;
	bool            m_visIsOverWorkspace;

	// is started? the vis renderer may be NULL if the vis. is started
	// (either for some moments when the vis. changes or if the renderer initialisation fails)
	bool            m_visIsStarted;

	// internal state, set when processing attach/detach asynchon
	bool            m_inAttachDetach;

	// set if vis. module opens a modal dialog; avoids closing
	long            m_modal;

	// temp. needed for IDMODMSG_VIS_FWD_SWITCH_RENDER, can be referred easily by SJ_SET_FROM_TEMP1STR
	wxString        m_temp1str__;
};


extern SjVisModule* g_visModule;


#endif // __SJ_VIS_MODULE_H__
