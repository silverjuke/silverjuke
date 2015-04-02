
// EDIT BY ME
// eine Uebersicht der Optionen hier: http://www.sqlite.org/compile.html

// Nur Features ausschalten, die GANZ SICHER NIE benötig werden!
// Einzelne SQL-Kommandos koennen immer mal nuetzlich sein, da diese z.B. auch in Skripten verwendet werden.

#define SQLITE_OMIT_DEPRECATED			// saves 1 K
#define SQLITE_OMIT_UTF16				// saves 4 K

#define SQLITE_OMIT_PROGRESS_CALLBACK	// saves 1 K
#define SQLITE_OMIT_AUTHORIZATION		// saves 3 K

#define	SQLITE_OMIT_LOAD_EXTENSION		// not needed, dlclose() seems to makes problems

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
