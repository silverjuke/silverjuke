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
 * File:    oscilloscope.h
 * Authors: Björn Petersen
 * Purpose: Simple "Oscilloscope visualization"
 *
 ******************************************************************************/



#ifndef __SJ_OSCILLOSCOPE_H__
#define __SJ_OSCILLOSCOPE_H__



class SjOscModule;
class SjOscWindow;
class SjOscFrame;



#define SJ_USE_FIG 0        // not really cool, I think if we do sth. into this direction we should do it REALLY



class SjOscThread : public wxThread
{
public:
	SjOscThread         (SjOscModule*);
	void*           Entry               ();

	wxBitmap        m_offscreenBitmap;
	wxMemoryDC      m_offscreenDc;

private:
	SjOscModule*    m_oscModule;
};



class SjOscData
{
public:
	// the following data are shared between threads using the critical section object
	wxCriticalSection
	m_critical;
	bool            m_forceSpectrAnim;
	bool            m_forceOscAnim;
	bool            m_titleChanged;
	wxString        m_trackName,
	                m_leadArtistName;
};



class SjVisImpl;



class SjOscModule : public SjVisRendererModule
{
public:
	SjOscModule         (SjInterfaceBase* interf);

	bool            Start               (SjVisImpl*, bool justContinue);
	void            Stop                ();

	void            ReceiveMsg          (int);
	void            AddMenuOptions      (SjMenu&);
	void            OnMenuOption        (int);
	void            PleaseUpdateSize    (SjVisImpl*);
private:
	SjOscThread*    m_oscThread;
	SjOscWindow*    m_oscWindow;

	SjVisImpl*      m_impl;
	SjOscData       m_data;
#define         SJ_OSC_SHOW_SPECTRUM    0x00000001L
#define         SJ_OSC_SHOW_OSC         0x00000002L
#define         SJ_OSC_SHOW_STARFIELD   0x00010000L
#define         SJ_OSC_SHOW_DEFAULT     0x0000FFFFL
	long            m_showFlags;
#if SJ_USE_FIG
	bool            m_showFigures;
#endif

	bool            m_doBlit;
	wxCriticalSection m_blitCritical;

	friend class    SjOscThread;
	friend class    SjOscWindow;
	friend class    SjOscFrame;
};



#endif // __SJ_OSCILLOSCOPE_H__
