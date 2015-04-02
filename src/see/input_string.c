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
/* $Id: input_string.c 751 2004-12-02 11:24:25Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
#endif

#include "see_mem.h"
#include "see_type.h"
#include "see_input.h"
#include "see_string.h"

#include "see_stringdefs.h"

/*
 * input for in-memory ECMAscript UTF-16 strings.
 * (Used when 'eval'ing a string from within the interpreter.)
 */

static SEE_unicode_t input_string_next(struct SEE_input *);
static void input_string_close(struct SEE_input *);

struct SEE_inputclass input_string_class = {
	input_string_next,
	input_string_close
};

struct input_string {
	struct SEE_input	inp;
	const SEE_char_t *cur, *end;
};

static SEE_unicode_t
input_string_next(inp)
	struct SEE_input *inp;
{
	struct input_string *inps = (struct input_string *)inp;
	SEE_unicode_t next, c, c2;

	next = inps->inp.lookahead;
	if (inps->cur >= inps->end) {
		inps->inp.eof = 1;
	} else {
		c = *inps->cur++;
		if ((c & 0xfc00) == 0xd800 && inps->cur < inps->end) {
		    c2 = *inps->cur;
		    if ((c2 & 0xfc00) == 0xdc00)
		    {
			inps->cur++;
			c = (((c & 0x3ff) << 10) | (c2 & 0x3ff)) + 0x10000;
		    } else {
			c = SEE_INPUT_BADCHAR;
		    }
		}
		inps->inp.lookahead = c;
		inps->inp.eof = 0;
	}
	return next;
}

static void
input_string_close(inp)
	struct SEE_input *inp;
{
	struct input_string *inps = (struct input_string *)inp;

	inps->cur = NULL;
	inps->end = NULL;
}

struct SEE_input *
SEE_input_string(interp, s)
	struct SEE_interpreter *interp;
	struct SEE_string *s;
{
	struct input_string *inps;

	inps = SEE_NEW(interp, struct input_string);
	inps->cur = s->data;
	inps->end = s->data + s->length;
	inps->inp.inputclass = &input_string_class;
	inps->inp.interpreter = interp;
	inps->inp.filename = STR(string_input_name);
	inps->inp.first_lineno = 1;
	SEE_INPUT_NEXT((struct SEE_input *)inps);	/* prime */
	return (struct SEE_input *)inps;
}
