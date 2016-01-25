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
 * File:    karaoke_module.cpp
 * Authors: Björn Petersen
 * Purpose: The Karaoke Module
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/imgthread.h>
#include <sjmodules/vis/vis_module.h>
#include <sjmodules/vis/vis_karaoke_module.h>
#include <sjmodules/vis/vis_window.h>
#include <sjmodules/vis/vis_cdg_reader.h>
#include <sjmodules/vis/vis_synctxt_reader.h>

#define IDC_BG_SUBMENU                  (IDO_VIS_OPTIONFIRST+1)
#define IDC_BG_DEFAULT_IMAGES           (IDO_VIS_OPTIONFIRST+2)
#define IDC_BG_BLACK                    (IDO_VIS_OPTIONFIRST+3)
#define IDC_BG_USERDEF_DIR_USE          (IDO_VIS_OPTIONFIRST+4)
#define IDC_BG_USERDEF_DIR_CHANGE       (IDO_VIS_OPTIONFIRST+5)
#define IDC_BG_SMOOTH                   (IDO_VIS_OPTIONFIRST+6)
#define IDC_BG_KEEPASPECT               (IDO_VIS_OPTIONFIRST+7)
#define IDC_BG_GRAYSCALE                (IDO_VIS_OPTIONFIRST+8)
#define IDC_BG_USEBGJPG                 (IDO_VIS_OPTIONFIRST+9)


/*******************************************************************************
 * SjKaraokeMaster
 ******************************************************************************/


SjKaraokeMaster::SjKaraokeMaster()
{
	m_reader = NULL;
}


wxFSFile* SjKaraokeMaster::CheckFile(const wxString& musicFile, const wxString& karaokeExt)
{
	wxFileSystem fs;
	wxFSFile* fsFile;

	wxString testFile = musicFile.BeforeLast('.') + wxT(".") + karaokeExt;
	if( (fsFile=fs.OpenFile(testFile, wxFS_READ|wxFS_SEEKABLE)) != NULL ) {
		return fsFile;
	}

	testFile = musicFile + wxT(".") + karaokeExt;
	if( (fsFile=fs.OpenFile(testFile, wxFS_READ|wxFS_SEEKABLE)) != NULL ) {
		return fsFile;
	}

	testFile = musicFile.BeforeLast('.') + wxT(".") + karaokeExt.Upper();
	if( (fsFile=fs.OpenFile(testFile, wxFS_READ|wxFS_SEEKABLE)) != NULL ) {
		return fsFile;
	}

	testFile = musicFile + wxT(".") + karaokeExt.Upper();
	if( (fsFile=fs.OpenFile(testFile, wxFS_READ|wxFS_SEEKABLE)) != NULL ) {
		return fsFile;
	}

	return NULL;
}


bool SjKaraokeMaster::Init(const wxString& musicFile, const wxString& artist, const wxString& title)
{
	wxFSFile* fsFile;

	// exit old stuff
	Exit();

	if( musicFile.StartsWith(wxT("http:")) // this may be a steam - in this case (or in others) we get into an endless loop
	 || musicFile.StartsWith(wxT("https:"))
	 || musicFile.StartsWith(wxT("ftp:")) )
		return false;

	// try to create CDG (try musicFile.cdg and musicFile.mp3.cdg)
	if( (fsFile=CheckFile(musicFile, wxT("cdg"))) )
	{
		m_reader = new SjCdgReader(fsFile); // SjCdgReader takes ownership of fsFile!
		return true; // success
	}

	// try to create LRC (Simple Lyrics)
	if( (fsFile=CheckFile(musicFile, wxT("lrc"))) )
	{
		m_reader = new SjSyncTxtReader(fsFile, SJ_SYNCTXT_LRC, artist, title); // SjSyncTxtReader takes ownership of fsFile!
		return true; // success
	}

	// no karaoke file available
	return false;
}


void SjKaraokeMaster::Exit()
{
	if( m_reader )
	{
		SjKaraokeReader* toDel = m_reader;
		m_reader = NULL;
		delete toDel;
	}
}


/*******************************************************************************
 * SjScreen Class
 ******************************************************************************/


class SjSjScreen
{
public:
	SjSjScreen(const wxString& line1, const wxString& line2, unsigned long stayMs);
	bool Render(wxDC&, SjVisBg&, bool pleaseUpdateAll);

private:
	wxString    m_line1, m_line2;
	wxFont      m_font;
	wxCoord     m_fontPixelH;
	wxColour    m_colour;
	unsigned long m_startMs, m_stayMs;
	bool        m_done;
	void        DrawCentered(wxDC& dc, long x, long y, const wxString&);

};


SjSjScreen::SjSjScreen(const wxString& line1, const wxString& line2, unsigned long stayMs)
	: m_font(10, wxSWISS, wxITALIC, wxNORMAL, FALSE)
{
	m_line1 = line1;
	m_line2 = line2;
	m_stayMs = stayMs;
	m_fontPixelH = 0;
	m_startMs = SjTools::GetMsTicks();
	m_done = false;
}


bool SjSjScreen::Render(wxDC& dc, SjVisBg& bg, bool pleaseUpdateAll)
{
	// basic screen calculations
	wxSize      dcSize = dc.GetSize(); if( dcSize.y <= 0 ) return false;
	float       dcAspect = (float)dcSize.x / (float)dcSize.y;

	float       areaAspect = 300.0F / 216.0F; // for a good compatibility to CDG, use the same area and aspect

	long areaX, areaY, areaW, areaH;
	if( dcAspect > areaAspect )
	{
		// scale by height
		areaH = long( (float)dcSize.y * KARAOKE_RENDER_SHRINK );
		areaW = long( (float)areaH * areaAspect );
	}
	else
	{
		// scale by width
		areaW = long( (float)dcSize.x * KARAOKE_RENDER_SHRINK );
		areaH = long( (float)areaW / areaAspect );
	}

	// find center
	areaX = (dcSize.x - areaW) / 2;
	areaY = (dcSize.y - areaH) / 2;

	// done?
	unsigned long stayMs = SjTools::GetMsTicks()-m_startMs;
	if( m_done )
	{
		return false;
	}
	else if( m_stayMs!=0 && stayMs > m_stayMs)
	{
		dc.SetClippingRegion(areaX, areaY, areaW, areaH);
		bg.DrawBackground(dc);
		dc.DestroyClippingRegion();
		m_done = true;
		return false;
	}

	// update font?
	{
		wxCoord fontPixelH = areaH/9;
		if( fontPixelH != m_fontPixelH )
		{
			SjVisBg::SetFontPixelH(dc, m_font, fontPixelH);
			m_fontPixelH = fontPixelH;
			pleaseUpdateAll = true;
		}

		dc.SetFont(m_font);
	}

	// draw!
	dc.SetBackgroundMode(wxTRANSPARENT);

	for( int i = 0; i <= 1; i++ )
	{
		int offset = 0;
		if( i == 0 )
		{
			// draw black shadow
			if( !pleaseUpdateAll )
				continue;

			dc.SetTextForeground(*wxBLACK);
			offset = m_fontPixelH / 16;
			if( m_fontPixelH < 2 ) m_fontPixelH = 2;
		}
		else
		{
			// draw white fade
			long g = stayMs/4;
			if( g > 245 ) g = 245;
			m_colour.Set(g, g, g);
			dc.SetTextForeground(m_colour);
		}

		long x = areaX+areaW/2;
		long y = areaY+areaH/2-m_fontPixelH; if( m_line2.IsEmpty() ) y += m_fontPixelH/2;
		DrawCentered(dc, x+offset, y+offset, m_line1);
		y += m_fontPixelH;
		DrawCentered(dc, x+offset, y+offset, m_line2);
	}

	return true;
}


void SjSjScreen::DrawCentered(wxDC& dc, long x, long y, const wxString& text)
{
	wxCoord w, h;
	dc.GetTextExtent(text, &w, &h);
	dc.DrawText(text, x-w/2, y);
}


/*******************************************************************************
 * SjKaraokeWindow Class
 ******************************************************************************/


class SjKaraokeWindow : public wxWindow
{
public:
	SjKaraokeWindow     (SjKaraokeModule*, wxWindow* parent);
	~SjKaraokeWindow        ();

	// read / write from the module
	SjKaraokeMaster m_karaokeMaster;

	wxTimer         m_timer;
	SjKaraokeModule* m_karaokeModule;
	bool            m_bgChanged;

private:
	// offscreen bitmap
	wxBitmap        m_offscreenBitmap;
	wxMemoryDC      m_offscreenDc;

	// states
	wxSize          m_clientSize;
	bool            m_pleaseUpdateAll;
	int             m_karaokeEnded;
	SjSjScreen*     m_sjScreen;

	// events
	void            OnEraseBackground   (wxEraseEvent&) {} // we won't erease the background explcitly, this is done in the thread
	void            OnPaint             (wxPaintEvent&);

	bool            ImplOk              () const { return (m_karaokeModule&&m_karaokeModule->m_impl); }
	void            OnKeyDown           (wxKeyEvent& e)     { if(ImplOk()) m_karaokeModule->m_impl->OnKeyDown(e); }
	void            OnKeyUp             (wxKeyEvent& e)     { if(ImplOk()) m_karaokeModule->m_impl->OnKeyUp(e); }
	/* -- for consistency with external plugins, do not use context menus etc. for the karaoke window!
	   -- people shall use the menu button atop of the window
	   Edit 3.04: well, but we'll need this for the kiosk mode */
	// /*
	void            OnMouseLeftDown     (wxMouseEvent& e)   { if(ImplOk()) m_karaokeModule->m_impl->OnMouseLeftDown(this, e); }
	void            OnMouseLeftUp       (wxMouseEvent& e)   { if(ImplOk()) m_karaokeModule->m_impl->OnMouseLeftUp(this, e); }
	void            OnMouseRightUp      (wxContextMenuEvent& e)   { if(ImplOk()) m_karaokeModule->m_impl->OnMouseRightUp(this, e); }
	void            OnMouseLeftDClick   (wxMouseEvent& e)   { if(ImplOk()) m_karaokeModule->m_impl->OnMouseLeftDClick(this, e); }
	// */
	void            OnMouseEnter        (wxMouseEvent& e)   { if(ImplOk()) m_karaokeModule->m_impl->OnMouseEnter(this, e); }

	bool            m_inPaint;

	void            OnTimer             (wxTimerEvent&);
#if SJ_USE_IMG_KARAOKE_BG
	void            OnImageThere        (SjImageThereEvent& e) { OnImageThere_(e.GetObj()); }
	void            OnImageThere_       (SjImgThreadObj*);
#endif
	DECLARE_EVENT_TABLE ();
};


#define IDC_SETTINGS            (IDM_FIRSTPRIVATE+3)
#define IDC_CLOSE               (IDM_FIRSTPRIVATE+4)
#define IDC_TIMER               (IDM_FIRSTPRIVATE+7)


BEGIN_EVENT_TABLE(SjKaraokeWindow, wxWindow)
	EVT_ERASE_BACKGROUND(                       SjKaraokeWindow::OnEraseBackground  )
	EVT_PAINT           (                       SjKaraokeWindow::OnPaint            )
	EVT_KEY_DOWN        (                       SjKaraokeWindow::OnKeyDown          )
	EVT_KEY_UP          (                       SjKaraokeWindow::OnKeyUp            )
	EVT_LEFT_DOWN       (                       SjKaraokeWindow::OnMouseLeftDown    )
	EVT_LEFT_UP         (                       SjKaraokeWindow::OnMouseLeftUp      )
	EVT_LEFT_DCLICK     (                       SjKaraokeWindow::OnMouseLeftDClick  )
	EVT_CONTEXT_MENU    (                       SjKaraokeWindow::OnMouseRightUp     )
	EVT_ENTER_WINDOW    (                       SjKaraokeWindow::OnMouseEnter       )
	EVT_TIMER           (IDC_TIMER,             SjKaraokeWindow::OnTimer            )
END_EVENT_TABLE()


SjKaraokeWindow::SjKaraokeWindow(SjKaraokeModule* karaokeModule, wxWindow* parent)
	: wxWindow( parent, -1,
	            wxPoint(-1000,-1000), wxSize(100,100),
	            (wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE ))
{
	m_karaokeModule     = karaokeModule;

	m_pleaseUpdateAll   = true;
	m_inPaint           = false;

	m_karaokeEnded      = 2;
	m_sjScreen          = NULL;
	m_bgChanged         = false;

	m_timer.SetOwner(this, IDC_TIMER);
	m_timer.Start(SJ_KARAOKE_SLEEP_MS);
}


SjKaraokeWindow::~SjKaraokeWindow()
{
	if( m_sjScreen )
		delete m_sjScreen;
}


void SjKaraokeWindow::OnPaint(wxPaintEvent&)
{
	wxPaintDC dc(this);
	m_pleaseUpdateAll = true;

	if( m_inPaint ) return;
	m_inPaint = true;

	// only draw the stuff right and below the old client size black
	// (everything else is updated in the timer)

	wxSize newClientSize = GetClientSize();
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(*wxBLACK_BRUSH);
	if( newClientSize.x > m_clientSize.x )
	{
		dc.DrawRectangle(m_clientSize.x, 0, newClientSize.x, newClientSize.y);
	}

	if( newClientSize.y > m_clientSize.y )
	{
		dc.DrawRectangle(0, m_clientSize.y, newClientSize.x, newClientSize.y);
	}

	m_inPaint = false;
}


void SjKaraokeWindow::OnTimer(wxTimerEvent&)
{
	wxASSERT( wxThread::IsMain() );

	/**********************************************************************
	 * PREPARE DRAWING
	 **********************************************************************/

	if( m_karaokeModule == NULL || m_karaokeModule->m_bg == NULL ) return;
	if( m_inPaint ) return;
	m_inPaint = true;

	// does the size of the window have changed?
	if( m_clientSize != GetClientSize() || m_bgChanged )
	{
		// yes: save the new size
		m_clientSize = GetClientSize();
		m_pleaseUpdateAll = true;

		// (re-)create the offscreen DC
		m_offscreenBitmap.Create(m_clientSize.x, m_clientSize.y);
		m_offscreenDc.SelectObject(m_offscreenBitmap);

		// force reloading of the background image
		m_karaokeModule->m_bg->SetSize(m_clientSize);
		m_bgChanged = false;
	}

	// check what to show
	int karaokeEnded = 1;
	if( m_karaokeMaster.HasKaraoke() )
	{
		long totalMs, elapsedMs, remainingMs;
		g_mainFrame->m_player.GetTime(totalMs, elapsedMs, remainingMs);
		if( elapsedMs >= 0 ) // -1=error, 0=normal position 0:00
		{
			karaokeEnded = m_karaokeMaster.SetPosition(elapsedMs)? 0 : 1;
		}
	}

	if( karaokeEnded != m_karaokeEnded )
	{
		if( karaokeEnded )
		{
			if( m_sjScreen == NULL )
				m_sjScreen = new SjSjScreen(wxT("SILVERJUKE KARAOKE"),
				                            g_mainFrame->IsKioskStarted()? wxT("www.silverjuke.net") : wxT(""), 8000);
		}
		else
		{
			if( m_sjScreen )
			{
				delete m_sjScreen;
				m_sjScreen = NULL;
			}
		}

		m_pleaseUpdateAll = true;
		m_karaokeEnded = karaokeEnded;
	}

	// set the DC to use
	wxClientDC clientDcDontUse(this);
	wxDC* dc;
	if( m_pleaseUpdateAll )
	{
		dc = &m_offscreenDc;
	}
	else
	{
		dc = &clientDcDontUse;
	}

	/**********************************************************************
	 * DRAW!
	 **********************************************************************/

	// redraw background?
	if( m_pleaseUpdateAll )
	{
		m_karaokeModule->m_bg->DrawBackground(*dc);
	}

	// redraw karaoke?
	if( karaokeEnded )
	{
		if(  m_sjScreen
		        && !m_sjScreen->Render(*dc, *(m_karaokeModule->m_bg), m_pleaseUpdateAll)
		        && !m_karaokeMaster.HasKaraoke() )
		{
			delete m_sjScreen;
			m_sjScreen = NULL;
			if( !g_mainFrame->IsKioskStarted() /*no error messages in kiosk mode!*/
			        && !g_mainFrame->IsStopped() )
			{
				m_sjScreen = new SjSjScreen(_("No lyrics found."), wxT(""), 0);
			}
		}
	}
	else
	{
		m_karaokeMaster.Render(*dc, *(m_karaokeModule->m_bg), m_pleaseUpdateAll);
	}

	/**********************************************************************
	 * DRAWING DONE
	 **********************************************************************/

	// BLIT if we are drawing offscreen
	if( m_pleaseUpdateAll )
	{
		clientDcDontUse.Blit(0, 0, m_clientSize.x, m_clientSize.y, &m_offscreenDc, 0, 0);
	}

	// really done!
	m_pleaseUpdateAll = false;
	m_inPaint = false;
}


/*******************************************************************************
 * SjKaraokeModule
 ******************************************************************************/


SjKaraokeModule::SjKaraokeModule(SjInterfaceBase* interf)
	: SjVisRendererModule(interf)
{
	m_file          = wxT("memory:karaoke.lib");
	m_name          = _("Karaoke Prompt");
	m_karaokeWindow = NULL;
	m_bg            = NULL;
	m_sort          = 3000; // end of list, default is 1000
}


bool SjKaraokeModule::FirstLoad()
{
	m_karFlags = g_tools->m_config->Read(wxT("player/karflags"), SJ_KAR_DEFAULT);
	m_bgType = (SjKaraokeBgType)(m_karFlags & SJ_KAR_BG_MASK);
	if( m_bgType < 0 || m_bgType >= SJ_KARAOKE_BG_COUNT )
		m_bgType = SJ_KARAOKE_BG_DEFAULT_IMAGES;

	m_bgUserDefDir  = g_tools->m_config->Read(wxT("player/karbgdir"), wxT(""));

	InitBgFiles();

	return true;
}


void SjKaraokeModule::WriteKarConfig()
{
	g_tools->m_config->Write(wxT("player/karflags"), long( (m_karFlags&~SJ_KAR_BG_MASK)|long(m_bgType) ) );
	g_tools->m_config->Write(wxT("player/karbgdir"), m_bgUserDefDir);
}


void SjKaraokeModule::LastUnload()
{
	if( m_bg )
	{
		delete m_bg;
		m_bg = NULL;
	}
}


void SjKaraokeModule::InitBgFiles()
{
	wxASSERT( wxThread::IsMain() );

	m_bgFiles.Clear();
	bool resetToDefault = false;

	if( m_bgType == SJ_KARAOKE_BG_DEFAULT_IMAGES )
	{
		for( int i = 1; i <= 3; i++ )
			m_bgFiles.Add(wxString::Format(wxT("memory:karaoke%i.jpg"), i));
	}
	else if( m_bgType == SJ_KARAOKE_BG_USERDEF_DIR )
	{
		if( ::wxDirExists(m_bgUserDefDir) )
		{
			SjDirIterator di(m_bgUserDefDir);
			wxString ext;
			while( di.Iterate() )
			{
				if( g_mainFrame->m_moduleSystem.FindImageHandlerByExt(SjTools::GetExt(di.m_fullPath)) )
				{
					m_bgFiles.Add(di.m_fullPath);
				}
			}

			if( m_bgUserDefDir.IsEmpty() )
			{
				wxLogError(_("No images found in \"%s\"."), m_bgUserDefDir.c_str());
				resetToDefault = true;
			}
		}
		else
		{
			wxLogError(_("Cannot open \"%s\"."), m_bgUserDefDir.c_str());
			resetToDefault = true;
		}
	}

	if( resetToDefault )
	{
		m_bgType = SJ_KARAOKE_BG_DEFAULT_IMAGES;
		m_bgUserDefDir.Clear();
		WriteKarConfig();
		InitBgFiles(); // recursive call!
		return;
	}
}


void SjKaraokeModule::UpdateBg(bool keepImg)
{
	wxASSERT( wxThread::IsMain() );
	SjVisBg* newBg = NULL;

	long imgOpFlags =       ((m_karFlags&SJ_KAR_SMOOTH)? SJ_IMGOP_SMOOTH : 0)
	                        |   ((m_karFlags&SJ_KAR_KEEPASPECT)? SJ_IMGOP_KEEPASPECT : 0)
	                        |   ((m_karFlags&SJ_KAR_GRAYSCALE)? SJ_IMGOP_GRAYSCALE : 0);

	// try to read a track-specific background
	if( m_karFlags&SJ_KAR_USEBGJPG )
	{
		static const wxChar *tryExts[] = {
			wxT("/<name>.bg.jpg"),
			wxT("/<name>.bg.png"),
			wxT("/<name>.bg.gif"),
			wxT("/album.bg.jpg"),
			wxT("/album.bg.png"),
			wxT("/album.bg.gif"),
			NULL
		}, **tryPtr;

		wxString tryBase = m_currUrl.BeforeLast(wxT('.')); tryBase.Replace(wxT("\\"), wxT("/"));
		wxString tryName = tryBase.AfterLast(wxT('/'));    tryBase = tryBase.BeforeLast(wxT('/'));
		wxString currTry;

		tryPtr = &tryExts[0];
		while( *tryPtr )
		{
			currTry = tryBase + *tryPtr;
			currTry.Replace(wxT("<name>"), tryName);
			wxLogDebug(wxT("%s"), currTry.c_str());
			if( wxFileExists(currTry) )
			{
				newBg = new SjVisImgBg(currTry, imgOpFlags);
				break;
			}
			tryPtr++;
		}
	}

	// fall back to a random default background
	if( newBg == NULL )
	{
		if( m_bgType == SJ_KARAOKE_BG_DEFAULT_IMAGES || m_bgType == SJ_KARAOKE_BG_USERDEF_DIR )
		{
			if( !keepImg )
			{
				wxArrayString possibleFiles = m_bgFiles;
				int bgCurrFileIndex = possibleFiles.Index(m_bgCurrFile, false);
				if( bgCurrFileIndex != wxNOT_FOUND && possibleFiles.GetCount() > 1 )
				{
					possibleFiles.RemoveAt(bgCurrFileIndex); // avoid using the same background image now
				}

				if( possibleFiles.GetCount() > 0 ) // "possibleFiles" may be empty, check this to avoid crashes, see http://www.silverjuke.net/forum/topic-1954.html
				{
					m_bgCurrFile = possibleFiles.Item( SjTools::Rand(possibleFiles.GetCount()) );
				}
			}

			newBg = new SjVisImgBg(m_bgCurrFile, imgOpFlags);
		}
		else
		{
			newBg = new SjVisGrBg(imgOpFlags);
		}
	}

	SjVisBg* toDelBg = m_bg;
	m_bg = newBg;
	delete toDelBg;

	if( m_karaokeWindow )
	{
		m_karaokeWindow->m_bgChanged = true;
	}
}


bool SjKaraokeModule::Start(SjVisImpl* impl, bool justContinue)
{
	m_impl = impl;
	m_currUrl = wxT(":*");  // something invalid, but not empty - otherwise the comparison for new tracks
	// will fail if the url is empty!
	if( m_karaokeWindow == NULL )
	{
		m_karaokeWindow = new SjKaraokeWindow(this, m_impl->GetWindow());

		if( m_karaokeWindow  )
		{
			ReceiveMsgTrackOnAirChanged(justContinue);

			wxRect visRect = impl->GetRendererClientRect();
			m_karaokeWindow->SetSize(visRect);
			m_karaokeWindow->Show();

			return true;
		}
	}

	return false;
}


void SjKaraokeModule::Stop()
{
	if( m_karaokeWindow )
	{
		SjKaraokeWindow* toDel = m_karaokeWindow;

		toDel->m_timer.Stop();

		// make sure, there are no more waiting images
		g_mainFrame->m_imgThread->RequireKill(toDel);

		// delete the window(s)
		m_karaokeWindow = NULL;
		toDel->m_karaokeModule = NULL;

		toDel->Hide();
		g_mainFrame->Update();
		toDel->Destroy();

		m_impl = NULL;
	}
}


void SjKaraokeModule::ReceiveMsgTrackOnAirChanged(bool justContinue)
{
	if( m_karaokeWindow )
	{
		wxString newUrl = g_mainFrame->GetQueueUrl(-1);

		SjTrackInfo ti;
		g_mainFrame->m_libraryModule->GetTrackInfo(newUrl, ti, SJ_TI_QUICKINFO|SJ_TI_TRACKCOVERURL, false);

		if( m_currUrl != newUrl )
		{
			m_currUrl = newUrl;

			if( m_bg == NULL || !justContinue )
				UpdateBg();

			m_karaokeWindow->m_karaokeMaster.Init(newUrl, ti.m_leadArtistName, ti.m_trackName);
			// no longer switching back from here ...
		}
	}
}


void SjKaraokeModule::ReceiveMsg(int msg)
{
	wxASSERT( wxThread::IsMain() );

	if( msg == IDMODMSG_TRACK_ON_AIR_CHANGED )
	{
		ReceiveMsgTrackOnAirChanged(false);
	}
}


void SjKaraokeModule::PleaseUpdateSize(SjVisImpl* impl)
{
	if( m_karaokeWindow )
	{
		wxRect visRect = impl->GetRendererClientRect();
		m_karaokeWindow->SetSize(visRect);
		m_karaokeWindow->Refresh();
	}
}


void SjKaraokeModule::AddMenuOptions(SjMenu& m)
{
	SjMenu* submenu = new SjMenu(0);
		submenu->AppendRadioItem(IDC_BG_DEFAULT_IMAGES, _("Default"));
		submenu->Check(IDC_BG_DEFAULT_IMAGES, (m_bgType == SJ_KARAOKE_BG_DEFAULT_IMAGES));

		submenu->AppendRadioItem(IDC_BG_BLACK, _("Black"));
		submenu->Check(IDC_BG_BLACK, (m_bgType == SJ_KARAOKE_BG_BLACK));

		if( !m_bgUserDefDir.IsEmpty() )
		{
			submenu->AppendRadioItem(IDC_BG_USERDEF_DIR_USE, SjTools::ShortenUrl(m_bgUserDefDir));
			submenu->Check(IDC_BG_USERDEF_DIR_USE, (m_bgType == SJ_KARAOKE_BG_USERDEF_DIR));
		}

		submenu->Append(IDC_BG_USERDEF_DIR_CHANGE, _("Select..."));
	m.Append(IDC_BG_SUBMENU, _("Background"), submenu);

	m.AppendCheckItem(IDC_BG_USEBGJPG, wxString::Format(_("Use %s-files"), wxT("*.bg.jpg")));
	m.Check(IDC_BG_USEBGJPG, (m_karFlags&SJ_KAR_USEBGJPG)!=0);

	m.AppendCheckItem(IDC_BG_KEEPASPECT, _("Fit to screen"));
	m.Check(IDC_BG_KEEPASPECT, (m_karFlags&SJ_KAR_KEEPASPECT)==0);

	m.AppendCheckItem(IDC_BG_SMOOTH, _("Smooth"));
	m.Check(IDC_BG_SMOOTH, (m_karFlags&SJ_KAR_SMOOTH)!=0);

	m.AppendCheckItem(IDC_BG_GRAYSCALE, _("Grayscale"));
	m.Check(IDC_BG_GRAYSCALE, (m_karFlags&SJ_KAR_GRAYSCALE)!=0);
}


void SjKaraokeModule::OnMenuOption(int i)
{
	switch( i )
	{
		case IDC_BG_BLACK:
			m_bgType = SJ_KARAOKE_BG_BLACK;
			WriteKarConfig();
			UpdateBg();
			break;

		case IDC_BG_USERDEF_DIR_USE:
		case IDC_BG_DEFAULT_IMAGES:
			m_bgType = (i==IDC_BG_USERDEF_DIR_USE)? SJ_KARAOKE_BG_USERDEF_DIR : SJ_KARAOKE_BG_DEFAULT_IMAGES;
			WriteKarConfig();
			InitBgFiles();
			UpdateBg();
			break;

		case IDC_BG_USERDEF_DIR_CHANGE:
			g_visModule->SetModal(true);
			{
				SJ_WINDOW_DISABLER(m_impl->GetWindow());

				::wxBeginBusyCursor(wxHOURGLASS_CURSOR);
				wxDirDialog dirDialog(m_impl->GetWindow(),
				                      _("Please select a directory with images"),
				                      m_bgUserDefDir);
				::wxEndBusyCursor();

				if( dirDialog.ShowModal() == wxID_OK )
				{
					wxFileName fn(dirDialog.GetPath());
					fn.Normalize();

					m_bgUserDefDir = SjLittleDirSel::NormalizeDir(fn.GetFullPath());
					m_bgType = SJ_KARAOKE_BG_USERDEF_DIR;
					WriteKarConfig();
					InitBgFiles();
					UpdateBg();
				}
			}
			g_visModule->SetModal(false);
			break;

		case IDC_BG_SMOOTH:
			SjTools::ToggleFlag(m_karFlags, SJ_KAR_SMOOTH);
			WriteKarConfig();
			UpdateBg(true/*keepImage*/);
			break;

		case IDC_BG_KEEPASPECT:
			SjTools::ToggleFlag(m_karFlags, SJ_KAR_KEEPASPECT);
			WriteKarConfig();
			UpdateBg(true/*keepImage*/);
			break;

		case IDC_BG_GRAYSCALE:
			SjTools::ToggleFlag(m_karFlags, SJ_KAR_GRAYSCALE);
			WriteKarConfig();
			UpdateBg(true/*keepImage*/);
			break;

		case IDC_BG_USEBGJPG:
			SjTools::ToggleFlag(m_karFlags, SJ_KAR_USEBGJPG);
			WriteKarConfig();
			UpdateBg();
			break;
	}
}
