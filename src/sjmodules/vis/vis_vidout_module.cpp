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
 * File:    vidout_module.cpp
 * Authors: Björn Petersen
 * Purpose: The video output module (not: the decoder module)
 *
 ******************************************************************************/


#include <sjbase/base.h>
#if SJ_USE_VIDEO
#include <wx/glcanvas.h>
#include <sjmodules/vis/vis_module.h>
#include <sjmodules/vis/vis_window.h>
#include <sjmodules/vis/vis_vidout_module.h>


#undef VIDEO_DEBUG_VIEW // other colors, visible "hidden" window etc.


/*******************************************************************************
 * the window holding the video
 ******************************************************************************/


#ifdef __WXGTK__
	#define PARENT_WINDOW_CLASS wxGLCanvas // we use wxGLCanvas as this class offers us the "real" window handle by GetXWindow()
#else
	#define PARENT_WINDOW_CLASS wxWindow
#endif


class SjVidoutWindow : public PARENT_WINDOW_CLASS
{
public:
	            SjVidoutWindow      (wxWindow* parent);
	bool        ImplOk              () const { return (g_vidoutModule!=NULL&&g_vidoutModule->m_impl); }

private:
	void        OnEraseBackground   (wxEraseEvent& e);
	void        OnPaint             (wxPaintEvent& e);

	void        OnMouseLeftDown     (wxMouseEvent& e)   { if(ImplOk()) g_vidoutModule->m_impl->OnMouseLeftDown(e); }
	void        OnMouseLeftUp       (wxMouseEvent& e)   { if(ImplOk()) g_vidoutModule->m_impl->OnMouseLeftUp(e); }
	void        OnMouseRightUp      (wxContextMenuEvent& e)   { if(ImplOk()) g_vidoutModule->m_impl->OnMouseRightUp(e); }
	void        OnMouseLeftDClick   (wxMouseEvent& e)   { if(ImplOk()) g_vidoutModule->m_impl->OnMouseLeftDClick(e); }

	DECLARE_EVENT_TABLE ();
};


BEGIN_EVENT_TABLE(SjVidoutWindow, PARENT_WINDOW_CLASS)
	EVT_LEFT_DOWN       (                       SjVidoutWindow::OnMouseLeftDown     )
	EVT_LEFT_UP         (                       SjVidoutWindow::OnMouseLeftUp           )
	EVT_LEFT_DCLICK     (                       SjVidoutWindow::OnMouseLeftDClick       )
	EVT_CONTEXT_MENU    (                       SjVidoutWindow::OnMouseRightUp          )

	EVT_ERASE_BACKGROUND(                       SjVidoutWindow::OnEraseBackground   )
	EVT_PAINT           (                       SjVidoutWindow::OnPaint             )
END_EVENT_TABLE()


SjVidoutWindow::SjVidoutWindow(wxWindow* parent)
	#if PARENT_WINDOW_CLASS==wxGLCanvas
		: wxGLCanvas(parent, wxID_ANY, NULL, wxDefaultPosition, wxDefaultSize,
			wxNO_BORDER | wxCLIP_CHILDREN | wxFULL_REPAINT_ON_RESIZE)
	#else
		: PARENT_WINDOW_CLASS(parent, -1, wxPoint(-1000,-1000), wxSize(100,100),
			wxNO_BORDER | wxCLIP_CHILDREN | wxFULL_REPAINT_ON_RESIZE)
	#endif
{
}


void SjVidoutWindow::OnEraseBackground(wxEraseEvent& e)
{
}


void SjVidoutWindow::OnPaint(wxPaintEvent& e)
{
	wxPaintDC paintDc(this);

	paintDc.SetBrush(
		#ifdef VIDEO_DEBUG_VIEW
			*wxLIGHT_GREY_BRUSH
		#else
			*wxBLACK_BRUSH
		#endif
	);
	paintDc.SetPen(*wxTRANSPARENT_PEN);
	paintDc.DrawRectangle(GetClientSize());
}



/*******************************************************************************
 * the module controlling the video
 ******************************************************************************/


/*
void SjVidoutModule::SetRecentVidCh(DWORD ch)
{
	if( s_theWindow )
	{
		BASS_DSHOW_ChannelSetWindow(ch, (HWND)s_theWindow->GetHandle());
		SetProperVideoSize(ch);
	}
}
*/

SjVidoutModule* g_vidoutModule = NULL;


SjVidoutModule::SjVidoutModule(SjInterfaceBase* interf)
	: SjVisRendererModule(interf)
{
	m_file              = wxT("memory:vidout.lib");
	m_name              = _("Video output");
	m_impl              = NULL;
	g_vidoutModule      = this;
	m_sort              = 2; // start of list, default is 1000
	m_os_window_handle  = 0;
}


SjVidoutModule::~SjVidoutModule()
{
	g_vidoutModule = NULL;
}


bool SjVidoutModule::FirstLoad()
{
	// creates a global window that can hold a video anytime.
	// as needed, the global window is assigned to the video output screen.
	//
	// we create the window here but we will find out the "real" window handle later on IDMODMSG_PROGRAM_LOADED -
	// this allows eg. GTK to really realize the window in between.
	m_theWindow = new SjVidoutWindow(g_visModule->GetVisWindow());
	MoveVidoutAway();
	m_theWindow->Show(); // sic! we hide the window by moving it away
	return true;
}


void SjVidoutModule::MoveVidoutAway()
{
	if( m_theWindow )
	{
		m_theWindow->SetSize(
			#ifdef VIDEO_DEBUG_VIEW
				0, 0,
			#else
				-10000, -10000,
			#endif
		    32, 32);
	}
}


bool SjVidoutModule::Start(SjVisWindow* impl)
{
	wxASSERT( wxThread::IsMain() );

	m_impl = impl;

	// create the window holding the vis.
	if( m_theWindow  )
	{
		PleaseUpdateSize(impl);
	}

	return true;
}


void SjVidoutModule::Stop()
{
	wxASSERT( wxThread::IsMain() );

	if( m_theWindow )
	{
		MoveVidoutAway();
	}

	m_impl = NULL;
}


void SjVidoutModule::ReceiveMsg(int msg)
{
	wxASSERT( wxThread::IsMain() );

	switch( msg )
	{
		case IDMODMSG_VIS_BEFORE_REPARENT:
			if( m_impl )
				g_mainFrame->Stop();
			break;

		case IDMODMSG_VIS_AFTER_REPARENT:
		case IDMODMSG_PROGRAM_LOADED:
			m_os_window_handle = (void*)m_theWindow->GetXWindow();
			break;
	}
}


void SjVidoutModule::AddMenuOptions(SjMenu&)
{
}


void SjVidoutModule::OnMenuOption(int)
{
}


void SjVidoutModule::PleaseUpdateSize(SjVisWindow* impl)
{
	if( m_theWindow )
	{
		wxRect visRect = impl->GetRendererClientRect();
		m_theWindow->SetSize(visRect);
	}
}


#endif // SJ_USE_VIDEO
