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
 * File:    tooltipssj.h
 * Authors: Björn Petersen
 * Purpose: Tool tip handling
 *
 ******************************************************************************/



#ifndef __SJ_TOOLTIPS_H__
#define __SJ_TOOLTIPS_H__

#if SJ_USE_TOOLTIPS



class SjToolTipProvider
{
public:
	// these function should be overwritten in derived classes
	virtual wxString    GetText         (long& flags/*width in the lower bits, rest reserved*/) = 0;
	virtual wxWindow*   GetWindow       () = 0;

	// get the rectangle relative to the client area of the window
	virtual wxRect      GetLocalRect    () = 0;
};



class SjToolTipTimer;
class SjTipWindow;



class SjToolTipManager
{
public:
	// constructor/destructor
	SjToolTipManager    ();
	~SjToolTipManager   () { ExitForever(); }

	// SetToolTipProvider() should be called by an object that wants a
	// tooltip to be displayed.
	//
	// If there is currently no open tooltip, the function openes the
	// tooltip after a little time (say 2 seconds); if there is currently
	// an open tooltip, the tooltip is opened immediately.
	//
	// The tooltip is opened at the current mouse position. The text
	// is requested by GetText() at the moment of opening -- so there is
	// no performance lack for tooltips that are never opened.
	//
	// The tooltip is closed if the mouse leaves the given rectangle or
	// if SetToolTipProvider(NULL) is called.
	void            SetToolTipProvider  (SjToolTipProvider*);

	// a synonym for SetProvider(NULL)
	void            ClearToolTipProvider() { SetToolTipProvider(NULL); }

	// closes the tooltip window if it is opened
	void            CloseToolTipWindow  ();

	// exit - should be called before the program terminates and
	// any stored SjToolTipProvider objects may get invalid
	void            ExitForever         ();

	// set the colours
	void            SetColours          (const wxColour& fg, const wxColour& bg, const wxColour& border) { m_fgColour=fg; m_bgColour=bg; m_borderColour=border; }
	void            SetDefColours       () { m_fgColour=wxSystemSettings::GetColour(wxSYS_COLOUR_INFOTEXT); m_bgColour=wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK); m_borderColour=m_fgColour; };

private:
	SjToolTipProvider*
	m_toolTipProvider;
	SjToolTipTimer* m_toolTipTimer;
	SjTipWindow*
	m_toolTipWindow;
	void            Notify              ();
	void            OpenToolTipWindow   ();
	wxColour        m_fgColour,
	                m_bgColour,
	                m_borderColour;
	wxPoint         m_lastMousePosition;

	unsigned long   m_fastReopenMsTicks;

	friend class    SjToolTipTimer;
	friend class    SjTipWindow;
};



#endif // SJ_USE_TOOLTIPS

#endif // __SJ_TOOLTIPS_H__
