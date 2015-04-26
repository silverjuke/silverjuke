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
 * File:    columnmixer.h
 * Authors: Björn Petersen
 * Purpose: The column mixer
 *
 ******************************************************************************/



#ifndef __SJ_COLUMN_MIXER_H__
#define __SJ_COLUMN_MIXER_H__



class SjModuleSystem;
class SjCol;



enum SjSelectedUrlsScope
{
    SJ_SEL_URLS_WORKSPACE,
    SJ_SEL_URLS_DISPLAY,
    SJ_SEL_URLS_SMART
};


class SjColumnMixer
{
public:
	SjColumnMixer       ();
	~SjColumnMixer      ();

	void            LoadModules         (SjModuleSystem*);
	void            UnloadModules       ();

	void            ReloadColumns       ();

	// search handling
	SjSearchStat    SetSearch           (const SjSearch&, bool deepSearch);
	static wxString GetAzDescr          (int targetIdOrChar);

	// column- and cover-view handling, a retrieved SjCol object that is no longer needed should be deleted
	long            GetUnmaskedColCount () { return m_unmaskedColCount; }
	SjCol*          GetUnmaskedCol      (long index);

	long            GetMaskedColCount   () {return m_maskedColCount;}
	long            GetMaskedColIndexByAz(int targetId);
	long            GetMaskedColIndexByColUrl(const wxString& colUrl);
	bool            GetMaskedColIndexByRow(SjRow*, long& colIndex, long& rowIndex);
	SjCol*          GetMaskedCol        (long index);
	SjCol*          GetMaskedCol        (const wxString& trackUrl, long& retIndex);

	// list-view handling, a list view that is no longer needed should be deleted
	long            GetListViewCount    () const { return m_moduleCount; }
	SjListView*     CreateListView      (int i, long order, bool desc, long minAlbumRows) { return m_modules[i]->CreateListView(order, desc, minAlbumRows); }

	// selection handling
	bool            IsAnythingSelected  ();
	long            GetSelectedUrlCount ();
	void            GetSelectedUrls     (wxArrayString&, SjSelectedUrlsScope scope=SJ_SEL_URLS_WORKSPACE);
	void            SelectAll           (bool select, bool keepAnchor=FALSE);
	void            SelectShifted       (SjRow* rowClicked);
	bool            SelectShifted       (int keyPressed/*-1=up, +1=down*/, long& retScopeColIndex, long& retScopeRowIndex);
	bool            PrevNextRow         (long& colIndex, long& rowIndex, int dir);
	long            DelInsSelection     (bool del); // return TRUE if the browser's view should be reloaded

	// retrieve information
	bool            GetQuickInfo        (const wxString& url, wxString& trackName, wxString& leadArtistName, wxString& albumName, long& playtimeMs);
	wxString        GetTrackCoverUrl    (const wxString& url);

private:
	#define         SJ_COLUNMMIXER_MAX_MODULES 16
	long            m_moduleCount;
	SjColModule*    m_modules[SJ_COLUNMMIXER_MAX_MODULES];
	SjColModule*    m_libraryModule; // may be NULL

	long            SetSelection        (long colIndex, long rowIndex) { return SetSelectionRange(colIndex, rowIndex, colIndex, rowIndex, FALSE); }
	bool            GetSelectionRange   (long& firstSelColIndex, long& firstSelRowIndex, long* lastSelColIndex=NULL, long* lastSelRowIndex=NULL);
	long            SetSelectionRange   (long firstSelColIndex, long firstSelRowIndex, long lastSelColIndex, long lastSelRowIndex, bool keepAnchor);

	long            m_unmaskedColCount;
	long            m_moduleUnmaskedColCount[SJ_COLUNMMIXER_MAX_MODULES];

	long            m_maskedColCount;
	long            m_moduleMaskedColCount[SJ_COLUNMMIXER_MAX_MODULES];

	long            m_selAnchorColIndex,
	                m_selAnchorRowIndex;
	bool            GetSelectionAnchor  (long& colIndex, long& rowIndex);

	void            SetColCount__       ();

	SjTrackInfo     m_tempInfo;
};


#endif // __SJ_COLUMN_MIXER_H__
