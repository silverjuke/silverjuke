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
 * File:    mainapp.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke main application class
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/skinml.h>
#include <sjtools/http.h>
#include <sjtools/gcalloc.h>
#include <sjtools/console.h>
#include <sjmodules/kiosk/kiosk.h>
#include <sjmodules/vis/vis_module.h>
#include <tagger/tg_a_tagger_frontend.h>
#include <see_dom/sj_see.h>
#include <wx/ipc.h>
#include <wx/display.h>
#include <wx/snglinst.h>
#include <wx/cmdline.h>
#include <wx/tokenzr.h>
#include <wx/socket.h>


/*******************************************************************************
 * SjClient/SjServer
 ******************************************************************************/


class SjConnection : public wxConnection
{
public:
	SjConnection(bool isServer)
		: wxConnection()
	{
		m_isServer = isServer;
	}

	bool OnExecute(const wxString& topic, const void*, size_t, wxIPCFormat format);

	bool OnDisconnect()
	{
		if( m_isServer )
		{
			delete this;
		}
		return TRUE;
	}

private:
	bool            m_isServer;
	void            Raise               ();
};


void SjConnection::Raise()
{
	wxASSERT( g_mainFrame );
	if( g_mainFrame )
	{
		g_mainFrame->Show();
		g_mainFrame->Iconize(FALSE);
		g_mainFrame->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDMODMSG_WINDOW_UNICONIZED));
		g_mainFrame->Raise();
	}
}


bool SjConnection::OnExecute(const wxString& topic, const void* data_, size_t size_, wxIPCFormat format_)
{
	// can we handle the request?
	if( SjMainApp::IsInShutdown()
	 || g_mainFrame ==NULL
	 || (SjMainFrame*)wxWindow::FindWindowById(IDC_MAINFRAME) == NULL )
	{
		return FALSE;
	}

	// get the data
	wxArrayString data;
	if( data_ )
	{
		wxStringTokenizer tkz(GetTextFromData(data_, size_, format_), "\n\r", wxTOKEN_RET_EMPTY_ALL);
		while( tkz.HasMoreTokens() )
		{
			wxString token = tkz.GetNextToken();
			token.Trim(TRUE);
			token.Trim(FALSE);
			if( !token.IsEmpty() )
			{
				data.Add(token);
			}
		}
	}

	// execute the given command
	if( topic==wxT("open") && data.GetCount() )
	{
		Raise();
		g_mainFrame->OpenFiles(data, SJ_OPENFILES_PLAY);
	}
	else if( topic==wxT("enqueue") && data.GetCount() )
	{
		g_mainFrame->OpenFiles(data, SJ_OPENFILES_ENQUEUE);
	}
	else if( topic==wxT("play") )
	{
		g_mainFrame->Play();
	}
	else if( topic==wxT("pause") )
	{
		g_mainFrame->Pause();
	}
	else if( topic==wxT("toggle") )
	{
		g_mainFrame->PlayOrPause(false /*no fade to play/pause*/);
	}
	else if( topic==wxT("togglevis") )
	{
		if( g_visModule )
		{
			if( g_visModule->IsVisStarted() )
				g_visModule->StopVis();
			else
				g_visModule->StartVis();
		}
	}
	else if( topic==wxT("volup") || topic==wxT("voldown") )
	{
		g_mainFrame->SetRelMainVol(topic==wxT("volup")? +8 : -8);
		g_mainFrame->SetSkinTargetValue(IDT_MAIN_VOL_MUTE, g_mainFrame->m_player.GetMainVolMute()? 1 : 0);
	}
	else if( topic==wxT("prev") )
	{
		g_mainFrame->GotoPrev(false /*no fade to next*/);
	}
	else if( topic==wxT("next") )
	{
		g_mainFrame->GotoNextRegardAP(false /*no fade to next*/);
	}
	else if( topic==wxT("fadetonext") )
	{
		g_mainFrame->GotoNextRegardAP(true /*fade to next*/);
	}
	else if( topic==wxT("kiosk") )
	{
		Raise();
		if( g_kioskModule ) g_kioskModule->StartRequest(TRUE);
	}
	else if( topic==wxT("addcredit") || topic==wxT("setcredit") )
	{
		if( data.GetCount() )
		{
			wxString data0(data[0]);            // be graceful and also allow the number in quotes -
			data0.Replace(wxT("\""), wxT(""));  // this is esp. needed for some clients who will put the
			data0.Replace(wxT("'"), wxT(""));   // data into a binary form otherwise.

			long addCredit = 0;
			if( !data0.ToLong(&addCredit, 10) ) { addCredit = 0; }
			if( g_kioskModule )
			{
				g_kioskModule->m_creditBase.AddCredit(addCredit,
				                                      SJ_ADDCREDIT_FROM_DDE |
				                                      (topic==wxT("setcredit")? SJ_ADDCREDIT_SET_TO_NULL_BEFORE_ADD : 0)
				                                     );
			}
		}
	}
	else if( topic=="displaymsg" ) // this is mainly for testing IPC
	{
		if( data.GetCount() )
		{
			g_mainFrame->SetDisplayMsg(data[0], 10000);
		}
	}
	else if( topic==wxT("execute") )
	{
		#if SJ_USE_SCRIPTS
			g_mainFrame->CmdLineAndDdeSeeExecute(GetTextFromData(data_, size_, format_));
		#else
			wxLogError(wxT("Scripts are not supported in this build."));
		#endif
	}
	else /* eg. "raise" */
	{
		Raise();
	}

	return TRUE;
}


class SjServer : public wxServer
{
public:
	wxConnectionBase* OnAcceptConnection(const wxString& topic)
	{
		return new SjConnection(TRUE);
	}
};


class SjClient : public wxClient
{
public:
	wxConnectionBase* OnMakeConnection()
	{
		return new SjConnection(FALSE);
	}
};


/*******************************************************************************
 * SjMainApp
 ******************************************************************************/


IMPLEMENT_APP(SjMainApp)


BEGIN_EVENT_TABLE(SjMainApp, wxApp)
	EVT_ACTIVATE_APP        (SjMainApp::OnActivate          )
	EVT_QUERY_END_SESSION   (SjMainApp::OnQueryEndSession   )
	EVT_END_SESSION         (SjMainApp::OnEndSession        )
END_EVENT_TABLE()


#define SJ_ISRUNNING_NAME   (wxT(".") SJ_PROGRAM_NAME wxT("-") + wxGetUserId() + g_tools->m_instance)

static wxSingleInstanceChecker* s_isRunningChecker  = NULL;

static SjServer* s_server                  = NULL;
static SjLogGui* s_logGui                  = NULL;
wxCmdLineParser* SjMainApp::s_cmdLine      = NULL;
bool             SjMainApp::s_isInShutdown = FALSE;


bool SjMainApp::OnInit()
{
	// call OnFatalException()
	//::wxHandleFatalExceptions(true);

	// init HTTP stuff - absolutely neccessary before any thread can use HTTP - eg. SjHttp or SjImgThread or maybe the player
	// see also http://www.litwindow.com/Knowhow/wxSocket/wxsocket.html
	wxSocketBase::Initialize();

	// set our logging routines
	s_logGui = new SjLogGui();

	// log silverjuke and platform version and release information
	wxLogInfo(wxT("Silverjuke %i.%i.%i-%ibit-%s%s%s started on %s-%ibit, %s, sqlite %s"),
	          SJ_VERSION_MAJOR, SJ_VERSION_MINOR, SJ_VERSION_REVISION, (int)(sizeof(void*)*8),
	          #ifdef wxUSE_UNICODE
	              wxT("u"),
	          #else
	              wxT("a"),
	          #endif
	          #ifdef __WXDEBUG__
                  wxT("d"),
	          #else
	              wxT(""),
	          #endif
	          #if SJ_USE_SCRIPTS
	              wxT("s"),
	          #else
	              wxT(""),
	          #endif
	          ::wxGetOsDescription().c_str(), ::wxIsPlatform64Bit()? 64 : 32,
	          wxVERSION_STRING,
	          wxSqltDb::GetLibVersion().c_str());

	// set the application name
	SetAppName(SJ_PROGRAM_NAME);

	// create and parse command line
	static const wxCmdLineEntryDesc s_cmdLineDesc[] =
	{
		// options and swithches, the order _is_ important as we can forward only _one_ command to other instances!
		// so please move things as "instance" down after "play", "pause" etc, see http://www.silverjuke.net/forum/post.php?p=10433#10433

		// player commands
		{ wxCMD_LINE_SWITCH, NULL, wxT_2("play"),        wxT_2("Play") },
		{ wxCMD_LINE_SWITCH, NULL, wxT_2("pause"),       wxT_2("Pause") },
		{ wxCMD_LINE_SWITCH, NULL, wxT_2("toggle"),      wxT_2("Toggle play/pause") },
		{ wxCMD_LINE_SWITCH, NULL, wxT_2("togglevis"),   wxT_2("Toggle vis.") },
		{ wxCMD_LINE_SWITCH, NULL, wxT_2("volup"),       wxT_2("Increase volume") },
		{ wxCMD_LINE_SWITCH, NULL, wxT_2("voldown"),     wxT_2("Decrease volume") },
		{ wxCMD_LINE_SWITCH, NULL, wxT_2("prev"),        wxT_2("Go to previous track") },
		{ wxCMD_LINE_SWITCH, NULL, wxT_2("next"),        wxT_2("Go to next track") },
		{ wxCMD_LINE_SWITCH, NULL, wxT_2("kiosk"),       wxT_2("Start kiosk mode") },
		{ wxCMD_LINE_SWITCH, NULL, wxT_2("update"),      wxT_2("Update index on startup") },
		{ wxCMD_LINE_SWITCH, NULL, wxT_2("raise"),       wxT_2("Show and raise the Silverjuke window") },
		{ wxCMD_LINE_SWITCH, NULL, wxT_2("open"),        wxT_2("Open the given file(s) (default)") },
		{ wxCMD_LINE_SWITCH, NULL, wxT_2("enqueue"),     wxT_2("Enqueue the given file(s)") },
		{ wxCMD_LINE_OPTION, NULL, wxT_2("setcredit"),   wxT_2("Set the number of credits in the credit system"), wxCMD_LINE_VAL_NUMBER },
		{ wxCMD_LINE_OPTION, NULL, wxT_2("addcredit"),   wxT_2("Add the number of credits to the credit system"), wxCMD_LINE_VAL_NUMBER },
		{ wxCMD_LINE_OPTION, NULL, wxT_2("displaymsg"),  wxT_2("Show the given message on the display"), wxCMD_LINE_VAL_STRING }, // this is mainly for testing IPC
		{ wxCMD_LINE_OPTION, NULL, wxT_2("execute"),     wxT_2("Execute the given scripting commands") },
		// environment settings
		{ wxCMD_LINE_SWITCH, NULL, wxT_2("skiperrors"),  wxT_2("Do not show startup errors") },
		{ wxCMD_LINE_SWITCH, NULL, wxT_2("minimize"),    wxT_2("Start minimized") },
		{ wxCMD_LINE_OPTION, NULL, wxT_2("kioskrect"),   wxT_2("The window rectangle for the kiosk mode as <x>,<y>,<w>,<h>[,clipmouse]") },
		{ wxCMD_LINE_OPTION, NULL, wxT_2("visrect"),     wxT_2("The window rectangle for the visualization as <x>,<y>,<w>,<h>") },
		{ wxCMD_LINE_OPTION, NULL, wxT_2("blackrect"),   wxT_2("Areas to cover by black rectangles, use as <x>,<y>,<w>,<h>[;<x>,<y>,<w>,<h>;...]") },
		{ wxCMD_LINE_OPTION, NULL, wxT_2("instance"),    wxT_2("Set the configuration file and the instance to use") },
		{ wxCMD_LINE_OPTION, NULL, wxT_2("ini"),         wxT_2("Set the configuration file to use") },
		{ wxCMD_LINE_OPTION, NULL, wxT_2("jukebox"),     wxT_2("Set the jukebox file to use") },
		{ wxCMD_LINE_OPTION, NULL, wxT_2("temp"),        wxT_2("Set the temporary directory to use") },
		// addional parameters
		{ wxCMD_LINE_PARAM,  NULL, NULL,                 wxT_2("File(s)"),  wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL|wxCMD_LINE_PARAM_MULTIPLE },
		{ wxCMD_LINE_NONE }
	};
	s_cmdLine = new wxCmdLineParser(s_cmdLineDesc, argc, argv);
	if( s_cmdLine == NULL )
	{
		FatalError();
	}

	if( s_cmdLine->Parse() != 0 /*-1: help option shown, >0 syntax error*/ )
	{
		return FALSE;
	}

	// create tools to use, the tools are deleted if SjMainFrame is deleted
	g_tools = new SjTools;

	// create the server name
	wxString serviceName;
	#ifndef __WXMSW__
		// under Unix (or Mac) the IPC communication goes over a temporary file
		serviceName = g_tools->m_cache.GetTempDir();
		wxASSERT( serviceName.Last() == wxT('/') );
	#endif
	serviceName << wxT("silverjuke");
	if( !g_tools->m_instance.IsEmpty() )
	{
		serviceName << wxT("-") << g_tools->m_instance;
	}
	#ifndef __WXMSW__
		serviceName << wxT("-ipc");
	#endif

	// Is there another instance of silverjuke? if so, forward the commmand line
	// to this instance. We use the wxSingleInstanceChecker class in addition
	// for speed reasons - wxClient::MakeConnection() is rather slow.
	{
		wxLogNull null; // don't show any suspicious errors

		wxASSERT( g_tools ); // needed in SJ_ISRUNNING_NAME
		s_isRunningChecker = new wxSingleInstanceChecker(SJ_ISRUNNING_NAME);
		if( s_isRunningChecker && s_isRunningChecker->IsAnotherRunning() )
		{
			bool executedSuccessfully = FALSE;
			SjClient* client = new SjClient;
			if( client )
			{
				// get all data (files)
				wxString data;
				int i;
				for( i = 0; i < (int)s_cmdLine->GetParamCount(); i++ )
				{
					data += i? wxT("\n") : wxT("");
					data += s_cmdLine->GetParam(i);
				}

				// get topic ("raise", "open" or one of the command line switches)
				wxString topic(data.IsEmpty()? wxT("raise") : wxT("open")), valString;
				long valLong = 0;
				for( i = 0; i < (int)(sizeof(s_cmdLineDesc)/sizeof(wxCmdLineEntryDesc)); i++ )
				{
					if( s_cmdLineDesc[i].kind == wxCMD_LINE_SWITCH
					 && s_cmdLine->Found(s_cmdLineDesc[i].longName) )
					{
						topic = s_cmdLineDesc[i].longName;
						break;
					}
					else if( s_cmdLineDesc[i].kind == wxCMD_LINE_OPTION )
					{
						if( s_cmdLineDesc[i].type == wxCMD_LINE_VAL_NUMBER
						 && s_cmdLine->Found(s_cmdLineDesc[i].longName, &valLong) )
						{
							topic = s_cmdLineDesc[i].longName;
							data = wxString::Format(wxT("%i"), (int)valLong);
							break;
						}
						else if( s_cmdLine->Found(s_cmdLineDesc[i].longName, &valString) )
						{
							topic = s_cmdLineDesc[i].longName;
							data = valString;
							break;
						}
					}
				}

				// execute topic
				wxConnectionBase* connection = client->MakeConnection(wxT("localhost"), serviceName, topic);
				if( connection )
				{
					executedSuccessfully = connection->Execute(data);
					connection->Disconnect();
					delete connection;
				}

				delete client;
				client = NULL;
			}

			// If execution has failed, wait a second to allow the previous instance
			// to terminated completly.  After that, we check the instance again
			// (for this, we have to recreate wxSingleInstanceChecker() as IsAnotherRunning()
			// always returns a static value).
			if( !executedSuccessfully )
			{
				::wxSleep(1);
				if( s_isRunningChecker ) { delete s_isRunningChecker; s_isRunningChecker = NULL; }
				s_isRunningChecker = new wxSingleInstanceChecker(SJ_ISRUNNING_NAME);
			}

			// On execution success or if the other process is still present, we're terminating now.
			// There's nothing more we can do here.  We check IsAnotherRunning() again as the prev.
			// instance may be terminted completly from our last check (see above).
			if( executedSuccessfully
			 || s_isRunningChecker == NULL
			 || s_isRunningChecker->IsAnotherRunning() )
			{
				if( s_isRunningChecker ) { delete s_isRunningChecker; s_isRunningChecker = NULL; }
				delete g_tools;
				g_tools = NULL;
				return FALSE;
			}
		}
	}

	// create the server
	s_server = new SjServer;
	if( s_server )
	{
		if( !s_server->Create(serviceName) )
		{
			delete s_server;
			s_server = NULL;
		}
	}

	// create main frame window
	long skinFlags = g_tools->m_config->Read(wxT("main/skinFlags"), SJ_SKIN_DEFAULT_FLAGS);

	new SjMainFrame(this, IDC_MAINFRAME, skinFlags, wxPoint(50, 50), wxSize(700, 500));

	return TRUE;
}


void SjMainApp::SetIsInShutdown()
{
	s_isInShutdown = TRUE;
}


int SjMainApp::OnExit()
{
	/* exit parser
	 */
	SjSkinMlParserData::FreeStaticData();

	/* exit tagging libraries
	 */
	SjExitID3Etc();

	/* client and server
	 */
	if( s_server )
	{
		delete s_server;
		s_server = NULL;
	}

	/* delete single instance checkers:
	 */
	if( s_isRunningChecker )
	{
		delete s_isRunningChecker;
		s_isRunningChecker = NULL;
	}

	/* delete command line
	 */
	if( s_cmdLine )
	{
		delete s_cmdLine;
		s_cmdLine = NULL;
	}

	/* remove our error logger
	*/
	delete s_logGui;

	/* delete pending temp. files
	 */
	SjTempNCache::LastCallOnExit();

	/* shutdown garbage collector
	 */
	#if SJ_USE_SCRIPTS
		::SjGcShutdown(); // this may free references to the objects deleted in the next lines
	#endif

	delete SjModuleSystem::s_delayedDbDelete;
	SjModuleSystem::s_delayedDbDelete = NULL;

	/* shutdown HTTP-services
	 */
	wxSocketBase::Shutdown();

	/* done.
	 */
	return 0;
}


void SjMainApp::OnActivate(wxActivateEvent& event)
{
	SjMainFrame* mainFrame = (SjMainFrame*)wxWindow::FindWindowById(IDC_MAINFRAME);
	if( mainFrame )
	{
		mainFrame->m_moduleSystem.BroadcastMsg(event.GetActive()? IDMODMSG_APP_ACTIVATED : IDMODMSG_APP_DEACTIVATED);
	}
}


void SjMainApp::OnQueryEndSession(wxCloseEvent& event)
{
	if( !IsInShutdown() )
	{
		SjMainFrame* mainFrame = (SjMainFrame*)GetTopWindow();
		if( mainFrame )
		{
			if( !event.CanVeto() || mainFrame->QueryEndSession(TRUE/*onShutdown*/) )
			{
				mainFrame->DoEndSession();
			}
			else
			{
				event.Veto();
			}
		}
	}
}
void SjMainApp::OnEndSession(wxCloseEvent& event)
{
	if( !IsInShutdown() )
	{
		SjMainFrame* mainFrame = (SjMainFrame*)GetTopWindow();
		if( mainFrame )
		{
			mainFrame->DoEndSession();
		}
	}
}
void SjMainApp::DoShutdownEtc(SjShutdownEtc action, long restoreOrgVol)
{
	// already in shutdown?
	if( IsInShutdown() )
	{
		return;
	}

	// user defined script?
	if( action>=SJ_SHUTDOWN_USER_DEF_SCRIPT && action<=SJ_SHUTDOWN_USER_DEF_SCRIPT_LAST )
	{
		#if SJ_USE_SCRIPTS
			SjSee::OnGlobalEmbedding(SJ_PERSISTENT_EXIT_OPTION_deprecated, action-SJ_SHUTDOWN_USER_DEF_SCRIPT);
		#endif
		return;
	}

	// stop playback
	g_mainFrame->Stop();

	if( restoreOrgVol != -1 )
	{
		g_mainFrame->SetAbsMainVol(restoreOrgVol);
	}

	if( action == SJ_SHUTDOWN_STOP_PLAYBACK )
	{
		return;
	}

	// just clear the playlist?
	if( action == SJ_SHUTDOWN_CLEAR_PLAYLIST )
	{
		g_mainFrame->UnqueueAll();
		return;
	}

	// exit, shutdown etc.
	g_tools->NotCrashed(TRUE); // On shutdown, the program may not really run to end ...

	if( action == SJ_SHUTDOWN_EXIT_SILVERJUKE )
	{
		g_mainFrame->DoEndSession();
	}
	else if( action == SJ_SHUTDOWN_POWEROFF_COMPUTER )
	{
		::wxShutdown(wxSHUTDOWN_POWEROFF);
	}
	else if( action == SJ_SHUTDOWN_REBOOT_COMPUTER )
	{
		::wxShutdown(wxSHUTDOWN_REBOOT);
	}
}


void SjMainApp::OnFatalException()
{
	FatalError();
}


void SjMainApp::FatalError()
{
	// show all errors
	wxLog* l = wxLog::GetActiveTarget();
	if( l )
	{
		l->Flush();
	}

	// show message that we will quite now
	::wxLogFatalError(wxString::Format(/*n/t, maybe locale error?*/wxT("We'll terminating %s now.\nSorry, there's nothing we can do."), SJ_PROGRAM_NAME));
}

