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
 * File:    monitor_overview.h
 * Authors: Björn Petersen
 * Purpose: The monitor-overview control
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/kiosk/monitor_overview.h>
#include <sjmodules/kiosk/kiosk.h>
#include <sjmodules/vis/vis_bg.h>

#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArrayMonitorOvItem);


#define IDC_FOR_MAIN        (IDM_FIRSTPRIVATE+1)
#define IDC_FOR_VIS         (IDM_FIRSTPRIVATE+2)


BEGIN_EVENT_TABLE(SjMonitorOverview, wxWindow)
	EVT_SIZE            (               SjMonitorOverview::OnSize               )
	EVT_ERASE_BACKGROUND(               SjMonitorOverview::OnEraseBackground    )
	EVT_PAINT           (               SjMonitorOverview::OnPaint              )
	EVT_RIGHT_UP        (               SjMonitorOverview::OnMouseRightUp       )
	EVT_LEFT_DCLICK     (               SjMonitorOverview::OnMouseLeftDClick    )
	EVT_MENU            (IDC_FOR_MAIN,  SjMonitorOverview::OnMenuSelect         )
	EVT_MENU            (IDC_FOR_VIS,   SjMonitorOverview::OnMenuSelect         )
END_EVENT_TABLE()


SjMonitorOverview::SjMonitorOverview(wxWindow* parent, int id, long style)
	: wxWindow(parent, id, wxDefaultPosition, wxDefaultSize, style | wxFULL_REPAINT_ON_RESIZE)
{
	#define CTRL_WIDTH 320
	#define CTRL_HEIGHT 80
	SetSizeHints(CTRL_WIDTH, CTRL_HEIGHT, -1, -1);
	SetSize(CTRL_WIDTH, CTRL_HEIGHT);

	m_fontNumber = wxFont(10/*recalculated later*/, wxFONTFAMILY_SWISS, wxNORMAL/*style*/, wxBOLD/*weight*/, false/*underline*/);
	m_fontIcon   = wxFont(10/*recalculated later*/, wxFONTFAMILY_ROMAN, wxFONTSTYLE_ITALIC/*style*/, wxBOLD/*weight*/, false/*underline*/);

	m_nonDummy = 0;
}


void SjMonitorOverview::Clear()
{
	m_monitors.Clear();
	m_nonDummy = 0;
	CalcPositions();
}


void SjMonitorOverview::AddMonitor(const wxRect& geom)
{
	SjMonitorOvItem* item = new SjMonitorOvItem;
	item->m_monitorGeom = geom;
	item->m_monitorUsage = 0;
	m_nonDummy++;

	m_monitors.Add(item);
	CalcPositions();
}


void SjMonitorOverview::AddDummy()
{
	SjMonitorOvItem* item = new SjMonitorOvItem;
	item->m_monitorGeom = wxRect(0, 0, 640, 480);
	item->m_monitorUsage = MONITOR_USAGE_DUMMY;
	int m, mCount = m_monitors.GetCount(), largestX = -1000000L;
	for( m = 0; m < mCount; m++ )
	{
		if( m_monitors[m].m_monitorGeom.x > largestX )
		{
			item->m_monitorGeom = m_monitors[m].m_monitorGeom;
			item->m_monitorGeom.x += item->m_monitorGeom.width;
			largestX = m_monitors[m].m_monitorGeom.x;
		}
	}

	m_monitors.Add(item);
	CalcPositions();
}


void SjMonitorOverview::SetMonitorUsage(long index, long flag, bool set)
{
	if( index >= 0 && index < (long)m_monitors.GetCount() )
	{
		SjTools::SetFlag(m_monitors[index].m_monitorUsage, flag, set);
	}
}


void SjMonitorOverview::OnSize(wxSizeEvent&)
{
	CalcPositions();
	Refresh();
}


void SjMonitorOverview::CalcPositions()
{
	// overall calculations based on the given geometries
	wxRect geomOverall;
	long geomOffsetX = 0x7FFFFFFFL, geomOffsetY = 0x7FFFFFFFL, geomMinH = 0x7FFFFFFFL, geomMinW = 0x7FFFFFFFL;
	long m, mCount = m_monitors.GetCount();
	if( mCount <= 0 ) // seems not to be yet initialized, needed for Win98, see http://www.silverjuke.net/forum/topic-1522.html
		return;

	for( m = 0; m < mCount; m++ )
	{
		wxRect& geom = m_monitors[m].m_monitorGeom;

		if( geom.x < geomOffsetX ) geomOffsetX = geom.x;
		if( geom.y < geomOffsetY ) geomOffsetY = geom.y;
		if( geom.width < geomMinW ) geomMinW = geom.width;
		if( geom.height < geomMinH ) geomMinH = geom.height;

		if( m )
			geomOverall.Union(geom);
		else
			geomOverall = geom;
	}

	// scale
	wxSize  clientSize = GetClientSize();
	float   scale;
	{
		long    border = 4;
		wxSize  dcSize = clientSize;
		dcSize.x -= border*2;
		dcSize.y -= border*2;
		if( dcSize.y <= 0 ) // we divide through dcSize.y below ...
			return;

		float   dcAspect = (float)dcSize.x / (float)dcSize.y;
		float   orgAspect = (float)geomOverall.width / (float)geomOverall.height;
		if( dcAspect > orgAspect )
		{
			// scale by height
			scale = ((float)dcSize.y) / ((float)geomOverall.height);
		}
		else
		{
			// scale by width
			scale = ((float)dcSize.x) / ((float)geomOverall.width);
		}
	}

	// avoid too much zooming (looks ugly)
	#define MAX_W 300
	#define MAX_H 300
	if( scale > 0.0F && geomOverall.width*scale > MAX_W )
	{
		scale = float(MAX_W) / float(geomOverall.width);
	}

	if( scale > 0.0F && geomOverall.height*scale > MAX_H )
	{
		scale = float(MAX_H) / float(geomOverall.height);
	}

	// space
	long space = (long) ((float)geomOverall.height*scale*0.02F);
	if( space < 1 ) space = 1;

	// calculate all pixel rectangles
	long borderLeft = (long)( (clientSize.x-((float)geomOverall.width *scale)) / 2 );
	long borderTop  = (long)( (clientSize.y-((float)geomOverall.height*scale)) / 2 );

	long smallestDrawW = 0x7FFFFFFFL, smallestDrawH = 0x7FFFFFFFL;
	for( m = 0; m < mCount; m++ )
	{
		wxRect& geom = m_monitors[m].m_monitorGeom;
		wxRect& drawRect = m_monitors[m].m_drawRect;

		drawRect.x = borderLeft + (long)((float)(geom.x - geomOffsetX)*scale);
		drawRect.y = borderTop  + (long)((float)(geom.y - geomOffsetY)*scale);
		drawRect.width  = (long)((float)geom.width  * scale);
		drawRect.height = (long)((float)geom.height * scale);
		drawRect.Deflate(space);

		if( drawRect.width  < smallestDrawW ) smallestDrawW = drawRect.width;
		if( drawRect.height < smallestDrawH ) smallestDrawH = drawRect.height;
	}

	// calculate the font
	wxSize wantedFontSize(smallestDrawW-2, wxCoord(float(smallestDrawH)*0.8));
	if( wantedFontSize != m_fontPxSize )
	{
		wxClientDC dc(this);
		SjVisBg::SetFontPixelH(dc, m_fontNumber, wantedFontSize.y, 128);
		m_fontPxSize = wantedFontSize;
	}
}


void SjMonitorOverview::DoPaint(wxDC& dc__)
{
	wxSize      clientSize = GetClientSize();

	wxColour    bgColour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	wxBrush     bgBrush(bgColour, wxSOLID);

	wxColour    monitorBorderColour(128, 128, 128);;
	wxPen       monitorBorderPen(monitorBorderColour);

	wxColour    monitorBgColour(0xF6, 0x8A, 0x8F);
	wxPen       monitorBgPen(monitorBgColour);
	wxColour    monitorBgDummyColour(0xEB, 0xEB, 0xEB);

	wxColour    monitorIconColour(0xF0, 0x40, 0x49);
	wxBrush     monitorIconBrush(monitorIconColour);
	wxPen       monitorIconPen(monitorIconColour);

	#define     digitColour *wxWHITE

	wxDC* drawDc = &dc__;

	// prepare offscreen drawing
	if( !m_offscreenBitmap.IsOk()
	 ||  m_offscreenBitmap.GetWidth() != clientSize.x
	 ||  m_offscreenBitmap.GetHeight() != clientSize.y )
	{
		m_offscreenBitmap.Create(clientSize.x, clientSize.y);
		m_offscreenDc.SelectObject(m_offscreenBitmap);
	}

	drawDc = &m_offscreenDc;

	// draw the background
	drawDc->SetPen(*wxTRANSPARENT_PEN);
	drawDc->SetBrush(bgBrush);
	drawDc->DrawRectangle(0, 0, clientSize.x, clientSize.y);
	drawDc->SetBackgroundMode(wxTRANSPARENT);

	// draw the monitors
	int m, mCount = m_monitors.GetCount();
	for( m = 0; m < mCount; m++ )
	{
		SjMonitorOvItem&    item = m_monitors[m];
		bool                isDummy = (item.m_monitorUsage & MONITOR_USAGE_DUMMY) != 0;
		wxRect&             drawRect = item.m_drawRect;

		wxBrush monitorBgBrush(isDummy? monitorBgDummyColour : monitorBgColour, wxSOLID);
		drawDc->SetBrush(monitorBgBrush);
		drawDc->SetPen(monitorBorderPen);

		drawDc->DrawRectangle(drawRect);

		// draw usage scheme
		drawDc->SetPen(monitorIconPen);
		drawDc->SetBrush(monitorIconBrush);
		if( item.m_monitorUsage & MONITOR_USAGE_MAIN )
		{
			DrawBg(*drawDc, MONITOR_USAGE_MAIN, drawRect, false);
		}

		if( (item.m_monitorUsage & MONITOR_USAGE_VIS) && m_nonDummy > 1 )
		{
			SjVisBg::SetFontPixelH(*drawDc, m_fontIcon, wxCoord(float(drawRect.height)*0.3), 64);
			drawDc->SetPen(monitorBgPen);
			drawDc->SetFont(m_fontIcon);
			drawDc->SetTextForeground(monitorBgColour);
			DrawBg(*drawDc, MONITOR_USAGE_VIS, drawRect, !(item.m_monitorUsage & MONITOR_USAGE_MAIN));
		}

		// draw text
		wxCoord textW, textH;
		drawDc->SetFont(m_fontNumber);
		drawDc->SetTextForeground(digitColour);

		wxString str = wxString::Format(wxT("%i"), (int)(m+1));
		drawDc->GetTextExtent(str, &textW, &textH);
		drawDc->DrawText(str, drawRect.x+drawRect.width/2-textW/2, drawRect.y+drawRect.height/2-textH/2);
	}

	// finish offscreen drawing
	dc__.Blit(0, 0, clientSize.x, clientSize.y, &m_offscreenDc, 0, 0);
}


void SjMonitorOverview::DrawBg(wxDC& dc, long what, const wxRect& drawRect, bool full)
{
	#define NUM_COVERS 4
	#define COVER_LINES_DEFINDED 3
	long lineCounts[COVER_LINES_DEFINDED] = { 5, 7, 6 };
	float coverW = float(drawRect.width) / float(NUM_COVERS+1);
	float space = (float(drawRect.width) - coverW*float(NUM_COVERS)) / float(NUM_COVERS+1);
	float lineH = space / 2; if( lineH < 1.0 ) lineH = 1.0;

	if( what == MONITOR_USAGE_MAIN )
	{
		float x = float(drawRect.x) + space;
		for( int c = 0; c < NUM_COVERS; c++ )
		{
			float y = float(drawRect.y) + space;

			dc.DrawRectangle((long)x, (long)y, (long)coverW, (long)coverW);
			y += coverW+lineH;

			for( int l = 0; l < lineCounts[c%COVER_LINES_DEFINDED]; l++ )
			{
				if( y >= drawRect.y+drawRect.height )
					break;
				dc.DrawRectangle((long)x, (long)y, (long)coverW, (long)lineH);
				y += lineH*2;
			}

			x += coverW+space;
		}
	}
	else
	{
		long border = long(space*2.0);
		wxRect rect = drawRect;
		rect.Deflate(border);
		if( !full )
		{
			rect.y += rect.height/2 + (border/2);
			rect.height /= 2;
		}
		dc.DrawRectangle(rect);

		wxString text = /*_("Karaoke, Vis. etc");*/wxT("Ob-la-di Ob-la-da life goes on bra Lala");
		wxCoord w, h, decent;
		dc.GetTextExtent(text, &w, &h, &decent);
		dc.SetClippingRegion(rect);
		dc.DrawText(text, rect.x-h/4, rect.y+rect.height-h+decent/2);
		dc.DestroyClippingRegion();
	}
}



/*void SjMonitorOverview::DrawVisBg(wxDC& dc, const wxRect& drawRect, bool full)
{
    wxString text = _("Karaoke, Vis. etc");//wxT("Ob-la-di Ob-la-da life goes on bra Lala");
    wxCoord w, h;
    dc.GetTextExtent(text, &w, &h);
    dc.SetClippingRegion(drawRect);
    dc.DrawText(text, drawRect.x-h/4, drawRect.y+drawRect.height-h);
    dc.DestroyClippingRegion();
}*/


long SjMonitorOverview::FindMonitorByPoint(int x, int y) const
{
	long m, mCount = (long)m_monitors.GetCount();
	for( m = 0; m < mCount; m++ )
	{
		if( m_monitors[m].m_drawRect.Contains(x, y) )
			return m;
	}
	return -1;
}


void SjMonitorOverview::OnContextMenu(int x, int y)
{
	m_affectedIndex = FindMonitorByPoint(x, y);
	if( m_affectedIndex != -1 )
	{
		SjMenu menu(0);

		menu.AppendCheckItem(IDC_FOR_MAIN, _("Main window"));
		menu.AppendCheckItem(IDC_FOR_VIS, _("Video screen"));

		long monitorUsage =  m_monitors[m_affectedIndex].m_monitorUsage;
		if( monitorUsage & MONITOR_USAGE_DUMMY )
		{
			menu.Enable(IDC_FOR_MAIN, false);
			menu.Enable(IDC_FOR_VIS, false);
		}
		else
		{
			menu.Check(IDC_FOR_MAIN, (monitorUsage & MONITOR_USAGE_MAIN)!=0);
			menu.Check(IDC_FOR_VIS, (monitorUsage & MONITOR_USAGE_VIS)!=0);
		}

		PopupMenu(&menu, ScreenToClient(::wxGetMousePosition()));
	}
}


void SjMonitorOverview::OnMenuSelect(wxCommandEvent& e)
{
	long mCount = (long)m_monitors.GetCount();
	if( m_affectedIndex >= 0 && m_affectedIndex < mCount )
	{
		long flag = e.GetId()==IDC_FOR_MAIN? MONITOR_USAGE_MAIN : MONITOR_USAGE_VIS;
		wxCommandEvent fwd(wxEVT_COMMAND_MENU_SELECTED, GetId());
		fwd.SetExtraLong(flag);
		if( !(m_monitors[m_affectedIndex].m_monitorUsage&flag) )
		{
			fwd.SetInt(m_affectedIndex);
			AddPendingEvent(fwd);
		}
		else if( mCount > 1 )
		{
			fwd.SetInt((m_affectedIndex+1)%mCount);
			AddPendingEvent(fwd);
		}
	}
}


void SjMonitorOverview::OnMouseLeftDClick(wxMouseEvent& e)
{
	if( FindMonitorByPoint(e.GetX(), e.GetY()) != -1 )
	{
		wxCommandEvent fwd(wxEVT_COMMAND_MENU_SELECTED, GetId());
		AddPendingEvent(fwd);
	}
}
