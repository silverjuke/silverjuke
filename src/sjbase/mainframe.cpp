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
 * Purpose: Silverjuke main frame
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/browser.h>
#include <sjtools/imgthread.h>
#include <sjtools/testdrive.h>
#include <sjtools/console.h>
#include <sjtools/msgbox.h>
#include <sjmodules/settings.h>
#include <sjmodules/arteditor.h>
#include <sjmodules/help/help.h>
#include <sjmodules/openfiles.h>
#include <sjmodules/basicsettings.h>
#include <sjmodules/kiosk/kiosk.h>
#include <sjmodules/advsearch.h>
#include <sjmodules/tageditor/tageditor.h>
#include <sjmodules/kiosk/virtkeybd.h>
#include <sjmodules/cinterface.h>
#include <sjmodules/playbacksettings.h>
#include <sjmodules/internalinterface.h>
#include <sjmodules/vis/vis_module.h>
#include <see_dom/sj_see.h>

#ifdef __WXMSW__
#include <windows.h>
#include <wx/msw/winundef.h> // undef symbols conflicting with wxWidgets
#endif
#include <wx/cmdline.h>
#include <wx/clipbrd.h>
#include <wx/stdpaths.h>
#include <wx/sysopt.h>

#include <sjdata/icons/xpm_sj_32.xpm>
#include <sjdata/icons/xpm_sj_16.xpm>


/*******************************************************************************
 * Public Functions
 ******************************************************************************/


SjMainFrame* g_mainFrame = NULL;


void SjMainFrame::OpenSettings(const wxString& selFile, int selIndex, int selPage)
{
	SjSettingsModule::OpenSettings(selFile, selIndex, selPage);
}


void SjMainFrame::Enqueue(const wxArrayString& urls, long enqueuePos,
                          bool urlsVerified, bool autoPlay, bool uiChecks)
{
	bool gotoPos            = false;
	bool fadeToPos          = false;
	bool forcePlayback      = false;
	bool queueEmptyBefore   = m_player.m_queue.GetCount()? false : true;
	long markForPlayingNext = 0;

	// anything to enqueue?
	if( urls.GetCount() <= 0 )
	{
		return; // nothing given
	}

	if( uiChecks )
	{
		// enought credits?
		if( !autoPlay
		 &&  g_kioskModule
		 && !g_kioskModule->CanEnqueue(urls, urlsVerified) )
		{
			return; // no, an error is already printed in CanEnqueue()
		}

		// interrupt AutoPlay?
		if( !autoPlay
		 &&  m_autoCtrl.HasInterruptibleAutoPlay()
		 &&  m_player.IsAutoPlayOnAir() )
		{
			enqueuePos = -3; /*play now*/
			fadeToPos  = true;
		}

		// get correct queue position
		if( enqueuePos == -2 /*play next*/ )
		{
			enqueuePos = m_player.m_queue.GetCurrPos()+1;
			markForPlayingNext = SJ_PLAYLISTENTRY_PLAYNEXT;
		}
		else if( enqueuePos == -3 /*play now*/ )
		{
			enqueuePos = m_player.m_queue.GetCurrPos()+1;
			gotoPos = true;
			forcePlayback = true;
		}
		else if( enqueuePos == -1 /*enqueue*/ )
		{
			;
		}

		// check if enqueing should also start playback
		if( m_player.m_queue.GetCount() == 0 )
		{
			if( (g_accelModule->m_flags&SJ_ACCEL_START_PLAYBACK_ON_ENQUEUE)
			 || !IsOpAvailable(SJ_OP_PLAYPAUSE) )
			{
				forcePlayback = true;
			}
		}

		if( !m_player.IsPlaying() && (IsKioskStarted() || !m_haltedManually ) ) // stopped and (in-kiosk or end-of-playlist)
		{
			if( g_accelModule->m_flags&SJ_ACCEL_START_PLAYBACK_ON_ENQUEUE )
			{
				gotoPos = true; // this will also mark the previous one as play which is unwanted if SJ_ACCEL_START_PLAYBACK_ON_ENQUEUE is unset
			}

			if( (g_accelModule->m_flags&SJ_ACCEL_START_PLAYBACK_ON_ENQUEUE)
			 || !IsOpAvailable(SJ_OP_PLAYPAUSE) )
			{
				forcePlayback = true;
			}
		}

		if( autoPlay )
		{
			forcePlayback = true;
			m_player.m_queue.EqualizeRepeatRound();
		}
	}

	// do what to do
	m_display.m_selectedIds.Clear();
	m_player.m_queue.Enqueue(urls, enqueuePos, urlsVerified, &m_display.m_selectedIds,
	                         markForPlayingNext|(autoPlay? SJ_PLAYLISTENTRY_AUTOPLAY : 0));

	if(  gotoPos
	 && (!queueEmptyBefore || !m_player.m_queue.GetShuffle()) /*if shuffle is enabled, let the queue select the track*/ )
	{
		m_player.GotoAbsPos(enqueuePos==-1? m_player.m_queue.GetCount()-urls.GetCount() : enqueuePos,
		                    fadeToPos);
	}

	if( forcePlayback && !m_player.IsPlaying() )
	{
		m_player.Play(); // don't call SjMainFrame::Play() - this save one UpdateDisplay()
		m_haltedManually = false;
	}

	if( !autoPlay )
	{
		GotInputFromUser();
	}

	m_display.m_selectedIdsTimestamp = SjTools::GetMsTicks();
	m_display.m_selectedIdsLockMs = SJ_DISPLAY_ENQUEUE_LOCK_MS;
	if( enqueuePos == -1 )
	{
		m_display.m_scrollPos = m_player.m_queue.GetCount()-urls.GetCount();
	}
	else
	{
		m_display.m_scrollPos = -1;
	}

	UpdateDisplay(); // visualize the changes
	m_browser->RefreshSelection();
	UpdateMenuBarQueue();
}


void SjMainFrame::UnqueueByPos(long queuePos)
{
	if( queuePos >= 0 && queuePos < m_player.m_queue.GetCount() )
	{
		long playlistEntryFlags = m_player.m_queue.GetFlags(queuePos);
		if( (playlistEntryFlags&SJ_PLAYLISTENTRY_AUTOPLAY)
		 && !IsPaused()
		 && !HasNextIgnoreAP() )
		{
			m_autoCtrl.SetAutoPlayUnqueueId(m_player.m_queue.GetIdByPos(queuePos));
		}

		m_player.m_queue.UnqueueByPos(queuePos, &m_player);
		UpdateDisplay();
		m_browser->RefreshSelection();

		UpdateMenuBarQueue();
	}
}


bool SjMainFrame::GotoNextRegardAP(bool fadeToNext, bool ignoreTimeouts)
{
	if( !HasNextIgnoreAP()
	 &&  HasNextRegardAP() )
	{
		// enqueue a new auto play URL
		if( !m_autoCtrl.DoAutoPlayIfEnabled(ignoreTimeouts) )
		{
			return false;
		}
	}

	bool success = m_player.GotoNextIgnoreAP(fadeToNext);
	m_display.m_scrollPos=-1;
	if( !IsPlaying() )
	{
		UpdateDisplay();
	}

	UpdateMenuBarQueue();
	return success;
}


void SjMainFrame::ReplayIfPlaying()
{
	// this function may be used to reflect new audio/player settings
	wxBusyCursor busy;

	// update player
	if( IsPlaying() )
	{
		long pos = GetElapsedTime();
		m_player.Stop(false);
		m_player.Play(pos);
	}
	else
	{
		Stop();
	}

	UpdateMenuBarQueue();
}


void SjMainFrame::SetAbsMainVol(long v)
{
	SjSkinValue value;

	m_player.SetMainVol(v);

	value.vmin      = 0;
	value.vmax      = 255;
	value.thumbSize = 0;
	value.value     = m_player.GetMainVol();
	SetSkinTargetValue(IDT_MAIN_VOL_SLIDER, value);

	value.vmax  = 0;
	value.value = m_player.GetMainVolMute()? 1 : 0;
	SetSkinTargetValue(IDT_MAIN_VOL_MUTE, value);
}


bool SjMainFrame::UpdateIndex(wxWindow* parent, bool deepUpdate)
{
	bool ret = TRUE;
	static bool inUpdate = FALSE;

	m_updateIndexAfterConstruction = false;

	if( !inUpdate )
	{
		inUpdate = TRUE;

		SJ_WINDOW_DISABLER(parent);

		// if we do not end the search and the music selection,
		// the search will be inconsistent after the update --
		// it may be possible to reset if afterwards, but this is more complicated.
		EndAllSearch();

		{
			SjBusyInfo /*construct exactly here*/ busy(parent,
			        deepUpdate? _("Recreating music library") :  _("Updating music library"),
			        IsAllAvailable() /*canCancel? (we may come here due the --update option in kiosk mode; in this case, the update may not be aborted)*/,
			        _("If you cancel the update process, your music library may not be up to date.\n\nCancel the update process?"));

			SjModuleList* list = m_moduleSystem.GetModules(SJ_MODULETYPE_COL);
			wxASSERT(list);

			SjModuleList::Node* moduleNode = list->GetFirst();
			while( moduleNode )
			{
				SjColModule* module = (SjColModule*)moduleNode->GetData();
				wxASSERT(module);

				if( !module->UpdateAllCol(&busy, deepUpdate) )
				{
					ret = FALSE;
					break; // user abort
				}

				moduleNode = moduleNode->GetNext();
			}
		}

		if( ret )
		{
			m_columnMixer.ReloadColumns();
			m_browser->ReloadColumnMixer();
		}

		UpdateMainMenu();

		inUpdate = FALSE;
	}

	return ret;
}


bool SjMainFrame::QueryEndSession(bool onShutdown)
{
	if( !m_mainApp->IsInShutdown() )
	{
		g_tools->NotCrashed(TRUE/*stopLogging*/);  // On shutdown, the program may not really run to end ...

		if( IsKioskStarted() )
		{
			if( onShutdown )
			{
				return !g_kioskModule->IsShutdownDisabled();
			}
			else
			{
				return FALSE;
			}
		}

		if( !onShutdown
		 &&  IsPlaying() )
		{
			if( g_accelModule->YesNo(
			            wxString::Format(_("%s is currently playing. Do you want to stop the playing track and exit %s?"),
			                             SJ_PROGRAM_NAME, SJ_PROGRAM_NAME
			                            ),
			            wxString::Format(_os(_("Exit %s")), SJ_PROGRAM_NAME),
			            this, SJ_ACCEL_ASK_ON_CLOSE_IF_PLAYING) != wxYES )
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}
void SjMainFrame::DoEndSession()
{
	if( !m_mainApp->IsInShutdown() )
	{
		m_mainApp->SetIsInShutdown();
		g_tools->NotCrashed(TRUE/*stopLogging*/);  // On shutdown, the program may not really run to end ...


		Destroy();
	}
}


/*******************************************************************************
 * Drag'n'Drop / Open Files
 ******************************************************************************/


class SjDropTarget : public wxDropTarget
{
public:
	SjDropTarget(wxWindow* linkedWindow)
	{
		m_linkedWindow = linkedWindow;
		m_dataObject = new SjDataObject(wxDragMove);
		m_dataObject->AddFileData();
		m_dataObject->AddBitmapData();
		SetDataObject(m_dataObject);
	}

	wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
	{
		if( g_mainFrame
		 && g_mainFrame->DragNDrop(SJ_DND_MOVE, g_mainFrame, wxPoint(x, y), NULL, NULL) )
		{
			return def;
		}
		else
		{
			return wxDragNone;
		}
	}

	bool OnDrop(wxCoord x, wxCoord y)
	{
		return (OnDragOver(x, y, wxDragCopy)==wxDragCopy);
	}

	wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def)
	{
		if( OnDragOver(x, y, wxDragCopy)!=wxDragCopy )
		{
			return wxDragNone;
		}

		m_dataObject = new SjDataObject(def);
		m_dataObject->AddFileData();
		m_dataObject->AddBitmapData();
		m_dataObject->m_dragResult = def;
		SetDataObject(m_dataObject);
		GetData();

		m_dataX = x;
		m_dataY = y;

		// convert the coordinates to client coordinated of the main window, if needed
		if( m_linkedWindow != g_mainFrame )
		{
			wxPoint linkedPos = m_linkedWindow->GetPosition();
			m_dataX += linkedPos.x;
			m_dataY += linkedPos.y;
		}

		wxCommandEvent* fwd = new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDO_DND_ONDATA);
		fwd->SetClientData((void*)this);
		g_mainFrame->GetEventHandler()->QueueEvent(fwd);

		return def;
	}

	SjDataObject*   m_dataObject;
	wxCoord         m_dataX,
	                m_dataY;
	wxWindow*       m_linkedWindow;
};


void SjMainFrame::OnIDO_DND_ONDATA(wxCommandEvent& evt)
{
	SjDropTarget* dropTarget = (SjDropTarget*)evt.GetClientData();
	if( dropTarget )
	{
		DragNDrop(SJ_DND_DROP, this, wxPoint(dropTarget->m_dataX, dropTarget->m_dataY), dropTarget->m_dataObject, NULL);
	}
}


bool SjMainFrame::DragNDrop(SjDragNDropAction action,
                            wxWindow* dndOwner, const wxPoint& dndOwnerPos,
                            SjDataObject* data,
                            wxArrayString* files) // "files" is only needed to change from the internal to the system-wide drag'n'drop
{
	static wxWindow*        s_currWindow = NULL;
	static SjCommonModule*  s_currModule = NULL;

	//
	// save the given data and get the screen position of the dnd action
	//

	wxPoint screenPos = dndOwner->ClientToScreen(dndOwnerPos);

	//
	// init drag'n'drop - this message is NOT forwarded to any module at this moment
	// (done when we receive eg. MOVE)
	//

	if( action == SJ_DND_ENTER )
	{
		s_currWindow = NULL;
		s_currModule = NULL;
		m_internalDragNDrop = TRUE;

		// release the mouse capture (handled by the dnd classes)
		if( dndOwner->HasCapture() )
		{
			dndOwner->ReleaseMouse();
		}

		// try to enter dnd using an image
		if( m_dragImage )
		{
			if( (g_accelModule->m_flags&SJ_ACCEL_USE_DND_IMAGES)
			 && m_dragImage->BeginDrag(m_dragHotspot, dndOwner, TRUE) )
			{
				m_dragRect.x = screenPos.x - m_dragHotspot.x;
				m_dragRect.y = screenPos.y - m_dragHotspot.y;
				m_dragImage->Move(dndOwnerPos);
				m_dragImage->Show();
				return TRUE; // start dnd by caller, the caller should call this function with the
			}                // actions SJ_DND_MOVE and SJ_DND_DROP

			delete m_dragImage;
			m_dragImage = NULL;
		}

		// use default system-dnd without an image
		if( files )
		{
			wxFileDataObject myData; for( size_t i = 0; i < files->GetCount(); i++ ) { myData.AddFile(files->Item(i)); }
			{
				wxDropSource dropSource(dndOwner); // it may be important that wxDropSource is freed BEFORE
				dropSource.SetData(myData);        // wxFileDataObject is freed (true as it is inside a nested block)
				dropSource.DoDragDrop(TRUE);
			}
		}
		m_internalDragNDrop = FALSE;
		return FALSE; // the caller should not call this function for the dnd progress anymore
	}

	//
	// find the top-level window under the mouse
	//

	wxPoint currPos;
	{
		wxWindow* newWindow = SjDialog::FindTopLevel(::wxFindWindowAtPoint(screenPos));
		if( newWindow )
		{
			currPos = newWindow->ScreenToClient(screenPos);
		}

		if( newWindow != s_currWindow )
		{
			if( s_currModule )
			{
				s_currModule->DragNDrop(SJ_DND_LEAVE, s_currWindow, currPos, NULL);
				s_currModule = NULL;
			}

			if( s_currWindow )
			{
				if( s_currWindow==g_mainFrame || s_currWindow->GetParent()==g_mainFrame )
					s_currWindow->SetCursor(SjVirtKeybdModule::GetStandardCursor());
				else
					s_currWindow->SetCursor(*wxSTANDARD_CURSOR); // wxNullCursor does not work!, see http://www.silverjuke.net/forum/topic-1478.html
			}

			s_currWindow = newWindow; // may be NULL

			if( s_currWindow )
			{
				// find out the module that will handle the dnd stuff
				SjModuleList* list = m_moduleSystem.GetModules(SJ_MODULETYPE_COMMON);
				SjModuleList::Node* moduleNode = list->GetFirst();
				while( moduleNode )
				{
					SjCommonModule* module = (SjCommonModule*)moduleNode->GetData();
					wxASSERT( module && module->m_type == SJ_MODULETYPE_COMMON );

					if( module->DragNDrop(SJ_DND_ENTER, s_currWindow, currPos, NULL) )
					{
						s_currModule = module;
						break;
					}

					moduleNode = moduleNode->GetNext();
				}
			}
		}
	}

	//
	// move the drag'n'drop image or, if no image is present, test if the dnd target
	// accepts the
	//

	if( action == SJ_DND_MOVE )
	{
		if( m_dragImage )
		{
			static int inHereRet = 0;
			if( inHereRet )
			{
				return inHereRet > 0? TRUE : FALSE;
			}

			wxRect r = GetRect(); r.Inflate(4);
			wxPoint mouse = ::wxGetMousePosition();
			if( (mouse.x < r.x || mouse.x > r.x+r.width ||  mouse.y < r.y || mouse.y > r.y+r.height)
			        && s_currWindow == NULL )
			{
				inHereRet = -1;

				// Item moved outside silverjuke: end imaged drag'n'drop and
				// switch to normal drag'n'drop to allow other applications to receive data.
				//
				// As there were some problems some times, I added lots of "crash precautions"
				// to see what happens.  I then added the "inHere" state - hopefully this
				// solved the problem. however, I think we leave the crash precautions here
				// some time.
				g_tools->CrashPrecaution(wxT("mainframe"), wxT("DragNDrop(DeleteImage)"));
				m_dragImage->Hide();
				m_dragImage->EndDrag();
				delete m_dragImage;
				m_dragImage = NULL;

				// start normal drag'n'drop
				if( files )
				{
					g_tools->CrashPrecaution(wxT("mainframe"), wxT("DragNDrop(CreateDataObject)"));
					wxFileDataObject myData; for( size_t i = 0; i < files->GetCount(); i++ ) { myData.AddFile(files->Item(i)); }
					{
						g_tools->CrashPrecaution(wxT("mainframe"), wxT("DragNDrop(CreateDropSrc)"));
						wxDropSource dropSource(dndOwner); // it may be important that wxDropSource is freed BEFORE

						g_tools->CrashPrecaution(wxT("mainframe"), wxT("DragNDrop(SetData)"));
						dropSource.SetData(myData);        // wxFileDataObject is freed (true as it is inside a nested block)

						g_tools->CrashPrecaution(wxT("mainframe"), wxT("DragNDrop(DoDragDrop)"));
						dropSource.DoDragDrop(TRUE);
					}
				}

				g_tools->CrashPrecaution(wxT("mainframe"), wxT("DragNDrop(EndInternal)"));
				m_internalDragNDrop = FALSE;

				inHereRet = 0;
				return FALSE; // stop dnd
			}
			else
			{
				// just move the image inside Silverjuke
				inHereRet = 1;

				m_dragRect.x = screenPos.x - m_dragHotspot.x;
				m_dragRect.y = screenPos.y - m_dragHotspot.y;

				m_dragImage->Move(dndOwnerPos);

				if( s_currModule
				 && s_currModule->DragNDrop(SJ_DND_MOVE, s_currWindow, currPos, NULL) )
				{
					if( s_currWindow==g_mainFrame || s_currWindow->GetParent()==g_mainFrame )
						s_currWindow->SetCursor(SjVirtKeybdModule::GetStandardCursor());
					else
						s_currWindow->SetCursor(*wxSTANDARD_CURSOR); // wxNullCursor does not work!, see http://www.silverjuke.net/forum/topic-1478.html
				}
				else if( s_currWindow )
				{
					s_currWindow->SetCursor(g_tools->m_staticNoEntryCursor);
				}

				inHereRet = 0;
				return TRUE; // continue dnd
			}
		}
		else if( s_currModule )
		{
			// when we're here, the function is called from SjDropTarget::OnDragOver() -
			// return if the position is acceptable or not
			wxASSERT( s_currWindow );
			return s_currModule->DragNDrop(SJ_DND_MOVE, s_currWindow, currPos, NULL);
		}
	}

	//
	// drop the item over the given dnd target
	//

	if( action == SJ_DND_DROP )
	{
		// finish drag'n'drop
		if( m_dragImage )
		{
			m_dragImage->Hide();
			m_dragImage->EndDrag();

			delete m_dragImage;
			m_dragImage = NULL;

			if( s_currWindow )
			{
				if( s_currWindow==g_mainFrame || s_currWindow->GetParent()==g_mainFrame )
					s_currWindow->SetCursor(SjVirtKeybdModule::GetStandardCursor());
				else
					s_currWindow->SetCursor(*wxSTANDARD_CURSOR); // wxNullCursor does not work!, see http://www.silverjuke.net/forum/topic-1478.html
			}
		}

		if( s_currModule )
		{
			wxASSERT( s_currWindow );

			if( data )
			{
				s_currModule->DragNDrop(SJ_DND_DROP, s_currWindow, currPos, data);
			}
			else if( files )
			{
				SjDataObject data(wxDragMove);
				data.AddFiles(*files);
				s_currModule->DragNDrop(SJ_DND_DROP, s_currWindow, currPos, &data);
			}
		}

		m_internalDragNDrop = FALSE;
	}

	return FALSE; // stop drag'n'drop
}


bool SjMainFrame::OpenFiles(const wxArrayString& files, int command, int x, int y)
{
	SjDataObject data(wxDragMove);
	data.AddFiles(files);
	return OpenData(&data, command, x, y);
}


bool SjMainFrame::OpenData(SjDataObject* data, int command, int mouseX, int mouseY)
{
	wxBusyCursor    busy;
	SjPlaylist      newPlaylist;

	// if the main frame is not enable, we probably cannot accept any files as
	// we may be in an undefined modal state.
	if( !IsEnabled() ) return FALSE;

	// any files given?
	if( data == NULL ) return FALSE;
	wxArrayString filenames;
	if( data->m_fileData )
	{
		filenames = data->m_fileData->GetFilenames();
		#ifdef __WXMAC__ // there seems to be a bug in the drag'n'drop handler of wx - the filenames contain lineend characters!?
		for( int i = filenames.GetCount()-1; i >= 0; i-- )
		{
			filenames[i].Replace(wxT("\n"), wxT(""));
			filenames[i].Replace(wxT("\r"), wxT(""));
		}
		#endif
	}
	long filenamesCount = filenames.GetCount();

	// get the extension, if there is only one file
	wxString ext;
	if( filenamesCount == 1 )
	{
		ext = SjTools::GetExt(filenames[0]);
	}

	// see what we can do with the data...
	long        i, iCount;

	if( filenamesCount == 1 && ext == wxT("sjs") )
	{
		// open a skin
		if( !::wxFileExists(filenames[0]) )
		{
			// TRANSLATORS: %s will be replaced by a filename
			wxLogError(_("Cannot open \"%s\"."), filenames[0].c_str());
			return false;
		}

		wxFileName srcfile(filenames[0]);
		wxFileName destfile(g_tools->GetSearchPath(0), srcfile.GetFullName());
		if( !::wxFileExists(destfile.GetFullPath()) )
		{
			::wxCopyFile(srcfile.GetFullPath(), destfile.GetFullPath());
		}

		Raise();
		OpenSettings(wxT("memory:viewsettings.lib"), 0, 0);
		return true;
	}
	else if( filenamesCount == 1 && ext == wxT("js") )
	{
		// open and execute a script ...
		#if SJ_USE_SCRIPTS
			if( !::wxFileExists(filenames[0]) )
			{
				wxLogError(_("Cannot open \"%s\"."), filenames[0].c_str());
				return false;
			}

			// ... copy to user dir
			Raise();
			{
				wxFileName srcfile(filenames[0]);
				wxFileName destfile(g_tools->GetSearchPath(0), srcfile.GetFullName());
				wxArrayString options;
				options.Add(wxString::Format(_("Also install the script to \"%s\" for permanent use"), destfile.GetPath(wxPATH_GET_VOLUME|wxPATH_GET_SEPARATOR).c_str()));
				int selOption = 0;
				if( SjMessageBox(wxString::Format(_("Execute the script \"%s\"?")+"\n\n"+
				                 _("CAUTION: Scripts may slow down Silverjuke or damage your data. Please use only scripts you trust."),
				                 srcfile.GetFullPath().c_str()),
				                 SJ_PROGRAM_NAME, wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION, this, &options, &selOption) != wxYES )
				{
					return false;
				}

				if( selOption )
				{
					::wxCopyFile(srcfile.GetFullPath(), destfile.GetFullPath(), true /*overwrite*/);
					filenames[0] = destfile.GetFullPath();
				}
			}

			// ... execute the script
			wxLogInfo(wxT("Loading %s"), filenames[0].c_str());

			wxFileSystem fs;
			wxFSFile* fsFile = fs.OpenFile(filenames[0]);
			if( fsFile == NULL )
				return false;

			SjSee* see = new SjSee;
			see->SetExecutionScope(filenames[0]);

			see->Execute(SjTools::GetFileContent(fsFile->GetStream(), &wxConvUTF8));
			delete fsFile;

			if( see->m_persistentAnchor->m_next == NULL )
				delete see; // script is a "one shot" and can be deleted
			else
				m_moduleSystem.m_sees.Add(see);
		#else
			wxLogError(wxT("Scripts are not supported in this build."));
		#endif
		return true;
	}
	else if( (filenamesCount == 1 && m_moduleSystem.FindImageHandlerByExt(ext))
	      || (data->m_bitmapData && data->m_bitmapData->GetBitmap().IsOk()) )
	{
		// open an image - this should be handled by the browser
		if( m_browser )
		{
			Raise();
			ClientToScreen(&mouseX, &mouseY);
			m_browser->ScreenToClient(&mouseX, &mouseY);
			m_browser->DropImage(data, mouseX, mouseY);
		}
		return TRUE;
	}
	else if( filenamesCount == 1 && m_moduleSystem.FindPlaylistHandlerByExt(ext) )
	{
		// modify playlist: open a single playlist
		if( !newPlaylist.AddFromFile(filenames[0]) )
		{
			return FALSE;
		}
	}
	else if( filenamesCount >= 1 )
	{
		// modify playlist: add music files
		//              or: if there are folders given, forward them to the library module
		wxArrayString folders;
		for( i = 0; i < filenamesCount; i++ )
		{
			if( ::wxDirExists(filenames[i]) )
			{
				folders.Add(filenames[i]);
			}
			else
			{
				newPlaylist.Add(filenames[i], FALSE/*not verified*/, 0/*flags*/);
			}
		}

		if( folders.GetCount() && m_libraryModule )
		{
			Raise();
			if( m_libraryModule->AddFolders(folders) )
			{
				UpdateIndex(g_mainFrame, FALSE);
				return TRUE;
			}
			return FALSE;
		}
	}
	else
	{
		// cannot handle the data
		return FALSE;
	}

	// add playlist to playing queue
	if( newPlaylist.GetCount() )
	{
		SjHaltAutoPlay haltAutoPlay;

		wxArrayString newUrls;
		iCount = newPlaylist.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			newUrls.Add(newPlaylist[i].GetUnverifiedUrl());
		}

		m_player.m_queue.MergeMetaData(newPlaylist);
		if( command!=SJ_OPENFILES_ENQUEUE )
		{
			m_player.m_queue.UnqueueAll(&m_player, FALSE);
		}
		Enqueue(newUrls, (command==SJ_OPENFILES_PLAY&&i==0)? -3/*play now*/ : -1/*play last*/, FALSE/*not verified*/);
		m_browser->RefreshSelection();
		return TRUE;
	}
	else
	{
		for( i = filenamesCount-1; i >=0 ; i-- )
		{
			wxLogError(_("Cannot open \"%s\"."), filenames[i].c_str());
		}
		return FALSE;
	}
}


/*******************************************************************************
 * Constructor / Destructor
 ******************************************************************************/


SjMainFrame::SjMainFrame(SjMainApp* mainApp, int id, long skinFlags, const wxPoint& pos, const wxSize& size) :
	SjSkinWindow(NULL, id, skinFlags, SJ_PROGRAM_NAME, pos, size, wxDEFAULT_FRAME_STYLE)
{
	/* =======================================================
	 * init as less and as fast as possible until the window
	 * can be shown - this makes the program look faster;
	 * remember that some objects are initialized before
	 * construction!
	 * =======================================================
	 */

	/* (/) Init some pointers
	 */
	g_mainFrame                     = this;
	#if SJ_USE_SCRIPTS
	m_cmdLineAndDdeSee              = NULL;
	#endif
	m_updateIndexAfterConstruction  = false;
	m_menuBar                       = NULL;
	m_fileMenu                      = NULL;
	m_editMenu                      = NULL;
	m_editExtrasMenu                = NULL;
	m_viewMenu                      = NULL;
	m_viewColMenu                   = NULL;
	m_playbackMenu                  = NULL;
	m_visMenu                       = NULL;
	m_kioskMenu                     = NULL;
	m_helpMenu                      = NULL;
	m_haltedManually                = FALSE;
	m_mainApp                       = mainApp;
	m_browser                       = NULL;
	m_imgThread                     = NULL;
	m_libraryModule                 = NULL;
	m_availOp                       = SJ_OP_DEF_NONKIOSK;
	m_internalDragNDrop             = FALSE;
	m_lastUserDisplayInputTimestamp = 0;
	m_lastUserBrowserInputTimestamp = 0;
	m_inConstruction                = TRUE;
	m_currZoom                      = -1; // set invalid, corrected when the browser view is loaded

	GotInputFromUser();

	/* (1) Init Tools, configuration may be used from here
	 */
	// already done in SjMainApp as needed for the parent constructor

	/* (/) Init I18n stuff, this needs the filesystem handlers
	 *     (SjModule constructor) and SjTools
	 */
	{
		// define search paths for .mo lookup
		#if defined(__WXMSW__)
			wxLocale::AddCatalogLookupPathPrefix(wxStandardPaths::Get().GetResourcesDir() + wxT("\\locale")); // eg. "C:\program Files\Silverjuke\locale"
		#elif !defined(__WXOSX__)
			wxLocale::AddCatalogLookupPathPrefix(wxT("/usr/local/share/locale")); // for proper installations, wx should already search here, however, for development installations, this explicit path may help
		#endif

		// get language to use as canonical name
		wxString langCanonicalName;
		const wxLanguageInfo* langInfo = wxLocale::GetLanguageInfo(wxLocale::GetSystemLanguage());
		langCanonicalName = langInfo? langInfo->CanonicalName : wxString(wxT("en_GB"));
		langCanonicalName = g_tools->m_config->Read(wxT("main/language"), langCanonicalName);

		// convert the canonical name to a language information structure, NULL on errors
		langInfo = wxLocale::FindLanguageInfo(langCanonicalName);

		// init the locale
		{
			wxLogNull null; // avoid warnings like "cannot set locale to ..." if the language is not available as a system language
			if( !m_locale.Init(langInfo? langInfo->Language : wxLANGUAGE_DEFAULT, 0) )
			{
				m_locale.Init(langCanonicalName, langCanonicalName); // not sure why, however, this form loads catalogs even if the locale is not supported by the system
			}
		}

		// add translation catalog
		m_locale.AddCatalog(wxT("silverjuke"));
	}

	/* (/) Crash handling (after i18n for localized messages)
	 */
	if( !SjMainApp::s_cmdLine->Found(wxT("skiperrors")) )
	{
		g_tools->ShowPossibleCrash();
	}

	/* (2) Init Image Thread
	 */
	m_imgThread = new SjImgThread();
	if( !m_imgThread )
	{
		SjMainApp::FatalError();
	}

	/* (3) create browser and search input window; the browser needs the image thread
	 */
	m_browser = new SjBrowserWindow(this);
	SetWorkspaceWindow(m_browser);

	m_inPerformingSearch = FALSE;

	m_simpleSearchInputFromUser = FALSE;

	#ifdef __WXMAC__
	// wxMAC_TEXTCONTROL_USE_MLTE supports using of background colours etc. but also has
	// some disadvantages - so we use this flag only if needed - and here it is needed.
	// see mac/carbon/textctrl.h for details.
	wxSystemOptions::SetOption(wxMAC_TEXTCONTROL_USE_MLTE, 1);
	#endif

	m_simpleSearchInputWindow = new SjVirtKeybdTextCtrl(this, IDO_SEARCHINPUT, wxT(""),
	        wxDefaultPosition, wxDefaultSize,
	        wxTE_LEFT | wxTE_PROCESS_ENTER | wxNO_BORDER);

	#ifdef __WXMAC__
	// switch back to the normal MAC controls
	wxSystemOptions::SetOption(wxMAC_TEXTCONTROL_USE_MLTE, 0);
	#endif

	SetInputWindow(m_simpleSearchInputWindow);

	m_simpleSearchInputFromUser = TRUE;

	m_simpleSearchInputTimer.SetOwner(this, IDTIMER_SEARCHINPUT);
	m_elapsedTimeTimer.SetOwner(this, IDTIMER_ELAPSEDTIME);

	// init search info text
	{	SjSkinValue v;
		v.value  = SJ_VFLAG_CENTER;
		v.string = _("Search");
		SetSkinTargetValue(IDT_SEARCH_INFO, v);
	}

	/* (/) Load Skin, Set Icons
	 */
	SetDefaultWindowSize();
	{
		wxString defaultSkin(wxT("memory:defaultskin.xml"));
		wxString skin = g_tools->m_config->Read(wxT("main/skinFile"), defaultSkin);

		wxLogInfo(wxT("Loading %s"), skin.c_str());

		if( !LoadSkin(skin, SJ_OP_DEF_NONKIOSK, g_tools->m_config->Read(wxT("main/skinSettings"), wxT(""))) )
		{
			wxLogError(wxT("Cannot load %s, falling back to default skin"), skin.c_str());

			LoadSkin(defaultSkin, SJ_OP_DEF_NONKIOSK);
			g_tools->m_config->Write(wxT("main/skinFile"), defaultSkin);
		}
	}

	m_iconBundle.AddIcon(wxIcon(xpm_sj_32));
	m_iconBundle.AddIcon(wxIcon(xpm_sj_16));
	SetIcons(m_iconBundle);

	// check whether to start minimized, some OS specials
	bool startMinimized = (SjMainApp::s_cmdLine->Found(wxT("minimize")) || g_tools->m_config->Read(wxT("main/minimize"), 0L)!=0);
	#ifdef __WXMSW__
	if( mainApp->m_nCmdShow == SW_HIDE
	 || mainApp->m_nCmdShow == SW_MINIMIZE
	 || mainApp->m_nCmdShow == SW_SHOWMINIMIZED
	 || mainApp->m_nCmdShow == SW_SHOWMINNOACTIVE )
	{
		startMinimized = true;
	}
	#endif

	// check whether to start in kiosk mode
	bool startKiosk = (SjMainApp::s_cmdLine->Found(wxT("kiosk")) || g_tools->m_config->Read(wxT("main/kiosk"), 0L)!=0);
	if( startKiosk )
	{
		startMinimized = false;
	}

	/* (/) Load settings if not yet done as needed before window showing
	 */
	m_baseColumnWidth       = CorrectColumnWidth(g_tools->m_config->Read(wxT("main/fontCoverSize")/*historical*/, SJ_DEF_COLUMN_WIDTH));
	m_baseCoverHeight       = CorrectCoverHeight(g_tools->m_config->Read(wxT("main/fontCoverHeight"), SJ_DEF_COVER_HEIGHT));
	m_baseFontSize          = CorrectFontSize(g_tools->m_config->Read(wxT("main/fontSize"), SJ_DEF_FONT_SIZE));
	m_baseFontFace          = g_tools->m_config->Read(wxT("main/fontFace"), SJ_DEF_FONT_FACE);

	m_allSearchMs           = g_tools->m_config->Read(wxT("main/searchTime"), SJ_SEARCH_DELAY_MS);
	m_allSearchCount        = 1; // fake one search to avoid division by zero

	m_autoCtrl.LoadAutoCtrlSettings();

	/* (4) Init Modules and Player
	 */
	m_moduleSystem.Init();
	m_moduleSystem.AddInterface(new SjInternalInterface());
	m_moduleSystem.AddInterface(new SjCInterface()); // the interface is also used for adding scripts, so add it independingly of SJ_USE_C_INTERFACE

	m_moduleSystem.LoadModules();

	m_libraryModule = (SjLibraryModule*)m_moduleSystem.FindModuleByFile(wxT("memory:library.lib"));
	wxASSERT(m_libraryModule);

	m_columnMixer.LoadModules(&m_moduleSystem); // this will also load the library module

	m_player.Init();

	m_showRemainingTime = g_tools->m_config->Read(wxT("main/showRemainingTime"), 1L)!=0;

	/* (/) load other GUI modules, this sets MANY global pointers to the modules
	 */
	{
		int i, iCount;
		for( i = 0; i < 2; i++ )
		{
			SjModuleList* list = m_moduleSystem.GetModules(SJ_MODULETYPE_ALL);
			SjModuleList::Node* moduleNode = list->GetFirst();
			while( moduleNode )
			{
				SjModule* module = moduleNode->GetData();
				wxASSERT(module);

				if( module->m_type == SJ_MODULETYPE_COMMON
				 || module->m_type == SJ_MODULETYPE_SCANNER )
				{
					if( (i==0 && module->m_interface == g_internalInterface)
					 || (i==1 && module->m_interface != g_internalInterface) )
					{
						module->Load();
					}
				}

				moduleNode = moduleNode->GetNext();
			}
		}

		// Load scripts - if things are not ready at this point, make it safe to use them anyway.
		// Remember, the skin, which may also use scripts, is also already loaded.
		wxFileSystem fs;
		m_moduleSystem.m_scripts.Sort();
		iCount = m_moduleSystem.m_scripts.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			#if SJ_USE_SCRIPTS
				wxLogInfo(wxT("Loading %s"), m_moduleSystem.m_scripts[i].c_str());

				wxFSFile* fsFile = fs.OpenFile(m_moduleSystem.m_scripts[i]);
				if( fsFile )
				{
					SjSee* see = new SjSee;
					see->SetExecutionScope(m_moduleSystem.m_scripts[i]);

					see->Execute(SjTools::GetFileContent(fsFile->GetStream(), &wxConvUTF8));
					delete fsFile;

					if( see->m_persistentAnchor->m_next == NULL )
						delete see; // script is a "one shot" and can be deleted
					else
						m_moduleSystem.m_sees.Add(see);
				}
			#else
				wxLogInfo(wxT("%s not loaded - scripts are not supported in this build"), m_moduleSystem.m_scripts[i].c_str());
			#endif
		}
	}

	if( SjMainApp::s_cmdLine->Found(wxT("execute")) )
	{
		#if SJ_USE_SCRIPTS
			wxString cmds;
			SjMainApp::s_cmdLine->Found(wxT("execute"), &cmds);
			CmdLineAndDdeSeeExecute(cmds);
		#else
			wxLogError(wxT("Scripts are not supported in this build."));
		#endif
	}

	/* (/) set the browser to the current state
	 */
	{
		m_browser->SetView_(g_tools->m_config->Read(wxT("main/selAlbumView"), (long)SJ_BROWSER_ALBUM_VIEW), false/*nothing to keep*/, false /*no redraw, done in GotoPos*/);
		long lastAdvSearchId = g_tools->m_config->Read(wxT("main/searchId"), 0L);
		bool reloadColumnMixer = TRUE;

		if( lastAdvSearchId )
		{
			wxASSERT( g_advSearchModule );
			SjAdvSearch lastSearch = g_advSearchModule->GetSearchById(lastAdvSearchId);
			if( lastSearch.GetId() )
			{
				SetSearch(SJ_SETSEARCH_SETADV, wxT(""), &lastSearch);
				if( m_searchStat.m_totalResultCount == 0 )
				{
					// if there are no results, clear the search; this may happen eg.
					// if the adv. search contains conditons regarding the playlist
					SetSearch(SJ_SETSEARCH_CLEARADV|SJ_SETSEARCH_NOAUTOHISTORYADD);
				}
				reloadColumnMixer = FALSE; // done in set search
			}
		}

		if( reloadColumnMixer )
		{
			m_browser->ReloadColumnMixer();
		}

		m_browser->GotoPos(g_tools->m_config->Read(wxT("main/selAlbumUrl"), wxT("")));
	}

	/* (/) set accelerators
	 * (some accelerators as "space" are done in the browser window
	 * as they're also used in the input window)
	 */
	SetAcceleratorTable(g_accelModule->GetAccelTable(SJA_MAIN));

	SetSkinTargetValue(IDT_PLAY_NOW_ON_DBL_CLICK, (g_accelModule->m_flags&SJ_ACCEL_PLAY_NOW_ON_DBL_CLICK)? 1 : 0);

	SetSkinTargetValue(IDT_TOGGLE_REMOVE_PLAYED, (m_player.m_queue.GetQueueFlags()&SJ_QUEUEF_REMOVE_PLAYED)? 1 : 0);

	/* (/) Init main menu
	 */
	CreateMainMenu();
	SetMenuBar(m_menuBar);
	UpdateMainMenu();

	/* open files from the command line or resume
	 */
	if( SjMainApp::s_cmdLine->GetParamCount() )
	{
		wxArrayString filenames;
		int i, iCount = SjMainApp::s_cmdLine->GetParamCount();
		for( i = 0; i < iCount; i++ )
		{
			filenames.Add(SjMainApp::s_cmdLine->GetParam(i));
		}

		int commandId = SJ_OPENFILES_DEFCMD;
		if( SjMainApp::s_cmdLine->Found(wxT("enqueue")) ) commandId = SJ_OPENFILES_ENQUEUE;
		OpenFiles(filenames, commandId);
	}
	else if( m_player.m_queue.GetQueueFlags()&SJ_QUEUEF_RESUME )
	{
		m_player.LoadFromResumeFile();
	}

	/* (/) Show the window, init Drag'n'Drop
	 */
	if( startMinimized )
	{
		Iconize(true);
	}

	UpdateDisplay();
	Show();
	Update();

	SetDropTarget(new SjDropTarget(this)); // Init Drag'n'Drop (don't wonder: this will start 2 threads under MSW) and set the focus to the browser
	#ifdef __WXMAC__
		m_browser->SetDropTarget(new SjDropTarget(m_browser));
	#endif

	m_browser->SetFocus();

	/* (/) Some Functionality Tests
	 */
	if( g_debug )
	{
		SjTestdrive1();
	}

	/* start kiosk mode?
	 */
	if( startKiosk )
	{
		g_kioskModule->StartRequest(TRUE/*force no query settings*/);
	}

	/* update the index?
	 */
	if( SjMainApp::s_cmdLine->Found(wxT("update")) || g_tools->m_config->Read(wxT("main/update"), 0L)!=0 )
	{
		UpdateIndexAfterConstruction();
	}

	if( m_updateIndexAfterConstruction )
	{
		g_mainFrame->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDT_UPDATE_INDEX));
	}

	/* start the timer - this should be VERY last as the timer
	 * is used eg. to start a playback
	 */
	m_blinkBlink = TRUE;
	m_elapsedTimeTimer.Start(1000);

	/* done
	 */
	m_inConstruction = FALSE;
	GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDMODMSG_PROGRAM_LOADED)); // do not call BroadcastMsg() directly to allow windows to get realized
}


SjMainFrame::~SjMainFrame(void)
{
	/* =======================================================
	 * First hide the top level windows (looks faster) as the real
	 * termination progress may take some seconds eg. when the temp.
	 * directory is cleaned.  Moreover, the fist we do is to
	 * fade down the music, if playing ;-)
	 * =======================================================
	 */

	m_moduleSystem.BroadcastMsg(IDMODMSG_WINDOW_BEFORE_CLOSE_HIDE_N_STOP);

	m_player.Pause();   // this will start fading the music out, the implementation may use a smart fading by regarding IsInShutdown()
	g_visModule->StopVis();

	#if SJ_USE_TOOLTIPS
		g_tools->m_toolTipManager.ExitForever();
	#endif

	wxWindowList::Node *node;
	for ( node = wxTopLevelWindows.GetFirst(); node; node = node->GetNext() )
	{
		wxWindow *winTop = node->GetData();
		if ( winTop->IsShown() )
		{
			winTop->Hide();
		}
	}

	m_imgThread->Shutdown();

	if( IsKioskStarted() )
	{
		g_kioskModule->DoExit(FALSE/*restoreWindow*/);
	}

	m_moduleSystem.BroadcastMsg(IDMODMSG_WINDOW_CLOSE);

	/* =======================================================
	 * now, as the window is hidden, start destruction
	 * =======================================================
	 */

	/* (/) Save some settings
	 */

	if( !IsIconized()
	 && !IsMaximized() // don't check for IsFakedMaximized(); the faked maximized positions can be used
	 && !IsFullScreen() )
	{
		g_tools->m_config->Write(wxT("main/skinSettings"), GetSavableSkinSettings());
	}

	g_tools->m_config->Write(wxT("main/showRemainingTime"), m_showRemainingTime? 1L : 0L);
	g_tools->m_config->Write(wxT("main/searchTime"), m_allSearchMs/m_allSearchCount);
	g_tools->m_config->Write(wxT("main/searchId"), m_search.m_adv.GetId());
	g_tools->m_config->Write(wxT("main/selAlbumView"), m_browser->GetView());
	g_tools->m_config->Write(wxT("main/selAlbumUrl"), m_browser->GetFirstVisiblePos());

	/* (3) Exit Browser, should be done _before_ existing the modules
	 * although the initialisation order is different
	 */
	m_browser->Exit();

	/* (4) Exit Player and Modules; m_player.Exit() may wait until the music is faded out
	 */
	m_player.Exit(); // still needs the module system
	m_player.SaveSettings();

	m_columnMixer.UnloadModules();
	m_moduleSystem.Exit();

	/* (2) Exit Image Thread
	 */
	if( m_imgThread )
	{
		delete m_imgThread;
		m_imgThread = 0;
	}

	/* (1) Exit Tools
	 */
	if( g_tools )
	{
		delete g_tools;
		g_tools = NULL;
	}

	g_mainFrame = NULL;
}


/*******************************************************************************
 * Events
 ******************************************************************************/


BEGIN_EVENT_TABLE(SjMainFrame, SjSkinWindow)
	EVT_MENU_RANGE  (IDT_FIRST, IDT_LAST,                       SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_TOGGLE_TIME_MODE2,                     SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_DISPLAY_COVER,                         SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_DISPLAY_COVER_EXPLR,                   SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_DISPLAY_TOTAL_TIME,                    SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_DISPLAY_TRACKNR,                       SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_DISPLAY_ARTIST,                        SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_DISPLAY_AUTOPLAY,                      SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_DISPLAY_PREFERALBCV,                   SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_START_PB_ON_ENQUEUE,                   SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_GOTO_CURR_MARK,                        SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_GOTOCURRAUTO,                          SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_SETTINGS_ADDFILES,                     SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_SETTINGS_SKINS,                        SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_SETTINGS_FONTNCOVER,                   SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_SETTINGS_QUEUE,                        SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_SETTINGS_AUTOCTRL,                     SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_SETTINGS_EQUALIZER,                    SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_REALLYENDSEARCH,                       SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_CORNERCLICK,                           SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_ABOUT_OPEN_WWW,                        SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_ABOUT,                                 SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_ONLINE_HELP,                           SjMainFrame::OnFwdToSkin     )
	EVT_MENU_RANGE  (IDO_VIS_FIRST__, IDO_VIS_LAST__,           SjMainFrame::OnFwdToSkin     )
	EVT_MENU_RANGE  (IDO_SCRIPT_MENU00, IDO_SCRIPT_MENU99,      SjMainFrame::OnFwdToSkin     )
	EVT_MENU_RANGE  (IDO_SCRIPTCONFIG_MENU00, IDO_SCRIPTCONFIG_MENU99, SjMainFrame::OnFwdToSkin )
	EVT_MENU        (IDO_CONSOLE,                               SjMainFrame::OnFwdToSkin     )
	EVT_MENU_RANGE  (IDM_FIRST, IDM_LAST,                       SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_BROWSER_RELOAD_VIEW,                   SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_DEBUGSKIN_RELOAD,                      SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_SELECTALL,                             SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_UNQUEUE_MARKED,                        SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_UNQUEUE_ALL_BUT_MARKED,                SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_EDITQUEUE,                             SjMainFrame::OnFwdToSkin     )
	EVT_MENU_RANGE  (IDO_ARTISTINFQUEUE00,IDO_ARTISTINFQUEUE49, SjMainFrame::OnFwdToSkin     )
	EVT_MENU_RANGE  (IDO_RATINGQUEUE00, IDO_RATINGQUEUE05,      SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_EXPLOREQUEUE,                          SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_SMOOTH,                                SjMainFrame::OnFwdToSkin     )
	EVT_MENU        (IDO_SAMEZOOMINALLVIEWS,                    SjMainFrame::OnFwdToSkin     )

	EVT_MENU_RANGE  (IDMODMSG_FIRST,
	                 IDMODMSG_LAST,             SjMainFrame::OnFwdToModules         )
	EVT_MENU_RANGE  (IDPLAYER_FIRST,
	                 IDPLAYER_LAST,             SjMainFrame::OnFwdToPlayer          )
	EVT_MENU        (IDO_DND_ONDATA,            SjMainFrame::OnIDO_DND_ONDATA       )
	EVT_MENU        (IDO_PASTE,                 SjMainFrame::OnPaste                )
	EVT_MENU        (IDO_PASTE_USING_COORD,     SjMainFrame::OnPaste                )
	EVT_MENU        (IDO_ESC,                   SjMainFrame::OnEsc                  )
	EVT_MENU        (IDO_TAB,                   SjMainFrame::OnTab                  )
	EVT_TEXT        (IDO_SEARCHINPUT,           SjMainFrame::OnSimpleSearchInput    )
	EVT_TEXT_ENTER  (IDO_SEARCHINPUT,           SjMainFrame::OnSimpleSearchInput    )
	EVT_MENU_RANGE  (IDO_SEARCHHISTORY00,
	                 IDO_SEARCHHISTORY99,       SjMainFrame::OnSearchHistory        )
	EVT_MENU_RANGE  (IDO_SEARCHGENRE000,
	                 IDO_SEARCHGENRE999,        SjMainFrame::OnSearchGenre          )
	EVT_MENU_RANGE  (IDO_SEARCHMUSICSEL000,
	                 IDO_SEARCHMUSICSEL999,     SjMainFrame::OnSearchMusicSel       )
	EVT_TIMER       (IDTIMER_SEARCHINPUT,       SjMainFrame::OnSimpleSearchInputTimer)
	EVT_TIMER       (IDTIMER_ELAPSEDTIME,       SjMainFrame::OnElapsedTimeTimer     )
	EVT_CLOSE       (                           SjMainFrame::OnCloseWindow          )
	EVT_ICONIZE     (                           SjMainFrame::OnIconizeWindow        )
	//EVT_IDLE      (                           SjMainFrame::OnIdle                 )

	#ifndef __WXMSW__
	EVT_MOUSEWHEEL  (                           SjMainFrame::OnMouseWheel           )
	#endif

	#ifdef SJHOOK_MAIN_EVENT_TABLE
	SJHOOK_MAIN_EVENT_TABLE
	#endif

END_EVENT_TABLE()


#if SJ_USE_SCRIPTS
void SjMainFrame::CmdLineAndDdeSeeExecute(const wxString& cmds__)
{
	wxString cmds(cmds__);

	if( m_mainApp == NULL || m_mainApp->IsInShutdown()
	        || g_mainFrame == NULL )
	{
		return; // cancel if in shutdown
	}

	if( m_cmdLineAndDdeSee == NULL )
	{
		m_cmdLineAndDdeSee = new SjSee;
		m_cmdLineAndDdeSee->SetExecutionScope(wxT("DDE"));
		m_moduleSystem.m_sees.Add(m_cmdLineAndDdeSee);
	}

	// cmds can be either some native commands or a file to load.
	// convert file to native commands:
	if( ::wxFileExists(cmds) )
	{
		wxFileSystem fs;
		wxFSFile* fsFile = fs.OpenFile(cmds);
		cmds.Empty();
		if( fsFile != NULL )
		{
			cmds = SjTools::GetFileContent(fsFile->GetStream(), &wxConvUTF8);
			delete fsFile;
		}
	}

	m_cmdLineAndDdeSee->Execute(cmds);
}
#endif


void SjMainFrame::OnSkinTargetEvent(int targetId, SjSkinValue& value, long accelFlags)
{
	/* hier sollten wirklich alle Fäden zusammenlaufen damit die Tastaturbedienung auch ohne
	Accel-Table funktioniert, vgl. http://www.silverjuke.net/forum/topic-3197.html */

	GotInputFromUser();

	if( targetId >= IDT_DISPLAY_LINE_FIRST && targetId <= IDT_DISPLAY_LINE_LAST )
	{
		OnSkinDisplayEvent(targetId, value, accelFlags);
	}
	else if( (targetId >= IDT_WORKSPACE_GOTO_A    && targetId <= IDT_WORKSPACE_GOTO_0_9) // Silverjuke is a "grown" program, so we have no continuous IDs for the workspace ...
	      || (targetId >= IDT_WORKSPACE_LINE_LEFT && targetId <= IDT_WORKSPACE_LINE_DOWN)
	      || (targetId >= IDT_WORKSPACE_PAGE_LEFT && targetId <= IDT_WORKSPACE_ENTER)
	      ||  targetId == IDT_WORKSPACE_GOTO_PREV_AZ
	      ||  targetId == IDT_WORKSPACE_GOTO_NEXT_AZ
	      ||  targetId == IDT_WORKSPACE_HOME
	      ||  targetId == IDT_WORKSPACE_END
	      ||  targetId == IDT_WORKSPACE_H_SCROLL
	      ||  targetId == IDT_WORKSPACE_V_SCROLL
	      ||  targetId == IDT_WORKSPACE_GOTO_RANDOM
	      ||  targetId == IDT_WORKSPACE_SHOW_COVERS
	      ||  targetId == IDT_ZOOM_IN
	      ||  targetId == IDT_ZOOM_OUT
	      ||  targetId == IDT_ZOOM_NORMAL )
	{
		// this stuff will be forwarded to the browser
		// (we may come from there, but this is not necessarily the case, eg. when using "real" accelerators or buttons for this)
		m_browser->OnSkinTargetEvent(targetId, value, accelFlags);
	}
	else if( targetId >= IDT_LAYOUT_FIRST && targetId <= IDT_LAYOUT_LAST )
	{
		SjSkinLayout* layout = GetLayout(targetId-IDT_LAYOUT_FIRST);
		if( layout )
		{
			LoadLayout(layout);
		}
	}
	else if( targetId >= IDT_NUMPAD_FIRST && targetId <= IDT_NUMPAD_LAST )
	{
		if( g_kioskModule )
		{
			g_kioskModule->m_numpadInput.KeyPressed(targetId);
		}
	}
	else if( targetId >= IDT_ADD_CREDIT_FIRST && targetId <= IDT_ADD_CREDIT_LAST )
	{
		g_kioskModule->m_creditBase.AddCredit((targetId-IDT_ADD_CREDIT_01)+1,
		                                      SJ_ADDCREDIT_FROM_DDE);
	}
	else if(  targetId >= IDO_SCRIPT_MENU00 && targetId <= IDO_SCRIPT_MENU99 )
	{
		#if SJ_USE_SCRIPTS
			SjSee::OnGlobalEmbedding(SJ_PERSISTENT_MENU_ENTRY, targetId-IDO_SCRIPT_MENU00);
		#endif
	}
	else if(  targetId >= IDO_SCRIPTCONFIG_MENU00 && targetId <= IDO_SCRIPTCONFIG_MENU99 )
	{
		#if SJ_USE_SCRIPTS
			SjSee::OnGlobalEmbedding(SJ_PERSISTENT_CONFIG_BUTTON, targetId-IDO_SCRIPTCONFIG_MENU00);
		#endif
	}
	else if( targetId >= IDM_FIRST && targetId <= IDM_LAST )
	{
		m_browser->OnModuleUserId(targetId);
	}
	else if( targetId >= IDO_ARTISTINFQUEUE00 && targetId <= IDO_ARTISTINFQUEUE49 )
	{
		if( IsAllAvailable() )
		{
			m_libraryModule->ShowArtistInfo(IDO_ARTISTINFQUEUE00, targetId);
		}
	}
	else if( targetId == IDT_VIS_TOGGLE || (targetId>=IDO_VIS_FIRST__ && targetId<=IDO_VIS_LAST__) )
	{
		if( g_visModule )
		{
			g_visModule->OnVisMenu(targetId);
		}
	}
	else
	{
		switch( targetId )
		{
			case IDT_SEARCH_BUTTON:
				if( IsOpAvailable(SJ_OP_SEARCH) )
				{
					bool focusInSearchWindow = (wxWindow::FindFocus()==m_simpleSearchInputWindow)? TRUE : FALSE;
					if( value.value == 1 )
					{
						EndOneSearch();
						if( focusInSearchWindow )
						{
							m_simpleSearchInputWindow->SetFocus();
						}
						else
						{
							m_browser->SetFocus();
						}
					}
					else
					{
						wxCommandEvent fwdEvent(wxEVT_COMMAND_TEXT_ENTER, IDO_SEARCHINPUT);
						OnSimpleSearchInput(fwdEvent);
						m_simpleSearchInputWindow->SetFocus();
						g_virtKeybd->OpenKeybd(m_simpleSearchInputWindow);
					}
				}
				break;

			case IDO_REALLYENDSEARCH:
				if( IsOpAvailable(SJ_OP_SEARCH) )
				{
					EndAllSearch();
				}
				break;

			case IDO_UNQUEUE_MARKED: // should go here, not to OnDisplayCommand(), see http://www.silverjuke.net/forum/topic-3197.html
			case IDO_UNQUEUE_ALL_BUT_MARKED:
				if( IsOpAvailable(SJ_OP_UNQUEUE) )
				{
					m_player.m_queue.UnqueueByIds(m_display.m_selectedIds, targetId==IDO_UNQUEUE_MARKED? 1 : 0, &m_player);
					UpdateDisplay();
					m_browser->RefreshSelection();
				}
				break;

			case IDO_EDITQUEUE: // should go here, not to OnDisplayCommand(), see http://www.silverjuke.net/forum/topic-3197.html
				if( IsAllAvailable() )
				{
					g_tagEditorModule->OpenTagEditor(new SjDisplayEditDlg());
				}
				break;

			case IDO_RATINGQUEUE00: // should go here, not to OnDisplayCommand(), see http://www.silverjuke.net/forum/topic-3197.html
			case IDO_RATINGQUEUE01:
			case IDO_RATINGQUEUE02:
			case IDO_RATINGQUEUE03:
			case IDO_RATINGQUEUE04:
			case IDO_RATINGQUEUE05:
				if( IsAllAvailable() && m_contextMenuClickedUrls.GetCount() > 0 )
				{
					m_libraryModule->SetRating(m_contextMenuClickedUrls, targetId-IDO_RATINGQUEUE00);
				}
				break;

			case IDO_EXPLOREQUEUE: // should go here, not to OnDisplayCommand(), see http://www.silverjuke.net/forum/topic-3197.html
				if( IsAllAvailable() && m_contextMenuClickedUrls.GetCount() )
				{
					g_tools->ExploreUrl(m_contextMenuClickedUrls[0]);
				}
				break;

			case IDO_SMOOTH:
				if( IsAllAvailable() )
				{
					SjTools::ToggleFlag(m_skinFlags, SJ_SKIN_IMG_SMOOTH);
					g_tools->m_config->Write(wxT("main/skinFlags"), m_skinFlags);
					m_browser->RefreshAll();
				}
				break;

			case IDO_SELECTALL:
				if( IsAllAvailable() )
				{
					m_columnMixer.SelectAll(TRUE);
					m_browser->RefreshSelection();
				}
				break;

			case IDO_BROWSER_RELOAD_VIEW:
				m_browser->RefreshAll();
				UpdateDisplay();
				break;

			case IDO_DEBUGSKIN_RELOAD:
				if( IsAllAvailable() )
				{
					wxBusyCursor busy;
					ReloadSkin(SJ_OP_DEF_NONKIOSK, true/*reloadScripts*/);
					m_browser->Refresh();
					m_simpleSearchInputWindow->Refresh();
				}
				break;

			case IDT_UPDATE_INDEX:
			case IDT_DEEP_UPDATE_INDEX:
				if( IsAllAvailable() || m_updateIndexAfterConstruction )
				{
					UpdateIndex(this, targetId==IDT_DEEP_UPDATE_INDEX);
				}
				break;

			case IDT_SETTINGS:
				if( IsAllAvailable() )
				{
					OpenSettings(); // use the last page used, defaults to "memory:mymusic.lib"
				}
				break;

			case IDO_SETTINGS_ADDFILES:
				if( IsAllAvailable() )
				{
					OpenSettings("memory:mymusic.lib", 0, 0);
				}
				break;

			case IDO_SETTINGS_SKINS:
			case IDO_SETTINGS_FONTNCOVER:
				if( IsAllAvailable() )
				{
					OpenSettings("memory:viewsettings.lib", 0, targetId==IDO_SETTINGS_SKINS? 0 : 1);
				}
				break;

			case IDO_SETTINGS_QUEUE:
			case IDO_SETTINGS_AUTOCTRL:
				if( IsAllAvailable() )
				{
					OpenSettings("memory:playbacksettings.lib", 0, targetId==IDO_SETTINGS_QUEUE? 0 : 2);
				}
				break;

			case IDO_CONSOLE:
				SjLogGui::OpenManually();
				break;

			case IDT_TOGGLE_KIOSK:
				g_kioskModule->ToggleRequest(SJ_KIOSKF_EXIT_KEY);
				break;

			case IDO_CORNERCLICK:
				g_kioskModule->ExitRequest(SJ_KIOSKF_EXIT_CORNER);
				break;

			case IDT_QUIT:
				if( QueryEndSession() )
				{
					DoEndSession();
				}
				break;

			case IDT_ALWAYS_ON_TOP:
				if( IsAllAvailable() )
				{
					ShowAlwaysOnTop(!IsAlwaysOnTop());
				}
				break;

			case IDT_MAIN_VOL_UP:
			case IDT_MAIN_VOL_DOWN:
			case IDT_MAIN_VOL_SLIDER:
			case IDT_MAIN_VOL_MUTE:
			case IDT_SEEK:
			case IDT_SEEK_BWD:
			case IDT_SEEK_FWD:
			case IDT_DISPLAY_COVER:
			case IDO_DISPLAY_COVER: // IDO_DISPLAY_COVER is only used for the menu item
			case IDO_DISPLAY_COVER_EXPLR:
			case IDT_DISPLAY_UP:
			case IDT_DISPLAY_DOWN:
			case IDT_DISPLAY_V_SCROLL:
			case IDT_PRELISTEN_VOL_UP:
			case IDT_PRELISTEN_VOL_DOWN:
			case IDT_CURR_TRACK:
			case IDT_NEXT_TRACK:
				OnSkinDisplayEvent(targetId, value, accelFlags);
				break;

			case IDT_SHUFFLE:
				if( IsOpAvailable(SJ_OP_EDIT_QUEUE) )
				{
					ToggleShuffle();
					SetDisplayMsg(_("Shuffle")+wxString(wxT(": "))+(GetShuffle()? _("On") : _("Off")), SDM_STATE_CHANGE_MS);
				}
				break;

			case IDT_REPEAT:
				if( IsOpAvailable(SJ_OP_REPEAT) )
				{
					ToggleRepeat();
					wxString msg(_("Repeat playlist")+wxString(wxT(": ")));
					switch(GetRepeat())
					{
						case SJ_REPEAT_OFF: msg+=_("Off"); break;
						case SJ_REPEAT_SINGLE: msg+=_("Repeat one"); break;
						case SJ_REPEAT_ALL: msg+=_("Repeat all"); break;
					}
					SetDisplayMsg(msg, SDM_STATE_CHANGE_MS);
				}
				break;

			case IDT_CURR_TIME:
				if( value.value!=SJ_SUBITEM_TEXT/*_MOUSEUP*/ )
					break;
				/*else fall through*/
			case IDT_TOGGLE_TIME_MODE:
			case IDO_TOGGLE_TIME_MODE2:
				m_showRemainingTime = !m_showRemainingTime;
				UpdateDisplay();
				UpdateMenuBarQueue();
				break;

			case IDO_DISPLAY_TOTAL_TIME:
			case IDO_DISPLAY_TRACKNR:
			case IDO_DISPLAY_ARTIST:
			case IDO_DISPLAY_AUTOPLAY:
			case IDO_DISPLAY_PREFERALBCV:
				if( IsAllAvailable() )
				{
					if( targetId==IDO_DISPLAY_TOTAL_TIME )  { SjTools::ToggleFlag(m_skinFlags, SJ_SKIN_SHOW_DISPLAY_TOTAL_TIME); }
					if( targetId==IDO_DISPLAY_TRACKNR )     { SjTools::ToggleFlag(m_skinFlags, SJ_SKIN_SHOW_DISPLAY_TRACKNR); }
					if( targetId==IDO_DISPLAY_ARTIST )      { SjTools::ToggleFlag(m_skinFlags, SJ_SKIN_SHOW_DISPLAY_ARTIST); }
					if( targetId==IDO_DISPLAY_AUTOPLAY )    { SjTools::ToggleFlag(m_skinFlags, SJ_SKIN_SHOW_DISPLAY_AUTOPLAY); }
					if( targetId==IDO_DISPLAY_PREFERALBCV ) { SjTools::ToggleFlag(m_skinFlags, SJ_SKIN_PREFER_TRACK_COVER); }
					g_tools->m_config->Write(wxT("main/skinFlags"), m_skinFlags);
					UpdateDisplay();
					UpdateMenuBarQueue();
				}
				break;

			case IDO_SAMEZOOMINALLVIEWS:
				if( IsAllAvailable() )
				{
					SjTools::ToggleFlag(g_accelModule->m_flags, SJ_ACCEL_SAME_ZOOM_IN_ALL_VIEWS);
					g_tools->m_config->Write(wxT("main/accelFlags"), g_accelModule->m_flags);
					UpdateViewMenu();
				}
				break;

			case IDT_PLAY:
			case IDT_PAUSE:
				if(  m_player.m_queue.GetCount()==0
				 && !m_player.IsPlaying() )
				{
					OnSkinTargetEvent(IDT_ENQUEUE_LAST, value, 0); // recursive call
					if( !(g_accelModule->m_flags&SJ_ACCEL_START_PLAYBACK_ON_ENQUEUE) && m_player.m_queue.GetCount() && !m_player.IsPlaying() )
					{
						PlayOrPause();// playback not started on enqueue, start it here
						return;
					}

					if( !m_player.IsPlaying() )
					{
						m_autoCtrl.DoAutoPlayIfEnabled(true /*ignoreTimeouts*/);

						if( !m_player.IsPlaying() )
						{
							SetDisplayMsg(_("Select a track, then start playback."), 2000);
						}
					}
					return;
				}

				if( IsOpAvailable(SJ_OP_PLAYPAUSE) )
				{
					PlayOrPause(); // this also sets m_haltedManually
				}
				break;

			case IDT_STOP:
				if( IsOpAvailable(SJ_OP_PLAYPAUSE) )
				{
					Stop(false);
				}
				break;

			case IDT_STOP_AFTER_THIS_TRACK:
				if( IsOpAvailable(SJ_OP_PLAYPAUSE) )
				{
					StopAfterThisTrack(!StopAfterThisTrack());
					SetDisplayMsg(_("Stop after this track")+wxString(wxT(": "))+(StopAfterThisTrack()? _("On") : _("Off")), SDM_STATE_CHANGE_MS);
				}
				break;

			case IDT_STOP_AFTER_EACH_TRACK:
				if( IsOpAvailable(SJ_OP_PLAYPAUSE) )
				{
					StopAfterEachTrack(!StopAfterEachTrack());
					SetDisplayMsg(_("Stop after each track")+wxString(wxT(": "))+(StopAfterEachTrack()? _("On") : _("Off")), SDM_STATE_CHANGE_MS);
				}
				break;

			case IDT_PLAY_NOW_ON_DBL_CLICK:
				if( IsOpAvailable(SJ_OP_EDIT_QUEUE) )
				{
					SjTools::ToggleFlag(g_accelModule->m_flags, SJ_ACCEL_PLAY_NOW_ON_DBL_CLICK);
					g_tools->m_config->Write(wxT("main/accelFlags"), g_accelModule->m_flags);

					SetSkinTargetValue(IDT_PLAY_NOW_ON_DBL_CLICK, (g_accelModule->m_flags&SJ_ACCEL_PLAY_NOW_ON_DBL_CLICK)? 1 : 0);
					SetDisplayMsg(_("Double click play tracks at once")+wxString(wxT(": "))+((g_accelModule->m_flags&SJ_ACCEL_PLAY_NOW_ON_DBL_CLICK)? _("On") : _("Off")), SDM_STATE_CHANGE_MS);
				}
				break;

			case IDO_START_PB_ON_ENQUEUE:
				if( IsOpAvailable(SJ_OP_PLAYPAUSE) )
				{
					SjTools::ToggleFlag(g_accelModule->m_flags, SJ_ACCEL_START_PLAYBACK_ON_ENQUEUE);
					g_tools->m_config->Write(wxT("main/accelFlags"), g_accelModule->m_flags);

					if( m_playbackMenu )
					{
						m_playbackMenu->Check(IDO_START_PB_ON_ENQUEUE, (g_accelModule->m_flags&SJ_ACCEL_START_PLAYBACK_ON_ENQUEUE)!=0);
					}

					SetDisplayMsg(_("Start playback on first enqueue")+wxString(wxT(": "))+((g_accelModule->m_flags&SJ_ACCEL_START_PLAYBACK_ON_ENQUEUE)? _("On") : _("Off")), SDM_STATE_CHANGE_MS);
				}
				break;

			case IDT_TOGGLE_REMOVE_PLAYED:
				if( IsOpAvailable(SJ_OP_EDIT_QUEUE) )
				{
					long queueFlags, boredomTrackMinutes, boredomArtistMinutes;
					g_mainFrame->m_player.m_queue.GetQueueFlags(queueFlags, boredomTrackMinutes, boredomArtistMinutes);
					SjTools::ToggleFlag(queueFlags, SJ_QUEUEF_REMOVE_PLAYED);
					g_mainFrame->m_player.m_queue.SetQueueFlags(queueFlags, boredomTrackMinutes, boredomArtistMinutes);

					SetSkinTargetValue(IDT_TOGGLE_REMOVE_PLAYED, (queueFlags&SJ_QUEUEF_REMOVE_PLAYED)? 1 : 0);
					SetDisplayMsg(_("Remove played tracks from queue")+wxString(wxT(": "))+((queueFlags&SJ_QUEUEF_REMOVE_PLAYED)? _("On") : _("Off")), SDM_STATE_CHANGE_MS);
				}
				break;

			case IDT_PREV:
				if( IsOpAvailable(SJ_OP_EDIT_QUEUE) )
				{
					GotoPrev();
				}
				break;

			case IDT_NEXT:
			case IDT_FADE_TO_NEXT:
				if( IsOpAvailable(SJ_OP_EDIT_QUEUE) )
				{
					GotoNextRegardAP(targetId==IDT_FADE_TO_NEXT || accelFlags!=0);
				}
				break;

			case IDT_ENQUEUE_LAST:
			case IDT_ENQUEUE_NEXT:
			case IDT_ENQUEUE_NOW:
				{
					GotBrowserInputFromUser();
					wxArrayString urls;
					m_columnMixer.GetSelectedUrls(urls);
					long enqueuePos = -1;

					if( targetId == IDT_ENQUEUE_NEXT && IsOpAvailable(SJ_OP_EDIT_QUEUE) )
					{
						enqueuePos = -2;
					}

					if( targetId == IDT_ENQUEUE_NOW && IsOpAvailable(SJ_OP_EDIT_QUEUE) )
					{
						enqueuePos = -3;
					}

					Enqueue(urls, enqueuePos, TRUE/*verified*/);

					m_libraryModule->UpdateMenuBar();
				}
				break;

			case IDT_MORE_FROM_CURR_ALBUM:
			case IDT_MORE_FROM_CURR_ARTIST:
				{
					GotBrowserInputFromUser();
					wxArrayString urls = m_libraryModule->GetMoreFrom(m_player.GetUrlOnAir(), targetId, &m_player.m_queue);
					if( urls.GetCount() )
					{
						long enqueuePos = -1; // last, if queue editing is not available

						if( IsOpAvailable(SJ_OP_EDIT_QUEUE) )
						{
							enqueuePos = -2; // next, this is the normal usage
						}

						Enqueue(urls, enqueuePos, TRUE/*verified*/);

						m_libraryModule->UpdateMenuBar();
					}
					else
					{
						wxString msg(targetId==IDT_MORE_FROM_CURR_ALBUM? _("More from current album") : _("More from current artist"));
						msg += wxT(": ");
						msg += _("No more tracks.");
						SetDisplayMsg(msg, 3000);
					}
				}
				break;

			case IDT_PRELISTEN:
				if( IsOpAvailable(SJ_OP_PRELISTEN) )
				{
					GotBrowserInputFromUser();
					wxArrayString urls;
					m_columnMixer.GetSelectedUrls(urls);

					m_player.TogglePrelisten(urls);
					UpdateDisplay();

					m_libraryModule->UpdateMenuBar();
				}
				break;

			case IDT_UNQUEUE:
				if( IsOpAvailable(SJ_OP_UNQUEUE) )
				{
					wxArrayString urls;
					m_columnMixer.GetSelectedUrls(urls);
					m_player.m_queue.UnqueueByUrls(urls, &m_player);
					UpdateDisplay();
					m_browser->RefreshSelection();
					m_libraryModule->UpdateMenuBar();
				}
				break;

			case IDT_UNQUEUE_ALL:
				if( IsOpAvailable(SJ_OP_UNQUEUE) )
				{
					bool doClear = TRUE;
					if(  m_player.m_queue.GetCount() > 1 )
					{
						if( g_accelModule->YesNo(wxString::Format(
						  // TRANSLATORS: %i will be replaced by the number of tracks
						  wxPLURAL("Remove %i track from the queue and stop playback?", "Remove %i tracks from the queue and stop playback?", m_player.m_queue.GetCount()),
						  (int)m_player.m_queue.GetCount()),
						  _("Clear playlist"), this, SJ_ACCEL_ASK_ON_CLEAR_PLAYLIST) != wxYES )
						{
							doClear = FALSE;
						}
					}

					if( doClear )
					{
						m_player.m_queue.UnqueueAll(&m_player, TRUE);
						UpdateDisplay();
						m_browser->RefreshSelection();
						m_moduleSystem.BroadcastMsg(IDMODMSG_TRACK_ON_AIR_CHANGED);
					}
				}
				break;

			case IDT_SAVE_PLAYLIST:
				if( m_player.m_queue.GetCount() && IsAllAvailable() )
				{
					m_player.m_queue.SaveAsDlg(this);
				}
				break;

			case IDT_APPEND_FILES: // before 1.23beta1, we checked for appending whether the playlist is empty and did nothing in this case -- I think this is not useful esp. when using a shortcut
			case IDT_OPEN_FILES:
				if( IsAllAvailable() && g_openFilesModule )
				{
					g_openFilesModule->OpenDialog(targetId==IDT_APPEND_FILES);
				}
				break;

			case IDO_ABOUT_OPEN_WWW:
				if( IsAllAvailable() )
				{
					g_tools->ExploreHomepage(SJ_HOMEPAGE_INDEX);
				}
				break;

			case IDO_ABOUT:
				g_helpModule->OpenTopic(this, SJ_HELP_TOPIC_ABOUT);
				break;

			case IDO_ONLINE_HELP:
				if( IsAllAvailable() )
				{
					#if defined(PKGDOCDIR)
						g_tools->ExploreUrl(wxT(PKGDOCDIR));
					#elif defined(__WXMAC__)
						g_tools->ExploreUrl(SjTools::GetGlobalAppDataDir()+"docs");
					#else
						g_tools->ExploreUrl(g_tools->GetSilverjukeProgramDir()+wxString(wxT("docs/")));
					#endif
				}
				break;

			case IDT_GOTO_CURR:
			case IDO_GOTO_CURR_MARK:
				m_autoCtrl.m_stateGotoCurrClickedTimestamp = SjTools::GetMsTicks();
				GotDisplayInputFromUser();
				// fall through

			case IDO_GOTOCURRAUTO:
				{
					wxString url;
					if( targetId == IDO_GOTO_CURR_MARK && m_contextMenuClickedUrls.GetCount()==1 )
					{
						url = m_contextMenuClickedUrls[0];
					}
					else
					{
						url = m_player.m_queue.GetUrlByPos(-1);
					}

					if( !url.IsEmpty() )
					{
						if( targetId!=IDO_GOTOCURRAUTO )
						{
							g_visModule->StopVisIfOverWorkspace();
						}

						if( !m_browser->GotoUrl(url) )
						{
							EndSimpleSearch();
							m_browser->GotoUrl(url);
						}
					}
				}
				break;

			case IDT_ADV_SEARCH:
				if( g_advSearchModule && IsAllAvailable() )
				{
					g_advSearchModule->OpenDialog();
				}
				break;

			case IDMODMSG_WINDOW_SIZED_MOVED:
				m_moduleSystem.BroadcastMsg((int)targetId);
				break;
		}
	}

	#ifdef SJHOOK_ON_MAIN_EVENT
	SJHOOK_ON_MAIN_EVENT
	#endif
}


void SjMainFrame::OnPaste(wxCommandEvent& event)
{
	if( IsAllAvailable() )
	{
		if( wxTheClipboard->Open() )
		{
			SjDataObject data(wxDragCopy);
			{
				wxBusyCursor busy;
				if( wxTheClipboard->IsSupported(wxDF_FILENAME) )
				{
					data.AddFileData();
					wxTheClipboard->GetData(*(data.m_fileData));
				}
				else if( wxTheClipboard->IsSupported(wxDF_BITMAP) )
				{
					data.AddBitmapData();
					wxTheClipboard->GetData(*(data.m_bitmapData));
				}
				else if( wxTheClipboard->IsSupported(wxDF_TEXT) )
				{
					wxTextDataObject text;
					wxTheClipboard->GetData(text);
					if( ::wxFileExists(text.GetText()) )
					{
						data.AddFileData();
						data.m_fileData->AddFile(text.GetText());
					}
				}
			}

			// use the data _before_ closing the clipboard (may get lost otherwise)
			wxPoint pt;
			if( event.GetId() == IDO_PASTE_USING_COORD )
			{
				pt = m_pasteCoord;
			}
			else
			{
				pt = ScreenToClient(::wxGetMousePosition());
			}
			OpenData(&data, SJ_OPENFILES_ENQUEUE, pt.x, pt.y);

			wxTheClipboard->Close();
		}
	}
}


void SjMainFrame::OnFwdToSkin(wxCommandEvent& event)
{
	SjSkinValue dummy;
	OnSkinTargetEvent(event.GetId(), dummy, 0);
}


void SjMainFrame::OnFwdToModules(wxCommandEvent& event)
{
	if( m_mainApp->IsInShutdown() )
	{
		return; // cancel pending events
	}

	int modMsg = (int)event.GetId();

	SjPlayer* testPlayer = (SjPlayer*)event.GetClientData();
	if( testPlayer != &m_player && testPlayer != NULL )
	{
		return; // this message comes eg. from another player
	}

	if( modMsg == IDMODMSG_TRACK_ON_AIR_CHANGED )
	{
		// new track or a manual stop/pause
		long queuef = m_player.m_queue.GetQueueFlags();
		if( queuef&SJ_QUEUEF_REMOVE_PLAYED )
		{
			m_player.m_queue.UnqueuePlayed();
		}

		UpdateDisplay();
	}
	else if( modMsg == IDMODMSG_PLAYER_STOPPED_BY_EOQ )
	{
		// this message is ONLY send if the player is stopped
		// as the end of the queue is reached (eg. if repeat is on, this is never send)
		// when we receive this message, we've received IDMODMSG_TRACK_ON_AIR_CHANGED before.
		if( m_player.IsStopped()
		 && m_player.m_queue.MoveToTopOnEoq() )
		{
			m_player.GotoAbsPos(0);
			UpdateDisplay();
		}
	}
	else if( modMsg == IDMODMSG_VIS_STATE_CHANGED )
	{
		// vis. state changed
		SetSkinTargetValue(IDT_VIS_TOGGLE, g_visModule->IsVisStarted()? 1 : 0);
		if( !g_visModule->IsVisStarted() )
		{
			m_autoCtrl.m_stateStopVisTimestamp = SjTools::GetMsTicks();
		}
	}

	m_moduleSystem.BroadcastMsg(modMsg);
}


void SjMainFrame::OnFwdToPlayer(wxCommandEvent& event)
{
	/* This function just forward command events between IDPLAYER_FIRST and IDPLAYER_LAST
	 * to the player given in the client data. This forwarding is needed to trigger
	 * the player's main thread from the decoding thread.  The meaning of the IDs is private
	 * to SjPlayer and not of any interest here.
	 *
	 * Using m_player is safe as we may have several players for several windows but only one player per window.
	 */
	wxASSERT( wxThread::IsMain() );
	if( SjBusyInfo::InYield() )
	{
		// we cannot process this event now as we're called from within wxYield()
		// and forwarding the event may result in opening a new http stream which
		// may also require wxYield(). So delay the event until the BusyInfo is gone.
		m_autoCtrl.m_pendingEvents.Add(event.Clone());
	}
	else
	{
		m_player.ReceiveSignal(event.GetId(), (uintptr_t)event.GetClientData());
	}
}


void SjMainFrame::OnCloseWindow(wxCloseEvent& event)
{
	if( m_player.m_queue.GetQueueFlags()&SJ_QUEUEF_RESUME )
	{
		m_player.SaveToResumeFile(); // this should be done _very_ first, the OS may kill us at any time and it is a good idea to have the resume file ready then.
	}

	if( !event.CanVeto() || QueryEndSession() )
	{
		DoEndSession();
	}
	else
	{
		event.Veto();
	}
}


void SjMainFrame::OnIconizeWindow(wxIconizeEvent& event)
{
	/* Some words about iconizing the window:

	If we would iconize manually (we've done this in former times frnom the skin)
	we just call  Iconize(!IsIconized()) from OnSkinTargetEvent/IDT_MINIMIZE which
	will result in a wxEVT_ICONIZE catched by OnIconizeWindow(). The same event
	is also emmited by iconizing due the system, so we're here in OnIconizeWindow()
	whenever the iconize state changes. In this function we're posting
	ourself a IDMODMSG_* message that will be forwarded to all modules (esp.
	the System Tray). We do not send this message directly to make sure the
	window is really iconized when the IDMODMSG_* is send. */

	if( event.IsIconized() )
	{
		if( g_visModule )
		{
			g_visModule->StopVis();
		}
	}

	QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, event.IsIconized()? IDMODMSG_WINDOW_ICONIZED : IDMODMSG_WINDOW_UNICONIZED));

	event.Skip();
}


void SjMainFrame::OnEsc(wxCommandEvent& event)
{
	if( g_visModule->IsVisStarted() && g_visModule->IsOverWorkspace() )
	{
		g_visModule->StopVis();
	}
	else if( HasAnySearch() )
	{
		bool focusInSearchWindow = (wxWindow::FindFocus()==m_simpleSearchInputWindow)? TRUE : FALSE;
		EndOneSearch();
		if( focusInSearchWindow )
		{
			m_simpleSearchInputWindow->SetFocus();
		}
		else
		{
			m_browser->SetFocus();
		}
	}
	else if( m_columnMixer.IsAnythingSelected() )
	{
		m_columnMixer.SelectAll(FALSE);
		m_browser->RefreshSelection();
	}
}


void SjMainFrame::OnTab(wxCommandEvent& event)
{
	if( wxWindow::FindFocus() == m_simpleSearchInputWindow )
	{
		m_browser->SetFocus();
	}
	else
	{
		m_simpleSearchInputWindow->SetFocus();
	}
}


void SjMainFrame::OnMouseWheel(wxMouseEvent& e)
{
	// just forward the mouse wheel event to the browser window where it is used.
	// (needed for Linux/Mac; on Windows mouse wheel events seems to go to the focused window)
	m_browser->OnMouseWheel(e);
}


/*******************************************************************************
 * Handling Tool Tips
 ******************************************************************************/


#if SJ_USE_TOOLTIPS
wxString SjMainFrameToolTipProvider::GetText(long& flags)
{
	wxString    hint;
	wxString    info;

	if( m_targetId >= IDT_DISPLAY_LINE_FIRST && m_targetId <= IDT_DISPLAY_LINE_LAST )
	{
		long displayLine = (m_targetId-IDT_DISPLAY_LINE_FIRST);
		long queuePos = g_mainFrame->m_display.m_firstLineQueuePos + displayLine;
		long queuePlayingPos = g_mainFrame->m_player.m_queue.GetCurrPos();
		if( queuePos >= 0 && queuePos < g_mainFrame->m_player.m_queue.GetCount() )
		{
			if( m_subitem == SJ_SUBITEM_ICONLEFT && queuePos != queuePlayingPos )
			{
				long entryFlags = g_mainFrame->m_player.m_queue.GetFlags(queuePos);
				long entryPlayCount = g_mainFrame->m_player.m_queue.GetPlayCount(queuePos);
				if( g_mainFrame->m_player.m_queue.WasPlayed(queuePos) )
				{
					hint = _("Track played, click to reset");
				}
				else if( entryFlags & SJ_PLAYLISTENTRY_MOVED_DOWN )
				{
					hint = _("Boredom avoided: Track moved down");
				}
			}
			else if( m_subitem == SJ_SUBITEM_ICONRIGHT )
			{
				hint = _("Remove this track");
			}
			else if( m_subitem == SJ_SUBITEM_TIME )
			{
				hint = _("Toggle time mode");
			}
		}
	}
	else if( m_targetId >= IDT_WORKSPACE_GOTO_A && m_targetId <= IDT_WORKSPACE_GOTO_0_9 )
	{
		hint.Printf(_("Go to \"%s\""), SjColumnMixer::GetAzDescr(m_targetId).c_str());
	}
	else if( m_targetId >= IDT_NUMPAD_FIRST && m_targetId <= IDT_NUMPAD_LAST )
	{
		; /*no tooltip*/
	}
	else switch( m_targetId )
		{
			case IDT_CURR_TIME:
				hint = _("Toggle time mode");
				break;

			case IDT_SEARCH_BUTTON:
			case IDT_SEARCH_INFO:
				hint = g_mainFrame->HasAnySearch()? _("End search") : _("Search");
				hint += wxT(" - ");
				hint += _("click right for options");
				break;

			case IDT_MAIN_VOL_SLIDER:
				hint = _("Volume");
				hint += wxString::Format(wxT(" (%i%%)"), (int)( (g_mainFrame->m_player.GetMainVol()*100)/255 ));
				break;

			case IDT_DISPLAY_COVER:
				if( g_mainFrame->m_player.m_queue.GetCount() )
				{
					hint = _("Double-click to enlarge the cover");
				}
				break;

			case IDT_WORKSPACE_H_SCROLL:
			case IDT_WORKSPACE_LINE_LEFT:
			case IDT_WORKSPACE_LINE_RIGHT:
			case IDT_WORKSPACE_LINE_UP:
			case IDT_WORKSPACE_LINE_DOWN:
			case IDT_WORKSPACE_PAGE_LEFT:
			case IDT_WORKSPACE_PAGE_RIGHT:
			case IDT_WORKSPACE_PAGE_UP:
			case IDT_WORKSPACE_PAGE_DOWN:   /*no tooltip, no fall-through as before 1.11...*/   break;

			case IDT_NEXT:                  hint = g_mainFrame->GetNextMenuTitle();             break;
			case IDT_QUIT:                  hint = wxString::Format(_("Close this window and exit %s"), SJ_PROGRAM_NAME);   break;
			case IDT_MAINMENU:              hint = _("Open main menu");                         break;
			case IDT_SHUFFLE:               hint = _("Toggle shuffle mode");                    break;
			case IDT_REPEAT:                hint = _("Repeat all/repeat current");              break;
			default:                        hint = g_accelModule->GetCmdNameById(m_targetId, true/*short name*/);   break;
		}

	return hint;
}
#endif


/*******************************************************************************
 * Handling Zoom and Font
 ******************************************************************************/


long SjMainFrame::CorrectColumnWidth(long columnWidth)
{
	if( columnWidth < SJ_MIN_COLUMN_WIDTH ) return SJ_MIN_COLUMN_WIDTH;
	if( columnWidth > SJ_MAX_COLUMN_WIDTH ) return SJ_MAX_COLUMN_WIDTH;
	return columnWidth;
}


long SjMainFrame::CorrectCoverHeight(long coverHeight)
{
	if( coverHeight < 10  ) return 10;
	if( coverHeight > 100 ) return 100;
	return coverHeight;
}


long SjMainFrame::CorrectFontSize(long fontsize)
{
	if( fontsize < SJ_MIN_FONT_SIZE ) return SJ_MIN_FONT_SIZE;
	if( fontsize > SJ_MAX_FONT_SIZE ) return SJ_MAX_FONT_SIZE;
	return fontsize;
}


void SjMainFrame::CalcCurrFontSizesFromZoom()
{
	// get permille zoom value
	static const long zoom2permille[1+7] =
	{	556,
		667, 778, 889, 1000, 1111, 1222, 1333
	};
	long permille;

	wxASSERT( m_currZoom >= 0 && m_currZoom <= 6 );

	permille = zoom2permille[m_currZoom+1];

	// calculate normal font size from permille and base font size
	m_currFontSize = CorrectFontSize((m_baseFontSize*permille) / 1000);

	// calculate small font size from permille and base font size
	m_currFontSizeSmall = CorrectFontSize((m_baseFontSize*zoom2permille[m_currZoom]) / 1000);

	if( m_currFontSizeSmall == m_currFontSize ) {
		m_currFontSizeSmall = CorrectFontSize(m_currFontSize - 1);
	}

	// calculate column width from permille and base column width
	m_currColumnWidth = CorrectColumnWidth((m_baseColumnWidth*permille) / 1000);

	// calculate spaces from column width
	m_currColumnXSpace = m_currColumnWidth / 8;
	m_currColumnYSpace = m_currColumnWidth / 14;

	// calculate cover height (=width) from column width and percentage
	if( m_baseCoverHeight < 99 )
	{
		m_currCoverHeight = (m_currColumnWidth*m_baseCoverHeight) / 100;
		m_currCoverHeight = (m_currCoverHeight/5) * 5;
		if( m_currCoverHeight < 32 ) { m_currCoverHeight = 32; }
		if( m_currCoverHeight > m_currColumnWidth ) { m_currCoverHeight = m_currColumnWidth; }
	}
	else
	{
		m_currCoverHeight = m_currColumnWidth;
	}
	m_currCoverWidth = m_currCoverHeight;
}


void SjMainFrame::CreateFontObjects()
{
	m_baseStdFont       = wxFont(m_baseFontSize,        wxSWISS, wxNORMAL, wxNORMAL, FALSE, m_baseFontFace);
	m_currStdFont       = wxFont(m_currFontSize,        wxSWISS, wxNORMAL, wxNORMAL, FALSE, m_baseFontFace);
	m_currSmallFont     = wxFont(m_currFontSizeSmall,   wxSWISS, wxNORMAL, wxNORMAL, FALSE, m_baseFontFace);
	m_currSmallBoldFont = wxFont(m_currFontSizeSmall,   wxSWISS, wxNORMAL, wxBOLD,   FALSE, m_baseFontFace);
	m_currBoldFont      = wxFont(m_currFontSize,        wxSWISS, wxNORMAL, wxBOLD,   FALSE, m_baseFontFace);
	m_currBoldItalicFont= wxFont(m_currFontSize,        wxSWISS, wxITALIC, wxBOLD,   FALSE, m_baseFontFace);
}


long SjMainFrame::CorrectZoom(long zoom)
{
	if( zoom < SJ_ZOOM_MIN ) return SJ_ZOOM_MIN;
	if( zoom > SJ_ZOOM_MAX ) return SJ_ZOOM_MAX;
	return zoom;
}


void SjMainFrame::SetZoom__(long newZoom, bool redraw)
{
	newZoom = CorrectZoom(newZoom);

	if( newZoom != m_currZoom )
	{
		g_visModule->StopVisIfOverWorkspace();

		m_currZoom = newZoom;

		CalcCurrFontSizesFromZoom();
		CreateFontObjects();

		if( redraw )
		{
			m_browser->RefreshAll();
		}
	}
}


void SjMainFrame::SetFontNCoverBase(const wxString& fontFace, long fontSize, long columnWidth, long coverHeight)
{
	fontSize= CorrectFontSize(fontSize);
	columnWidth  = CorrectColumnWidth(columnWidth);
	coverHeight = CorrectCoverHeight(coverHeight);
	m_currZoom = CorrectZoom(m_currZoom); // just for safety, normally the zoom should be fine

	if( m_baseFontFace != fontFace
	        || m_baseFontSize != fontSize
	        || m_baseColumnWidth != columnWidth
	        || m_baseCoverHeight != coverHeight )
	{
		m_baseFontFace      = fontFace;
		m_baseFontSize      = fontSize;
		m_baseColumnWidth   = columnWidth;
		m_baseCoverHeight   = coverHeight;

		g_tools->m_config->Write(wxT("main/fontFace"), m_baseFontFace);
		g_tools->m_config->Write(wxT("main/fontSize"), m_baseFontSize);
		g_tools->m_config->Write(wxT("main/fontCoverSize")/*historical*/, m_baseColumnWidth);
		g_tools->m_config->Write(wxT("main/fontCoverHeight"), m_baseCoverHeight);
		g_tools->m_config->Flush();

		CalcCurrFontSizesFromZoom();
		CreateFontObjects();
		m_browser->RefreshAll();
	}
}


void SjMainFrame::SetDefaultWindowSize()
{
	// set wanted window size
	wxRect displRect = SjDialog::GetDisplayWorkspaceRect(NULL);
	wxRect wantedRect;

	wantedRect = wxRect(displRect.x + displRect.width/20,     displRect.y + displRect.height/20,
	                    displRect.width - displRect.width/10, displRect.height - displRect.height/4);

	// just set the default size; the size may be corrected
	// when a skin gets loaded
	SetSize(wantedRect);
}


/*******************************************************************************
 * Misc.
 ******************************************************************************/


bool SjMainFrame::IsAnyDialogOpened()
{
	if(  SjSettingsModule::IsDialogOpen()
	 || (g_openFilesModule && g_openFilesModule->IsDialogOpened() )
	 || (g_helpModule && g_helpModule->IsHelpOpen() )
	 || (g_tagEditorModule && g_tagEditorModule->IsTagEditorOpened())
	 || (g_advSearchModule && g_advSearchModule->IsDialogOpen())
	 || (g_virtKeybd && g_virtKeybd->IsKeybdOpened())
	 || (g_kioskModule && g_kioskModule->IsPasswordDlgOpened()) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/*******************************************************************************
 * Handling the search input field
 ******************************************************************************/


void SjMainFrame::SetSkinAzValues(int az)
{
	// this selects/deselects the goto a-z buttons in the skin.
	//
	// if the target does not exist in the skin, the button left of the
	// letter is selected.

	wxASSERT(az==0 || (az>='a' && az<=('z'+1)));

	int testId, idToMark = IDT_WORKSPACE_GOTO_A;
	{
		int     sollId = IDT_WORKSPACE_GOTO_A + (az - 'a');
		bool    markNext = false;
		bool    hasAnyTarget = false;
		for( testId = IDT_WORKSPACE_GOTO_0_9; testId >= IDT_WORKSPACE_GOTO_A; testId-- )
		{
			bool hasThisTarget = HasSkinTarget(testId);
			if( hasThisTarget )
			{
				hasAnyTarget = true;
			}

			if( testId == sollId || markNext )
			{
				if( testId == IDT_WORKSPACE_GOTO_A || hasThisTarget )
				{
					idToMark = hasAnyTarget? testId : sollId;
					break;
				}
				else
				{
					markNext = true;
				}
			}
		}
	}

	SjSkinValue value;
	for( testId = IDT_WORKSPACE_GOTO_0_9; testId >= IDT_WORKSPACE_GOTO_A; testId-- )
	{
		value.value = testId==idToMark? 1 : 0;
		SetSkinTargetValue(testId, value);
	}
}


wxString SjMainFrame::NormalizeSearchText(const wxString& text__)
{
	wxString text(text__);
	text.Trim(TRUE);
	text.Trim(FALSE);
	while( text.Find(wxT("  ")) != -1 )
	{
		text.Replace(wxT("  "), wxT(" "));
	}
	return text;
}


void SjMainFrame::OnSimpleSearchInput(wxCommandEvent& event)
{
	if( !g_advSearchModule || !m_simpleSearchInputFromUser ) return;

	wxString text = NormalizeSearchText(m_simpleSearchInputWindow->GetValue());
	GotInputFromUser();

	// This is a real hack - under MSW the curor disappears on
	// text input (it reappaers on mouse move, and it does not disappear
	// when typing controls as backspace... i don't know, however,
	// using WarpPointer() makes it appearing again :-|
	#ifdef __WXMSW__
		wxPoint mousePos = ScreenToClient(::wxGetMousePosition());
		WarpPointer(mousePos.x, mousePos.y);
	#endif

	// set skin value if "search while typing" is disabled
	// (a click on the magnifier will start the search)
	if( !(g_advSearchModule->m_flags&SJ_SEARCHFLAG_SEARCHWHILETYPING) )
	{
		SetSkinTargetValue(IDT_SEARCH_BUTTON,
		                   (!text.IsEmpty() && text==m_search.m_simple.GetWords())? 1 : 0);
	}

	// prepare performung the search
	WXTYPE eventType = event.GetEventType();
	if( (eventType == wxEVT_COMMAND_TEXT_UPDATED && (g_advSearchModule->m_flags&SJ_SEARCHFLAG_SEARCHWHILETYPING))
	 ||  eventType == wxEVT_COMMAND_TEXT_ENTER )
	{
		// avoid recursion and disable further calls of this functions
		static bool inSearchInput = FALSE;
		if( !inSearchInput )
		{
			inSearchInput = TRUE;

			m_simpleSearchInputTimer.Stop();

			if( !text.IsEmpty() && (eventType==wxEVT_COMMAND_TEXT_UPDATED || m_inPerformingSearch) )
			{
				// delayed search by timer: use the average search time plus 10% as delay
				long delay = (m_allSearchMs / m_allSearchCount);
				delay += delay/10;
				if( delay < 70) { delay = 70; }     // few persons type faster
				if( delay > 1600) { delay = 1600; } // few persons wait longer

				// start the timer
				m_simpleSearchDelayedText = text;
				m_simpleSearchInputTimer.Start(delay, TRUE /*one shot*/);
			}
			else
			{
				// undelayed search
				SetSearch(SJ_SETSEARCH_SETSIMPLE|
				          SJ_SETSEARCH_NOTEXTCTRLUPDATE, text);
			}

			inSearchInput = FALSE;
		}
	}
}
void SjMainFrame::OnSimpleSearchInputTimer(wxTimerEvent& event)
{
	SetSearch(SJ_SETSEARCH_SETSIMPLE|
	          SJ_SETSEARCH_NOTEXTCTRLUPDATE|SJ_SETSEARCH_NOAUTOHISTORYADD, m_simpleSearchDelayedText);
}


void SjMainFrame::UpdateSearchInfo(long numTracksRemoved)
{
	if( numTracksRemoved )
	{
		m_searchStat.m_advResultCount -= numTracksRemoved;
		m_searchStat.m_totalResultCount -= numTracksRemoved;
		m_searchStat.m_mbytes = 0;
		m_searchStat.m_seconds = 0;
		// m_moduleSystem.BroadcastMsg(IDMODMSG_SEARCH_CHANGED); -- not needed
	}

	SjSkinValue v;
	v.value = SJ_VFLAG_CENTER;
	if( !m_search.m_simple.IsSet()
	 && (!m_search.m_adv.IsSet() || m_searchStat.m_totalResultCount>0 || m_libraryModule->GetUnmaskedTrackCount()==0) )
	{
		if( m_search.m_adv.IsSet()
		        && (IsOpAvailable(SJ_OP_MUSIC_SEL) || IsOpAvailable(SJ_OP_MUSIC_SEL_GENRE)) )
		{
			v.string = wxString::Format(_("Search in \"%s\""), m_search.m_adv.GetName().c_str());
		}
		else
		{
			v.string = _("Search");
		}
	}
	else
	{
		v.string = SjAdvSearchModule::FormatResultString(
		               m_searchStat.m_totalResultCount,
		               m_search.m_adv.IsSet()?
		               m_search.m_adv.GetName() : wxString());
	}

	SetSkinTargetValue(IDT_SEARCH_INFO, v);
}


void SjMainFrame::SetSearch(long flags, const wxString& newSimpleSearch, const SjAdvSearch* newAdvSearch)
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	// avoid recursion, this flag is also checked by OnSimpleSearchInput()
	if( !m_inPerformingSearch )
	{
		m_inPerformingSearch = TRUE;

		static unsigned long    lastSearchTime = 0;
		unsigned long           thisSearchTime = SjTools::GetMsTicks();
		bool                    searchModified = FALSE;
		bool                    useHourglass = FALSE;
		bool                    deepSearch = FALSE;
		bool                    gatherStatistics = FALSE;

		// save pending stuff
		m_libraryModule->SavePendingData();

		// add previous search to the history if SJ_HISTORY_DELAY_MS have expired
		if(   thisSearchTime > lastSearchTime+SJ_HISTORY_DELAY_MS
		 || !(flags&SJ_SETSEARCH_NOAUTOHISTORYADD) )
		{
			if( m_search.IsSet() && m_searchStat.m_totalResultCount > 0 )
			{
				g_advSearchModule->AddToHistory(m_search);
			}
		}
		lastSearchTime = thisSearchTime;

		// modify the search object - first we modify the adv. search as
		// the simple search may check this.
		if( (flags&SJ_SETSEARCH_CLEARADV)
		 && (IsOpAvailable(SJ_OP_MUSIC_SEL) || IsOpAvailable(SJ_OP_MUSIC_SEL_GENRE)) )
		{
			bool doClear = TRUE;
			if( IsKioskStarted() )
			{
				// reset to the default music selection
				long limitToId = g_kioskModule->GetLimitToId();
				if( limitToId )
				{
					m_search.m_adv = g_advSearchModule->GetSearchById(limitToId);
					searchModified = TRUE;
					useHourglass = TRUE;
					deepSearch = TRUE;
					doClear = FALSE;
				}
			}

			if( doClear && m_search.m_adv.IsSet() )
			{
				m_search.m_adv.Clear();
				searchModified = TRUE;
			}
		}
		else if( (flags&SJ_SETSEARCH_SETADV)
		      && (IsOpAvailable(SJ_OP_MUSIC_SEL) || IsOpAvailable(SJ_OP_MUSIC_SEL_GENRE)) )
		{
			wxASSERT( newAdvSearch );
			/*if( m_search.m_adv != *newAdvSearch )
			{*/ // always set a new advanced search to allow the user
			// eg. to reflect his changes in the database
			// by hitting the search button again.
			// this has no really bad performance impact as an
			// advanced search is done rather seldomly (compared with
			// the simple search which is normally not cached)
			m_search.m_adv = *newAdvSearch;
			searchModified = TRUE;
			useHourglass = TRUE;
			deepSearch = TRUE;
			/*}*/
		}

		if( flags&SJ_SETSEARCH_CLEARSIMPLE )
		{
			if( m_search.m_simple.IsSet() )
			{
				m_search.m_simple.Clear();
				searchModified = TRUE;
			}
		}
		else if( flags&SJ_SETSEARCH_SETSIMPLE )
		{
			if( m_search.m_simple.GetWords() != newSimpleSearch )
			{
				m_search.m_simple = newSimpleSearch;
				searchModified = TRUE;
				gatherStatistics = TRUE;
				if( (m_allSearchMs / m_allSearchCount) > 500
				 ||  m_allSearchCount < 3 )
				{
					useHourglass = TRUE;
				}
			}
		}

		if( searchModified )
		{
			// show hourglass if previous searches take a long time or
			// if this is one of the first two searches (which will take
			// longer; btw m_allSearchCount starts with 1)
			if( useHourglass )
			{
				::wxBeginBusyCursor();
				useHourglass = TRUE;
			}

			// update the search input window, if needed
			if( !(flags&SJ_SETSEARCH_NOTEXTCTRLUPDATE) )
			{
				m_simpleSearchInputFromUser = FALSE;
				m_simpleSearchInputWindow->SetValue(m_search.m_simple.GetWords());
				m_simpleSearchInputWindow->SetInsertionPointEnd();
				m_simpleSearchInputFromUser = TRUE;
			}

			// stop the vis. if it is running
			if( g_visModule->IsOverWorkspace() )
			{
				g_visModule->StopVis();
				m_simpleSearchInputWindow->SetFocus();
			}

			// prepare updating the browser after the search
			wxString columnGuid;
			long columnGuidViewOffset = 0;
			if( !m_search.m_simple.IsSet() )
			{
				columnGuid = m_browser->GetFirstSelectedOrVisiblePos(columnGuidViewOffset);
			}

			// do the search
			{
				SjSearchStat stat = m_columnMixer.SetSearch(m_search, deepSearch);
				m_searchStat.m_totalResultCount = stat.m_totalResultCount;
				if( flags&(SJ_SETSEARCH_SETADV|SJ_SETSEARCH_CLEARADV) )
				{
					m_searchStat.m_advResultCount = stat.m_advResultCount;
					m_searchStat.m_mbytes = stat.m_mbytes;
					m_searchStat.m_seconds = stat.m_seconds;
				}
			}

			// set skin values (should be done before updating the browser
			// as the browser uses the text from the search information)
			SjSkinValue v;
			v.value = m_search.IsSet()? 1 : 0;
			SetSkinTargetValue(IDT_SEARCH_BUTTON, v);
			UpdateSearchInfo(0);

			// update the browser
			m_browser->ReloadColumnMixer(false);
			if(  !m_search.m_simple.IsSet()
			 && (!columnGuid.IsEmpty() || !m_searchLastColumnGuid.IsEmpty())  )
			{
				if( !columnGuid.IsEmpty() )
				{
					m_browser->GotoPos(columnGuid, columnGuidViewOffset);
				}
				else
				{
					m_browser->GotoPos(m_searchLastColumnGuid, m_searchLastColumnGuidViewOffset);
				}
			}

			GotDisplayInputFromUser();

			// remember the row to use after an unsuccessful search or after ending the search
			if( !m_search.IsSet() || m_searchStat.m_totalResultCount )
			{
				m_searchLastColumnGuidViewOffset = 0;
				m_searchLastColumnGuid = m_browser->GetFirstSelectedOrVisiblePos(m_searchLastColumnGuidViewOffset);
			}

			// gather statistics for finding out the search delay
			thisSearchTime = SjTools::GetMsTicks() - thisSearchTime;
			wxLogDebug(wxT("%i ms needed for the query"), (int)thisSearchTime);
			if( gatherStatistics )
			{
				m_allSearchCount++;
				m_allSearchMs += m_allSearchCount>3? thisSearchTime : SJ_SEARCH_DELAY_MS;
				// use default values for the first two
				// searches as the real times are not
				// representive as nothing is cached yet
				// (m_allSearchCount starts with "1")
			}

			// inform all modules about the new search
			m_moduleSystem.BroadcastMsg(IDMODMSG_SEARCH_CHANGED);

			// clear hourglass if set before
			if( useHourglass )
			{
				::wxEndBusyCursor();
			}
		}

		m_inPerformingSearch = FALSE;
	}
}


void SjMainFrame::OnSearchHistory(wxCommandEvent& event)
{
	int index = event.GetId() - IDO_SEARCHHISTORY00;
	int count = g_advSearchModule->GetHistoryCount();
	if( index >= 0 && index < count )
	{
		GotInputFromUser();

		const SjSearchHistory*  historyItem = g_advSearchModule->GetHistory(index);
		SjSearch                newSearch = historyItem->GetSearch();

		SetSearch(      (newSearch.m_simple.IsSet()? SJ_SETSEARCH_SETSIMPLE : SJ_SETSEARCH_CLEARSIMPLE)
		            |   (newSearch.m_adv.IsSet()?    SJ_SETSEARCH_SETADV    : SJ_SETSEARCH_CLEARADV),
		                newSearch.m_simple.GetWords(),
		                &newSearch.m_adv    );
	}
}


void SjMainFrame::OnSearchGenre(wxCommandEvent& event)
{
	wxArrayString data = m_libraryModule->GetUniqueValues(SJ_TI_GENRENAME);
	int index = event.GetId() - IDO_SEARCHGENRE000;
	if( index >= 0 && index < (int)data.GetCount()
	 && IsOpAvailable(SJ_OP_MUSIC_SEL_GENRE) )
	{
		GotInputFromUser();

		m_tempSearch.Init(data[index], SJ_SELECTSCOPE_TRACKS, SJ_SELECTOP_ALL);
		m_tempSearch.AddRule(SJ_FIELD_GENRENAME, SJ_FIELDOP_IS_EQUAL_TO, data[index]);
		SetSearch(  SJ_SETSEARCH_CLEARSIMPLE | SJ_SETSEARCH_SETADV,
		            wxT(""), &m_tempSearch );
	}
}


void SjMainFrame::OnSearchMusicSel(wxCommandEvent& event)
{
	int iCount = g_advSearchModule->GetSearchCount();
	int index = event.GetId() - IDO_SEARCHMUSICSEL000;
	if( index >= 0 && index < iCount
	 && IsOpAvailable(SJ_OP_MUSIC_SEL) )
	{
		GotInputFromUser();

		SjAdvSearch advSearch = g_advSearchModule->GetSearchByIndex(index);
		SetSearch(  SJ_SETSEARCH_CLEARSIMPLE | SJ_SETSEARCH_SETADV,
		            wxT(""), &advSearch );
	}
}



