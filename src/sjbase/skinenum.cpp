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
 * File:    skinenum.h
 * Authors: Björn Petersen
 * Purpose: Get a list of skins available
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjbase/skinenum.h>


#include <wx/listimpl.cpp>
WX_DEFINE_LIST(SjSkinEnumeratorList);


SjSkinEnumerator::SjSkinEnumerator()
{
	// DeleteContents() tells the list that all objects should be deleted after usage of the list.  Very handy.
	m_listOfSkins.DeleteContents(TRUE);

	// add all skins
	SjSkinMlParser parser(NULL, 0);
	AddSkin(parser, wxT("memory:defaultskin.xml"));

	int currSearchDirIndex;
	for( currSearchDirIndex = 0; currSearchDirIndex < g_tools->GetSearchPathCount(); currSearchDirIndex++ )
	{
		ScanPath(parser, g_tools->GetSearchPath(currSearchDirIndex));
	}

	// add URL to duplicate names
	SjSkinEnumeratorList::Node *node1 = m_listOfSkins.GetFirst(), *node2;
	SjSkinEnumeratorItem *item1, *item2;
	bool name1Ok;
	while( node1 )
	{
		item1 = node1->GetData();
		name1Ok = TRUE;
		wxASSERT(item1);

		node2 = node1->GetNext();
		while( node2 )
		{
			item2 = node2->GetData();
			wxASSERT(item2);

			if( item1->m_name.CmpNoCase(item2->m_name) == 0 )
			{
				item2->m_name += wxT(" (") + item2->m_url + wxT(")");
				name1Ok = FALSE;
			}

			node2 = node2->GetNext();
		}

		if( !name1Ok )
		{
			item1->m_name += wxT(" (") + item1->m_url + wxT(")");
		}

		node1 = node1->GetNext();
	}
}


void SjSkinEnumerator::ScanPath(SjSkinMlParser& parser, const wxString& url)
{
	wxArrayString   possibleSkins;
	wxArrayString   subdirs;
	size_t          e;

	// read directory
	if( wxDirExists(url) )
	{
		wxFileSystem fs;
		fs.ChangePathTo(url, TRUE);
		wxString entryStr = fs.FindFirst(wxT("*.*"));
		while( !entryStr.IsEmpty() )
		{
			if( SjTools::GetExt(entryStr) == wxT("sjs")
			 || SjTools::GetExt(entryStr) == wxT("zip") )
			{
				possibleSkins.Add(::wxDirExists(entryStr)? entryStr : (entryStr + wxT("#zip:")));
			}
			else if( ::wxDirExists(entryStr) )
			{
				subdirs.Add(entryStr);
			}

			entryStr = fs.FindNext();
		}
	}

	// recurse into some special subdirectories.
	// we don't recurse all directories for speed reasons
	// as the program path is also in the search path and we don't know
	// where it is and how many thousands file we have to scan.
	for( e = 0; e < subdirs.GetCount(); e++ )
	{
		wxString lastDir = g_tools->GetLastDir(subdirs.Item(e)).Lower();
		if( lastDir == wxT("skins") )
		{
			ScanPath(parser, subdirs.Item(e));
		}
	}

	// add skins
	for( e = 0; e < possibleSkins.GetCount(); e++ )
	{
		ScanArchive(parser, possibleSkins.Item(e));
	}
}


void SjSkinEnumerator::ScanArchive(SjSkinMlParser& parser, const wxString& url)
{
	wxLogNull logNull;
	wxFileSystem fs;
	fs.ChangePathTo(url, TRUE);
	wxString entryStr = fs.FindFirst(wxT("*.xml"));
	while( !entryStr.IsEmpty() )
	{
		AddSkin(parser, entryStr);
		entryStr = fs.FindNext();
	}
}


void SjSkinEnumerator::AddSkin(SjSkinMlParser& parser, const wxString& url)
{
	SjSkinSkin* skin = parser.ParseFile(url, true /*load name only*/);
	if( skin )
	{
		SjSkinEnumeratorItem* item = new SjSkinEnumeratorItem();
		if( item )
		{
			item->m_name        = skin->GetName();
			item->m_url         = url;
			SjBusyInfo::Set(item->m_url, TRUE);
			m_listOfSkins.Append(item);
		}

		delete skin;
	}
}

