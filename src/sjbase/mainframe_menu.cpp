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


void SjMainFrame::AllocMainMenu()
{
	// This function just sets the menu titles,
	// so that they are visible on load directly after showing the window.
	// This function is called very soon - don't assume any modules to be loaded.
	//
	// More menu entries are set in InitMainMenu() when eg. all modules are
	// loaded.

	m_menuBar = new wxMenuBar();

	m_fileMenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
	m_menuBar->Append(m_fileMenu, _("File"));

	m_editMenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
	m_menuBar->Append(m_editMenu, _("Edit"));

	m_viewMenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
	m_menuBar->Append(m_viewMenu, _("View"));

	m_playbackMenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
	m_menuBar->Append(m_playbackMenu, _("Playback"));

	m_kioskMenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
	m_menuBar->Append(m_kioskMenu, _("Kiosk mode"));

	m_helpMenu = new SjMenu(SJ_SHORTCUTS_LOCAL);
	m_menuBar->Append(m_helpMenu, _("Help"));
}


void SjMainFrame::InitMainMenu()
{
	// Add the menu entries to the menu titles allocated in AllocMainMenu().

	if( m_menuBar )
	{
		// mac specific

		#ifdef __WXMAC__
		wxApp::s_macHelpMenuTitleName = _("Help");
		wxApp::s_macAboutMenuItemId = IDO_ABOUT;
		wxApp::s_macExitMenuItemId = IDT_QUIT;
		#endif

		// file menu - do not update as there are problems with the MAC-specific items

		if( !m_menuBarComplete )
		{
			bool enableQueue = m_player.m_queue.GetCount()!=0;

			m_fileMenu->Append(IDO_SETTINGS_ADDFILES, _("Add folders and files")+wxString(wxT("..."))); // just a second menu entry with the same funktionality to make things clearer
			m_fileMenu->Append(IDT_UPDATE_INDEX);

			m_fileMenu->AppendSeparator();

			m_fileMenu->Append(IDT_OPEN_FILES);
			m_fileMenu->Append(IDT_SAVE_PLAYLIST);
			m_fileMenu->Enable(IDT_SAVE_PLAYLIST, enableQueue);

			m_fileMenu->Append(IDT_UNQUEUE_ALL);
			m_fileMenu->Enable(IDT_UNQUEUE_ALL, enableQueue);

			m_fileMenu->AppendSeparator();

			m_fileMenu->Append(IDT_QUIT);
		}

		// edit menu

		m_editMenu->Clear();
		m_playbackMenu->Clear(); // clear here as partly initalized with m_libraryModule->CreateMenu()

		m_libraryModule->CreateMenu(m_playbackMenu, m_editMenu, TRUE);
		m_libraryModule->UpdateMenuBar();

		m_editMenu->AppendSeparator();

		m_editMenu->Append(IDO_SELECTALL);

		m_editMenu->AppendSeparator();

		m_editMenu->Append(IDT_SEARCH_BUTTON);

		m_editMenu->Append(IDT_ADV_SEARCH);

		m_editMenu->Append(IDO_REALLYENDSEARCH, _("End search"));
		m_editMenu->Enable(IDO_REALLYENDSEARCH, HasAnySearch());

		m_editMenu->AppendSeparator();

		m_editMenu->Append(IDT_SETTINGS);

		// view menu

		m_viewMenu->Clear();
		CreateViewMenu(m_viewMenu);

		// playback menu

		m_playbackMenu->AppendSeparator();

		CreatePlaybackMenu(m_playbackMenu);

		// kiosk menu
		if( !m_menuBarComplete )
		{
			CreateKioskMenu(m_kioskMenu);
		}

		// help menu - do not update as there are problems with the MAC-specific items

		if( !m_menuBarComplete )
		{
			m_helpMenu->Append(IDO_ABOUT_OPEN_WWW, wxString::Format(_("%s on the web"), SJ_PROGRAM_NAME)+wxString(wxT("...")));
			m_helpMenu->Append(IDO_ONLINE_HELP, _("All files")+wxString(wxT("...")));
			m_helpMenu->AppendSeparator();
			m_helpMenu->Append(IDO_CONSOLE);
			m_helpMenu->AppendSeparator();
			m_helpMenu->Append(IDO_ABOUT, wxString::Format(_("About %s"), SJ_PROGRAM_NAME)+wxString(wxT("...")));
		}

		#ifdef SJHOOK_INIT_MAIN_MENU
		SJHOOK_INIT_MAIN_MENU
		#endif

		m_menuBarComplete = TRUE;

		m_libraryModule->UpdateMenuBar();

		UpdateMenuBarQueue();
	}
}


void SjMainFrame::UpdateMenuBarValue(int targetId, const SjSkinValue& v)
{
	// update the menu bar - if any
	if( !m_menuBarComplete ) return;

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
			     if( v.value==1 ) { m_playbackMenu->SetLabel(IDT_REPEAT, _("Repeat one")); }
			else if( v.value==2 ) { m_playbackMenu->SetLabel(IDT_REPEAT, _("Repeat all")); }
			else                  { m_playbackMenu->SetLabel(IDT_REPEAT, _("Repeat playlist")); }
			m_playbackMenu->Check(IDT_REPEAT, v.value!=0);
			break;

		case IDT_WORKSPACE_ALBUM_VIEW: // IDT_WORKSPACE_ALBUM_VIEW, IDT_WORKSPACE_COVER_VIEW and IDT_WORKSPACE_LIST_VIEW always come "together"; so catching one event is sufficient here
			m_viewMenu->Clear();
			CreateViewMenu(m_viewMenu);
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
	if( !m_menuBarComplete ) return;

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


void SjMainFrame::UpdateMenuBarView()
{
	m_viewMenu->Check(IDO_SAMEZOOMINALLVIEWS, (g_accelModule->m_flags&SJ_ACCEL_SAME_ZOOM_IN_ALL_VIEWS)!=0);
}


/*******************************************************************************
 * The view menu
 ******************************************************************************/


void SjMainFrame::CreateViewMenu(SjMenu* viewMenu)
{
	bool enableQueue = m_player.m_queue.GetCount()!=0;

	// goto options

	viewMenu->Append(IDT_GOTO_CURR);
	viewMenu->Enable(IDT_GOTO_CURR, enableQueue);
	viewMenu->Append(IDT_WORKSPACE_GOTO_RANDOM);

	viewMenu->AppendSeparator();

	// view select / view toggle

	int view = m_browser->GetView();

	viewMenu->AppendRadioItem(IDT_WORKSPACE_ALBUM_VIEW);
	viewMenu->Check(IDT_WORKSPACE_ALBUM_VIEW, (view==SJ_BROWSER_ALBUM_VIEW));

	viewMenu->AppendRadioItem(IDT_WORKSPACE_COVER_VIEW);
	viewMenu->Check(IDT_WORKSPACE_COVER_VIEW, (view==SJ_BROWSER_COVER_VIEW));

	viewMenu->AppendRadioItem(IDT_WORKSPACE_LIST_VIEW);
	viewMenu->Check(IDT_WORKSPACE_LIST_VIEW, (view==SJ_BROWSER_LIST_VIEW));

	viewMenu->Append(IDT_WORKSPACE_TOGGLE_VIEW);

	viewMenu->AppendSeparator();

	if( IsOpAvailable(SJ_OP_TOGGLE_TIME_MODE) || IsOpAvailable(SJ_OP_TOGGLE_ELEMENTS) || IsOpAvailable(SJ_OP_ZOOM) )
	{

		// display options

		if( IsOpAvailable(SJ_OP_TOGGLE_TIME_MODE) )
		{
			SjMenu* displayMenu = new SjMenu(viewMenu->ShowShortcuts());
				displayMenu->AppendCheckItem(IDT_TOGGLE_TIME_MODE, _("Show remaining time"));
				displayMenu->AppendCheckItem(IDO_TOGGLE_TIME_MODE2, _("Show elapsed time"));
				if( IsAllAvailable() )
				{
					displayMenu->AppendCheckItem(IDO_DISPLAY_TOTAL_TIME, _("Show total time"));
					displayMenu->AppendCheckItem(IDO_DISPLAY_TRACKNR, _("Show track number"));
					displayMenu->AppendCheckItem(IDO_DISPLAY_ARTIST, _("Show artist name"));
					displayMenu->AppendCheckItem(IDO_DISPLAY_AUTOPLAY, _("Show AutoPlay"));
					displayMenu->AppendCheckItem(IDO_DISPLAY_PREFERALBCV, _("Prefer album- to track-cover"));
				}
			viewMenu->Append(0, _("Display"), displayMenu);
		}

		// browser options

		if( IsOpAvailable(SJ_OP_TOGGLE_ELEMENTS) )
		{
			SjMenu* colMenu = new SjMenu(viewMenu->ShowShortcuts());

				colMenu->AppendCheckItem(IDT_WORKSPACE_SHOW_COVERS, (m_browser->GetView()==SJ_BROWSER_COVER_VIEW)? _("Show cover titles") : _("Show covers"));
				colMenu->Check(IDT_WORKSPACE_SHOW_COVERS, m_browser->AreCoversShown());

				m_browser->AddItemsToColMenu(colMenu);

			viewMenu->Append(0, _("Columns"), colMenu);
		}

		if( IsOpAvailable(SJ_OP_ZOOM) )
		{
			SjMenu* zoomMenu = new SjMenu(viewMenu->ShowShortcuts());
			zoomMenu->Append(IDT_ZOOM_IN);
			zoomMenu->Append(IDT_ZOOM_OUT);
			zoomMenu->Append(IDT_ZOOM_NORMAL);
			zoomMenu->AppendSeparator();
			zoomMenu->AppendCheckItem(IDO_SAMEZOOMINALLVIEWS, _("Same zoom in all views"));
			zoomMenu->Check(IDO_SAMEZOOMINALLVIEWS, (g_accelModule->m_flags&SJ_ACCEL_SAME_ZOOM_IN_ALL_VIEWS)!=0);
			viewMenu->Append(0, _("Zoom"), zoomMenu);
		}

		viewMenu->AppendSeparator();
	}

	// window options

	if( !IsKioskStarted() )
	{
		viewMenu->AppendCheckItem(IDT_ALWAYS_ON_TOP);
		viewMenu->Check(IDT_ALWAYS_ON_TOP, IsAlwaysOnTop());

		viewMenu->AppendSeparator();
	}


	if( g_visModule )
	{
		viewMenu->AppendCheckItem(IDT_START_VIS, _("Video screen"));
		viewMenu->Check(IDT_START_VIS, g_visModule->IsVisStarted());
	}

	UpdateMenuBarQueue();
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
 * The Kiosk menu
 ******************************************************************************/


void SjMainFrame::CreateKioskMenu(SjMenu* kioskMenu)
{
	kioskMenu->Append(IDT_TOGGLE_KIOSK, _("Start kiosk mode..."));
}


/*******************************************************************************
 * The context menus
 ******************************************************************************/


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
		if(  IsAllAvailable() )
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

					mainMenu.Append(IDO_GOTO_CURR_MARK);
				}
			}

			long markedCount = m_contextMenuClickedUrls.GetCount();

			CreateUnqueueMenu(mainMenu);

			mainMenu.AppendSeparator();

			mainMenu.Append(IDO_EDITQUEUE, _("Edit tracks/Get info")+wxString(wxT("...")));
			mainMenu.Enable(IDO_EDITQUEUE, (markedCount>0));

			m_libraryModule->CreateRatingMenu(mainMenu, IDO_RATINGQUEUE00, m_contextMenuClickedUrls);
			m_libraryModule->CreateArtistInfoMenu(mainMenu, IDO_ARTISTINFQUEUE00, m_contextMenuClickedUrls);

			mainMenu.Append(IDO_EXPLOREQUEUE, _("Show file"));
			mainMenu.Enable(IDO_EXPLOREQUEUE, (markedCount==1));
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
						mainMenu.Append(IDO_DISPLAY_COVER_EXPLR, _("Show file"));
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
