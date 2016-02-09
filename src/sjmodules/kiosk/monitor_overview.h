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


#ifndef __SJ_MONITOR_OVERVIEW__
#define __SJ_MONITOR_OVERVIEW__


class SjMonitorOvItem
{
public:
	wxRect          m_monitorGeom;
	#define         MONITOR_USAGE_DUMMY 0x01
	#define         MONITOR_USAGE_MAIN  0x02
	#define         MONITOR_USAGE_VIS   0x04
	long            m_monitorUsage;

	wxRect          m_drawRect;
};

WX_DECLARE_OBJARRAY(SjMonitorOvItem, SjArrayMonitorOvItem);


class SjMonitorOverview : public wxWindow
{
public:
	                SjMonitorOverview   (wxWindow* parent, int id, long style);

	void            Clear               ();
	void            AddMonitor          (const wxRect&);
	void            AddDummy            ();
	void            SetMonitorUsage     (long index, long flag, bool set);

private:
	SjArrayMonitorOvItem m_monitors;
	long            m_nonDummy;
	wxSize          m_fontPxSize;
	wxFont          m_fontNumber;
	wxFont          m_fontIcon;

	wxBitmap        m_offscreenBitmap;
	wxMemoryDC      m_offscreenDc;

	void            CalcPositions       ();

	void            DrawBg              (wxDC&, long what, const wxRect& drawRect, bool full);

	long            FindMonitorByPoint  (int x, int y) const;

	long            m_affectedIndex;

	void            DoPaint             (wxDC&);
	void            OnSize              (wxSizeEvent&);
	void            OnEraseBackground   (wxEraseEvent&) {}
	void            OnPaint             (wxPaintEvent& event) {wxPaintDC dc(this); DoPaint(dc);}

	DECLARE_EVENT_TABLE ()
};

#endif // __SJ_MONITOR_OVERVIEW__
