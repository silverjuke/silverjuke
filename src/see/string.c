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
/* $Id: string.c 1098 2006-07-28 15:20:53Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <string.h>
# include <stdarg.h>
#endif

#if 1 // HAVE_LIMITS_H - EDIT BY ME limmits.h is available
# include <limits.h>
#else
# define UINT_MAX (~(unsigned int)0)
#endif

#include "see_mem.h"
#include "see_type.h"
#include "see_string.h"
#include "see_object.h"
#include "see_error.h"
#include "see_try.h"
#include "see_interpreter.h"
#include "see_value.h"

#include "see_stringdefs.h"
#include "see_printf.h"

static void growby(struct SEE_string *s, unsigned int extra);
static void simple_growby(struct SEE_string *s, unsigned int extra);
static void string_append_int(struct SEE_string *s, unsigned int i);

static struct SEE_stringclass fixed_stringclass = {
	0						/* growby */
};

#define IS_GROWABLE(s)	((s)->stringclass && (s)->stringclass->growby)
#define MAKE_UNGROWABLE(s) (s)->stringclass = 0
#define ASSERT_GROWABLE(s) SEE_ASSERT(s->interpreter, IS_GROWABLE(s))

/*
 * Strings.
 *
 * Strings are arrays of 16-bit characters with UTF-16 encoding.
 * Because the ECMAScript standard never needs the strings
 * interpreted in their full unicode form, (such as UCS-4),
 * the implementation here maintains them as an array of 16-bit 
 * unsigned integers.
 */

/*
 * Grows a string's data[] array by the given length increment
 */
static void
growby(s, extra)
	struct SEE_string *s;
	unsigned int extra;
{
	if (!IS_GROWABLE(s))
		SEE_error_throw_string(s->interpreter, s->interpreter->Error, 
			STR(no_string_space));
	(*s->stringclass->growby)(s, extra);
}

/*
 * Creates a new, growable string containing a copy of an existing one.
 */
struct SEE_string *
SEE_string_dup(interp, s)
	struct SEE_interpreter *interp;
	const struct SEE_string *s;
{
	struct SEE_string *cp;

	cp = SEE_string_new(interp, s->length);
	SEE_string_append(cp, s);
	return cp;
}

/*
 * Creates a new (ungrowable) string that is a substring of another.
 * Raises an error if the substring indixies are out of bounds.
 * The source string (s) may continue to be grown, but should not
 * be changed.
 */
struct SEE_string *
SEE_string_substr(interp, s, start, len)
	struct SEE_interpreter *interp;
	struct SEE_string *s;
	int start, len;
{
	struct SEE_string *subs;

	if (start < 0 
	 || len < 0 
	 || (unsigned int)(start + len) > s->length)
		SEE_error_throw_string(interp, interp->Error, STR(bad_arg));

	subs = SEE_NEW(interp, struct SEE_string);
	subs->length = len;
	subs->data = s->data + start;
	subs->interpreter = interp;
	subs->flags = 0;
	subs->stringclass = &fixed_stringclass;
	return subs;
}

/*
 * Compares two strings, a and b, lexicographically by UTF-16. Returns
 *	-1 if a < b
 *	 0 if a = b
 *	+1 if a > b
 */
int
SEE_string_cmp(a, b)
	const struct SEE_string *a, *b;
{
	const SEE_char_t *ap, *bp;
	unsigned int alen, blen;

	if (a == b)
		return 0;

	ap = a->data; alen = a->length;
	bp = b->data; blen = b->length;

	while (alen && blen && *ap == *bp) {
		alen--;
		blen--;
		ap++;
		bp++;
	}
	if (!alen) {
		if (!blen)
			return 0;
		return -1;
	}
	if (!blen)
		return 1;
	return (*ap < *bp) ? -1 : 1;
}

/*
 * Appends character c to the end of string s.
 */
void
SEE_string_addch(s, c)
	struct SEE_string *s;
	int c;				/* promoted SEE_char_t */
{
	ASSERT_GROWABLE(s);
	growby(s, 1);
	s->data[s->length++] = c;
}

/*
 * Appends string t to the end of string s.
 */
void
SEE_string_append(s, t)
	struct SEE_string *s;
	const struct SEE_string *t;
{
	ASSERT_GROWABLE(s);
	if (t->length) {
	    growby(s, t->length);
	    memcpy(s->data + s->length, t->data, 
		t->length * sizeof (SEE_char_t));
	    s->length += t->length;
	}
}

/*
 * Appends 7-bit ascii string to the end of string s.
 */
void
SEE_string_append_ascii(s, ascii)
	struct SEE_string *s;
	const char *ascii;
{
	const char *p;

	ASSERT_GROWABLE(s);
	for (p = ascii; *p; p++)
		SEE_ASSERT(s->interpreter, !(*p & 0x80));
	if (p - ascii) {
	    growby(s, p - ascii);
	    for (p = ascii; *p; p++)
	    	s->data[s->length++] = *p;
	}
}

/*
 * Appends a signed integer onto the end of string s
 */
void
SEE_string_append_int(s, i)
	struct SEE_string *s;
	int i;
{
	ASSERT_GROWABLE(s);
	if (i < 0) {
		i = -i;
		SEE_string_addch(s, '-');
	}
	string_append_int(s, i);
}

static void
string_append_int(s, i)
	struct SEE_string *s;
	unsigned int i;
{
	if (i >= 10)
		string_append_int(s, i / 10);
	growby(s, 1);
	s->data[s->length++] = (i % 10) + '0';
}

/* 
 * Converts a UTF-16 string to UTF-8 and write to a stdio file.
 * Returns 0 on success, like fputs().
 * Returns EOF on write error, like fputs().
 * Throws exception on conversion error, unlike fputs().
 * Ref: RFC2279, RFC2781
 */
int
SEE_string_fputs(s, f)
	const struct SEE_string *s;
	FILE *f;
{
	unsigned int i;
	SEE_char_t ch, ch2;
	struct SEE_interpreter *interp = s->interpreter;

#define OUTPUT(c) do { if (fputc(c, f) == EOF) goto error; } while (0)

	for (i = 0; i < s->length; i++) {
		ch = s->data[i];
		if ((ch & 0xff80) == 0) 
		    OUTPUT(ch & 0x7f);
		else if ((ch & 0xf800) == 0) {
		    OUTPUT(0xc0 | ((ch >> 6) & 0x1f));
		    OUTPUT(0x80 | (ch & 0x3f));
		} else if ((ch & 0xfc00) != 0xd800) {
		    OUTPUT(0xe0 | ((ch >> 12) & 0x0f));
		    OUTPUT(0x80 | ((ch >> 6) & 0x3f));
		    OUTPUT(0x80 | (ch & 0x3f));
		} else {
		    if (i == s->length - 1)
			SEE_error_throw_string(interp, interp->Error, 
				STR(bad_utf16_string));
		    ch2 = s->data[++i];
		    if ((ch2 & 0xfc00) != 0xdc00)
			SEE_error_throw_string(interp, interp->Error, 
				STR(bad_utf16_string));
		    ch = (ch & 0x03ff) + 0x0040;
		    OUTPUT(0xf0 | ((ch >> 8) & 0x07));
		    OUTPUT(0x80 | ((ch >> 2) & 0x3f));
		    OUTPUT(0x80 | ((ch & 0x3) << 4) |
				 ((ch2 & 0x03c0) >> 6));
		    OUTPUT(0x80 | (ch2 & 0x3f));
		}
	}
	return 0;
    error:
	return EOF;
#undef OUTPUT
}

/*------------------------------------------------------------
 * The simple string class
 */
struct simple_string {
	struct SEE_string string;
	unsigned int space;
};

/*
 * Tunable parameters, specified in bytes.
 * These can be varied at build time to suit a particular platform.
 * TODO: provide support for segmented strings?
 */
#ifndef STRING_INITIAL_SIZE
# define STRING_INITIAL_SIZE	512
#endif
#ifndef STRING_MAXIMUM_SIZE
# define STRING_MAXIMUM_SIZE	UINT_MAX
#endif
#ifndef STRING_MAXIMUM_PADDING
# define STRING_MAXIMUM_PADDING	4096
#endif

/*
 * Derived variables, in 'SEE_char_t's:
 *   INITSPACE: initial length of a string
 *   MAXSPACE:  biggest string length possible with length type
 *   BIGSPACE:  largest string length allocation likely to succeed
 */
#define INITSPACE	(STRING_INITIAL_SIZE / sizeof (SEE_char_t))
#define MAXSPACE	(STRING_MAXIMUM_SIZE / sizeof (SEE_char_t))
#define BIGSPACE	((STRING_MAXIMUM_SIZE - STRING_MAXIMUM_PADDING) / \
			 sizeof (SEE_char_t))

/* 
 * Grows the string storage to have at least current+extra elements of storage.
 * Simple strings never shrink. Growth is in powers of two from INITSPACE.
 */
static void
simple_growby(s, extra)
	struct SEE_string *s;
	unsigned int extra;
{
	struct simple_string *ss = (struct simple_string *)s;
	unsigned int minspace, new_space;
	SEE_char_t *new_data;
	struct SEE_interpreter *interp = s->interpreter;

	if (s->length > MAXSPACE - extra)
		SEE_error_throw_string(interp, interp->Error,
		                STR(string_limit_reached));
	minspace = s->length + extra;

	if (ss->space < minspace) {
	    /*
	     * The non-empty string space starts at INITSPACE, and then
	     * doubles until it would exceepd BIGSPACE, at which point it
	     * is set to BIGSPACE (the practical size limit of memory 
	     * allocations). A further increase is permitted to MAXSPACE,
	     * but the memory subsystem is expected to complain at that
	     */
	    new_space = ss->space;
	    while (new_space < minspace) {
		if (new_space == 0)
		    new_space = INITSPACE;
		else if (ss->space >= BIGSPACE)
		    new_space = MAXSPACE;
		else if (ss->space > BIGSPACE/2)
		    new_space = BIGSPACE;
		else
		    new_space = new_space * 2;
	    }
	    new_data = SEE_NEW_STRING_ARRAY(interp, SEE_char_t, new_space);
	    if (s->length)
		memcpy(new_data, s->data, s->length * sizeof (SEE_char_t));
	    ss->string.data = new_data;
            ss->space = new_space;
	}
}

static struct SEE_stringclass simple_stringclass = {
	simple_growby					/* growby */
};

/*
 * Constrycts a new, empty string. 
 * Storage is pre-allocated for the number of UTF-16 characters indicated
 * by the 'space' argument.
 */
struct SEE_string *
SEE_string_new(interp, space)
	struct SEE_interpreter *interp;
	unsigned int space;
{
	struct simple_string *ss = SEE_NEW(interp, struct simple_string);

	ss->string.length = 0;
	ss->string.data = NULL;
	ss->string.interpreter = interp;
	ss->string.flags = 0;
	ss->space = 0;
	ss->string.stringclass = &simple_stringclass;
	if (space)
	    simple_growby((struct SEE_string *)ss, space);
	return (struct SEE_string *)ss;
}

/*
 * Creates a string using vsprintf-like arguments.
 */
struct SEE_string *
SEE_string_vsprintf(interp, fmt, ap)
	struct SEE_interpreter *interp;
	const char *fmt;
	va_list ap;
{
	struct simple_string *ss;
	
	ss = (struct simple_string *)SEE_string_new(interp, 0);
	_SEE_vsprintf(interp, &ss->string, fmt, ap);
	ss->space = ss->string.length;
	return (struct SEE_string *)ss;
}

/*
 * Creates a string using sprintf-like arguments.
 */
struct SEE_string *
SEE_string_sprintf(struct SEE_interpreter *interp, const char *fmt, ...)
{
	va_list ap;
	struct SEE_string *s;

	va_start(ap, fmt);
	s = SEE_string_vsprintf(interp, fmt, ap);
	va_end(ap);
	return s;
}

/**
 * Returns a quoted, escaped string, suitable for lexical analysis.
 */
struct SEE_string *
SEE_string_literal(interp, s)
	struct SEE_interpreter *interp;
	const struct SEE_string *s;
{
	struct SEE_string *lit;
	unsigned int i;
	SEE_char_t c;

	if (s == NULL)
		return NULL;

	lit = SEE_string_new(interp, 0);
	SEE_string_addch(lit, '\"');
	for (i = 0; i < s->length; i++) {
	    c = s->data[i];
	    switch (c) {
	    case 0x0008:	SEE_string_addch(lit, '\\');
				SEE_string_addch(lit, 'b');
				break;
	    case 0x0009:	SEE_string_addch(lit, '\\');
				SEE_string_addch(lit, 't');
				break;
	    case 0x000a:	SEE_string_addch(lit, '\\');
				SEE_string_addch(lit, 'n');
				break;
	    case 0x000b:	SEE_string_addch(lit, '\\');
				SEE_string_addch(lit, 'v');
				break;
	    case 0x000c:	SEE_string_addch(lit, '\\');
				SEE_string_addch(lit, 'f');
				break;
	    case 0x000d:	SEE_string_addch(lit, '\\');
				SEE_string_addch(lit, 'r');
				break;
	    case '\\':
	    case '\"':		SEE_string_addch(lit, '\\');
				SEE_string_addch(lit, c);
				break;
	    default:
		if (c >= 0x20 && c < 0x7f)
		   SEE_string_addch(lit, c);
		else if (c < 0x100) {
		   SEE_string_addch(lit, '\\');
		   SEE_string_addch(lit, 'x');
		   SEE_string_addch(lit, SEE_hexstr_lowercase[(c >> 4) & 0xf]);
		   SEE_string_addch(lit, SEE_hexstr_lowercase[ c       & 0xf]);
		} else {
		   SEE_string_addch(lit, '\\');
		   SEE_string_addch(lit, 'u');
		   SEE_string_addch(lit, SEE_hexstr_lowercase[(c >>12) & 0xf]);
		   SEE_string_addch(lit, SEE_hexstr_lowercase[(c >> 8) & 0xf]);
		   SEE_string_addch(lit, SEE_hexstr_lowercase[(c >> 4) & 0xf]);
		   SEE_string_addch(lit, SEE_hexstr_lowercase[ c       & 0xf]);
		}
	    }
	}
	SEE_string_addch(lit, '\"');
	return lit;
}

/*
 * Frees a string. The caller must know that the string data is not in 
 * use in any other place. That includes by substring references, and
 * the piggybacking side effects of SEE_string_concat.
 */
void
SEE_string_free(interp, sp)
	struct SEE_interpreter *interp;
	struct SEE_string **sp;
{
	if (*sp) {
		SEE_free(interp, (void **)&(*sp)->data);
		SEE_free(interp, (void **)sp);
	}
}

/*
 * Returns the number of characters a UTF-8 representation would take.
 * Does not include the trailing nul
 */
SEE_size_t
SEE_string_utf8_size(interp, s)
	struct SEE_interpreter *interp;
	const struct SEE_string *s;
{
	SEE_size_t len;
	unsigned int i;
	SEE_char_t ch, ch2;

	len = 0;
	for (i = 0; i < s->length; i++) {
		ch = s->data[i];
		if ((ch & 0xff80) == 0)
		    len += 1;
		else if ((ch & 0xf800) == 0)
		    len += 2;
		else if ((ch & 0xfc00) != 0xd800)
		    len += 3;
		else {
		    if (i == s->length - 1)
			SEE_error_throw_string(interp, interp->Error, 
				STR(bad_utf16_string));
		    ch2 = s->data[++i];
		    if ((ch2 & 0xfc00) != 0xdc00)
			SEE_error_throw_string(interp, interp->Error, 
				STR(bad_utf16_string));
		    len += 4;
		}
	}
	return len;
}

/*
 * Converts a SEE string into a UTF8 buffer.
 * Throws a RangeError if the decoded string, including terminating nul, 
 * would exceed the size of the given buffer.
 * If the string itself is illegally formed, a generic Error is thrown.
 */
void
SEE_string_toutf8(interp, buf, buflen, s)
	struct SEE_interpreter *interp;
	char *buf;
	SEE_size_t buflen;
	const struct SEE_string *s;
{
	unsigned int i;
	SEE_char_t ch, ch2;

#define OUTPUT(c) do { 				\
	if (buflen <= 1) goto toolong; 		\
	*buf++ = (c); 				\
	buflen--; 				\
    } while (0)

	for (i = 0; i < s->length; i++) {
		ch = s->data[i];
		if ((ch & 0xff80) == 0) 
		    OUTPUT(ch & 0x7f);
		else if ((ch & 0xf800) == 0) {
		    OUTPUT(0xc0 | ((ch >> 6) & 0x1f));
		    OUTPUT(0x80 | (ch & 0x3f));
		} else if ((ch & 0xfc00) != 0xd800) {
		    OUTPUT(0xe0 | ((ch >> 12) & 0x0f));
		    OUTPUT(0x80 | ((ch >> 6) & 0x3f));
		    OUTPUT(0x80 | (ch & 0x3f));
		} else {
		    if (i == s->length - 1)
			SEE_error_throw_string(interp, interp->Error, 
				STR(bad_utf16_string));
		    ch2 = s->data[++i];
		    if ((ch2 & 0xfc00) != 0xdc00)
			SEE_error_throw_string(interp, interp->Error, 
				STR(bad_utf16_string));
		    ch = (ch & 0x03ff) + 0x0040;
		    OUTPUT(0xf0 | ((ch >> 8) & 0x07));
		    OUTPUT(0x80 | ((ch >> 2) & 0x3f));
		    OUTPUT(0x80 | ((ch & 0x3) << 4) |
				 ((ch2 & 0x03c0) >> 6));
		    OUTPUT(0x80 | (ch2 & 0x3f));
		}
	}

	if (buflen < 1) goto toolong;
	*buf = '\0';
	return;

    toolong:
	SEE_error_throw_string(interp, interp->RangeError, 
		STR(string_limit_reached));
}
#undef OUTPUT

/*
 * Extends a string, marking the original string as ungrowable.
 */
static struct SEE_string *
simple_concat(interp, a, b)
	struct SEE_interpreter *interp;
	struct simple_string *a;
	const struct SEE_string *b;
{
	struct simple_string *cp;

	cp = SEE_NEW(interp, struct simple_string);
	memcpy(cp, a, sizeof (struct simple_string));
	MAKE_UNGROWABLE(&a->string);
	SEE_string_append(&cp->string, b);
	return (struct SEE_string *)cp;
}

/*
 * Concatenates two strings together and return the resulting string.
 * May return one of the original strings, or a new string altogether.
 * May modify a. String b will not be modified, but it may be returned.
 */
struct SEE_string *
SEE_string_concat(interp, a, b)
	struct SEE_interpreter *interp;
	struct SEE_string *a, *b;
{
	struct SEE_string *s;

	if (a->length == 0)
		return b;
	if (b->length == 0)
		return a;

	if (a->stringclass == &simple_stringclass) 
		return simple_concat(interp, (struct simple_string *)a, b);

	s = SEE_string_new(interp, a->length + b->length);
	if (a->length)
		memcpy(s->data, a->data, a->length * sizeof (SEE_char_t));
	if (b->length)
		memcpy(s->data + a->length, b->data, 
		    b->length * sizeof (SEE_char_t));
	s->length = a->length + b->length;
	return s;
}
