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
/* $Id: obj_Number.c 1125 2006-08-03 14:28:56Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "see_mem.h"
#include "see_value.h"
#include "see_string.h"
#include "see_object.h"
#include "see_native.h"
#include "see_cfunction.h"
#include "see_error.h"
#include "see_interpreter.h"

#include "see_stringdefs.h"
#include "see_dtoa.h"
#include "see_init.h"
#include "see_nmath.h"
#include "see_array.h"

/*
 * 15.7 The Number object.
 */

/* structure of number instances */
struct number_object {
	struct SEE_native native;
	SEE_number_t number;		/* Value */
};

/* Prototypes */
static void radix_tostring(struct SEE_string *, SEE_number_t, int);
static struct number_object *tonumber(struct SEE_interpreter *, 
        struct SEE_object *);

static void number_construct(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void number_call(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void number_proto_toString(struct SEE_interpreter *, 
        struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
        struct SEE_value *);
static void number_proto_toLocaleString(struct SEE_interpreter *, 
        struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
        struct SEE_value *);
static void number_proto_valueOf(struct SEE_interpreter *, 
        struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
        struct SEE_value *);
static void number_proto_toFixed(struct SEE_interpreter *, 
        struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
        struct SEE_value *);
static void number_proto_toExponential(struct SEE_interpreter *, 
        struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
        struct SEE_value *);
static void number_proto_toPrecision(struct SEE_interpreter *, 
        struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
        struct SEE_value *);

/* object class for Number constructor */
static struct SEE_objectclass number_const_class = {
	"NumberConstructor",		/* Class */
	SEE_native_get,			/* Get */
	SEE_native_put,			/* Put */
	SEE_native_canput,		/* CanPut */
	SEE_native_hasproperty,		/* HasProperty */
	SEE_native_delete,		/* Delete */
	SEE_native_defaultvalue,	/* DefaultValue */
	SEE_native_enumerator,		/* DefaultValue */
	number_construct,		/* Construct */
	number_call			/* Call */
};

/* object class for Number.prototype and number instances */
static struct SEE_objectclass number_inst_class = {
	"Number",			/* Class */
	SEE_native_get,			/* Get */
	SEE_native_put,			/* Put */
	SEE_native_canput,		/* CanPut */
	SEE_native_hasproperty,		/* HasProperty */
	SEE_native_delete,		/* Delete */
	SEE_native_defaultvalue,	/* DefaultValue */
	SEE_native_enumerator		/* enumerator */
};

void
SEE_Number_alloc(interp)
	struct SEE_interpreter *interp;
{
	interp->Number = 
	    (struct SEE_object *)SEE_NEW(interp, struct SEE_native);
	interp->Number_prototype = 
	    (struct SEE_object *)SEE_NEW(interp, struct number_object);
}

void
SEE_Number_init(interp)
	struct SEE_interpreter *interp;
{
	struct SEE_object *Number;		/* struct SEE_native */
	struct SEE_object *Number_prototype;	/* struct number_object */
	struct SEE_value v;

	Number = interp->Number;
	SEE_native_init((struct SEE_native *)Number, interp,
		&number_const_class, interp->Function_prototype);

	Number_prototype = interp->Number_prototype;


	/* 15.7.3 Number.length = 1 */
	SEE_SET_NUMBER(&v, 1);
	SEE_OBJECT_PUT(interp, Number, STR(length), &v, SEE_ATTR_LENGTH);

	/* 15.7.3.1 Number.prototype */
	SEE_SET_OBJECT(&v, Number_prototype);
	SEE_OBJECT_PUT(interp, Number, STR(prototype), &v,
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY);

	/* 15.7.3.2 Number.MAX_VALUE */
	SEE_SET_NUMBER(&v, SEE_MaxNumber);
	SEE_OBJECT_PUT(interp, Number, STR(MAX_VALUE), &v,
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY);

	/* 15.7.3.3 Number.MIN_VALUE */
	SEE_SET_NUMBER(&v, SEE_MinNumber);
	SEE_OBJECT_PUT(interp, Number, STR(MIN_VALUE), &v,
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY);

	/* 15.7.3.4 Number.NaN */
	SEE_SET_NUMBER(&v, SEE_NaN);
	SEE_OBJECT_PUT(interp, Number, STR(NaN), &v,
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY);

	/* 15.7.3.5 Number.NEGATIVE_INFINITY */
	SEE_SET_NUMBER(&v, -SEE_Infinity);
	SEE_OBJECT_PUT(interp, Number, STR(NEGATIVE_INFINITY), &v,
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY);

	/* 15.7.3.6 Number.POSITIVE_INFINITY */
	SEE_SET_NUMBER(&v, SEE_Infinity);
	SEE_OBJECT_PUT(interp, Number, STR(POSITIVE_INFINITY), &v,
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY);

	SEE_native_init((struct SEE_native *)Number_prototype, interp,
		&number_inst_class, interp->Object_prototype); /* 15.7.4 */
	((struct number_object *)Number_prototype)->number = 0; /* 15.7.4 */

	/* 15.7.4.1 Number.prototype.constructor */
	SEE_SET_OBJECT(&v, Number);
	SEE_OBJECT_PUT(interp, Number_prototype, STR(constructor), &v, 
		SEE_ATTR_DEFAULT);

#define PUTFUNC(name, len) \
	SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, 		\
		number_proto_##name, STR(name), len));		\
	SEE_OBJECT_PUT(interp, Number_prototype, STR(name), &v,	\
		SEE_ATTR_DEFAULT);

	PUTFUNC(toString, 1)
	PUTFUNC(toLocaleString, 0)
	PUTFUNC(valueOf, 0)
	PUTFUNC(toFixed, 1)
	PUTFUNC(toExponential, 1)
	PUTFUNC(toPrecision, 1)
}

static struct number_object *
tonumber(interp, o)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
{
	if (o->objectclass != &number_inst_class)
		SEE_error_throw_string(interp, interp->TypeError, 
		    STR(not_number));
	return (struct number_object *)o;
}

/* 15.7.2.1 */
static void
number_construct(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	struct number_object *no;
	struct SEE_value v;

	if (argc == 0)
		SEE_SET_NUMBER(&v, 0);
	else
		SEE_ToNumber(interp, argv[0], &v);

	no = SEE_NEW(interp, struct number_object);
	SEE_native_init(&no->native, interp, &number_inst_class,
		interp->Number_prototype);
	no->number = v.u.number;

	SEE_SET_OBJECT(res, (struct SEE_object *)no);
}

/* 15.7.1.1 */
static void
number_call(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	if (argc < 1)
		SEE_SET_NUMBER(res, 0);
	else if (SEE_COMPAT_JS(interp, ==, JS12) &&
			SEE_VALUE_GET_TYPE(argv[0]) == SEE_OBJECT &&
			SEE_is_Array(argv[0]->u.object))
		SEE_SET_NUMBER(res, SEE_Array_length(interp,
			argv[0]->u.object));
	else 
		SEE_ToNumber(interp, argv[0], res);
}

static void
radix_tostring(s, n, radix)
	struct SEE_string *s;
	SEE_number_t n;
	int radix;
{
	int d;

	if (n >= radix) {
		radix_tostring(s, n / radix, radix);
		n = fmod(n, (SEE_number_t)radix);
	}
	d = floor(n);
	if (d < 10) SEE_string_addch(s, '0' + d);
	else        SEE_string_addch(s, 'a' + d - 10);
}
	

/* 15.7.4.2 Number.prototype.toString() */
static void
number_proto_toString(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct number_object *no;
	SEE_int32_t radix;
	struct SEE_value v;

	no = tonumber(interp, thisobj);

	if (argc == 0)
		radix = 10;
	else 
		radix = SEE_ToInt32(interp, argv[0]);

	if (radix == 10) {
		SEE_SET_NUMBER(&v, no->number);
		SEE_ToString(interp, &v, res);
	} else if (radix >= 2 && radix <= 36) {
		/*
		 * The specs say to do something "implementation dependent",
		 * but here I try and convert the number into the desired
		 * representation. Numbers bigger than 10^20 or smaller than
		 * 10^-6 are printed in p-notation (with the exponent being
		 * expressed in base 10)
		 */
		struct SEE_string *s;
		SEE_number_t n = no->number, ni, nf;
		int expon;

		if (SEE_ISNAN(n)) {
			SEE_SET_STRING(res, STR(NaN));
			return;
		}
		if (n == 0) {
			SEE_SET_STRING(res, STR(zero_digit)); 	/* "0" */
			return;
		}
		s = SEE_string_new(interp, 0);
		if (n < 0) {
			SEE_string_addch(s, '-');
			n = -n;
		}
		if (!SEE_ISFINITE(n)) {
			SEE_string_append(s, STR(Infinity));
			SEE_SET_STRING(res, s);
			return;
		}
		if (n > 1e20 || n < 1e-6) {
			expon = NUMBER_floor(NUMBER_log(n) / 
				NUMBER_log((SEE_number_t)radix));
			n /= NUMBER_pow((SEE_number_t)radix, 
				(SEE_number_t)expon);
			if (n == 0) {
			    /* e.g.: Number(MAX_VALUE).toString(16) */
			    SEE_string_append(s, STR(Infinity));
			    SEE_SET_STRING(res, s);
			    return;
			}
			if (!SEE_ISFINITE(n)) {
			    /* e.g.: Number(MIN_VALUE).toString(16) */
			    SEE_SET_STRING(res, STR(zero_digit));
			    return;
			}
		} else
			expon = 0;
		ni = floor(n);
		nf = n - ni;
		radix_tostring(s, ni, radix);
		if (nf > 0) {
			int i;
#define MAXPREC 20
			SEE_string_addch(s, '.');
			for (i = 0; i < MAXPREC && nf; i++) {
			    SEE_number_t d;
			    nf *= radix;
			    if (i == MAXPREC - 1) {
#if HAVE_RINT
				    d = rint(nf);
#else /* !HAVE_RINT */
				    d = floor(nf + 0.5);
#endif /* !HAVE_RINT */
			    } else {
				    d = floor(nf);
				    nf -= d;
			    }
			    if (d < 10) SEE_string_addch(s, '0' + (int)d);
			    else        SEE_string_addch(s, 'a' + (int)d - 10);
			};
		}
		if (expon) {
			SEE_string_addch(s, 'p');
			if (expon < 0) {
				expon = -expon;
				SEE_string_addch(s, '-');
			}
			radix_tostring(s, (SEE_number_t)expon, 10);
		}

		SEE_SET_STRING(res, s);
	} else
		SEE_error_throw_string(interp, interp->RangeError, 
		    STR(bad_radix));
}

/* 15.7.4.3 Number.prototype.toLocaleString() */
static void
number_proto_toLocaleString(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct number_object *no;

	no = tonumber(interp, thisobj);

	/* XXX - really should use localeconv() */
	number_proto_toString(interp, self, thisobj, 0, NULL, res);
}

/* 15.7.4.4 Number.prototype.valueOf() */
static void
number_proto_valueOf(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct number_object *no;

	no = tonumber(interp, thisobj);
	SEE_SET_NUMBER(res, no->number);
}

/* 15.7.4.5 Number.prototype.toFixed() */
static void
number_proto_toFixed(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct number_object *no;
	struct SEE_value v;
	struct SEE_string *m;
	SEE_number_t x;
	char *ms, *endstr;
	int f, sign, n, i, k;

	if (argc > 0 && SEE_VALUE_GET_TYPE(argv[0]) != SEE_UNDEFINED) {
	    SEE_ToInteger(interp, argv[0], &v);
	    if (v.u.number < 0 || v.u.number > 20 || SEE_NUMBER_ISNAN(&v))
		SEE_error_throw(interp, interp->RangeError, "%f", v.u.number);
	    f = v.u.number;
	} else
	    f = 0;

	no = tonumber(interp, thisobj);
	x = no->number;
	if (!SEE_ISFINITE(x) || x <= -1e21 || x >= 1e21) {
	    SEE_SET_NUMBER(&v, x);
	    SEE_ToString(interp, &v, res);
	    return;
	}

	ms = SEE_dtoa(x, DTOA_MODE_FCVT, f, &n, &sign, &endstr);
        k = endstr - ms;

        m = SEE_string_new(interp, 0);
	if (x < 0)
	    SEE_string_addch(m, '-');
	if (n <= 0)
	    SEE_string_addch(m, '0');
	if (n < 0) {
	    SEE_string_addch(m, '.');
	    for (i = 0; i < -n; i++)
	        SEE_string_addch(m, '0');
	}
	for (i = 0; i < k; i++) {
	    if (i == n)
	        SEE_string_addch(m, '.');
	    SEE_string_addch(m, ms[i]);
	}
	for (i = k; i < f + n; i++) {
	    if (i == n)
	        SEE_string_addch(m, '.');
	    SEE_string_addch(m, '0');
	}
	SEE_freedtoa(ms);
	SEE_SET_STRING(res, m);
}

/* 15.7.4.6 Number.prototype.toExponential() */
static void
number_proto_toExponential(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct number_object *no;
	struct SEE_value v;
	struct SEE_string *s;
	SEE_number_t x;
	char *ms, *endstr;
	int e, f, n, i, k, sign;

	if (argc > 0 && SEE_VALUE_GET_TYPE(argv[0]) != SEE_UNDEFINED) {
	    SEE_ToInteger(interp, argv[0], &v);
	    if (v.u.number < 0 || v.u.number > 20 || SEE_NUMBER_ISNAN(&v))
		SEE_error_throw(interp, interp->RangeError, "%f", v.u.number);
	    f = v.u.number;
	} else
	    f = 0;

	no = tonumber(interp, thisobj);
	x = no->number;
	SEE_SET_NUMBER(&v, x);
	if (!SEE_NUMBER_ISFINITE(&v)) {
	    SEE_ToString(interp, &v, res);
	    return;
	}

	if (f)
		ms = SEE_dtoa(x, DTOA_MODE_ECVT, f, &n, &sign, &endstr);
	else
		ms = SEE_dtoa(x, DTOA_MODE_SHORT_SW, 31, &n, &sign, &endstr);
        k = endstr - ms;
	if (x)
	    e = n - 1;
	else
	    e = 0;

	s = SEE_string_new(interp, 0);
	if (x < 0)
	    SEE_string_addch(s, '-');
	if (k == 0)
	    SEE_string_addch(s, '0');
	else
	    SEE_string_addch(s, ms[0]);
	if (k > 1 || f) {
	    SEE_string_addch(s, '.');
	    for (i = 1; i < k; i++) 
		SEE_string_addch(s, ms[i]);
	    for (; i < f + 1; i++)
		SEE_string_addch(s, '0');
	}
	SEE_string_addch(s, 'e');
	if (e >= 0)
	    SEE_string_addch(s, '+');
	SEE_string_append_int(s, e);
	SEE_freedtoa(ms);
	SEE_SET_STRING(res, s);
}

/* 15.7.4.7 Number.prototype.toPrecision() */
static void
number_proto_toPrecision(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct number_object *no;
	struct SEE_value v;
	struct SEE_string *s;
	SEE_number_t x;
	char *ms, *endstr;
	int p, n, k, e, i, sign;

	no = tonumber(interp, thisobj);
	x = no->number;

	SEE_SET_NUMBER(&v, x);
	if (argc < 1 || 
		SEE_VALUE_GET_TYPE(argv[0]) == SEE_UNDEFINED || 
		!SEE_NUMBER_ISFINITE(&v))
	{
	    SEE_ToString(interp, &v, res);
	    return;
	}

	SEE_ToInteger(interp, argv[0], &v);
	if (v.u.number < 1 || v.u.number > 21 || SEE_NUMBER_ISNAN(&v))
	    SEE_error_throw(interp, interp->RangeError, "%f", v.u.number);
	p = v.u.number;

	s = SEE_string_new(interp, 0);
	if (x < 0)
	    SEE_string_addch(s, '-');
	ms = SEE_dtoa(x, DTOA_MODE_ECVT, p, &n, &sign, &endstr);
	k = endstr - ms;
	if (x)
	    e = n - 1;
	else {
	    e = 0;
	    goto fixed;
	}
	if (e < -6 || e >= p) {
	    if (k == 0)
		SEE_string_addch(s, '0');
	    else
		SEE_string_addch(s, ms[0]);
	    if (p > 1) {
		SEE_string_addch(s, '.');
		for (i = 1; i < k; i++)
		    SEE_string_addch(s, ms[i]);
		for (; i < p - 1; i++)
		    SEE_string_addch(s, '0');
	    }
	    SEE_string_addch(s, 'e');
	    if (e >= 0)
		SEE_string_addch(s, '+');
	    SEE_string_append_int(s, e);
	} else {
    fixed:
	    if (n <= 0)
		SEE_string_addch(s, '0');
	    if (n < 0) {
		SEE_string_addch(s, '.');
		for (i = 0; i < -n; i++)
		    SEE_string_addch(s, '0');
	    }
	    for (i = 0; i < k; i++) {
		if (i == n)
		    SEE_string_addch(s, '.');
		SEE_string_addch(s, ms[i]);
	    }
	    for (i = k; i < p; i++) {
		if (i == n)
		    SEE_string_addch(s, '.');
		SEE_string_addch(s, '0');
	    }
	}
	SEE_freedtoa(ms);
	SEE_SET_STRING(res, s);
}
