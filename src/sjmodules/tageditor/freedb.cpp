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
 * File:    freedb.cpp
 * Authors: Björn Petersen
 * Purpose: Freedb access
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/tageditor/freedb.h>

#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArrayFreedbTrack);
WX_DEFINE_OBJARRAY(SjArrayFreedbPossibleResult);


// the used protocol defines the encoding that can be used
#if wxUSE_UNICODE
#define PROTOCOL_LEVEL wxT("&proto=6")
#define PROTOCOL_ENC   &wxConvUTF8
#else
#define PROTOCOL_LEVEL wxT("&proto=5")
#define PROTOCOL_ENC   &wxConvISO8859_1
#endif


/*******************************************************************************
 * Constructor
 ******************************************************************************/


SjFreedbQuery::SjFreedbQuery()
{
	m_state = SJ_FREEDB_STATE_INITIALIZED;
	m_resultTracks = &m_tracks1__;
}


/*******************************************************************************
 * Query
 ******************************************************************************/


bool SjFreedbQuery::LoadTracks(const wxArrayString& localUrls, SjArrayFreedbTrack& retTracks,
                               long& incompleteAlbumId)
{
	SjTrackInfo     ti;
	SjFreedbTrack*  newTrack;
	int             i;
	wxSqlt          sql;
	long            thisAlbumId, firstAlbumId = 0;
	bool            uniqueAlbumId = TRUE;
	int             trackCount = localUrls.GetCount();

	incompleteAlbumId = 0;
	for( i = 0; i < trackCount; i++ )
	{
		ti.Clear();
		if( !g_mainFrame->m_libraryModule->GetTrackInfo(localUrls[i], ti, SJ_TI_QUICKINFO, FALSE) )
		{
			m_error = wxString::Format(_("Cannot open \"%s\"."), localUrls[i].c_str());
			m_state = SJ_FREEDB_STATE_QUERY_ERROR;
			retTracks.Clear();
			return FALSE;
		}

		newTrack = new SjFreedbTrack;
		newTrack->m_localUrl = localUrls[i];
		newTrack->m_localMs = ti.m_playtimeMs;
		retTracks.Add(newTrack);

		if( uniqueAlbumId )
		{
			sql.Query(wxT("SELECT albumid FROM tracks WHERE url='") + sql.QParam(localUrls[i]) + wxT("';"));
			if( sql.Next() )
			{
				thisAlbumId = sql.GetLong(0);
				if( firstAlbumId == 0 ) firstAlbumId = thisAlbumId;
				if( firstAlbumId != thisAlbumId ) uniqueAlbumId = FALSE;
			}
		}
	}

	if( uniqueAlbumId )
	{
		sql.Query(wxString::Format(wxT("SELECT COUNT(*) FROM tracks WHERE albumid=%i;"), (int)firstAlbumId));
		if( sql.Next() )
		{
			if( sql.GetLong(0) != trackCount )
			{
				incompleteAlbumId = firstAlbumId;
			}
		}
	}

	return TRUE;
}


bool SjFreedbQuery::Query(const wxArrayString& localUrls, wxWindow* rcvWindow, int rcvCommandId)
{
	// init and clean previous result (if any)
	m_state = SJ_FREEDB_STATE_INITIALIZED;
	m_http.Init(rcvWindow, rcvCommandId);

	m_possibleResults.Clear();
	m_tracks1__.Clear();
	m_tracks2__.Clear();

	// track count in range?
	int trackCount = (int)localUrls.GetCount();
	if( !m_error.IsEmpty() )
	{
		m_state = SJ_FREEDB_STATE_QUERY_ERROR;
		return FALSE;
	}
	else if( trackCount <= 0 )
	{
		m_error = wxT("No tracks given to query freedb for.")/*n/t - should not happen*/;
		m_state = SJ_FREEDB_STATE_QUERY_ERROR;
		return FALSE;
	}
	else if( trackCount > 99 )
	{
		m_error = _("No matches");
		m_state = SJ_FREEDB_STATE_QUERY_ERROR;
		return FALSE;
	}

	// get the URLs #2 which is the exact selection of the user
	long incompleteAlbumId;
	if( !LoadTracks(localUrls, m_tracks2__, incompleteAlbumId) )
	{
		m_state = SJ_FREEDB_STATE_QUERY_ERROR;
		return FALSE;
	}

	if( incompleteAlbumId )
	{
		wxSqlt sql;
		wxArrayString completeAlbumUrls;
		sql.Query(wxString::Format(wxT("SELECT url FROM tracks WHERE albumid=%i ORDER BY disknr,tracknr,url;"), (int)incompleteAlbumId));
		while( sql.Next() )
		{
			completeAlbumUrls.Add(sql.GetString(0));
		}

		LoadTracks(completeAlbumUrls, m_tracks1__, incompleteAlbumId);
		QueryDo(m_tracks1__, SJ_FREEDB_STATE_WAIT_FOR_QUERY1);
	}
	else
	{
		QueryDo(m_tracks2__, SJ_FREEDB_STATE_WAIT_FOR_QUERY2);
	}

	return TRUE;
}


void SjFreedbQuery::QueryDo(SjArrayFreedbTrack& tracks, SjFreedbState newState)
{
	int i, trackCount = tracks.GetCount();

	// get the discid and build the query string for freedb http access
	// For backward compatibility this algorithm must not change
	unsigned long   discid = 0;
	wxString        offsets;
	int             totalTime;

	{
		int n = 0;
		int currSeconds = 2;
		for( i = 0; i < trackCount; i++ )
		{
			n = n + CddbSum(currSeconds);

			// add track to offsets
			if( !offsets.IsEmpty() ) offsets += wxT(" ");
			offsets += wxString::Format(wxT("%i"), (int)(currSeconds*75));

			// next track
			currSeconds += tracks[i].m_localMs/1000 + 2;
		}

		totalTime = currSeconds-2;

		discid = ((n % 0xff) << 24 | totalTime << 8 | trackCount);
	}

	// query the server
	m_httpCmd = wxString::Format(wxT("cddb query %08x %i %s %i"),
	                             (int)discid,
	                             (int)trackCount,
	                             offsets.c_str(),
	                             (int)totalTime);
	m_state = newState;
	m_http.OpenFile(BuildQueryUrl(m_httpCmd), PROTOCOL_ENC);
}


bool SjFreedbQuery::QueryDoContinued(SjArrayFreedbTrack& tracks, SjFreedbState newState)
{
	wxString result = m_http.GetString();
	if( result.IsEmpty() )
	{
		wxString cmd(m_httpCmd);
		if( cmd.Len() > 64 ) cmd.Truncate(60)+wxT("...");
		m_error = wxString::Format(_("Cannot open \"%s\"."), cmd.c_str());
		m_state = SJ_FREEDB_STATE_QUERY_ERROR;
		return FALSE;
	}

	wxLogDebug(result);

	// go through the result
	const wxChar*           currLinePtr;
	wxString                currLine;
	bool                    firstLine = TRUE;
	SjLineTokenizer         tkz(result);
	while( (currLinePtr=tkz.GetNextLine()) )
	{
		if( currLinePtr[0] )
		{
			currLine = currLinePtr;
			if( firstLine )
			{
				wxString codeStr = currLine.BeforeFirst(' ');
				long codeLong = 0;
				if( !codeStr.ToLong(&codeLong, 10) ) { codeLong = 0; }
				if( codeLong == 202 )
				{
					m_error = _("No matches");
					m_state = SJ_FREEDB_STATE_QUERY_ERROR;
					return FALSE;
				}
				else if( codeLong != 200 && codeLong != 210 && codeLong != 211 )
				{
					m_error = wxString::Format(_("Error %i"), (int)codeLong) + wxT(":") + currLine;
					m_state = SJ_FREEDB_STATE_QUERY_ERROR;
					return FALSE;
				}
				firstLine = FALSE;
			}
			else if( currLine == wxT(".") )
			{
				break; // done
			}
			else
			{
				SjFreedbPossibleResult* possibleResult = new SjFreedbPossibleResult(&tracks);

				possibleResult->m_genre = currLine.BeforeFirst(' ');
				currLine = currLine.AfterFirst(' ');

				possibleResult->m_discId = currLine.BeforeFirst(' ');

				possibleResult->m_discName = currLine.AfterFirst(' ');

				m_possibleResults.Add(possibleResult);
			}
		}
	}

	if( m_possibleResults.GetCount() <= 0 )
	{
		m_error = _("No matches");
		m_state = SJ_FREEDB_STATE_QUERY_ERROR;
		return FALSE;
	}

	// mark some results as "secondary" ...
	{
		// ...remove entries with too many question marks as "??? / ????"
		int i, j, iCount = m_possibleResults.GetCount();
		int questionMarkedCount = 0;
		SjFreedbPossibleResult *iResult, *jResult;
		for( i = 0; i < iCount; i++ )
		{
			iResult = &(m_possibleResults[i]);
			if( HasTooManyQuestionMarks(iResult->m_discName) )
			{
				iResult->m_isSecondary = TRUE;
				questionMarkedCount++;
			}
		}

		// ...no primary left? remove all secondary marks
		if( questionMarkedCount == iCount )
		{
			for( i = 0; i < iCount; i++ )
			{
				m_possibleResults[i].m_isSecondary = FALSE;
			}
		}

		// ...search for duplicate entries
		for( i = 0; i < iCount; i++ )
		{
			iResult = &(m_possibleResults[i]);
			for( j = i+1; j < iCount; j++ )
			{
				jResult = &(m_possibleResults[j]);
				if( iResult->m_discName == jResult->m_discName )
				{
					jResult->m_isSecondary = TRUE;
				}
			}
		}
	}

	m_state = newState;
	return TRUE;
}


/*******************************************************************************
 * Select a result
 ******************************************************************************/


bool SjFreedbQuery::SelectResult(int resultToUseIndex)
{
	// check index
	if( resultToUseIndex < 0
	        || resultToUseIndex >= (int)m_possibleResults.GetCount() )
	{
		m_error = wxT("Invalid index.");
		m_state = SJ_FREEDB_STATE_RESULT_ERROR;
		return FALSE;
	}

	const SjFreedbPossibleResult* resultToUse = GetPossibleResult(resultToUseIndex);
	wxASSERT( resultToUse );
	m_resultTracks = resultToUse->m_tracks;

	// init
	int i, trackCount = m_resultTracks->GetCount();
	for( i = 0; i < trackCount; i++ )
	{
		m_resultTracks->Item(i).m_freedbGenre.Clear();
		m_resultTracks->Item(i).m_freedbArtist.Clear();
		m_resultTracks->Item(i).m_freedbAlbum.Clear();
		m_resultTracks->Item(i).m_freedbTitle.Clear();
		m_resultTracks->Item(i).m_freedbYear = 0;
		m_resultTracks->Item(i).m_freedbTrackNr = 0;
		m_resultTracks->Item(i).m_freedbTrackCount = 0;
	}

	// query the server
	m_httpCmd = wxString::Format(wxT("cddb read %s %s"),
	                             resultToUse->m_genre.c_str(),
	                             resultToUse->m_discId.c_str());
	m_state = SJ_FREEDB_STATE_WAIT_FOR_RESULT;
	m_http.OpenFile(BuildQueryUrl(m_httpCmd), PROTOCOL_ENC);
	return TRUE;
}


bool SjFreedbQuery::SelectResultContinued()
{
	int i, trackCount = m_resultTracks->GetCount();

	wxString result = m_http.GetString();
	if( result.IsEmpty() )
	{
		wxString cmd(m_httpCmd);
		if( cmd.Len() > 64 ) cmd.Truncate(60)+wxT("...");
		m_error = wxString::Format(_("Cannot open \"%s\"."), cmd.c_str());
		m_state = SJ_FREEDB_STATE_RESULT_ERROR;
		return FALSE;
	}

	wxLogDebug(result);

	// go through the result
	wxString dartist, dtitle, dgenre;
	long dyear = 0;
	{
		const wxChar*   currLinePtr;
		wxString        currLine;
		wxString        currKey, currKeyRest, currValue;
		long            currLong;
		bool            firstLine = TRUE;
		SjLineTokenizer tkz(result);
		while( (currLinePtr=tkz.GetNextLine()) )
		{
			if( currLinePtr[0]
			 && currLinePtr[0] != '#' )
			{
				currLine = currLinePtr;
				if( firstLine )
				{
					wxString codeStr = currLine.BeforeFirst(' ');
					long codeLong = 0;
					if( !codeStr.ToLong(&codeLong, 10) ) { codeLong = 0;  }
					if( codeLong != 210 )
					{
						m_error = wxString::Format(_("Error %i"), (int)codeLong) + wxT(":") + currLine;
						m_state = SJ_FREEDB_STATE_RESULT_ERROR;
						return FALSE;
					}
					firstLine = FALSE;
				}
				else if( currLine == wxT(".") )
				{
					break; // done
				}
				else
				{
					currKey = currLine.BeforeFirst('=').Trim().Upper();
					currValue = currLine.AfterFirst('=').Trim(FALSE);
					if( !currKey.IsEmpty() && !currValue.IsEmpty() )
					{
						if( currKey.StartsWith(wxT("TTITLE"), &currKeyRest) )
						{
							currLong = 0;
							if( currKeyRest.ToLong(&currLong, 10)
							        && currLong>=0
							        && currLong<trackCount )
							{
								SplitField(currValue, m_resultTracks->Item(currLong).m_freedbArtist, m_resultTracks->Item(currLong).m_freedbTitle);
							}
						}
						else if( currKey == wxT("DTITLE") )
						{
							SplitField(currValue, dartist, dtitle);
							if( dartist.Lower() == wxT("various") )
							{
								dartist.Clear();  // do not set this artist name
							}
							else if( dartist.IsEmpty() )
							{
								dartist = dtitle; // If the "/" is absent, it is implied that the
								// artist and disc title are the same
							}
						}
						else if( currKey == wxT("DYEAR") )
						{
							currLong = 0;
							if( currValue.ToLong(&currLong, 10)
							        && currLong > 0 )
							{
								dyear = currLong;
							}
						}
						else if( currKey == wxT("DGENRE") )
						{
							dgenre = currValue;
						}
					}
				}
			}
		}
	}

	// finalize the result
	for( i = 0; i < trackCount; i++ )
	{
		m_resultTracks->Item(i).m_freedbTrackNr = i+1;
		m_resultTracks->Item(i).m_freedbTrackCount = trackCount;
		m_resultTracks->Item(i).m_freedbAlbum = dtitle;
		m_resultTracks->Item(i).m_freedbYear = dyear;
		m_resultTracks->Item(i).m_freedbGenre = dgenre;
		if( m_resultTracks->Item(i).m_freedbArtist.IsEmpty() )
		{
			m_resultTracks->Item(i).m_freedbArtist = dartist;
		}
	}

	m_state = SJ_FREEDB_STATE_RESULT_DONE;
	return TRUE;
}


const SjFreedbTrack* SjFreedbQuery::GetResultingTrack(const wxString& url)
{
	int i, trackCount = m_resultTracks->GetCount();
	for( i = 0; i < trackCount; i++ )
	{
		if( m_resultTracks->Item(i).m_localUrl == url )
		{
			return &(m_resultTracks->Item(i));
		}
	}
	return NULL;
}


/*******************************************************************************
 * Tools
 ******************************************************************************/


bool SjFreedbQuery::HasTooManyQuestionMarks(const wxString& str)
{
	return (str.Freq('?')>=(int)(str.Len()/4));
}


void SjFreedbQuery::SplitField(const wxString& in, wxString& out1, wxString& out2)
{
	int pos = in.Find(wxT(" / ") /*by freedb convention, note the spaces*/);
	if( pos == -1 )
	{
		out1.Clear();
		out2 = in;
	}
	else
	{
		out1 = in.Left(pos).Trim();
		out2 = in.Mid(pos+3).Trim(FALSE);
	}
}


int SjFreedbQuery::CddbSum(int n)
{
	int ret;

	/* For backward compatibility this algorithm must not change */

	ret = 0;

	while (n > 0) {
		ret = ret + (n % 10);
		n = n / 10;
	}

	return (ret);
}


wxString SjFreedbQuery::BuildQueryUrl(const wxString& cmd)
{
	return wxString::Format(wxT("http://%s/~cddb/cddb.cgi?cmd=%s&hello=%s+%s+Silverjuke+%i.%i") PROTOCOL_LEVEL,
	                        GetFreedbHost().c_str(),
	                        SjTools::Urlencode(cmd).c_str(),
	                        SjTools::Urlencode(GetUserName()).c_str(),
	                        SjTools::Urlencode(GetUserHost()).c_str(),
	                        SJ_VERSION_MINOR, SJ_VERSION_MAJOR);
}


void SjFreedbQuery::OnHttpEvent()
{
	switch( m_state )
	{
		case SJ_FREEDB_STATE_WAIT_FOR_QUERY1:
			QueryDoContinued(m_tracks1__, SJ_FREEDB_STATE_QUERY1_DONE);
			m_error.Clear(); // ignore any errors, we're not ready with the search
			QueryDo(m_tracks2__, SJ_FREEDB_STATE_WAIT_FOR_QUERY2);
			break;

		case SJ_FREEDB_STATE_WAIT_FOR_QUERY2:
			QueryDoContinued(m_tracks2__, SJ_FREEDB_STATE_QUERY2_DONE);
			break;

		case SJ_FREEDB_STATE_WAIT_FOR_RESULT:
			SelectResultContinued();
			break;

		default:
			break;
	}
}


wxString SjFreedbQuery::GetFreedbHost()
{
	return g_tools->m_config->Read(wxT("tageditor/freedbHost"), SJ_FREEDB_DEF_HOST);
}


wxString SjFreedbQuery::GetUserName()
{
	return wxT("anonymous");
}


wxString SjFreedbQuery::GetUserHost()
{
	return wxT("localhost");
}




