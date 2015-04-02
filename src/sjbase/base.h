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
 * File:    basesj.h
 * Authors: Björn Petersen
 * Purpose: This file includes the most common headers and may be useful to
 *          generate precompiles headers
 *
 ******************************************************************************/


#ifndef __SJ_BASE_H__
#define __SJ_BASE_H__
#ifdef __cplusplus  // Needed as Xcode includes this file for C-Sources as a precompiled header


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdint.h>

#include <wx/wx.h>
#include <wx/file.h>
#include <wx/filesys.h>
#include <wx/stream.h>
#include <wx/confbase.h>
#include <wx/imaglist.h>
#include <wx/regex.h>
#include <wx/dnd.h>
#include <wx/dragimag.h>
#include <wx/display.h>

#include <sjbase/compileoptions.h>
#include <sjtools/sqlt.h>
#include <sjtools/types.h>
#include <sjbase/ids.h>
#include <sjtools/normalise.h>
#include <sjtools/dialog.h>
#include <sjtools/busyinfo.h>
#include <sjtools/tooltips.h>
#include <sjtools/tools.h>
#include <sjtools/http.h>
#include <sjtools/ext_list.h>
#include <sjtools/volumecalc.h>
#include <sjbase/search.h>
#include <sjbase/playlist.h>
#include <sjbase/skin.h>
#include <sjtools/littleoption.h>
#include <sjmodules/modulebase.h>
#include <sjbase/queue.h>
#include <sjbase/player.h>
#include <sjbase/mainapp.h>
#include <sjbase/autoctrl.h>
#include <sjbase/columnmixer.h>
#include <sjbase/display.h>
#include <sjbase/mainframe.h>
#include <sjmodules/library.h>
#include <sjmodules/accel.h>


#endif // __cplusplus
#endif // __SJ_BASE_H__


