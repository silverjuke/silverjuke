/*
 * Copyright (c) 2004
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
/* $Id: object.c 1108 2006-08-01 14:22:09Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#include "see_value.h"
#include "see_object.h"
#include "see_interpreter.h"
#include "see_try.h"
#include "see_error.h"
#include "see_string.h"
#include "see_system.h"

#include "see_stringdefs.h"

static void transit_sec_domain(struct SEE_interpreter *, struct SEE_object *);

/*
 * Transits a security domain.
 */
static void
transit_sec_domain(interp, obj)
	struct SEE_interpreter *interp;
	struct SEE_object *obj;
{
	void *sec_domain;

	if (SEE_system.transit_sec_domain &&
	    SEE_OBJECT_HAS_GET_SEC_DOMAIN(obj))
	{
		sec_domain = SEE_OBJECT_GET_SEC_DOMAIN(interp, obj);
		if (interp->sec_domain != sec_domain)
			SEE_system.transit_sec_domain(interp, sec_domain);
	}
}

/*
 * Calls the object method, after checking that any recursion
 * limit has not been reached.
 */
void
SEE_object_call(interp, obj, thisobj, argc, argv, res)
	struct SEE_interpreter *interp; 
	struct SEE_object *obj;
	struct SEE_object *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	SEE_try_context_t c;
	int saved_recursion_limit = interp->recursion_limit;
	void *saved_sec_domain = interp->sec_domain;

	if (interp->recursion_limit == 0)
	    SEE_error_throw_string(interp, interp->Error,
		STR(recursion_limit_reached));
	else if (interp->recursion_limit > 0) 
	    interp->recursion_limit--;
	transit_sec_domain(interp, obj);
	SEE_TRY(interp, c) {
	    _SEE_OBJECT_CALL(interp, obj, thisobj, argc, argv, res);
	}
	interp->sec_domain = saved_sec_domain;
	interp->recursion_limit = saved_recursion_limit;
	SEE_DEFAULT_CATCH(interp, c);
}

/*
 * Calls the object constructor, after checking that any recursion
 * limit has not been reached.
 */
void
SEE_object_construct(interp, obj, thisobj, argc, argv, res)
	struct SEE_interpreter *interp; 
	struct SEE_object *obj;
	struct SEE_object *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	SEE_try_context_t c;
	int saved_recursion_limit = interp->recursion_limit;
	void *saved_sec_domain = interp->sec_domain;

	if (interp->recursion_limit == 1) {
	    interp->recursion_limit = 0;
	    SEE_error_throw_string(interp, interp->Error,
		STR(recursion_limit_reached));
	} else if (interp->recursion_limit > 0) 
	    interp->recursion_limit--;
	transit_sec_domain(interp, obj);
	SEE_TRY(interp, c) {
	    _SEE_OBJECT_CONSTRUCT(interp, obj, thisobj, argc, argv, res);
	}
	interp->sec_domain = saved_sec_domain;
	interp->recursion_limit = saved_recursion_limit;
	SEE_DEFAULT_CATCH(interp, c);
}
