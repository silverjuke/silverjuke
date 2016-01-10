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
 * File:    browser_list.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke list browser
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjtools/imgthread.h>
#include <sjbase/browser.h>
#include <sjbase/browser_list.h>
#include <sjbase/columnmixer.h>
#include <sjmodules/kiosk/kiosk.h>
#include <sjmodules/kiosk/virtkeybd.h>
#include <sjmodules/vis/vis_module.h>


/*******************************************************************************
 * SjListBrowser
 ******************************************************************************/


#define SPACE_TOP               (g_mainFrame->m_currColumnYSpace)
#define SPACE_LEFT              (g_mainFrame->m_currColumnXSpace)
#define SPACE_MID               (g_mainFrame->m_currColumnYSpace)
#define MIN_COLUMN_WIDTH        32
#define MAX_COLUMN_WIDTH        2048


static const long s_possibleCols[] =
{
	 SJ_TI_TRACKNAME
	,SJ_TI_PLAYTIMEMS

	,SJ_TI_LEADARTISTNAME
	,SJ_TI_ORGARTISTNAME
	,SJ_TI_COMPOSERNAME

	,SJ_TI_ALBUMNAME
	,SJ_TI_TRACKNR
	,SJ_TI_TRACKCOUNT
	,SJ_TI_DISKNR
	,SJ_TI_DISKCOUNT

	,SJ_TI_GENRENAME
	,SJ_TI_YEAR
	,SJ_TI_GROUPNAME
	,SJ_TI_BEATSPERMINUTE
	,SJ_TI_RATING
	,SJ_TI_COMMENT

	,SJ_TI_X_TIMESPLAYED
	,SJ_TI_X_LASTPLAYED
	,SJ_TI_X_TIMEADDED
	,SJ_TI_X_TIMEMODIFIED

	,SJ_TI_X_DATABYTES
	,SJ_TI_X_BITRATE
	,SJ_TI_X_SAMPLERATE
	,SJ_TI_X_CHANNELS
	,SJ_TI_X_AUTOVOL

	,SJ_TI_URL
	,SJ_TI_Y_FILETYPE
	,SJ_TI_Y_QUEUEPOS

	,0
};
static bool isAzPossibleForCol(long ti)
{
	if( ti == SJ_TI_TRACKNAME
	        || ti == SJ_TI_LEADARTISTNAME
	        || ti == SJ_TI_ORGARTISTNAME
	        || ti == SJ_TI_COMPOSERNAME
	        || ti == SJ_TI_ALBUMNAME
	        || ti == SJ_TI_GENRENAME
	        || ti == SJ_TI_GROUPNAME
	        || ti == SJ_TI_COMMENT
	        || ti == SJ_TI_URL
	        || ti == SJ_TI_Y_FILETYPE )
	{
		return true;
	}
	return false;
}
static bool isPossibleCol(long ti)
{
	for( int i = 0; s_possibleCols[i]; i++ )
	{
		if( s_possibleCols[i] == ti )
			return true;
	}
	return false;
}


#define IDC_RESET_COL       (IDM_LASTPRIVATE-101)
#define IDC_FIRST_COL_ID    (IDM_LASTPRIVATE-100)
#define IDC_LAST_COL_ID     (IDM_LASTPRIVATE-1)


SjListBrowser::SjListBrowser(SjBrowserWindow* w)
	: SjBrowserBase(w)
{
	m_listView = NULL;
	m_scrollPos = 0;
	m_sortField = SJ_TI_ALBUMNAME;
	m_sortDesc = false;
	m_tracksHScroll = 0;
	m_columnDragIndex = -1;
	m_lastContextMenuTrackIndex = -1;
	m_visibleTrackCount = 1;
	m_tracksNeededToFitCoverHeight = -1;
	m_lastClickedCover = NULL;

	LoadConfig();
}


void SjListBrowser::FreeAllocatedCover()
{
	int i, iCount = m_allocatedCover.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		SjCol* col = (SjCol*)m_allocatedCover[i];
		wxASSERT( col );
		delete col;
	}

	m_allocatedCover.Empty();
	m_lastClickedCover = NULL;
}


void SjListBrowser::LoadConfig()
{
	wxASSERT( m_columns.GetCount() == 0 );
	wxASSERT( m_columnWidths.GetCount() == 0 );

	wxString data_ = g_tools->m_config->Read(wxT("main/columns"), wxT(""));
	wxArrayString data = SjTools::Explode(data_, ',', 4);

	// read sort field
	long l = 0;
	int i;
	if( !data[0].ToLong(&l, 10) ) { l = 0; }
	if( !isPossibleCol(l) )
		goto LoadConfig_Failed;
	m_sortField = l;

	if( !data[1].ToLong(&l, 10) ) { l = 0; }
	m_sortDesc = l!=0;

	// read columns
	for( i = 4; i < (int)data.GetCount(); i += 2 )
	{
		if( !data[i].ToLong(&l, 10) ) { l = 0; }
		if( !isPossibleCol(l) )
			goto LoadConfig_Failed;
		m_columns.Add(l);

		if( !data[i+1].ToLong(&l, 10) ) { l = 0; }
		if( l < MIN_COLUMN_WIDTH || l > MAX_COLUMN_WIDTH)
			l = MIN_COLUMN_WIDTH;
		m_columnWidths.Add(l);
	}

	if( m_columns.GetCount() <= 0 )
		goto LoadConfig_Failed;

	// success
	return;

	// error
LoadConfig_Failed:

	InitCols();
}


void SjListBrowser::SaveConfig()
{
	wxString data = wxString::Format(wxT("%i,%i,0,0"), (int)m_sortField, m_sortDesc? 1 : 0);
	for( int i = 0; i < (int)m_columns.GetCount(); i++ )
	{
		data += wxString::Format(wxT(",%i,%i"), (int)m_columns[i], (int)m_columnWidths[i]);
	}
	g_tools->m_config->Write(wxT("main/columns"), data);
}


void SjListBrowser::SetOrder(long sortField, bool desc)
{
	wxBusyCursor busy;

	if( !isPossibleCol(sortField) )
		return;

	m_sortField = sortField; // this function is also called from extern!
	m_sortDesc = desc;

	wxString pos;
	long posOffset = 0;

	#define KEEP_POSITION 1 // see http://www.silverjuke.net/forum/viewtopic.php?t=1350
	#if KEEP_POSITION
	pos = GetFirstSelectedAndVisiblePos(posOffset);
	#endif

	if( m_listView ) // if m_listView is NULL, the view is not yet realized
	{
		m_listView->ChangeOrder(m_sortField, m_sortDesc);
		CalculatePositions();

		if( pos.IsEmpty() )
			GotoOffset__(0, 0, false/*select*/, true/*update*/, false/*hscroll*/);
		else
			GotoUrl__(pos, posOffset, false/*select*/, true/*update*/, false/*hscroll*/);

		m_selAnchor = -1;
	}
}


void SjListBrowser::SetColumns(const wxArrayLong& newCols)
{
	if( newCols.IsEmpty() )
		return;

	BackupWidths();
	m_columns.Clear();
	m_columnWidths.Clear();
	int i, iCount = newCols.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		if( isPossibleCol(newCols[i])
		        && m_columns.Index(newCols[i]) == wxNOT_FOUND )
		{
			m_columns.Add(newCols[i]);
			m_columnWidths.Add(GetBackupWidth(newCols[i]));
		}
	}

	if( m_columns.IsEmpty() )
		InitCols();

	if( m_listView ) // if m_listView is NULL, the view is not yet realized
	{
		CalcRefreshNUpdateHeaderNTracks();
	}
}


void SjListBrowser::BackupWidths()
{
	int i, iCount = m_columns.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		if( m_columnWidths[i] > 0  )
			m_columnBackupWidths.Insert(m_columns[i], m_columnWidths[i]);
	}
}
long SjListBrowser::GetBackupWidth(long ti)
{
	long w = m_columnBackupWidths.Lookup(ti);
	if( w <= 10 )
		w = 80;
	return w;
}


void SjListBrowser::InitCols()
{
	m_columns.Clear();
	m_columnWidths.Clear();

	m_columns.Add(SJ_TI_TRACKNR);
	m_columnWidths.Add(32);

	m_columns.Add(SJ_TI_TRACKNAME);
	m_columnWidths.Add(160);

	m_columns.Add(SJ_TI_PLAYTIMEMS);
	m_columnWidths.Add(45);

	m_columns.Add(SJ_TI_LEADARTISTNAME);
	m_columnWidths.Add(150);

	m_columns.Add(SJ_TI_ALBUMNAME);
	m_columnWidths.Add(150);

	m_columns.Add(SJ_TI_GENRENAME);
	m_columnWidths.Add(80);

	m_columns.Add(SJ_TI_YEAR);
	m_columnWidths.Add(40);

	m_columns.Add(SJ_TI_X_TIMESPLAYED);
	m_columnWidths.Add(40);

	m_columns.Add(SJ_TI_X_LASTPLAYED);
	m_columnWidths.Add(120);
}


void SjListBrowser::Exit()
{
	if( m_listView )
		delete m_listView;

	SaveConfig();
	ClearTooltips();
	FreeAllocatedCover();
}


void SjListBrowser::Realize(bool reloadColumnMixer, bool keepColIndex)
{
	// first, calculate the number of rows needed to fit the height of a cover
	CalculateFontHeight();
	long tracksNeededToFitCoverHeight = 0;
	if( m_flags&SJ_BROWSER_VIEW_COVER )
		tracksNeededToFitCoverHeight = (m_coverW / m_fontHeight)+1;

	if( m_tracksNeededToFitCoverHeight != tracksNeededToFitCoverHeight )
	{
		m_tracksNeededToFitCoverHeight = tracksNeededToFitCoverHeight;
		reloadColumnMixer = true;
	}

	// reload column mixer?
	if( reloadColumnMixer )
	{
		// for the moment, we have only one list view; for future versions,
		// this may change (current CD-ROM, Radio Stations etc.)

		wxBusyCursor busy; //-- I think, this is normally not needed :-) we're fast enough eg. for 30.000 tracks on my machine (always less than 200 ms to create a view)

		if( m_listView )
			delete m_listView;

		m_listView = g_mainFrame->m_columnMixer.CreateListView(0, m_sortField, m_sortDesc, m_tracksNeededToFitCoverHeight);
	}

	if( m_scrollPos >= (m_listView->GetTrackCount()-m_visibleTrackCount) )
		m_scrollPos = (m_listView->GetTrackCount()-m_visibleTrackCount);

	if( m_scrollPos < 0 || !keepColIndex )
		m_scrollPos = 0;

	m_selAnchor = -1;
	wxASSERT( m_listView );
	CalculatePositions(false /*no font height calculation*/);
}


void SjListBrowser::CalculateFontHeight()
{
	wxClientDC  dc(m_window);
	int dummy2;
	m_window->GetFontPxSizes(dc, m_fontVDiff, dummy2, m_fontHeight);

	// map base cover percentage from GetBaseCoverHeight() to the number of lines
	static const int s_covers[11] = { 3, 3, 4, 4, 5, 5, 6, 6, 7, 7/*def*/, 8 };
	int coversIndex = g_mainFrame->GetBaseCoverHeight() / 10;
	if( coversIndex <  0 ) coversIndex =  0;
	if( coversIndex > 10 ) coversIndex = 10;
	m_coverW = m_fontHeight * s_covers[coversIndex];
}


void SjListBrowser::CalculatePositions(bool calculateFontHeight)
{
	// calculate the current font height
	if( calculateFontHeight )
		CalculateFontHeight();

	// calculate the toggle covers rect
	int x = 0;
	m_toggleCoversRect.x = x;
	m_toggleCoversRect.y = SPACE_TOP;
	m_toggleCoversRect.width = SPACE_LEFT;
	m_toggleCoversRect.height = m_fontHeight;

	// calculate the uptext rect
	m_uptextRect.x = x;
	m_uptextRect.y = SPACE_TOP+m_fontHeight;
	m_uptextRect.width = SPACE_LEFT;
	m_uptextRect.height = m_window->m_clientH - (SPACE_TOP+m_fontHeight);

	x += m_uptextRect.width;

	// calculate the covers rect
	m_coversRect.x = x;
	m_coversRect.y = 0;
	m_coversRect.width = m_coverW;
	m_coversRect.height = m_window->m_clientH;
	if( !(m_flags&SJ_BROWSER_VIEW_COVER) )
	{
		m_coversRect.x = -10000;
	}
	else
	{
		x += m_coversRect.width;
	}

	// calculate the playmark rect
	m_playmarkRect.x = x;
	m_playmarkRect.y = SPACE_TOP;
	m_playmarkRect.width = SPACE_MID;
	m_playmarkRect.height = m_window->m_clientH - SPACE_TOP;

	x += m_playmarkRect.width;

	// calculate the header rect
	m_headerRect.x = x;
	m_headerRect.y = SPACE_TOP;
	m_headerRect.width = m_window->m_clientW - m_headerRect.x;
	m_headerRect.height = m_fontHeight;

	if( m_headerRect.width < 0 )
	{
		m_headerRect.width = 0; // may get negative on window sizes close to zero; as the horizontal thumb rely on this value, we correct it here
	}

	// calculate the tracks rect
	m_tracksRect.x = m_headerRect.x;
	m_tracksRect.y = m_headerRect.y + m_headerRect.height;
	m_tracksRect.width = m_headerRect.width;
	m_tracksRect.height = m_window->m_clientH - m_tracksRect.y;

	m_tracksVirtualW = 0;
	int col, colCount = m_columns.GetCount();
	for( col = 0; col < colCount; col ++ )
	{
		m_tracksVirtualW += m_columnWidths[col];
	}

	// get the number of tracks currently visible
	m_visibleTrackCount = m_tracksRect.height / m_fontHeight;
	if( m_visibleTrackCount < 1 )
		m_visibleTrackCount = 1; // for safety

	if( m_scrollPos > (m_listView->GetTrackCount()-m_visibleTrackCount) )
	{
		m_scrollPos = (m_listView->GetTrackCount()-m_visibleTrackCount);
	}

	if( m_scrollPos < 0 )
	{
		m_scrollPos = 0;
	}

	SetVScrollInfo();

	// set the horizontal scrollbar
	if( m_tracksVirtualW <= m_tracksRect.width )
	{
		m_tracksHScroll = 0;
	}
	else if( m_tracksHScroll > (m_tracksVirtualW-m_tracksRect.width) )
	{
		m_tracksHScroll = m_tracksVirtualW-m_tracksRect.width;
	}
	SetHScrollInfo();
}


/*******************************************************************************
 * Mouse Handling
 ******************************************************************************/


bool SjListBrowser::MouseInHeader(int x, int y, long& colIndex, bool& inSizer,
                                  wxArrayLong* columns, wxArrayLong* widths)
{
	if( columns == NULL )
		columns = &m_columns;

	if( widths == NULL )
		widths = &m_columnWidths;

	#define COLUMNDRAG_WIDTH    10L
	#define COLUMNDRAG_OVERHANG  2L // the number of pixels in the next column
	colIndex = 0;
	inSizer = false;
	if( x >= m_headerRect.x
	        && x <  m_headerRect.x+m_headerRect.width
	        && y >= m_headerRect.y
	        && y <  m_headerRect.y+m_headerRect.height )
	{
		int currLeft = m_headerRect.x - m_tracksHScroll, currRight;
		int col, colCount = columns->GetCount();
		for( col = 0; col < colCount; col ++ )
		{
			currRight = currLeft + widths->Item(col) + COLUMNDRAG_OVERHANG;

			if( x >= currLeft && x < currRight )
			{
				if( x > currRight - COLUMNDRAG_WIDTH )
					inSizer = true;
				colIndex = col;
				return true;
			}

			currLeft = currRight - COLUMNDRAG_OVERHANG;
		}
	}

	return false;
}


bool SjListBrowser::MouseInTrack(int x, int y, long& offset)
{
	if( m_listView
	        && x >= m_tracksRect.x
	        && x <  m_tracksRect.x+m_tracksRect.width
	        && y >= m_tracksRect.y
	        && y <  m_tracksRect.y+m_tracksRect.height )
	{
		long relOffset = (y-m_tracksRect.y) / m_fontHeight;
		offset = relOffset + m_scrollPos;
		if( offset >= 0 && offset < m_listView->GetTrackCount() )
		{
			int currLeft = m_headerRect.x - m_tracksHScroll, currRight;
			int col, colCount = m_columns.GetCount();
			for( col = 0; col < colCount; col ++ )
			{
				currRight = currLeft + m_columnWidths.Item(col);
				if( x >= currLeft && x < currRight )
				{
					return (m_listView->GetTrackSpecial(offset)&SJ_LISTVIEW_SPECIAL_GAP)==0;
				}

				currLeft = currRight;
			}
		}
	}
	return false;
}


bool SjListBrowser::MouseInCover(int x, int y, SjCol** cover)
{
	if( m_listView
	        && x >= m_coversRect.x
	        && x <  m_coversRect.x+m_coversRect.width )
	{
		int i, iCount = m_allocatedCover.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			SjCol* col = (SjCol*)m_allocatedCover[i];
			wxASSERT( col );
			if( y >= col->m_rows[0]->m_top
			        && y <  col->m_rows[0]->m_top+m_coverW )
			{
				if( cover )
					*cover = col;
				return true;
			}
		}
	}

	return false;
}


long SjListBrowser::GetFirstSelectedAndVisible()
{
	long add = (m_tracksRect.height%m_fontHeight)==0? 0 : 1;
	long listViewTrackCount = m_listView->GetTrackCount();
	for( long i = 0; i < m_visibleTrackCount+add; i++ )
	{
		long offset = m_scrollPos+i;
		if( m_listView && offset >= 0 && offset < listViewTrackCount )
		{
			if( m_listView->IsTrackSelected(offset) )
				return offset;
		}
	}
	return -1; // none
}


long SjListBrowser::GetLastSelectedAndVisible(bool alsoUseGaps)
{
	long overhang = (m_tracksRect.height%m_fontHeight)==0? 0 : 1;
	long listViewTrackCount = m_listView->GetTrackCount();
	for( long i = (m_visibleTrackCount-1)+overhang; i >= 0; i-- )
	{
		long offset = m_scrollPos+i;
		if( m_listView && offset >= 0 && offset < listViewTrackCount )
		{
			if(   m_listView->IsTrackSelected(offset)
			        && (alsoUseGaps || !(m_listView->GetTrackSpecial(offset)&SJ_LISTVIEW_SPECIAL_GAP)) )
			{
				return offset;
			}
		}
	}
	return -1; // none
}


void SjListBrowser::SelectRange(long i1, long i2)
{
	long start, end;
	if( i1 < i2 ) { start = i1; end = i2; }
	else          { start = i2; end = i1; }
	g_mainFrame->m_columnMixer.SelectAll(false);
	for( long i = start; i <= end; i++ )
	{
		m_listView->SelectTrack(i, true);
	}
}


void SjListBrowser::OnMouseSelect(wxMouseEvent& event)
{
	bool multiEnqueueAvailable = g_mainFrame->IsOpAvailable(SJ_OP_MULTI_ENQUEUE);

	/* (de-)select the item under the mouse (if any)
	 */
	long index;
	SjCol* cover;
	if( MouseInTrack(event.GetX(), event.GetY(), index) )
	{
		if( multiEnqueueAvailable && event.ShiftDown() )
		{
			if( m_selAnchor == -1 )
			{
				m_selAnchor = GetFirstSelectedAndVisible();
				if( m_selAnchor == -1 )
					m_selAnchor = index;
			}
			SelectRange(m_selAnchor, index);
			RefreshSelection();
		}
		else if( multiEnqueueAvailable && event.ControlDown() )
		{
			m_listView->SelectTrack(index, !m_listView->IsTrackSelected(index));
			RefreshSelection();
			m_selAnchor = index;
		}
		else
		{
			g_mainFrame->m_columnMixer.SelectAll(false);
			m_listView->SelectTrack(index, true);
			RefreshSelection();
			m_selAnchor = index;
		}
	}
	else if( MouseInCover(event.GetX(), event.GetY(), &cover) )
	{
		if( g_mainFrame->m_libraryModule->PlayOnDblClick() )
		{
			g_mainFrame->m_columnMixer.SelectAll(FALSE);
			long index = cover->m_textlLeft; // a little hack - we use this custom var for the index of the view
			long iCount = m_listView->GetTrackCount();

			// select everything up to the next gap
			while(   index < iCount
			         &&   !(m_listView->GetTrackSpecial(index)&SJ_LISTVIEW_SPECIAL_GAP) )
			{
				m_listView->SelectTrack(index, true);
				index++;
			}

			RefreshSelection();
		}
	}
	else if( !event.ShiftDown() && !event.ControlDown() && g_mainFrame->m_columnMixer.IsAnythingSelected() )
	{
		g_mainFrame->m_columnMixer.SelectAll(FALSE);
		RefreshSelection();
	}

	g_mainFrame->m_libraryModule->UpdateMenuBar();
}


void SjListBrowser::OnMouseLeftDown(wxMouseEvent& event)
{
	/* prepare dragscroll and object dragging
	 */
	m_window->m_mouseAction = SJ_ACTION_NONE;
	m_dragStartX            = event.GetX(); /* client coordinates */
	m_dragStartY            = event.GetY(); /* client coordinates */

	/* change column width?
	 */
	long index;
	if( g_mainFrame->IsOpAvailable(SJ_OP_TOGGLE_ELEMENTS) /*toggle columns?*/ )
	{
		bool inSizer;
		if( MouseInHeader(event.GetX(), event.GetY(), index, inSizer) && inSizer )
		{
			m_window->m_mouseAction = SJ_ACTION_COLUMNWIDTH;
			m_columnDragIndex = index;
			m_columnDragOrgWidth = m_columnWidths[index];
			CalcRefreshNUpdateHeader();
			return;
		}
	}

	/* select a track?
	 */
	m_mouseSelectionOnDown = FALSE;
	if(  g_accelModule->m_selDragNDrop == 1
	        &&  MouseInTrack(m_dragStartX, m_dragStartY, index)
	        && !m_listView->IsTrackSelected(index) )
	{
		OnMouseSelect(event);
		m_mouseSelectionOnDown = TRUE;
	}
}


void SjListBrowser::OnMouseLeftUp(wxMouseEvent& event)
{
	if( ResetSpecialMouseAction() )
		return; // done

	// nothing in view
	if( g_mainFrame->m_columnMixer.GetMaskedColCount() == 0 )
	{
		g_mainFrame->EndOneSearch();
		return; // done
	}

	// toggle sort?
	long index;
	bool inSizer;
	if( MouseInHeader(event.GetX(), event.GetY(), index, inSizer) && !inSizer )
	{
		wxBusyCursor busy;
		if( m_columns[index] == m_sortField )
		{
			m_sortDesc = !m_sortDesc;
		}
		else
		{
			m_sortField = m_columns[index];
			m_sortDesc = false;
			if( m_sortField == SJ_TI_PLAYTIMEMS
			        || m_sortField == SJ_TI_YEAR
			        || m_sortField == SJ_TI_X_TIMESPLAYED || m_sortField == SJ_TI_X_LASTPLAYED
			        || m_sortField == SJ_TI_X_TIMEADDED || m_sortField == SJ_TI_X_TIMEMODIFIED
			        || m_sortField == SJ_TI_RATING )
			{
				m_sortDesc = true;
			}
		}
		SetOrder(m_sortField, m_sortDesc);
		return;
	}
	else if( m_toggleCoversRect.Contains(event.GetX(), event.GetY()) )
	{
		g_mainFrame->GetEventHandler()->QueueEvent(new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, IDT_WORKSPACE_SHOW_COVERS));
		return;
	}

	// perform selection
	if( !m_mouseSelectionOnDown )
	{
		OnMouseSelect(event);
		return;
	}
}


void SjListBrowser::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
	ResetSpecialMouseAction();
}
bool SjListBrowser::ResetSpecialMouseAction()
{
	// dragscroll/columnmove/columnwidth: restore cursor if changed by dragscroll
	if( m_window->m_mouseAction == SJ_ACTION_DRAGSCROLL || m_window->m_mouseAction == SJ_ACTION_COLUMNMOVE || m_window->m_mouseAction == SJ_ACTION_COLUMNWIDTH )
	{
		if( m_window->m_mouseAction == SJ_ACTION_COLUMNMOVE || m_window->m_mouseAction == SJ_ACTION_COLUMNWIDTH )
		{
			m_columnDragIndex = -1;
			CalcRefreshNUpdateHeader();
		}

		m_window->m_mouseAction = SJ_ACTION_NONE;
		return true; // resetted
	}

	return false;  // nothing done
}


bool SjListBrowser::OnMouseLeftDClick(wxMouseEvent& event)
{
	int clickX = event.GetX();
	int clickY = event.GetY();
	long colIndex;
	bool inSizer;
	SjCol* cover;
	if( MouseInTrack(clickX, clickY, colIndex) )
	{
		m_listView->OnDoubleClick(colIndex, event.ShiftDown() || event.ControlDown());
		return true; // click used
	}
	else if( MouseInHeader(clickX, clickY, colIndex, inSizer) )
	{
		return true; // click used - by the "normal" clicks (two mouseclicks in the header simply toggle the sorting up and down again)
	}
	else if( MouseInCover(clickX, clickY, &cover) )
	{
		cover->m_rows[0]->OnDoubleClick(event.ShiftDown() || event.ControlDown());
		/*
		g_mainFrame->m_columnMixer.SelectAll(false);
		cover->SelectAllRows(true);
		if( m_window->IsViewAvailable(SJ_BROWSER_ALBUM_VIEW) )
		{
		    m_window->SetView_(SJ_BROWSER_ALBUM_VIEW, true, true);
		}
		else if( m_window->IsViewAvailable(SJ_BROWSER_COVER_VIEW) )
		{
		    m_window->SetView_(SJ_BROWSER_COVER_VIEW, true, true);
		}
		*/
		return true;
	}

	return false; // click not used
}


void SjListBrowser::OnMouseMiddleUp(wxMouseEvent& event)
{
	bool    skip = TRUE;
	int clickX = event.GetX();
	int clickY = event.GetY();
	long index;
	if( MouseInTrack(clickX, clickY, index) )
	{
		// select the item under the mouse
		if( !m_listView->IsTrackSelected(index) )
		{
			g_mainFrame->m_columnMixer.SelectAll(false);
			m_listView->SelectTrack(index, true);
			RefreshSelection();
			m_selAnchor = index;
		}

		m_listView->OnMiddleClick(index, event.ShiftDown() || event.ControlDown());
		skip = false;
	}

	if( skip )
	{
		event.Skip();
	}

	g_mainFrame->m_libraryModule->UpdateMenuBar();
}


void SjListBrowser::OnMouseMotion(wxMouseEvent& event)
{
	long    index;
	bool    inSizer;

	long    xPos = event.GetX(); /* client coordinates */
	long    yPos = event.GetY(); /* client coordinates */

	/* start dragging?
	 */
	long hDifference = xPos - m_dragStartX;
	long vDifference = yPos - m_dragStartY;
	if( m_window->m_mouseAction == SJ_ACTION_NONE && m_window->HasCapture() )
	{
		if( hDifference >  DRAGSCROLL_DELTA
		        || hDifference < -DRAGSCROLL_DELTA
		        || vDifference >  DRAGSCROLL_DELTA
		        || vDifference < -DRAGSCROLL_DELTA )
		{
			if(  MouseInHeader(m_dragStartX, m_dragStartY, index, inSizer)
			        && !inSizer )
			{
				// move columns
				if( g_mainFrame->IsOpAvailable(SJ_OP_TOGGLE_ELEMENTS) /*toggle columns?*/ )
				{
					m_window->m_mouseAction = SJ_ACTION_COLUMNMOVE;
					m_columnDragIndex       = index;
					CalcRefreshNUpdateHeader();
				}
			}
			else if( MouseInTrack(m_dragStartX, m_dragStartY, index)
			         && m_listView->IsTrackSelected(index) )
			{
				// do object dragging
				m_window->m_dragUrls.Clear();
				g_mainFrame->m_columnMixer.GetSelectedUrls(m_window->m_dragUrls);
				if( g_accelModule->m_flags&SJ_ACCEL_USE_DND_IMAGES )
				{
					g_mainFrame->m_dragImage = new wxDragImage(GetDragNDropBitmap(m_dragStartX, m_dragStartY, g_mainFrame->m_dragHotspot, g_mainFrame->m_dragRect));
				}

				if( g_mainFrame->DragNDrop(SJ_DND_ENTER, m_window, event.GetPosition(), NULL, &m_window->m_dragUrls) )
				{
					m_window->m_mouseAction = SJ_ACTION_DRAGNDROP;
				}
			}
			else if( g_accelModule->m_flags&SJ_ACCEL_CONTENT_DRAG
			         && m_listView->GetTrackCount() > 0 )
			{
				// start dragscroll
				m_dragScrollOrgHScroll = m_tracksHScroll;
				m_dragScrollOrgScrollPos = m_scrollPos;
				m_window->m_mouseAction = SJ_ACTION_DRAGSCROLL;
			}
		}
	}

	/* in drag'n'drop?
	 */
	if( m_window->m_mouseAction == SJ_ACTION_DRAGNDROP )
	{
		if( !g_mainFrame->DragNDrop(SJ_DND_MOVE, m_window, event.GetPosition(), NULL, &m_window->m_dragUrls) )
		{
			m_window->m_mouseAction = SJ_ACTION_NONE;
		}
	}


	/* in dragscroll?
	 */
	if( m_window->m_mouseAction == SJ_ACTION_DRAGSCROLL )
	{
		OnHScroll(IDT_WORKSPACE_H_SCROLL, m_dragScrollOrgHScroll-hDifference, true);
		OnVScroll(IDT_WORKSPACE_V_SCROLL, m_dragScrollOrgScrollPos-((vDifference+m_fontHeight/2)/m_fontHeight), true);
		return;
	}

	/* change column width
	 */
	if( g_mainFrame->IsOpAvailable(SJ_OP_TOGGLE_ELEMENTS) /*toggle columns?*/ )
	{
		if( !m_window->HasCapture() )
		{
			if( MouseInHeader(xPos, yPos, index, inSizer) && inSizer )
			{
				if( !m_window->m_cursorChanged )
				{
					m_window->SetCursor(g_tools->m_staticResizeWECursor);
					m_window->m_cursorChanged = true;
				}
			}
			else
			{
				m_window->ResetCursor();
			}
		}
		else if( m_window->m_mouseAction == SJ_ACTION_COLUMNWIDTH )
		{
			long diff = xPos - m_dragStartX;
			m_columnWidths[m_columnDragIndex] = m_columnDragOrgWidth + diff;
			if( m_columnWidths[m_columnDragIndex] < MIN_COLUMN_WIDTH )
				m_columnWidths[m_columnDragIndex] = MIN_COLUMN_WIDTH;

			CalcRefreshNUpdateHeaderNTracks();
			return;
		}
	}

	/* in dragscroll?
	 */
	if( m_window->m_mouseAction == SJ_ACTION_COLUMNMOVE )
	{
		if( MouseInHeader(xPos, yPos, index, inSizer)
		        && !inSizer
		        && index != m_columnDragIndex )
		{
			// change the column order ...
			wxArrayLong newColumns = m_columns;
			wxArrayLong newWidths  = m_columnWidths;

			long oldField = newColumns[m_columnDragIndex];
			long oldWidth = newWidths[m_columnDragIndex];

			newColumns.RemoveAt(m_columnDragIndex);
			newWidths.RemoveAt(m_columnDragIndex);

			newColumns.Insert(oldField, index);
			newWidths.Insert(oldWidth, index);

			// check if the mouse is REALLY over the new position
			long testIndex;
			if( MouseInHeader(xPos, yPos, testIndex, inSizer, &newColumns, &newWidths)
			        && !inSizer
			        && testIndex == index )
			{
				m_columns = newColumns;
				m_columnWidths = newWidths;
				m_columnDragIndex = index;
				CalcRefreshNUpdateHeaderNTracks();
			}
		}

		return;
	}
}


void SjListBrowser::OnMouseWheel(wxMouseEvent& event, bool scrollVert)
{
	long rotation = event.GetWheelRotation();
	long delta = event.GetWheelDelta();

	if( rotation != 0 && delta > 0 )
	{
		if( scrollVert )
		{
			// We take action 4 times faster as normal to make things more comparable with
			// the scrolling in the album view (line-by-line scrolling is too slow and page scrolling is too fast)
			delta /= 4;

			// add multiple small rotations (smaller than the delta to take action) to bigger ones
			static long s_addedRotation = 0;
			if( (rotation < 0 && s_addedRotation > 0) || (rotation > 0 && s_addedRotation < 0) )
			{
				s_addedRotation = 0; // discard saved scrolling for the wrong direction
			}
			s_addedRotation += rotation;

			long rotateLines = s_addedRotation/delta;
			if( rotateLines != 0 )
			{
				s_addedRotation -= rotateLines*delta;
				OnVScroll(IDT_WORKSPACE_V_SCROLL, m_scrollPos + rotateLines*-1, TRUE/*redraw*/);
			}
		}
		else
		{
			OnHScroll(IDT_WORKSPACE_H_SCROLL, m_tracksHScroll + rotation, TRUE/*redraw*/);
		}
	}
}


void SjListBrowser::OnDropImage(SjDataObject* data, int mouseX, int mouseY)
{
	SjCol* cover;
	if( MouseInCover(mouseX, mouseY, &cover) )
	{
		if( cover->m_rows[0]->OnDropData(data) )
		{
			Realize(false, true); // true = keep col index
			m_window->Refresh();
			m_window->Update();
		}
	}
}


void SjListBrowser::AddItemsToColMenu(SjMenu* m)
{
	m->AppendSeparator();
	for( int i = 0; s_possibleCols[i]; i++ )
	{
		m->AppendCheckItem(IDC_FIRST_COL_ID+i, SjTrackInfo::GetFieldDescr(s_possibleCols[i]));
	}
	m->AppendSeparator();
	m->Append(IDC_RESET_COL, _("Reset to default values"));
	UpdateItemsInColMenu(m);
}


void SjListBrowser::UpdateItemsInColMenu(SjMenu* m)
{
	for( int i = 0; s_possibleCols[i]; i++ )
	{
		m->Check(IDC_FIRST_COL_ID+i, m_columns.Index(s_possibleCols[i]) != wxNOT_FOUND);
	}
}


void SjListBrowser::OnContextMenu(int clickX, int clickY)
{
	SjMenu  mainMenu(0);
	bool    inSizer;

	long index;
	m_columnDragIndex = -1;
	m_lastClickedCover = NULL;
	SjCol* cover;
	if( MouseInTrack(clickX, clickY, index) )
	{
		// select the item under the mouse
		if( !m_listView->IsTrackSelected(index) )
		{
			g_mainFrame->m_columnMixer.SelectAll(false);
			m_listView->SelectTrack(index, true);
			RefreshSelection();
			m_selAnchor = index;
		}

		// build the context menu
		m_listView->CreateContextMenu(index, mainMenu);
		m_lastContextMenuTrackIndex = index;
	}
	else if( MouseInHeader(clickX, clickY, index, inSizer) )
	{
		// build the header context menu
		if( g_mainFrame->IsOpAvailable(SJ_OP_TOGGLE_ELEMENTS) /*toggle columns?*/ )
		{
			AddItemsToColMenu(&mainMenu);
		}

		m_lastContextMenuColIndex = index;
		m_columnDragIndex = index;
		m_window->Refresh(false, &m_headerRect);
	}
	else if( MouseInCover(clickX, clickY, &cover) )
	{
		cover->m_rows[0]->CreateContextMenu(mainMenu);
		m_lastClickedCover = cover;
	}

	// add main items to menu
	g_mainFrame->CreateContextMenu_(mainMenu);

	// show menu
	if( mainMenu.GetMenuItemCount() )
	{
		m_window->PopupMenu(&mainMenu);
	}

	// deselect header (if selected before)
	if( m_columnDragIndex != -1 )
	{
		m_columnDragIndex = -1;
		m_window->Refresh(false, &m_headerRect);
	}

	g_mainFrame->m_libraryModule->UpdateMenuBar();
}
void SjListBrowser::OnContextMenuSelect(int id)
{
	if( m_lastClickedCover )
	{
		m_lastClickedCover->m_rows[0]->OnContextMenu(id);
		m_lastClickedCover = NULL;
	}
	else if( id == IDC_RESET_COL )
	{
		if( g_mainFrame->IsOpAvailable(SJ_OP_TOGGLE_ELEMENTS) /*toggle columns?*/ )
		{
			InitCols();
			CalcRefreshNUpdateHeaderNTracks();
		}

		if( g_mainFrame->m_viewMenu )
		{
			UpdateItemsInColMenu(g_mainFrame->m_viewMenu);
		}
	}
	else if( id>=IDC_FIRST_COL_ID && id<=IDC_LAST_COL_ID )
	{
		// toggle columns
		if( g_mainFrame->IsOpAvailable(SJ_OP_TOGGLE_ELEMENTS) /*toggle columns?*/ )
		{
			long ti = s_possibleCols[id-IDC_FIRST_COL_ID];
			if( m_lastContextMenuColIndex < 0 || m_lastContextMenuColIndex >= (long)m_columns.GetCount() )
				m_lastContextMenuColIndex = 0;

			long index = m_columns.Index(ti);
			if( index == wxNOT_FOUND )
			{
				// switch column ON
				m_columns.Insert(ti, m_lastContextMenuColIndex);
				m_columnWidths.Insert(GetBackupWidth(ti), m_lastContextMenuColIndex);
			}
			else if( m_columns.GetCount() > 1 )
			{
				// switch column OFF
				BackupWidths();
				m_columns.RemoveAt(index);
				m_columnWidths.RemoveAt(index);
			}
			CalcRefreshNUpdateHeaderNTracks();
		}

		if( g_mainFrame->m_viewMenu )
		{
			UpdateItemsInColMenu(g_mainFrame->m_viewMenu);
		}
	}
	else
	{
		// forward to library module
		if( m_lastContextMenuTrackIndex < 0 || m_lastContextMenuTrackIndex >= m_listView->GetTrackCount() )
			m_lastContextMenuTrackIndex = GetFirstSelectedAndVisible();

		if( m_lastContextMenuTrackIndex != -1 )
			m_listView->OnContextMenu(m_lastContextMenuTrackIndex, id);
	}
}


class SjListTooltip
{
public:
	SjListTooltip(const wxRect& r, const wxString& s) { m_rect = r; m_string = s; }
	wxRect      m_rect;
	wxString    m_string;
};
void SjListBrowser::ClearTooltips()
{
	long i, iCount = m_tooltips.GetCount();
	SjListTooltip* t;
	for( i = 0; i < iCount; i++ )
	{
		t = (SjListTooltip*)m_tooltips.Item(i);
		delete t;
	}
	m_tooltips.Empty();
}
void SjListBrowser::AddToTooltips(const wxRect& r, const wxString& s)
{
	m_tooltips.Add(new SjListTooltip(r, s));
}
SjListTooltip* SjListBrowser::FindTooltip(int mouseX, int mouseY)
{
	long i, iCount = m_tooltips.GetCount();
	SjListTooltip* t;
	for( i = 0; i < iCount; i++ )
	{
		t = (SjListTooltip*)m_tooltips.Item(i);
		if( t->m_rect.Contains(mouseX, mouseY) )
			return t;
	}
	return NULL;
}


wxRect SjListBrowser::GetToolTipRect(int mouseX, int mouseY)
{
	wxRect retRect;
	retRect.x = -1000;
	retRect.y = -1000;
	retRect.width = 1;
	retRect.height = 1;

	if( m_headerRect.Contains(mouseX, mouseY)
	        || m_tracksRect.Contains(mouseX, mouseY)
	        || m_toggleCoversRect.Contains(mouseX, mouseY) )
	{
		SjListTooltip* t = FindTooltip(mouseX, mouseY);
		if( t )
		{
			retRect = t->m_rect;
		}
	}

	return retRect;
}


wxString SjListBrowser::GetToolTipText(int mouseX, int mouseY, long &flags)
{
	wxString retString;

	if( m_headerRect.Contains(mouseX, mouseY)
	        || m_tracksRect.Contains(mouseX, mouseY)
	        || m_toggleCoversRect.Contains(mouseX, mouseY) )
	{
		SjListTooltip* t = FindTooltip(mouseX, mouseY);
		if( t )
		{
			retString = t->m_string;
		}
	}

	retString.Replace(wxT("\t"), wxT("")); // TAB is used for hiliting;
	return retString;
}


/*******************************************************************************
 *  selection handling & scrolling
 ******************************************************************************/


bool SjListBrowser::OnSkinTargetEvent(int targetId, SjSkinValue& value, long accelFlags)
{
	bool shiftSelection = g_mainFrame->IsOpAvailable(SJ_OP_MULTI_ENQUEUE)? ((accelFlags&wxACCEL_SHIFT)!=0) : false;
	if( targetId >= IDT_WORKSPACE_GOTO_A && targetId <= IDT_WORKSPACE_GOTO_0_9 )
	{
		g_visModule->StopVisIfOverWorkspace();
		g_mainFrame->EndSimpleSearch();

		bool azSet = false;
		if( isAzPossibleForCol(m_sortField) )
		{
			long offset = m_listView->Az2TrackOffset('a'+(targetId-IDT_WORKSPACE_GOTO_A));
			if( offset != -1 )
			{
				// g_mainFrame->m_columnMixer.SelectAll(false); -- is this usefull? browser_album.cpp does this not.  If we look at A-Z same as scrolling we should leave the selection.
				GotoOffset__(offset, 0, false/*select*/, true/*update*/, true/*hscroll*/);
				azSet = true;
			}
		}

		if( !azSet )
			g_mainFrame->SetSkinAzValues(0);

		m_window->SetFocus();
		return true;
	}
	else switch( targetId )
		{
			case IDT_WORKSPACE_GOTO_RANDOM:
				g_visModule->StopVisIfOverWorkspace();
				g_mainFrame->EndSimpleSearch();
				GotoOffset__(SjTools::Rand(m_listView->GetTrackCount()),
				             SjTools::Rand(m_visibleTrackCount) | (SJ_BROWSER_LIST_VIEW<<24), true, true, true);
				m_window->SetFocus();
				return true;;

			case IDT_WORKSPACE_V_SCROLL:
			case IDT_WORKSPACE_LINE_UP:
			case IDT_WORKSPACE_LINE_DOWN:
			case IDT_WORKSPACE_PAGE_UP:
			case IDT_WORKSPACE_PAGE_DOWN:
			case IDT_WORKSPACE_HOME:
			case IDT_WORKSPACE_END:
				OnVScroll(targetId, value.value, TRUE /*redraw*/);
				return true;;

			case IDT_WORKSPACE_H_SCROLL:
			case IDT_WORKSPACE_LINE_LEFT:
			case IDT_WORKSPACE_LINE_RIGHT:
			case IDT_WORKSPACE_PAGE_LEFT:
			case IDT_WORKSPACE_PAGE_RIGHT:
			case IDT_WORKSPACE_MINOR_HOME:
			case IDT_WORKSPACE_KEY_LEFT:
			case IDT_WORKSPACE_KEY_RIGHT:
				OnHScroll(targetId, value.value, TRUE /*redraw*/);
				return true;;

			case IDT_WORKSPACE_KEY_UP:
				if( !g_mainFrame->IsWorkspaceMovedAway() )
					DoChangeSelection(SJ_SEL_UP, shiftSelection);
				return true;;

			case IDT_WORKSPACE_KEY_DOWN:
				if( !g_mainFrame->IsWorkspaceMovedAway() )
					DoChangeSelection(SJ_SEL_DOWN, shiftSelection);
				return true;;
		}

	return false;
}


void SjListBrowser::OnHScroll(int nScrollCode, int nPos, bool redraw)
{
	// returns TRUE if scrolling appears
	long newIndex = m_tracksHScroll;

	if( nScrollCode == IDT_WORKSPACE_MINOR_HOME )
	{
		newIndex = 0;
	}
	else if( nScrollCode == IDT_WORKSPACE_MINOR_END )
	{
		newIndex = m_tracksVirtualW; // sth. substracted below
	}
	else if( nScrollCode == IDT_WORKSPACE_LINE_LEFT || nScrollCode == IDT_WORKSPACE_KEY_LEFT )
	{
		newIndex -= g_mainFrame->m_currFontSize*2;
	}
	else if( nScrollCode == IDT_WORKSPACE_LINE_RIGHT || nScrollCode == IDT_WORKSPACE_KEY_RIGHT )
	{
		newIndex += g_mainFrame->m_currFontSize*2;
	}
	else if( nScrollCode == IDT_WORKSPACE_PAGE_LEFT )
	{
		newIndex -= m_tracksRect.width;
	}
	else if( nScrollCode == IDT_WORKSPACE_PAGE_RIGHT )
	{
		newIndex += m_tracksRect.width;
	}
	else if( nScrollCode == IDT_WORKSPACE_H_SCROLL )
	{
		newIndex = nPos;
	}

	if( newIndex < 0 )
	{
		newIndex = 0;
	}

	if( newIndex > m_tracksVirtualW - m_tracksRect.width )
	{
		newIndex = m_tracksVirtualW - m_tracksRect.width;
		if( newIndex < 0 )
		{
			newIndex = 0;
		}
	}

	if( newIndex != m_tracksHScroll )
	{
		m_tracksHScroll = newIndex;
		SetHScrollInfo();

		if( redraw )
		{
			m_window->Refresh(FALSE, &m_headerRect);
			m_window->Refresh(FALSE, &m_tracksRect);
			m_window->Update();
		}
	}
}
void SjListBrowser::SetHScrollInfo()
{

	SjSkinValue value;
	value.value         = m_tracksHScroll;
	value.vmax          = m_tracksVirtualW;
	value.thumbSize     = m_tracksRect.width;
	g_mainFrame->SetSkinTargetValue(IDT_WORKSPACE_H_SCROLL, value);
}


void SjListBrowser::OnVScroll(int nScrollCode, int nPos, bool redraw)
{
	// returns TRUE if scrolling appears
	long newIndex = m_scrollPos;

	if( nScrollCode == IDT_WORKSPACE_HOME )
	{
		newIndex = 0;
	}
	else if( nScrollCode == IDT_WORKSPACE_END )
	{
		newIndex = m_listView->GetTrackCount(); // sth. substracted below
	}
	else if( nScrollCode == IDT_WORKSPACE_LINE_UP )
	{
		newIndex --;
	}
	else if( nScrollCode == IDT_WORKSPACE_LINE_DOWN )
	{
		newIndex ++;
	}
	else if( nScrollCode == IDT_WORKSPACE_PAGE_UP )
	{
		newIndex -= m_visibleTrackCount;
	}
	else if( nScrollCode == IDT_WORKSPACE_PAGE_DOWN )
	{
		newIndex += m_visibleTrackCount;
	}
	else if( nScrollCode == IDT_WORKSPACE_V_SCROLL )
	{
		newIndex = nPos;
	}

	if( newIndex < 0 )
	{
		newIndex = 0;
	}

	if( newIndex > m_listView->GetTrackCount() - m_visibleTrackCount )
	{
		newIndex = m_listView->GetTrackCount() - m_visibleTrackCount;
		if( newIndex < 0 )
		{
			newIndex = 0;
		}
	}

	if( newIndex != m_scrollPos )
	{
		m_scrollPos = newIndex;
		SetVScrollInfo();

		if( redraw )
		{
			m_window->Refresh(FALSE, &m_tracksRect);

			if( m_flags&SJ_BROWSER_VIEW_COVER )
			{
				m_window->Refresh(FALSE, &m_coversRect);
				m_window->Refresh(FALSE, &m_playmarkRect);
			}

			m_window->Update();
		}
	}
}
void SjListBrowser::SetVScrollInfo()
{

	SjSkinValue value;
	value.value         = m_scrollPos;
	value.vmax          = m_listView->GetTrackCount();
	value.thumbSize     = m_visibleTrackCount;
	g_mainFrame->SetSkinTargetValue(IDT_WORKSPACE_V_SCROLL, value);
}


void SjListBrowser::ScrollOneLineInView(long index)
{
	// scroll max. one line up/down to put the index in view
	if( index < m_scrollPos )
	{
		OnVScroll(IDT_WORKSPACE_LINE_UP, 0, true /*redraw*/);
	}
	else if( index >= (m_scrollPos+m_visibleTrackCount) )
	{
		OnVScroll(IDT_WORKSPACE_LINE_DOWN, 0, true /*redraw*/);
	}
}


bool SjListBrowser::DoChangeSelection(long dir, bool selectShifted)
{
	bool up = (dir == SJ_SEL_UP || dir == SJ_SEL_LEFT || dir == SJ_SEL_PREV);

	if( selectShifted && m_selAnchor != -1 )
	{
		long firstSel = m_selAnchor, lastSel = m_selAnchor, newPos;
		while( firstSel > 0 )
		{
			if( !m_listView->IsTrackSelected(firstSel-1) )
				break;
			firstSel--;
		}

		while( lastSel < (m_listView->GetTrackCount()-2) )
		{
			if( !m_listView->IsTrackSelected(lastSel+1) )
				break;
			lastSel++;
		}

		if( up )
		{
			if( lastSel == m_selAnchor )
			{
				newPos = firstSel-1;
				if( newPos < 0 )
					return false;
				m_listView->SelectTrack(newPos, true);
			}
			else
			{
				newPos = lastSel;
				if( newPos < 0 )
					return false;
				m_listView->SelectTrack(newPos, false);
			}
		}
		else
		{
			if( firstSel == m_selAnchor )
			{
				newPos = lastSel+1;
				if( newPos >= m_listView->GetTrackCount() )
					return false;
				m_listView->SelectTrack(newPos, true);
			}
			else
			{
				newPos = firstSel;
				if( newPos < 0 )
					return false;
				m_listView->SelectTrack(newPos, false);
			}
		}

		ScrollOneLineInView(newPos);
		RefreshSelection();
		g_mainFrame->m_libraryModule->UpdateMenuBar();
		return true;
	}

	long newPos;
	if( up )
	{
		newPos = GetFirstSelectedAndVisible();
		if( newPos != -1 )
		{
			// change selection
			newPos --;
			if( newPos < 0 )
				return false;
		}
		else
		{
			// start a new selection, before http://www.silverjuke.net/forum/topic-2892.html we used "m_scrollPos - 1" here
			newPos = m_scrollPos;
			if( newPos < 0 )
				return false;
		}
	}
	else
	{
		newPos = GetLastSelectedAndVisible();
		if( newPos != -1 )
		{
			// change selection
			if( newPos >= (m_scrollPos+m_visibleTrackCount) )
				newPos --;

			newPos ++;
			if( newPos >= m_listView->GetTrackCount() )
				return false;
		}
		else
		{
			// start a new selection, we're selection the first track here (as we do in the album view), see http://www.silverjuke.net/forum/topic-2892.html
			newPos = m_scrollPos;
			if( newPos < 0 )
				return false;
		}
	}

	ScrollOneLineInView(newPos);
	m_selAnchor = newPos;
	g_mainFrame->m_columnMixer.SelectAll(false);

	m_listView->SelectTrack(newPos, true);
	RefreshSelection();
	g_mainFrame->m_libraryModule->UpdateMenuBar();

	return true;
}


bool SjListBrowser::GotoOffset__(long offset, long viewOffset, bool select, bool update, bool hscroll)
{
	if( (viewOffset>>24) != SJ_BROWSER_LIST_VIEW )
		viewOffset = 0;
	else
		viewOffset &= 0x00FFFFFFL;

	if( offset < 0 || offset >= m_listView->GetTrackCount() )
		return false;

	// goto column
	OnVScroll(IDT_WORKSPACE_V_SCROLL, offset-viewOffset, FALSE/*redraw*/);
	if( hscroll )
	{
		OnHScroll(IDT_WORKSPACE_H_SCROLL, 0, FALSE/*redraw*/);
	}

	// select the correct row
	if( select )
	{
		g_mainFrame->m_columnMixer.SelectAll(FALSE);
		m_listView->SelectTrack(offset, true);
	}

	// update the view
	if( update )
	{
		m_window->Refresh();
		m_window->Update();
	}

	// success
	return true;
}


bool SjListBrowser::GotoUrl__(const wxString& url, long viewOffset, bool select, bool update, bool hscroll)
{
	if( m_listView )
	{
		long offset = m_listView->Url2TrackOffset(url);
		if( offset != -1 )
		{
			GotoOffset__(offset, viewOffset, select, update, hscroll);
			return true;
		}
	}

	return false;
}


wxString SjListBrowser::GetFirstVisiblePos()
{
	if( m_listView )
	{
		SjTrackInfo ti;
		long albumId, special;
		m_listView->GetTrack(m_scrollPos, ti, albumId, special);
		return ti.m_url;
	}
	return wxEmptyString;
}


wxString SjListBrowser::GetFirstSelectedOrVisiblePos(long& retViewOffset)
{
	retViewOffset = 0;

	if( m_listView )
	{
		long offset = GetFirstSelectedAndVisible();
		if( offset == -1 )
			return GetFirstVisiblePos();

		SjTrackInfo ti;
		long albumId, special;
		m_listView->GetTrack(offset, ti, albumId, special);
		retViewOffset = (offset-m_scrollPos) | (SJ_BROWSER_LIST_VIEW<<24);
		return ti.m_url;
	}

	wxASSERT( 0 );
	return wxEmptyString;
}


wxString SjListBrowser::GetFirstSelectedAndVisiblePos(long& retViewOffset)
{
	retViewOffset = 0;

	if( m_listView )
	{
		long offset = GetFirstSelectedAndVisible();
		if( offset != -1 )
		{
			SjTrackInfo ti;
			long albumId, special;
			m_listView->GetTrack(offset, ti, albumId, special);
			retViewOffset = (offset-m_scrollPos) | (SJ_BROWSER_LIST_VIEW<<24);
			return ti.m_url;
		}
	}

	return wxEmptyString;
}


/*******************************************************************************
 * Painting the window
 ******************************************************************************/


#define SPACE_BRUSH g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgBrush
//#define SPACE_BRUSH (*wxGREEN_BRUSH)


void SjListBrowser::OnSize(wxSizeEvent& event)
{
	CalculatePositions();
}


void SjListBrowser::OnImageThere(SjImageThereEvent& event)
{
	SjImgThreadObj* obj = event.GetObj();

	int i, iCount = m_allocatedCover.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		SjCol* col = (SjCol*)m_allocatedCover[i];
		wxASSERT( col );
		wxASSERT( col->m_rowCount > 0 );

		if( col->m_rows[0]->m_textm == obj->m_url
		        && (m_flags&SJ_BROWSER_VIEW_COVER) )
		{
			wxClientDC dc(m_window);
			m_window->PaintCover(dc, obj, m_coversRect.x, col->m_rows[0]->m_top, m_coverW);
		}
	}

	g_mainFrame->m_imgThread->ReleaseImage(m_window, obj);
}


void SjListBrowser::DoPaintHeader(wxDC& dc, long x, long y, long w, long h, bool addTooltips)
{
	dc.SetPen(*wxTRANSPARENT_PEN);

	// draw the columns
	wxString str;
	wxRect drawRect;
	long currX = x;
	int col, colCount = m_columns.GetCount();
	for( col = 0; col < colCount; col ++ )
	{
		// get information
		if( m_columns[col] == SJ_TI_TRACKNR )
		{
			str = _("Nr.");
		}
		else if( m_columns[col] == SJ_TI_X_BITRATE )
		{
			// TRANSLATORS: Abbreviation of "Bits per second"
			str = _("bit/s");
		}
		else if( m_columns[col] == SJ_TI_X_SAMPLERATE )
		{
			str = _("Samplerate") + wxString(wxT("/")) +
			      // TRANSLATORS: Abbreviation of "Hertz"
			      _("Hz");
		}
		else
		{
			str = SjTrackInfo::GetFieldDescr(m_columns[col]);
		}

		bool selected = (col == m_columnDragIndex);

		// recreate complete draw rect as it gets modified by DrawText();
		drawRect.x = currX;
		drawRect.y = y;
		drawRect.height = h;
		drawRect.width = m_columnWidths[col];

		// draw background
		dc.SetBrush(g_mainFrame->m_workspaceColours[selected? SJ_COLOUR_SELECTION : SJ_COLOUR_NORMAL].bgBrush);
		SjSkinColour* colour = &g_mainFrame->m_workspaceColours[selected? SJ_COLOUR_SELECTION : SJ_COLOUR_TITLE1];
		dc.SetTextBackground(colour->bgColour);
		dc.SetTextForeground(colour->fgColour);
		if( selected )
		{
			drawRect.height --;
		}
		dc.DrawRectangle(drawRect);
		if( selected )
		{
			dc.SetPen(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgPen);
			//dc.SetPen(*wxGREEN_PEN);
			dc.DrawLine(drawRect.x, drawRect.y+drawRect.height, drawRect.x+drawRect.width, drawRect.y+drawRect.height);
			dc.SetPen(*wxTRANSPARENT_PEN);
			drawRect.height ++;
		}

		// draw text
		if( m_columns[col] == m_sortField )
		{
			wxRect iconRect = drawRect;
			iconRect.y+=2;
			iconRect.height-=4;
			iconRect.width = iconRect.height;

			dc.SetPen(g_mainFrame->m_workspaceColours[selected? SJ_COLOUR_SELECTION : SJ_COLOUR_TITLE1].fgPen);
			SjTools::DrawIcon(dc, iconRect, m_sortDesc? SJ_DRAWICON_TRIANGLE_DOWN : SJ_DRAWICON_TRIANGLE_UP);
			dc.SetPen(*wxTRANSPARENT_PEN);

			drawRect.x += iconRect.width+2;
			drawRect.width -= iconRect.width+2;
		}
		drawRect.width -= 2;
		if( g_tools->DrawSingleLineText(dc, str, drawRect, g_mainFrame->m_currBoldFont,  g_mainFrame->m_currSmallBoldFont, NULL, colour->hiColour) )
		{
			if( addTooltips )
				AddToTooltips(drawRect, str);
		}

		// next
		currX += m_columnWidths[col];
	}

	// draw space aright
	if( currX < x+w )
	{
		dc.SetBrush(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgBrush);
		dc.DrawRectangle(currX, y, (x+w)-currX, h);
	}
}


void SjListBrowser::DoPaintTrack(wxDC& dc, long x, long y, long w, long h, long offset,
                                 bool& lastTrackSelected,
                                 bool addTooltips, bool setAz, int* lastAz,
                                 wxArrayLong* retLinePos, wxArrayLong* retAlbumIds)
{
	SjTrackInfo  ti;
	// get the track information
	long tiAlbumId, tiSpecial;
	m_listView->GetTrack(offset, ti, tiAlbumId, tiSpecial);
	bool tiGap = (tiSpecial&SJ_LISTVIEW_SPECIAL_GAP)!=0;
	bool tiSelected = m_listView->IsTrackSelected(offset);
	if( retAlbumIds )
		retAlbumIds->Add(tiSpecial&SJ_LISTVIEW_SPECIAL_FIRST_TRACK? tiAlbumId : 0);

	// set A-Z
	bool hiliteFirstChar = false;
	wxString str;
	int thisAz = 0;
	if( isAzPossibleForCol(m_sortField) ) // can sort by A-Z?
	{
		str = SjNormaliseString(ti.GetFormattedValue(m_sortField), 0);
		thisAz = str.Len()? str[0] : 'z'+1;
		if( thisAz < 'a' || thisAz > 'z' )
		{
			thisAz = 'z'+1;
		}
	}

	if( lastAz != NULL
	 && thisAz
	 && thisAz != *lastAz )
	{
		if( thisAz != 'z'+1 )
		{
			hiliteFirstChar = true;
		}

		*lastAz = thisAz;
	}

	if(  setAz
	 && !g_mainFrame->HasSimpleSearch())
	{
		g_mainFrame->SetSkinAzValues(thisAz);
	}

	// set objects to use
	SjSkinColour* colour;
	if( tiSpecial&SJ_LISTVIEW_SPECIAL_DARK )
	{
		colour = &g_mainFrame->m_workspaceColours[tiSelected? SJ_COLOUR_SELECTIONODD : SJ_COLOUR_NORMALODD];
	}
	else
	{
		colour = &g_mainFrame->m_workspaceColours[(tiSelected&&!tiGap)? SJ_COLOUR_SELECTION : SJ_COLOUR_NORMAL];
	}

	dc.SetBrush(colour->bgBrush);
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetTextBackground(colour->bgColour);
	dc.SetTextForeground(colour->fgColour);

	wxRect drawRect;
	int col, colCount = m_columns.GetCount();
	long currX = x, field;
	wxFont* largeFont;
	wxFont* boldFont;
	bool showNumpadNumbers = (g_kioskModule && g_accelModule && g_accelModule->UseNumpad());
	for( col = 0; col < colCount; col ++ )
	{
		// get information
		field = m_columns[col];
		str = ti.GetFormattedValue(field);
		if( showNumpadNumbers && field == SJ_TI_TRACKNR )
		{
			// TODO: hier sollte mal die "richtige" nummer im Numpad-Modus angezeigt werden ...
			//str = wxT("x");
		}

		largeFont = &g_mainFrame->m_currSmallFont;
		boldFont  = &g_mainFrame->m_currSmallBoldFont;
		if( field == SJ_TI_TRACKNAME
		        || field == SJ_TI_LEADARTISTNAME
		        || field == SJ_TI_ORGARTISTNAME
		        || field == SJ_TI_COMPOSERNAME
		        || field == SJ_TI_ALBUMNAME )
		{
			largeFont = &g_mainFrame->m_currStdFont;
			boldFont = &g_mainFrame->m_currBoldFont;
		}

		// recreate complete draw rect as it gets modified by DrawText();
		drawRect.x = currX;
		drawRect.y = y;
		drawRect.height = h;
		drawRect.width = m_columnWidths[col];

		// draw background
		bool drawLine = false;
		if( lastTrackSelected && tiSelected && !tiGap )
		{
			dc.SetPen(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgPen);
			dc.DrawLine(drawRect.x, drawRect.y, drawRect.x+drawRect.width, drawRect.y);
			dc.SetPen(*wxTRANSPARENT_PEN);
			drawLine = true;
		}

		dc.DrawRectangle(drawRect.x, drawRect.y+(drawLine?1:0), drawRect.width, drawRect.height-(drawLine?1:0));

		// draw text
		if( !tiGap )
		{
			drawRect.width -= 2;
			if( g_tools->DrawSingleLineText(dc, str, drawRect, *largeFont, g_mainFrame->m_currSmallFont,
			                                (hiliteFirstChar && m_sortField==field)? boldFont : NULL,
			                                colour->hiColour) )
			{
				if( addTooltips )
					AddToTooltips(drawRect, str);
			}
		}

		// next
		currX += m_columnWidths[col];
	}

	// draw space aright
	if( currX < x+w )
	{
		dc.SetBrush(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].bgBrush);
		dc.DrawRectangle(currX, y, (x+w)-currX, h);
	}

	// draw a line?
	if( (tiSpecial&SJ_LISTVIEW_SPECIAL_LAST_GAP) && retLinePos )
	{
		long lineY = y + m_fontHeight/2;
		dc.SetPen(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].fgPen);
		dc.DrawLine(m_tracksRect.x, lineY, m_tracksRect.x+m_tracksRect.width, lineY);
		if( retLinePos )
			retLinePos->Add(lineY);
	}

	// remember some stuff
	lastTrackSelected = tiGap? false : tiSelected;
}


void SjListBrowser::DoPaintHeaderNTracks(wxDC& dc, long x, long y_, long w, long h, bool addTooltips,
        wxArrayLong* retLinePos, wxArrayLong* retAlbumIds)
{
	// draw header
	long currY = y_;
	DoPaintHeader(dc, x, currY, w, m_fontHeight, addTooltips);
	currY += m_fontHeight;

	// init lastAz
	int lastAz = 0;
	if( m_scrollPos > 0 && isAzPossibleForCol(m_sortField) )
	{
		SjTrackInfo  ti;
		long         tiAlbumId, tiSpecial;
		m_listView->GetTrack(m_scrollPos-1, ti, tiAlbumId, tiSpecial);

		wxString str = SjNormaliseString(ti.GetFormattedValue(m_sortField), 0);
		lastAz = str.Len()? str[0] : 'z'+1;
		if( lastAz < 'a' || lastAz > 'z' )
		{
			lastAz = 'z'+1;
		}
	}

	// draw all tracks
	long index = m_scrollPos;
	bool lastTrackSelected = false;
	long listViewTrackCount = m_listView->GetTrackCount();
	while( currY < m_window->m_clientH && index < listViewTrackCount )
	{
		DoPaintTrack(dc, x, currY, w, m_fontHeight, index,
		             lastTrackSelected /*used internally*/,
		             addTooltips, (index==m_scrollPos)/*set Az?*/,
		             &lastAz /*used internally*/,
		             retLinePos, retAlbumIds);

		// next
		index++;
		currY += m_fontHeight;
	}

	// draw space abottom
	if( currY < y_+h )
	{
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(SPACE_BRUSH);
		dc.DrawRectangle(x, currY, w, (y_+h)-currY);
	}
}


void SjListBrowser::DoPaint(wxDC& dc)
{
	ClearTooltips();

	// empty whole background (for testing only
	/*dc.SetBrush(*wxGREY_BRUSH);
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(0, 0, m_window->m_clientW, m_window->m_clientH); */

	// draw space atop
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(SPACE_BRUSH);
	if( m_flags&SJ_BROWSER_VIEW_COVER )
	{
		dc.DrawRectangle(0, 0, m_uptextRect.width, SPACE_TOP);
		dc.DrawRectangle(m_playmarkRect.x, 0, m_window->m_clientW, SPACE_TOP);
	}
	else
	{
		dc.DrawRectangle(0, 0, m_window->m_clientW, SPACE_TOP);
	}

	// draw toggle cover rect
	{
		dc.DrawRectangle(m_toggleCoversRect);
		dc.SetPen(g_mainFrame->m_workspaceColours[SJ_COLOUR_TITLE1].fgPen);

		wxRect iconRect = m_toggleCoversRect;
		iconRect.y+=2;
		iconRect.height-=4;
		iconRect.width = iconRect.height;

		iconRect.x += (m_toggleCoversRect.width-iconRect.width) / 2;
		SjTools::DrawIcon(dc, iconRect,
		                  (m_flags&SJ_BROWSER_VIEW_COVER)? SJ_DRAWICON_TRIANGLE_DOWN : SJ_DRAWICON_TRIANGLE_RIGHT);

		if( g_mainFrame->IsOpAvailable(SJ_OP_TOGGLE_ELEMENTS) )
			AddToTooltips(m_toggleCoversRect, _("Show covers"));
	}

	// draw space left of covers
	if( m_listView )
		m_window->DrawUpText(dc, m_listView->GetUpText(), m_uptextRect.x, m_uptextRect.y, m_uptextRect.width, m_uptextRect.height);
	else
		dc.DrawRectangle(m_uptextRect);

	// draw the workspace
	wxRect workspaceRect = m_headerRect;
	workspaceRect.height += m_tracksRect.height;

	dc.SetClippingRegion(workspaceRect);

	int w = workspaceRect.width;
	if( m_tracksVirtualW > workspaceRect.width )
		w = m_tracksVirtualW;

	wxArrayLong linePos;
	wxArrayLong albumIds;
	DoPaintHeaderNTracks(dc, workspaceRect.x-m_tracksHScroll, workspaceRect.y, w, workspaceRect.height,
	                     true /*add tooltips*/, &linePos, &albumIds);

	dc.DestroyClippingRegion();

	// draw space right of covers
	{
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(SPACE_BRUSH);
		dc.DrawRectangle(m_playmarkRect);
		dc.SetPen(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].fgPen);
		for( size_t i = 0; i < linePos.GetCount(); i++ )
			dc.DrawLine(m_playmarkRect.x, linePos[i], m_playmarkRect.x+m_playmarkRect.width, linePos[i]);
	}


	// draw the covers
	if( m_flags&SJ_BROWSER_VIEW_COVER )
	{
		wxRect updateRect = m_window->GetUpdateRegion().GetBox();
		if( updateRect.Intersects(m_coversRect) )
		{
			FreeAllocatedCover();

			// rewind the index to the previous cover (do this always as it may be partly visible)
			long index = m_scrollPos;
			long indexMinus = (m_tracksNeededToFitCoverHeight+1);
			if( index - indexMinus < 0 )
				indexMinus = index;
			index -= indexMinus;
			wxASSERT( index >= 0 );

			// init y
			long currY = m_tracksRect.y - indexMinus*m_fontHeight;
			long lastCoverBottom = 0;

			// draw all covers
			g_mainFrame->m_imgThread->RequireStart(m_window);

			long listViewTrackCount = m_listView->GetTrackCount();
			while( currY < m_window->m_clientH && index < listViewTrackCount )
			{
				// find out album Id
				long tiAlbumId = 0;
				if( index >= m_scrollPos
				        && (index-m_scrollPos) < (long)albumIds.GetCount() )
				{
					tiAlbumId = albumIds[index-m_scrollPos];
				}
				else if( m_listView->GetTrackSpecial(index)&SJ_LISTVIEW_SPECIAL_FIRST_TRACK )
				{
					SjTrackInfo  tiDummy;
					long         tiSpecial;
					m_listView->GetTrack(index, tiDummy, tiAlbumId, tiSpecial);
				}

				if( tiAlbumId )
				{
					// draw space above cover
					if( currY > 0 )
					{
						dc.SetBrush(SPACE_BRUSH);
						dc.SetPen(*wxTRANSPARENT_PEN);
						dc.DrawRectangle(m_coversRect.x, lastCoverBottom, m_coversRect.width, currY-lastCoverBottom);
					}

					// get the cover pointer
					SjCol* cover = m_listView->GetCol(tiAlbumId);
					if( cover )
					{
						if( cover->m_rowCount <= 0 )
						{
							delete cover;
							cover = NULL;
						}
						else
						{
							m_allocatedCover.Add(cover);
							cover->m_textlLeft = index; // a little hack - we use this custom var for the index of the view

							cover->m_rows[0]->m_top = currY;
						}
					}

					// draw cover
					SjImgThreadObj* obj = cover==NULL? NULL : m_window->RequireImage(cover->m_rows[0]->m_textm, m_coverW); // cachedImg may be NULL, but this is okay for PaintCover() and for ReleaseImage()
					m_window->PaintCover(dc, obj, m_coversRect.x, currY, m_coverW);
					g_mainFrame->m_imgThread->ReleaseImage(m_window, obj);

					// draw a line
					if( index )
					{
						dc.SetPen(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].fgPen);
						long y = (currY-m_fontHeight)+(m_fontHeight/2);
						dc.DrawLine(m_coversRect.x, y, m_coversRect.x+m_coversRect.width, y);
					}

					// prepare for the next cover
					lastCoverBottom = currY + m_coverW;
				}

				// next
				index++;
				currY += m_fontHeight;
			}

			g_mainFrame->m_imgThread->RequireEnd(m_window);

			// draw space below last cover
			if( lastCoverBottom < m_window->m_clientH )
			{
				dc.SetBrush(SPACE_BRUSH);
				dc.SetPen(*wxTRANSPARENT_PEN);
				dc.DrawRectangle(m_coversRect.x, lastCoverBottom, m_coversRect.width, m_window->m_clientH);

				dc.SetPen(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMAL].fgPen);
				for( size_t i = 0; i < linePos.GetCount(); i++ )
				{
					if( linePos[i] >= lastCoverBottom )
						dc.DrawLine(m_coversRect.x, linePos[i], m_coversRect.x+m_coversRect.width, linePos[i]);
				}
			}
		}
	}
}


wxBitmap SjListBrowser::GetDragNDropBitmap(int mouseX, int mouseY, wxPoint& retHotspot, wxRect& retSize)
{
	// calculate the width and the height of the bitmap
	long firstSelected = GetFirstSelectedAndVisible();
	long lastSelected = GetLastSelectedAndVisible(false);
	if( firstSelected == -1 || lastSelected == -1 )
		return wxNullBitmap;

	long dndWidth = m_tracksVirtualW;
	long dndHeight = m_fontHeight * ((lastSelected-firstSelected)+1);

	retHotspot.x  = mouseX - m_tracksRect.x + m_tracksHScroll;
	retHotspot.y  = mouseY - m_tracksRect.y - ((firstSelected-m_scrollPos)*m_fontHeight);

	// create an offscreen DC with the given Size
	wxBitmap            imgBitmap(dndWidth, dndHeight);
	wxMemoryDC          imgDc;
	imgDc.SelectObject(imgBitmap);
	wxBitmap            maskBitmap(dndWidth, dndHeight, 1);
	wxMemoryDC          maskDc;
	maskDc.SelectObject(maskBitmap);

	// draw the selected rows to the offscreen dc
	imgDc.SetPen(*wxTRANSPARENT_PEN);
	imgDc.SetBrush(g_mainFrame->m_workspaceColours[SJ_COLOUR_NORMALODD].bgBrush);
	imgDc.DrawRectangle(0, 0, dndWidth, dndHeight);

	maskDc.SetPen(*wxTRANSPARENT_PEN);
	maskDc.SetBrush(*wxBLACK_BRUSH);
	maskDc.DrawRectangle(0, 0, dndWidth, dndHeight);
	maskDc.SetBrush(*wxWHITE_BRUSH);

	long y = 0;
	bool lastTrackSelected = false;
	for( long index = firstSelected; index <= lastSelected; index++ )
	{
		if( m_listView->IsTrackSelected(index) )
		{
			DoPaintTrack(imgDc, 0, y, dndWidth, m_fontHeight, index,
			             lastTrackSelected,
			             false /*addTooltips*/, false /*setAz*/,
			             NULL);

			maskDc.DrawRectangle(0, y, dndWidth, m_fontHeight);
		}
		y += m_fontHeight;
	}

	// done creating the images
	imgDc.SelectObject(wxNullBitmap);
	maskDc.SelectObject(wxNullBitmap);

	// scale the bitmaps down if they are too large
	#define MAX_W (m_window->m_clientW)
	#define MAX_H (m_window->m_clientH)
	#define MAX_PIXELS (250*250)
	if( (dndWidth*dndHeight) > MAX_PIXELS
     ||  dndWidth            > MAX_W
     ||  dndHeight           > MAX_H )
	{
		// calculate the new width and height
		long oldWidth = dndWidth, oldHeight = dndHeight;

		if( dndWidth > MAX_W )
		{
			dndHeight = (dndHeight*MAX_W) / dndWidth;
			dndWidth  =  MAX_W;
		}

		if( dndHeight>MAX_H )
		{
			dndWidth  = (dndWidth*MAX_H) / dndHeight;
			dndHeight =  MAX_H;
		}

		if( (dndWidth*dndHeight) > MAX_PIXELS  )
		{
			float div = (float)(dndWidth*dndHeight) / (float)MAX_PIXELS;

			dndHeight = (long) ((float)dndHeight/div);
			dndWidth  = (long) ((float)dndWidth/div);
			wxASSERT( (dndWidth*dndHeight) <= MAX_PIXELS );
		}

		// scale down image
		wxImage img = imgBitmap.ConvertToImage();
		//img.Rescale(dndWidth, dndHeight);
		SjImgOp::DoResize(img, dndWidth, dndHeight, SJ_IMGOP_SMOOTH);
		imgBitmap = wxBitmap(img);

		// scale down mask
		img = maskBitmap.ConvertToImage();
		img.Rescale(dndWidth, dndHeight);
		maskBitmap = wxBitmap(img, 1);

		// correct hotspot
		retHotspot.x = (retHotspot.x*dndWidth) / oldWidth;
		retHotspot.y = (retHotspot.y*dndHeight) / oldHeight;
	}

	// done
	imgBitmap.SetMask(new wxMask(maskBitmap));

	retSize.x     = 0;
	retSize.y     = 0;
	retSize.width = dndWidth;
	retSize.height= dndHeight;

	return imgBitmap;
}
