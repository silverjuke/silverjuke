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
 * File:    browser_cover.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke cover browser
 *
 ******************************************************************************/



#ifndef __SJ_BROWSER_COVER_H__
#define __SJ_BROWSER_COVER_H__



class SjCoverBrowser : public SjBrowserBase
{
public:
	// this is a pure-virtual base class defining a browser view
	                SjCoverBrowser      (SjBrowserWindow*);

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
	bool            OnSkinTargetEvent   (int targetId, SjSkinValue& value, long accelFlags); // return true if the key/the event was used
	bool            DoChangeSelection   (long dir, bool shiftSelection);
	void            RefreshSelection    () { m_window->Refresh(); }
	bool            GotoUrl             (const wxString& url);
	void            GotoPos             (const wxString& guid, long viewOffset);
	wxString        GetFirstVisiblePos  ();
	wxString        GetFirstSelectedOrVisiblePos(long& retViewOffset);

	// do paint the window, paint-related events
	void            DoPaint             (wxDC&);
	void            OnSize              (wxSizeEvent& event);
	void            OnImageThere        (SjImageThereEvent& event);

private:
	// positions
	void            CalcPositions       ();
	long            m_coverXSpace;
	long            m_coversXCount, m_coversYCount;
	SjCol*          FindFirstSelectedCover(int* index=NULL);

	// loaded data
	void            FreeAllocatedCol    ();
	long            m_allocatedCoverCount;
	SjCol**         m_allocatedCover;

	// scrolling
	bool            OnVScroll           (int nScrollCode, int nPos, bool redraw); // returns true if sth. was changed
	void            SetVScrollInfo      ();
	bool            GotoCover           (long offset, bool selectOneInRow=false);

	// misc
	void            SelectAllRows       (SjCol*) const;
	long            m_applCoverCount;       // the real number of colums (covers for us) available by SjColumnMixer::GetMaskedCol()

	void            CalcRowStuff();
	long            m_applRowIndex, m_applRowCount;
	int             m_fontHeight, m_fontVDiff;

	long            m_scrollY;              // always positive and always smaller than g_mainFrame->m_currCoverWidth - g_mainFrame->m_currColumnXSpace

	#define         FOUND_NOTHING       0
	#define         FOUND_COVER         1
	#define         FOUND_COVER_ARROW   2
	long            FindCover           (long xPos, long yPos, SjCol** retCover, wxRect* retRect=NULL);

	void            ToggleView          ();

	long            m_coverNTitleHeight; // may be equal to g_mainFrame->m_currCoverHeight in the easiest case
	long            m_linesBelowCover;

	// mouse
	SjCol*          m_lastClickedCover;
	SjCol*          m_lastTooltipCover;
	long            m_dragStartX, m_dragStartY;
	long            m_dragscrollCurrY;
	bool            m_mouseSelectionOnDown;
	void            OnMouseSelect(wxMouseEvent& event);

	wxString        m_preservedVisible;
};



#endif // __SJ_BROWSER_COVER_H__
