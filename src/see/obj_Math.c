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
/* $Id: obj_Math.c 936 2006-01-21 03:51:30Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdlib.h>
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
#include "see_init.h"
#include "see_nmath.h"

/*
 * 15.8 The Math object.
 */

#define SET_NO_RESULT(res)	SEE_SET_NUMBER(res, SEE_NaN)
#define IS_NEGZERO(n)		((n) == 0 && NUMBER_copysign(1.0,n) < 0)
#define IS_POSZERO(n)		((n) == 0 && NUMBER_copysign(1.0,n) > 0)

static void math_abs(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_acos(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_asin(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_atan(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_atan2(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_ceil(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_cos(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_exp(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_floor(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_log(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_max(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_min(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_pow(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_random(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_round(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_sin(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_sqrt(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void math_tan(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

/* Math is a normal native object */
static struct SEE_objectclass math_class = {
	"Math",				/* Class */
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
SEE_Math_alloc(interp)
	struct SEE_interpreter *interp;
{
	interp->Math = 
	    (struct SEE_object *)SEE_NEW(interp, struct SEE_native);
}

void
SEE_Math_init(interp)
	struct SEE_interpreter *interp;
{
	struct SEE_object *Math;		/* struct SEE_native */
	struct SEE_value v;

	Math = interp->Math;
	SEE_native_init((struct SEE_native *)Math, interp,
		&math_class, interp->Object_prototype);

#define PUTVAL(name, val) 						\
	SEE_SET_NUMBER(&v, val); 					\
	SEE_OBJECT_PUT(interp, Math, STR(name), &v,			\
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE | 		\
		SEE_ATTR_READONLY);

	PUTVAL(E, M_E)				/* 15.8.1.1 */
	PUTVAL(LN10, M_LN10)			/* 15.8.1.2 */
	PUTVAL(LN2, M_LN2)			/* 15.8.1.3 */
	PUTVAL(LOG2E, M_LOG2E)			/* 15.8.1.4 */
	PUTVAL(LOG10E, M_LOG10E)		/* 15.8.1.5 */
	PUTVAL(PI, M_PI)			/* 15.8.1.6 */
	PUTVAL(SQRT1_2, M_SQRT1_2)		/* 15.8.1.7 */
	PUTVAL(SQRT2, M_SQRT2)			/* 15.8.1.8 */

#define PUTFUNC(name, len) 						\
	SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, math_##name, 	\
		STR(name), len));					\
	SEE_OBJECT_PUT(interp, Math, STR(name), &v, 			\
		SEE_ATTR_DEFAULT);

	PUTFUNC(abs, 1)				/* 15.8.2.1 */
	PUTFUNC(acos, 1)			/* 15.8.2.2 */
	PUTFUNC(asin, 1)			/* 15.8.2.3 */
	PUTFUNC(atan, 1)			/* 15.8.2.4 */
	PUTFUNC(atan2, 2)			/* 15.8.2.5 */
	PUTFUNC(ceil, 1)			/* 15.8.2.6 */
	PUTFUNC(cos, 1)				/* 15.8.2.7 */
	PUTFUNC(exp, 1)				/* 15.8.2.8 */
	PUTFUNC(floor, 1)			/* 15.8.2.9 */
	PUTFUNC(log, 1)				/* 15.8.2.10 */
	PUTFUNC(max, 2)				/* 15.8.2.11 */
	PUTFUNC(min, 2)				/* 15.8.2.12 */
	PUTFUNC(pow, 2)				/* 15.8.2.13 */
	PUTFUNC(random, 0)			/* 15.8.2.14 */
	PUTFUNC(round, 1)			/* 15.8.2.15 */
	PUTFUNC(sin, 1)				/* 15.8.2.16 */
	PUTFUNC(sqrt, 1)			/* 15.8.2.17 */
	PUTFUNC(tan, 1)				/* 15.8.2.18 */
}

/* 15.8.2.1 Math.abs() */
static void
math_abs(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	if (argc == 0)
		SET_NO_RESULT(res);
	else {
		SEE_ToNumber(interp, argv[0], res);
		if (!SEE_NUMBER_ISNAN(res))
			res->u.number = NUMBER_copysign(res->u.number, 1.0);
	}
}

/* 15.8.2.2 Math.acos() */
static void
math_acos(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	if (argc == 0)
		SET_NO_RESULT(res);
	else {
		SEE_ToNumber(interp, argv[0], res);
		if (SEE_NUMBER_ISNAN(res))
			;
		else if (res->u.number < -1 || res->u.number > 1)
			SEE_SET_NUMBER(res, SEE_NaN);
		else if (res->u.number == 1)
			SEE_SET_NUMBER(res, 0);
		else
			SEE_SET_NUMBER(res, NUMBER_acos(res->u.number));
	}
}


/* 15.8.2.3 Math.asin() */
static void
math_asin(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	if (argc == 0)
		SET_NO_RESULT(res);
	else {
		SEE_ToNumber(interp, argv[0], res);
		if (SEE_NUMBER_ISNAN(res))
			;
		else if (res->u.number < -1 || res->u.number > 1)
			SEE_SET_NUMBER(res, SEE_NaN);
		else if (res->u.number == 0)
			;
		else 
			SEE_SET_NUMBER(res, NUMBER_asin(res->u.number));
	}
}

/* 15.8.2.4 Math.atan() */
static void
math_atan(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;

	if (argc == 0)
		SET_NO_RESULT(res);
	else {
		SEE_ToNumber(interp, argv[0], &v);
		if (v.u.number == 0)
			SEE_SET_NUMBER(res, v.u.number);
		else
			SEE_SET_NUMBER(res, NUMBER_atan(v.u.number));
	}
}

/* 15.8.2.5 Math.atan2() */
static void
math_atan2(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v1, v2;

	if (argc < 2)
		SET_NO_RESULT(res);
	else {
		SEE_number_t x, y;
		SEE_ToNumber(interp, argv[0], &v1);
		SEE_ToNumber(interp, argv[1], &v2);

		y = v1.u.number;
		x = v2.u.number;

		/*
		 * XXX: on my system, atan2() only differs in
		 * the case where x is -0
		 */
		if (IS_POSZERO(y) && IS_NEGZERO(x))
			SEE_SET_NUMBER(res, M_PI);
		else if (IS_NEGZERO(y) && IS_NEGZERO(x))
			SEE_SET_NUMBER(res, -M_PI);
		else
			SEE_SET_NUMBER(res, NUMBER_atan2(y, x));
	}
}

/* 15.8.2.6 Math.ceil() */
static void
math_ceil(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;

	if (argc == 0)
		SET_NO_RESULT(res);
	else {
		SEE_ToNumber(interp, argv[0], &v);
		SEE_SET_NUMBER(res, NUMBER_ceil(v.u.number));
	}
}

/* 15.8.2.7 Math.cos() */
static void
math_cos(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;

	if (argc == 0)
		SET_NO_RESULT(res);
	else {
		SEE_ToNumber(interp, argv[0], &v);
		SEE_SET_NUMBER(res, NUMBER_cos(v.u.number));
	}
}

/* 15.8.2.8 Math.exp() */
static void
math_exp(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;

	if (argc == 0)
		SET_NO_RESULT(res);
	else {
		SEE_ToNumber(interp, argv[0], &v);
		if (SEE_NUMBER_ISINF(&v))
			SEE_SET_NUMBER(res, v.u.number < 0 ? 0 : SEE_Infinity);
		else
			SEE_SET_NUMBER(res, NUMBER_exp(v.u.number));
	}
}

/* 15.8.2.9 Math.floor() */
static void
math_floor(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;

	if (argc == 0)
		SET_NO_RESULT(res);
	else {
		SEE_ToNumber(interp, argv[0], &v);
		SEE_SET_NUMBER(res, NUMBER_floor(v.u.number));
	}
}

/* 15.8.2.10 Math.log() */
static void
math_log(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;

	if (argc == 0)
		SET_NO_RESULT(res);
	else {
		SEE_ToNumber(interp, argv[0], &v);
		if (v.u.number < 0)
		    SEE_SET_NUMBER(res, SEE_NaN);
		else
		    SEE_SET_NUMBER(res, NUMBER_log(v.u.number));
	}
}

/* 15.8.2.11 Math.max() */
static void
math_max(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	SEE_number_t maxnum = -SEE_Infinity;
	int i;

	for (i = 0; i < argc; i++) {
	    SEE_ToNumber(interp, argv[i], res);
	    if (SEE_NUMBER_ISNAN(res))
		return;
	    if (i == 0 || res->u.number > maxnum ||
	        (res->u.number == 0 && IS_NEGZERO(maxnum)))
		maxnum = res->u.number;
	}
	SEE_SET_NUMBER(res, maxnum);
}

/* 15.8.2.12 Math.min() */
static void
math_min(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	SEE_number_t minnum = SEE_Infinity;
	int i;

	for (i = 0; i < argc; i++) {
	    SEE_ToNumber(interp, argv[i], res);
	    if (SEE_NUMBER_ISNAN(res))
		return;
	    if (i == 0 || res->u.number < minnum ||
	        (minnum == 0 && IS_NEGZERO(res->u.number)))
		minnum = res->u.number;
	}
	SEE_SET_NUMBER(res, minnum);
}

/* 15.8.2.13 Math.pow() */
static void
math_pow(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v1, v2;

	if (argc < 2)
		SET_NO_RESULT(res);
	else {
		SEE_ToNumber(interp, argv[0], &v1);
		SEE_ToNumber(interp, argv[1], &v2);

		if (IS_NEGZERO(v1.u.number) && v2.u.number < 0) 
			SEE_SET_NUMBER(res, NUMBER_copysign(
			    NUMBER_fmod(v2.u.number, 2.0), 1.0) == 1 
				? -SEE_Infinity : SEE_Infinity); 
		else if (v1.u.number == 0 && v2.u.number < 0) 
			SEE_SET_NUMBER(res, 
				NUMBER_copysign(SEE_Infinity,v1.u.number));
		else
			SEE_SET_NUMBER(res, 
				NUMBER_pow(v1.u.number, v2.u.number));
	}
}

/* 15.8.2.14 Math.random() */
static void
math_random(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	unsigned int rval;

#if HAVE_RAND_R
	rval = rand_r(&interp->random_seed);
#else
	/* XXX this is not thread safe */
	static int srand_initialised = 0;
	if (!srand_initialised) {
		srand_initialised++;
		srand(interp->random_seed);
	}
	rval = rand();
#endif
	SEE_SET_NUMBER(res, (SEE_number_t)rval / RAND_MAX);
}

/* 15.8.2.15 Math.round() */
static void
math_round(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;
	SEE_number_t x;

	if (argc == 0)
		SET_NO_RESULT(res);
	else {
		SEE_ToNumber(interp, argv[0], &v);
		x = v.u.number;
		if (IS_NEGZERO(x) || (x >= -0.5 && x < 0))
		    SEE_SET_NUMBER(res, -0.0);
		else
		    SEE_SET_NUMBER(res, NUMBER_floor(v.u.number + 0.5));
	}
}

/* 15.8.2.16 Math.sin() */
static void
math_sin(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;

	if (argc == 0)
		SET_NO_RESULT(res);
	else {
		SEE_ToNumber(interp, argv[0], &v);
		SEE_SET_NUMBER(res, NUMBER_sin(v.u.number));
	}
}

/* 15.8.2.17 Math.sqrt() */
static void
math_sqrt(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;

	if (argc == 0)
		SET_NO_RESULT(res);
	else {
		SEE_ToNumber(interp, argv[0], &v);
		SEE_SET_NUMBER(res, NUMBER_sqrt(v.u.number));
	}
}

/* 15.8.2.18 Math.tan() */
static void
math_tan(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;

	if (argc == 0)
		SET_NO_RESULT(res);
	else {
		SEE_ToNumber(interp, argv[0], &v);
		SEE_SET_NUMBER(res, NUMBER_tan(v.u.number));
	}
}
