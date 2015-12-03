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
 * File:    browser_album.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke album browser
 *
 ******************************************************************************/



#ifndef __SJ_BROWSER_ALBUM_H__
#define __SJ_BROWSER_ALBUM_H__



class SjAlbumBrowser : public SjBrowserBase
{
public:
	// this is a pure-virtual base class defining a browser view
	// currently implemented classes are SjAlbumBrowser and SjListBrowser
	                SjAlbumBrowser      (SjBrowserWindow*);

	// Exit() is be called sooner than the destructor is called for save-freeing pointers.
	void            Exit                ();

	// called to show the browser, after this function is called
	// the derived class should be ready to process paint events!
	void            Realize             (bool reloadColumnMixer, bool keepColIndex);

	// mouse handling
	void            OnMouseLeftDown     (wxMouseEvent& event);
	void            OnMouseLeftUp       (wxMouseEvent& event);
	void            OnMouseCaptureLost  (wxMouseCaptureLostEvent& event);
	bool            OnMouseLeftDClick   (wxMouseEvent& event);
	void            OnMouseMiddleUp     (wxMouseEvent& event);
	void            OnMouseMotion       (wxMouseEvent& event);
	void            OnMouseWheel        (wxMouseEvent& event, bool scrollVert);
	void            OnDropImage         (SjDataObject* data, int mouseX, int mouseY);

	void            OnContextMenu       (int clickX, int clickY);
	void            OnContextMenuSelect (int id);

	wxRect          GetToolTipRect      (int mouseX, int mouseY);
	wxString        GetToolTipText      (int mouseX, int mouseY, long &flags);

	void            AddItemsToColMenu   (SjMenu*);
	void            UpdateItemsInColMenu(SjMenu*);

	// selection handling & scrolling
	bool            OnSkinTargetEvent   (int targetId, SjSkinValue& value, long accelFlags); // return true if the key/event was used
	bool            DoChangeSelection   (long dir, bool shiftSelection);
	void            RefreshSelection    () { InvalidateNonCovers(); }
	bool            GotoUrl             (const wxString& url);
	void            GotoPos             (const wxString& guid, long viewOffset);
	wxString        GetFirstVisiblePos  ();
	wxString        GetFirstSelectedOrVisiblePos(long& retViewOffset);

	// do paint the window, paint-related events
	void            DoPaint             (wxDC&);
	void            OnSize              (wxSizeEvent& event);
	void            OnImageThere        (SjImageThereEvent& event);

private:
	// setting the scrollbars
	void            SetHScrollInfo      ();
	void            SetVScrollInfo      ();
	bool            OnHScroll           (int nScrollCode, int nPos, bool redraw); // returns TRUE if scrolling appears
	bool            OnVScroll           (int nScrollCode, int nPos, bool redraw); // returns TRUE if scrolling appears
	long            m_maxBottom;            // the max. y-index for the calculated columns
	long            m_scrollX;              // always positive and always smaller than bs->appl->view.currCoverW - bs->appl->view.currCoverXSpace
	long            m_scrollY;              // positive or negative

	// selecting
	#define         FOUND_NOTHING       0
	#define         FOUND_ROW           1
	#define         FOUND_COVER_ARROW   2
	long            FindRow             (long xPos, long yPos, SjCol** retCol, SjRow** retRow, wxRect* retRect=NULL);
	bool            FindSelectedRowInView (SjCol**, SjRow**, long* colIndex=NULL, long* rowIndex=NULL, bool findFromTop=TRUE);
	bool            FindBoundingRowInView (SjCol**, SjRow**, long* colIndex=NULL, long* rowIndex=NULL, bool findFromTop=TRUE);

	// painting
	wxBitmap        GetDragNDropBitmap  (int mouseX, int mouseY, wxPoint& retHotspot, wxRect& retSize);

	void            InvalidateNonCovers ();
	void            InvalidateCovers    ();

	void            PaintCol            (wxDC&, SjCol*, long rcPaintLeft, long rcPaintTop, long rcPaintRight, long rcPaintBottom);
	void            PaintRow            (wxDC&, SjCol*, SjRow*,
	                                     long drawRectLeft, long drawRectTop, long drawRectRight, long drawRectBottom,
	                                     bool odd, bool rowSelected, bool lastRowSelected);

	void            CalcRowPositions    (wxDC& dc, SjCol*, int x, int y);
	void            CalcPositions       ();
	int             m_fontVDiff;            // this is the difference in height between the
	// small and the normal font
	int             m_fontSpace;            // this is the space between the left/mid/right columns
	int             m_fontStdHeight;

	// mouse
	void            OnMouseSelect       (wxMouseEvent& event);
	SjCol*          m_lastClickedCol;
	SjRow*          m_lastClickedRow;
	long            m_dragStartX, m_dragStartY;
	long            m_dragscrollCurrX, m_dragscrollCurrY;
	bool            m_mouseSelectionOnDown;

	// misc
	long            m_applColCount;         // the real number of colums available by SjColumnMixer::GetMaskedCol()
	long            m_applColIndex;         // the SjColumnMixer index of the first column visible
	#define         MAX_COL_COUNT 64        // MAX_COL_COUNT is the max. number of columns visible the SAME time
	SjCol*          m_applCol[MAX_COL_COUNT];

	long            m_visibleColCount;      // the number of COMPLETELY visible columns
	long            m_allocatedColCount;    // the number of COMPLETELY OR PARTLY visible columns,
	// this is the number of valid objects in applCol[]
	bool            GotoColumn(long offset, bool selectOneInRow=false); // return true if the position was changed

	// for the tooltips
	SjCol*          m_lastTooltipCol;
	SjRow*          m_lastTooltipRow;
};


#endif // __SJ_BROWSER_ALBUM_H__
