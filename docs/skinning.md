How to create Skins for Silverjuke
================================================================================

**Contents**

- Overview
- Tags
    - skin
    - layout
    - img
    - button
    - scrollbar
    - workspace
    - input
    - box
    - tooltips
    - div
    - color
    - if, else
    - include
    - script
- Targets
    - Targets for the button-Tag
    - Targets for the box-Tag
    - Targets for the scrollbar-Tag
    - Targets for the div-Tag


Overview
================================================================================

**Whats needed**

To create your own skins for Silverjuke, you need any text editor and any
painting program. notepad.exe and paint.exe will do the job, however, you may
prefer some more powerful applications

For finalizing the skin, you need a ZIP-packer. Finalizing the skin is
optional, but recommended if you want to spread the skin as all needed files
are put into a single archive.


**Files skins are build of**

The skin definitions are read from one or more XML-files which specify the
images to use and the interaction with Silverjuke.

You should put all files together into a single folder which must have the
extension .sjs (stands for silverjuke skin).

While developing the skin, using a folder is very useful, as you can access
each file directly. Later, if your skin is ready, you can compress the files
in the folder (not the folder itself!) using any ZIP-Packer. After that, you
have to rename the extension from .zip again to .sjs. Please also note, that
the filenames in the packed skin are case sensitive.

To use the skin in Silverjuke, the .sjs-folder or the .sjs-files must be in
the program directory or in another search path (see Settings/Advanced/Further
options/Search paths).

BTW: The best way starting creating your own skins is to have a look at the
existing skins.


**Images and subimages**

Depending on the way an image is used, several subimages are expected inside
an image (eg. to reflect different button states). These subimages are divided
from each other using horizontal and vertical lines of the colour of the first
upper left pixel in the whole image. We call this colour the control colour.
Even if an image has only one subimage, you have to surround the image by the
control colour. See the existing skins to get the idea.

The second pixel in the first scanline defines an optional mask colour. Inside
a subimage, you can use this colour to define a mask; for pixels with the mask
colour the background is painted instead of the mask colour.

Finally, the third pixel in the first scanline may optionally define the
colour to skip an subimage, the skip colour. If you fill a subimage completly
with the skip colour, this subimage is ignored by Silverjuke and, if possible,
another subimage is used. Eg. if you do not want to create an image for special
button states as "clicked", you may fill the subimage for this state with the
skip colour.

To remember the meaning of the first three pixels, you can use the alphabet:
CMS - Control colour, Mask colour, Skip colour.

In addition to a simple mask you can also use a full alpha channel. Alpha
channels are read from PNG files in Silverjuke 2.10beta8 or newer.


**Images formats**

You may use PNG, GIF or BMP files for your images. As JPEG files do not save
concrete colour values, which are needed eg. for the control colour, using
this file format is not recommended.


**Supported tags and targets**

See the chapters Tags and Targets for a complete list.


**Releasing skins**

If you have created a skin, you can post it in our Skins+Modules forum - it's
free, and if you become a registered forum member, you have the possiblity to
attach files to your topics as well as edit them later.


Tags
================================================================================


skin-Tag
--------------------------------------------------------------------------------

    Tag:        <skin>
    Child tags: <layout>, <tooltips>, <if>, <include>, <script>

This tag introduces a  new  skin. The tag must appear exactly one time for a
skin. To provide several skins in one *.sjs archive, you can use several *.xml
files containing skin-tags.

Attributes:

- name -  The  name  of the skin as it appears eg. in the selection dialog.
  This attribute is always required.

- about - Any information about who created this skin. May  be  shown  from
  within the skin selection dialog.

- debuginfo - Set to "1" to show a loading protocol. The protocol will also
  be shown partly on errors.

- debugoutline - Set to "1" to enable debug outlines.  Debug  outlines  are
  automatically enabled on errors.

- debugcond -  Override  the  system conditions using any testing settings.
  May be usefull in combination with <if>.

An example skin may look like the following:

    <skin name="Hello World!">
        <layout="My Layout">
            <!-- your default layout here -->
        </layout>
    </skin>


layout-Tag
--------------------------------------------------------------------------------

    Tag:        <layout>
    Child tags: <img>, <button>, <scrollbar>, <workspace>, <input>, <box>,
                <div>, <if>, <include>, <script>

This tag introduces a new layout for a skin. Each skin must have at least  one
layout. You can use several layouts for a single skin.

Attributes:

- name - The name of the layout. May be used as a target to allow switching
  between different layouts. This attribute is always required.  
  A special name is "kiosk": this layout is used as the default  layout  in
  the  kiosk  mode.  Note the remarks for if-tags when designing skins for the
  kiosk mode.

- minh, maxw - The minimal size of the window in pixels.

- maxh, maxw - The maximal size of the window in pixels.

- defh, defw - The default size of the window in pixels.

- usew, useh, usepos - Any optional unique  names.  Different  layouts  may
  have  the  same  usew-/useh-/usepos-names  and  share the size and/or the
  position therefore. By default, every layout  has  its  own,  independent
  size and position.

- doubleclicktarget - The action that should be performed on a double click
  in unused areas. You may use most targets that can be used for  button-
  tags here; for a list of all available targets see the chapter Targets.


img-Tag
--------------------------------------------------------------------------------

    Tag:        <img>
    Child tags: none

With this  tag  you  can  paint  a  simple  image in the window. Images do not
receive any user input.

Attributes:

- src - The filename of the image to use. The image must have  3x1  or  1x3
  subimages, depending on its orientation. Subimages may use a mask and may
  be skipped. The image is never stretched, but the middle subimage will be
  repeated to fill the given size, see the figures below. This attribute is
  always required.

- x, y - The position of the image. The position is relative to the  parent
  tag,  point  0/0  is  the upper left corner. You may give the position in
  pixels, as a percentage value or as a combination of both,  eg.  "50%-22"
  or "10%+70". Other Calculations are not supported.  
  Moreover, for  subsequent  items,  you can use same to use the same x- or
  y-value again or you can use next to use the  position  of  the  previous
  item plus its width or height.  
  If x or y are not specified, "0" is used.

- w,  h -  The  width and the height of the image. You may give the size in
  pixels, as a percentage value or as a combination of both,  eg.  "50%-22"
  or "10%+70". Other Calculations are not supported.  
  Moreover, for  subsequent items, you can use same to use the width or the
  height of the previous item. Since Silverjuke 2.52 you can also  use  the
  already  calcualted width or height of the item itself; for this purpose,
  use opposite.  
  If the width or the height are not specified  they  default  to  smallest
  possible image size.

Do not  forget  to  add  the  control  colour border around the subimages (see
chapter "Images and Subimages" in the introduction). The border is even needed
if  you  use  a  simple  image  with  only  one  subimage; please refer to the
following images to get an idea of the possible orientations.

A horizontal  image must  have  the  following  subimages  where  prologue and
epilogue may be skipped:

A vertical  image looks  like the following - again, prologue and epilogue may
be skipped:

A simple image has only one horizonzally- and  vertically-repeatable  subimage
as shown below.

Finally, some examples to get an idea about the x/y and w/h attributes:

    <!-- definition of x and y -->
    <img src="image1.png" x="100" y="200" w="300" h="400" />
    <img src="image2.png" x="same+10" y="next" w="300" h="400" />

    <!-- definition of width and height -->
    <img src="image1.png" x="100" y="100" w="100%-200" h="200" />
    <img src="image2.png" x="200" y="200" w="same" h="100" />
    <img src="image3.png" x="300" y="300" w="30%" h="opposite+10%" />


button-Tag
--------------------------------------------------------------------------------

    Tag:        <button>
    Child tags: none

Buttons are  one  of the most important way to receive user input. Each button
may be linked to a target which defines the action to take if the user  clicks
onto the button.

Attributes:

- target -  The action to take on a click on this button. There are lots of
  several targets, eg. Play  or  Pause,  see  the  chapter  Targets  for  a
  complete list. This attribute is always required.

- onclick -  Here you can give some scripting commands (see "Scripting") to
  Silverjuke and define, what should happen on a click on this button.  
  Note: If you define a target, this target always  comes  with  predefined
  commands  (eg.  obviously  the  target  "play" should start playing) - so
  often, there will be no need to define explicit commands. If you use  the
  onclick-attribute,  you  can  return  false from  your onclick-handler to
  avoid the default target processing - else, the default  target  commands
  are executed after your script.

- src -  The filename of the image to use. The image must have at least 1x3
  subimage where the first subimage is the  "normal",  the  second  is  the
  "hover"  and  the  third  the  "clicked"  mouse  state.  If the button is
  selectable, 3 more subimages must follow  representing  the  three  mouse
  states for the selected button. Some targets use even more button states,
  eg. the Repeat target has the  button  states  "off",  "repeat  all"  and
  "repeat  one".  See  also  the  figure  below.  This  attribute is always
  required.

- srcindex - You can put the subimages of different buttons into  a  single
  image  file  -  one  column  for  one  button.  With this option you tell
  Silverjuke which column to use. "0" is the default value  and  represents
  the first column.

- x,  y, w, h - The position and the size of the button, see img-tag for more
  information about these attributes.

- inactive - If set to "1", the button is not clickable,  but  the  current
  state  will  still  be  shown.  So you can use this option for displaying
  additional state information. Note: only the button gets inactive,  other
  buttons with the same target, menus or shortcuts are not disabled.  
  If set to "0", the button is clickable (default).

- cmw - You can add an optional menu area aright of a button. Clicking left
  into this area is the same than clicking  right  onto  the  button.  With
  "cmw" you define the pixel width (context menu width) of this area.

A button image can have the following subimages:

Most subimages  may  be  skipped. The width and height defaults to the size of
the "normal" subimage. If a larger sizer is  set  later,  the  image  will  be
centered vertically and/or horizontally.


scrollbar-Tag
--------------------------------------------------------------------------------

    Tag:        <scrollbar>
    Child tags: none

Scrollbars are used for scrolling the workspace or the display. Moreover, they
are also used as sliders eg. for the volume control.

Attributes:

- target - The action to take if the scrollbar is used. There are  lots  of
  several targets, eg. WorkspaceHScroll, VolSlider or Seek, see the chapter
  Targets for a complete list. This attribute is always required.

- src - The image to use for the scrollbar. Depending on  the  orientation,
  the  image  must  have  3x5 or 5x3 subimages. See the figures below. This
  attribute is always required.

- x, y, w, h - The position and the size of the scrollbar, see img-tag for
  more information about these attributes.

- hideifunused -  If set to "1", the scrollbar is hidden if it is currently
  not needed. Normally, the scrollbar is even shown in this case.

- inactive - See button-Tag.

An image for a vertical scrollbar looks like the following:

The page up/down subimages must also be repeatable,  the  thumb  will  have  a
minimal height of prologue+epiloge height. If the repeatable part of the thumb
is skipped, the thumb will have a fixed size (may be used for sliders).

An image for a horizontal scrollbar looks like:

Page left/right/up/down functionality is done by normal buttons if wanted.


workspace-Tag
--------------------------------------------------------------------------------

    Tag:        <workspace>
    Child tags: <color>

Use this tag to specify the workspace. Currently, the workspace is always  the
list  of all albums. Moreover, if no visualization-indent-rectangle is defined
in any <div>-tag, the workspace may also be replaced by the visualization.

Attributes:

- x, y, w, h - The position and the size of the workspace,  See  img-tag  for
  more information about these attributes.


input-Tag
--------------------------------------------------------------------------------

    Tag:        <input>
    Child tags: <color>

Use this  tag  to  specify  an input field. Currently, the input field is only
used for the search.

Attributes:

- x, y, w, h - The position and the size of the input, See img-tag  for  more
  information about these attributes.


box-Tag
--------------------------------------------------------------------------------

    Tag:        <box>
    Child tags: <color>

With the  box-tag  you can draw a simple rectangle with or without a border
(foreground) colour and with or without a background colour.

If you link the box to a target, Silverjuke  may  draw  some  text  using  the
foreground colour.

If no   background   colour  is  given  using  the  color-tag,  the  box  is
transparent.

Attributes:

- target - Most common targets are Line00, Line01  etc.  and  DisplayCover.
  See the chapter Targets for a complete list.

- x,  y,  w,  h -  The position and the size of the box, See img-tag for more
  information about these attributes.

- border - "1" draws a border, "0" doesn't (default)

- centerOffset - If text should be  drawn  centered  (this  is  decided  by
  Silverjuke,  not  by the skin) you can move the center origin to the left
  (negative pixel count) or to the right (positive  pixel  count)  by  this
  option.

- inactive - See button-tag.

- hideCreditInDisplay -  special  flag for the target CurrCredit, if set to
  "1", the display does not show the credit changes

- text - The box will display the given line of text.  Useful  only  if  no
  target  is  set;  the  text  should  be  encoded  as  UTF-8. Available in
  Silverjuke 2.10.

- id - If you have defined a text using the text-Attribute,  you  may  also
  want  to  change  it  afterwards. For this purpose, you can define an ID;
  then   you   can   change   the   text   of   the   box    later    using
  Program.setSkinText(). Available in Silverjuke 2.52beta2.

- font -  With this attribute, you can use different font for different box
  items. Please specify the font name of an installed font  here.  You  can
  also  specify  more  than  one  name  by  using  a  comma separated list;
  Silverjuke will use the first match then. If the font cannot be found,  a
  default font is used. Available in Silverjuke 2.73.


tooltips-Tag
--------------------------------------------------------------------------------

    Tag:        <tooltips>
    Child tags: none

With the  tooltips-tag  tag  you  can  define  the foreground and the background
colours that should be used for the tooltips.

Attributes:

- bgcolor, fgcolor - The colours to use for the tooltips, always required. Only
  strict RGB colours as "#RRGGBB" are supported, see the color-tag for more
  information about colour definitions.

- bordercolor - The colour of the 1-pixel-border, only strict RGB colours as
  "#RRGGBB" are supported, defaults to the foreground colour


div-Tag
--------------------------------------------------------------------------------

    Tag:        <div>
    Child tags: <img>, <button>, <scrollbar>, <workspace>, <input>, <box>,
                <div>, <if>, <include>, <script>

The div-tag may be used as a container for other items, as all positions and
sizes of child items are relative to div-tag,  using  div-tags  makes  layouts
easier to handle. div-tags may be nested.

Attributes:

- x,  y,  w,  h - The position and the size of the container, see img-tag for
  more information about these attributes.

- doubleclicktarget - The action that should be performed on a double click
  in  unused areas. You may use most targets that can be used for button-
  tags here; for a list of all available targets see the chapter Targets.

- target - Target for this container, currently only the target VisRect  is
  usefull  here.  With  VisRect  you define the container the visualization
  will be displayed in.

- visautostart - If the target is VisRect, and you set  this  attribute  to
  "1",  the  vis.  is  started  automatically together with the kiosk mode.

- indent -   Set   indent   borders   for    the    vis.    rectangle    as
  "left,top,right,bottom" eg. "1,2,1,2"


color-Tag
--------------------------------------------------------------------------------

    Tag:        <color>
    Child tags: none

With the  colour  tag,  you  can specify the colours to use for an item. Note,
that the tag is called "color" and not "colour".

Attributes:

- target - This attribute is always required and sets the colour target  to
  set the colour for. This may be one of the following:
    - "normal" - eg. a track
    - "normalodd" - odd tracks and the cover background
    - "selection" - eg. a selected track
    - "selectionodd" - an odd selected track
    - "title1" - artist or album
    - "title2" - album
    - "title3" - disk number
    - "verttext" - the vertical text in the browser
    - "stubtext" - used for the message if nothing is found
  Not all items use all (or any) colour targets.

- bgcolor -  The  background colour to use in the given colour target, only
  strict  RGB  colours  as  "#RRGGBB"  are  supported.  Defaults  to  white
  ("#FFFFFF").  
  By convention,   this   attribute  is  ignored  for  "title1",  "title2",
  "title3", "verttext" and "stubtext" and the "normal" background colour is
  used instead.

- fgcolor -  The  foreground colour to use in the given colour target, only
  strict  RGB  colours  as  "#RRGGBB"  are  supported.  Defaults  to  black
  ("#000000").

- hicolor -  Used  for  hiliting  eg.  search string in the browser or as a
  shadow colour for other targets, only strict RGB colours as "#RRGGBB" are
  supported. Defaults to red ("#FF0000").

- offsetx, offsety  - If "hicolor" is used as a shadow in a target, you can
  set the position of the shadow with these options. By default,  "offsetx"
  is 1 and "offsety" is null which means the shadow goes to the right.

For less  typing,  you  may also set the "normal" colours directly in the item
tags, eg. you can use

    <box fgcolor="#FF0000" />

instead of

    <box>
        <color target="normal" fgcolor="#FF0000" />
    </box>

Your favourite image processing program should be able to select  and  display
such colours, however, there are also many colour pickers on the web, look eg.
at http://de.selfhtml.org/helferlein/farben.htm .


if-, else-Tag
--------------------------------------------------------------------------------

    Tag:        <if>
    Child tags: <img>, <button>, <scrollbar>, <workspace>,
                <input>, <box>, <div>,
                <tooltips>, <color>, <if>, <include>, <script>

With this tag you can let tags appear only under certain circumstances.

Attributes:

- cond - Comma-separated list of conditions. Use the following strings to
  check the current operating system:
    - "win"
    - "mac"
    - "gtk"
  With the following strings you can check certain options:
    - "kiosk"
    - "creditsystem"
    - "playpause"
    - "editqueue" (includes "prev" and "next")
    - "unqueue" 
    - "volume"
    - "search"
    - "startvis"
    - "enlargedisplay"
    - "albumview" 
    - "coverview" 
    - "listview" 
    - "toggleview" (true if more than one view is available) 
    - "toggleelementes" 
    - "toggletimemode" 
    - "zoom" 
    - "repeat" 
    - "all" 
  If you  use  the  comma to give several options, the condition is true if
  any of the options is set. You can also negate the list by using  "!"  as
  the  first  character.  If  an  older  version  of  Silverjuke  does  not
  understand a condition, this version will always  see  the  condition  as
  false.

- version -  check  if  Silverjuke  runs  at  least  with the given version
  number. The version number must be given as "major.minor.revision" where
  "revision" may be skipped.  
  Example: To check against version 1.22, use 
  
        <if version="1.22">...</if>

You should not use if-tags to skip complete layouts or skins for options that 
may change during Silverjuke is running (this is everything beside the 
os-flags).  To test certain conditions, also note the attribute debugcond
for the skin-tag. 

Finally, the else-tag may be used as follows: 

    <if cond="mac">
        ...
    </if>
    <else>
        ...
    </else>


include-Tag
--------------------------------------------------------------------------------

    Tag:        <include>
    Child tags: none

Use this tag to  include  other  XML-Files  to  the
skin.  This  will  have  the  same  effect  as just writing the content of the
included file at the position of the include-tag.

Attributes:

- src - The name of the file to include. The  file  must  be  in  the  same
  directory, so do not prepend any path to the file name.
  
Example:

    <include src="filetoinclude.xml" />


script-Tag
--------------------------------------------------------------------------------

    Tag:        <script>
    Child tags: none

Use this tag to include a script to the skin. See the file _scripting_ for more
information about scripts.

Attributes:

- src - The name of the script to include. The file must  be  in  the  same
  directory, so do not prepend any path to the file name.
  
Example:

    <script  src="scripting.sj"  />

Alternatively, you  can also write the script directly to in the XML-file of a
skin:

    <script>
        // your script here
        print('skin loaded, just a test');
    </script>

For details about scripting, see the chapter "Scripting" below.


Targets
================================================================================


Targets for the button-Tag
--------------------------------------------------------------------------------

The following list shows possible targets for the button-Tag. It is okay  to
use  the  same Target more than one time. button-targets may also be used as
doubleclick targets for layout- or div-tags.

There is no need to support all targets in a layout, it is up to you to decide
which  functions  you  want  to  implement  by  the skin. Many - but not all -
functions are also available by the context menus or by shortcuts.

- AlwaysOnTop

  A click onto this button  will  toggle  the  "Always  on  top"  state  of  the
  Silverjuke  main  winow. This target is used in the "small layout view" of the
  default skin.

- AdvSearch, Settings, Help

  A click on these  button  will  open  the  "Music  selection",  "Settings"  or
  "Help/About"  dialogs.  These  targets  are  not used in the default skin; the
  functions are accessible from the menu or by shortcuts.

- DisplayDown, DisplayUp

  Clicks on these buttons will scroll the display lines up or down. The  display
  lines themselves are define by the targets Line00 .. Line99 using a box-tag.
  Also note the scrollbar-target DisplayVScroll.

- EnqueueLast, EnqueueNext, EnqueueNow, Unqueue, UnqueueAll, Prelisten

  Clicks on these buttons will enqueue the track(s)  selected  in  the  browser,
  prelisten  to  them  or  unqueue  the track(s) selected in the display. In the
  default skin, only the  UnqueueAll target  is  used  (see  the  large  display
  layout).  In  the  Skin "Silveriness Touched" we're also using the EnqueueNext
  target.  
  The images show the targets UnqueueAll (left) and EnqueueNext (right).

- MoreFromCurrAlbum, MoreFromCurrArtist

  Clicks on these buttons will enqueue missing tracks from the currently playing
  artist/album  to  be  playing next. These targets are introduced in Silverjuke
  1.50beta1.

- GotoA, GotoB, GotoC .. GotoZ, Goto0

  Clicks on these buttons will scroll the browser to the  first  album  starting
  with the given letter or, in case of Goto0, to the first album starting with a
  character different from a-z (available in Silverjuke 1.19beta4).

- GotoPrevLetter, GotoNextLetter

  Clicks on these buttons will scroll  the  browser  to  the  previous  or  next
  letter.  In  the default skin, these targets are not used but you can use them
  in your own skins or define some shortcuts for them. 

- GotoRandom, GotoCurr, GotoFirst, GotoLast

  Go to  a random, the current, the first or the last track in the browser. Only
  the target gotocurr is used in the default skin of Silverjuke.  
  The image shows the target GotoCurr.

- OpenFiles, AppendFiles, SavePlaylist

  These targets  open  the  "Open file(s)", "Append file(s)" and "Save playlist"
  dialogs.

- Play, Pause, Prev, Next, FadeToNext

  The well-known player commands. Note that "Play" and "Pause" both do the same,
  toggling  between play and pause. However, you may use both targets in your
  skin for layout reasons.  The  default  skin  only  uses  the  target  "Play".
  "FadeToNext" is introduced in Silverjuke 1.50beta1.

- Repeat, Shuffle, RemovePlayed

  "Repeat" toggles  the  repeat  state  from  "off",  "all" to "single" (in this
  direction). So make sure, you have defined enough subimages (see button-tag)
  to represent all button states.  
  "Shuffle" and "RemovePlayed" just toggles the corresponding states.

- SearchButton

  Starts/ends a  search  on  click.  As  there are some important options in the
  context menu for this button, we've added a context menu area in  the  default
  skin (see the parameter "cmw" for the button-tag).  
  The context menu area of the search button is highlited in green.

- SeekBwd, SeekFwd

  With these buttons you can define explicit buttons for seeking.

- StartVis

  This target  starts  or stops the currently selected or running visualization.
  Also note the div-target VisRect.

- Stop, StopAfterThisTrack, StopAfterEachTrack

  These targets set the player to the "stop" mode -  either  immediately,  after
  the running track or after each running track.  
  The last  option  is  useful esp. for karaoke setups where you have to hit the
  "play" button before each track then. This allows the next singer  to  prepare
  himself.  
  All three  targets  are not used in the default skin but may be reached by the
  menu.

- ToggleTimeMode

  A click on a button with this target toggles the time mode from  "elapsed"  to
  "remaining" and back. This target is also automatically provided by Silverjuke
  as a click on the running time in the display, therefore it is not used in the
  default skin.

- ToggleKiosk

  A click  on a button with this target switches the kiosk mode on and off. This
  target is not used in the default skin.

- VolDown, VolUp, Mute

  Clicks on these targets alter the main volume. In the default skin we  do  not
  use  the  buttons  and only use a volume slider defined by the scrollbar-tag
  VolSlider. However, in  the  skin  Silveriness  Touched  we  use  the  targets
  "VolDown" and "VolUp"

- WorkspaceDown, WorkspaceLeft, WorkspaceRight, WorkspaceUp

  These targets alter the horizontal or vertical position in the  browser.  Also
  note the scrollbar-targets WorkspaceHScroll and WorkspaceVScroll.

- WorkspacePageDown, WorkspacePageLeft, WorkspacePageRight, WorkspacePageUp
  
  These targets  alter the horizontal or vertical position in the browser by one
  full page  and  are  introduced  in  Silverjuke  1.50beta1.  If  you  use  the
  WorkspaceHScroll  or  WorkspaceVScroll  targets,  this  functionality  is also
  available through the scrollbars by clicking aside of the thumb.

- ZoomIn, ZoomOut, ZoomNormal

  These button targets alter the zoom of the browser. The target ZoomNormal is 
  not used in the default skin but is available with the shortcut "*".

- AlbumView, CoverView, ListView, ToggleView

  These button targets can be used to switch the current view of the  workspace.
  Note  that  views  can be disabled in the kiosk mode, you can check this using
  the if-tag. 

- Layout:yourLayoutName

  You can also use a layout as a target for a button.  This allows you to use 
  several switchable layouts, eg. you can show/hide some items this way.


Targets for the box-Tag
--------------------------------------------------------------------------------

The following list shows possible targets for the box-Tag.

- DisplayCover

  Shows the  cover  of  the  currently  played track. The cover is automatically
  scaled to the size of the box.

- Box targets "Line00", "Line01" .. "Line99"

  With these targets you define the display lines. The font size is  adapted  to
  the  height  of  the  box,  the  strings to show are truncated, if needed. You
  should  not  forget  to  define  DisplayVScroll  and/or  DisplayDown/DisplayUp
  targets  -  otherwise  the  user  cannot  scroll  eg. the playlist show in the
  display.  
  Moreover, you should  define  at  least  2  display  lines  -  otherwise  some
  functions of Silverjuke won't work.  
  The normal  view  of the default skin only uses the targets Line00, Line01 and
  Line02.

- Box targets "CurrTrack", "NextTrack" and "CurrTime"

  These targets may be used in addition or  as  a  replacement  for  the  "line"
  targets  and  display  the  current  and the upcoming track and the elapsed or
  remained time of the current track. If there is not  current/next  track,  the
  targets display an empty string or "-:-" for the time. 

- SearchInfo

  If you use this target for a box, some search information  as  the  number  of
  matches  are  shown  in  the  box.  Normally, this target is placed close to a
  search target.

- CurrCredit

  If you use this target for a box, the currently available credits are shown as
  a number in the box. If there are no credits left, "0" is shown, if the credit
  system is not enabled and there are infinite credits available, "oo" is shown.


Targets for the scrollbar-Tag
--------------------------------------------------------------------------------

The following list shows possible targets for the scrollbar-Tag

- DisplayVScroll

  This target  sets  the  scrollbar to scroll the display (eg. the tracklist) up
  and down. The display lines themselves are define by  the  targets  Line00  ..
  Line99  using  a  box-tag.  Also  note the button-targets DisplayDown and
  DisplayUp.

- Seek

  This target sets the scrollbar to alter the playing position of the  currently
  played  track  (it  is used in the default skin as a very thin bar atop of the
  display).

- VolSlider

  This target sets the scrollbar to alter the main volume  of  Silverjuke.  Also
  note the button-targets VolUp, VolDown and Mute.

- WorkspaceHScroll, WorkspaceVScroll

  These targets set the scrollbar to alter the horizontal or  vertical  position
  in  the  browser. Also note the button-targets WorkspaceDown, WorkspaceLeft,
  WorkspaceRight and WorkspaceUp.


Targets for the div-Tag
--------------------------------------------------------------------------------

The following list shows possible targets for the div-Tag.

- VisRect

  With "VisRect"  you  define  the container the visualization will be displayed
  in. If you do not define a div-area with this  target,  the  workspace-tag is
  used  as  the  area  to  display  the visualization in. Also note the
  button-target StartVis.  
  The image shows the workspace (green)  and  the  VisRect  (red)  used  in  the
  default skin.

- Layout:yourLayoutName

  You can also use layouts as a doubleclick target for a
  div-tag. This allows you to use several switchable layouts, eg. you can
  show/hide some items this way.  
  Moreover, you  can  use the button-targets from above as doubleclick targets
  for the div-tags.

  
Copyright (c) Bjoern Petersen Software Design and Development, http://b44t.com

