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
 * File:    arteditor.h
 * Authors: Björn Petersen
 * Purpose: Art Editor
 *
 ******************************************************************************/



#ifndef __SJ_ARTEDITOR_H__
#define __SJ_ARTEDITOR_H__



class SjArtEditor;



#define SJ_CROP_DELTA 4



class SjArtEditorModule : public SjCommonModule
{
public:
	SjArtEditorModule   (SjInterfaceBase*);

	void            CreateArtMenu       (SjMenu&                menu,
	                                     const wxArrayString&   allUrls,
	                                     const wxString&        selUrl,
	                                     const wxString&        defaultUrl=wxT(""),
	                                     SjArtEditor*           artEditor=NULL);

	bool            OnArtMenu           (wxWindow*              parent,
	                                     const wxArrayString&   allUrls,
	                                     const wxString&        selUrl,
	                                     long                   albumId,
	                                     long                   menuId,
	                                     SjArtEditor*           artEditor,
	                                     bool                   isPlaylistCover);

	bool            IsOpened            () const { return (m_artEditor!=NULL); }

protected:
	void            LastUnload          ();

private:
	SjArtEditor*    m_artEditor;
	friend class    SjArtEditor;
};


extern SjArtEditorModule* g_artEditorModule;





#endif // __SJ_ARTEDITOR_H__
