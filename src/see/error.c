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
/* $Id: error.c 1081 2006-05-07 05:58:32Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <string.h>
# include <stdarg.h>
#endif

#if HAVE_ERRNO_H
# include <errno.h>
#else
extern int errno;
#endif

#include "see_value.h"
#include "see_object.h"
#include "see_interpreter.h"
#include "see_try.h"
#include "see_mem.h"
#include "see_error.h"
#include "see_string.h"

#include "see_stringdefs.h"
#include "see_dprint.h"

#ifndef NDEBUG
int SEE_error_debug = 0;
#endif

static void error_throw(struct SEE_interpreter *, struct SEE_object *, int, 
			const char *, int, const char *, va_list) SEE_dead;

/*
 * Throw an error, optionally using the given string as the error message.
 * The string is prefixed it with the current try location.
 */
void
SEE_error__throw_string(interp, obj, filename, lineno, s)
	struct SEE_interpreter *interp;
	struct SEE_object *obj;
	const char *filename;
	int lineno;
	struct SEE_string *s;
{
	volatile struct SEE_try_context *ctxt_save;
	struct SEE_value res;

	/* If no try-catch context exists, we should abort immediately */
	if (!interp->try_context) {
		struct SEE_value v;
#ifndef NDEBUG
		if (s) {
			dprintf("message: ");
			dprints(s);
			dprintf("\n");
		}
#endif
		SEE_SET_OBJECT(&v, obj);
		SEE_throw_abort(interp, &v, filename, lineno);
		/* NOTREACHED */
	}

	/* 
	 * Temporarily remove the current try-catch context
	 * so that any exceptions thrown recusively during the construction
	 * of the error cause an interpreter abort in the block above. 
	 */
	ctxt_save = interp->try_context;
	interp->try_context = NULL;
	{
		struct SEE_string *msg;
		struct SEE_value v, *argv[1];

		msg = SEE_string_concat(interp, 
		    SEE_location_string(interp, interp->try_location), 
		    	s ? s : STR(error));
		SEE_SET_STRING(&v, msg);
		argv[0] = &v;
		SEE_OBJECT_CONSTRUCT(interp, obj, obj, 1, argv, &res);
#ifndef NDEBUG
		if (SEE_error_debug)
		    dprintf("throwing object %p from %s:%d\n",
			res.u.object, filename ? filename : "unknown", lineno);
#endif
	}
	interp->try_context = ctxt_save;

	SEE__THROW(interp, &res, filename, lineno);
}

/*
 * Helper function for constructing an error message
 */
static void
error_throw(interp, obj, errval, filename, lineno, fmt, ap)
	struct SEE_interpreter *interp;
	struct SEE_object *obj;
	int errval;		/* -1 means no error message */
	const char *filename;
	int lineno;
	const char *fmt;
	va_list ap;
{
	struct SEE_string *s, *t;
	volatile struct SEE_try_context *ctxt_save;

	/* Detect recursive errors: */
	if (!interp->try_context) {
		struct SEE_value v;
		SEE_SET_OBJECT(&v, obj);
		SEE_throw_abort(interp, &v, filename, lineno); 
	}
	ctxt_save = interp->try_context;
	interp->try_context = NULL;

	if (fmt) {
	    s = SEE_string_vsprintf(interp, fmt, ap);
	    if (errval != -1) {
#if HAVE_STRERROR
		t = SEE_string_sprintf(interp, ": %s", strerror(errval));
#else
		t = SEE_string_sprintf(interp, ": error %d", errval);
#endif
		SEE_string_append(s, t);
	    }
	} else
	    s = NULL;
	interp->try_context = ctxt_save;

	SEE_error__throw_string(interp, obj, filename, lineno, s);
}

/*
 * Throw an error, using the given arguments as the error
 * message.
 */
void
SEE_error__throw(struct SEE_interpreter *interp, struct SEE_object *obj,
	const char *filename, int lineno, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	error_throw(interp, obj, -1, filename, lineno, fmt, ap);
	va_end(ap);
}

#if !HAVE_VARIADIC_MACROS
void
SEE_error__throw0(struct SEE_interpreter *interp, struct SEE_object *obj,
	const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	error_throw(interp, obj, -1, NULL, 0, fmt, ap);
	va_end(ap);
}
#endif

/*
 * Throw an error, with the given message appending ':' and the
 * system error message. (See manual page for 'errno')
 */
void
SEE_error__throw_sys(struct SEE_interpreter *interp, struct SEE_object *obj,
	const char *filename, int lineno, const char *fmt, ...)
{
	va_list ap;
	int errval = errno;

	va_start(ap, fmt);
	error_throw(interp, obj, errval, filename, lineno, fmt, ap);
	va_end(ap);
}

#if !HAVE_VARIADIC_MACROS
void
SEE_error__throw_sys0(struct SEE_interpreter *interp, struct SEE_object *obj,
	const char *fmt, ...)
{
	va_list ap;
	int errval = errno;

	va_start(ap, fmt);
	error_throw(interp, obj, errval, NULL, 0, fmt, ap);
	va_end(ap);
}
#endif
