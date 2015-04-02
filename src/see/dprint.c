/*
 * Copyright (c) 2005
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
/* $Id$ */

/*
 * This module controls how debugging messages are printed.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef NDEBUG

#if STDC_HEADERS
# include <stdio.h>
# include <stdarg.h>
#endif

#include "see_string.h"
#include "see_debug.h"

#include "see_dprint.h"

/*
 * Prints text for debugging
 */
void
SEE_dprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
}

/*
 * Prints a SEE string for debugging
 */
void
SEE_dprints(s)
	const struct SEE_string *s;
{
	(void)SEE_string_fputs(s, stderr);
}

/*
 * Prints a SEE value for debugging
 */
void
SEE_dprintv(interp, v)
	struct SEE_interpreter *interp;
	const struct SEE_value *v;
{
	SEE_PrintValue(interp, v, stderr);
}

/*
 * Prints a SEE object for debugging
 */
void
SEE_dprinto(interp, o)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
{
	SEE_PrintObject(interp, o, stderr);
}

#endif
