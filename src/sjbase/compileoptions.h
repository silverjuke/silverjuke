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
 * File:    compileoptions.h
 * Authors: Björn Petersen
 * Purpose: Program configuration
 *
 ******************************************************************************/


#ifndef __SJ_CONFIG_H__
#define __SJ_CONFIG_H__

#define SJ_VERSION_MAJOR    15      // when changing the version number here, also change it in configure.ac and maybe in the OS-depending installer
#define SJ_VERSION_MINOR    10
#define SJ_VERSION_REVISION 6

#ifndef SJ_PROGRAM_NAME
#define SJ_PROGRAM_NAME wxT("Silverjuke")
#endif

#ifndef SJ_USE_C_INTERFACE          // allow C plugins? Makes only sense if scripting is enabled, too.
#define SJ_USE_C_INTERFACE 1
#endif

#ifndef SJ_USE_SCRIPTS              // Can the user use scripts?
#define SJ_USE_SCRIPTS 1
#endif

#ifndef SJ_USE_TOOLTIPS             // Use tooltips?
#define SJ_USE_TOOLTIPS 0
#endif

#ifndef SJ_USE_VIDEO                // Use the video decoder and the video display?
#define SJ_USE_VIDEO 0
#endif

#ifndef SJ_CAN_USE_MM_KEYBD
#define SJ_CAN_USE_MM_KEYBD 0       // Can the multimedia keyboards keys be used?
#endif

#ifndef SJ_CAN_USE_GLOBAL_HOTKEYS   // Can global hotkeys be used?
#define SJ_CAN_USE_GLOBAL_HOTKEYS 0
#endif

#ifndef SJ_CAN_DISABLE_SCRSAVER     // Can the screensaver be disabled?
#define SJ_CAN_DISABLE_SCRSAVER 0
#endif

#ifndef SJ_CAN_DISABLE_POWERMAN     // Can the power management be disabled?
#define SJ_CAN_DISABLE_POWERMAN 0
#endif

#ifndef SJ_DEF_SQLITE_SYNC
#define SJ_DEF_SQLITE_SYNC  0L      // 0=off (fast), 1=normal (save but slower), 2=full (very save and slow)
#endif

#ifndef SJ_DEF_SQLITE_CACHE_BYTES
#define SJ_DEF_SQLITE_CACHE_BYTES 0x100000L
#endif

#ifndef SJ_DEF_IMGTHREAD_CACHE_BYTES
#define SJ_DEF_IMGTHREAD_CACHE_BYTES 0x200000L
#endif

#ifndef SJ_DEF_SEARCH_WHILE_TYPING
#define SJ_DEF_SEARCH_WHILE_TYPING 1L
#endif

#ifndef SJ_RATING_CHARS_DLG
#if wxUSE_UNICODE
	#define SJ_RATING_CHARS_DLG       wxT("\u2605") // "BLACK STAR"
	#define SJ_RATING_CHARS_ELSEWHERE wxT("\u2605") // "BLACK STAR"
#else
	#define SJ_RATING_CHARS_DLG       wxT("*")
	#define SJ_RATING_CHARS_ELSEWHERE wxT("*")
#endif
#endif

#ifndef SJ_DEF_FONT_FACE
#define SJ_DEF_FONT_FACE wxT("Arial")
#endif

#ifndef SJ_DEF_FONT_SIZE
#define SJ_DEF_FONT_SIZE 10L
#endif

#ifndef SJ_ALT_FONT_FACE
#define SJ_ALT_FONT_FACE wxT("Times New Roman") // used for creating dummy covers
#endif

#ifndef SJ_DEF_COLUMN_WIDTH
#define SJ_DEF_COLUMN_WIDTH  180L   // pixels
#define SJ_MIN_COLUMN_WIDTH   80L
#define SJ_MAX_COLUMN_WIDTH 1600L
#endif

#ifndef SJ_DEF_COVER_HEIGHT
#define SJ_DEF_COVER_HEIGHT 90L     // percent - 90% looks a little bit smarter as if using the full column width
#endif

#ifndef SJ_MIN_FONT_SIZE
#define SJ_MIN_FONT_SIZE 6L         // Pt
#define SJ_MAX_FONT_SIZE 64L
#endif

#ifndef SJ_DEF_VOLUME
#define SJ_DEF_VOLUME 240L          // 0..255
#endif

#ifndef SJ_DEF_OUTPUT_BUF_MS
#define SJ_DEF_OUTPUT_BUF_MS 92     // buffer size should be a power of 2 in bytes (this is corrected automatically)
#endif

#ifndef SJ_SEARCH_DELAY_MS
#define SJ_SEARCH_DELAY_MS 180L     // the initial number of milliseconds between text input and search start,
#endif                              // the number is adapted on-the-fly by the 'real' search time

#ifndef SJ_HISTORY_DELAY_MS
#define SJ_HISTORY_DELAY_MS 3000L   // the number of milliseconds before new text input is added to the history
#endif

#ifndef SJ_SEEK_RESOLUTION
#define SJ_SEEK_RESOLUTION 500L     // ms, the display is updated at most every 1000 ms
#endif

#ifndef SJ_FILENAME_MAX_LEN         // some information about a file name (NOT: a path!)
#define SJ_FILENAME_MAX_LEN 240
#define SJ_FILENAME_FORBIDDEN           wxT("\\") wxT("\"") wxT("/:*?<>|")
#define SJ_FILENAME_FORBIDDEN_REPLACE   wxT("_")  wxT("'")  wxT("__+____")
#endif


#endif // __SJ_CONFIG_H__
