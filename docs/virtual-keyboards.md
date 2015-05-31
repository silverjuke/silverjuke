Creating Virtual Keyboards
================================================================================

Silverjuke Versions support a virtual keyboard which is useful esp. in 
combination with the kiosk mode (see "Settings/Kiosk mode/Virtual keyboard").

This document describes how the virtual keyboard keys may be customized eg. to 
fit the needs of a complete localization.


**A brief overview**

A virtual keyboard layout is a plain text file that contains statements as 

    name=value;

Line breaks are not significant, you may put any number of statements into a 
single line - however, do not forget the trailing semicolon. The file must be
encoded as ISO8859-1 (see http://en.wikipedia.org/wiki/ISO8859-1 ). To use 
the file in Silverjuke, it must have the extension *.sjk and must be located 
in one of the search paths.

To define a virtual keyboard, first set its name and then simply define all keys
one by one, starting from the first key in the first row and ending with the 
last key in the last row. The following snippet contains a complete and valid 
keyboard with the keys a-c:

    layout=My first test;
    key=a; 
    key=b;
    key=c;

If you also want to type the upper-case letters, you can define a shift key and
set the shifted values for all letters:

    layout=My second test;
    key=a; shift=A;
    key=b; shift=B;
    key=c; shift=C;
    key=shift;

Before reading on, it may be a good idea to have a look at the already existing
virtual keyboard layouts.


**Multiple lines of keys**

Of course, you can use multiple lines of keys; just use the statement 
`nextline;` for this purpose.

    key=a;
    nextline;
    key=b;


**The title of a key**

If you want to show a different title than the key itself on key, add the title
in double quotes to the value:

    key=a "A";


**Alt- and Dead-keys**

You may define up to 32 Alt- or Dead-keys which work simelar to the shift keys.
Moreover, each Alt-key may be combined with "shift", so you may have up to 64
additional states of the keyboard.

The Alt-keys have the special names "alt1", "alt2" ... "alt32" and are defined
as any other key using eg. the statement `key=alt1;`. To define the 
alternative keys if an alt key is pressed, just add statements as `alt1=@` and
`alt1shift=$` behind the key-statement. Example:

    key=a; shift=A; alt1=@; alt2=[; alt2shift=];
    key=alt1;
    key=alt2 "Alt";

As you see we've added a title only to the second alt-key. This is because the
first alt-key automatically gets the title "AltGr" if noting else is given.

Note: Silverjuke versions smaller that 3.02 only allow up to 9 Alt-Keys.


**Defining special keys**

You can also define any key using its hexadecimal representation, eg. `key=a;`
and `key=0x61;` refer to the same character. Some keys must _always_ be written 
in this representation: the space, the backslash and the semicolon; in this 
order:

    key=0x20;
    key=0x5c;
    key=0x3b;

This way you can also access all Unicode characters, eg. with `key=0x3b1;` you
can access the greek letter "alpha". Some other special keys are `backsp`,
`clearall` and `enter`:

    key=backsp;
    key=clearall;
    key=enter;

Finally, very special is the pseudo-key "entercont" which may be used to span
the enter key over two rows, see the already existing keyboard layouts for an
example.


**Using different key widths**

Normally, all keys have a relative width of 1.0. With the "width"-statement you
can give some keys different widths. If you want to let the spacebar be 5 times 
wider than a normal key, use the following statement:

    key=0x20; width=5.0;

To create a "natural" keyboard layout, you may need some empty room, esp. aleft
of a row start. For this, use the "spacer"-statement:

    key=backsp;
    nextline;
    spacer; width=0.7;
    key=q;


**Locking the state of the Alt-keys**

Normally, the Alt-states are reset if a key is pressed. You can optionally 
"lock" the Alt-states; for this purpose, simply add the "lock" keyword to the
key-statement:

    key=alt1 lock;

This way, you can create "shift lock" or "caps lock" keys. Or you can add two 
completely different alphabets to one keyboard layout.


Copyright (c) Bjoern Petersen Software Design and Development, http://b44t.com

