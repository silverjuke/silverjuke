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

#define IDC_GO_TO_PREV_PRESET   (IDO_VIS_OPTIONFIRST+1)
#define IDC_GO_TO_NEXT_PRESET   (IDO_VIS_OPTIONFIRST+2)
#define IDC_GO_TO_RANDOM_PRESET (IDO_VIS_OPTIONFIRST+3)
#define IDC_SHOW_PRESET_NAME    (IDO_VIS_OPTIONFIRST+4)
#define IDC_DO_SHUFFLE          (IDO_VIS_OPTIONFIRST+5)
#define IDC_DURATIONS_FIRST     (IDO_VIS_OPTIONFIRST+6)
#define IDC_DURATIONS_LAST      (IDO_VIS_OPTIONFIRST+56)


class SjProjectmGlCanvas : public wxGLCanvas
{
public:
				SjProjectmGlCanvas (wxWindow* parent);

	bool        ImplOk() const { return (s_theProjectmModule!=NULL&&s_theProjectmModule->m_impl); }

	void        OnEraseBackground   (wxEraseEvent& e);
	void        OnPaint             (wxPaintEvent& e);

	void        OnMouseLeftDown     (wxMouseEvent& e)   { if(ImplOk()) s_theProjectmModule->m_impl->OnMouseLeftDown(e); }
	void        OnMouseLeftUp       (wxMouseEvent& e)   { if(ImplOk()) s_theProjectmModule->m_impl->OnMouseLeftUp(e); }
	void        OnMouseRightUp      (wxContextMenuEvent& e)   { if(ImplOk()) s_theProjectmModule->m_impl->OnMouseRightUp(e); }
	void        OnMouseLeftDClick   (wxMouseEvent& e)   { if(ImplOk()) s_theProjectmModule->m_impl->OnMouseLeftDClick(e); }

	void        OnTimer             (wxTimerEvent&);

	wxTimer     m_timer;
    bool        m_triedCreation;

	DECLARE_EVENT_TABLE ();
};


BEGIN_EVENT_TABLE(SjProjectmGlCanvas, wxGLCanvas)
	EVT_LEFT_DOWN       (                       SjProjectmGlCanvas::OnMouseLeftDown     )
	EVT_LEFT_UP         (                       SjProjectmGlCanvas::OnMouseLeftUp       )
	EVT_LEFT_DCLICK     (                       SjProjectmGlCanvas::OnMouseLeftDClick   )
	EVT_CONTEXT_MENU    (                       SjProjectmGlCanvas::OnMouseRightUp      )

	EVT_ERASE_BACKGROUND(                       SjProjectmGlCanvas::OnEraseBackground   )
	EVT_PAINT           (                       SjProjectmGlCanvas::OnPaint             )

	EVT_TIMER           (IDC_TIMER,             SjProjectmGlCanvas::OnTimer             )
END_EVENT_TABLE()


SjProjectmGlCanvas::SjProjectmGlCanvas(wxWindow* parent)
	: wxGLCanvas(parent, wxID_ANY, NULL, wxDefaultPosition, wxDefaultSize, wxCLIP_CHILDREN)
{
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
		wxString presetPath = g_tools->m_config->Read("main/prjmdir", "");
		if( presetPath == "" ) {
			presetPath = SjTools::EnsureTrailingSlash(SjTools::GetGlobalAppDataDir()) + "vis";
		}
		presetPath = SjTools::EnsureTrailingSlash(presetPath);

		if( g_debug )
		{
			wxLogInfo("Loading %s", presetPath.c_str());
		}

		// init projectM itself
		float beatSensitivity = (float)g_tools->m_config->Read("main/prjmbeatsense", 20);
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
			s_theProjectmModule->m_prjmDuration,    // int presetDuration;
			beatSensitivity,  // float beatSensitivity;
			true,  // bool aspectCorrection;
			0.0,   // float easterEgg;
			(s_theProjectmModule->m_prjmFlags&SJ_PRJMFLAGS_SHUFFLE)!=0,  // bool shuffleEnabled;
			false, // bool softCutRatingsEnabled;
		};

		try {
			s_theProjectmModule->m_projectMobj = new projectM(s, projectM::FLAG_NONE);

			#ifdef __WXMSW__
				wxASSERT( _CrtCheckMemory() );
			#endif

			// by default, after creation the "Idle preset with the projectM" is selected until we fade to the next one;
			// as this gets boring after little time, we switch over to another preset.
			if( (s_theProjectmModule->m_prjmFlags&SJ_PRJMFLAGS_SHUFFLE)
			 && (s_theProjectmModule->m_prjmDuration<=SJ_PRJMDURATION_STD) )
			{
				s_theProjectmModule->m_projectMobj->selectRandom(true /*hardCut*/); // init by random if (1) random is enabled and (2) a longer preset time is choosen
			}
			else
			{
				long desiredIndex = g_tools->m_config->Read("main/prjmindex", (long)-1);
				if( desiredIndex >= 0 && desiredIndex < s_theProjectmModule->m_projectMobj->getPlaylistSize() ) {
					s_theProjectmModule->m_projectMobj->selectPreset(desiredIndex, true /*hardCut*/);
				}
				else {
					s_theProjectmModule->m_projectMobj->selectRandom(true /*hardCut*/);
				}
			}

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
		#ifdef __WXMSW__
			wxASSERT( _CrtCheckMemory() );
		#endif

		//SetCurrent(*s_theProjectmModule->m_glContext); -- this is only needed if we use several GL contexts at the same in the same thread

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


bool SjProjectmModule::FirstLoad()
{
	m_prjmFlags    = g_tools->m_config->Read("main/prjmflags",   SJ_PRJMFLAGS_DEFAULT);
	m_prjmDuration = g_tools->m_config->Read("main/prjmseconds", SJ_PRJMDURATION_STD);
	if( m_prjmDuration < SJ_PRJMDURATION_MIN ) { m_prjmDuration = SJ_PRJMDURATION_MIN; }
	if( m_prjmDuration > SJ_PRJMDURATION_MAX ) { m_prjmDuration = SJ_PRJMDURATION_MAX; }
	return true;
}


void SjProjectmModule::WritePrjmConfig()
{
	g_tools->m_config->Write("main/prjmflags", m_prjmFlags);
	g_tools->m_config->Write("main/prjmseconds", m_prjmDuration);

	unsigned int currIndex = -1;
	if( !m_projectMobj || !m_projectMobj->selectedPresetIndex(currIndex) ) { currIndex = -1; }
	g_tools->m_config->Write("main/prjmindex", (long)currIndex);
}


bool SjProjectmModule::Start(SjVisWindow* impl)
{
	m_impl = impl;

	// create and show the window, if not yet done
	if( m_glCanvas != NULL )
		{ return false; }

	m_glCanvas = new SjProjectmGlCanvas(impl);
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

	WritePrjmConfig();

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


static const int s_durations[] = { 5, 10, 15, 20, 30, 45, 60, 90, 120, 180, 240, 300, /*420, 600, 900, 1800, 3600,*/  0/*mark end*/ }; // do not use too long preset times; not sure, if this always works. Moreover, "bad" presets should not take too long. The user can set longer presettimes in the INI.
static wxString s_duration_text(int i)
{
	wxString text;
	if( s_durations[i] >= 120 ) {
		text = wxString::Format("%i ", s_durations[i]/60) + _("minutes");
	}
	else {
		text = wxString::Format("%i ", s_durations[i]) + _("seconds");
	}
	if( s_durations[i] == SJ_PRJMDURATION_STD ) { text += " (" + _("Default") + ")"; }
	return text;
}


void SjProjectmModule::AddMenuOptions(SjMenu& m)
{
	m.Append(IDC_SHOW_PRESET_NAME, _("Preset info"));
	m.Append(IDC_GO_TO_PREV_PRESET, _("Go to previous preset"));
	m.Append(IDC_GO_TO_NEXT_PRESET, _("Go to next preset"));
	m.Append(IDC_GO_TO_RANDOM_PRESET, _("Go to random preset"));

	m.AppendSeparator();

	m.AppendCheckItem(IDC_DO_SHUFFLE, _("Preset shuffle"));
	m.Check(IDC_DO_SHUFFLE, (m_prjmFlags&SJ_PRJMFLAGS_SHUFFLE)!=0);

	SjMenu* submenu = new SjMenu(0);
		for( int i = 0; s_durations[i]; i++ ) {
			submenu->AppendCheckItem(IDC_DURATIONS_FIRST+i, s_duration_text(i));
			if( s_durations[i] == m_prjmDuration ) { submenu->Check(IDC_DURATIONS_FIRST+i, true); }
		}
	m.Append(wxID_ANY, _("Preset duration"), submenu);
}


void SjProjectmModule::OnMenuOption(int id)
{
	if( !m_projectMobj ) { return; }

	bool showNewPresetName = false;
	if( id >= IDC_DURATIONS_FIRST && id <= IDC_DURATIONS_LAST )
	{
		for( int i = 0; s_durations[i]; i++ ) {
			if( i==(id-IDC_DURATIONS_FIRST) ) {
				m_prjmDuration = s_durations[i];
				WritePrjmConfig();
				m_projectMobj->changePresetDuration(m_prjmDuration);
				g_mainFrame->SetDisplayMsg(_("Preset duration") + ": " + s_duration_text(i), SDM_STATE_CHANGE_MS);
			}
		}
	}
	else switch( id )
	{
		case IDT_WORKSPACE_KEY_LEFT:
		case IDC_GO_TO_PREV_PRESET:
			m_projectMobj->selectPrevious(true /*hard cut*/);
			showNewPresetName = true;
			break;

		case IDT_WORKSPACE_KEY_RIGHT:
		case IDC_GO_TO_NEXT_PRESET:
			m_projectMobj->selectNext(true /*hard cut*/);
			showNewPresetName = true;
			break;

		case IDT_WORKSPACE_KEY_DOWN:
		case IDC_GO_TO_RANDOM_PRESET:
			m_projectMobj->selectRandom(true /*hard cut*/);
			showNewPresetName = true;
			break;

		case IDT_WORKSPACE_KEY_UP:
		case IDC_SHOW_PRESET_NAME:
			showNewPresetName = true;
			break;

		case IDC_DO_SHUFFLE:
			SjTools::ToggleFlag(m_prjmFlags, SJ_PRJMFLAGS_SHUFFLE);
			WritePrjmConfig();
			m_projectMobj->setShuffleEnabled((m_prjmFlags&SJ_PRJMFLAGS_SHUFFLE)!=0);
			// do a change right now to reflect the new state
			if( m_prjmFlags&SJ_PRJMFLAGS_SHUFFLE ) {
				m_projectMobj->selectRandom(true /*hard cut*/);
				g_mainFrame->SetDisplayMsg(_("Preset shuffle") + ": " + _("On"), SDM_STATE_CHANGE_MS);
			}
			else {
				g_mainFrame->SetDisplayMsg(_("Preset shuffle") + ": " + _("Off"), SDM_STATE_CHANGE_MS);
			}
			break;
	}

	if( showNewPresetName )
	{
		wxString msg;
		unsigned int newIndex, presetCount = m_projectMobj->getPlaylistSize();
		if( presetCount > 0 && m_projectMobj->selectedPresetIndex(newIndex) )
		{
            msg = wxString::Format("%i/%i: ", (int)(newIndex+1), (int)presetCount);
            std::string name = m_projectMobj->getPresetName(newIndex);
            msg += name;
            msg.Replace(".milk", "");
		}
		else
		{
			msg = "0/0: " + _("No presets found.");
		}
		g_mainFrame->SetDisplayMsg(msg);
	}
}


void SjProjectmModule::PleaseUpdateSize(SjVisWindow* impl)
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


void SjProjectmModule::AddVisData(const float* buffer, long bytes)
{
	if( m_projectMobj )
	{
		m_projectMobj->pcm()->addPCMfloat(buffer, bytes/sizeof(float));
	}
}


#endif // SJ_USE_PROJECTM
