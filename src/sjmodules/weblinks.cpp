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
 * File:    weblinks.cpp
 * Authors: Björn Petersen
 * Purpose: Handle internet links to covers and artist information
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/weblinks.h>

#include <wx/arrimpl.cpp> // sic!
WX_DEFINE_OBJARRAY(SjArrayWebLink);


/*******************************************************************************
 * The URLs
 ******************************************************************************/


// international artist information
// Unused sites:
// "FreeDB.org=http://www.freedb.org/freedb_search.php?words=%artist%&allfields=NO&fields=artist&allcats=YES&grouping=cats\n"
// Get-Lyrics.net=http://www.get-lyrics.net/Titel.php?interpret=%artist% [German, this is now a DIALER site]
// MusicBrother.com=http://www.musicbrother.com/index.php?page=searching&text=%artist%&cat=interpret [German, this is now a DIALER site]
// www.radiofox.ee [this is a DIALER site]
static const wxChar s_artistinfo_int[] =

    wxT("[homepage]\n")
    wxT("Google.com/hp=http://www.google.com/search?q=%22%artistutf8%%22&btnI=1\n")

    wxT("[info2]\n")
    wxT("AllMusic.com=http://www.allmusic.com/cg/amg.dll?P=amg&sql=%artist%&opt1=1\n")
    wxT("RollingStone.com=http://www.rollingstone.com/?searchtype=RSArtist&query=%artist%\n")
    wxT("ArtistDirect.com=http://www.artistdirect.com/cgi-bin/gx.cgi/AppLogic+Search?select=MusicArtist&searchstr=%artist%&searchtype=NormalSearch&start=1\n")
    wxT("RateYourMusic.com=http://rateyourmusic.com/search?searchterm=%artist%&searchtype=artist\n")

    wxT("[lyrics]\n")
    wxT("LyricWiki.org=http://lyricwiki.org/Special:Search?search=%artistutf8%:%trackutf8%\n")
    wxT("SongMeanings.net=http://www.songmeanings.net/query.php?type=artists&query=%artist%\n")

    wxT("[search]\n")
    wxT("Google.com=http://www.google.com/search?q=%22%artistutf8%%22\n")
    wxT("AllTheWeb.com=http://www.alltheweb.com/search?q=%22%artistutf8%%22\n")
    ;


// artist information for english only,
// may be overwritten by local *.mo file
static const wxChar s_artistinfo_en[] =
    wxT("[info]\n")
    wxT("Wikipedia.org=http://en.wikipedia.org/wiki/%artist%\n")
    ;


// international cover search
static const wxChar s_coversearch_int[] =
    wxT("[search]\n")
    wxT("AllMusic.com=http://www.allmusic.com/cg/amg.dll?P=amg&sql=%album%&opt1=2\n")
    wxT("AllTheWeb.com=http://www.alltheweb.com/search?cat=img&q=%artistutf8%+%albumutf8%\n")
    wxT("AltaVista.com=http://www.altavista.com/image/results?pg=q&stype=simage&imgset=2&q=%artist%+%album%&avkw=aapt\n")
    wxT("Amazon.com=http://www.amazon.com/exec/obidos/external-search?index=music&keyword=%artist%+%album%\n")
    wxT("Buy.com=http://www.buy.com/retail/searchresults.asp?searchtype=1&qutype=0&qu=%artist%+%album%&search_store=6\n")
    wxT("eBay.com=http://search.ebay.com/search/search.dll?cgiurl=http%3A%2F%2Fcgi.ebay.com%2Fws%2F&krd=1&from=R8&MfcISAPICommand=GetResult&ht=1&SortProperty=MetaEndSort&query=%artist%+%album%\n")
    wxT("Google.com=http://images.google.com/images?q=%artistutf8%+%albumutf8%\n")
    wxT("WalMart.com=http://www.walmart.com/catalog/search-ng.gsp?search_constraint=4104&search_query=%artist%+%album%\n")
    wxT("CDuniverse.com=http://www.cduniverse.com/sresult.asp?HT_Search_Info=%album%&style=music&HT_Search=TITLE\n")
    ;


// cover search for english only,
// may be overwritten by local *.mo file
static const wxChar s_coversearch_en[] =
    wxT("\n")
    ;


/*******************************************************************************
 * SjWebLinks class
 ******************************************************************************/


static int SjWebLinks_Sort(SjWebLink** i1__, SjWebLink** i2__)
{
	SjWebLink *i1 = *i1__, *i2 = *i2__;

	int ret = i1->GetSection().Cmp(i2->GetSection());
	if( ret == 0 )
	{
		ret = i1->GetName().CmpNoCase(i2->GetName());
	}

	return ret;
}


SjWebLinks::SjWebLinks(SjWebLinksType type)
{
	wxString    source;
	wxChar*     linePtr;
	wxString    currSection, currUrl, currName;

	m_type = type;

	// get raw lines
	{
		const wxChar* source_en = NULL;
		const wxChar* source_int = NULL;
		if( type==SJ_WEBLINK_COVERSEARCH )
		{
			source = SjTools::LocaleConfigRead(wxT("__COVER_SEARCH_URLS__"), wxT(""));
			source_en = s_coversearch_en;
			source_int = s_coversearch_int;
		}
		else
		{
			source = SjTools::LocaleConfigRead(wxT("__ARTIST_INFO_URLS__"), wxT(""));
			source_en = s_artistinfo_en;
			source_int = s_artistinfo_int;
		}

		if( source.Find(wxT("http"))==-1 )
		{
			wxASSERT(source_en);
			source = source_en;
		}

		if( source.Find(wxT("[homepage]"))==-1 )
		{
			wxASSERT(source_int);
			source.Prepend(source_int);
		}
	}

	// parse lines
	SjLineTokenizer tkz(source);
	while( (linePtr=tkz.GetNextLine()) )
	{
		if( linePtr[0] )
		{
			if( linePtr[0] == '[' )
			{
				currSection = linePtr;
				currSection = currSection.AfterFirst('[').Trim(FALSE).BeforeLast(']').Trim().Lower();
			}
			else
			{
				currUrl  = linePtr;
				currName = currUrl.BeforeFirst('=').Trim();
				currUrl  = currUrl.AfterFirst('=').Trim(FALSE);
				if( !currUrl.IsEmpty() && !currName.IsEmpty() )
				{
					// name/url found - add section to name
					if( currSection == wxT("homepage") )
					{
						currName = _("Search homepage");
					}
					else if( currSection == wxT("lyrics") )
					{
						currName = wxString::Format(_("Lyrics on %s"), currName.c_str());
					}
					else if( currSection == wxT("search") )
					{
						currName = wxString::Format(_("Search with %s"), currName.c_str());
					}
					else
					{
						currName = wxString::Format(_("Information on %s"), currName.c_str());
					}

					currName += wxT("...");
					m_urls.Add(new SjWebLink(currSection, currUrl, currName));
				}
			}
		}
	}

	m_urls.Sort(SjWebLinks_Sort);
}


static void specialUrlencodeNreplace(
    wxString&           url,
    const wxString&     placeholderName,
    const wxString&     placeholderValue,
    bool                placeholderUtf8 )
{
	// create the possible replacement ...

	// ... normal urlencode()
	wxString replacement = SjTools::Urlencode(placeholderValue, placeholderUtf8);

	// ... something special? the only difference to SjTools::Urlencode() is the "_" instead of the "+". Used in the Wikipedia.
	if( url.Find(wxT(".wikipedia.org/wiki/%"))!=-1 )
		replacement.Replace(wxT("+"), wxT("_"));

	// replace, done
	url.Replace(placeholderName, replacement);
}


wxString SjWebLinks::GetUrl(int i, const wxString& leadArtistName, const wxString& albumName, const wxString& trackName)
{
	wxString url = GetUrl(i);

	specialUrlencodeNreplace(   url, wxT("%artist%"),       leadArtistName, false   );
	specialUrlencodeNreplace(   url, wxT("%artistutf8%"),   leadArtistName, true    );
	specialUrlencodeNreplace(   url, wxT("%album%"),        albumName,      false   );
	specialUrlencodeNreplace(   url, wxT("%albumutf8%"),    albumName,      true    );
	specialUrlencodeNreplace(   url, wxT("%track%"),        trackName,      false   );
	specialUrlencodeNreplace(   url, wxT("%trackutf8%"),    trackName,      true    );

	return url;
}


void SjWebLinks::Explore(int i, const wxString& leadArtistName, const wxString& albumName, const wxString& trackName)
{
	wxString url = GetUrl(i, leadArtistName, albumName, trackName);

	g_tools->ExploreUrl(url);
}


bool SjWebLinks::BreakAfter(int i)
{
	if( i < GetCount()-1 )
	{
		if( m_urls[i].GetSection() != m_urls[i+1].GetSection() )
		{
			return TRUE;
		}
	}

	return FALSE;
}

