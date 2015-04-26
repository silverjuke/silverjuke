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
wxArrayLong     SjExtList::s_repositoryType;
wxArrayString   SjExtList::s_repositoryDescr;


wxArrayString SjExtList::String2Array(const wxString& str__)
{
	wxArrayString ret;
	wxString str(str__);

	str.Replace(wxT(";"), wxT(","));
	str.Replace(wxT(" "), wxT(","));
	str.Replace(wxT("/"), wxT(","));
	str.Replace(wxT("\\"),wxT(","));
	str.Replace(wxT("?"), wxT(""));
	str.Replace(wxT("*"), wxT(""));
	str.Replace(wxT("."), wxT(""));
	str.Replace(wxT("("), wxT(""));
	str.Replace(wxT(")"), wxT(""));
	str.Replace(wxT("["), wxT(""));
	str.Replace(wxT("]"), wxT(""));
	str.Replace(wxT("\""),wxT(""));
	str.Replace(wxT("'"), wxT(""));
	str.MakeLower();

	wxStringTokenizer tkz(str, wxT(","));
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
			ret += wxT(", ");
		}

		if( maxExtToReturn == 0 )
		{
			ret += wxT("...");
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
	size_t i, count = other.GetCount();
	for( i = 0; i < count; i++ )
	{
		if( m_ext.Index(other.Item(i)) == wxNOT_FOUND )
		{
			m_ext.Add(other.Item(i));
		}
	}
	m_ext.Sort();
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
		// the list gives only names to types that MAY be supported.
		//
		// do not merge different file types (okay, let's say _too_ different file types),
		// as we use these descriptions also to let the user select a file format on save.
		//
		// moreover, this list is also used by some players to filter supported, playable files
		// eg. from text and image files that are also supported, but not useful in this sense for silverjuke
		// (more concrete, we filter eg. xine_get_file_extensions() using this repository)
		RepositoryAdd(wxT("669 amf bp bp3 cus cust dm2 far ")
		              wxT("fc fc13 fc14 fc3 fc4 hip it itz mdz mo3 ")
		              wxT("mod mon mtm nst okt ptm s3m s3z sfx sid1 ")
		              wxT("stm stz ult umx xm xmz"),                  SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-files"), wxT("MOD")));
		RepositoryAdd(wxT("aif aiff"),                      SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-audio"), wxT("AIFF")));
		RepositoryAdd(wxT("ani bmp cur dib ico rle"),       SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxString::Format(_("%s-images"),wxT("BMP")));
		RepositoryAdd(wxT("ape apl mac"),                   SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-audio"), wxT("Monkey")));
		RepositoryAdd(wxT("avi"),                           SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-video"), wxT("AVI")));
		RepositoryAdd(wxT("cue"),                           SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxT("Cue Sheets"));
		RepositoryAdd(wxT("fla flac"),                      SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-audio"), wxT("FLAC")));
		RepositoryAdd(wxT("gif"),                           SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxString::Format(_("%s-images"),wxT("GIF")));
		RepositoryAdd(wxT("ham ham6 ham8"),                 SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxString::Format(_("%s-images"),wxT("HAM")));
		RepositoryAdd(wxT("iff"),                           SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxString::Format(_("%s-images"),wxT("IFF")));
		RepositoryAdd(wxT("jpeg jpe jpg jif"),              SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxString::Format(_("%s-images"),wxT("JPEG")));
		RepositoryAdd(wxT("m3u"),                           SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxString::Format(_("%s-playlists"), wxT("M3U")));
		RepositoryAdd(wxT("m3u8"),                          SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxString::Format(_("%s-playlists"), wxT("M3U8/Unicode")));
		RepositoryAdd(wxT("mid midi miz rmi kar"),          SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-files"), wxT("MIDI")));
		RepositoryAdd(wxT("mov qt"),                        SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-video"), wxT("QuickTime")));
		RepositoryAdd(wxT("mp1 mp2 mp3 mpa mpga"),          SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-audio"), wxT("MPEG")));
		RepositoryAdd(wxT("mpg mpeg m2v"),                  SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-video"), wxT("MPEG")));
		RepositoryAdd(wxT("mp4 m4a"),                       SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-audio"), wxT("MP4")));
		RepositoryAdd(wxT("mpc mpp mp+"),                   SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-audio"), wxT("Musepack/MPEGplus")));
		RepositoryAdd(wxT("nsv nsa"),                       SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-audio"), wxT("Nullsoft")));
		RepositoryAdd(wxT("ogg"),                           SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-audio"), wxT("Ogg-Vorbis")));
		RepositoryAdd(wxT("pls"),                           SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxString::Format(_("%s-playlists"), wxT("PLS")));
		RepositoryAdd(wxT("png"),                           SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxString::Format(_("%s-files"), wxT("PNG")));
		RepositoryAdd(wxT("ra rm rv rmvb rmj rms ")
		              wxT("rmm rax rvx rp rtx rt, mx"),     SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-audio"), wxT("Real")));
		RepositoryAdd(wxT("ram"),                           SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxString::Format(_("%s-playlists"), wxT("RAM")));
		RepositoryAdd(wxT("js"),                            SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   _("Script-files"));
		RepositoryAdd(wxT("sjs"),                           SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   _("Silverjuke skin-files"));
		RepositoryAdd(wxT("jukebox"),                       SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   _("Silverjuke jukebox-files"));
		RepositoryAdd(wxT("spx"),                           SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-audio"), wxT("Speex")));
		RepositoryAdd(wxT("tif tiff"),                      SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxString::Format(_("%s-images"), wxT("TIFF")));
		RepositoryAdd(wxT("tta"),                           SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-files"), wxT("TrueAudio")));
		RepositoryAdd(wxT("txt"),                           SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxString::Format(_("%s-files"), wxT("Text")));
		RepositoryAdd(wxT("wma wmv wm asf asx wax wvx wmx"),SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-files"), wxT("Windows Media")));
		RepositoryAdd(wxT("wpl"),                           SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxString::Format(_("%s-playlists"), wxT("Windows Media")));
		RepositoryAdd(wxT("wv"),                            SJ_EXT_TYPE_PLAYABLE,           wxString::Format(_("%s-audio"), wxT("WavPack")));
		RepositoryAdd(wxT("xml"),                           SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxString::Format(_("%s-playlists"), wxT("XML")));
		RepositoryAdd(wxT("xspf"),                          SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   wxString::Format(_("%s-playlists"), wxT("XSPF")));
		RepositoryAdd(wxT("zip rar tar gz tgz"),            SJ_EXT_TYPE_KNOWN_UNPLAYABLE,   _("Archive-files"));
	}

	wxASSERT( s_repositoryExt.GetCount() == s_repositoryDescr.GetCount() );
	wxASSERT( s_repositoryExt.GetCount() == s_repositoryType.GetCount() );
}


void SjExtList::RepositoryAdd(const wxString& extStr, long extType, const wxString& extDescr)
{
	RepositoryInit();
	if( !extStr.IsEmpty() )
	{
		wxArrayString extArray = String2Array(extStr);
		int i, iCount = extArray.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			if( s_repositoryExt.Index(extArray[i]) == wxNOT_FOUND )
			{
				s_repositoryExt.Add(extArray[i]);
				s_repositoryType.Add(extType);
				s_repositoryDescr.Add(extDescr);
			}
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


long SjExtList::RepositoryType(const wxString& ext)
{
	RepositoryInit();
	int index = s_repositoryExt.Index(ext);
	if( index == wxNOT_FOUND )
	{
		return SJ_EXT_TYPE_UNKNOWN;
	}
	else
	{
		return s_repositoryType.Item(index);
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
				allExt.Add(wxT("*.")+m_ext[i]);
			}
			else
			{
				allExt[currIndex] += wxT(";*.")+m_ext[i];
			}

			if( i ) supportedExt += wxT(";");
			supportedExt += wxT("*.")+m_ext[i];
		}

		// add extensions after descriptions and sort them
		iCount = allDescr.GetCount();
		for( i = 0; i < iCount; i++ )
		{
			allDescr[i] += wxT("\n") + allExt[i];
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

		ret << _("Supported file-types") << wxT("|") << supportedExt << wxT("|");
		m_extFileDialog.Add(wxT(""));
	}

	iCount = allDescr.GetCount();
	for( i = 0; i < iCount; i++ )
	{
		wxString currDescr = allDescr[i].BeforeFirst('\n');
		wxString currExt = allDescr[i].AfterFirst('\n');
		ret += currDescr + wxT(" (") + currExt+ wxT(")") wxT("|") + currExt + wxT("|");

		m_extFileDialog.Add(currExt);
	}

	if( !(flags & wxFD_SAVE) )
	{
		ret << _("All files") << wxT(" (*.*)|*.*");
		m_extFileDialog.Add(wxT(""));
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

	return wxT("");
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
			selPath << wxT(".") << selExt;
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
