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
 * File:    ids.h
 * Authors: Björn Petersen
 * Purpose: Silverjuke main application id definitions
 *
 *******************************************************************************
 *
 * We do not use enums for the IDs as they may be send through windows, wx
 * etc. which will need too much conversion; moreover a conversion from
 * int to enum is undefined (however, normally this may work...)
 *
 ******************************************************************************/


#ifndef __SJ_IDS_H__
#define __SJ_IDS_H__


/* Take care, the identifiers should NOT be in the range of wxID_LOWEST (4999)
 * to wxID_HIGHEST (5999). Moreover, all IDs should only use 16 Bit as
 * the IDs may be used together with some flags in the high-order word.
 */


/* First, we define the [T]arget [ID]s, IDT_*, as they're used in an zero-based
 * array in SjSkin. So don't put unneeded gaps in the target IDs.
 */
#define IDT_NONE                    0
#define IDT_UNUSABLE_01             1   // range start (avoid other system ID conflicts)
#define IDT_UNISABLE_20             20  // range end
#define IDT_FIRST                21     // first target ID relavant to menu selection
#define IDT_WORKSPACE_GOTO_A        21  // range start (A, B, C, ... Z, 0-9)
#define IDT_WORKSPACE_GOTO_Z        46
#define IDT_WORKSPACE_GOTO_0_9      47  // range end
#define IDT_LAYOUT_FIRST            48  // range start
#define IDT_LAYOUT_LAST             96  // range end, max. 49 layouts
#define IDT_DISPLAY_LINE_FIRST      97  // range start
#define IDT_DISPLAY_LINE_LAST       146 // range end, max. 50 lines
#define IDT_PRELISTEN_VOL_DOWN      147
#define IDT_PRELISTEN_VOL_UP        148
#define IDT_DISPLAY_UP              149
#define IDT_DISPLAY_DOWN            150
#define IDT_DISPLAY_V_SCROLL        151
#define IDT_GOTO_CURR               152
#define IDT_WORKSPACE_HOME          153
#define IDT_TOGGLE_TIME_MODE        154
#define IDT_WORKSPACE_END           155
#define IDT_TOGGLE_KIOSK            156
#define IDT_DISPLAY_COVER           157
#define IDT_PLAY                    158
#define IDT_PAUSE                   159
#define IDT_STOP                    160
#define IDT_PREV                    161
#define IDT_NEXT                    162
#define IDT_SEEK                    163
#define IDT_SEARCH                  164
#define IDT_SEARCH_BUTTON           165
#define IDT_SEARCH_INFO             166
#define IDT_CURR_CREDIT             167
#define IDT_CURR_TRACK              168
#define IDT_CURR_TIME               169
#define IDT_MAIN_VOL_SLIDER         170
#define IDT_MAIN_VOL_UP             171
#define IDT_MAIN_VOL_DOWN           172
#define IDT_MAIN_VOL_MUTE           173
#define IDT_WORKSPACE               175
#define IDT_WORKSPACE_LINE_LEFT     176 // this a the button beside a scrollbar, see also IDT_WORKSPACE_KEY_*
#define IDT_WORKSPACE_LINE_RIGHT    177 //          - " -
#define IDT_WORKSPACE_LINE_UP       178 //          - " -
#define IDT_WORKSPACE_LINE_DOWN     179 //          - " -
#define IDT_WORKSPACE_H_SCROLL      180
#define IDT_WORKSPACE_V_SCROLL      181
#define IDT_ZOOM_IN                 182
#define IDT_ZOOM_OUT                183
#define IDT_ZOOM_NORMAL             184
#define IDT_QUIT                    187
#define IDT_ENQUEUE_LAST            192
#define IDT_ENQUEUE_NEXT            193
#define IDT_ENQUEUE_NOW             194
#define IDT_UNQUEUE                 195
#define IDT_UNQUEUE_ALL             196
#define IDT_MAINMENU                197
#define IDT_VIS_TOGGLE              198
#define IDT_UPDATE_INDEX            199
#define IDT_DEEP_UPDATE_INDEX       200
#define IDT_SETTINGS                201
#define IDT_WORKSPACE_GOTO_RANDOM   202
#define IDT_STOP_AFTER_THIS_TRACK   203
#define IDT_SHUFFLE                 204
#define IDT_REPEAT                  205
#define IDT_ALWAYS_ON_TOP           207
#define IDT_VIS_RECT                208
#define IDT_OPEN_FILES              211
#define IDT_APPEND_FILES            212
#define IDT_SAVE_PLAYLIST           213
#define IDT_ADV_SEARCH              214
#define IDT_WORKSPACE_SHOW_COVERS   215
#define IDT_UNUSED_216              216
#define IDT_NUMPAD_FIRST         217    // Range start
#define IDT_NUMPAD_0                217
#define IDT_NUMPAD_1                218
#define IDT_NUMPAD_2                219
#define IDT_NUMPAD_3                220
#define IDT_NUMPAD_4                221
#define IDT_NUMPAD_5                222
#define IDT_NUMPAD_6                223
#define IDT_NUMPAD_7                224
#define IDT_NUMPAD_8                225
#define IDT_NUMPAD_9                226
#define IDT_NUMPAD_CANCEL           227
#define IDT_NUMPAD_PAGE_LEFT        228
#define IDT_NUMPAD_PAGE_RIGHT       229
#define IDT_NUMPAD_PAGE_UP          230
#define IDT_NUMPAD_PAGE_DOWN        231
#define IDT_NUMPAD_MENU             232
#define IDT_NUMPAD_RESERVED_01      233
#define IDT_NUMPAD_RESERVED_08      240
#define IDT_NUMPAD_LAST          240    // Range end
#define IDT_PLAY_NOW_ON_DBL_CLICK   241
#define IDT_PRELISTEN               242
#define IDT_ADD_CREDIT_FIRST     243    // Range start
#define IDT_ADD_CREDIT_01           243
#define IDT_ADD_CREDIT_16           258
#define IDT_ADD_CREDIT_RESERVED_01  259
#define IDT_ADD_CREDIT_RESERVED_08  266
#define IDT_ADD_CREDIT_LAST      266    // Range end
#define IDT_NEXT_TRACK              267
#define IDT_FADE_TO_NEXT            268
#define IDT_MORE_FROM_CURR_ALBUM    269
#define IDT_MORE_FROM_CURR_ARTIST   270
#define IDT_WORKSPACE_PAGE_LEFT     271
#define IDT_WORKSPACE_PAGE_RIGHT    272
#define IDT_WORKSPACE_PAGE_UP       273
#define IDT_WORKSPACE_PAGE_DOWN     274
#define IDT_WORKSPACE_INSERT        275
#define IDT_WORKSPACE_DELETE        276
#define IDT_WORKSPACE_KEY_LEFT      277 // this a pressed cursor key, see also IDT_WORKSPACE_LINE_*
#define IDT_WORKSPACE_KEY_RIGHT     278 //          - " -
#define IDT_WORKSPACE_KEY_UP        279 //          - " -
#define IDT_WORKSPACE_KEY_DOWN      280 //          - " -
#define IDT_WORKSPACE_MINOR_HOME    281 // this resets eg. the vertical position in the album view; for the list view, this does nothing
#define IDT_WORKSPACE_MINOR_END     282 //          - " -
#define IDT_WORKSPACE_ALBUM_VIEW    283
#define IDT_WORKSPACE_COVER_VIEW    284
#define IDT_WORKSPACE_LIST_VIEW     285
#define IDT_WORKSPACE_TOGGLE_VIEW   286
#define IDT_WORKSPACE_ENTER         287
#define IDT_WORKSPACE_GOTO_PREV_AZ  288
#define IDT_WORKSPACE_GOTO_NEXT_AZ  289
#define IDT_SEEK_FWD                290
#define IDT_SEEK_BWD                291
#define IDT_STOP_AFTER_EACH_TRACK   294
#define IDT_TOGGLE_REMOVE_PLAYED    295
#define IDT_LAST                 295 /* last relavant target ID */


/* Here comes the [M]odules [ID]s, IDM_*, which are normally forwarded
 * to the current module. Most times these IDs are used in
 * context menus. Modules can define their own, private IDs in
 * the range IDM_FIRSTPRIVATE..IDM_LASTPRIVATE
 */
#define IDM_FIRST               1000 /* range start */
#define IDM_EDITSELECTION       1001
#define IDM_ROTATELEFT          1003
#define IDM_ROTATERIGHT         1004
#define IDM_DONTROTATE          1005
#define IDM_FULLSCREEN          1006
#define IDM_FLIPHORZ            1008
#define IDM_FLIPVERT            1009
#define IDM_BRIGHTPLUS          1010
#define IDM_BRIGHTMINUS         1011
#define IDM_CONTRASTPLUS        1012
#define IDM_CONTRASTMINUS       1013
#define IDM_CONTRASTBRIGHTNULL  1014
#define IDM_GRAYSCALE           1015
#define IDM_NEGATIVE            1016
#define IDM_OPENCOVER           1017
#define IDM_SAVECOVER           1019
#define IDM_CROP                1054
#define IDM_EXPLORE             1055
#define IDM_WWWCOVER00          1056    /* range start, place for 50 IDs */
#define IDM_WWWCOVER49          1105    /* range end */
#define IDM_SELECTIMG00         1106    /* range start, place for 300 IDs */
#define IDM_SELECTIMG299        1405    /* range end */
#define IDM_SELECTIMGAUTO       1406
#define IDM_NOALTCOVERS         1407
#define IDM_PREVIMG             1408
#define IDM_NEXTIMG             1409
#define IDM_OPENARTEDITOR       1410
#define IDM_OPENARTEDFORCROP    1411
#define IDM_CLOSEARTEDITOR      1412
#define IDM_SELECTARTISTINF00   1413    /* range start */
#define IDM_SELECTARTISTINF49   1462    /* range end */
#define IDM_RATINGSELECTION00   1463
#define IDM_RATINGSELECTION01   1464
#define IDM_RATINGSELECTION02   1465
#define IDM_RATINGSELECTION03   1466
#define IDM_RATINGSELECTION04   1467
#define IDM_RATINGSELECTION05   1468
#define IDM_RATINGSELECTION06   1469
#define IDM_FIRSTPRIVATE        1500    /* range start, place for 1500 IDs */
#define IDM_LASTPRIVATE         2999    /* range end */
#define IDM_LAST                2999 /* range end */


/* [O]ther (Menu) [ID]s, IDO_*, as they're not used as a target or
 * by a modules.
 */
#define IDO_VIS_FIRST__         4800 /* range start */
#define IDO_VIS_STARTFIRST      4800    /* range start */
#define IDO_VIS_STARTLAST       4848    /* range end */
#define IDO_VIS_STOP            4849
#define IDO_VIS_OPTIONFIRST     4850    /* range start */
#define IDO_VIS_OPTIONLAST      4899    /* range end */
#define IDO_VIS_HALFSIZE        4901
#define IDO_VIS_AUTOSWITCHOVER  4902
#define IDO_VIS_SHOWPRESETNAME  4903
#define IDO_VIS_GOTORNDPRESET   4904
#define IDO_VIS_NEXTPRESET      4905
#define IDO_VIS_PREVPRESET      4906
#define IDO_VIS_LAST__          4920 /* range end */
#define IDO_wxID_LOWEST         4999 /* range start, used by wx */
#define IDO_wxID_HIGHEST        5999 /* range end, used by wx */
#define IDO_SEARCHGENRE000      6000 /* range start */
#define IDO_SEARCHGENRE999      6999 /* range end */
#define IDO_SEARCHMUSICSEL000   7000 /* range start */
#define IDO_SEARCHMUSICSEL999   7999 /* range end */
#define IDO_SAMEZOOMINALLVIEWS  8000
#define IDO_START_PB_ON_ENQUEUE 8001
#define IDO_CORNERCLICK         8002
#define IDO_SETTINGS_ADDFILES   8004
#define IDO_ESC                 8005
#define IDO_DEBUGSKIN_RELOAD    8006
#define IDO_DISPLAY_TRACKNR     8007
#define IDO_DISPLAY_ARTIST      8008
#define IDO_DISPLAY_TOTAL_TIME  8009
#define IDO_DISPLAY_AUTOPLAY    8010
#define IDO_DISPLAY_PREFERALBCV 8011
#define IDO_TOGGLE_TIME_MODE2   8012
#define IDO_SETTINGS_QUEUE      8013
#define IDO_SETTINGS_AUTOCTRL   8014
#define IDO_SETTINGS_SKINS      8015
#define IDO_SETTINGS_FONTNCOVER 8016
#define IDO_SETTINGS_EQUALIZER  8017
#define IDO_SMOOTH              8109
#define IDO_SEARCHINPUT         8110
#define IDO_TAB                 8111
#define IDO_SEARCHHISTORY00     8112 /* range start */
#define IDO_SEARCHHISTORY99     8211 /* range end */
#define IDO_SELECTALL           8215
#define IDO_LINKCLICKED         8224
#define IDO_INIPROGRAMLISTFIRST 8228 /* range start, 100 IDs required */
#define IDO_INIPROGRAMLISTLAST  8328 /* range end */
#define IDO_MARKERSOPTIONS      8329
#define IDO_REALLYENDSEARCH     8330
#define IDO_DISPLAY_COVER       8331
#define IDO_PASTE               8332
#define IDO_PASTE_USING_COORD   8333
#define IDO_DISPLAY_COVER_EXPLR 8336
#define IDO_EDITQUEUE           8337
#define IDO_RATINGQUEUE00       8338
#define IDO_RATINGQUEUE01       8339
#define IDO_RATINGQUEUE02       8340
#define IDO_RATINGQUEUE03       8341
#define IDO_RATINGQUEUE04       8342
#define IDO_RATINGQUEUE05       8343
#define IDO_RATINGQUEUE06       8344
#define IDO_ARTISTINFQUEUE00    8347 /* range start */
#define IDO_ARTISTINFQUEUE49    8396 /* range end */
#define IDO_UNQUEUE_MARKED      8397
#define IDO_UNQUEUE_ALL_BUT_MARKED 8398
#define IDO_GOTO_CURR_MARK      8399
#define IDO_DND_ONDATA          8400
#define IDO_VIRTKEYBDFRAME      8401
#define IDO_GOTOCURRAUTO        8402
#define IDO_OPENSETTINGSASYNC   8403
#define IDO_SCRIPTCONFIG_MENU00 8404 /* range start */
#define IDO_SCRIPTCONFIG_MENU99 8503 /* range end */
#define IDO_ONLINE_HELP         8606
#define IDO_EXPLOREQUEUE        8607
#define IDO_BROWSER_RELOAD_VIEW 8609
#define IDO_ABOUT_OPEN_WWW      8610
#define IDO_ABOUT               8611
#define IDO_SCRIPT_MENU00       8613 /* range start */
#define IDO_SCRIPT_MENU99       8712 /* range end */
#define IDO_CONSOLE             8713
/* take care, we're close to end! At 8800 the IDPLAYER_ IDs start! */

/* [PLAYER] [ID]s, IDPLAYER_*, posted from SjPlayer -> SjMainFrame -> SjPlayer.OnPostBack()
 */
#define IDPLAYER_FIRST          8800 /* range start */
#define IDPLAYER_LAST           8899 /* range end */


/* [TIMER] [ID]s, IDTIMER_*
 */
#define IDTIMER_SEARCHINPUT     8900
#define IDTIMER_ELAPSEDTIME     8903
#define IDTIMER_TOOLTIPTIMER    8905
#define IDTIMER_EFFECTINFO      8906
#define IDTIMER_VIRTKEYBD       8907
#define IDTIMER_TRIGGERBALLOON  8908
#define IDTIMER_CLOSEBALLOON    8909
#define IDTIMER_SETSIZEHACK     8910

#define ID_HTTP_SERVER          8980
#define ID_HTTP_SOCKET          8981


/* [C]ontrol [ID]s, IDC_*, used in dialogs, may be mixed with the
 * other IDs
 */
#define IDC_MAINFRAME           9000
#define IDC_PREVDLGPAGE         9022
#define IDC_NEXTDLGPAGE         9023
#define IDC_HTMLWINDOW          9055
#define IDC_BUTTONBARMENU       9056
#define IDC_MODULE00            9081 /* range start */
#define IDC_MODULE99            9180 /* range end */
#define IDC_MODLIST             9191 /* global! */
#define IDC_SKINUPDATELIST      9192 /* global! */
#if 0
#define IDC_MODUPDATELIST       9193 /* global! */
#endif
#define IDC_NOTEBOOK            9197 /* global! */
#define IDC_IDXLIST             9198
#define IDC_IDXADDSOURCES       9199
#define IDC_IDXDELSOURCE        9200
#define IDC_IDXCONFIGSOURCE     9202
#define IDC_IDXCONFIGSOURCEMENU 9203
#define IDC_IDXEXPLORE          9204
#define IDC_IDXUPDATEMENU       9206
#define IDC_NEWSEARCH           9207
#define IDC_SAVESEARCH          9208
#define IDC_SAVESEARCHAS        9209
#define IDC_RENAMESEARCH        9210
#define IDC_DELETESEARCH        9211
#define IDC_ENDADVSEARCH        9212
#define IDC_REVERTTOSAVEDSEARCH 9213
#define IDC_EFFECTSLOTOPT00     9214 /* range start */
#define IDC_EFFECTSLOTOPT09     9224 /* range end */

#define IDC_PLUGIN_FIRST        9225 /* range start */
#define IDC_PLUGIN_REPLACE      9226
#define IDC_PLUGIN_RENAME       9227
#define IDC_PLUGIN_SPLIT        9228
#define IDC_PLUGIN_FREEDB       9229
#define IDC_PLUGIN_LAST         9250 /* range end */

#define IDC_HOOK_FIRST          9260 /* range start */
#define IDC_HOOK_LAST           9299 /* range end*/


/* [MOD]ule [M]e[S]sa[G]e [ID]s, IDMODMSG_*
 * send via a SjModuleSystem::BroadcastMsg() from silverjuke --> all modules
 */
#define IDMODMSG_FIRST                              11000 /* range start */

#define IDMODMSG_WINDOW_ICONIZED                    11000
#define IDMODMSG_WINDOW_UNICONIZED                  11001
#define IDMODMSG_WINDOW_CLOSE                       11004 /* sent if the program is about to terminate; objects are valid, player is already stopped, window is already hidden */
#define IDMODMSG_WINDOW_SIZED_MOVED                 11006
#define IDMODMSG_WINDOW_BEFORE_CLOSE_HIDE_N_STOP    11008 /* send a moment before IDMODMSG_WINDOW_CLOSE is send; objects are valid, player is still playing, window is still visible */

#define IDMODMSG_APP_ACTIVATED                      11010
#define IDMODMSG_APP_DEACTIVATED                    11011

#define IDMODMSG_SETTINGS_CLOSE                     11020 /* sent if the settings dialog is about to be closed, needed only for very special tasks */
#define IDMODMSG_ADV_SEARCH_CONFIG_CHANGED          11021 /* sent if adv. searches are created/deleted/modified */

#define IDMODMSG_TRACK_ON_AIR_CHANGED               11030 /* also send if only the player state changes */
#define IDMODMSG_VIDEO_DETECTED                     11031
#define IDMODMSG_VIS_STATE_CHANGED                  11033
#define IDMODMSG_KIOSK_STARTING                     11034
#define IDMODMSG_KIOSK_STARTED                      11035
#define IDMODMSG_KIOSK_ENDING                       11036
#define IDMODMSG_KIOSK_ENDED                        11037
#define IDMODMSG_PLAYER_STOPPED_BY_EOQ              11038 /* ONLY send if the player was stopped by end of queue! */

#define IDMODMSG_SEARCH_CHANGED                     11040 /* also send if the search is cleared */

#define IDMODMSG_MODULE_DISABLED                    11050 /* send if a DSP or output module is disabled, may be the module wants to close modeless dialogs after this message */

#define IDMODMSG__VIS_MOD_PRIVATE_1__               11051
#define IDMODMSG__VIS_MOD_PRIVATE_2__               11052
#define IDMODMSG__VIS_MOD_PRIVATE_4__               11054

#define IDMODMSG_PROGRAM_LOADED                     11060 /* send to indicate, the program is loaded and the main window is shown*/

#define IDMODMSG_LAST                               11999 /* range end */


#endif /* __SJ_IDS_H__ */
