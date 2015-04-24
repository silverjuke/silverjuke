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
#include <sjtools/tools.h>
#include <sjmodules/kiosk/kiosk.h> // for statistics in ExploreHomepage()
#include <sjmodules/kiosk/virtkeybd.h> // for statistics in ExploreHomepage()


/*******************************************************************************
 * Constructor
 ******************************************************************************/


void SjTools::InitExplore()
{
	m_exploreProgram = g_tools->m_config->Read(wxT("main/exploreWith"),
	                   SjLittleExploreSetting::s_explorePrograms[0]);
}


/*******************************************************************************
 * Check whether an URL is exploreable
 ******************************************************************************/


static bool wxFileSystemHandler_IsSupportedProtocol(const wxString& location, int hashPos)
{
	// check the rough syntax of a protocol to allow filenames with a '#' sign
	wxString test = location.Mid(hashPos+1);
	int doublePointPos = test.Find(':');
	return (doublePointPos>=2 && doublePointPos<=6); // max. known length is "memory:"
}


static wxString wxFileSystemHandler_GetLeftLocation(const wxString& location)
{
	int i;
	bool fnd;

	fnd = FALSE;
	for (i = location.Length()-1; i >= 0; i--) {
		if ((location[i] == wxT(':')) && (i != 1 /*win: C:\path*/)) fnd = TRUE;
		else if (fnd && (location[i] == wxT('#') && wxFileSystemHandler_IsSupportedProtocol(location,i))) return location.Left(i);
	}
	return location;
}


bool SjTools::IsUrlExplorable(const wxString& url)
{
	wxString left = wxFileSystemHandler_GetLeftLocation(url);
	if( ::wxFileExists(url) || ::wxDirExists(url)
	        || ::wxFileExists(left) || ::wxDirExists(left) )
	{
		return true;
	}

	wxString prot = left.BeforeFirst(wxT(':'));
	if( prot == wxT("memory")
	        || prot == wxT("http")
	        || prot == wxT("https")
	        || ::wxFileExists(url) || ::wxDirExists(url)
	        || ::wxFileExists(left) || ::wxDirExists(left) )
	{
		return true;
	}

	return false;
}


/*******************************************************************************
 * Explore any URL - in the internet or in the local file system
 ******************************************************************************/


void SjTools::ExploreUrl(const wxString& url)
{
	if( !g_mainFrame->IsAllAvailable() )
	{
		return;
	}

	wxString left = wxFileSystemHandler_GetLeftLocation(url);
	wxString prot = left.BeforeFirst(':');
	if( prot == wxT("memory") )
	{
		ExploreFile_(GetSilverjukeProgramDir(), m_exploreProgram);
	}
	else if( prot == wxT("http") || prot == wxT("https") )
	{
		::wxLaunchDefaultBrowser(url);
	}
	else if( ::wxFileExists(url) || ::wxDirExists(url) )
	{
		ExploreFile_(url, m_exploreProgram);
	}
	else if( ::wxFileExists(left) || ::wxDirExists(left) )
	{
		ExploreFile_(left, m_exploreProgram);
	}
	else
	{
		::wxLogError(_("Cannot open \"%s\"."), url.c_str());
	}
}


/*******************************************************************************
 *  Explore our homepage
 ******************************************************************************/


bool SjTools::IsLangGerman()
{
	wxString lang(LocaleConfigRead(wxT("__THIS_LANG__"), wxT("en")));
	lang.MakeLower();
	lang = lang.Left(2);
	if( lang == wxT("de") )
	{
		return TRUE;
	}
	return FALSE;
}


void SjTools::ExploreHomepage(SjHomepageId pageId, const wxString& param)
{
	if( !g_mainFrame->IsAllAvailable() )
	{
		return;
	}

	// get the language
	wxString lang = wxT("en");
	if( IsLangGerman() )
		lang = wxT("de");

	// create the url

	wxString url = wxString::Format(
	                   wxT("http://www.silverjuke.net/go/?page=%i&lang=%s"),
	                   (int)pageId,
	                   lang.c_str());

	// add param

	if( !param.IsEmpty() )
	{
		url += wxT("&param=") + SjTools::Urlencode(param);
	}

	// explore now.

	ExploreUrl(url);
}


