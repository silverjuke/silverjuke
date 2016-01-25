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
 * Purpose: This file provides two classes, SjVisEmbed and
 *          SjVisFrame that are used as parents for the visualizations
 *          (either embedded or floating)
 *
 ******************************************************************************/


#ifndef __SJ_VIS_WINDOW_H__
#define __SJ_VIS_WINDOW_H__


class SjVisImpl
{
public:
	                SjVisImpl           ();
	void            Init                (wxWindow* thisWindow, bool thisWindowIsFrame);
	void            Exit                ();

	void            OnSize              (wxSizeEvent& e) { CalcPositions(); }
	void            OnEraseBackground   (wxEraseEvent&);
	void            OnPaint             (wxPaintEvent&);
	void            OnKeyDown           (wxKeyEvent&) { m_keyDown = TRUE; }
	void            OnKeyUp             (wxKeyEvent&);
	void            OnMouseLeftDown     (wxWindow* from, wxMouseEvent&) { m_mouseDown = true; }
	void            OnMouseLeftUp       (wxWindow* from, wxMouseEvent&);
	void            OnMouseRightUp      (wxWindow* from, wxContextMenuEvent&);
	void            OnMouseLeftDClick   (wxWindow* from, wxMouseEvent&);
	void            OnMouseEnter        (wxWindow* from, wxMouseEvent&);

	void            SetRenderer         (SjVisRendererModule*, bool justContinue);
	SjVisRendererModule* GetRenderer    () const { return m_renderer; }

	// The following functions are meant to be used for the vis. implementations (SjVisRendererModule)
	wxWindow*       GetWindow               () const { return m_thisWindow; }
	wxRect          GetRendererClientRect   () const;
	wxRect          GetRendererScreenRect   () const;

private:
	bool            Ok                  () const { return m_thisWindow!=NULL; }
	void            CalcPositions       ();
	void            RemoveRenderer      ();

	wxPoint         GetEventMousePosition(wxWindow* from, wxMouseEvent&) const;

	bool            m_keyDown,
	                m_mouseDown;

	wxWindow*       m_thisWindow;
	bool            m_thisWindowIsFrame;

	wxRect          m_rendererRect;
	SjVisRendererModule* m_renderer;

	friend class    SjVisModule;
};



class SjVisFrame : public wxFrame
{
public:
					SjVisFrame          (wxWindow* parent, wxWindowID id, const wxString& title,
	                                     const wxPoint& pos, const wxSize& size,
	                                     long style);
	                ~SjVisFrame         () { m_impl.Exit(); }
	void            InitImpl            () { m_impl.Init(this, true); }

private:
	SjVisImpl       m_impl;

	void            OnSize              (wxSizeEvent& e)    { m_impl.OnSize(e); }
	void            OnEraseBackground   (wxEraseEvent& e)   { m_impl.OnEraseBackground(e); }
	void            OnPaint             (wxPaintEvent& e)   { m_impl.OnPaint(e); }
	void            OnKeyDown           (wxKeyEvent& e)     { m_impl.OnKeyDown(e); }
	void            OnKeyUp             (wxKeyEvent& e)     { m_impl.OnKeyUp(e); }
	void            OnMouseLeftDown     (wxMouseEvent& e)   { m_impl.OnMouseLeftDown(this, e); }
	void            OnMouseLeftUp       (wxMouseEvent& e)   { m_impl.OnMouseLeftUp(this, e); }
	void            OnMouseRightUp      (wxContextMenuEvent& e)   { m_impl.OnMouseRightUp(this, e); }
	void            OnMouseLeftDClick   (wxMouseEvent& e)   { m_impl.OnMouseLeftDClick(this, e); }
	void            OnMouseEnter        (wxMouseEvent& e)   { m_impl.OnMouseEnter(this, e); }
	void            OnFwdToMainFrame    (wxCommandEvent& e);
	void            OnCloseWindow       (wxCloseEvent&);

	friend class    SjVisModule;

	DECLARE_EVENT_TABLE ();
};



class SjVisEmbed : public wxWindow
{
public:
	                SjVisEmbed          (wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
	                                      : wxWindow(parent, id, pos, size, style) { }
	                ~SjVisEmbed         () { m_impl.Exit(); }
	void            InitImpl            () { m_impl.Init(this, false); }

private:
	SjVisImpl       m_impl;
	void            OnSize              (wxSizeEvent& e)    { m_impl.OnSize(e); }
	void            OnEraseBackground   (wxEraseEvent& e)   { m_impl.OnEraseBackground(e); }
	void            OnPaint             (wxPaintEvent& e)   { m_impl.OnPaint(e); }
	void            OnKeyDown           (wxKeyEvent& e)     { m_impl.OnKeyDown(e); }
	void            OnKeyUp             (wxKeyEvent& e)     { m_impl.OnKeyUp(e); }
	void            OnMouseLeftDown     (wxMouseEvent& e)   { m_impl.OnMouseLeftDown(this, e); }
	void            OnMouseLeftUp       (wxMouseEvent& e)   { m_impl.OnMouseLeftUp(this, e); }
	void            OnMouseRightUp      (wxContextMenuEvent& e)   { m_impl.OnMouseRightUp(this, e); }
	void            OnMouseLeftDClick   (wxMouseEvent& e)   { m_impl.OnMouseLeftDClick(this, e); }
	void            OnMouseEnter        (wxMouseEvent& e)   { m_impl.OnMouseEnter(this, e); }

	friend class    SjVisModule;

	DECLARE_EVENT_TABLE ();
};


#endif // __SJ_VIS_WINDOW_H__
