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
 * File:    skinml.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke skins - Skin [m]arkup [l]anguage parsing
 * OS:      independent
 *
 * (C) Björn Petersen Software Design and Development, http://b44t.com
 *
 ******************************************************************************/


#ifndef __SJ_SKINML_H__
#define __SJ_SKINML_H__


#include <wx/html/htmlwin.h>


struct SjSkinMlParserRaw
{
	const wxChar*   m_name;
	int             m_id;
};


class SjSkinMlParserData
{
public:
	SjSkinMlParserData
	() { }
	static void     FreeStaticData      ();

	// on parsing, we use the following items
	SjSkinSkin* m_skin;
	int             m_skinCount; // MUST be 1!
	SjSkinLayout*
	m_currLayout;
	SjSkinItemList
	m_itemStack;
	SjSkinItem*  GetLastItem            () { SjSkinItemList::Node* n = m_itemStack.GetLast(); return n? n->GetData() : NULL; }

	// conditions, one of the SJ_OP_* flags
	long            m_currCond;
	long            ParseCond           (const wxString&, bool& notFlag);
	bool            CheckCond           (const wxHtmlTag* tag);

	// others
	bool            m_loadNameOnly;
	bool            CheckName           (const wxString& name, const wxHtmlTag* tagPosition);
	bool            CreateNAppendItem   (const wxHtmlTag&, SjSkinItem* item);
	void            LogError            (const wxString& error, const wxHtmlTag* tagPosition);
	SjSkinImage*    LoadSkinImage       (const wxString& file, const wxHtmlTag* tagPosition);
	int             m_includeRecursion;

	static SjSLHash*
	s_targetHash;
	static SjSLHash*
	s_colourHash;

	wxString        LoadFile_(const wxString& xmlCmdFile);

	SjSLHash*       HashInit(SjSkinMlParserRaw* src);

	wxString        GetUrl      ();
};


class SjSkinMlParser : public wxHtmlParser
{
public:
	SjSkinMlParser  (SjSkinMlParserData* data, long conditions);
	~SjSkinMlParser ();
	SjSkinSkin* ParseFile           (const wxString& givenPath, bool loadNameOnly = false
	                                #if SJ_USE_SCRIPTS
	                                 , SjSee* see=NULL
									#endif
	                                );

	void            InitParser          (const wxString& source);
	void            AddText             (const wxChar* txt);
	wxObject*       GetProduct          ();

private:
	SjSkinMlParserData*
	m_data;
	bool            m_deleteData;
	friend class    SjSkinMlTagHandler;
};


#endif // __SJ_SKINML_H__

