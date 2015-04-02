Silverjuke
================================================================================

Silverjuke is the easy-to-use jukebox with more than ten yeas of experience, see
http://silverjuke.net for further information.

The Silverjuke source code is available under the GPL license, see the file
LICENSE for details.


Todo
================================================================================

This version of Silverjuke is still be and the following points are with high
priority on our todo-list:

- Make player basics work properly
- Create configure/make scripts
- Fix known bug: Visualisation screen freezes Silverjuke
- Fix known bug: Program crashes on editing shortcuts
- Adapt the skin
- Fix known bug: make filenames with "%" and "#" work
- Other bug fixed, please report bugs to
  https://github.com/r10s/silverjuke/issues


Installation
================================================================================

Linux/Unix:  With chance, Silverjuke is available in your favorite packet
manager.  If it is not there, please contact your linux distributor - we cannot
help on this point.
Alternatively, more experienced users can build Silverjuke theirself.  The
source code from https://github.com/r10s/silverjuke plus a simple `./configure`,
`make` and `make install` should do the job.

Windows:  For Windows, an installation program with the most recent version is
available at http://silverjuke.net .  Note, that the windows binaries are not
opened sourced.


Coding
================================================================================

Beside the possibility to code on the Silverjuke project itself, you can create
simple scripts that will be loaded and executed by Silverjuke.  For the
possibilities please refer to docs/scripting.md etc.

If you want to code on Silverjuke yourself here are some hints that may be
usefull esp. if you want to give your work back to the core (pull requests).

- For indentation we use tabs.  Alignments that are not placed at the beginning
  of a line should be done with spaces.

- For padding between funktions, classes etc. we use 2 empty lines

- Files are encoded as UTF-8 with Unix-Lineends

- `/sjbase` contains the main functions and the startup routines

- `/sjmodules` does not contain loadable libraries but program parts that can
  communicate with message without knowing each other

- `/sjtools` contains - more or less - self-contained and independent classes
  and functions; there should be no references to /sjbase or /sjmodules

- the other directories are used libraries

- includes are done using the `#include <dir/file.h>` scheme; this allows us to
  use a different file with the same name in another director (by using
  different include directorues with higher priority);  moreover, this allows us
  to move files around in the directory structure without changing the includes.

If you use the Silverjuke source code as a base of you projects, please keep in
mind that Silverjuke is released under the GPL and you have to open source your
work as well.  See the file LICENSE that comes together with the source code.


Copyright (c) Bjoern Petersen Software Design and Development, http://b44t.com

