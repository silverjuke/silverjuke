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
 * File:    weblinks.h
 * Authors: Björn Petersen
 * Purpose: Handle internet links to covers and artist information
 *
 ******************************************************************************/


#ifndef __SJ_WEBLINKS_H__
#define __SJ_WEBLINKS_H__


class SjWebLink
{
public:
					SjWebLink           (const wxString& section, const wxString& url, const wxString& name) { m_section=section; m_url=url; m_name=name; }
	wxString        GetUrl              () const { return m_url; }
	wxString        GetSection          () const { return m_section; }
	wxString        GetName             () const { return m_name; }

private:
	wxString        m_url;
	wxString        m_name;
	wxString        m_section;
};

WX_DECLARE_OBJARRAY(SjWebLink, SjArrayWebLink);


enum SjWebLinksType
{
     SJ_WEBLINK_COVERSEARCH
    ,SJ_WEBLINK_ARTISTINFO
};


class SjWebLinks
{
public:
	SjWebLinks  (SjWebLinksType);
	int             GetCount    () const { return m_urls.GetCount(); }
	wxString        GetName     (int i) const { return m_urls[i].GetName(); }
	wxString        GetUrl      (int i) const { return m_urls[i].GetUrl(); }
	wxString        GetUrl      (int i, const wxString& leadArtistName, const wxString& albumName, const wxString& trackName);
	void            Explore     (int i, const wxString& leadArtistName, const wxString& albumName, const wxString& trackName);
	bool            BreakAfter  (int i);

private:
	SjWebLinksType  m_type;
	SjArrayWebLink  m_urls;
};


#endif // __SJ_WEBLINKS_H__
