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
 * File:    internalinterface.cpp
 * Authors: Björn Petersen
 * Purpose: Handling built-in modules
 *
 ******************************************************************************/


#include <sjbase/base.h>
#include <sjmodules/internalinterface.h>
#include <sjmodules/basicsettings.h>
#include <sjmodules/scanner/folder_scanner.h>
#include <sjmodules/scanner/server_scanner.h>
#include <sjmodules/scanner/upnp_scanner.h>
#include <sjmodules/library.h>
#include <sjmodules/help/help.h>
#include <sjmodules/settings.h>
#include <sjmodules/tageditor/tageditor.h>
#include <sjmodules/openfiles.h>
#include <sjmodules/viewsettings.h>
#include <sjmodules/arteditor.h>
#include <sjmodules/mymusic.h>
#include <sjmodules/playbacksettings.h>
#include <sjmodules/fx/fx_settings.h>
#include <sjmodules/vis/vis_module.h>
#include <sjmodules/vis/vis_karaoke_module.h>
#include <sjmodules/vis/vis_oscilloscope.h>
#include <sjmodules/vis/vis_projectm_module.h>
#include <sjmodules/vis/vis_vidout_module.h>
#include <sjmodules/kiosk/kiosk.h>
#include <sjmodules/advsearch.h>
#include <sjmodules/kiosk/virtkeybd.h>


SjInternalInterface* g_internalInterface = NULL;


SjInternalInterface::SjInternalInterface()
	: SjInterfaceBase(_("Internal"))
{
	g_internalInterface = this;
}


void SjInternalInterface::LoadModules(SjModuleList& list)
{
	// add COMMON modules
	list.Append   (new SjSettingsModule           (this));
	list.Append   (new SjTagEditorModule          (this));
	list.Append   (new SjOpenFilesModule          (this));
	list.Append   (new SjMyMusicModule            (this));
	list.Append   (new SjBasicSettingsModule      (this));
	list.Append   (new SjPlaybackSettingsModule   (this));
	list.Append   (new SjFxSettingsModule         (this));
	list.Append   (new SjViewSettingsModule       (this));
	list.Append   (new SjHelpModule               (this));
	list.Append   (new SjArtEditorModule          (this));
	list.Append   (new SjAccelModule              (this));
	list.Append   (new SjKioskModule              (this));
	list.Append   (new SjAdvSearchModule          (this));
	list.Append   (new SjVirtKeybdModule          (this));

	// add MLR modules
	list.Append   (new SjFolderScannerModule      (this));
	list.Append   (new SjServerScannerModule      (this));
	#if SJ_USE_UPNP
	list.Append   (new SjUpnpScannerModule        (this));
	#endif

	// add COL modules
	list.Append   (new SjLibraryModule            (this));

	// add PLAYER and VIS modules
	list.Append   (new SjPlayerModule             (this));
	list.Append   (new SjVisModule                (this));
	list.Append   (new SjOscModule                (this));
	list.Append   (new SjKaraokeModule            (this));
	#if SJ_USE_PROJECTM
	list.Append   (new SjProjectmModule           (this));
	#endif
	#if SJ_USE_VIDEO
	list.Append   (new SjVidoutModule             (this));
	#endif
}



