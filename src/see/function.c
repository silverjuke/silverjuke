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
/* $Id: function.c 1126 2006-08-05 12:48:25Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
#endif

#include "see_mem.h"
#include "see_string.h"
#include "see_object.h"
#include "see_native.h"
#include "see_interpreter.h"
#include "see_debug.h"
#include "see_context.h"

#include "see_function.h"
#include "see_parse.h"
#include "see_stringdefs.h"

/*
 * A function is an internal object that embodies executable code, and
 * can be shared between Function instances.
 * The terminology used in the standard is to say that Function instances
 * which share the same 'function' (but may have different scope), and
 * have all their non-internal properties linked, are "joined". (13.1.2)
 *
 * These internal function structures encapsulate a FormalParameterList
 * and a FunctionBody (but not the Scope chain). In the mechanism for
 * Function instance creation (13.2) 'struct function' pointer comparison
 * is sufficient for telling if two Function instances are joined.
 */
 
/*
 * Create a new function 'core' entity (struct function) with a initial
 * common function object instance.
 */
struct function *
SEE_function_make(interp, name, params, body)
	struct SEE_interpreter *interp;
	struct SEE_string *name;
	struct var *params;
	void *body;
{
	struct function *f;
	int i;
	struct var *v;
	struct SEE_value val, r9;
	struct SEE_object *F;

	f = SEE_NEW(interp, struct function);

	f->body = body;
	f->sec_domain = interp->sec_domain;

	/*
	 * Convert the linked list of parameter name strings into
	 * a fixed array of string pointers. This is done because
	 * the Arguments object needs to seek the list. 
	 */
	f->nparams = 0;
	for (v = params; v; v = v->next)
	    f->nparams++;
	if (f->nparams) {
	    f->params = SEE_NEW_ARRAY(interp, struct SEE_string *, f->nparams);
	    for (i = 0, v = params; v; v = v->next, i++)
	        f->params[i] = v->name;
	} else
	    f->params = NULL;
	f->name = name;

	f->next = NULL;
	f->cache = NULL;
	f->common = NULL;

	/* 13.2 step 2: make object F */
	F = SEE_function_inst_create(interp, f, NULL);

	/*
	 * NB: SEE_function_inst_create() will perform the equivalent
	 * of steps 13 through 19 of 13.2 at this point.
	 */

	/* 13.2 step 8 + 15.3.5.1: add the 'length' property */
	SEE_SET_NUMBER(&val, f->nparams);
	SEE_OBJECT_PUT(interp, F, STR(length),
	    &val, SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY | SEE_ATTR_DONTENUM);

	/* 13.2 step 9 - a prototype object */
	SEE_SET_OBJECT(&r9, SEE_Object_new(interp));

	/* 13.2 step 10 - the prototype's 'constructor' property */
	SEE_SET_OBJECT(&val, F);
	SEE_OBJECT_PUT(interp, r9.u.object, STR(constructor), &val, 
		SEE_ATTR_DONTENUM);

	/* 13.2 step 11 + 15.3.5.2 - add the 'prototype' property */
	SEE_OBJECT_PUT(interp, F, STR(prototype),
		&r9, SEE_ATTR_DONTDELETE); /* (see 15.3.5.2) */

	/* f.arguments = null */
	if (SEE_COMPAT_JS(interp, >=, JS11)) {	/* EXT:2 */
		struct SEE_value v;
		SEE_SET_NULL(&v);
		SEE_OBJECT_PUT(interp, F,
			STR(arguments), &v, SEE_ATTR_DONTDELETE | 
			SEE_ATTR_READONLY | SEE_ATTR_DONTENUM);
	}

	/* Check if the function body is empty */
	f->is_empty = SEE_functionbody_isempty(interp, f);

	return f;
}

/*
 * 10.1.3 parameter instantiation
 */
void
SEE_function_put_args(context, f, argc, argv)
	struct SEE_context *context;
	struct function *f;
	int argc;
	struct SEE_value **argv;
{
	struct SEE_value undefv;
	int i;

	/* Install the actual arguments */
	SEE_SET_UNDEFINED(&undefv);
	for (i = 0; i < f->nparams; i++) 
	    SEE_OBJECT_PUT(context->interpreter,
		context->variable, 
		f->params[i],
		i < argc ? argv[i] : &undefv,
		context->varattr);
}
