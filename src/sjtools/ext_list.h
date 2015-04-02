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
 * File:    ext_list.h
 * Authors: Björn Petersen
 * Purpose: Handling extensions
 *
 ******************************************************************************/


#ifndef __SJ_EXT_LIST_H__
#define __SJ_EXT_LIST_H__


class SjExtList
{
public:
	SjExtList       ()                          { }
	SjExtList       (const wxString& ext)       { SetExt(ext); }
	SjExtList       (const SjExtList& o)        { m_ext = o.m_ext; }
	SjExtList       (const SjExtList* o)        { if(o) { m_ext = o->m_ext; } }

	void            Clear           ()                          { m_hash.Clear(); m_ext.Clear(); }

	void            SetExt          (const wxString& ext)       { Clear(); AddExt(ext); }
	void            SetExt00        (const char* p)             { Clear(); AddExt00(p); }
	void            AddExt          (const wxString& ext)       { AddArray(String2Array(ext)); }
	void            AddExt          (const SjExtList& o)        { AddArray(o.m_ext); }
	void            AddExt          (const SjExtList* o)        { if(o) { AddArray(o->m_ext); } }
	void            AddExt00        (const char*);
	int             GetCount        () const                    { return (int)m_ext.GetCount(); }
	wxString        GetExt          (int i) const               { return m_ext.Item(i); }
	wxString        GetExt          () const                    { return Array2String(m_ext, -1); }
	wxString        GetFirstExts    (int count)                 { return Array2String(m_ext, count); }
	bool            LookupExt       (const wxString& ext) const;

	SjExtList&      operator =      (const SjExtList& o)        { m_ext = o.m_ext; return *this; }
	bool            operator ==     (const SjExtList& o) const  { return (m_ext == o.m_ext); }
	bool            operator !=     (const SjExtList& o) const  { return (m_ext != o.m_ext); }

	static wxString NormalizeExt    (const wxString& ext)       { return SjExtList(ext).GetExt(); }

	wxString        GetFileDlgStr   (long flags=0);
	long            GetFileDlgIndex (const wxString& ext);
	wxString        GetFileDlgExt   (long index);
	void            GetFileDlgPath  (wxFileDialog&, wxString& selPath, wxString& selExt);

	bool            HasIntersectionWith (const SjExtList&) const;

	// our little repository with information about some well-known extensions (used to find out playable files and for the file selector)
#define SJ_EXT_TYPE_UNKNOWN             0
#define SJ_EXT_TYPE_PLAYABLE            1 // playable files are audio or video files as mp3, ogg, avi, mp4; just to decide whether to add a file to the library; playlists are _not_ playable in this sense!
#define SJ_EXT_TYPE_KNOWN_UNPLAYABLE    2
	static void     RepositoryAdd   (const wxString& ext, long extType, const wxString& descr);
	static wxString RepositoryDescr (const wxString& ext);
	static long     RepositoryType  (const wxString& ext);

private:
	wxArrayString           m_ext;
	mutable SjSLHash        m_hash;
	void                    AddArray        (const wxArrayString&);
	static wxArrayString    String2Array    (const wxString&);
	static wxString         Array2String    (const wxArrayString&, int maxExtToReturn);
	static void             RepositoryInit  ();
	static wxArrayString    s_repositoryExt;
	static wxArrayLong      s_repositoryType;
	static wxArrayString    s_repositoryDescr;
	wxArrayString           m_extFileDialog; // only valid after calling PrepareForFileDialog()
};


#endif // __SJ_EXT_LIST_H__
