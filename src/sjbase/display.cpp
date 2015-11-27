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
#include <sjbase/display.h>
#include <sjbase/mainframe.h>
#include <sjbase/browser.h>
#include <sjmodules/kiosk/kiosk.h>
#include <sjmodules/tageditor/tageditor.h>
#include <sjmodules/arteditor.h>

// wanted wxLogDebug() outputs
#undef DEBUG_DISPLAY


SjDisplay::SjDisplay()
{
	m_scrollPos                     = -1; // use queue pos
	m_msgCancelAtTimestamp          = 0;
	m_selectedIdsTimestamp          = 0;
	m_selectedAnchorId              = 0;
	m_tagEditorJustOpened           = false;
	m_backupedCurrLine              = -1;
}


void SjDisplay::ClearDisplaySelection()
{
	m_selectedIds.Clear();
	m_selectedAnchorId = 0;
	g_mainFrame->UpdateDisplay();
}


/*******************************************************************************
 * Handling the Display - The edit dialog
 ******************************************************************************/


SjDisplayEditDlg::SjDisplayEditDlg()
	:   SjTagEditorDlg(g_mainFrame, (g_mainFrame->m_contextMenuClickedUrls.GetCount()>1))
{
	g_mainFrame->m_display.m_tagEditorJustOpened = TRUE;

	g_mainFrame->m_libraryModule->SavePendingData();

	m_id = g_mainFrame->m_contextMenuClickedId;

	if( g_mainFrame->m_contextMenuClickedUrls.GetCount() > 1 )
	{
		m_urls = g_mainFrame->m_contextMenuClickedUrls;
	}
	else
	{
		m_urls.Add(g_mainFrame->m_player.m_queue.GetUrlById(m_id));
		MarkId(0);
	}
}


void SjDisplayEditDlg::MarkId(long changeScrollPosDir)
{
	g_mainFrame->m_display.m_selectedIds.Clear();
	g_mainFrame->m_display.m_selectedIds.Insert(m_id, 1);

	long markedPos = g_mainFrame->m_player.m_queue.GetPosById(m_id);
	long queuePos = g_mainFrame->m_display.m_scrollPos==-1? g_mainFrame->m_player.m_queue.GetCurrPos() : g_mainFrame->m_display.m_scrollPos;
	if( changeScrollPosDir
	        && (markedPos < queuePos || markedPos >= queuePos+g_mainFrame->GetLinesCount()) )
	{
		if( changeScrollPosDir < 0 )
		{
			g_mainFrame->m_display.m_scrollPos = markedPos;
		}
		else
		{
			g_mainFrame->m_display.m_scrollPos = (markedPos-g_mainFrame->GetLinesCount())+1;
		}
	}

	g_mainFrame->GotDisplayInputFromUser();

	g_mainFrame->UpdateDisplay();
	g_mainFrame->m_display.m_selectedIdsTimestamp = SjTools::GetMsTicks();
	g_mainFrame->m_display.m_selectedIdsLockMs = SJ_DISPLAY_SELECTION_LOCK_MS;
}


bool SjDisplayEditDlg::GetUrls(wxArrayString& retUrls, int what)
{
	if( what != 0 )
	{
		m_urls.Clear();

		long newId = 0;
		long currPos = g_mainFrame->m_player.m_queue.GetPosById(m_id);
		long queueCount = g_mainFrame->m_player.m_queue.GetCount();
		if( currPos >= 0 && currPos < queueCount )
		{
			if( what == 1 && currPos < queueCount-1 )
			{
				// get next track
				newId = g_mainFrame->m_player.m_queue.GetIdByPos(currPos+1);
			}
			else if( what == -1 && currPos > 0 )
			{
				// get prev. track
				newId = g_mainFrame->m_player.m_queue.GetIdByPos(currPos-1);
			}
		}

		if( newId )
		{
			m_id = newId;
			m_urls.Clear();
			m_urls.Add(g_mainFrame->m_player.m_queue.GetUrlById(m_id));
			MarkId(what);
		}
	}

	// return the urls in scope
	retUrls = m_urls;
	return TRUE;
}


void SjDisplayEditDlg::OnUrlChanged(const wxString& oldUrl, const wxString& newUrl)
{
	if( !newUrl.IsEmpty() )
	{
		int index = m_urls.Index(oldUrl);
		if( index != wxNOT_FOUND )
		{
			m_urls[index] = newUrl;
		}
	}
}


/*******************************************************************************
 * Receiving display events
 ******************************************************************************/


void SjMainFrame::OnSkinDisplayEvent(int targetId, SjSkinValue& value, long accelFlags)
{
	if( targetId >= IDT_DISPLAY_LINE_FIRST && targetId <= IDT_DISPLAY_LINE_LAST )
	{
		long displayLine = targetId-IDT_DISPLAY_LINE_FIRST;
		GotDisplayInputFromUser();

		if( !m_display.m_msg1.IsEmpty() )
		{
			ClearDisplayMsg();
			return;
		}

		long queuePos = m_display.m_firstLineQueuePos + displayLine;
		if( queuePos >= 0 && queuePos < m_player.m_queue.GetCount() )
		{
			if( value.value == SJ_SUBITEM_ICONRIGHT && IsOpAvailable(SJ_OP_UNQUEUE) )
			{
				UnqueueByPos(queuePos);
			}
			else if( value.value == SJ_SUBITEM_ICONLEFT )
			{
				if( m_player.m_queue.GetCurrPos() == queuePos )
				{
					if( IsOpAvailable(SJ_OP_PLAYPAUSE) )
					{
						PlayOrPause(accelFlags != 0);
					}
				}
				else if( m_player.m_queue.WasPlayed(queuePos) )
				{
					if( IsOpAvailable(SJ_OP_EDIT_QUEUE) )
					{
						m_player.m_queue.ResetPlayCount(queuePos);
						UpdateDisplay();
					}
				}
			}
			else if( value.value == SJ_SUBITEM_TEXT_DCLICK && IsOpAvailable(SJ_OP_EDIT_QUEUE) )
			{
				#ifdef DEBUG_DISPLAY
				SjPlaylistEntry& urlInfo = m_player.m_queue.GetInfo(queuePos);
				wxLogDebug("Line pos. %i: %s - %s", (int)queuePos, urlInfo.GetLeadArtistName().c_str(), urlInfo.GetTrackName().c_str());
				#endif

				if( m_player.m_queue.GetCurrPos() == queuePos && IsPlaying() )
				{
					; // PlayOrPause(); // better do nothing
				}
				else
				{
					m_player.GotoAbsPos(queuePos, accelFlags!=0);
					if( !IsPlaying() )
					{
						Play();
						UpdateDisplay();
					}
				}
			}
			else if( (value.value == SJ_SUBITEM_TEXT_MOUSEDOWN || value.value == SJ_SUBITEM_TEXT/*_MOUSEUP*/ )
			         &&  IsOpAvailable(SJ_OP_EDIT_QUEUE) )
			{
				MarkDisplayTrack(targetId, value.value == SJ_SUBITEM_TEXT_MOUSEDOWN, value.vmin);
			}
			else if( value.value == SJ_SUBITEM_TEXT_MIDDLECLICK )
			{
				if( g_accelModule->m_middleMouseAction )
				{
					long queueId = m_player.m_queue.GetIdByPos(queuePos);
					if( queueId == 0 )
					{
						return;
					}

					if( !m_display.m_selectedIds.Lookup(queueId)  )
					{
						// mark the item under the mouse if it is not marked
						m_display.m_selectedIds.Clear();
						m_display.m_selectedIds.Insert(queueId, 1);
						m_display.m_selectedAnchorId = queueId;

						UpdateDisplay();
					}

					m_display.m_selectedIdsTimestamp = SjTools::GetMsTicks();
					m_display.m_selectedIdsLockMs = SJ_DISPLAY_SELECTION_LOCK_MS;

					m_contextMenuClickedUrls = m_player.m_queue.GetUrlsByIds(m_display.m_selectedIds);
					m_contextMenuClickedId = queueId;

					switch( g_accelModule->m_middleMouseAction )
					{
						case SJ_ACCEL_MID_OPENS_TAG_EDITOR:
							if( IsAllAvailable() )
							{
								g_tagEditorModule->OpenTagEditor(new SjDisplayEditDlg());
							}
							break;
					}
				}
			}
		}
		else
		{
			// click on empty line; remove all marks
			m_display.m_selectedIds.Clear();
			m_display.m_selectedAnchorId = 0;
			UpdateDisplay();
		}
	}
	else switch( targetId )
		{
				// curr / next tracks
			case IDT_CURR_TRACK:
				if( value.value == SJ_SUBITEM_TEXT_DCLICK && !IsStopped() && IsPaused() && IsOpAvailable(SJ_OP_PLAYPAUSE) )
				{
					Play();
					UpdateDisplay();
				}
				break;

			case IDT_NEXT_TRACK:
				if( value.value == SJ_SUBITEM_TEXT_DCLICK && !IsStopped() && !GetShuffle() && HasNextIgnoreAP() && IsOpAvailable(SJ_OP_EDIT_QUEUE) )
				{
					GotoNextRegardAP(accelFlags!=0);
					if( !IsPlaying() )
						Play();
					UpdateDisplay();
				}
				break;

				// main volume

			case IDT_MAIN_VOL_UP:
			case IDT_MAIN_VOL_DOWN:
				if( IsOpAvailable(SJ_OP_MAIN_VOL) )
				{
					SetRelMainVol(targetId==IDT_MAIN_VOL_UP? 8 : -8);
					SetSkinTargetValue(IDT_MAIN_VOL_MUTE, m_player.GetMainVolMute()? 1 : 0);
					// maybe later -- SetDisplayMsg(_("Volume")+wxString::Format(": %i%%", (m_player.GetMainVolume()*100)/255), SDM_STATE_CHANGE_MS);
				}
				break;

			case IDT_MAIN_VOL_SLIDER:
				if( IsOpAvailable(SJ_OP_MAIN_VOL) )
				{
					SetAbsMainVol(value.value);
					SetSkinTargetValue(IDT_MAIN_VOL_MUTE, m_player.GetMainVolMute()? 1 : 0);
					// maybe later -- SetDisplayMsg(_("Volume")+wxString::Format(": %i%%", (m_player.GetMainVolume()*100)/255), SDM_STATE_CHANGE_MS);
				}
				break;

			case IDT_MAIN_VOL_MUTE:
				if( IsOpAvailable(SJ_OP_MAIN_VOL) )
				{
					m_player.ToggleMainVolMute();
					UpdateDisplay();
				}
				break;

				// seek

			case IDT_SEEK:
				if( IsOpAvailable(SJ_OP_EDIT_QUEUE) )
				{
					SjPlayer* recentPlayer = &g_mainFrame->m_player;

					recentPlayer->SeekAbs(value.value*SJ_SEEK_RESOLUTION);
					UpdateDisplay();
				}
				break;

			case IDT_SEEK_BWD:
			case IDT_SEEK_FWD:
				if( IsOpAvailable(SJ_OP_EDIT_QUEUE) )
				{
					SjPlayer* recentPlayer = &g_mainFrame->m_player;

					recentPlayer->SeekRel(targetId==IDT_SEEK_BWD? -5000 : 5000);
					UpdateDisplay();
				}
				break;

				// misc.

			case IDO_DISPLAY_COVER_EXPLR:
			{
				SjPlayer* recentPlayer = &g_mainFrame->m_player;
				g_tools->ExploreUrl(m_columnMixer.GetTrackCoverUrl(recentPlayer->m_queue.GetUrlByPos(-1)));
			}
			break;

			case IDT_DISPLAY_COVER:
			case IDO_DISPLAY_COVER: // IDO_DISPLAY_COVER is only used for the menu item
				if(  value.value == SJ_SUBITEM_TEXT_DCLICK
				        || (value.value == SJ_SUBITEM_TEXT_MIDDLECLICK && g_accelModule->m_middleMouseAction==SJ_ACCEL_MID_OPENS_TAG_EDITOR)
				        ||  targetId != IDT_DISPLAY_COVER )
				{

					// open the cover editor for the display cover
					SjPlayer* recentPlayer = &g_mainFrame->m_player;

					wxString trackUrl = recentPlayer->m_queue.GetUrlByPos(-1);
					wxSqlt sql;
					sql.Query(wxT("SELECT albumid FROM tracks WHERE url='") + sql.QParam(trackUrl) + wxT("';"));
					unsigned long albumId = sql.Next()? sql.GetLong(0) : 0;

					wxString selArtUrl = m_columnMixer.GetTrackCoverUrl(recentPlayer->m_queue.GetUrlByPos(-1));

					wxArrayLong artIds;
					wxArrayString artUrls;
					g_mainFrame->m_libraryModule->GetPossibleAlbumArts(albumId, artIds, &artUrls, true/*addAutoCover*/);

					g_artEditorModule->OnArtMenu(g_mainFrame, artUrls, selArtUrl, albumId, IDM_OPENARTEDITOR, NULL, true /*IsPlaylistCover*/ );

					/*
					// simulate a double click on the cover row or explore the cover url
					long index, r;
					SjCol* col = m_columnMixer.GetMaskedCol(recentPlayer->m_queue.GetUrlByPos(recentPlayer->m_queue.GetCurrPos()), index);
					SjAlbumCoverRow* row;
					if( col )
					{
					    for( r = 0; r < col->m_rowCount; r++ )
					    {
					        if( col->m_rows[r]->m_roughType == SJ_RRTYPE_COVER )
					        {
					            row = (SjAlbumCoverRow*)col->m_rows[r];
					            row->IsPlaylistCover(true); // if set to true, "follow playlist" is always active
					            row->OnMiddleClick(false);
					            break;
					        }
					    }

					    delete col;
					}
					*/
				}
				break;

			case IDT_DISPLAY_UP:
				GotDisplayInputFromUser();

				if( m_display.m_scrollPos == -1 )
				{
					m_display.m_scrollPos = GetQueuePos();
				}

				if( m_display.m_scrollPos >= 1 )
				{
					m_display.m_scrollPos--;
					UpdateDisplay();
				}
				break;

			case IDT_DISPLAY_DOWN:
				GotDisplayInputFromUser();

				if( m_display.m_scrollPos == -1 )
				{
					m_display.m_scrollPos = GetQueuePos();
				}

				m_display.m_scrollPos++; // corrected in UpdateDisplay()
				UpdateDisplay();
				break;

			case IDT_DISPLAY_V_SCROLL:
				GotDisplayInputFromUser();

				m_display.m_scrollPos = value.value;
				UpdateDisplay();
				break;

		}
}


/*******************************************************************************
 * Setting the display
 ******************************************************************************/


void SjMainFrame::UpdateCurrTime()
{
	long showMs = -1;
	bool prependMinus = false;
	if( m_player.IsStopped() )
	{
		// we show not title if the player is stopped
		/*
		long pos = m_player.m_queue.GetCurrPos();
		if( pos >= 0 )
		{
		    SjPlaylistEntry& urlInfo = m_player.m_queue.GetInfo(pos);
		    showMs = urlInfo.GetPlaytimeMs();
		    if( showMs <= 0 )
		        showMs = -1;
		}
		*/
	}
	else
	{
		long totalMs, elapsedMs, remainingMs;
		m_player.GetTime(totalMs, elapsedMs, remainingMs);
		if( m_showRemainingTime && remainingMs >= 0 )
		{
			showMs = remainingMs;
			prependMinus = true;
		}
		else
		{
			showMs = elapsedMs;
		}
	}

	SjSkinValue value;
	if( showMs >= 0 )
	{
		value.string.Append(SjTools::FormatTime(showMs/1000, SJ_FT_ALLOW_ZERO|SJ_FT_MIN_5_CHARS|(prependMinus?SJ_FT_PREPEND_MINUS:0)));
	}
	else
	{
		value.string = wxT("--:--");
	}
	SetSkinTargetValue(IDT_CURR_TIME, value);
}


wxString SjMainFrame::CombineArtistNTitle(const wxString& artist, const wxString& title, const wxString& sep)
{
	wxString ret(artist);
	if( m_skinFlags&SJ_SKIN_SHOW_DISPLAY_ARTIST )
	{
		if( !ret.IsEmpty() ) ret.Append(sep);
		ret.Append(title);
	}
	else
	{
		ret = title;
	}
	return ret;
}


void SjMainFrame::UpdateDisplay()
{
	SjSkinValue     value;
	long            totalMs, elapsedMs;

	// get the number of visible lines
	long            visLineCount = GetLinesCount(),
	                visLine;
	wxASSERT(visLineCount>=0);

	// get the player
	SjPlayer*       recentPlayer = &g_mainFrame->m_player;

	// get the number of tracks in the queue
	long            queueTrackCount = recentPlayer->m_queue.GetCount(),
	                queuePlayingPos = recentPlayer->m_queue.GetCurrPos(),
	                queuePos;
	wxASSERT(queueTrackCount>=0);

	// current stuff
	SjPlaylistEntry&    urlInfo1 = recentPlayer->m_queue.GetInfo(-1);
	wxString            currTrackName = urlInfo1.GetTrackName();
	wxString            currArtistName = urlInfo1.GetLeadArtistName();

	unsigned long       currTimestamp = SjTools::GetMsTicks();

	// update cover
	{
		if( queuePlayingPos != -1 )
		{
			value.string = m_columnMixer.GetTrackCoverUrl(recentPlayer->m_queue.GetUrlByPos(-1));
			value.value = SJ_VFLAG_STRING_IS_IMAGE_URL;
			SetSkinTargetValue(IDT_DISPLAY_COVER, value);
		}
		else
		{
			SetSkinTargetValue(IDT_DISPLAY_COVER, value);
		}
	}

	// update seeker
	{
		totalMs = recentPlayer->GetTotalTime();
		value.string.Empty();
		if( !recentPlayer->IsStopped() && totalMs > 0 )
		{
			elapsedMs = recentPlayer->GetElapsedTime();

			value.vmax  = totalMs/SJ_SEEK_RESOLUTION; if( value.vmax==0 ) value.vmax = 1;
			value.value = elapsedMs/SJ_SEEK_RESOLUTION;
			SetSkinTargetValue(IDT_SEEK, value);

		}
		else
		{
			value.value = 0;
			SetSkinTargetValue(IDT_SEEK, value);
		}
	}

	// update main window title and the "currTrack" target
	if( !IsStopped() )
	{
		wxString title, titleArtistName(currArtistName), titleTrackName(currTrackName);
		if( recentPlayer != &m_player )
		{
			SjPlaylistEntry& urlInfo4 = m_player.m_queue.GetInfo(-1);
			titleTrackName = urlInfo4.GetTrackName();
			titleArtistName = urlInfo4.GetLeadArtistName();
		}

		// set windows title
		title = CombineArtistNTitle(titleArtistName, titleTrackName, wxT(" - "));
		title.Append(wxT(" - "));
		title.Append(SJ_PROGRAM_NAME);
		SetTitle(title);
	}
	else
	{
		// set windows title
		SetTitle(SJ_PROGRAM_NAME);
	}

	// update display lines...
	bool displayMsgInCurrNextTrack = false;

	if( (m_display.m_msgCancelAtTimestamp && !m_display.m_msg1.IsEmpty()) )
	{
		// ...display message
		wxString msg1(m_display.m_msg1), msg2(m_display.m_msg2);

		if( HasSkinTarget(IDT_DISPLAY_LINE_FIRST) )
		{
			m_display.m_backupedCurrLine = -1;
			value.value = SJ_VFLAG_CENTER;
			value.value |= queueTrackCount? SJ_VFLAG_IGNORECENTEROFFSET : 0;
			for( visLine = 0; visLine < visLineCount; visLine++ )
			{
				if( visLine==visLineCount/2 )
				{
					value.string = msg1;
				}
				else if( visLine==(visLineCount/2)+1 )
				{
					value.string = msg2;
				}
				else
				{
					value.string.Empty();
				}
				SetSkinTargetValue(IDT_DISPLAY_LINE_FIRST+visLine, value);
			}
		}
		else if( HasSkinTarget(IDT_CURR_TRACK) )
		{
			displayMsgInCurrNextTrack = true;
			value.value = 0;
			if( HasSkinTarget(IDT_NEXT_TRACK) )
			{
				value.string = msg1;
				SetSkinTargetValue(IDT_CURR_TRACK, value);
				value.string = msg2;
				SetSkinTargetValue(IDT_NEXT_TRACK, value);
			}
			else
			{
				value.string = msg1;
				if( !msg2.IsEmpty() ) value.string += wxT(" ") + msg2;
				SetSkinTargetValue(IDT_CURR_TRACK, value);
			}
		}
	}
	else if( queueTrackCount == 0 )
	{
		// ...nothing in queue, display logo
		m_display.m_firstLineQueuePos = -1;

		value.thumbSize = 1;
		value.vmin  = 0;
		value.vmax  = 1;
		value.value = 0;
		SetSkinTargetValue(IDT_DISPLAY_V_SCROLL, value);

		value.value = SJ_VFLAG_CENTER;
		for( visLine = 0; visLine < visLineCount; visLine++ )
		{
			if( visLine==visLineCount/2 )
			{
				value.string = wxString::Format(wxT("%s %i.%i"), SJ_PROGRAM_NAME, SJ_VERSION_MAJOR, SJ_VERSION_MINOR);
			}
			else
			{
				value.string.Empty();
			}
			SetSkinTargetValue(IDT_DISPLAY_LINE_FIRST+visLine, value);
		}
	}
	else
	{
		// ...sth. in queue, display queue...
		m_display.m_backupedCurrLine = -1;
		m_display.m_backupedCurrBold = FALSE;

		queuePos = m_display.m_scrollPos==-1? queuePlayingPos : m_display.m_scrollPos;
		if( queuePos < 0 ) queuePos = 0;
		if( queueTrackCount > visLineCount && queuePos > queueTrackCount-visLineCount ) queuePos = queueTrackCount-visLineCount;
		if( queueTrackCount <= visLineCount ) queuePos = 0;

		if( m_display.m_scrollPos != -1 ) m_display.m_scrollPos = queuePos; // correct the value

		value.thumbSize = visLineCount;
		value.vmin  = 0;
		value.vmax  = queueTrackCount;
		value.value = queuePos;
		SetSkinTargetValue(IDT_DISPLAY_V_SCROLL, value);

		value.thumbSize = 0;
		m_display.m_firstLineQueuePos = queuePos;
		for( visLine = 0; visLine < visLineCount; visLine++, queuePos++ )
		{
			value.vmax = 0;
			value.vmin = 0;
			if( queuePos >= 0 && queuePos < queueTrackCount )
			{
				// ...name
				SjPlaylistEntry& urlInfo2 = recentPlayer->m_queue.GetInfo(queuePos);
				value.string = CombineArtistNTitle(urlInfo2.GetLeadArtistName(), urlInfo2.GetTrackName());

				if( m_skinFlags&SJ_SKIN_SHOW_DISPLAY_AUTOPLAY )
				{
					if( recentPlayer->m_queue.GetFlags(queuePos)&SJ_PLAYLISTENTRY_AUTOPLAY )
					{
						value.string.Prepend(_("AutoPlay")+wxString(wxT(": ")));
					}
				}

				if( m_skinFlags&SJ_SKIN_SHOW_DISPLAY_TRACKNR )
				{
					value.string.Prepend(wxString::Format(wxT("%i. "), (int)queuePos+1));
				}

				// ...time / right icon
				value.value = 0;

				if( IsOpAvailable(SJ_OP_EDIT_QUEUE) )
				{
					value.value |= (SJ_VFLAG_MOVABLE);
				}

				if( IsOpAvailable(SJ_OP_UNQUEUE) )
				{
					value.value |= (SJ_VFLAG_ICONR_DELETE);
				}

				if( queuePos == queuePlayingPos && !recentPlayer->IsStopped() )
				{
					value.value |= SJ_VFLAG_VMIN_IS_TIME;
					if( IsOpAvailable(SJ_OP_TOGGLE_TIME_MODE) )
						value.value |= SJ_VFLAG_TIME_CLICKABLE;

					long totalTime = recentPlayer->GetTotalTime();
					if( m_showRemainingTime && totalTime > 0 )
					{
						value.value |= SJ_VFLAG_VMIN_MINUS;
						value.vmin = recentPlayer->GetRemainingTime();
						if( value.vmin <= 0 )
						{
							value.vmin = urlInfo2.GetPlaytimeMs();
						}
					}
					else
					{
						value.vmin = recentPlayer->GetElapsedTime();
					}

					if( m_skinFlags&SJ_SKIN_SHOW_DISPLAY_TOTAL_TIME )
					{
						value.vmax = totalTime;
						value.value |= value.vmax>0? SJ_VFLAG_VMAX_IS_TIME : 0;
					}
				}
				else
				{
					value.vmin = urlInfo2.GetPlaytimeMs();
					value.value |= value.vmin>0? SJ_VFLAG_VMIN_IS_TIME : 0;
				}

				// marked (or just added?)
				if( m_display.m_selectedIds.Lookup(urlInfo2.GetId()) )
				{
					value.value |= SJ_VFLAG_BOLD;
					if( queuePos == queuePlayingPos )
					{
						m_display.m_backupedCurrBold = TRUE;
					}
				}

				// ...left icon
				if( queuePos == queuePlayingPos )
				{
					if( recentPlayer->IsStopped() ) { value.value |= SJ_VFLAG_ICONL_STOP; }
					else if( recentPlayer->IsPaused() )  { value.value |= SJ_VFLAG_ICONL_PAUSE; }
					else                                 { value.value |= SJ_VFLAG_ICONL_PLAY; }

					m_display.m_backupedCurrTitle = value.string;
					m_display.m_backupedCurrLine = visLine;
				}
				else
				{
					long entryFlags     = recentPlayer->m_queue.GetFlags(queuePos);
					if( entryFlags & SJ_PLAYLISTENTRY_MOVED_DOWN )
					{
						if( !recentPlayer->m_queue.IsBoring(queuePos, currTimestamp) )
						{
							entryFlags &= ~SJ_PLAYLISTENTRY_MOVED_DOWN;
							recentPlayer->m_queue.SetFlags(queuePos, entryFlags);
						}
					}

					if( entryFlags&SJ_PLAYLISTENTRY_ERRONEOUS )        { value.value |= SJ_VFLAG_ICONL_ERRONEOUS; }
					else if( recentPlayer->m_queue.WasPlayed(queuePos) )    { value.value |= SJ_VFLAG_ICONL_PLAYED; }
					else if( entryFlags&SJ_PLAYLISTENTRY_MOVED_DOWN )       { value.value |= SJ_VFLAG_ICONL_MOVED_DOWN; }
					else                                                    { value.value |= SJ_VFLAG_ICONL_EMPTY; }
				}
			}
			else
			{
				// ...empty line
				value.value = SJ_VFLAG_ICONL_EMPTY;
				value.string.Empty();
			}

			SetSkinTargetValue(IDT_DISPLAY_LINE_FIRST+visLine, value);
		}
	}

	// set "currTrack", "nextTrack" and "currTime" targets
	if( !displayMsgInCurrNextTrack )
	{
		if( HasSkinTarget(IDT_CURR_TRACK) )
		{
			value.value = 0;
			value.string = SJ_PROGRAM_NAME;
			if( !IsStopped() && queuePlayingPos < queueTrackCount )
			{
				SjPlaylistEntry& urlInfo4 = m_player.m_queue.GetInfo(queuePlayingPos);
				value.string = CombineArtistNTitle(urlInfo4.GetLeadArtistName(), urlInfo4.GetTrackName());
			}
			SetSkinTargetValue(IDT_CURR_TRACK, value);
		}

		if( HasSkinTarget(IDT_NEXT_TRACK) )
		{
			value.value = 0;
			value.string.Empty();
			if( !IsStopped() )
			{
				if( GetShuffle() )
				{
					if( HasNextRegardAP() )
					{
						value.string = _("Shuffle");
					}
				}
				else
				{
					if( queuePlayingPos+1 < queueTrackCount )
					{
						SjPlaylistEntry& urlInfo4 = m_player.m_queue.GetInfo(queuePlayingPos+1);
						value.string = CombineArtistNTitle(urlInfo4.GetLeadArtistName(), urlInfo4.GetTrackName());
					}
				}
			}
			SetSkinTargetValue(IDT_NEXT_TRACK, value);
		}
	}

	if( HasSkinTarget(IDT_CURR_TIME) )
	{
		UpdateCurrTime();
	}

	// playing states
	value.vmin      = 0;
	value.vmax      = 255;
	value.thumbSize = 0;
	value.value     = m_player.GetMainVol();
	SetSkinTargetValue(IDT_MAIN_VOL_SLIDER, value);

	value.vmax  = 0;
	value.value = m_player.GetMainVolMute()? 1 : 0;
	SetSkinTargetValue(IDT_MAIN_VOL_MUTE, value);

	value.value = m_player.IsPlaying()? 1 : 0;
	SetSkinTargetValue(IDT_PLAY, value);

	value.value = m_player.IsPaused()? 1 : 0;
	SetSkinTargetValue(IDT_PAUSE, value);

	value.value = m_player.IsStopped()? 1 : 0;
	SetSkinTargetValue(IDT_STOP, value);

	value.value = HasPrev()? 0 : 2;
	SetSkinTargetValue(IDT_PREV, value);

	value.value = HasNextRegardAP()? 0 : 2;
	SetSkinTargetValue(IDT_NEXT, value);
	SetSkinTargetValue(IDT_FADE_TO_NEXT, value);

	value.value = m_player.StopAfterThisTrack()? 1 : (/*m_player.IsStopped()? 2 :*/ 0);
	SetSkinTargetValue(IDT_STOP_AFTER_THIS_TRACK, value);

	value.value = m_player.StopAfterEachTrack()? 1 : (/*m_player.IsStopped()? 2 :*/ 0);
	SetSkinTargetValue(IDT_STOP_AFTER_EACH_TRACK, value);

	value.value = m_player.m_queue.GetShuffle()? 1 : 0;
	SetSkinTargetValue(IDT_SHUFFLE, value);

	value.value = m_player.m_queue.GetRepeat(); /*0=off, 1=single, 2=all*/
	SetSkinTargetValue(IDT_REPEAT, value);

	value.value = queueTrackCount? 0/*enabled*/ : 2/*disabled*/;
	SetSkinTargetValue(IDT_APPEND_FILES, value);
	SetSkinTargetValue(IDT_SAVE_PLAYLIST, value);
	SetSkinTargetValue(IDT_UNQUEUE_ALL, value);
	SetSkinTargetValue(IDT_MORE_FROM_CURR_ALBUM, value);
	SetSkinTargetValue(IDT_MORE_FROM_CURR_ARTIST, value);
}


void SjMainFrame::OnElapsedTimeTimer(wxTimerEvent& event)
{
	// flush pending messages?
	// in wxWidgets 2.4.2 this is needed for windows eg. if a menu
	// or a modal system dialog is opened the events are not sent otherwise.
	// this may change in future wxWidgets versions, and, however, for other
	// OS this may not be necessary.
	#ifdef __WXMSW__
	m_mainApp->ProcessPendingEvents();
	#endif

	// display stuff
	unsigned long   thisTimestamp = SjTools::GetMsTicks();
	SjPlayer*       recentPlayer = &g_mainFrame->m_player;

	if( HasSkinTarget(IDT_CURR_TIME) )
	{
		UpdateCurrTime();
	}

	// snap back to current position?
	if( IsMouseInDisplayMove() )
	{
		m_display.m_selectedIdsTimestamp = thisTimestamp;
		m_display.m_selectedIdsLockMs = SJ_DISPLAY_SELECTION_LOCK_MS;
	}
	else if( m_display.m_scrollPos != -1 || m_display.m_selectedIdsTimestamp != 0 || m_display.m_msgCancelAtTimestamp != 0 )
	{

		bool updateDisplay = FALSE;

		if( m_display.m_selectedIdsTimestamp != 0
		        && thisTimestamp > (m_display.m_selectedIdsTimestamp+m_display.m_selectedIdsLockMs) )
		{
			m_display.m_selectedIds.Clear();
			m_display.m_selectedIdsTimestamp = 0;
			if( thisTimestamp > (m_lastUserDisplayInputTimestamp+SJ_DISPLAY_ENQUEUE_LOCK_MS) )
			{
				m_display.m_scrollPos = -1;
			}
			updateDisplay = TRUE;
		}

		if( thisTimestamp > (m_lastUserInputTimestamp+SJ_DISPLAY_SCROLL_LOCK_MS) )
		{
			m_display.m_scrollPos = -1;
			updateDisplay = TRUE;
		}

		if( m_display.m_msgCancelAtTimestamp != 0
		        && thisTimestamp > m_display.m_msgCancelAtTimestamp )
		{
			if( g_kioskModule )
			{
				g_kioskModule->m_numpadInput.DisplayTimeout();
			}

			m_display.m_msgCancelAtTimestamp = 0;
			m_display.m_msg1.Empty();
			m_display.m_msg2.Empty();
			updateDisplay = TRUE;
		}

		if( updateDisplay )
		{
			UpdateDisplay();
			goto Done;
		}
	}

	if( recentPlayer && !recentPlayer->IsStopped() )
	{
		SjSkinValue     value;
		long            totalMs = recentPlayer->GetTotalTime();
		long            elapsedMs = recentPlayer->GetElapsedTime();
		bool            isPaused = recentPlayer->IsPaused();
		bool            stopAfterTrack = ( recentPlayer->StopAfterThisTrack()||recentPlayer->StopAfterEachTrack() );
		bool            blink = (  (&m_player==recentPlayer&&stopAfterTrack )
		                           || isPaused );

		if( totalMs > 0 )
		{
			value.vmin      = 0;
			value.vmax      = totalMs/SJ_SEEK_RESOLUTION;
			value.thumbSize = 0;
			value.value     = elapsedMs/SJ_SEEK_RESOLUTION;
		}
		SetSkinTargetValueIfPossible(IDT_SEEK, value);

		if( m_display.m_backupedCurrLine != -1 )
		{
			value.value  = SJ_VFLAG_VMIN_IS_TIME | SJ_VFLAG_ICONL_PLAY;
			if( IsOpAvailable(SJ_OP_TOGGLE_TIME_MODE) )
				value.value |= SJ_VFLAG_TIME_CLICKABLE;

			if( m_showRemainingTime && totalMs > 0 )
			{
				value.vmin = recentPlayer->GetRemainingTime();
				value.value |= SJ_VFLAG_VMIN_MINUS;
			}
			else
			{
				value.vmin = elapsedMs;
			}

			if( m_display.m_backupedCurrBold )
			{
				value.value |= SJ_VFLAG_BOLD;
			}


			if( IsOpAvailable(SJ_OP_EDIT_QUEUE) )
			{
				value.value |= (SJ_VFLAG_MOVABLE);
			}

			if( IsOpAvailable(SJ_OP_UNQUEUE) )
			{
				value.value |= (SJ_VFLAG_ICONR_DELETE);
			}

			value.string = m_display.m_backupedCurrTitle;
			if( m_skinFlags&SJ_SKIN_SHOW_DISPLAY_TOTAL_TIME )
			{
				value.vmax = totalMs;
				value.value |= totalMs>0? SJ_VFLAG_VMAX_IS_TIME : 0;
			}

			if( blink )
			{
				if( isPaused )
				{
					value.value &= ~SJ_VFLAG_ICONL_PLAY;
					value.value |= m_blinkBlink? SJ_VFLAG_ICONL_PAUSE : SJ_VFLAG_ICONL_EMPTY;
				}
				else if( !m_blinkBlink )
				{
					value.value &= ~SJ_VFLAG_ICONL_PLAY;
					value.value |= SJ_VFLAG_ICONL_STOP;
				}
			}

			SetSkinTargetValueIfPossible(IDT_DISPLAY_LINE_FIRST+m_display.m_backupedCurrLine, value);

			m_blinkBlink = !m_blinkBlink;
		}
	}

Done:

	// invoke auto control stuff
	m_autoCtrl.OnOneSecondTimer();
}


/*******************************************************************************
 * Movin' selected tracks
 ******************************************************************************/


void SjMainFrame::OnSkinTargetMotion(int clickTargetId, int requestedMotionAmount)
{
	GotDisplayInputFromUser();

	if( m_display.m_selectedIds.GetCount() )
	{
		m_display.m_backupedCurrLine = -1; // stop timer display updating until UpdateDisplay() is called

		long visLineCount = GetLinesCount();
		long queueTrackCount = m_player.m_queue.GetCount();
		long queuePlayingPos = m_player.m_queue.GetCurrPos();

		long scrollPos = m_display.m_scrollPos==-1? queuePlayingPos : m_display.m_scrollPos;
		if( scrollPos < 0 ) scrollPos = 0;
		if( queueTrackCount > visLineCount && scrollPos > queueTrackCount-visLineCount )
		{
			scrollPos = queueTrackCount-visLineCount;
			#ifdef DEBUG_DISPLAY
			wxLogDebug("          vv corrected vv");
			#endif
		}
		if( queueTrackCount <= visLineCount ) scrollPos = 0;

		int realMotionAmount = m_player.m_queue.MoveByIds(m_display.m_selectedIds, requestedMotionAmount);

		int resumeTargetId = clickTargetId;
		if( realMotionAmount )
		{
			#ifdef DEBUG_DISPLAY
			wxLogDebug("oldScrollPos=%i (oldDisplayScrollPos=%i, queuePlayingPos=%i)", (int)scrollPos, (int)m_display.m_scrollPos, (int)queuePlayingPos);
			#endif

			resumeTargetId += realMotionAmount;
			if( resumeTargetId < IDT_DISPLAY_LINE_FIRST )
			{
				m_display.m_scrollPos = scrollPos + realMotionAmount;
				if( m_display.m_scrollPos < 0 ) m_display.m_scrollPos = 0;
				resumeTargetId = IDT_DISPLAY_LINE_FIRST;

				#ifdef DEBUG_DISPLAY
				wxLogDebug("  ^ newDisplayScrollPos=%i, resumeTargetLine=%i", (int)m_display.m_scrollPos, (int)resumeTargetId-IDT_DISPLAY_LINE_FIRST);
				#endif
			}
			else if( resumeTargetId >= IDT_DISPLAY_LINE_FIRST+visLineCount )
			{
				m_display.m_scrollPos = scrollPos + realMotionAmount;
				resumeTargetId = (IDT_DISPLAY_LINE_FIRST+visLineCount)-1;

				#ifdef DEBUG_DISPLAY
				wxLogDebug("  v newDisplayScrollPos=%i, resumeTargetLine=%i", (int)m_display.m_scrollPos, (int)resumeTargetId-IDT_DISPLAY_LINE_FIRST);
				#endif
			}
			else
			{
				m_display.m_scrollPos = scrollPos;

				#ifdef DEBUG_DISPLAY
				wxLogDebug("  * newDisplayScrollPos=%i, resumeTargetLine=%i", (int)m_display.m_scrollPos, (int)resumeTargetId-IDT_DISPLAY_LINE_FIRST);
				#endif
			}
		}

		ResumeSkinTargetMotion(clickTargetId, resumeTargetId);
		UpdateDisplay();

		#ifdef DEBUG_DISPLAY
		wxLogDebug("    resultingDisplayScrollPos=%i", (int)m_display.m_scrollPos);
		#endif

		m_display.m_selectedIdsTimestamp = SjTools::GetMsTicks();
		m_display.m_selectedIdsLockMs = SJ_DISPLAY_SELECTION_LOCK_MS;
	}
}


void SjMainFrame::MarkDisplayTrack(int targetId, bool mouseDown, long accelFlags)
{
	// get needed data
	long queuePos = m_display.m_firstLineQueuePos + (targetId-IDT_DISPLAY_LINE_FIRST);
	if( queuePos < 0 || queuePos >= m_player.m_queue.GetCount() )
	{
		return;
	}

	long queueId = m_player.m_queue.GetIdByPos(queuePos);
	if( queueId == 0 )
	{
		return;
	}

	bool queuePosMarked = m_display.m_selectedIds.Lookup(queueId)!=0;
	// bool updateDisplay  = FALSE;

	// do the selection now?
	if( mouseDown )
	{
		if( queuePosMarked )
		{
			m_display.m_selectionDoneOnMouseDown = 0;
			return;
		}
		else if( accelFlags & (wxACCEL_SHIFT|wxACCEL_CTRL) )
		{
			m_display.m_selectionDoneOnMouseDown = -1;
			if( (accelFlags&wxACCEL_CTRL) && m_display.m_selectedIds.GetCount()==0 )
			{
				m_display.m_selectedAnchorId = queueId;
			}
		}
		else
		{
			m_display.m_selectedIds.Clear();
			m_display.m_selectedIds.Insert(queueId, 1);
			m_display.m_selectedAnchorId = queueId;

			UpdateDisplay();
			m_display.m_selectedIdsTimestamp = SjTools::GetMsTicks();
			m_display.m_selectedIdsLockMs = SJ_DISPLAY_SELECTION_LOCK_MS;

			m_display.m_selectionDoneOnMouseDown = queueId;
			return;                                                 // if a queue id is set, it is removed on "mouse up"
		}                                                           // .
	}                                                               // .
	else if( m_display.m_selectionDoneOnMouseDown )                 // .
	{
		if( m_display.m_selectionDoneOnMouseDown != -1 )
		{	// .
			// . here
			m_display.m_selectedIds.Remove(m_display.m_selectionDoneOnMouseDown);
			UpdateDisplay();
			m_display.m_selectedIdsTimestamp = SjTools::GetMsTicks();
			m_display.m_selectedIdsLockMs = SJ_DISPLAY_SELECTION_LOCK_MS;
		}
		return;
	}

	// mark the track
	if( accelFlags & wxACCEL_CTRL )
	{
		// (de-)select using CTRL

		if( m_display.m_selectedIds.Lookup(queueId) )
		{
			m_display.m_selectedIds.Remove(queueId);
		}
		else
		{
			m_display.m_selectedIds.Insert(queueId, 1);
		}
	}
	else if( accelFlags & wxACCEL_SHIFT )
	{
		// select a range using SHIFT
		// (the first SHIFT/CONTROL always defines the anchor and selects one time;
		// this is by design as a single item is only selected temporary until the mouse
		// is released)

		long anchorPos = m_player.m_queue.GetPosById(m_display.m_selectedAnchorId), p1, p2, p;
		if( anchorPos < 0 ) anchorPos = queuePos;

		if( anchorPos < queuePos ) { p1=anchorPos; p2=queuePos; } else { p1=queuePos; p2=anchorPos; }

		m_display.m_selectedIds.Clear();
		for( p = p1; p <= p2; p++ )
		{
			m_display.m_selectedIds.Insert(m_player.m_queue.GetIdByPos(p), 1);
		}
	}
	else
	{
		// normal selection
		// (the normal selection is temporary and is removed as the mouse is released)

		m_display.m_selectedIds.Clear();
		m_display.m_selectedAnchorId = queueId;
	}

	UpdateDisplay();
	m_display.m_selectedIdsTimestamp = SjTools::GetMsTicks();
	m_display.m_selectedIdsLockMs = SJ_DISPLAY_SELECTION_LOCK_MS;
}


/*******************************************************************************
 * Simple display messages
 ******************************************************************************/


void SjMainFrame::SetDisplayMsg(const wxString& text, long holdMs)
{
	m_display.m_msg1 = text.BeforeFirst('\n');
	m_display.m_msg2 = text.AfterFirst('\n');
	m_display.m_msgCancelAtTimestamp = SjTools::GetMsTicks()+holdMs;
	UpdateDisplay();
}


