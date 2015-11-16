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



bool SjTools::IsUrlExplorable(const wxString& url)
{
	wxString prot = url.BeforeFirst(wxT(':'));
	if( prot == wxT("file")
	 || prot == wxT("http")
	 || prot == wxT("https") )
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

	wxString prot = url.BeforeFirst(':');
	if( prot == wxT("http") || prot == wxT("https") )
	{
		::wxLaunchDefaultBrowser(url);
		return; // done
	}
	else if( prot == "file" )
	{
		ExploreFile_(wxFileSystem::URLToFileName(url).GetFullPath(), m_exploreProgram);
		return; // done
	}
	else if( wxFileExists(url) || wxDirExists(url) )
	{
		ExploreFile_(url, m_exploreProgram);
		return; // done, however, this is a little hack as the argument is not a URL but a native file. The caller should use ExploreFile() instead.
	}

	// error
	wxLogError(_("Cannot open \"%s\"."), url.c_str());
}


/*******************************************************************************
 *  Explore our homepage
 ******************************************************************************/


void SjTools::ExploreHomepage(SjHomepageId pageId, const wxString& param)
{
	if( !g_mainFrame->IsAllAvailable() )
	{
		return;
	}

	// get the language

	wxString lang = g_mainFrame->m_locale.GetCanonicalName();

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


