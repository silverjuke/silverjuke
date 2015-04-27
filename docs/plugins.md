Plugins
================================================================================

**Contents**

- Overview
- Initialization
- Receiving Notifications from Silverjuke
    - SJ_PLUGIN_INIT
    - SJ_PLUGIN_EXIT
    - SJ_PLUGIN_CALL
- Sending Commands to Silverjuke
    - SJ_GET_VERSION
    - SJ_EXECUTE
- Miscellaneous
    - Threads
    - Used Types
    - Strings in Plugins


Overview
================================================================================

First note: Currently, by default Silverjuke is compiled without the plugin
capability. This may change in future, however, currently this is not
foreseeable. For this reason, please also check if other customisation
methods will do the job, eg. skinning, shell scripts and Silvejuke commandline
options or DDE.

The following chapters describes how to write plugins for Silverjuke using the
Silverjuke API.

A plugin is a compiled so, dll- or dynlib-file that is loaded together with 
Silverjuke. Plugins can be written in nearly any language that can create eg. 
DLLs.

With a Silverjuke plugin you can do everything you can do with Scripting plus
you have full system access and your programs may run much faster. Eg. you can
do the following:

- Access the Silverjuke credit system - eg. you can write a plugin for a
  special coin tester hardware
- Access the player - eg. you can write a plugin for remote controlling
  Silverjuke
- Access the playlist - eg. you can write a plugin that displays some
  information on an external LCD display
- And much more we cannot imagine at the moment ;-)

To allow a wide audience to use the Silverjuke API, the interface is an C
interface that does not use any advanced language features. The interface is
also usable directly in C++ and C#. For all other languages, you will need a
small wrapper.

To get started, please have a look at the chapter Initialization or at the
examples available. 


Initialization
================================================================================

A Silverjuke plugin is a simple DLL (or "dynlib" or "so", for easier reading, we
simply use DLL in this manual) that exports the symbol SjGetInterface.
SjGetInterface is a function that should be declared as follows:

    SjInterface* SjGetInterface(void);

If you use Visual C++, you can export symbols eg. by adding a *.def file to
your project that contains the following:

    LIBRARY "hello_world.dll"

    EXPORTS
        SjGetInterface

If such a DLL is placed in one of Silverjuke's search paths, Silverjuke loads
the DLL and calls the SjGetInterface function which should  eturn a
SjInterface structure then:

    struct SjInterface
    {
        SJPROC*  CallPlugin;
        SJPROC*  CallMaster;
        LPARAM   rsvd;
        LPARAM   user;
    };

The structure must exist all the time the plugin is loaded and will be a
global or a static object normally.

SjInterface is used for communication between Silverjuke and the Plugin - if
you want to call Silverjuke, use the CallMaster() function and if Silverjuke
wants to notify the plugin, Silverjuke calls CallPlugin().

So, before you return the structure, you have to set the CallPlugin() member
to a function that will receive notifications from Silverjuke, moreover, you
can use the "user" member for any internal purposes. The CallMaster() member
will be set up by Silverjuke and "rsvd" should not be touched by the plugin.
BTW: all structures and prototypes are declared in "sj_api.h".

A working example:

    #include <sj_api.h>
    
    SjInterface interf;
    
    LPARAM CALLBACK MyPluginHandler(SjInterface* interf, UINT msg,
                    LPARAM param1, LPARAM param2, LPARAM param3)
    {
        if( msg == SJ_PLUGIN_INIT )
        {
            interf->CallMaster(interf, SJ_EXECUTE,
                (LPARAM)"m = 'A first test';"
                "i = 'Hello world!';"
                "program.addMenuEntry(m, function(){program.alert(i)})",
                0, 0);
  
            return 1; // success
        }
        
        return 0;
    }

    SjInterface* SjGetInterface()
    {
        interf.CallPlugin = MyPluginHandler;
        return &interf;
    }

After you restart Silverjuke, the plugin should be availab in the context menu
at "Modules" and in the settings dialog at "Advanced / Further options".

As you can see, the example let Silverjuke executes some code snippets. In
fact, most of the API calls are just wrappers for the scripting engine. This
allows us to keep the interface small and simple and you can write parts of
the plugin much faster using the often easier Scripting API.

If all this works, you can start implementing your plugin's functionality; for
this purpose, please have a look at the chapters "Receiving Notifications from
Silverjuke" and "Sending Commands to Silverjuke".


Receiving Notifications from Silverjuke
--------------------------------------------------------------------------------

If something happens that Silverjuke wants to tell a plugin, Silverjuke calls
the CallPlugin() function of the SjInterface structure. This function is
normally implemented as follows:

    LPARAM CALLBACK MyPluginHandler(SjInterface* interf, UINT msg,
                     LPARAM param1, LPARAM param2, LPARAM param3)
    {
        switch( msg )
        {
            case SJ_PLUGIN_INIT:
                // ...
                return 1;
         
            case SJ_PLUGIN_EXIT:
                // ...
                return 1;

            case SJ_PLUGIN_CALL:
                // ...
                return 1;

           // catch other notifications of interest here
       }
       
       return 0;
    }

The function has five parameters: "interf" is the same object as returned by the
plugin from SjGetInterface and "msg" is one of the SJ_PLUGIN_* constants as
defined below. The meaning of "param1", "param2" and "param3" depends on the
notification.

If not stated otherwise in the following descriptions, you should return "1" if
you have processed a notification and "0" otherwise. "0" should also be returned
for all unknown notifications - this will allow your plugin to be compatible
with future enhancements.

If Silverjuke sents strings together with the notifications, they're valid only
until the notification is processed (until you return from the CallPlugin()
function).


SJ_PLUGIN_INIT
--------------------------------------------------------------------------------

    bSuccess = CallPlugin(interf, SJ_PLUGIN_INIT, 0, 0, 0);

SJ_PLUGIN_INIT is sent to the plugin directly after the plugin has called
SjGetInterface(). Please return 1 on success or 0 on failure, in the latter
case Silverjuke will unload the DLL without calling SJ_PLUGIN_EXIT before.

Example:

    void* neededMemory;

    LPARAM CALLBACK MyPluginHandler(SjInterface* interf, UINT msg,
                      LPARAM param1, LPARAM param2, LPARAM param3)
    {
        switch( msg )
        {
            case SJ_PLUGIN_INIT:
                neededMemory = malloc(10000);
                if( neededMemory == 0 )
                    return 0; // error - SJ_PLUGIN_EXIT won't be called
                return 1; // success

            case SJ_PLUGIN_EXIT:
                free(neededMemory);
                return 1;
        }
        
        return 0;
    }


SJ_PLUGIN_EXIT
--------------------------------------------------------------------------------

    CallPlugin(interf, SJ_PLUGIN_EXIT, 0, 0, 0);

SJ_PLUGIN_EXIT is sent to the plugin just before it gets unloaded, this is the
last message sent to the plugin; not called if SJ_PLUGIN_INIT failed before.

See SJ_PLUGIN_INIT for an example.


SJ_PLUGIN_CALL
--------------------------------------------------------------------------------

    CallPlugin(interf, SJ_PLUGIN_CALL, param1, param2, param3);

You will receive this notification if you call program.callPlugin() from
within a script.

The param1 param2 and param3 parameters are exactly forwarded from
Program.callPlugin() and may be used for any purpose, depending on what is
given to Program.callPlugin(). Independingly of what types are given to
program.callPlugin(), all parameters are UTF-8 encoded strings from the view
of SJ_PLUGIN_CALL (if less than three parameters are given to 
Program.callPlugin(), the strings are empty).

It is very common to use param1 as the "reason for call", however, this is not
neccessary.

You can return the long values 0 or 1 directly from this message. For all
other values or for strings, please return an UTF-8 encoded string.

Example:

    LPARAM CALLBACK MyPluginHandler(SjInterface* interf, UINT msg,
                      LPARAM param1, LPARAM param2, LPARAM param3)
    {
        switch( msg )
        {
            case SJ_PLUGIN_CALL:
                return (LPARAM)"test";
        }

        return 0;
    }

In the scripting part, you can receive the string as follows

    var resultFromNativePart;

    resultFromNativePart = program.callPlugin();

    // resultFromNativePart contains "test" now

See also: SJ_EXECUTE, Scripting, Program.callPlugin()


Sending Commands to Silverjuke
--------------------------------------------------------------------------------

Call CallMaster() to send messages to Silverjuke. If not stated otherwise, the
return value all parameters are 0.

If Silverjuke returns a string as a result of a command, this string is only
valid until you call CallMaster() the next time.


SJ_GET_VERSION
--------------------------------------------------------------------------------

    lVersion = CallMaster(interf, SJ_GET_VERSION, 0, 0, 0);

Returns the Silverjuke version as 0xjjnn00rr with jj=major, nn=minor and
rr=revision as BCD (see http://en.wikipedia.org/wiki/Binary-coded_decimal ).
Eg. for Silverjuke 2.10 rev17, the function returns 0x02100017. Note, that the
revision number is not always equal to the beta- or rc-number. You can find
out the correct revision number eg. in the "Properties" dialog of Windows.

This message is thread-save and can be called everywhere.

Example:

    if( interf->CallMaster(interf,SJ_GET_VERSION,0,0,0) >= 0x02100000 )
    {
        // version is fine ...
    }

See also: Program.version


SJ_EXECUTE
--------------------------------------------------------------------------------

    result = CallMaster(interf, SJ_EXECUTE, szScript, bReturnString, 0);

This is the most powerful function in this API - it executes any script as
defined in the chapters Scripting and returns the result.

The script should be given as an UTF-8 string encoded in szScript. The
function returns the result as a long value (set bReturnString to 0) or as an
UTF-8 encoded string (set bReturnString to 1). If a string is returned, the
string is valid as long as no other API function is called.

Example:

    interf->CallMaster(interf, SJ_EXECUTE, (LPARAM)"player.play();", 0, 0);

    You can also callback yourself from within a script:

    LPARAM CALLBACK MyPluginHandler(SjInterface* interf, UINT msg,
                      LPARAM param1, LPARAM param2, LPARAM param3)
    {
        switch( msg )
        {
            case SJ_PLUGIN_INIT:
                // ...
                return 1;

            case SJ_PLUGIN_CALL:
                if( strcmp((char*)param1, "MyID") == 0 )
                {
                    // param2 is the current playing time now!
                }
                return 1;
        }

        return 0;
    }

    void sendPlayingTime()
    {
        interf.CallMaster(&interf, SJ_EXECUTE,
            (LPARAM)"program.callPlugin('MyID', player.time)", 0, 0);
    }

The example above is only there to demonstrate the possibilities; of course
you can find out the playing time easier with the following statement:

    long time = interf.CallMaster(&interf, SJ_EXECUTE,
        (LPARAM)"player.time", 0, 0);

See also: SJ_PLUGIN_CALL, Scripting, Program.callPlugin()


Miscellaneous
================================================================================

In this chapter you find some topics that do not fit well to other chapters.


Threads
--------------------------------------------------------------------------------

Silverjuke uses different threads internally and your plugin can also used
more than one thread.

However, when using threads, please note the following:

- The CallPlugin() function is always called in the main thread
- You should also call CallMaster() only from the main thread

There may be a very few exceptions from these rules, this is stated explicitly
in the descriptions of the messages and notifications then.


Used Types
--------------------------------------------------------------------------------

The types UINT, LPARAM, CALLBACK and SJPROC are defined in sj_api.h depending
on the operating system in use.

- UINT is normally defined as "unsigned long", you can expect this type
  always to have 32 bit.
- LPARAM is also defined as "unsigned long" normally, however, as this type
  should always be large enough to contain pointers, it may be defined as
  "unsigned long long" here and there. You can expect this type to have at
  least 32 bit.
- CALLBACK defines the calling conventios of the CallMaster and CallPlugin
  functions.
- SJPROC is used by the SjInterface structure and defined a callback type
  used for the communication between Silverjuke to/from plugin.

See also: Strings (see "Strings in Plugins"), sj_api.h


Strings in Plugins
--------------------------------------------------------------------------------

LPARAM is also used to return strings or to use strings as parameters. In this
case, please cast eg. your char* to LPARAM or the other way round. The string
encoding is always UTF-8.

Example:

    // Give a string to Silverjuke
    char string[256] = "Just a test";
    interf->CallMaster(interf, SJ_MENU_ADD_ENTRY, (LPARAM)string, 0, 0);

    // Get a string from Silverjuke
    strcpy(string, (char*)interf->CallMaster(interf,SJ_GET_LOCALE,0,0,0));

To make the things more clear, we use "plain C strings" for the example,
however, you can easily use eg. CString objects instead (which are also more
secure).


Copyright (c) Bjoern Petersen Software Design and Development, http://b44t.com

