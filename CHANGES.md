Silverjuke Changes
================================================================================


V15.? (23.11.2015)
================================================================================

- New: Read and write ratings from MP3, MPC, FLAC and OGG-files


V15.9 (13.11.2015)
================================================================================

- New: "Limit Play Time" option
- New: The settings dialog opens with most recent page used
- New: INI-Switch "debug", see file "user-guide"
- Dialog "Automatic control/Further options" moved to a page in the main dialog
- Menu and settings dialog cleanup
- Bug fixes


V15.8 (27.10.2015)
================================================================================

- Play non-DRM AAC- and ALAC-m4a-files if supported by the operating system
- Bug fixes


V15.6 (14.10.2015)
================================================================================

- New: Add three more default skins
- New: Option to browse the help files locally
- Update artist info and cover search  URLs
- Bug fixes


V15.4 (26.06.2015)
================================================================================

- Silverjuke core made open source, see github.com/r10s/silverjuke
- Source and feature cleanup, scripting disabled
- New: "My music library/Source options/Read hidden folders", defaults to "on"
- New: Read/write support for XSPF (pronounced "spiff") playlists
- New: Read support for iTunes playlists (*.xml)
- New: Read support for Windows Media Player playlists (*.wpl)
- New: Scripting: Dialog.show('saveplaylist')
- New: Virtual keyboards: "alt" keys may be locked
- New: Virtual keyboards: Allowing up to 32 different "alt"-keys
- New: The search field allows searching for single words, eg. "Stones Black"
  will find "Rolling Stones - Paint it Black"
- New: "Settings/Advanced/Further options/Search: Search single words"
- New: New AutoPlay feature: Ignore tracks from music selection (eg. from "Worst
  rated")
- New: New "Change password" button at "Settings/Kiosk mode/Start"
- New: Covers may contain unicode characters
- Playlists handling on network drives improved
- Using the default font for the Video/Karaoke/Visualization title
- Search: No longer searching for genre names by default; please use the context
  menu for this purpose or enable the feature manually at "Settings/Advanced/
  Further options/Search: Lookup genre on simple search"
- Fixed a bug with compressed ID3v2 frame headers
- Fixed a bug when loading skins
- Fixed a bug that prevents timers from unloaded scripts being stopped
- Fixed: Memory leak in "Settings/View/Skins/Menu/Update list"
- Fixed a problem with bad values assigned to program.repeat


V2.75-V3.05 (2009-2011)
================================================================================

- Special and testing versions not belonging to the Silverjuke core


V2.74 (09.04.2009)
================================================================================

- New: Allowing the cursor keys to be assigned to any command; same for other
  basic keys
- New: Scripting: prelisten.volume property
- encodeURI() handles Unicode strings correctly
- Fixed a bug when redrawing skins


V2.73 (04.01.2009)
================================================================================

- New: AutoPlay regards the "Avoid boredom" settings
- New: Music selection: Playlists can be used for include/exclude rules
- New: font-attribute in box-tags
- New: Scripting: Program.visMode
- New: Scripting: Player.avoidBoredom
- New: Scripting: Program.getSelection() can return workspace or display
  selections independently
- New: Display: Tracks moved due to the "Avoid boredom" settings are marked
- New: Display: Silverjuke shows the track cover now, see "Settings/Advanced/
  Further options/Display: Preferred cover"
- New: New Option "Further options/Image cache: Regard file chances": If set,
  covers will reflect changes made with external applications
- New: The command line parameters "--instance" and "--ini" may be combined with
  other command options
- New: Support for UTF-16 encoded karaoke files and playlists andded
- Skinning: Different input-items in different layouts can have different font
  colours and sizes
- Skinning: If-Tags: The condition "creditsystem" also works on startup
- Music selection: When using the "is set" operator, the unit choice is always
  hidden
- Music selection: No more crashes when using multiple include/exclude rules
- List view: Improved handling of the cursor keys
- Fixed a bug with shaped skins returning from kiosk mode
- Scripting: Calling Player.stopAfterThisTrack or Player.stopAfterEachTrack
  updates the belonging button states


V2.72 (05.10.2008)
================================================================================

- New: Large cover view: Alphanumeric sorting of the cover list


V2.71 (29.09.2008)
================================================================================

- New: Karaoke files can be used from within ZIP archives


V2.70 (28.09.2008)
================================================================================

- New: New option "Playback/Queue/Remove played tracks from queue"
- New: New option "Playback/Queue/Avoid boredom: No artist repetition"; this is
  also regarded by "Avoid double tracks waiting in queue"
- New: New option "Playback/Automatic Control/Further options/Reset view after..
  minutes of inactivity to .."
- New: "Kiosk mode/Functionality/Toggle elements": This option allows or forbids
  users to toggle covers, enlarge displays, change columns etc.  Replaces
  "Enlarge display".
- New: "Kiosk mode/Functionality/Toggle time mode", "Search for genre",
  "Unqueue" and "Zoom"
- New: "Further options/Kiosk mode: Maintenance password": When using a
  password to exit the kiosk mode, an additional password can be defined here.
  Entering this password, only the kiosk mode is ended, without eg. a shutdown.
- New: New option "My music library/Add source/Add a server containing music
  files"
- New: New karaoke background option: "Use *.bg.jpg-files"
- New option: "Stop after each track", a shortcut can also be defined for this
- New: Cover view: Artist and album names can be shown below the covers (use the
  little triangle in the upper left corner)
- New: Large cover view: After a little time, the enlarged cover is updated if
  "Go to current track after .. minutes of inactivity" is enabled or if the
  cover belonging to the current track was opened
- New: Genres, comments and ratings may be shown in the tracklist, see
  "Settings/Advanced/Further options/Tracklist: Show genre", "Show comments",
  "Show rating"; for longer comments, only the first 40 characters are shown
- New: New command line and DDE option --execute
- New: New option "Kiosk mode/Functionality/Repeat"
- New: New skin button-target "removePlayed"
- New: A shortcut can be assigned to "Remove played tracks from queue"
- New: Support for M3U8-playlists
- Tag editor/Split field: Empty destination fields are okay
- Large cover view: Sooner update if the cover belonging to the current track
  was opened
- Large cover view: Reproducible sorting order of the cover list
- Fixed an issue with disappearing skins
- Kiosk mode: Enqueuing multiple tracks from the cover view is only possible if
  the option "Enqueue multiple tracks" is selected
- Kiosk mode/Exit by clicking into two different corners:  Only clicks not used
  by the skin are consumed
- Save playlist: Remembering the last playlist format used
- Showing more music selections and genres in the search menu
- "Goto random" selects tracks in album and cover view
- Numpad: The "Cancel" key may be used to correct entered numbers
- Larger slider range for "Settings/View/Column width" and for "Settings/View/
  Font size"
- Preserving manually entered genre names when updating of WAV-files
- Scripting: Remembering Program.autoPlay, Program.sleepMode and
  Program.sleepMinutes between program starts
- Scripting: File.pos with negative values work as expected
- New: Scripting: Player.removePlayed
- Scripting: Scripts can be started by a double click to *.sj-files or by
  dragging  *.sjs-files to the main window
- Scripting: Proper order for added menu entries and buttons
- Scripting: Shortcuts can be defined for the script's menu entries, see
  "Settings/Advanced/Further options/Shortcuts: Tools" if scripts are loaded
- New: Scripting: HttpRequest object
- New Scripting: Program.autoPlay, Program.lastUserInput, Program.layout,
  Program.onUnload, Program.selectAll(), Program.setSkinText() and Program.zoom
- Scripting: Program.refreshWindows() can be used to update the workspace
- Scripting: Dialog.setValue() works for static text controls
- Scripting: Dialog.addButton() accepts any label for "ok" buttons
- Scripting: Dialogs use the virtual keyboard, if needed
- New: Scripting: Player.repeat, Player.shuffle, Player.stopAfterThisTrack and
  Player.stopAfterEachTrack
- Scripting: Player.addAtPos() does no longer start playback implicitly; use
  Player.play() for this purpose.
- New: Scripting: Rights.zoom
- Scripting: Rights.credits, Rights.editQueue, Rights.unqueue,
  Rights.multiEnqueue are writable
- New skin condition "zoom"
- New skin attribute for box-tags: "id"
- New skin button-target: "stopAfterEachTrack"
- New skin option: h="opposite" assigns the height of a box to its width, also
  works for w="opposite"
- In skin "Silveriness": When showing the display only, the display can still be
  enlarged
- In skin "Silveriness": The transport buttons are available in all layouts
- Better support of some VST plugins that do not follow the specification
  correctly
- Menu entries with the character "&" in their names are displayed correctly
- Unsupported characters in M3U-files are converted to the question mark
- LRC files in the lrcdb-format supported
- Spaces in Wikipedia-URLs handled correctly
- Fixed a visualization/miniplayer bug
- Correcting some details of VU meters etc. displayed by VST plugins
- Cover view: Fixed a bug that hides all covers under some circumstances
- Fixed a bug when renaming music selections was cancelled
- Album view: Fixed a sort order bug
- Karaoke background images: Fixed a bug that crashes Silverjuke when selecting
  a folder without images
- Tag editor: Fixed a bug when editing the genre/group of the currently playing
  FLAC track
- When dragging tracks to the display, the correct cursor is shown again (bug
  introduced in 2.50)
- The very first track in shuffle mode becomes "marked as played"
- Quotes in music selection rules should work in any case now


V2.52 (22.06.2008)
================================================================================

- This is a special version for all c't magazine readers
- Congratulations to 25 years of c't from the Silverjuke Team!


V2.51 (18.01.2007)
================================================================================

- Kiosk mode monitor settings: Bug fixed for Windows 98


V2.50 (16.12.2006)
================================================================================

- New: New Karaoke options: Silverjuke shows the lyrics from CDG and LRC files
- New: New "Open playlist" dialog
- New: Dual head options: The karaoke/visualization screen may be shown on a
  separate display (see "Settings/Kiosk mode/Monitors")
- New: Microphone support
- New: New option "Kiosk mode/Avoid double tracks waiting in queue"
- New: New speaker assignment options, see "Settings/FX/Menu/Setup/Speakers"
- New: New console, see "Tools/Console" in the menus
- New: New options "Disable screensaver" and "Disable power management" at
  "Kiosk mode/Monitors/"
- New: New option "Kiosk mode/Functionality/Enqueue multiple tracks"
- New: New option "Playback/Automatic Control/Manually enqueued tracks interrupt
  AutoPlay immediately"
- New: New option "Read hidden files" in the source options of a folder,
  defaults to "on"; this is useful eg. for covers saved by the Windows Media
  Player with the "hidden" attribute
- New: New option "Settings/Advanced/Further options/Display: Show AutoPlay"
  which prepends automatically enqueued tracks in the display by the string
  "AutoPlay"
- New: Separate FX settings for output, prelisten output, microphones and
  AutoPlay
- New: Numpad: A full album can be enqueued using the track number "00"
- New: Scripting
- New: MP4 tags and embedded covers are read
- New: Option "Settings/Advanced/Further options/Menus: Use 'view' font" moved
  to "Skins/Fonts"; the option affects menus and dialogs
- New: New option "Settings/Advanced/Further options/Start playback on enqueue"
- New: Scrolling long titles in the default visualization
- New: The mouse cursor can be hidden in kiosk mode, see "Kiosk mode/Virtual
  keyboard"
- List view improved
- If the mouse is over the "A-Z" buttons, the wheel scrolls one letter up/down;
  if the mouse is over the scrollbar, the whell scroll one page up/down/left/
  right (can both be disabled at "Settings/Advanced/Further options/Mouse wheel:
  Context-sensitive")
- Goto next/prev letter works if no "A-Z" buttons are defined
- The default freedb server is "freedb.freedb.org"
- Added case-insensitivity for german umlauts and special characters of other
  languages
- Corrected the forwarding of UTF-8-encoded strings to the web services for
  cover and information search
- If not yet done, existing databases are automatically converted to UTF-8
- Fixed a bug that lead to crashes when saving ID3-tags with bad genre data
- Different decoders that use the same extensions (eg. AAC and ALAC both use
  .m4a) can be used at the same time now
- Allowing lower case Xiph-comment identifiers in Ogg-tags
- Avoid saving covers to themselves
- The visualization/karaoke screen may be placed anywhere outside the workspace
- Shortcuts can be defined for "Seek Forward" and "Seek backward"
- Remembering different zoom settings for the different views by default; this
  behaviour can be disabled at "Further options/View: Same zoom in all views"
- For a better kiosk mode safety, we hide the taskbar and disable the "scroll
  lock" key
- "Use hardware" is no longer enabled by default
- "Use system volume" options moved to "Settings/Advanced/Further options"
- "Automatic volume control" and "Fading" moved to "Playback/Automatic control"
- New skin target: Goto next/prev letter, "currTrack", "currTime", "currCredit"
- New skin attributes: "text", "hideCreditInDisplay", "visAutoStart"
- New skin condition: "creditsystem"
- Skinning: The text size in the search input controls scales to fit the real
  size of the control
- Better recognition of visualization DLLs
- Numpad disabled outside kiosk and enable in kiosk works correctly
- Fixed a bug in the delay parameter of the effect "Echo 2"
- A click on "OK" in the virtual keyboard starts the search if "Settings/
  Advanced/Further options/Search while typing" is off


V2.03 (31.07.2006)
================================================================================

- Fixed some bugs in the automatic volume detection for erroneous files


V2.02 (28.07.2006)
================================================================================

- The "Euro" sign is rendered correctly in the virtual keyboards
- The "Select cover" context menu shows all possible covers from all album
  tracks
- Embedded images are read correctly from deprecated ID3 tags
- Sometimes the year was read incorrectly for deprecated ID3 tags; fixed now
- The "BPM" field is displayed correctly in the list view now
- Bug fixed when writing Ogg-Vorbis tags


V2.01 (14.06.2006)
================================================================================

- Fixed a little problem with the Unicode stuff
- Fixed a bug in the silence detection


V2.00 (13.06.2006)
================================================================================

- New: Mac OS X support
- New: Unicode support
- New: Two new views - please try the new icon below the vertical scrollbar
- New: Completely rewritten and improved sound processing:
- Native or system-based support for MPx, Ogg-Vorbis, FLAC, Musepack, WAV, WMA,
  AIFF, Monkey's Audio, WavPack and MOD playback added or improved
- For a better sound quality, we now rely on the hardware whereever possible;
  moreover, all calculations are always 32-bit now
- Less latency
- New effect section for internal effects and for VST effects added to the
  settings dialog
- Shoutcast and Icecast support added, see "Open playlist or files/Open URL"
- New context menu command "Fade to next"; as an alternative, hold the shift key
  while pressing eg. the "next" button
- New crossfading option "Only fade out, start new track with full volume";
  dialog "Playback settings/Fading" improved
- New: Mouse and keyboard improvements:
- The middle mouse button can be configured to edit or prelisten tracks now (see
  "Settings/Advanced/Further options/Middle mouse button")
- Shortcuts defined for the action "Unqueue marked tracks" will work
- More options for adding credits by shortcuts (see "Settings/Kiosk mode/Credit
  system")
- All cursor and workspace shortcuts can be changed at "Settings/Advanced/
  Further options" now, adding targets for pagination to the skin engine
- Less flickering on object dragging and on dialog resizing
- The font defined at "Settings/View" can now also be used for the popup-menus;
  see "Settings/Advanced/Further options/Menus: Use 'view' font"
- New: Further changes:
- New command line options --volup, --voldown and --togglevis
- New command line parameter "--kioskrect"; this can be used to let Silverjuke
  only use some parts of the screen in kiosk mode
- New skin condition: "version"
- New option in the playback context menu: "More from current album/artist"
- Added amazon.com and jpc.de to the german cover search (thanks to timewind)
  and lyricwiki.org to the lyrics search
- Reading (ID3-)tags can be disabled per source at "My music library/Source
  options"
- Some optimizations make the database about 15% smaller; as the cache is better
  used for this reason, this also results in some speed improvements, eg. on
  search
- The pages "Buy", "Language" and "Further options" are grouped under "Settings/
  Advanced" now; module settings are available at "Further options"
- If the tag information is read from the file name, we now allow the character
  "." there; however, the last point always introduces the file extension
- All searches are switched off before changing tag data that may affect the
  sorting and before a database update


V1.22 (01.03.2006)
================================================================================

- New: Prelistening is directly available for tracks in the playlist
- New: The action to perform when terminating the kiosk mode can now be
  predefined or can be queried independently of a password
- New: New command line parameter "--update" which will update the index on
  startup
- For display selections, the context menu option "View/Go to marked tracked" is
  shown in addition to "View/Go to current track"
- The kiosk mode password must be entered on exit even if "all functions" is
  enabled
- Fixed a bug that crashes Silverjuke sometimes on exit if the kiosk settings
  dialog is still open
- Track numbers are read correctly from UTF-16 ID3-tags
- German umlauts are read correctly from UTF-16 ID3-tags
- Fixed a problem if the key to exit the kiosk mode was still hold in the
  shutdown period
- Improved loading of Winamp 5.2 modules
- MilkDrop 1.04d that comes with Winamp 5.2 is currently incompatible to
  Silverjuke due to undocumented API calls and is not loaded therefore


V1.20 (01.02.2006)
================================================================================

- New: The AutoPlay function can also play tracks from the current view (see
  "Playback settings/Automatic control/AutoPlay")
- New: Credits may also be added by shortcuts (see "Settings/Kiosk mode/
  Credit system")
- New: In skin "Silveriness": New button "0-9"
- New target for the skin engine: "goto0" which refers all albums not starting
  with a letter between A to Z
- "Go to current track" only ends the simple search if needed
- "Play track next" works as expected if the shuffle mode on on
- Fix a bug when terminating the kiosk mode when ctrl-alt-del is disabled
- No more "beeps" when entering the kiosk mode with ctrl-alt-del disabled
- The tag editor is only available in the kiosk mode if the functionality is set
  to "All functions"


V1.18 (09.12.2005)
================================================================================

- New: The location and the max. size of the temporary directory can be defined
  in "Settings/Advanced/Further options"
- New: http-files are cached to the temporary directory
- New: Each decoder module may be preferred for local file/http playback
  independently; by default, Winamp modules are preferred for local file
  playback and the ACM-MPx module is preferred for http playback
- New option "Kiosk mode/Numpad/Also use numpad controls outside the kiosk mode"
- Unknown total times are no longer shown as "?:??" in the display
- Speed improvements when adding files to the queue
- Fixed a bug that avoids removing the last source from the library


V1.16 (20.11.2005)
================================================================================

- New: Prelistening
- New: Tag editor: Option to query the freedb online database added, see the
  "Menu" button
- New: A balloon with information about the current track can be shown in the
  system tray, see "Settings/Advanced/System tray"
- New: Sleep mode functionality added to "Settings/Playback settings/Automatic
  control"
- New: If the kiosk mode is protected by a password, you have the option to
  shutdown or to reboot the computer on exit
- New: Albums may be created by directories and the album order may be changed,
  see "Settings/My music library/Combine tracks to albums"
- New: Credit system support
- Option "Playback settings/Crossfade/Options/No crossfades between subsequent
  tracks of the same album" added (enabled by default)
- Option "Settings/Advanced/Further options/Mouse: Double click on covers" with
  the settings "Open cover editor" (default) and "Select/play album" added
- The cover and the tag editors may also be opened with the middle mouse button
- Unused system accessibility features are disabled in kiosk mode to prevent
  system dialogs coming up; used accessibility features are still available
- Shutdown handling for the kiosk mode improved, new option "Settings/Kiosk
  mode/Start kiosk mode/Disable shutdown" added
- New functionality switch for the kiosk mode: "Music selection"
- If needed, playing times are corrected after the first continuous playback
- For the mouse wheel, Silverjuke now also checks, if the mouse is over the
  volume slider or over a scrollbar (requires "Further options/Mouse wheel:
  Scroll display" to be set)
- "My rating" and "Artist on the web" are available directly from the system
  tray icon
- The prior searches, music selections and genres are accessible directly from
  the search menu now (use eg. the little arrow right of the search control)
- New command line parameter "--instance=INI-file"
- New skin condition: "shaped", else-tag added
- The kiosk mode option "Query settings before start" works as expected
- Fixed a bug that avoids ID3-tag-reading under some circumstances
- Fixed a bug that hides some tracks under some circumstances


V1.14 (24.08.2005)
================================================================================

- New: When dragging a cover from an external program to an album row or when
  using the clipboard, the user has the option to copy it to the album directory
  now; moreover, a thumbnail of the cover is displayed in the message box
- New: Context menu option "Playback/Play now on double click" added; the
  predefined shortcut for this option is "ctrl-d"
- New: Automatically played tracks (see "Settings/Playback/Automatic control")
  can be skipped by pressing "next"
- Tracks may be played directly from the music selection dialog (see the "Menu"
  button in the dialog)
- The display in the kiosk mode may be optionally enlarged, see "Settings/Kiosk
  mode/Functionality/Enlarge display"
- If "Settings/Kiosk mode/Functionality/All functions" is enabled, the music
  selections work as expected
- Some confirmation dialogs (eg. enqueue, clear playlist, rating, exit) can be
  be disabled (see "Settings/Advanced/Further options" and the dialogs
  themselves)
- If a track is enqueued multiple times, all queue positions are shown in the
  tooltips and in the "Get info" dialog
- The current layout (eg. the size of the display) and all window positions are
  remembered and reloaded on the next Silverjuke startup
- If "search covers on the web" is called for the first time, the online help is
  displayed and some tips are shown
- Better error handling when loading invalid skins
- Automatically started and manually stopped visualizations stay stopped in any
  case
- Funny: The tooltip "Exit Silverjuke" was shown for the horizontal scrollbar...
  fixed


V1.10 (31.07.2005)
================================================================================

- New: Added the possibility to control Silverjuke with the numpad or special
  hardware buttons (see "Settings/Kiosk mode/Numpad")
- New: Sources may be excluded from the update process
- New: In shuffle mode, already played tracks may be played again by removing
  the little check mark in front of the title in the display
- New: Online help improved
- Removing tracks from the queue regards the shuffle, repeat and autoplay states
- If the player is stopped because of the end of the queue and kiosk, repeat and
  autoplay are disabled, the queue is reset to the first position
- A double click on an already enqueued track will just enqueue it again
  (instead of enqueueing it again).  To unqueue songs, use the display.
- Using the display instead of a message box to show the message "Queue full" in
  kiosk mode
- By default, the built-in visualization shows the oscilloscope and the spectrum
- Automatic detection of Nero improved
- Language detection improved
- If any decoder module cannot find out the correct playing time, it is saved by
  Silverjuke to the database after the first continuous playback
- If the kiosk mode is started, the settings dialog is closed under all
  circumstances
- As ACM cannot play MPEG Layer I/II files by default, decoders other than ACM
  are preferred (if available); otherwise, a detailed error message is shown
  when trying to play a MPEG Layer I/II file
- If an ID3 copyright message contains quotes, this halted the database update
  from time to time; fixed (thanks to jobo)
- Bug fixed: Files with the character "#" in the path or in the file name can be
  read
- Bug fixed: Sometimes, Winamp modules that do not use the output were loaded
- Bug fixed: Sometimes, already unqueued tracks were still marked in the album
  view


V1.05 (07.07.2005)
================================================================================

- Improved the loading strategy for Winamp input modules as, under some
  circumstances, eg. the WMA plug-in crashed together with Silverjuke


V1.04 (04.07.2005)
================================================================================

- New: Added a virtual keyboard which is useful esp. in combination with
  touchscreens or if no physical keyboard is present (see "Settings/Kiosk mode/
  Virtual keyboard")
- New: Silverjuke can automatically play songs if the playlist is empty, can
  follow the playlist or can start/stop the visualization automatically, see
  "Settings/Playback/Automatic control"
- New: French language support added (thanks to Claude Sturzel)
- The cover size can be adjusted independently of the column width (see
  "Settings/View/Font and cover")
- After the player is stopped and started again, already played tracks in the
  playlist are no longer played again if "repeat" is off; please enable the
  repeat function to get this behaviour
- Music selections can be opened directly from the kiosk functionality page
- In skin "Silveriness": If wanted, the kiosk mode shows the visualization,
  play, prev/next and volume controls
- Skin outlines are no longer shown when loading an erroneous skin, only an
  error message is printed
- Visualization is stopped if one of the "A-Z" buttons is clicked
- In kiosk mode, the visualization is stopped when clicking anywhere into the
  visualization window


V1.01 (04.06.2005)
================================================================================

- New: Added an automatic version check; the version check is disabled by
  default, use "Settings/Advanced/Further options" to enable it
- Fixed a bug that halts playback under some circumstances
- Fixed a bug that unloads VST effect modules after a database update
- Better support for unusual bitrate/samplerate combinations in the ACM decoder


V1.00 (25.04.2005)
================================================================================

- New: First official release of Silverjuke
- New: Tracks in the queue may be moved
- New: Improved selection handling in queue display (ctrl-click, shift-click)
- New: More context menu options for tracks in the queue ("Edit tracks", "Burn
  CD", "Unqueue marked tracks", "Unqueue all but marked" etc.)
- New: Option to switch the screen resolution in kiosk mode
- New: Option "Control/Go to current track" added
- New: Support for VST modules added
- Better output quality: automatically switching to 32-bit float samples if
  needed by DSP modules; if this is not needed, we work with faster 16-bit
  integer samples
- Better output quality: to avoid clipping when working with 16-bit integers,
  the automatic volume and the main volume are adjusted in only one step
- Resuming DSP buffers on previous/next
- Enlarged covers are centered correctly in kiosk mode
- Editing covers is no longer possible in kiosk mode
- Also matching image names against album names when searching for covers
- Removing the correct registry entry when removing a source
- Using a popup menu instead of a dialog to select the source type to add
- Options "Open cover" and "Search covers on the web" splitted into two menu
  entries
- Enlarging the display cover works if covers in the browser are disabled
- The state of the mute button (if present in a skin) is set correctly on volume
  changes
- New kiosk mode icon
- No longer showing errors, if non-privileged users don't have access to the
  default Winamp directory
- Vertical scrollbar updated correctly when clicking on "A-Z"
- Effect modules are inserted into "effect slots", this also allows to define
  the order of effects to process
- Options for active effects are also available in the main menu at "Playback"
- Longer drag'n'drop operations do no longer interrupt the calling application
- Public homepage access; from now on, the beta-test forum is available for
  registered users only


V0.32 (08.04.2005)
================================================================================

- New: New default skin "Silveriness"
- New: New jingle "Silverjuke is running" (thanks to Chester)
- New: Tag editor: options to rename files and to split fields added
- Placeholders are surrounded by "<" and ">"
- A double click on an item in "Settings/Advanced/Further options" always opens
  the context menu if there are more than two options present
- Optimized the icons to work with non-standard colors
- Bug fixed that lead the tag editor confirmation dialog crash in some cases
- Bug fixed when selecting tracks using shift-up
- The dialog "Automatic volume" shows more details about the current volume
- Updated the screenshots on the homepage
- Bug fixed for skins using relative positions


V0.30 (21.03.2005)
================================================================================

- New: Tag editor: replace dialog added
- New: Tag editor: detailed confirmation dialog added
- New: Burning CDs simplified and improved
- New: Option "Automatic volume control/Play all tracks of an album with the
  same volume" added
- New: Clipboard support for pasting covers, music files or directories
- New: When dropping a folder, the user is asked whether to add it to the
  database
- When opening or dropping files that are not in the database, the (ID3-)tags
  and the playing time for these files are read and shown correctly
- Allowing bitmap objects for drag'n'drop and clipboard (in earlier versions
  only existing bitmap files were supported)
- Covers may be copied or moved on drag'n'drop (use the "shift" key); on copy,
  they're copied to the directory the first track of the album belongs to; copy
  is the default action when using the clipboard
- Avoid loading Winamp modules that say, they use the default audio output but
  finally don't
- Some interface options added to "Advanced/Modules/Load modules"
- Oscilloscope: added a spectrum analyzer; some more changes and improvements
- Stopping visualization when entering kiosk mode
- Better embedding of visualizations
- When changing (ID3-)tags of the currently played file, it will be paused for a
  second to allow the file modification; same if the file is renamed
- Option to write (ID3-)tags move from the source options to the tag editor
- Handling improved when Silverjuke is about to terminate and started again in
  these moments
- Allowing "short" paths in playlists
- Giving volume-adjusted samples to the visualizations
- Bug fixes when switching from internal to global drag'n'drop (let to program
  hang under some circumstances)


V0.29 (14.03.2005)
================================================================================

- New: Looking forward when gathering automatic volume information; this leads
  to less volume adjustments inside a track
- New: Automatic volume information are written to the database; so no more
  volume adjustments inside a track are needed for subsequent playbacks
- New: Options dialog added for the automatic volume control
- New: Automatic volume may be used as a search criteria for music selections
- New: Covers may be added using drag'n'drop from external applications
- Improved automatic volume in combination with cross fading
- Pressing "Cancel" in the playback settings dialog does no longer restart the
  automatic volume control
- Showing the correct text in the browser for unsuccessful music selections
- Using a higher thread priority for the main audio thread, may be configured at
  "Playback settings/Default audio output/Options"
- A database update is initiated automatically when this version is started
  first; normally, this will only take a few seconds
- Cross fading, automatic volume and output dialogs are modeless
- Opened menus or modal system dialogs do no longer avoid playing the next title
  in the queue
- Fading some milliseconds on play, pause, stop, previous, next and exit
- In prior versions, when clicking previous/next, the first two decoded buffers
  of the new track were forgotten; fixed
- Keeping window size and position when enlarging/shrinking the display; the
  small player view has a completely independent size and position
- "Always on top" can be set individually for every layout; the corresponding
  button in skin "Silverblue" is now available in the small player view only
  (makes no sense in the normal view)
- If there is much silence skipped at the end of the previous track, we allow
  cross fades up to half of the desired length
- Increasing the playing counter for short tracks
- Some bugs fixed in the player for very short or erroneous tracks
- Remembering the browser position and the selected tracks when ending or
  changing a search
- Keeping the cover information when editing tags
- Default output buffer size changed from 185 ms to 92 ms for a better response
  to play, pause, volume etc.; on my slow-testing system (450 MHz), there were
  no buffer underruns
- The decoder buffer size is adjusted automatically; this is done by regarding
  the cross fade and and the automatic volume settings


V0.28 (04.03.2005)
================================================================================

- New: "Automatic volume" implemented
- New: Seeking implemented for ACM decoders
- Corrected erroneous seeking at the end of a track, seeking is only possible
  within a track
- Corrected the title below the search field when switching to/from kiosk mode
- Corrected counting the unplayed titles in kiosk mode if shuffle is not
  enabled; in this case, tracks before the current position will never be
  played, so we do not count them as being waiting
- Remembering the correct browser position after ending a search
- Releasing allocated images correctly in the small cover display
- "Advanced search" renamed to "Music selection" (thanks to "m")
- Partly corrected the english homepage and the help file (thanks to "m")
- Updated the screenshots on the homepage
- Kiosk mode start/stop buttons shown in bold
- Scaling down the drag'n'drop image, if it is very large
- More aggressive silence skipping on track end


V0.27 (27.02.2005)
================================================================================

- New: Shuffle implemented
- New: On program startup, the last music selection is loaded, if any
- New: Tracks can be excluded/included to music selections using "del" or "ins"
  in the main window; drag'n'drop is also supported
- New: More "limit by" options for music selections
- New: Queue position and file type may be used as criteria in music selections
- New: Showing the queue position in "Get info" and in the tooltips, if any
- New: Showing the file type in "Get info"
- New: If the mouse is over the display, the mouse wheel optionally scrolls it
- New: Kiosk mode improved
- Display of music selections in the main window improved: no truncating of
  quotes, hiding the strings "Albums - A" etc. for small results
- Menu option "View/Show covers" is available in all context menus
- When adding tracks to the queue the tracks are shown in bold and the display
  scrolls to the adding position; after a moment the normal state is restored
- Adding some more options for using colours in skins
- For internal drag'n'drop, the images of the selected tracks are optionally
  moved together with the mouse
- Showing the text "no results" in the browser if a search failed; clicking onto
  it will end the search
- Fixed an redrawing error for the vertical text in the browser
- Total time given correctly to Winamp modules
- Date output in "Edit tags/Get info" and other places corrected
- The calender control returns the correct date and time
- Mute does no longer pause playback
- Memory leak fixed when writing ID3 tags
- "Play title now" works in shuffle mode
- No more errors as "Image not found. Source tag is..." when loading a
  correct skin
- If there are no columns in the browser, the thumb of the horizontal scrollbar
  is as wide as scrollbar
- Restarting tracks in queue with "repeat" disabled replays all tracks
- Moved option "Ask on close if playing" to "Advanced/Further options"
- Renamed "Rating" to "My rating"
- Renamed "Playback settings/Further options" to "Playback settings/Shuffle"
- IDs of deleted tracks are never reused again; future versions may depend on
  this behavior, so you may want to delete mymusic.db and create it again;
  however, currently this is not needed
- Music selections are stored in the database instead of registry/ini
- In skin "Silverblue": Button to start the visualizations moved to the display
  area
- In skin "Silverblue": Removed play/previous/next from the small and from the
  kiosk layout (you can use the display for these functions)


V0.26 (18.02.2005)
================================================================================

- Bug fixed in the error logging system which was not really thread-save and
  lead to program crashes under some circumstances; hopefully everything is okay
  now...
- Bug fixed when adding user-defined keys (lead to program crashes)
- Using negative values for image brightness/contrast is okay now
- When enlarging erroneous images the error is displayed


V0.25 (16.02.2005)
================================================================================

- New: "Music selections" (former name was "Advanced search") implemented
- New: Local and system-wide drag'n'drop support from the browser window eg. to
  the display
- New options for drag'n'drop in "Advanced/Further options"
- Command line option "--temp=DIR" added
- Hiliting conflicting decoder modules in "Settings/Modules"
- Sorting of strings that begin with a number corrected
- Tooltips are closed if the mouse is over another window
- Moved all keyboard, mouse, expert and minor settings from different dialogs
  and menus to "Advanced/Further options"
- Improved time display
- Starting the kiosk mode closes all open dialogs
- Using a larger font for the vertical text in the browser
- Help updated


V0.24 (22.01.2005)
================================================================================

- New: Option to force auto-generated covers (see cover's context menu)
- New: Resampling "odd" frequencies as 48 KHz to 44.1 KHz, resampling filter
  added (see "Playback settings/Audio output/Options")
- New: The ACM decoder can play files of all common frequencies
- Oscilloscope: Optionally drawing offscreen to avoid flickering; using font
  from "View settings"; option "Show title" added, "Flash Background" is no
  longer enabled by default (many people thought, this is a bug)
- Auto-generated covers are shown correctly if "Advanced/Further options/Image
  cache/Disk cache" is enabled
- German umlauts and other special characters are shown correctly in auto-
  generated covers
- Added little more space between the options in "Combine tracks to albums"
- Reading MPx headers in files without ID3v2 but with ID3v1 is okay (hit
  shift-F5 to recreate your music library)
- Some english strings corrected (thanks to Thorsten)
- System-wide shortcuts are no longer enabled by default
- The button "My music library/Update music library" now opens a popup menu with
  the options "Update" and "Recreate"


V0.23 (15.01.2005)
================================================================================

- New: Tooltips added
- New: Accelerator handling improved
- New: Improved selection handling in main window (ctrl-click, shift-click,
  shift-cursor)
- Option to enable/disable tooltips added to "Advanced/Further Settings"
- Option "Advanced/Further options/Cursor keys" added
- In skin "Silverblue": Button to append files added to large display view
- In skin "Silverblue": Button to toggle "always on top" added
- In skin "Silverblue": Menu- and minimize-button in small view added
- Previous/next works in the track editor, alternatively the page up/down keys
  may be used (these keys may also be customized in "Advanced/Further options")
- Downloading covers from buy.com should work now
- "-" and other special characters may be entered into the search control
- Bug fixed for hover handling if a click is aborted by moving the mouse outside
  the button rectangle
- English capitalization improved
- If errors occur while loading a cover, these errors are printed at the place
  of the erroneous cover instead of opening a message box.
- Using different subdirectories in temp. directory for different
  configurations/different databases. This is needed as files are stored in temp
  eg. using IDs from the database.
- Initial visualization window position corrected for some visualizations that
  set their own position even when embedding (eg. Milkdrop)


V0.22 (10.12.2004)
================================================================================

- New: First beta release of Silverjuke
- silverjuke.net incl. the beta-test forum opened to some selected people


V0.01 (23.11.2003)
================================================================================

- New: First prototype of Silverjuke
- Start counting the version numbers


Copyright (c) Bjoern Petersen Software Design and Development, http://b44t.com

