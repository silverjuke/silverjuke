Silverjuke User Guide
================================================================================


**Contents**

- First Steps
- My Music Library
    - Combine tracks to Albums
    - Update Music Library
- Using Covers
    - Search Covers on the Web
- Playback
    - Queue
    - Automatic Control and Fading
    - Karaoke
- Skins, fonts and covers
- Kiosk Mode
    - Functionality
    - Monitors
    - Virtual Keyboard
    - Numpad
    - Credit System
- Music Selection
    - SQL Expressions
    - Playlists
- Edit Tracks
   - Replace
   - Split Fields
   - Rename Files
   - Query freedb
- Visualizations
- Advanced and further options


First Steps
================================================================================

After you have downloaded Silverjuke from http://www.silverjuke.net/ and started
the program, you have to add your music files to your music library. For this
purpose, just drag the folders with music files to the main window.

Alternativly, select the page "Music library" in the "Jukebox Settings" dialog.
The (empty) list shows all sources used to build your music library from. Click
on the button "Add source" and select a folder with your music files. Music 
files are read recursively. If your music files are in different folders, you 
have to repeat this step for every folder.

After clicking "OK", Silverjuke starts searching the given folders for music
files recursively. Depending on the number of music files, this may take some
minutes, for further information see "Music Library" below.


**Usage**

We tried to make the usage of Silverjuke as easy as possible - so maybe you can
stop reading here and just try out the program yourself...


**Search for albums and tracks**

To scroll the albums in the main window, use the well-known scrollbars which are
normally placed aright and abottom. You can also use the mouse-wheel for this
purpose - if you hold the right mouse button while using the wheel, Silverjuke
scrolls horizontally.

The letters "A" to "Z" show your position in the album list. Click a letter just
to go to a certain position.

To search an artist, an album or a track, just type the name into the search
field - maybe the first letters of the name are enough. Normally, there is no
need to commit your search - Silverjuke starts searching and displays the result
at once.

If you want to stop a search and view all albums, empty the search field, click
one of the letters "A" to "Z" or just hit "Esc".

See the chapter "Music Selection" for further searching options.


**Sorting**

By default, the albums are sorted by the name of the artist, the year of the
album and finally by the name of the album itself.

If Silverjuke cannot obtain the needed information, the tracks will be found
under the name of the genre or under "Unsorted tracks". In this case you should
edit the tracks and add the missing information (see "Edit tracks" below).

More sorting options are described in the chapter "Music Library".


**Playing tracks**

To play a track, just double-click its name in the album list. Repeat this for
each track you want to listen to and Silverjuke will play the tracks 
successively.

Using the context menu (normally invoked using the right mouse button) you may
also add the tracks to other positions in the queue.

By the way, you'll find some more functions menu and in the context menus - just
try them out.


**Seeking**

If you want to seek inside a track, use the little bar atop of the display (this
is true for the Skin "Silveriness" - other skins may have different approaches).

The bar shows the current playback position inside a track. By clicking on the
end of the bar and dragging the mouse left or right you can seek to another
position.


Music Library
================================================================================

In the "Music Library", you have to tell Silverjuke all sources (folders, files,
(compressed) archives, servers and so on) to read music-files from.

To open the dialog, select "Jukebox settings" from the menu and the the page 
"Music Library".

Using the corresponding buttons, you can easily add or remove sources. Moreover,
you can set several options for each source.

The sources themselves will _not_ be modified by Silverjuke. Removing a source
for instance will just delete the reference to this source, not the source
itself.

If you add a folder as a source, Silverjuke will automatically scan all
subdirectories recursively.


Combine tracks to Albums
--------------------------------------------------------------------------------

To combine difference tracks from different sources to albums, Silverjuke uses
its own algorithm which can be configured at "Jukebox Settings / Combine tracks
to Albums":

- Tracks belong together if the artist name and/or album name of different
  tracks is identical [1]. If this is true for a given number of tracks,
  Silverjuke will assume these to be an album. This given number of tracks can
  be edited using the first value. The default value is 2.

- To avoid splitting samplers or compilations into several pieces by the method
  described above, you can use the second value to define how many tracks of the
  same artist a sampler may have. This value should be larger than the first 
  value. The default value is 4.

Note: You should edit these values only if you really understand what they do -
normally you will be happy with the default values.

The sorting order of artist- and album names can be optimized by defining
stop-words which have to be ignored at the beginning of the name so that these
words will be skipped from the sorting. Eg. if you decide to ignore the word
"the", "The Rolling Stones" will be read "Rolling Stones, The" and their albums
will be found under "R" instead of "T".

Separate the stop-words using commas, the function is case-insensitive. To avoid
omitting special word groups, just add them complete. Eg. if you want to ignore
the german article "die" but leave the band "Die Happy" as it is, type in
"die, die happy".

Moreover, you can decide whether the ignored words should be displayed tailing
or leading the search term - however this does not affect the sorting. The above
mentioned example, "The Rolling Stones" will always be sorted in the "R"-list
but you can decide whether Silverjuke will display "Rolling Stones, The"
(ignored words tailing) or "The Rolling Stones" (ignored words leading).

For using covers, see the chapter "Using Covers".


Footnotes:

[1] The genre may be used as an additional criteria. Moreover, if you really
have only one album per directory, you can also select the directory as the main
sort criterion.


Update Music Library
--------------------------------------------------------------------------------

Silverjuke saves all information on the music-files in its own database.
Normally, this database will be updated automatically as needed, for instance,
if you add a folder or change special options.

This update process may take some minutes for the first time as Silverjuke has
to analyze all music files. You should not abort the process without reasons -
otherwise the Silverjuke database is out of sync with the music files.

If you abort the process anyway or if you change music files outside from
Silverjuke you can start the update process manually using "Jukebox Settings /
Music Library / Update Music Library" or by just hitting the key "F5".

With the option "recreate music library" the music library will be recreated
"from scratch" which may take more time.



Using Covers
================================================================================

When creating or updating your music library, Silverjuke automatically searches
for images embedded in music files (eg. by ID3-tags) and for images in the same
directory as a music file. Silverjuke assumes that one of the pictures found is
useful as a cover.

Currently the file formats BMP, GIF, ICO, IFF, JIF, JPE, JPEG, JPG, PXX, PNG,
PNM, TIF, TIFF, XPM are supported. If several images are found, the correct
cover is identified by one of the keyword defined in "Jukebox Settings / Music
Library / Combine tracks to Albums":

- With the keywords you can define which words should appear in the file name of
  an image that should be used as the correct cover.

- Separate several keywords using a comma, keywords are not case-sensitive, and
  keywords aleft have a higher priority.

- If none of the keywords match to one of the images found, Silverjuke uses the
  first image as the cover for an album.

Example: Assume you have the images with the names "cover_front.jpg" and
"cover_back.jpg" in a folder together with the tracks of an album. If you have
defined the keywords as "front, outside, cover", the image "cover_front.jpg"
will be used as the keyword "front" has the highest priority and matches first


**If no cover is found...**

...Silverjuke creates one of its own ;-) However, you may want to use a more
original using the option Search Covers on the Web. For this purpose, click with
the right mouse button onto the (maybe wrong) cover and select one of the 
options below "Search Covers on the Web" from the context menu. After that,
Silverjuke opens the internet browser with your request. Within the browser,
just save the correct cover eg. into the directory with the album tracks.

When updating your music library next, the new cover should be used for the
album.

Alternatively, you can copy the image to Silverjuke using the clipboard (move
the mouse to the desired album row) or simply using drag'n'drop. However, note
that "drag'n'drop" does not work with all browsers.


**Set covers manually**

The automatic cover recognition may be overrided by you for some albums. For
this purpose, just click with the right mouse button onto a cover and select one
of the images presented under the option "Select cover"; you can also select a
completely different cover using the option "Select cover / Browse".


**Enlarge covers**

In the main window you can zoom the covers together with the track lists using
the keys "+" and "-". The key "*" sets the zoom back to the normal size. To
enlarge a single cover and show it full screen, just double click onto the
cover; this also leads us to the next topic:


**Edit covers**

To edit a cover, double click onto the cover. In the enlarged cover view, you
can use the following options from the context menu (right mouse button):

- You can rotate the cover in 90 degrees step,

- you can mirror the cover horizontally or vertically,

- you can change the brightness and the contrast,

- you can grayscale or negative the cover,

- and finally you can crop it.

For cropping covers, click with the left mouse button onto the upper left corner
of the desired selection, hold the mouse button and move the mouse to the lower
right corner. To remove a cropped selection, use the context menu or hit the key
"C".

Btw. the context menu for editing covers is also available in the main window.


**Options for all covers**

Finally, there are some options that affect all images; you find them in the
context menu of a cover:

- You can smooth images which may increase the image quality. Using the key
  "Alt-S" you may switch this option on and off to get the idea.

- When enlarging covers, you can decide whether the enlarged cover should fit
  to screen or to the main window.

Moreover, using the option use default images, you can reset the manually
selection images back to the images automatically recognized by Silverjuke.


Search Covers on the Web
--------------------------------------------------------------------------------

If you want to search covers on the web, click with the right mouse button onto
the (maybe wrong) cover and select one of the options below "Search covers on
the web" from the context menu. After that, Silverjuke opens the internet
browser with your request. Within the browser, you have the following
possibilities to embed the cover to Silverjuke:

- Clipboard

  Choose the wanted cover in the browser and make sure the image is not too
  small. After that, inside the browser, click the right mouse button on the 
  desired cover and select the option "Copy" (or simelar, however, do not use 
  "Copy link location").  
  Switch to Silverjuke. In Silverjuke, click with the right mouse button on the
  (missing) cover and select the option "Open cover/Past image from clipboard" 
  from the context menu.

- Drag'n'Drop

  Choose the wanted cover in the browser and make sure, both, the browser and
  Silverjuke, are visible on the screen. After that, click and hold the left
  mouse button on the desired image in the browser window and move the mouse to 
  the corresponding album row in Silverjuke. If you want to create a copy of the 
  image in the album's directory, please hold the "CTRL" key while this
  operation.  
  Unfortunately, this method does not work in all browsers.

- Save as

  Inside the browser, click the right mouse button on the desired cover and
  select the option "Save image as..." (or simelar, however, do not use
  "Save target as..." if you do not know exactly that the target is an image of
  higher quality).  
  In the appearing "Save as"-dialog, select a directory (using the folder where 
  the *.mp3 files of the album are placed in is normally a good choice) and hit 
  "OK".  
  In Silverjuke you can open the saved cover explicitly by clicking with the
  right mouse button on a cover and selecting "Open cover/Open file...". If you 
  have saved the image to the album's directory, it will also be used implicitly 
  on the next update of your music library (eg. hit "F5" for this purpose).



Playback
================================================================================

In the dialog "Jukebox Settings / Playback" you can configure different hardware
and software options affecting the playback in Silverjuke.


Queue
--------------------------------------------------------------------------------

- With the option "Kiosk mode: Allow max. ... tracks waiting" in the queue you
  can avoid having too many titles waiting in the queue. If a user tries to 
  enqueue a title in this case, a warning is shown in the display.

- "Kiosk mode: Avoid double tracks waiting in queue"

- "Remove played tracks from queue"

- "Avoid boredom"

- With "Shuffle intensity" you can configure how "randomly" the shuffle mode of
  Silverjuke should be. Moreover, you can define, that Silverjuke should take
  care about not playing one title several times in a given time period. Use the 
  option avoid boredom for this purpose.

With the button "Reset to default values" you can init all options _only_ on 
this page to their default values.


Automatic control and Fading
--------------------------------------------------------------------------------

With the options at "Jukebox Settings / Automatic control" you can define which
actions should be done automatically after given timeouts or at certain
conditions.


**AutoPlay**

You can use the option If the playlist is empty ... to let Silverjuke enqueue
and play tracks automatically:

- With the first input field, you'll define the number of minutes Silverjuke
  should wait on empty playlists before taking further actions.

- After the given number of minutes, Silverjuke plays the number of tracks
  defined in the second input field.

- The tracks to play are chosen randomly from the selected Music Selection in
  the last popup. You may also define your own Music Selections and use them 
  here, more details are found the chapter Music Selections (btw. you can open 
  the "Music Selection"-Dialog directly for here by pressing the button "...").

Finally, some hints to the option "If the playlist is empty":

- If "Repeat" is enabled or if you stop Silverjuke manually, no automatic
  playback takes place.

- The automatic playback flow is interrupted if a user puts a new title to the
  queue. However, the currently playing title will always be played to end.

- If the given number of tracks have been played, Silverjuke waits again the
  given time span; after that, it plays again the given number of tracks. This 
  all is repeated until a user enqueues a track itself.

- If you set the number of minutes to "0", Silverjuke plays further tracks
  immediately if the playlist gets empty. In this case, the number of tracks to 
  play is meaningless.

- The automatic playback also takes plave if you've just started Silverjuke. If
  you've set the number of minutes to "0" in this case, Silverjuke will start 
  playback immediately after the program start.


**Sleep mode**

With these options you can force a specific behaviour at a given time or after a
given timeout.


**Fading**

If you click on the button "Fading...", you can open another dialog where you
can configure several options affecting (cross-)fadings.

The option Automatic crossfades enables smart changes between tracks. This is
done by fading the volume down at the end of the playing track while fading the
volume of the next track up at the same time. The length of the fading process
can be configured using the slider. We recommend values about 10 seconds.

Moreover, you should leave the option skip silence between tracks enabled -
otherwise this effect may just fade in and out silent parts of the tracks and
you won't hear anything.

With the slider beside Manual crossfades you can define the time for a crossfade
initiated manually by you. You can start manual crossfades by using the
(context-)menu option "Playback / Fade to next" or by holding down the "Shift"
key when pressing eg. the forward button.


**Volume control**

In a large music library, many tracks may have different volume levels. If these
tracks are played after each other, the one tracks seems to be louder than the
other one.

To avoid adjusting the volume always manually, you can use the effect
"Automatic volume" which calculates the perfect volume and stores it in your
music library. The effect has the following options:

- With the option desired volume you can set the relative overall volume,
  normally you should leave +0 dB here.

- With the Max. gain you define the maximal amplification that may be used to
  reach the desired volume for a track. If the max. gain is too small, the 
  automatic volume won't work for more quietly tracks; if it is too large, you 
  may notice overdrives if the calculated volume is not yet perfect. Normally,
   you should simply leave the default value here.

- With the adjustment time you define the time that is used for smooth
  volume changes inside a track. Volume adjustments are needed if you play a
  track for the first time and the first assumption of the volume is not perfect 
  and needs to be corrected.

- Finally, if Play all tracks of an album with the same volume is enabled,
  Silverjuke regards all tracks of the album together, so that all tracks of an
  album will have the same calculated gain. You should enable this option only 
  if all tracks of the albums come from the same source, eg. from a CD, so that 
  one can assume any volume changes are wanted by the artist.

To avoid misunderstandings: Low volumes inside a track stay low and loud ones
stay loud. Only the overall volume for the duration of a track will be adapted.
When a new track starts, the volume will be adapted again. However, during the
first playback of a track, the calculated volume may be corrected.


**Further options**

- "Go to current track after ... minutes of inactivity": If there is no user
  input [1] in the given number of minutes, Silverjuke will scroll the album 
  view to the playing album/track. Hint: you may override this timeout by 
  pressing the button "Go to current track" in the main window (usually this 
  button is a "*" beside the "A-Z" bar).

- "Reset view after ... minutes of inactivitiy to ...": Use this option to force
  the given view after the given timeout 

- "Start Visualization after ... minutes of inactivity": If there is no user
  input [2] in the given number of minutes, Silverjuke will start the selected 
  Visualization.

- With the option "Stop Visualization after ... minutes" you can stop a running
  Visualization after the given number of minutes. It does not matter whether 
  the Visualization was started automatically or manually before.


Footnotes:

[1] In this case, "user input" means all mouse clicks and key presses affecting
the album view. Simple mouse movements are no user input in this meaning.

[2] In this case, "user input" means all mouse clicks and key presses affecting
Silverjuke. Simple mouse movements are no user input in this meaning. The
visualization is not started if the Silverjuke main window is not the active
window.


Karaoke
--------------------------------------------------------------------------------

Silverjuke supports playback of Karaoke files in the formats CDG and LRC:

If you have such files, just make sure they are placed in the same directory
under the same name as the belonging *.mp3 files: Eg. if you have a file
c:\music\Karaoke.mp3, the CDG file must be named c:\music\Karaoke.cdg.

That all! To start the Karaoke screen, first start a normal playback of the
file and then click on the little icon in the lower left corner of the display.

On the first start you may have to select "Karaoke prompt" from the menu atop
of the Visualization. You can also place the Karaoke prompt on a secondary
monitor; please see the chapter Monitors for details.


Skins, fonts and covers
================================================================================

With the options placed at "Jukebox Settings / Skins" you can change the 
appearance and some Functionality of Silverjuke.


**Skins**

With so called "Skins" you can change the appearance of Silverjuke completely.
The basic Functionality cannot be changed by skins, however, different skins
may provide different buttons for existing functions.

Skins always have the file extension *.sjs (for Silverjuke Skin). For
installation copy the *.sjs files into one of the search paths defined at
"Advanced / Search paths". 

Btw. there is also documentation about how to create your own skins in the file
"skinning.md" (or "skinning.txt").


**Fonts and covers**

You can define the font face, the font size and the size of the covers
independently (changes are displayed at once in the main window, however, do
not forget to hit "OK" to save them).

The given sizes refer to a zoom setting of 100%. If you change the zoom in the
main window eg. by the keys "+" and "-", the font and the cover sizes change
proportionally. Btw. the key "*" brings you back to a zoom of 100%.


Kiosk Mode
================================================================================

With the so called "Kiosk Mode" you can run the program in full screen mode and
with a limited Functionality. Doing so, you can use Silverjuke eg. as a jukebox
on a party.

Before you start the Kiosk Mode, you should set some options in the dialog
"Settings / Kiosk mode":

First, on dialog page "Start kiosk mode", make sure, you know how to end the
Kiosk mode after you have started it. By default, the Kiosk mode can be ended
using the key F11. Moreover, you can protect the Kiosk Mode by a password and
disable special system keys. If you enter a new password, Silverjuke asks you
to confirm the new password before you start the Kiosk Mode. This is needed to
avoid typing errors.

If everything is set correctly in your opinion, click onto the button Start to
start the Kiosk Mode... we wish you many fun with Silverjuke!


On the dialog pages beside "Start Kiosk Mode" you'll find some options to
configure the Kiosk Mode to fit your needs.

Moreover, you may like to use the options for the "Automatic Control" in
combination with the Kiosk Mode.


Functionality
--------------------------------------------------------------------------------

Under "Kiosk Mode / Functionality" you'll find several options that will alter
the behaviour or limit the Functionality of the Kiosk Mode.

- With the options in the upper area you can disable the named functions for the
  use from within the Kiosk Mode. Depending on the skin in use, the
  corrensponding buttons will be hidden. If a button is not shown even if the 
  function is enabled, you can use the function by a shortcut or by the display.

- The option "Limit tracks to the Music Selection ..." let the Kiosk Mode
  display only the tracks from this Music Selection. You may also define your 
  own Music Selections and use them here, more details are found the chapter 
  "Music Selections" (btw. you can open the "Music Selection"-Dialog directly 
  for here by pressing the button "...").


Monitors
--------------------------------------------------------------------------------

You can use two monitors in the Kiosk Mode of Silverjuke - eg. one for the
Visualization or for the Karaoke screen and the other for the main window.

Just open "Kiosk Mode / Monitors" and define which monitor to use for which 
purpose.

Finally, you can switch the display resolution while the Kiosk Mode is runnning.
This is useful eg. if you find the view too small.


Virtual Keyboard
--------------------------------------------------------------------------------

If you don't have a real keyboard available (eg. if you use a touchscreen), you
may activate the built-in virtual keyboard on the page "Settings / Kiosk Mode /
Virtual keyboard":

- To use the virtual keyboard only if the Kiosk Mode is started, enable the
  option "Use the virtual keyboard"

- To use the virtual keyboard also outside the Kiosk Mode, enable the option
  "Use the virtual keyboard" and then "Also use the virtual keyboard outside 
  the Kiosk Mode".

If you click into the test field or into the search field in the main window
now, the virtual keyboard is shown abottom of the screen.

To close the virtual keyboard, click onto "OK", onto the little "X" in the
upper-right corner or somewhere outside the keyboard window.

Hints:

- The virtual keyboard is not shown if you only set the focus by the tab-key to
  a text field - in this case, we assume you have a real keyboard.

- You may use the virtual keyboard as a normal keyboard, every key pressed will
  be entered to the text field and the shift- and alt-keys will work as usual. 
  To delete the last character entered, klick on the "Delete" key. If you want 
  to delete all characters, hit "Shift-Delete".

- If you protect the Kiosk Mode using a password, the virtual keyboard is also
  shown if you click into the password-entry field.

- The skin "Silveriness Touched" is created especially for touchscreens.

Finally, you may customize the appearance of the virtual keyboard with the
following options:

- With the popup at Virtual keyboard layout you may define which keys appear
  where on the keyboard - just select a layout from the list. If your favourite 
  layout is missing, you can also create virtual keyboard layouts yourself, see 
  the file "virtual-keyboards.md" (or "virtual-keyboards.txt").


Numpad
--------------------------------------------------------------------------------

You may control Silverjuke completly using a numpad (keys "0"-"9") plus some
other special keys for paging left/right and up/down.

To enable the numpad control, select the option "Use numpad controls" at "Kiosk
Mode / Numpad". After that, the album view displays unique album- and 
track-numbers left of the names. If you press one of these numbers (first the 
album- and then the track-number), Silverjuke plays or enqueues the selected
title.

You can use any keys to react to the numpad control, eg. not only "0"-"9". This
should make it possible, to use external hardware devices that emulate 
keypresses. Just click onto "Edit..." to edit the keys.

Some hints to the keys:

- The keys to input "0"-"9" should always be defined.

- Moreover, you should define keys for "Left", "Right", "Up" and "Down". If you
  have not defined these keys, you can also navigage using the numpad's menu
  (see below), which, however, is not very comfortable.

- If you have defined a key for "Down" but not for "Up", pressing the
  "Down"-Button if the album view is already abottom, will bring you back to the
  top.

- If you do not define a key for "Cancel", you can cancel the selection by
  pressing an invalid track number or just a sequence of "0".

Moreover, you can certainly use all other shortcuts available in Silverjuke and
map them to your hardware control, if wanted.


**The numpad's menu**

If you have defined a menu-key or if you hit the key "0" several times [1], you
can enter the menu-mode.

In the menu-mode, the display shows which actions can be done with which keys.
You can switch to the next menu page using the key "0". The available actions
depend on the current state of Silverjuke and on the Functionality you have
allowed for the Kiosk Mode.

To exit the menu, hit "0" until the normal display of Silverjuke appears or hit
any undefined key.

Using the numpad-menu, you can also access the search Functionality: For this
purpose, select the entry "Search" from the numpad-menu as described above.
After that, you can enter the text using the keys "0"-"9" - you may know this
from a mobile phone:

- The display shows with which keys you can enter which characters. To enter a
  character, you have to press the key multiple times. The display does not show
  all possible characters, by pressing a key several times, you can also reach 
  other characters - overview... (see "Text Input Using the Numpad's Menu")

- If you enter only one character, the "go to" function (same as pressing one of
  the "A"-"Z" buttons) is used instead of a search.

- With the key "1" you can delete the last entered character.

- With the key "0" you submit your entry.

- To cancel the process, first delete all characters using "1" and then hit "0".

- Upper-case letters cannot be entered, however, they are not needed as the
  search is not case-sensitive and the password input ignores the case here.

If you have protected the Kiosk Mode by a Password and want to exit it using the
numpad-menu, first select the entry "Exit Kiosk Mode". After that, you have to
enter the password using the keys "0"-"9" as decribed above. Characters already
entered are not shown for security reasons. Moreover, make sure, you use a
password that can be entered this way.


**Text Input Using the Numpad's Menu**

- Pressing the **key 2** multiple times, results in the following characters:

    a b c 0 1 2 ä - + * / . , : ; ! ( ) [ ] { } | @ < > _ = % & ...

- Pressing the **key 3** multiple times, results in the following characters:

    d e f 3 ...

- Pressing the **key 4** multiple times, results in the following characters:

    g h i 4 ...

- Pressing the **key 5** multiple times, results in the following characters:

    j k l 5 ...

- Pressing the **key 6** multiple times, results in the following characters:

    m n o 6 ö ...

- Pressing the **key 7** multiple times, results in the following characters:

    p q r s 7 ß ...

- Pressing the **key 8** multiple times, results in the following characters:

    t u v 8 ü ...

- Pressing the **key 9** multiple times, results in the following characters:

    w x y z 9 ...

Moreover, at the end of each list, you'll find a space. With the key "1" you can
delete the last entered character and with the key "0" you submit your text.


Footnotes:

[1] For each digit in the album numbers, you have to press "0" one time. Eg.
if you have 99 albums or less, you have to enter "00" to enter the menu-mode;
if you have between 100 and 999 albums, you have to enter "000" and so on.


Credit System
--------------------------------------------------------------------------------

With the credit system of the Silverjuke Kiosk Mode you can let the users only
allow to play tracks if they have inserted eg. some coins into any hardware.

To enable the credit system, open the settings dialog and go to the page "Kiosk
Mode/Credit system". There, just enable the options "Use credit system" and
"Credits may be added by external programs".

Done so, you can add credits from external programs or scripts using the command
line or DDE - see the files "command-line.rst" and "dde.md" (or ".txt") for
details about this.

As the external programs or scripts are responsible for communicating with the
hardware, Silverjuke supports eg. any type or coin or or bill acceptors this
way.


Music Selection
================================================================================

Beside the search options described in the First Steps, Silverjuke offers a more
powerful possibility to search for tracks and albums, the so called "Music
Selection". With the Music Selection you can combine different search criteria,
eg. you can search for all titles of a defined genre in a defined period.

You open the dialog for the Music Selection using the right mouse button and by
selecting the menu entry "Music Selection" then.


**Saved Music Selections**

At the dialog's left side you see a list with the saved Music Selections
(starting the program for the first time you will find some predefined
selections by default). With the button menu below this list you can create new
Music Selections, rename existing ones, save a copy of a Music Selection under
a new name and finally you have the option to delete Music Selections.

Hint: If you delete all Music Selections, the predefined selections will be
added again.


**Start and end search**

To search for titles in a Music Selection, select the Music Selection and then
click search. Only titles matching the Music Selection will be shown in the main
window, so that you can verify the result at once. As long as the Music 
Selection is activated each action in the main window will only affect the found
titles, eg. a search using the search field in the main window will search in
the result of the Music Selection only.

Hint: the "Music Selection" dialog is not modal - you can switch between the
dialog and the main window without closing the dialog. Btw. this is true for
most dialogs in Silverjuke.

To cancel a Music Selection click onto the button end search. Alternatively,
you can also cancel the search from within the main window by clicking onto the
"X" aright of the search field (if you have entered any text in the search
field, the first click removes the text and the second click ends the Music
Selection).

Closing the Music Selection dialog does not end the search, so you can use an
Music Selection as a permanent filter without having the search dialog opened
all the time.

Moreover, by the query history, which you access by the little arrow aright of
the search field in the main window, you have the possibility to access prior
Music Selections without opening the Music Selection dialog.


**Edit Music Selections**

To edit a Music Selection, select the Music Selection to edit in the list aleft.
After this, the details about the Music Selection are show aright in the dialog.

Atop of the details, you have the choice whether to select single tracks or full
albums and how to combine the search criteria below. Search criteria always
affect fields in the tracks - you decide to select full albums, missing tracks
of are simply added to the result.


**Edit criteria**

- To add a criterion, click onto "+", to remove one, click onto "-". Hint: there
  must be at least one criterion defined.

- A criterion normally has three parts: the field to search in (first popup), an
  operator for comparison (second popup) and the values to search for (further
  popups or other controls).

- Normally, after adding a new search criterion, you'll first select the field,
  then the operator and finally defined the values to search for.

Hint: depending on the selected files and the selected operator, the operator/
the value controls may change. Eg. the operator "is in range" requires two
values while the operator "is equal to" requires only one.

To get an idea about the possibilities of the Music Selection, the best is to
try it out for yourself. However, there are still some important points:

- If you select the operator "is equal to" or "is unequal to" for a textfield
  (eg. "title"), the upper- and lower-case of the value is important. For other 
  operators, case is not significant.

- date fields (eg. "last played" or "modified") offer the operator "is in the
  last days/hours/minutes". The other operators offer you a calendar control 
  which is reachable using the little arrow aright of the text (choose 
  "select..." from the popup).

- Using the file limit result to you can limit the result to a given number of
  tracks, albums, minutes or MB. Moreover, you have to define the field the 
  tracks should be selected by eg. by "most often played" or by "random". It is 
  okay to use this criterion several times - in this case the limit reached 
  first is used.

- You may include or exclude single tracks by just selecting them in the main
  window and then pressing the keys "Ins" or "Del".

Finally, more experienced users have the possibility to use the field "SQL
Expression" which allows to defined more complex Music Selections in the query
language "SQL".


SQL Expressions
--------------------------------------------------------------------------------

If you choose the option "SQL Expression" for a field in a criterion of a
Music Selection, the operator popup disappears to make place for a larger text
field.

In this text field you can input any valid SQL Expressions as

    rating=3 OR (rating=2 AND timesplayed>2)

See eg. http://en.wikipedia.org/wiki/SQL for details about SQL.

Allwed is everything that is allowed in the WHERE clause; you may also use
subselects.

We do not want to explain SQL in this help file, so here is only a brief
overview about the available fields, operators and functions:


**Fields**

    url
    timeadded
    timemodified
    timesplayed
    lastplayed
    databytes
    bitrate
    samplerate
    channels
    playtimems
    autovol
    trackname
    tracknr
    trackcount
    disknr
    diskcount
    leadartistname
    orgartistname
    composername
    albumname
    genrename
    groupname
    comment
    beatsperminute
    rating
    year
    id


**Operators and Functions**

    =, ==, !=, <>, <, >, <=, >=
    +, -, *, /, %, &, |, ||
    and, glob, in, like, like escape, not, or
    val = abs(val)
    str = filetype(filename)
    val = length(str)
    bool = levensthein(str, pattern)
    dist = levensthein(str, pattern, max_dist)
    str = lower(str)
    val = max(val1, val2, ...)
    val = min(val1, val2, ...)
    pos = queuepos(url)
    curr_pos = queuepos()
    val = random()
    val = round(val)
    str = substr(str, pos, len)
    str = soundex(str)
    val = sum(val)
    val = timestamp(date_time)
    str = upper(str)

Hint: You can define a criterion first using the normal selection controls for
the field, the operator and the value, and then change the field to "SQL
Expression" which will convert the given criteria to SQL Expression. Finally,
you can refine the converted expression.


Playlists
--------------------------------------------------------------------------------

A playlist is a compilation of different tracks from your music library. If you
eg. put different tracks into the queue, you can save them all as a playlist to
be re-opened and played again later at any time.

You can read and write playlists in different formats. Currently Silverjuke
supports the formats CUE, M3U, PLS; however, we recommend the M3U format as it
is widely spread.

To save the queue as a playlist, press the right mouse button in the queue
display and select "save playlist" from the context menu. In the following
dialog you have to enter a name for the playlist. Finally, hit "OK" to save the
playlist.

To open a saved playlist again, press the right mouse button and select "open
playlist or files" or "append playlist or files" from the context menu. Again,
in the following dialog select the name of the desired playlist and hit "OK".
Alternativly, you can just drag a playlist to the Silverjuke main window.

Hint: you can also use playlists which were not created with Silverjuke, if your
music library contains the referenced files.


Edit Tracks
================================================================================

To view or edit information for a track, select the track and press Ctrl-I or
use the option "Edit Tracks/Get info" from the context menu. You can edit
multiple tracks at the same time by just selecting them together in the main
window using eg. the "ctrl" or the "shift" keys.

The dialog "Edit Tracks" now shows all available information about the track(s)
whereas most of them can also be edited as follows:

- The title is the name of the tracks. You should always enter a title as
  otherwise it is hard to identify a track.

- The name of the artist. You should not leave this field empty as Silverjuke 
  uses this information to combine different tracks to albums.

- The names of the original artist and the composer's name are optional.
  However, please to not enter sth. like "unknown" here, just leave this field 
  blank if you do not want to enter these information.

- The name of the album. You should not leave this field empty as
  Silverjuke uses this information to combine different tracks to albums.

- Any optional comment

- The track number and the track count on this disk (see below). Silverjuke
  uses these information to sort the tracks within an album. If an album has 
  only one track, enter "1 of 1" here. Feel free to leave this field blank.

- The disk number and the disk count on the album. Silverjuke uses these
  information to sort the tracks within an album. If an album has only one
  disk (mostly true), enter "1 of 1" here. Feel free to leave this field blank.

- The genre of the track. If you fill out this field for a significant number of
  tracks, you can use it as an criterion for Music Selections (btw, this is true 
  for all information).

- The group can be used for additional criterions specific for you music
  library.

- The year of the track; this is not the year where the original title (if any)
  was released. Silverjuke uses this information to combine different tracks to 
  albums. Feel free to leave this field blank.

- The BPM or "beats per minute". Feel free to leave this field blank.

- Your rating from one to five stars where five stars should be the best rating.
  If you do not want to rate a track, select "no rating" here.

Finally, clicking "OK" saves the modified information in the database and, if
possible and wanted in the (ID3-)tags of the file.

If you edit more than one track, a detailed confirmation dialog is displayed
before the changes will take effect. In this dialog, you can also edit the
changes in a "final" step by double-clicking on am item.

Writing (ID3)-tags can be disabled using the corresponding option in the
confirmation dialog or with the "menu" button.


Further edit tools:

A click onto the "menu"-button shows a menu which gives you access to some
more edit tools, see below.

BTW: If you want to Extract information from the file name use the function
"split fields" which also allows to use the path and/or file name as source 
field.


Replace
--------------------------------------------------------------------------------

The dialog "Replace" allows you to search and replace strings in some
(ID3-)tags or in the file name of any tracks. This is most useful if you edit
the information of several tracks in one step.

To open the dialog:

- select all tracks to edit in the main window eg. by holding the "ctrl" or the
  "shift" keys while clicking on them ...

- click with the right mouse button on one of the selected tracks and select
  "Edit Tracks/Get info..." from the context-menu; this will open the 
  Edit Tracks-dialog ...

- here, click onto the "Menu"-button and select "Replace".

In the dialog:

- enter the text to search for and decide whether the search should be
  case-sensitive and whether you want to search for whole words only.

- select the field where to search in. You can also search in the file or the
  path name of the tracks. If you select "All fields", this refers to all fields 
  but the path and file names.

- enter the text found strings should be replaced with

- more experienced users can also use regular expressions in the search strings.
  Note: the regular expressions follow the POSIX-standard and not the 
  PERL-standard.

Finally, clicking OK will show a detailed confirmation dialog; if everything is
okay there, press OK once again and the changes will be written.


Split Fields
--------------------------------------------------------------------------------

The dialog "Split fields" allows you to split the text of one source field (eg.
one ID3-information or the path or the file name) into several destination
fields. This is most useful if you edit the information of several tracks in one
step.

To open the dialog:

- select all tracks to edit in the main window eg. by holding the "ctrl" or the
  "shift" keys while clicking on them ...

- click with the right mouse button on one of the selected tracks and select
  "Edit Tracks/Get info..." from the context-menu; this will open the 
  Edit Tracks-dialog ...

- here, click onto the "Menu"-button and select "Split fields".

In the dialog:

- first, select the field to split. The selected field should contain some text
  information that can be splitted into several other fields; often this is true 
  for the path and file name.

- using Destination fields and pattern you'll define how the field to split
  looks like:

  - Text, that is identical in each record, is entered directly.

  - Text that contains information you want to copy to another field is entered
    as a info-placeholder, where "info" must be replaced by the name of the
    destination field. The easiest way to enter these placeholders is to use the
    Insert-button.

  - Text, that is not identical in each record, but does not contain any useful
    information is entered as "*".

- the Example shows you at any time all source and destination fields of a
  sample record. So you can verify if the pattern matches your wishes.

Finally, clicking OK will show a detailed confirmation dialog; if everything is
okay there, press OK once again and the changes will be written.


Rename Files
--------------------------------------------------------------------------------

The dialog "Rename files" allows you to rename files based on the old file name
and/or on some (ID3-)tag-informationen This is most useful if you edit the
information of several tracks in one step.

To open the dialog:

- select all tracks to edit in the main window eg. by holding the "ctrl" or the
  "shift" keys while clicking on them ...

- click with the right mouse button on one of the selected tracks and select
  "Edit Tracks/Get info..." from the context-menu; this will open the 
  Edit Tracks-dialog ...

- here, click onto the "Menu"-button and select "Rename files".

In the dialog:

- Enter the Pattern for the new file name. The pattern may be built out of
  several placehoders as _title_, for a list of possible placeholders, use the
  Insert-button.

- Text that should be identical for all files to rename can be entered directly
  to the pattern.

- You can use the character "/" to also rename some folders in the path of the
  file.

Finally, clicking OK will show a detailed confirmation dialog; if everything is
okay there, press OK once again and the changes will be written.


Query freedb
--------------------------------------------------------------------------------

When editing track information using the Track Editor, you can also query the
freedb-Database for missing track information.

This works best, if you select all tracks of a given album. Then open the Track
Editor and select "Query online database..." from the "Menu..." button. A little
moment later, you get a list with possible albums. If the correct album is in
the list, select it and click on "OK". After that, the normal confirmation
dialog is shown where you can also edit the suggested information.

The default server used to query the freedb database is http://www.freedb.org ,
however, you can also define a mirror that is closer to you at "Advanced /
Further Options / Online database: Server name".


Visualizations
================================================================================

So called "Visualizations" are graphical output plugins which render their
graphics - more or less - in synchronity with the playing music.

If playback is started you can start a visualizations using the key F2 or the
symbol (eg. a wave) in the main window.

Usually, visualizations can be stopped using the "escape" key.

You can select the visualization to use by the menu atop of the visualization.
However, only one visualization can be started at the same time.


Advanced and further options
================================================================================

With the options found at "Settings / Advanced / Further options" you can define
some less common settings for Silverjuke. Moreover, you can change all shortcuts
here.

To change an option, select it and click onto the button "Customize".
Alternatively you may use the right mouse button.

Changed options are shown in a highlighted font in the list. To reset options to
their default values, select them (you may select several options using the
"shift" and the "ctrl" keys) and click onto "Customize / Reset selection".

The options in detail:

- **Playback:** These options let you select the playback device; depending on 
  the hardware and the operating system, there may be some more options 
  affecting it

- **Language:** By default, Silverjuke just uses the system language. Here you 
  can define another language to use. Further details about the localization 
  can be found in the file "localization.md" (or "localization.txt").

- **Image cache:** With the option RAM cache you can set the amount of memory to
  use to cache covers; by default 2 MB are used here which is enough for about 3 
  pages of covers in the standard size. With the option Use temporary directory
  you can use your harddisk to cache images. When using this option your 
  harddisk should be "large" enough to cache all possible covers; however, for 
  actual hardware this should be no problem. You can use the temporary directory 
  cache in two ways: asynchony or directly. Asynchony means, the images to
  display are rendered in a second thread and displayed when ready; in between,
  Silverjuke shows an empty square at the place of the cover. Directly means,
  the program display is halted until the image is ready and therefore no empty
  squares are shown and the output is a little "smarter" but may halt the user
  input for a moment. If you are unsure, use the asynchony method or disable the
  use of the temporary directory completly.

- **Index file:** This option displays the index file location (the index file
  contains the database for your music library). If you want to use another 
  index file, you have to add the parameter -db=FILE to the command line when
  starting Silverjuke.

- **Show files with:** By default when selecting the menu entry "Explore...",
  files are shown eg. with the Explorer under Windows or with the Finder on 
  Mac OS. However, you can use any other program that supports the command line:
  Just double-click onto the entry and select the program to use. After that,
  you may want to change the command line given to the selected program by
  right-clicking onto the entry and choosing "Edit...".

- **Temporary directory:** This option displays the temporary directory in use.
  The temporary directory is used eg. for the cover cache (see above). If you 
  want to use a different temporary directory, you have to start Silverjuke with
  the option -temp=DIR in the command line.

- **Ask on close if playing:** When the program is about to be closed by a user
  request and there is still some music playing, Silverjuke can ask you whether
  really to quit. This option has no effect if the program is closed by other
  reasons, eg. by a shutdown of windows.

- **Display:** With the "Show"-options, you can define whether the artist name,
  the total time and/or the track number (queue position) should be shown in the
  display.

- **Track list: Automatic track number:** If this option is set to "yes",
  Silverjuke automatically enumerates all tracks of an album with no gaps
  starting at "1". This will ignore the information stored in the database or in 
  the (ID3-)tags which will be used otherwise.

- **Track list: Show:** With the "Show"-options, you can define which details
  should be displayed in the track list. You can switch on and off the display
  of album names, composers, (original) Artists, disk numbers, track numbers,
  year, and double tracks. Moreover, for the artist- and album names you can 
  define if they should be shown if they differ from the artist-/album names of 
  the whole album.

- **Search: Lookup genre on simple search:** If this option is set to "yes", you
  can type in a genre into the simple search field in the main window. If the 
  genre is found, Silverjuke displays all tracks of this genre then.

- **Search: Max. history size:** With this option you define the number of
  previous searches that should be saved in the history. You can access the
  history by clicking onto the little arrow beside the search field.

- **Search: Save modified Music Selections:** By default, Silverjuke asks you
  whether to save a modified Music Selection eg. if you close the dialog.
  However, Silverjuke can also save (select "always") or cancel (select
  "never") all modified Music Selections automatically without asking. If you 
  select "never", do not forget to save the Music Selection manually eg. by 
  pressing "Ctrl-S".

- **Search: Search while typing:** If you enter a search text into the simple
  search field in the main window, Silverjuke can start the search while you are 
  still typing (incremental search) or wait until you hit "enter".

- **Mouse wheel:** With this option you can define if the mouse wheel should
  scroll the album view horizontally or vertically. Btw: holding the
  "shift"-key or pressing the right mouse button while using the mouse
  wheel toggles the direction.

- **Mouse wheel: Context sensitive:** If this option is enabled and the mouse is
  over the display, the mouse wheel scrolls the display content (the queue)
  instead of the album view.

- **Shortcuts:** Lots of options refer to shortcuts which may be customized by
  double-clicking on an option and then hitting the new shortcut. You may define 
  several shortcuts for a command. To remove a shortcut, use the right mouse 
  button and then "Remove shortcut...". You can even define system-wide 
  shortcuts. System-wide shortcuts may be used even if Silverjuke is not the
  active program, eg. you can use the "pause" key to start/stop playback from 
  within any running application. To define a system-wide shortcut, press the 
  right mouse button and choose "Add system-wide shortcut...".

- **Shortcuts: Numpad:** These options are available if you enable the numpad
  control for the Kiosk Mode. In this case, you can customize the keys to use 
  for the numpad control as described above.

Some other options can be set in the `[main]` section of the globals.ini file:

- `assert =` If set to 1, assert warnings are enabled; the state is shown in 
  the console window in this case. To invoke an additional test assert, use the
  value 2. Needed for debugging purposes only, defaults to 0.

- `testdrive =` If set to 1, some tests are done at the start of Silverjuke,
  the result is shown in the console window. Needed for debugging purposes only,
  defaults to 0.

Example:

    [main]
    assert = 1
    testdrive = 1


Copyright (c) Bjoern Petersen Software Design and Development and contributors

