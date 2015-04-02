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
 * File:    freedb.h
 * Authors: Björn Petersen
 * Purpose: Freedb access
 *
 ******************************************************************************/


#ifndef __SJ_FREEDB_H__
#define __SJ_FREEDB_H__


#include <sjtools/http.h>


#define SJ_FREEDB_DEF_HOST wxT("freedb.freedb.org")


class SjFreedbTrack
{
public:
	wxString        m_localUrl;
	int             m_localMs;

	wxString        m_freedbGenre;
	wxString        m_freedbArtist;
	wxString        m_freedbAlbum;
	wxString        m_freedbTitle;
	long            m_freedbYear;
	long            m_freedbTrackNr;
	long            m_freedbTrackCount;
};
WX_DECLARE_OBJARRAY(SjFreedbTrack, SjArrayFreedbTrack);


class SjFreedbPossibleResult
{
public:
	SjFreedbPossibleResult(SjArrayFreedbTrack* tracks) { m_tracks = tracks; m_isSecondary = FALSE; }
	wxString            m_genre;
	wxString            m_discId;
	wxString            m_discName;
	SjArrayFreedbTrack* m_tracks; // a pointer to m_tracks1 or to m_tracks2
	bool                m_isSecondary;
};
WX_DECLARE_OBJARRAY(SjFreedbPossibleResult, SjArrayFreedbPossibleResult);


enum SjFreedbState
{
    SJ_FREEDB_STATE_INITIALIZED,
    SJ_FREEDB_STATE_WAIT_FOR_QUERY1,
    SJ_FREEDB_STATE_QUERY1_DONE,
    SJ_FREEDB_STATE_WAIT_FOR_QUERY2,
    SJ_FREEDB_STATE_QUERY2_DONE,
    SJ_FREEDB_STATE_QUERY_ERROR,
    SJ_FREEDB_STATE_WAIT_FOR_RESULT,
    SJ_FREEDB_STATE_RESULT_DONE,
    SJ_FREEDB_STATE_RESULT_ERROR
};


class SjFreedbQuery
{
public:
	SjFreedbQuery       ();

	// OnHttpEvent should be called by the client for asyncronous HTTP read events
	void            OnHttpEvent         ();

	// Do query the freedb; if Query() returns TRUE, this only means there are no
	// obvious errors, the query is only ready if IsQueryDone() returns TRUE.
	bool            Query               (const wxArrayString& localUrls, wxWindow* rcvWindow, int rcvCommandId);
	bool            IsQueryDone         () const { return (m_state>=SJ_FREEDB_STATE_QUERY2_DONE); }

	// Get the possible results, only valid if IsQueryDone() returns TRUE
	int             GetPossibleResultCount
	() const { return m_possibleResults.GetCount(); }
	const SjFreedbPossibleResult*
	GetPossibleResult   (int i) const { return &(m_possibleResults[i]); }

	// Select a result; if SelectResult() returns TRUE, this only means there are no
	// obvious errors, the result is only ready if IsResultSelected() returns TRUE
	bool            SelectResult        (int i);
	bool            IsResultSelected    () const { return (m_state>=SJ_FREEDB_STATE_RESULT_DONE); }

	// Query the result, only valid if IsResultSelected() returns TRUE
	const SjFreedbTrack*
	GetResultingTrack   (const wxString& url);


	// check for errors
	bool            HasErrors           () const { return !m_error.IsEmpty(); }
	wxString        GetError            () const { return m_error; }

	// settings
	static wxString GetFreedbHost       ();
	static wxString GetUserName         ();
	static wxString GetUserHost         ();

private:
	// the HTTP object
	SjFreedbState   m_state;
	wxString        m_httpCmd;
	SjHttp          m_http;

	// continued functions
	void            QueryDo             (SjArrayFreedbTrack& tracks, SjFreedbState newState);
	bool            QueryDoContinued    (SjArrayFreedbTrack& tracks, SjFreedbState newState);
	bool            SelectResultContinued();

	// misc
	SjArrayFreedbTrack              m_tracks1__;
	SjArrayFreedbTrack              m_tracks2__;
	SjArrayFreedbPossibleResult     m_possibleResults;
	SjArrayFreedbTrack*             m_resultTracks;

	bool            LoadTracks          (const wxArrayString& localUrls, SjArrayFreedbTrack& retTracks,
	                                     long& incompleteAlbumId);

	wxString        m_error;

	wxString        BuildQueryUrl       (const wxString& cmd);
	void            SplitField          (const wxString& in, wxString& out1, wxString& out2);
	bool            HasTooManyQuestionMarks
	(const wxString& str);
	int             CddbSum             (int n);
};


#endif // __SJ_FREEDB_H__
