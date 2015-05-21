
AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = \
	-Isrc \
	$(WX_CXXFLAGS) 

AM_CFLAGS = \
	-Isrc \
	-Wall \
	-Wno-unused-but-set-variable \
	-g \
	$(WX_CPPFLAGS)

dist_doc_DATA = \
	CHANGES.md \
	LICENSE \
	README.md \
	docs/command-line.md \
	docs/dde.md \
	docs/localization.md \
	docs/plugins.md \
	docs/scripting.md \
	docs/skinning.md \
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

bin_PROGRAMS = silverjuke

silverjuke_SOURCES = \
	src/kiss_fft/kiss_fft.c \
	src/kiss_fft/kiss_fftr.c \
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
	src/sqlite/sqlite3.c \
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
	
silverjuke_LDADD = \
	$(WX_LIBS) -lz -lxine
