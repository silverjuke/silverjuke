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
/* $Id: obj_String.c 1083 2006-05-07 08:24:13Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <string.h>
#endif

#include "see_mem.h"
#include "see_type.h"
#include "see_value.h"
#include "see_string.h"
#include "see_object.h"
#include "see_native.h"
#include "see_cfunction.h"
#include "see_error.h"
#include "see_interpreter.h"
#include "see_debug.h"

#include "see_stringdefs.h"
#include "see_unicode.h"
#include "see_array.h"
#include "see_regex.h"
#include "see_init.h"
#include "see_nmath.h"

/*
 * The String object.
 * 15.5 
 */

static void string_construct(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void string_call(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void string_fromCharCode(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void string_proto_toString(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, 
	int, struct SEE_value **, struct SEE_value *);
static void string_proto_charAt(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, 
	int, struct SEE_value **, struct SEE_value *);
static void string_proto_charCodeAt(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, 
	int, struct SEE_value **, struct SEE_value *);
static void string_proto_concat(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void string_proto_indexOf(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void string_proto_lastIndexOf(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *,
	int, struct SEE_value **, struct SEE_value *);
static void string_proto_localeCompare(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *,
	int, struct SEE_value **, struct SEE_value *);
static void string_proto_match(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void string_proto_replace(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void string_proto_search(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void string_proto_slice(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void string_proto_split(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void string_proto_substring(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *,
	int, struct SEE_value **, struct SEE_value *);
static void string_proto_substr(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *,
	int, struct SEE_value **, struct SEE_value *);
static void string_proto_toLowerCase(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *,
	int, struct SEE_value **, struct SEE_value *);
static void string_proto_toLocaleLowerCase(struct SEE_interpreter *,
	struct SEE_object *, 
	struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void string_proto_toUpperCase(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *,
	int, struct SEE_value **, struct SEE_value *);
static void string_proto_toLocaleUpperCase(struct SEE_interpreter *,
	struct SEE_object *,
	struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void string_proto__ns_nop(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *,
	int, struct SEE_value **, struct SEE_value *);

static struct SEE_string *object_to_string(struct SEE_interpreter *,
	struct SEE_object *);

/* object class for String constructor */
static struct SEE_objectclass string_const_class = {
	"StringConstructor",		/* Class */
	SEE_native_get,			/* Get */
	SEE_native_put,			/* Put */
	SEE_native_canput,		/* CanPut */
	SEE_native_hasproperty,		/* HasProperty */
	SEE_native_delete,		/* Delete */
	SEE_native_defaultvalue,	/* DefaultValue */
	SEE_native_enumerator,		/* DefaultValue */
	string_construct,		/* Construct */
	string_call			/* Call */
};

/* object class for String.prototype and string instances */
static struct SEE_objectclass string_inst_class = {
	"String",			/* Class */
	SEE_native_get,			/* Get */
	SEE_native_put,			/* Put */
	SEE_native_canput,		/* CanPut */
	SEE_native_hasproperty,		/* HasProperty */
	SEE_native_delete,		/* Delete */
	SEE_native_defaultvalue,	/* DefaultValue */
	SEE_native_enumerator		/* enumerator */
};

/* structure of string instances */
struct string_object {
	struct SEE_native native;
	struct SEE_string *string;	/* Value */
};

void
SEE_String_alloc(interp)
	struct SEE_interpreter *interp;
{
	interp->String = 
	    (struct SEE_object *)SEE_NEW(interp, struct SEE_native);
	interp->String_prototype = 
	    (struct SEE_object *)SEE_NEW(interp, struct string_object);
}

void
SEE_String_init(interp)
	struct SEE_interpreter *interp;
{
	struct SEE_object *String;		/* struct SEE_native */
	struct SEE_object *String_prototype;	/* struct string_object */
	struct SEE_value v;

	String = interp->String;
	SEE_native_init((struct SEE_native *)String, interp,
		&string_const_class, interp->Function_prototype);

	String_prototype = interp->String_prototype;
	SEE_native_init((struct SEE_native *)String_prototype, interp,
		&string_inst_class, interp->Object_prototype); /* 15.5.4 */

	((struct string_object *)String_prototype)->string = 
		STR(empty_string); /* 15.5.4 */

	/* 15.5.3 String.length = 1 */
	SEE_SET_NUMBER(&v, 1);
	SEE_OBJECT_PUT(interp, String, STR(length), &v, SEE_ATTR_LENGTH);

	/* 15.5.3.1 String.prototype */
	SEE_SET_OBJECT(&v, String_prototype);
	SEE_OBJECT_PUT(interp, String, STR(prototype), &v,
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY);

	/* 15.5.5.1 String.prototype.length */
	SEE_SET_NUMBER(&v, 0);
	SEE_OBJECT_PUT(interp, String_prototype, STR(length), &v, 
	    SEE_ATTR_LENGTH);

	/* 15.5.3.2 String.fromCharCode */
	SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, 	
		string_fromCharCode, STR(fromCharCode), 1));
	SEE_OBJECT_PUT(interp, String, STR(fromCharCode), &v, 
		SEE_ATTR_DEFAULT);

	/* 15.5.4.1 String.prototype.constructor */
	SEE_SET_OBJECT(&v, String);
	SEE_OBJECT_PUT(interp, String_prototype, STR(constructor), &v, 
		SEE_ATTR_DEFAULT);

#define PUTFUNC(name, len) \
	SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, 		\
		string_proto_##name, STR(name), len));		\
	SEE_OBJECT_PUT(interp, String_prototype, STR(name), &v,	\
		SEE_ATTR_DEFAULT);

	PUTFUNC(toString, 0)
	SEE_OBJECT_PUT(interp, String_prototype, STR(valueOf), &v, 
		SEE_ATTR_DEFAULT);
					/* NB: .valueOf === .toString */
	PUTFUNC(charAt, 1)
	PUTFUNC(charCodeAt, 1)
	PUTFUNC(concat, 1)
	PUTFUNC(indexOf, 1)
	PUTFUNC(lastIndexOf, 1)
	PUTFUNC(localeCompare, 1)
	PUTFUNC(match, 1)
	PUTFUNC(replace, 1)
	PUTFUNC(search, 1)
	PUTFUNC(slice, 2)
	PUTFUNC(split, 2)
	PUTFUNC(substring, 2)
	PUTFUNC(toLowerCase, 0)
	PUTFUNC(toLocaleLowerCase, 0)
	PUTFUNC(toUpperCase, 0)
	PUTFUNC(toLocaleUpperCase, 0)


	if (interp->compatibility & SEE_COMPAT_262_3B ||
	    SEE_COMPAT_JS(interp, >= ,JS11)) {
	    PUTFUNC(substr, 2)
	} 

	if (SEE_COMPAT_JS(interp, >= ,JS11)) {
#define PUTNOPFUNC(name, len_IGNORED) \
	SEE_OBJECT_PUT(interp, String_prototype, STR(name), &v,	\
		SEE_ATTR_DEFAULT);
	    SEE_SET_OBJECT(&v, SEE_cfunction_make(interp,
		string_proto__ns_nop, STR(_ns_nop), 0));
	    PUTNOPFUNC(anchor, 1)
	    PUTNOPFUNC(big, 0)
	    PUTNOPFUNC(blink, 0)
	    PUTNOPFUNC(bold, 0)
	    PUTNOPFUNC(fixed, 0)
	    PUTNOPFUNC(fontcolor, 1)
	    PUTNOPFUNC(fontsize, 1)
	    PUTNOPFUNC(italics, 0)
	    PUTNOPFUNC(link, 1)
	    PUTNOPFUNC(small, 0)
	    PUTNOPFUNC(strike, 0)
	    PUTNOPFUNC(sub, 0)
	    PUTNOPFUNC(sup, 0)
	}
}

/* 15.5.2.1 */
static void
string_construct(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	struct string_object *so;
	struct SEE_value strv, lenv;

	if (argc == 0)
		SEE_SET_STRING(&strv, STR(empty_string));
	else
		SEE_ToString(interp, argv[0], &strv);

	so = SEE_NEW(interp, struct string_object);
	SEE_native_init(&so->native, interp, &string_inst_class,
		interp->String_prototype);
	so->string = strv.u.string;

	/* 15.5.5.1 length */
	SEE_SET_NUMBER(&lenv, so->string->length);
	SEE_OBJECT_PUT(interp, (struct SEE_object *)so, STR(length), &lenv,
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY);

	SEE_SET_OBJECT(res, (struct SEE_object *)so);
}

/* 15.5.1.1 */
static void
string_call(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	if (argc == 0)
		SEE_SET_STRING(res, STR(empty_string));
	else 
		SEE_ToString(interp, argv[0], res);
}

/* 15.5.3.2 */
static void
string_fromCharCode(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	int i;
	struct SEE_string *s;
	SEE_char_t ch;

	s = SEE_string_new(interp, 0);
	for (i = 0; i < argc; i++) {
		ch = SEE_ToUint16(interp, argv[i]);
		SEE_string_addch(s, ch);
	}
	SEE_SET_STRING(res, s);
}


/* 15.5.4.2 String.prototype.toString() */
/* 15.5.4.3 String.prototype.valueOf() */
static void
string_proto_toString(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct string_object *so;

	if (thisobj->objectclass != &string_inst_class)
		SEE_error_throw_string(interp, interp->TypeError, 
		   STR(not_string));
	so = (struct string_object *)thisobj;
	SEE_SET_STRING(res, so->string);
}

static struct SEE_string *
object_to_string(interp, o)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
{
	struct SEE_value ov, sv;
	if (o == NULL)
		SEE_SET_NULL(&ov);
	else
		SEE_SET_OBJECT(&ov, o);
	SEE_ToString(interp, &ov, &sv);
	SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&sv) == SEE_STRING);
	return sv.u.string;
}

/* 15.5.4.4 String.prototype.charAt() */
static void
string_proto_charAt(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value vi;
	struct SEE_string *s;

	s = object_to_string(interp, thisobj);
	SEE_ToInteger(interp, argv[0], &vi);
	if (SEE_NUMBER_ISFINITE(&vi) && vi.u.number >= 0 &&
		vi.u.number < s->length)
	    SEE_SET_STRING(res, SEE_string_substr(interp, s, 
	    	(unsigned int)vi.u.number, 1));
	else
	    SEE_SET_STRING(res, STR(empty_string));
}

/* 15.5.4.5 String.prototype.charCodeAt() */
static void
string_proto_charCodeAt(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value vi;
	struct SEE_string *s;

	s = object_to_string(interp, thisobj);
	SEE_ToInteger(interp, argv[0], &vi);
	if (SEE_NUMBER_ISFINITE(&vi) && vi.u.number >= 0 &&
		vi.u.number < s->length)
	{
	    unsigned int pos = (unsigned int)vi.u.number;
	    SEE_SET_NUMBER(res, s->data[pos]);
	} else
	    SEE_SET_NUMBER(res, SEE_NaN);
}

/* 15.5.4.6 String.prototype.concat() */
static void
string_proto_concat(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_string *s;
	int i;
	struct SEE_value v;

	s = SEE_string_dup(interp, object_to_string(interp, thisobj));
	for (i = 0; i < argc; i++) {
		SEE_ToString(interp, argv[i], &v);
		SEE_string_append(s, v.u.string);
	}
	SEE_SET_STRING(res, s);
}

/* 15.5.4.7 String.prototype.indexOf() */
static void
string_proto_indexOf(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_string *s;
	struct SEE_value vss, vi;
	int position;
	unsigned int k, sslen, slen;
		
	s = object_to_string(interp, thisobj);
	slen = s->length;

	if (argc < 1)
		SEE_SET_STRING(&vss, STR(undefined));
	else
		SEE_ToString(interp, argv[0], &vss);
	sslen = vss.u.string->length;
	if (argc < 2 || SEE_VALUE_GET_TYPE(argv[1]) == SEE_UNDEFINED)
		position = 0;
	else {
		SEE_ToInteger(interp, argv[1], &vi);
		position = (int)vi.u.number;
	}
	if (position < 0) position = 0;
	if (position > slen) position = slen;
	
	if (slen >= sslen)
	    for (k = (unsigned int)position; k <= slen - sslen; k++)
	        if (memcmp(&s->data[k], vss.u.string->data,
		    sslen * sizeof vss.u.string->data[0]) == 0)
	        {
		    SEE_SET_NUMBER(res, k);
		    return;
	        }
	SEE_SET_NUMBER(res, -1);
}

/* 15.5.4.8 String.prototype.lastIndexOf() */
static void
string_proto_lastIndexOf(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_string *r1s, *r2s;
	struct SEE_value r3v, r2v, r4v;
	int k;
	unsigned int r5, r6, r7;
		
/*1*/	r1s = object_to_string(interp, thisobj);

/*2*/	if (argc > 0)
	    SEE_ToString(interp, argv[0], &r2v);
	else
	    SEE_SET_STRING(&r2v, STR(undefined));
	r2s = r2v.u.string;

/*3*/	if (argc > 1 && SEE_VALUE_GET_TYPE(argv[1]) != SEE_UNDEFINED)
	    SEE_ToNumber(interp, argv[1], &r3v);
	else 
	    SEE_SET_NUMBER(&r3v, SEE_NaN);

/*4*/	if (SEE_ISNAN(r3v.u.number))
	    SEE_SET_NUMBER(&r4v, SEE_Infinity);
	else
	    SEE_ToInteger(interp, &r3v, &r4v);

/*5*/	r5 = r1s->length;

/*6*/	r6 = (unsigned int)MIN(MAX(r4v.u.number, 0), r5);

/*7*/	r7 = r2s->length;

/*8*/
	if (r5 < r7) {
		SEE_SET_NUMBER(res, -1);
		return;
	}

	for (k = MIN(r6, r5 - r7); k >= 0; k--)
	    if (memcmp(r1s->data + k, r2s->data, 
	    	r7 * sizeof (SEE_char_t)) == 0)
	    {
		SEE_SET_NUMBER(res, k);
		return;
	    }
	SEE_SET_NUMBER(res, -1);
}

/* 15.5.4.9 String.prototype.localeCompare() */
static void
string_proto_localeCompare(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	/*
	 * XXX TODO properly:
	 * this function should do some unicode canonicalisation.
	 */

	struct SEE_string *s1, *s2;
	struct SEE_value v2;

	s1 = object_to_string(interp, thisobj);
	if (argc < 1) 
	    SEE_SET_STRING(&v2, STR(undefined));
	else
	    SEE_ToString(interp, argv[1], &v2);
	s2 = v2.u.string;

	SEE_SET_NUMBER(res, SEE_string_cmp(s1, s2));
}

/* Convert an argument into a RegExp if it isn't already */
static struct SEE_object *
regexp_arg(interp, vp)
	struct SEE_interpreter *interp;
	struct SEE_value *vp;
{
	struct SEE_value *args[1], v;

	if (vp == NULL) {
		SEE_OBJECT_CONSTRUCT(interp, interp->RegExp, interp->RegExp,
			0, NULL, &v);
		return v.u.object;
	} else if (SEE_VALUE_GET_TYPE(vp) == SEE_OBJECT &&
	    SEE_is_RegExp(vp->u.object))
		return vp->u.object;
	else {
		args[0] = vp;
		SEE_OBJECT_CONSTRUCT(interp, interp->RegExp, interp->RegExp,
			1, args, &v);
		return v.u.object;
	}
}

/* 15.5.4.10 String.prototype.match() */
static void
string_proto_match(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_object *regexp, *reexec, *a;
	struct SEE_value v, *vp, *vpv[1];
	struct SEE_string *s, *nstr;
	SEE_boolean_t global;
	int n;
	
	regexp = regexp_arg(interp, argc < 1 ? NULL : argv[0]);

	/* fetch the regexp's exec method */
	SEE_OBJECT_GET(interp, regexp, STR(exec), &v);
	SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&v) == SEE_OBJECT);
	reexec = v.u.object;

	/* fetch its global property */
	SEE_OBJECT_GET(interp, regexp, STR(global), &v);
	SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&v) == SEE_BOOLEAN);
	global = v.u.boolean;

	/* s = String(this) */
	s = object_to_string(interp, thisobj);

	if (!global) {
		SEE_SET_STRING(&v, s);
		vp = &v; vpv[0] = vp;
		SEE_OBJECT_CALL(interp, reexec, regexp, 1, vpv, res);
	} else {
		/* regexp.lastIndex = 0 */
		SEE_SET_NUMBER(&v, 0);
		SEE_OBJECT_PUT(interp, regexp, STR(lastIndex), &v, 0);

		/* a = new Array() */
		SEE_OBJECT_CONSTRUCT(interp, interp->Array,
		    interp->Array, 0, NULL, &v);
		a = v.u.object;
		nstr = SEE_string_new(interp, 0);

		for (n = 0; ; n++) {
		    struct SEE_value vres;

		    /* Call regexp.exec(s) -> array of strings */
		    SEE_SET_STRING(&v, s);
		    vp = &v; vpv[0] = vp;
		    SEE_OBJECT_CALL(interp, reexec, regexp, 1, vpv, &vres);
		    if (SEE_VALUE_GET_TYPE(&vres) == SEE_NULL)
			break;
		    SEE_ASSERT(interp, 
		    	SEE_VALUE_GET_TYPE(&vres) == SEE_OBJECT && 
			SEE_is_Array(vres.u.object));
		    SEE_OBJECT_GET(interp, vres.u.object, STR(zero_digit), &v);
		    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&v) == SEE_STRING);

		    /* a[n] = result string */
		    nstr->length = 0;
		    SEE_string_append_int(nstr, n);	/* nstr = String(n) */
		    SEE_OBJECT_PUT(interp, a, nstr, &v, 0);

		    if (v.u.string->length == 0) {
			/* Increment the index by one if it matched empty */
			SEE_OBJECT_GET(interp, regexp, STR(lastIndex), &v);
			SEE_ASSERT(interp, 
				SEE_VALUE_GET_TYPE(&v) == SEE_NUMBER);
			v.u.number += 1;
			SEE_OBJECT_PUT(interp, regexp, STR(lastIndex), &v, 0);
		    }
		}
		SEE_SET_OBJECT(res, a);
	}
}

/* Expand replace using array a, appending to out. Updates *previndexp */
static void
replace_helper(interp, previndexp, out, a, source, replacev, ncaps)
	struct SEE_interpreter *interp;
	unsigned int *previndexp;
	struct SEE_object *a;
	struct SEE_string *out, *source;
	struct SEE_value *replacev;
	int ncaps;
{
	struct SEE_value v, v2;
	int n;
	unsigned int index, i, j, k;
	struct SEE_string *ns = NULL;
	struct SEE_string *ms = NULL;
	struct SEE_string *replace;

	/* get the index of the match string */
	SEE_OBJECT_GET(interp, a, STR(index), &v);
	index = SEE_ToUint32(interp, &v);

	/* get the match string */
	SEE_OBJECT_GET(interp, a, STR(zero_digit), &v);
	SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&v) == SEE_STRING);
	ms = v.u.string;

	/* Copy the intermediate characters we missed */
	for (i = *previndexp; i < index; i++)
	    SEE_string_addch(out, source->data[i]);
	*previndexp = index + ms->length;

	if (SEE_VALUE_GET_TYPE(replacev) == SEE_OBJECT) {
	    struct SEE_value **av, *vp;
	    av = SEE_ALLOCA(interp, struct SEE_value *, ncaps + 2);
	    vp = SEE_ALLOCA(interp, struct SEE_value, ncaps + 2);
	    for (i = 0; i < ncaps + 2; i++)
		av[i] = &vp[i];
	    SEE_OBJECT_GET(interp, a, STR(zero_digit), &vp[0]);
	    for (i = 1; i < ncaps; i++) {
		if (ns == NULL) ns = SEE_string_new(interp, 0);
		ns->length = 0;
		SEE_string_append_int(ns, i);
		SEE_OBJECT_GET(interp, a, ns, &vp[i]);
	    }
	    SEE_SET_NUMBER(&vp[ncaps], index);
	    SEE_SET_STRING(&vp[ncaps + 1], source);
	    SEE_OBJECT_CALL(interp, replacev->u.object, replacev->u.object,
		ncaps + 2, av, &v);
	    SEE_ToString(interp, &v, &v2);
	    SEE_string_append(out, v2.u.string);
	    return;
	}

	SEE_ToString(interp, replacev, &v);
	replace = v.u.string;

	/* Expand the replace text */
	i = 0;
	while (i < replace->length)
	    if (replace->data[i] == '$' && i + 1 < replace->length) {
		i++;

		switch (replace->data[i]) {
		case '$':
		    SEE_string_addch(out, '$');
		    i++;
		    continue;
		case '`':
		    for (k = 0; k < index; k++)
		        SEE_string_addch(out, source->data[k]);
		    i++;
		    continue;
		case '\'':
		    for (k = *previndexp; k < source->length; k++)
		        SEE_string_addch(out, source->data[k]);
		    i++;
		    continue;
		case '&':
		    SEE_string_append(out, ms);
		    i++;
		    continue;
		}
		j = i;
		n = 0;
		while (j < replace->length &&
		    replace->data[j] >= '0' && replace->data[j] <= '9')
			n = n * 10 + replace->data[j++] - '0';
		if (j == i) {
		    /* Didn't see any digits */
		    SEE_string_addch(out, '$');
		    continue;
		}
		/* Build up an index into the capture array  */
		if (ns == NULL) ns = SEE_string_new(interp, 0);
		ns->length = 0;
		SEE_string_append_int(ns, n);

		/* fetch the captured substring */
		SEE_OBJECT_GET(interp, a, ns, &v);
		if (SEE_VALUE_GET_TYPE(&v) != SEE_UNDEFINED) {
		    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&v) == SEE_STRING);
		    SEE_string_append(out, v.u.string);
		}
		i = j;
	    } else {
	        SEE_string_addch(out, replace->data[i]);
	        i++;
	    }
}

/* 15.5.4.11 String.prototype.replace() */
static void
string_proto_replace(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_object *regexp, *reexec;
	struct SEE_value v, *vp, *vpv[1], v2, *replacev, replv;
	struct SEE_string *s, *out = NULL;
	SEE_boolean_t global;
	int ncaps;
	unsigned int previndex = 0;
	
	regexp = regexp_arg(interp, argc < 1 ? NULL : argv[0]);
	ncaps = SEE_RegExp_count_captures(interp, regexp);

	/* Convert the replace arg to a string or callable function */
	if (argc < 2) {
		SEE_SET_STRING(&replv, STR(empty_string));
		replacev = &replv;
	} else if (SEE_VALUE_GET_TYPE(argv[1]) == SEE_OBJECT &&
		   SEE_OBJECT_HAS_CALL(argv[1]->u.object))
		replacev = argv[1];
	else {
		SEE_ToString(interp, argv[1], &replv);
		replacev = &replv;
	}

	/* fetch the regexp's exec method */
	SEE_OBJECT_GET(interp, regexp, STR(exec), &v);
	SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&v) == SEE_OBJECT);
	reexec = v.u.object;

	/* fetch its global property */
	SEE_OBJECT_GET(interp, regexp, STR(global), &v);
	SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&v) == SEE_BOOLEAN);
	global = v.u.boolean;

	/* s = String(this) */
	s = object_to_string(interp, thisobj);

	if (!global) {
		SEE_SET_STRING(&v, s);
		vp = &v; vpv[0] = vp;
		SEE_OBJECT_CALL(interp, reexec, regexp, 1, vpv, &v2);
		if (SEE_VALUE_GET_TYPE(&v2) != SEE_NULL) {
		    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&v2) == SEE_OBJECT
			&& SEE_is_Array(v2.u.object));
		    out = SEE_string_new(interp, 0);
		    replace_helper(interp, &previndex, out, v2.u.object,
			s, replacev, ncaps);
		}
	} else {
		/* regexp.lastIndex = 0 */
		SEE_SET_NUMBER(&v, 0);
		SEE_OBJECT_PUT(interp, regexp, STR(lastIndex), &v, 0);

		for (;;) {
		    struct SEE_value vres;

		    /* Call regexp.exec(s) -> array of strings */
		    SEE_SET_STRING(&v, s);
		    vp = &v; vpv[0] = vp;
		    SEE_OBJECT_CALL(interp, reexec, regexp, 1, vpv, &vres);
		    if (SEE_VALUE_GET_TYPE(&vres) == SEE_NULL)
			break;

		    SEE_ASSERT(interp, 
		    	SEE_VALUE_GET_TYPE(&vres) == SEE_OBJECT && 
			SEE_is_Array(vres.u.object));

		    SEE_OBJECT_GET(interp, vres.u.object, STR(zero_digit), &v);
		    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&v) == SEE_STRING);

		    if (v.u.string->length != 0) {
		        if (out == NULL) 
			    out = SEE_string_new(interp, 0);
		        replace_helper(interp, &previndex, out, vres.u.object,
			    s, replacev, ncaps);
		    } else {
			/* Increment the index by one if it matched empty */
			SEE_OBJECT_GET(interp, regexp, STR(lastIndex), &v);
			SEE_ASSERT(interp, 
				SEE_VALUE_GET_TYPE(&v) == SEE_NUMBER);
			v.u.number += 1;
			SEE_OBJECT_PUT(interp, regexp, STR(lastIndex), &v, 0);
		    }
		}
	}

	if (out)
	    /* Copy rest of source text */
	    while (previndex < s->length)
		SEE_string_addch(out, s->data[previndex++]);
	else
	    out = s;

	SEE_SET_STRING(res, out);
}

/* 15.5.4.12 String.prototype.search() */
static void
string_proto_search(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_string *s;
	struct SEE_object *regexp;
	int ncaps;
	struct capture *captures;
	int index;

	s = object_to_string(interp, thisobj);
	regexp = regexp_arg(interp, argc < 1 ? NULL : argv[0]);
	ncaps = SEE_RegExp_count_captures(interp, regexp);
	captures = SEE_STRING_ALLOCA(interp, struct capture, ncaps);

	/*
	 * As this function is not supposed to touch the regexp object's
	 * lastIndex property , and ignore its global property,
	 * it is a perfect candidate for calling the regex 
	 * engine (nearly) directly.
	 */
	for (index = 0; index < s->length; index++)
	    if (SEE_RegExp_match(interp, regexp, s, index, captures)) {
		SEE_SET_NUMBER(res, captures[0].start);
		return;
	    }
	SEE_SET_NUMBER(res, -1);
}

/* 15.5.4.13 String.prototype.slice() */
static void
string_proto_slice(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_string *s;
	struct SEE_value vs, ve;
	int start, end, len;

	s = object_to_string(interp, thisobj);
	if (argc < 1)
	    SEE_SET_NUMBER(&vs, 0);
	else
	    SEE_ToInteger(interp, argv[0], &vs);
	if (argc < 2 || SEE_VALUE_GET_TYPE(argv[1]) == SEE_UNDEFINED)
	    SEE_SET_NUMBER(&ve, s->length);
	else
	    SEE_ToInteger(interp, argv[1], &ve);

	if (vs.u.number < 0)
		start = MAX(s->length + vs.u.number, 0);
	else
		start = MIN(vs.u.number, s->length);
	if (ve.u.number < 0)
		end = MAX(s->length + ve.u.number, 0);
	else
		end = MIN(ve.u.number, s->length);
	len = MAX(end - start, 0);
	if (len == 0)
	    SEE_SET_STRING(res, STR(empty_string));
	else
	    SEE_SET_STRING(res, SEE_string_substr(interp, s, start, len));
}

/*
 * Helper function for String.prototype.split().
 * spec bug: SplitMatch takes parameters in the order (R,S,q), not (S,q,R).
 */
static int
SplitMatch(interp, R, S, q, captures)
	struct SEE_interpreter *interp;
	struct SEE_value *R;
	struct SEE_string *S;
	int q;
	struct capture *captures;
{
	int r, s, i;

	if (SEE_VALUE_GET_TYPE(R) != SEE_OBJECT) {
	    r = R->u.string->length;
	    s = S->length;
	    if (q + r > s) return 0;
	    for (i = 0; i < r; i++)
		if (S->data[q+i] != R->u.string->data[i])
		    return 0;
	    captures[0].start = q;		 /* NB: ncap == 1 */
	    captures[0].end = q+r;
	    return 1;
	} else 
	    return SEE_RegExp_match(interp, R->u.object, S,
		q, captures);
}

/* 15.5.4.14 String.prototype.split() */
static void
string_proto_split(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v, separatorv, *R;
	struct SEE_object *A;
	struct SEE_string *S, *T;
	SEE_uint32_t lim;
	int p, s, ncap, z, e, q, i;
	struct capture *captures = NULL;

/*1*/	S = object_to_string(interp, thisobj);
/*2*/	SEE_OBJECT_CONSTRUCT(interp, interp->Array, interp->Array, 
		0, NULL, res);
	A = res->u.object;
/*3*/	if (argc < 2 || SEE_VALUE_GET_TYPE(argv[1]) == SEE_UNDEFINED)
	    lim = 0xffffffff;
	else
	    lim = SEE_ToUint32(interp, argv[1]);
/*4*/	s = S->length;
	if (s == 0 && SEE_COMPAT_JS(interp, == ,JS12))
	    return;
/*5*/	p = 0;
/*6*/	if (argc < 1 || SEE_VALUE_GET_TYPE(argv[0]) == SEE_UNDEFINED) {
	    SEE_SET_STRING(&separatorv, STR(undefined));
	    R = &separatorv;
	    ncap = 1;
	} else if (SEE_VALUE_GET_TYPE(argv[0]) == SEE_OBJECT &&
		   SEE_is_RegExp(argv[0]->u.object)) {
	    R = argv[0];
	    ncap = SEE_RegExp_count_captures(interp, R->u.object);
	} else {
	    SEE_ToString(interp, argv[0], &separatorv);
	    if (SEE_COMPAT_JS(interp, == ,JS12) &&
	        separatorv.u.string->length == 1 &&
	        separatorv.u.string->data[0] == ' ')
	    {
	    	struct SEE_value a, *av[1];
		struct SEE_string *wss = SEE_string_new(interp, 3);

		SEE_string_addch(wss, '\\');
		SEE_string_addch(wss, 's');
		SEE_string_addch(wss, '+');
		SEE_SET_STRING(&a, wss);
		av[0] = &a;
		SEE_OBJECT_CONSTRUCT(interp, interp->RegExp, 
			interp->Global, 1, av, &separatorv);
		while (p < s && UNICODE_IS_Zs(S->data[p]))
		    p++;
	    }
	    R = &separatorv;
	    ncap = 1;
	}
	if (ncap)
		captures = SEE_STRING_ALLOCA(interp, struct capture, ncap);
/*7*/	if (lim == 0) return;
/*8*/	if (argc < 1 || (SEE_VALUE_GET_TYPE(argv[0]) == SEE_UNDEFINED &&
		!SEE_GET_JS_COMPAT(interp)))
	    goto step33;
/*9*/	if (s == 0) goto step31;
step10:	q = p;
step11:	if (q == s) goto step28;
/*12*/	z = SplitMatch(interp, R, S, q, captures);
/*13*/	if (!z) goto step26;
/*14*/	e = captures[0].end;
/*15*/	if (e == p) goto step26;
/*16*/	T = SEE_string_substr(interp, S, p, q-p);
/*17*/	SEE_SET_STRING(&v, T); SEE_Array_push(interp, A, &v);
/*18*/	if (SEE_Array_length(interp, A) == lim) return;
/*19*/	p = e;
/*20*/	i = 0;
step21:	if (i == ncap - 1) goto step10;
/*22*/	i++;
/*23*/	if (CAPTURE_IS_UNDEFINED(captures[i]))
	    SEE_SET_UNDEFINED(&v);
	else
	    SEE_SET_STRING(&v, SEE_string_substr(interp, S, captures[i].start,
		captures[i].end - captures[i].start));
	SEE_Array_push(interp, A, &v);
/*24*/	if (SEE_Array_length(interp, A) == lim) return;
/*25*/	goto step21;
step26:	q++;
/*27*/	goto step11;
step28:	T = SEE_string_substr(interp, S, p, s-p);
/*29*/	SEE_SET_STRING(&v, T); SEE_Array_push(interp, A, &v);
/*30*/	return;
step31:	z = SplitMatch(interp, R, S, 0, captures);
/*32*/	if (z) return;
step33:	SEE_SET_STRING(&v, S); SEE_Array_push(interp, A, &v);
/*34*/	return;
}

/* String("A<B>bold</B>and<CODE>coded</CODE>".split(/<(\/)?([^<>]+)>/)) */

/* 15.5.4.15 String.prototype.substring() */
static void
string_proto_substring(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_string *s;
	int a, b, start, end, len;
	struct SEE_value v;

	s = object_to_string(interp, thisobj);
	if (argc < 1)
	    a = 0;
	else {
	    SEE_ToInteger(interp, argv[0], &v);
	    if (SEE_NUMBER_ISNAN(&v))
		a = 0;
	    else
		a = MIN(MAX(v.u.number, 0), s->length);
	}
	if (argc < 2)
	    b = s->length;
	else if (SEE_VALUE_GET_TYPE(argv[1]) == SEE_UNDEFINED)
	    b = s->length;
	else {
	    SEE_ToInteger(interp, argv[1], &v);
	    if (SEE_NUMBER_ISNAN(&v))
		b = 0;
	    else
		b = MIN(MAX(v.u.number, 0), s->length);
	}
	start = MIN(a,b);
	end = MAX(a,b);
	len = end - start;

	if (len == 0)
	    SEE_SET_STRING(res, STR(empty_string));
	else
	    SEE_SET_STRING(res, SEE_string_substr(interp, s, start, len));
}

/* B.2.3 String.prototype.substr() */
static void
string_proto_substr(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_string *s;
	int start, len;
	struct SEE_value v;

	s = object_to_string(interp, thisobj);
	if (argc < 1)
	    SEE_SET_NUMBER(&v, 0);
	else
	    SEE_ToInteger(interp, argv[0], &v);
	if (v.u.number < 0)
		start = MAX(s->length + v.u.number, 0);
	else
		start = MIN(v.u.number, s->length);
	if (argc < 2 || SEE_VALUE_GET_TYPE(argv[1]) == SEE_UNDEFINED)
	    len = s->length - start;
	else {
	    SEE_ToInteger(interp, argv[1], &v);
	    len = MIN(v.u.number, s->length - start);
	}

	if (len == 0)
	    SEE_SET_STRING(res, STR(empty_string));
	else
	    SEE_SET_STRING(res, SEE_string_substr(interp, s, start, len));
}

/* 15.5.4.16 String.prototype.toLowerCase() */
static void
string_proto_toLowerCase(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_string *s, *rs;
	int i;

	s = object_to_string(interp, thisobj);
	if (s->length == 0) {
	    SEE_SET_STRING(res, STR(empty_string));
	    return;
	}

	rs = SEE_string_new(interp, s->length);
	for (i = 0; i < s->length; i++) {
	    SEE_char_t c = UNICODE_TOLOWER(s->data[i]);
	    SEE_string_addch(rs, c);
	}
	SEE_SET_STRING(res, rs);
}

/* 15.5.4.17 String.prototype.toLocaleLowerCase() */
static void
string_proto_toLocaleLowerCase(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	/* XXX TODO properly */
	string_proto_toLowerCase(interp, self, thisobj, 
		argc, argv, res);
}

/* 15.5.4.18 String.prototype.toUpperCase() */
static void
string_proto_toUpperCase(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_string *s, *rs;
	int i;

	s = object_to_string(interp, thisobj);
	if (s->length == 0) {
	    SEE_SET_STRING(res, STR(empty_string));
	    return;
	}

	rs = SEE_string_new(interp, s->length);
	for (i = 0; i < s->length; i++) {
	    SEE_char_t c = UNICODE_TOUPPER(s->data[i]);
	    SEE_string_addch(rs, c);
	}
	SEE_SET_STRING(res, rs);
}

/* 15.5.4.19 String.prototype.toLocaleUpperCase() */
static void
string_proto_toLocaleUpperCase(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	/* XXX TODO properly */
	string_proto_toUpperCase(interp, self, thisobj, argc, argv, res);
}

static void
string_proto__ns_nop(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	string_proto_toString(interp, NULL, thisobj, 0, NULL, res);
}
