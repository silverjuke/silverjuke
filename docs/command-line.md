Command Line Options
================================================================================

You may control Silverjuke by the command line with the following options:

* --ini=<file>  
  set the configuration file to use

* --db=<file>  
  set the database file to use

* --temp=<directory>  
  set the temp. directory to use

* --play

* --pause

* --toggle  
  toggle play/pause

* --prev

* --next

* --kiosk  
  start Silverjuke in kiosk mode
  
* --minimize  
  start Silverjuke minimized

* --open <files>  
  open the given files, --open may be ommited

* --enqueue <files>  
  enqueue the given files

* --help  
  Show all available commands

* --addcredit=<num>  
  Adds the numer of credits (tracks that may be enqueued) to the credit system.
  This option only work if the option "Credits may be added by external
  programs" is enabled in the kiosk mode settings.

* --setcredit=<num>  
  Sets the number of credits.  This option only work if the option "Credits may
  be added by external programs" is enabled in the kiosk mode settings.

* --skiperrors  
  With this option you can avoid showing errors about an erroneous termination 
  on the last Silverjuke run. This is especially useful when turning off
  Silverjuke using the "Power off" button, as in this case the operating system
  may not give us the time we need to unload all modules completely.

* --instance=<INI-file>  
  If you give a different INI-file to each instance, you can use multiple 
  instances of Silverjuke with this option. All settings for each instance are 
  stored in the INI-file, so the intances are completely independent; however, 
  make sure, there are not conflicts regarding the used hardware.  
  You can give the INI-files as complete paths; if the files do not exist, they
  are created as needed.  
  If you want to use different music libraries for each instance, you have to
  use the --db=<file> option as described above.

* --update  
  This will automatically upate the index as if you hit F5 just after starting 
  Silverjuke. If you use this option in combination with --kiosk and the kiosk
  mode is running with limited functionality, the update process cannot be
  aborted.

* --kioskrect=[<x>,<y>,]<width>,<height>[,clipmouse]  
  With this option you can force the Silverjuke window to the given rectangle
  when using the kiosk mode. This is usefull if you want to show sth. beside the
  Silverjuke window (by default, the whole screen is used). If the x and y
  parameters are skipped, the window will be placed at the upper left corner at
  0/0.  
  Moreover, if you add the "clipmouse" modified, the mouse cannot be moved
  outside the given rectangle. Using the following command line option, you can
  also assign a static portion of the screen for the visualization:

* --visrect=[<x>,<y>,]<width>,<height>  
  If --visrect or --kioskrect are given, portions of the screen not used by
  Silverjuke can be used by other programs - otherwise, Silverjuke would
  "darken" eg. an unused 2nd or 3rd screen.

* --volup  
  increase the main volume

* --voldown  
  decrease the main volume

* --togglevis  
  toggle the selected visualizations on and off

* --execute=<script or file>  
  Execute the given script snippet or file. With this command line option you
  are able to execute any little script by the  command line. For scripting 
  details, please refer to the Silverjuke SDK. Instead of using the command line
  for this purpose, you can also use DDE, which, however, is a little bit more tricky.

Although there is always only one instance of Silverjuke running, you may call
Silverjuke with these commands which will then be forwarded to the running
instance if possible.

If you want to use the command line on Apple Mac OS X, you have to execute
the program in the app's package directly, eg. 
/Applications/Silverjuke.app/Contents/MacOS/Silverjuke --help.
Under Windows Windows, you use eg. c:\Programs\Silverjuke\Silverjuke.exe --help.

You can set the "db", "temp", "kiosk" and "minimize" options also manually to
the current configuration file (ini file) in the section "main". Moreover, if 
there is a file called mysettings.ini in the program's directory (where
silverjuke.exe is), this file is used as the configuration file. All this allows
you to run Silverjuke independently from a windows system eg. on a removable
disk where you save your music together with Silverjuke.

BTW, most commands are also available through DDE or TCP/IP communication. Feel
free to post any questions or comments to this thread.


Copyright (c) Bjoern Petersen Software Design and Development, http://b44t.com

