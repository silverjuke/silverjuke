/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2016 Björn Petersen Software Design and Development
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
 * File:    vis_overlay.cpp
 * Authors: Björn Petersen
 * Purpose: Visualisation overlay with cover and title
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/imgthread.h>
#include <sjmodules/vis/vis_module.h>
#include <sjmodules/vis/vis_overlay.h>
#include <wx/popupwin.h>


/*******************************************************************************
 * SjVo*Timer
 ******************************************************************************/


#define STD_OVERLAY_DELAY_MS 2000 // 1 second: wait for the window being realized + 1 second: wait for the cover being loaded


#define TIMER_ACTION_REQUIRE_COVER  1
#define TIMER_ACTION_SHOW           2
#define TIMER_ACTION_CLOSE          3
#define TIMER_ACTION_POLL_FOR_EOS   4


class SjVoActionTimer : public wxTimer
{
public:
	SjVoActionTimer(SjVisOverlay* vo)
		: wxTimer()
	{
		m_vo = vo;
	}

	void SetAction(int action)
	{
		m_action = action;
	}

	void Notify()
	{
		if( m_action == TIMER_ACTION_REQUIRE_COVER )
		{
			m_vo->RequireCover();

			m_action = TIMER_ACTION_SHOW;
			Start(m_vo->m_delayMs/2, true/*one shot*/);
		}
		else if( m_action == TIMER_ACTION_SHOW )
		{
			m_vo->ShowOverlay();

			m_action = TIMER_ACTION_CLOSE;
			Start(g_visModule->m_visOverlaySeconds*1000, true/*one shot*/);
		}
		else if( m_action == TIMER_ACTION_CLOSE )
		{
			m_vo->CloseOverlay();

			m_action = TIMER_ACTION_POLL_FOR_EOS;
			Start(1000, false/*continuous*/);
		}
		else if( m_action == TIMER_ACTION_POLL_FOR_EOS )
		{
			long headroomMs = (long)((float)g_visModule->m_visOverlaySeconds*1000*0.5);
			if( g_mainFrame->m_autoCtrl.IsCurrTrackCloseToEnd(headroomMs+STD_OVERLAY_DELAY_MS) == 1 ) {
				Stop();
				if( !m_vo->GetCoverWindow() ) {
					SjVisRendererModule* r = g_visModule->GetCurrRenderer();
					if( r ) {
						m_vo->TrackOnAirChanged(r->GetSuitableParentForOverlay(), STD_OVERLAY_DELAY_MS);
					}
				}
			}
		}
	}

private:
	SjVisOverlay* m_vo;
	int           m_action;
};



#if SJ_OVERLAY_BASE==2
class SjVoPopupTimer : public wxTimer
{
public:
	SjVoPopupTimer(SjVisOverlay* vo)
		: wxTimer()
	{
		m_vo = vo;
	}

	void StartWatchingPosition()
	{
		m_lastRect = g_mainFrame->GetRect();
		Start(100, false/*continuous*/);
	}

	void Notify()
	{
		wxRect currRect = g_mainFrame->GetRect();

		if( !IsActiveInOurMeans()
		 || m_lastRect.x!=currRect.x || m_lastRect.y!=currRect.y ) {
			Stop();
            m_vo->CloseOverlay();
		}
	}

	bool IsActiveInOurMeans()
	{
		// is the Silverjuke main window active and not overlapped?
		if( g_mainFrame->IsAnyDialogOpened() ) { return false; }

		//return g_mainFrame->IsActive(); -- does not work on MAC
		return wxTheApp->IsActive();
	}

private:
	SjVisOverlay* m_vo;
	wxRect        m_lastRect;
};
#endif



/*******************************************************************************
 * The overlay windows
 ******************************************************************************/


#if SJ_OVERLAY_BASE==2
	#define PARENT_WINDOW_CLASS wxPopupWindow
#else
	#define PARENT_WINDOW_CLASS wxWindow
#endif


#define IDM_SCROLLTIMER (IDM_FIRSTPRIVATE+1)


class SjVoWindow : public PARENT_WINDOW_CLASS
{
public:
	SjVoWindow(wxWindow* parent, int id) :
		#if SJ_OVERLAY_BASE==2
			wxPopupWindow(parent,
		#else
			wxWindow(parent, wxID_ANY, wxPoint(-1000,-1000), wxSize(10,10),
		#endif
		wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE
		)
	{
		Hide();
		SetId(id);

		m_mouseDown = false;

		#define BG_GRAY 0x22
		#define FG_GRAY 0xEE
		m_bgColour.Set(BG_GRAY, BG_GRAY, BG_GRAY);
		m_bgBrush.SetColour(m_bgColour);
		m_bgPen.SetColour(m_bgColour);
		m_fgColour.Set(FG_GRAY, FG_GRAY, FG_GRAY);

		m_font = wxFont(10, wxSWISS, wxNORMAL, wxBOLD, FALSE, g_mainFrame->GetBaseFontFace());
		m_textYoff = 0;
		m_textCalculatedWidth = 0;

		m_scroll = 	(g_visModule->m_visFlags&SJ_VIS_FLAGS_OVERLAY_SCROLL)? -1/*don't know*/ : 0/*no scrolling*/;
		m_scrollX = 0;
		m_scrollViewportWidth = 0;
		m_scrollVirtualWidth = 0;
		m_scrollTimer.SetOwner(this, IDM_SCROLLTIMER);

		#define COVER_BORDER 4
	}

	~SjVoWindow()
	{
		if( GetId() == IDW_OVERLAY_COVER ) {
			g_mainFrame->m_imgThread->RequireKill(this);
		}
	}


	bool m_mouseDown;
	void OnMouseLeftDown(wxMouseEvent& e)
	{
		m_mouseDown = true;
	}
	void OnMouseLeftUp(wxMouseEvent& e)
	{
		if( !m_mouseDown || g_mainFrame == NULL || g_visModule == NULL ) { return; }
		if( g_visModule->IsOverWorkspace() ) { g_visModule->StopOrCloseRequest(); }
	}


	wxColour m_bgColour;
	wxBrush  m_bgBrush;
	wxPen    m_bgPen;
	wxColour m_fgColour;
	wxBitmap m_bitmap;
	wxFont   m_font;
	wxString m_text;
	int      m_textYoff, m_textCalculatedWidth;

	void OnImageThere(SjImageThereEvent& event)
	{
		SjImgThreadObj* obj = event.GetObj();

		wxBitmap* bitmap = obj->CreateBitmap();
		if( bitmap )
		{
			m_bitmap = *bitmap;
			delete bitmap;

			if( IsShown() )
			{
				wxClientDC dc(this);
				dc.DrawBitmap(m_bitmap, COVER_BORDER, COVER_BORDER, FALSE/*not transparent*/);
			}
		}

		g_mainFrame->m_imgThread->ReleaseImage(this, obj);
	}

	void OnEraseBackground(wxEraseEvent&)
	{
	}

	void OnPaint(wxPaintEvent& e)
	{
		wxPaintDC dc(this);
		wxSize size = GetClientSize();
		dc.SetBrush(m_bgBrush);
		dc.SetPen(m_bgPen);
		dc.DrawRectangle(0, 0, size.x, size.y);

		if( GetId() == IDW_OVERLAY_COVER )
		{
			if( m_bitmap.Ok() ) {
				dc.DrawBitmap(m_bitmap, COVER_BORDER, COVER_BORDER, FALSE/*not transparent*/);
			}
		}
		else
		{
			dc.SetFont(m_font);
			dc.SetTextBackground(m_bgColour);
			dc.SetTextForeground(m_fgColour);

			wxString textToDraw = m_text;

			if( m_scroll == -1 /*don't know*/ )
			{
				wxSize dcSize = dc.GetSize();
				wxCoord orgTextWidth, modTextWidth, height;
				dc.GetTextExtent(m_text, &orgTextWidth, &height);
				if( orgTextWidth > dcSize.x )
				{
					#define SCROLL_BEFORE_REPEAT " - "
					textToDraw = m_text+SCROLL_BEFORE_REPEAT;
					dc.GetTextExtent(textToDraw, &modTextWidth, &height);
					textToDraw = textToDraw+textToDraw; // make sure, the initial drawing is complete

					#define SCROLL_TIMER_MS  75
					#define SCROLL_MOVE_PX    5
					m_scroll             = 1; /*use scrolling*/
					m_scrollVirtualWidth = modTextWidth;
					m_scrollViewportWidth= dcSize.x;
					m_scrollX            = -orgTextWidth;
					m_scrollTimer.Start(SCROLL_TIMER_MS, false/*continuous*/);
				}
				else
				{
					m_scroll = 0; /*no scrolling required*/
				}
			}

			dc.DrawText(textToDraw, m_scrollX, 0-m_textYoff);
		}
	}

	int     m_scroll;
	long    m_scrollX, m_scrollViewportWidth, m_scrollVirtualWidth;
	wxTimer m_scrollTimer;

	void OnScrollTimer(wxTimerEvent& e)
	{
		wxClientDC dc(this);

		m_scrollX -= SCROLL_MOVE_PX;
		if( m_scrollX < -m_scrollVirtualWidth ) {
			m_scrollX += m_scrollVirtualWidth;
		}

		dc.SetBackgroundMode(wxPENSTYLE_SOLID);
		dc.SetFont(m_font);
		dc.SetTextBackground(m_bgColour);
		dc.SetTextForeground(m_fgColour);

		dc.DrawText(m_text+SCROLL_BEFORE_REPEAT+m_text+SCROLL_BEFORE_REPEAT, m_scrollX, 0-m_textYoff);
	}

	DECLARE_EVENT_TABLE();
};


BEGIN_EVENT_TABLE(SjVoWindow, PARENT_WINDOW_CLASS)
	EVT_IMAGE_THERE         (SjVoWindow::OnImageThere       )
	EVT_ERASE_BACKGROUND    (SjVoWindow::OnEraseBackground  )
	EVT_PAINT               (SjVoWindow::OnPaint            )
	EVT_LEFT_DOWN           (SjVoWindow::OnMouseLeftDown    )
	EVT_LEFT_UP             (SjVoWindow::OnMouseLeftUp      )
	EVT_TIMER               (IDM_SCROLLTIMER, SjVoWindow::OnScrollTimer)
END_EVENT_TABLE()


/*******************************************************************************
 * SjVisOverlay
 ******************************************************************************/


SjVisOverlay::SjVisOverlay()
{
	m_freshOpened = true;
	m_actionTimer = new SjVoActionTimer(this);

	#if SJ_OVERLAY_BASE==2
		m_popupTimer  = new SjVoPopupTimer(this);
	#endif
};


SjVisOverlay::~SjVisOverlay()
{
	CloseOverlay();

	delete m_actionTimer;

	#if SJ_OVERLAY_BASE==2
		delete m_popupTimer;
	#endif
}


void SjVisOverlay::AllocateOverlay(wxWindow* parent)
{
	#if SJ_OVERLAY_BASE!=0
		parent = parent->GetParent(); // for popups and siblings, we need the parent's parent
	#endif

	new SjVoWindow(parent, IDW_OVERLAY_COVER);
	new SjVoWindow(parent, IDW_OVERLAY_ARTIST_NAME);
	new SjVoWindow(parent, IDW_OVERLAY_TRACK_NAME);

	#if SJ_OVERLAY_BASE==2
		m_popupTimer->StartWatchingPosition();
	#endif
}


void SjVisOverlay::ShowOverlay()
{
	SjVoWindow *coverWindow = GetCoverWindow(), *artistNameWindow = GetArtistNameWindow(), *trackNameWindow = GetTrackNameWindow();
	if( !coverWindow || !artistNameWindow || !trackNameWindow ) { return; }

	UpdatePositions();

	coverWindow->Show();
	artistNameWindow->Show();
	trackNameWindow->Show();
}


void SjVisOverlay::CloseOverlay()
{
	m_actionTimer->Stop();

	#if SJ_OVERLAY_BASE==2
		m_popupTimer->Stop();
	#endif

	SjVoWindow* coverWindow = GetCoverWindow();
	if( coverWindow ) {
		coverWindow->Hide();
		coverWindow->Destroy();
	}

	SjVoWindow* artistNameWindow = GetArtistNameWindow();
	if( artistNameWindow ) {
		artistNameWindow->Hide();
		artistNameWindow->Destroy();
	}

	SjVoWindow* trackNameWindow = GetTrackNameWindow();
	if( trackNameWindow ) {
		trackNameWindow->Hide();
		trackNameWindow->Destroy();
	}

	m_leadArtistName.Empty();
	m_trackName.Empty();
}


void SjVisOverlay::TrackOnAirChanged(wxWindow* parent, long delayMs)
{
	if( !(g_visModule->m_visFlags&SJ_VIS_FLAGS_OVERLAY) ) {
		return;  // overlay disabled, do _really_ nothing
	}

	if( GetArtistNameWindow()==NULL) {
		CloseOverlay(); // if the overlay was closed by other reasons in between, clean up
	}

	#if SJ_OVERLAY_BASE==2
		if( !m_popupTimer->IsActiveInOurMeans() ) {
			return; // if the main window is not active, otheres may lay over it, do not show the overlay in this case
		}
	#endif

	SjPlaylistEntry& urlInfo = g_mainFrame->GetQueueInfo(-1);
	wxString newTrackName = urlInfo.GetTrackName();
	wxString newLeadArtistName = urlInfo.GetLeadArtistName();

	if( newTrackName.IsEmpty() )
	{
		// nothing on air
		CloseOverlay();
	}
	else
	{
		// sth. on air
		if( newTrackName != m_trackName || newLeadArtistName != m_leadArtistName )
		{
			CloseOverlay();

			m_trackName = newTrackName;
			m_leadArtistName = newLeadArtistName;
			m_coverUrl = g_mainFrame->m_columnMixer.GetTrackCoverUrl(urlInfo.GetUrl());

			if( delayMs == -1 )
			{
				// by default, we show the overlay a little delayed _after_ the song really starts (this is eg. after crossfade)
				m_delayMs = 0;
				if( !m_freshOpened && g_mainFrame->m_player.GetAutoCrossfade() ) {
					m_delayMs = g_mainFrame->m_player.m_autoCrossfadeMs;
					if( g_mainFrame->m_player.GetOnlyFadeOut() ) {
						m_delayMs /= 3; // on "only fade out", the song is earlier "active"
					}
				}
				if( m_delayMs < 2000 ) {
					m_delayMs = 2000;
				}
			}
			else
			{
				m_delayMs = delayMs;
			}

			m_freshOpened = false;

			AllocateOverlay(parent);

			// We need a delay in first initialisation as we have to wait until the parent is really realized (shown on screen with valid position).
			// For easier implementation, we just use the delay always - this also makes the cover appear without an delay normally.
			m_actionTimer->SetAction(TIMER_ACTION_REQUIRE_COVER);
			m_actionTimer->Start(m_delayMs/2, true/*one shot*/);
		}
	}
}


void SjVisOverlay::RequireCover()
{
	SjVoWindow *coverWindow = GetCoverWindow(), *artistNameWindow = GetArtistNameWindow(), *trackNameWindow = GetTrackNameWindow();
	if( !coverWindow || !artistNameWindow || !trackNameWindow ) { return; }

	wxRect coverRect, artistNameRect, trackNameRect;
	CalcPositions(coverRect, artistNameRect, trackNameRect);

	g_mainFrame->m_imgThread->RequireStart(coverWindow);
		SjImgOp op;
		op.LoadFromDb(m_coverUrl);
		op.m_flags |= SJ_IMGOP_RESIZE|SJ_IMGOP_SMOOTH;
		op.m_resizeW = coverRect.width-COVER_BORDER*2;
		op.m_resizeH = coverRect.height-COVER_BORDER*2;
		SjImgThreadObj* obj = g_mainFrame->m_imgThread->RequireImage(coverWindow, m_coverUrl, op);
		if( obj )
		{
			wxBitmap* bitmap = obj->CreateBitmap();
			coverWindow->m_bitmap = *bitmap;
			delete bitmap;

			g_mainFrame->m_imgThread->ReleaseImage(coverWindow, obj);
		}
	g_mainFrame->m_imgThread->RequireEnd(coverWindow);

	// set a fine font height
	wxClientDC dc(artistNameWindow); // use _any_ dc
	for( int i = 0; i <= 1; i++ ) {
		SjVoWindow* window = i==0?  artistNameWindow :  trackNameWindow;
		wxRect*     rect   = i==0? &artistNameRect   : &trackNameRect;
		if( window->m_textCalculatedWidth == 0 ) {
			window->m_textYoff = rect->height/10;
			window->m_text = i==0? m_leadArtistName : m_trackName;
			window->m_text.MakeUpper();
			int fontPtSize = 64;
			while( 1 ) {
				wxCoord w, h;
				window->m_font.SetPointSize(fontPtSize);
				dc.SetFont(window->m_font);
				dc.GetTextExtent(window->m_text, &w, &h);
				dc.SetFont(*wxNORMAL_FONT);
				if( h<=rect->height+(window->m_textYoff*2) || fontPtSize<=6 ) {
					window->m_textCalculatedWidth = w;
					break;
				}
				fontPtSize--;
			}
		}
	}
}


void SjVisOverlay::CalcPositions(wxRect& coverRect, wxRect& artistNameRect, wxRect& trackNameRect)
{
	SjVoWindow *artistNameWindow = GetArtistNameWindow(), *trackNameWindow = GetTrackNameWindow();
	if( !trackNameWindow ) { return; }

	wxWindow* parent = artistNameWindow->GetParent();

	#if SJ_OVERLAY_BASE==2
		wxRect r = parent->GetScreenRect();
	#else
		wxRect r = parent->GetClientRect();
	#endif

	int smaller_side = r.width<r.height? r.width : r.height;

	int border_w = r.width/20;
	int border_h = smaller_side/20;
	int cover_wh = (int)((float)smaller_side*0.40);
	int title_h = smaller_side/10;

	coverRect = wxRect(r.x+border_w, r.y+r.height-border_h-cover_wh, cover_wh, cover_wh);

	// track name rect
	#define MAX_TEXT_WIDTH (r.width-border_w*3-cover_wh)
	int titleW = MAX_TEXT_WIDTH;
	if( trackNameWindow->m_textCalculatedWidth > 0 && trackNameWindow->m_textCalculatedWidth < titleW ) {
		titleW = trackNameWindow->m_textCalculatedWidth;
	}
	trackNameRect = wxRect(r.x+r.width-border_w-titleW, r.y+r.height-border_h-title_h, titleW, title_h);

	// finally, artist name rect
	artistNameRect = trackNameRect;
	artistNameRect.height = (int)((float)trackNameRect.height*0.66F);
	artistNameRect.y -= artistNameRect.height+(artistNameRect.height/4);

	artistNameRect.width = MAX_TEXT_WIDTH;
	if( artistNameWindow->m_textCalculatedWidth > 0 && artistNameWindow->m_textCalculatedWidth < artistNameRect.width ) {
		artistNameRect.width = artistNameWindow->m_textCalculatedWidth;
	}
	artistNameRect.x = r.x+r.width-border_w-artistNameRect.width;
}


void SjVisOverlay::UpdatePositions()
{
	SjVoWindow *coverWindow = GetCoverWindow(), *artistNameWindow = GetArtistNameWindow(), *trackNameWindow = GetTrackNameWindow();
	if( !coverWindow || !artistNameWindow || !trackNameWindow ) { return; }

	wxRect coverRect, artistNameRect, trackNameRect;
	CalcPositions(coverRect, artistNameRect, trackNameRect);

	if( g_visModule->m_visFlags&SJ_VIS_FLAGS_OVERLAY_COVER ) {
		coverWindow->SetSize(coverRect);
	}
	else {
		coverWindow->SetSize(wxRect(-1000,-1000,10,10));
	}

	if( g_visModule->m_visFlags&SJ_VIS_FLAGS_OVERLAY_ARTIST ) {
		artistNameWindow->SetSize(artistNameRect);
	}
	else {
		artistNameWindow->SetSize(wxRect(-1000,-1000,10,10));
	}

	trackNameWindow->SetSize(trackNameRect);
}
