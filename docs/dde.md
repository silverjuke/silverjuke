DDE and TCP/IP data exchange
================================================================================

Using the host "localhost" and the service "silverjuke", you can also control
Silverjuke by DDE on Windows or TCP/IP on Linux.

The supported _topics_ are simelar the the command line options:

- open - clear the playlist and open the files given as _data_; playback is 
  started using the first given file

- enqueue - enqueue the files given as _data_; the currently playing track stays
  unchanged and existing files are not removed from the queue

- play - same as pressing the "play" button

- pause - same as pressing the "pause" button

- toggle - toggle play/pause

- prev - play the previous track, if any

- next - play the next track, if any

- kiosk - start kiosk mode

- raise - bring the Silverjuke main window to the top of all windows.
  Please also note the command line options which are easier to use in some
  cases.

- volup - Increase the main volume

- voldown - Decrease the main volume

- togglevis - Toggle the selected visualizations on and off

- execute - Execute the script snippet given in _data_. Instead of giving the
  script directly, you can also write the script to a file and give the filename
  as _data_.  
  With this DDE command you are able to execute any little script by the command
  line. For scripting details, please refer to the file docs/scripting.

Silverjuke also supports a basic credit system that may be controlled with the
following additional DDE commands:

- addcredit - adds the numer of credits (tracks that may be enqueued) given in 
  _data_ to the credit system

- setcredit - sets the number of available credits to the number given in _data_

These options only work if "Credits may be added by external programs" is
enabled in the kiosk mode options.


Copyright (c) Bjoern Petersen Software Design and Development, http://b44t.com

