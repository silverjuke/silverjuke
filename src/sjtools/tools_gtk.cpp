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
 * File:    tools_gtk.c
 * Authors: Björn Petersen
 * Purpose: GTK specific implementations
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/kiosk/kiosk.h>
#include <wx/stdpaths.h>


/*******************************************************************************
 * Get/Create Some Default Directories
 ******************************************************************************/


wxString SjTools::GetSilverjukeProgramDir()
{
	return GetUserAppDataDir();
}


/*******************************************************************************
 * Explore - Showing Files
 ******************************************************************************/


void SjTools::ExploreFile_(const wxString& location__, const wxString& program__)
{
	wxLogError(wxT("SjTools::ExploreFile_() not yet implemented for GTK."));
}


/*******************************************************************************
 * Kiosk mode - set exclusive
 ******************************************************************************/


void SjKioskModule::SetExclusive(bool setExcl)
{
}


bool SjKioskModule::CanDisableCtrlAltDel(wxWindow* parent)
{
	return false;
}


void SjKioskModule::ClipMouse(const wxRect* wxr)
{
}


/*******************************************************************************
 * SjAccelModule - System-wide shortcuts
 ******************************************************************************/


void SjAccelModule::InitSystemAccel()
{
}


void SjAccelModule::ExitSystemAccel()
{
}


void SjAccelModule::UpdateSystemAccel(bool set)
{
}


/*******************************************************************************
 * Transparent Windows
 ******************************************************************************/


bool SjTools::CanSetWindowTransparency()
{
	return false;
}


void SjTools::SetWindowTransparency(wxWindow* window, int transparency/*0=fully opaque, 100=fully transparent*/)
{
}


void SjTools::UnloadWindowTransparencyLibs()
{
}

