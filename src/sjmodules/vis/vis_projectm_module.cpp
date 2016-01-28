/*******************************************************************************
 *
 *                                 Silverjuke
 *     Copyright (C) 2016 Björn Petersen Software Design and Development
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
 * File:    vis_projectm_module.cpp
 * Authors: Björn Petersen
 * Purpose: ProjectM visualisation module
 *
 ******************************************************************************/


#include <sjbase/base.h>
#if SJ_USE_PROJECTM
#include <wx/glcanvas.h>
#include <sjmodules/vis/vis_window.h>
#include <sjmodules/vis/vis_projectm_module.h>
#include <prjm/src/projectM.hpp>
#include <prjm/src/Renderer/BeatDetect.hpp>


/*******************************************************************************
 * the window holding the vis.
 ******************************************************************************/


static SjProjectmModule* s_theProjectmModule = NULL;


#define SLEEP_MS                40 // results in 25 frames/s - TODO: We should adapt this to larger values if rendering takes too long (or render only every Nth frame, so that we need not to change the timer)
#define IDC_TIMER               (IDM_FIRSTPRIVATE+130)


class SjProjectmGlCanvas : public wxGLCanvas
{
public:
				SjProjectmGlCanvas (wxWindow* parent);

	bool        ImplOk() const { return (s_theProjectmModule!=NULL&&s_theProjectmModule->m_impl); }

	void        OnEraseBackground   (wxEraseEvent& e);
	void        OnPaint             (wxPaintEvent& e);

	void        OnMouseLeftDown     (wxMouseEvent& e)   { if(ImplOk()) s_theProjectmModule->m_impl->OnMouseLeftDown(this, e); }
	void        OnMouseLeftUp       (wxMouseEvent& e)   { if(ImplOk()) s_theProjectmModule->m_impl->OnMouseLeftUp(this, e); }
	void        OnMouseRightUp      (wxContextMenuEvent& e)   { if(ImplOk()) s_theProjectmModule->m_impl->OnMouseRightUp(this, e); }
	void        OnMouseLeftDClick   (wxMouseEvent& e)   { if(ImplOk()) s_theProjectmModule->m_impl->OnMouseLeftDClick(this, e); }

	void        OnTimer             (wxTimerEvent&);

	wxTimer     m_timer;

    long                m_sampleCount;
    unsigned char*      m_bufferStart;
    bool                m_triedCreation;

	DECLARE_EVENT_TABLE ();
};


BEGIN_EVENT_TABLE(SjProjectmGlCanvas, wxWindow)
	EVT_LEFT_DOWN       (                       SjProjectmGlCanvas::OnMouseLeftDown     )
	EVT_LEFT_UP         (                       SjProjectmGlCanvas::OnMouseLeftUp       )
	EVT_LEFT_DCLICK     (                       SjProjectmGlCanvas::OnMouseLeftDClick   )
	EVT_CONTEXT_MENU    (                       SjProjectmGlCanvas::OnMouseRightUp      )

	EVT_ERASE_BACKGROUND(                       SjProjectmGlCanvas::OnEraseBackground   )
	EVT_PAINT           (                       SjProjectmGlCanvas::OnPaint             )

	EVT_TIMER           (IDC_TIMER,             SjProjectmGlCanvas::OnTimer             )
END_EVENT_TABLE()


SjProjectmGlCanvas::SjProjectmGlCanvas(wxWindow* parent)
	: wxGLCanvas(parent, wxID_ANY, NULL, wxDefaultPosition, wxDefaultSize)
{
	// get the number of samples wanted
	// (currently we exactly 576 for the spcectrum analyzer)
	m_sampleCount = SjMs2Bytes(SLEEP_MS) / (SJ_WW_CH*SJ_WW_BYTERES);
	if( m_sampleCount < 576*2 )
	{
		m_sampleCount = 576*2;
	}

	m_bufferStart = (unsigned char*)malloc(m_sampleCount*SJ_WW_CH*SJ_WW_BYTERES);

	m_triedCreation = false;
}


void SjProjectmGlCanvas::OnEraseBackground(wxEraseEvent& e)
{

}


void SjProjectmGlCanvas::OnPaint(wxPaintEvent& e)
{
	wxPaintDC paintDc(this); // this declararion is needed to validate the rectangles; redrawing is done by OpenGL
	if( s_theProjectmModule->m_projectMobj == NULL )
	{
		paintDc.SetBrush(*wxBLACK_BRUSH);
		paintDc.SetPen(*wxTRANSPARENT_PEN);
		paintDc.DrawRectangle(GetClientSize());
	}
}


void SjProjectmGlCanvas::OnTimer(wxTimerEvent&)
{
	SJ_FORCE_IN_HERE_ONLY_ONCE

	if( !m_triedCreation && IsShownOnScreen() && s_theProjectmModule )
	{
		m_triedCreation = true;

		// create and select the gl context
		s_theProjectmModule->m_glContext = new wxGLContext(this);
		if( s_theProjectmModule->m_glContext == NULL )
			{ wxLogError("Cannot init projectM (context creation failed)."); return; }
		SetCurrent(*s_theProjectmModule->m_glContext);

		// vis. presets path
		wxString presetPath = SjTools::EnsureTrailingSlash(SjTools::GetGlobalAppDataDir());
		presetPath = SjTools::EnsureTrailingSlash(presetPath + "vis");

		if( g_debug )
		{
			wxLogInfo("Loading %s", presetPath.c_str());
		}

		// init projectM itself
		projectM::Settings s = {
			32,    // int meshX           -- Defaults from projectM::readConfig().
			24,    // int meshY           -- Defining the values this way will force
			35,    // int fps;            -- an error if the structure changes.
			512,   // int textureSize;
			512,   // int windowWidth;
			512,   // int windowHeight;
			presetPath.ToStdString(),    // std::string presetURL;
			"",    // std::string titleFontURL;
			"",    // std::string menuFontURL;
			10,    // int smoothPresetDuration;
			15,    // int presetDuration;
			10.0,  // float beatSensitivity;
			true,  // bool aspectCorrection;
			0.0,   // float easterEgg;
			true,  // bool shuffleEnabled;
			false, // bool softCutRatingsEnabled;
		};

		try {
			s_theProjectmModule->m_projectMobj = new projectM(s, projectM::FLAG_NONE);

				#ifdef __WXMSW__
					wxASSERT( _CrtCheckMemory() );
				#endif

			wxSize size = GetSize();
			s_theProjectmModule->m_projectMobj->projectM_resetGL(size.x, size.y);
		}
		catch(...) {
			s_theProjectmModule->m_projectMobj = NULL;
			wxLogError("Cannot init projectM (constructor failed).");
		}
	}

	// to set the frames, ths. like  globalPM->beatDetect->pcm->addPCM8( renderData.waveformData );
	// should be called, see .../itunes/iprojectM.cpp
	if( s_theProjectmModule && s_theProjectmModule->m_projectMobj 
	 && s_theProjectmModule->m_glCanvas && s_theProjectmModule->m_glContext )
	{
		g_mainFrame->m_player.GetVisData(m_bufferStart, m_sampleCount*SJ_WW_CH*SJ_WW_BYTERES, 0);

		#ifdef __WXMSW__
			wxASSERT( _CrtCheckMemory() );
		#endif
		
		// TODO: is this really correct?
		s_theProjectmModule->m_projectMobj->pcm()->addPCM16Data( (const short*)m_bufferStart, m_sampleCount );
		s_theProjectmModule->m_projectMobj->renderFrame();
		s_theProjectmModule->m_glCanvas->SwapBuffers();

		#ifdef __WXMSW__
			wxASSERT( _CrtCheckMemory() );
		#endif
	}
}


/*******************************************************************************
 * the module controlling the vis.
 ******************************************************************************/


SjProjectmModule::SjProjectmModule(SjInterfaceBase* interf)
	: SjVisRendererModule(interf)
{
	m_file              = "memory:projectm.lib";
	m_name              = _("Visualization");
	m_impl              = NULL;
	m_glCanvas          = NULL;
	m_glContext         = NULL;
	m_projectMobj       = NULL;
	s_theProjectmModule = this;
	m_sort              = 1; // start of list, defaukt is 1000
}


SjProjectmModule::~SjProjectmModule()
{
	s_theProjectmModule = NULL;
}


bool SjProjectmModule::Start(SjVisImpl* impl, bool justContinue)
{
	m_impl = impl;

	// create and show the window, if not yet done
	if( m_glCanvas != NULL )
		{ return false; }

	m_glCanvas = new SjProjectmGlCanvas(impl->GetWindow());
	if( m_glCanvas == NULL )
		{ wxLogError("Cannot init projectM (canvas creation failed)."); return false; }

	wxRect visRect = impl->GetRendererClientRect();
	m_glCanvas->SetSize(visRect);
	m_glCanvas->Show();

	// start timer; the real initialisation is done in the timer if IsShownOnScreen() is true
	m_glCanvas->m_timer.SetOwner(m_glCanvas, IDC_TIMER);
	m_glCanvas->m_timer.Start(SLEEP_MS, wxTIMER_CONTINUOUS);

	return true;
}


void SjProjectmModule::Stop()
{
	if( m_glCanvas ) {
		m_glCanvas->m_timer.Stop();
	}

	if( m_projectMobj )
	{
		try {
			delete m_projectMobj;
			m_projectMobj = NULL;
		}
		catch(...) {
		}
	}

	if( m_glCanvas ) {
		m_glCanvas->Destroy();
		m_glCanvas = NULL;
	}

	if( m_glContext ) {
		delete m_glContext;
		m_glContext = NULL;
	}

	g_mainFrame->Update();

	m_impl = NULL;
}


void SjProjectmModule::ReceiveMsg(int msg)
{
}


void SjProjectmModule::AddMenuOptions(SjMenu&)
{
}


void SjProjectmModule::OnMenuOption(int)
{
}


void SjProjectmModule::PleaseUpdateSize(SjVisImpl* impl)
{
	if( m_glCanvas )
	{
		wxRect visRect = impl->GetRendererClientRect();
		m_glCanvas->SetSize(visRect);

		if( m_projectMobj )
		{
			int width = visRect.GetWidth();
			int height = visRect.GetHeight();

			m_projectMobj->projectM_resetGL(width, height);
		}
	}
}


#endif // SJ_USE_PROJECTM
