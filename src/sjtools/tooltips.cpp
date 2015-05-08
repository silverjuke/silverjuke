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
 * File:    tooltipssj.cpp
 * Authors: Björn Petersen
 * Purpose: Tool tip handling
 *
 *******************************************************************************
 *
 * Notes:
 * 23.01.2005   Tooltips are closed/not opened if the mouse is over another
 *              window.  As we do not capture the mouse, we check this in
 *              a timer.
 * 23.01.2005   Tooltips are opened only at different mouse positions,
 *              see ArePointsNearTogether().
 *
 ******************************************************************************/


#include <sjbase/base.h>
#if SJ_USE_TOOLTIPS


#define TOOLTIPWINDOW_DELAYOPEN          800
#define TOOLTIPWINDOW_STAYOPEN_MIN      5000
#define TOOLTIPWINDOW_STAYOPEN_PER_CHAR  150
#define TOOLTIPWINDOW_WAITFASTREOPEN     600 //500
#define TOOLTIPWINDOW_DELAYFASTREOPEN     80 //120

#undef DEBUG_TOOLTIPS // define for tooltip wxLogDebug() output


/*******************************************************************************
 * SjTipWindow
 ******************************************************************************/


class SjTipWindow : public wxFrame
{
public:
	// the mandatory ctor parameters are: the parent window and the text to
	// show
	//
	// optionally you may also specify the length at which the lines are going
	// to be broken in rows (100 pixels by default)
	SjTipWindow         (wxWindow *parent,
	                     const wxString& text,
	                     wxCoord maxLength,
	                     const wxRect& screenRect);

	virtual         ~SjTipWindow        ();
	wxRect          GetScreenRect       () { return m_screenRect; }

	// calculate the client rect we need to display the text
	// the magin is okay for MSW, may be adjusted for other OS
	#define         MARGIN_LEFT         3
	#define         MARGIN_RIGHT        2
	#define         MARGIN_TOP          2
	#define         MARGIN_BOTTOM       0
	void            Adjust              (const wxString& text, wxCoord maxLength);

protected:

	// event handlers
	void            OnPaint             (wxPaintEvent& event);
	void            OnMouseClick        (wxMouseEvent& event);
	void            OnCheckMouseTimer   (wxTimerEvent& event);

private:
	wxTimer         m_checkMouseTimer;
	wxArrayString   m_textLines;
	wxCoord         m_heightLine;
	wxRect          m_screenRect;
	DECLARE_EVENT_TABLE ()
	DECLARE_NO_COPY_CLASS(SjTipWindow)
};


#define IDM_CHECKMOUSETIMER (IDM_FIRSTPRIVATE+0)


BEGIN_EVENT_TABLE(SjTipWindow, wxFrame)
	EVT_PAINT       (SjTipWindow::OnPaint)

	EVT_LEFT_DOWN   (SjTipWindow::OnMouseClick)
	EVT_RIGHT_DOWN  (SjTipWindow::OnMouseClick)
	EVT_MIDDLE_DOWN (SjTipWindow::OnMouseClick)

	EVT_TIMER       (IDM_CHECKMOUSETIMER, SjTipWindow::OnCheckMouseTimer)
END_EVENT_TABLE()


SjTipWindow::SjTipWindow(wxWindow* parent, const wxString& text, wxCoord maxLength, const wxRect& screenRect)
	: wxFrame(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
	          wxNO_BORDER|wxFRAME_NO_TASKBAR|wxFRAME_FLOAT_ON_PARENT )
{
	m_screenRect = screenRect;

	// set tooltip size
	Adjust(text, maxLength);

	// init the tooltiop position to the mouse position
	//
	// we want to show the tip below the mouse, not over it
	//
	// NB: the reason we use "/ 2" here is that we don't know where the current
	//     cursors hot spot is... it would be nice if we could find this out
	//     though
	int x, y, dy = (wxSystemSettings::GetMetric(wxSYS_CURSOR_Y) / 3) * 2;
	::wxGetMousePosition(&x, &y);

	y += dy;

	// check x and y against the screen size
	wxRect displayRect = SjDialog::GetDisplayWorkspaceRect(x, y);
	wxSize tooltipSize = GetSize();

	if( x + tooltipSize.x > displayRect.x + displayRect.width )
	{
		x = displayRect.x + displayRect.width - tooltipSize.x;
	}

	if( y + tooltipSize.y > displayRect.y + displayRect.height )
	{
		y -= dy + tooltipSize.y + 3;
	}

	// move and show the tootip window
	Move(x, y);
	Show(true);

	// init timers
	m_checkMouseTimer.SetOwner(this, IDM_CHECKMOUSETIMER);
	m_checkMouseTimer.Start(333); // check the mouse position 3 times a second
}


SjTipWindow::~SjTipWindow()
{
	if( g_tools )
	{
		g_tools->m_toolTipManager.m_fastReopenMsTicks = SjTools::GetMsTicks() + TOOLTIPWINDOW_WAITFASTREOPEN;
	}
}


void SjTipWindow::Adjust(const wxString& text, wxCoord maxLength)
{
	wxClientDC dc(this);
	dc.SetFont(GetFont());

	// calculate the length: we want each line be no longer than maxLength
	// pixels and we only break lines at words boundary
	wxString current;
	wxCoord height, width, widthMax = 0;

	m_heightLine = 0;

	bool breakLine = false;
	for ( const wxChar *p = static_cast<const wxChar*>(text.c_str()); ; p++ )
	{
		if ( *p == _T('\n') || *p == _T('\0') )
		{
			dc.GetTextExtent(current, &width, &height);
			if ( width > widthMax )
				widthMax = width;

			if ( height > m_heightLine )
				m_heightLine = height;

			m_textLines.Add(current);

			if ( !*p )
			{
				// end of text
				break;
			}

			current.clear();
			breakLine = false;
		}
		else if ( breakLine && (*p == _T(' ') || *p == _T('\t')) )
		{
			// word boundary - break the line here
			m_textLines.Add(current);
			current.clear();
			breakLine = false;
		}
		else // line goes on
		{
			current += *p;
			dc.GetTextExtent(current, &width, &height);
			if ( width > maxLength )
				breakLine = true;

			if ( width > widthMax )
				widthMax = width;

			if ( height > m_heightLine )
				m_heightLine = height;
		}
	}

	// take into account the border size and the margins
	width  = MARGIN_LEFT + MARGIN_RIGHT + 2 + widthMax;
	height = MARGIN_TOP + MARGIN_BOTTOM + 2 + m_textLines.GetCount()*m_heightLine;

	SetSize(0, 0, width, height);
}


void SjTipWindow::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxPaintDC dc(this);

	wxRect rect;
	wxSize size = GetClientSize();
	rect.width = size.x;
	rect.height = size.y;

	// first fill the background
	dc.SetBrush(wxBrush(g_tools->m_toolTipManager.m_bgColour, wxSOLID));
	dc.SetPen(wxPen(g_tools->m_toolTipManager.m_borderColour, 1, wxSOLID));
	dc.DrawRectangle(rect);

	// and then draw the text line by line
	dc.SetTextBackground(g_tools->m_toolTipManager.m_bgColour);
	dc.SetTextForeground(g_tools->m_toolTipManager.m_fgColour);
	dc.SetFont(GetFont());

	wxPoint pt;
	pt.x = MARGIN_LEFT;
	pt.y = MARGIN_TOP;
	size_t count = m_textLines.GetCount();

	for ( size_t n = 0; n < count; n++ )
	{
		dc.DrawText(m_textLines[n], pt);
		pt.y += m_heightLine;
	}
}


void SjTipWindow::OnMouseClick(wxMouseEvent& WXUNUSED(event))
{
	g_tools->m_toolTipManager.CloseToolTipWindow();
}


void SjTipWindow::OnCheckMouseTimer(wxTimerEvent& event)
{
	// As we do not capture the mouse, we have to check the mouse position
	// by a timer.
	//
	// We assume that the caller of the tooltip functions call
	// ClearToolTipProvider() if a special range in its windows is left,
	// so we only check if the mouse is over another window here.

	static bool inHere = FALSE;

	if( !inHere )
	{
		inHere = TRUE;

		wxPoint     mousePosition = ::wxGetMousePosition();
		wxWindow*   windowUnderMouse = ::wxFindWindowAtPoint(mousePosition);

		if( SjDialog::FindTopLevel(GetParent()) != SjDialog::FindTopLevel(windowUnderMouse) )
		{
			// if the window was closed in between, this is checked by CloseToolTipWindow()
			g_tools->m_toolTipManager.CloseToolTipWindow();
		}

		inHere = FALSE;
	}
}


/*******************************************************************************
 * SjToolTipTimer Class
 ******************************************************************************/


class SjToolTipTimer : public wxTimer
{
public:
	SjToolTipTimer(SjToolTipManager* m)
		: wxTimer()
	{
		m_toolTipManager = m;
	}

	void Notify()
	{
		m_toolTipManager->Notify();
	}

private:
	SjToolTipManager* m_toolTipManager;
};


/*******************************************************************************
 * SjToolTipManager Class
 ******************************************************************************/


SjToolTipManager::SjToolTipManager()
{
	m_toolTipProvider   = NULL;
	m_toolTipWindow     = NULL;
	m_toolTipTimer      = new SjToolTipTimer(this);
	m_fastReopenMsTicks = 0;

	SetDefColours();
}


void SjToolTipManager::SetToolTipProvider(SjToolTipProvider* provider)
{
	// timer available? if not, we're eg. terminating
	// and there's nothing to do
	if( m_toolTipTimer == NULL || g_accelModule == NULL || !(g_accelModule->m_flags&SJ_ACCEL_TOOLTIPS) )
	{
		return;
	}

	#ifdef DEBUG_TOOLTIPS
		wxLogDebug("tooltip provider set to %i", (int)provider);
	#endif

	// if another tooltip is already opened, close it
	if( m_toolTipWindow )
	{
		if( provider )
		{
			wxRect oldRect = m_toolTipWindow->GetScreenRect();
			wxRect newRect = provider->GetLocalRect();
			wxWindow* parent = provider->GetWindow();
			if( parent )
			{
				parent->ClientToScreen(&newRect.x, &newRect.y);
				if( oldRect == newRect )
				{
					#ifdef DEBUG_TOOLTIPS
						wxLogDebug("tooltip provider ignored (a tooltip at the same position is already opened)");
					#endif
					return; // nothing to do, this tooltip is already opened
				}
			}
		}

		CloseToolTipWindow();
		wxASSERT(m_toolTipWindow == NULL);
	}

	// save new provider (may be NULL)
	m_toolTipProvider = provider;

	// if a provider is given, set timeout
	if( provider )
	{
		unsigned long delay = m_fastReopenMsTicks > SjTools::GetMsTicks()? TOOLTIPWINDOW_DELAYFASTREOPEN : TOOLTIPWINDOW_DELAYOPEN;
		m_toolTipTimer->Start(delay,
		                      TRUE/*one shot*/);
	}
}


void SjToolTipManager::Notify()
{
	if( m_toolTipWindow )
	{
		CloseToolTipWindow();
	}
	else
	{
		OpenToolTipWindow();
	}
}


static bool ArePointsNearTogether(const wxPoint& p1, const wxPoint& p2)
{
	#define DIFF 1
	wxRect r1(p1.x-DIFF, p1.y-DIFF, DIFF*2, DIFF*2);
	wxRect r2(p2.x-DIFF, p2.y-DIFF, DIFF*2, DIFF*2);
	return r1.Intersects(r2);
}


void SjToolTipManager::OpenToolTipWindow()
{
	#ifdef DEBUG_TOOLTIPS
		wxLogDebug("try to open tooltip (got timer)...");
	#endif

	// - if we're on exit do nothing.
	// - if the window is already opend, do nothing.
	//   (the caller should call CloseTooltip() before in this case)
	// - if there is no tool tip provider, do nothing.
	if( m_toolTipTimer == NULL || m_toolTipWindow || m_toolTipProvider == NULL )
	{
		#ifdef DEBUG_TOOLTIPS
			if(m_toolTipTimer==NULL)   wxLogDebug("...failed, no timer");
			if(m_toolTipWindow)        wxLogDebug("...failed, window already opened");
			if(m_toolTipProvider==NULL)wxLogDebug("...failed, no tooltip provider");
		#endif
		return;
	}

	// open the window only if the parent has the focus
	wxASSERT( m_toolTipProvider );
	wxASSERT( m_toolTipWindow == NULL );

	wxPoint     mousePosition = ::wxGetMousePosition();

	wxWindow*   parent = m_toolTipProvider->GetWindow();
	wxWindow*   focusWindow = wxWindow::FindFocus();
	wxWindow*   windowUnderMouse = ::wxFindWindowAtPoint(mousePosition);

	if( SjDialog::FindTopLevel(parent) != SjDialog::FindTopLevel(focusWindow)
	        || SjDialog::FindTopLevel(parent) != SjDialog::FindTopLevel(windowUnderMouse) )
	{
		#ifdef DEBUG_TOOLTIPS
			wxLogDebug("...failed, bad parent/focus/mouse window combination");
		#endif
		m_toolTipProvider = NULL;
		return;
	}

	if( ArePointsNearTogether(m_lastMousePosition, mousePosition) )
	{
		#ifdef DEBUG_TOOLTIPS
			wxLogDebug("...failed, mouse is near the old position");
		#endif
		m_toolTipProvider = NULL;
		return;
	}

	long        flags = 0;
	wxString    text = m_toolTipProvider->GetText(flags);
	wxRect      rect = m_toolTipProvider->GetLocalRect();
	if( parent == NULL || text.IsEmpty() )
	{
		#ifdef DEBUG_TOOLTIPS
			if( parent == NULL ) wxLogDebug("...failed, no parent");
			if( text.IsEmpty() ) wxLogDebug("...failed, no text");
		#endif
		m_toolTipProvider = NULL;
		return;
	}

	// set width
	if( (flags & 0x0000FFFFL) == 0 )
	{
		flags |= 160;
	}

	parent->ClientToScreen(&rect.x, &rect.y);

	long stayOpenMs = text.Len() * TOOLTIPWINDOW_STAYOPEN_PER_CHAR;
	if( stayOpenMs < TOOLTIPWINDOW_STAYOPEN_MIN )
	{
		stayOpenMs = TOOLTIPWINDOW_STAYOPEN_MIN;
	}

	//text += wxString::Format(" [%i ms open]", stayOpenMs);

	m_toolTipWindow = new SjTipWindow(parent, text, flags & 0x0000FFFFL, rect);

	if( m_toolTipWindow == NULL )
	{
		#ifdef DEBUG_TOOLTIPS
			wxLogDebug("...failed, tooltip allocation error");
		#endif
		m_toolTipProvider = NULL;
		return;
	}

	m_toolTipTimer->Start(stayOpenMs, TRUE/*one shot*/);

	#ifdef DEBUG_TOOLTIPS
		wxLogDebug("...tooltip \"%s\" opened [x=%i, y=%i, %ix%i]", text.c_str(),
				   rect.x, rect.y, rect.width, rect.height);
	#endif

	// preserve the focus
	focusWindow->SetFocus();

	m_lastMousePosition = mousePosition;

	m_toolTipProvider = NULL;
}


void SjToolTipManager::CloseToolTipWindow()
{
	static bool inHere = FALSE;

	if( !inHere )
	{
		inHere = TRUE;

		// - if we're on exit do nothing.
		if( m_toolTipTimer != NULL )
		{
			// close window
			if( m_toolTipWindow )
			{
				// hide the window
				// (this is to reflect the new state immediately as
				// destroying may be delayed)
				m_toolTipWindow->Hide();

				// destroy the window
				m_toolTipWindow->Destroy();
				m_toolTipWindow = NULL;
				#ifdef DEBUG_TOOLTIPS
					wxLogDebug("tooltip closed");
				#endif
			}
		}

		inHere = FALSE;
	}
}


void SjToolTipManager::ExitForever()
{
	// after this function is called, no tooltips will
	// ever be opened by this object; all referenced providers
	// are no longer used.
	if( m_toolTipTimer )
	{
		CloseToolTipWindow();
		wxASSERT( m_toolTipWindow == NULL );

		delete m_toolTipTimer;
		m_toolTipTimer = NULL;
		m_toolTipProvider = NULL;
	}
}


#endif // SJ_USE_TOOLTIPS
