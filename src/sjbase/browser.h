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
 * File:    browser.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke main browser
 *
 ******************************************************************************/


#ifndef __SJ_BROWSER_H__
#define __SJ_BROWSER_H__


class SjMainFrame;
class SjBrowserWindow;


enum SjBrowserAction
{
    SJ_ACTION_NONE
    ,SJ_ACTION_DRAGSCROLL
    ,SJ_ACTION_DRAGNDROP
    ,SJ_ACTION_COLUMNWIDTH
    ,SJ_ACTION_COLUMNMOVE
};


#if SJ_USE_TOOLTIPS
class SjBrowserToolTipProvider : public SjToolTipProvider
{
public:
	                SjBrowserToolTipProvider () { }
	wxString        GetText             (long& flags);
	wxWindow*       GetWindow           () { return (wxWindow*)m_browserWindow; }
	wxRect          GetLocalRect        ();

private:
	SjBrowserWindow* m_browserWindow;
	long            m_xPos, m_yPos;
	friend class    SjBrowserWindow;
};
#endif


class SjBrowserBase;
class SjListBrowser;


class SjBrowserWindow : public wxWindow
{
public:

	// Constructor and destructor
	                SjBrowserWindow     (SjMainFrame* parent);
	                ~SjBrowserWindow    ();

	// Exit() is be called sooner than the destructor is called for save-freeing pointers.
	void            Exit                ();

	// Reload/redraw (parts) of the browser
	void            RefreshSelection    ();                         // fast
	void            RefreshAll          ();                         // ...
	void            ReloadColumnMixer   (bool keepColIndex=true);   // slow

	// Call these functions for
	void            OnModuleUserId      (int id);
	bool            IsVScrollAbottom    ();

	// Are covers shown?
	bool            AreCoversShown      () const;

	// handling target events
	void            OnSkinTargetEvent   (int targetId, SjSkinValue& value, long accelFlags);

	// Goto specific columns, GotoPos() may only be used for positions
	// returned by GetFirstVisiblePos() or GetFirstSelectedOrVisiblePos()
	bool            GotoUrl             (const wxString& url);
	void            GotoPos             (const wxString& guid, long viewOffset = 0);
	wxString        GetFirstVisiblePos  ();
	wxString        GetFirstSelectedOrVisiblePos(long& retViewOffset);

	// Change Selection
	#define         SJ_SEL_UP           1
	#define         SJ_SEL_DOWN         2
	#define         SJ_SEL_LEFT         3
	#define         SJ_SEL_RIGHT        4
	#define         SJ_SEL_PREV         5 // SJ_SEL_UP or SJ_SEL_LEFT if "up" is impossible
	#define         SJ_SEL_NEXT         6 // SJ_SEL_DOWN or SJ_SEL_RIGHT if "down" is impossible
	bool            ChangeSelection     (long dir, bool shiftSelection=FALSE);

	// Dropping images onto the browser
	// (the coordinated may be outside the browser window)
	void            DropImage           (SjDataObject*, int mouseX, int mouseY);

	// switching the views
	#define         SJ_BROWSER_ALBUM_VIEW   0L
	#define         SJ_BROWSER_COVER_VIEW   1L
	#define         SJ_BROWSER_LIST_VIEW    2L
	#define         SJ_BROWSER_VIEW_COUNT   3L
	int             GetView             () const;
	void            SetView_            (int, bool keepPosition, bool redraw);
	bool            IsViewAvailable     (int);
	SjListBrowser*  GetListBrowser      () const {return (SjListBrowser*)m_views[SJ_BROWSER_LIST_VIEW];}

	void            SetZoom_            (int targetIdOrAbs);

private:
	// events
	void            OnPaint             (wxPaintEvent&);
	void            OnEraseBackground   (wxEraseEvent&) {}
	void            OnSize              (wxSizeEvent&);
	void            OnImageThere        (SjImageThereEvent&);
	void            OnMouseLeftDown     (wxMouseEvent&);
	void            OnMouseLeftUp       (wxMouseEvent&);
	void            OnMouseCaptureLost  (wxMouseCaptureLostEvent&);
	void            OnMouseLeftDClick   (wxMouseEvent&);
	void            OnMouseMotion       (wxMouseEvent&);
	void            OnMouseEnter        (wxMouseEvent&);
	void            OnMouseWheel        (wxMouseEvent&);
	void            OnMouseRight        (wxMouseEvent&);
	void            OnMouseMiddleUp     (wxMouseEvent&);
	void            OnMouseSelect       (wxMouseEvent&);
	void            OnKeyDown           (wxKeyEvent&);

	// misc
	long            m_clientW, m_clientH;

	#define         DRAGSCROLL_DELTA 8
	SjBrowserAction m_mouseAction;
	wxArrayString   m_dragUrls;

	#if SJ_USE_TOOLTIPS
	SjBrowserToolTipProvider
	m_toolTipProvider;
	#endif

	SjBrowserBase*  m_views[SJ_BROWSER_VIEW_COUNT];
	SjBrowserBase*  m_currView;

	int             m_skipContextMenuEvent; // set if the right mouse button was used eg. for
	// scrolling by the mouse wheel to avoid appearing
	// the context menu on release
	DECLARE_EVENT_TABLE ()

	// tools for our friends
	SjImgThreadObj* RequireImage(const wxString& url, long w=0);
	void            PaintCover(wxDC&, SjImgThreadObj*, long x, long y, long w=0);
	void            GetFonts(int roughtType, wxFont** font1, wxFont** font2, bool isEnqueued);
	void            GetFontPxSizes(wxDC& dc,
	                               int& fontVDiff, // this is the difference in height between the small and the normal font
	                               int& fontSpace, // this is the space between the left/mid/right columns
	                               int& fontStdHeight);
	#if 0
	wxDragImage*    GetCoverDragNDropBitmap(SjCol*, wxRect& retSize);
	#endif
	bool            m_cursorChanged;
	void            ResetCursor();
	void            DrawUpText(wxDC& dc, const wxString& textup, long x, long y, long w, long h);

	friend class    SjBrowserToolTipProvider;
	friend class    SjAlbumBrowser;
	friend class    SjListBrowser;
	friend class    SjCoverBrowser;
};


class SjBrowserBase
{
public:
	// this is a pure-virtual base class defining a browser view
	// currently implemented classes are SjAlbumBrowser, SjCoverBrowser and SjListBrowser
	                SjBrowserBase       (SjBrowserWindow* b) { m_window = b; __needsColumnMixerReload = true; }
	virtual         ~SjBrowserBase      () { }

	// Exit() is be called sooner than the destructor is called for save-freeing pointers.
	virtual void    Exit                () = 0;

	// called to show the browser, after this function is called
	// the derived class should be ready to process paint events!
	virtual void    Realize             (bool reloadColumnMixer, bool keepColIndex) = 0;

	// mouse handling
	virtual void    OnMouseLeftDown     (wxMouseEvent& event) = 0;
	virtual void    OnMouseLeftUp       (wxMouseEvent& event) = 0;
	virtual void    OnMouseCaptureLost  (wxMouseCaptureLostEvent&) = 0;
	virtual bool    OnMouseLeftDClick   (wxMouseEvent& event) = 0; // return true if the click was used
	virtual void    OnMouseMiddleUp     (wxMouseEvent& event) = 0;
	virtual void    OnMouseMotion       (wxMouseEvent& event) = 0;
	virtual void    OnMouseWheel        (wxMouseEvent& event, bool scrollVert) = 0;
	virtual void    OnDropImage         (SjDataObject* data, int mouseX, int mouseY) = 0;

	virtual void    OnContextMenu       (wxMouseEvent& event) = 0;
	virtual void    OnContextMenuSelect (int id) = 0;

	virtual wxRect   GetToolTipRect     (int mouseX, int mouseY) = 0;
	virtual wxString GetToolTipText     (int mouseX, int mouseY, long &flags) = 0;

	// selection handling & scrolling
	virtual bool    OnSkinTargetEvent   (int targetId, SjSkinValue& value, long accelFlags) = 0; // return true if the key/event was used
	virtual bool    DoChangeSelection   (long dir, bool shiftSelection) = 0;
	virtual void    RefreshSelection    () = 0;
	virtual bool    GotoUrl             (const wxString& url) = 0;
	virtual void    GotoPos             (const wxString& guid, long viewOffset) = 0;
	virtual wxString GetFirstVisiblePos () = 0;
	virtual wxString GetFirstSelectedOrVisiblePos(long& retViewOffset) = 0;

	// do paint the window, paint-related events
	virtual void    DoPaint             (wxDC&) = 0;
	virtual void    OnSize              (wxSizeEvent& event) = 0;
	virtual void    OnImageThere        (SjImageThereEvent& event) = 0;

	// used by SjBrowserWindow, should not be used by classes derived from SjBrowserBase
	bool __needsColumnMixerReload;

	// the following flags are mainly for SjBrowserBase derived classes
	// which can check these values and decide eg. if covers should be shown or not
	#define         SJ_BROWSER_VIEW_COVER           0x00000001L
	#define         SJ_BROWSER_VIEW_DEFAULT_FLAGS   0x0000FFFFL
	long            m_flags;
	long            m_zoom;

protected:
	// stuff that may be used by derived classes
	SjBrowserWindow* m_window;
};


#endif // __SJ_BROWSER_H__

