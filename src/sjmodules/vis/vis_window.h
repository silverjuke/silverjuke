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
 * File:    vis_window.h
 * Authors: Björn Petersen
 * Purpose: Parent classes for the visualizations
 *
 ******************************************************************************/


#ifndef __SJ_VIS_WINDOW_H__
#define __SJ_VIS_WINDOW_H__


class SjVisWindow : public wxWindow
{
public:
	                SjVisWindow           (wxWindow* parent);
	                ~SjVisWindow          ();

	void            SetRenderer           (SjVisRendererModule*);
	SjVisRendererModule* GetRenderer      () const { return m_renderer; }

	// The following functions are meant to be used for the vis. implementations (SjVisRendererModule)
	wxRect          GetRendererClientRect () const;
	wxRect          GetRendererScreenRect () const;
	void            CalcPositions         ();
	void            OnKeyDown             (wxKeyEvent&) { m_keyDown = TRUE; }
	void            OnKeyUp               (wxKeyEvent&);
	void            OnMouseLeftDown       (wxMouseEvent& e) { m_mouseDown = true; }
	void            OnMouseLeftUp         (wxMouseEvent& e);
	void            OnMouseRightUp        (wxContextMenuEvent& e) { } // when using this again, take care of popup-entries that destroy windows!
	void            OnMouseLeftDClick     (wxMouseEvent& e) { }
	void            OnMouseEnter          (wxMouseEvent& e);

private:
	void            RemoveRenderer        ();

	bool            m_keyDown;
	bool            m_mouseDown;

	wxRect          m_rendererRect;
	SjVisRendererModule* m_renderer;

	void            OnSize                (wxSizeEvent& e) { CalcPositions(); }
	void            OnEraseBackground     (wxEraseEvent& e) { }
	void            OnPaint               (wxPaintEvent& e);

	friend class    SjVisModule;

	DECLARE_EVENT_TABLE ();
};


#endif // __SJ_VIS_WINDOW_H__
