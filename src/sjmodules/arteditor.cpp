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
 * File:    arteditor.cpp
 * Authors: Björn Petersen
 * Purpose: Art Editor
 *
 * We do no longer close the window on deactivation because this is a hard job
 * to make this compatible on the different OS (the events come in different
 * and undocumented order)
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/imgthread.h>
#include <sjbase/browser.h>
#include <sjtools/console.h>
#include <sjtools/msgbox.h>
#include <sjmodules/arteditor.h>
#include <sjmodules/weblinks.h>
#include <sjmodules/library.h>
#include <wx/clipbrd.h>

#define ALWAYS_CLOSE // undef for debugging

#define IDC_SELECTLASTIMG   (IDM_FIRSTPRIVATE+7)
#define IDC_ART_TIMER       (IDM_FIRSTPRIVATE+8)

#define SJ_DEF_ARTEDTOR_SMOOTH TRUE


/*******************************************************************************
 * SjArtEditor
 ******************************************************************************/


class SjArtEditor : public wxFrame
{
public:
	SjArtEditor         (wxWindow* parent,
	                     const wxArrayString& allArtUrls, const wxString& defaultUrl,
	                     long albumId,
	                     bool isPlaylistCover);
	~SjArtEditor        ();
	void            ShowCropMessage     ();
	void            LoadImage           (const wxString& art); // also calls CreateBitmapFromImage()
	void            CreateBitmapFromImage();

	// the URLs the art editor is opened for
	wxArrayString   m_allUrls;
	long            m_albumId;
	wxString        m_defaultUrl;

	// some settings
	int             m_stay;
	bool            m_smooth,
	                m_centerOnDisplay;

private:
	// the currently loaded art
	wxString        m_currUrl;
	wxImage         m_currImage;
	wxBitmap*       m_currBitmap;

	// mouse state
	int             m_mouseLeftDown;
	int             m_cropState; // 0, [b]efore crop, in [c]rop
	wxPoint         m_cropStartPos,
	                m_cropEndPos;
	bool            m_cropRectDrawn;
	wxRect          CalcOrgCrop         (SjImgOp& op, const wxRect& inRect);

	// misc.
	wxString        m_errors;
	bool            m_isPlaylistCover;
	wxTimer         m_timer;
	void            GetMaxSize          (int& w, int& h);

	// just a small hash for the last album ID
	wxString        m_rememberedTrackUrl;
	long            m_rememberedAlbumId;

	// events
	void            OnMouseLeftDown     (wxMouseEvent&);
	void            OnMouseLeftUp       (wxMouseEvent&);
	void            OnMouseCaptureLost  (wxMouseCaptureLostEvent&);
	void            OnMouseRight        (wxMouseEvent&);
	void            OnMouseMotion       (wxMouseEvent&);
	void            OnEraseBackground   (wxEraseEvent&) { }
	void            OnPaint             (wxPaintEvent&);
    void            OnActivate          (wxActivateEvent&);
	void            OnTimer             (wxTimerEvent&);
	void            OnCloseWindow       (wxCloseEvent&);
	void            OnCommand           (wxCommandEvent&);
	DECLARE_EVENT_TABLE ();
};


BEGIN_EVENT_TABLE(SjArtEditor, wxFrame)
	EVT_MENU_RANGE          (IDM_FIRST, IDM_LAST,   SjArtEditor::OnCommand          )
	EVT_MENU                (IDO_SMOOTH,            SjArtEditor::OnCommand          )
	EVT_LEFT_DOWN           (                       SjArtEditor::OnMouseLeftDown    )
	EVT_LEFT_UP             (                       SjArtEditor::OnMouseLeftUp      )
	EVT_MOUSE_CAPTURE_LOST  (                       SjArtEditor::OnMouseCaptureLost )
	EVT_MOTION              (                       SjArtEditor::OnMouseMotion      )
	#ifdef __WXMAC__ // the context menu MUST be opened on down on MAC: TODO: We should use EVT_CONTEXT_MENU instead
	EVT_RIGHT_DOWN          (                       SjArtEditor::OnMouseRight       )
	#else
	EVT_RIGHT_UP            (                       SjArtEditor::OnMouseRight       )
	#endif
	EVT_PAINT               (                       SjArtEditor::OnPaint            )
	EVT_ERASE_BACKGROUND    (                       SjArtEditor::OnEraseBackground  )
	EVT_ACTIVATE            (                       SjArtEditor::OnActivate         )
	EVT_TIMER               (IDC_ART_TIMER,         SjArtEditor::OnTimer            )
	EVT_CLOSE               (                       SjArtEditor::OnCloseWindow      )
END_EVENT_TABLE()


SjArtEditor::SjArtEditor(wxWindow* parent,
                         const wxArrayString& allArtUrls, const wxString& defaultUrl,
                         long albumId, bool isPlaylistCover)
	: wxFrame(parent, -1, _("Cover editor"), wxDefaultPosition, wxDefaultSize, wxFRAME_NO_TASKBAR)

{
	// store backward pointers
	wxASSERT(parent);
	wxASSERT(g_artEditorModule);

	m_albumId           = albumId;
	m_smooth            = g_tools->m_config->Read(wxT("arteditor/imgSmooth"), SJ_DEF_ARTEDTOR_SMOOTH)!=0;
	m_centerOnDisplay   = g_tools->m_config->Read(wxT("arteditor/centerOnDisplay"), 1L)!=0;
	m_currBitmap        = NULL;
	m_defaultUrl        = defaultUrl;
	m_isPlaylistCover   = isPlaylistCover;
	#ifdef ALWAYS_CLOSE
	m_stay              = 0;
	#else
	m_stay              = 1000;
	#endif
	m_cropState         = 0;
	m_cropRectDrawn     = FALSE;

	// if the cover is equal to the currently playing cover, force isPlaylistCover to true (this will update the cover later)
	if( !m_isPlaylistCover )
	{
		if( g_mainFrame->m_columnMixer.GetTrackCoverUrl(g_mainFrame->m_player.m_queue.GetUrlByPos(-1)) == defaultUrl )
			m_isPlaylistCover = true;
	}

	// set accelerators
	const int ACCEL_COUNT = 11;
	wxAcceleratorEntry entries[ACCEL_COUNT];
	entries[ 0].Set(wxACCEL_NORMAL,  WXK_ESCAPE,    IDM_CLOSEARTEDITOR);
	entries[ 1].Set(wxACCEL_NORMAL,  WXK_LEFT,      IDM_PREVIMG);
	entries[ 2].Set(wxACCEL_NORMAL,  WXK_UP,        IDM_PREVIMG);
	entries[ 3].Set(wxACCEL_NORMAL,  WXK_PAGEUP,    IDM_PREVIMG);
	entries[ 4].Set(wxACCEL_NORMAL,  WXK_PAGEDOWN,  IDM_NEXTIMG);
	entries[ 5].Set(wxACCEL_NORMAL,  WXK_BACK,      IDM_PREVIMG);
	entries[ 6].Set(wxACCEL_NORMAL,  WXK_RIGHT,     IDM_NEXTIMG);
	entries[ 7].Set(wxACCEL_NORMAL,  WXK_DOWN,      IDM_NEXTIMG);
	entries[ 8].Set(wxACCEL_NORMAL,  WXK_SPACE,     IDM_NEXTIMG);
	entries[ 9].Set(wxACCEL_NORMAL,  WXK_HOME,      IDM_SELECTIMG00);
	entries[10].Set(wxACCEL_NORMAL,  WXK_END,       IDC_SELECTLASTIMG);
	SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_ART|SJA_ADDNOMODIFIERKEYS, entries, ACCEL_COUNT));

	// init mouse state
	m_mouseLeftDown = 0;

	// get all arts into an array
	m_allUrls = allArtUrls;

	// load art
	LoadImage(defaultUrl);

	// show window
	if( g_mainFrame->IsAlwaysOnTop() )
		SetWindowStyle(GetWindowStyle() | wxSTAY_ON_TOP);

	Show();

	// start timer
	g_mainFrame->GotInputFromUser();
	m_timer.SetOwner(this, IDC_ART_TIMER);
	m_timer.Start(1000);
}


SjArtEditor::~SjArtEditor()
{
	if( m_currBitmap )
		delete m_currBitmap;
}


void SjArtEditor::OnActivate(wxActivateEvent& event)
{
	if( !event.GetActive() && m_stay == 0 )
		Close();
}


void SjArtEditor::OnTimer(wxTimerEvent& event)
{
	SJ_FORCE_IN_HERE_ONLY_ONCE

	// if we're not playing, there is nothing to do
	if( !g_mainFrame || !g_mainFrame->IsPlaying() || m_mouseLeftDown>0 || m_stay>0 )
		return;

	// check settings and timeouts ...
	if( !(g_mainFrame->m_autoCtrl.m_flags&SJ_AUTOCTRL_FOLLOW_PLAYLIST)
	        && !m_isPlaylistCover )
		return;

	long followCoverSeconds = (g_mainFrame->m_autoCtrl.m_flags&SJ_AUTOCTRL_FOLLOW_PLAYLIST)?
	                          (g_mainFrame->m_autoCtrl.m_followPlaylistMinutes*60) :
	                          (SJ_AUTOCTRL_DEF_FOLLOWPLAYLISTMINUTES*60);
	if( followCoverSeconds <= (SJ_AUTOCTRL_MIN_FOLLOWPLAYLISTMINUTES*60) )
		followCoverSeconds = (SJ_AUTOCTRL_MIN_FOLLOWPLAYLISTMINUTES*60);

	if( m_isPlaylistCover )
		followCoverSeconds = 3;

	unsigned long thisTimestamp = SjTools::GetMsTicks();
	unsigned long refTimestamp = g_mainFrame->LastInputFromUser();
	if( thisTimestamp < (refTimestamp+followCoverSeconds*1000) )
		return;

	// find out the currently playing URL and the currently playing albumId
	wxString newTrackUrl = g_mainFrame->m_player.m_queue.GetUrlByPos(-1);
	long     newAlbumId;
	if( newTrackUrl == m_rememberedTrackUrl && !m_rememberedTrackUrl.IsEmpty() )
	{
		newAlbumId = m_rememberedAlbumId;
	}
	else
	{
		wxSqlt sql;
		sql.Query(wxT("SELECT albumid FROM tracks WHERE url='") + sql.QParam(newTrackUrl) + wxT("';"));
		if( !sql.Next() )
			return;
		newAlbumId = sql.GetLong(0);
		m_rememberedAlbumId = newAlbumId;
		m_rememberedTrackUrl = newTrackUrl;
	}

	// albumId not changed? nothing to do
	if( m_albumId == newAlbumId )
		return;

	// load the new possible art URLs
	wxArrayLong     newArtIds;
	wxArrayString   newArtUrls;

	g_mainFrame->m_libraryModule->GetPossibleAlbumArts(newAlbumId, newArtIds, &newArtUrls, TRUE/*addAutoCover*/);
	if( newArtIds.IsEmpty() )
		return;

	wxString newDefaultUrl = g_mainFrame->m_columnMixer.GetTrackCoverUrl(newTrackUrl);
	if( newDefaultUrl.IsEmpty() )
		return;

	m_albumId    = newAlbumId;
	m_defaultUrl = newDefaultUrl;
	m_allUrls    = newArtUrls;
	LoadImage(newDefaultUrl);
}


void SjArtEditor::OnCloseWindow(wxCloseEvent& event)
{
	if( g_artEditorModule )
		g_artEditorModule->m_artEditor = NULL;

	Show(FALSE); // needed as destroy may be delayed if the application gets inactive...
	Destroy();
}


void SjArtEditor::ShowCropMessage()
{
	m_stay++;
	wxWindowDisabler disabler(this);

	::SjMessageBox(_("Please press and hold the left mouse button. Then, select the area to crop by moving the mouse."),
	               SJ_PROGRAM_NAME, wxOK|wxICON_INFORMATION, this);

	m_stay--;
}


wxRect SjArtEditor::CalcOrgCrop(SjImgOp& op, const wxRect& inRect)
{
	if( m_currBitmap == NULL )
	{
		return inRect; // error
	}

	// Function calculates the original cropping rectangle from the given rectangle
	// which may contain resizing, mirroring and previous cropping.
	int x = inRect.x,
	    y = inRect.y,
	    w = inRect.width,
	    h = inRect.height;
	int scaledW = m_currBitmap->GetWidth(),
	    scaledH = m_currBitmap->GetHeight();
	int currOrgW = m_currImage.GetWidth(),
	    currOrgH = m_currImage.GetHeight();

	// apply rotating
	if( op.m_flags & (SJ_IMGOP_ROTATE90|SJ_IMGOP_ROTATE270) )
	{
		int temp = scaledW; scaledW = scaledH; scaledH = temp;
		temp = w; w = h; h = temp;
		temp = x; x = y; y = temp;
		if( op.m_flags & SJ_IMGOP_ROTATE90 )
		{
			y = scaledH - y - h;
		}
		else
		{
			x = scaledW - x - w;
		}
	}
	else if( op.m_flags & SJ_IMGOP_ROTATE180 )
	{
		x = scaledW - x - w;
		y = scaledH - y - h;
	}

	// apply flipping
	if( op.m_flags & SJ_IMGOP_FLIPHORZ )
	{
		x = scaledW - x - w;
	}

	if( op.m_flags & SJ_IMGOP_FLIPVERT ) {
		y = scaledH - y - h;
	}

	// apply resizing
	x = (x * currOrgW) / scaledW;
	w = (w * currOrgW) / scaledW;
	y = (y * currOrgH) / scaledH;
	h = (h * currOrgH) / scaledH;

	// apply previous cropping
	if( op.m_flags & SJ_IMGOP_CROP )
	{
		x = op.m_cropX + ((x * op.m_cropW) / currOrgW);
		y = op.m_cropY + ((y * op.m_cropH) / currOrgH);
		w = (w * op.m_cropW) / currOrgW;
		h = (h * op.m_cropH) / currOrgH;
	}

	// done
	return wxRect(x, y, w, h);
}


void SjArtEditor::OnMouseLeftDown(wxMouseEvent& event)
{
	g_mainFrame->GotInputFromUser();

	m_mouseLeftDown++;

	if( g_mainFrame->IsAllAvailable() )
	{
		m_cropState = 'b';
		m_cropStartPos = wxPoint(event.GetX(), event.GetY()); // client coordinates
	}
}


void SjArtEditor::OnMouseLeftUp(wxMouseEvent& event)
{
	// crop?
	if( m_cropRectDrawn )
	{
		wxClientDC dc(this);
		SjTools::DrawRubberbox(dc, m_cropStartPos, m_cropEndPos);
		m_cropRectDrawn = false;
	}

	if( HasCapture() )
	{
		ReleaseMouse();
	}

	if( m_cropState == 'c' )
	{
		m_cropState = 0;
		m_mouseLeftDown = 0;

		SjImgOp op;
		op.LoadFromDb(m_currUrl);

		wxRect r = CalcOrgCrop(op, wxRect(m_cropStartPos, m_cropEndPos));
		if( r.width >= 16 && r.height >= 16 )
		{
			op.SetCrop(r);
			op.SaveToDb(m_currUrl);
			CreateBitmapFromImage();
			g_mainFrame->m_browser->RefreshAll();
		}
		return; // nothing more to do
	}
	m_cropState = 0;

	// make sure, we received a mouse down event before
	if( m_mouseLeftDown<=0 )
	{
		return; // nothing more to do
	}
	m_mouseLeftDown = 0;

	// close the window
	if( m_stay == 0 )
	{
		Close();
	}
}


void SjArtEditor::OnMouseCaptureLost(wxMouseCaptureLostEvent&)
{
	wxLogDebug(wxT("~~~~~~~~~~ CAPTURE LOST"));

	if( m_cropRectDrawn )
	{
		wxClientDC dc(this);
		SjTools::DrawRubberbox(dc, m_cropStartPos, m_cropEndPos);
		m_cropRectDrawn = false;
	}

	m_cropState = 0;
	m_mouseLeftDown = 0;
}


void SjArtEditor::OnMouseMotion(wxMouseEvent& event)
{
	long x = event.GetX(); // client coordinates
	long y = event.GetY(); // client coordinates

	if( m_cropState == 'b' )
	{
		// switch to crop?
		long hDifference = x - m_cropStartPos.x;
		long vDifference = y - m_cropStartPos.y;
		if( hDifference >  SJ_CROP_DELTA
		        || hDifference < -SJ_CROP_DELTA
		        || vDifference >  SJ_CROP_DELTA
		        || vDifference < -SJ_CROP_DELTA )
		{
			m_cropState = 'c';
			m_cropRectDrawn = FALSE;
			CaptureMouse();
		}
	}

	if( m_cropState == 'c' )
	{
		wxSize clientSize = GetClientSize();
		if( x < 0 )            { x = 0; }
		if( y < 0 )            { y = 0; }
		if( x > clientSize.x ) { x = clientSize.x; }
		if( y > clientSize.y ) { y = clientSize.y; }

		if( x != m_cropEndPos.x
		        || y != m_cropEndPos.y )
		{
			wxClientDC dc(this);

			if( m_cropRectDrawn )
			{
				SjTools::DrawRubberbox(dc, m_cropStartPos, m_cropEndPos);
				m_cropRectDrawn = FALSE;
			}

			m_cropEndPos.x = x;
			m_cropEndPos.y = y;

			SjTools::DrawRubberbox(dc, m_cropStartPos, m_cropEndPos);
			m_cropRectDrawn = TRUE;
		}
	}
}


void SjArtEditor::OnMouseRight(wxMouseEvent& event)
{
	// stop cropping?
	if( m_cropRectDrawn )
	{
		wxClientDC dc(this);
		SjTools::DrawRubberbox(dc, m_cropStartPos, m_cropEndPos);
		m_cropRectDrawn = FALSE;
	}

	if( HasCapture() )
	{
		ReleaseMouse();
	}

	// show context menu
	SjMenu menu(0);
	if( g_artEditorModule )
	{
		g_artEditorModule->CreateArtMenu(menu,
		                                 m_allUrls,
		                                 m_currUrl,
		                                 m_defaultUrl,
		                                 this);
	}

	if( g_mainFrame->IsAllAvailable()
	        && menu.GetMenuItemCount() )
	{
		m_mouseLeftDown = -1; // ignore next click as it will only close the menu
		m_stay++;
			PopupMenu(&menu, event.GetX(), event.GetY());
		m_stay--;
	}
}


void SjArtEditor::OnCommand(wxCommandEvent& event)
{
	int     id = event.GetId();
	bool    idUsed = TRUE;

	if( id == IDM_CLOSEARTEDITOR )
	{
		Close();
	}
	else if( id == IDM_PREVIMG || id == IDM_NEXTIMG )
	{
		if( m_allUrls.GetCount() > 1 )
		{
			long itemCount = (long)m_allUrls.GetCount();
			long newIndex = m_allUrls.Index(m_currUrl) + (id==IDM_NEXTIMG? 1 : -1);

			if( newIndex < 0          ) newIndex = itemCount-1;
			if( newIndex >= itemCount ) newIndex = 0;

			if( m_allUrls[newIndex].StartsWith(wxT("cover:"))/*last*/ && itemCount>=2 ) newIndex = id==IDM_NEXTIMG? 0 : itemCount-2;

			LoadImage(m_allUrls.Item(newIndex));
		}
	}
	else if( (id == IDM_EXPLORE || (id >= IDM_WWWCOVER00 && id <= IDM_WWWCOVER49))
	         && g_mainFrame->IsAllAvailable() )
	{
		Close(); // close first, to avoid the main frame becoming active on opening th browser in OnArtMenu()
		if( g_artEditorModule )
		{
			g_artEditorModule->OnArtMenu(this, m_allUrls, m_currUrl, m_albumId, id, this, m_isPlaylistCover);
		}
	}
	else if( id == IDO_SMOOTH && g_mainFrame->IsAllAvailable() )
	{
		m_smooth = !m_smooth;
		g_tools->m_config->Write(wxT("arteditor/imgSmooth"), m_smooth? 1L : 0L);
		CreateBitmapFromImage();
	}
	else if( id == IDM_FULLSCREEN && g_mainFrame->IsAllAvailable() )
	{
		m_centerOnDisplay = !m_centerOnDisplay;
		g_tools->m_config->Write(wxT("arteditor/centerOnDisplay"), m_centerOnDisplay? 1L : 0L);
		CreateBitmapFromImage();
	}
	else if( g_artEditorModule
	         && g_artEditorModule->OnArtMenu(this, m_allUrls, m_currUrl, m_albumId, id, this, m_isPlaylistCover) )
	{
		;
	}
	else
	{
		idUsed = FALSE;
	}

	if( idUsed )
	{
		m_mouseLeftDown = 0; // the mouse click seems to be used
	}
}


void SjArtEditor::GetMaxSize(int& maxW, int& maxH)
{
	if( GetParent() == NULL
	        || m_centerOnDisplay
	        || g_mainFrame->IsKioskStarted() )
	{
		wxRect displRect = SjDialog::GetDisplayWorkspaceRect(g_mainFrame);
		if( g_mainFrame->IsKioskStarted() )
		{
			displRect = g_mainFrame->GetRect();
		}

		maxW = displRect.width;
		maxH = displRect.height;
	}
	else
	{
		g_mainFrame->GetSize(&maxW, &maxH);
		if( maxW < 400 ) maxW = 400;
		if( maxH < 400 ) maxH = 400;
	}
	maxW -= maxW/50;
	maxH -= maxH/50;
}


void SjArtEditor::LoadImage(const wxString& url)
{
	wxBusyCursor busy;
	wxFileSystem fileSystem;

	SjLogString artLog;

	m_currUrl = url;

	// load file
	if( url.StartsWith(wxT("cover:")) )
	{
		int maxW, maxH;
		GetMaxSize(maxW, maxH);
		m_currImage = SjImgOp::CreateDummyCover(url, maxH);
	}
	else
	{
		wxFSFile* fsFile = fileSystem.OpenFile(url, wxFS_READ|wxFS_SEEKABLE); // i think, seeking is needed by at least one format ...
		if( fsFile )
		{
			m_currImage.LoadFile(*(fsFile->GetStream()), wxBITMAP_TYPE_ANY);
			delete fsFile;
		}
	}

	if( !m_currImage.IsOk() )
	{
		wxLogError(_("Cannot open \"%s\"."), url.c_str());
	}


	// done so far
	CreateBitmapFromImage();

	m_errors = artLog.GetAndClearErrors();
}


void SjArtEditor::CreateBitmapFromImage()
{
	wxBusyCursor busy;
	int          imgW, imgH;

	// delete old bitmap
	if( m_currBitmap )
	{
		delete m_currBitmap;
		m_currBitmap = NULL;
	}

	// get max. display size
	int maxW, maxH;
	GetMaxSize(maxW, maxH);

	// calculate image size
	if( m_currImage.IsOk() )
	{
		wxImage currImage = m_currImage.Copy(); // a simple assignment will share the data

		// load operations
		SjImgOp op;
		op.LoadFromDb(m_currUrl);

		// first crop
		if( op.HasFlag(SJ_IMGOP_CROP) )
		{
			op.DoCrop(currImage);
			op.ClearCrop();
		}

		// then rotate
		if( op.IsRotated() )
		{
			op.DoRotate(currImage);
			op.ClearRotate();
		}

		// calculate final size, and apply the other operations
		long orgW = currImage.GetWidth(),
		     orgH = currImage.GetHeight();

		op.m_flags |= SJ_IMGOP_RESIZE;
		if( m_smooth )
		{
			op.m_flags |= SJ_IMGOP_SMOOTH;
		}

		op.m_resizeH = maxH;
		op.m_resizeW = (op.m_resizeH*orgW)/orgH;
		if( op.m_resizeW > maxW )
		{
			op.m_resizeW = maxW;
			op.m_resizeH = (op.m_resizeW*orgH)/orgW;
		}

		op.Do(currImage);

		m_currBitmap = new wxBitmap(currImage);
		if( m_currBitmap )
		{
			if( !m_currBitmap->IsOk() )
			{
				delete m_currBitmap;
				m_currBitmap = NULL;
			}
		}

		imgW = op.m_resizeW;
		imgH = op.m_resizeH;
	}
	else
	{
		imgW = maxH;
		imgH = maxH;
	}

	// calculate the window size
	SetSize(imgW, imgH);

	if( GetParent()==NULL
	        || m_centerOnDisplay
	        || g_mainFrame->IsKioskStarted() )
	{
		SjDialog::CenterOnDisplay(this, g_mainFrame);
	}
	else
	{
		CenterOnParent();
	}

	Refresh();
}


void SjArtEditor::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxSize windowSize = GetClientSize();

	if( m_currBitmap )
	{
		dc.DrawBitmap(*m_currBitmap, 0, 0, FALSE /*not transparent*/);
	}
	else
	{
		dc.SetBrush(*wxWHITE_BRUSH);
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.DrawRectangle(0, 0, windowSize.x, windowSize.y);
	}

	if( !m_errors.IsEmpty() )
	{
		wxRect rect(10, 10, windowSize.x-20, windowSize.y-20);
		g_tools->DrawText(dc, m_errors,
		                  rect,
		                  g_mainFrame->m_currStdFont, g_mainFrame->m_currStdFont, *wxBLACK);
	}

	if( m_cropRectDrawn )
	{
		SjTools::DrawRubberbox(dc, m_cropStartPos, m_cropEndPos);
	}
}


/*******************************************************************************
 * SjArtEditorModule: Open etc.
 ******************************************************************************/


SjArtEditorModule* g_artEditorModule = NULL;


SjArtEditorModule::SjArtEditorModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	m_file              = wxT("memory:arteditor.lib");
	m_name              = _("Cover editor");
	m_artEditor         = NULL;
	g_artEditorModule   = this;
}


void SjArtEditorModule::LastUnload()
{
	if( m_artEditor )
	{
		m_artEditor->Close();
		m_artEditor = NULL;
	}

	g_artEditorModule = NULL;
}


/*******************************************************************************
 * SjArtEditorModule: Menu Handling
 ******************************************************************************/


void SjArtEditorModule::CreateArtMenu(
    SjMenu&                 menu,
    const wxArrayString&    allUrls,
    const wxString&         selUrl,
    const wxString&         defaultUrl,
    SjArtEditor*            artEditor /*may be NULL*/  )
{
	wxFileSystem fileSystem;
	SjMenu* submenu;
	int     i;
	long    showShortcuts = artEditor? menu.ShowShortcuts() : 0;
	long    oldShortcutSetting = menu.ShowShortcuts(showShortcuts);

	SjImgOp op;
	op.LoadFromDb(selUrl);


	if( g_mainFrame->IsAllAvailable() )
	{
		// open cover
		submenu = new SjMenu(showShortcuts);
		{
			int allUrlsCount = allUrls.GetCount();
			wxString txt;

			for( i=0; i<allUrlsCount; i++ )
			{
				if( allUrls.Item(i).StartsWith(wxT("cover:")) )
				{
					txt = _("<Abstract>");
				}
				else
				{
					txt = SjTools::ShortenUrl(allUrls.Item(i));
				}
				if( allUrls.Item(i) == defaultUrl ) txt << wxT(" ") << _("(Default)");

				submenu->AppendRadioItem(IDM_SELECTIMG00+i, txt);
				if( allUrls.Item(i) == selUrl )
				{
					submenu->Check(IDM_SELECTIMG00+i, TRUE);
				}
			}

			submenu->AppendSeparator();

			// paste
			if( !artEditor )
			{

				bool enable = FALSE;
				if( wxTheClipboard->Open() )
				{
					if( wxTheClipboard->IsSupported(wxDF_FILENAME)
					        || wxTheClipboard->IsSupported(wxDF_BITMAP) )
					{
						enable = TRUE;
					}
					else if( wxTheClipboard->IsSupported(wxDF_TEXT) )
					{
						wxTextDataObject textObj;
						wxTheClipboard->GetData(textObj);
						wxString text  = textObj.GetText();
						if( !text.IsEmpty()
						        /*&& ::wxFileExists(text) -- if the string eg. begins with "a:", windows will promt the user to insert a disk, so we do nocht check this currently*/ )
						{
							enable = TRUE;
						}
					}
					wxTheClipboard->Close();
				}

				submenu->ShowShortcuts(oldShortcutSetting);
				submenu->Append(IDO_PASTE_USING_COORD, _("Paste image from clipboard"));
				submenu->Enable(IDO_PASTE_USING_COORD, enable);
				submenu->ShowShortcuts(showShortcuts);
			}

			submenu->Append(IDM_OPENCOVER, _("Browse")+wxString(wxT("...")));

		}
		menu.Append(0, _("Select cover"), submenu);

		// "covers on the web"
		submenu = new SjMenu(showShortcuts);
		SjWebLinks weblinks(SJ_WEBLINK_COVERSEARCH);
		for( i = 0; i < (int)weblinks.GetCount(); i++ )
		{
			submenu->Append(IDM_WWWCOVER00+i, weblinks.GetName(i));
		}
		menu.Append(0, _("Search covers on the web"), submenu);

		// save cover
		menu.Append(IDM_SAVECOVER);

		/* -- we won't enable/disable the menu entry as checking the fsFile object
		   -- may take some time - esp. when the file is a remove file or is embedded to a MP3 file
		wxFSFile* fsFile = fileSystem.OpenFile(selUrl);
		if( fsFile )
		{
		    delete fsFile;
		}
		else
		{
		    menu.Enable(IDM_SAVECOVER, FALSE);
		}
		   -- instead we just check if it is an auto-generated cover (which is not saveable at the moment)*/
		if( selUrl.StartsWith(wxT("cover:")) )
		{
			menu.Enable(IDM_SAVECOVER, FALSE);
		}
	}


	if( !artEditor )
	{
		// enlarge
		menu.Append(IDM_OPENARTEDITOR, _("Cover editor"));
	}

	if( g_mainFrame->IsAllAvailable() )
	{

		// rotate/mirror
		submenu = new SjMenu(showShortcuts);

		submenu->AppendCheckItem(IDM_ROTATERIGHT);
		submenu->Check(IDM_ROTATERIGHT, op.IsRightRotated());

		submenu->AppendCheckItem(IDM_ROTATELEFT);
		submenu->Check(IDM_ROTATELEFT, op.IsLeftRotated());

		submenu->Append(IDM_DONTROTATE, _("Don't rotate"));
		submenu->Enable(IDM_DONTROTATE, op.IsRotated());

		submenu->AppendSeparator();

		submenu->AppendCheckItem(IDM_FLIPHORZ);
		submenu->Check(IDM_FLIPHORZ, op.HasFlag(SJ_IMGOP_FLIPHORZ));

		submenu->AppendCheckItem(IDM_FLIPVERT);
		submenu->Check(IDM_FLIPVERT, op.HasFlag(SJ_IMGOP_FLIPVERT));
		menu.Append(0, _("Rotate/flip"), submenu);

		// effects/options
		submenu = new SjMenu(showShortcuts);

		int cropId = (artEditor||op.HasFlag(SJ_IMGOP_CROP))? IDM_CROP : IDM_OPENARTEDFORCROP;
		submenu->AppendCheckItem(cropId, _("Crop"));
		submenu->Check(cropId, op.HasFlag(SJ_IMGOP_CROP));

		submenu->AppendSeparator();

		{
			int b = op.GetBrightness(), c = op.GetContrast();

			submenu->AppendCheckItem(IDM_BRIGHTMINUS);
			submenu->Check          (IDM_BRIGHTMINUS, (b<0));
			submenu->AppendCheckItem(IDM_BRIGHTPLUS);
			submenu->Check          (IDM_BRIGHTPLUS, (b>0));
			submenu->AppendCheckItem(IDM_CONTRASTMINUS);
			submenu->Check          (IDM_CONTRASTMINUS, (c<0));
			submenu->AppendCheckItem(IDM_CONTRASTPLUS);
			submenu->Check          (IDM_CONTRASTPLUS, (c>0));
			submenu->AppendCheckItem(IDM_CONTRASTBRIGHTNULL);
			submenu->Enable         (IDM_CONTRASTBRIGHTNULL, (b||c));
		}

		submenu->AppendSeparator();

		submenu->AppendCheckItem(IDM_GRAYSCALE);
		submenu->Check(IDM_GRAYSCALE, op.HasFlag(SJ_IMGOP_GRAYSCALE));

		submenu->AppendCheckItem(IDM_NEGATIVE);
		submenu->Check(IDM_NEGATIVE, op.HasFlag(SJ_IMGOP_NEGATIVE));

		submenu->AppendSeparator();

		submenu->ShowShortcuts(oldShortcutSetting);
		submenu->AppendCheckItem(IDO_SMOOTH);
		submenu->Check(IDO_SMOOTH, artEditor?
		               (artEditor->m_smooth!=0)
		               : ((g_mainFrame->m_skinFlags&SJ_SKIN_IMG_SMOOTH)!=0));
		submenu->ShowShortcuts(showShortcuts);

		if( artEditor )
		{
			submenu->AppendCheckItem(IDM_FULLSCREEN, _("Fit to screen"));
			submenu->Check(IDM_FULLSCREEN, artEditor->m_centerOnDisplay);
		}

		submenu->Append(IDM_NOALTCOVERS, _("Use default images..."));

		menu.Append(0, _("Effects/options"), submenu);

		// show file
		menu.Append(IDM_EXPLORE, _("Show file"));
		menu.Enable(IDM_EXPLORE, g_tools->IsUrlExplorable(selUrl));
	}

	// done
	menu.ShowShortcuts(oldShortcutSetting);
}


bool SjArtEditorModule::OnArtMenu(
    wxWindow*            parent,
    const wxArrayString& allUrls,
    const wxString&      selUrl,
    long                 albumId,
    long                 id,
    SjArtEditor*         artEditor,
    bool                 isPlaylistCover )
{
	bool    idUsed = TRUE,
	        reloadView = FALSE;

	SjImgOp op;
	op.LoadFromDb(selUrl);

	SjImgOp orgOp = op;

	if( id >= IDM_SELECTIMG00 && id <= IDM_SELECTIMG299 )
	{
		// select a cover
		long oldIndex = allUrls.Index(selUrl);
		long newIndex = id-IDM_SELECTIMG00;
		if( oldIndex != newIndex )
		{
			if( artEditor )
			{
				if( newIndex >= 0 && newIndex < (long)artEditor->m_allUrls.GetCount() )
				{
					artEditor->LoadImage(artEditor->m_allUrls.Item(newIndex));
				}
			}
			else if( g_mainFrame->IsAllAvailable() )
			{
				wxSqlt sql;
				wxArrayLong artIds;
				g_mainFrame->m_libraryModule->GetPossibleAlbumArts(albumId, artIds, NULL, TRUE/*addAutoCover*/);
				sql.Query(wxString::Format(wxT("UPDATE albums SET artiduser=%lu WHERE id=%lu;"), artIds.Item(newIndex), albumId));
				reloadView = TRUE;
			}
		}
	}
	else if( id >= IDM_WWWCOVER00 && id <= IDM_WWWCOVER49 && g_mainFrame->IsAllAvailable() )
	{
		// search covers on the web
		wxString albumName, leadArtistName;
		{
			wxSqlt sql;
			sql.Query(wxString::Format(wxT("SELECT albumname, leadartistname FROM albums WHERE id=%lu;"), albumId));
			if( sql.Next() )
			{
				albumName = sql.GetString(0);
				leadArtistName = sql.GetString(1);
			}
		}

		SjWebLinks weblinks(SJ_WEBLINK_COVERSEARCH);
		weblinks.Explore(id-IDM_WWWCOVER00, leadArtistName, albumName, wxT(""));
	}
	else switch( id )
		{
				// select the last image
			case IDC_SELECTLASTIMG:
			{
				long newIndex = allUrls.GetCount()-1;
				if( allUrls[newIndex].StartsWith(wxT("cover:")) && newIndex > 0 )
				{
					newIndex--;
				}
				OnArtMenu(parent, allUrls, selUrl, albumId, IDM_SELECTIMG00+newIndex, artEditor, isPlaylistCover);
			}
			break;

			// open art editor (for crop)
			case IDM_OPENARTEDITOR:
			case IDM_OPENARTEDFORCROP:
				if( m_artEditor )
				{
					m_artEditor->Destroy();
					m_artEditor = NULL;
				}

				m_artEditor = new SjArtEditor(parent, allUrls, selUrl, albumId, isPlaylistCover);

				if( id==IDM_OPENARTEDFORCROP && m_artEditor && g_mainFrame->IsAllAvailable() )
				{
					m_artEditor->ShowCropMessage();
				}
				break;

				// open a file
			case IDM_OPENCOVER:
				if( albumId && g_mainFrame->IsAllAvailable() )
				{
					if( artEditor ) { artEditor->m_stay++; }
					wxWindowDisabler disabler(parent);
					wxFileDialog fileDialog(parent, _("Select image"), wxT(""), wxT(""),
					                        g_mainFrame->m_moduleSystem.GetAssignedExt(SJ_EXT_IMAGEFILES).GetFileDlgStr(), wxFD_OPEN|wxFD_CHANGE_DIR);
					int fileDialogRet = fileDialog.ShowModal();
					if( artEditor ) { artEditor->m_stay--; }

					if( fileDialogRet == wxID_OK )
					{
						wxString newImage = fileDialog.GetPath();
						if( ::wxFileExists(newImage) )
						{
							// check if the file is already in the album's image list
							for( size_t i = 0; i < allUrls.GetCount(); i++ )
							{
								if( allUrls.Item(i).CmpNoCase(newImage) == 0 )
								{
									return OnArtMenu(parent, allUrls, selUrl, albumId, IDM_SELECTIMG00+i, artEditor, isPlaylistCover);
								}
							}

							// add the image to the list of directories
							wxSqlt sql;
							SjDataObject data(wxDragMove);
							data.AddFile(newImage);
							if( g_mainFrame->m_libraryModule->AddArt__(&data, albumId, FALSE/*don't ask*/) )
							{
								reloadView = TRUE;
								if( artEditor )
								{
									wxArrayLong artIds;
									wxArrayString artUrls;

									g_mainFrame->m_libraryModule->GetPossibleAlbumArts(albumId, artIds, &artUrls, TRUE/*addAutoCover*/);
									m_artEditor->m_allUrls = artUrls;
									m_artEditor->LoadImage(newImage);
								}
							}
						}
					}
				}
				break;

			case IDM_SAVECOVER:
				if( g_mainFrame->IsAllAvailable() )
				{
					if( artEditor ) { artEditor->m_stay++; }
					wxWindowDisabler disabler(parent);

					SjExtList extList(SjTools::GetExt(selUrl));
					wxFileDialog dlg(parent, _("Save cover"), wxT(""),
					                 wxFileName(selUrl).GetFullName().AfterLast(wxT(':')),
					                 extList.GetFileDlgStr(wxFD_SAVE),
					                 wxFD_SAVE|wxFD_OVERWRITE_PROMPT|wxFD_CHANGE_DIR);

					if( dlg.ShowModal() == wxID_OK )
					{
						wxString destPath, destExt;
						extList.GetFileDlgPath(dlg, destPath, destExt);

						if( SjTools::AreFilesSame(selUrl, destPath) )
						{
							wxLogError(wxT("You cannot copy an image to itself."));
							wxLogError(_("Cannot write \"%s\"."), destPath.c_str());
						}
						else
						{
							wxFile outputFile(destPath, wxFile::write);
							if( outputFile.IsOpened() )
							{
								wxFileSystem fileSystem;
								wxFSFile* inputFile = fileSystem.OpenFile(selUrl);
								if( inputFile )
								{
									SjTools::CopyStreamToFile(*(inputFile->GetStream()), outputFile);
									delete inputFile;
								}
							}
						}
					}

					if( artEditor ) { artEditor->m_stay--; }
				}
				break;

				// reset alternative covers?
			case IDM_NOALTCOVERS:
				if( g_mainFrame->IsAllAvailable() )
				{
					if( artEditor ) { artEditor->m_stay++; }
					wxWindowDisabler disabler(parent);

					if( ::SjMessageBox(_("Reset all selected alternative images to their default value?"),
					                   SJ_PROGRAM_NAME, wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION, parent) == wxYES )
					{
						wxBusyCursor busy;
						wxSqlt sql;
						sql.Query(wxT("UPDATE albums SET artiduser=0;"));
						reloadView = TRUE;
					}

					if( artEditor ) { artEditor->m_stay--; }
				}
				break;

				// show selected file
			case IDM_EXPLORE:
				if( g_mainFrame->IsAllAvailable() )
				{
					g_tools->ExploreUrl(selUrl);
				}
				break;

				// increase/decrease contrast
			case IDM_CONTRASTPLUS:
			case IDM_CONTRASTMINUS:
			{
				long add = id==IDM_CONTRASTPLUS? +10 : -10;
				op.SetContrast(op.GetContrast()+add);
			}
			break;

			// increase/decrease brightness
			case IDM_BRIGHTPLUS:
			case IDM_BRIGHTMINUS:
			{
				long add = id==IDM_BRIGHTPLUS? +10 : -10;
				op.SetBrightness(op.GetBrightness()+add);
			}
			break;

			// center contrast and brightness
			case IDM_CONTRASTBRIGHTNULL:
				op.SetContrast(0);
				op.SetBrightness(0);
				break;

				// rotate the image
			case IDM_DONTROTATE: op.ClearRotate();      break;
			case IDM_ROTATERIGHT:op.SetRotatePlus90();  break;
			case IDM_ROTATELEFT: op.SetRotateMinus90(); break;

				// flip the imaeg
			case IDM_FLIPHORZ:   op.ToggleFlag(SJ_IMGOP_FLIPHORZ); break;
			case IDM_FLIPVERT:   op.ToggleFlag(SJ_IMGOP_FLIPVERT); break;

				// toggle other effects
			case IDM_GRAYSCALE:  op.ToggleFlag(SJ_IMGOP_GRAYSCALE); break;
			case IDM_NEGATIVE:   op.ToggleFlag(SJ_IMGOP_NEGATIVE);  break;

				// crop the image
			case IDM_CROP:
				if( op.HasFlag(SJ_IMGOP_CROP) )
				{
					op.ClearCrop();
				}
				else if( m_artEditor && g_mainFrame->IsAllAvailable() )
				{
					m_artEditor->ShowCropMessage();
				}
				break;

				// nothing to do
			default:
				idUsed = FALSE;
				break;
		}

	// image settings changed?
	if( op != orgOp && g_mainFrame->IsAllAvailable() )
	{
		op.SaveToDb(selUrl);
		reloadView = TRUE;
	}

	// reload view? (the art editor will reflect its changes himself)
	if( reloadView )
	{
		g_mainFrame->m_browser->RefreshAll(); // reflect changes in the browser
		if( artEditor )
		{
			artEditor->CreateBitmapFromImage();
		}
	}

	// done so far
	return idUsed;
}

