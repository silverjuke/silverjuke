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
 * File:    karaoke_module.h
 * Authors: Björn Petersen
 * Purpose: The Karaoke Module
 *
 ******************************************************************************/


#ifndef __SJ_KARAOKE_MODULE_H__
#define __SJ_KARAOKE_MODULE_H__


#include "vis_bg.h"

class SjKaraokeModule;
class SjKaraokeWindow;
class SjKaraokeFrame;

class SjVisImpl;


enum SjKaraokeBgType
{
    SJ_KARAOKE_BG_BLACK = 0,
    SJ_KARAOKE_BG_DEFAULT_IMAGES = 1,
    SJ_KARAOKE_BG_USERDEF_DIR = 2,

    SJ_KARAOKE_BG_COUNT
};


class SjKaraokeReader
{
public:
	                SjKaraokeReader     () {}
	virtual         ~SjKaraokeReader    () {}

	// return false if at end; however, the reader must allow seeking backward
	virtual bool    SetPosition         (long ms) = 0;

	#define         KARAOKE_RENDER_SHRINK 0.95F // max. amount of the screen to take
	virtual void    Render              (wxDC&, SjVisBg&, bool pleaseUpdateAll) = 0;

protected:
	wxString        m_artist;
	wxString        m_title;

};


class SjKaraokeMaster
{
public:
	                    SjKaraokeMaster     ();
	                    ~SjKaraokeMaster    () { Exit(); }
	bool                Init                (const wxString& musicFile, const wxString& artist, const wxString& title);

	bool                HasKaraoke          () { return (m_reader!=NULL); }

	bool                SetPosition         (long ms) { if(m_reader) return m_reader->SetPosition(ms); else return false; }
	void                Render              (wxDC& dc, SjVisBg& bg, bool pleaseUpdateAll) { if(m_reader) m_reader->Render(dc, bg, pleaseUpdateAll); }

private:
	wxFSFile*           CheckFile           (const wxString& musicFile, const wxString& karaokeExt);
	void                Exit                ();
	SjKaraokeReader*    m_reader;
};


class SjKaraokeModule : public SjVisRendererModule
{
public:
						SjKaraokeModule     (SjInterfaceBase* interf);

	bool                Start               (SjVisImpl*, bool justContinue);
	void                Stop                ();

	void                ReceiveMsg          (int);
	void                PleaseUpdateSize    (SjVisImpl*);

	void                AddMenuOptions      (SjMenu& m);
	void                OnMenuOption        (int i);

protected:
	bool                FirstLoad           ();
	void                LastUnload          ();

private:
	SjKaraokeWindow*    m_karaokeWindow;
	void                ReceiveMsgTrackOnAirChanged(bool justContinue);

	SjVisImpl*          m_impl;

	wxString            m_currUrl;

	// background stuff
	SjKaraokeBgType     m_bgType;
	wxString            m_bgUserDefDir;
	wxArrayString       m_bgFiles;
	wxString            m_bgCurrFile;
	SjVisBg*            m_bg;
	void                InitBgFiles         ();
	void                UpdateBg            (bool keepImage=false);

	// more configurations
	#define             SJ_KAR_SMOOTH       0x00000100L
	#define             SJ_KAR_USEBGJPG     0x00000200L
	#define             SJ_KAR_BG_MASK      0x000000FFL
	#define             SJ_KAR_KEEPASPECT   0x00010000L
	#define             SJ_KAR_GRAYSCALE    0x00020000L
	#define             SJ_KAR_DEFAULT      0x0000FF01L // 01 = SJ_KARAOKE_BG_DEFAULT_IMAGES
	long                m_karFlags;
	void                WriteKarConfig      ();

	friend class        SjKaraokeWindow;
};


// you should not change SLEEP_MS without reasons.
// IF you change it, also check if really all time-depending calculations are still correct.
#define SJ_KARAOKE_SLEEP_MS 100


#endif // __SJ_KARAOKE_MODULE_H__
