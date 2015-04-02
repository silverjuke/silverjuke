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
/* $Id: interpreter.c 1108 2006-08-01 14:22:09Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
#endif

#include "see_cfunction.h"
#include "see_context.h"
#include "see_intern.h"
#include "see_interpreter.h"
#include "see_mem.h"
#include "see_native.h"
#include "see_system.h"

#include "see_init.h"

/**
 * Initialises/reinitializes an interpreter structure
 * using the default compatibility flags.
 * The interpreter structure must be initialized at least once
 * before it can be used.
 */
void
SEE_interpreter_init(interp)
	struct SEE_interpreter *interp;
{
	SEE_interpreter_init_compat(interp, 
		SEE_system.default_compat_flags);
}

/**
 * Initialises/reinitializes an interpreter structure
 * using the given compatibility flags.
 * The interpreter structure must be initialized at least once
 * before it can be used.
 */
void
SEE_interpreter_init_compat(interp, compat_flags)
	struct SEE_interpreter *interp;
	int compat_flags;
{
	interp->try_context = NULL;
	interp->try_location = NULL;

	interp->compatibility = compat_flags;
	interp->random_seed = (*SEE_system.random_seed)();
	interp->trace = SEE_system.default_trace;
	interp->traceback = NULL;
	interp->locale = SEE_system.default_locale;
	interp->recursion_limit = SEE_system.default_recursion_limit;
	interp->sec_domain = NULL;

	/* Allocate object storage first, since dependencies are complex */
	SEE_Array_alloc(interp);
	SEE_Boolean_alloc(interp);
	SEE_Date_alloc(interp);
	SEE_Error_alloc(interp);
	SEE_Function_alloc(interp);
	SEE_Global_alloc(interp);
	SEE_Math_alloc(interp);
	SEE_Number_alloc(interp);
	SEE_Object_alloc(interp);
	SEE_RegExp_alloc(interp);
	SEE_String_alloc(interp);
	_SEE_module_alloc(interp);

	/* Initialise the per-interpreter intern table now */
	_SEE_intern_init(interp);

	/* Initialise the objects; order *shouldn't* matter */
	SEE_Array_init(interp);
	SEE_Boolean_init(interp);
	SEE_Date_init(interp);
	SEE_Error_init(interp);
	SEE_Global_init(interp);
	SEE_Math_init(interp);
	SEE_Number_init(interp);
	SEE_Object_init(interp);
	SEE_RegExp_init(interp);
	SEE_String_init(interp);
	SEE_Function_init(interp);	/* Call late because of parser use */
	_SEE_module_init(interp);
}
