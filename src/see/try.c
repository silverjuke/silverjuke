/*
 * Copyright (c) 2003
 *      David Leonard.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Mr Leonard nor the names of the contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY DAVID LEONARD AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL DAVID LEONARD OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* $Id: try.c 1081 2006-05-07 05:58:32Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
#endif

#include "see_type.h"
#include "see_value.h"
#include "see_try.h"
#include "see_object.h"
#include "see_string.h"
#include "see_debug.h"
#include "see_interpreter.h"
#include "see_system.h"

#include "see_stringdefs.h"
#include "see_dprint.h"

/*
 * Exception handling
 *
 * This implementation of exceptions uses the C stack and
 * C's setjmp/longjmp to implement exceptions. The TRY macro
 * is a sets up and tests a setjmp, and the THROW macros 
 * just do a longjmp. (That is why a garbage collector is
 * needed!)
 *
 * NOTE: I had considered using a special-return mechanism
 * for exceptions, but the complexity, performance and annoyance 
 * of the vast number of 'return checking' required and the
 * small number of expected actual exceptions during normal
 * operation made this mechanism far more favourable. Also,
 * setjmp/longjmp is part of standard POSIX and ANSI C, so
 * portability is not really lost.
 */

/**
 * Handles an uncatchable exception by aborting.
 *
 * This can happen when a SEE_THROW occurs outside of any SEE_TRY context,
 * or when an error occurs during low-level throw handling. (See 
 * SEE_error_throw(), for instance).
 */
void
SEE_throw_abort(interp, v, file, line)
	struct SEE_interpreter *interp;
	const struct SEE_value *v;
	const char *file;
	int line;
{
#ifndef NDEBUG
	dprintf("%s:%d: threw uncatchable exception\n", file, line);
	if (v != NULL) {
	    SEE_try_context_t ctx;
	    dprintf("  exception: ");
	    SEE_TRY(interp, ctx)
		dprintv(interp, v);
	    if (SEE_CAUGHT(ctx))
		dprintf("<error printing value>");
	    dprintf("\n");
	}
#endif
	SEE_ABORT(interp, "exception thrown but no TRY block");
}

/**
 * Returns a growable string containing a location in the
 * form "program.js:23: ", or empty if loc is NULL. 
 * This string is useful as a prefix for forming messages generated 
 * by exceptions.
 */
struct SEE_string *
SEE_location_string(interp, loc)
	struct SEE_interpreter *interp;
	struct SEE_throw_location *loc;
{
	struct SEE_string *s;

	s = SEE_string_new(interp, 0);
	if (loc == NULL)
		return s;
	SEE_string_append(s, loc->filename ? loc->filename 
					   : STR(unknown_file));
	SEE_string_addch(s, ':');
	SEE_string_append_int(s, loc->lineno);
	SEE_string_addch(s, ':');
	SEE_string_addch(s, ' ');
	return s;
}

#ifndef NDEBUG
/**
 * A dummy function for debugging.
 * This function exists solely for debuggers to break on.
 * It is called immediately before the longjmp takes effect.
 */
void
SEE_throw()
{
}
#endif

/**
 * Handles a longjmp failure by calling the interpreter abort hook.
 * This function is provided by ANSI for catching problems with
 * longjmp (eg, corrupted jmp_buf). Defining it here occludes any
 * system definition (which on unix is just a call to abort()).
 */
void
longjmperror()
{
	SEE_ABORT(NULL, "longjmp error");
}
