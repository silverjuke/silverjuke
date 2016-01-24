
Prjm - projectM fork for Silverjuke
================================================================================

This directory contains "Prjm" (pronounced "Preem"), a fork of the "projectM" 
library,  a Milkdrop-esque visualisation.

This fork mainly optimizes the core parts of the projectM for the use with 
Silverjuke (includes, defines, directory structure, avoid using glu.h etc.).
Any changes are maked by "EDIT BY SJ".

For easy usage of projectM with Silverjuke and with different compilers, we
decided to put all configuration to the C-side into `amalgation.h`; the only
files to include are `amalgation1.cpp` and `amalgation2.cpp`.  We had to create
two files to allow the usage of different structures with the same name, see
comments in `amalgation1.cpp`.

The main interfaces are _not_ changed, so we may decide to switch back to
projectM any time.

The licence of this fork is the same as the original projectM license, see file 
"COPYING".


Further notes
================================================================================

Packets needed for compilation:

Ubuntu:
$ sudo apt-get install mesa-common-dev

Packets needed for linking:

nothing special, however, the linker option `-lGL` does not work on my Ubuntu,
`-l/usr/lib/x86_64-linux-gnu/mesa/libGL.so.1` does.




