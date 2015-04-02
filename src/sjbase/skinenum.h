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
 * File:    skinenum.h
 * Authors: Björn Petersen
 * Purpose: Get a list of skins available
 *
 ******************************************************************************/


#ifndef __SJ_SKINENUM_H__
#define __SJ_SKINENUM_H__


#include "skinml.h"


class SjSkinEnumeratorItem
{
public:
	wxString        m_url;
	wxString        m_name;
};
WX_DECLARE_LIST(SjSkinEnumeratorItem, SjSkinEnumeratorList);


class SjSkinEnumerator
{
public:
	SjSkinEnumerator();

	int                     GetCount        () { return m_listOfSkins.GetCount(); }
	SjSkinEnumeratorItem*   GetSkin         (int index) { return m_listOfSkins.Item(index)->GetData(); }

private:
	void                    ScanPath        (SjSkinMlParser&, const wxString& url);
	void                    ScanArchive     (SjSkinMlParser&, const wxString& url);
	void                    AddSkin         (SjSkinMlParser&, const wxString& url);
	SjSkinEnumeratorList    m_listOfSkins;

};


#endif // __SJ_SKINENUM_H__

