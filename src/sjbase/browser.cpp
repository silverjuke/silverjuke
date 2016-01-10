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
 * File:    browser.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke main browser
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/imgthread.h>
#include <sjbase/browser.h>
#include <sjbase/browser_album.h>
#include <sjbase/browser_cover.h>
#include <sjbase/browser_list.h>
#include <sjmodules/kiosk/virtkeybd.h>
#include <sjbase/columnmixer.h>


BEGIN_EVENT_TABLE(SjBrowserWindow, wxWindow)
	EVT_PAINT               (   SjBrowserWindow::OnPaint            )
	EVT_IMAGE_THERE         (   SjBrowserWindow::OnImageThere       )
	EVT_ERASE_BACKGROUND    (   SjBrowserWindow::OnEraseBackground  )
	EVT_SIZE                (   SjBrowserWindow::OnSize             )
	EVT_LEFT_DOWN           (   SjBrowserWindow::OnMouseLeftDown    )
	EVT_LEFT_UP             (   SjBrowserWindow::OnMouseLeftUp      )
	EVT_MOUSE_CAPTURE_LOST  (   SjBrowserWindow::OnMouseCaptureLost )
	EVT_LEFT_DCLICK         (   SjBrowserWindow::OnMouseLeftDClick  )
	EVT_ENTER_WINDOW        (   SjBrowserWindow::OnMouseEnter       )
	EVT_MOTION              (   SjBrowserWindow::OnMouseMotion      )
	EVT_MOUSEWHEEL          (   SjBrowserWindow::OnMouseWheel       )
	EVT_CONTEXT_MENU        (   SjBrowserWindow::OnMouseRight       )
	EVT_MIDDLE_UP           (   SjBrowserWindow::OnMouseMiddleUp    )
	EVT_KEY_DOWN            (   SjBrowserWindow::OnKeyDown          )
END_EVENT_TABLE()


void SjBrowserWindow::OnMouseLeftDown(wxMouseEvent& event)
{
	if( g_accelModule == NULL || m_currView == NULL ) return;

	g_mainFrame->GotBrowserInputFromUser();
	g_mainFrame->m_display.ClearDisplaySelection();

	/* capture mouse
	 */
	#if SJ_USE_TOOLTIPS
		g_tools->m_toolTipManager.ClearToolTipProvider();
	#endif
	SetFocus();
	CaptureMouse();

	/* give the event to the current view
	 */
	m_currView->OnMouseLeftDown(event);
}


void SjBrowserWindow::OnMouseLeftUp(wxMouseEvent& event)
{
	if( g_accelModule == NULL || m_currView == NULL ) return;

	g_mainFrame->GotBrowserInputFromUser();

	// drag'n'drop: end (wxDragImage uses its own mouse capturing)
	if( m_mouseAction == SJ_ACTION_DRAGNDROP )
	{
		m_mouseAction = SJ_ACTION_NONE;
		g_mainFrame->DragNDrop(SJ_DND_DROP, this, event.GetPosition(), NULL, &m_dragUrls);
		return; // done
	}

	// release the mouse if it was captured before
	if( HasCapture() )
	{
		ReleaseMouse();
	}
	else
	{
		return; // mouse was not captured
	}

	/* give the event to the current view
	 */
	m_currView->OnMouseLeftUp(event);
}


void SjBrowserWindow::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
	if( g_accelModule == NULL || m_currView == NULL ) return;

	wxLogDebug(wxT("~~~~~~~~~~~~~~~~~~~~~~~~~~ CAPTURE LOST!"));
	wxASSERT( m_mouseAction != SJ_ACTION_DRAGNDROP ); // should not be true - capture is handled by the Drag'n'Drop classes themselves!

	m_currView->OnMouseCaptureLost(event);
}


void SjBrowserWindow::OnMouseLeftDClick(wxMouseEvent& event)
{
	if( g_accelModule == NULL || m_currView == NULL ) return;

	g_mainFrame->GotBrowserInputFromUser();

	/* give the event to the current view
	 */
	if( !m_currView->OnMouseLeftDClick(event) )
	{
		// click not used -> toggle view
		g_mainFrame->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDT_WORKSPACE_TOGGLE_VIEW));
	}
}


void SjBrowserWindow::OnMouseEnter(wxMouseEvent& event)
{
	// if the mouse enters this window, send a mouse leave
	// message to the parent window
	GetParent()->GetEventHandler()->QueueEvent(new wxMouseEvent(wxEVT_LEAVE_WINDOW));

	if( !HasCapture() )
		ResetCursor();
}


void SjBrowserWindow::ResetCursor()
{
	if( m_cursorChanged )
	{
		SetCursor(SjVirtKeybdModule::GetStandardCursor());
		m_cursorChanged = false;
	}
}


void SjBrowserWindow::OnMouseMotion(wxMouseEvent& event)
{
	if( g_accelModule == NULL || m_currView == NULL ) return;

	if( !HasCapture() )
	{
		#if SJ_USE_TOOLTIPS
			m_toolTipProvider.m_xPos = event.GetX();
			m_toolTipProvider.m_yPos = event.GetY();
			g_tools->m_toolTipManager.SetToolTipProvider(&m_toolTipProvider);
		#endif
		m_currView->OnMouseMotion(event);
		return;
	}

	#if SJ_USE_TOOLTIPS
		g_tools->m_toolTipManager.ClearToolTipProvider();
	#endif

	g_mainFrame->GotBrowserInputFromUser();

	/* give the event to the current view
	 */
	m_currView->OnMouseMotion(event);
}


static void simulateSlider(long zDelta, long wheelDelta, int targetUp, int targetDown)
{
	SjSkinValue dummy;
	long i, cnt = (zDelta>0? zDelta : (zDelta*-1))/wheelDelta;
	if( cnt<1 ) cnt = 1; if( cnt > 3 ) cnt = 3;
	for( i = 0; i < cnt; i++ )
	{
		g_mainFrame->OnSkinTargetEvent(zDelta>0?targetUp:targetDown, dummy, 0);
	}
}


void SjBrowserWindow::OnMouseWheel(wxMouseEvent& event)
{
	if( g_accelModule == NULL || m_mouseAction == SJ_ACTION_DRAGNDROP
	 || m_mouseAction == SJ_ACTION_COLUMNWIDTH || m_mouseAction == SJ_ACTION_COLUMNMOVE || m_currView == NULL ) return;

	#if SJ_USE_TOOLTIPS
		g_tools->m_toolTipManager.ClearToolTipProvider();
	#endif

	long zDelta = event.GetWheelRotation();
	long wheelDelta = event.GetWheelDelta();

	// is the mouse over the display? over the volume slider?

	int mouseX, mouseY;
	::wxGetMousePosition(&mouseX, &mouseY);
	g_mainFrame->ScreenToClient(&mouseX, &mouseY);
	int targetId = g_mainFrame->FindTargetId(mouseX, mouseY);

	if( g_accelModule->m_flags&SJ_ACCEL_CONTEXT_SENSITIVE_WHEEL )
	{
		if( (targetId>=IDT_DISPLAY_LINE_FIRST && targetId<=IDT_DISPLAY_LINE_LAST)
		 ||  targetId==IDT_DISPLAY_UP
		 ||  targetId==IDT_DISPLAY_DOWN
		 ||  targetId==IDT_DISPLAY_V_SCROLL )
		{
			simulateSlider(zDelta, wheelDelta, IDT_DISPLAY_UP, IDT_DISPLAY_DOWN);
			return;
		}
		else if( targetId==IDT_MAIN_VOL_UP
			  || targetId==IDT_MAIN_VOL_DOWN
		      || targetId==IDT_MAIN_VOL_SLIDER
		      || targetId==IDT_MAIN_VOL_MUTE )
		{
			simulateSlider(zDelta, wheelDelta, IDT_MAIN_VOL_UP, IDT_MAIN_VOL_DOWN);
			return;
		}
		else if( (targetId>=IDT_WORKSPACE_GOTO_A && targetId<=IDT_WORKSPACE_GOTO_0_9)
		      ||  targetId==IDT_WORKSPACE_GOTO_PREV_AZ
		      ||  targetId==IDT_WORKSPACE_GOTO_NEXT_AZ )
		{
			// added for 2.10, see http://www.silverjuke.net/forum/post.php?p=2720#2720
			SjSkinValue dummy;
			g_mainFrame->OnSkinTargetEvent(zDelta>0?IDT_WORKSPACE_GOTO_PREV_AZ:IDT_WORKSPACE_GOTO_NEXT_AZ, dummy, 0);
			return;
		}
		else if( targetId == IDT_WORKSPACE_H_SCROLL
		      || targetId == IDT_WORKSPACE_LINE_LEFT
		      || targetId == IDT_WORKSPACE_LINE_RIGHT
		      || targetId == IDT_WORKSPACE_PAGE_LEFT
		      || targetId == IDT_WORKSPACE_PAGE_RIGHT )
		{
			SjSkinValue dummy;
			g_mainFrame->OnSkinTargetEvent(zDelta>0?IDT_WORKSPACE_PAGE_LEFT:IDT_WORKSPACE_PAGE_RIGHT, dummy, 0);
			return;
		}
		else if( targetId == IDT_WORKSPACE_V_SCROLL
		      || targetId == IDT_WORKSPACE_LINE_UP
		      || targetId == IDT_WORKSPACE_LINE_DOWN
		      || targetId == IDT_WORKSPACE_PAGE_UP
		      || targetId == IDT_WORKSPACE_PAGE_DOWN )
		{
			SjSkinValue dummy;
			g_mainFrame->OnSkinTargetEvent(zDelta>0?IDT_WORKSPACE_PAGE_UP:IDT_WORKSPACE_PAGE_DOWN, dummy, 0);
			return;
		}
	}

	// find out scrolling orientation

	g_mainFrame->GotBrowserInputFromUser();

	bool scrollVert = event.GetWheelAxis() == wxMOUSE_WHEEL_HORIZONTAL? false : true;

	if( event.ShiftDown()
	 || event.ControlDown()
	 || event.RightIsDown() )
	{
		// toggle direction if shift, ctrl or the right mouse button is down
		scrollVert = !scrollVert;
	}
	else if( g_accelModule->m_flags&SJ_ACCEL_CONTEXT_SENSITIVE_WHEEL ) // if "scroll display" is false, the user seems not to like the scroll wheel to react to mouse positions
	{
		// regard the scrollbars directions if the mouse is over them
		if( targetId == IDT_WORKSPACE_H_SCROLL
		 || targetId == IDT_WORKSPACE_LINE_LEFT
		 || targetId == IDT_WORKSPACE_LINE_RIGHT
		 || targetId == IDT_WORKSPACE_PAGE_LEFT
		 || targetId == IDT_WORKSPACE_PAGE_RIGHT )
		{
			scrollVert = FALSE;
		}
		else if( targetId == IDT_WORKSPACE_V_SCROLL
			  || targetId == IDT_WORKSPACE_LINE_UP
		      || targetId == IDT_WORKSPACE_LINE_DOWN
		      || targetId == IDT_WORKSPACE_PAGE_UP
		      || targetId == IDT_WORKSPACE_PAGE_DOWN )
		{
			scrollVert = TRUE;
		}
	}

	/* give the event to the current view
	 */
	m_currView->OnMouseWheel(event, scrollVert);

	if( event.RightIsDown() )
	{
		m_skipContextMenuEvent = 1; // skip WM_RBUTTONUP and WM_CONTEXTMENU
	}
}


void SjBrowserWindow::OnMouseRight(wxContextMenuEvent& event)
{
	#if SJ_USE_TOOLTIPS
		g_tools->m_toolTipManager.ClearToolTipProvider();
	#endif

	if( m_mouseAction == SJ_ACTION_DRAGSCROLL || m_mouseAction == SJ_ACTION_DRAGNDROP || m_mouseAction == SJ_ACTION_COLUMNWIDTH || m_mouseAction == SJ_ACTION_COLUMNMOVE || m_currView == NULL ) { return;  }

	g_mainFrame->GotBrowserInputFromUser();
	g_mainFrame->m_display.ClearDisplaySelection();

	if( m_skipContextMenuEvent )
	{
		m_skipContextMenuEvent = 0;
		return; // no right button up this pass
	}

	/* give the event to the current view
	 */
	wxPoint clickPoint = ScreenToClient(event.GetPosition());
	m_currView->OnContextMenu(clickPoint.x, clickPoint.y);
}
void SjBrowserWindow::OnModuleUserId(int id)
{
	m_currView->OnContextMenuSelect(id);
}


void SjBrowserWindow::OnMouseMiddleUp(wxMouseEvent& event)
{
	#if SJ_USE_TOOLTIPS
		g_tools->m_toolTipManager.ClearToolTipProvider();
	#endif

	if( m_mouseAction == SJ_ACTION_DRAGSCROLL || m_mouseAction == SJ_ACTION_DRAGNDROP || m_mouseAction == SJ_ACTION_COLUMNWIDTH || m_mouseAction == SJ_ACTION_COLUMNMOVE || m_currView == NULL ) { return;  }

	g_mainFrame->GotBrowserInputFromUser();
	g_mainFrame->m_display.ClearDisplaySelection();

	/* give the event to the current view
	 */
	m_currView->OnMouseMiddleUp(event);
}


void SjBrowserWindow::OnKeyDown(wxKeyEvent& event)
{
	if( g_accelModule == NULL || m_mouseAction == SJ_ACTION_DRAGNDROP || m_mouseAction == SJ_ACTION_COLUMNWIDTH || m_mouseAction == SJ_ACTION_COLUMNMOVE ) { return; }
	#if SJ_USE_TOOLTIPS
		g_tools->m_toolTipManager.ClearToolTipProvider();
	#endif

	long accelFlags = (event.ShiftDown()? wxACCEL_SHIFT : 0);
	int targetId = g_accelModule->KeyEvent2CmdId(event, SJA_MAIN);
	if( targetId == 0 && accelFlags )
	{
		// try again without shift
		wxKeyEvent eventWithoutShift = event;
		eventWithoutShift.m_shiftDown = false;
		targetId = g_accelModule->KeyEvent2CmdId(eventWithoutShift, SJA_MAIN);
	}

	if( targetId )
	{
		SjSkinValue dummy;
		g_mainFrame->OnSkinTargetEvent(targetId, dummy, accelFlags);
	}
	else
	{
		event.Skip();
	}
	// from here ...
}
void SjBrowserWindow::OnSkinTargetEvent (int targetId, SjSkinValue& value, long accelFlags)
{
	// ... we may get here eg. for cursor presses
	g_mainFrame->GotBrowserInputFromUser();
	g_mainFrame->m_display.ClearDisplaySelection();

	if( targetId == IDT_WORKSPACE_TOGGLE_VIEW )
	{
		// toggle the view
		for( int test = 1; test <= SJ_BROWSER_VIEW_COUNT; test ++ )
		{
			int newView = (GetView()+test)%SJ_BROWSER_VIEW_COUNT;
			if( IsViewAvailable(newView) )
			{
				SetView_(newView, true, true);
				break;
			}
		}
	}
	else if( targetId == IDT_WORKSPACE_ALBUM_VIEW )
	{
		// if the view is not available, nothing happens
		SetView_(SJ_BROWSER_ALBUM_VIEW, true, true);
	}
	else if( targetId == IDT_WORKSPACE_COVER_VIEW )
	{
		// if the view is not available, nothing happens
		SetView_(SJ_BROWSER_COVER_VIEW, true, true);
	}
	else if( targetId == IDT_WORKSPACE_LIST_VIEW )
	{
		// if the view is not available, nothing happens
		SetView_(SJ_BROWSER_LIST_VIEW, true, true);
	}
	else if( m_currView )
	{
		if( targetId == IDT_WORKSPACE_SHOW_COVERS )
		{
			// toggle covers
			if( g_mainFrame->IsOpAvailable(SJ_OP_TOGGLE_ELEMENTS) )
			{
				SjTools::ToggleFlag(m_currView->m_flags, SJ_BROWSER_VIEW_COVER);
				RefreshAll();

				if( g_mainFrame->m_viewMenu )
				{
					g_mainFrame->m_viewMenu->Check(IDT_WORKSPACE_SHOW_COVERS, AreCoversShown());
				}
			}
		}
		else
		{
			// forward to the current view
			if( !m_currView->OnSkinTargetEvent(targetId, value, accelFlags) )
			{
				if( targetId == IDT_WORKSPACE_ENTER )
				{
					// "enter" not used by the current view, play/enqueue the files
					int action = IDT_ENQUEUE_LAST;
					if(  g_mainFrame->IsOpAvailable(SJ_OP_EDIT_QUEUE)
					 && (g_accelModule->m_flags&SJ_ACCEL_PLAY_NOW_ON_DBL_CLICK) )
					{
						action = IDT_ENQUEUE_NOW;
					}
					SjSkinValue v;
					g_mainFrame->OnSkinTargetEvent(action, v, accelFlags);
				}
				else if( targetId == IDT_WORKSPACE_GOTO_PREV_AZ || targetId == IDT_WORKSPACE_GOTO_NEXT_AZ )
				{
					// default handling for "goto prev/next letter"

					// find out the current letter selection
					int initialId   = -1;
					int thisTestId  = -1;
					int thisTestDir = (targetId == IDT_WORKSPACE_GOTO_PREV_AZ)? -1 : +1;
					for( int i = IDT_WORKSPACE_GOTO_A; i <= IDT_WORKSPACE_GOTO_0_9; i ++ )
					{
						if( g_mainFrame->GetSkinTargetValue(i).value )
						{
							initialId  = i;
							thisTestId = i + thisTestDir;
							break;
						}
					}

					if( thisTestId == -1 )
					{
						thisTestId  = IDT_WORKSPACE_GOTO_A;
						thisTestDir = +1;
					}

					// test test loop
					SjSkinValue fwd;
					fwd.value    = 1;
					while( 1 )
					{
						if( thisTestId < IDT_WORKSPACE_GOTO_A || thisTestId > IDT_WORKSPACE_GOTO_0_9 )
							break; // done

						m_currView->OnSkinTargetEvent(thisTestId, fwd, 0);
						if( initialId == -1 || !g_mainFrame->GetSkinTargetValue(initialId).value )
							break; // done - the original id is no longer selected

						thisTestId += thisTestDir;
					}
				}
				else if( targetId == IDT_ZOOM_IN || targetId == IDT_ZOOM_OUT || targetId == IDT_ZOOM_NORMAL )
				{
					g_mainFrame->GotBrowserInputFromUser();

					if( g_mainFrame->IsOpAvailable(SJ_OP_ZOOM) )
					{
						SetZoom_(targetId);
					}
				}
			}
		}
	}
}


void SjBrowserWindow::OnPaint(wxPaintEvent& event)
{
	wxPaintDC   dc(this);
	bool        drawStub = false;
	wxString    stubText;

	// anything to draw?
	if( g_mainFrame->m_libraryModule )
	{
		if( g_mainFrame->m_columnMixer.GetMaskedColCount() == 0 )
		{
			drawStub = true;
			if(  g_mainFrame->m_libraryModule->GetUnmaskedTrackCount() <= 0
			 || !g_mainFrame->HasAnySearch() )
			{
				// the condition above is not perfect, however it works in most cases
				// (not if the windows is redrawn during the _first_ update with any search set)
				stubText = _("Drag folders with music here.");
			}
			else
			{
				stubText = g_mainFrame->GetSkinTargetValue(IDT_SEARCH_INFO).string;
			}
		}
	}
	else
	{
		drawStub = true;
	}

	if( m_currView == NULL )
		drawStub = true;

	if( drawStub )
	{
		// draw the background
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgBrush);
		dc.DrawRectangle(0, 0, m_clientW, m_clientH);

		// get text to show - check if the text is not empty (the text is empty
		// if the library modules is not yet available; in this case, also the fonts are not yet set)
		if( stubText.Len()  )
		{
			// get font
			wxFont font = g_mainFrame->m_currBoldFont;
			font.SetPointSize(font.GetPointSize()*2);
			dc.SetFont(font);

			// calculate the text position
			wxCoord x, y, w, h;
			dc.GetTextExtent(stubText, &w, &h);

			x = m_clientW/2 - w/2; if( x<8 ) x = 8;
			y = m_clientH/2 - h/2; if( y<8 ) y = 8;

			// draw the text
			dc.SetTextBackground(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgColour);
			dc.SetTextForeground(g_mainFrame->m_workspaceColours[g_mainFrame->m_workspaceColours[SJ_COLOUR_STUBTEXT].fgSet? SJ_COLOUR_STUBTEXT : SJ_COLOUR_NORMAL].fgColour);
			dc.DrawText(stubText, x, y);
		}
	}
	else
	{
		// forward to the module
		m_currView->DoPaint(dc);
	}
}


void SjBrowserWindow::OnImageThere(SjImageThereEvent& event)
{
	// forward to the current view
	if( m_currView )
		m_currView->OnImageThere(event);
}


void SjBrowserWindow::OnSize(wxSizeEvent& event)
{
	wxSize clientSize = GetClientSize();

	m_clientW = clientSize.x;
	if( m_clientW < 0 /*seen on OS X*/ )
	{
		m_clientW = 0;
	}

	m_clientH = clientSize.y;
	if( m_clientH < 0 /*seen on OS X*/ )
	{
		m_clientH = 0;
	}

	if( m_currView )
		m_currView->OnSize(event);
}


bool SjBrowserWindow::IsVScrollAbottom()
{
	const SjSkinValue& value = g_mainFrame->GetSkinTargetValue(IDT_WORKSPACE_V_SCROLL);

	if( value.value + value.thumbSize >= value.vmax )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


void SjBrowserWindow::DropImage(SjDataObject* data, int mouseX, int mouseY)
{
	if( m_currView )
		m_currView->OnDropImage(data, mouseX, mouseY);
}


#if SJ_USE_TOOLTIPS
wxString SjBrowserToolTipProvider::GetText(long& flags)
{
	if( m_browserWindow->m_currView )
		return m_browserWindow->m_currView->GetToolTipText(m_xPos, m_yPos, flags);

	return wxT("");
}
wxRect SjBrowserToolTipProvider::GetLocalRect()
{
	if( m_browserWindow->m_currView )
		return m_browserWindow->m_currView->GetToolTipRect(m_xPos, m_yPos);

	return wxRect(-1000, -1000, 1, 1);
}
#endif


bool SjBrowserWindow::GotoUrl(const wxString& url)
{
	return m_currView? m_currView->GotoUrl(url) : false;
}


void SjBrowserWindow::GotoPos(const wxString& guid, long viewOffset)
{
	if( m_currView )
		m_currView->GotoPos(guid, viewOffset);
}


wxString SjBrowserWindow::GetFirstVisiblePos()
{
	if( m_currView )
		return m_currView->GetFirstVisiblePos();

	return wxEmptyString;
}


wxString SjBrowserWindow::GetFirstSelectedOrVisiblePos(long& retViewOffset)
{
	if( m_currView )
		return m_currView->GetFirstSelectedOrVisiblePos(retViewOffset);

	return wxEmptyString;
}


bool SjBrowserWindow::ChangeSelection(long dir, bool shiftSelection)
{
	if( m_currView )
		return m_currView->DoChangeSelection(dir, shiftSelection);

	return false;
}


void SjBrowserWindow::ReloadColumnMixer(bool keepColIndex)
{
	// this function reloads the SjColumnMixer depending stuff
	// and updates the window. an extra call to ReloadView()
	// is not needed!

	for( int i = 0; i < SJ_BROWSER_VIEW_COUNT; i++ )
	{
		if( m_currView == m_views[i] )
		{
			m_currView->Realize(true, keepColIndex);
			m_currView->__needsColumnMixerReload = false;
		}
		else
		{
			m_views[i]->__needsColumnMixerReload = true;
		}
	}

	Refresh(); // no Update() - this avoids flickering if eg. GotoColumn() is
	// called just after ReloadColumnMixer()
}


void SjBrowserWindow::RefreshAll()
{
	// this function should be called to update the browser window contents,
	// the SjColumnMixer depending stuff IS NOT reloaded, use
	// ReloadColumnMixer() for this purpose.

	if( m_currView )
	{
		m_currView->Realize(m_currView->__needsColumnMixerReload, true /*keep col index*/);
		m_currView->__needsColumnMixerReload = false;
	}

	Refresh();
	Update();
}


void SjBrowserWindow::RefreshSelection()
{
	if( m_currView )
	{
		m_currView->RefreshSelection();
	}
}


int SjBrowserWindow::GetView() const
{
	for( int i = 0; i < SJ_BROWSER_VIEW_COUNT; i++ )
	{
		if( m_views[i] == m_currView )
		{
			return i;
		}
	}

	return 0; // may happen on startup (eg. if called by a script or implicit via InitMainMenu())
}


bool SjBrowserWindow::IsViewAvailable(int i)
{
	if( (i == SJ_BROWSER_ALBUM_VIEW && !g_mainFrame->IsOpAvailable(SJ_OP_ALBUM_VIEW))
	 || (i == SJ_BROWSER_COVER_VIEW && !g_mainFrame->IsOpAvailable(SJ_OP_COVER_VIEW))
	 || (i == SJ_BROWSER_LIST_VIEW  && !g_mainFrame->IsOpAvailable(SJ_OP_LIST_VIEW)) )
	{
		return false;
	}

	return true;
}


void SjBrowserWindow::SetView_(int i, bool keepPosition, bool redraw)
{
	wxString        viewPos;
	long            viewOffset = 0, j;
	SjSkinValue value;

	// corect index
	if( i < 0 || i >= SJ_BROWSER_VIEW_COUNT )
	{
		i = SJ_BROWSER_ALBUM_VIEW;
	}

	// is the requested view available? anything to change?
	if( IsViewAvailable(i) && m_currView != m_views[i] )
	{
		// get the old position
		if( m_currView == NULL )
		{
			keepPosition = false;
		}

		if( keepPosition )
		{
			viewPos = m_currView->GetFirstSelectedOrVisiblePos(viewOffset);
		}

		// set the new view
		m_currView = m_views[i];

		// set the zoom
		g_mainFrame->SetZoom__(m_currView->m_zoom, redraw);

		m_currView->Realize(m_currView->__needsColumnMixerReload, true /*keep col index*/);
		m_currView->__needsColumnMixerReload = false;

		// reset the old position
		if( keepPosition && !viewPos.IsEmpty() )
		{
			m_currView->GotoPos(viewPos, viewOffset);
		}

		if( redraw )
		{
			Refresh();
			Update();
		}
	}

	// set the skin target values - do this even if the view is not changed as this may
	// be needed by a delayed button refresh
	wxASSERT( IDT_WORKSPACE_ALBUM_VIEW == IDT_WORKSPACE_ALBUM_VIEW+SJ_BROWSER_ALBUM_VIEW );
	wxASSERT( IDT_WORKSPACE_COVER_VIEW == IDT_WORKSPACE_ALBUM_VIEW+SJ_BROWSER_COVER_VIEW );
	wxASSERT( IDT_WORKSPACE_LIST_VIEW  == IDT_WORKSPACE_ALBUM_VIEW+SJ_BROWSER_LIST_VIEW  );
	for( j = 0; j < SJ_BROWSER_VIEW_COUNT; j++ )
	{
		value.value = m_currView == m_views[j]? 1 : 0;
		g_mainFrame->SetSkinTargetValue(IDT_WORKSPACE_ALBUM_VIEW+j, value);
	}
}


void SjBrowserWindow::SetZoom_(int targetIdOrAbs)
{
	wxASSERT( IDT_ZOOM_IN > SJ_ZOOM_MAX );
	wxASSERT( IDT_ZOOM_OUT > SJ_ZOOM_MAX );
	wxASSERT( IDT_ZOOM_NORMAL > SJ_ZOOM_MAX );

	long newZoom;

	if( targetIdOrAbs == IDT_ZOOM_IN )
	{
		newZoom = m_currView->m_zoom + 1;
	}
	else if( targetIdOrAbs == IDT_ZOOM_OUT )
	{
		newZoom = m_currView->m_zoom - 1;
	}
	else if( targetIdOrAbs == IDT_ZOOM_NORMAL )
	{
		newZoom = SJ_ZOOM_DEF; /*default*/
	}
	else
	{
		newZoom = targetIdOrAbs;
	}

	newZoom = SjMainFrame::CorrectZoom(newZoom); // CorrectZoom() added in 2.10beta5; we forgot this in 10beta3+4 when we implemented different zooms for the views

	m_currView->m_zoom = newZoom;
	if( g_accelModule->m_flags & SJ_ACCEL_SAME_ZOOM_IN_ALL_VIEWS )
	{
		for( int i = 0; i < SJ_BROWSER_VIEW_COUNT; i++ )
		{
			m_views[i]->m_zoom = newZoom;
		}
	}
	g_mainFrame->SetZoom__(newZoom);
}


void SjBrowserWindow::AddItemsToColMenu(SjMenu* m)
{
	if( m_currView )
	{
		m_currView->AddItemsToColMenu(m);
	}
}


/*******************************************************************************
 * Constructor / Destructor
 ******************************************************************************/


SjBrowserWindow::SjBrowserWindow(SjMainFrame* mainFrame)
	: wxWindow(mainFrame, -1, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
{
	g_mainFrame = mainFrame;
	#if SJ_USE_TOOLTIPS
		m_toolTipProvider.m_browserWindow = this;
	#endif

	m_clientW               = 0;
	m_clientH               = 0;
	m_mouseAction           = SJ_ACTION_NONE;
	m_skipContextMenuEvent  = 0;
	m_cursorChanged         = false;

	// create the views
	m_views[0] = new SjAlbumBrowser(this);
	m_views[1] = new SjCoverBrowser(this);
	m_views[2] = new SjListBrowser(this);

	m_currView = NULL;

	// load the view flags and the zoom
	int i;
	SjStringSerializer ser(g_tools->m_config->Read(wxT("main/browserFlags"), wxT("")));
	for( i = 0; i < SJ_BROWSER_VIEW_COUNT; i++ )
	{
		m_views[i]->m_flags = ser.GetLong();
		if( ser.HasErrors() )
			m_views[i]->m_flags = SJ_BROWSER_VIEW_DEFAULT_FLAGS;
	}

	for( i = 0; i < SJ_BROWSER_VIEW_COUNT; i++ )
	{
		m_views[i]->m_zoom = SjMainFrame::CorrectZoom(ser.GetLong());
		if( ser.HasErrors() )
		{
			m_views[i]->m_zoom = 3; // default
		}
	}
}


bool SjBrowserWindow::AreCoversShown() const
{
	if(  m_currView
	 && (m_currView->m_flags&SJ_BROWSER_VIEW_COVER) )
	{
		return true;
	}
	return false;
}


#ifdef __WXDEBUG__
static bool s_exitCalled = false;
#endif
void SjBrowserWindow::Exit()
{
	#ifdef __WXDEBUG__
		wxASSERT( !s_exitCalled );
		s_exitCalled = true;
	#endif

	SjStringSerializer ser;
	int i;
	for( i = 0; i < SJ_BROWSER_VIEW_COUNT; i++ )
	{
		ser.AddLong(m_views[i]->m_flags);
	}

	for( i = 0; i < SJ_BROWSER_VIEW_COUNT; i++ )
	{
		ser.AddLong(m_views[i]->m_zoom);
		m_views[i]->Exit();
	}

	g_tools->m_config->Write(wxT("main/browserFlags"), ser.GetResult());
}


SjBrowserWindow::~SjBrowserWindow()
{
	#ifdef __WXDEBUG__
		wxASSERT( s_exitCalled );
	#endif

	for( int i = 0; i < SJ_BROWSER_VIEW_COUNT; i++ )
	{
		delete m_views[i];
	}
}


/*******************************************************************************
 * Tools for our friends :-)
 ******************************************************************************/


SjImgThreadObj* SjBrowserWindow::RequireImage(const wxString& url, long w)
{
	SjImgOp         op;

	op.LoadFromDb(url); // the ID may be NULL which is okay

	op.m_flags      |= SJ_IMGOP_RESIZE|SJ_IMGOP_BORDER;
	if( g_mainFrame->m_skinFlags&SJ_SKIN_IMG_SMOOTH )
	{
		op.m_flags  |= SJ_IMGOP_SMOOTH;
	}
	op.m_resizeW    = w? w : g_mainFrame->m_currCoverWidth;
	op.m_resizeH    = w? w : g_mainFrame->m_currCoverHeight;

	return g_mainFrame->m_imgThread->RequireImage(this, url, op);
}


void SjBrowserWindow::PaintCover(wxDC& dc, SjImgThreadObj* obj, long x, long y, long w)
{
	bool error = TRUE;
	long coverW = w? w : g_mainFrame->m_currCoverWidth,
	     coverH = w? w : g_mainFrame->m_currCoverHeight;

	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMALODD].bgBrush);

	if( obj == NULL )
	{
		// no cover there - this is a normal situation as the object is rendered asynchron;
		// just draw the background
		dc.DrawRectangle(x, y, coverW, coverH);
		return;
	}



	// hide the drag image
	bool dragImageHidden = false;
	if( g_mainFrame->m_dragImage )
	{
		wxRect scrRect(x, y, coverW, coverH);
		ClientToScreen(&scrRect.x, &scrRect.y);
		if( g_mainFrame->m_dragRect.Intersects(scrRect) )
		{
			g_mainFrame->m_dragImage->Hide();
			dragImageHidden = true;
		}
	}

	// draw ...
	wxBitmap* bitmap = obj->CreateBitmap();
	if( bitmap )
	{
		int     bitmapWidth = bitmap->GetWidth();
		int     bitmapHeight = bitmap->GetHeight();
		bool    clippingRegionSet = FALSE;

		if( bitmapWidth != coverW || bitmapHeight != coverH )
		{
			#if 1
				// Draw bitmap of incorrect size using borders aright and abottom.
				if( bitmapWidth < (coverW/2) )
				{
					dc.DrawRectangle(x, y, coverW, coverH); // bitmap is too small, wait for correct bitmap
				}
				else
				{
					if( bitmapWidth > coverW || bitmapHeight > coverH )
					{
						dc.SetClippingRegion(x, y, coverW, coverH);
						clippingRegionSet = TRUE;
					}

					if( bitmapWidth < coverW ) // draw whitespace right of image (if any)
					{
						dc.DrawRectangle(x+bitmapWidth, y, coverW-bitmapWidth, coverH);
					}

					if( bitmapHeight < coverH ) // draw whitespace abottom of image (if any)
					{
						dc.DrawRectangle(x, y+bitmapHeight, coverW, coverH-bitmapHeight);
					}

					dc.DrawBitmap(*bitmap, x, y, FALSE /*not transparent*/);
				}
			#else
				// Draw bitmap of incorrect size using resizing.
				// This is no good idea as the resizing takes as much time
				// as the correct bitmap arrives from the image thread.
				wxImage image = bitmap->ConvertToImage();
				image.Scale(width, height);
				wxBitmap newBitmap(image);
				dc.DrawBitmap(newBitmap, x, y, FALSE /*not transparent*/);
			#endif
		}
		else
		{
			// Draw bitmap of correct size
			dc.DrawBitmap(*bitmap, x, y, FALSE /*not transparent*/);
		}

		delete bitmap;

		// destroy clipping (if set)
		if( clippingRegionSet )
		{
			dc.DestroyClippingRegion();
		}

		error = FALSE;
	}

	// error?
	if( error )
	{
		dc.DrawRectangle(x, y, coverW, coverH);

		wxRect tempRect(x+2, y+2, coverW-2, coverH-2);
		g_tools->DrawText(dc, obj->m_errors,
		                  tempRect,
		                  g_mainFrame->m_currSmallFont, g_mainFrame->m_currSmallFont,
		                  g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMALODD].fgColour);
	}

	// show the drag image again
	if( dragImageHidden
	 && g_mainFrame->m_dragImage )
	{
		g_mainFrame->m_dragImage->Show();
	}
}


void SjBrowserWindow::DrawUpText(wxDC& dc, const wxString& textup, long x, long y, long w, long h)
{
	// draw vertical text aleft of column
	// (we're using a rotated offscreen DC instead of wxDC::DrawRotatedText()
	// as the latter has problems with non-true-type fonts and is not present
	// on all operating systems)
	wxFont& upFont = g_mainFrame->m_currBoldFont;

	dc.SetFont(upFont);
	wxCoord textw, texth;
	dc.GetTextExtent(textup, &textw, &texth);
	texth++;
	textw += 2;

	if( texth > w )
	{
		texth = w;
	}

	if( textw > h )
	{
		textw = h;
	}

	wxBitmap memBitmap1(textw, texth);
	wxMemoryDC memDc;
	memDc.SelectObject(memBitmap1);
	if( memDc.IsOk() )
	{
		memDc.SetBrush(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgBrush);
		memDc.SetPen(*wxTRANSPARENT_PEN);
		memDc.DrawRectangle(0, 0, textw, texth);
		memDc.SetFont(upFont);
		memDc.SetTextBackground(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgColour);
		memDc.SetTextForeground(g_mainFrame->m_workspaceColours[SJ_COLOUR_VERTTEXT].fgColour);
		memDc.DrawText(textup, 0, 0);

		wxImage memImage = memBitmap1.ConvertToImage();
		if( memImage.IsOk() )
		{
			memImage = memImage.Rotate90(FALSE/*not clockwise*/);
			if( memImage.IsOk() )
			{
				wxBitmap memBitmap2(memImage);
				if( memBitmap2.IsOk() )
				{
					dc.SetBrush(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgBrush);
					dc.SetPen(*wxTRANSPARENT_PEN);

					#if 0
						// right-aligned
						if( w > texth )
							dc.DrawRectangle(x, y, w-texth, h);

						dc.DrawBitmap(memBitmap2, x+(w-texth), y);
					#else
						// centered
						long spaceRight = (w-texth)/2;
						long spaceLeft  = (w-texth)-spaceRight;
						if( spaceLeft > 0 )
							dc.DrawRectangle(x, y, spaceLeft, h);

						dc.DrawBitmap(memBitmap2, x+spaceLeft, y);

						if( spaceRight > 0 )
							dc.DrawRectangle(x+spaceLeft+texth, y, spaceRight, h);
					#endif

					// space abottom
					if( h > textw )
					{
						dc.DrawRectangle(x, y+textw, w, h-textw);
					}
				}
			}
		}

		memDc.SelectObject(wxNullBitmap);
	}
}


void SjBrowserWindow::GetFonts(int roughType, wxFont** font1, wxFont** font2, bool isEnqueued)
{
	wxASSERT(font1);
	wxASSERT(font2);

	if( roughType == SJ_RRTYPE_TITLE1 || roughType == SJ_RRTYPE_TITLE2 )
	{
		*font1 = &(g_mainFrame->m_currBoldFont);
		*font2 = &(g_mainFrame->m_currSmallBoldFont);
	}
	else if( roughType == SJ_RRTYPE_TITLE3 )
	{
		*font1 = &(g_mainFrame->m_currBoldItalicFont);
		*font2 = &(g_mainFrame->m_currBoldItalicFont);
	}
	else
	{
		*font1 = /*isEnqueued? &(g_mainFrame->m_currItalicFont) :*/ &(g_mainFrame->m_currStdFont);
		*font2 = &(g_mainFrame->m_currSmallFont);
	}
}


void SjBrowserWindow::GetFontPxSizes(wxDC& dc,
                                     int& fontVDiff, // this is the difference in height between the small and the normal font
                                     int& fontSpace, // this is the space between the left/mid/right columns
                                     int& fontStdHeight)
{
	if( g_mainFrame->m_currStdFont.IsOk() )
	{
		wxCoord w1, w2, h2;
		dc.SetFont(g_mainFrame->m_currStdFont);
		dc.GetTextExtent(wxT("Ag"), &w1, &fontStdHeight);
		dc.SetFont(g_mainFrame->m_currSmallFont);
		dc.GetTextExtent(wxT("Ag"), &w2, &h2);

		fontSpace = h2 / 2;

		fontVDiff = fontStdHeight - h2;
		if( fontVDiff < 0 )
		{
			fontVDiff = 0;
		}
	}
	else
	{
		fontStdHeight = 12;
		fontSpace = 4;
		fontVDiff = 4;
	}
}



