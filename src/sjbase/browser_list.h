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
 * File:    browser_list.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke list browser
 *
 ******************************************************************************/



#ifndef __SJ_BROWSER_LIST_H__
#define __SJ_BROWSER_LIST_H__



class SjListTooltip;
class SjImageThereEvent;



class SjListBrowser : public SjBrowserBase
{
public:
	// this is a pure-virtual base class defining a browser view
	                SjListBrowser       (SjBrowserWindow*);

	// Exit() is be called sooner than the destructor is called for save-freeing pointers.
	void            Exit                ();

	// called to show the browser, after this function is called
	// the derived class should be ready to process paint events!
	void            Realize             (bool reloadColumnMixer, bool keepColIndex);

	// functions for externs
	void            SetOrder            (long sortField, bool desc);
	void            GetOrder            (long& sortField, bool& desc) const {sortField=m_sortField; desc=m_sortDesc;}
	void            SetColumns          (const wxArrayLong&);
	wxArrayLong     GetColumns          () const {return m_columns;}

	// mouse handling
	void            OnMouseLeftDown     (wxMouseEvent& event);
	void            OnMouseLeftUp       (wxMouseEvent& event);
	void            OnMouseCaptureLost  (wxMouseCaptureLostEvent& event);
	bool            OnMouseLeftDClick   (wxMouseEvent& event);
	void            OnMouseMiddleUp     (wxMouseEvent& event);
	void            OnMouseMotion       (wxMouseEvent& event);
	void            OnMouseWheel        (wxMouseEvent& event, bool scrollVert);
	void            OnDropImage         (SjDataObject* data, int mouseX, int mouseY);

	void            OnContextMenu       (wxMouseEvent& event);
	void            OnContextMenuSelect (int id);

	wxRect          GetToolTipRect      (int mouseX, int mouseY);
	wxString        GetToolTipText      (int mouseX, int mouseY, long &flags);
	bool            ResetSpecialMouseAction();

	// selection handling & scrolling
	bool            OnSkinTargetEvent   (int targetId, SjSkinValue& value, long accelFlags); // return true if the key/the event was used
	bool            DoChangeSelection   (long dir, bool shiftSelection);
	void            RefreshSelection    () { m_window->Refresh(false, &m_tracksRect); }
	bool            GotoUrl             (const wxString& url) { return GotoUrl__(url, 0, true/*select*/, true/*update*/); }
	void            GotoPos             (const wxString& pos, long viewOffset) { GotoUrl__(pos, viewOffset, false/*select*/, true/*update*/); }
	wxString        GetFirstVisiblePos  ();
	wxString        GetFirstSelectedOrVisiblePos(long& retViewOffset);
	wxString        GetFirstSelectedAndVisiblePos(long& retViewOffset);

	// do paint the window, paint-related events
	void            DoPaint             (wxDC&);
	void            OnSize              (wxSizeEvent& event);
	void            OnImageThere        (SjImageThereEvent& event);

private:
	bool            GotoUrl__           (const wxString& pos, long viewOffset, bool select, bool update, bool hscroll=true);
	bool            GotoOffset__        (long offset, long viewOffset, bool select, bool update, bool hscroll=true);

	wxArrayLong     m_columns; // values from SJ_TI_*
	wxArrayLong     m_columnWidths;

	SjLLHash        m_columnBackupWidths;
	void            BackupWidths();
	long            GetBackupWidth(long ti);

	SjListView*     m_listView;
	long            m_visibleTrackCount;
	long            m_scrollPos;

	wxRect          m_uptextRect;
	wxRect          m_toggleCoversRect;
	wxRect          m_coversRect;
	wxRect          m_playmarkRect; // left of the header/tracks
	wxRect          m_headerRect;
	wxRect          m_tracksRect;
	long            m_tracksVirtualW;
	long            m_tracksHScroll;
	long            m_selAnchor;

	int             m_fontHeight, m_fontVDiff;
	int             m_coverW;
	long            m_sortField; // value from SJ_TI_*
	bool            m_sortDesc;
	void            DoPaintHeaderNTracks(wxDC& dc, long x, long y, long w, long h, bool addTooltips, wxArrayLong* retLinePos=NULL, wxArrayLong* retAlbumIds=NULL);
	void            DoPaintHeader(wxDC& dc, long x, long y, long w, long h, bool addTooltips);
	void            DoPaintTrack(wxDC& dc, long x, long y, long w, long h, long offset, bool& lastTrackSelected, bool addTooltips, bool setAz, int* lastAz, wxArrayLong* retLinePos=NULL, wxArrayLong* retAlbumIds=NULL);
	void            CalculateFontHeight();
	void            CalculatePositions(bool calculateFontHeight=true);
	void            OnHScroll(int nScrollCode, int nPos, bool redraw);
	void            SetHScrollInfo();

	void            OnVScroll(int nScrollCode, int nPos, bool redraw);
	void            SetVScrollInfo();
	bool            MouseInHeader(int x, int y, long& colIndex, bool& inSizer, wxArrayLong* columns=NULL, wxArrayLong* widths=NULL);
	bool            MouseInTrack(int x, int y, long& offset);
	bool            MouseInCover(int x, int y, SjCol** cover);
	void            OnMouseSelect(wxMouseEvent& event);

	long            m_dragStartX, m_dragStartY, m_columnDragIndex, m_columnDragOrgWidth;
	bool            m_mouseSelectionOnDown;
	long            GetFirstSelectedAndVisible(); // -1 if none
	long            GetLastSelectedAndVisible(bool alsoUseGaps=true); // -1 if none
	void            SelectRange(long i1, long i2);
	void            ScrollOneLineInView(long index); // scroll max. one line up/down to put the index in view
	long            m_lastContextMenuTrackIndex, m_lastContextMenuColIndex;
	void            InitCols();
	void            LoadConfig();
	void            SaveConfig();
	wxBitmap        GetDragNDropBitmap(int mouseX, int mouseY, wxPoint& retHotspot, wxRect& retSize);
	long            m_dragScrollOrgHScroll, m_dragScrollOrgScrollPos;


	void            CalcRefreshNUpdateHeader()
	{
		CalculatePositions();
		m_window->Refresh(false, &m_headerRect);
		m_window->Update();
	}
	void            CalcRefreshNUpdateTracks(bool refreshCovers)
	{
		CalculatePositions();
		m_window->Refresh(false, &m_tracksRect);
		if( refreshCovers )
			m_window->Refresh(false, &m_coversRect);
		m_window->Update();
	}
	void            CalcRefreshNUpdateHeaderNTracks()
	{
		CalculatePositions();
		m_window->Refresh(false, &m_headerRect);
		m_window->Refresh(false, &m_tracksRect);
		m_window->Update();
	}

	// tooltip helper
	wxArrayPtrVoid  m_tooltips;
	void            AddToTooltips(const wxRect&, const wxString&);
	void            ClearTooltips();
	SjListTooltip*  FindTooltip(int mouseX, int mouseY);

	// misc.
	long            m_tracksNeededToFitCoverHeight;

	// loaded data
	void            FreeAllocatedCover  ();
	wxArrayPtrVoid  m_allocatedCover;
	SjCol*          m_lastClickedCover;
};



#endif // __SJ_BROWSER_LIST_H__
