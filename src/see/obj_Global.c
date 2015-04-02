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
/* $Id: obj_Global.c 1126 2006-08-05 12:48:25Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#endif

#include "see_mem.h"
#include "see_type.h"
#include "see_string.h"
#include "see_value.h"
#include "see_try.h"
#include "see_object.h"
#include "see_native.h"
#include "see_cfunction.h"
#include "see_error.h"
#include "see_interpreter.h"
#include "see_debug.h"
#include "see_context.h"

#include "see_stringdefs.h"
#include "see_parse.h"
#include "see_scope.h"
#include "see_function.h"
#include "see_unicode.h"
#include "see_dtoa.h"
#include "see_init.h"
#include "see_dprint.h"
#include "see_nmath.h"

#define POSITIVE	(1)
#define NEGATIVE	(-1)

/*
 * The Global object.
 *
 * This object is always at the end of the scope chain.
 * (Host objects should be inserted into it at startup
 * to be reachable.)
 */

static void global_eval(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void global_parseInt(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void global_parseFloat(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void global_isNaN(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void global_isFinite(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void global_decodeURI(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void global_decodeURIComponent(struct SEE_interpreter *, 
        struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
        struct SEE_value *);
static void global_encodeURI(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void global_encodeURIComponent(struct SEE_interpreter *, 
        struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
        struct SEE_value *);
static void global_escape(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void global_unescape(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
#ifndef NDEBUG
static void global_write(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void global_writeval(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
#endif

static int is_StrWhiteSpace(SEE_char_t);
static void AddEscape(struct SEE_interpreter *, struct SEE_string *, 
        unsigned int);
static struct SEE_string *Encode(struct SEE_interpreter *, 
        struct SEE_string *, const unsigned char *);
static SEE_char_t urihexval(struct SEE_interpreter *, unsigned int, 
        unsigned int);
static struct SEE_string *Decode(struct SEE_interpreter *, 
        struct SEE_string *, const unsigned char *);

/* Note: [[Class]] is not "Global" but "global" for mozilla compatibility */
static struct SEE_objectclass global_class = {
	"global",			/* Class */
	SEE_native_get,			/* Get */
	SEE_native_put,			/* Put */
	SEE_native_canput,		/* CanPut */
	SEE_native_hasproperty,		/* HasProperty */
	SEE_native_delete,		/* Delete */
	SEE_native_defaultvalue,	/* DefaultValue */
	SEE_native_enumerator,		/* DefaultValue */
	NULL,				/* Construct */
	NULL				/* Call */
};

void
SEE_Global_alloc(interp)
	struct SEE_interpreter *interp;
{
	interp->Global = 
		(struct SEE_object *)SEE_NEW(interp, struct SEE_native);
	interp->Global_scope = 
		SEE_NEW(interp, struct SEE_scope);

	/* XXX should properly check that this is never referenced */
	interp->Global_eval = (struct SEE_object *)1;	
}

void
SEE_Global_init(interp)
	struct SEE_interpreter *interp;
{
	struct SEE_object *Global;		/* struct SEE_native */
	struct SEE_value v;

	Global = interp->Global;
	interp->Global_scope->next = NULL;
	interp->Global_scope->obj = Global;

	SEE_native_init((struct SEE_native *)Global, interp, &global_class, 
	    (SEE_COMPAT_JS(interp, >=, JS11))  	/* EXT:17 */
		? interp->Object_prototype 
		: NULL);

	/* NaN - 15.1.1.1 */
	SEE_SET_NUMBER(&v, SEE_NaN);
	SEE_OBJECT_PUT(interp, Global, STR(NaN), &v, 
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE);

	/* Infinity - 15.1.1.2 */
	SEE_SET_NUMBER(&v, SEE_Infinity);
	SEE_OBJECT_PUT(interp, Global, STR(Infinity), &v, 
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE);

	/* undefined - 15.1.1.3 */
	SEE_SET_UNDEFINED(&v);
	SEE_OBJECT_PUT(interp, Global, STR(undefined), &v, 
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE);

#define PUTFUNC(name,len) \
	SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, global_##name, 	\
		STR(name), len)); 					\
	SEE_OBJECT_PUT(interp, Global, STR(name), &v, SEE_ATTR_DEFAULT);

	PUTFUNC(eval, 1)				 /* 15.1.2.1 */
	interp->Global_eval = v.u.object;

	PUTFUNC(parseInt, 2)				 /* 15.1.2.2 */
	PUTFUNC(parseFloat, 1)				 /* 15.1.2.3 */
	PUTFUNC(isNaN, 1)				 /* 15.1.2.4 */
	PUTFUNC(isFinite, 1)				 /* 15.1.2.5 */
	PUTFUNC(decodeURI, 1)				 /* 15.1.3.1 */
	PUTFUNC(decodeURIComponent, 1)			 /* 15.1.3.2 */
	PUTFUNC(encodeURI, 1)				 /* 15.1.3.3 */
	PUTFUNC(encodeURIComponent, 1)			 /* 15.1.3.4 */

	if (interp->compatibility & SEE_COMPAT_262_3B) {
	    PUTFUNC(escape, 1)
	    PUTFUNC(unescape, 1)
	}

#ifndef NDEBUG
	/* Nonstandard methods */
	PUTFUNC(write, 1)
	PUTFUNC(writeval, 1)
#endif

#define PUTOBJ(name) \
	SEE_SET_OBJECT(&v, interp->name); \
	SEE_OBJECT_PUT(interp, Global, STR(name), &v, SEE_ATTR_DEFAULT);

	PUTOBJ(Object)					/* 15.1.4.1 */
	PUTOBJ(Function)				/* 15.1.4.2 */
	PUTOBJ(Array)					/* 15.1.4.3 */
	PUTOBJ(String)					/* 15.1.4.4 */
	PUTOBJ(Boolean)					/* 15.1.4.5 */
	PUTOBJ(Number)					/* 15.1.4.6 */
	PUTOBJ(Date)					/* 15.1.4.7 */
	PUTOBJ(RegExp)					/* 15.1.4.8 */
	PUTOBJ(Error)					/* 15.1.4.9 */
	PUTOBJ(EvalError)				/* 15.1.4.10 */
	PUTOBJ(RangeError)				/* 15.1.4.11 */
	PUTOBJ(ReferenceError)				/* 15.1.4.12 */
	PUTOBJ(SyntaxError)				/* 15.1.4.13 */
	PUTOBJ(TypeError)				/* 15.1.4.14 */
	PUTOBJ(URIError)				/* 15.1.4.15 */

	PUTOBJ(Math)					/* 15.1.5.1 */
}

/* Global.eval (15.1.2.1) */
static void
global_eval(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
        /*
	 * This function should never be called.
	 *
         * Invocation of the Global.eval() function object is special:
         * parse.c checks for it and calls the C function eval()
         * there directly. The only way we could get here is if the host
         * application has tried to call it directly, which is naughty!
         */
	SEE_error_throw_string(interp, interp->EvalError, STR(internal_error));
}

static int
is_StrWhiteSpace(ch)
	SEE_char_t ch;
{
#if WITH_UNICODE_TABLES
	int i;
#endif

	if (ch == 0x0009 ||	/* TAB */
	    ch == 0x0020 ||	/* SP */
	    ch == 0x000c ||	/* FF */
	    ch == 0x000b ||	/* VT */
	    ch == 0x000d ||	/* CR */
	    ch == 0x000a)  	/* LF */
		return 1;
#if WITH_UNICODE_TABLES
	if (ch == 0x00a0 ||	/* NBSP */
	    ch == 0x2028 ||	/* LS */
	    ch == 0x2029)  	/* PS */
		return 1;
	for (i = 0; i < SEE_unicode_Zscodeslen; i++)
		if (ch == SEE_unicode_Zscodes[i])
			return 1;
#endif
	return 0;
}

/* Global.parseInt (15.1.2.2) */
static void
global_parseInt(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	unsigned int j, i, start;
	SEE_int32_t R;
	struct SEE_value v;
	struct SEE_string *s;
	SEE_number_t n, sign;

	if (argc < 1) {
		SEE_SET_NUMBER(res, SEE_NaN);
		return;
	}

	SEE_ToString(interp, argv[0], &v);
	s = v.u.string;
	i = 0;
	while (i < s->length && is_StrWhiteSpace(s->data[i])) 
		i++;
	sign = POSITIVE;
	if (i < s->length && s->data[i] == '-') 
		sign = NEGATIVE;
	if (i < s->length && (s->data[i] == '-' || s->data[i] == '+')) 
		i++;
	if (argc < 2)
		R = 0;
	else
		R = SEE_ToInt32(interp, argv[1]);
	if (R != 0 && (R < 2 || R > 36)) {
		SEE_SET_NUMBER(res, SEE_NaN);
		return;
	}
	if ((R == 0 || R == 16) && 
 	    i + 1 < s->length && 
	    s->data[i] == '0' &&
	    (s->data[i + 1] == 'x' || s->data[i + 1] == 'X'))
	{
		i += 2;
		R = 16;
	}
	else if (SEE_COMPAT_JS(interp, >=, JS11) && 	/* EXT:18 */
	    R == 0 && i < s->length &&
	    s->data[i] == '0')
	{
		R = 8;
	}
	if (R == 0)
		R = 10;
	start = i;
	while (i < s->length) {
		SEE_char_t ch;
		int digit;

		ch = s->data[i];
		if (ch >= '0' && ch <= '9') 
			digit = ch - '0';
		else if (ch >= 'a' && ch <= 'z')
			digit = ch - 'a' + 10;
		else if (ch >= 'A' && ch <= 'Z')
			digit = ch - 'A' + 10;
		else
			break;
		if (digit >= R)
			break;
		i++;
	}
	if (i == start) {
		SEE_SET_NUMBER(res, SEE_NaN);
		return;
	}
	n = 0;
	for (j = 0; j < i - start; j++) {
		SEE_number_t factor = 
			NUMBER_pow((SEE_number_t)R, (SEE_number_t)j);
		SEE_char_t ch;
		int digit;

		if (SEE_ISINF(factor)) {
			n = factor;
			break;
		}
		ch = s->data[i - j - 1];
		if (ch >= '0' && ch <= '9') 
			digit = ch - '0';
		else if (ch >= 'a' && ch <= 'z')
			digit = ch - 'a' + 10;
		else /* if (ch >= 'A' && ch <= 'Z') */
			digit = ch - 'A' + 10;
		n += factor * digit;
	}
	SEE_SET_NUMBER(res, NUMBER_copysign(n, sign));
}


/* Global.parseFloat (15.1.2.3) */
static void
global_parseFloat(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	/*
	 * StrDecimalLiteral:
	 *	[+-]? 
	 *	( Infinity | ( Digits | . Digits | Digits . ) 
	 *	             ( [Ee] [+-]? Digits ) ? )
	 * Digits:
	 *	[0-9]+
	 */

	struct SEE_value v;
	struct SEE_string *s, *Inf;
	SEE_number_t n, sign;
	unsigned int i, hasdigits;

	if (argc < 1) {
		SEE_SET_NUMBER(res, SEE_NaN);
		return;
	}
	SEE_ToString(interp, argv[0], &v);
	s = v.u.string;
	i = 0;

	while (i < s->length && is_StrWhiteSpace(s->data[i]))
		i++;

	sign = POSITIVE;
	if (i < s->length && (s->data[i] == '-' || s->data[i] == '+')) {
		if (s->data[i] == '-') sign = NEGATIVE;
		i++;
	}

	Inf = STR(Infinity);
	if (i + Inf->length <= s->length && 
	    memcmp(&s->data[i], Inf->data, 
		   Inf->length * sizeof Inf->data[0]) == 0)
	{
	    n = SEE_Infinity;
	    i += Inf->length;
	} else {
	    char *numbuf, *endstr;
	    int start = i;
	    int upto = i;

	    hasdigits = 0;
	    while (i < s->length && (s->data[i] >= '0' && s->data[i] <= '9')) {
		hasdigits = 1;
		i++;
	    }
	    if (i < s->length && s->data[i] == '.') {
		i++;
		while (i < s->length && 
		       (s->data[i] >= '0' && s->data[i] <= '9'))
		{
		    hasdigits = 1;
		    i++;
		}
	    }
	    if (!hasdigits) {
		SEE_SET_NUMBER(res, SEE_NaN);
		return;
	    }
	    upto = i;
	    if (i < s->length && (s->data[i] == 'e' || s->data[i] == 'E')) {
		i++;
		if (i < s->length && (s->data[i] == '+' || s->data[i] == '-')) 
		    i++;
		hasdigits = 0;
		while (i < s->length && 
		       (s->data[i] >= '0' && s->data[i] <= '9'))
		{
		    hasdigits = 1;
		    i++;
		}
		/*
		 * Note: even if the exponent is corrupt, the mantissa still
		 * makes a valid number.
		 */
		if (hasdigits)
		    upto = i;
	    }
	    numbuf = SEE_STRING_ALLOCA(interp, char, upto - start + 1);
	    for (i = start; i < upto; i++)
		numbuf[i - start] = s->data[i] & 0x7f;
	    numbuf[i - start] = '\0';
	    endstr = NULL;
	    n = SEE_strtod(numbuf, &endstr);
	    if (!endstr) {
		SEE_SET_NUMBER(res, SEE_NaN);
		return;
	    }
	}
	SEE_SET_NUMBER(res, NUMBER_copysign(n, sign));
}


/* Global.isNaN (15.1.2.4) */
static void
global_isNaN(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;

	if (argc < 1)
		SEE_SET_BOOLEAN(res, 1);
	else {
		SEE_ToNumber(interp, argv[0], &v);
		SEE_SET_BOOLEAN(res, SEE_NUMBER_ISNAN(&v));
	}
}


/* Global.isFinite (15.1.2.5) */
static void
global_isFinite(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;

	if (argc < 1)
		SEE_SET_BOOLEAN(res, 0);
	else {
		SEE_ToNumber(interp, argv[0], &v);
		SEE_SET_BOOLEAN(res, SEE_NUMBER_ISFINITE(&v));
	}
}

static void
AddEscape(interp, R, i)
	struct SEE_interpreter *interp;
	struct SEE_string *R;
	unsigned int i;		/* promoted unsigned char */
{
	char *hexstr = SEE_hexstr_uppercase;

	SEE_string_addch(R, '%');
	SEE_string_addch(R, hexstr[(i >> 4) & 0xf]);
	SEE_string_addch(R, hexstr[i & 0xf]);
}

/* 15.1.3 */
static struct SEE_string *
Encode(interp, s, unesc)
	struct SEE_interpreter *interp;
	struct SEE_string *s;
	const unsigned char *unesc;
{
	struct SEE_string *R;
	int k;
	SEE_unicode_t C;

	R = SEE_string_new(interp, 0);
	k = 0;
	while (k < s->length) {
	    /*
	     * XXX we decode UTF-16 surrogates much earlier than the
	     * expository definition of Encode in the standard does.
	     */
	    if ((s->data[k] & 0xfc00) == 0xdc00)	/* 2nd surrogate */
		SEE_error_throw_string(interp, interp->URIError,
			STR(bad_utf16_string));
	    if ((s->data[k] & 0xfc00) == 0xd800) {
	        C = (s->data[k++] & 0x3ff) << 10;
		if (k < s->length && (s->data[k] & 0xfc00) == 0xdc00)
		    C = (C | (s->data[k++] & 0x3ff)) + 0x10000;
		else
		    SEE_error_throw_string(interp, interp->URIError,
			STR(bad_utf16_string));
	    } else
	        C = s->data[k++];

	    if (C < 0x80) {
		    if (unesc[(C & 0x7f) >> 3] & (1 << (C & 0x7)))
			SEE_string_addch(R, C);
		    else
			AddEscape(interp, R, (unsigned char)(C & 0x7f));
	    } else if (C < 0x800) {
		AddEscape(interp, R, (unsigned char)(0xc0 | (C >>  6 & 0x1f)));
		AddEscape(interp, R, (unsigned char)(0x80 | (C >>  0 & 0x3f))); /* 0x3f, nicht 0x2f: UTF-8 ist für den Bereich 0x80-7FF "% 110xxxxx 10xxxxxx", s. http://www.silverjuke.net/forum/topic-3234.html */
	    } else if (C < 0x10000) {
		AddEscape(interp, R, (unsigned char)(0xe0 | (C >> 12 & 0x0f)));
		AddEscape(interp, R, (unsigned char)(0x80 | (C >>  6 & 0x3f))); /* 0x3f, nicht 0x2f ... */
		AddEscape(interp, R, (unsigned char)(0x80 | (C >>  0 & 0x3f))); /* 0x3f, nicht 0x2f ... */
	    } else /* if (C < 0x200000) */ {
		AddEscape(interp, R, (unsigned char)(0xf0 | (C >> 18 & 0x07)));
		AddEscape(interp, R, (unsigned char)(0x80 | (C >> 12 & 0x3f))); /* 0x3f, nicht 0x2f ... */
		AddEscape(interp, R, (unsigned char)(0x80 | (C >>  6 & 0x3f))); /* 0x3f, nicht 0x2f ... */
		AddEscape(interp, R, (unsigned char)(0x80 | (C >>  0 & 0x3f))); /* 0x3f, nicht 0x2f ... */
	    }
	}
	return R;
}

static unsigned char hexbitmap[] = 
{ 0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x03,    /* [0-9a-fA-F] */
  0x7e,0x00,0x00,0x00,0x7e,0x00,0x00,0x00, };
#define ishex(c) ((c) < 0x80 && (hexbitmap[(c)>>3] & (1 << ((c)&7))))
#define hexval(c) ((c) <= '9' ? (c) - '0' : \
		   (c) <= 'F' ? (c) - 'A' + 10 : (c) - 'a' + 10)

/* Return a decoded hex character, or throw a URIError */
static SEE_char_t
urihexval(interp, c1, c2)
	struct SEE_interpreter *interp;
	unsigned int c1, c2;	/* promoted SEE_char_t */
{

	if (ishex(c1) && ishex(c2))
	    return (hexval(c1) << 4) | hexval(c2);
	else
	    SEE_error_throw_string(interp, interp->URIError, STR(uri_badhex));
	    /* NOTREACHED */
		return 0;
}

/* 15.1.3 */
static struct SEE_string *
Decode(interp, s, resv)
	struct SEE_interpreter *interp;
	struct SEE_string *s;
	const unsigned char *resv;
{
	struct SEE_string *R;
	int k, i, j, start;
	SEE_unicode_t C;
	SEE_char_t D;
	static unsigned char mask[] = { 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe };

	R = SEE_string_new(interp, 0);
	k = 0;
	while (k < s->length) {
	    /*
	     * Decode %-encoded, UTF-8-encoded but leave
	     * encoded strings alone if they would decode into the reserved
	     * set!
	     */
	    start = k;
	    if ((s->data[k] & 0xfc00) == 0xdc00)	/* 2nd surrogate */
		SEE_error_throw_string(interp, interp->URIError,
			STR(bad_utf16_string));
	    if ((s->data[k] & 0xfc00) == 0xd800) {
	        C = (s->data[k++] & 0x3ff) << 10;
		if (k < s->length && (s->data[k] & 0xfc00) == 0xdc00)
		    C = (C | (s->data[k++] & 0x3ff)) + 0x10000;
		else
		    SEE_error_throw_string(interp, interp->URIError,
			STR(bad_utf16_string));
	    } else
	        C = s->data[k++];

	    if (C == '%') {
		/* Next two characters must be hex digits */
		if (k + 1 >= s->length)
		    SEE_error_throw_string(interp, interp->URIError, 
			STR(uri_badhex));
		C = urihexval(interp, s->data[k], s->data[k + 1]);
		k += 2;
		if (C & 0x80) {
		    for (i = 1; i < 6; i++)
		        if ((C & mask[i]) == mask[i - 1])
			    break;
		    if (i >= 6)
			SEE_error_throw_string(interp, interp->URIError, 
			    STR(bad_utf8));
		    C &= ~mask[i];
		    for (j = 0; i--; j++) {
			if (!(k + 2 < s->length && s->data[k] == '%'))
			    SEE_error_throw_string(interp, interp->URIError, 
				STR(bad_utf8));
			D = urihexval(interp, s->data[k + 1], s->data[k + 2]);
			k += 3;
			if ((D & ~0x3f) != 0x80)
			    SEE_error_throw_string(interp, interp->URIError, 
				STR(bad_utf8));
			C = (C << 6) | (D & 0x3f);
		    }
		}
	    }

	    /* Encode into UTF-16 unless it is in the reserved set */
	    if (C < 0x10000) {
		if (C < 0x80 && (resv[(C & 0x7f) >> 3] & (1 << (C & 0x7)))) 
		    while (start < k) {
			SEE_string_addch(R, s->data[start]);
			start++;
		    }
		else
		    SEE_string_addch(R, (SEE_char_t)C);
	    } else if (C < 0x110000) {
		C -= 0x10000;
		SEE_string_addch(R, (SEE_char_t)(0xd800 | (C >> 10 & 0x3ff)));
		SEE_string_addch(R, (SEE_char_t)(0xdc00 | (C & 0x3ff)));
	    } else {
		SEE_error_throw_string(interp, interp->URIError, 
			STR(bad_unicode));
	    }
	}
	return R;
}

/* Global.decodeURI (15.1.3.1) */
static void
global_decodeURI(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;
	struct SEE_string *r;
	static unsigned char uriResrved_plus_hash[] =
	  { 0x00,0x00,0x00,0x00,0x58,0x98,0x00,0xac,    /* [;/?:@&=+$,#] */
	    0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

	if (argc < 1)
		SEE_SET_UNDEFINED(res);
	else {
		SEE_ToString(interp, argv[0], &v);
		r = Decode(interp, v.u.string, uriResrved_plus_hash);
		SEE_SET_STRING(res, r);
	}
}

/* Global.decodeURIComponent (15.1.3.2) */
static void
global_decodeURIComponent(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;
	struct SEE_string *r;
	static unsigned char empty[] =
	  { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,    /* empty */
	    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

	if (argc < 1)
		SEE_SET_UNDEFINED(res);
	else {
		SEE_ToString(interp, argv[0], &v);
		r = Decode(interp, v.u.string, empty);
		SEE_SET_STRING(res, r);
	}
}

/* Global.encodeURI (15.1.3.3) */
static void
global_encodeURI(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;
	struct SEE_string *r;
	static unsigned char uriReserved_plus_uriUnescaped_plus_hash[] =
	    { 0x00,0x00,0x00,0x00,0xda,0xff,0xff,0xaf,    
	      0xff,0xff,0xff,0x87,0xfe,0xff,0xff,0x47 };
	      /* [-_.!~*'();/?:@&=+$,#a-zA-Z0-9] */

	if (argc < 1)
		SEE_SET_UNDEFINED(res);
	else {
		SEE_ToString(interp, argv[0], &v);
		r = Encode(interp, v.u.string, 
		    uriReserved_plus_uriUnescaped_plus_hash);
		SEE_SET_STRING(res, r);
	}
}

/* Global.encodeURIComponent (15.1.3.4) */
static void
global_encodeURIComponent(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;
	struct SEE_string *r;
	static unsigned char uriUnescaped[] =
	    { 0x00,0x00,0x00,0x00,0x82,0x67,0xff,0x03,
	      0xfe,0xff,0xff,0x87,0xfe,0xff,0xff,0x47 };
	      /* [-_.!~*'()a-zA-Z0-9] */

	if (argc < 1)
		SEE_SET_UNDEFINED(res);
	else {
		SEE_ToString(interp, argv[0], &v);
		r = Encode(interp, v.u.string, uriUnescaped);
		SEE_SET_STRING(res, r);
	}
}

/* Global.escape - (B.2.1) */
static void
global_escape(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;
	SEE_char_t c;
	struct SEE_string *s, *r;
	int i;
	static unsigned char ok[] =
	{ 0x00,0x00,0x00,0x00,0x00,0xec,0xff,0x03,    /* [A-Za-z0-9@*_+\-./] */
	  0xff,0xff,0xff,0x87,0xfe,0xff,0xff,0x07, };
	char *hexstr = SEE_COMPAT_JS(interp, >=, JS11)  /* EXT:19 */
		? SEE_hexstr_uppercase : SEE_hexstr_lowercase;

	if (argc < 1) {
		SEE_SET_STRING(res, STR(undefined));
		return;
	}
	SEE_ToString(interp, argv[0], &v);

	s = v.u.string;
	r = SEE_string_new(interp, 0);
	for (i = 0; i < s->length; i++) {
	    c = s->data[i];
	    if (c < 0x80 && (ok[c >> 3] & (1 << (c & 7))))
		SEE_string_addch(r, c);
	    else if (c < 0x100) {
		SEE_string_addch(r, '%');
		SEE_string_addch(r, hexstr[(c >> 4) & 0xf]);
		SEE_string_addch(r, hexstr[c & 0xf]);
	    } else {
		SEE_string_addch(r, '%');
		SEE_string_addch(r, 'u');
		SEE_string_addch(r, hexstr[(c >> 12) & 0xf]);
		SEE_string_addch(r, hexstr[(c >> 8) & 0xf]);
		SEE_string_addch(r, hexstr[(c >> 4) & 0xf]);
		SEE_string_addch(r, hexstr[c & 0xf]);
	    }
	}
	SEE_SET_STRING(res, r);
}

/* Global.unescape - (B.2.2) */
static void
global_unescape(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;
	SEE_char_t c;
	struct SEE_string *s, *r;
	int i;

	if (argc < 1) {
		SEE_SET_STRING(res, STR(undefined));
		return;
	}

	SEE_ToString(interp, argv[0], &v);
	s = v.u.string;
	r = SEE_string_new(interp, 0);
	i = 0;
	while (i < s->length) {
	    c = s->data[i++];
	    if (c == '%' && 
		i + 4 < s->length && 
		s->data[i] == 'u' &&
		ishex(s->data[i + 1]) &&
		ishex(s->data[i + 2]) &&
		ishex(s->data[i + 3]) &&
		ishex(s->data[i + 4]))
	    {
		c = (hexval(s->data[i + 1]) << 12) |
		    (hexval(s->data[i + 2]) <<  8) |
		    (hexval(s->data[i + 3]) <<  4) |
		    (hexval(s->data[i + 4]) <<  0);
		i += 5;
	    } else if (c == '%' &&
		i + 1 < s->length &&
		ishex(s->data[i]) &&
		ishex(s->data[i + 1]))
	    {
		c = (hexval(s->data[i + 0]) << 4) |
		    (hexval(s->data[i + 1]) << 0);
		i += 2;
	    } else {
		/* leave character alone */
	    }
	    SEE_string_addch(r, c);
	}
	SEE_SET_STRING(res, r);
}

#ifndef NDEBUG
/* Global.write(v) */
static void
global_write(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;

	if (argc) {
		SEE_ToString(interp, argv[0], &v);
		SEE_string_fputs(v.u.string, stdout);
	}
	SEE_SET_UNDEFINED(res);
}
#endif

#ifndef NDEBUG
/* Global.writeval(v...) */
static void
global_writeval(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	int i;

	for (i = 0; i < argc; i++) {
		dprintf("argv[%d] = ", i);
		dprintv(interp, argv[i]);
		dprintf("\n");
	}
	SEE_SET_UNDEFINED(res);
}
#endif

/*------------------------------------------------------------
 * Public API to execute something in the Global context.
 * Does not close the input.
 * Returns the last value result of the last statement
 * executed, or undefined if none.
 */

void
SEE_Global_eval(interp, inp, res)
	struct SEE_interpreter *interp;
	struct SEE_input *inp;
	struct SEE_value *res;
{
	struct function *f;
	struct SEE_context context;
	struct SEE_value cres, *v;
	struct SEE_traceback *old_traceback;

	old_traceback = interp->traceback;
	interp->traceback = NULL;

	f = SEE_parse_program(interp, inp);

	context.interpreter = interp;
	context.activation = SEE_Object_new(interp);
	context.scope = interp->Global_scope;
	context.variable = interp->Global;
	context.varattr = SEE_ATTR_DONTDELETE;
	context.thisobj = interp->Global;

	SEE_SET_UNDEFINED(&cres);
	SEE_eval_functionbody(f, &context, &cres);

	if (SEE_VALUE_GET_TYPE(&cres) != SEE_COMPLETION)
		SEE_error_throw_string(interp, interp->Error, 
			STR(internal_error));
	if (cres.u.completion.type != SEE_COMPLETION_NORMAL)
		SEE_error_throw_string(interp, interp->Error, 
			STR(internal_error));

	v = cres.u.completion.value;
	if (res) {
	    if (!v)
		SEE_SET_UNDEFINED(res);
	    else if (SEE_VALUE_GET_TYPE(v) == SEE_REFERENCE) {
		/* GetValue */
		if (v->u.reference.base == NULL)
		    SEE_SET_UNDEFINED(res);
		else
		    SEE_OBJECT_GET(interp, v->u.reference.base, 
			v->u.reference.property, res);
	    } else 
		SEE_VALUE_COPY(res, cres.u.completion.value);
	}

	interp->traceback = old_traceback;
}
