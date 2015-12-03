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
 * File:    columnmixer.cpp
 * Authors: Björn Petersen
 * Purpose: The column mixer
 *
 ******************************************************************************/


#include <sjbase/base.h>


/*******************************************************************************
 * Constructor / Destructor
 ******************************************************************************/


SjColumnMixer::SjColumnMixer()
{
	m_moduleCount = 0;
	m_libraryModule = NULL;
	m_selAnchorColIndex = -1;
	m_maskedColCount = 0;
	m_unmaskedColCount = 0;
}


SjColumnMixer::~SjColumnMixer()
{
}


void SjColumnMixer::LoadModules(SjModuleSystem* moduleSystem)
{
	wxASSERT(m_moduleCount==0);
	wxASSERT(m_libraryModule==NULL);

	SjModuleList*       moduleList = moduleSystem->GetModules(SJ_MODULETYPE_COL);
	SjModuleList::Node* moduleNode = moduleList->GetFirst();
	while( moduleNode )
	{
		SjColModule* module = (SjColModule*)moduleNode->GetData();
		wxASSERT(module);

		if( module->Load() )
		{
			if( module->m_file == wxT("memory:library.lib") )
			{
				m_libraryModule = module;
			}

			m_modules[m_moduleCount] = module;
			m_moduleMaskedColCount[m_moduleCount] = -1;
			m_moduleUnmaskedColCount[m_moduleCount] = -1;
			m_moduleCount++;

			if( m_moduleCount>=SJ_COLUNMMIXER_MAX_MODULES )
			{
				wxLogError(wxT("Too many column modules."));
				break; // out of while
			}
		}

		moduleNode = moduleNode->GetNext();
	}

	SetColCount__();
}


void SjColumnMixer::UnloadModules()
{
	int m;
	for( m = 0; m < m_moduleCount; m++ )
	{
		wxASSERT(m_modules[m]);

		m_modules[m]->Unload();
	}

	m_moduleCount = 0;
	m_libraryModule = NULL;

	SetColCount__();
}



void SjColumnMixer::ReloadColumns()
{
	SetColCount__();
}


/*******************************************************************************
 * Search Handling
 ******************************************************************************/


long SjColumnMixer::DelInsSelection(bool delPressed)
{
	long numDeletedTracks = 0;
	int  m;

	// remember the first selection position on delete
	// (the new item at the same position will be restored after deletion)
	long oldColIndex = 0, oldRowIndex = 0;
	wxString oldUrl;
	if( delPressed )
	{
		GetSelectionAnchor(oldColIndex, oldRowIndex);
		SjCol* col = GetMaskedCol(oldColIndex);
		if( col )
		{
			oldUrl = col->m_url;
			delete col;
		}
	}

	// delete the selection
	for( m = 0; m < m_moduleCount; m++ )
	{
		numDeletedTracks += m_modules[m]->DelInsSelection(delPressed);
	}

	if( numDeletedTracks )
	{
		// recalculate internals
		SetColCount__();

		// set a new selection
		if( delPressed )
		{
			int i;
			if( GetMaskedColIndexByColUrl(oldUrl) == -1 )
			{
				// complete column deleted
				if( oldColIndex >= GetMaskedColCount() ) oldColIndex = GetMaskedColCount()-1;
				for( i = 0; i<=4; i++ )
				{
					if( SetSelection(oldColIndex, i) ) // find selectable row from top of column
						break;
				}
			}
			else
			{
				// row deleted
				for( i=0; i<=2; i++ )
				{
					if( SetSelection(oldColIndex, oldRowIndex-i) ) // find selectable row from old pos. upwards
						break;
				}
			}
		}
	}

	return numDeletedTracks;
}


SjSearchStat SjColumnMixer::SetSearch(const SjSearch& search, bool deepSearch)
{
	// we're assuming, the search has really changed -- the caller
	// (currently only SjMainFrame::PerformSearch() should check this

	SjSearchStat retStat;

	int m;
	for( m = 0; m < m_moduleCount; m++ )
	{
		wxASSERT(m_modules[m]);

		retStat += m_modules[m]->SetSearch(search, deepSearch);
	}

	SetColCount__();

	return retStat;
}


void SjColumnMixer::SetColCount__()
{
	m_maskedColCount = 0;
	m_unmaskedColCount = 0;
	m_selAnchorColIndex = -1;
	int m;
	for( m = 0; m < m_moduleCount; m++ )
	{
		wxASSERT(m_modules[m]);

		m_moduleMaskedColCount[m] = m_modules[m]->GetMaskedColCount();
		m_maskedColCount += m_moduleMaskedColCount[m];

		m_moduleUnmaskedColCount[m] = m_modules[m]->GetUnmaskedColCount();
		m_unmaskedColCount += m_moduleUnmaskedColCount[m];
	}
}


long SjColumnMixer::GetMaskedColIndexByAz(int targetId)
{
	int m;
	long index = 0;
	for( m = 0; m < m_moduleCount; m++ )
	{
		wxASSERT(m_modules[m]);

		if( m_modules[m] == m_libraryModule )
		{
			return index + m_modules[m]->GetMaskedColIndexByAz(targetId);
		}

		index += m_moduleMaskedColCount[m];
	}

	return 0;
}


wxString SjColumnMixer::GetAzDescr(int targetIdOrChar)
{
	int azChar = targetIdOrChar;
	if( azChar >= IDT_WORKSPACE_GOTO_A && azChar <= IDT_WORKSPACE_GOTO_0_9 )
	{
		azChar = 'a' + (targetIdOrChar-IDT_WORKSPACE_GOTO_A);
	}

	if( azChar == ('z'+1) /*character behind 'z' is normally '{'*/ )
	{
		return wxT("0-9");
	}
	else
	{
		return wxString::Format(wxT("%c"), (wxChar)azChar).Upper();
	}
}


long SjColumnMixer::GetMaskedColIndexByColUrl(const wxString& colUrl)
{
	int m;
	long index = 0, modIndex;
	for( m = 0; m < m_moduleCount; m++ )
	{
		wxASSERT(m_modules[m]);
		modIndex = m_modules[m]->GetMaskedColIndexByColUrl(colUrl);
		if( modIndex >= 0 )
		{
			return index + modIndex;
		}

		index += m_moduleMaskedColCount[m];
	}

	return -1; // not found
}


bool SjColumnMixer::GetMaskedColIndexByRow(SjRow* row, long& colIndex, long& rowIndex)
{
	SjCol* col = GetMaskedCol(row->m_url, colIndex);
	if( col )
	{
		for( rowIndex = 0; rowIndex < col->m_rowCount; rowIndex++ )
		{
			if( col->m_rows[rowIndex]->m_url == row->m_url )
			{
				delete col;
				return TRUE; // success
			}
		}

		delete col;
	}

	return FALSE; // error
}


SjCol* SjColumnMixer::GetMaskedCol(long index)
{
	SjCol* ret;
	int m;

	if( index >= 0 && index < m_maskedColCount )
	{
		for( m = 0; m < m_moduleCount; m++ )
		{
			wxASSERT(m_modules[m]);

			if( index < m_moduleMaskedColCount[m] )
			{
				ret = m_modules[m]->GetMaskedCol(index);
				if( ret )
				{
					wxASSERT(ret->m_module == m_modules[m]);
					return ret;
				}
				else
				{
					return NULL;
				}
			}

			index -= m_moduleMaskedColCount[m];
		}
	}

	return NULL;
}


SjCol* SjColumnMixer::GetUnmaskedCol(long index)
{
	SjCol* ret;
	int m;

	if( index >= 0 && index < m_unmaskedColCount )
	{
		for( m = 0; m < m_moduleCount; m++ )
		{
			wxASSERT(m_modules[m]);

			if( index < m_moduleUnmaskedColCount[m] )
			{
				ret = m_modules[m]->GetUnmaskedCol(index);
				if( ret )
				{
					wxASSERT(ret->m_module == m_modules[m]);
					return ret;
				}
				else
				{
					return NULL;
				}
			}

			index -= m_moduleUnmaskedColCount[m];
		}
	}

	return NULL;
}


SjCol* SjColumnMixer::GetMaskedCol(const wxString& trackUrl, long& retIndex)
{
	SjCol* col;
	int m;
	long otherIndex = 0;

	for( m = 0; m < m_moduleCount; m++ )
	{
		wxASSERT(m_modules[m]);
		col = m_modules[m]->GetMaskedCol(trackUrl, retIndex);
		if( col )
		{
			if( retIndex != -1 ) retIndex += otherIndex;
			return col;
		}
		otherIndex += m_moduleMaskedColCount[m];
	}

	return NULL;
}


/*******************************************************************************
 * Selection Handling
 ******************************************************************************/


bool SjColumnMixer::IsAnythingSelected()
{
	int m;

	for( m = 0; m < m_moduleCount; m++ )
	{
		wxASSERT(m_modules[m]);

		if( m_modules[m]->GetSelectedUrlCount() )
		{
			return TRUE;
		}
	}

	return FALSE;
}


long SjColumnMixer::GetSelectedUrlCount()
{
	int  m;
	long ret = 0;

	for( m = 0; m < m_moduleCount; m++ )
	{
		wxASSERT(m_modules[m]);

		ret += m_modules[m]->GetSelectedUrlCount();
	}

	return ret;
}


void SjColumnMixer::SelectAll(bool select, bool keepAnchor)
{
	int m;

	if( !keepAnchor )
	{
		m_selAnchorColIndex = -1;
	}

	for( m = 0; m < m_moduleCount; m++ )
	{
		wxASSERT(m_modules[m]);

		m_modules[m]->SelectAllRows(select);
	}
}


void SjColumnMixer::GetSelectedUrls(wxArrayString& urls, SjSelectedUrlsScope scope)
{
	int m;

	switch( scope )
	{
		case SJ_SEL_URLS_WORKSPACE:
			// use the selected workspace URLs
			for( m = 0; m < m_moduleCount; m++ )
				m_modules[m]->GetSelectedUrls(urls);
			break;

		case SJ_SEL_URLS_DISPLAY:
			// use the selected display URLS
			urls = g_mainFrame->m_player.m_queue.GetUrlsByIds(g_mainFrame->m_display.m_selectedIds);
			break;

		case SJ_SEL_URLS_SMART:
			// return the (a) selected display URLs, (b) selected workspace URLs or (c) _all_ display URLs
			if( g_mainFrame->m_display.m_selectedIds.GetCount() )
			{
				urls = g_mainFrame->m_player.m_queue.GetUrlsByIds(g_mainFrame->m_display.m_selectedIds);
				if( urls.GetCount() )
					return;
			}

			for( m = 0; m < m_moduleCount; m++ )
				m_modules[m]->GetSelectedUrls(urls);

			if( urls.GetCount() == 0 )
				urls = g_mainFrame->m_player.m_queue.GetUrls();
			break;
	}
}


bool SjColumnMixer::GetSelectionRange(long& firstSelColIndex, long& firstSelRowIndex,
                                      long* lastSelColIndex,  long* lastSelRowIndex)
{
	SjCol*          currCol;
	long            s, c;
	wxArrayString   selUrls;
	long            selUrlsCount;

	bool            busyCursorSet = FALSE;
	unsigned long   startBusyTime = SjTools::GetMsTicks() + 80;

	// Get all selected URLs.
	GetSelectedUrls(selUrls);
	selUrlsCount = selUrls.GetCount();

	// Find out the first/last selected column and row index.
	// The calculation of the last selection is optional and may be skipped for speed reasons.
	firstSelColIndex = -1;
	if( lastSelColIndex ) { *lastSelColIndex  = -1; }

	for( s = 0; s < selUrlsCount; s++ )
	{
		currCol = GetMaskedCol(selUrls[s], c);
		if( currCol )
		{
			if( c < firstSelColIndex || firstSelColIndex == -1 )
			{
				firstSelColIndex = c;
				for( firstSelRowIndex = 0; firstSelRowIndex < currCol->m_rowCount; firstSelRowIndex++ )
				{
					if( currCol->m_rows[firstSelRowIndex]->IsSelected() )
					{
						break;
					}
				}

				if( firstSelRowIndex >= currCol->m_rowCount )
				{
					firstSelColIndex = -1; // error -> start over
				}
			}

			if(  lastSelColIndex
			        && (c > *lastSelColIndex || *lastSelColIndex == -1) )
			{
				*lastSelColIndex = c;
				for( *lastSelRowIndex = currCol->m_rowCount-1; *lastSelRowIndex >= 0; (*lastSelRowIndex)-- )
				{
					if( currCol->m_rows[*lastSelRowIndex]->IsSelected() )
					{
						break;
					}
				}

				if( *lastSelRowIndex < 0 )
				{
					*lastSelColIndex = -1; // error -> start over
				}
			}

			delete currCol;
		}

		// Check busy cursor for longer operations.
		if( !busyCursorSet && SjTools::GetMsTicks() > startBusyTime )
		{
			::wxBeginBusyCursor();
			busyCursorSet = TRUE;
		}
	}

	// Done.
	if( busyCursorSet )
	{
		::wxEndBusyCursor();
	}

	return (firstSelColIndex!=-1); // if a first selection is found, there is always a last selection
}


long SjColumnMixer::SetSelectionRange(long firstSelColIndex, long firstSelRowIndex,
                                      long lastSelColIndex,  long lastSelRowIndex,
                                      bool keepAnchor )
{
	long c, r;
	SjCol* currCol;

	bool            busyCursorSet = FALSE;
	unsigned long   startBusyTime = SjTools::GetMsTicks() + 80;
	long            selectionCount = 0;

	// deselect all rows
	SelectAll(FALSE, keepAnchor);

	// select given rows
	for( c = firstSelColIndex; c <= lastSelColIndex; c++ )
	{
		currCol = GetMaskedCol(c);
		if( currCol )
		{
			for( r = 0; r < currCol->m_rowCount; r++ )
			{
				if( (firstSelColIndex!=lastSelColIndex && ((c==firstSelColIndex && r>=firstSelRowIndex) || (c> firstSelColIndex && c<lastSelColIndex) || (c==lastSelColIndex  && r<=lastSelRowIndex)))
				        || (r>=firstSelRowIndex && r<=lastSelRowIndex) )
				{
					if( currCol->m_rows[r]->IsSelectable()==2 )
					{
						currCol->m_rows[r]->Select(TRUE);
						selectionCount++;
					}
				}
			}

			delete currCol;
		}

		// check busy cursor for longer operations
		if( !busyCursorSet && SjTools::GetMsTicks() > startBusyTime )
		{
			::wxBeginBusyCursor();
			busyCursorSet = TRUE;
		}
	}

	// Done.
	if( busyCursorSet )
	{
		::wxEndBusyCursor();
	}

	return selectionCount;
}


bool SjColumnMixer::GetSelectionAnchor(long& colIndex__, long& rowIndex__)
{
	SjCol* currCol;

	// verify stored anchor
	if( m_selAnchorColIndex != -1 )
	{
		currCol = GetMaskedCol(m_selAnchorColIndex);
		if( currCol == NULL )
		{
			m_selAnchorColIndex = -1; // recalculate anchor - this is no error!
		}
		else
		{
			if(  m_selAnchorRowIndex<0 || m_selAnchorRowIndex>=currCol->m_rowCount
			        || !currCol->m_rows[m_selAnchorRowIndex]->IsSelected() )
			{
				m_selAnchorColIndex = -1; // recalculate anchor - this is no error!
			}
			delete currCol;
		}
	}

	// recalculate anchor if needed
	if( m_selAnchorColIndex == -1 || GetSelectedUrlCount() == 1 )
	{
		GetSelectionRange(m_selAnchorColIndex, m_selAnchorRowIndex);
	}

	// done
	colIndex__ = m_selAnchorColIndex;
	rowIndex__ = m_selAnchorRowIndex;

	return m_selAnchorColIndex!=-1;
}


void SjColumnMixer::SelectShifted(SjRow* rowClicked)
{
	// Function selects all rows from the "currently selected" row
	// up to the given row. All other rows are deselected.

	// The "currently selected" row is the first selected row or a stored anchor.

	wxASSERT( rowClicked );
	wxASSERT( rowClicked->IsSelectable()==2 );

	long rowClickedColIndex, rowClickedRowIndex;

	// If there is no selected row/no anchor, we cannot shift to the given row;
	// simply select the given row in this case.
	if( !GetSelectionAnchor(rowClickedColIndex/*dummy*/, rowClickedRowIndex/*dummy*/) )
	{
		rowClicked->Select(TRUE);
		return; // Done.
	}

	// find out "shift to" column and row index
	if( GetMaskedColIndexByRow(rowClicked, rowClickedColIndex, rowClickedRowIndex) )
	{
		// okay, now we have the first and the last selected row index
		// and the index of the row to shift to -- see what to do
		if(  rowClickedColIndex > m_selAnchorColIndex
		        || (rowClickedColIndex == m_selAnchorColIndex && rowClickedRowIndex > m_selAnchorRowIndex) )
		{
			// select from the first selected row up to the shifted row
			SetSelectionRange(m_selAnchorColIndex, m_selAnchorRowIndex, rowClickedColIndex, rowClickedRowIndex, TRUE);
		}
		else
		{
			// select from the shifted row up to the first selected row;
			SetSelectionRange(rowClickedColIndex, rowClickedRowIndex, m_selAnchorColIndex, m_selAnchorRowIndex, TRUE);
		}
	}
}


bool SjColumnMixer::SelectShifted(int keyPressed/*-1=up, +1=down*/, long& retScopeColIndex, long& retScopeRowIndex)
{
	long    firstSelColIndex, firstSelRowIndex,
	        lastSelColIndex, lastSelRowIndex;
	bool    prevNextOk;

	// get selection anchor and the first/last selected row

	if( !GetSelectionAnchor(firstSelColIndex/*dummy*/, firstSelRowIndex/*dummy*/)
	        || !GetSelectionRange(firstSelColIndex, firstSelRowIndex, &lastSelColIndex, &lastSelRowIndex) )
	{
		return FALSE; // no anchor, the caller should preform a "simple" selection
	}

	// calculate the new selection...

	if( firstSelColIndex == lastSelColIndex && firstSelRowIndex == lastSelRowIndex )
	{
		if( keyPressed == -1 )
		{
			// ...initial selection from anchor to top
			prevNextOk = PrevNextRow(firstSelColIndex, firstSelRowIndex, -1);
			retScopeColIndex = firstSelColIndex;
			retScopeRowIndex = firstSelRowIndex;
		}
		else
		{
			// ...initial selection from anchor to bottom
			prevNextOk = PrevNextRow(lastSelColIndex, lastSelRowIndex, +1);
			retScopeColIndex = lastSelColIndex;
			retScopeRowIndex = lastSelRowIndex;
		}
	}
	else if(  firstSelColIndex < m_selAnchorColIndex
	          || (firstSelColIndex == m_selAnchorColIndex && firstSelRowIndex < m_selAnchorRowIndex) )
	{
		// ...selected stuff above the anchor...
		prevNextOk = PrevNextRow(firstSelColIndex, firstSelRowIndex, keyPressed);
		retScopeColIndex = firstSelColIndex;
		retScopeRowIndex = firstSelRowIndex;
	}
	else
	{
		// ...select stuff below the anchor...
		prevNextOk = PrevNextRow(lastSelColIndex, lastSelRowIndex, keyPressed);
		retScopeColIndex = lastSelColIndex;
		retScopeRowIndex = lastSelRowIndex;
	}

	if( prevNextOk )
	{
		// set the new selection

		SetSelectionRange(firstSelColIndex, firstSelRowIndex, lastSelColIndex, lastSelRowIndex, TRUE);

		// make sure, the anchor is always selected

		SjCol* col = GetMaskedCol(m_selAnchorColIndex);
		if( col )
		{
			if( m_selAnchorRowIndex >= 0 && m_selAnchorRowIndex < col->m_rowCount )
			{
				col->m_rows[m_selAnchorRowIndex]->Select(TRUE);
			}
			delete col;
		}
	}

	return TRUE;
}


bool SjColumnMixer::PrevNextRow(long& colIndex, long& rowIndex, int dir)
{
	long orgColIndex = colIndex,
	     orgRowIndex = rowIndex;

	wxASSERT( dir==+1 || dir==-1 );

	SjCol*  col;

	while( 1 )
	{
		col = GetMaskedCol(colIndex);
		if( col == NULL ) break;
		if( rowIndex == -2 /* (X) see below */ ) rowIndex = col->m_rowCount;

		if( dir == +1 )
		{
			rowIndex++;
			for( ; rowIndex < col->m_rowCount; rowIndex++ )
			{
				if( col->m_rows[rowIndex]->IsSelectable() == 2 )
				{
					delete col;
					return TRUE; // success
				}
			}

			colIndex++;
			rowIndex = -1;
		}
		else
		{
			rowIndex--;
			for( ; rowIndex >= 0; rowIndex-- )
			{
				if( col->m_rows[rowIndex]->IsSelectable() == 2 )
				{
					delete col;
					return TRUE; // success
				}
			}

			colIndex--;
			rowIndex = -2; // corrected above (X)
		}

		delete col;
	}

	// no subsequent row found, return given row
	colIndex = orgColIndex;
	rowIndex = orgRowIndex;
	return FALSE;
}


/*******************************************************************************
 * Get Column/Row Information
 ******************************************************************************/


bool SjColumnMixer::GetQuickInfo(const wxString& url,
                                 wxString& trackName, wxString& leadArtistName, wxString& albumName,
                                 long& playtimeMs)
{
	int  m;
	bool ok = FALSE;

	m_tempInfo.m_trackName.Empty();
	m_tempInfo.m_leadArtistName.Empty();
	m_tempInfo.m_playtimeMs = 0;

	// first, try the "A-Z" module (normally the largest module)
	if( m_libraryModule )
	{
		ok = m_libraryModule->GetTrackInfo(url, m_tempInfo, SJ_TI_QUICKINFO, FALSE/*no log*/);
	}

	// then, try the other modules
	if( !ok )
	{
		for( m = 0; m < m_moduleCount; m++ )
		{
			wxASSERT(m_modules[m]);
			if( m_modules[m] != m_libraryModule )
			{
				ok = m_modules[m]->GetTrackInfo(url, m_tempInfo, SJ_TI_QUICKINFO, FALSE/*no log*/);
				if( ok )
				{
					break;
				}
			}
		}
	}

	// done so far
	if( ok )
	{
		trackName = m_tempInfo.m_trackName;
		leadArtistName = m_tempInfo.m_leadArtistName;
		albumName = m_tempInfo.m_albumName;
		playtimeMs = m_tempInfo.m_playtimeMs;
		return TRUE;
	}
	else
	{
		trackName = url;
		leadArtistName.Empty();
		playtimeMs = 0;
		return FALSE;
	}
}


wxString SjColumnMixer::GetTrackCoverUrl(const wxString& trackUrl)
{
	int  m;
	bool ok = FALSE;

	// amy track given?
	if( trackUrl.IsEmpty() )
		return wxEmptyString;

	// first, try the "A-Z" module (normally the largest module)
	if( m_libraryModule )
	{
		m_tempInfo.m_arts.Empty();
		ok = m_libraryModule->GetTrackInfo(trackUrl, m_tempInfo, SJ_TI_TRACKCOVERURL, FALSE/*no log*/);
	}

	// then, try the other modules
	if( !ok )
	{
		for( m = 0; m < m_moduleCount; m++ )
		{
			wxASSERT(m_modules[m]);
			if( m_modules[m] != m_libraryModule )
			{
				m_tempInfo.m_arts.Empty();
				ok = m_modules[m]->GetTrackInfo(trackUrl, m_tempInfo, SJ_TI_TRACKCOVERURL, FALSE/*no log*/);
				if( ok )
				{
					break;
				}
			}
		}
	}

	// anything found?
	if( !ok )
		return wxEmptyString;

	// simply return the first cover
	return m_tempInfo.m_arts.BeforeFirst(wxT('\n'));
}

