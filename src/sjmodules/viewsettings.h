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
 * File:    viewsettings.h
 * Authors: Björn Petersen
 * Purpose: The view settings module
 *
 ******************************************************************************/



#ifndef __SJ_VIEWSETTINGS_H__
#define __SJ_VIEWSETTINGS_H__



class SjViewSettingsModule : public SjCommonModule
{
public:
	SjViewSettingsModule(SjInterfaceBase*);

	SjEmbedTo       EmbedTo             () { return SJ_EMBED_TO_MAIN; }
	wxWindow*       GetConfigPage       (wxWindow* parent, int selectedPage);
	void            DoneConfigPage      (wxWindow* configPage, int action);

	SjHomepageId    GetHelpTopic        () { return SJ_HELP_VIEWSETTINGS; }
};




#endif // __SJ_VIEWSETTINGS_H__
