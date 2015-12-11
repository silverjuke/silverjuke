## Process this file with automake to produce Makefile.in.
#
# Copyright © 2015 Björn Petersen
# Copyright © 2015 Dr. Tobias Quathamer
#
# This file is part of silverjuke.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# This is needed for gettext, it is too much work to rewrite
# po/Makefile.in.in for a non-recursive approach.
SUBDIRS = po

ACLOCAL_AMFLAGS = --install -I m4

# Provide the pkgdatadir variable to cpp files in order to be able
# to reliably locate the skins and keyboards.
silverjuke_CPPFLAGS = -Isrc $(WX_CPPFLAGS) -DPKGDATADIR=\"$(pkgdatadir)\" -DPKGDOCDIR=\"$(docdir)\"
silverjuke_CXXFLAGS = \
	-Isrc \
	-Wall \
	-Wno-unused-but-set-variable \
	-g \
	$(ZLIB_CFLAGS) $(LIBXINE_CFLAGS) $(SQLITE3_CFLAGS) $(WX_CXXFLAGS)
silverjuke_LDADD = $(ZLIB_LIBS) $(LIBXINE_LIBS) $(SQLITE3_LIBS) $(WX_LIBS)

dist_doc_DATA = \
	CHANGES.md \
	LICENSE \
	README.md \
	docs/command-line.rst \
	docs/dde.md \
	docs/localization.md \
	docs/plugins.md \
	docs/scripting.md \
	docs/skinning.md \
	docs/user-guide.md \
	docs/virtual-keyboards.md

desktopdir = $(datadir)/applications
dist_desktop_DATA = silverjuke.desktop

iconsdatadir = $(datadir)/icons/hicolor
icons48dir = $(iconsdatadir)/48x48/apps
icons64dir = $(iconsdatadir)/64x64/apps
icons128dir = $(iconsdatadir)/128x128/apps

dist_icons48_DATA = src/sjdata/icons/48x48/silverjuke.png
dist_icons64_DATA = src/sjdata/icons/64x64/silverjuke.png
dist_icons128_DATA = src/sjdata/icons/128x128/silverjuke.png

# Install keyboard files and skin directories
EXTRA_DIST = \
	$(top_srcdir)/src/sjdata/skins \
	$(top_srcdir)/src/sjdata/keyboards

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(datadir)/silverjuke/keyboards
	$(MKDIR_P) $(DESTDIR)$(datadir)/silverjuke/skins/golden-goal.sjs
	$(MKDIR_P) $(DESTDIR)$(datadir)/silverjuke/skins/old-style-jukebox.sjs
	$(MKDIR_P) $(DESTDIR)$(datadir)/silverjuke/skins/racky.sjs
	$(MKDIR_P) $(DESTDIR)$(datadir)/silverjuke/skins/silveriness-touched.sjs
	$(MKDIR_P) $(DESTDIR)$(datadir)/silverjuke/skins/vistact.sjs
	$(INSTALL_DATA) $(top_srcdir)/src/sjdata/keyboards/* $(DESTDIR)$(datadir)/silverjuke/keyboards
	$(INSTALL_DATA) $(top_srcdir)/src/sjdata/skins/golden-goal.sjs/* $(DESTDIR)$(datadir)/silverjuke/skins/golden-goal.sjs
	$(INSTALL_DATA) $(top_srcdir)/src/sjdata/skins/old-style-jukebox.sjs/* $(DESTDIR)$(datadir)/silverjuke/skins/old-style-jukebox.sjs
	$(INSTALL_DATA) $(top_srcdir)/src/sjdata/skins/racky.sjs/* $(DESTDIR)$(datadir)/silverjuke/skins/racky.sjs
	$(INSTALL_DATA) $(top_srcdir)/src/sjdata/skins/silveriness-touched.sjs/* $(DESTDIR)$(datadir)/silverjuke/skins/silveriness-touched.sjs
	$(INSTALL_DATA) $(top_srcdir)/src/sjdata/skins/vistact.sjs/* $(DESTDIR)$(datadir)/silverjuke/skins/vistact.sjs

# Uninstall keyboard files and skin directories
uninstall-local:
	rm -rf $(DESTDIR)$(datadir)/silverjuke/skins
	rm -rf $(DESTDIR)$(datadir)/silverjuke/keyboards

# Create and install manpage
man1_MANS = silverjuke.1
CLEANFILES = silverjuke.1
silverjuke.1:
	$(RST2MAN) $(top_srcdir)/docs/command-line.rst silverjuke.1

bin_PROGRAMS = silverjuke

silverjuke_SOURCES = \
	src/kiss_fft/kiss_fft.c \
	src/kiss_fft/kiss_fftr.c \
	src/see/cfunction.c \
	src/see/debug.c \
	src/see/dprint.c \
	src/see/dtoa.c \
	src/see/enumerate.c \
	src/see/error.c \
	src/see/function.c \
	src/see/input_lookahead.c \
	src/see/input_string.c \
	src/see/intern.c \
	src/see/interpreter.c \
	src/see/lex.c \
	src/see/mem.c \
	src/see/module.c \
	src/see/native.c \
	src/see/no.c \
	src/see/obj_Array.c \
	src/see/obj_Boolean.c \
	src/see/obj_Date.c \
	src/see/object.c \
	src/see/obj_Error.c \
	src/see/obj_Function.c \
	src/see/obj_Global.c \
	src/see/obj_Math.c \
	src/see/obj_Number.c \
	src/see/obj_Object.c \
	src/see/obj_RegExp.c \
	src/see/obj_String.c \
	src/see/parse.c \
	src/see/platform_posix.c \
	src/see/printf.c \
	src/see/regex.c \
	src/see/scope.c \
	src/see/string.c \
	src/see/stringdefs.c \
	src/see/system.c \
	src/see/tokens.c \
	src/see/try.c \
	src/see/unicase.c \
	src/see/unicode.c \
	src/see/value.c \
	src/see_dom/obj_Database.cpp \
	src/see_dom/obj_Dialog.cpp \
	src/see_dom/obj_File.cpp \
	src/see_dom/obj_HttpRequest.cpp \
	src/see_dom/obj_Player.cpp \
	src/see_dom/obj_Program.cpp \
	src/see_dom/obj_Rights.cpp \
	src/see_dom/sj_see.cpp \
	src/sjbase/autoctrl.cpp \
	src/sjbase/browser.cpp \
	src/sjbase/browser_album.cpp \
	src/sjbase/browser_cover.cpp \
	src/sjbase/browser_list.cpp \
	src/sjbase/columnmixer.cpp \
	src/sjbase/display.cpp \
	src/sjbase/mainapp.cpp \
	src/sjbase/mainframe.cpp \
	src/sjbase/mainframe_menu.cpp \
	src/sjbase/player_base.cpp \
	src/sjbase/player_impl_xine.cpp \
	src/sjbase/playlist.cpp \
	src/sjbase/queue.cpp \
	src/sjbase/search.cpp \
	src/sjbase/skin.cpp \
	src/sjbase/skinenum.cpp \
	src/sjbase/skinml.cpp \
	src/sjdata/data.cpp \
	src/sjmodules/accel.cpp \
	src/sjmodules/advsearch.cpp \
	src/sjmodules/arteditor.cpp \
	src/sjmodules/basicsettings.cpp \
	src/sjmodules/cinterface.cpp \
	src/sjmodules/help/help.cpp \
	src/sjmodules/help/htmlwindow.cpp \
	src/sjmodules/internalinterface.cpp \
	src/sjmodules/kiosk/creditbase.cpp \
	src/sjmodules/kiosk/kiosk.cpp \
	src/sjmodules/kiosk/monitor_overview.cpp \
	src/sjmodules/kiosk/numpad.cpp \
	src/sjmodules/kiosk/numpadmenu.cpp \
	src/sjmodules/kiosk/numpadpassword.cpp \
	src/sjmodules/kiosk/numpadplay.cpp \
	src/sjmodules/kiosk/numpadsearch.cpp \
	src/sjmodules/kiosk/numpadtextentry.cpp \
	src/sjmodules/kiosk/password_dlg.cpp \
	src/sjmodules/kiosk/virtkeybd.cpp \
	src/sjmodules/library.cpp \
	src/sjmodules/modulebase.cpp \
	src/sjmodules/mymusic.cpp \
	src/sjmodules/openfiles.cpp \
	src/sjmodules/playbacksettings.cpp \
	src/sjmodules/scanner/folder_scanner.cpp \
	src/sjmodules/scanner/server_scanner_config.cpp \
	src/sjmodules/scanner/server_scanner.cpp \
	src/sjmodules/settings.cpp \
	src/sjmodules/tageditor/freedb.cpp \
	src/sjmodules/tageditor/tageditor.cpp \
	src/sjmodules/tageditor/tageditorfreedb.cpp \
	src/sjmodules/tageditor/tageditorplugin.cpp \
	src/sjmodules/tageditor/tageditorrename.cpp \
	src/sjmodules/tageditor/tageditorreplace.cpp \
	src/sjmodules/tageditor/tageditorsplit.cpp \
	src/sjmodules/viewsettings.cpp \
	src/sjmodules/vis/vis_bg.cpp \
	src/sjmodules/vis/vis_cdg_raw.cpp \
	src/sjmodules/vis/vis_cdg_reader.cpp \
	src/sjmodules/vis/vis_karaoke_module.cpp \
	src/sjmodules/vis/vis_module.cpp \
	src/sjmodules/vis/vis_oscilloscope.cpp \
	src/sjmodules/vis/vis_synctxt_raw.cpp \
	src/sjmodules/vis/vis_synctxt_reader.cpp \
	src/sjmodules/vis/vis_vidout_module.cpp \
	src/sjmodules/vis/vis_window.cpp \
	src/sjmodules/weblinks.cpp \
	src/sjtools/busyinfo.cpp \
	src/sjtools/console.cpp \
	src/sjtools/csv_tokenizer.cpp \
	src/sjtools/dialog.cpp \
	src/sjtools/explore.cpp \
	src/sjtools/ext_list.cpp \
	src/sjtools/fs_inet.cpp \
	src/sjtools/gcalloc.cpp \
	src/sjtools/hash.c \
	src/sjtools/http.cpp \
	src/sjtools/imgop.cpp \
	src/sjtools/imgthread.cpp \
	src/sjtools/levensthein.c \
	src/sjtools/littleoption.cpp \
	src/sjtools/msgbox.cpp \
	src/sjtools/normalise.cpp \
	src/sjtools/sqlt.cpp \
	src/sjtools/temp_n_cache.cpp \
	src/sjtools/testdrive.cpp \
	src/sjtools/timeout.cpp \
	src/sjtools/tools.cpp \
	src/sjtools/tools_gtk.cpp \
	src/sjtools/tooltips.cpp \
	src/sjtools/volumecalc.cpp \
	src/tagger/tg_ape_tag.cpp \
	src/tagger/tg_a_tagger_frontend.cpp \
	src/tagger/tg_bytefile.cpp \
	src/tagger/tg_bytevector.cpp \
	src/tagger/tg_flac_file.cpp \
	src/tagger/tg_id3v1_tag.cpp \
	src/tagger/tg_id3v2_frame.cpp \
	src/tagger/tg_id3v2_framefactory.cpp \
	src/tagger/tg_id3v2_header_footer.cpp \
	src/tagger/tg_id3v2_knownframes.cpp \
	src/tagger/tg_id3v2_tag.cpp \
	src/tagger/tg_mp4_boxes.cpp \
	src/tagger/tg_mp4_file.cpp \
	src/tagger/tg_mpc_file.cpp \
	src/tagger/tg_mpeg_file.cpp \
	src/tagger/tg_mpeg_header.cpp \
	src/tagger/tg_mpeg_properties.cpp \
	src/tagger/tg_ogg_file.cpp \
	src/tagger/tg_oggvorbis_file.cpp \
	src/tagger/tg_oggvorbis_properties.cpp \
	src/tagger/tg_ogg_xiphcomment.cpp \
	src/tagger/tg_tagger_base.cpp \
	src/tagger/tg_wma_file_asf.cpp \
	src/tagger/tg_wma_file.cpp \
	src/tagger/tg_wma_properties.cpp \
	src/tagger/tg_wma_tag.cpp
