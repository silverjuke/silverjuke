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
 * Purpose: This file provides two classes, SjVisEmbed and
 *          SjVisFrame that are used as parents for the visualizations
 *          (either embedded or floating)
 *
 *******************************************************************************
 *
 * Normally one would use multiple inheritance for the both vis. windows
 * (one based on wxWindow and the other based on wxFrame) - however, I did
 * not really get it to work with wxWidget :-(
 *
 * So there are to "dummy" classes SjVisEmbed and SjVisFrame which
 * does nothing but forwarding all important calls to SjVisImpl.
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/vis/vis_module.h>
#include <sjmodules/vis/vis_window.h>
#include <sjmodules/vis/vis_bg.h>


/*******************************************************************************
 *  SjVisImpl Constructor / Configuration
 ******************************************************************************/


#define IDCI_FIRST                  (IDM_FIRSTPRIVATE+0)

#define IDC_FIRST_RENDERER_OPTION       (IDM_FIRSTPRIVATE+0)    // this range should be used by the renderers if they implement SjVisRendererModule::AddMenuOptions()
#define IDC_LAST_RENDERER_OPTION        (IDM_FIRSTPRIVATE+201)  // range end
#define IDC_RENDERER_TITLE              (IDM_FIRSTPRIVATE+202)
#define IDC_STOP_OR_CLOSE               (IDM_FIRSTPRIVATE+203)

#define IDC_STARTVIS_FIRST              (IDM_FIRSTPRIVATE+300)
#define IDC_STARTVIS_LAST               (IDM_FIRSTPRIVATE+500)
#define IDC_EMBED_WINDOW                (IDM_FIRSTPRIVATE+501)
#define IDC_HALF_SIZE                   (IDM_FIRSTPRIVATE+502)
#define IDC_SWITCH_OVER_AUTOMATICALLY   (IDM_FIRSTPRIVATE+504)

#define IDCI_LAST                   (IDM_FIRSTPRIVATE+600)


SjVisImpl::SjVisImpl()
{
	m_thisWindow = NULL;
	m_renderer = NULL;

	m_menuColour.Set(0x80, 0x80, 0x80);
	m_menuPen    = wxPen(m_menuColour, 1, wxSOLID);

	m_keyDown = false;
	m_mouseDown = false;
}


void SjVisImpl::Init(wxWindow* thisWindow, bool thisWindowIsFrame)
{
	wxASSERT( m_thisWindow == NULL );

	m_thisWindow        = thisWindow;
	m_thisWindowIsFrame = thisWindowIsFrame;

	CalcPositions();
}


void SjVisImpl::Exit()
{
	RemoveRenderer();
	wxASSERT( m_renderer == NULL );
}


/*******************************************************************************
 * SjVisImpl Drawing Related stuff
 ******************************************************************************/

static void rectAway(wxRect& r)
{
	r.x = r.y = -1000;
	r.width = r.height = 16;
}


void SjVisImpl::CalcPositions()
{
	int w, h;
	m_thisWindow->GetClientSize(&w, &h);

	wxCoord w1, TOP_H;
	wxClientDC dc(m_thisWindow);
	dc.SetFont(g_mainFrame->m_baseStdFont);
	dc.GetTextExtent(wxT("Ag"), &w1, &TOP_H);
#define PADDING (TOP_H/2)

	m_topRect.x = 0;
	m_topRect.y = 0;
	m_topRect.width = w;
	m_topRect.height = TOP_H;

	m_triangleRect = m_topRect;
	m_triangleRect.x += PADDING;
	m_triangleRect.width = TOP_H;

	m_closeRect = m_triangleRect;
	m_closeRect.x = (m_topRect.x+m_topRect.width) - TOP_H - PADDING;

	m_msgLineRect = m_topRect;
	m_msgLineRect.x = m_triangleRect.x + m_triangleRect.width;
	m_msgLineRect.width = m_closeRect.x - (m_triangleRect.x+m_triangleRect.width);

	m_rendererRect = m_topRect;
	m_rendererRect.y += m_topRect.height;
	m_rendererRect.height = h - m_topRect.height;

	// finally, move the triangle button and the close button a little bit to the left and to the right
	m_triangleRect.x -= (PADDING/2);
	m_closeRect.x += (PADDING/2);

	// hide unused stuff
	if( !g_mainFrame->IsAllAvailable() )
	{
		rectAway(m_triangleRect);
		rectAway(m_msgLineRect);
	}

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


void SjVisImpl::OnEraseBackground(wxEraseEvent& e)
{
}


void SjVisImpl::OnPaint(wxPaintEvent& e)
{
	wxPaintDC paintDc(m_thisWindow);

	paintDc.SetBrush(
#if 0
	    *wxGRAY_BRUSH
#else
	    *wxBLACK_BRUSH
#endif
	);
	paintDc.SetPen(*wxTRANSPARENT_PEN);
	paintDc.DrawRectangle(m_thisWindow->GetClientSize());

	paintDc.SetPen(m_menuPen);
	wxRect drawRect = m_triangleRect;
	drawRect.Deflate(3);
	drawRect.x ++;
	SjTools::DrawIcon(paintDc, drawRect, SJ_DRAWICON_TRIANGLE_DOWN);

	if( g_visModule->IsCloseButtonNeeded() )
	{
		drawRect = m_closeRect;
		drawRect.Deflate(3);
		SjTools::DrawIcon(paintDc, drawRect, SJ_DRAWICON_DELETE);
	}

	paintDc.SetFont(g_mainFrame->m_baseStdFont);
	paintDc.SetClippingRegion(m_msgLineRect);
	paintDc.SetTextForeground(m_menuColour);
	paintDc.SetBackgroundMode(wxTRANSPARENT);

	wxString msg = _("Options");
	if( m_renderer )
		msg = m_renderer->m_name;

	wxCoord w, h;
	paintDc.GetTextExtent(msg, &w, &h);
	m_msgTextRect = wxRect(m_msgLineRect.x, m_msgLineRect.y, w, h);
	paintDc.DrawText(msg, m_msgTextRect.x, m_msgTextRect.y);

	paintDc.DestroyClippingRegion();
}


/*******************************************************************************
 *  SjVisImpl Mouse / Keyboard / Context-Menu
 ******************************************************************************/


wxPoint SjVisImpl::GetEventMousePosition(wxWindow* from, wxMouseEvent& e) const
{
	// as mouse events are forwared from m_thisWindow or from any client,
	// this function converts to coordinated relative to m_thisWindow

	wxASSERT( from );

	wxPoint pt = from->ClientToScreen(e.GetPosition());
	pt = m_thisWindow->ScreenToClient(pt);

	return pt;
}


void SjVisImpl::OnKeyUp(wxKeyEvent& event)
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


void SjVisImpl::OnMouseEnter(wxWindow* from, wxMouseEvent& event)
{
	// if the mouse enters this window, send a mouse leave
	// message to the parent window
	wxMouseEvent pendEvt(wxEVT_LEAVE_WINDOW);
	if( !m_thisWindowIsFrame )
	{
		wxASSERT( m_thisWindow->GetParent() );
		m_thisWindow->GetParent()->GetEventHandler()->AddPendingEvent(pendEvt);
	}
}


void SjVisImpl::OnMouseLeftUp(wxWindow* from, wxMouseEvent& event)
{
	if( !m_mouseDown || g_mainFrame == NULL || g_visModule == NULL )
		return; // do not consume "half clicks"

	wxPoint mousePt = GetEventMousePosition(from, event);

	if( m_closeRect.Contains(mousePt) && g_visModule->IsCloseButtonNeeded() )
	{
		g_visModule->StopOrCloseRequest();
	}
	else if( g_mainFrame->IsAllAvailable() )
	{
		if( m_triangleRect.Contains(mousePt) || m_msgTextRect.Contains(mousePt) )
		{
			ShowContextMenu(m_triangleRect.x+2, m_triangleRect.y+m_triangleRect.height);
		}
		else if( wxSystemSettings::GetMetric(wxSYS_MOUSE_BUTTONS)<=1 && event.ControlDown() )
		{
			ShowContextMenu(mousePt.x, mousePt.y);
		}
	}
	else
	{
		if( g_visModule->IsOverWorkspace() || g_mainFrame->IsOpAvailable(SJ_OP_STARTVIS) )
		{
			g_visModule->StopOrCloseRequest();
		}
	}
}


void SjVisImpl::OnMouseRightUp(wxWindow* from, wxContextMenuEvent& event)
{
	wxPoint mousePt = m_thisWindow->ScreenToClient(event.GetPosition());

	ShowContextMenu(mousePt.x, mousePt.y);
}


void SjVisImpl::OnMouseLeftDClick(wxWindow* from, wxMouseEvent& event)
{
	wxCommandEvent fwd(wxEVT_COMMAND_MENU_SELECTED, IDC_EMBED_WINDOW);
	m_thisWindow->GetEventHandler()->AddPendingEvent(fwd);
}


void SjVisImpl::ShowContextMenu(int x, int y)
{
	if( g_mainFrame->IsAllAvailable() )
	{
		SjMenu m(0);

		// add renderer list
		SjModuleList* moduleList = g_mainFrame->m_moduleSystem.GetModules(SJ_MODULETYPE_VIS_RENDERER);
		int i = 0;
		SjModuleList::Node* moduleNode = moduleList->GetFirst();
		while( moduleNode )
		{
			SjVisRendererModule* module = (SjVisRendererModule*)moduleNode->GetData();
			wxASSERT(module);
			m.AppendRadioItem(IDC_STARTVIS_FIRST+i, module->m_name);
			if( module == m_renderer )
				m.Check(IDC_STARTVIS_FIRST+i, true);

			// next
			moduleNode = moduleNode->GetNext();
			i++;
		}

		m.AppendCheckItem(IDC_SWITCH_OVER_AUTOMATICALLY, _("If appropriate, switch over automatically"));
		m.Check(IDC_SWITCH_OVER_AUTOMATICALLY, (g_visModule->m_visFlags&SJ_VIS_FLAGS_SWITCH_OVER_AUTOMATICALLY)!=0);

		m.AppendSeparator();

		// add renderer options
		wxString rendererName = m_renderer? m_renderer->m_name : wxString(wxT("..."));
		m.Append(IDC_RENDERER_TITLE, wxString::Format(_("Options for \"%s\""), rendererName.c_str()) + wxT(":"));
		m.Enable(IDC_RENDERER_TITLE, false);
		if( m_renderer )
		{
			m_renderer->AddMenuOptions(m);
		}

		// add view options
		SjMenu* submenu = new SjMenu(0);

		submenu->AppendCheckItem(IDC_EMBED_WINDOW, _("Attached window"));
		submenu->Check(IDC_EMBED_WINDOW, g_visModule->IsEmbedded());
		submenu->Enable(IDC_EMBED_WINDOW, !g_mainFrame->IsKioskStarted() && g_mainFrame->GetVisEmbedRect());

		submenu->AppendCheckItem(IDC_HALF_SIZE, _("Half size"));
		submenu->Check(IDC_HALF_SIZE, (g_visModule->m_visFlags & SJ_VIS_FLAGS_HALF_SIZE)!=0);

		m.Append(0, _("View"), submenu);

		// misc.
		m.AppendSeparator();

		m.Append(IDC_STOP_OR_CLOSE, _("Close"));
		m.Enable(IDC_STOP_OR_CLOSE, g_visModule->IsCloseMenuEntryNeeded());

		// do popup!
		m_thisWindow->PopupMenu(&m, x, y);
	}
}


void SjVisImpl::OnCommand(wxCommandEvent& e)
{
	if( !g_mainFrame->IsAllAvailable() )
		return;

	int cmdId = e.GetId();
	if( cmdId >= IDC_STARTVIS_FIRST && cmdId <= IDC_STARTVIS_LAST )
	{
		// change the selected vis. renderer
		int i = 0;
		SjModuleList* moduleList = g_mainFrame->m_moduleSystem.GetModules(SJ_MODULETYPE_VIS_RENDERER);
		SjModuleList::Node* moduleNode = moduleList->GetFirst();
		while( moduleNode )
		{
			SjVisRendererModule* newRenderer = (SjVisRendererModule*)moduleNode->GetData();
			wxASSERT(newRenderer);
			if( IDC_STARTVIS_FIRST+i == cmdId )
			{
				if( newRenderer != m_renderer )
				{
					g_tools->m_config->Write(wxT("player/vismodule"), newRenderer->PackFileName());
					g_visModule->SetCurrRenderer(newRenderer);
				}

				break;
			}

			// next
			moduleNode = moduleNode->GetNext();
			i++;
		}
	}
	else if( cmdId == IDC_SWITCH_OVER_AUTOMATICALLY )
	{
		SjTools::ToggleFlag(g_visModule->m_visFlags, SJ_VIS_FLAGS_SWITCH_OVER_AUTOMATICALLY);
		g_visModule->WriteVisFlags();
	}
	else if( cmdId >= IDC_FIRST_RENDERER_OPTION && cmdId <= IDC_LAST_RENDERER_OPTION )
	{
		// renderer options
		if( m_renderer )
			m_renderer->OnMenuOption(cmdId);
	}
	else if( cmdId == IDC_HALF_SIZE )
	{
		// global renderer options
		SjTools::ToggleFlag(g_visModule->m_visFlags, SJ_VIS_FLAGS_HALF_SIZE);
		g_visModule->WriteVisFlags();
		CalcPositions();
		m_thisWindow->Refresh();
	}
	else if( cmdId == IDC_EMBED_WINDOW )
	{
		// attach / detach
		g_visModule->AttachDetachRequest();
	}
	else if( cmdId == IDC_STOP_OR_CLOSE )
	{
		// close it
		g_visModule->StopOrCloseRequest();
	}
}


/*******************************************************************************
 * Changing the renderers
 ******************************************************************************/


void SjVisImpl::SetRenderer(SjVisRendererModule* rendererModule, bool justContinue)
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
			rendererModule->Start(this, justContinue);
		}
	}
	else
	{
		// remove an exiting renderer
		RemoveRenderer();
		wxASSERT( m_renderer == NULL );
	}

	m_thisWindow->Refresh(true, &m_msgLineRect);
	m_thisWindow->Refresh(true, &m_closeRect);
}


void SjVisImpl::RemoveRenderer()
{
	if( m_renderer )
	{
		m_renderer->Stop();
		m_renderer->Unload();
		m_renderer = NULL;
	}
}


/*******************************************************************************
 * The following functions are meant to be used for the
 * vis. implementations (SjVisRendererModule)
 ******************************************************************************/


wxRect SjVisImpl::GetRendererClientRect() const
{
	return m_rendererRect;
}


wxRect SjVisImpl::GetRendererScreenRect() const
{
	wxRect  screenRect = m_rendererRect;
	int     x = screenRect.x;
	int     y = screenRect.y;

	m_thisWindow->ClientToScreen(&x, &y);

	screenRect.x = x;
	screenRect.y = y;

	return screenRect;
}


/*******************************************************************************
 * Instanceable Classes
 ******************************************************************************/


void SjVisFrame::OnCloseWindow(wxCloseEvent&)
{
	g_visModule->StopVis();
}


SjVisFrame::SjVisFrame( wxWindow* parent, wxWindowID id, const wxString& title,
                        const wxPoint& pos, const wxSize& size,
                        long style)
	: wxFrame(parent, id, title, pos, size, style)
{
	SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_MAIN));
}


void SjVisFrame::OnFwdToMainFrame(wxCommandEvent& e)
{
	g_mainFrame->GetEventHandler()->ProcessEvent(e);
}


BEGIN_EVENT_TABLE(SjVisFrame, wxFrame)
	EVT_ERASE_BACKGROUND(                       SjVisFrame::OnEraseBackground   )
	EVT_PAINT           (                       SjVisFrame::OnPaint             )
	EVT_SIZE            (                       SjVisFrame::OnSize              )
	EVT_KEY_DOWN        (                       SjVisFrame::OnKeyDown           )
	EVT_KEY_UP          (                       SjVisFrame::OnKeyUp             )
	EVT_LEFT_DOWN       (                       SjVisFrame::OnMouseLeftDown     )
	EVT_LEFT_UP         (                       SjVisFrame::OnMouseLeftUp       )
	EVT_LEFT_DCLICK     (                       SjVisFrame::OnMouseLeftDClick   )
	EVT_CONTEXT_MENU    (                       SjVisFrame::OnMouseRightUp      )
	EVT_ENTER_WINDOW    (                       SjVisFrame::OnMouseEnter        )
	EVT_MENU_RANGE      (IDCI_FIRST, IDCI_LAST, SjVisFrame::OnCommand           )
	EVT_MENU_RANGE      (IDT_FIRST, IDT_LAST,   SjVisFrame::OnFwdToMainFrame    )
	EVT_CLOSE           (                       SjVisFrame::OnCloseWindow       )
END_EVENT_TABLE()


BEGIN_EVENT_TABLE(SjVisEmbed, wxWindow)
	EVT_ERASE_BACKGROUND(                       SjVisEmbed::OnEraseBackground   )
	EVT_PAINT           (                       SjVisEmbed::OnPaint             )
	EVT_SIZE            (                       SjVisEmbed::OnSize              )
	EVT_KEY_DOWN        (                       SjVisEmbed::OnKeyDown           )
	EVT_KEY_UP          (                       SjVisEmbed::OnKeyUp             )
	EVT_LEFT_DOWN       (                       SjVisEmbed::OnMouseLeftDown     )
	EVT_LEFT_UP         (                       SjVisEmbed::OnMouseLeftUp       )
	EVT_LEFT_DCLICK     (                       SjVisEmbed::OnMouseLeftDClick   )
	EVT_CONTEXT_MENU    (                       SjVisEmbed::OnMouseRightUp      )
	EVT_ENTER_WINDOW    (                       SjVisEmbed::OnMouseEnter        )
	EVT_MENU_RANGE      (IDCI_FIRST, IDCI_LAST, SjVisEmbed::OnCommand           )
END_EVENT_TABLE()
