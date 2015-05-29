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
 * File:    homepageids.h
 * Authors: Björn Petersen
 * Purpose: The definition of the homepage IDs
 *
 ******************************************************************************/



#ifndef __SJ_HOMEPAGEIDS_H__
#define __SJ_HOMEPAGEIDS_H__



enum SjHomepageId
{
    // Do not change any of the given values! These values are used from
    // www.silverjuke.net/go/ for forwarding to the correct page.
	SJ_HOMEPAGE_INDEX                  = 0,
	SJ_HOMEPAGE_MODULES                = 3,
	SJ_HOMEPAGE_MODULES_SKINS          = 5,
	SJ_HOMEPAGE_HELP_INDEX             = 9,
};




#endif // __SJ_HOMEPAGEIDS_H__
