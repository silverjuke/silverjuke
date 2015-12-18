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
 * File:    browser_cover.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke cover browser
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/imgthread.h>
#include <sjbase/browser.h>
#include <sjbase/browser_cover.h>
#include <sjbase/columnmixer.h>
#include <sjmodules/kiosk/virtkeybd.h>
#include <sjmodules/vis/vis_module.h>

#define SEL_BORDER_W 4  // 3 pixels border, 1 pixel space
// the selection border is just draw around the covers, no special calculations for this

#define SPACE_LEFT (g_mainFrame->m_currColumnXSpace)
#define SPACE_TOP  (g_mainFrame->m_currColumnYSpace)

#define SCROLL_RECT wxRect(SPACE_LEFT-SEL_BORDER_W, 0, (m_window->m_clientW-SPACE_LEFT)+SEL_BORDER_W, m_window->m_clientH)

#undef USE_COVER_DND


SjCoverBrowser::SjCoverBrowser(SjBrowserWindow* w)
	: SjBrowserBase(w)
{
	m_coversXCount = 1;
	m_coversYCount = 1;

	m_allocatedCoverCount = 0;
	m_allocatedCover = NULL;

	m_applCoverCount        = 0;
	m_applRowIndex          = 0;
	m_applRowCount          = 0;
	m_scrollY               = 0;

	m_dragStartX            = 0;
	m_dragStartY            = 0;
	m_dragscrollCurrY       = 0;
	m_linesBelowCover       = 0;

	m_lastTooltipCover      = NULL;

	m_lastClickedCover = NULL;
}

void SjCoverBrowser::FreeAllocatedCol()
{
	if( m_allocatedCover )
	{
		for( int i = 0; i < m_allocatedCoverCount; i++ )
		{
			if( m_allocatedCover[i] )
			{
				delete m_allocatedCover[i];
			}
		}

		free(m_allocatedCover);
	}

	m_allocatedCover = NULL;
	m_allocatedCoverCount = 0;
	m_lastClickedCover = NULL;
}


void SjCoverBrowser::Exit()
{
	FreeAllocatedCol();
}


void SjCoverBrowser::Realize(bool reloadColumnMixer, bool keepColIndex)
{
	// save old index
	long oldCoverXCount = m_coversXCount;
	long oldCoverIndex = m_applRowIndex*m_coversXCount;

	// recalculate the needed stuff
	if( reloadColumnMixer )
	{
		m_scrollY = 0;
		m_applCoverCount = g_mainFrame->m_columnMixer.GetMaskedColCount();
	}

	SjSkinValue value;
	value.value         = 0;
	value.vmax          = 0;
	value.thumbSize     = 0;
	g_mainFrame->SetSkinTargetValue(IDT_WORKSPACE_H_SCROLL, value);

	CalcRowStuff();

	// try to set a new, fine row index
	if( oldCoverXCount != m_coversXCount )
	{
		m_applRowIndex = oldCoverIndex / m_coversXCount;
	}

	if( m_applRowIndex >= (m_applRowCount-m_coversYCount) )
		m_applRowIndex = (m_applRowCount-m_coversYCount);

	if( m_applRowIndex<0 || !keepColIndex )
		m_applRowIndex = 0;

	// recalculate the rest
	CalcPositions();

	SetVScrollInfo();

	m_lastTooltipCover      = NULL;
}


/*******************************************************************************
 * Mouse Handling
 ******************************************************************************/


void SjCoverBrowser::SelectAllRows(SjCol* cover) const
{
	// s http://www.silverjuke.net/forum/post.php?p=5379#5379
	bool multiEnqueueAvailable = g_mainFrame->IsOpAvailable(SJ_OP_MULTI_ENQUEUE);

	if( multiEnqueueAvailable )
	{
		cover->SelectAllRows(true);
	}
	else
	{
		SjRow* r = cover->FindSelectableRow();
		if( r )
		{
			r->Select(true);
		}
	}
}


void SjCoverBrowser::OnMouseSelect(wxMouseEvent& event)
{
	/* (de-)select the item under the mouse (if any)
	 */
	SjCol* cover;
	long found = FindCover(event.GetX(), event.GetY(), &cover);
	if( found == FOUND_COVER )
	{
		g_mainFrame->m_columnMixer.SelectAll(false);
		SelectAllRows(cover);
		RefreshSelection();
	}
	else if( found == FOUND_COVER_ARROW )
	{
		g_mainFrame->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDT_WORKSPACE_SHOW_COVERS));
	}
	else if( g_mainFrame->m_columnMixer.IsAnythingSelected() )
	{
		g_mainFrame->m_columnMixer.SelectAll(false);
		RefreshSelection();
	}
}


void SjCoverBrowser::OnMouseLeftDown(wxMouseEvent& event)
{
	/* prepare dragscroll and object dragging
	 */
	m_window->m_mouseAction = SJ_ACTION_NONE;
	m_dragStartX            = event.GetX(); /* client coordinates */
	m_dragStartY            = event.GetY(); /* client coordinates */

	/* perform selection
	 */
	m_mouseSelectionOnDown = FALSE;
	#ifdef USE_COVER_DND
		SjCol* cover;
		if(  g_accelModule->m_selDragNDrop == 1
				&&  (cover=FindCover(m_dragStartX, m_dragStartY))!=NULL
				/*&& !cover->IsAnyRowSelected() -- we preserve given selections but allow only one album selected by us*/ )
		{
			OnMouseSelect(event);
			m_mouseSelectionOnDown = TRUE;
		}
	#endif
}


void SjCoverBrowser::OnMouseLeftUp(wxMouseEvent& event)
{
	// dragscroll: restore cursor if changed by dragscroll
	if( m_window->m_mouseAction == SJ_ACTION_DRAGSCROLL )
	{
		m_window->SetCursor(SjVirtKeybdModule::GetStandardCursor());
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


void SjCoverBrowser::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
	// dragscroll: restore cursor if changed by dragscroll
	if( m_window->m_mouseAction == SJ_ACTION_DRAGSCROLL )
	{
		m_window->SetCursor(SjVirtKeybdModule::GetStandardCursor());
		m_window->m_mouseAction = SJ_ACTION_NONE;
		return; // done
	}
}


void SjCoverBrowser::ToggleView()
{
	if( m_window->IsViewAvailable(SJ_BROWSER_ALBUM_VIEW) )
	{
		m_window->SetView_(SJ_BROWSER_ALBUM_VIEW, true, true);
	}
	else
	{
		m_window->SetView_(SJ_BROWSER_LIST_VIEW, true, true);
	}
}


bool SjCoverBrowser::OnMouseLeftDClick(wxMouseEvent& event)
{
	SjCol* cover;
	long found = FindCover(event.GetX(), event.GetY(), &cover);
	if( found == FOUND_COVER )
	{
		ToggleView();
		return true; // click used
	}
	else if( found == FOUND_COVER_ARROW )
	{
		return true; // click used (avoid toggle the view by fast clicks on the cover toggler)
	}

	return false; // click not used
}


void SjCoverBrowser::OnMouseMiddleUp(wxMouseEvent& event)
{
	bool    skip = TRUE;
	SjCol* cover;
	long found = FindCover(event.GetX(), event.GetY(), &cover);
	if( found == FOUND_COVER && cover->m_rowCount > 0 )
	{
		SjRow* row = cover->m_rows[0];
		int usesMiddleClick = row->UsesMiddleClick();
		if( usesMiddleClick )
		{
			g_mainFrame->m_columnMixer.SelectAll(false);
			SelectAllRows(cover);
			RefreshSelection();
			m_window->Update();
			skip = FALSE;

			row->OnMiddleClick(event.ShiftDown() || event.ControlDown());
		}
	}

	if( skip )
	{
		event.Skip();
	}
}


void SjCoverBrowser::OnMouseMotion(wxMouseEvent& event)
{
	if( !m_window->HasCapture() ) return;

	long    hDifference, vDifference;

	long    oldRowIndex, oldScrollY;

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
			#ifdef USE_COVER_DND
			SjCol* cover;
			if( g_accelModule->m_selDragNDrop
			 && (cover=FindCover(m_dragStartX, m_dragStartY))!=NULL
			 && cover->IsAnyRowSelected() )
			{
				// do object dragging
				m_window->m_dragUrls.Clear();
				g_mainFrame->m_columnMixer.GetSelectedUrls(m_window->m_dragUrls);
				if( g_accelModule->m_flags&SJ_ACCEL_USEDNDIMAGES )
				{
					g_mainFrame->m_dragImage = m_window->GetCoverDragNDropBitmap(cover, g_mainFrame->m_dragRect);
					if( g_mainFrame->m_dragImage )
					{
						g_mainFrame->m_dragHotspot.x = xPos - cover->m_textlLeft;
						g_mainFrame->m_dragHotspot.y = yPos - cover->m_top + m_scrollY;
					}
				}
				if( g_mainFrame->DragNDrop(SJ_DND_ENTER, m_window, event.GetPosition(), NULL, &m_window->m_dragUrls) )
				{
					m_window->m_mouseAction = SJ_ACTION_DRAGNDROP;
				}
			}
			else
			#endif
			if( g_accelModule->m_flags&SJ_ACCEL_CONTENT_DRAG
			 && m_applRowCount )
			{
				// start dragscroll
				m_window->m_mouseAction = SJ_ACTION_DRAGSCROLL;
				m_dragscrollCurrY       = yPos;
				m_window->SetCursor(g_tools->m_staticMovehandCursor);
			}
		}
	}

	// in drag'n'drop?
	#ifdef USE_COVER_DND
	if( m_window->m_mouseAction == SJ_ACTION_DRAGNDROP )
	{
		if( !g_mainFrame->DragNDrop(SJ_DND_MOVE, m_window, event.GetPosition(), NULL, &m_window->m_dragUrls) )
		{
			m_window->m_mouseAction = SJ_ACTION_NONE;
		}
	}
	#endif

	// in dragscroll?
	if( m_window->m_mouseAction == SJ_ACTION_DRAGSCROLL )
	{
		oldScrollY  = m_scrollY;
		oldRowIndex = m_applRowIndex;

		// horizontal scrolling
		vDifference = yPos - m_dragscrollCurrY;
		m_dragscrollCurrY = yPos;
		if( vDifference )
		{
			if( vDifference > m_coverNTitleHeight ) {
				vDifference = m_coverNTitleHeight;
			}
			else if( vDifference < 0 - m_coverNTitleHeight ) {
				vDifference = 0 - m_coverNTitleHeight;
			}

			m_scrollY -= vDifference;

			if( m_scrollY < 0 )
			{
				// decrease application column position if possible
				if( m_applRowIndex > 0 )
				{
					m_scrollY += m_coverNTitleHeight + SPACE_TOP;
					m_applRowIndex--;
					CalcPositions();
					SetVScrollInfo();
				}
				else
				{
					m_scrollY = 0;
				}
			}
			else if( m_scrollY >= m_coverNTitleHeight + SPACE_TOP )
			{
				// increase application column position if possible
				if( m_applRowIndex < m_applRowCount-m_coversYCount )
				{
					m_scrollY -= m_coverNTitleHeight + SPACE_TOP;
					m_applRowIndex++;
					CalcPositions();
					SetVScrollInfo();
				}
				else
				{
					m_scrollY = m_coverNTitleHeight + SPACE_TOP;
				}
			}
			else if( (m_allocatedCoverCount/m_coversXCount)*(m_coverNTitleHeight+SPACE_TOP)-m_scrollY < m_window->m_clientH )
			{
				// a new column will fit on the right
				if( m_applRowIndex+m_coversYCount < m_applRowCount )
				{
					CalcPositions();
				}
			}
		}

		// update window
		if( vDifference )
		{
			if( g_mainFrame->m_imgThread->HasWaitingImages() )
			{
				// if there are waiting images, invalidate the whole rectangle as
				// some areas are no yet okay
				m_window->Refresh();
			}
			else
			{
				vDifference =
				    (oldRowIndex*(m_coverNTitleHeight+SPACE_TOP) + oldScrollY)
				    -   (m_applRowIndex*(m_coverNTitleHeight+SPACE_TOP) + m_scrollY);

				wxRect scrollRect = SCROLL_RECT;
				m_window->ScrollWindow(0, vDifference, &scrollRect);

				m_window->Update();
			}
		}
	}
}


void SjCoverBrowser::OnMouseWheel(wxMouseEvent& event, bool scrollVert)
{
	long zDelta = event.GetWheelRotation();
	long wheelDelta = event.GetWheelDelta();

	// scroll!
	if( scrollVert && wheelDelta>0 )
	{
		OnVScroll(IDT_WORKSPACE_V_SCROLL, m_applRowIndex + (zDelta*-1 /* *m_coversYCount */)/wheelDelta, TRUE/*redraw*/);
		//                  ^^^ may be used to scroll one page - but I think this is too much
	}
}


void SjCoverBrowser::OnDropImage(SjDataObject* data, int mouseX, int mouseY)
{
	SjCol* cover;
	long found = FindCover(mouseX, mouseY, &cover);
	if( found == FOUND_COVER && cover->m_rowCount > 0 )
	{
		if( cover->m_rows[0]->OnDropData(data) )
		{
			Realize(false, true /*keep col index*/);
			m_window->Refresh();
			m_window->Update();
		}
	}
}


void SjCoverBrowser::OnContextMenu(int clickX, int clickY)
{
	SjMenu  mainMenu(0);
	SjCol* cover;
	long found = FindCover(clickX, clickY, &cover);
	if( found == FOUND_COVER && cover->m_rowCount > 0 && cover->m_rows[0]->m_roughType == SJ_RRTYPE_COVER )
	{
		// select the item under the mouse
		if( cover->FindFirstSelectedRow() == NULL )
		{
			g_mainFrame->m_columnMixer.SelectAll(FALSE);
			SelectAllRows(cover);
			RefreshSelection();
		}

		m_lastClickedCover = cover;
		cover->m_rows[0]->CreateContextMenu(mainMenu);
	}
	else
	{
		m_lastClickedCover = NULL;
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
#define IDC_L_SHOWLEADARTISTNAME     (IDM_LASTPRIVATE-12)
#define IDC_L_SHOWALBUMNAME          (IDM_LASTPRIVATE-8)
#define IDC_L_SHOWYEAR               (IDM_LASTPRIVATE-5)
#define IDC_L_LAST                   (IDM_LASTPRIVATE-1) // range end


void SjCoverBrowser::OnContextMenuSelect(int id)
{
	if( id >= IDC_L_FIRST && id <= IDC_L_LAST )
	{
		long flags = g_mainFrame->m_libraryModule->GetFlags();
		if( id == IDC_L_SHOWLEADARTISTNAME )     { SjTools::ToggleFlag(flags, SJ_LIB_SHOWLEADARTISTNAME); }
		if( id == IDC_L_SHOWALBUMNAME )          { SjTools::ToggleFlag(flags, SJ_LIB_SHOWALBUMNAME); }
		if( id == IDC_L_SHOWYEAR )               { SjTools::ToggleFlag(flags, SJ_LIB_SHOWYEAR); }
		g_mainFrame->m_libraryModule->SetFlags(flags);
		g_mainFrame->m_browser->RefreshAll();
		UpdateItemsInColMenu(g_mainFrame->m_viewMenu);
	}
	else if( m_lastClickedCover )
	{
		m_lastClickedCover->m_rows[0]->OnContextMenu(id);
	}
}


wxRect SjCoverBrowser::GetToolTipRect(int mouseX, int mouseY)
{
	wxRect retRect;

	SjCol* cover;
	long found = FindCover(mouseX, mouseY, &cover, &retRect);
	if( found == FOUND_COVER || found == FOUND_COVER_ARROW )
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


wxString SjCoverBrowser::GetToolTipText(int mouseX, int mouseY, long &flags)
{
	wxString retString;

	SjCol* cover;
	long found = FindCover(mouseX, mouseY, &cover);
	if( found == FOUND_COVER )
	{
		if( cover != m_lastTooltipCover )
		{
			for( int r = 0; r < cover->m_rowCount; r++ )
			{
				SjRow* row = cover->m_rows[r];
				if(  row
				        && (row->m_roughType == SJ_RRTYPE_TITLE1 || row->m_roughType == SJ_RRTYPE_TITLE2) )
				{
					if( !retString.IsEmpty() ) retString += wxT("\n");
					retString += row->m_textm;
				}
			}
			m_lastTooltipCover = cover;
		}
	}
	else
	{
		if( found == FOUND_COVER_ARROW  )
		{
			retString = _("Show cover titles");
		}

		m_lastTooltipCover = NULL;
	}

	retString.Replace(wxT("\t"), wxT("")); // TAB is used for hiliting
	return retString;
}


void SjCoverBrowser::AddItemsToColMenu(SjMenu* m)
{
	m->AppendSeparator();
	m->AppendCheckItem(IDC_L_SHOWLEADARTISTNAME,     _("Artist"));
	m->AppendCheckItem(IDC_L_SHOWALBUMNAME,          _("Album"));
	m->AppendCheckItem(IDC_L_SHOWYEAR,               _("Year"));

	UpdateItemsInColMenu(m);
}


void SjCoverBrowser::UpdateItemsInColMenu(SjMenu* m)
{
	long flags = g_mainFrame->m_libraryModule->GetFlags();
	m->Check(IDC_L_SHOWLEADARTISTNAME,     (flags&SJ_LIB_SHOWLEADARTISTNAME)!=0);
	m->Check(IDC_L_SHOWALBUMNAME,          (flags&SJ_LIB_SHOWALBUMNAME)!=0);
	m->Check(IDC_L_SHOWYEAR,               (flags&SJ_LIB_SHOWYEAR)!=0);
}


long SjCoverBrowser::FindCover(long xPos, long yPos, SjCol** retCover, wxRect* retRect)
{
	int     c;
	SjCol*  col;

	yPos += m_scrollY;

	*retCover = NULL;
	wxRect dummy; if(retRect==NULL) retRect = &dummy;

	for( c = 0; c < m_allocatedCoverCount; c++ )
	{
		col = m_allocatedCover[c];
		if( col )
		{
			if( xPos >= col->m_textlLeft
			        && xPos < col->m_textlRight
			        && yPos >= col->m_top
			        && yPos < col->m_bottom )
			{
				*retCover = col;
				retRect->x = col->m_textlLeft;
				retRect->y = col->m_top - m_scrollY;
				retRect->width = g_mainFrame->m_currCoverWidth;
				retRect->height = m_coverNTitleHeight;
				return FOUND_COVER;
			}
		}
	}

	yPos -= m_scrollY;

	if( xPos >= 0
	        && xPos < SPACE_LEFT
	        && yPos >= SPACE_TOP
	        && yPos < SPACE_TOP+m_fontHeight
	        && g_mainFrame->IsOpAvailable(SJ_OP_TOGGLE_ELEMENTS) )
	{
		retRect->x = 0;
		retRect->y = SPACE_TOP;
		retRect->width = SPACE_LEFT;
		retRect->height = m_fontHeight;
		return FOUND_COVER_ARROW;
	}



	return FOUND_NOTHING;
}


/*******************************************************************************
 *  selection handling & scrolling
 ******************************************************************************/


bool SjCoverBrowser::OnSkinTargetEvent(int targetId, SjSkinValue& value, long accelFlags)
{
	bool shiftSelection = g_mainFrame->IsOpAvailable(SJ_OP_MULTI_ENQUEUE)? ((accelFlags&wxACCEL_SHIFT)!=0) : false;

	m_preservedVisible.Clear();

	//bool shiftSelection   = g_mainFrame->IsAllAvailable()? ((accelFlags&wxACCEL_SHIFT)!=0) : false;
	if( targetId >= IDT_WORKSPACE_GOTO_A && targetId <= IDT_WORKSPACE_GOTO_0_9 )
	{

		g_visModule->StopVisIfOverWorkspace();
		g_mainFrame->EndSimpleSearch();
		if( !GotoCover(g_mainFrame->m_columnMixer.GetMaskedColIndexByAz(targetId)) )
			CalcPositions(); // needed to reset a-z
		m_window->SetFocus();
		return true;
	}
	else switch( targetId )
		{
			case IDT_WORKSPACE_GOTO_RANDOM:
				g_visModule->StopVisIfOverWorkspace();
				g_mainFrame->EndSimpleSearch();
				GotoCover(SjTools::Rand(g_mainFrame->m_columnMixer.GetMaskedColCount()), true);
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

			case IDT_WORKSPACE_V_SCROLL:
			case IDT_WORKSPACE_LINE_UP:
			case IDT_WORKSPACE_LINE_DOWN:
			case IDT_WORKSPACE_PAGE_UP:
			case IDT_WORKSPACE_PAGE_DOWN:
			case IDT_WORKSPACE_HOME: // the search should stay "as is"
			case IDT_WORKSPACE_END:
				OnVScroll(targetId, value.value, TRUE /*redraw*/);
				return true;

			case IDT_WORKSPACE_PAGE_LEFT: // this is only accessible for explicit page left/right buttons as used in the "Old-style Jukebox"
				OnVScroll(IDT_WORKSPACE_PAGE_UP, 0, TRUE /*redraw*/);
				return true;

			case IDT_WORKSPACE_PAGE_RIGHT:  // this is only accessible for explicit page left/right buttons as used in the "Old-style Jukebox"
				OnVScroll(IDT_WORKSPACE_PAGE_DOWN, 0, TRUE /*redraw*/);
				return true;

			case IDT_WORKSPACE_ENTER:
				if( FindFirstSelectedCover() )
					ToggleView();
				return true;
		}

	return false;
}


bool SjCoverBrowser::OnVScroll(int nScrollCode, int nPos, bool redraw)
{
	wxASSERT( nScrollCode == IDT_WORKSPACE_V_SCROLL
	          || nScrollCode == IDT_WORKSPACE_LINE_UP
	          || nScrollCode == IDT_WORKSPACE_LINE_DOWN
	          || nScrollCode == IDT_WORKSPACE_PAGE_UP
	          || nScrollCode == IDT_WORKSPACE_PAGE_DOWN
	          || nScrollCode == IDT_WORKSPACE_HOME
	          || nScrollCode == IDT_WORKSPACE_END );

	long newIndex = m_applRowIndex, oldIndex, abs;
	bool canUseScroll;

	if( nScrollCode == IDT_WORKSPACE_HOME )
	{
		newIndex = 0;
	}
	else if( nScrollCode == IDT_WORKSPACE_END )
	{
		newIndex = m_applRowCount; // sth. substracted below
	}
	else if( nScrollCode == IDT_WORKSPACE_LINE_UP )
	{
		newIndex--;
	}
	else if( nScrollCode == IDT_WORKSPACE_LINE_DOWN )
	{
		newIndex++;
	}
	else if( nScrollCode == IDT_WORKSPACE_PAGE_UP )
	{
		newIndex -= m_coversYCount;
	}
	else if( nScrollCode == IDT_WORKSPACE_PAGE_DOWN )
	{
		newIndex += m_coversYCount;
	}
	else if( nScrollCode == IDT_WORKSPACE_V_SCROLL )
	{
		newIndex = nPos;
	}

	if( newIndex > m_applRowCount-m_coversYCount ) {
		newIndex = m_applRowCount-m_coversYCount;
		// may get below zero, this is checked below
	}

	if( newIndex < 0 ) {
		newIndex = 0;
	}

	if( (newIndex != m_applRowIndex)
	        || (newIndex == 0 && m_scrollY != 0) )
	{
		oldIndex = m_applRowIndex;
		canUseScroll = TRUE;

		if( newIndex == 0 ) {
			m_scrollY = 0;
			canUseScroll = FALSE;
		}

		m_applRowIndex = newIndex;
		CalcPositions();
		SetVScrollInfo();

		abs = oldIndex - newIndex;
		if( abs < 0 )
		{
			abs*=-1;
		}

		if( abs > m_coversYCount )
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
				wxRect scrollRect = SCROLL_RECT;
				m_window->ScrollWindow(0, (oldIndex - newIndex)*(m_coverNTitleHeight + SPACE_TOP), &scrollRect);

				m_window->Update();
			}
			else
			{
				m_window->Refresh();
			}

			return true;
		}
	}

	return false;
}


void SjCoverBrowser::SetVScrollInfo()
{
	// this function set the horzontal scrollbar information according to
	// the max. visible columns in bs->visibleColCount and to the
	// number of total columns in bs->applColCount

	SjSkinValue value;
	value.value         = m_applRowIndex;
	value.vmax          = m_applRowCount;
	value.thumbSize     = m_coversYCount;
	g_mainFrame->SetSkinTargetValue(IDT_WORKSPACE_V_SCROLL, value);
}


bool SjCoverBrowser::GotoCover(long offset, bool selectOneInRow)
{
	m_scrollY = 0;
	bool sthChanged = OnVScroll(IDT_WORKSPACE_V_SCROLL, offset/m_coversXCount, FALSE/*redraw*/);

	if( selectOneInRow ) // only used by "goto random"
	{
		int randMax = m_coversXCount;
		if( randMax > m_allocatedCoverCount ) randMax = m_allocatedCoverCount;

		SjCol* currCover = m_allocatedCover[SjTools::Rand(randMax)];
		if( currCover )
		{
			g_mainFrame->m_columnMixer.SelectAll(false);
			SelectAllRows(currCover);
		}
	}

	SetVScrollInfo();
	m_window->Refresh();
	m_window->Update();
	return sthChanged;
}


bool SjCoverBrowser::DoChangeSelection(long dir, bool shiftSelection)
{
	bool ret = false;
	int currCoverIndex, currCoverX, currCoverY;

	// get the currently selected cover
	SjCol* currCover = FindFirstSelectedCover(&currCoverIndex);
	if( currCover == NULL )
	{
		// no selected cover in view -> use the first one
		if( m_allocatedCoverCount <= 0 )
			goto Cleanup;
		currCoverIndex = 0;
		currCover = m_allocatedCover[0];
	}

	// calulate the x and the y position from the index
	if( m_coversXCount <= 0 )
		goto Cleanup;

	currCoverY = currCoverIndex / m_coversXCount;
	currCoverX = currCoverIndex % m_coversXCount;

	// correct the X and Y values to the new position
	if( dir == SJ_SEL_UP )
	{
		if(  currCoverY == 0
		        || (currCoverY == 1 && m_scrollY>30) )
		{
			OnVScroll(IDT_WORKSPACE_LINE_UP, 0, false);
		}
		else
		{
			currCoverY--;
		}
	}
	else if( dir == SJ_SEL_DOWN )
	{
		if( currCoverY == (m_coversYCount-1) )
		{
			OnVScroll(IDT_WORKSPACE_LINE_DOWN, 0, false);
		}
		else
		{
			currCoverY++;
		}
	}
	else if( dir == SJ_SEL_LEFT )
	{
		if( currCoverX == 0 )
			goto Cleanup;
		currCoverX--;
	}
	else if( dir == SJ_SEL_RIGHT )
	{
		if( currCoverX == (m_coversXCount-1) )
			goto Cleanup;
		currCoverX++;
	}
	else
	{
		goto Cleanup;
	}

	// calculate the new index from the X/Y values
	currCoverIndex = (currCoverY * m_coversXCount) + currCoverX;
	if( currCoverIndex < 0 || currCoverIndex >= m_allocatedCoverCount
	        || m_allocatedCover[currCoverIndex] == NULL )
		goto Cleanup;

	// do select the new cover
	currCover = m_allocatedCover[currCoverIndex];
	g_mainFrame->m_columnMixer.SelectAll(false);
	SelectAllRows(currCover);

	// success
	ret = true;

	// cleanup - always do a redraw as we do some scrolling and "errors" may occur afterwards
Cleanup:
	m_window->Refresh();
	m_window->Update();

	return ret;
}


bool SjCoverBrowser::GotoUrl(const wxString& url)
{
	bool    ret = FALSE;
	long    colIndex;
	SjCol*  col = g_mainFrame->m_columnMixer.GetMaskedCol(url, colIndex);
	if( col )
	{
		delete col;

		// check index
		col = NULL;
		if( colIndex >= 0 )
		{
			// goto column
			m_scrollY = 0;
			OnVScroll(IDT_WORKSPACE_V_SCROLL, colIndex/m_coversXCount, FALSE/*redraw*/);
			SetVScrollInfo();

			// select the correct row
			g_mainFrame->m_columnMixer.SelectAll(FALSE);
			for( colIndex = 0; colIndex < m_allocatedCoverCount; colIndex++ )
			{
				col = m_allocatedCover[colIndex];
				if( col )
				{
					for( int rowIndex = 0; rowIndex < col->m_rowCount; rowIndex++ )
					{
						SjRow* row = col->m_rows[rowIndex];
						if( row->m_url == url )
						{
							if( row->IsSelectable()==2 )
							{
								row->Select(TRUE);
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


SjCol* SjCoverBrowser::FindFirstSelectedCover(int* retIndex)
{
	for( int i = 0; i < m_allocatedCoverCount; i++ )
	{
		if( m_allocatedCover[i] )
		{
			if( m_allocatedCover[i]->FindFirstSelectedRow() )
			{
				if( retIndex ) *retIndex = i;
				return m_allocatedCover[i];
			}
		}
	}
	return NULL;
}


wxString SjCoverBrowser::GetFirstVisiblePos()
{
	if( !m_preservedVisible.IsEmpty() )
		return m_preservedVisible;

	if( m_allocatedCoverCount && m_allocatedCover[0] )
	{
		SjRow* row = m_allocatedCover[0]->FindSelectableRow();
		if( row )
			return row->m_url;
	}
	return wxEmptyString;
}


wxString SjCoverBrowser::GetFirstSelectedOrVisiblePos(long& retViewOffset)
{
	retViewOffset = 0;
	SjCol* cover = FindFirstSelectedCover();
	if( cover == NULL )
		return GetFirstVisiblePos();

	SjRow* row = cover->FindFirstSelectedRow();
	if( row )
		return row->m_url;

	return wxEmptyString;
}


void SjCoverBrowser::GotoPos(const wxString& guid, long viewOffset)
{
	m_preservedVisible = guid;

	long    colIndex;
	SjCol*  col = g_mainFrame->m_columnMixer.GetMaskedCol(guid, colIndex);
	if( col )
	{
		delete col;

		// check index
		col = NULL;
		if( colIndex >= 0 )
		{
			// goto column
			m_scrollY = 0;
			OnVScroll(IDT_WORKSPACE_V_SCROLL, colIndex/m_coversXCount, FALSE/*redraw*/);
			SetVScrollInfo();
		}
	}
}


/*******************************************************************************
 * Painting the window
 ******************************************************************************/


void SjCoverBrowser::CalcRowStuff()
{
	long minXSpace = g_mainFrame->m_currColumnXSpace;
	if( g_mainFrame->m_currColumnYSpace < g_mainFrame->m_currColumnXSpace )
		minXSpace = g_mainFrame->m_currColumnYSpace;

	// calculate the number of columns that can be displayed currently
	m_coversXCount = (m_window->m_clientW - SPACE_LEFT)
	                 / (g_mainFrame->m_currCoverWidth + minXSpace);
	if( m_coversXCount <= 0 )
		m_coversXCount = 1;

	m_coverXSpace = (m_window->m_clientW - SPACE_LEFT - (m_coversXCount*g_mainFrame->m_currCoverWidth))
	                / m_coversXCount;


	// calculate the number of rows that can be displayed currently
	m_coversYCount = (m_window->m_clientH - SPACE_TOP)
	                 / (m_coverNTitleHeight + SPACE_TOP);
	if( m_coversYCount <= 0 )
		m_coversYCount = 1;

	// calculate the current and the max. row index
	m_applRowCount = m_applCoverCount / m_coversXCount;
	if( (m_applCoverCount%m_coversXCount) != 0 )
		m_applRowCount++;
}


#define ADD_ROWS 2
void SjCoverBrowser::CalcPositions()
{
	// free all previously allocated objects
	FreeAllocatedCol();

	wxClientDC  dc(m_window);
	int dummy2;
	m_window->GetFontPxSizes(dc, m_fontVDiff, dummy2, m_fontHeight);

	m_coverNTitleHeight = g_mainFrame->m_currCoverHeight;
	m_linesBelowCover = 0;
	if( m_flags&SJ_BROWSER_VIEW_COVER )
	{
		m_coverNTitleHeight += m_fontHeight;
		m_linesBelowCover++;
		if( g_mainFrame->m_libraryModule
		        && g_mainFrame->m_libraryModule->DefTitleCount()>1 )
		{
			m_coverNTitleHeight += m_fontHeight;
			m_linesBelowCover++;
		}
	}

	CalcRowStuff();

	if( m_applRowIndex >= (m_applRowCount-m_coversYCount)
	        && m_applRowCount >= m_coversYCount )
		m_applRowIndex = (m_applRowCount-m_coversYCount); // m_applRowCount and m_coversYCount may have changed, cut large index

	// allocated the space to hold all columns
	long maxIndex = m_coversXCount * (m_coversYCount+ADD_ROWS/*add. row atop/abottom*/);
	long bytesNeeded = maxIndex * sizeof(SjCol*);
	m_allocatedCover = (SjCol**)malloc(bytesNeeded);
	memset(m_allocatedCover, 0, bytesNeeded);

	// get the columns and calculate the positions
	long currIndex = m_applRowIndex*m_coversXCount;
	long currRow = 0;
	long currCol = 0;
	int setAzTo = 'a';
	SjCol* col;
	while( 1 )
	{
		if( currIndex >= m_applCoverCount
		        || m_allocatedCoverCount >= maxIndex )
		{
			break; // no more columns
		}

		int top = g_mainFrame->m_currColumnYSpace + currRow*(m_coverNTitleHeight+g_mainFrame->m_currColumnYSpace);

		// if the y-position is out-of-view, we're done
		if( top >= m_window->m_clientH + m_scrollY )
		{
			break;
		}

		// allocate the columns
		col = g_mainFrame->m_columnMixer.GetMaskedCol(currIndex);
		m_allocatedCover[m_allocatedCoverCount++] = col;

		// calculate the column position
		if( col )
		{
			if( currRow==0 )
			{
				setAzTo = col->m_az;    // use the last letter of the first row; this is to avoid
				// selecting the "wrong" letter if the user clicks onto one
				// of the a-z buttons (remember, we do not use the exact cover offset!)
			}

			// calculate the top/bottom positions
			col->m_top = top;
			col->m_bottom = top + m_coverNTitleHeight;

			// calculate the left/right positions
			col->m_textlLeft = SPACE_LEFT + currCol*(g_mainFrame->m_currCoverWidth+m_coverXSpace);
			col->m_textlRight = col->m_textlLeft + g_mainFrame->m_currCoverWidth;
		}

		// prepare for next
		currIndex++;
		currCol++;
		if( currCol >= m_coversXCount )
		{
			currCol = 0;
			currRow++;
		}
	}

	g_mainFrame->SetSkinAzValues(setAzTo);
}


void SjCoverBrowser::OnSize(wxSizeEvent& event)
{
	CalcPositions();
	SetVScrollInfo();
}


void SjCoverBrowser::OnImageThere(SjImageThereEvent& event)
{
	SjImgThreadObj* obj = event.GetObj();
	int             c, r;
	SjCol*          applCol;
	SjRow*          applRow;

	wxASSERT(obj);

	for( c = 0; c < m_allocatedCoverCount; c++ )
	{
		applCol = m_allocatedCover[c];
		if( applCol )
		{
			for( r = 0; r < applCol->m_rowCount; r++ )
			{
				applRow = applCol->m_rows[r];
				if( applRow->m_roughType == SJ_RRTYPE_COVER )
				{
					if( obj->m_url == applRow->m_textm )
					{
						wxClientDC dc(m_window);

						m_window->PaintCover(dc, obj,
						                     applCol->m_textlLeft,
						                     applCol->m_top - m_scrollY);
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


void SjCoverBrowser::DoPaint(wxDC& dc)
{
	long    x, y, w, h;
	#define SPACE_BRUSH g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgBrush

	// reset required and waiting images
	g_mainFrame->m_imgThread->RequireStart(m_window);

	// draw stuff very left (does not scroll)
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(SPACE_BRUSH);
	dc.DrawRectangle(0, 0, SPACE_LEFT, m_window->m_clientH);

	dc.SetClippingRegion(wxRect(0, 0, SPACE_LEFT-SEL_BORDER_W, m_window->m_clientH));
	w = m_fontHeight - 4;
	h = m_fontHeight - 4;
	x = (SPACE_LEFT-w) / 2;
	y = SPACE_TOP + 2;
	dc.SetPen(g_mainFrame->m_workspaceColours[SJ_COLOUR_TITLE1].fgPen);
	SjTools::DrawIcon(dc, wxRect(x, y, w, h),
	                  (m_flags&SJ_BROWSER_VIEW_COVER)? SJ_DRAWICON_TRIANGLE_DOWN : SJ_DRAWICON_TRIANGLE_RIGHT);

	if( g_mainFrame->m_libraryModule )
		m_window->DrawUpText(dc, g_mainFrame->m_libraryModule->GetUpText(), 0, SPACE_TOP+m_fontHeight, SPACE_LEFT, m_window->m_clientH-(SPACE_TOP+m_fontHeight));
	dc.DestroyClippingRegion();

	// draw space atop of all covers
	long currY = -m_scrollY;
	dc.SetBrush(SPACE_BRUSH);
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(SPACE_LEFT, currY, m_window->m_clientW, g_mainFrame->m_currColumnYSpace);

	// go through all cover rows
	int coverIndex = 0;
	currY += g_mainFrame->m_currColumnYSpace;
	for( int yIndex = 0; yIndex < m_coversYCount+ADD_ROWS/*try two rows more*/; yIndex++ )
	{
		// go through all cover columns
		long currX = SPACE_LEFT;
		for( int xIndex = 0; xIndex < m_coversXCount; xIndex++ )
		{
			bool selectCover = false, coverDrawn = false, borderBottom = true;

			if( coverIndex < m_allocatedCoverCount )
			{
				SjCol* cover = m_allocatedCover[coverIndex++];
				if( cover && cover->m_rowCount>0 && cover->m_rows[0]->m_roughType==SJ_RRTYPE_COVER )
				{
					SjImgThreadObj* cachedImg = m_window->RequireImage(cover->m_rows[0]->m_textm); // cachedImg may be NULL, but this is okay for PaintCover() and for ReleaseImage()
					m_window->PaintCover(dc, cachedImg, currX, currY);
					g_mainFrame->m_imgThread->ReleaseImage(m_window, cachedImg);
					coverDrawn = true;
					selectCover = cover->FindFirstSelectedRow()!=NULL;

					// draw text below cover
					if( m_flags&SJ_BROWSER_VIEW_COVER )
					{
						// get text
						wxString texts[2];
						int trackCount = 0;
						for( int r = 0; r < cover->m_rowCount; r++ )
						{
							SjRow* row = cover->m_rows[r];
							if( row )
							{
								if( row->m_roughType == SJ_RRTYPE_TITLE1 || row->m_roughType == SJ_RRTYPE_TITLE2 )
								{
									if( texts[0].IsEmpty() ) texts[0] = row->m_textm; else texts[1] = row->m_textm;
								}
								else if( row->m_roughType == SJ_RRTYPE_NORMAL )
								{
									trackCount ++;
								}
							}
						}

						if( texts[0].IsEmpty() ) {
							texts[0].Printf(_("%s tracks"), SjTools::FormatNumber(trackCount).c_str());
						}

						// draw text
						for( int i=0; i<m_linesBelowCover; i++ )
						{
							wxRect drawRect(currX, currY+g_mainFrame->m_currCoverHeight+m_fontHeight*i, g_mainFrame->m_currCoverWidth, m_fontHeight);

							SjSkinColour* bgcolour = &g_mainFrame->m_workspaceColours[selectCover? SJ_COLOUR_SELECTIONODD : SJ_COLOUR_NORMAL];
							SjSkinColour* fgcolour = &g_mainFrame->m_workspaceColours[selectCover? SJ_COLOUR_SELECTIONODD : (i==0?SJ_COLOUR_TITLE1:SJ_COLOUR_TITLE2)];

							wxFont* stdFont   = i==0? &g_mainFrame->m_currBoldFont : &g_mainFrame->m_currStdFont;
							wxFont* smallFont = i==0? &g_mainFrame->m_currSmallBoldFont : &g_mainFrame->m_currSmallFont;

							dc.SetPen(*wxTRANSPARENT_PEN);
							dc.SetBrush(bgcolour->bgBrush);
							dc.DrawRectangle(drawRect);

							dc.SetTextBackground(fgcolour->bgColour);
							dc.SetTextForeground(fgcolour->fgColour);
							g_tools->DrawSingleLineText(dc, texts[i], drawRect,
							                            *stdFont, *smallFont, NULL, fgcolour->hiColour);

							if( selectCover )
								borderBottom = false;
						}
					}
				}
			}

			if( !coverDrawn )
			{
				// draw space instead of cover (normally this happens if this are the last covers)
				dc.SetPen(*wxTRANSPARENT_PEN);
				dc.SetBrush(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgBrush);
				dc.DrawRectangle(currX-SEL_BORDER_W, currY-SEL_BORDER_W, g_mainFrame->m_currCoverWidth+SEL_BORDER_W*2, m_coverNTitleHeight+SEL_BORDER_W*2);
			}
			else
			{
				// draw the (selection) border
				SjSkinColour* colour = &g_mainFrame->m_workspaceColours[selectCover? SJ_COLOUR_SELECTIONODD : SJ_COLOUR_NORMAL];

				dc.SetPen(colour->bgPen);
				dc.SetBrush(*wxTRANSPARENT_BRUSH);
				for( int i = 1; i <= SEL_BORDER_W; i++ )
				{
					dc.DrawRectangle(currX-i, currY-i, g_mainFrame->m_currCoverWidth+i*2, m_coverNTitleHeight+i*(borderBottom?2:1));
					if( !borderBottom )
					{
						dc.SetPen(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgPen);
						y = currY+m_coverNTitleHeight+i-1;
						dc.DrawLine(currX-SEL_BORDER_W, y, currX+g_mainFrame->m_currCoverWidth+SEL_BORDER_W, y);
						dc.SetPen(colour->bgPen);
					}
				}
			}

			// draw space aright of the cover
			x = currX+g_mainFrame->m_currCoverWidth+SEL_BORDER_W;
			y = currY-SEL_BORDER_W;
			h = m_coverNTitleHeight+SEL_BORDER_W*2;
			w = m_coverXSpace-SEL_BORDER_W*2;
			if( xIndex == m_coversXCount-1 )
				w = m_window->m_clientW-(currX+g_mainFrame->m_currCoverWidth+SEL_BORDER_W);
			if( w > 0 )
			{
				dc.SetPen(*wxTRANSPARENT_PEN);
				dc.SetBrush(SPACE_BRUSH);
				dc.DrawRectangle(x, y, w, h);
			}

			// next cover in row
			currX += g_mainFrame->m_currCoverWidth+m_coverXSpace;
		}

		// draw space below this cover row
		x = SPACE_LEFT-SEL_BORDER_W;
		y = currY+m_coverNTitleHeight+SEL_BORDER_W;
		w = m_window->m_clientW-x;
		h = g_mainFrame->m_currColumnYSpace-SEL_BORDER_W*2;
		if( yIndex == (m_coversYCount-1)+ADD_ROWS /*we've added 2 in the for() loop, so no -1 here*/ )
			h = m_window->m_clientH-(currY+m_coverNTitleHeight+SEL_BORDER_W);
		if( h > 0 )
		{
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.SetBrush(SPACE_BRUSH);
			dc.DrawRectangle(x, y, w, h);
		}

		// next row
		currY += m_coverNTitleHeight+g_mainFrame->m_currColumnYSpace;
	}

	// free all images that are no longer required but still waiting
	g_mainFrame->m_imgThread->RequireEnd(m_window);
}
