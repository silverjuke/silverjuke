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
/* $Id: input_lookahead.c 751 2004-12-02 11:24:25Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
#endif

#include "see_type.h"
#include "see_input.h"
#include "see_mem.h"

/*
 * an n-character lookahead input filter
 */

static SEE_unicode_t la_next(struct SEE_input *);
static void	    la_close(struct SEE_input *);

static struct SEE_inputclass la_inputclass = {
	la_next,
	la_close
};

struct lookahead {
	struct SEE_input    input;
	struct SEE_input *	sub;
	int		max, ptr;
	struct {
		SEE_unicode_t ch;
		int	     eof;
	} buf[1];
};

static SEE_unicode_t
la_next(inp)
	struct SEE_input *inp;
{
	struct lookahead *la = (struct lookahead *)inp;
	SEE_unicode_t next = inp->lookahead;
	struct SEE_input *sub = la->sub;

	inp->lookahead = la->buf[la->ptr].ch;
	inp->eof = la->buf[la->ptr].eof;
	la->buf[la->ptr].ch = sub->lookahead;
	la->buf[la->ptr].eof = sub->eof;
	if (!sub->eof)
		SEE_INPUT_NEXT(sub);
	la->ptr = (la->ptr + 1) % la->max;
	return next;
}

/*
 * Return the lookahead buffer that we have available
 */
int
SEE_input_lookahead_copy(inp, buf, buflen)
	struct SEE_input *inp;
	SEE_unicode_t *buf;
	int buflen;
{
	struct lookahead *la = (struct lookahead *)inp;
	int i;

	if (buflen <= 0 || inp->eof)
		return 0;
	buf[0] = inp->lookahead;
	for (i = 0; i < la->max && i + 1 < buflen && 
	    !la->buf[(la->ptr + i) % la->max].eof; i++)
		buf[i+1] = la->buf[(la->ptr + i) % la->max].ch;
	return i + 1;
}

static void
la_close(inp)
	struct SEE_input *inp;
{
	struct lookahead *la = (struct lookahead *)inp;

	SEE_INPUT_CLOSE(la->sub);
}

struct SEE_input *
SEE_input_lookahead(sub, max)
	struct SEE_input *sub;
	int max;
{
	struct lookahead *la;
	int i;

	la = (struct lookahead *)SEE_malloc(sub->interpreter,
	    sizeof (struct lookahead) + (max - 1) * sizeof la->buf[0]);
	la->input.inputclass = &la_inputclass;
	la->input.filename = sub->filename;
	la->input.first_lineno = sub->first_lineno;
	la->input.interpreter = sub->interpreter;
	la->sub = sub;
	la->ptr = 0;
	la->max = max;
	for (i = 0; i < max + 1; i++)
		la_next((struct SEE_input *)la);
	return (struct SEE_input *)la;
}
