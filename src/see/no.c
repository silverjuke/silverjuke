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
/* $Id: no.c 893 2005-12-21 04:38:29Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "see_type.h"
#include "see_value.h"
#include "see_object.h"
#include "see_no.h"
#include "see_mem.h"
#include "see_string.h"
#include "see_error.h"
#include "see_interpreter.h"

#include "see_stringdefs.h"

static struct SEE_string *no_enum_next(struct SEE_interpreter *,
	struct SEE_enum *, int *);

/*------------------------------------------------------------
 * No-op methods
 */

void
SEE_no_get(interp, o, p, res)
	struct SEE_interpreter *interp;
        struct SEE_object *o;
        struct SEE_string *p;
        struct SEE_value *res;
{
	SEE_SET_UNDEFINED(res);
}

void
SEE_no_put(interp, o, p, val, attr)
	struct SEE_interpreter *interp;
        struct SEE_object *o;
        struct SEE_string *p;
        struct SEE_value *val;
	int attr;
{
}

int
SEE_no_canput(interp, o, p)
	struct SEE_interpreter *interp;
        struct SEE_object *o;
        struct SEE_string *p;
{
        return 0;
}

int
SEE_no_hasproperty(interp, o, p)
	struct SEE_interpreter *interp;
        struct SEE_object *o;
        struct SEE_string *p;
{
        return 0;
}

int
SEE_no_delete(interp, o, p)
	struct SEE_interpreter *interp;
        struct SEE_object *o;
        struct SEE_string *p;
{
        return 0;
} 

void
SEE_no_defaultvalue(interp, o, hint, res)
	struct SEE_interpreter *interp;
        struct SEE_object *o;
        struct SEE_value *hint;
        struct SEE_value *res;
{
	SEE_error_throw_string(interp, interp->TypeError, 
		(hint &&
		 SEE_VALUE_GET_TYPE(hint) == SEE_OBJECT &&
		 hint->u.object == interp->String) 
			? STR(defaultvalue_string_bad) :
		(hint &&
		 SEE_VALUE_GET_TYPE(hint) == SEE_OBJECT &&
		 hint->u.object == interp->Number) 
			? STR(defaultvalue_number_bad) :
		STR(defaultvalue_no_bad));
}

static struct SEE_string *
no_enum_next(interp, e, dd)
	struct SEE_interpreter *interp;
	struct SEE_enum *e;
	int *dd;
{
	return NULL;
}

static struct SEE_enumclass no_enumclass = {
	0,
	no_enum_next
};

struct SEE_enum *
SEE_no_enumerator(interp, o)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
{
	struct SEE_enum *e;
	e = SEE_NEW(interp, struct SEE_enum);
	e->enumclass = &no_enumclass;
	return e;
}
