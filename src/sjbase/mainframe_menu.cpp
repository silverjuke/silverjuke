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
 * File:    mainframe.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke main frame - Menu part
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/browser.h>
#include <sjtools/console.h>
#include <sjmodules/advsearch.h>
#include <sjmodules/kiosk/kiosk.h>
#include <sjmodules/vis/vis_module.h>
#include <see_dom/sj_see.h>


/*******************************************************************************
 * The main menu
 ******************************************************************************/


void SjMainFrame::CreateMainMenu()
{
	// set up operating-system specific stuff, should be done before the menu bar is really created
	#ifdef __WXMAC__
		wxApp::s_macHelpMenuTitleName = _("Help");
		wxApp::s_macAboutMenuItemId = IDO_ABOUT;
		wxApp::s_macPreferencesMenuItemId = IDT_SETTINGS;
		wxApp::s_macExitMenuItemId = IDT_QUIT;
	#endif

	// create menu bar
	m_menuBar = new wxMenuBar();

	// create menus
	m_fileMenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
	m_editMenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
	m_viewMenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
	m_playbackMenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
	m_kioskMenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
	m_helpMenu = new SjMenu(SJ_SHORTCUTS_LOCAL);

	// create "file" menu
	m_fileMenu->Append(IDO_SETTINGS_ADDFILES, _("Add folders and files")+wxString(wxT("..."))); // same as "Settings", but makes things clearer
	m_fileMenu->Append(IDT_UPDATE_INDEX);
	m_fileMenu->AppendSeparator();
	m_fileMenu->Append(IDT_OPEN_FILES);
	m_fileMenu->Append(IDT_SAVE_PLAYLIST);
	m_fileMenu->Append(IDT_UNQUEUE_ALL);
	m_fileMenu->AppendSeparator();
	m_fileMenu->Append(IDT_QUIT);

	// create "edit" menu
	m_libraryModule->CreateMenu(m_playbackMenu, m_editMenu, TRUE);
	m_editExtrasMenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
	m_editMenu->Append(0, _("Extras"), m_editExtrasMenu);
	m_editMenu->AppendSeparator();
	m_editMenu->Append(IDO_SELECTALL);
	m_editMenu->AppendSeparator();
	m_editMenu->Append(IDT_SEARCH_BUTTON);
	m_editMenu->Append(IDT_ADV_SEARCH);
	m_editMenu->Append(IDO_REALLYENDSEARCH, _("End search"));
	m_editMenu->AppendSeparator();
	m_editMenu->Append(IDT_SETTINGS);

	// create "view" menu
	m_viewMenu->Append(IDT_GOTO_CURR);
	m_viewMenu->Append(IDT_WORKSPACE_GOTO_RANDOM);
	m_viewMenu->AppendSeparator();
	m_viewMenu->AppendRadioItem(IDT_WORKSPACE_ALBUM_VIEW);
	m_viewMenu->AppendRadioItem(IDT_WORKSPACE_COVER_VIEW);
	m_viewMenu->AppendRadioItem(IDT_WORKSPACE_LIST_VIEW);
	m_viewMenu->Append(IDT_WORKSPACE_TOGGLE_VIEW);
	m_viewMenu->AppendSeparator();
	SjMenu* submenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
		submenu->AppendCheckItem(IDT_TOGGLE_TIME_MODE, _("Show remaining time"));
		submenu->AppendCheckItem(IDO_TOGGLE_TIME_MODE2, _("Show elapsed time"));
		submenu->AppendCheckItem(IDO_DISPLAY_TOTAL_TIME, _("Show total time"));
		submenu->AppendCheckItem(IDO_DISPLAY_TRACKNR, _("Show track number"));
		submenu->AppendCheckItem(IDO_DISPLAY_ARTIST, _("Show artist name"));
		submenu->AppendCheckItem(IDO_DISPLAY_AUTOPLAY, _("Show AutoPlay"));
		submenu->AppendCheckItem(IDO_DISPLAY_PREFERALBCV, _("Prefer album- to track-cover"));
	m_viewMenu->Append(0, _("Display"), submenu);
	m_viewColMenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
	m_viewMenu->Append(0, _("Columns"), m_viewColMenu);
	submenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
		submenu->Append(IDT_ZOOM_IN);
		submenu->Append(IDT_ZOOM_OUT);
		submenu->Append(IDT_ZOOM_NORMAL);
		submenu->AppendSeparator();
		submenu->AppendCheckItem(IDO_SAMEZOOMINALLVIEWS, _("Same zoom in all views"));
	m_viewMenu->Append(0, _("Zoom"), submenu);
	m_viewMenu->AppendSeparator();
	m_viewMenu->AppendCheckItem(IDT_ALWAYS_ON_TOP);
	m_viewMenu->AppendSeparator();
	m_viewMenu->AppendCheckItem(IDT_START_VIS, _("Video screen"));

	// init "playback" menu
	m_playbackMenu->AppendSeparator();
	CreatePlaybackMenu(m_playbackMenu);

	// create "kiosk" menu
	m_kioskMenu->Append(IDT_TOGGLE_KIOSK, _("Start kiosk mode..."));

	// create "help" menu
	m_helpMenu->Append(IDO_ABOUT_OPEN_WWW, wxString::Format(_("%s on the web"), SJ_PROGRAM_NAME)+wxString(wxT("...")));
	m_helpMenu->Append(IDO_ONLINE_HELP, _("All files")+wxString(wxT("...")));
	m_helpMenu->AppendSeparator();
	m_helpMenu->Append(IDO_CONSOLE);
	m_helpMenu->AppendSeparator();
	m_helpMenu->Append(IDO_ABOUT, wxString::Format(_("About %s"), SJ_PROGRAM_NAME)+wxString(wxT("...")));
	#ifdef SJHOOK_INIT_MAIN_MENU
	SJHOOK_INIT_MAIN_MENU
	#endif

	// append menus to menu bar
	// (wxMenuBar::Append() should be called with all items added to the menu to append; this allows eg. OS X to move menu items around)
	m_menuBar->Append(m_fileMenu, _os(_("File")));
	m_menuBar->Append(m_editMenu, _("Edit"));
	m_menuBar->Append(m_viewMenu, _os(_("View")));
	m_menuBar->Append(m_playbackMenu, _("Playback"));
	m_menuBar->Append(m_kioskMenu, _("Kiosk mode"));
	m_menuBar->Append(m_helpMenu, _("Help"));

}


void SjMainFrame::UpdateMainMenu()
{
	if( m_menuBar == NULL || m_fileMenu == NULL || m_libraryModule == NULL || m_editMenu == NULL ) return;

    bool enableQueue = m_player.m_queue.GetCount()!=0;
    m_fileMenu->Enable(IDT_SAVE_PLAYLIST, enableQueue);
    m_fileMenu->Enable(IDT_UNQUEUE_ALL, enableQueue);

    m_libraryModule->UpdateMenuBar();

	m_editMenu->Enable(IDO_REALLYENDSEARCH, HasAnySearch());

	UpdateMenuBarQueue();
	UpdateViewMenu();

	// update scripts menu
    m_editExtrasMenu->Clear();
	AddScriptMenuEntries(*m_editExtrasMenu);
	if( m_editExtrasMenu->GetMenuItemCount() == 0 ) {
		m_editExtrasMenu->Append(IDO_SCRIPT_MENU99, _("None"));
		m_editExtrasMenu->Enable(IDO_SCRIPT_MENU99, false);
	}
}


void SjMainFrame::UpdateMenuBarValue(int targetId, const SjSkinValue& v)
{
	// update the menu bar - if any
	if( m_menuBar == NULL ) return;

	switch( targetId )
	{
		case IDT_PLAY:
		case IDT_PAUSE:
		case IDT_STOP:
		case IDT_MAIN_VOL_MUTE:
		case IDT_SHUFFLE:
		case IDT_PLAY_NOW_ON_DBL_CLICK:
		case IDT_ALWAYS_ON_TOP:
			m_menuBar->Check(targetId, v.value==1);
			break;

		case IDT_SAVE_PLAYLIST:
		case IDT_UNQUEUE_ALL:
		case IDT_MORE_FROM_CURR_ALBUM:
		case IDT_MORE_FROM_CURR_ARTIST:
			m_menuBar->Enable(targetId, v.value!=2);
			break;

		case IDT_STOP_AFTER_THIS_TRACK:
			m_menuBar->Check(IDT_STOP_AFTER_THIS_TRACK, v.value==1);
			break;

		case IDT_STOP_AFTER_EACH_TRACK:
			m_menuBar->Check(IDT_STOP_AFTER_EACH_TRACK, v.value==1);
			break;

		case IDT_PREV:
		case IDT_NEXT:
		case IDT_FADE_TO_NEXT:
			m_menuBar->Enable(targetId, v.value!=2);
			break;

		case IDT_REPEAT:
			if( m_playbackMenu )
			{
					 if( v.value==1 ) { m_playbackMenu->SetLabel(IDT_REPEAT, _("Repeat one")); }
				else if( v.value==2 ) { m_playbackMenu->SetLabel(IDT_REPEAT, _("Repeat all")); }
				else                  { m_playbackMenu->SetLabel(IDT_REPEAT, _("Repeat playlist")); }
				m_playbackMenu->Check(IDT_REPEAT, v.value!=0);
			}
			break;

		case IDT_WORKSPACE_ALBUM_VIEW: // IDT_WORKSPACE_ALBUM_VIEW, IDT_WORKSPACE_COVER_VIEW and IDT_WORKSPACE_LIST_VIEW always come "together"; so catching one event is sufficient here
			UpdateViewMenu();
			UpdateMenuBarQueue();
			if( m_libraryModule )
			{
				m_libraryModule->UpdateMenuBar();
			}
			break;

		case IDT_SEARCH_BUTTON:
			m_menuBar->Enable(IDO_REALLYENDSEARCH, HasAnySearch());
			break;

		case IDT_TOGGLE_TIME_MODE:
		case IDO_TOGGLE_TIME_MODE2:
			UpdateMenuBarQueue();
			break;

		case IDT_START_VIS:
			if( g_visModule ) {
				m_menuBar->Check(IDT_START_VIS, g_visModule->IsVisStarted());
			}
			break;
	}
}


void SjMainFrame::UpdateMenuBarQueue()
{
	// update the menu bar - if any
	if( m_menuBar == NULL || m_viewMenu == NULL ) return;

	bool enableQueue = m_player.m_queue.GetCount()!=0;

	m_menuBar->Enable(IDT_GOTO_CURR, enableQueue);

	m_viewMenu->Check (IDT_TOGGLE_TIME_MODE, m_showRemainingTime);
	m_viewMenu->Check (IDO_TOGGLE_TIME_MODE2, !m_showRemainingTime);
	m_viewMenu->Check (IDO_DISPLAY_TOTAL_TIME, (m_skinFlags&SJ_SKIN_SHOW_DISPLAY_TOTAL_TIME)!=0);
	m_viewMenu->Check (IDO_DISPLAY_TRACKNR, (m_skinFlags&SJ_SKIN_SHOW_DISPLAY_TRACKNR)!=0);
	m_viewMenu->Check (IDO_DISPLAY_ARTIST, (m_skinFlags&SJ_SKIN_SHOW_DISPLAY_ARTIST)!=0);
	m_viewMenu->Check (IDO_DISPLAY_AUTOPLAY, (m_skinFlags&SJ_SKIN_SHOW_DISPLAY_AUTOPLAY)!=0);
	m_viewMenu->Check (IDO_DISPLAY_PREFERALBCV, (m_skinFlags&SJ_SKIN_PREFER_TRACK_COVER)==0);
}



/*******************************************************************************
 * The view menu
 ******************************************************************************/


void SjMainFrame::UpdateViewMenu()
{
	if( m_viewMenu == NULL || m_viewColMenu == NULL ) return;

	bool enableQueue = m_player.m_queue.GetCount()!=0;

	m_viewMenu->Enable(IDT_GOTO_CURR, enableQueue);

	if( m_browser )
	{
		int view = m_browser->GetView();
		m_viewMenu->Check(IDT_WORKSPACE_ALBUM_VIEW, (view==SJ_BROWSER_ALBUM_VIEW));
		m_viewMenu->Check(IDT_WORKSPACE_COVER_VIEW, (view==SJ_BROWSER_COVER_VIEW));
		m_viewMenu->Check(IDT_WORKSPACE_LIST_VIEW, (view==SJ_BROWSER_LIST_VIEW));

		m_viewColMenu->Clear();
		m_viewColMenu->AppendCheckItem(IDT_WORKSPACE_SHOW_COVERS, (m_browser->GetView()==SJ_BROWSER_COVER_VIEW)? _("Show cover titles") : _("Show covers"));
		m_viewColMenu->Check(IDT_WORKSPACE_SHOW_COVERS, m_browser->AreCoversShown());
		m_browser->AddItemsToColMenu(m_viewColMenu);
	}

	if( g_accelModule )
	{
		m_viewMenu->Check(IDO_SAMEZOOMINALLVIEWS, (g_accelModule->m_flags&SJ_ACCEL_SAME_ZOOM_IN_ALL_VIEWS)!=0);
	}

	m_viewMenu->Check(IDT_ALWAYS_ON_TOP, IsAlwaysOnTop());

	if( g_visModule )
	{
		m_viewMenu->Check(IDT_START_VIS, g_visModule->IsVisStarted());
	}
}



/*******************************************************************************
 *  The playback menu
 ******************************************************************************/


void SjMainFrame::CreatePlaybackMenu(SjMenu* playbackMenu)
{
	bool anythingInQueue = GetQueueCount()>0;
	playbackMenu->Append(IDT_MORE_FROM_CURR_ALBUM);
	playbackMenu->Enable(IDT_MORE_FROM_CURR_ALBUM, anythingInQueue);
	playbackMenu->Append(IDT_MORE_FROM_CURR_ARTIST);
	playbackMenu->Enable(IDT_MORE_FROM_CURR_ARTIST, anythingInQueue);
	playbackMenu->AppendSeparator();

	if( IsOpAvailable(SJ_OP_PLAYPAUSE) )
	{
		playbackMenu->AppendRadioItem(IDT_PLAY);
		playbackMenu->Check(IDT_PLAY, m_player.IsPlaying());

		playbackMenu->AppendRadioItem(IDT_PAUSE);
		playbackMenu->Check(IDT_PAUSE, m_player.IsPaused());

		playbackMenu->AppendRadioItem(IDT_STOP);
		playbackMenu->Check(IDT_STOP, m_player.IsStopped());

		playbackMenu->AppendCheckItem(IDT_STOP_AFTER_THIS_TRACK);
		playbackMenu->Check(IDT_STOP_AFTER_THIS_TRACK, m_player.StopAfterThisTrack());

		playbackMenu->AppendCheckItem(IDT_STOP_AFTER_EACH_TRACK);
		playbackMenu->Check(IDT_STOP_AFTER_EACH_TRACK, m_player.StopAfterEachTrack());
	}

	if( IsOpAvailable(SJ_OP_EDIT_QUEUE) )
	{
		if( !IsOpAvailable(SJ_OP_PLAYPAUSE) ) playbackMenu->AppendSeparator();

		playbackMenu->Append(IDT_PREV);
		playbackMenu->Enable(IDT_PREV, HasPrev());

		bool hasNext = HasNextRegardAP();
		playbackMenu->Append(IDT_NEXT, GetNextMenuTitle());
		playbackMenu->Enable(IDT_NEXT, hasNext);

		playbackMenu->Append(IDT_FADE_TO_NEXT);
		playbackMenu->Enable(IDT_FADE_TO_NEXT, hasNext);

	}

	if( IsOpAvailable(SJ_OP_MAIN_VOL) )
	{
		SjMenu* volMenu = new SjMenu(playbackMenu->ShowShortcuts());

		volMenu->Append(IDT_MAIN_VOL_UP);

		volMenu->Append(IDT_MAIN_VOL_DOWN);

		volMenu->AppendCheckItem(IDT_MAIN_VOL_MUTE);
		volMenu->Check(IDT_MAIN_VOL_MUTE, m_player.GetMainVolMute());

		playbackMenu->Append(0, _("Volume"), volMenu);
	}

	if( IsOpAvailable(SJ_OP_EDIT_QUEUE) || IsOpAvailable(SJ_OP_REPEAT) || IsOpAvailable(SJ_OP_PLAYPAUSE) )
	{
		playbackMenu->AppendSeparator();

		if( IsOpAvailable(SJ_OP_REPEAT) )
		{
			wxString repeatText; // default by SjAccelModule
			if( m_player.m_queue.GetRepeat()==1 ) repeatText = _("Repeat one");
			if( m_player.m_queue.GetRepeat()==2 ) repeatText = _("Repeat all");
			playbackMenu->AppendCheckItem(IDT_REPEAT, repeatText);
			playbackMenu->Check(IDT_REPEAT, m_player.m_queue.GetRepeat()!=0);
		}

		if( IsOpAvailable(SJ_OP_EDIT_QUEUE) )
		{
			playbackMenu->AppendCheckItem(IDT_SHUFFLE);
			playbackMenu->Check(IDT_SHUFFLE, m_player.m_queue.GetShuffle());
		}

		if( IsOpAvailable(SJ_OP_EDIT_QUEUE) || IsOpAvailable(SJ_OP_PLAYPAUSE) )
		{
			SjMenu* queueMenu = new SjMenu(playbackMenu->ShowShortcuts());

			if( IsOpAvailable(SJ_OP_PLAYPAUSE) )
			{
				queueMenu->AppendCheckItem(IDO_START_PB_ON_ENQUEUE, _("Start playback on first enqueue"));
				queueMenu->Check(IDO_START_PB_ON_ENQUEUE, (g_accelModule->m_flags&SJ_ACCEL_START_PLAYBACK_ON_ENQUEUE)!=0);
			}

			if( IsOpAvailable(SJ_OP_EDIT_QUEUE) )
			{
				queueMenu->AppendCheckItem(IDT_PLAY_NOW_ON_DBL_CLICK);
				queueMenu->Check(IDT_PLAY_NOW_ON_DBL_CLICK, (g_accelModule->m_flags&SJ_ACCEL_PLAY_NOW_ON_DBL_CLICK)!=0);
			}

			playbackMenu->Append(0, _("Queue"), queueMenu);
		}
	}

}


/*******************************************************************************
 * The context menus
 ******************************************************************************/


void SjMainFrame::AddScriptMenuEntries(SjMenu& m)
{
	// add the menu entries creted by the scripts to the given menu; the function is used in the main menu and
	// for the context menus.  Before any entry is added, a separator is added (if there are no entries, no separator is added)
	#if SJ_USE_SCRIPTS
		wxArrayString arrMenu   = SjSee::GetGlobalEmbeddings(SJ_PERSISTENT_MENU_ENTRY);
		wxArrayString arrConfig = SjSee::GetGlobalEmbeddings(SJ_PERSISTENT_CONFIG_BUTTON);
		int iCountMenu   = arrMenu.GetCount(); if( iCountMenu   > (IDO_SCRIPT_MENU99-IDO_SCRIPT_MENU00)+1 ) { iCountMenu = IDO_SCRIPT_MENU99-IDO_SCRIPT_MENU00+1; }
		int iCountConfig = arrConfig.GetCount(); if( iCountConfig > (IDO_SCRIPTCONFIG_MENU99-IDO_SCRIPTCONFIG_MENU00)+1 ) { iCountConfig = IDO_SCRIPTCONFIG_MENU99-IDO_SCRIPTCONFIG_MENU00+1; }

		if( iCountMenu > 0 )
		{
			for( int i = 0; i<iCountMenu; i++ )
			{
				m.Append(IDO_SCRIPT_MENU00+i, arrMenu[i]);
			}

			if( iCountConfig > 0 )
			{
				m.AppendSeparator();
			}
		}

		if( iCountConfig > 0 )
		{
			for( int i = 0; i<iCountConfig; i++ )
			{
				m.Append(IDO_SCRIPTCONFIG_MENU00+i, arrConfig[i]);
			}
		}
	#endif
}


wxString SjMainFrame::GetNextMenuTitle()
{
	if( !HasNextIgnoreAP()
	        &&  HasNextRegardAP() )
	{
		wxString from;
		if( m_autoCtrl.m_autoPlayMusicSelId == 0 )
		{
			from = _("Current view");
		}
		else
		{
			SjAdvSearch advSearch = g_advSearchModule->GetSearchById(m_autoCtrl.m_autoPlayMusicSelId);
			from = advSearch.GetName();
		}

		return wxString::Format(_("Next track from \"%s\""), from.c_str());
	}
	else
	{
		return _("Next track");
	}
}


void SjMainFrame::CreateContextMenu_(SjMenu& mainMenu,
                                     bool embedFastSearch)
{
	// search
	// -- as we have a normal menu bar since 15.1, the context menu can be more focused (search only added if embedFastSearch set, formally this was always the case)
	if( embedFastSearch && IsOpAvailable(SJ_OP_SEARCH) )
	{
		// open adv. search dialog
		if( mainMenu.GetMenuItemCount() )
		{
			mainMenu.AppendSeparator();
		}

		if( IsAllAvailable() )
		{
			mainMenu.Append(IDT_ADV_SEARCH);
		}

		// "fast search"
		if( embedFastSearch )
		{
			CreateSearchMenu(mainMenu);
		}

		// end search
		mainMenu.Append(IDO_REALLYENDSEARCH, _("End search"));
		mainMenu.Enable(IDO_REALLYENDSEARCH, HasAnySearch());
	}

	// help
	if( IsKioskStarted() )
	{
		mainMenu.Append(IDO_ABOUT, wxString::Format(_("About %s"), SJ_PROGRAM_NAME)+wxString(wxT("...")));
	}
}


void SjMainFrame::CreateUnqueueMenu(SjMenu& mainMenu)
{
	long trackCount = m_player.m_queue.GetCount();
	long markedCount = m_display.m_selectedIds.GetCount();

	if( IsOpAvailable(SJ_OP_UNQUEUE) )
	{
		mainMenu.Append(IDO_UNQUEUE_MARKED);
		mainMenu.Enable(IDO_UNQUEUE_MARKED, (markedCount>0));

		mainMenu.Append(IDO_UNQUEUE_ALL_BUT_MARKED);
		mainMenu.Enable(IDO_UNQUEUE_ALL_BUT_MARKED, (markedCount>0 && trackCount>markedCount));
	}
}


void SjMainFrame::CreateSearchMenu(SjMenu& m)
{
	// History
	if( m_search.IsSet() && m_searchStat.m_totalResultCount > 0 )
	{
		g_advSearchModule->AddToHistory(m_search);
	}

	int i, iCount = g_advSearchModule->GetHistoryCount();
	SjSearchHistory currentSearch(m_search);
	if( iCount )
	{
		for( i=iCount-1; i >= 0; i-- )
		{
			const SjSearchHistory* item = g_advSearchModule->GetHistory(i);
			wxString title = item->GetTitle();
			if( !title.IsEmpty() )
			{
				if( *item == currentSearch )
				{
					m.AppendCheckItem(IDO_SEARCHHISTORY00+i, title);
					m.Check(IDO_SEARCHHISTORY00+i, TRUE);
				}
				else
				{
					m.Append(IDO_SEARCHHISTORY00+i, title);
				}
			}
		}
	}

	if( IsOpAvailable(SJ_OP_MUSIC_SEL_GENRE) )
	{
		// Direct genre selection
		wxArrayString data = m_libraryModule->GetUniqueValues(SJ_TI_GENRENAME);
		iCount = (int)data.GetCount();
		if( iCount )
		{
			#define MAX_SEARCH_GENRE_IN_MENU (IDO_SEARCHGENRE999-IDO_SEARCHGENRE000)
			if( iCount > MAX_SEARCH_GENRE_IN_MENU ) iCount = MAX_SEARCH_GENRE_IN_MENU;

			SjMenu* genreMenu = new SjMenu(m.ShowShortcuts());
			for( i =0; i < iCount; i++ )
			{
				genreMenu->Append(IDO_SEARCHGENRE000+i, data[i]);
			}
			m.Append(0, _("Search for genre"), genreMenu);
		}
	}


	if( IsOpAvailable(SJ_OP_MUSIC_SEL) )
	{
		// Direct music selection
		iCount = g_advSearchModule->GetSearchCount();
		if( iCount )
		{
			#define MAX_MUSIC_SEL_IN_MENU (IDO_SEARCHMUSICSEL999-IDO_SEARCHMUSICSEL000)
			if( iCount > MAX_MUSIC_SEL_IN_MENU ) iCount = MAX_MUSIC_SEL_IN_MENU;

			SjMenu* advSearchMenu = new SjMenu(m.ShowShortcuts());
			wxString name;
			for( i =0; i < iCount; i++ )
			{
				name = g_advSearchModule->GetSearchByIndex(i, FALSE).GetName();
				if( !name.IsEmpty() )
				{
					advSearchMenu->Append(IDO_SEARCHMUSICSEL000+i, name);
				}
			}
			m.Append(0, _("Search for music selection"), advSearchMenu);
		}
	}
}


void SjMainFrame::OnSkinTargetContextMenu(int targetId, long x, long y)
{
	SjMenu      mainMenu(0);
	bool        embedFastSearch = FALSE;
	long        unmarkQueueId = 0;

	m_display.m_tagEditorJustOpened = FALSE;

	GotInputFromUser();

	m_contextMenuClickedUrls.Clear();
	m_contextMenuClickedId = 0;

	if( (targetId>=IDT_DISPLAY_LINE_FIRST && targetId<=IDT_DISPLAY_LINE_LAST)
	 ||  targetId==IDT_CURR_TRACK
	 ||  targetId==IDT_NEXT_TRACK )
	{
		// queue menu items
		if( IsAllAvailable() )
		{
			if( targetId==IDT_CURR_TRACK || targetId==IDT_NEXT_TRACK )
			{
				long queuePos = m_player.m_queue.GetCurrPos()+(targetId==IDT_NEXT_TRACK?1:0);
				if( !IsStopped() && queuePos >= 0 && queuePos < m_player.m_queue.GetCount() )
				{
					if( targetId==IDT_CURR_TRACK || !GetShuffle() )
					{
						m_contextMenuClickedUrls.Add(m_player.m_queue.GetUrlByPos(queuePos));
						m_contextMenuClickedId = m_player.m_queue.GetIdByPos(queuePos);
						m_display.m_selectedIds.Clear();
						m_display.m_selectedIds.Insert(m_contextMenuClickedId, 1);
						m_display.m_selectedIdsTimestamp = SjTools::GetMsTicks();
						m_display.m_selectedIdsLockMs = 60*60*1000; // long enough, i think after one hour, the context menu should be closed
						unmarkQueueId = m_contextMenuClickedId;
						UpdateDisplay();
					}
				}
			}
			else
			{
				long trackCount = m_player.m_queue.GetCount();
				if( trackCount > 0 )
				{
					// mark the line under the mouse if it is not marked
					long queuePos = m_display.m_firstLineQueuePos + (targetId-IDT_DISPLAY_LINE_FIRST);
					if( queuePos >= 0 && queuePos < m_player.m_queue.GetCount() )
					{
						m_contextMenuClickedId = m_player.m_queue.GetIdByPos(queuePos);
						if( m_contextMenuClickedId != 0 )
						{
							if( m_display.m_selectedIds.Lookup(m_contextMenuClickedId) == 0 )
							{
								m_display.m_selectedIds.Clear();
								m_display.m_selectedIds.Insert(m_contextMenuClickedId, 1);
								m_display.m_selectedIdsTimestamp = SjTools::GetMsTicks();
								m_display.m_selectedIdsLockMs = 60*60*1000; // long enough, i think after one hour, the context menu should be closed
								unmarkQueueId = m_contextMenuClickedId;
								UpdateDisplay();
							}
						}
					}

					m_contextMenuClickedUrls = m_player.m_queue.GetUrlsByIds(m_display.m_selectedIds);
					if( m_contextMenuClickedUrls.GetCount() )
					{
						mainMenu.Append(IDO_GOTO_CURR_MARK);
					}
				}
			}

			long markedCount = m_contextMenuClickedUrls.GetCount();
			if( markedCount > 0 )
			{
				CreateUnqueueMenu(mainMenu);

				mainMenu.AppendSeparator();

				mainMenu.Append(IDO_EDITQUEUE, _("Edit tracks/Get info")+wxString(wxT("...")));

				m_libraryModule->CreateRatingMenu(mainMenu, IDO_RATINGQUEUE00, m_contextMenuClickedUrls);
				m_libraryModule->CreateArtistInfoMenu(mainMenu, IDO_ARTISTINFQUEUE00, m_contextMenuClickedUrls);

				mainMenu.Append(IDO_EXPLOREQUEUE, _os(_("Show file")));
				mainMenu.Enable(IDO_EXPLOREQUEUE, (markedCount==1));

				SjMenu* submenu = new SjMenu(0);
				AddScriptMenuEntries(*submenu);
				if( submenu->GetMenuItemCount() ) {
					mainMenu.Append(0, _("Extras"), submenu);
				}
				else {
					delete submenu;
				}
			}
		}
	}
	else
	{
		switch( targetId )
		{
			case IDT_DISPLAY_COVER:
				if( m_player.m_queue.GetCount() )
				{
					mainMenu.Append(IDO_DISPLAY_COVER, _("Cover editor"));
					if( IsAllAvailable() )
					{
						mainMenu.Append(IDO_DISPLAY_COVER_EXPLR, _os(_("Show file")));
					}
				}
				break;

			case IDT_OPEN_FILES:
			case IDT_APPEND_FILES:
			case IDT_SAVE_PLAYLIST:
			case IDT_UNQUEUE_ALL:
			case IDT_DISPLAY_UP:
			case IDT_DISPLAY_DOWN:
			case IDT_DISPLAY_V_SCROLL:
				CreateUnqueueMenu(mainMenu);
				break;

			case IDT_SEARCH:
			case IDT_SEARCH_BUTTON:
			case IDT_SEARCH_INFO:
				// "Search" menu
				if( IsAllAvailable() )
				{
					embedFastSearch = TRUE;
				}
				else
				{
					CreateSearchMenu(mainMenu);
				}
				break;
		}
	}

	CreateContextMenu_(mainMenu, embedFastSearch);

	if( targetId == IDT_MAINMENU && !IsKioskStarted() )
	{
		mainMenu.AppendSeparator();
		mainMenu.Append(IDT_QUIT);
	}

	if( mainMenu.GetMenuItemCount() )
	{
		PopupMenu(&mainMenu);
	}

	if(  unmarkQueueId
	 && !m_display.m_tagEditorJustOpened ) // if the tag editor is opened in between, the marked tracks in the display should stay
	{
		m_display.m_selectedIds.Remove(unmarkQueueId);
		UpdateDisplay();
	}
}
