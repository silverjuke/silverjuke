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
 * File:    browser_album.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke album browser
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/imgthread.h>
#include <sjbase/browser.h>
#include <sjbase/browser_album.h>
#include <sjbase/columnmixer.h>
#include <sjmodules/kiosk/virtkeybd.h>
#include <sjmodules/vis/vis_module.h>


SjAlbumBrowser::SjAlbumBrowser(SjBrowserWindow* w)
	: SjBrowserBase(w)
{
	memset(m_applCol, 0, MAX_COL_COUNT*sizeof(SjCol*));
	m_applColCount          = 0;
	m_applColIndex          = 0;
	m_visibleColCount       = -1;
	m_allocatedColCount     = 0;
	m_maxBottom             = 0;
	m_scrollX               = 0;
	m_scrollY               = 0;
	m_dragStartX            = 0;
	m_dragStartY            = 0;
	m_dragscrollCurrX       = 0;
	m_dragscrollCurrY       = 0;
	m_fontVDiff             = 0;
	m_fontSpace             = 0;
	m_lastClickedCol        = NULL;
	m_lastClickedRow        = NULL;
}


void SjAlbumBrowser::Exit()
{
	int i;
	for( i = 0; i < m_allocatedColCount; i++ )
	{
		if( m_applCol[i] )
		{
			if( m_applCol[i] == m_lastClickedCol )
			{
				m_lastClickedCol = NULL;
				m_lastClickedRow = NULL;
			}

			delete m_applCol[i];
		}
	}

	m_allocatedColCount = 0;
	m_visibleColCount = 0;
	m_applColCount = 0;
	m_applColIndex = 0;
}


void SjAlbumBrowser::Realize(bool reloadColumnMixer, bool keepColIndex)
{
	if( reloadColumnMixer )
	{
		m_scrollY = 0;
		m_scrollX = 0;

		m_applColCount = g_mainFrame->m_columnMixer.GetMaskedColCount();
	}

	if( m_applColIndex >= m_applColCount-m_visibleColCount )
	{
		m_applColIndex = m_applColCount-m_visibleColCount;
	}

	if( m_applColIndex < 0 || !keepColIndex )
	{
		m_applColIndex = 0;
	}

	CalcPositions();
	SetHScrollInfo();
	if( m_scrollY+m_window->m_clientH > m_maxBottom )
	{
		m_scrollY = m_maxBottom - m_window->m_clientH;
		if( m_scrollY < 0 )
		{
			m_scrollY = 0;
		}
	}
	SetVScrollInfo();
	m_lastTooltipCol        = NULL;
	m_lastTooltipRow        = NULL;
}


/*******************************************************************************
 * Mouse Handling
 ******************************************************************************/


void SjAlbumBrowser::OnMouseSelect(wxMouseEvent& event)
{
	bool multiEnqueueAvailable = g_mainFrame->IsOpAvailable(SJ_OP_MULTI_ENQUEUE);

	/* (de-)select the item under the mouse (if any)
	 */
	SjCol*  col;
	SjRow*  row;
	long found = FindRow(event.GetX(), event.GetY(), &col, &row);
	if( found == FOUND_ROW )
	{
		if( row->IsSelectable() )
		{
			if( multiEnqueueAvailable && event.ShiftDown() )
			{
				if( row->IsSelectable()==2 )
				{
					g_mainFrame->m_columnMixer.SelectShifted(row);
					RefreshSelection();
				}
			}
			else if( multiEnqueueAvailable && event.ControlDown() )
			{
				row->Select(!row->IsSelected());
				RefreshSelection();
			}
			else
			{
				g_mainFrame->m_columnMixer.SelectAll(false);
				row->Select(true);
				RefreshSelection();
			}
		}
	}
	else if( found == FOUND_COVER_ARROW )
	{
		g_mainFrame->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDT_WORKSPACE_SHOW_COVERS));
	}
	else if( !event.ShiftDown() && !event.ControlDown() && g_mainFrame->m_columnMixer.IsAnythingSelected() )
	{
		g_mainFrame->m_columnMixer.SelectAll(FALSE);
		RefreshSelection();
	}
}


void SjAlbumBrowser::OnMouseLeftDown(wxMouseEvent& event)
{
	/* prepare dragscroll and object dragging
	 */
	m_window->m_mouseAction = SJ_ACTION_NONE;
	m_dragStartX            = event.GetX(); /* client coordinates */
	m_dragStartY            = event.GetY(); /* client coordinates */

	/* perform selection
	 */
	m_mouseSelectionOnDown = FALSE;
	SjCol* col;
	SjRow* row;
	if(  g_accelModule->m_selDragNDrop == 1
	        &&  FindRow(m_dragStartX, m_dragStartY, &col, &row) == FOUND_ROW
	        &&  row->IsSelectable() == 2
	        && !row->IsSelected() )
	{
		OnMouseSelect(event);
		m_mouseSelectionOnDown = TRUE;
	}
}


void SjAlbumBrowser::OnMouseLeftUp(wxMouseEvent& event)
{
	// dragscroll: restore cursor if changed by dragscroll
	if( m_window->m_mouseAction == SJ_ACTION_DRAGSCROLL )
	{
		m_window->m_mouseAction = SJ_ACTION_NONE;
		return; // done
	}

	// nothing in view
	if( g_mainFrame->m_columnMixer.GetMaskedColCount() == 0 )
	{
		g_mainFrame->EndOneSearch();
		return; // done
	}

	// perform selection
	if( !m_mouseSelectionOnDown )
	{
		OnMouseSelect(event);
	}
}


void SjAlbumBrowser::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
	// dragscroll: restore cursor if changed by dragscroll
	if( m_window->m_mouseAction == SJ_ACTION_DRAGSCROLL )
	{
		m_window->m_mouseAction = SJ_ACTION_NONE;
		return; // done
	}
}


bool SjAlbumBrowser::OnMouseLeftDClick(wxMouseEvent& event)
{
	SjCol*  col;
	SjRow*  row;

	long found = FindRow(event.GetX(), event.GetY(), &col, &row);
	if( found == FOUND_ROW )
	{
		row->OnDoubleClick(event.ShiftDown() || event.ControlDown());
		return true; // click used
	}
	else if( found == FOUND_COVER_ARROW )
	{
		return true; // click used (avoid toggle the view by fast clicks on the cover toggler)
	}

	return false; // click not used
}


void SjAlbumBrowser::OnMouseMiddleUp(wxMouseEvent& event)
{
	SjCol*  col;
	SjRow*  row;
	bool    skip = TRUE;

	int xPos = event.GetX();
	int yPos = event.GetY();

	if( FindRow(xPos, yPos, &col, &row) == FOUND_ROW )
	{
		int usesMiddleClick = row->UsesMiddleClick();
		if( usesMiddleClick )
		{
			if( usesMiddleClick == 2 && row->IsSelectable() )
			{
				if( !row->IsSelected() )
				{
					g_mainFrame->m_columnMixer.SelectAll(FALSE);
					row->Select(TRUE);
					RefreshSelection();
				}
				skip = FALSE;
			}

			if( row->OnMiddleClick(event.ShiftDown() || event.ControlDown()) )
			{
				skip = FALSE;
			}
		}
	}

	if( skip )
	{
		event.Skip();
	}
}


void SjAlbumBrowser::OnMouseMotion(wxMouseEvent& event)
{
	if( !m_window->HasCapture() ) return;

	long    hDifference;
	long    vDifference;

	long    oldColIndex, oldScrollX, oldScrollY;

	long    xPos = event.GetX(); /* client coordinates */
	long    yPos = event.GetY(); /* client coordinates */

	// start dragscroll or object dragging?
	if( m_window->m_mouseAction == SJ_ACTION_NONE )
	{
		hDifference = xPos - m_dragStartX;
		vDifference = yPos - m_dragStartY;
		if( hDifference >  DRAGSCROLL_DELTA
		 || hDifference < -DRAGSCROLL_DELTA
		 || vDifference >  DRAGSCROLL_DELTA
		 || vDifference < -DRAGSCROLL_DELTA )
		{
			SjCol* col;
			SjRow* row;
			if( g_accelModule->m_selDragNDrop
			 && FindRow(m_dragStartX, m_dragStartY, &col, &row) == FOUND_ROW
			 && row->IsSelected() )
			{
				// do object dragging
				m_window->m_dragUrls.Clear();
				g_mainFrame->m_columnMixer.GetSelectedUrls(m_window->m_dragUrls);
				if( g_accelModule->m_flags&SJ_ACCEL_USE_DND_IMAGES )
				{
					g_mainFrame->m_dragImage = new wxDragImage(GetDragNDropBitmap(m_dragStartX, m_dragStartY, g_mainFrame->m_dragHotspot, g_mainFrame->m_dragRect));
				}

				if( g_mainFrame->DragNDrop(SJ_DND_ENTER, m_window, event.GetPosition(), NULL, &m_window->m_dragUrls) )
				{
					m_window->m_mouseAction = SJ_ACTION_DRAGNDROP;
				}
			}
			else if( g_accelModule->m_flags&SJ_ACCEL_CONTENT_DRAG
			      && m_applColCount )
			{
				// start dragscroll
				m_window->m_mouseAction = SJ_ACTION_DRAGSCROLL;
				m_dragscrollCurrX       = xPos;
				m_dragscrollCurrY       = yPos;
			}
		}
	}

	// in drag'n'drop?
	if( m_window->m_mouseAction == SJ_ACTION_DRAGNDROP )
	{
		if( !g_mainFrame->DragNDrop(SJ_DND_MOVE, m_window, event.GetPosition(), NULL, &m_window->m_dragUrls) )
		{
			m_window->m_mouseAction = SJ_ACTION_NONE;
		}
	}

	// in dragscroll?
	if( m_window->m_mouseAction == SJ_ACTION_DRAGSCROLL )
	{
		oldScrollX  = m_scrollX;
		oldScrollY  = m_scrollY;
		oldColIndex = m_applColIndex;

		// horizontal scrolling
		hDifference = xPos - m_dragscrollCurrX;
		m_dragscrollCurrX = xPos;
		if( hDifference )
		{
			if( hDifference > g_mainFrame->m_currColumnWidth ) {
				hDifference = g_mainFrame->m_currColumnWidth;
			}
			else if( hDifference < 0 - g_mainFrame->m_currColumnWidth ) {
				hDifference = 0 - g_mainFrame->m_currColumnWidth;
			}

			m_scrollX -= hDifference;

			if( m_scrollX < 0 )
			{
				// decrease application column position if possible
				if( m_applColIndex > 0 )
				{
					m_scrollX += g_mainFrame->m_currColumnWidth + g_mainFrame->m_currColumnXSpace;
					m_applColIndex--;
					CalcPositions();
					SetHScrollInfo();
					if( m_scrollY+m_window->m_clientH > m_maxBottom )
					{
						m_scrollY = m_maxBottom - m_window->m_clientH;
						if( m_scrollY < 0 )
						{
							m_scrollY = 0;
						}
					}
					SetVScrollInfo();
				}
				else
				{
					m_scrollX = 0;
				}
			}
			else if( m_scrollX >= g_mainFrame->m_currColumnWidth + g_mainFrame->m_currColumnXSpace )
			{
				// increase application column position if possible
				if( m_applColIndex < m_applColCount-m_visibleColCount )
				{
					m_scrollX -= g_mainFrame->m_currColumnWidth + g_mainFrame->m_currColumnXSpace;
					m_applColIndex++;
					CalcPositions();
					SetHScrollInfo();
					if( m_scrollY+m_window->m_clientH > m_maxBottom )
					{
						m_scrollY = m_maxBottom - m_window->m_clientH;
						if( m_scrollY < 0 )
						{
							m_scrollY = 0;
						}
					}
					SetVScrollInfo();
				}
				else
				{
					m_scrollX = g_mainFrame->m_currColumnWidth + g_mainFrame->m_currColumnXSpace;
				}
			}
			else if( m_allocatedColCount*(g_mainFrame->m_currColumnWidth+g_mainFrame->m_currColumnXSpace)-m_scrollX < m_window->m_clientW )
			{
				// a new column will fit on the right
				if( m_applColIndex+m_visibleColCount < m_applColCount )
				{
					CalcPositions();
					if( m_scrollY+m_window->m_clientH > m_maxBottom )
					{
						m_scrollY = m_maxBottom - m_window->m_clientH;
						if( m_scrollY < 0 )
						{
							m_scrollY = 0;
						}
					}
					SetVScrollInfo();
				}
			}
		}

		// vertical scrolling
		vDifference = yPos - m_dragscrollCurrY;
		m_dragscrollCurrY = yPos;
		if( vDifference )
		{
			if( !OnVScroll(IDT_WORKSPACE_V_SCROLL, m_scrollY + vDifference*-1, FALSE/*redraw*/) )
			{
				vDifference = 0;
			}
		}

		// update window
		if( hDifference || vDifference )
		{
			if( g_mainFrame->m_imgThread->HasWaitingImages() )
			{
				// if there are waiting images, invalidate the whole rectangle as
				// some areas are no yet okay
				m_window->Refresh();
			}
			else
			{
				hDifference =
				    (oldColIndex*(g_mainFrame->m_currColumnWidth+g_mainFrame->m_currColumnXSpace) + oldScrollX)
				    -   (m_applColIndex*(g_mainFrame->m_currColumnWidth+g_mainFrame->m_currColumnXSpace) + m_scrollX);

				vDifference = oldScrollY - m_scrollY;

				m_window->ScrollWindow(hDifference, vDifference, NULL);
				m_window->Update();
			}
		}
	}
}


void SjAlbumBrowser::OnMouseWheel(wxMouseEvent& event, bool scrollVert)
{
	long rotation = event.GetWheelRotation();

	if( rotation != 0 )
	{
		if( g_accelModule->m_flags&SJ_ACCEL_WHEEL_HORZ_IN_ALBUMVIEW )
		{
			scrollVert = !scrollVert;
		}

		if( scrollVert )
		{
			OnVScroll(IDT_WORKSPACE_V_SCROLL, m_scrollY + rotation*-1, TRUE/*redraw*/);
		}
		else
		{
			// add multiple small rotations (smaller than the delta to take action) to bigger ones
			static SjWheelHelper s_albumWheelHelper;
			long rotateCols, dir; s_albumWheelHelper.PushRotationNPopAction(event, rotateCols, dir);
			if( rotateCols != 0 )
			{
				if( g_accelModule->m_flags&SJ_ACCEL_WHEEL_HORZ_IN_ALBUMVIEW )
				{
					dir *= -1; // simulate the old behaviour of Silverjuke 2.x - not compatible with modern scroll wheels with more than one axis!
				}
				OnHScroll(IDT_WORKSPACE_H_SCROLL, m_applColIndex + rotateCols*dir, TRUE/*redraw*/);
			}
		}
	}
}



void SjAlbumBrowser::OnDropImage(SjDataObject* data, int mouseX, int mouseY)
{
	SjCol* col;
	SjRow* row;
	if( FindRow(mouseX, mouseY, &col, &row) != FOUND_ROW )
	{
		return;
	}

	wxASSERT( col && row );

	if( row->OnDropData(data) )
	{
		Realize(false, true /*keep col index*/);
		m_window->Refresh();
		m_window->Update();
	}
}


void SjAlbumBrowser::OnContextMenu(int clickX, int clickY)
{
	SjMenu  mainMenu(0);

	// add module items to menu
	SjCol* col;
	SjRow* row;
	if( FindRow(clickX, clickY, &col, &row) == FOUND_ROW )
	{
		// select the item under the mouse
		if( row->IsSelectable() && !row->IsSelected() )
		{
			g_mainFrame->m_columnMixer.SelectAll(FALSE);
			row->Select(TRUE);
			RefreshSelection();
		}

		// create context menu for the item under the mouse
		row->CreateContextMenu(mainMenu);
		m_lastClickedCol = col;
		m_lastClickedRow = row;
	}
	else
	{
		// no item under the mouse
		m_lastClickedCol = NULL;
		m_lastClickedRow = NULL;
	}

	// add main items to menu
	g_mainFrame->CreateContextMenu_(mainMenu);

	// show menu
	if( mainMenu.GetMenuItemCount() )
	{
		m_window->PopupMenu(&mainMenu);
	}
}


#define IDC_L_FIRST                  (IDM_LASTPRIVATE-17) // range start
#define IDC_L_SHOWDOUBLETRACKS       (IDM_LASTPRIVATE-16)
#define IDC_L_SHOWTRACKNR            (IDM_LASTPRIVATE-15)
#define IDC_L_AUTOMTRACKNR           (IDM_LASTPRIVATE-14)
#define IDC_L_SHOWDISKNR             (IDM_LASTPRIVATE-13)
#define IDC_L_SHOWLEADARTISTNAME     (IDM_LASTPRIVATE-12)
#define IDC_L_SHOWDIFFLEADARTISTNAME (IDM_LASTPRIVATE-11)
#define IDC_L_SHOWORGARTISTNAME      (IDM_LASTPRIVATE-10)
#define IDC_L_SHOWCOMPOSERNAME       (IDM_LASTPRIVATE-9)
#define IDC_L_SHOWALBUMNAME          (IDM_LASTPRIVATE-8)
#define IDC_L_SHOWDIFFALBUMNAME      (IDM_LASTPRIVATE-7)
#define IDC_L_SHOWTIME               (IDM_LASTPRIVATE-6)
#define IDC_L_SHOWYEAR               (IDM_LASTPRIVATE-5)
#define IDC_L_SHOWCOMMENT            (IDM_LASTPRIVATE-4)
#define IDC_L_SHOWGENRE              (IDM_LASTPRIVATE-3)
#define IDC_L_SHOWRATING             (IDM_LASTPRIVATE-2)
#define IDC_L_LAST                   (IDM_LASTPRIVATE-1) // range end


void SjAlbumBrowser::OnContextMenuSelect(int id)
{
	SjCol*  col;
	SjRow*  row;

	if( id >= IDC_L_FIRST && id <= IDC_L_LAST )
	{
		long flags = g_mainFrame->m_libraryModule->GetFlags();
		if( id == IDC_L_SHOWDOUBLETRACKS )       { SjTools::ToggleFlag(flags, SJ_LIB_SHOWDOUBLETRACKS); }
		if( id == IDC_L_SHOWTRACKNR )            { SjTools::ToggleFlag(flags, SJ_LIB_SHOWTRACKNR); }
		if( id == IDC_L_AUTOMTRACKNR )           { SjTools::ToggleFlag(flags, SJ_LIB_AUTOMTRACKNR); }
		if( id == IDC_L_SHOWDISKNR )             { SjTools::ToggleFlag(flags, SJ_LIB_SHOWDISKNR); }
		if( id == IDC_L_SHOWLEADARTISTNAME )     { SjTools::ToggleFlag(flags, SJ_LIB_SHOWLEADARTISTNAME); }
		if( id == IDC_L_SHOWDIFFLEADARTISTNAME ) { SjTools::ToggleFlag(flags, SJ_LIB_SHOWDIFFLEADARTISTNAME); }
		if( id == IDC_L_SHOWORGARTISTNAME )      { SjTools::ToggleFlag(flags, SJ_LIB_SHOWORGARTISTNAME); }
		if( id == IDC_L_SHOWCOMPOSERNAME )       { SjTools::ToggleFlag(flags, SJ_LIB_SHOWCOMPOSERNAME); }
		if( id == IDC_L_SHOWALBUMNAME )          { SjTools::ToggleFlag(flags, SJ_LIB_SHOWALBUMNAME); }
		if( id == IDC_L_SHOWDIFFALBUMNAME )      { SjTools::ToggleFlag(flags, SJ_LIB_SHOWDIFFALBUMNAME); }
		if( id == IDC_L_SHOWTIME )               { SjTools::ToggleFlag(flags, SJ_LIB_SHOWTIME); }
		if( id == IDC_L_SHOWYEAR )               { SjTools::ToggleFlag(flags, SJ_LIB_SHOWYEAR); }
		if( id == IDC_L_SHOWCOMMENT )            { SjTools::ToggleFlag(flags, SJ_LIB_SHOWCOMMENT); }
		if( id == IDC_L_SHOWGENRE )              { SjTools::ToggleFlag(flags, SJ_LIB_SHOWGENRE); }
		if( id == IDC_L_SHOWRATING )             { SjTools::ToggleFlag(flags, SJ_LIB_SHOWRATING); }
		g_mainFrame->m_libraryModule->SetFlags(flags);
		g_mainFrame->m_browser->RefreshAll();
		UpdateItemsInColMenu(g_mainFrame->m_viewMenu);
	}
	else if( m_lastClickedCol && m_lastClickedRow )
	{
		m_lastClickedRow->OnContextMenu(id);
	}
	else if( FindSelectedRowInView(&col, &row) )
	{
		wxASSERT(col && row);
		row->OnContextMenu(id);
	}
}


wxRect SjAlbumBrowser::GetToolTipRect(int mouseX, int mouseY)
{
	wxRect      retRect;
	SjCol*      col;
	SjRow*      row;

	long found = FindRow(mouseX, mouseY, &col, &row, &retRect);
	if( found == FOUND_ROW || found == FOUND_COVER_ARROW )
	{
		; // rect is set up by FindRow
	}
	else
	{
		retRect.x = -1000;
		retRect.y = -1000;
		retRect.width = 1;
		retRect.height = 1;
	}

	return retRect;
}


wxString SjAlbumBrowser::GetToolTipText(int mouseX, int mouseY, long &flags)
{
	wxString    retString;
	SjCol*      col;
	SjRow*      row;

	long found = FindRow(mouseX, mouseY, &col, &row);
	if( found == FOUND_ROW )
	{
		if( col!=m_lastTooltipCol || row!=m_lastTooltipRow)
		{
			retString = row->GetToolTip(flags);
			m_lastTooltipCol = col;
			m_lastTooltipRow = row;
		}
	}
	else
	{
		if( found == FOUND_COVER_ARROW  )
			retString = _("Show covers");

		m_lastTooltipCol = NULL;
		m_lastTooltipRow = NULL;
	}

	return retString;
}


void SjAlbumBrowser::AddItemsToColMenu(SjMenu* m)
{
	m->AppendSeparator();
	m->AppendCheckItem(IDC_L_SHOWTIME,               _("Duration"));
	m->AppendCheckItem(IDC_L_SHOWLEADARTISTNAME,     _("Artist"));
	m->AppendCheckItem(IDC_L_SHOWORGARTISTNAME,      _("Original artist"));
	m->AppendCheckItem(IDC_L_SHOWCOMPOSERNAME,       _("Composer"));
	m->AppendCheckItem(IDC_L_SHOWALBUMNAME,          _("Album"));
	m->AppendCheckItem(IDC_L_SHOWTRACKNR,            _("Track number"));
	m->AppendCheckItem(IDC_L_AUTOMTRACKNR,           _("Automatic track number"));
	m->AppendCheckItem(IDC_L_SHOWDISKNR,             _("Disk number"));
	m->AppendCheckItem(IDC_L_SHOWGENRE,              _("Genre"));
	m->AppendCheckItem(IDC_L_SHOWYEAR,               _("Year"));
	m->AppendCheckItem(IDC_L_SHOWRATING,             _("Rating"));
	m->AppendCheckItem(IDC_L_SHOWCOMMENT,            _("Comment"));
	m->AppendSeparator();
	m->AppendCheckItem(IDC_L_SHOWDOUBLETRACKS,       _("Show double tracks"));
	m->AppendCheckItem(IDC_L_SHOWDIFFLEADARTISTNAME, _("Show different artist names"));
	m->AppendCheckItem(IDC_L_SHOWDIFFALBUMNAME,      _("Show different album names"));

	UpdateItemsInColMenu(m);
}


void SjAlbumBrowser::UpdateItemsInColMenu(SjMenu* m)
{
	long flags = g_mainFrame->m_libraryModule->GetFlags();
	m->Check(IDC_L_SHOWTIME,               (flags&SJ_LIB_SHOWTIME)!=0);
	m->Check(IDC_L_SHOWLEADARTISTNAME,     (flags&SJ_LIB_SHOWLEADARTISTNAME)!=0);
	m->Check(IDC_L_SHOWORGARTISTNAME,      (flags&SJ_LIB_SHOWORGARTISTNAME)!=0);
	m->Check(IDC_L_SHOWCOMPOSERNAME,       (flags&SJ_LIB_SHOWCOMPOSERNAME)!=0);
	m->Check(IDC_L_SHOWALBUMNAME,          (flags&SJ_LIB_SHOWALBUMNAME)!=0);
	m->Check(IDC_L_SHOWTRACKNR,            (flags&SJ_LIB_SHOWTRACKNR)!=0);
	m->Check(IDC_L_AUTOMTRACKNR,           (flags&SJ_LIB_AUTOMTRACKNR)!=0);
	m->Check(IDC_L_SHOWDISKNR,             (flags&SJ_LIB_SHOWDISKNR)!=0);
	m->Check(IDC_L_SHOWGENRE,              (flags&SJ_LIB_SHOWGENRE)!=0);
	m->Check(IDC_L_SHOWYEAR,               (flags&SJ_LIB_SHOWYEAR)!=0);
	m->Check(IDC_L_SHOWRATING,             (flags&SJ_LIB_SHOWRATING)!=0);
	m->Check(IDC_L_SHOWCOMMENT,            (flags&SJ_LIB_SHOWCOMMENT)!=0);
	m->Check(IDC_L_SHOWDOUBLETRACKS,       (flags&SJ_LIB_SHOWDOUBLETRACKS)!=0);
	m->Check(IDC_L_SHOWDIFFLEADARTISTNAME, (flags&SJ_LIB_SHOWDIFFLEADARTISTNAME)!=0);
	m->Check(IDC_L_SHOWDIFFALBUMNAME,      (flags&SJ_LIB_SHOWDIFFALBUMNAME)!=0);
}


/*******************************************************************************
 * selection handling & scrolling
 ******************************************************************************/


bool SjAlbumBrowser::OnSkinTargetEvent(int targetId, SjSkinValue& value, long accelFlags)
{
	bool shiftSelection = g_mainFrame->IsOpAvailable(SJ_OP_MULTI_ENQUEUE)? ((accelFlags&wxACCEL_SHIFT)!=0) : false;

	if( targetId >= IDT_WORKSPACE_GOTO_A && targetId <= IDT_WORKSPACE_GOTO_0_9 )
	{
		g_visModule->StopVisIfOverWorkspace();
		g_mainFrame->EndSimpleSearch();
		if(!GotoColumn(g_mainFrame->m_columnMixer.GetMaskedColIndexByAz(targetId)))
			CalcPositions(); // needed to reset the correct a-z button
		m_window->SetFocus();
		return true;
	}
	else switch( targetId )
		{
			case IDT_WORKSPACE_GOTO_RANDOM:
				g_visModule->StopVisIfOverWorkspace();
				g_mainFrame->EndSimpleSearch();
				GotoColumn(SjTools::Rand(g_mainFrame->m_columnMixer.GetMaskedColCount()), true);
				m_window->SetFocus();
				return true;

			case IDT_WORKSPACE_KEY_LEFT:
				if( !g_mainFrame->IsWorkspaceMovedAway() )
					DoChangeSelection(SJ_SEL_LEFT, shiftSelection);
				return true;

			case IDT_WORKSPACE_KEY_RIGHT:
				if( !g_mainFrame->IsWorkspaceMovedAway() )
					DoChangeSelection(SJ_SEL_RIGHT, shiftSelection);
				return true;

			case IDT_WORKSPACE_KEY_UP:
				if( !g_mainFrame->IsWorkspaceMovedAway() )
					DoChangeSelection(SJ_SEL_UP, shiftSelection);
				return true;

			case IDT_WORKSPACE_KEY_DOWN:
				if( !g_mainFrame->IsWorkspaceMovedAway() )
					DoChangeSelection(SJ_SEL_DOWN, shiftSelection);
				return true;

			case IDT_WORKSPACE_INSERT:
			case IDT_WORKSPACE_DELETE:
				if( !g_mainFrame->IsWorkspaceMovedAway() && g_mainFrame->IsAllAvailable() )
				{
					long numDeletedTracks = g_mainFrame->m_columnMixer.DelInsSelection(targetId==IDT_WORKSPACE_DELETE);
					if( numDeletedTracks )
					{
						m_window->ReloadColumnMixer();
						m_window->Update();
						g_mainFrame->UpdateSearchInfo(numDeletedTracks);
					}
				}
				return true;

			case IDT_WORKSPACE_H_SCROLL:
			case IDT_WORKSPACE_LINE_LEFT:
			case IDT_WORKSPACE_LINE_RIGHT:
			case IDT_WORKSPACE_PAGE_LEFT:
			case IDT_WORKSPACE_PAGE_RIGHT:
				OnHScroll(targetId, value.value, TRUE /*redraw*/);
				return true;

			case IDT_WORKSPACE_V_SCROLL:
			case IDT_WORKSPACE_LINE_UP:
			case IDT_WORKSPACE_LINE_DOWN:
			case IDT_WORKSPACE_PAGE_UP:
			case IDT_WORKSPACE_PAGE_DOWN:
			case IDT_WORKSPACE_MINOR_HOME:
				OnVScroll(targetId, value.value, TRUE /*redraw*/);
				return true;

			case IDT_WORKSPACE_HOME: // the search should stay "as is"
			case IDT_WORKSPACE_END:
				OnVScroll(IDT_WORKSPACE_MINOR_HOME, 0, FALSE/*redraw*/);
				OnHScroll(targetId, 0, TRUE/*redraw*/);
				return true;
		}

	return false;
}


bool SjAlbumBrowser::OnHScroll(int nScrollCode, int nPos, bool redraw)
{
	wxASSERT( nScrollCode == IDT_WORKSPACE_H_SCROLL
	          || nScrollCode == IDT_WORKSPACE_LINE_LEFT
	          || nScrollCode == IDT_WORKSPACE_LINE_RIGHT
	          || nScrollCode == IDT_WORKSPACE_PAGE_LEFT
	          || nScrollCode == IDT_WORKSPACE_PAGE_RIGHT
	          || nScrollCode == IDT_WORKSPACE_HOME
	          || nScrollCode == IDT_WORKSPACE_END );

	long newIndex = m_applColIndex, oldIndex, oldScrollY, abs;
	bool canUseScroll;

	if( nScrollCode == IDT_WORKSPACE_HOME )
	{
		newIndex = 0;
	}
	else if( nScrollCode == IDT_WORKSPACE_END )
	{
		newIndex = m_applColCount; // sth. substracted below
	}
	else if( nScrollCode == IDT_WORKSPACE_LINE_LEFT )
	{
		newIndex--;
	}
	else if( nScrollCode == IDT_WORKSPACE_LINE_RIGHT )
	{
		newIndex++;
	}
	else if( nScrollCode == IDT_WORKSPACE_PAGE_LEFT )
	{
		newIndex -= m_visibleColCount;
	}
	else if( nScrollCode == IDT_WORKSPACE_PAGE_RIGHT )
	{
		newIndex += m_visibleColCount;
	}
	else if( nScrollCode == IDT_WORKSPACE_H_SCROLL )
	{
		newIndex = nPos;
	}

	if( newIndex > m_applColCount-m_visibleColCount ) {
		newIndex = m_applColCount-m_visibleColCount;
		// may get below zero, this is checked below
	}

	if( newIndex < 0 ) {
		newIndex = 0;
	}

	if( (newIndex != m_applColIndex)
	        || (newIndex == 0 && m_scrollX != 0) )
	{
		oldIndex = m_applColIndex;
		canUseScroll = TRUE;

		if( newIndex == 0 ) {
			m_scrollX = 0;
			canUseScroll = FALSE;
		}

		m_applColIndex = newIndex;
		CalcPositions();
		SetHScrollInfo();

		if( m_scrollY+m_window->m_clientH > m_maxBottom )
		{
			oldScrollY = m_scrollY;
			m_scrollY = m_maxBottom - m_window->m_clientH;
			if( m_scrollY < 0 )
			{
				m_scrollY = 0;
			}

			if( oldScrollY != m_scrollY )
			{
				canUseScroll = FALSE;
			}
		}
		SetVScrollInfo();

		abs = oldIndex - newIndex;
		if( abs < 0 )
		{
			abs*=-1;
		}

		if( abs > m_visibleColCount )
		{
			canUseScroll = FALSE;
		}

		if( canUseScroll )
		{
			if( g_mainFrame->m_imgThread->HasWaitingImages() )
			{
				canUseScroll = FALSE;
			}
		}

		if( redraw )
		{
			if( canUseScroll )
			{
				m_window->ScrollWindow((oldIndex - newIndex)*(g_mainFrame->m_currColumnWidth + g_mainFrame->m_currColumnXSpace), 0,
				                       NULL);

				m_window->Update();
			}
			else
			{
				m_window->Refresh();
			}

			return true; // sth changed
		}
	}

	return false; // nothing changed
}


bool SjAlbumBrowser::OnVScroll(int nScrollCode, int nPos, bool redraw)
{
	wxASSERT( nScrollCode == IDT_WORKSPACE_V_SCROLL
	          || nScrollCode == IDT_WORKSPACE_LINE_UP
	          || nScrollCode == IDT_WORKSPACE_LINE_DOWN
	          || nScrollCode == IDT_WORKSPACE_PAGE_UP
	          || nScrollCode == IDT_WORKSPACE_PAGE_DOWN
	          || nScrollCode == IDT_WORKSPACE_MINOR_HOME
	          || nScrollCode == IDT_WORKSPACE_MINOR_END );

	// returns TRUE if scrolling appears
	long newIndex = m_scrollY, oldScrollY;

	if( nScrollCode == IDT_WORKSPACE_MINOR_HOME )
	{
		newIndex = 0;
	}
	else if( nScrollCode == IDT_WORKSPACE_MINOR_END )
	{
		newIndex = m_maxBottom; // sth. substracted below
	}
	else if( nScrollCode == IDT_WORKSPACE_LINE_UP )
	{
		newIndex -= g_mainFrame->m_currFontSize*2;
	}
	else if( nScrollCode == IDT_WORKSPACE_LINE_DOWN )
	{
		newIndex += g_mainFrame->m_currFontSize*2;
	}
	else if( nScrollCode == IDT_WORKSPACE_PAGE_UP )
	{
		newIndex -= m_window->m_clientH;
	}
	else if( nScrollCode == IDT_WORKSPACE_PAGE_DOWN )
	{
		newIndex += m_window->m_clientH;
	}
	else if( nScrollCode == IDT_WORKSPACE_V_SCROLL )
	{
		newIndex = nPos;
	}

	if( newIndex < 0 )
	{
		newIndex = 0;
	}

	if( newIndex > m_maxBottom - m_window->m_clientH )
	{
		newIndex = m_maxBottom - m_window->m_clientH;
		if( newIndex < 0 )
		{
			newIndex = 0;
		}
	}

	if( newIndex != m_scrollY )
	{
		oldScrollY = m_scrollY;
		m_scrollY = newIndex;
		SetVScrollInfo();

		if( redraw )
		{
			if( g_mainFrame->m_imgThread->HasWaitingImages() )
			{
				// if there are waiting images, invalidate the whole rectangle as
				// some areas are no yet okay

				m_window->Refresh();
			}
			else
			{
				m_window->ScrollWindow(0, oldScrollY - newIndex, NULL);

				m_window->Update();
			}
		}

		return 1;
	}

	return 0;
}


bool SjAlbumBrowser::DoChangeSelection(long dir, bool shiftSelection)
{
	#define findSelectionFromTop  (dir==SJ_SEL_UP || dir==SJ_SEL_LEFT || dir==SJ_SEL_PREV)
	#define findBoundingFromTop   (dir!=SJ_SEL_RIGHT)

	SjCol   *oldCol, *newCol = NULL;
	SjRow   *oldRow, *newRow = NULL;
	long    oldColIndex, oldRowIndex;

	bool    redrawAll = FALSE;

	// Shifted selection?
	/////////////////////

	if( shiftSelection )
	{
		long scopeColIndex, scopeRowIndex;
		if( g_mainFrame->m_columnMixer.SelectShifted((dir==SJ_SEL_PREV || dir==SJ_SEL_UP || dir==SJ_SEL_LEFT)? -1 : +1, scopeColIndex, scopeRowIndex) )
		{
			// scroll horizontally, if needed
			if( scopeColIndex < m_applColIndex || scopeColIndex > m_applColIndex+m_visibleColCount )
			{
				OnHScroll(IDT_WORKSPACE_H_SCROLL, scopeColIndex, FALSE/*redraw*/);
				redrawAll = TRUE;
			}

			// set newCol and newRow
			if( scopeColIndex-m_applColIndex < 0
			        || scopeColIndex-m_applColIndex >= m_allocatedColCount
			        || m_applCol[scopeColIndex-m_applColIndex] == NULL )
			{
				goto ChangeSelection_FinalRedraw;
			}
			newCol = m_applCol[scopeColIndex-m_applColIndex];

			if( scopeRowIndex < 0
			        || scopeRowIndex >= newCol->m_rowCount
			        || newCol->m_rows[scopeRowIndex] == NULL )
			{
				goto ChangeSelection_FinalRedraw;
			}

			newRow = newCol->m_rows[scopeRowIndex];

			// a real dirty hack...
			goto ChangeSelection_FinalScroll;
		}
	}

	// Find currently selected row the new row to select.
	/////////////////////////////////////////////////////

	if( !FindSelectedRowInView(&oldCol, &oldRow, &oldColIndex, &oldRowIndex, findSelectionFromTop) )
	{
		// no selected row found - select first/last row
		if( dir == SJ_SEL_UP || dir==SJ_SEL_DOWN || dir==SJ_SEL_PREV || dir==SJ_SEL_NEXT )
		{
			if( !FindBoundingRowInView(&newCol, &newRow) )
			{
				return FALSE;
			}
		}
		else
		{
			if( !FindBoundingRowInView(&oldCol, &oldRow, &oldColIndex, &oldRowIndex, findBoundingFromTop) )
			{
				return FALSE;
			}
		}
	}

	// Select the row next to the selected row in the given direction.
	//////////////////////////////////////////////////////////////////

	if( newRow == NULL )
	{
		wxASSERT( oldRow && oldCol );

		// find out the new row to select...
		if( dir == SJ_SEL_DOWN || dir == SJ_SEL_NEXT )
		{
			// ...select down
			if( (newRow=oldCol->FindSelectableRow(oldRowIndex+1, +1))!=NULL )
			{
				newCol = oldCol;
			}
			else if( dir != SJ_SEL_NEXT )
			{
				if( (newRow=oldCol->FindSelectableRow(-1, -1))!=NULL )
				{
					newCol = oldCol;
				}
			}
		}
		else if( dir == SJ_SEL_UP || dir == SJ_SEL_PREV )
		{
			// ...select up
			if( (newRow=oldCol->FindSelectableRow(oldRowIndex-1, -1))!=NULL )
			{
				newCol = oldCol;
			}
			else if( dir != SJ_SEL_PREV )
			{
				if( (newRow=oldCol->FindSelectableRow(0, +1))!=NULL )
				{
					newCol = oldCol;
				}
			}
		}

		if(  dir == SJ_SEL_RIGHT
		        ||  dir == SJ_SEL_LEFT
		        || (dir == SJ_SEL_NEXT && newRow == NULL)
		        || (dir == SJ_SEL_PREV && newRow == NULL) )
		{
			// ...select left/right: get new column index
			long newColIndex = oldColIndex + ((dir==SJ_SEL_RIGHT||dir==SJ_SEL_NEXT)? +1 : -1);

			if( newColIndex < 0
			        || newColIndex >= m_allocatedColCount
			        || m_applCol[newColIndex] == NULL )
			{
				// the column index is not allocated -> scroll horizontally
				OnHScroll(newColIndex < 0? IDT_WORKSPACE_LINE_LEFT : IDT_WORKSPACE_LINE_RIGHT, 0, FALSE/*redraw*/);
				if( !FindSelectedRowInView(&oldCol, &oldRow, &oldColIndex, &oldRowIndex, findSelectionFromTop) )
				{
					if( !FindBoundingRowInView(&oldCol, &oldRow, &oldColIndex, &oldRowIndex, findBoundingFromTop) )
					{
						m_window->Refresh();
						return FALSE;
					}
				}

				newColIndex += ((dir==SJ_SEL_RIGHT||dir==SJ_SEL_NEXT)? -1 : +1);
				if( newColIndex < 0 || newColIndex >= m_allocatedColCount || m_applCol[newColIndex] == NULL )
				{
					m_window->Refresh();
					return FALSE;
				}

				redrawAll = TRUE;
			}

			// ...select left/right: find row at near y-position (for left/right)
			// or use the first/last row (for prev/next)
			newCol = m_applCol[newColIndex];

			if( dir == SJ_SEL_NEXT )
			{
				newRow = newCol->FindSelectableRow(0, +1);
			}
			else if( dir == SJ_SEL_PREV )
			{
				newRow = newCol->FindSelectableRow(-1, -1);
			}
			else
			{
				long minDiff = 100000L, r;
				for( r = 0; r < newCol->m_rowCount; r++ )
				{
					if( newCol->m_rows[r]->IsSelectable() == 2 )
					{
						long currDiff = newCol->m_rows[r]->m_top - oldRow->m_top;
						if( currDiff < 0 ) currDiff *= -1;
						if( currDiff < minDiff )
						{
							minDiff = currDiff;
							newRow  = newCol->m_rows[r];
							if( oldCol->IsFirstSelectableRow(oldRow) )
							{
								break;
							}
						}
					}
				}
			}
		}

		// new row to select found?
		if( newRow == NULL )
		{
			return FALSE; // nothing to select / selection not changed
		}
	}

	// change the selection
	///////////////////////

	wxASSERT( newCol && newRow );

	g_mainFrame->m_columnMixer.SelectAll(FALSE);
	newRow->Select(TRUE);

	// make sure, the new row is visible
	////////////////////////////////////

ChangeSelection_FinalScroll:

	wxASSERT( newCol && newRow );

	{
		bool newRowIsFirstRow = newCol->IsFirstSelectableRow(newRow);

		if( newRow->m_bottom-m_scrollY > m_window->m_clientH )
		{
			// scroll down
			OnVScroll(IDT_WORKSPACE_V_SCROLL, newRow->m_bottom - m_window->m_clientH, !redrawAll/*redraw*/);
		}
		else if( newRow->m_top-m_scrollY < 0 || newRowIsFirstRow )
		{
			// scroll up
			OnVScroll(IDT_WORKSPACE_V_SCROLL, newRowIsFirstRow? 0 : newRow->m_top, !redrawAll/*redraw*/);
		}

		if( newCol->m_textrRight-m_scrollX > m_window->m_clientW )
		{
			// scroll right
			OnHScroll(IDT_WORKSPACE_LINE_RIGHT, 0, FALSE/*redraw*/);
			redrawAll = TRUE;
		}
		else if( newCol->m_textlLeft-m_scrollX < 0 )
		{
			// scroll left
			OnHScroll(IDT_WORKSPACE_LINE_LEFT, 0, FALSE/*redraw*/);
			redrawAll = TRUE;
		}
	}

	// redraw
	/////////

ChangeSelection_FinalRedraw:

	if( redrawAll )
	{
		m_window->Refresh();
	}
	else
	{
		RefreshSelection();
	}

	m_window->Update(); // without an explicit Update() call, the update may come
	// very late if eg. a cursor key is held and there are many
	// scrolling events.

	return TRUE;
}


bool SjAlbumBrowser::GotoUrl(const wxString& url)
{
	bool    ret = FALSE;
	long    colIndex, rowIndex;
	SjRow*  row;
	SjCol*  col = g_mainFrame->m_columnMixer.GetMaskedCol(url, colIndex);
	if( col )
	{
		// check index
		delete col;

		col = NULL;
		if( colIndex >= 0 )
		{
			// goto column
			m_scrollX = 0;
			m_scrollY = 0;
			OnHScroll(IDT_WORKSPACE_H_SCROLL, colIndex, FALSE/*redraw*/);
			SetVScrollInfo();

			// select the correct row
			g_mainFrame->m_columnMixer.SelectAll(FALSE);
			for( colIndex = 0; colIndex < m_allocatedColCount; colIndex++ )
			{
				col = m_applCol[colIndex];
				if( col )
				{
					for( rowIndex = 0; rowIndex < col->m_rowCount; rowIndex++ )
					{
						row = col->m_rows[rowIndex];
						if( row->m_url == url )
						{
							if( row->IsSelectable()==2 )
							{
								row->Select(TRUE);
								if( row->m_bottom > m_window->m_clientH )
								{
									m_scrollY = row->m_bottom - m_window->m_clientH;
									SetVScrollInfo();
								}
							}

							ret = TRUE; // URL found!
						}
					}
				}
			}

			m_window->Refresh();
			m_window->Update();
		}
	}
	return ret;
}


bool SjAlbumBrowser::GotoColumn(long offset, bool selectOneInRow)
{
	m_scrollX = 0;
	m_scrollY = 0;
	bool sthChanged = OnHScroll(IDT_WORKSPACE_H_SCROLL, offset, FALSE/*redraw*/);

	if( selectOneInRow ) // only used by "goto random"
	{
		g_mainFrame->m_columnMixer.SelectAll(false);
		if( m_applColCount > 0 && m_allocatedColCount > 0 )
		{
			int selectableCount=0, rowIndex;
			for( rowIndex = 0; rowIndex < m_applCol[0]->m_rowCount; rowIndex++ )
			{
				SjRow* row = m_applCol[0]->m_rows[rowIndex];
				if( row->m_roughType == SJ_RRTYPE_NORMAL )
				{
					if( selectableCount > 8 )
					{ break; }
					selectableCount++;
				}
			}

			int doSelect = SjTools::Rand(selectableCount);

			selectableCount=0;
			for( rowIndex = 0; rowIndex < m_applCol[0]->m_rowCount; rowIndex++ )
			{
				SjRow* row = m_applCol[0]->m_rows[rowIndex];
				if( row->m_roughType == SJ_RRTYPE_NORMAL )
				{
					if( selectableCount == doSelect )
					{ row->Select(true); break; };
					selectableCount++;
				}
			}
		}


	}

	SetVScrollInfo();
	m_window->Refresh();
	m_window->Update();
	return sthChanged;
}


wxString SjAlbumBrowser::GetFirstVisiblePos()
{
	if( m_allocatedColCount > 0
	        && m_applCol[0] != NULL )
	{
		#if 0
			// use column url
			return m_applCol[0]->m_url;
		#else
			// use "real" url
			SjRow* row = m_applCol[0]->FindSelectableRow();
			if( row )
				return row->m_url;
		#endif
	}

	return wxEmptyString;
}


wxString SjAlbumBrowser::GetFirstSelectedOrVisiblePos(long& retViewOffset)
{
	SjCol* col;
	SjRow* row;
	if( FindSelectedRowInView(&col, &row, &retViewOffset) )
	{
		wxASSERT( col );
		retViewOffset |= (SJ_BROWSER_ALBUM_VIEW<<24);
		#if 0
			// use column url
			return col->m_url;
		#else
			// use "real" url
			return row->m_url;
		#endif
	}
	else
	{
		retViewOffset = 0;
		return GetFirstVisiblePos();
	}
}


void SjAlbumBrowser::GotoPos(const wxString& guid, long viewOffset_)
{
	#if 0
		long colOffset = g_mainFrame->m_columnMixer.GetMaskedColIndexByColUrl(guid);
	#else
		long colOffset;
		{
			SjCol* col = g_mainFrame->m_columnMixer.GetMaskedCol(guid, colOffset);
			if( col )
				delete col;
			else
				colOffset = -1;
		}
	#endif

	if( colOffset < 0 )
	{
		colOffset = 0;
	}
	else if( (viewOffset_&0x00FFFFFFL)!=0 && (viewOffset_>>24)==SJ_BROWSER_ALBUM_VIEW )
	{
		colOffset += (viewOffset_&0x00FFFFFFL) * -1;

		if( colOffset < 0 )
		{
			colOffset = 0;
		}

		if( colOffset > m_applColCount-m_visibleColCount )
		{
			colOffset = m_applColCount-m_visibleColCount;
		}
	}

	GotoColumn(colOffset);
}


void SjAlbumBrowser::SetHScrollInfo()
{
	// this function set the horzontal scrollbar information according to
	// the max. visible columns in bs->visibleColCount and to the
	// number of total columns in bs->applColCount

	SjSkinValue value;
	value.value         = m_applColIndex;
	value.vmax          = m_applColCount;
	value.thumbSize     = m_visibleColCount;
	g_mainFrame->SetSkinTargetValue(IDT_WORKSPACE_H_SCROLL, value);
}


void SjAlbumBrowser::SetVScrollInfo()
{
	// this function set the vertical scrollbar information according to
	// the logical height of the visible columns in bs->maxBottom
	// and to the physical window height in bs->clientH.

	int maxBottom = m_maxBottom;

	if( m_scrollY+m_window->m_clientH >= maxBottom )
	{
		maxBottom = m_scrollY + m_window->m_clientH;    // make sure, the vertical scrollbar is
		// always present
	}

	wxASSERT(g_mainFrame);
	SjSkinValue value;
	value.value     = m_scrollY;
	value.vmax      = maxBottom;
	value.thumbSize = m_window->m_clientH;
	g_mainFrame->SetSkinTargetValue(IDT_WORKSPACE_V_SCROLL, value);
}


long SjAlbumBrowser::FindRow(long xPos, long yPos, SjCol** retCol, SjRow** retRow, wxRect* retRect)
{
	int     c, r;
	SjCol*  col;
	SjRow*  row;
	wxRect  dummyRect; if( retRect == NULL ) retRect = &dummyRect;

	wxASSERT(retCol);
	wxASSERT(retRow);
	wxASSERT(retRect);

	*retCol = NULL;
	*retRow = NULL;

	xPos += m_scrollX;
	yPos += m_scrollY;

	for( c = 0; c < m_allocatedColCount; c++ )
	{
		col = m_applCol[c];
		if( col == NULL )
			continue; // -- bug fix in 1.15beta7: -- do _not_ return FALSE is simply one column is erroneous

		if( xPos >= col->m_textlLeft )
		{
			if( xPos < col->m_textrRight )
			{
				for( r = 0; r < col->m_rowCount; r++ )
				{
					row = col->m_rows[r];
					if( yPos >= row->m_top )
					{
						if( yPos < row->m_bottom )
						{
							if( row->m_roughType == SJ_RRTYPE_COVER
							 && (xPos > col->m_textlLeft+g_mainFrame->m_currCoverWidth || !(m_flags&SJ_BROWSER_VIEW_COVER)) )
							{
								return FOUND_NOTHING;
							}
							else
							{
								*retCol = col;
								*retRow = row;
								retRect->x = col->m_textlLeft - m_scrollX;
								retRect->width = row->m_roughType==SJ_RRTYPE_COVER? g_mainFrame->m_currCoverWidth : (col->m_textrRight - col->m_textlLeft);
								retRect->y = row->m_top - m_scrollY;
								retRect->height = row->m_bottom - row->m_top;
								return FOUND_ROW; // row found at the given postion
							}
						}
					}
					else
					{
						return FOUND_NOTHING;
					}
				}
				return FOUND_NOTHING;
			}
		}
		else
		{
			if( xPos >= col->m_textlLeft-g_mainFrame->m_currColumnXSpace
			        && col->m_rowCount > 0
			        && yPos >= col->m_rows[0]->m_top
			        && yPos <  col->m_rows[0]->m_top+m_fontStdHeight
			        && !col->m_textUp.IsEmpty()
			        && g_mainFrame->IsOpAvailable(SJ_OP_TOGGLE_ELEMENTS) )
			{
				retRect->x = (col->m_textlLeft-g_mainFrame->m_currColumnXSpace) - m_scrollX;
				retRect->width = g_mainFrame->m_currColumnXSpace;
				retRect->y = col->m_rows[0]->m_top - m_scrollY;
				retRect->height = m_fontStdHeight;
				return FOUND_COVER_ARROW;
			}
			else
			{
				return FOUND_NOTHING;
			}
		}
	}

	return FOUND_NOTHING;
}


bool SjAlbumBrowser::FindSelectedRowInView(SjCol** col__,       SjRow** row__,
        long*   colIndex__, long*   rowIndex__,
        bool    findFromTop)
{
	int     c, r;
	SjCol*  col;
	SjRow*  row;
	long    dummy;

	if( colIndex__ == NULL ) { colIndex__ = &dummy; }
	if( rowIndex__ == NULL ) { rowIndex__ = &dummy; }

	wxASSERT(col__);
	wxASSERT(row__);

	*col__          = NULL;
	*row__          = NULL;
	*colIndex__     = 0;
	*rowIndex__     = 0;

	if( findFromTop )
	{
		for( c = 0; c < m_allocatedColCount; c++ )
		{
			col = m_applCol[c];
			if( col )
			{
				for( r = 0; r < col->m_rowCount; r++ )
				{
					row = col->m_rows[r];
					if( row->IsSelected() )
					{
						*col__      = col;
						*row__      = row;
						*colIndex__ = c;
						*rowIndex__ = r;
						return TRUE;
					}
				}
			}
		}
	}
	else
	{
		for( c = m_allocatedColCount-1; c >= 0; c-- )
		{
			col = m_applCol[c];
			if( col )
			{
				for( r = col->m_rowCount-1; r >= 0; r-- )
				{
					row = col->m_rows[r];
					if( row->IsSelected() )
					{
						*col__      = col;
						*row__      = row;
						*colIndex__ = c;
						*rowIndex__ = r;
						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}


bool SjAlbumBrowser::FindBoundingRowInView(SjCol**  col__,      SjRow** row__,
        long*   colIndex__, long*   rowIndex__,
        bool    findFromTop)
{
	// the function searches the first selectable row
	// in the first or in the last visible column

	SjCol*  col;
	long    dummy, r;

	if( colIndex__ == NULL ) { colIndex__ = &dummy; }
	if( rowIndex__ == NULL ) { rowIndex__ = &dummy; }

	*col__          = NULL;
	*row__          = NULL;
	*colIndex__     = 0;
	*rowIndex__     = 0;

	if( m_allocatedColCount <= 0 || m_visibleColCount <= 0 )
	{
		return FALSE; // no columns - no rows
	}

	// get column
	*colIndex__ = findFromTop? 0 : m_visibleColCount-1;

	col = m_applCol[*colIndex__];
	if( col==NULL || col->m_rowCount <= 0 )
	{
		return FALSE; // no rows
	}

	// get row
	for( r = 0; r < col->m_rowCount; r++ )
	{
		if( col->m_rows[r]->IsSelectable() == 2 )
		{
			*col__      = col;
			*row__      = col->m_rows[r];
			*rowIndex__ = r;
			return TRUE; // use this row
		}
	}

	return FALSE; // no selectable row
}


/*******************************************************************************
 *  Painting the window
 ******************************************************************************/


void SjAlbumBrowser::OnSize(wxSizeEvent& event)
{
	CalcPositions();
	SetHScrollInfo();

	if( m_scrollY+m_window->m_clientH > m_maxBottom )
	{
		m_scrollY = m_maxBottom - m_window->m_clientH;

		if( m_scrollY < 0 )
		{
			m_scrollY = 0;
		}
	}

	SetVScrollInfo();
}


void SjAlbumBrowser::OnImageThere(SjImageThereEvent& event)
{
	SjImgThreadObj* obj = event.GetObj();
	int             c, r;
	SjCol*          applCol;
	SjRow*          applRow;

	wxASSERT(obj);

	for( c = 0; c < m_allocatedColCount; c++ )
	{
		applCol = m_applCol[c];
		if( applCol )
		{
			for( r = 0; r < applCol->m_rowCount; r++ )
			{
				applRow = applCol->m_rows[r];
				if( applRow->m_roughType == SJ_RRTYPE_COVER && (m_flags&SJ_BROWSER_VIEW_COVER) )
				{
					if( obj->m_url == applRow->m_textm )
					{
						/*#ifdef __WXMAC__
						m_window->RefreshRect(wxRect(applCol->m_textlLeft-m_scrollX, applRow->m_top - m_scrollY, g_mainFrame->m_currCoverWidth, g_mainFrame->m_currCoverHeight));
						#else*/
						wxClientDC dc(m_window);

						m_window->PaintCover(dc, obj,
						                     applCol->m_textlLeft - m_scrollX,
						                     applRow->m_top - m_scrollY);
						/*#endif*/
					}

					break;  // break out of inner loop;
					// continue searching rows that need this
					// image in the outer loop
				}
			}
		}
	}

	g_mainFrame->m_imgThread->ReleaseImage(m_window, obj);
}


void SjAlbumBrowser::InvalidateNonCovers()
{
	int         c, r;
	SjCol*  col;
	SjRow*  row;
	wxSize      size;
	int         left, top, right, bottom;

	size    = m_window->GetClientSize();
	left    = 0;
	top     = 0;
	right   = size.x;
	bottom  = size.y;

	for( c = 0; c < m_allocatedColCount; c++ )
	{
		col = m_applCol[c];
		if( col )
		{
			left = col->m_textlLeft - m_scrollX;
			right= col->m_textrRight - m_scrollX;
			for( r = 0; r < col->m_rowCount; r++ )
			{
				row = col->m_rows[r];
				if( row->m_roughType != SJ_RRTYPE_COVER )
				{
					top = row->m_top - m_scrollY;
					m_window->RefreshRect(wxRect(left, top, right-left, bottom-top));
					break;
				}
			}
		}
	}
}


void SjAlbumBrowser::InvalidateCovers()
{
	int         c, r;
	SjCol*  col;
	SjRow*  row;
	wxSize      size;
	int         left, top, right, bottom;

	size    = m_window->GetClientSize();
	left    = 0;
	top     = 0;
	right   = size.x;
	bottom  = size.y;

	for( c = 0; c < m_allocatedColCount; c++ )
	{
		col = m_applCol[c];
		if( col )
		{
			left = col->m_textlLeft - m_scrollX;
			right= col->m_textrRight - m_scrollX;
			for( r = 0; r < col->m_rowCount; r++ )
			{
				row = col->m_rows[r];
				if( row->m_roughType == SJ_RRTYPE_COVER )
				{
					bottom = row->m_bottom - m_scrollY;
					m_window->RefreshRect(wxRect(left, top, right-left, bottom-top));
					break;
				}
			}
		}
	}
}


wxBitmap SjAlbumBrowser::GetDragNDropBitmap(int mouseX, int mouseY, wxPoint& retHotspot, wxRect& retSize)
{
	// calculate the width and the height of the bitmap
	long    dndLeft=1000000, dndTop=1000000, dndRight=-1000000, dndBottom=-1000000;
	long    dndWidth, dndHeight;
	int     c, r;
	SjCol*  col;
	SjRow*  row;
	for( c = 0; c < m_allocatedColCount; c++ )
	{
		col = m_applCol[c];
		if( col )
		{
			for( r = 0; r < col->m_rowCount; r++ )
			{
				row = col->m_rows[r];
				if( row->IsSelected() )
				{
					if( col->m_textlLeft < dndLeft  ) dndLeft  = col->m_textlLeft;
					if( col->m_textrRight> dndRight ) dndRight = col->m_textrRight;
					if( row->m_top       < dndTop   ) dndTop   = row->m_top;
					if( row->m_bottom    > dndBottom) dndBottom= row->m_bottom;

				}
			}
		}
	}

	if( dndLeft>=dndRight || dndTop>=dndBottom )
	{
		return wxNullBitmap;
	}

	dndWidth  = dndRight-dndLeft;
	dndHeight = dndBottom-dndTop;

	retHotspot.x  = (mouseX+m_scrollX) - dndLeft;
	retHotspot.y  = (mouseY+m_scrollY) - dndTop;

	// create an offscreen DC with the given Size
	wxBitmap            imgBitmap(dndWidth, dndHeight);
	wxMemoryDC          imgDc;
	imgDc.SelectObject(imgBitmap);
	wxBitmap            maskBitmap(dndWidth, dndHeight, 1);
	wxMemoryDC          maskDc;
	maskDc.SelectObject(maskBitmap);

	// draw the selected rows to the offscreen dc
	imgDc.SetPen(*wxTRANSPARENT_PEN);
	imgDc.SetBrush(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMALODD].bgBrush);
	imgDc.DrawRectangle(0, 0, dndWidth, dndHeight);

	maskDc.SetPen(*wxTRANSPARENT_PEN);
	maskDc.SetBrush(*wxBLACK_BRUSH);
	maskDc.DrawRectangle(0, 0, dndWidth, dndHeight);
	maskDc.SetBrush(*wxWHITE_BRUSH);

	wxSize scrollBackup(m_scrollX, m_scrollY);
	m_scrollX = dndLeft;
	m_scrollY = dndTop;
	for( c = 0; c < m_allocatedColCount; c++ )
	{
		col = m_applCol[c];
		if( col )
		{
			bool lastRowSelected = FALSE;
			for( r = 0; r < col->m_rowCount; r++ )
			{
				row = col->m_rows[r];
				if( row->IsSelected() )
				{
					PaintRow(imgDc, col, row,
					         col->m_textlLeft-m_scrollX, row->m_top-m_scrollY, col->m_textrRight-m_scrollX, row->m_bottom-m_scrollY,
					         FALSE, TRUE, lastRowSelected);
					lastRowSelected = TRUE;

					maskDc.DrawRectangle(col->m_textlLeft-m_scrollX, row->m_top-m_scrollY,
					                     col->m_textrRight-col->m_textlLeft, row->m_bottom-row->m_top);
				}
				else
				{
					lastRowSelected = FALSE;
				}
			}
		}
	}
	m_scrollX = scrollBackup.x;
	m_scrollY = scrollBackup.y;

	// done creating the images
	imgDc.SelectObject(wxNullBitmap);
	maskDc.SelectObject(wxNullBitmap);

	// scale the bitmaps down if they are too large
	#define MAX_W 300
	#define MAX_H 300
	if( dndWidth>MAX_W || dndHeight>MAX_H )
	{
		// calculate the new width and height
		long oldWidth = dndWidth, oldHeight = dndHeight;

		if( dndWidth>MAX_W )
		{
			dndHeight = (dndHeight*MAX_W) / dndWidth;
			dndWidth  =  MAX_W;
		}

		if( dndHeight>MAX_H )
		{
			dndWidth  = (dndWidth*MAX_H) / dndHeight;
			dndHeight =  MAX_H;
		}

		// scale down image
		wxImage img = imgBitmap.ConvertToImage();
		//img.Rescale(dndWidth, dndHeight);
		SjImgOp::DoResize(img, dndWidth, dndHeight, SJ_IMGOP_SMOOTH);
		imgBitmap = wxBitmap(img);

		// scale down mask
		img = maskBitmap.ConvertToImage();
		img.Rescale(dndWidth, dndHeight);
		maskBitmap = wxBitmap(img, 1);

		// correct hotspot
		retHotspot.x = (retHotspot.x*dndWidth) / oldWidth;
		retHotspot.y = (retHotspot.y*dndHeight) / oldHeight;
	}

	// done
	imgBitmap.SetMask(new wxMask(maskBitmap));

	retSize.x     = 0;
	retSize.y     = 0;
	retSize.width = dndWidth;
	retSize.height= dndHeight;

	return imgBitmap;
}


void SjAlbumBrowser::PaintRow(wxDC& dc, SjCol* col, SjRow* row,
                              long drawRectLeft, long drawRectTop, long drawRectRight, long drawRectBottom,
                              bool odd, bool rowSelected, bool lastRowSelected)
{
	bool            rowEnqueued = g_mainFrame->IsEnqueued(row->m_url);
	wxFont          *font1, *font2;
	SjSkinColour* colour;
	wxRect          drawRect;

	if( row->m_roughType == SJ_RRTYPE_COVER )
	{
		// draw cover
		if( m_flags & SJ_BROWSER_VIEW_COVER )
		{
			SjImgThreadObj* cachedImg = m_window->RequireImage(row->m_textm); // cachedImg may be NULL, but this is okay for PaintCover() and for ReleaseImage()
			m_window->PaintCover(dc, cachedImg,
			                     drawRectLeft, drawRectTop);
			g_mainFrame->m_imgThread->ReleaseImage(m_window, cachedImg);

			if( g_mainFrame->m_currCoverWidth < g_mainFrame->m_currColumnWidth )
			{
				dc.SetBrush(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgBrush);
				dc.DrawRectangle(drawRectLeft+g_mainFrame->m_currCoverWidth, drawRectTop,
				                 g_mainFrame->m_currColumnWidth-g_mainFrame->m_currCoverWidth, g_mainFrame->m_currCoverHeight);
			}
		}
	}
	else
	{
		// get fonts
		m_window->GetFonts(row->m_roughType, &font1, &font2, rowEnqueued);

		if( rowSelected )
		{
			dc.SetBrush(g_mainFrame->m_workspaceColours[odd? SJ_COLOUR_SELECTIONODD : SJ_COLOUR_SELECTION].bgBrush);

			if( lastRowSelected )
			{
				dc.SetPen(g_mainFrame->m_workspaceColours[/*odd? SJ_COLOUR_NORMALODD : */SJ_COLOUR_NORMAL].bgPen);
				dc.DrawLine(drawRectLeft, drawRectTop, drawRectRight, drawRectTop);
				dc.SetPen(*wxTRANSPARENT_PEN);
			}

			dc.DrawRectangle(drawRectLeft, drawRectTop + (lastRowSelected? 1 : 0),
			                 drawRectRight-drawRectLeft, drawRectBottom-drawRectTop);

			colour = &g_mainFrame->m_workspaceColours[odd? SJ_COLOUR_SELECTIONODD : SJ_COLOUR_SELECTION];
		}
		else
		{
			dc.SetBrush(g_mainFrame->m_workspaceColours[odd? SJ_COLOUR_NORMALODD : SJ_COLOUR_NORMAL].bgBrush);
			dc.DrawRectangle(drawRectLeft, drawRectTop, drawRectRight-drawRectLeft, drawRectBottom-drawRectTop);

			if( row->m_roughType == SJ_RRTYPE_TITLE1 )
			{
				colour = &g_mainFrame->m_workspaceColours[SJ_COLOUR_TITLE1];
			}
			else if( row->m_roughType == SJ_RRTYPE_TITLE2 )
			{
				colour = &g_mainFrame->m_workspaceColours[SJ_COLOUR_TITLE2];
			}
			else if( row->m_roughType == SJ_RRTYPE_TITLE3 )
			{
				colour = &g_mainFrame->m_workspaceColours[SJ_COLOUR_TITLE3];
			}
			else
			{
				colour = &g_mainFrame->m_workspaceColours[odd? SJ_COLOUR_NORMALODD : SJ_COLOUR_NORMAL];
			}
		}

		wxASSERT(colour);
		dc.SetTextBackground(colour->bgColour);
		dc.SetTextForeground(colour->fgColour);

		// draw mid text
		if( row->m_roughType < SJ_RRTYPE_TITLE1 || row->m_roughType > SJ_RRTYPE_TITLE3 )
		{
			drawRectLeft    = col->m_textmLeft - m_scrollX;
			drawRectRight   = col->m_textmRight - m_scrollX;
		}
		else if(  g_accelModule && g_accelModule->UseNumpad() )
		{
			drawRectLeft    = col->m_textmLeft - m_scrollX;
		}

		drawRect = wxRect(drawRectLeft, drawRectTop, drawRectRight-drawRectLeft, drawRectBottom-drawRectTop);
		g_tools->DrawText(dc, row->m_textm, drawRect, *font1, *font2, colour->hiColour);

		// draw left/right text
		if( !row->m_textl.IsEmpty() || !row->m_textr.IsEmpty() )
		{
			dc.SetFont(g_mainFrame->m_currSmallFont);

			drawRectTop += m_fontVDiff;

			// right text
			wxCoord extentW, extentH;
			dc.GetTextExtent(row->m_textr, &extentW, &extentH);
			dc.DrawText(row->m_textr, col->m_textrRight - extentW - m_scrollX - 1, drawRectTop);

			// left text
			if( rowEnqueued )
			{
				dc.SetTextForeground(colour->hiColour);
				dc.SetFont(g_mainFrame->m_currSmallBoldFont);
			}
			dc.DrawText(row->m_textl, col->m_textlLeft - m_scrollX + 1, drawRectTop);
		}
	}
}


void SjAlbumBrowser::PaintCol(wxDC& dc, SjCol* col,
                              long rcPaintLeft, long rcPaintTop, long rcPaintRight, long rcPaintBottom)
{
	// this function assumes, that the pen is set to transparent on call!
	SjRow*          row;
	bool            rowSelected, lastRowSelected = FALSE;
	int             r;
	long            drawRectLeft, drawRectTop, drawRectRight, drawRectBottom;
	bool            coverDrawn = FALSE;

	bool odd = TRUE, toggleOdd = TRUE;
	for( r = 0; r < col->m_rowCount; r++ )
	{
		row = col->m_rows[r];

		rowSelected = row->IsSelected();

		drawRectTop     = row->m_top - m_scrollY;
		drawRectBottom  = row->m_bottom - m_scrollY;

		if( drawRectTop > rcPaintBottom )
		{
			break; // nothing more to draw
		}

		// set "odd"
		if( row->m_roughType == SJ_RRTYPE_COVER )
		{
			odd = TRUE; toggleOdd = TRUE;
		}
		else if( row->m_roughType == SJ_RRTYPE_NORMAL )
		{
			if( toggleOdd ) { odd = !odd; } else { odd = TRUE; toggleOdd = TRUE; }
		}
		else
		{
			odd = FALSE; toggleOdd = FALSE;
		}

		// row visible?
		if( drawRectBottom > rcPaintTop )
		{
			drawRectLeft    = col->m_textlLeft - m_scrollX;
			drawRectRight   = col->m_textrRight - m_scrollX;

			if( drawRectRight < rcPaintLeft
			        || drawRectLeft > rcPaintRight )
			{
				break; // nothing more to draw
			}

			if( row->m_roughType == SJ_RRTYPE_COVER )
			{
				coverDrawn = TRUE;
			}

			// draw row
			PaintRow(dc, col, row, drawRectLeft, drawRectTop, drawRectRight, drawRectBottom,
			         odd, rowSelected, lastRowSelected);
		}

		lastRowSelected = rowSelected;
	}

	//
	// if the cover is not yet drawn, require the cover anyway
	// as it may be needed for other sections (before PaintCol() was called, all covers were invalidated)
	//
	if( !coverDrawn && (m_flags&SJ_BROWSER_VIEW_COVER) )
	{
		for( r = 0; r < col->m_rowCount; r++ )
		{
			row = col->m_rows[r];
			if( row->m_roughType == SJ_RRTYPE_COVER )
			{
				drawRectTop     = row->m_top - m_scrollY;
				drawRectBottom  = row->m_bottom - m_scrollY;
				drawRectLeft    = col->m_textlLeft - m_scrollX;
				drawRectRight   = col->m_textrRight - m_scrollX;

				if( drawRectLeft < m_window->m_clientW // comparison against the window size is okay
				        && drawRectTop < m_window->m_clientH    // (comparison against the redrawing area would leave images unrequired)
				        && drawRectRight > 0
				        && drawRectBottom > 0 )
				{
					SjImgThreadObj* cachedImg = m_window->RequireImage(row->m_textm);
					if( cachedImg )
					{
						g_mainFrame->m_imgThread->ReleaseImage(m_window, cachedImg);
					}
				}
				break;
			}
		}
	}
}


void SjAlbumBrowser::DoPaint(wxDC& dc)
{
	int         colRectLeft, colRectTop, colRectRight, colRectBottom,
	            drawRectLeft, drawRectTop, drawRectRight, drawRectBottom;
	int         c;
	bool        anythingDrawn = FALSE;

	// get region outer box
	int         regionLeft, regionTop, regionRight, regionBottom;
	wxRegion    region = m_window->GetUpdateRegion();
	region.GetBox(regionLeft, regionTop, regionRight, regionBottom);
	regionRight     += regionLeft;
	regionBottom    += regionTop;

	// reset required and waiting images
	g_mainFrame->m_imgThread->RequireStart(m_window);

	// draw whitespace atop
	dc.SetPen(*wxTRANSPARENT_PEN);
	drawRectLeft    = 0;
	drawRectRight   = m_window->m_clientW;
	drawRectTop     = 0;
	drawRectBottom  = g_mainFrame->m_currColumnYSpace - m_scrollY;
	if( drawRectBottom > 0 )
	{
		dc.SetBrush(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgBrush);
		dc.DrawRectangle(drawRectLeft, drawRectTop, drawRectRight-drawRectLeft, drawRectBottom-drawRectTop);
	}

	// calculate position of the first column
	colRectLeft     = g_mainFrame->m_currColumnXSpace - m_scrollX;
	colRectTop      = g_mainFrame->m_currColumnYSpace - m_scrollY;
	colRectRight    = colRectLeft;
	colRectBottom   = m_window->m_clientH;

	for( c = 0; c < m_allocatedColCount; c++ )
	{
		// complete column position column
		colRectRight = colRectLeft + g_mainFrame->m_currColumnWidth;

		if( colRectLeft > 0 )
		{
			drawRectLeft    = colRectLeft - g_mainFrame->m_currColumnXSpace;
			drawRectTop     = colRectTop;
			drawRectRight   = colRectLeft;
			drawRectBottom  = colRectBottom;

			// draw whitespace aleft of column, the space left of the column belong to the column!
			dc.SetBrush(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgBrush);
			if( m_applCol[c] && !m_applCol[c]->m_textUp.IsEmpty() )
			{
				// draw icon
				wxRect toggleCoversRect;
				toggleCoversRect.x = drawRectLeft; //x;
				toggleCoversRect.y = drawRectTop; //SPACE_TOP;
				toggleCoversRect.width = g_mainFrame->m_currColumnXSpace; //SPACE_LEFT;
				toggleCoversRect.height = m_fontStdHeight; //m_fontHeight;
				dc.DrawRectangle(toggleCoversRect);

				wxRect iconRect = toggleCoversRect;
				iconRect.y+=2;
				iconRect.height-=4;
				iconRect.width = iconRect.height;

				iconRect.x += (toggleCoversRect.width-iconRect.width) / 2;

				dc.SetPen(g_mainFrame->m_workspaceColours[SJ_COLOUR_TITLE1].fgPen);
				SjTools::DrawIcon(dc, iconRect,
				                  (m_flags&SJ_BROWSER_VIEW_COVER)? SJ_DRAWICON_TRIANGLE_DOWN : SJ_DRAWICON_TRIANGLE_RIGHT);
				dc.SetPen(*wxTRANSPARENT_PEN);

				// draw uptext
				int upY = drawRectTop + toggleCoversRect.height;
				m_window->DrawUpText(dc, m_applCol[c]->m_textUp, drawRectLeft, upY, drawRectRight-drawRectLeft, drawRectBottom-upY);
			}
			else
			{
				dc.DrawRectangle(drawRectLeft, drawRectTop, drawRectRight-drawRectLeft, drawRectBottom-drawRectTop);
			}
		}

		// draw column
		if( m_applCol[c] )
		{
			PaintCol(dc, m_applCol[c], regionLeft, regionTop, regionRight, regionBottom);
			drawRectTop = m_applCol[c]->m_rows[m_applCol[c]->m_rowCount-1]->m_bottom - m_scrollY;
			anythingDrawn = TRUE;
		}
		else
		{
			drawRectTop = colRectTop;
		}

		// draw whitespace abottom of column
		if( drawRectTop < m_window->m_clientH )
		{
			drawRectLeft    = colRectLeft;
			drawRectRight   = colRectRight;
			drawRectBottom  = m_window->m_clientH;
			dc.SetBrush(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgBrush);
			dc.DrawRectangle(drawRectLeft, drawRectTop, drawRectRight-drawRectLeft, drawRectBottom-drawRectTop);
		}

		// calculate next position
		colRectLeft = colRectRight;
		if( colRectLeft >= m_window->m_clientW )
		{
			// done!
			break;
		}

		colRectLeft += g_mainFrame->m_currColumnXSpace;
	}

	// draw whitespace aright of last column; this will draw all if there are no columns at all
	if( colRectRight < m_window->m_clientW )
	{
		drawRectLeft    = colRectRight;
		drawRectTop     = colRectTop;
		drawRectRight   = m_window->m_clientW;
		drawRectBottom  = m_window->m_clientH;
		dc.SetBrush(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgBrush);
		dc.DrawRectangle(drawRectLeft, drawRectTop, drawRectRight-drawRectLeft, drawRectBottom-drawRectTop);
	}

	// free all images that are no longer required but still waiting
	g_mainFrame->m_imgThread->RequireEnd(m_window);

	// draw a stub text if there was nothing drawn before
	if( !anythingDrawn && g_mainFrame->m_libraryModule )
	{

	}
}


void SjAlbumBrowser::CalcRowPositions(wxDC& dc, SjCol* applCol, int x, int y)
{
	SjRow*      applRow;
	int         i;
	int         maxWidth,
	            leftWidth, rightWidth, midWidth,
	            currWidth, currHeight;
	wxFont      *font1, *font2;

	//
	// calculate the width of the left and the right columns
	//

	dc.SetFont(g_mainFrame->m_currSmallFont);

	maxWidth    = g_mainFrame->m_currColumnWidth / 4;
	leftWidth   = 0;
	rightWidth  = 0;
	for( i = 0; i < applCol->m_rowCount; i++ )
	{
		applRow = applCol->m_rows[i];

		applRow->m_bottom = m_fontVDiff; // temporary height

		if( !applRow->m_textl.IsEmpty() )
		{
			dc.GetTextExtent(applRow->m_textl, &currWidth, &currHeight);
			if( currWidth > maxWidth )
			{
				currWidth = maxWidth;
			}

			if( currWidth > leftWidth )
			{
				leftWidth = currWidth;
			}

			if( currHeight > applRow->m_bottom )
			{
				applRow->m_bottom = currHeight + m_fontVDiff;
			}
		}

		if( !applRow->m_textr.IsEmpty() )
		{
			dc.GetTextExtent(applRow->m_textr, &currWidth, &currHeight);
			if( currWidth > maxWidth )
			{
				currWidth = maxWidth;
			}

			if( currWidth > rightWidth )
			{
				rightWidth = currWidth;
			}

			if( currHeight > applRow->m_bottom )
			{
				applRow->m_bottom = currHeight + m_fontVDiff;
			}
		}
	}

	if( leftWidth )             { leftWidth += m_fontSpace;     }
	if( leftWidth > maxWidth )  { leftWidth = maxWidth;         }
	if( rightWidth )            { rightWidth += m_fontSpace;    }
	if( rightWidth > maxWidth ) { rightWidth = maxWidth;        }

	midWidth = g_mainFrame->m_currColumnWidth - (leftWidth+rightWidth);

	//
	// set the x positions in the column object
	//

	applCol->m_textlLeft  = x;
	applCol->m_textlRight = applCol->m_textlLeft + leftWidth;
	applCol->m_textmLeft  = applCol->m_textlRight;
	applCol->m_textmRight = applCol->m_textmLeft + midWidth;
	applCol->m_textrLeft  = applCol->m_textmRight;
	applCol->m_textrRight = applCol->m_textrLeft + rightWidth;

	//
	// calculate the height of each row
	//

	for( i = 0; i < applCol->m_rowCount; i++ )
	{
		applRow = applCol->m_rows[i];

		if( applRow->m_roughType == SJ_RRTYPE_COVER )
		{
			applRow->m_top    = y;
			if(m_flags&SJ_BROWSER_VIEW_COVER)
				y += g_mainFrame->m_currCoverHeight;
			applRow->m_bottom = y;
		}
		else
		{
			m_window->GetFonts(applRow->m_roughType, &font1, &font2, FALSE);

			int availWidth = midWidth;
			if( applRow->m_roughType>=SJ_RRTYPE_TITLE1 && applRow->m_roughType<=SJ_RRTYPE_TITLE3 )
			{
				availWidth = g_mainFrame->m_currColumnWidth;
				if(  g_accelModule && g_accelModule->UseNumpad() )
				{
					availWidth -= leftWidth;
				}
			}

			applRow->m_top = y;
			currHeight = g_tools->CalcTextHeight(dc, applRow->m_textm, *font1, *font2, availWidth);
			if( applRow->m_bottom > currHeight )
			{
				currHeight = applRow->m_bottom; // set to height needed by the left / right columns
			}
			y += currHeight;
			applRow->m_bottom = y;
		}
	}
}


void SjAlbumBrowser::CalcPositions()
{
	// function calculates column position based on bs->applColIndex
	wxClientDC  dc(m_window);
	int         i, azSet = 0;
	int         colRectLeft, colRectTop, colRectRight;

	//
	// calculate the height difference between the normal and the small fonts
	//

	m_window->GetFontPxSizes(dc, m_fontVDiff, m_fontSpace, m_fontStdHeight);

	// free all previously allocated objects
	for( i = 0; i < m_allocatedColCount; i++ )
	{
		if( m_applCol[i] )
		{
			if( m_applCol[i] == m_lastClickedCol )
			{
				m_lastClickedCol = NULL;
				m_lastClickedRow = NULL;
			}

			delete m_applCol[i];
			m_applCol[i] = 0;
		}
	}

	m_allocatedColCount = 0;
	m_visibleColCount   = 0;

	// calculate position of the first row
	m_maxBottom     = g_mainFrame->m_currColumnYSpace;
	colRectLeft     = g_mainFrame->m_currColumnXSpace;
	colRectTop      = g_mainFrame->m_currColumnYSpace;

	for( i = 0; i < MAX_COL_COUNT; i++ )
	{
		// complete column position column
		colRectRight = colRectLeft + g_mainFrame->m_currColumnWidth;

		// allocate column object for this column
		if( m_applColIndex+i < m_applColCount )
		{
			m_applCol[i] = g_mainFrame->m_columnMixer.GetMaskedCol(m_applColIndex+i);
		}
		else
		{
			m_applCol[i] = NULL;
		}

		m_allocatedColCount++;

		if( colRectRight <= m_window->m_clientW )
		{
			m_visibleColCount++;
		}

		// calculate all row positions
		if( m_applCol[i] )
		{
			if( i==0 )
			{
				g_mainFrame->SetSkinAzValues(m_applCol[0]->m_az);
				azSet = 1;
			}

			CalcRowPositions(dc, m_applCol[i], colRectLeft, colRectTop);

			if( m_applCol[i]->m_rows[m_applCol[i]->m_rowCount-1]->m_bottom > m_maxBottom )
			{
				m_maxBottom = m_applCol[i]->m_rows[m_applCol[i]->m_rowCount-1]->m_bottom;
			}
		}

		// calculate next position
		colRectLeft = colRectRight;
		if( colRectLeft >= m_window->m_clientW + m_scrollX )
		{
			// done!
			break;
		}

		colRectLeft += g_mainFrame->m_currColumnXSpace;
	}

	if( !azSet )
	{
		g_mainFrame->SetSkinAzValues('a');
	}
}
