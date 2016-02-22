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
 * File:    vis_module.cpp
 * Authors: Björn Petersen
 * Purpose: Video, Karaoke, Visualisations (vis=[vi]deo [s]creen)
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/vis/vis_module.h>
#include <sjmodules/vis/vis_window.h>
#include <sjmodules/vis/vis_frame.h>
#include <sjmodules/vis/vis_karaoke_module.h>
#include <sjmodules/vis/vis_vidout_module.h>
#include <wx/display.h>

#define IDMODMSG_VIS_FWD_SWITCH_RENDERER    IDMODMSG__VIS_MOD_PRIVATE_1__
#define IDMODMSG_VIS_FWD_CLOSE              IDMODMSG__VIS_MOD_PRIVATE_2__
#define IDMODMSG_VIS_FWD_SWITCH_BACK        IDMODMSG__VIS_MOD_PRIVATE_4__

#if SJ_USE_PROJECTM
	#define DEFAULT_VIS_FILE_NAME "memory:projectm.lib"
#else
	#define DEFAULT_VIS_FILE_NAME "memory:oscilloscope.lib"
#endif

#undef VIS_DEBUG_VIEW


/*******************************************************************************
 * Constructor & Co.
 ******************************************************************************/


SjVisModule* g_visModule = NULL;


SjVisModule::SjVisModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	m_file                      = "memory:vis.lib";
	m_name                      = _("Video screen");
	m_visOwnFrame               = NULL;
	m_visWindow                 = NULL;
	m_visWindowVisible          = false;
	m_visIsStarted              = false;
	m_modal                     = 0;
}


void SjVisModule::MoveVisAway()
{
	m_visWindow->SetSize(
		#ifdef VIS_DEBUG_VIEW
			0, 0,
		#else
			-10000, -10000,
		#endif
		64, 64);
}


bool SjVisModule::FirstLoad()
{
	g_visModule = this;
	m_visFlags = g_tools->m_config->Read("player/visflags", SJ_VIS_FLAGS_DEFAULT);

	// create the vis. window - the window is always needed as it is eg. a parent for videos;
	// if it is not visible, it is moved aside.
	wxString rectStr;
	if( g_tools->ReadFromCmdLineOrIni("visrect", rectStr) )
	{
		wxRect visRect;
		bool   visRectFullscreen;
		if( SjTools::ParseRectOrDisplayNumber(rectStr, visRect, visRectFullscreen) )
		{
			m_visOwnFrame = new SjVisFrame(NULL, // <-- <-- without this and with g_mainFrame instead of NULL as parent, the tastbar stays visible!
				wxPoint(visRect.x, visRect.y), wxSize(visRect.width, visRect.height),
				visRectFullscreen);
			if( m_visOwnFrame )
			{
				m_visOwnFrame->Show();
				if( g_mainFrame->IsAlwaysOnTop() ) {
					ShowVisAlwaysOnTop(true); // sync with the always-on-top-state of the main window; SjMainFrame::SetAlwaysOnTop() also calls SetVisAlwaysOnTop()
				}
			}
		}
	}

	m_visWindow = new SjVisWindow(m_visOwnFrame? (wxWindow*)m_visOwnFrame : (wxWindow*)g_mainFrame);
	if( !m_visWindow ) { return false; }

	MoveVisAway();
	m_visWindow->Show(); // sic! we hide the window by moving it out of view
	m_visWindowVisible = false;

	// the "video window handle" is needed even if the "video screen" is closed
	#if SJ_USE_VIDEO
	g_vidoutModule->Load();
	#endif
	return true;
}


void SjVisModule::LastUnload()
{
	SetCurrRenderer(NULL);
	#if SJ_USE_VIDEO
	g_vidoutModule->Unload();
	#endif
	CloseWindow__();

	if( m_visOwnFrame )
	{
		m_visOwnFrame->Destroy();
		m_visOwnFrame = NULL;
	}

	g_visModule = NULL;
}


void SjVisModule::WriteVisFlags()
{
	g_tools->m_config->Write("player/visflags", m_visFlags);
}


/*******************************************************************************
 * User Functions
 ******************************************************************************/


void SjVisModule::StartVis()
{
	if( IsVisStarted() )
		return;

	g_mainFrame->m_autoCtrl.m_stateStartVisTimestamp = SjTools::GetMsTicks(); // we forgot this in 2.10beta3, see http://www.silverjuke.net/forum/viewtopic.php?p=2994#2994
	OpenWindow__();
	if( m_visFlags&SJ_VIS_FLAGS_SWITCH_OVER_AUTOMATICALLY )
	{
		wxString desiredRenderer = GetDesiredRenderer();
		wxString realNextRenderer = GetRealNextRenderer(desiredRenderer);
		SjVisRendererModule* m = (SjVisRendererModule*)g_mainFrame->m_moduleSystem.FindModuleByFile(realNextRenderer);
		SetCurrRenderer(m? m : SJ_SET_LAST_SELECTED);
	}
	else
	{
		SetCurrRenderer(SJ_SET_LAST_SELECTED);
	}
}


void SjVisModule::StopVis()
{
	if( m_modal == 0 )
	{
		g_mainFrame->m_autoCtrl.m_stateStopVisTimestamp = SjTools::GetMsTicks(); // we forgot this in 2.10beta3, see http://www.silverjuke.net/forum/viewtopic.php?p=2994#2994

		SetCurrRenderer(NULL);
		CloseWindow__();
	}
}


/*******************************************************************************
 * Open / Close the Vis. Window
 ******************************************************************************/


bool SjVisModule::OpenWindow__()
{
	wxASSERT( wxThread::IsMain() );

	// if the window is already opened, this function simply does nothing
	if( m_visWindowVisible || g_mainFrame == NULL || g_visModule == NULL )
		return false;

	if( !m_visOwnFrame )
	{
		wxRect r;
		g_mainFrame->GetVisEmbedRect(&r, &m_visIsOverWorkspace);

        m_visWindow->SetSize(r);

		if( m_visIsOverWorkspace ) {
			g_mainFrame->MoveWorkspaceAway(true);
		}

		g_mainFrame->Update();
	}
	else
	{
		int windowW, windowH;
        m_visOwnFrame->GetSize(&windowW, &windowH);
        m_visWindow->SetSize(wxRect(0, 0, windowW, windowH));
	}

	m_visWindowVisible = true;
	return true;
}


bool SjVisModule::CloseWindow__()
{
	wxASSERT( wxThread::IsMain() );

	if( m_visWindow == NULL )
		return false;

	// move workspace back to view
	if( !m_visOwnFrame && m_visIsOverWorkspace )
	{
		g_mainFrame->MoveWorkspaceAway(false);
	}

	// hide the window
	m_visWindow->SetRenderer(NULL);

	MoveVisAway();
	m_visWindowVisible = false;

	return true;
}


/*******************************************************************************
 * Start / Stop a Vis.
 ******************************************************************************/


void SjVisModule::StopOrCloseRequest()
{
	g_mainFrame->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDMODMSG_VIS_FWD_CLOSE));
}


void SjVisModule::ShowVisAlwaysOnTop(bool set) const
{
	// this only affects non-embedded video screens using a separate top-level-window
	if( m_visOwnFrame )
	{
		long s = m_visOwnFrame->GetWindowStyle();
		if( set ) {
			m_visOwnFrame->SetWindowStyle(s | wxSTAY_ON_TOP);
		}
		else {
			m_visOwnFrame->SetWindowStyle(s & ~wxSTAY_ON_TOP);
		}
	}
}


wxString SjVisModule::GetDesiredRenderer()
{
	if( g_mainFrame )
	{
		// has the currently running title a video stream?
		if( g_mainFrame->m_player.IsVideoOnAir() )
		{
			return "memory:vidout.lib";
		}

		// has the currently playing track a karaoke file?
		wxString url = g_mainFrame->GetQueueUrl(-1);
		if( !url.IsEmpty() )
		{
			SjKaraokeMaster tempMaster;
			if( tempMaster.Init(url, ""/*artist not needed for testing*/, ""/*album not needed for testing*/) )
			{
				return "memory:karaoke.lib";
			}
		}
	}

	// the currently playing track is a "normal" music file
	return "";
}


wxString SjVisModule::GetRealNextRenderer(const wxString& desiredRenderer)
{
	SjVisRendererModule* currRenderer__ = GetCurrRenderer(); // may be NULL
	wxString nextRenderer, currRenderer = currRenderer__? currRenderer__->m_file : "";

	if( desiredRenderer.IsEmpty() )
	{
		// neither vidout nor karaoke desired, switch to a visualisation
		if( currRenderer == "" || currRenderer == "memory:vidout.lib" || currRenderer == "memory:karaoke.lib" )
		{
			nextRenderer = DEFAULT_VIS_FILE_NAME;
			wxString moduleName = g_tools->m_config->Read("player/vismodule", "");
			if( moduleName != "memory:vidout.lib" && moduleName != "memory:karaoke.lib" ) {
				SjModule* m = g_mainFrame->m_moduleSystem.FindModuleByFile(moduleName);
				if( m && m->m_type == SJ_MODULETYPE_VIS_RENDERER ) {
					nextRenderer = moduleName;
				}
			}
		}
	}
	else
	{
		// videout or karaoke desired, mark as next
		if( currRenderer != desiredRenderer )
			nextRenderer = desiredRenderer;
	}

	return nextRenderer; // may be empty if the renderer should not be changed
}


SjVisRendererModule* SjVisModule::GetCurrRenderer() const
{
	SjVisWindow* impl = GetVisWindow();
	if( impl )
	{
		return impl->GetRenderer();
	}

	return NULL;
}


void SjVisModule::AddVisData(const float* data, long bytes)
{
	SjVisRendererModule* renderer = GetCurrRenderer();
	if( renderer )
	{
		renderer->AddVisData(data, bytes);
	}
}


void SjVisModule::ReceiveMsg(int msg)
{
	if( msg == IDMODMSG_WINDOW_SIZED_MOVED )
	{
		if( m_visWindowVisible && !m_visOwnFrame )
		{
			wxRect r;
			g_mainFrame->GetVisEmbedRect(&r, &m_visIsOverWorkspace);
			m_visWindow->SetSize(r);
			g_mainFrame->MoveWorkspaceAway(m_visIsOverWorkspace); // the state may change on layout switches!
		}
	}
	else if( msg == IDMODMSG_TRACK_ON_AIR_CHANGED || msg == IDMODMSG_VIDEO_DETECTED )
	{
		// check if we should switch to another module
		SjSkinValue buttonState = g_mainFrame->GetSkinTargetValue(IDT_VIS_TOGGLE);
		buttonState.vmax = 0;

		wxString desiredRenderer = GetDesiredRenderer();

		if( m_visWindowVisible )
		{
			if( (m_visFlags&SJ_VIS_FLAGS_SWITCH_OVER_AUTOMATICALLY) )
			{
				wxString realNextRenderer = GetRealNextRenderer(desiredRenderer);
				if( !realNextRenderer.IsEmpty() )
				{
					// switch to the next renderer ... in a moment ...
					if( m_temp1str__.IsEmpty() )
					{
						m_temp1str__ = realNextRenderer;
						g_mainFrame->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDMODMSG_VIS_FWD_SWITCH_RENDERER));
					}
				}
			}
		}
		else if( !m_visIsStarted )
		{
			// the the vis. button blink!
			if( g_mainFrame->IsOpAvailable(SJ_OP_STARTVIS) && !desiredRenderer.IsEmpty() )
			{
				buttonState.vmax = SJ_VMAX_BLINK;
			}
		}

		g_mainFrame->SetSkinTargetValue(IDT_VIS_TOGGLE, buttonState);
	}
	else if( msg == IDMODMSG_VIS_FWD_SWITCH_RENDERER )
	{
		// ... we are here again
		SetCurrRenderer(SJ_SET_FROM_m_temp1str__);
	}
	else if( msg == IDMODMSG_VIS_FWD_CLOSE )
	{
		StopVis();
	}
}


void SjVisModule::SetCurrRenderer(SjVisRendererModule* m /*may be NULL for disable vis.*/)
{
	if( !m_visWindowVisible )
		return;

	// handle some special values
	if( m==SJ_SET_FROM_m_temp1str__ )
	{
		m = (SjVisRendererModule*)g_mainFrame->m_moduleSystem.FindModuleByFile(m_temp1str__);
		m_temp1str__.Clear();
		if( m == NULL )
			return;
	}
	else if( m==SJ_SET_LAST_SELECTED )
	{
		wxString moduleName = g_tools->m_config->Read("player/vismodule", "");
		m = (SjVisRendererModule*)g_mainFrame->m_moduleSystem.FindModuleByFile(moduleName);
		if( m == NULL || m->m_type != SJ_MODULETYPE_VIS_RENDERER )
		{
			m = (SjVisRendererModule*)g_mainFrame->m_moduleSystem.FindModuleByFile(DEFAULT_VIS_FILE_NAME);
			if( m == NULL )
				return; // this should not happen - either "memory:projectm.lib" or "memory:oscilloscope.lib" are always available
		}
	}

	// clear for safe re-usage under all circumstances
	if( m )
		m_temp1str__.Clear();

	// change our state
	m_visIsStarted = (m!=NULL);

	// let the window set the renderer
	m_visWindow->SetRenderer(m);

	// inform all others
	g_mainFrame->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDMODMSG_VIS_STATE_CHANGED));
}


void SjVisModule::UpdateVisMenu(SjMenu* visMenu)
{
	// function creates the video screen menu; the menu starts with a group of possible
	// video screens (Visualisation, Spectrum monitor, Karaoke prompt, Video output etc.)
	// and is continued by a number of options valid only for the selected screen.
	//
	// So, in contrast to the other menus, this menu is completely recreated whenever
	// a different visualisation is selected.

	if( g_mainFrame == NULL )
		{ return; }

	// get active and selected renderer
	SjVisRendererModule* currRenderer = GetCurrRenderer();

	// add renderer list
	visMenu->Clear();
	SjModuleList* moduleList = g_mainFrame->m_moduleSystem.GetModules(SJ_MODULETYPE_VIS_RENDERER);
	int i = 0;
#if 0
	SjModuleList::Node* moduleNode = moduleList->GetFirst();
#else
	SjModuleList::compatibility_iterator moduleNode = moduleList->GetFirst();
#endif
	while( moduleNode )
	{
		SjVisRendererModule* module = (SjVisRendererModule*)moduleNode->GetData();
		wxASSERT(module);

		visMenu->AppendRadioItem(IDO_VIS_STARTFIRST+i, module->m_name);
		visMenu->Check(IDO_VIS_STARTFIRST+i, (currRenderer==module));

		// next
		moduleNode = moduleNode->GetNext();
		i++;
	}

	visMenu->AppendCheckItem(IDO_VIS_STOP, _("Off"));
	visMenu->Check(IDO_VIS_STOP, (currRenderer==NULL));

	visMenu->Append(IDT_VIS_TOGGLE);

	// add renderer options
	visMenu->AppendSeparator();

	if( currRenderer )
	{
		currRenderer->AddMenuOptions(*visMenu);
	}

	SjMenu* submenu = new SjMenu(0);

	submenu->AppendCheckItem(IDO_VIS_AUTOSWITCHOVER, _("If appropriate, switch over automatically"));
	submenu->Check(IDO_VIS_AUTOSWITCHOVER, (m_visFlags&SJ_VIS_FLAGS_SWITCH_OVER_AUTOMATICALLY)!=0);

	submenu->AppendCheckItem(IDO_VIS_HALFSIZE, _("Half size"));
	submenu->Check(IDO_VIS_HALFSIZE, (m_visFlags & SJ_VIS_FLAGS_HALF_SIZE)!=0);
	submenu->Enable(IDO_VIS_HALFSIZE, (currRenderer!=NULL));

	visMenu->Append(-1, _("Further options"), submenu);
}



void SjVisModule::OnVisMenu(int id)
{
	if( m_visWindow == NULL ) return;

	// start an explicit video screen
	if( id>=IDO_VIS_STARTFIRST && id<=IDO_VIS_STARTLAST )
	{
		if( g_mainFrame->IsAllAvailable() )
		{
			// change the selected vis. renderer
			int i = 0;
			SjModuleList* moduleList = g_mainFrame->m_moduleSystem.GetModules(SJ_MODULETYPE_VIS_RENDERER);
#if 0
			SjModuleList::Node* moduleNode = moduleList->GetFirst();
#else
			SjModuleList::compatibility_iterator moduleNode = moduleList->GetFirst();
#endif
			while( moduleNode )
			{
				SjVisRendererModule* newRenderer = (SjVisRendererModule*)moduleNode->GetData();
				wxASSERT(newRenderer);
				if( IDO_VIS_STARTFIRST+i == id )
				{
					if( newRenderer != GetCurrRenderer() )
					{
						g_tools->m_config->Write("player/vismodule", newRenderer->PackFileName());
						if( !m_visWindowVisible ) {
							g_mainFrame->m_autoCtrl.m_stateStartVisTimestamp = SjTools::GetMsTicks(); // we forgot this in 2.10beta3, see http://www.silverjuke.net/forum/viewtopic.php?p=2994#2994
							OpenWindow__();
						}
						SetCurrRenderer(newRenderer); // UpdateVisMenu() implicitly called
					}
					else
					{
                        UpdateVisMenu(g_mainFrame->m_visMenu); // needed as otherwise items get deselected
					}
					break;
				}

				// next
				moduleNode = moduleNode->GetNext();
				i++;
			}
			return;
		}
		else
		{
			// no rights available, convert the commmand to a simple toggle command (may still be denied, see below)
			id = IDT_VIS_TOGGLE;
		}
	}
	// no else - we might have changed the ID above

	// other video screen commands
	if( id >= IDO_VIS_OPTIONFIRST && id <= IDO_VIS_OPTIONLAST )
	{
		// renderer options
		SjVisRendererModule* currRenderer = GetCurrRenderer();
		if( currRenderer )
		{
			currRenderer->OnMenuOption(id);
		}
	}
	else switch( id )
	{
		case IDT_VIS_TOGGLE:
			if(  g_mainFrame->IsOpAvailable(SJ_OP_STARTVIS)
			 || (IsOverWorkspace() && IsVisStarted()) )
			{
				if( IsVisStarted() )
				{
					StopVis();
				}
				else
				{
					StartVis();
				}
				// the menu bar is updated through IDMODMSG_VIS_STATE_CHANGED
			}
			return;

		case IDO_VIS_STOP:
			if(  g_mainFrame->IsOpAvailable(SJ_OP_STARTVIS)
			 || (IsOverWorkspace() && IsVisStarted()) )
			{
				if( IsVisStarted() )
				{
					StopVis();
				}
				else
				{
					UpdateVisMenu(g_mainFrame->m_visMenu); // needed as otherwise items get deselected
				}
			}
			return;

		case IDO_VIS_HALFSIZE:
			if( g_mainFrame->IsAllAvailable() && m_visWindowVisible )
			{
				SjTools::ToggleFlag(m_visFlags, SJ_VIS_FLAGS_HALF_SIZE);
				WriteVisFlags();
				m_visWindow->CalcPositions();
				m_visWindow->Refresh();
			}
			return;

		case IDO_VIS_AUTOSWITCHOVER:
			if( g_mainFrame->IsAllAvailable() )
			{
				SjTools::ToggleFlag(m_visFlags, SJ_VIS_FLAGS_SWITCH_OVER_AUTOMATICALLY);
				WriteVisFlags();
			}
			return;
	}
}

