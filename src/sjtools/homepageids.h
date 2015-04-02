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
	SJ_HOMEPAGE_MODULES_VIRTKEYB       = 7,

	// common help topics
	SJ_HELP_INDEX                      = 9,
	SJ_HELP_COVERS                     = 10,
	SJ_HELP_ADVSEARCH                  = 11,
	SJ_HELP_VIEWSETTINGS               = 14,

	// "my music" help topics
	SJ_HELP_MYMUSIC                    = 15,
	SJ_HELP_FOLDEROPTIONS              = 16,

	// "basic settings" help topics
	SJ_HELP_BASICSETTINGS              = 17,
	SJ_HELP_MODULES                    = 18,
	SJ_HELP_SEARCHPATHS                = 20,
	SJ_HELP_FURTHEROPTIONS             = 21,

	// "playback settings" help topics
	SJ_HELP_PLAYBACKSETTINGS           = 22,
	SJ_HELP_AUDIOEFFECTS               = 23,
	SJ_HELP_AUDIOOUTPUTS               = 24,
	SJ_HELP_QUEUE                      = 26,
	SJ_HELP_AUTOCTRL                   = 27,

	// help topics for concrete modules
	SJ_HELP_AUTOVOL                    = 28,
	SJ_HELP_CROSSFADE                  = 29,
	SJ_HELP_DEFAUDIOOUTPUT             = 30,
	SJ_HELP_DECODEROPTIONS             = 31,
	SJ_HELP_VSTOPTIONS                 = 32,

	// "tag editor" help topics
	SJ_HELP_TAGEDITOR                  = 33,
	SJ_HELP_TAGEDITOR_RENAME           = 34,
	SJ_HELP_TAGEDITOR_SPLIT            = 35,
	SJ_HELP_TAGEDITOR_REPLACE          = 36,
	SJ_HELP_TAGEDITOR_CONFIRM          = 37,

	// "kiosk" help topics
	SJ_HELP_KIOSK                      = 38,
	SJ_HELP_KIOSK_FUNC                 = 39,
	SJ_HELP_VIRTKEYBD                  = 40,
	SJ_HELP_NUMPAD                     = 41,

	// info pages
	SJ_HELP_MODULE_INFO                = 42,
	SJ_HELP_FOLDEROPTIONS_INFO         = 43,
	SJ_HELP_SKIN_INFO                  = 44,

	// search for covers
	SJ_COVER_BROWSER_ONE_SHOT_HOWTO    = 45,
	SJ_COVER_BROWSER_HOWTO             = 46,

	// added to 1.15
	SJ_HELP_TAGEDITOR_FREEDB           = 47,
	SJ_HELP_CREDIT_SYSTEM              = 49,

	// added to 1.30
	SJ_HELP_FX                         = 51,
	SJ_HELP_FX_SETUP                   = 52,

	// added to 2.10
	SJ_HELP_OPENFILES                  = 53,
	SJ_HELP_KIOSK_MONITORS             = 54,
	SJ_HELP_KARAOKE                    = 56,
	SJ_HELP_VIS                        = 57,
	SJ_HOMEPAGE_IMPRINT                = 59,
	SJ_HELP_CONSOLE                    = 60,
	SJ_HELP_SPEAKERS                   = 61,

	// added to 2.52
	SJ_HELP_SERVERSCANNER              = 62,

	// must be last
	SJ_HOMEPAGE_MAX_PAGE_INDEX
};




#endif // __SJ_HOMEPAGEIDS_H__
