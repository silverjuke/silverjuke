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
 * File:    vis_window.cpp
 * Authors: Björn Petersen
 * Purpose: Parent classes for the visualizations
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/vis/vis_module.h>
#include <sjmodules/vis/vis_window.h>
#include <sjmodules/vis/vis_bg.h>


/*******************************************************************************
 * SjVisWindow
 ******************************************************************************/


SjVisWindow::SjVisWindow(wxWindow* parent)
		: wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
					wxCLIP_CHILDREN | wxFULL_REPAINT_ON_RESIZE)
{
	m_renderer = NULL;

	m_keyDown = false;
	m_mouseDown = false;

	CalcPositions();
}


SjVisWindow::~SjVisWindow()
{
	RemoveRenderer();
	wxASSERT( m_renderer == NULL );
}


BEGIN_EVENT_TABLE(SjVisWindow, wxWindow)
	EVT_ERASE_BACKGROUND(                       SjVisWindow::OnEraseBackground   )
	EVT_PAINT           (                       SjVisWindow::OnPaint             )
	EVT_SIZE            (                       SjVisWindow::OnSize              )
	EVT_KEY_DOWN        (                       SjVisWindow::OnKeyDown           )
	EVT_KEY_UP          (                       SjVisWindow::OnKeyUp             )
	EVT_LEFT_DOWN       (                       SjVisWindow::OnMouseLeftDown     )
	EVT_LEFT_UP         (                       SjVisWindow::OnMouseLeftUp       )
	EVT_LEFT_DCLICK     (                       SjVisWindow::OnMouseLeftDClick   )
	EVT_CONTEXT_MENU    (                       SjVisWindow::OnMouseRightUp      )
	EVT_ENTER_WINDOW    (                       SjVisWindow::OnMouseEnter        )
END_EVENT_TABLE()


void SjVisWindow::CalcPositions()
{
	int w, h;
	GetClientSize(&w, &h);

	m_rendererRect.x = 0;
	m_rendererRect.y = 0;
	m_rendererRect.width = w;
	m_rendererRect.height = h;

	if( g_visModule->m_visFlags & SJ_VIS_FLAGS_HALF_SIZE )
	{
		int subW = m_rendererRect.width/2;
		int subH = m_rendererRect.height/2;
		m_rendererRect.x += subW/2;
		m_rendererRect.y += subH/2;
		m_rendererRect.width -= subW;
		m_rendererRect.height -= subH;
	}

	if( m_renderer )
		m_renderer->PleaseUpdateSize(this);
}


void SjVisWindow::OnPaint(wxPaintEvent& e)
{
	wxPaintDC paintDc(this);

	paintDc.SetBrush(*wxBLACK_BRUSH);
	paintDc.SetPen(*wxTRANSPARENT_PEN);
	paintDc.DrawRectangle(GetClientSize());
}


void SjVisWindow::OnKeyUp(wxKeyEvent& event)
{
	int targetId = g_accelModule->KeyEvent2CmdId(event, SJA_MAIN);

	if( targetId )
	{
		// some keystrokes are forwarded to the parent
		// (the parent should have NEVER the focus, so it doesn't receive key events itself)
		wxCommandEvent fwd(wxEVT_COMMAND_MENU_SELECTED, targetId);
		g_mainFrame->GetEventHandler()->ProcessEvent(fwd);
	}
	else if( m_keyDown
	      && event.GetKeyCode() == WXK_ESCAPE )
	{
		if( g_visModule->IsOverWorkspace() || g_mainFrame->IsOpAvailable(SJ_OP_STARTVIS) )
			g_visModule->StopOrCloseRequest();
	}
	else
	{
		event.Skip();
	}
}


void SjVisWindow::OnMouseEnter(wxMouseEvent& event)
{
	// if the mouse enters this window, send a mouse leave
	// message to the parent window
	g_mainFrame->GetEventHandler()->QueueEvent(new wxMouseEvent(wxEVT_LEAVE_WINDOW));
}


void SjVisWindow::OnMouseLeftUp(wxMouseEvent& event)
{
	if( !m_mouseDown || g_mainFrame == NULL || g_visModule == NULL )
		return; // do not consume "half clicks"

	/* -- if over workspace, always close the window on a left click,
	   -- this is a good idea to be consisten with the kiosk mode.
	   -- special vis. function can be created using the keyboard or the main menu.
	   -- as we create all vis. ourselves now, there should be no problems with
	   -- inconsistency now.
	*/
	if( g_visModule->IsOverWorkspace() )
	{
		g_visModule->StopOrCloseRequest();
	}
}


void SjVisWindow::SetRenderer(SjVisRendererModule* rendererModule)
{
	wxASSERT( wxThread::IsMain() );

	if( rendererModule )
	{
		// set a new renderer
		RemoveRenderer();
		CalcPositions();

		// load the renderer
		if( rendererModule->Load() )
		{
			m_renderer = rendererModule;
			rendererModule->Start(this);
		}
	}
	else
	{
		// remove an exiting renderer
		RemoveRenderer();
		wxASSERT( m_renderer == NULL );
	}
}


void SjVisWindow::RemoveRenderer()
{
	if( m_renderer )
	{
		m_renderer->Stop();
		m_renderer->Unload();
		m_renderer = NULL;
	}
}


wxRect SjVisWindow::GetRendererClientRect() const
{
	return m_rendererRect;
}


wxRect SjVisWindow::GetRendererScreenRect() const
{
	wxRect  screenRect = m_rendererRect;
	int     x = screenRect.x;
	int     y = screenRect.y;

	ClientToScreen(&x, &y);

	screenRect.x = x;
	screenRect.y = y;

	return screenRect;
}


