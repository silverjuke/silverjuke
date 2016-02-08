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
#include <sjmodules/vis/vis_karaoke_module.h>
#include <sjmodules/vis/vis_vidout_module.h>
#include <wx/display.h>

#define IDMODMSG_VIS_FWD_SWITCH_RENDERER    IDMODMSG__VIS_MOD_PRIVATE_1__
#define IDMODMSG_VIS_FWD_CLOSE              IDMODMSG__VIS_MOD_PRIVATE_2__
#define IDMODMSG_VIS_FWD_ATTACH_DETACH      IDMODMSG__VIS_MOD_PRIVATE_3__
#define IDMODMSG_VIS_FWD_SWITCH_BACK        IDMODMSG__VIS_MOD_PRIVATE_4__

#if SJ_USE_PROJECTM
	#define DEFAULT_VIS_FILE_NAME "memory:projectm.lib"
#else
	#define DEFAULT_VIS_FILE_NAME "memory:oscilloscope.lib"
#endif


/*******************************************************************************
 * Constructor & Co.
 ******************************************************************************/


SjVisModule* g_visModule = NULL;


SjVisModule::SjVisModule(SjInterfaceBase* interf)
	: SjCommonModule(interf)
{
	m_file                      = wxT("memory:vis.lib");
	m_name                      = _("Video screen");
	m_visWindow                 = NULL;
	m_visIsStarted              = false;
	m_explicitlyPrepared        = false;
	m_inAttachDetach            = false;
	m_modal                     = 0;
}


bool SjVisModule::FirstLoad()
{
	g_visModule = this;
	m_visFlags = g_tools->m_config->Read(wxT("player/visflags"), SJ_VIS_FLAGS_DEFAULT);
	g_vidoutModule->Load(); // the "video window handle" is needed even if the "video screen" is closed
	return true;
}


void SjVisModule::LastUnload()
{
	g_vidoutModule->Unload();
	UnprepareWindow();
	g_visModule = NULL;
}


void SjVisModule::WriteVisFlags()
{
	g_tools->m_config->Write(wxT("player/visflags"), m_visFlags);
}


/*******************************************************************************
 * User Functions
 ******************************************************************************/


void SjVisModule::PrepareWindow(int fullscreenDisplay, const wxRect* fullscreenVisRect__)
{
	if( OpenWindow__(fullscreenDisplay, fullscreenVisRect__) )
	{
		m_explicitlyPrepared = true;
	}
}


void SjVisModule::UnprepareWindow()
{
	SetCurrRenderer(NULL);
	CloseWindow__();
	m_explicitlyPrepared = false;
}


void SjVisModule::StartVis()
{
	g_mainFrame->m_autoCtrl.m_stateStartVisTimestamp = SjTools::GetMsTicks(); // we forgot this in 2.10beta3, see http://www.silverjuke.net/forum/viewtopic.php?p=2994#2994
	OpenWindow__(-1, NULL);
	if( m_visFlags&SJ_VIS_FLAGS_SWITCH_OVER_AUTOMATICALLY )
	{
		wxString desiredRenderer = GetDesiredRenderer();
		wxString realNextRenderer = GetRealNextRenderer(desiredRenderer);
		SjVisRendererModule* m = (SjVisRendererModule*)g_mainFrame->m_moduleSystem.FindModuleByFile(realNextRenderer);
		wxASSERT( m );
		SetCurrRenderer(m);
	}
	else
	{
		SetCurrRenderer(SJ_SET_LAST_SELECTED, false); // false=just continue
	}
}


void SjVisModule::StopVis()
{
	if( m_modal == 0 )
	{
		g_mainFrame->m_autoCtrl.m_stateStopVisTimestamp = SjTools::GetMsTicks(); // we forgot this in 2.10beta3, see http://www.silverjuke.net/forum/viewtopic.php?p=2994#2994

		SetCurrRenderer(NULL);
		if( !m_explicitlyPrepared )
			CloseWindow__();
	}
}


/*******************************************************************************
 * Open / Close the Vis. Window
 ******************************************************************************/


bool SjVisModule::OpenWindow__(int fullscreenDisplay, const wxRect* fullscreenVisRect__)
{
	wxASSERT( wxThread::IsMain() );

	// if the window is already opened, this function simply does nothing
	if( m_visWindow || g_mainFrame == NULL || g_visModule == NULL )
		return false;

	bool    createFullScreen = false;
	long    createStyle = wxCLIP_CHILDREN | wxFULL_REPAINT_ON_RESIZE;
	wxRect  createRect;
	if( fullscreenDisplay == -1 )
	{
		// open the vis. window embedded or floating
		// ---------------------------------------------------------------------------------------------

		wxRect defaultClientRect;
		bool   defaultClientRectValid = g_mainFrame->GetVisEmbedRect(&defaultClientRect, &m_visIsOverWorkspace);

		m_visState = SJ_VIS_EMBEDDED;
		if( !g_mainFrame->IsKioskStarted() )
			m_visState = (m_visFlags & SJ_VIS_FLAGS_EMBEDDED)? SJ_VIS_EMBEDDED : SJ_VIS_FLOATING;

		if( m_visState == SJ_VIS_EMBEDDED && !defaultClientRectValid )
		{
			if( g_mainFrame->IsKioskStarted() )
				return false; // in kiosk mode we should not allow floating windows!
			m_visState = SJ_VIS_FLOATING;
		}

		if( m_visState == SJ_VIS_EMBEDDED )
		{
			// create embedded
			// -----------------------------------------------------------------------------------------

			wxASSERT( defaultClientRectValid );

			createRect = defaultClientRect;
		}
		else
		{
			// create floating
			// -----------------------------------------------------------------------------------------

			wxASSERT( !g_mainFrame->IsKioskStarted() );

			createStyle |= (wxDEFAULT_FRAME_STYLE & ~wxMINIMIZE_BOX);

			bool setDefault = false;
			createRect = g_tools->ReadRect(wxT("player/visrect"));
			if( createRect.width == 0 || createRect.height == 0 )
			{
				setDefault = true;
			}
			else
			{
				wxRect testRect = createRect;
				SjDialog::EnsureRectDisplayVisibility(testRect);
				if( testRect != createRect )
					setDefault = true;
			}

			if( setDefault )
			{
				createRect = defaultClientRect;
				g_mainFrame->ClientToScreen(&createRect.x, &createRect.y);
				createRect.x += 60;
				createRect.y += 60;
				if( createRect.width  < 320 ) createRect.width  = 320;
				if( createRect.height < 200 ) createRect.height = 200;
			}
		}
	}
	else
	{
		// open the vis. window as a fullscreen; optionally, we use only a part of the screen
		// ---------------------------------------------------------------------------------------------

		m_visState = SJ_VIS_OWNSCREEN;
		createStyle |= wxFRAME_NO_TASKBAR;
		createStyle |= wxSTAY_ON_TOP; // <-- without this and with g_mainFrame instead of NULL as parent, the tastbar stays visible!
		if( fullscreenVisRect__ )
		{
			createRect = *fullscreenVisRect__;
		}
		else
		{
			wxDisplay display(fullscreenDisplay);
			if( !display.IsOk() )
				return false;
			createRect = display.GetGeometry();
			createFullScreen = true;
		}
	}

	// finally, create the window
	// ---------------------------------------------------------------------------------------------

	if( m_visState == SJ_VIS_EMBEDDED )
	{
		SjVisEmbed* visEmbed = new SjVisEmbed(g_mainFrame, wxID_ANY,
		                                      wxPoint(createRect.x, createRect.y), wxSize(createRect.width, createRect.height),
		                                      createStyle);
		visEmbed->InitImpl();

		// show the control
		visEmbed->Show();
		if( m_visIsOverWorkspace )
			g_mainFrame->MoveWorkspaceAway(true);
		g_mainFrame->Update();

		// done with the control creation
		m_visWindow = visEmbed;
	}
	else
	{
		SjVisFrame* visFrame = new SjVisFrame(createFullScreen? NULL : g_mainFrame, // <-- <-- without this and with g_mainFrame instead of NULL as parent, the tastbar stays visible!
		                                      wxID_ANY,
		                                      m_name,
		                                      wxPoint(createRect.x, createRect.y), wxSize(createRect.width, createRect.height),
		                                      createStyle);
		visFrame->InitImpl();

		// show the frame
		if( createFullScreen )
		{
			visFrame->ShowFullScreen(true);
		}
		else
		{
			visFrame->SetIcons(g_mainFrame->m_iconBundle);
			visFrame->Show();
		}

		// done with the frame creation
		m_visWindow = visFrame;
	}

	return true;
}


bool SjVisModule::CloseWindow__()
{
	wxASSERT( wxThread::IsMain() );

	if( m_visWindow == NULL )
		return false;

	if( m_visState == SJ_VIS_EMBEDDED && m_visIsOverWorkspace )
		g_mainFrame->MoveWorkspaceAway(false);

	// save position
	if( m_visState == SJ_VIS_FLOATING && !((SjVisFrame*)m_visWindow)->IsMaximized() )
	{
		wxRect r = m_visWindow->GetRect();
		g_tools->WriteRect(wxT("player/visrect"), r);
	}

	// hide the window
	m_visWindow->Hide();
	m_visWindow->Destroy();
	m_visWindow = NULL;

	return true;
}


/*******************************************************************************
 * Start / Stop a Vis.
 ******************************************************************************/


void SjVisModule::StopOrCloseRequest()
{
	g_mainFrame->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDMODMSG_VIS_FWD_CLOSE));
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
	wxString nextRenderer, currRenderer = currRenderer__? currRenderer__->m_file : wxT("");

	if( desiredRenderer.IsEmpty() )
	{
		// neither vidout nor karaoke desired, switch to a visualisation
		if( currRenderer == wxT("") || currRenderer == wxT("memory:vidout.lib") || currRenderer == wxT("memory:karaoke.lib") )
		{
			nextRenderer = DEFAULT_VIS_FILE_NAME;
			wxString moduleName = g_tools->m_config->Read(wxT("player/vismodule"), wxT(""));
			if( moduleName != wxT("memory:vidout.lib") && moduleName != wxT("memory:karaoke.lib") ) {
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
	SjVisImpl* impl = GetVisImpl();
	if( impl )
	{
		return impl->GetRenderer();
	}

	return NULL;
}


SjVisImpl* SjVisModule::GetVisImpl() const
{
	if( m_visWindow )
	{
		if( m_visState == SJ_VIS_EMBEDDED )
			return &(((SjVisEmbed*)m_visWindow)->m_impl);
		else
			return &(((SjVisFrame*)m_visWindow)->m_impl);
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
		if( m_visWindow && m_visState == SJ_VIS_EMBEDDED )
		{
			wxRect r;
			bool wasOverWorkspace = m_visIsOverWorkspace;
			if( g_mainFrame->GetVisEmbedRect(&r, &m_visIsOverWorkspace) )
			{
				m_visWindow->SetSize(r);
				g_mainFrame->MoveWorkspaceAway(m_visIsOverWorkspace); // the state may change on layout switches!
			}
			else
			{
				StopOrCloseRequest();
				if( wasOverWorkspace )
					g_mainFrame->MoveWorkspaceAway(false); // added in 2.53beta1, see http://www.silverjuke.net/forum/topic-2438.html
			}
		}
	}
	else if( msg == IDMODMSG_TRACK_ON_AIR_CHANGED )
	{
		// check if we should switch to another module
		SjSkinValue buttonState = g_mainFrame->GetSkinTargetValue(IDT_VIS_TOGGLE);
		buttonState.vmax = 0;

		wxString desiredRenderer = GetDesiredRenderer();

		if( m_visWindow )
		{
			if( !m_inAttachDetach && (m_visFlags&SJ_VIS_FLAGS_SWITCH_OVER_AUTOMATICALLY) )
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
		if( !m_inAttachDetach )
		{
			SetCurrRenderer(SJ_SET_FROM_m_temp1str__);
		}
	}
	else if( msg == IDMODMSG_VIS_FWD_ATTACH_DETACH )
	{
		if( m_inAttachDetach )
		{
			StopVis();
			OpenWindow__(-1, NULL);
			SetCurrRenderer(SJ_SET_FROM_m_temp1str__, false /*just continue*/);

			m_inAttachDetach = false;
		}
	}
	else if( msg == IDMODMSG_VIS_FWD_CLOSE )
	{
		if( !m_inAttachDetach )
		{
			StopVis();
		}
	}
}


void SjVisModule::SetCurrRenderer(SjVisRendererModule* m /*may be NULL for disable vis.*/, bool justContinue)
{
	if( m_visWindow == NULL )
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
		wxString moduleName = g_tools->m_config->Read(wxT("player/vismodule"), wxT(""));
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
	SjVisImpl* impl = GetVisImpl();
	if( impl )
	{
		impl->SetRenderer(m, justContinue);
	}

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
	SjModuleList::Node* moduleNode = moduleList->GetFirst();
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

	submenu->AppendCheckItem(IDO_VIS_EMBEDWINDOW, _("Attached window"));
	submenu->Check(IDO_VIS_EMBEDWINDOW, IsEmbedded());
	submenu->Enable(IDO_VIS_EMBEDWINDOW, (currRenderer!=NULL) && !g_mainFrame->IsKioskStarted() && g_mainFrame->GetVisEmbedRect());

	submenu->AppendCheckItem(IDO_VIS_HALFSIZE, _("Half size"));
	submenu->Check(IDO_VIS_HALFSIZE, (m_visFlags & SJ_VIS_FLAGS_HALF_SIZE)!=0);
	submenu->Enable(IDO_VIS_HALFSIZE, (currRenderer!=NULL));

	visMenu->Append(-1, _("Further options"), submenu);
}



void SjVisModule::OnVisMenu(int id)
{
	SjVisImpl* impl = GetVisImpl();

	// start an explicit video screen
	if( id>=IDO_VIS_STARTFIRST && id<=IDO_VIS_STARTLAST )
	{
		if( g_mainFrame->IsAllAvailable() )
		{
			// change the selected vis. renderer
			int i = 0;
			SjModuleList* moduleList = g_mainFrame->m_moduleSystem.GetModules(SJ_MODULETYPE_VIS_RENDERER);
			SjModuleList::Node* moduleNode = moduleList->GetFirst();
			while( moduleNode )
			{
				SjVisRendererModule* newRenderer = (SjVisRendererModule*)moduleNode->GetData();
				wxASSERT(newRenderer);
				if( IDO_VIS_STARTFIRST+i == id )
				{
					if( newRenderer != GetCurrRenderer() )
					{
						g_tools->m_config->Write(wxT("player/vismodule"), newRenderer->PackFileName());
						if( impl==NULL ) {
							g_mainFrame->m_autoCtrl.m_stateStartVisTimestamp = SjTools::GetMsTicks(); // we forgot this in 2.10beta3, see http://www.silverjuke.net/forum/viewtopic.php?p=2994#2994
							OpenWindow__(-1, NULL);
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
			if( g_mainFrame->IsAllAvailable() && impl )
			{
				SjTools::ToggleFlag(m_visFlags, SJ_VIS_FLAGS_HALF_SIZE);
				WriteVisFlags();
				impl->CalcPositions();
				impl->m_thisWindow->Refresh();
			}
			return;

		case IDO_VIS_AUTOSWITCHOVER:
			if( g_mainFrame->IsAllAvailable() )
			{
				SjTools::ToggleFlag(m_visFlags, SJ_VIS_FLAGS_SWITCH_OVER_AUTOMATICALLY);
				WriteVisFlags();
			}
			return;

		case IDO_VIS_EMBEDWINDOW:
			if( g_mainFrame->IsAllAvailable() && !g_mainFrame->IsKioskStarted() && !m_inAttachDetach )
			{
				bool newEmbedded = !IsEmbedded();
				if( newEmbedded && !g_mainFrame->GetVisEmbedRect() )
					return; // can't embed now

				m_inAttachDetach = true;

				SjVisRendererModule* m = GetCurrRenderer();
				if( m )
					m_temp1str__ = m->PackFileName();

				SetCurrRenderer(NULL);

				SjTools::SetFlag(m_visFlags, SJ_VIS_FLAGS_EMBEDDED, newEmbedded);
				WriteVisFlags();

				g_mainFrame->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDMODMSG_VIS_FWD_ATTACH_DETACH));
			}
			return;
	}
}

