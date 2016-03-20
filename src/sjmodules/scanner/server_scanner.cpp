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
 * File:    server_scanner.cpp
 * Authors: Björn Petersen
 * Purpose: The "server scanner" module; this format requires a very special
 *          CSV-format, sooner or later we should support UPnP AV or DLNA or
 *          sth. like that.  However, the advantage of our format is, that it
 *          does not require any special service, a simple HTTP-server will
 *          do the job.
 *
 *******************************************************************************
 *
 * Settings are stored in the database.  See library.cpp for some statistics.
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/scanner/server_scanner.h>
#include <sjmodules/scanner/server_scanner_config.h>
#include <sjtools/csv_tokenizer.h>
#include <sjtools/fs_inet.h>
#include <sjtools/msgbox.h>

// wx includes
#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArrayServerScannerSource);


/*******************************************************************************
 *  Constructor / Destructor
 ******************************************************************************/


SjServerScannerModule::SjServerScannerModule(SjInterfaceBase* interf)
	: SjScannerModule(interf)
{
	m_file                  = wxT("memory:serverscanner.lib");
	m_sort                  = 10; // start of list
	m_name                  = wxT("Server Scanner")/*n/t*/;

	m_addSourceTypes_.Add(_("HTTP server"));
	m_addSourceIcons_.Add(SJ_ICON_INTERNET_SERVER);
}


bool SjServerScannerModule::FirstLoad()
{
	LoadSettings();
	return TRUE;
}


void SjServerScannerModule::LoadSettings()
{
	m_sources.Clear();
	wxSqlt                  sql;
	int                     sourceCount = sql.ConfigRead(wxT("serverscanner/sCount"), 0L), i;
	SjServerScannerSource*  currSourceObj;
	wxString                currServerName;
	for( i = 0; i < sourceCount; i++ )
	{
		currServerName = sql.ConfigRead(wxString::Format(wxT("serverscanner/s%iname"), (int)i), wxT(""));
		if( !currServerName.IsEmpty() )
		{
			currSourceObj = new SjServerScannerSource;
			if( currSourceObj )
			{
				currSourceObj->m_serverName     = currServerName;
				currSourceObj->m_serverType     = (SjServerScannerType)sql.ConfigRead(wxString::Format(wxT("serverscanner/s%itype"), (int)i), (long)SJ_SERVERSCANNER_TYPE_UNKNOWN);
				currSourceObj->m_loginName      = sql.ConfigRead(wxString::Format(wxT("serverscanner/s%iloginName"), (int)i), wxT(""));
				currSourceObj->m_loginPassword  = SjTools::UnscrambleString(sql.ConfigRead(wxString::Format(wxT("serverscanner/s%iloginPw"), (int)i), wxT("")));
				currSourceObj->m_flags          = sql.ConfigRead(wxString::Format(wxT("serverscanner/s%iflags"), (int)i), SJ_SERVERSCANNER_DEF_FLAGS);
				currSourceObj->m_lastCfgFile    = sql.ConfigRead(wxString::Format(wxT("serverscanner/s%ilastCfgFile"), (int)i), wxT(""));

				m_sources.Add(currSourceObj);

				SjInternetFSHandler::SetAuth(currSourceObj->m_serverName, currSourceObj->m_loginName, currSourceObj->m_loginPassword);
			}
		}
	}
}


void SjServerScannerModule::SaveSettings()
{
	wxBusyCursor        busy;
	wxSqltTransaction   transaction; // for speed reasons

	{
		wxSqlt sql;

		int sourceCount = m_sources.GetCount(), i;
		sql.ConfigWrite(wxT("serverscanner/sCount"), sourceCount);
		for( i = 0; i < sourceCount; i++ )
		{
			sql.ConfigWrite(wxString::Format(wxT("serverscanner/s%iname"),        (int)i), m_sources[i].m_serverName);
			sql.ConfigWrite(wxString::Format(wxT("serverscanner/s%itype"),        (int)i), (long)m_sources[i].m_serverType);
			sql.ConfigWrite(wxString::Format(wxT("serverscanner/s%iloginName"),   (int)i), m_sources[i].m_loginName);
			sql.ConfigWrite(wxString::Format(wxT("serverscanner/s%iloginPw"),     (int)i), SjTools::ScrambleString(m_sources[i].m_loginPassword));
			sql.ConfigWrite(wxString::Format(wxT("serverscanner/s%iflags"),       (int)i), (long)m_sources[i].m_flags);
			sql.ConfigWrite(wxString::Format(wxT("serverscanner/s%ilastCfgFile"), (int)i), m_sources[i].m_lastCfgFile);

			SjInternetFSHandler::SetAuth(m_sources[i].m_serverName, m_sources[i].m_loginName, m_sources[i].m_loginPassword);
		}
	}

	transaction.Commit();
}


/*******************************************************************************
 * Adding/Removing Sources
 ******************************************************************************/


wxString SjServerScannerSource::GetPath(const wxString& path) const
{
	wxString ret;

	// set protocol and server name
	ret = wxT("http://") + m_serverName;

	// add path
	if( path[0] != '/' )
	{
		ret += wxT("/");
	}

	ret += path;

	return ret;
}


long SjServerScannerModule::AddSources(int sourceType, wxWindow* parent)
{
	// the function returns the first new index or -1 for errors or abort

	SjServerScannerSource* newSource = new SjServerScannerSource;
	SjServerScannerConfigDialog dlg(parent, *newSource);

	if(  dlg.ShowModal() != wxID_OK
	 || !dlg.GetChanges(*newSource) )
	{
		delete newSource;
		return -1; // dialog canceled, no update required
	}

	// add the new source

	m_sources.Add(newSource);
	SaveSettings();
	return m_sources.GetCount()-1;
}


bool SjServerScannerModule::DeleteSource(long index, wxWindow* parent)
{
	if( index >= 0 && index < GetSourceCount() )
	{
		if( SjMessageBox(wxString::Format(_("Remove \"%s\" from the music library?"), m_sources[index].m_serverName.c_str()),
		                   SJ_PROGRAM_NAME, wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION, parent) == wxYES )
		{
			m_sources.RemoveAt(index);
			SaveSettings();
			return TRUE;
		}
	}

	return FALSE;
}


bool SjServerScannerModule::ConfigSource(long index, wxWindow* parent)
{
	if( index >= 0 && index < GetSourceCount() )
	{
		SjServerScannerConfigDialog dlg(parent, m_sources[index]);
		if( dlg.ShowModal() == wxID_OK
		 && dlg.GetChanges(m_sources[index]) )
		{
			SaveSettings();
			return TRUE; // needs update
		}
	}

	return FALSE; // no update needed
}


/*******************************************************************************
 * Get Source Information
 ******************************************************************************/


wxString SjServerScannerModule::GetSourceNotes(long index)
{
	if( index >= 0 && index < GetSourceCount() )
	{
		if( !(m_sources[index].m_flags & SJ_SERVERSCANNER_ENABLED) )
		{
			return _("Disabled");
		}

		if( !(m_sources[index].m_flags & SJ_SERVERSCANNER_DO_UPDATE) )
		{
			return _("No update");
		}
	}

	return wxEmptyString;
}


SjIcon SjServerScannerModule::GetSourceIcon(long index)
{
	if( index >= 0 && index < GetSourceCount() )
	{
		if( m_sources[index].m_flags&SJ_SERVERSCANNER_ENABLED )
		{
			return m_sources[index].GetIcon();
		}
	}

	return SJ_ICON_EMPTY;
}


/*******************************************************************************
 * CSV over HTTP Scanning
 ******************************************************************************/


bool SjServerScannerModule::IterateCsvOverHttp(SjColModule* receiver, SjServerScannerSource* currSource,
                                               SjCfgTokenizer* options)
{
	bool            ret = FALSE;

	wxString        busyInfoStr = wxString::Format(_("Loading %s"), currSource->m_serverName.c_str());

	#define         BUFFER_MAX_BYTES 4096
	unsigned char   buffer[BUFFER_MAX_BYTES];
	long            bufferValidBytes;
	long            bufferTotalBytesRead = 0;

	wxFileSystem    fileSystem;

	wxString        csvFileName;
	wxFSFile*       csvFsFile = NULL;
	wxInputStream*  csvStream;
	SjCsvTokenizer  csvTkz(options->Lookup(wxT("csv-delim"), wxT(",")), options->Lookup(wxT("csv-enclose"), wxT("\"")), options->Lookup(wxT("csv-escape"), wxT("\"")));
	bool            csvEof = FALSE;
	bool            csvSkipFirstRow = options->Lookup(wxT("csv-skip-first-row"), wxT("0"))==wxT("1")? TRUE : FALSE;
	long            csvRecordCount = 0;

	wxArrayLong     csvTrackInfo;
	wxArrayString   csvPlaceholder;
	long            csvPlaceholderCount;

	long            fieldCount, fieldIndex;

	wxArrayString*  currRecord;

	wxString        trackInfoUrlPattern = currSource->GetPath(options->Lookup(wxT("music-files"), wxT("")));
	wxString        trackInfoImgPattern = currSource->GetPath(options->Lookup(wxT("img-files"), wxT("")));

	SjTrackInfo*    currTrackInfo;
	wxString        currTrackInfoImg;
	wxString        currPlaceholder;
	wxString        currReplacement;

	bool            diskNrInAlbumName = FALSE;

	// init the busy info
	if( !SjBusyInfo::Set(busyInfoStr, TRUE) )
	{
		goto IterateCsvOverHttp_Done;
	}

	// get the CSV column definitions
	{
		wxString str = options->Lookup(wxT("csv-columns"), wxT("")), strLower;
		str.Replace(wxT(";"), wxT(","));
		wxArrayString arr = SjTools::Explode(str, ',', 1);
		csvPlaceholderCount = arr.GetCount();
		for( fieldIndex = 0; fieldIndex < csvPlaceholderCount; fieldIndex++ )
		{
			str = arr[fieldIndex];

			     if( str == wxT("<Nr>") )           { csvTrackInfo.Add(SJ_TI_TRACKNR);          }
			else if( str == wxT("<DiskNr>") )       { csvTrackInfo.Add(SJ_TI_DISKNR);           }
			else if( str == wxT("<Title>") )        { csvTrackInfo.Add(SJ_TI_TRACKNAME);        }
			else if( str == wxT("<Artist>") )       { csvTrackInfo.Add(SJ_TI_LEADARTISTNAME);   }
			else if( str == wxT("<OrgArtist>") )    { csvTrackInfo.Add(SJ_TI_ORGARTISTNAME);    }
			else if( str == wxT("<Composer>") )     { csvTrackInfo.Add(SJ_TI_COMPOSERNAME);     }
			else if( str == wxT("<Album>") )        { csvTrackInfo.Add(SJ_TI_ALBUMNAME);        }
			else if( str == wxT("<Album(DiskNr)>") ){ csvTrackInfo.Add(SJ_TI_ALBUMNAME); str = wxT("<Album>"); diskNrInAlbumName = TRUE; }
			else if( str == wxT("<Year>") )         { csvTrackInfo.Add(SJ_TI_YEAR);             }
			else if( str == wxT("<Comment>") )      { csvTrackInfo.Add(SJ_TI_COMMENT);          }
			else if( str == wxT("<Genre>") )        { csvTrackInfo.Add(SJ_TI_GENRENAME);        }
			else if( str == wxT("<Group>") )        { csvTrackInfo.Add(SJ_TI_GROUPNAME);        }
			else                                    { csvTrackInfo.Add(0);                      }

			     if( str[0] == wxT('<') )           { csvPlaceholder.Add(str);                  }
			else                                    { csvPlaceholder.Add(wxT(""));              }

		}


	}

	// open the CSV file
	csvFileName =
	    currSource->GetPath(options->Lookup(wxT("csv-file"), wxT("")))
	    //"http://me:dummy@localhost/silverjuke/music/media/servlet/secure/text/plain/jukeboxwk41.csv"
	    //"http://silverjuke:123456@music.silverjuke.net/media/servlet/secure/text/plain/jukeboxwk41.csv"
	    ;

	g_tools->m_cache.RemoveFromCache(csvFileName, TRUE/*never cache again*/);

	csvFsFile = fileSystem.OpenFile(csvFileName);
	if( csvFsFile == NULL )
	{
		wxLogError(_("Cannot open \"%s\"."), currSource->m_serverName.c_str());
		goto IterateCsvOverHttp_Done;
	}

	// read the CSV file
	csvStream = csvFsFile->GetStream();
	while( !csvEof )
	{
		// read the next data block ...
		csvStream->Read(buffer, BUFFER_MAX_BYTES);
		bufferValidBytes = csvStream->LastRead();
		bufferTotalBytesRead += bufferValidBytes;

		csvTkz.AddData(buffer, bufferValidBytes);
		if( bufferValidBytes <= 0 || csvStream->Eof() )
		{
			csvTkz.AddData((unsigned char*)"\n", 1);
			csvEof = TRUE;
		}

		// ...see what's in the data block
		while( (currRecord=csvTkz.GetRecord()) != NULL )
		{
			// skip first record, if needed
			csvRecordCount++;
			if( csvSkipFirstRow && csvRecordCount==1 )
			{
				continue;
			}

			// add this record
			currTrackInfo = new SjTrackInfo;
			currTrackInfo->m_url = trackInfoUrlPattern;
			currTrackInfoImg = trackInfoImgPattern;

			fieldCount = currRecord->GetCount();
			wxASSERT( csvPlaceholderCount == (long)csvTrackInfo.GetCount() );
			if( fieldCount > csvPlaceholderCount ) fieldCount = csvPlaceholderCount;
			for( fieldIndex = 0; fieldIndex < fieldCount; fieldIndex++ )
			{
				currPlaceholder = csvPlaceholder[fieldIndex];

				currReplacement = currRecord->Item(fieldIndex);
				currReplacement.Trim(TRUE);
				currReplacement.Trim(FALSE);

				if( csvTrackInfo[fieldIndex] )
				{
					currTrackInfo->SetValue(csvTrackInfo[fieldIndex], currReplacement);
				}

				if( !currPlaceholder.IsEmpty() )
				{
					currTrackInfo->m_url.Replace(currPlaceholder, currReplacement);
					currTrackInfoImg.Replace(currPlaceholder, currReplacement);

					if( currPlaceholder == wxT("<Nr>") )
					{
						currPlaceholder = wxT("<Nr(2)>");
						if( currReplacement.Len() == 1 ) currReplacement.Prepend(wxT("0"));

						currTrackInfo->m_url.Replace(currPlaceholder, currReplacement);
						currTrackInfoImg.Replace(currPlaceholder, currReplacement);
					}
				}
			}

			if( diskNrInAlbumName )
			{
				int p1=currTrackInfo->m_albumName.Find(' ', TRUE),
				    p2=currTrackInfo->m_albumName.Find('(', TRUE);
				if( p1!=-1 || p2!=-1 )
				{
					if( p2>p1 ) p1=p2;
					if( p1 > 0 )
					{
						wxString suffix = currTrackInfo->m_albumName.Mid(p1+1).Lower();
						suffix.Trim(FALSE);

						if( SjTools::StripPrefix(suffix, wxT("cd"))
						 || SjTools::StripPrefix(suffix, wxT("disk"))
						 || SjTools::StripPrefix(suffix, wxT("disc")) )
						{
							suffix.Trim(FALSE);
							suffix.Replace(wxT("#"), wxT(""), FALSE/*only the first*/);
							if( p2>p1 ) suffix.Replace(wxT(")"), wxT(""), FALSE/*only the first*/);
							if( suffix.IsNumber() && suffix.Len()<=2 )
							{
								long l=0;
								if( suffix.ToLong(&l, 10)
								        && l > 0 )
								{
									currTrackInfo->m_diskNr = l;
									currTrackInfo->m_albumName = currTrackInfo->m_albumName.Left(p1).Trim();
								}
							}
						}
					}
				}
			}

			if( currTrackInfo->m_trackName.IsEmpty() )
			{
				currTrackInfo->m_trackName = wxString::Format(wxT("Track #%i"), (int)csvRecordCount);
			}

			if( !currTrackInfoImg.IsEmpty() )
			{
				currTrackInfo->AddArt(currTrackInfoImg);
			}

			if( !receiver->Callback_ReceiveTrackInfo(currTrackInfo) )
			{
				goto IterateCsvOverHttp_Done; // user abort
			}

			// progress information
			if( !SjBusyInfo::Set(busyInfoStr+wxT(" (")+SjTools::FormatBytes(bufferTotalBytesRead)+wxT(")"), FALSE) )
			{
				goto IterateCsvOverHttp_Done;
			}
		}

		// progress information
		if( !SjBusyInfo::Set(wxT(""), FALSE) )
		{
			goto IterateCsvOverHttp_Done;
		}
	}

	// final check if we got any data - this is needed as OpenFile() does not return
	// FALSE if the stream is not available.
	if( bufferTotalBytesRead == 0 )
	{
		wxLogError(_("Cannot open \"%s\"."), currSource->m_serverName.c_str());
		goto IterateCsvOverHttp_Done;
	}

	// done so far
	ret = TRUE;

IterateCsvOverHttp_Done:

	if( csvFsFile )
	{
		delete csvFsFile;
	}

	return ret;
}


/*******************************************************************************
 *  Common Scanning
 ******************************************************************************/


bool SjServerScannerModule::IterateHttp(SjColModule* receiver, SjServerScannerSource* currSource,
                                        bool& retSaveSettings)
{
	SjCfgTokenizer  options;
	wxString        currCfgFile;

	{
		wxLogNull       null;
		wxFileSystem    fileSystem;
		wxString        currTryFileName;
		wxFSFile*       currTryFsFile;

		// get normalized server name
		wxString normServerName = currSource->m_serverName.Lower();
		normServerName.Replace(wxT("."), wxT("-"));
		normServerName.Replace(wxT("/"), wxT("-"));
		normServerName += wxT(".txt");

		// lookup options
		int searchPathIndex, searchPathCount = g_tools->GetSearchPathCount();
		for( searchPathIndex=0; searchPathIndex<searchPathCount + 1/*check server*/; searchPathIndex++ )
		{
			if( searchPathIndex < searchPathCount )
			{
				// first, check all local search paths
				wxFileName fn(g_tools->GetSearchPath(searchPathIndex), normServerName);
				fn.Normalize();
				currTryFileName = fn.GetFullPath();
			}
			else
			{
				// then, check the remove server configuration
				currTryFileName = currSource->GetPath(wxT("music-lib-cfg.txt"));
				g_tools->m_cache.RemoveFromCache(currTryFileName, TRUE/*never cache again*/);
			}

			currTryFsFile = fileSystem.OpenFile(currTryFileName);
			if( currTryFsFile != NULL )
			{
				options.AddFromStream(currTryFsFile->GetStream());
				delete currTryFsFile;
			}

			if( options.Lookup(wxT("access-type"), wxT("")) != wxT("") )
			{
				// success! configuration found!
				currCfgFile = currTryFileName;
				break;
			}
			else
			{
				// continue searching the configuration
				currCfgFile.Clear();
				options.Clear();
			}
		}
	}


	if( currSource->m_lastCfgFile != currCfgFile )
	{
		currSource->m_lastCfgFile = currCfgFile;
		retSaveSettings = true;
	}


	if( options.Lookup(wxT("access-type"), wxT("")) == wxT("csv-over-http") )
	{
		return IterateCsvOverHttp(receiver, currSource, &options);
	}
	else
	{
		wxLogError(wxT("Cannot find configuration/invalid configuration."));
		wxLogError(_("Cannot open \"%s\"."), currSource->m_serverName.c_str());
		return FALSE;
	}
}


bool SjServerScannerModule::IterateTrackInfo(SjColModule* receiver)
{
	bool                    ret = true, doSaveSettings = false;
	int                     sourceCount = GetSourceCount(), s;
	SjServerScannerSource*  currSource;

	// go through all sources
	for( s = 0; s < sourceCount; s++ )
	{
		// get source
		currSource = &(m_sources.Item(s));

		// source enabled?
		if( !(currSource->m_flags & SJ_SERVERSCANNER_ENABLED) )
		{
			continue;
		}

		// read data
		if( currSource->m_serverType == SJ_SERVERSCANNER_TYPE_HTTP )
		{
			if( !IterateHttp(receiver, currSource, doSaveSettings) )
			{
				ret = false;
				break; // aborted
			}
		}
	}

	if( ret && doSaveSettings )
	{
		SaveSettings();
	}

	return ret;
}


bool SjServerScannerModule::IsMyUrl(const wxString& url)
{
	return FALSE;
}




