Scripting in Silverjuke
================================================================================


**Contents**

- Overview
- Scripting Syntax
- Object Reference
- Program Object
    - Program.version
    - Program.os
    - Program.locale
    - Program.layout
    - Program.viewMode
    - Program.listMode
    - Program.zoom
    - Program.search
    - Program.musicSel
    - Program.memory
    - Program.hwnd
    - Program.onLoad
    - Program.onUnload
    - Program.loaded
    - Program.lastUserInput
    - Program.onKiosk
    - Program.kioskMode
    - Program.sleepMode
    - Program.visMode
    - Program.autoPlay
    - Program.addMenuEntry()
    - Program.addConfigButton()
    - Program.addSkinsButton()
    - Program.addExitOption()
    - Program.setTimeout()
    - Program.setDisplayMsg()
    - Program.setSkinText()
    - Program.refreshWindows()
    - Program.selectAll()
    - Program.getSelection()
    - Program.getMusicSels()
    - Program.run()
    - Program.launchBrowser()
    - Program.iniRead()
    - Program.iniWrite()
    - Program.exportFunction()
    - Program.callExported()
    - Program.callPlugin()
    - Program.gc()
    - Program.shutdown()
- Player Object
    - Player.time
    - Player.duration
    - Player.volume
    - Player.onTrackChange
    - Player.onPlaybackDone
    - Player.play()
    - Player.pause()
    - Player.stop()
    - Player.stopAfterThisTrack
    - Player.stopAfterEachTrack
    - Player.isPlaying() etc.
    - Player.prev()
    - Player.next()
    - Player.hasPrev()
    - Player.hasNext()
    - Player.queueLength
    - Player.queuePos
    - Player.getUrlAtPos() etc.
    - Player.addAtPos()
    - Player.removeAtPos()
    - Player.removeAll()
    - Player.repeat
    - Player.shuffle
    - Player.removePlayed
    - Player.avoidBoredom
- Rights Object
    - Rights.all
    - Rights.credits
    - Rights.useCredits
    - Rights.play
    - Rights.pause
    - Rights.stop
    - Rights.editQueue
    - Rights.unqueue
    - Rights.multiEnqueue
    - Rights.repeat
    - Rights.search
    - Rights.startVis
    - Rights.volume
    - Rights.viewMode()
    - Rights.zoom
- Dialog Object
    - Dialog Constructor
    - Dialog.addCtrl()
    - Dialog.getValue()
    - Dialog.setValue()
    - Dialog.show()
    - Dialog.showModal()
    - Dialog.close()
- Database Object
    - Database Constructor
    - Database.openQuery()
    - Database.nextRecord()
    - Database.getFieldCount()
    - Database.getField()
    - Database.closeQuery()
    - Database.getFile()
    - Static Database Functions
- File Object
    - File Constructor
    - File.length
    - File.pos
    - File.read()
    - File.write()
    - File.flush()
    - Static File Functions
- HttpRequest Object
    - HttpRequest Constructor
    - HttpRequest.responseText
    - HttpRequest.status
    - HttpRequest.request()
    - HttpRequest.setRequestHeader()
    - HttpRequest.getResponseHeader()
    - HttpRequest.abort()
- Math Object
    - Math Constants
    - Math.sin() etc.
    - Math.abs()
    - Math.ceil()
    - Math.exp()
    - Math.floor()
    - Math.log()
    - Math.max(), Math.min()
    - Math.pow()
    - Math.random()
    - Math.round()
    - Math.sqrt()
- Date Object
    - Date Constructor
    - Date.parse()
    - Date.UTC()
    - Date.get()
    - Date.set()
    - Date.toString()
    - Date.toLocaleString()
    - Date.toUTCString()
- Array Object
    - Array Constructor
    - Array.length
    - Array.pop()
    - Array.push()
    - Array.reverse()
    - Array.shift()
    - Array.sort()
    - Array.splice()
    - Array.unshift()
    - Array.concat()
    - Array.join()
    - Array.slice()
- String Object
    - String Constructor
    - String.length
    - String.fromCharCode()
    - String.charAt()
    - String.charCodeAt()
    - String.concat()
    - String.indexOf()
    - String.lastIndexOf()
    - String.match()
    - String.replace()
    - String.search()
    - String.slice()
    - String.split()
    - String.substr()
    - String.substring()
    - String.toLowerCase()
    - String.toUpperCase()
- RegExp Object
    - RegExp Constructor
    - RegExp.exec()
    - RegExp.test()
- Globals
    - eval()
    - parseInt()
    - parseFloat()
    - isNaN()
    - isFinite()
    - decodeURI()
    - decodeURIComponent()
    - encodeURI()
    - encodeURIComponent()
    - print()
    - logError()
    - logWarning()
    - alert()
    - confirm()
    - prompt()
    - fileSel()
- Further Topics


Overview
================================================================================

Many things in Silverjuke can be controlled using scripting. A script is built
of simple lines of code placed eg. in a text file or in attributes attributes
of skins (eg. "onclick" in button-Tags).

These chapters describe how to write scripts for Silverjuke.


**Where scripting can be used**

- As mentioned, you can use scripts when creating your skins (see  "How  to
  create Skins for Silverjuke").

- You  can  create independent scripts that are executed when Silverjuke is
  loaded; these scripts must be placed in one of the search path  with  the
  extension .sj

- You  can  use  scripts  even  when  writing  plugins  (see "Plugins") for
  Silverjuke using eg. C or C++.


**Scripting commands and Targets**

In contrast to  targets  in  skins  (see  "Targets"),  commands  are  executed
independingly of the functionality allowed by the user. Example:

    <button onclick="pause();" /> <!-- will always pause the player -->
    
    <button target="pause" />     <!-- will pause the player only if allowed
                                  in Silverjuke's kiosk mode options -->

However, if required, you can check the allowed functionality before  defining
eg. a button; please us the if-tag or the Rights Object for this purpose.


**Read on ...**

For some  examples, please have a look at the files coming together with 
Skilverjuke.

The following chapters will describe the required syntax and the objects,
methods, properties and functions that can be used in your scripts.


Scripting Syntax
================================================================================

This chapter  will  give  you  an overview about the syntax used in Silverjuke
scripts. If you are already familiar with this, the next chapters describe the
objects and methods (see "Object Reference") specific to Silverjuke.


**Overview**

The syntax  for Silverjuke scripts follow the rules defined by ECMAScript, 3rd
edition http://www.ecma-international.org/publications/standards/Ecma-262.htm .
The  syntax  is  very  close to JavaScript, C/C++ and to other
well-known languages.

If you have ever written a little program or a script in another language, you
should have no problems with Silverjuke scripts.


**Variables**

Variables have no type attached, and any value can be stored in any variable.

- Variables  can  be  declared with a var statement. If this is done, these
  variables are local to the scope the variable is declared.

- Variables declared outside any function, and variables not declared  with
  var, are global and can be used by the entire script.

- Variable  names  (as  well  as  functions  and  reserved  words) are case
  sensitive.

        a = 1;         // declare a global variable
        function var_test()
        {
            var b = 2; // this is a variable local to var_test()
            c = 'str'; // this also declares a global variable
        }
    
        var_test();    // call var_test()
        print(a);      // will print 1
        print(b);      // b is undefined as local to var_test()
        print(c);      // will print 'str'


**Basic Data Types**

Basic data types are boolean, numbers, strings, arrays and objects. Conversion
between these types is done as needed.

- Booleans are values which represent one of the states true or false. Each
  other data type may be converted to a boolean; this is needed if they are
  used eg. as expressions in control structures.

- Numbers are  represented as floating point values. Basic calculations can
  be done directly using eg. the +, -, *, /, %, ++, -, &, |, ^,  <<  or  >>
  operators, for some advanced calculations, please have a look at the Math
  Object.

- Strings are a sequence of characters  and  can  be  created  directly  by
  placing  the  series  of  characters  between  double  or  single quotes.
  Individual characters within a string can  be  accessed  through  the  []
  operator;  to  append  two strings together, use the + operator. For some
  advanced operations, please have a look at the String Object.

- Arrays are  maps  from  integer  numbers  to  other  basic  data   types.
  Individual  values  within  an  array  can  be  accessed  through  the []
  operator. For some advanced operations, please have a look at  the  Array
  Object (see "Array Object").

- Objects are  values  with  some attached named properties and are usually
  created using the new operator; some  of  the  named  properties  may  be
  functions (callable  properties).  To access properties of an object, use
  the . operator

        a = true;              // create a boolean
        
        b = 12.2;              // assign a number to b
        c = 'test string';     // assign a string to c
        d = "another string";  // assign a string to d
        
        e = [0,1,2];           // create an array with three elements
        f = new Array(0,1,2);  // same using the new operator
        
        g = new Object;        // create a new (empty) object
        g.prop = 23;           // assign a property to the object

        h = new Date;          // create an object of a predefined class
        h.getDay()             // call a method (a callable property) of the object

There is no need to worry about deleting variables - this is done
automatically by Silverjuke's garbage collection.


**Control Structures**

You can use the control structures if, switch, for, while and do..while.

    if ( expr ) { 
        statements; 
    }
    else {
        statements; 
    }

If-expression: The curly braces are needed only if you use more than one 
statment. In general, they are recommended in most cases. The else part is 
optional.

    switch ( expr ) {
        case 1:  statements; break;
        case 2:  statements; break;
        default: statements; break;
    }

Switch-expression: The curly braces are required, break is optional but 
recommended in  most cases - if left out, execution will continue to the body of
the next case block. The default part is optional.

    for ( initial-expr; cond-expr; expr evaluated after each loop ) { 
        statements to execute each loop; 
    }

For-expression: Every expression part can be left out if unneeded. The curly 
braces are needed only  if you use more than one statment. In general, they are 
recommended in most cases.

    while ( cond-expr ) { 
        statements;
    }

While-expression: The curly braces are needed only if you use more than one
statment.  In general, they are recommended in most cases.

    do { 
        statements; 
    } while ( cond-expr );

While-expression: The curly  braces are  needed  only if you use more than one 
statment. In general, they are recommended in most cases. In contrast to
while(..){..},  the  do{..}while(..) loop is always executed at least one time.

Examples:

    if( program.kioskMode ) {
        // we're in kiosk mode, do what to do
    } else  {
        // we're not in kiosk mode, do what to do
    }

    switch( program.viewMode ) {
        case 0: // we're in album mode, do what to do
            break;

        case 1: // we're in cover mode, do what to do
            break;

        default: // handle other modes here
            break;
    }


**Functions**

A function is a block with a (possibly empty) argument list  and  an  optional
name. A function may give back a return value.

    // declare the function
    function my_func_text( a )
    {
        print( a ); // call the predefined function print()
    }

    // call the function
    my_func_test( 'Hello!' );

    // functions may also be used as properties for objects:
    program.launchBrowser( 'http://...' );

    // you can also give "anonymous" (unnamed) functions directly as arguments:
    program.addMenuEntry( 'bla', function(){alert('Unamed function')} );

Are arguments given by value or by reference? This is quite easy to answer:

- Booleans,  Numbers and Strings are given by value - you get a copy of the
  value, if you change the value,  the  changes  are  not  visible  to  the
  caller.

- All  other  types are given by reference for performance reasons - if you
  change eg. an object given as an argument, these changes are  visible  to
  the caller.


**Error Handling**

If any  error  occur while your script is executed, an error object is thrown.
You may catch these errors using a try..catch block:

    try {
      // do what to do here
    }
    catch(err) {
      // if any error occurs in the try block above, we go here.
    }
    // here we go if no erros occur

If any error is thrown outside a try..catch block, the  script  is  terminated
immediately;  in  this  case - and eg. for syntax errors - Silverjuke logs and
error to the console window.

BTW: You can also easily test a function, a macro or a little script  by  just
entering it to the text file and click on "Evaluate".

Now, as  you have an idea about the syntax, you may want to have a look at the
next chapters which describe the objects and methods (see "Object  Reference")
specific for Silverjue.


Object Reference
================================================================================

The following  chapters  describe  the  objects and methods available for your
scripts or macros. If not stated otherwise, the  functions  do  not  return  a
value nor expect any arguments. For a brief overview about the syntax, see the
previous chapter, "Scripting Syntax".


Program Object
================================================================================

The Program object gives you access to the Silverjuke program itself,  it  has
one predefined instance, program (written lower case).


Program.version
--------------------------------------------------------------------------------

    value = program.version;

Read only property. Contains the Silverjuke version as 0xjjnn00rr with:

- jj as the major version
- nn ans the minor version
- rr as the revision

The single components are encoded as BCD (see http://en.wikipedia.org/wiki/
Binary-coded_decimal); eg. for Silverjuke 2.10 rev17, the function returns
0x02100017. Note, that the revision number is not always equal to the beta- or
rc-number.  You can find out the correct revision number eg. in the "Properties"
dialog of Windows.

See also: SJ_GET_VERSION


Program.os
--------------------------------------------------------------------------------

    value = program.os;

Read only property.  Contains one of the following strings representing the
underlying operating system:

- gtk - Silverjuke and the scripts run unter GTK, normally Linux
- win - Silverjuke and the scripts runs under Microsoft Windows or compatible
  software.
- mac - Silverjuke and the scripts runs under Mac OS X or compatible software.

Example:

    if( program.os == 'gtk' ) {
        print("we're running under Linux");
    }
    if( program.os == 'win' ) {
        print("we're running under Windows");
    }
    else if( program.os == 'mac' ) {
        print("we're running under OS X");
    }
    else {
        print("we're running under an unknown OS?!");
    }


Program.locale
--------------------------------------------------------------------------------

    value = program.locale;

Read only property. Contains a string with the language and the country  as  a
two- or five-letter string in xx or xx_YY format:

- xx - the ISO-639 code, of the language, see 
  http://en.wikipedia.org/wiki/List_of_ISO_639-1_codes
- YY is the ISO-3166 code of the country, 
  see http://en.wikipedia.org/wiki/ISO_3166-1

Examples are  "en", "en_GB", "en_US" or "fr_FR". If in doubt, please write all
messages given to the user in english.


Program.layout
--------------------------------------------------------------------------------

    value = program.layout;

Read/write property. Contains the name of the currently selected  layout.  The
layout  names  must  be  defined  in  the  skins  using the layout-tag.

Example:

    name = program.layout;    // find out the current layout
    program.layout = "Small"; // switch to another layout


Program.viewMode
--------------------------------------------------------------------------------

    value = program.viewMode;

Read/write property. Contains the current view  mode  (0=album  view,  1=cover
view, 2=list view).

Example:

    program.viewMode++;   // switch to the next view mode
    program.viewMode = 0; // set view mode to "album view"

See also: Rights.viewMode(), Program.zoom


Program.listMode
--------------------------------------------------------------------------------

    value =  program.listModeOrder;
    values = program.listModeColumns;

Read/write properties  to  access  the  columns  in  the  list  view mode (see
"Program.viewMode").

- program.listModeOrder contains the column that is used for  sorting.  The
  column  is  defined  by  one of the column IDs below. Negative column IDs
  indicate descending ordering, positive column IDs are used for  ascending
  ordering.
- program.listModeColumns is  an  array  (see "Array Object") that contains
  the column IDs currently displayed. Each item in the array represents one
  column,  the  array  index  specified  the order of the columns (0 is the
  first, most left column).

For the column IDs, please refer to the following list:

- 1 - URL or file
- 2 - track name
- 4 - track number
- 8 - track count
- 16 - disk number
- 31 - time added
- 32 - disk count
- 47 - time modified
- 63 - time last played
- 64 - lead artist name
- 79 - times played
- 95 - size
- 111 - bitrate
- 127 - samplerate (frequency)
- 128 - original artist name
- 143 - channels
- 159 - gain
- 175 - type
- 191 - current queue position
- 256 - composer name
- 512 - alum name
- 1024 - genre name
- 2048 - group name
- 4096 - comment
- 8192 - beats per minute (bpm)
- 16384 - rating
- 32768 - year
- 65536 - duration

Example:

    // set list mode to "artist - album - track name"
    // ordered by artist
    program.listModeColumns = [128,512,2];
    program.listModeOrder   = 128;

    // switch to list mode (if not yet done)
    if( program.viewMode != 2 )
        program.viewMode = 2;

    // toggle ordering
    program.listModeOrder *= -1;

See also: Program.viewMode


Program.zoom
--------------------------------------------------------------------------------

    value = program.zoom;

Read/write property that contains the current zoom of the  current  view.  The
zoom  is  defined  by a value between 0 and 6 (0=very small, 3=default, 6=very
large view).

Example:

    var cur = program.zoom; // find out the current zoom
    program.zoom = 2;       // set another zoom

    program.zoom++;         // zoom in
    program.zoom--;         // zoom out

See also: Rights.zoom, Program.viewMode


Program.search
--------------------------------------------------------------------------------

    words = program.search;

This property contains the words of the current "simple search". If  there  is
no  search at the moment, the property is undefined. You can change the search
words by just assinging a string to this property.

Note that this property may not work if Silverjuke is not yet loaded
completely (see "Program.loaded"). 

Example:

    var cur = program.search;   // find out the current search
    program.search = "beatles"; // set another search

See also: Program.musicSel


Program.musicSel
--------------------------------------------------------------------------------

    name = program.musicSel;

This property contains the name of the currently selected music  selection  or
undefined.  You can change the current music selection by assinging one of the
existing music seletions to this property.

Note that this  property may not work if Silverjuke is not yet loaded
completely (see "Program.loaded").

Example:

    var cur = program.musicSel;      // find out the current music seletion
    program.musicSel = "70's music"; // set another music selection

See also: Program.getMusicSels(), Prorgram.search


Program.memory
--------------------------------------------------------------------------------

    value = program.memory;
    value = program.memoryPeak;

Read only property. Contains the number of bytes used currently by all scripts
together. There is also a property program.memoryPeak which contains the  "all
time peak".

Example:

    currentlyUsedBytes = program.memory;
    maxUsedBytes = program.memoryPeak;

See also: Program.gc()


Program.hwnd
--------------------------------------------------------------------------------

    value = program.hwnd;

Read only  property  that  contains  the  "system handle" of Silverjuke's main
window - this is eg. a "HWND" if Silverjuke runs under Microsoft windows.


Program.onLoad
--------------------------------------------------------------------------------

    program.onLoad = function;

This function  is  triggered  after  Silverjuke  is  loaded   completely.   If
Silverjuke   is   already   loaded   completely,  the  function  is  triggered
immediately.

Example:

    function silverjukeLoaded()
    {
        // Silverjuke is loaded now, do what to do here.
    }

    program.onLoad = silverjukeLoaded;

See also: Program.loaded, Program.onUnload, Program.onKiosk


Program.onUnload
--------------------------------------------------------------------------------

    program.onUnload = function;

This function is triggered before Silverjuke is terminated.

Example:

    function silverjukeExit()
    {
        // Silverjuke is about to quit now - do what to do here.
    }

    program.onUnload = silverjukeExit;

To remove the callback function, just assign "undefined" to the property.

See also: Program.onLoad


Program.loaded
--------------------------------------------------------------------------------

    state = program.loaded;

Read only property that contains true if Silverjuke is loaded completely, else
false.

See also: Program.onLoad


Program.lastUserInput
--------------------------------------------------------------------------------

    ms = program.lastUserInput;

Property that contains the number of milliseconds expired since the last  user
input.  This  can be used eg. from within a timer to determinate if Silverjuke
is not used by the user for some time.

Please note, that not all  possible  actions  result  in  an  update  of  this
property.  However,  you  can  assume  this property to be compatible with the
Silverjuke built-in options at "Automatic control" (eg. "Go to  current  track
after .. minutes of inactivity").

See also: Program.setTimeout()


3.2.1.16  Program.onKiosk
--------------------------------------------------------------------------------

    program.onKioskStarting = function;
    program.onKioskStarted  = function;
    program.onKioskEnding   = function;
    program.onKioskEnded    = function;

If you want to be informed on any changes affecting the kiosk mode, please set
one or more of these properties to a callback function.

- onKioskStarting is  called  just  before  the  kiosk  mode  is   started;
  Program.kioskMode   will   still   return  false when  you  receive  this
  notification.
- onKioskStarted is  called  just  after  the  kiosk   mode   is   started;
  Program.kioskMode   will   already  return  true when  you  receive  this
  notification.
- onKioskEnding is  called  just  before   the   kiosk   mode   is   ended;
  Program.kioskMode   will   still   return   true when  you  receive  this
  notification.
- onKioskEnded is  called   just   after   the   kiosk   mode   is   ended;
  Program.kioskMode   will  already  return  false when  you  receive  this
  notification.

To remove a callback function, just assign "undefined" to the properties.

See also: Player.onTrackChange, Program.onLoad


Program.kioskMode
--------------------------------------------------------------------------------

    state = program.kioskMode;

Read/write property that is set to true if Silverjuke is currently running the
kiosk mode, else this property is set to false.

See also: Program.onKiosk, Program.shutdown(), Program.addExitOption()


Program.sleepMode
--------------------------------------------------------------------------------

    enabled = program.sleepMode;
    minutes = program.sleepMinutes;

Read/write propterties to access the most important sleep mode settings.

- sleepMode contains  the  value  true if  the sleep mode is enabled, false
  otherwise.
- sleepMinutes are to the number  of  minutes  to  wait before  the  "sleep
  action"  is  started.  If the user has defined a static time instead of a
  number minutes, this value is undefined and  cannot  be  changed  by  the
  script.

Example:

    // enable the sleep mode to wait for 60 minutes;
    // we do this only if the number of minutes are changeable,
    // see the comments above
    if( typeof program.sleepMinutes != 'undefined' )
    {
      program.sleepMode = true;
      program.sleepMinutes = 60;
    }

See also: Program.setTimeout()


Program.visMode
--------------------------------------------------------------------------------

    enabled = program.visMode;

Read/write propterty  to  start/stop the selected visualization or to find out
if it is running.

Example:

    // start the visualization
    program.visMode = true;

    // stop the visualization
    program.visMode = false;

    // toggle the visualization on/off
    program.visMode = !program.visMode;

    // check if the visualization is running
    if( program.visMode )
    {
        // yes, the vis. is running, do what to do here
    }

See also: Rights.startVis


Program.autoPlay
--------------------------------------------------------------------------------

    enabled = program.autoPlay;

Read/write propterty to access the AutoPlay state. The state is simply true or
false for  "AutoPlay  enabled"  or  "AutoPlay  disabled".

Example:

    enabled = program.autoPlay; // get AutoPlay state
    program.autoPlay = enabled; // set AutoPlay state

To add a "Toggle AutoPlay" button to a skin:

    <button ... onclick="program.autoPlay=!program.autoPlay;" />

With the  following  little  script, you can also add a shortcut to access the
toggle function ...

    function toggleAutoPlayCallback()
    {
      program.autoPlay=!program.autoPlay;
    }
    
    program.addMenuEntry('Toggle AutoPlay', toggleAutoPlayCallback);

... at "Advanced / Further options / Shortcut" you can define a (even  global)
shortcut for "Toggle AutoPlay" then.

See also: Program.sleepMode, Program.addMenuEntry()


Program.addMenuEntry()
--------------------------------------------------------------------------------

    program.addMenuEntry(name, callbackFn);

Adds a button eg. for your plugin's configuration dialog to the main menu. The
name of the menu entry is defined by name. If the user selects one  of  "your"
menu entry, the given function is called. You can also add shortcuts for a menu
entry; see "Advanced / Further options / Shortcut" for this purpose.

See also: Program.addConfigButton(), Program.addSkinsButton(),
Program.addExitOption()


Program.addConfigButton()
--------------------------------------------------------------------------------

    program.addConfigButton(name, callbackFn);

Adds a button or an entry eg. for your plugin's configuration  dialog  to  the
"Advanced Page". If the user clicks the button, the given function is called.

See also: Program.addMenuEntry(), Program.addSkinsButton(),
Program.addExitOption()


Program.addSkinsButton()
--------------------------------------------------------------------------------

    program.addSkinsButton(name, callbackFn);

Adds a button eg. for your plugin's configuration dialog to the "skins  page".
If the user clicks the button, the given function is called.

See also: Program.addMenuEntry(), Program.addConfigButton(),
Program.addExitOption()


Program.addExitOption()
--------------------------------------------------------------------------------

    program.addExitOption(name, callbackFn);

Adds an option for terminating the kiosk mode and/or the sleep  mode.  If  the
user  selects  the  action  -  either  as  the  default action or as an action
selected on "Ask" - the given function is called.

Example:

    function doWhatToDo()
    {
      // well, just do what to do here ...
    }

    program.addExitOption('Special task', doWhatToDo);

See also: Program.addMenuEntry(), Program.addConfigButton(),
Program.addSkinsButton(), Program.shutdown()


Program.setTimeout()
--------------------------------------------------------------------------------

    program.setTimeout(callbackFn, timeoutMs, [continuous]);

With setTimeout()  you  can  let Silverjuke call the given callbackFn function
after a given timeout. The timeout  is  specified  in  milliseconds  with  the
second parameter, timeoutMs.

By default, the callback function is called only once after the given timeout.
By setting the third parameter, continuous to true you can let Silverjuke call
your callback perdiodically every timeoutMs milliseconds.

Example:

    // the following instruction will call the function
    // oneSecondTimer() periodically once a second:

    function oneSecondTimer()
    {
        // do what to do here ...
    }

    program.setTimeout(oneSecondTimer, 1000, true);

To remove a timeout, just use "undefined" as the callback function.

See also: Program.lastUserInput


Program.setDisplayMsg()
--------------------------------------------------------------------------------

    program.setDisplayMsg(msg, [holdMs]);

Shows the given message msg in the "display" of the main window for the number
of milliseconds given in holdMs. If holdMs is left out,  a  default  value  is
used.

Note that the room in the display may be very limited - you should create your
message as short as possible. Moreover, you should  not  display  any  message
longer than about 20 seconds.

Example:

    program.setDisplayMsg('Hello!');

See also: print(), logWarning(), logError()


Program.setSkinText()
--------------------------------------------------------------------------------

    program.setSkinText(id, text);

With this  function  you  can change the text of a box-tag. The id should be
the same string as given to the id-attribute in the box-tag and text is the
text to display.

If you use the same ID in different layout, the text is changed for all items.

Example - the skin part:

    <skin name="test">
       <layout name="test">
          <box x="0" y="0" w="90" h="20" id="myId" />
       </layout>
    </skin>

Example - the script part - here we write the current artist to the box:

    function updateBox()
    {
       program.setSkinText("myId", player.getArtistAtPos());
    }

    player.onTrackChange = updateBox;

See also: box-tag


Program.refreshWindows()
--------------------------------------------------------------------------------

    program.refreshWindows(what);

This function  refreshes  (redraws)  some windows as soon as possible. what is
the window to refresh, see the following values:

- 1 - Refresh the display
- 2 - Refresh the workspace
- 3 - Refresh the  display  and  the  workspace

Please note:  An  explicit refresh is only needed eg. if you change change the
content of the Database. Most other interface  functions  refresh  the  needed
parts automatically (eg. when using Program.search), so most times there is no
need for an explicit refresh.


Program.selectAll()
--------------------------------------------------------------------------------

    program.selectAll([state]);

Selects (state=true or left out) or deselects (state=false) all tracks in  the
workspace. This function is available in V2.52beta2 or later.

Example:

    // select all tracks
    program.selectAll();

    // deselect all tracks
    program.selectAll(false);

See also: Program.getSelection()


Program.getSelection()
--------------------------------------------------------------------------------

    selUrls = program.getSelection([what]);

Returns an  array  (see "Array Object") containing all currently selected URLs
(or files) in the  workspace  or  in  the  queue  (also  referred  to  as  the
"display").  The  selected  tracks  are  useful  eg. if you want to perform an
action on the selection eg. from Program.addMenuEntry().

The parameter "what" defines which selection should be returned:

- 0 -  Smart selection detection. Return the selection in the workspace. If
  nothing is selected there, return the selection in the queue. If  nothing
  is  selected  there  return  all tracks  in  the  queue. Only if there is
  nothing enqueued, an empty array is returned.  
  This behaviour is useful eg. if you want to do something with  some  user
  selected  tracks, but you do not want to worry about where the tracks are
  selected or where they come from. Internally, we use this  behaviour  eg.
  in  our  "burn" dialog. If you do not specify "what", this is the default 
  behaviour.
- 1 - Return the  workspace  selection.  If  nothing  is  selected  in  the
  workspace, an empty array is returned.
- 2 -  Return  the queue selection. If nothing is selected in the queue, an
  empty array is returned.

See also: Program.selectAll()


Program.getMusicSels()
--------------------------------------------------------------------------------

    array = program.getMusicSels();

Returns an array (see "Array Object") containin all the  names  of  all  music
selections available.

Note that this function may not work if Silverjuke is not yetloaded
completely (see "Program.loaded").

See also: Program.musicSel


Program.run()
--------------------------------------------------------------------------------

    program.run('c:/programs/whatever.exe');

Just run the given program. Silverjuke does not wait until the program
terminates.

Example:

    program.run('notepad.exe filetoopen.txt');

See also: Program.launchBrowser()


Program.launchBrowser()
--------------------------------------------------------------------------------

    program.launchBrowser('http://www.whatever.com');

Open the given site in the default browser.

See also: Program.run()


Program.iniRead()
--------------------------------------------------------------------------------

    value = program.iniRead(key, defaultValue);

Reads a  value  from  Silverjuke's INI-file of from the registry. The value is
identified by key which is a unique name or a path (please use  the  separator
"/").

If the  value  does  not  exist,  the  function  just returns defaultValue, if
defaultValue is ommited, an empty string aka 0 is returned.

You can read all "Silverjuke values" or your own  keys  you've  written  using
Program.iniWrite().

Example:

    // read our own settings
    mySetting = program.iniRead("myPlugin/importantSetting", 13);

    // read a Silverjuke value
    searchPath = program.iniRead("main/searchPath", "");

See also: Program.iniWrite()


Program.iniWrite()
--------------------------------------------------------------------------------

    program.iniWrite(key, value);

Writes value to  Silverjuke's  INI-file  or  to  the  registry.  The  value is
identified by key which is a unique name or a path (please use  the  separator
"/").

Example:

    // write our own settings
    program.iniWrite("myPlugin/importantSetting", 13);

See also: Program.iniRead()


Program.exportFunction()
--------------------------------------------------------------------------------

    program.exportFunction(function);

With this  method  you  can make a function available for other scripts. Other
scripts can call your function using Program.callExported() then.

Example:

    // this function will be exported ...

    function theSuperglobal(param)
    {
        alert('You have called my superglobal.\n'
            + 'You gave me the following parameter:\n'
            + param);
    }

    // ... here:

    program.exportFunction(theSuperglobal);

See also: Program.callExported()


Program.callExported()
--------------------------------------------------------------------------------

    success = program.callExported(functionName, param, ...);

With this  method you can call functions exported by other scripts or plugins.
functionName must be set to the name of the exported function;  this  must  be
the same name as the name of the exported function.

The parameters  and  the  return value are just forwarded from/to the external
function. Note that only "simple" parameters as  numbers  or  strings  can  be
given to exported function or are returned from them.

Example:

    // This example calls the exported function
    // of the Program.exportFunction() example:

    program.callExported('theSuperglobal', 'show this string');

    // call the imported function

    program.theSuperglobal();

See also: Program.exportFunction(), Program.callPlugin()


Program.callPlugin()
--------------------------------------------------------------------------------

    result = program.callPlugin(param, ...);

If the running script is executed from within a Plugin, you can  callback  the
native  part  of  the  plugin  using  this  function. You may give up to three
parameters  to  the  plugin  which  will  be  received  by   the   plugin   on
SJ_PLUGIN_CALL.

The return value is the value returned from SJ_PLUGIN_CALL.

See also: Program.callExported()


Program.gc()
--------------------------------------------------------------------------------

    program.gc();

Force a  garbage collection as soon as possible. Normally, there is no need to
start the garbage collection manually; this is done automatically from time to
time.

See also: Program.memory


Program.shutdown()
--------------------------------------------------------------------------------

    program.shutdown(mode);

With this  function  you  can  shutdown  Silverjuke.  mode can  be  one of the
following values:

- 30 - Just exit Silverjuke.
- 40 - Exit Silverjuke and shutdown the computer.
- 50 - Exit Silverjuke and reboot the computer.

Note that your script may terminate immediately when calling this function.

See also: Program.kioskMode, Program.addExitOption()


Player Object
================================================================================

The Player object gives you access  to  the  player,  it  has  one  predefined
instance,  player (written  lower  case),  a second one named prelisten may be
added in the future.


Player.time
--------------------------------------------------------------------------------

    ms = player.time;

Read/write property that contains the current position of the current track in
milliseconds. Only valid if the player is currently not stopped.

Example:

    ms =  player.time;    // find out the current postion
    player.time += 10000; // seek forward 10 seconds
    player.time -= 10000; // seek backward 10 seconds
    player.time = 60000;  // seek to "1 minute"


Player.duration
--------------------------------------------------------------------------------

    ms = player.duration;

Read only property that contains the total time of the currently playing track
in milliseconds. On errors or if there is nothing playing at the  moment,  the
property contains the value -1.


Player.volume
--------------------------------------------------------------------------------

    vol = player.volume;

Read and write property that contains the current main volume. Volumes between
0 and 255 (including) are valid.


Player.onTrackChange
--------------------------------------------------------------------------------

    player.onTrackChange = function;

If you  want  to be informed on changes affecting the currently playing track,
please set this property to a callback function.

The given function is called eg. if the next track is started automatically or
manually  or  if  Silverjuke  assumes  more or other details about a track are
available. So, this notification is sent more than one time for  the  playback
of a single track - please check the relevant information yourself to find out
a change really important to you.

Also note, that this notification is also sent if the reason  for  the  change
was your script.

    function myPlayerNotificationHandler()
    {
      // do what to do here
    }

    player.onTrackChange = myPlayerNotificationHandler;

To remove the callback function, just assign "undefined" to the property.


Player.onPlaybackDone
--------------------------------------------------------------------------------

    player.onPlaybackDone = function;

If you  want  to  be  informed  when  a track has beed played, please set this
property to a callback function.

The given function is called a little moment after the end of a track  or  eg.
if the user hits the "next" button. Silverjuke eg. increases the play count on
this notification. 

To remove the callback function, just assign "undefined" to the property.


Player.play()
--------------------------------------------------------------------------------

    player.play([startMs]);

Brings the  player  to  the  "Play"  state.  If the player is already playing,
nothing happens. Calling this command is not always exactly the same as if the
user  hits  the play button - eg. this command does not auto enqueuing or sth.
like that.

startMs is only used if the player was stopped before. In this case,  this  is
the  starting  position  in  milliseconds of the new track. You could also use
Player.time after you've started the song, but using startMs is a  little  bit
smarter and avoids crackle.


Player.pause()
--------------------------------------------------------------------------------

    player.pause();

Pauses the player. If the player is already paused, nothing happens.


Player.stop()
--------------------------------------------------------------------------------

    player.stop();

Bring the  player  to  the  "stopped" state. If the player is already stopped,
nothing happens.


Player.stopAfterThisTrack
--------------------------------------------------------------------------------

    state = player.stopAfterThisTrack;

Read/write property that contains the state of the "Stop after this track"
option. 

Example:

    state = player.stopAfterThisTrack; // find out the current state
    player.stopAfterThisTrack = state; // set the state


Player.stopAfterEachTrack
--------------------------------------------------------------------------------

    state = player.stopAfterEachTrack;

Read/write property that contains the state of the "Stop after each track"
option. 

Example:

    state = player.stopAfterEachTrack; // find out the current state
    player.stopAfterEachTrack = state; // set the state


Player.isPlaying() etc.
--------------------------------------------------------------------------------

    state = player.isPlaying();
    state = player.isPaused();
    state = player.isStopped();

Check if the player is currently playing, paused or stopped.


Player.prev()
--------------------------------------------------------------------------------

    player.prev();

Go to  the  previoustrack  in  the  playlist.  If  there is no previous track,
nothing happens.


Player.next()
--------------------------------------------------------------------------------

    player.next([ignoreAutoPlay]);

Go to the next track in the playlist.

- If there is no next track  and  AutoPlay  is  enabled  by  the  user  and
  ignoreAutoPlay is false or left out, a new track is enqueued.
- If there is no next track and ignoreAutoPlay is true, nothing happens.



Player.hasPrev()
--------------------------------------------------------------------------------

    state = player.hasPrev();

Checks if there is a previous track in the playlist.



Player.hasNext()
--------------------------------------------------------------------------------

    state = player.hasNext([ignoreAutoPlay]);

Checks if   there   is  a  next  track  in  the  playlist.  For  the  optional
ignoreAutoPlay parameter, please see Player.next() for some comments.


Player.queueLength
--------------------------------------------------------------------------------

    len = player.queueLength;

Read only property that contains the total number of tracks -  played  or  not
played - in the queue. If the queue is this value is 0.


Player.queuePos
--------------------------------------------------------------------------------

    pos = player.queuePos;

Read/write property  that contains the current playback position in the queue.
This is a number between 0 and Player.queueLength-1. Note, that there is  also
a  queue  position  if  the player is stopped or paused. If the queue is empty
this value is -1.

Example:

    currentPos = player.queuePos; // find out the current position
    player.queuePos += 2;         // go two tracks forward
    player.queuePos = 5;          // go to position "five"


Player.getUrlAtPos() etc.
--------------------------------------------------------------------------------

    url        = player.getUrlAtPos       (queuePos);
    isAutoPlay = player.getAutoplayAtPos  (queuePos);
    playCount  = player.getPlayCountAtPos (queuePos);
    artist     = player.getArtistAtPos    (queuePos);
    album      = player.getAlbumAtPos     (queuePos);
    title      = player.getTitleAtPos     (queuePos);
    ms         = player.getDurationAtPos  (queuePos);

With these methods you can find out some information  above  a  track  in  the
queue.  The  track  is  defined  by  queuePos which  is  a value between 0 and
Player.queueLength-1.

Some notes:

- You can also skip the queuePos parameter - in this  case  Player.queuePos
  is used as the postion

- The  url returned  from getUrlAtPos() is often equal to a file name. Note
  that, for different reasons, the "forward slash" may be used  as  a  path
  seperator - even on Windows where normally the "backslash" is used.

- isAutoPlay is  true if the track is marked as being "auto-played". Tracks
  marked as being auto-played may have different FX settings and/or may  be
  interrupted  when the user - or your plugin - enqueues the next non-auto-
  play track. You can also add auto-played tracks  from  your  plugin,  see
  Player.addAtPos().

- getPlayCountAtPos() returns the number of times the title was played while it
  was in the queue.  This is not the "overall" play count from the Database
  (see "Database Object").

Example:

    var next = player.queuePos+1;

    var info = "The next track is " + player.getTitleAtPos(next)
             + " by " + player.getArtistAtPos(next);

See also: Program.getSelection()


Player.addAtPos()
--------------------------------------------------------------------------------

    player.addAtPos(queuePos, file, [markAsAutoplay]);

Adds the  given  file or URL to the queue at the position defined by queuePos,
eg. if you set queuePos to 0, the new track will become the first one and  all
other  tracks  will move one step downward. If you set queuePos to -1, the new
track will be the last in the queue; this is the behaviour most often wanted.

With the flag markAsAutoplay you can optionally  mark  the  new  track  as  an
"auto-played"  track  -  tracks marked this way may have different FX settings
and/or may be interrupted when the user - or your plugin - enqueues  the  next
non-auto-play  track. To find out the auto-play state of tracks already in the
queue, see Player.getAutoplayAtPos().


Player.removeAtPos()
--------------------------------------------------------------------------------

    player.removeAtPos(queuePos);

Removes the track at the given  position  from  the  queue.  If  this  is  the
currently playing track, normally, the next track in the queue is played.


Player.removeAll()
--------------------------------------------------------------------------------

    player.removeAll();

Removes all  tracks from the queue. If auto play is enabled, new tracks may be
enqueued immediately.


Player.repeat
--------------------------------------------------------------------------------

    state = player.repeat;

Read/write property that contains the current repeat mode:

0 = repeat off  
1 = repeat single  
2 = repeat all

This property is available in V2.52beta2 or later.

See also: Rights.repeat


Player.shuffle
--------------------------------------------------------------------------------

    state = player.shuffle;

Read/write property that contains the current shuffle state. This property  is
available in V2.52beta2 or later.


Player.removePlayed
--------------------------------------------------------------------------------

    state = player.removePlayed;

Read/write property  that  contains  the  current state for the "Remove played
tracks from queue" option. This property is available in V2.60beta5 or later.


Player.avoidBoredom
--------------------------------------------------------------------------------

    flags = player.avoidBoredom;

Read/write property that contains the current "avoid boredom" flags as:

0 = avoid boredom off  
1 = avoid boredom regarding the tracks  
2 = avoid boredom regarding the artists  
3 = avoid boredom regarding the track and artists (1+2)


Rights Object
================================================================================

With the Rights object you can check the rights given to the user eg.  in  the
kiosk  mode.  The  Rights  object  has  one predefined instance, rights (lower
case), which reflects the current  settings  from  Settings  /  Kiosk  mode  /
Functionality. Note that the rights object only gives some hints - you may have 
good reasons for your script to ignore some of them.

See also: if-tag in skins


Rights.all
--------------------------------------------------------------------------------

    state = rights.all;

Read only property that is true if anything that is possible is allowed to the
user (normally outside the kiosk mode). If not, this property is set to  false
(normally inside the kiosk mode).

This property is only a hint.

See also: Program.kioskMode


Rights.credits
--------------------------------------------------------------------------------

    value = rights.credits;

Read/write property. Contains the number of credits currently availabe for the
credit system.

Example:

    rights.credits += 3; // add three credits to the credit system


Rights.useCredits
--------------------------------------------------------------------------------

    state = rights.useCredits;

This read only property contains  the  value  true if  the  credit  system  is
enabled by the user, false if not.


Rights.play
--------------------------------------------------------------------------------

    state = rights.play;

Read only  property  that  is  true if  Player.play() is allowed. If not, this
property is set to false.

This property is only a hint - Player.play() will work in any case.


Rights.pause
--------------------------------------------------------------------------------

    state = rights.pause;

Read only property that is true if Player.pause() is  allowed.  If  not,  this
property is set to false.

This property is only a hint - Player.pause() will work in any case.


Rights.stop
--------------------------------------------------------------------------------

    state = rights.stop;

Read only  property  that  is  true if  Player.stop() is allowed. If not, this
property is set to false.

This property is only a hint - Player.stop() will work in any case.


Rights.editQueue
--------------------------------------------------------------------------------

    state = rights.editQueue;

Property that is true if editing the queue is allowed in any way. If not, this
property is set to false. This property can be read and written.

"Edit" in this meaning does not include removing tracks from  the  queue.  For
this property, please have a look at Rights.unqueue

This property  is  only a hint - editing the queue by your script will work in
any case.

See also: Player.addAtPos(), Player.removeAtPos()


Rights.unqueue
--------------------------------------------------------------------------------

    state = rights.unqueue;

Property that is true if removing tracks from the queue is allowed. If not, this
property is set to false.  This property can be read and written.

This property  is  only a hint - removing tracks from the queue by your script
will work in any case.

See also: Player.removeAtPos(), Player.removeAll()


Rights.multiEnqueue
--------------------------------------------------------------------------------

    state = rights.multiEnqueue;

Property that is true if enqueueing multiple tracks at the same time (in one
step) is allowed. If not, this property is set to false. This property can be
read and written.

This property  is  only a hint - editing the queue by your script will work in
any case.

See also: Player.addAtPos()


Rights.repeat
--------------------------------------------------------------------------------

    state = rights.repeat;

Property that is true if the repeat mode can be changed. If not, this property
is set to false.

This property is only a hint - changing Player.repeat will work in any case.

See also: Player.repeat


Rights.search
--------------------------------------------------------------------------------

    state = rights.search;

Read only  property  that  is true if searching is allowed in any way. If not,
this property is set to false.

This property is only a hint.

See also: Program.search, Program.musicSel


Rights.startVis
--------------------------------------------------------------------------------

    state = rights.startVis;

Read only property that is true if starting the visualization  is  allowed  in
any way. If not, this property is set to false.

This property is only a hint.

See also: Program.visMode


Rights.volume
--------------------------------------------------------------------------------

    state = rights.volume;

Read only  property that is true if changing the main volume is allowed in any
way. If not, this property is set to false.

This property is only a hint.

See also: Player.volume


Rights.viewMode()
--------------------------------------------------------------------------------

    state = rights.viewMode(mode);

This function  checks  if a given view mode is currently available or not. The
function returns true if the mode is available, false if not.

See also: Program.viewMode


Rights.zoom
--------------------------------------------------------------------------------

    state = rights.zoom;

Read only property that is true if changing the zoom is allowed in any way. If
not, this property is set to false.

This property  is  only  a  hint.

See also: Program.zoom


Dialog Object
================================================================================

The Dialog object provides a simple way to create your own dialogs.


Dialog Constructor
--------------------------------------------------------------------------------

    dlg = new Dialog();

Create a new dialog object, you can add  controls  to  the  dialog  using  the
Dialog.addCtrl()   methods.   When   done,  you  can  show  the  dialog  using
Dialog.showModal() or Dialog.show().

For simple dialogs, please also have a look at  the  static  methods  alert(),
confirm() and prompt().


Dialog.addCtrl()
--------------------------------------------------------------------------------

    dlg.addTextCtrl      (id,  label, value);
    dlg.addMultilineCtrl (id,  label, value);
    dlg.addPasswordCtrl  (id,  label, value);
    dlg.addSelectCtrl    (id,  label, value, option1, option2, ...);
    dlg.addCheckCtrl     (id,  label, value);
    dlg.addStaticText    ([id,]label);
    dlg.addButton        (id,  label, [callbackFn]);

With the  addCtrl()  functions you add controls to a dialog object. When done,
you can show the dialog using Dialog.show() or  Dialog.showModal()  and  query
(see  "Dialog.getValue()")  the values entered by the user after the dialog is
closed (see "Dialog.close()").

- The id is any identifier unique to the dialog and is needed if  you  want
  to  set or get the values of the control. You can mix numbers and strings
  as you like.
- label is any string shown beside or inside the control.
- value is the default value of the control.

With the addButton() function you can add buttons to the dialog. If  the  user
presses the button, the given callback function is called.

- The  callbackFn callback  function  is  called in the scope of the dialog
  object.
- If you do not provide a callback function, Dialog.close() is called  when
  the button is pressed.
- Buttons  with the IDs "ok", "cancel" or "help" are shown at the bottom of
  the dialog and the labels can be left out.  The label of the button "help" may 
  be replaced by another text or icon.
- All other buttons are shown in the order as added.
- If you do not add at least an "ok" button, the buttons "ok" and  "cancel"
  are added automatically.

Example:

    var d = new Dialog;

    d.addTextCtrl('name', 'Your name:');
    d.addSelectCtrl('like', 'Like this script?', 0, 'yes', 'no');

    if( d.showModal() == 'ok' )
    {
        usersName     = d.getValue('name');
        likeOrDislike = d.getValue('like');
    }

See also: Dialog.showModal(), Dialog.show(), Dialog.getValue(), 
Dialog.setValue()


Dialog.getValue()
--------------------------------------------------------------------------------

    value = dlg.getValue(id);

Returns the current value of the control identified by id. The id is the value
given to Dialog.addCtrl().


Dialog.setValue()
--------------------------------------------------------------------------------

    dlg.setValue(id, newValue);

Sets a  new  value for the control identified by id. The id is the value given
to Dialog.addCtrl().


Dialog.show()
--------------------------------------------------------------------------------

    dlg.show();
    Dialog.show(type);

Shows the dialog and let the user change the  values.  To  close  the  dialog,
please   use   the  Dialog.close()  method  eg.  in  a  button  callback  (see
"Dialog.addCtrl()").

There is also a static implementation of this function which can  be  used  to
open  some  predefined dialogs; in this case, use one of the following strings
as the argument type:

- console - show the console window
- settings - show the  settings  dialog,  additional  parameters  are  just
  forwarded to the dialog
- musicsel - show the "music selection" dialog
- openfiles -  show  the  "open  files" dialog and let the user select some
  files to enqueue; the second  parameter  may  be  set  to  true to  force
  appending.
- saveplaylist -  show  the "save playlist" dialog

Example:

    // show a user-defined dialog
    var d = new Dialog;

    d.addTextCtrl('name', 'Your name:');
    d.addButton('ok', '', function(){alert('Thanks!'); close();});
    d.addButton('cancel');

    d.show();

    // show the predefined "settings" dialog
    Dialog.show('settings');

See also: Dialog.showModal()


Dialog.showModal()
--------------------------------------------------------------------------------

    result = dlg.showModal();

Shows the dialog and let  the  user  change  the  values.  When  the  function
returns,  the  dialog  is  already  closed  and you can query the values using
Dialog.getValue(). The return value is the ID of the button  that  closes  the
dialog.

Example:

    var d = new Dialog;

    d.addTextCtrl('name', 'Your name:');
    d.addButton('ok');
    d.addButton('cancel');

    if( d.showModal() == 'ok' )
    {
        alert('Thanks!')
    }

See also: Dialog.show()


Dialog.close()
--------------------------------------------------------------------------------

    dlg.close();

Closes the  dialog,  normally you will call this function from within a button
callback (see "Dialog.addCtrl()").

You can find out the values entered by the user  using  the  Dialog.GetValue()
function.


Database Object
================================================================================

With the database object you can access the sqlite database used by Silverjuke.
You have the full power of SQL - for a list of used fields in the main table
"tracks", see the topic SQL Expressions in the User Manual or inspect the
database directly.

Moreover, you can create your own databases.

Silverjuke's database uses the sqlite3 engine. If needed, you can access the 
database eg. using the sqlite3explorer or via ODBC.

- A list of tools for sqlite can be found at 
  http://www.sqlite.org/cvstrac/wiki?p=SqliteTools.

- For ODBC drivers eg. for use with Excel, please have a look at the following 
  search results: http://www.google.com/search?q=sqlite+odbc 

The tables of interest are normally "tracks" and "albums"; the used fields in 
these tables should be self-explaining.

However, note that using the Silverjuke database outside of Silverjuke is not an 
official feature and that the Silverjuke database format is always subject to
change without notice.


Database Constructor
--------------------------------------------------------------------------------

    db = new Database([file]);

Constructs a new database object.

- If  you do not specify a file, the database object will be an an accessor
  to the default database of Silverjuke.

- If you specify a file, you can access any other database. If the database
  file does not exist, it is created.

To execute SQL statements on the database, use Database.openQuery().


Database.openQuery()
--------------------------------------------------------------------------------

    success = database.openQuery(sqlStatement);

Execute any  SQL  statement.  If  the  statement returns a result, this can be
queried using the  functions  Database.nextRecord()  and  Database.getField().
Please  do  not  forget  to  close  the  query using  Database.closeQuery()  -
depending on the type of query some tables may stay locked otherwise.

On success, the function returns true. For errors, false is returned.

Example:

    // find out the composer of the playing track
    var url = player.getUrlAtPos();

    var db = new Database;

    db.openQuery("select composername from tracks where url='" + url + "';");

    if( db.nextRecord() )
    {
        alert("The composer of the playing track is " + db.getField(0));
    }

    db.closeQuery();

The example above is a  little  bit  buggy  for  strings  containing  a  quote
character - these characters should be replaced by two quotes (not: the double
quote). So the 6th line should read as follows:

    var quoteExpr = new RegExp("'", "g");
    db.openQuery("select composername from tracks where url='"
                 + url.replace(quoteExpr, "''") + "';");

The most common database usage will be to query for information as done above.
For  a  list  of  used  fields  in  the main table "tracks", see the topic "SQL
Expressions" in the User Manual or inspect the database directly.

However, you  can  also  change  the tables using UPDATE, INSERT or DELETE (in
this case, you can speed up processing  by  using  transaction  as  BEGIN  and
COMMIT or ROLLBACK statements). Please do not expect Silverjuke to reflect all
changes immediately; for speed reasons many things are cached  in  a  way  not
easy to describe here. If in doubt, please contact us.

Moreover, if you need your own databases or columns - no problem, you can also
use the CREATE TABLE or ALTER TABLE commands. However, please also contact  us
in this case, so that we can avoid incompatibilities.

See also: Database.nextRecord(), Database.getField(), Database.closeQuery()


Database.nextRecord()
--------------------------------------------------------------------------------

    hasNext = database.nextRecord();

Moves the record pointer in the result returned by Database.openQuery() to the
next record. If there is a next record, true is returned; if there are no more
records, the function returns false. To find out the values of the record, use
Database.getField(). If the submitted query does not return a result set, this
function is useless.

See Database.openQuery() for an example.


Database.getFieldCount()
--------------------------------------------------------------------------------

    cnt = database.getFieldCount();

Returns the   number  of  fields  in  each  row  of  the  result  returned  by
Database.openQuery().  The  return  value  is  equal  eg.  to  the  number  of
fields/columns in a "SELECT" statement.

To find out the values of the field, use Database.getField().


Database.getField()
--------------------------------------------------------------------------------

    value = database.getField(n);

Returns the value of the nth field of the current record. n is a value between
0 and the Database.getFieldCount() minus 1. To move to the  next  record,  use
Database.nextRecord().

Depending on  the  type  of  the  query, you can also find out some additional
information with the following values for n:

- If you set n to -1, the function returns the  "insert  id"  of  the  last
  insert statement.
- If  you set n to -2, the function returns the number of affected rows eg.
  of an update or delete statement.

Please note, that getField always returns a string, independently of  what  is
queried  in  the  SQL statement. So, under some circumstances an explicit type
conversion may be needed; this can be done by some mathematic statements or by
using the Number constructor:

    db = new Database;
    db.openQuery("select count(*) from albums;");

    foo = db.getField(0);

    alert(foo + 1)              // for 20 record, this shows "201" :-(
    bar = 1*foo; alert(bar + 1) // for 20 record, this shows "21" :-)
    alert(Number(foo) + 1)      // for 20 record, this shows "21" :-)

    db.closeQuery();


Database.closeQuery()
--------------------------------------------------------------------------------

    value = database.closeQuery();

Closes the  query (see "Database.openQuery()") and unlocks all used tables. We
recommend always to call this function after a query as  soon  as  possible  -
depending on the query some tables may get unusable for Silverjuke otherwise.


Database.getFile()
--------------------------------------------------------------------------------

    file = database.getFile();

This function  returns the file name and path used as the physical storage for
the  database.  Normally,  this  is  the  same  file  name  as  given  to  the
constructor;  if  you  have  not  given  any file name to the constructor this
function returns the file name of the default database.

See also: Database Constructor


Static Database Functions
--------------------------------------------------------------------------------

    success = Database.update([deepUpdate]);

With Database.update() you synchronize the default database with the  assigned
music sources and their meta data. By default, only new or changed sources are
checked; if you set deepUpdate to true all files will  be  rescanned.  Calling
this function has the same effect as pressing eg. F5 in Silverjuke.

The function  returns  true if the update was completed. For errors, or if the
user aborts the update, false is returned.


File Object
================================================================================

With the file object you can read or write any file in the file system.


File Constructor
--------------------------------------------------------------------------------

    file = new File(name, [binary]);

Constructs a new file object. name is the file to open. You should always  use
the  forward  slash  instead  of  the  backslash in file names, eg. please use
"c:/filename.txt" instead of "c:\filename.txt".

The file can be opened in two modes:

- If binary is set to true, the file is opened in binary mode: All  strings
  read  and  written will contain exactly one byte per character. This mode
  can also be used if the file contains ISO 8859-1 encoded text.
- If binary is left out or is set to false, the file  is  opened  in  UTF-8
  mode: The file should be an UTF-8 encoded text then.

To access  the  file  data,  use eg. File.read() or File.write(). Although the
file is closed automatically by the garbage collection, it may be a good  idea
to call File.flush() as soon as you're done with the file.


File.length
--------------------------------------------------------------------------------

    value = file.length;

Property which  reflects the current size in bytes of the file. The length may
change if you write over the end of the file using File.write().

You can also empty or truncate files using the property.

Example:

    fileBytes = file.length; // find out the size
    file.length = 0;         // empty the file
    file.length = 100;       // truncate the file to 100 bytes
    file.length -= 32;       // make the file 32 bytes smaller


File.pos
--------------------------------------------------------------------------------

    value = file.pos;

Read and write property which reflects the current read/write position that is
used for reading and writing. The position is always relative to the beginning
of the file, however, you can give negative values to seek from the end.

Every read and write call will increase the position by the  number  of  bytes
read or written.

Example:

    bytesFromBeg = file.pos; // find out the current position
    file.pos = 0;            // move to start of file
    file.pos = file.length;  // move to end of file
    file.pos = 100;          // move the postion to byte 100
    file.pos = -100;         // move to 100 bytes from end
    file.pos += 32;          // move the pointer 32 bytes forward
    file.pos -= 32;          // move the pointer 32 bytes backward


File.read()
--------------------------------------------------------------------------------

    data = file.read([length]);

Reads and returns data from the given file. The data is returned as a string.

- If  you  give  the length argument, the number of bytes are read from the
  file.
- If you do not give the  length argument,  the  next  line  is  read.  The
  returned string may contain the character "\n" at its end.

The File.pos is increased by the number of bytes read.

See also: File.write(), Binary mode (see "File Constructor")


File.write()
--------------------------------------------------------------------------------

    file.write(data);

Write data to the given file at the current position.

- If data is a string, this string is written to the file. If you want to add a
  line end after the string, you have add eg. `\n` to data explicitly.
- If data is a number, a single byte is written to the file.

The File.pos  is  increased  by the number of bytes written, if you write over
the end of the file, this will also changed File.length.

Example:

    f = new File('c:/temp/test');

    f.write('hello');      // writes 5 bytes
    f.write('two\nlines'); // writes 2 lines (9 bytes)

    f.write(12);           // writes 1 byte to the file

    f.write(0);            // write a null-byte

See also: File.read(), File.flush(), Binary mode (see "File Constructor")


File.flush()
--------------------------------------------------------------------------------

    file.flush();

Flushes the given file and closes it temporarily.  Although  closing  is  also
done  by  the  garbage  collection,  closing  the file explicitly allows other
programs to access the file immediately.

If you use any other file method or property after calling flush(),  the  file
is opened again.


Static File Functions
--------------------------------------------------------------------------------

    state   = File.exists(path);
    state   = File.isdir(path);
    array   = File.dir(path, [what]);
    success = File.mkdir(path);
    success = File.copy(src, dest);
    success = File.rename(src, dest);
    success = File.remove(path);
    ms      = File.time(path);

These functions  provide some file operations are not bound to a specific file
object.

- File.exists() returns true if the given path specifies an  existing  file
  or directory; else false is returned.
- File.isdir()   returns  true if  the  given  path specifies  an  existing
  directory; else false is returned.
- File.dir() searches the given path for files (set what to 0 or skip  this
  parameter),  for directories (set what to 1) or for both (set what to 2).
  The found files or directories are returned as an array of strings.
- File.mkdir() tries to create the directory path.
- File.copy() tries to copy the file src to dest; the destination  file  is
  overwritten, if it exists.
- File.rename() renames the file src to dest. The destination must be given
  as a full path; the function may also be used to move files therefore.
- File.remove() deletes the given file path
- File.time() returns the timestamp of the last modification, you can  give
  this timestamp to the Date Constructor to convert it to a Date Object.

Examples:

    // create directories
    if( !File.isdir('c:/temp') )
    {
        File.mkdir('c:/temp');
        File.mkdir('c:/temp/subdir');
    }

    // print all files and directory in c:/temp
    files = File.dir('c:/temp', 2);
    for( i = 0; i < files.length; i++ )
    {
        if( File.isdir(files[i]) ) {
            print('['+files[i]+']');
        }
        else {
            print(files[i]);
        }
    }

    // rename a file
    File.rename('c:/temp/bla.txt', 'c:/temp/blub.txt');

    // delete a file
    File.remove('c:/temp/blub.txt');


HttpRequest Object
================================================================================

With the  HttpRequest  object  you  can  read or post data from/to web servers
using the HTTP protocol.


HttpRequest Constructor
--------------------------------------------------------------------------------

    httprequest = new HttpRequest();

Constructs a new HttpRequest object which can be used for HTTP requests then.

See also: HttpRequest.request()


HttpRequest.responseText
--------------------------------------------------------------------------------

    data = httprequest.responseText;

This property  contains   the   result   of   a   request   started   eg.   by
HttpRequest.request().

See also: HttpRequest.getResponseHeader(), HttpRequest.status


HttpRequest.status
--------------------------------------------------------------------------------

    state = httprequest.status;

This property  contains  the  numeric  status  code  in  response to a request
started eg. by HttpRequest.request().

On, success, the status code is normally 200  (OK).  Other  status  codes  may
indicate  errors  or  warnings, eg. 404 (Not Found) or 400 (Bad Request - also
used if no data are requested yet).

For a    list    of    possible    status    codes,    please     refer     to
http://www.faqs.org/rfcs/rfc2616.html .

See also: HttpRequest.responseText, HttpRequest.getResponseHeader()


HttpRequest.request()
--------------------------------------------------------------------------------

    httprequest.request(url, [postData], callbackFn);

With this  method  you  can  retrieve  the  resource  defined by url using the
methods GET or POST (set or skip postData).  Any  request  started  before  is
aborted when this function is called.

The data  are  loaded asynchronous. When done, the callback function is called
which should check  the  HttpRequest.status  and  can  read  the  result  from
HttpRequest.responseText.

Example:

    // the following function is called when the request is done
    function myCallback()
    {
        alert("done - status is " + this.status);
    }

    // start a GET request
    obj = new HttpRequest();
    obj.request("http://foo.com/?param1=data&param2=cont", myCallback);

See also: HttpRequest.abort()


HttpRequest.setRequestHeader()
--------------------------------------------------------------------------------

    httprequest.setRequestHeader([header, value]);

Set header for the next request(s) started eg. by HttpRequest.request().

Request headers   are   not   cleared   by   calling   HttpRequest.abort()  or
HttpRequest.request(); for this purpose, please call this function without any
parameters.

Especially when  using the POST method on HttpRequest.request(), do not forget
to set the correct content type:

Example:

    // the following function is called when the request is done
    function myCallback()
    {
        alert("done - status is " + this.status);
    }

    // start a POST request
    obj = new HttpRequest();
    obj.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
    obj.request("http://foo.com/", "param1=data&param2=cont", myCallback);

See also: HttpRequest.request(), HttpRequest.getResponseHeader()


HttpRequest.getResponseHeader()
--------------------------------------------------------------------------------

    value = httprequest.getResponseHeader(header);

Returns a   header   of   a   response   to   a   request   started   eg.   by
HttpRequest.request().

See also: HttpRequest.responseText, HttpRequest.status


HttpRequest.abort()
--------------------------------------------------------------------------------

    httprequest.abort();

Stops a  request  started eg. by HttpRequest.request(). Moreover, calling this
function will initialize HttpRequest.status to 400 (Bad Request).


Math Object
================================================================================

The Math object is a single object that  has  some  properties  and  functions
useful for working with numbers.


Math Constants
--------------------------------------------------------------------------------

    value = Math.E;
    value = Math.PI;
    value = Math.LN10;
    value = Math.LN2;
    value = Math.LOG2E;
    value = Math.LOG10E;
    value = Math.SQRT1_2;
    value = Math.SQRT2;

The Math objects defines some constant properties as follows:

- E - The number value for e, the base of the natural logarithms, which is
  approximately 2.7182818284590452354.
- PI - The number value for "PI", the ratio of the circumference of a circle
  to its diameter, which is approximately 3.1415926535897932.
- LN10 -  The number value for the natural logarithm of 10, which is
  approximately 2.302585092994046.
- LN2 - The number value for the natural logarithm of 2, which is
  approximately 0.6931471805599453.
- LOG2E - The number value for the base-2 logarithm of e, the base of the
  natural logarithms; this value is approximately 1.4426950408889634. The
  value of Math.LOG2E is approximately the reciprocal of the value of
  Math.LN2.
- LOG10E - The number value for the base-10 logarithm of e, the base of the
  natural logarithms; this value is approximately 0.4342944819032518. The
  value of Math.LOG10E is approximately the reciprocal of the value of
  Math.LN10.
- SQRT1_2 - The number value for the square root of 1/2, which is
  approximately  0.7071067811865476. The value of Math.SQRT1_2 is
  approximately the reciprocal of the value of Math.SQRT2.
- SQRT2 - The number value for the square root of 2, which is approximately
  1.4142135623730951.

Obviously, all properties are read only.


3.2.8.2  Math.sin() etc.
--------------------------------------------------------------------------------

    result = Math.sin(x);
    result = Math.cos(x);
    result = Math.tan(x);
    result = Math.asin(x);
    result = Math.acos(x);
    result = Math.atan(x);
    result = Math.atan2(y, x);

Some trigonometry functions:

- sin(x) - Returns an approximation to the  sine  of  x.  The  argument  is
  expressed in radians.
    - If x is NaN, the result is NaN
    - If x is 0, the result is 0
    - If x is +oo or -oo, the result is NaN

- cos(x) -  Returns  an  approximation  to the cosine of x. The argument is
  expressed in radians.
    - If x is NaN, the result is NaN
    - If x is 0, the result is 1
    - If x is +oo, the result is NaN
    - If x is -oo, the result is NaN
- tan(x) - Returns an approximation to the tangent of x.  The  argument  is
  expressed in radians.
    - If x is NaN, the result is NaN
    - If x is 0, the result is 0
    - If x is +oo or -oo, the result is NaN
- asin(x) -  Returns  an  approximation to the arc sine of x. The result is
  expressed in radians and ranges from -PI/2 to +PI/2.
    - If x is NaN, the result is NaN
    - If x is greater than 1, the result is NaN
    - If x is less than -1, the result is NaN
    - If x is 0, the result is 0
- acos(x) - Returns an approximation to the arc cosine of x. The result  is
  expressed in radians and ranges from +0 to +PI.
    - If x is NaN, the result is NaN
    - If x is greater than 1, the result is NaN
    - If x is less than -1, the result is NaN
    - If x is exactly 1, the result is 0
- atan(x) - Returns an approximation to the arc tangent of x. The result is
  expressed in radians and ranges from -PI/2 to +PI/2.
    - If x is NaN, the result is NaN
    - If x is 0, the result is 0
    - If x is +oo, the result is an approximation to +PI/2
    - If x is -oo, the result is an approximation to -PI/2
- atan2(y, x) - Returns an approximation to the arc tangent of the quotient y/x
  of the arguments y and x, where the signs of y and x are used to determine the 
  quadrant of the result. Note that it is intentional and traditional for the 
  two-argument arc tangent function that the argument named y be first and the 
  argument named x b  second. The result is expressed in radians and ranges 
  from -PI to +PI.
    - If either x or y is NaN, the result is NaN
    - If y>0 and x is 0, the result is an approximation to +PI/2
    - If y is 0 and x>0, the result is 0
    - If y<0 and x is 0, the result is an approximation to -PI/2
    - If y>0 and y is finite and x is +oo, the result is 0
    - If y<0 and y is finite and x is +oo, the result is 0
    - If y>0 and y is finite and x is -oo, the result if an approximation to +PI
    - If y<0 and y is finite and x is -oo, the result is an approximation to -PI
    - If y is +oo and x is finite, the result is an approximation to +PI/2
    - If y is -oo and x is finite, the result is an approximation to -PI/2
    - If y is +oo and x is +oo, the result is an approximation to +PI/4
    - If y is +oo and x is -oo, the result is an approximation to +3PI/4
    - If y is -oo and x is +oo, the result is an approximation to -PI/4
    - If y is -oo and x is -oo, the result is an approximation to -3PI/4


Math.abs()
--------------------------------------------------------------------------------

    result = Math.abs(x):

Returns the  absolute  value  of x; the result has the same magnitude as x but
has positive sign.

Some special cases:

- If x is NaN, the result is NaN.
- If x is -oo, the result is +oo.


Math.ceil()
--------------------------------------------------------------------------------

    result = Math.ceil(x);

Returns the smallest number value that is not less than x and is  equal  to  a
mathematical  integer.  The  value of Math.ceil(x) is the same as the value of
-Math.floor(-x).

Some special cases:

- If x is already an integer, the result is x.
- If x is NaN, the result is NaN.
- If x is 0, the result is 0.
- If x is +oo, the result is +oo.
- If x is -oo, the result is -oo.
- If x is less than 0 but greater than -1, the result is 0.


Math.exp()
--------------------------------------------------------------------------------

    result = Math.exp(x);

Returns an approximation to the exponential function of x  (e  raised  to  the
power of x, where e is the base of the natural logarithms).

Some special cases:

- If x is NaN, the result is NaN.
- If x is 0, the result is 1.
- If x is +oo, the result is +oo.
- If x is -oo, the result is 0.


Math.floor()
--------------------------------------------------------------------------------

    result = Math.floor(x);

Returns the greatest number value that is not greater than x and is equal to a
mathematical integer. The value of Math.floor(x) is the same as the  value  of
-Math.ceil(-x).

Some special cases:

- If x is already an integer, the result is x.
- If x is NaN, the result is NaN.
- If x is 0, the result is 0.
- If x is +oo, the result is +oo.
- If x is -oo, the result is -oo.
- If x is greater than 0 but less than 1, the result is 0.


Math.log()
--------------------------------------------------------------------------------

    result = Math.log(x);

Returns an approximation to the natural logarithm of x.

Some special cases:

- If x is NaN, the result is NaN.
- If x is less than 0, the result is NaN.
- If x is 0, the result is -oo.
- If x is 1, the result is 0.
- If x is +oo, the result is +oo.


Math.max(), Math.min()
--------------------------------------------------------------------------------

    result = Math.max(value1, value2, ...);
    result = Math.min(value1, value2, ...);

Returns the largest/smallest of the given values.

Some special cases:

- If no arguments are given, the result is -oo/+oo
- If any value is NaN, the result is NaN.


3.2.8.10  Math.pow()
--------------------------------------------------------------------------------

    result = Math.pow(x, y);

Returns an approximation of raising x to the power y.

Some special cases:

- If y is NaN, the result is NaN.
- If y is 0, the result is 1, even if x is NaN.
- If x is NaN and y is nonzero, the result is NaN.
- If abs(x)>1 and y is +oo, the result is +oo.
- If abs(x)>1 and y is -oo, the result is 0.
- If abs(x)==1 and y is +oo, the result is NaN.
- If abs(x)==1 and y is -oo, the result is NaN.
- If abs(x)<1 and y is +oo, the result is 0.
- If abs(x)<1 and y is -oo, the result is +oo.


Math.random()
--------------------------------------------------------------------------------

    result = Math.random();

Returns a number value with positive sign, greater than or equal to 0 but less
than 1, chosen pseudo randomly with an approximately uniform distribution over
that range.


Math.round()
--------------------------------------------------------------------------------

    result = Math.round(x);

Returns the  integer  number value that is closest to x. If two integer number
values are equally close to x, the larger one is returned.

Some special cases:

- If x is already an integer, the result is x.
- If x is NaN, the result is NaN.
- If x is +oo, the result is +oo
- If x is -oo, the result is -oo


Math.sqrt()
--------------------------------------------------------------------------------

    result = Math.sqrt(x);

Returns an approximation to the square root of x.

Some special cases:

- If x is NaN, the result is NaN.
- If x less than 0, the result is NaN.
- If x is 0, the result is 0.
- If x is +oo, the result is +oo.


Date Object
================================================================================

The Date object offers you some method to work with the time.


Date Constructor
--------------------------------------------------------------------------------

    obj = new Date();
    obj = new Date(milliseconds);
    obj = new Date(dateString);
    obj = new Date(year, month, dayNum[, hour, minute, second[, ms]]);

Constructs a new Date object.

You can construct a date object using the number of milliseconds since January
1,  1970,  00:00:00,  using a formatted date string (see Date.parse()) or by a
some concrete values.

For the month, the function expects january=0, february=1 and so on.

If you do not specify any arguments, the current date and time is set  to  the
object.


Date.parse()
--------------------------------------------------------------------------------

    ms = Date.parse("Tue, 1 Jan 2000 00:00:00 GMT");

This static  method  parses a string representation of a date, and returns the
number of milliseconds since January 1, 1970, 00:00:00 (universal time if  you
append "GMT" to the string).


Date.UTC()
--------------------------------------------------------------------------------

    ms = Date.UTC(year, month, dayNum[, hour, minute, second[, ms]]);

This static  method  accepts  the  same  parameters as the longest form of the
constructor, and returns the number of milliseconds in  a  Date  object  since
January 1, 1970, 00:00:00, universal time.

For the month, the function expects january=0, february=1 and so on.


Date.get()
--------------------------------------------------------------------------------

    var obj = Date();
    result = obj.getDate();
    result = obj.getDay();
    result = obj.getFullYear();
    result = obj.getHours();
    result = obj.getMilliseconds();
    result = obj.getMinutes();
    result = obj.getMonth();
    result = obj.getSeconds();
    result = obj.getTime();
    result = obj.getTimezoneOffset();
    result = obj.getUTCDate();
    result = obj.getUTCDay();
    result = obj.getUTCFullYear();
    result = obj.getUTCHours();
    result = obj.getUTCMilliseconds();
    result = obj.getUTCMinutes();
    result = obj.getUTCMonth();
    result = obj.getUTCSeconds();

With the Date.get() methods you can find out some values of a Date object.

- getDate() - Returns the day of the month for the specified date according to
  local time.
- getDay() - Returns the day of the week for the specified date according to
  local time.
- getFullYear() - Returns the year of the specified date according to local
  time.
- getHours() - Returns the hour in the specified date according to local time.
- getMilliseconds() - Returns the milliseconds in the specified date according
  to local time.
- getMinutes() - Returns the minutes in the specified date according to local
  time.
- getMonth() - Returns the month in the specified date according to local
  time (0=january, 1=february etc.).
- getSeconds() - Returns the seconds in the specified date according to local
  time.
- getTime() - Returns the numeric value corresponding to the time for the
  specified date according to universal time. The result is in milliseconds
  since January 1, 1970, 00:00:00.
- getTimezoneOffset() - Returns the time-zone offset in minutes for the current
  locale.
- getUTCDate() - Returns the day (date) of the month in the specified date
  according to universal time.
- getUTCDay() - Returns the day of the week in the specified date according
  to universal time.
- getUTCFullYear() - Returns the year in the specified date according to
  universal time.
- getUTCHours() - Returns the hours in the specified date according to
  universal time.
- getUTCMilliseconds() - Returns the milliseconds in the specified date
  according to universal time.
- getUTCMinutes() - Returns the minutes in the specified date according to
  universal time.
- getUTCMonth() - Returns the month in the specified date according to
  universal time (0=january, 1=february etc.).
- getUTCSeconds() - Returns the seconds in the specified date according to
  universal time.


3.2.9.5  Date.set()
--------------------------------------------------------------------------------

    obj = new Date("June 17, 1973 19:27:12");
    obj.setDate(newDayNum);
    obj.setFullYear(newYear);
    obj.setHours(newHours);
    obj.setMilliseconds(newMs);
    obj.setMinutes(newMinutes);
    obj.setMonth(newMonth);
    obj.setSeconds(newSeconds);
    obj.setTime(newTime);
    obj.setUTCDate(newDayNum);
    obj.setUTCFullYear(newYear);
    obj.setUTCHours(newHours);
    obj.setUTCMilliseconds(newMs);
    obj.setUTCMinutes(newMinutes);
    obj.setUTCMonth(newMonth);
    obj.setUTCSeconds(newSeconds);

With the Date.set() methods, you can set some components a date is built of:

- setDate() - Sets the day of the month for a specified date according to local
  time.
- setFullYear() - Sets the full year for a specified date according to local
  time.
- setHours() - Sets the hours for a specified date according to local time.
- setMilliseconds() - Sets the milliseconds for a specified date according to
  local time.
- setMinutes() - Sets the minutes for a specified date according to local time.
- setMonth() - Sets the month for a specified date according to local time
  (0=january, 1=february etc.).
- setSeconds() - Sets the seconds for a specified date according to local time.
- setTime() - Sets the value of the Date object according to local time.  The
  argument is expected as milliseconds since January 1, 1970, 00:00:00.
- setUTCDate() - Sets the day of the month for a specified date according to
  universal time.
- setUTCFullYear() - Sets the full year for a specified date according to
  universal time.
- setUTCHours() - Sets the hour for a specified date according to universal
  time.
- setUTCMilliseconds() - Sets the milliseconds for a specified date according
  to universal time.
- setUTCMinutes() - Sets the minutes for a specified date according to
  universal time.
- setUTCMonth() - Sets the month for a specified date according to universal
  time (0=january, 1=february etc.).
- setUTCSeconds() - Sets the seconds for a specified date according to universal
  time.


Date.toString()
--------------------------------------------------------------------------------

    result = date.toString();

Returns a string representing the specified Date object.


Date.toLocaleString()
--------------------------------------------------------------------------------

    result = date.toLocaleString();
    result = date.toLocaleDateString();
    result = date.toLocaleTimeString();

Returns the "date" and/or portion of the Date as a string, using  the  current
locale's conventions.


Date.toUTCString()
--------------------------------------------------------------------------------

result = date.toUTCString();

Converts a date to a string, using the universal time convention.


Array Object
================================================================================

The array object provides everything you should need to work with arrays.


Array Constructor
--------------------------------------------------------------------------------

    var arrObj1 = new Array(arrayLength);
    var arrObj2 = new Array(elem0, elem1, ..., elemN);

If you  give  arrayLength to  the constructor, an array with the given initial
length is constructed. You can access this value  using  the  length  property
(see  "Array.length").  If  the  value  specified is not a number, an array of
length 1 is created, with the first element having the  specified  value.  The
maximum length allowed for an array is 4,294,967,295.

The second  form  constructs  an  array  from  a  list of values; the array is
initialized with the specified values as its elements, and the array's  length
property is set to the number of arguments.


Array.length
--------------------------------------------------------------------------------

    value = array.length;

Property which reflects the length of the array (the number of elements in the
array). For an empty array, length is 0.

You can set the length property to truncate an array at  any  time.  When  you
extend an array by changing its length property, the number of actual elements
does not increase; for example, if you set length to 3 when it is currently 2,
the array still contains only 2 elements.


Array.pop()
--------------------------------------------------------------------------------

    elem = array.pop();

Removes the last element from an array and returns that element.


Array.push()
--------------------------------------------------------------------------------

    newLength = array.push(elem0, elem1, ..., elemN);

Adds one or more elements to the end of an array and returns the new length of
the array.


Array.reverse()
--------------------------------------------------------------------------------

    array.reverse();

Reverses the order of the elements of an array - the first becomes  the  last,
and the last becomes the first.

Example:

    myArray = new Array("one", "two", "three");
    myArray.reverse();

    // myArray[0] is now "three"
    // myArray[1] is now "two"
    // myArray[2] is now "one"


Array.shift()
--------------------------------------------------------------------------------

    elem = array.shift();

Removes the first element from an array and returns that element.


Array.sort()
--------------------------------------------------------------------------------

    array.sort([fn]);

Sorts the  elements  of  an  array.  For  this  purpose, you have to provide a
function that can compare two elements in fn:

- If fn(a, b) returns a value less than 0, b is sorted to a lower index than a.
- If fn(a, b) returns 0, a and b are assumed to be equal.
- If fn(a, b) returns a value greater than 0, b is sorted to a higher index
  than a.

So, a compare function for sorting numbers can have the following form:

    function compareNumbers(a, b)
    {
       if( a < b ) {    // a is less than b
            return -1;
       }
       if( a > b ) {    // a is greater than b
            return 1;
       }
       return 0;        // a must be equal to b
    }

Or more simple:

    function compareNumbers(a, b)
    {
        return a - b;
    }

If you do not specify a compare function, the array is sorted lexicographically
(in dictionary order) according to the string conversion of each element.

Example:

    var test = new Array("yellow", "green", "red");
    test.sort();

    // test[0] is now "green"
    // test[1] is now "red"
    // test[2] is now "yellow"


Array.splice()
--------------------------------------------------------------------------------

    removed = Array.splice(index, count, [elem1, elem2, ..., elemN]);

Changes the array by removing count elements at position index and  optionally
adds elem1, elem2, ..., elemN instead.

- index is the offset in the array at which to start changing the array; this
  may be a value between 0 and Array.length-1.
- count are the number of elements to remove at the given offset; if set to
  0, no elements are removed; in this case, you should specify at least one
  new element.
- elem1, elem2, ..., elemN are the elements to add to the array.  If you don't
  specify any elements, splice simply removes elements from the array.
- If you specify a different number of elements to insert than the number you're
  removing, the array will have a different length at the end of the call.

The splice method returns an array containing the removed  elements.  If  only
one element is removed, an array of one element is returned.

Example:

    var test = new Array("yellow", "green", "red");
    removed = test.splice(1, 1, "blue");

    // test[0] is now "yellow"
    // test[1] is now "blue"
    // test[2] is now "red"
    // test.length is still 3
    // removed[0] is "green"

    removed = test.splice(1, 1);

    // test[0] is now "yellow"
    // test[2] is now "red"
    // test.length is now 2
    // removed[0] is "blue"


Array.unshift()
--------------------------------------------------------------------------------

    newLength = array.unshift(elem0, elem1, ..., elemN);

Adds one  or more elements to the front of an array and returns the new length
of the array.


Array.concat()
--------------------------------------------------------------------------------

    newArray = array.concat(value1, value2, ..., valueN);

Returns a new array built out of the given array plus  other  array(s)  and/or
value(s).

Example:

    colours = new Array("red", "green", "blue");
    fruits = new Array("apple", "banana");
    together = colours.concat(fruits);

    // together[0] is set to "red"
    // together[1] is set to "green"
    // together[2] is set to "blue"
    // together[3] is set to "apple"
    // together[4] is set to "banana"

    evenMore = together.concat(1, 2);

    // evenMore[0] is set to "red"
    // evenMore[1] is set to "green"
    // evenMore[2] is set to "blue"
    // evenMore[3] is set to "apple"
    // evenMore[4] is set to "banana"
    // evenMore[5] is set to 1
    // evenMore[6] is set to 2


Array.join()
--------------------------------------------------------------------------------

    string = array.join(separator);

Joins all  elements of an array to a single string. The elements are separated
by the given separator string; if left out, the comma is used to separate  the
elements.

Example:

    colours = new Array("red", "green", "blue");

    t1 = colours.join();        // t1 is set to "red,green,blue"
    t2 = colours.join(" and "); // t2 is set to "red and green and blue"
    t3 = colours.join("");      // t3 is set to "redgreenblue"


Array.slice()
--------------------------------------------------------------------------------

    newArray = array.slice(begin, [end]);

Returns a  section  of the array as a new array. The section is defined by the
parameters begin and end:

- begin - Zero-based index at which to begin extraction.
- end - Zero-based index at which to end extraction. slice extracts up to but
  not including end. If end is omitted, slice extracts to the end of the array.

Example:

    var test = new Array("yellow", "green", "red", "blue");
    part = test.slice(1, 3);

    // part.length is 2
    // part[0] is "green"
    // part[1] is "red"
    // "test" is not changed


String Object
================================================================================

The string object provides everything you should need to work with strings.

See also: RegExp Object


String Constructor
--------------------------------------------------------------------------------

    var string = "A new string object";

The statement above construct a new string object that can  be  use  with  all
string methods (see "String Object").


String.length
--------------------------------------------------------------------------------

    value = string.length;

Read only  property  which  reflects  the  length  of the string. For an empty
string, length is 0.


String.fromCharCode()
--------------------------------------------------------------------------------

    result = String.fromCharCode(charCode1, charCode2, ..., charCodeN);

Returns a string created by using the specified sequence of Unicode values.

See also: String Constructor


String.charAt()
--------------------------------------------------------------------------------

    result = charAt(index);

Returns the character at the specified index.


String.charCodeAt()
--------------------------------------------------------------------------------

    result = string.charCodeAt(index);

Returns the number indicating the Unicode value of the character at the  given
index.


String.concat()
--------------------------------------------------------------------------------

    result = string.concat(str2, str3, ...);

This function appends all given strings to the string in the string object and
returns the result.


String.indexOf()
--------------------------------------------------------------------------------

    result = string.indexOf(searchString [,fromIndex]);

Returns the index within the calling string object of the first occurrence  of
searchString. If searchString was not found, -1 is returned.


String.lastIndexOf()
--------------------------------------------------------------------------------

    result = string.lastIndexOf(searchString [,fromIndex]);

Returns the  index  within the calling string object of the last occurrence of
searchString. If searchString was not found, -1 is returned.


String.match()
--------------------------------------------------------------------------------

    result = string.match(regexp);

The function matches the string object agains the given regular expression and
returns all requested matches.

See also: RegExp Object


String.replace()
--------------------------------------------------------------------------------

    result = string.replace(regexp, newSubString);

The function searched the string for the regular expression and replaces it by
newSubString in the resulting string.

Example:

    str = "Test"; print(str.replace(/t/, "X"));
    // this will print "TesX"

    str = "Test"; print(str.replace(/T/, "X"));
    // this will print "Xest"

    str = "Test"; print(str.replace(/t/gi, "X"));
    // this will print "XesX" (note the "g" and "i" flags)

See also: RegExp Object


String.search()
--------------------------------------------------------------------------------

    result = string.search(regexp);

The function the index of the regular expression inside  the  string.  If  the
regular expression was not found, returns -1 is returned.

See also: RegExp Object


String.slice()
--------------------------------------------------------------------------------

    result = string.slice(beginslice [,endSlice]);

Extract a  section of a string and return a new string. The section to extract
is defined by beginSlice which is the  zero-based  index  at  which  to  begin
extraction.  and  endSlice which  is  the  zero-based  index  at  which to end
extraction. If endSlice is omitted, the function extracts to the  end  of  the
string.


String.split()
--------------------------------------------------------------------------------

    result = string.split(separator);

Split a  string  object into an array of strings by separating the string into
substrings. Separation is done using the string given as separator.

Example:

    var mystr = 'a,b,c';
    var result = mystr.split(',');

    // result is now an array with the length 3 containing
    // the three strings 'a', 'b' and 'c'


String.substr()
--------------------------------------------------------------------------------

    result = string.substr(start [,length]);

Returns the characters in a string beginning at the specified location through
the  specified  number  of characters. start is the location at which to begin
extracting characters (an integer between 0 and one less than  the  length  of
the string). length are the number of characters to extract.

Some special cases:

- If  start  is  positive and is the length of the string or longer, substr
  returns no characters.
- If start is negative, substr uses it as a character index from the end of
  the string. If start is negative and abs(start) is larger than the length
  of the string, substr uses 0 is the start index.
- If length is 0 or negative, substr returns no characters.  If  length  is
  omitted, start extracts characters to the end of the string.

Example:

    var anyString = "Silverjuke";

    // Sets test to "verj"
    var test = anyString.substr(3, 4);


String.substring()
--------------------------------------------------------------------------------

    result = string.substring(indexA [,indexB]);

indexA is an integer between 0 and one less than the length of the string. The
optional indexB is an integer between 0 and the length of the string. Then the
function extracts characters from indexA up to but not including indexB.

Some special cases:

- If indexA equals indexB, substring returns an empty string.
- If indexB is omitted, substring extracts characters to the end of the string.
- If either argument is less than 0 or is NaN, it is treated as if it  were 0.
- If either argument is greater than stringName.length, it is treated as if
  it were stringName.length.

Example:

    var anyString = "Jukebox";

    // Sets test to "Juk"
    test = anyString.substring(0,3);
    test = anyString.substring(3,0);

    // Sets test to "box"
    test = anyString.substring(4,7);
    test = anyString.substring(7,4);

    // Sets test to "Jukebo"
    test = anyString.substring(0,6);

    // Sets test to "Jukebox"
    test = anyString.substring(0,7);
    test = anyString.substring(0,10);


String.toLowerCase()
--------------------------------------------------------------------------------

    result = string.toLowerCase();

Returns the calling string value converted to lowercase.


String.toUpperCase()
--------------------------------------------------------------------------------

    result = string.toUpperCase();

Returns the calling string value converted to uppercase.


RegExp Object
================================================================================

The RegExp object provides some tools to work with regular expressions.

See also: String.search(), String.match(), String.replace()


RegExp Constructor
--------------------------------------------------------------------------------

    obj = /pattern/flags;
    obj = new RegExp(pattern, [flags]);

Both statements - the literal form and the constructor function - construct  a
regular expression object with the given pattern and some optional flags.

Example:

    var re = /e/;
    while( (input=prompt('Enter a word with an "e" ...')) )
    {
        if( re.test(input) )
            alert(input + ' contains an "e".');
        else
            alert(input + ' does not contain an "e".');
    }


RegExp.exec()
--------------------------------------------------------------------------------

    result = RegExp.exec(input);

This function  takes  an  input  string  and  matches  it  agains  the regular
expression  of  the  RegExp  object.  All  matches  are  returned  simelar  to
String.match().

See also: RegExp.test(), String.match()


RegExp.test()
--------------------------------------------------------------------------------

    result = regexp.test(input);

This function checks if a given input string matches against the given regular
expression object. If a match was found, the function returns true, else false
is returned. For more information (but slower execution) use the RegExp.exec()
method which is similar to String.match() then.

Example:

    var re = new RegExp("e");
    while( (input=prompt('Enter a word with an "e" ...')) )
    {
        if( re.test(input) )
            alert(input + ' contains an "e".');
        else
            alert(input + ' does not contain an "e".');
    }

See also: RegExp.exec(), String.match()


Globals
================================================================================

The following methods are not bound to any object.


eval()
--------------------------------------------------------------------------------

    var result = eval(string);

This is a top-level function that is not associated with any object.

The argument of the eval function is a string. If  the  string  represents  an
expression,  eval  evaluates the expression. If the argument represents one or
more statements, eval performs these statements. Finally, the  eval()  returns
the most recent result of the expressions or statements.

Note: If  the  argument  of  eval  is  not a string, eval returns the argument
unchanged.

Example:

    eval(new String("2+2")); // returns a String object containing "2+2"
    eval("2+2");             // returns 4


parseInt()
--------------------------------------------------------------------------------

    result = parseInt(string, [radix]);

This function takes the given string and tries to convert  it  to  an  integer
number regarding the radix (aka base).

If the radix is not specified or is specified as 0, we assumes the following:

- If the input string begins with "0x", the radix is 16 (hexadecimal).
- If the input string begins with "0", the radix is eight (octal).
- If the input string begins with any other value, the radix is 10 (decimal).

If the first character cannot be converted to a  number, parseInt() returns NaN.
You can call the isNaN() function to determine if the result of parseInt() is
NaN.

Example:

    // The following statements all set i to 15:
    i = parseInt("F", 16);
    i = parseInt("17", 8);
    i = parseInt("15", 10);
    i = parseInt(15.99, 10);
    i = parseInt("fire123", 16); // "f" is parsed, "i" is not in base
    i = parseInt("1111", 2);
    i = parseInt("15*3", 10); // "*" is not in base, use eval() for calculations
    i = parseInt("12", 13); // uncommon base

    // This statement sets i to NaN - "Jukebox" is not a number at all
    i = parseInt("Jukebox", 8);

    // This statement sets i to NaN - "0x6" is not in base 10 format
    i = parseInt("0x6", 10);

    // This statement sets i to NaN - for base 2 only the 0 and 1 are valid
    i = parseInt("234", 2);


parseFloat()
--------------------------------------------------------------------------------

    result = parseFloat(string);

This function takes the given string and tries to convert it  to  an  floating
point number. If it encounters a character other than a sign (+ or -), numeral
(0-9), a decimal point, or an exponent, it returns the value up to that  point
and ignores that character and all succeeding characters. Leading and trailing
spaces are allowed.

If the given string cannot be converted, the function  returns  NaN.  You  can
call the isNaN() function to determine if the result of parseFloats() is NaN.

Example:

    // The following statements all set f to 3.14:
    f = parseFloat("3.14");
    f = parseFloat("3.14*2"); // "*" is ignored, use eval() for calculations
    f = parseFloat("314e-2");
    f = parseFloat("0.0314E+2");
    f = parseFloat("3.14bla"); // "bla" is just ignored

    // The following statement sets f to NaN:
    f = parseFloat("bla");


isNaN()
--------------------------------------------------------------------------------

    result = isNaN(val);

NaN stands  for  "Not  a  Number",  which is a special value a number type can
have. With the global function isNaN() you can check if a  given  number  is a
number (return value is false) or is not a number (return value is true).

Example:

    isNaN(NaN);      // returns true
    isNaN("string"); // returns true
    isNaN("12");     // returns false
    isNaN(12);       // returns false


isFinite()
--------------------------------------------------------------------------------

    result = isFinite(val);

You can  use this method to determine whether a number is a finite number. The
isFinite method examines the number in its argument. If the argument  is  NaN,
positive  infinity  or negative infinity, this method returns false, otherwise
it returns true.


decodeURI()
--------------------------------------------------------------------------------

    var result = decodeURI(encodedUri);

Replaces each  escape  sequence  in  encodedUri with  the  character  that  it
represents,  the  result  is  returned.  encodedUri may  have  beed created by
previously by encodeURI() or by a similar routine.


decodeURIComponent()
--------------------------------------------------------------------------------

    var result = decodeURIComponent(encodedUriComponent);

Replaces each escape sequence in encodedUriComponent with the  character  that
it  represents,  the  result  is  returned.  decodeURIComponent may  have beed
created by previously by encodeURIComponent() or by a similar routine.


encodeURI()
--------------------------------------------------------------------------------

    var result = encodeURI(string);

Encodes each  character  in  string as  an  escape  sequence  if  needed.  The
following characters are not encoded:

    - _ . !   * ' ( ) ; / ? : @ & = + $ , # a-z A-Z 0-9

The resulting  string  is  returned. The function assumes that the string is a
complete URI, so it does not encode some reserved characters that have special
meaning  in  the  URI; to encode more characters, use encodeURIComponent(). To
decode the encoded string, use decodeURI().

Example:

    s = encodeURI("foo bar");
    // s is "foo%20bar" now - space is encoded

    s = encodeURI("foo&bar");
    // s is "foo&bar" now - "&" is not encoded by encodeURI()

    s = encodeURIComponent("foo&bar")
    // s is "foo%26bar" now - "&" is encoded by encodeURIComponent()


encodeURIComponent()
--------------------------------------------------------------------------------

    var result = encodeURIComponent(string);

Encodes each  character  in  string as  an  escape  sequence  if  needed.  The
following characters are not encoded:

    - _ . !   * ' ( ) a-z A-Z 0-9

The resulting  string is returned and can be forwarded as parts of an URI, eg.
entered parameters; to encode a complete  URI,  use  encodeURI()  instead.  To
decode the encoded string, use decodeURIComponent().

Example:

    s = encodeURI("foo bar");
    // s is "foo%20bar" now - space is encoded

    s = encodeURI("foo&bar");
    // s is "foo&bar" now - "&" is not encoded by encodeURI()

    s = encodeURIComponent("foo&bar")
    // s is "foo%26bar" now - "&" is encoded by encodeURIComponent()


print()
--------------------------------------------------------------------------------

    print(string);

Prints out  the  given  string  to  the  console. The console is not raised or
opened automatically, however, this can be done by  the  user  anytime.  Using
Dialog.show() you can also open the console by your script.


logError()
--------------------------------------------------------------------------------

    logError(msg);

Logs and displays an error. If several errors or warnings must be logged, just
call this function several times; if possible the console is opened as soon as
possible then.


logWarning()
--------------------------------------------------------------------------------

    logWarning(msg);

Logs and  displays  a  warning.  If several errors or warnings must be logged,
just call this function several times; if possible the console  is  opened  as
soon as possible then.


alert()
--------------------------------------------------------------------------------

    alert(msg);

This static function simply shows a dialog with the given message.


confirm()
--------------------------------------------------------------------------------

    yesNo = confirm(msg);

This static  function  shows a dialog given message and two buttons, "Yes" and
"No". If the user  hits  "Yes",  the  function  returns  true,  else  false is
returned.

Example:

    function Aclicked()
    {
        return confirm('goto "A"?');
    }

In your  skin  you  can  call  this  function eg. using:

    <button target="gotoA" onclick="Aclicked();" />.


prompt()
--------------------------------------------------------------------------------

    input = prompt(msg, [defaultValue]);

This static function shows a dialog with a message and the possiblity to enter
any  (string)  value.  The  function  returns the value entered by the user or
false if the user hits the "Cancel" button.

With defaultValue you can specify what should be shown initally in  the  value
text control.

Example:

    var name = prompt('Please enter your name:');
    if( name ) {
        alert('Welcome, ' + name + '!');
    }
    else {
        alert('Hello guest!');
    }


fileSel()
--------------------------------------------------------------------------------

    file = fileSel([msg], [defaultFile], [flags]);

This static function shows a dialog where the user can select a file.

- With msg you can define a little text that is shown to the user (normally
  in the title bar)
- With defaultFile you can define the path and the file initally selected.
- flags can be set to 1 to show a "Save as" dialog or to 0 to show a  "Open
  file" dialog. If flags is left out, the "Open file" dialog is shown.
- If the user selects a file, the function returns the path and the name of
  the file.
- If the user hits the "Cancel" button, the function returns false.

Example:

    var dlg;

    function onSelectFileButton()
    {
        var file = fileSel('Please select a file to use');
        if( file )
            dlg.setValue('file', file);
    }

    function onOpenDlgMenuEntry()
    {
        dlg = new Dialog();
        dlg.addTextCtrl('file', 'File:');
        dlg.addButton('select', 'Select...', onSelectFileButton);
        dlg.showModal();
    }

    program.addMenuEntry('fileSel test', onOpenDlgMenuEntry);

The example creates a little dialog (see "Dialog Object") where the  user  can
enter a file name manually or select it using the "Open file" dialog.


Copyright (c) Bjoern Petersen Software Design and Development, http://b44t.com

