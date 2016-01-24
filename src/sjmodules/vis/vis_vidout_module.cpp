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
#include <sjmodules/vis/vis_window.h>
#include <sjmodules/vis/vis_vidout_module.h>
#if SJ_USE_VIDEO

#ifdef __WXDEBUG__
#define VIDEO_DEBUG_VIEW // other colors, visible "hidden" window etc.
#endif


/*******************************************************************************
 * the window holding the video
 ******************************************************************************/


static SjVidoutModule* s_theVidoutModule = NULL;


class SjVidoutWindow : public wxWindow
{
public:
	SjVidoutWindow          (wxWindow* parent);

	bool                ImplOk() const { return (s_theVidoutModule!=NULL&&s_theVidoutModule->m_impl); }

private:
	//SjVidoutModule*   m_vidoutModule;

	void        OnEraseBackground   (wxEraseEvent& e);
	void        OnPaint             (wxPaintEvent& e);

	void        OnMouseLeftDown     (wxMouseEvent& e)   { if(ImplOk()) s_theVidoutModule->m_impl->OnMouseLeftDown(this, e); }
	void        OnMouseLeftUp       (wxMouseEvent& e)   { if(ImplOk()) s_theVidoutModule->m_impl->OnMouseLeftUp(this, e); }
	void        OnMouseRightUp      (wxContextMenuEvent& e)   { if(ImplOk()) s_theVidoutModule->m_impl->OnMouseRightUp(this, e); }
	void        OnMouseLeftDClick   (wxMouseEvent& e)   { if(ImplOk()) s_theVidoutModule->m_impl->OnMouseLeftDClick(this, e); }

	DECLARE_EVENT_TABLE ();
};


BEGIN_EVENT_TABLE(SjVidoutWindow, wxWindow)
	EVT_LEFT_DOWN       (                       SjVidoutWindow::OnMouseLeftDown     )
	EVT_LEFT_UP         (                       SjVidoutWindow::OnMouseLeftUp           )
	EVT_LEFT_DCLICK     (                       SjVidoutWindow::OnMouseLeftDClick       )
	EVT_CONTEXT_MENU    (                       SjVidoutWindow::OnMouseRightUp          )

	EVT_ERASE_BACKGROUND(                       SjVidoutWindow::OnEraseBackground   )
	EVT_PAINT           (                       SjVidoutWindow::OnPaint             )
END_EVENT_TABLE()


SjVidoutWindow::SjVidoutWindow(wxWindow* parent)
	: wxWindow( parent, -1, /*oscModule->m_name,*/
	            wxPoint(-1000,-1000), wxSize(100,100),
	            (wxNO_BORDER /*| wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT*/) )
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
 * the static window instance
 ******************************************************************************/


static SjVidoutWindow* s_theWindow = NULL;
static void HideTheWindow()
{
	if( s_theWindow )
	{
		s_theWindow->SetSize(
#ifdef VIDEO_DEBUG_VIEW
		    0, 0,
#else
		    -10000, -10000,
#endif
		    32, 32);

#ifdef VIDEO_DEBUG_VIEW
		s_theWindow->Show();
#endif
	}
}
static void CreateTheWindow()
{
	// static function that creates a global window that can hold a video anytime.
	// as needed, the global window is assigned to the video output screen
	if( s_theWindow == NULL )
	{
		s_theWindow = new SjVidoutWindow(g_mainFrame);
		HideTheWindow();
	}
}


/*******************************************************************************
 * the module controlling the video
 ******************************************************************************/


void SjVidoutModule::SetRecentVidCh(DWORD ch)
{
	CreateTheWindow();
	if( s_theWindow )
	{
		BASS_DSHOW_ChannelSetWindow(ch, (HWND)s_theWindow->GetHandle());
		SetProperVideoSize(ch);
	}
}


SjVidoutModule::SjVidoutModule(SjInterfaceBase* interf)
	: SjVisRendererModule(interf)
{
	m_file              = wxT("memory:vidout.lib");
	m_name              = _("Video output");
	m_impl              = NULL;
	s_theVidoutModule   = this;
	m_sort              = 2; // start of list, default is 1000
}


SjVidoutModule::~SjVidoutModule()
{
	s_theVidoutModule = NULL;
}


bool SjVidoutModule::Start(SjVisImpl* impl, bool justContinue)
{
	wxASSERT( wxThread::IsMain() );

	m_impl = impl;

	// create the window holding the vis.
	CreateTheWindow();
	if( s_theWindow  )
	{
		s_theWindow->Hide();

		s_theWindow->Reparent(impl->GetWindow());

		wxRect visRect = impl->GetRendererClientRect();
		s_theWindow->SetSize(visRect);

		s_theWindow->Show();

		ReceiveMsg(IDMODMSG_TRACK_ON_AIR_CHANGED); // this will assign the stream to the video window
	}

	return true;
}


void SjVidoutModule::Stop()
{
	wxASSERT( wxThread::IsMain() );

	if( s_theWindow )
	{
		s_theWindow->Hide();
		s_theWindow->Reparent(g_mainFrame);
		HideTheWindow();
	}

	m_impl = NULL;
}


void SjVidoutModule::ReceiveMsg(int msg)
{
	wxASSERT( wxThread::IsMain() );

	if( msg == IDMODMSG_TRACK_ON_AIR_CHANGED && s_theWindow )
	{
		// see if the most recent stream is a video file
		SjBassStream* hstream = g_mainFrame->m_player.RefCurrStream();
		if( hstream )
		{
			if( hstream->m_decoderModule
			        && hstream->m_decoderModule->IsVideoDecoder() )
			{
				BASS_DSHOW_ChannelSetWindow(hstream->m_chHandle, (HWND)s_theWindow->GetHandle());
				SetProperVideoSize(hstream->m_chHandle);
			}

			hstream->UnRef();
		}

	}
}


void SjVidoutModule::AddMenuOptions(SjMenu&)
{
}


void SjVidoutModule::OnMenuOption(int)
{
}


void SjVidoutModule::PleaseUpdateSize(SjVisImpl* impl)
{
	if( s_theWindow )
	{
		wxRect visRect = impl->GetRendererClientRect();
		s_theWindow->SetSize(visRect);

		SjBassStream* hstream = g_mainFrame->m_player.RefCurrStream();
		if( hstream )
		{
			if( hstream->m_decoderModule
			        && hstream->m_decoderModule->IsVideoDecoder() )
			{
				SetProperVideoSize(hstream->m_chHandle);
			}

			hstream->UnRef();
		}
	}
}


void SjVidoutModule::SetProperVideoSize(DWORD ch)
{
	int winWidth, winHeight, videoWidth, videoHeight, newWidth, newHeight;
	float widthRatio, heightRatio, scale;

	// get available window size
	{	wxSize s = s_theWindow->GetClientSize();
		winWidth = s.x; winHeight = s.y;
	}

	if( winWidth <= 32 || winHeight <= 32 )
		goto Fallback;

	// get video size
	{	VideoInfo video;
		BASS_DSHOW_ChannelGetInfo(ch, &video);
		videoWidth = video.Width; videoHeight = video.Height;
	}

	if( videoWidth < 10 || videoHeight < 10 )
		goto Fallback;

	// SET NEW SIZE: calculate the size regarding the aspect ratio
	widthRatio  = (float)winWidth / (float)videoWidth;
	heightRatio = (float)winHeight / (float)videoHeight;
	scale = widthRatio < heightRatio? widthRatio : heightRatio;

	newWidth = (int)((float)videoWidth * scale);
	newHeight = (int)((float)videoHeight * scale);

	BASS_DSHOW_ChannelResizeWindow(ch, (winWidth-newWidth)/2, (winHeight-newHeight)/2, newWidth, newHeight);
	return;

	// SET NEW SIZE: use the whole screen
Fallback:
	BASS_DSHOW_ChannelResizeWindow(ch, 0, 0, winWidth, winHeight);
}


#endif // SJ_USE_VIDEO
