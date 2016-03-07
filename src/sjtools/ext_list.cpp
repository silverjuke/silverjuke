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
 * File:    tools.cpp
 * Authors: Björn Petersen
 * Purpose: Silverjuke tools
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <wx/tokenzr.h>
#include <sjtools/ext_list.h>


wxArrayString   SjExtList::s_repositoryExt;
wxArrayString   SjExtList::s_repositoryDescr;


wxArrayString SjExtList::String2Array(const wxString& str__)
{
	wxArrayString ret;
	wxString str(str__);

	str.Replace(";", ",");
	str.Replace(" ", ",");
	str.Replace("/", ",");
	str.Replace("\\",",");
	str.Replace("?", "");
	str.Replace("*", "");
	str.Replace(".", "");
	str.Replace("(", "");
	str.Replace(")", "");
	str.Replace("[", "");
	str.Replace("]", "");
	str.Replace("\"","");
	str.Replace("'", "");
	str.MakeLower();

	wxStringTokenizer tkz(str, ",");
	wxString curr;
	while( tkz.HasMoreTokens() )
	{
		curr = tkz.GetNextToken();
		curr.Trim(TRUE/*from right*/);
		curr.Trim(FALSE/*from left*/);
		if( !curr.IsEmpty() )
		{
			ret.Add(curr);
		}
	}
	ret.Sort();
	return ret;
}


wxString SjExtList::Array2String(const wxArrayString& array, int maxExtToReturn)
{
	wxString ret;
	size_t i, count = array.GetCount();
	for( i = 0; i < count; i++, maxExtToReturn-- )
	{
		if( i )
		{
			ret += ", ";
		}

		if( maxExtToReturn == 0 )
		{
			ret += "...";
			break;
		}
		else
		{
			ret += array.Item(i);
		}
	}
	return ret;
}


void SjExtList::AddArray(const wxArrayString& other)
{
	size_t o, oCount = other.GetCount();
	for( o = 0; o < oCount; o++ )
	{
		if( m_ext.Index(other.Item(o)) == wxNOT_FOUND )
		{
			m_ext.Add(other.Item(o));
		}
	}
	m_ext.Sort();
}


void SjExtList::SubArray(const wxArrayString& other)
{
	size_t o, oCount = other.GetCount();
	for( o = 0; o < oCount; o++ )
	{
		int selfIndex = m_ext.Index(other.Item(o));
		if( selfIndex != wxNOT_FOUND )
		{
			m_ext.RemoveAt(selfIndex);
		}
	}
	m_ext.Sort();
	m_hash.Clear(); // we rebuild the hash if the number of item differs. as long as we always add, this works. If we sub, we have to invalidate as we may come to the same count.
}

void SjExtList::AddExt00(const char* p)
{
	bool isExtensionNode = TRUE;
	while( *p )
	{
		if( isExtensionNode )
		{
			#if wxUSE_UNICODE
				AddExt(wxString(p, wxConvUTF8));
			#else
				AddExt(p);
			#endif
		}

		isExtensionNode = !isExtensionNode;
		p = &p[strlen(p)+1];
	}
}


bool SjExtList::LookupExt(const wxString& ext) const
{
	if( m_hash.GetCount()!=(long)m_ext.GetCount() )
	{
		m_hash.Clear();
		long i, iCount = m_ext.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			m_hash.Insert(m_ext.Item(i), 1);
		}
	}

	return m_hash.Lookup(ext.Lower())!=0;
}


void SjExtList::RepositoryInit()
{
	static bool s_repositoryInitialized = FALSE;
	if( !s_repositoryInitialized )
	{
		s_repositoryInitialized = TRUE;

		// the list does not describe all supported types;
		// the list gives only names to types that _may_ be supported.
		//
		// do not merge different file types (okay, let's say _too_ different file types),
		// as we use these descriptions also to let the user select a file format on save.
		RepositoryAdd("3g2 3gp",                       wxString::Format(_("%s-video"), "3GPP"));
		RepositoryAdd("aac",                           wxString::Format(_("%s-audio"), "AAC"));
		RepositoryAdd("ac3",                           wxString::Format(_("%s-audio"), "AC-3"));
		RepositoryAdd("aif aiff",                      wxString::Format(_("%s-audio"), "AIFF"));
		RepositoryAdd("ani bmp cur dib ico rle",       wxString::Format(_("%s-images"),"BMP"));
		RepositoryAdd("ape apl mac",                   "Monkey's Audio");
		RepositoryAdd("au snd",                        wxString::Format(_("%s-audio"), "Sun/NeXT"));
		RepositoryAdd("avi",                           wxString::Format(_("%s-video"), "AVI"));
		RepositoryAdd("cue",                           "Cue Sheets");
		RepositoryAdd("dts",                           wxString::Format(_("%s-audio"), "DTS"));
		RepositoryAdd("dv",                            "Digital Video");
		RepositoryAdd("eq",                            "Shibatch SuperEQ Preset");
		RepositoryAdd("eqf q2",                        "Winamp Equalizer Preset");
		RepositoryAdd("feq",                           "Foobar Equalizer Preset");
		RepositoryAdd("fla flac",                      wxString::Format(_("%s-audio"), "FLAC"));
		RepositoryAdd("flv f4a f4v f4p",               wxString::Format(_("%s-video"), "Flash"));
		RepositoryAdd("fx-eq",                         "Sj2 Equalizer Preset");
		RepositoryAdd("gif",                           wxString::Format(_("%s-images"),"GIF"));
		RepositoryAdd("ham ham6 ham8",                 wxString::Format(_("%s-images"),"HAM"));
		RepositoryAdd("iff",                           wxString::Format(_("%s-images"),"IFF"));
		RepositoryAdd("jpeg jpe jpg jif",              wxString::Format(_("%s-images"),"JPEG"));
		RepositoryAdd("m3u",                           wxString::Format(_("%s-playlists"), "M3U"));
		RepositoryAdd("m3u8",                          wxString::Format(_("%s-playlists"), "M3U8/Unicode"));
		RepositoryAdd("mid midi miz rmi kar",          wxString::Format(_("%s-files"), "MIDI"));
		RepositoryAdd("mjpg",                          "Motion JPEG");
		RepositoryAdd("mkv mka mks mk3d",              wxString::Format(_("%s-files"), "Matroska"));
		RepositoryAdd("mov qt qtl",                    wxString::Format(_("%s-video"), "QuickTime"));
		RepositoryAdd("mp1 mp2 mp3 mpa mpga",          wxString::Format(_("%s-audio"), "MP3"));
		RepositoryAdd("mpg mpeg mpe m2v mpv",          wxString::Format(_("%s-video"), "MPEG"));
		RepositoryAdd("mp4 m4a m4v m4p m4b",           wxString::Format(_("%s-files"), "MP4"));
		RepositoryAdd("mpc mpp mp+",                   wxString::Format(_("%s-audio"), "Musepack/MPEGplus"));
		RepositoryAdd("nsv nsa",                       wxString::Format(_("%s-files"), "Nullsoft"));
		RepositoryAdd("ogg oga ogm ogv ogx",           "Ogg-Vorbis"); // skip "-files" to shorten the longest strings in the extension list
		RepositoryAdd("opus",                          wxString::Format(_("%s-audio"), "Opus"));
		RepositoryAdd("ofr",                           wxString::Format(_("%s-audio"), "OptimFrog"));
		RepositoryAdd("pls",                           wxString::Format(_("%s-playlists"), "PLS"));
		RepositoryAdd("png mng",                       wxString::Format(_("%s-files"), "PNG"));
		RepositoryAdd("pva",                           wxString::Format(_("%s-video"), "Hauppauge"));
		RepositoryAdd("ra rm rv rmvb rmj rms rmm rax rvx rp rtx rt, mx", "RealAudio/RealVideo");
		RepositoryAdd("ram",                           wxString::Format(_("%s-playlists"), "RAM"));
		RepositoryAdd("js",                            _("Script-files"));
		RepositoryAdd("sjs",                           _("Silverjuke skin-files"));
		RepositoryAdd("jukebox",                       _("Silverjuke jukebox-files"));
		RepositoryAdd("shn",                           wxString::Format(_("%s-audio"), "Shorten"));
		RepositoryAdd("spx",                           wxString::Format(_("%s-audio"), "Speex"));
		RepositoryAdd("tif tiff",                      wxString::Format(_("%s-images"), "TIFF"));
		RepositoryAdd("ts trp m2t m2ts mts",           "Transport Stream");
		RepositoryAdd("tta",                           "TrueAudio");
		RepositoryAdd("txt",                           wxString::Format(_("%s-files"), "Text"));
		RepositoryAdd("vob",                           "DVD Video");
		RepositoryAdd("wav",                           wxString::Format(_("%s-audio"), "Wave"));
		RepositoryAdd("webm",                          wxString::Format(_("%s-video"), "WebM"));
		RepositoryAdd("wma wmv wm asf asx wax wmx",    "Windows Media"); // skip "-files" to shorten the longest strings in the extension list
		RepositoryAdd("wpl wvx",                       wxString::Format(_("%s-playlists"), "Windows Media"));
		RepositoryAdd("wv",                            wxString::Format(_("%s-audio"), "WavPack"));
		RepositoryAdd("xml",                           wxString::Format(_("%s-playlists"), "XML"));
		RepositoryAdd("xspf",                          wxString::Format(_("%s-playlists"), "XSPF"));
		RepositoryAdd("zip rar tar gz tgz",            _("Archive-files"));
	}

	wxASSERT( s_repositoryExt.GetCount() == s_repositoryDescr.GetCount() );
}


void SjExtList::RepositoryAdd(const wxString& extStr, const wxString& extDescr)
{
	RepositoryInit();
	if( !extStr.IsEmpty() )
	{
		wxArrayString extArray = String2Array(extStr);
		int i, iCount = extArray.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			wxASSERT( s_repositoryExt.Index(extArray[i]) == wxNOT_FOUND );

			s_repositoryExt.Add(extArray[i]);
			s_repositoryDescr.Add(extDescr);
		}
	}
}


wxString SjExtList::RepositoryDescr(const wxString& ext)
{
	RepositoryInit();
	int index = s_repositoryExt.Index(ext);
	if( index == wxNOT_FOUND )
	{
		return wxString::Format(_("%s-files"), ext.Upper().c_str());
	}
	else
	{
		return s_repositoryDescr.Item(index);
	}
}


wxString SjExtList::GetFileDlgStr(long flags)
{
	int             i, iCount;
	wxArrayString   allDescr;
	wxString        supportedExt;

	// group extensions by their descriptions
	{
		wxArrayString allExt;
		iCount = (int)m_ext.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			wxString currDescr = RepositoryDescr(m_ext[i]);
			int      currIndex = allDescr.Index(currDescr);
			if( currIndex == wxNOT_FOUND )
			{
				allDescr.Add(currDescr);
				allExt.Add("*."+m_ext[i]);
			}
			else
			{
				allExt[currIndex] += ";*."+m_ext[i];
			}

			if( i ) supportedExt += ";";
			supportedExt += "*."+m_ext[i];
		}

		// add extensions after descriptions and sort them
		iCount = allDescr.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			allDescr[i] += "\n" + allExt[i];
		}

		allDescr.Sort();
	}

	// build return string
	wxString ret;
	m_extFileDialog.Clear();

	if( !(flags & wxFD_SAVE) )
	{
		// KNOWN BUG: GetOpenFileName()/GetSaveFileName() does not accept
		// more than 256 characters in a single filter (tested on Windows 2K);
		// if a filter is longer, "*.*" is used.  I don't know a work around --
		// the supported extensions get easily longer than 256 bytes.

		ret << _("Known file-types") << "|" << supportedExt << "|";
		m_extFileDialog.Add("");
	}

	iCount = allDescr.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		wxString currDescr = allDescr[i].BeforeFirst('\n');
		wxString currExt = allDescr[i].AfterFirst('\n');
		ret += currDescr + " (" + currExt+ ")" "|" + currExt + "|";

		m_extFileDialog.Add(currExt);
	}

	if( !(flags & wxFD_SAVE) )
	{
		ret << _("All files") << " (*.*)|*.*";
		m_extFileDialog.Add("");
	}
	else
	{
		ret.Truncate(ret.Len()-1); // remove last pipe
	}

	return ret;
}


long SjExtList::GetFileDlgIndex(const wxString& ext)
{
	int         i, iCount = m_extFileDialog.GetCount();
	SjExtList   temp;
	for( i = 0; i < iCount; i++ )
	{
		temp.SetExt(m_extFileDialog[i]);
		if( temp.LookupExt(ext) )
		{
			return i;
		}
	}
	return -1; // not found
}


wxString SjExtList::GetFileDlgExt(long index)
{
	if( index >= 0 && index < (long)m_extFileDialog.GetCount() )
	{
		SjExtList temp(m_extFileDialog[index]);
		if( temp.GetCount() )
		{
			return temp.GetExt(0);
		}
	}

	return "";
}


void SjExtList::GetFileDlgPath(wxFileDialog& fileDialog, wxString& selPath, wxString& selExt)
{
	// get selected path
	selPath = fileDialog.GetPath();

	// normalize path - I think this is not neede; moreover, this converts the filename to lower case
	/*wxFileName fn(selPath);
	fn.Normalize();
	selPath = fn.GetFullPath();*/

	// add extension if not given by the user - or -
	// find out the selected filter if the extension given by the user s unknown
	selExt = SjTools::GetExt(selPath);
	if( selExt.IsEmpty() )
	{
		selExt = GetFileDlgExt(fileDialog.GetFilterIndex());
		if( !selExt.IsEmpty() )
		{
			selPath << "." << selExt;
		}
	}
	else if( !LookupExt(selExt) )
	{
		selExt = GetFileDlgExt(fileDialog.GetFilterIndex());
	}
}


bool SjExtList::HasIntersectionWith(const SjExtList& o) const
{
	int i, iCount = m_ext.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		if( o.LookupExt(m_ext[i]) )
		{
			return TRUE;
		}
	}
	return FALSE;
}
