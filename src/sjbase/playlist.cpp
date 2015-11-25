/*********************************************************************************
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
 * File:    playlist.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke playlist handling
 *
 *******************************************************************************
 *
 *  Some information about playlists:
 *  - http://gonze.com/playlists/playlist-format-survey.html
 *
 *  Further notes:
 * - Relative URLs supported on load
 * - We do a very lazy but _fast_ load: We just take the strings as given and
 *   remember the containing playlist file (if any), later on, we verify the
 *   entry as needed.  This allows us to load playlists with thousands tracks
 *   in a fragment of a second.
 * - We're using arist and title information in *.m3u and *.pls files if the URL
 *   is not found.  This allows moving playlists with relative paths if the
 *   title is in the library.
 * - Hack: The artist/title is added after a tab to the unverified URL
 * - Nero does not like spaces in CUE filenames
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/playlist.h>
#include <tagger/tg_a_tagger_frontend.h>
#include <wx/html/htmlwin.h>

#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArrayPlaylistEntry);

#undef DEBUG_VERIFY


/*******************************************************************************
 * SjPlaylistEntry
 ******************************************************************************/


long SjPlaylistEntry::s_nextId = 1;


void SjPlaylistEntry::LoadAddInfo(long what)
{
	if( g_mainFrame == NULL )
	{
		return;
	}

	// allocate add. information
	if( m_addInfo == NULL )
	{
		m_addInfo = new SjPlaylistAddInfo();
	}

	// for performance reasons, LoadAddInfo() should be called only if really needed;
	// use CheckAddInfo() if you are unsure
	wxASSERT( (m_addInfo->m_what & what) == 0 );

	// handle playcount and timestamp
	if( what&SJ_ADDINFO_PLAYCOUNT )
	{
		m_addInfo->m_what |= SJ_ADDINFO_PLAYCOUNT;
	}

	// load track, artist and album name, if needed
	if(  (what&SJ_ADDINFO_MISC)
	 && !(m_addInfo->m_what&SJ_ADDINFO_MISC) )
	{
		// try to get them from the library
		if( !g_mainFrame->m_columnMixer.GetQuickInfo(GetUrl(), m_addInfo->m_trackName, m_addInfo->m_leadArtistName, m_addInfo->m_albumName, m_addInfo->m_playtimeMs) )
		{
			// try to get them from the decoding module that will handle this file
			wxASSERT( m_urlVerified );
			if( m_urlOk )
			{
				wxString testUrl = GetUrl();
				if( !testUrl.StartsWith(wxT("http:")) // this may be a steam - in this case (or in others) we get into an endless loop
				 && !testUrl.StartsWith(wxT("https:"))
				 && !testUrl.StartsWith(wxT("ftp:")) )
				{
					SjTrackInfo trackInfo;
					wxFileSystem fs;
					wxFSFile*    fsFile = fs.OpenFile(GetUrl(), wxFS_READ|wxFS_SEEKABLE);
					if( fsFile )
					{
						if( SjGetTrackInfoFromID3Etc(fsFile, trackInfo, SJ_TI_QUICKINFO) == SJ_SUCCESS )
						{
							m_addInfo->m_trackName      = trackInfo.m_trackName;
							m_addInfo->m_leadArtistName = trackInfo.m_leadArtistName;
							m_addInfo->m_albumName      = trackInfo.m_albumName;
							m_addInfo->m_playtimeMs     = trackInfo.m_playtimeMs;

							if( m_addInfo->m_trackName.IsEmpty() )
							{
								m_addInfo->m_trackName = GetUrl();
							}
						}
						delete fsFile;
					}
				}
			}
		}

		// SjPlaylistEntry uses -1 as invalid playing times
		if( m_addInfo->m_playtimeMs <= 0 )
		{
			m_addInfo->m_playtimeMs = -1;
		}

		// remember that these info are checked
		m_addInfo->m_what |= SJ_ADDINFO_MISC;
	}
}


wxString SjPlaylistEntry::GetLocalFile(const wxString& containerFile__)
{
	wxFileName urlFn = wxFileSystem::URLToFileName(GetUrl());

	if( !containerFile__.IsEmpty() )
	{
		wxString containerFile(containerFile__);
		#ifdef __WXMSW__
			containerFile.Replace(wxT("/"), wxT("\\"));
		#endif
		wxFileName containerFn(containerFile);

		urlFn.MakeRelativeTo(containerFn.GetPath(wxPATH_GET_VOLUME|wxPATH_GET_SEPARATOR));
	}

	return urlFn.GetFullPath();
}


void SjPlaylistEntry::SetRealtimeInfo(const wxString& info__)
{
	// normalize the given info -- some broadcasting stations
	// use "--" as the artist/title separator, leave out the artist, show all in uppercase
	// and so on ...
	wxString info(info__);
	info.Replace(wxT("--"), wxT("-"));
	if( info.Upper() == info || info.Lower() == info )
	{
		info = SjTools::Capitalize(info);
	}

	while( info.Len() > 0 && (info[0] == wxT('-') || info[0] == wxT(' ')) )
	{
		info = info.Mid(1);
	}

	while( info.Len() > 0 && (info.Last() == wxT('-') || info.Last() == wxT(' ')) )
	{
		info = info.Left(info.Len()-1);
	}

	// set the normalized info string as
	CheckAddInfo(SJ_ADDINFO_MISC);

	int p = info.Find(wxT(" - "));
	if( p != -1 )
	{
		wxString i1 = info.Left(p).Trim();
		wxString i2 = info.Mid(p+3).Trim(false);
		if( !i1.IsEmpty() || !i2.IsEmpty() )
		{
			if( i2.IsEmpty() )
			{
				m_addInfo->m_trackName = i1;
			}
			else
			{
				m_addInfo->m_leadArtistName = i1;
				m_addInfo->m_trackName = i2;
			}

		}
	}
	else if( !info.IsEmpty() )
	{
		m_addInfo->m_trackName = info;
	}
}


/*******************************************************************************
 * SjPlaylist - Handling URLs
 ******************************************************************************/


void SjPlaylistEntry::VerifyUrl()
{
	// as we're verifing, don't log any errors
	wxLogNull null;

	// main frame available? without the main frame, we cannot verify any URLs.
	if( g_mainFrame == NULL )
	{
		return;
	}

	// for speed reasons, the calling function should call
	// VerifyUrl() not without reason
	wxASSERT( !m_urlVerified );
	m_urlVerified = TRUE;

	// copy the original url as it will be modified;
	// the original url should only be overwritten on total success
	wxString url(m_url.BeforeFirst('\t'));

	// get the long and absolute version of the URL
	if( url.StartsWith(wxT("file:")) )
	{
		wxFileName urlFn = wxFileSystem::URLToFileName(url);
		url = urlFn.GetLongPath();
	}
	else if( !url.StartsWith(wxT("stub:")) )
	{
		wxFileName urlFn(url, wxPATH_NATIVE);
		if( !urlFn.IsAbsolute() )
		{
			// try relative paths from m_playlist->GetContainerUrls()
			wxArrayString containerFiles = m_playlist->GetContainerFiles();
			for( int containerIndex = (int)containerFiles.GetCount()-1; containerIndex >=0; containerIndex-- )
			{
				// re-assign the relatice path to urlFn - needed if for() has more than one iteration
				urlFn.Assign(url, wxPATH_NATIVE);

				// make urlFn absolute using MakeAbsolute(GetPath()) (GetPath() does not return the name, so this should work just fine)
				wxString tempStr(containerFiles[containerIndex]);
				#ifdef __WXMSW__
					tempStr.Replace(wxT("/"), wxT("\\")); // needed as wxPATH_NATIVE obviously expects native paths ... see http://www.silverjuke.net/forum/post.php?p=14207#14207
				#endif
				wxFileName tempFn(tempStr, wxPATH_NATIVE);
				urlFn.MakeAbsolute(tempFn.GetPath(wxPATH_GET_VOLUME));

				// success?
				if( urlFn.FileExists() )
				{
					url = urlFn.GetLongPath();
					break;
				}
			}
		}
		else
		{
			url = urlFn.GetLongPath();
		}
	}

	// open file - as we also support eg. ZIP archives this is needed for validating
	// relative paths are not valid at this moment
	wxString fsFileLocation;
	{
		if(  0 && url.Len()>2
		        &&
			#ifdef __WXMSW__
		        url[1u]==wxT(':')
			#else
		        url[0]=='/'
			#endif
		        && ::wxFileExists(url) )
		{
			// some speed improvements for "normal" files
			fsFileLocation = url;
			fsFileLocation.Replace(wxT("\\"), wxT("/"));
		}
		else
		{
			wxFSFile* fsFile = NULL;

			if( !url.StartsWith(wxT(".."))
			 && !url.StartsWith(wxT("./"))
			 && !url.StartsWith(wxT(".\\"))
			 && !url.StartsWith(wxT("stub:")) )
			{
				wxFileSystem fileSystem;
				fsFile = fileSystem.OpenFile(url);
			}

			if( fsFile == NULL )
			{
				// try to lookup the URL by artist/album/track
				wxString artistName = m_url.AfterFirst('\t').BeforeFirst('\t');
				wxString albumName  = m_url.AfterFirst('\t').AfterFirst('\t').BeforeFirst('\t');
				wxString trackName  = m_url.AfterLast('\t');
				if( !artistName.IsEmpty() && !trackName.IsEmpty() /*album may be empty, eg. for m3u*/)
				{
					url = g_mainFrame->m_libraryModule->GetUrl(artistName, albumName, trackName);
					if( !url.IsEmpty() )
					{
						wxFileSystem fileSystem;
						fsFile = fileSystem.OpenFile(url);
					}
				}

				if( fsFile == NULL )
				{
					m_url = m_url.BeforeFirst('\t');
					return; // Url not found
				}
			}

			fsFileLocation = fsFile->GetLocation();
			delete fsFile;
		}
	}

	// convert the file name to a URL
	if( !fsFileLocation.StartsWith("file:")
	 && !fsFileLocation.StartsWith("http:")
	 && !fsFileLocation.StartsWith("https:")
	 && !fsFileLocation.StartsWith("ftp:") )
	{
		wxFileName fn(fsFileLocation);
		fsFileLocation = wxFileSystem::FileNameToURL(fsFileLocation);
	}


	// make sure, we're using the correct case, see
	// http://www.silverjuke.net/forum/topic-2406.html
	#ifdef __WXMSW__
	{
		wxSqlt sql;
		sql.Query(wxT("SELECT url FROM tracks WHERE url='") + sql.QParam(fsFileLocation) + wxT("';"));
		if( !sql.Next() )
		{
			sql.Query(wxT("SELECT url FROM tracks WHERE url LIKE '") + sql.QParam(fsFileLocation) + wxT("';"));
			while( sql.Next() )                 //          ^^^ LIKE is case-insensitive
			{	//          <<< use while() as the url may contain '%'
				wxString test = sql.GetString(0);
				if( test.Lower() == fsFileLocation.Lower() )
				{
					fsFileLocation = test;
					break;
				}
			}
		}
	}
	#endif

	// file opened - save the location as the verified URL
	// and close the file
	if( m_playlist )
	{
		m_playlist->RehashUrl(m_url/*really the original URL*/, fsFileLocation);
	}

	m_url = fsFileLocation;

	// file is okay - but is it playable by us?
	if( g_mainFrame->m_player.TestUrl(m_url) )
	{
		m_urlOk = TRUE; // very fine ;-)
	}

	// done
	#ifdef DEBUG_VERIFY
		wxLogDebug(wxT("%s verified..."), m_url.c_str());
	#endif
}


void SjPlaylist::RehashUrl(const wxString& oldUrl, const wxString& newUrl)
{
	// Get the sum of occurences from old and new url
	// and insert both together as the new url.
	//
	// Before Silverjuke 1.10 there was a bad error here - instead of using
	//
	//      " long count = m_urlCounts.Remove(oldUrl); "
	//      " count += m_urlCounts.Lookup(newUrl);     "
	//
	// we've used
	//
	//      " long count = m_urlCounts.Remove(oldUrl) + m_urlCounts.Lookup(newUrl); "
	//
	// which leads to crashes only in the release version... remember, the order
	// the compiler sums the stuff up is undefined - this lead to problems if
	// oldUrl and newUrl were equal and Lookup() was processed first.
	//
	// The error was visible in Silverjuke: Already unqueued tracks were still
	// marked in the album view as IsInPlaylist() returns a bad value.  Moreover,
	// this lead to crashes eg. if the tag editor was opened for these URLs.
	long count = m_urlCounts.Remove(oldUrl);
	count += m_urlCounts.Lookup(newUrl);

	wxASSERT( count > 0 );
	m_urlCounts.Insert(newUrl, count);
}


void SjPlaylist::OnUrlChanged(const wxString& oldUrl, const wxString& newUrl)
{
	if( IsInPlaylist(oldUrl) )
	{
		// url renamed?
		if( !newUrl.IsEmpty() )
		{
			long i, iCount = m_array.GetCount();
			for( i = 0; i < iCount; i++ )
			{
				m_array[i].RenameUrl(oldUrl, newUrl);
			}

			long count = m_urlCounts.Remove(oldUrl);
			if( count )
			{
				m_urlCounts.Insert(newUrl, count);
			}
		}

		// force reloading information about this url
		long i, iCount = m_array.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			if( m_array[i].GetUnverifiedUrl()==oldUrl )
			{
				SjPlaylistEntry& e = m_array[i];
				long playCount = e.GetPlayCount(); // as UrlChanged() simply forgets everything about the URL, preserve some data manually
				long flags = e.GetFlags();
				e.UrlChanged();
				e.SetPlayCount(playCount);
				e.SetFlags(flags);
			}
		}
	}
}


/*******************************************************************************
 * SjPlaylist - add'n'remove
 ******************************************************************************/


void SjPlaylist::Add(const wxArrayString& urls, bool urlsVerified)
{
	long i, iCount = urls.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		Add(urls[i], urlsVerified, 0);
	}
}


long SjPlaylist::RemoveAt(long index)
{
	m_cacheFlags = 0;

	wxString url = m_array[index].GetUrl();

	long restCount = m_urlCounts.Remove(url);
	if( restCount > 1 )
	{
		m_urlCounts.Insert(url, restCount-1);
	}

	m_array.RemoveAt(index);

	return restCount-1;
}


void SjPlaylist::Remove(const wxArrayString& urls)
{
	long u, urlsCount = urls.GetCount(), index;
	for( u = 0; u < urlsCount; u++ )
	{
		while( (index=GetPosByUrl(urls[u])) != wxNOT_FOUND )
		{
			RemoveAt(index);
		}
	}
}


long SjPlaylist::GetPosByUrl(const wxString& url) const
{
	if( IsInPlaylist(url) )
	{
		long i, iCount = m_array.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			if( url.CmpNoCase(m_array[i].GetUrl())==0 )
			{
				return i;
			}
		}
	}

	return wxNOT_FOUND;
}


long SjPlaylist::GetUnplayedCount(long currPos, long maxCnt) const
{
	long    unplayedCnt = 0;
	long    i;

	if( currPos < 0 )
	{
		currPos = 0;
	}

	// Count the unplayed titles; we're starting at the end of the list
	// as normally the unplayed titles are here, esp. in kiosk mode where
	// we use this function.
	for( i = (long)(m_array.GetCount())-1; i >= currPos; i-- )
	{
		if( m_array[i].GetPlayCount() == 0 )
		{
			unplayedCnt++;
			if( unplayedCnt >= maxCnt )
			{
				wxASSERT( unplayedCnt == maxCnt );
				break;
			}
		}
	}

	return unplayedCnt;
}


void SjPlaylist::MovePos(long srcPos, long destPos)
{
	SjPlaylistEntry* entryToMove = m_array.Detach(srcPos);

	m_array.Insert(entryToMove, destPos);
}


void SjPlaylist::UpdateUrl(const wxString& url, bool urlVerified, long playtimeMs)
{
	if( IsInPlaylist(url) )
	{
		long i, iCount = m_array.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			if( url.CmpNoCase(m_array[i].GetUrl())==0 )
			{
				m_array[i].SetPlaytimeMs(playtimeMs);
			}
		}
	}
}


/*******************************************************************************
 * SjPlaylist - id -> index
 ******************************************************************************/


long SjPlaylist::GetPosById(long id) const
{
	// This function may only be called from the main thread.
	wxASSERT( wxThread::IsMain() );

	long queueCount = GetCount(), i;

	// search the id
	for( i = 0; i < queueCount; i++ )
	{
		if( m_array.Item(i).GetId()==id )
		{
			return i;
		}
	}

	// id not found
	return -1;
}


/*******************************************************************************
 * SjPlaylist - import / export basicss
 ******************************************************************************/


void SjPlaylist::LoadOverallNames()
{
	// stuff already loaded?
	if( m_cacheFlags & SJ_CACHE_OVERALL_LOADED )
	{
		return;
	}

	// load overall stuff
	m_cacheFlags |= SJ_CACHE_OVERALL_LOADED;
	long i, iCount = GetCount();

	if( iCount > 0 )
	{
		m_cacheFlags |= SJ_CACHE_LEAD_ARTIST_FINE|SJ_CACHE_ALBUM_FINE;
		m_overallLeadArtistName = Item(0).GetLeadArtistName();
		m_overallAlbumName = Item(0).GetAlbumName();
		for( i = 1; i < iCount; i++ )
		{
			if( Item(i).GetLeadArtistName() != m_overallLeadArtistName )
			{
				m_cacheFlags &= ~SJ_CACHE_LEAD_ARTIST_FINE;
				if( !(m_cacheFlags&SJ_CACHE_ALBUM_FINE) ) break; // nothing more we can find out
			}

			if( Item(i).GetAlbumName() != m_overallAlbumName )
			{
				m_cacheFlags &= ~SJ_CACHE_ALBUM_FINE;
				if( !(m_cacheFlags&SJ_CACHE_LEAD_ARTIST_FINE) ) break; // nothing more we can find out
			}
		}
	}

	m_overallLeadArtistName.Trim(TRUE);
	m_overallLeadArtistName.Trim(FALSE);

	m_overallAlbumName.Trim(TRUE);
	m_overallAlbumName.Trim(FALSE);

	if( !(m_cacheFlags&SJ_CACHE_LEAD_ARTIST_FINE) || m_overallLeadArtistName.IsEmpty() )    m_overallLeadArtistName = _("Several artists");
	if( !(m_cacheFlags&SJ_CACHE_ALBUM_FINE) || m_overallAlbumName.IsEmpty() )               m_overallAlbumName = _("Unknown title");
}


wxString SjPlaylist::SuggestPlaylistName()
{
	LoadOverallNames();

	// try to use album or artist name
	wxString ret;
	if( !m_playlistName.IsEmpty() )
	{
		ret = m_playlistName;
	}
	else if( (m_cacheFlags&SJ_CACHE_ALBUM_FINE) && (m_cacheFlags&SJ_CACHE_LEAD_ARTIST_FINE) )
	{
		ret = m_overallLeadArtistName + wxT(" - ") + m_overallAlbumName;
	}
	else if( m_cacheFlags&SJ_CACHE_ALBUM_FINE )
	{
		ret = m_overallAlbumName;
	}
	else if( m_cacheFlags&SJ_CACHE_LEAD_ARTIST_FINE )
	{
		ret = m_overallLeadArtistName;
	}

	// done, so far
	return ret;
}


wxString SjPlaylist::SuggestPlaylistFileName()
{
	return SjTools::EnsureValidFileNameChars(SuggestPlaylistName());
}


void SjPlaylist::MergeMetaData(const SjPlaylist& o)
{
	if( m_playlistName.IsEmpty() )
	{
		m_playlistName = o.m_playlistName;
	}

	if( m_playlistUrl.IsEmpty() )
	{
		m_playlistUrl = o.m_playlistUrl;
	}

	int oContainerIndex, oContainerCount = o.m_containerFiles.GetCount();
	for( oContainerIndex = 0; oContainerIndex < oContainerCount; oContainerIndex++ )
	{
		AddContainerFile(o.m_containerFiles[oContainerIndex]);
	}
}


/*******************************************************************************
 * SjPlaylist - Handling *.m3u Playlists
 ******************************************************************************/


bool SjPlaylist::AddFromM3u(wxFSFile* fsFile, long addMax, long flags)
{
	// get file content
	wxString ext = SjTools::GetExt(fsFile->GetLocation());

	wxMBConv* fileContentMbConv = &wxConvISO8859_1;
	if( ext == wxT("m3u8") )
	{
		fileContentMbConv = &wxConvUTF8;
	}

	wxString content = SjTools::GetFileContent(fsFile->GetStream(), fileContentMbConv); // GetFileContent() will also check for the BOM (Byte order mark)

	// process
	SjLineTokenizer     tkz(content);
	wxChar*             currLinePtr;
	wxString            currTitle;
	long                filesAdded = 0;

	while( (currLinePtr=tkz.GetNextLine()) )
	{
		if( *currLinePtr == 0 )
		{
			// skip empty lines
			continue;
		}
		else if( *currLinePtr == wxT('#') )
		{
			// read comment - the comment is used by VerifyUrl() to find the track in the library if the URL cannot be found (bad path, bad name etc.)
			if( wxStrncmp(currLinePtr, wxT("#EXTINF:"), 8)==0 )
			{
				currTitle = wxString(&currLinePtr[8]);
				currTitle = currTitle.AfterFirst(',');              // skip seconds parameter from "#EXTINF:seconds,artiest ..."

				if( currTitle.Replace(wxT(" - "), wxT("\t\t")) < 1 )// normally, the format ist "Artist - Title" ...
				{                                                   // ... however, since 3.02, we also allow "Artist-Title" ...
					currTitle.Replace(wxT("-"), wxT("\t\t"));       // ... and, later in VerifyUrl() also "Title - Artist" and "Title-Artist" :-)
				}
			}
			continue;
		}

		Add(currLinePtr + (wxT("\t") + currTitle), FALSE, 0);
		currTitle.Empty();

		filesAdded++;

		if( filesAdded >= addMax )
		{
			break;
		}
	}

	return TRUE;
}


wxString SjPlaylist::SaveAsM3u(const wxString& containerFile, long flags)
{
	wxString    ret;
	wxString    linebreak = SjTools::GetLineBreak();
	wxString    urlToSave;

	if( !(flags & SJ_M3U_NO_EXT) )
	{
		ret << wxT("#EXTM3U") << linebreak;
	}

	long i, iCount = GetCount(), seconds;
	for( i = 0; i < iCount; i++ )
	{
		urlToSave = Item(i).GetLocalFile(containerFile);
		if( Item(i).IsUrlOk() )
		{
			if( !(flags & SJ_M3U_NO_EXT) )
			{
				seconds = Item(i).GetPlaytimeMs()/1000;
				ret << wxString::Format(wxT("#EXTINF:%i,%s - %s"), (int)(seconds==-1? 0 : seconds), Item(i).GetLeadArtistName().c_str(), Item(i).GetTrackName().c_str()) << linebreak;
			}

			ret << urlToSave + linebreak;
		}

		if( !SjBusyInfo::Set(urlToSave) ) break;
	}

	return ret;
}


/*******************************************************************************
 * SjPlaylist - Handling *.pls Playlists
 ******************************************************************************/


bool SjPlaylist::AddFromPls(wxFSFile* fsFile, long addMax, long flags)
{
	wxString            content = SjTools::GetFileContent(fsFile->GetStream(), &wxConvISO8859_1);
	SjLineTokenizer     tkz(content);
	wxChar*             currLinePtr;
	wxString            currLine, currBegin, currNumStr, currRest;
	long                currNumLong;

	wxArrayString       urls;
	long                urlCount = 0, urlsAdded = 0, urlIndex;

	wxArrayString       titles;
	long                titleCount = 0;

	while( (currLinePtr=tkz.GetNextLine()) )
	{
		if( *currLinePtr == 0 ) continue; // skip empty line

		// read line
		currLine = currLinePtr;

		// split line at '='
		currBegin = currLine.BeforeFirst(wxT('='));
		currBegin.Trim(TRUE); // beginning is already trimmed
		if( currBegin.IsEmpty() ) continue;

		currRest = currLine.AfterFirst(wxT('='));
		currRest.Trim(FALSE); // end is already trimmed
		if( currRest.IsEmpty() ) continue;

		// does the line begin with 'file<num>'
		currBegin.MakeLower();
		if( currBegin.StartsWith(wxT("file"), &currNumStr) )
		{
			if( !currNumStr.ToLong(&currNumLong, 10) || currNumLong < 1 || currNumLong > 0xFFFFL ) continue;

			// add file
			if( currNumLong>urlCount ) urls.Insert(wxEmptyString, urlCount, currNumLong-urlCount);
			urls[currNumLong-1] = currRest;
			urlCount++;
		}
		else if( currBegin.StartsWith(wxT("title"), &currNumStr) )
		{
			if( !currNumStr.ToLong(&currNumLong, 10) || currNumLong < 1 || currNumLong > 0xFFFFL ) continue;

			// set title, helpful for VerifyUrl(), see comments in AddFromM3u()
			if( currNumLong>titleCount ) titles.Insert(wxEmptyString, titleCount, currNumLong-titleCount);
			if( currRest.Replace(wxT(" - "), wxT("\t\t")) < 1 )
				currRest.Replace(wxT("-"), wxT("\t\t"));
			titles[currNumLong-1] = currRest;
			titleCount++;
		}
		else
		{
			continue;
		}
	}

	urlCount = urls.GetCount();
	for( urlIndex = 0; urlIndex < urlCount; urlIndex++ )
	{
		currLine = urls[urlIndex];
		if( !currLine.IsEmpty() )
		{
			if( urlIndex < titleCount )
			{
				currLine += wxT("\t") + titles[urlIndex] /*may be empty*/;
			}

			Add(currLine, FALSE/*not verified*/, 0);

			urlsAdded++;
			if( urlsAdded >= addMax ) break;
		}
	}

	return TRUE;
}


wxString SjPlaylist::SaveAsPls(const wxString& containerFile, long flags)
{
	wxString    ret;
	wxString    linebreak = SjTools::GetLineBreak();
	wxString    num;
	wxString    playlistName = SuggestPlaylistName();
	wxString    urlToSave;

	ret << wxT("[playlist]") << linebreak;

	if( !playlistName.IsEmpty() )
	{
		ret << wxT("PlaylistName=") << playlistName << linebreak;
	}

	long i, iCount = GetCount(), savedCount = 0, seconds;
	for( i = 0; i < iCount; i++ )
	{
		urlToSave = Item(i).GetLocalFile(wxEmptyString/*always save abs. path*/);

		if( Item(i).IsUrlOk() )
		{
			num = wxString::Format(wxT("%i"), (int)i+1);
			seconds = Item(i).GetPlaytimeMs()/1000;

			ret << wxT("File")   << num << wxT("=") << urlToSave << linebreak;
			ret << wxT("Title")  << num << wxT("=") << Item(i).GetLeadArtistName() << wxT(" - ") << Item(i).GetTrackName() << linebreak;
			ret << wxT("Length") << num << wxT("=") << wxString::Format(wxT("%i"), seconds==-1? 0 : (int)seconds) << linebreak;

			savedCount++;
		}

		if( !SjBusyInfo::Set(urlToSave) ) break;
	}

	// these MUST be the last entries,
	// see http://docs.wasabidev.org/wasabi_developer_manual/winamp_playlists_and_playlist_directory.php#playlists_formats
	ret << wxString::Format(wxT("NumberOfEntries=%i"), (int)savedCount) << linebreak;
	ret << wxT("Version=2") << linebreak;

	return ret;
}


/*******************************************************************************
 * SjPlaylist - Handling Cue Sheets
 ******************************************************************************/


bool SjPlaylist::AddFromCue(wxFSFile* fsFile, long addMax, long flags)
{
	wxString            content = SjTools::GetFileContent(fsFile->GetStream(), &wxConvISO8859_1);
	SjLineTokenizer     tkz(content);
	wxChar*             currLinePtr;
	wxString            currLine;
	long                filesAdded = 0;

	while( (currLinePtr=tkz.GetNextLine()) )
	{
		if( *currLinePtr == 0 ) continue; // skip empty line

		// read line
		currLine = currLinePtr;
		currLine.Replace(wxT("\t"), wxT(" "));
		if( currLine.Left(5).Upper()!=wxT("FILE ") ) continue;

		// get stuff between quotes
		if( currLine.Find('"')!=-1 )
		{
			currLine = currLine.AfterFirst('"').BeforeLast('"');
		}
		else
		{
			currLine = currLine.Mid(4);
			currLine.Trim(FALSE);
			currLine = currLine.BeforeFirst(' ');
		}

		currLine.Trim(TRUE);
		currLine.Trim(FALSE);
		if( currLine.IsEmpty() ) continue;

		// file already added? - this is quite usual for cue-sheets as they may use a
		// large file and the INDEX parameter.  however, we only want the referenced files
		if( GetPosByUrl(currLine) != wxNOT_FOUND ) continue;

		// add file
		Add(currLine, FALSE/*not verified*/, 0);
		filesAdded++;

		if( filesAdded >= addMax ) break;
	}

	return TRUE;
}


wxString SjPlaylist::SaveAsCue(const wxString& containerFile, long flags)
{
	wxString    ret;
	wxString    linebreak = SjTools::GetLineBreak();
	wxString    urlToSave;

	ret << wxT("PERFORMER \"") << GetLeadArtistName() << wxT("\"") << linebreak;
	ret << wxT("TITLE \"") << GetAlbumName() << wxT("\"") << linebreak;

	long i, iCount = GetCount();
	for( i = 0; i < iCount; i++ )
	{
		urlToSave = Item(i).GetLocalFile(wxEmptyString/*always save abs. path*/);

		if( Item(i).IsUrlOk() )
		{
			#ifdef __WXMSW__
				if( flags & SJ_CUE_SHORTPATHS )
				{
					wxFileName fn(urlToSave);
					urlToSave = fn.GetShortPath();
				}
			#endif

			ret << wxT("FILE \"") << urlToSave << wxT("\" WAVE") << linebreak;
			ret << wxT("  TRACK ") << wxString::Format(i<=99? wxT("%02i") : wxT("%i"), (int)i+1) << wxT(" AUDIO") << linebreak;
			ret << wxT("    TITLE \"") << Item(i).GetTrackName() << wxT("\"") << linebreak;
			ret << wxT("    PERFORMER \"") << Item(i).GetLeadArtistName() << wxT("\"") << linebreak;
			ret << wxT("    INDEX 01 00:00:00") << linebreak;
		}

		if( !SjBusyInfo::Set(urlToSave) ) break;
	}

	return ret;
}


/*******************************************************************************
 * SjPlaylist - Handling XSPF- and XML/iTunes-playlists
 ******************************************************************************/


bool SjPlaylist::AddFromXspfOrXmlOrWpl(wxFSFile* fsFile, long addMax, long flags)
{
	/* desired XSPF-Format ...
	<track>
	    <creator>We Are Scientists</creator>
	    <album>the album name</album>
	    <title>Nobody Move, Nobody Get Hurt</title>
	    <location>file:///mp3s/titel_1.mp3</location>
	    ...
	</track> */
	wxString content = SjTools::GetFileContent(fsFile->GetStream(), &wxConvUTF8);
	content.Replace(wxT("\n"), wxT("")); // remove line-ends, this allows to put tags over several lines as
	content.Replace(wxT("\r"), wxT("")); // <location> \n \n \n bla \n \n \n</location>.
	// below, we will convert the character "<" to a linebreak ...

	/* ... convert the following XML/iTunes format to XSPF ...
	<dict>
	    <key>Artist</key><string>Led Zeppelin</string>
	    <key>Album</key><string>Coda</string>
	    <key>Name</key><string>Ozone Baby</string>
	    <key>Location</key><string>file://localhost/Volumes/music/mp3/L/Led%20Zeppelin/1982%20Coda/05%20Ozone%20Baby.mp3</string>
	    ...
	</dict> */
	if( content.Replace(wxT("<key>Name</key><string>"), wxT("<title>")) > 0 )
	{
		content.Replace(wxT("<key>Artist</key><string>"), wxT("<creator>"));
		content.Replace(wxT("<key>Album</key><string>"), wxT("<album>"));
		content.Replace(wxT("<key>Location</key><string>"), wxT("<location>"));
		content.Replace(wxT("/dict"), wxT("/track"));
	}

	/* ... convert the following Windows Media Player/WPL format  to XSPF ...
	<?wpl version="1.0"?>
	<smil>
	<head>
	    <meta name="QInfo" content="..."/>
	    ...
	    <title>test</title>
	</head>
	<body>
	    <seq>
	        <media src="file1.mp3"/>
	        <media src="file2.mp3"/>
	        ...
	    </seq>
	</body> */
	if( content.Find(wxT("<?wpl")) != wxNOT_FOUND )
	{
		if( content.Replace(wxT("<media src=\""), wxT("<location>")) > 0 )
		{
			content.Replace(wxT("\""), wxT("</track>"));
			content.Replace(wxT("&apos;"), wxT("'")); // &apos; is no real html entity. normal entities are handled below
		}
	}

	// treat "<" as a new line mark, this allows easy parsing without using a XML tree
	content.Replace(wxT("<"), wxT("\n"));

	// go through the content
	long                    filesAdded = 0;
	SjLineTokenizer         tkz(content);
	wxChar*                 currLinePtr;

	wxString                lastArtistName;
	wxString                lastAlbumName;
	wxString                lastTrackName;
	wxString                lastLocation;

	wxHtmlEntitiesParser    entPars;

	while( (currLinePtr=tkz.GetNextLine()) )
	{
		if( *currLinePtr == 0 ) continue; // skip empty line

		// remove xspf prefix, if any (used by some apps, see http://wiki.xiph.org/List_of_known_XSPF_extensions )
		if( wxStrncmp(currLinePtr, wxT("xspf:"), 5)==0 )
			currLinePtr += 5;

		if( wxStrncmp(currLinePtr, wxT("creator"), 7)==0 )
		{
			// set last artist name
			lastArtistName = currLinePtr;
			lastArtistName = lastArtistName.AfterFirst(wxT('>'));
			lastArtistName = entPars.Parse(lastArtistName);
		}
		else if( wxStrncmp(currLinePtr, wxT("album"), 5)==0 )
		{
			// set last album name
			lastAlbumName = currLinePtr;
			lastAlbumName = lastAlbumName.AfterFirst(wxT('>'));
			lastAlbumName = entPars.Parse(lastAlbumName);
		}
		else if( wxStrncmp(currLinePtr, wxT("title"), 5)==0 )
		{
			// set last track name
			lastTrackName = currLinePtr;
			lastTrackName = lastTrackName.AfterFirst(wxT('>'));
			lastTrackName = entPars.Parse(lastTrackName);
		}
		else if( wxStrncmp(currLinePtr, wxT("location"), 8)==0 )
		{
			// set last location
			lastLocation = currLinePtr;
			lastLocation = lastLocation.AfterFirst(wxT('>'));
			lastLocation = entPars.Parse(lastLocation);
		}
		else if( wxStrncmp(currLinePtr, wxT("/track"), 6)==0 )
		{
			// flush
			if( lastLocation.IsEmpty() && !lastTrackName.IsEmpty() && !lastArtistName.IsEmpty() )
			{
				lastLocation = wxT("stub://") + SjTools::EnsureValidFileNameChars(lastArtistName) + wxT("-") + SjTools::EnsureValidFileNameChars(lastAlbumName) + wxT("-") + SjNormaliseString(lastTrackName, 0) + wxT(".mp3");
				// in 99.99% of all cases, this stub location will fail, however, this creates a fine entry in the playlist
			}

			if( !lastLocation.IsEmpty() )
			{
				Add(lastLocation + wxT("\t") + lastArtistName + wxT("\t") + lastAlbumName + wxT("\t") + lastTrackName, FALSE/*not verified*/, 0);
				filesAdded++;
				if( filesAdded >= addMax )
					break;
			}

			lastArtistName.Empty();
			lastAlbumName.Empty();
			lastTrackName.Empty();
			lastLocation.Empty();
		}
	}

	return TRUE;
}


wxString SjPlaylist::SaveAsXspf(const wxString& containerFile, long flags)
{
	wxString    ret, url, realUrl;
	wxString    linebreak = SjTools::GetLineBreak();

	// prepare date
	wxDateTime dt = wxDateTime::Now().ToUTC();
	wxString dtString = dt.Format(wxT("%Y-%m-%dT%H:%M:%S+00:00"));

	// write prologue
	ret << wxT("<?xml version=\"1.0\" encoding=\"UTF-8\"?>") << linebreak;
	ret << wxT("<playlist version=\"1\" xmlns=\"http://xspf.org/ns/0/\">") << linebreak;
	ret << wxT("<date>") << dtString << wxT("</date>") << linebreak;
	ret << wxT("<meta rel=\"generator\">") << wxString::Format(wxT("%s %i.%i"), SJ_PROGRAM_NAME, SJ_VERSION_MAJOR, SJ_VERSION_MINOR) << wxT("</meta>") << linebreak;
	ret << wxT("<trackList>") << linebreak;

	// write the tracks - the recommended order is location-title-creator-album, see http://wiki.xiph.org/index.php/XSPF_v1_Notes_and_Errata
	long i, iCount = GetCount();
	for( i = 0; i < iCount; i++ )
	{
		ret << wxT("\t<track>") << linebreak;

		// write url
		ret << wxT("\t\t<location>") << SjTools::Htmlentities(Item(i).GetUrl()) << wxT("</location>") << linebreak;

		// write title
		ret << wxT("\t\t<title>") << SjTools::Htmlentities(Item(i).GetTrackName()) << wxT("</title>") << linebreak;

		// write artist
		ret << wxT("\t\t<creator>") << SjTools::Htmlentities(Item(i).GetLeadArtistName()) << wxT("</creator>") << linebreak;

		// write album
		ret << wxT("\t\t<album>") << SjTools::Htmlentities(Item(i).GetAlbumName()) << wxT("</album>") << linebreak;

		ret << wxT("\t</track>") << linebreak;

		if( !SjBusyInfo::Set(url) ) break;
	}

	// write epilogue
	ret << wxT("</trackList>") << linebreak;
	ret << wxT("</playlist>") << linebreak;

	return ret;
}


/*******************************************************************************
 * SjPlaylist - Save Dialogs & Co.
 ******************************************************************************/


bool SjPlaylist::SaveAsDlg(wxWindow* parentWindow)
{
	bool                ret;
	SjBusyInfo*         busyInfo = NULL;
	wxWindowDisabler    disabler(parentWindow);
	SjExtList           extList = g_mainFrame->m_moduleSystem.GetAssignedExt(SJ_EXT_PLAYLISTS_WRITE);

	// create the dialog
	wxFileDialog dlg(parentWindow, _("Save playlist"), wxT(""),
	                 SuggestPlaylistFileName(),
	                 extList.GetFileDlgStr(wxFD_SAVE),
	                 wxFD_SAVE|wxFD_OVERWRITE_PROMPT|wxFD_CHANGE_DIR);

	// set the last extension used
	wxString lastPlaylistFormat = g_tools->m_config->Read(wxT("main/playlistFormat"));
	long filterIndex = extList.GetFileDlgIndex(lastPlaylistFormat);
	if( filterIndex == -1 )
		filterIndex = extList.GetFileDlgIndex(wxT("m3u"));
	dlg.SetFilterIndex(filterIndex);

	// show the dialog
	if( dlg.ShowModal() != wxID_OK )
		return FALSE;

	// postprocessing
	wxString selPath, selExt;
	extList.GetFileDlgPath(dlg, selPath, selExt);
	if( selExt != lastPlaylistFormat )
		g_tools->m_config->Write(wxT("main/playlistFormat"), selExt);

	if( GetCount() > 500 )
	{
		busyInfo = new SjBusyInfo(parentWindow, _("Save playlist"));
	}

	ret = SaveAsFile(selPath, selExt, 0);

	if( busyInfo )
	{
		delete busyInfo;
	}

	return ret;
}


bool SjPlaylist::SaveAsFile(const wxString& path, const wxString& type, long flags)
{
	wxString            fileContent;
	wxMBConv*           fileContentMbConv = &wxConvUTF8;
	wxBusyCursor        busy;

	if( type == wxT("cue") )
	{
		fileContent = SaveAsCue(path, flags);
		fileContentMbConv = &wxConvISO8859_1;
	}
	else if( type == wxT("pls") )
	{
		fileContent = SaveAsPls(path, flags);
		fileContentMbConv = &wxConvISO8859_1;
	}
	else if( type == wxT("xspf") )
	{
		fileContent = SaveAsXspf(path, flags);
	}
	else /* "m3u" or "m3u8" */
	{
		fileContent = SaveAsM3u(path, flags);
		if( type != wxT("m3u8") /*else leave default*/ )
			fileContentMbConv = &wxConvISO8859_1;
	}

	// for ISO 8859-1, convert every character > 0xFF to the character "?"
	if( fileContentMbConv == &wxConvISO8859_1 )
	{
		if( SjTools::ReplaceNonISO88591Characters(fileContent) )
		{
			/*
			wxLogWarning(wxT("Because of limitations of the file format, some characters could not be written to \"%s\"."),
			    path.c_str());
			*/
		}
	}

	// do write
	wxFile file(path, wxFile::write);
	if( !file.IsOpened() )
	{
		// TRANSLATORS: %s will be replaced by a filename
		wxLogError(_("Cannot write \"%s\"."), path.c_str());
		return false;
	}

	if( fileContentMbConv == &wxConvUTF8 )
	{
		static const unsigned char utf8byteOrderMark[3] = {0xEF, 0xBB, 0xBF};
		file.Write(utf8byteOrderMark, 3);
	}

	if( !file.Write(fileContent, *fileContentMbConv) )
	{
		wxLogError(_("Cannot write \"%s\"."), path.c_str());
		return false;
	}

	return true;
}


/*******************************************************************************
 * SjPlaylist - Add Dialogs & Co.
 ******************************************************************************/


bool SjPlaylist::AddFromFileDlg(wxWindow* parentWindow)
{
	wxWindowDisabler    disabler(parentWindow);

	// create the dialog
	wxFileDialog dlg(parentWindow, _("Open playlist"), wxT(""),
	                 SuggestPlaylistFileName(),
	                 g_mainFrame->m_moduleSystem.GetAssignedExt(SJ_EXT_PLAYLISTS_READ).GetFileDlgStr(),
	                 wxFD_OPEN|wxFD_CHANGE_DIR);

	// show the dialog
	if( dlg.ShowModal() != wxID_OK )
		return FALSE;

	// do add
	return AddFromFile(dlg.GetPath());
}


bool SjPlaylist::AddFromFile(const wxString& path, long addMax, long flags)
{
	wxFileSystem fileSystem;
	wxFSFile* fsFile = fileSystem.OpenFile(path, wxFS_READ|wxFS_SEEKABLE);
	if( fsFile == NULL )
	{
		wxLogError(_("Cannot open \"%s\"."), path.c_str());
		return FALSE;
	}

	bool ret = AddFromFile(fsFile, addMax, flags);
	delete fsFile;
	return ret;
}


bool SjPlaylist::AddFromFile(wxFSFile* playlistFsFile, long addMax, long flags)
{
	wxString        ext = SjTools::GetExt(playlistFsFile->GetLocation());
	bool            ret;

	if( !g_mainFrame ) return FALSE;
	if( addMax <= 0 ) addMax = 0x7FFFFFFL;

	// load basic urls - the AddFrom*() function should not validate the files!
	if( ext == wxT("pls") )
	{
		ret = AddFromPls(playlistFsFile, addMax, flags);
	}
	else if( ext == wxT("cue") )
	{
		ret = AddFromCue(playlistFsFile, addMax, flags);
	}
	else if( ext == wxT("xspf") || ext == wxT("xml") || ext == wxT("wpl") )
	{
		ret = AddFromXspfOrXmlOrWpl(playlistFsFile, addMax, flags);
	}
	else /* "m3u", "m3u8" */
	{
		ret = AddFromM3u(playlistFsFile, addMax, flags);
	}

	wxString nativeFileName = wxFileSystem::URLToFileName(playlistFsFile->GetLocation()).GetFullPath();

	if( !ret )
	{
		wxLogError(_("Cannot open \"%s\"."), nativeFileName.c_str());
		return FALSE;
	}

	AddContainerFile(nativeFileName);

	return TRUE;
}


