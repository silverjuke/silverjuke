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
/* $Id: obj_Date.c 1078 2006-05-07 04:43:54Z d $ */

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

#include "see_stringdefs.h"
#include "see_init.h"
#include "see_dprint.h"
#include "see_nmath.h"
#include "see_platform.h"

/*
 * 15.9 The Date object.
 */

/* structure of date instances */
struct date_object {
	struct SEE_native native;
	SEE_number_t t;		/* time value with 53 bit precision */
};

#define SGN(x)  ((x) < 0 ? -1 : 1)
#define ABS(x)  ((x) < 0 ? -(x) : (x))

/* The number of milliseconds from midnight 1 Jan 0000 to 1 Jan 1970 */
#define msPerY1			(365 * msPerDay)
#define msPerY4			(4 * msPerY1 + msPerDay)
#define msPerY100		(25 * msPerY4 - msPerDay)
#define msPerY400		(4 * msPerY100 + msPerDay)

/* 1970 = 1600+300+68+2 */
#define T1970			(4 * msPerY400 +		\
				 3 * msPerY100 +		\
				 17 * msPerY4 +			\
				 2 * msPerY1)

#define maxTime			(msPerDay * 100000000)
#define minTime			(-maxTime)

#define msPerDay 		(86400000.0)		/* 15.9.1.2 */
#define Day(t) 			NUMBER_floor((t) / msPerDay)/* 15.9.1.2 */
#define TimeWithinDay(t)	modulo(t, msPerDay)	/* 15.9.1.2 */

#define DaysInYear(y) 					/* 15.9.1.3 */	\
	((y) % 4) ? 365 : ((y) % 100) ? 366 : ((y) % 400) ? 365 : 366
static SEE_number_t DayFromYear(SEE_number_t);		/* 15.9.1.3 */
#define TimeFromYear(y)					/* 15.9.1.3 */	\
	(msPerDay * DayFromYear(y))
static SEE_number_t YearFromTime(SEE_number_t);		/* 15.9.1.3 */
#define InLeapYear(t)	isleapyear(YearFromTime(t))	/* 15.9.1.3 */
static int isleapyear(SEE_number_t);
#define DayWithinYear(t) \
		(Day(t) - DayFromYear(YearFromTime(t)))
static SEE_number_t MonthFromTime(SEE_number_t);	/* 15.9.1.4 */
static SEE_number_t DateFromTime(SEE_number_t);			/* 15.9.1.5 */
#define WeekDay(t)		modulo(Day(t) + 4, 7.0)	/* 15.9.1.6 */

#define LocalTZA(i)	_SEE_platform_tza(i)		/* 15.9.1.7(8) */
static SEE_number_t DaylightSavingTA(struct SEE_interpreter *, SEE_number_t);
static SEE_number_t UTC(struct SEE_interpreter *, SEE_number_t);
static SEE_number_t LocalTime(struct SEE_interpreter *, SEE_number_t);

/* 15.9.1.10 */
#define	HourFromTime(t)	modulo(NUMBER_floor((t) / msPerHour), HoursPerDay)
#define	MinFromTime(t)	modulo(NUMBER_floor((t) / msPerMinute), MinutesPerHour)
#define	SecFromTime(t)	modulo(NUMBER_floor((t) / msPerSecond), \
			       SecondsPerMinute)
#define	msFromTime(t)	modulo(t, msPerSecond)
#define	HoursPerDay		24.0
#define MinutesPerHour		60.0
#define SecondsPerMinute	60.0
#define	msPerSecond		1000.0
#define	msPerMinute		(msPerSecond * SecondsPerMinute)
#define	msPerHour		(msPerMinute * MinutesPerHour)

static SEE_number_t modulo(SEE_number_t, SEE_number_t);
static SEE_number_t MakeTime(SEE_number_t, SEE_number_t,	/* 15.9.1.11 */
				SEE_number_t, SEE_number_t);
static SEE_number_t MakeDay(SEE_number_t, SEE_number_t,		/* 15.9.1.12 */
				SEE_number_t);
static SEE_number_t MakeDate(SEE_number_t, SEE_number_t);	/* 15.9.1.13 */
static SEE_number_t TimeClip(SEE_number_t);			/* 15.9.1.14 */

static SEE_number_t now(struct SEE_interpreter *);
static SEE_number_t parsetime(struct SEE_interpreter *, struct SEE_string *);
static SEE_number_t parse_netscape_time(struct SEE_interpreter *, 
	struct SEE_string *);
static struct SEE_string *reprtime(struct SEE_interpreter *, SEE_number_t);

static struct date_object *todate(struct SEE_interpreter *,
	struct SEE_object *);

static void date_call(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_construct(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_defaultvalue(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_value *,
	struct SEE_value *);

static void date_parse(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_UTC(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);

static void date_proto_toString(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_toDateString(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_toTimeString(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_toLocaleString(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_toLocaleDateString(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_toLocaleTimeString(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_valueOf(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getTime(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getFullYear(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getUTCFullYear(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getMonth(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getUTCMonth(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getDate(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getUTCDate(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getDay(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getUTCDay(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getHours(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getUTCHours(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getMinutes(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getUTCMinutes(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getSeconds(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getUTCSeconds(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getMilliseconds(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getUTCMilliseconds(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_getTimezoneOffset(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setTime(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setMilliseconds(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setUTCMilliseconds(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setSeconds(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setUTCSeconds(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setMinutes(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setUTCMinutes(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setHours(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setUTCHours(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setDate(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setUTCDate(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setMonth(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setUTCMonth(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setFullYear(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setUTCFullYear(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_toUTCString(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);

static void date_proto_getYear(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void date_proto_setYear(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);


/* object class for Date constructor */
static struct SEE_objectclass date_const_class = {
	"DateConstructor",		/* Class */
	SEE_native_get,			/* Get */
	SEE_native_put,			/* Put */
	SEE_native_canput,		/* CanPut */
	SEE_native_hasproperty,		/* HasProperty */
	SEE_native_delete,		/* Delete */
	SEE_native_defaultvalue,	/* DefaultValue */
	SEE_native_enumerator,		/* DefaultValue */
	date_construct,			/* Construct */
	date_call			/* Call */
};

/* object class for Date.prototype and date instances */
static struct SEE_objectclass date_inst_class = {
	"Date",				/* Class */
	SEE_native_get,			/* Get */
	SEE_native_put,			/* Put */
	SEE_native_canput,		/* CanPut */
	SEE_native_hasproperty,		/* HasProperty */
	SEE_native_delete,		/* Delete */
	date_defaultvalue,		/* DefaultValue */
	SEE_native_enumerator		/* enumerator */
};

static SEE_number_t
modulo(a, b)
	SEE_number_t a, b;
{
	SEE_number_t r;
	
	r = NUMBER_fmod(a, b);
	if (r < 0) r += b;
	return r;
}

/* 15.9.1.9 */
static SEE_number_t
UTC(interp, t)
	struct SEE_interpreter *interp;
	SEE_number_t t;
{
	return t - LocalTZA(interp) 
	         - DaylightSavingTA(interp, t - LocalTZA(interp));
}

/* 15.9.1.9 */
static SEE_number_t
LocalTime(interp, t)
	struct SEE_interpreter *interp;
	SEE_number_t t;
{
	return t + LocalTZA(interp) + DaylightSavingTA(interp, t);
}

/*
 * Can only use the following four properties for computing DST:
 *   ysec - time since beginning of year
 *   InLeapYear(t)
 *   weekday of beginning of year
 *   geographic location
 * 15.9.1.8(9)
 */
static SEE_number_t
DaylightSavingTA(interp, t)
	struct SEE_interpreter *interp;
	SEE_number_t t;
{
	SEE_number_t ysec = t - TimeFromYear(YearFromTime(t));
	int ily = InLeapYear(t);
	int wstart = WeekDay(TimeFromYear(YearFromTime(t)));

	return _SEE_platform_dst(interp, ysec, ily, wstart);
}

void
SEE_Date_alloc(interp)
	struct SEE_interpreter *interp;
{
	interp->Date = 
	    (struct SEE_object *)SEE_NEW(interp, struct SEE_native);
	interp->Date_prototype = 
	    (struct SEE_object *)SEE_NEW(interp, struct date_object);
}

void
SEE_Date_init(interp)
	struct SEE_interpreter *interp;
{
	struct SEE_object *Date;		/* struct SEE_native */
	struct SEE_object *Date_prototype;	/* struct date_object */
	struct SEE_value v;

	Date = interp->Date;
	Date_prototype = interp->Date_prototype;

	SEE_native_init((struct SEE_native *)Date, interp,
		&date_const_class, interp->Function_prototype);

	/* 15.9.4.1 Date.prototype */
	SEE_SET_OBJECT(&v, Date_prototype);
	SEE_OBJECT_PUT(interp, Date, STR(prototype), &v,
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY);

	/* 15.9.5.4 Date.length = 7 */
	SEE_SET_NUMBER(&v, 7);
	SEE_OBJECT_PUT(interp, Date, STR(length), &v, SEE_ATTR_LENGTH);

#define PUTCFUNC(name, len)						\
	SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, date_##name, 	\
		STR(name), len));					\
	SEE_OBJECT_PUT(interp, Date, STR(name), &v, SEE_ATTR_DEFAULT);

	PUTCFUNC(parse, 1)			/* 15.9.4.2 */
	PUTCFUNC(UTC, 7)			/* 15.9.4.3 */

	/* 15.9.5 */
	SEE_native_init((struct SEE_native *)Date_prototype, interp,
		&date_inst_class, interp->Object_prototype);


	((struct date_object *)Date_prototype)->t = SEE_NaN;

	/* 15.9.5.1 Date.prototype.constructor */
	SEE_SET_OBJECT(&v, Date);
	SEE_OBJECT_PUT(interp, Date_prototype, STR(constructor), &v, 
		SEE_ATTR_DEFAULT);

#define PUTFUNC(name, len) \
	SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, 			\
		date_proto_##name, STR(name), len));			\
	SEE_OBJECT_PUT(interp, Date_prototype, STR(name), &v, 		\
		SEE_ATTR_DEFAULT);

	PUTFUNC(toString, 0)
	PUTFUNC(toDateString, 0)
	PUTFUNC(toTimeString, 0)
	PUTFUNC(toLocaleString, 0)
	PUTFUNC(toLocaleDateString, 0)
	PUTFUNC(toLocaleTimeString, 0)
	PUTFUNC(valueOf, 0)
	PUTFUNC(getTime, 0)
	PUTFUNC(getFullYear, 0)
	PUTFUNC(getUTCFullYear, 0)
	PUTFUNC(getMonth, 0)
	PUTFUNC(getUTCMonth, 0)
	PUTFUNC(getDate, 0)
	PUTFUNC(getUTCDate, 0)
	PUTFUNC(getDay, 0)
	PUTFUNC(getUTCDay, 0)
	PUTFUNC(getHours, 0)
	PUTFUNC(getUTCHours, 0)
	PUTFUNC(getMinutes, 0)
	PUTFUNC(getUTCMinutes, 0)
	PUTFUNC(getSeconds, 0)
	PUTFUNC(getUTCSeconds, 0)
	PUTFUNC(getMilliseconds, 0)
	PUTFUNC(getUTCMilliseconds, 0)
	PUTFUNC(getTimezoneOffset, 0)
	PUTFUNC(setTime, 1)
	PUTFUNC(setMilliseconds, 1)
	PUTFUNC(setUTCMilliseconds, 1)
	PUTFUNC(setSeconds, 2)
	PUTFUNC(setUTCSeconds, 2)
	PUTFUNC(setMinutes, 3)
	PUTFUNC(setUTCMinutes, 3)
	PUTFUNC(setHours, 4)
	PUTFUNC(setUTCHours, 4)
	PUTFUNC(setDate, 1)
	PUTFUNC(setUTCDate, 1)
	PUTFUNC(setMonth, 2)
	PUTFUNC(setUTCMonth, 2)
	PUTFUNC(setFullYear, 3)
	PUTFUNC(setUTCFullYear, 3)
	PUTFUNC(toUTCString, 0)
	if (interp->compatibility & SEE_COMPAT_262_3B) {
	    /* toGMTString() == toUTCString() */
	    SEE_OBJECT_PUT(interp, Date_prototype, STR(toGMTString), &v, 
	    	SEE_ATTR_DEFAULT);

	    PUTFUNC(getYear, 0)
	    PUTFUNC(setYear, 1)
	}

}

/* 15.9.1.3 */
static SEE_number_t
DayFromYear(y)
	SEE_number_t y;
{
	return 365.0 * (y-1970) + NUMBER_floor((y-1969)/4) -
		NUMBER_floor((y-1901)/100) + NUMBER_floor((y-1601)/400);
}

/* 15.9.1.3 */
static SEE_number_t
YearFromTime(t0)
	SEE_number_t t0;
{
	/*
	 * "return the largest integer y (closest to +Inf) such
	 *  that TimeFromYear(y) <= t"
	 */
	SEE_number_t y, t;

	y = 0;
	t = t0 + T1970;
	y += 400 * NUMBER_floor(t / msPerY400);
	t = modulo(t, msPerY400);
	y += 100 * NUMBER_floor(t / msPerY100);
	t = modulo(t, msPerY100);
	y += 4 * NUMBER_floor(t / msPerY4);
	t = modulo(t, msPerY4);
	y += 1 * NUMBER_floor(t / msPerY1);
	t = modulo(t, msPerY1);

#ifndef NDEBUG
#define AS(x) do {if (!(x)) \
	dprintf("%s:%d: FAILURE: '%s'; y=%g t0=%g\n", \
	__FILE__, __LINE__, #x, y, t0); } while(0)
AS(TimeFromYear(y) <= t0);
AS(TimeFromYear(y + 1) > t0);
#undef AS
#endif

	return y;
}

/* 15.9.1.3 */
static int
isleapyear(y)
	SEE_number_t y;
{
	if (modulo(y, 4.0) != 0)	return 0;
	if (modulo(y, 100.0) != 0)	return 1;
	if (modulo(y, 400.0) != 0)	return 0;
	return 1;
}

/* 15.9.1.4 */
static SEE_number_t
MonthFromTime(t)
	SEE_number_t t;
{
	SEE_number_t dwy = DayWithinYear(t);
	SEE_number_t ily = InLeapYear(t);

	if (dwy < 31)		return 0;
	if (dwy < 59+ily)	return 1;
	if (dwy < 90+ily)	return 2;
	if (dwy < 120+ily)	return 3;
	if (dwy < 151+ily)	return 4;
	if (dwy < 181+ily)	return 5;
	if (dwy < 212+ily)	return 6;
	if (dwy < 243+ily)	return 7;
	if (dwy < 273+ily)	return 8;
	if (dwy < 304+ily)	return 9;
	if (dwy < 334+ily)	return 10;
	if (dwy < 365+ily)	return 11;
	return -1;
}

/* 15.9.1.5 */
static SEE_number_t
DateFromTime(t)
	SEE_number_t t;
{
	SEE_number_t dwy = DayWithinYear(t);
	int ily = InLeapYear(t);
	
	switch ((int)MonthFromTime(t)) {
	case 0:		return dwy + 1;
	case 1:		return dwy - 30;
	case 2:		return dwy - 58 - ily;
	case 3:		return dwy - 89 - ily;
	case 4:		return dwy - 119 - ily;
	case 5:		return dwy - 150 - ily;
	case 6:		return dwy - 180 - ily;
	case 7:		return dwy - 211 - ily;
	case 8:		return dwy - 242 - ily;
	case 9:		return dwy - 272 - ily;
	case 10:	return dwy - 303 - ily;
	case 11:	return dwy - 333 - ily;
	default:	return -1;
	}
}

/* see 9.4 - this version works directly on numbers */
static SEE_number_t
ToInteger(n)
	SEE_number_t n;
{
	if (SEE_ISNAN(n))
		return 0;
	if (SEE_ISINF(n))
		return n;
	return SGN(n) * NUMBER_floor(ABS(n));
}

/* 15.9.1.11 */
static SEE_number_t
MakeTime(hour, min, sec, ms)
	SEE_number_t hour, min, sec, ms;
{
	if (!SEE_ISFINITE(hour) || !SEE_ISFINITE(min) || 
	    !SEE_ISFINITE(sec) || !SEE_ISFINITE(ms))
		return SEE_NaN;
	return	ToInteger(hour) * msPerHour +
		ToInteger(min) * msPerMinute +
		ToInteger(sec) * msPerSecond +
		ToInteger(ms);
}

/* Julian date of the 1st of each month. */
static unsigned int 
  julian[]    = { 1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 },
  julian_ly[] = { 1, 32, 61, 92, 122, 153, 183, 214, 245, 275, 306, 336 };

/* 15.9.1.12 */
static SEE_number_t
MakeDay(year, month, date)
	SEE_number_t year, month, date;
{
	SEE_number_t r2, r3, r4, y, m, t, day;
	int ily;

	if (SEE_ISNAN(year) || SEE_ISNAN(month) || SEE_ISNAN(date))
		return SEE_NaN;
	r2 = ToInteger(year);
	r3 = ToInteger(month);
	r4 = ToInteger(date);
	y = r2 + NUMBER_floor(r3 / 12);
	m = modulo(r3, 12.0);

	if (DayFromYear(y) < -100000000 ||
	    DayFromYear(y) >  100000000)
		return SEE_NaN;

	ily = isleapyear(NUMBER_rint(y));
	t = (DayFromYear(y) + (ily ? julian_ly:julian)[(int)m] - 1) * msPerDay;

#ifndef NDEBUG
#define AS(x,y) do { SEE_number_t __x = (x); if (__x!=(y)) \
	dprintf("%s:%d: FAILURE: %s = %g; expected %g (%g/%g/%g:%d)\n",\
	__FILE__, __LINE__, #x, __x, (SEE_number_t)(y), \
	year,month,date,ily); } while(0)
AS(YearFromTime(t), y);
AS(MonthFromTime(t), m);
AS(DateFromTime(t), 1);
#undef AS
#endif

	day = Day(t) + r4 - 1;
	return (day < -100000000 || day > 100000000) ? SEE_NaN : day;
}

/* 15.9.1.13 */
static SEE_number_t
MakeDate(day, time)
	SEE_number_t day, time;
{
	if (!SEE_ISFINITE(day) || !SEE_ISFINITE(time))
		return SEE_NaN;
	return day * msPerDay + time;
}

/* 15.9.1.14 */
static SEE_number_t
TimeClip(t)
	SEE_number_t t;
{
	if (!SEE_ISFINITE(t))
		return SEE_NaN;
	if (t > 8.64e15 || t < -8.64e15)
		return SEE_NaN;
	/*
	 * spec bug: TimeClip definition refers to Result(1) and Result(2), 
	 * which are meaningless. Only meaningful interpretation is to 
	 * treat them as references to 'time' (which is variable 't' in this
	 * implementation)
	 */
	return ToInteger(t);
}

/* Return the time right now in milliseconds since Jan 1 1970 UTC 0:00 */
static SEE_number_t
now(interp)
	struct SEE_interpreter *interp;
{
	return TimeClip(_SEE_platform_time(interp));
}

#define ISWHITE(c)	(s[i] == ' ' || s[i] == '\t')
#define ISLETTER(c)	(((c) >= 'A' && (c) <= 'Z') || \
		      	 ((c) >= 'a' && (c) <= 'z'))
#define ISDIGIT(c)	((c) >= '0' && (c) <= '9')
#define TOLOWER(c)	(((c) >= 'A' && (c) <= 'Z') ? (c) - 'A' + 'a' : (c))

/* ref. 15.9.4.2 */
static SEE_number_t
parsetime(interp, str)
	struct SEE_interpreter *interp;
	struct SEE_string *str;
{
	/*
	 * The standard does not specify how dates should be
	 * formatted, or what formats are parseable. Here, I 
	 * simply have times of the fixed format
	 *	"wkd, dd Mmm yyyy hh:mm:ss GMT"
	 *	"Sun, 12 Oct 2003 07:19:24 GMT"
	 * This corresponds with RFC ????, with extended years, although a 
	 * deficiency is that the milliseconds component is lost. 
	 * NaN is returned if the date wasn't parseable.
	 *
	 * (Yes, "GMT" and not "UTC")
	 *
	 * If the GMT suffix is missing, the date is assumed to be
	 * in the local timezone.
	 *	"wkd, dd Mmm yyyy hh:mm:ss"  - assume local timezone
	 *	"wkd, dd Mmm yyyy hh:mm"     - assume local tz ss=00
	 *	"wkd, dd Mmm yyyy"           - assume local tz 00:00:00
	 *
	 * Also, the day and month may be swapped around:
	 *      "wkd Mmm dd yyyy hh:mm:ss"   - Netscape-style
	 *
	 * The "GMT" can be followed by an offset
	 */

	int i, d, m, y, yneg, hr, min, sec, wd;
	int len = str->length;
	SEE_char_t *s = str->data;
	static char mname[] = "janfebmaraprmayjunjulaugsepoctnovdec";
	static char wname[] = "sunmontuewedthufrisat";
	SEE_number_t t;
		

	i = 0;
	while (i < len && ISWHITE(s[i])) i++;

	if (i + 2 < len && ISLETTER(s[i]) && ISLETTER(s[i+1]) &&
			   ISLETTER(s[i+2]))
	{
		for (wd = 0; wd < 7; wd++)
		    if (wname[wd*3] == TOLOWER(s[i]) &&
		        wname[wd*3+1] == TOLOWER(s[i+1]) &&
		        wname[wd*3+2] == TOLOWER(s[i+2]))
			    break;
		if (wd < 7) {
		    i += 3;					/* Wkday */
		    if (i < len && s[i] == ',') i++;		/* ','? */
		    while (i < len && ISWHITE(s[i])) i++;	/* space? */
		}
	}

	if (i < len && ISDIGIT(s[i])) {
	    for (d = 0; i < len && ISDIGIT(s[i]); i++)		/* day */
		d = 10 * d + s[i] - '0';
	    if (!(i < len && ISWHITE(s[i]))) return SEE_NaN;	/* space */
	    while (i < len && ISWHITE(s[i])) i++;
	    if (!(i + 3 < len)) return SEE_NaN;			/* month */
	    for (m = 0; m < 12; m++) 
		if (mname[m*3] == TOLOWER(s[i]) &&
		    mname[m*3+1] == TOLOWER(s[i+1]) &&
		    mname[m*3+2] == TOLOWER(s[i+2]))
			break;
	    i += 3;
	} else {
	    if (!(i + 3 < len)) return SEE_NaN;			/* month */
	    for (m = 0; m < 12; m++) 
		if (mname[m*3] == TOLOWER(s[i]) &&
		    mname[m*3+1] == TOLOWER(s[i+1]) &&
		    mname[m*3+2] == TOLOWER(s[i+2]))
			break;
	    i += 3;
	    if (!(i < len && ISWHITE(s[i]))) return SEE_NaN;	/* space */
	    while (i < len && ISWHITE(s[i])) i++;
	    if (!(i < len && ISDIGIT(s[i]))) return SEE_NaN;	/* day */
	    for (d = 0; i < len && ISDIGIT(s[i]); i++)
		d = 10 * d + s[i] - '0';
	}
	if (d < 1 || d > 31) return SEE_NaN;
	if (m >= 12) return SEE_NaN;
	if (!(i < len && ISWHITE(s[i]))) return SEE_NaN;	/* space */
	while (i < len && ISWHITE(s[i])) i++;
	if (i < len && s[i] == '-') {				/* -? */
		yneg = 1;
		i++; 
	} else { 
		yneg = 0;
	}
	if (!(i < len && ISDIGIT(s[i]))) return SEE_NaN;	/* year */
	for (y = 0; i < len && ISDIGIT(s[i]); i++)
		y = 10 * y + s[i] - '0';
	if (yneg) y = -y;

	hr = min = sec = 0;
	if (i < len && ISWHITE(s[i])) {
	    while (i < len && ISWHITE(s[i])) i++;		/* space+ */
	    if (i + 4 < len &&					/* hh:mm? */
	      ISDIGIT(s[i+0]) && ISDIGIT(s[i+1]) && s[i+2] == ':' &&
	      ISDIGIT(s[i+3]) && ISDIGIT(s[i+4]))
	    {
	        hr  = (s[i+0]-'0') * 10 + (s[i+1]-'0');
	        min = (s[i+3]-'0') * 10 + (s[i+4]-'0');
	        i += 5;
	        if (i + 2 < len &&				/* :ss? */
		    s[i+0] == ':' && ISDIGIT(s[i+1]) && ISDIGIT(s[i+2])) 
	        {
	    	    sec = (s[i+1]-'0') * 10 + (s[i+2]-'0');
		    i += 3;
	        }
	    }
	}
	if (hr >= 24 || min >= 60 || sec >= 60)
		return SEE_NaN;

	t = MakeDate(
		MakeDay((SEE_number_t)y, (SEE_number_t)m, (SEE_number_t)d),
		MakeTime((SEE_number_t)hr, (SEE_number_t)min, 
		         (SEE_number_t)sec, 0.0));

	/* 
	 * If the next few characters are "GMT", then it is already UTC, 
	 * otherwise assume date is in local timezone and convert to UTC.
	 */
	while (i < len && ISWHITE(s[i])) i++;			/* space */
	if (i + 2 < len && s[i] == 'G' && s[i+1] == 'M' && s[i+2] == 'T') {
		i += 3;
		if (i + 4 < len && (s[i+0] == '-' || s[i+0] == '+') &&
		    ISDIGIT(s[i+1]) && ISDIGIT(s[i+2]) &&
		    s[i+3] == '0' && s[i+4] == '0')
		{
		    int gmtoff = (s[i+1]-'0') * 10 + (s[i+2]-'0');
		    if (s[i+0] == '-') gmtoff = -gmtoff;
		    t -= msPerHour * gmtoff;
		    i += 5;
		}
	} else
		t = UTC(interp, t);

	/* XXX extra text is ignored */

	return TimeClip(t);
}

static SEE_number_t
parse_netscape_time(interp, str)
	struct SEE_interpreter *interp;
	struct SEE_string *str;
{
	/* 
	 * Netscape's javascript engine generates
	 * strings of the form '1/1/1999 12:30 AM'
	 */

	int i, d, m, y, hr=0, min=0, sec=0;
	int n[3], j;
	int len = str->length;
	SEE_char_t *s = str->data;
	SEE_number_t t;

	i = 0;
	while (i < len && ISWHITE(s[i])) i++;

	for (j = 0; j < 3; j++) {
	    int isneg;
	    if (j) {
		while (i < len && ISWHITE(s[i])) i++;
		if (!(i < len && s[i] == '/')) goto fail; i++;
		while (i < len && ISWHITE(s[i])) i++;
	    }
	    n[j] = 0;
	    if (i < len && s[i] == '-') { i++; isneg = 1; } else isneg = 0;
	    if (!(i < len && ISDIGIT(s[i]))) goto fail;
	    for (n[j] = 0; i < len && ISDIGIT(s[i]); i++)
		    n[j] = n[j] * 10 + s[i] - '0';
	    if (isneg)
	    	n[j] = -n[j];
	}
	if (n[0] >= 70 && n[1] >= 70) goto fail;
	if (n[0] >= 70) {
		y = n[0] + 1900;
		m = n[1];
		d = n[2];
	} else {
		m = n[0];
		d = n[1];
		y = n[2];
		if (y < 100) y += 1900;
	}

	hr = min = sec = 0;

	if (!(i < len && ISWHITE(s[i]))) goto done;
	while (i < len && ISWHITE(s[i])) i++;

	if (!(i < len && ISDIGIT(s[i]))) goto done;
	for (hr = 0; i < len && ISDIGIT(s[i]); i++)
		hr = hr * 10 + s[i] - '0';

	while (i < len && ISWHITE(s[i])) i++;
	if (!(i < len && s[i] == ':')) goto done; i++;
	while (i < len && ISWHITE(s[i])) i++;

	if (!(i < len && ISDIGIT(s[i]))) goto fail;
	for (min = 0; i < len && ISDIGIT(s[i]); i++)
		min = min * 10 + s[i] - '0';

	while (i < len && ISWHITE(s[i])) i++;
	if (!(i < len && s[i] == ':')) goto ampm; i++;
	while (i < len && ISWHITE(s[i])) i++;

	if (!(i < len && ISDIGIT(s[i]))) goto fail;
	for (sec = 0; i < len && ISDIGIT(s[i]); i++)
		sec = sec * 10 + s[i] - '0';

	while (i < len && ISWHITE(s[i])) i++;
  ampm:
	if (i+1 < len && (s[i+1] == 'm' || s[i+1] == 'M')) {
		if (s[i] == 'p' || s[i] == 'P') {
			if (hr > 12 || hr < 1) goto fail;
			hr = (hr % 12) + 12;
		} else if (s[i] == 'a' || s[i] == 'A') {
			if (hr > 12 || hr < 1) goto fail;
			hr = (hr % 12);
		} else
			goto fail;
		i += 2;
		while (i < len && ISWHITE(s[i])) i++;
	}

  done:
  /*
	if (hr > 24 || min >= 60 || sec >= 60) goto fail;
	if (m < 1 || m > 12 || d < 1 || d > 31) goto fail;
    */

	t = MakeDate(
	    MakeDay((SEE_number_t)y, (SEE_number_t)(m - 1), (SEE_number_t)d),
	    MakeTime((SEE_number_t)hr, (SEE_number_t)min, 
	    	     (SEE_number_t)sec, 0.0));
	return TimeClip(UTC(interp, t));

  fail:
	return SEE_NaN;
}

#undef ISWHITE
#undef ISLETTER
#undef ISDIGIT
#undef TOLOWER

static char wkdayname[] = "SunMonTueWedThuFriSat";
static char monthname[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

static struct SEE_string *
repr_baddate(interp)
	struct SEE_interpreter *interp;
{
	if (SEE_GET_JS_COMPAT(interp))
		return SEE_string_sprintf(interp, "Invalid Date");
	else
		return STR(NaN);
}

/* ref 15.9.5.2 */
static struct SEE_string *
reprdatetime(interp, t, utc)
	struct SEE_interpreter *interp;
	SEE_number_t t;
	int utc;
{
	SEE_int32_t wkday, day, month, year, hour, min, sec;
	int gmtoff;

	if (SEE_ISNAN(t)) return repr_baddate(interp);

	if (!utc) {
		t = LocalTime(interp, t);
		gmtoff = (int)((t - LocalTime(interp, t)) / msPerMinute);
	} else
		gmtoff = 0;

	wkday = WeekDay(t);
	day = DateFromTime(t);
	month = MonthFromTime(t);
	year = YearFromTime(t);
	hour = HourFromTime(t);
	min = MinFromTime(t);
	sec = SecFromTime(t);


	if (SEE_GET_JS_COMPAT(interp)) {
	    if (utc)
		return SEE_string_sprintf(interp,
			"%.3s, %02d %.3s %04d %02d:%02d:%02d GMT",
			&wkdayname[wkday * 3], day, &monthname[month * 3],
			year, hour, min, sec);
	    return SEE_string_sprintf(interp,
	    	"%.3s %.3s %02d %04d %02d:%02d:%02d GMT%+03d00",
		&wkdayname[wkday * 3], &monthname[month * 3],
		day, year, hour, min, sec, -gmtoff / 60);
	}

	/* "Sun, 12 Oct 2003 07:19:24" */
	return SEE_string_sprintf(interp,
		"%.3s, %2d %.3s %4d %02d:%02d:%02d%s",
		&wkdayname[wkday * 3], day, &monthname[month * 3],
		year, hour, min, sec, utc ? " GMT" : "");
}

static struct SEE_string *
reprdate(interp, t)
	struct SEE_interpreter *interp;
	SEE_number_t t;
{
	SEE_int32_t wkday, day, month, year;

	if (SEE_ISNAN(t)) return repr_baddate(interp);

	wkday = WeekDay(t);
	day = (SEE_int32_t)DateFromTime(t);
	month = (SEE_int32_t)MonthFromTime(t);
	year = (SEE_int32_t)YearFromTime(t);

	return SEE_string_sprintf(interp,
		"%.3s, %2d %.3s %d",
		&wkdayname[wkday * 3], day,
		&monthname[month * 3], year);
}

static struct SEE_string *
reprtime(interp, t)
	struct SEE_interpreter *interp;
	SEE_number_t t;
{
	SEE_int32_t hour, min, sec10;
	SEE_number_t secms;

	if (SEE_ISNAN(t)) return repr_baddate(interp);

	hour = HourFromTime(t);
	min = MinFromTime(t);
	secms = modulo(t / msPerSecond, SecondsPerMinute);
	sec10 = NUMBER_floor(secms / 10);
	secms = NUMBER_fmod(secms, 10.0);

	return SEE_string_sprintf(interp,
		"%02d:%02d:%02d",
		hour, min, sec10, NUMBER_floor(secms));
}

/* Return a date object or raise a type error */
static struct date_object *
todate(interp, o)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
{
	if (o->objectclass != &date_inst_class)
		SEE_error_throw_string(interp, interp->TypeError, 
		   STR(not_date));
	return (struct date_object *)o;
}

/* 15.9.2.1 */
static void
date_call(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	/* Ignore arguments; equiavlent to (new Date()).toString() */
	SEE_SET_STRING(res, reprdatetime(interp, now(interp), 0));
}

/* 15.9.3.1 */
static void
date_construct(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	struct date_object *d;
	struct SEE_value v, w, hint;
	SEE_number_t year, month, date, hours, minutes, seconds, ms;
	SEE_number_t t;

	if (argc == 0) 
		t = now(interp);
	else if (argc == 1) {
		SEE_SET_OBJECT(&hint, interp->Number);
		SEE_ToPrimitive(interp, argv[0], &hint, &v);
		if (SEE_VALUE_GET_TYPE(&v) != SEE_STRING) {
		    SEE_ToNumber(interp, &v, &w);
		    t = TimeClip(w.u.number);
		} else  {
		    t = parsetime(interp, v.u.string);
		    if (SEE_GET_JS_COMPAT(interp) && SEE_ISNAN(t))
		        t = parse_netscape_time(interp, v.u.string);
		}
	} else {
		SEE_ToNumber(interp, argv[0], &v);
		year = v.u.number;
		if (!SEE_NUMBER_ISNAN(&v)) {
			SEE_int32_t year_int = ToInteger(year);
			if (0 <= year_int && year_int <= 99)
				year += 1900;
		}

		SEE_ToNumber(interp, argv[1], &v);
		month = v.u.number;
		if (argc > 2) {
		    SEE_ToNumber(interp, argv[2], &v);
		    date = v.u.number;
		} else
		    date = 1;
		if (argc > 3) {
		    SEE_ToNumber(interp, argv[3], &v);
		    hours = v.u.number;
		} else
		    hours = 0;
		if (argc > 4) {
		    SEE_ToNumber(interp, argv[4], &v);
		    minutes = v.u.number;
		} else
		    minutes = 0;
		if (argc > 5) {
		    SEE_ToNumber(interp, argv[5], &v);
		    seconds = v.u.number;
		} else
		    seconds = 0;
		if (argc > 6) {
		    SEE_ToNumber(interp, argv[6], &v);
		    ms = v.u.number;
		} else
		    ms = 0;

		t = TimeClip(UTC(interp, MakeDate(MakeDay(year, month, date),
			MakeTime(hours, minutes, seconds, ms))));
	}

	d = SEE_NEW(interp, struct date_object);
	SEE_native_init(&d->native, interp, &date_inst_class,
		interp->Date_prototype);
	d->t = t;

	SEE_SET_OBJECT(res, (struct SEE_object *)d);
}

/* See NOTE at 11.6.1 */
static void
date_defaultvalue(interp, obj, hint, res)
	struct SEE_interpreter *interp;
	struct SEE_object *obj;
	struct SEE_value *hint;
	struct SEE_value *res;
{
	struct SEE_value string_hint;

	if (hint == NULL) {
		SEE_SET_OBJECT(&string_hint, interp->String);
		hint = &string_hint;
	}
	SEE_native_defaultvalue(interp, obj, hint, res);
}

/* 15.9.4.2 */
static void
date_parse(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;
	struct SEE_string *s;

	if (argc) {
		SEE_ToString(interp, argv[0], &v);
		s = v.u.string;
	} else
		s = STR(empty_string);

	SEE_SET_NUMBER(res, parsetime(interp, s));
}

/* 15.9.4.3 */
static void
date_UTC(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_value v;
	SEE_number_t year, month, date, hours, minutes, seconds, ms;

	if (argc < 2)
		SEE_error_throw_string(interp, interp->RangeError,
			STR(implementation_dependent));

	SEE_ToNumber(interp, argv[0], &v);
	year = v.u.number;
	if (!SEE_NUMBER_ISNAN(&v)) {
	    SEE_int32_t year_int = ToInteger(year);
	    if (0 <= year_int && year_int <= 99)
		year += 1900;
	}
	SEE_ToNumber(interp, argv[1], &v);
	month = v.u.number;
	if (argc > 2) {
		SEE_ToNumber(interp, argv[2], &v);
		date = v.u.number;
	} else
		date = 1;
	if (argc > 3) {
		SEE_ToNumber(interp, argv[3], &v);
		hours = v.u.number;
	} else
		hours = 0;
	if (argc > 4) {
		SEE_ToNumber(interp, argv[4], &v);
		minutes = v.u.number;
	} else
		minutes = 0;
	if (argc > 5) {
		SEE_ToNumber(interp, argv[5], &v);
		seconds = v.u.number;
	} else
		seconds = 0;
	if (argc > 6) {
		SEE_ToNumber(interp, argv[6], &v);
		ms = v.u.number;
	} else
		ms = 0;

	SEE_SET_NUMBER(res, TimeClip(MakeDate(MakeDay(year, month, date),
		MakeTime(hours, minutes, seconds, ms))));
}

/* 15.9.5.2 */
static void
date_proto_toString(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	SEE_SET_STRING(res, reprdatetime(interp, d->t, 0));
}

/* 15.9.5.3 */
static void
date_proto_toDateString(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	SEE_SET_STRING(res, reprdate(interp, d->t));
}

/* 15.9.5.4 */
static void
date_proto_toTimeString(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	SEE_SET_STRING(res, reprtime(interp, d->t));
}

/* 15.9.5.5 */
static void
date_proto_toLocaleString(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	date_proto_toString(interp, self, thisobj, argc, argv, res);
}

/* 15.9.5.6 */
static void
date_proto_toLocaleDateString(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	date_proto_toDateString(interp, self, thisobj, argc, argv, res);
}

/* 15.9.5.7 */
static void
date_proto_toLocaleTimeString(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	date_proto_toTimeString(interp, self, thisobj, argc, argv, res);
}

/* 15.9.5.8 */
static void
date_proto_valueOf(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.9 */
static void
date_proto_getTime(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.10 */
static void
date_proto_getFullYear(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, 
		    YearFromTime(LocalTime(interp, d->t)));
}

/* 15.9.5.11 */
static void
date_proto_getUTCFullYear(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, 
		    YearFromTime(d->t));
}

/* 15.9.5.12 */
static void
date_proto_getMonth(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, MonthFromTime(LocalTime(interp, d->t)));
}

/* 15.9.5.13 */
static void
date_proto_getUTCMonth(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, MonthFromTime(d->t));
}

/* 15.9.5.14 */
static void
date_proto_getDate(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, DateFromTime(LocalTime(interp, d->t)));
}

/* 15.9.5.15 */
static void
date_proto_getUTCDate(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, DateFromTime(d->t));
}

/* 15.9.5.16 */
static void
date_proto_getDay(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, WeekDay(LocalTime(interp, d->t)));
}

/* 15.9.5.17 */
static void
date_proto_getUTCDay(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, WeekDay(d->t));
}

/* 15.9.5.18 */
static void
date_proto_getHours(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, HourFromTime(LocalTime(interp, d->t)));
}

/* 15.9.5.19 */
static void
date_proto_getUTCHours(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, HourFromTime(d->t));
}

/* 15.9.5.20 */
static void
date_proto_getMinutes(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, MinFromTime(LocalTime(interp, d->t)));
}

/* 15.9.5.21 */
static void
date_proto_getUTCMinutes(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, MinFromTime(d->t));
}

/* 15.9.5.22 */
static void
date_proto_getSeconds(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, SecFromTime(LocalTime(interp, d->t)));
}

/* 15.9.5.23 */
static void
date_proto_getUTCSeconds(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, SecFromTime(d->t));
}

/* 15.9.5.24 */
static void
date_proto_getMilliseconds(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, msFromTime(LocalTime(interp, d->t)));
}

/* 15.9.5.25 */
static void
date_proto_getUTCMilliseconds(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, msFromTime(d->t));
}

/* 15.9.5.26 */
static void
date_proto_getTimezoneOffset(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, 
		    (d->t - LocalTime(interp, d->t)) / msPerMinute);
}

/* 15.9.5.27 */
static void
date_proto_setTime(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;

	if (argc < 1) 
		d->t = SEE_NaN;
	else {
		SEE_ToNumber(interp, argv[0], &v);
		d->t = TimeClip(v.u.number);
	}
	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.28 */
static void
date_proto_setMilliseconds(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;
	SEE_number_t t = LocalTime(interp, d->t);

	if (argc < 1)
		d->t = SEE_NaN;
	else {
		SEE_ToNumber(interp, argv[0], &v);
		d->t = TimeClip(UTC(interp, 
			MakeDate(Day(t), MakeTime(HourFromTime(t), 
			MinFromTime(t), SecFromTime(t), v.u.number))
		       ));
	}
	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.29 */
static void
date_proto_setUTCMilliseconds(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;
	SEE_number_t t = d->t;

	if (argc < 1)
		d->t = SEE_NaN;
	else {
		SEE_ToNumber(interp, argv[0], &v);
		d->t = TimeClip(   (
			MakeDate(Day(t), MakeTime(HourFromTime(t), 
			MinFromTime(t), SecFromTime(t), v.u.number))
		       ));
	}
	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.30 */
static void
date_proto_setSeconds(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;
	SEE_number_t ms;
	SEE_number_t t = LocalTime(interp, d->t);

	if (argc < 1)
		d->t = SEE_NaN;
	else {
		if (argc < 2)
			ms = msFromTime(t);
		else {
			SEE_ToNumber(interp, argv[1], &v);
			ms = v.u.number;
		}
		SEE_ToNumber(interp, argv[0], &v);
		d->t = TimeClip(UTC(interp, 
			MakeDate(Day(t), MakeTime(HourFromTime(t), 
			MinFromTime(t), v.u.number, ms))
		       ));
	}
	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.31 */
static void
date_proto_setUTCSeconds(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;
	SEE_number_t ms;
	SEE_number_t t = d->t;

	if (argc < 1)
		d->t = SEE_NaN;
	else {
		if (argc < 2)
			ms = msFromTime(t);
		else {
			SEE_ToNumber(interp, argv[1], &v);
			ms = v.u.number;
		}
		SEE_ToNumber(interp, argv[0], &v);
		d->t = TimeClip(   (
			MakeDate(Day(t), MakeTime(HourFromTime(t), 
			MinFromTime(t), v.u.number, ms))
		       ));
	}
	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.32 */
static void
date_proto_setMinutes(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;
	SEE_number_t sec, ms;
	SEE_number_t t = LocalTime(interp, d->t);

	if (argc < 1)
		d->t = SEE_NaN;
	else {
		if (argc < 2)
			sec = SecFromTime(t);
		else {
			SEE_ToNumber(interp, argv[1], &v);
			sec = v.u.number;
		}
		if (argc < 3)
			ms = msFromTime(t);
		else {
			SEE_ToNumber(interp, argv[2], &v);
			ms = v.u.number;
		}
		SEE_ToNumber(interp, argv[0], &v);
		d->t = TimeClip(UTC(interp, 
			MakeDate(Day(t), MakeTime(HourFromTime(t), 
			v.u.number, sec, ms))
		       ));
	}
	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.33 */
static void
date_proto_setUTCMinutes(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;
	SEE_number_t sec, ms;
	SEE_number_t t = d->t;

	if (argc < 1)
		d->t = SEE_NaN;
	else {
		if (argc < 2)
			sec = SecFromTime(t);
		else {
			SEE_ToNumber(interp, argv[1], &v);
			sec = v.u.number;
		}
		if (argc < 3)
			ms = msFromTime(t);
		else {
			SEE_ToNumber(interp, argv[2], &v);
			ms = v.u.number;
		}
		SEE_ToNumber(interp, argv[0], &v);
		d->t = TimeClip(   (
			MakeDate(Day(t), MakeTime(HourFromTime(t), 
			v.u.number, sec, ms))
		       ));
	}
	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.34 */
static void
date_proto_setHours(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;
	SEE_number_t min, sec, ms;
	SEE_number_t t = LocalTime(interp, d->t);

	if (argc < 1)
		d->t = SEE_NaN;
	else {
		if (argc < 2)
			min = MinFromTime(t);
		else {
			SEE_ToNumber(interp, argv[1], &v);
			min = v.u.number;
		}
		if (argc < 3)
			sec = SecFromTime(t);
		else {
			SEE_ToNumber(interp, argv[2], &v);
			sec = v.u.number;
		}
		if (argc < 4)
			ms = msFromTime(t);
		else {
			SEE_ToNumber(interp, argv[3], &v);
			ms = v.u.number;
		}
		SEE_ToNumber(interp, argv[0], &v);
		d->t = TimeClip(UTC(interp, 
			MakeDate(Day(t), MakeTime(v.u.number, min, sec, ms))
		       ));
	}
	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.35 */
static void
date_proto_setUTCHours(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;
	SEE_number_t min, sec, ms;
	SEE_number_t t = d->t;

	if (argc < 1)
		d->t = SEE_NaN;
	else {
		if (argc < 2)
			min = MinFromTime(t);
		else {
			SEE_ToNumber(interp, argv[1], &v);
			min = v.u.number;
		}
		if (argc < 3)
			sec = SecFromTime(t);
		else {
			SEE_ToNumber(interp, argv[2], &v);
			sec = v.u.number;
		}
		if (argc < 4)
			ms = msFromTime(t);
		else {
			SEE_ToNumber(interp, argv[3], &v);
			ms = v.u.number;
		}
		SEE_ToNumber(interp, argv[0], &v);
		d->t = TimeClip(   (
			MakeDate(Day(t), MakeTime(v.u.number, min, sec, ms))
		       ));
	}
	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.36 */
static void
date_proto_setDate(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;
	SEE_number_t t = LocalTime(interp, d->t);

	if (argc < 1)
		d->t = SEE_NaN;
	else {
		SEE_ToNumber(interp, argv[0], &v);
		d->t = TimeClip(UTC(interp, MakeDate(MakeDay(
			YearFromTime(t),
			MonthFromTime(t), v.u.number),
			TimeWithinDay(t))
		       ));
	}
	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.37 */
static void
date_proto_setUTCDate(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;
	SEE_number_t t = d->t;

	if (argc < 1)
		d->t = SEE_NaN;
	else {
		SEE_ToNumber(interp, argv[0], &v);
		d->t = TimeClip(   (MakeDate(MakeDay(
			YearFromTime(t), 
			MonthFromTime(t), v.u.number),
			TimeWithinDay(t))
		       ));
	}
	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.38 */
static void
date_proto_setMonth(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;
	SEE_number_t date;
	SEE_number_t t = LocalTime(interp, d->t);

	if (argc < 1)
		d->t = SEE_NaN;
	else {
		if (argc < 2) {
			date = DateFromTime(t);
			if (date < 0) date = SEE_NaN;
		} else {
			SEE_ToNumber(interp, argv[1], &v);
			date = v.u.number;
		}
		SEE_ToNumber(interp, argv[0], &v);
		d->t = TimeClip(UTC(interp, MakeDate(MakeDay(
			YearFromTime(t), v.u.number, date),
			TimeWithinDay(t))
		       ));
	}
	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.39 */
static void
date_proto_setUTCMonth(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;
	SEE_number_t date;
	SEE_number_t t = d->t;

	if (argc < 1)
		d->t = SEE_NaN;
	else {
		if (argc < 2) {
			date = DateFromTime(t);
			if (date < 0) date = SEE_NaN;
		} else {
			SEE_ToNumber(interp, argv[1], &v);
			date = v.u.number;
		}
		SEE_ToNumber(interp, argv[0], &v);
		d->t = TimeClip(   (MakeDate(MakeDay(
			YearFromTime(t), v.u.number, date),
			TimeWithinDay(t))
		       ));
	}
	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.40 */
static void
date_proto_setFullYear(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;
	SEE_number_t date, month;
	SEE_number_t t = LocalTime(interp, d->t);

	if (argc < 1)
		d->t = SEE_NaN;
	else {
		if (argc < 2)
			month = MonthFromTime(t);
		else {
			SEE_ToNumber(interp, argv[1], &v);
			month = v.u.number;
		}
		if (argc < 3) {
			date = DateFromTime(t);
			if (date < 0) date = SEE_NaN;
		} else {
			SEE_ToNumber(interp, argv[2], &v);
			date = v.u.number;
		}
		SEE_ToNumber(interp, argv[0], &v);
		d->t = TimeClip(UTC(interp,
			MakeDate(MakeDay(v.u.number, month, date),
			TimeWithinDay(t))
		       ));
	}
	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.41 */
static void
date_proto_setUTCFullYear(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;
	SEE_number_t date, month;
	SEE_number_t t = d->t;

	if (argc < 1)
		d->t = SEE_NaN;
	else {
		if (argc < 2)
			month = MonthFromTime(t);
		else {
			SEE_ToNumber(interp, argv[1], &v);
			month = v.u.number;
		}
		if (argc < 3) {
			date = DateFromTime(t);
			if (date < 0) date = SEE_NaN;
		} else {
			SEE_ToNumber(interp, argv[2], &v);
			date = v.u.number;
		}
		SEE_ToNumber(interp, argv[0], &v);
		d->t = TimeClip(   (MakeDate(MakeDay(v.u.number, month, date),
			TimeWithinDay(t))
		       ));
	}
	SEE_SET_NUMBER(res, d->t);
}

/* 15.9.5.2 */
static void
date_proto_toUTCString(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	SEE_SET_STRING(res, reprdatetime(interp, d->t, 1));
}

/* B.2.4 */
static void
date_proto_getYear(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);

	if (SEE_ISNAN(d->t))
		SEE_SET_NUMBER(res, SEE_NaN);
	else
		SEE_SET_NUMBER(res, 
		    (YearFromTime(LocalTime(interp, d->t)) - 1900));
}

/* B.2.5 */
static void
date_proto_setYear(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct date_object *d = todate(interp, thisobj);
	struct SEE_value v;
	SEE_number_t year;
	SEE_number_t t = LocalTime(interp, d->t);

	if (SEE_ISNAN(t)) t = 0;

	if (argc < 1)
		year = SEE_NaN;
	else {
		SEE_ToNumber(interp, argv[0], &v);
		year = v.u.number;
	}
	if (SEE_ISNAN(year)) {
		d->t = SEE_NaN;
	} else {
		if (0 <= year && year <= 99)
			year += 1900;
		d->t = TimeClip(UTC(interp, MakeDate(MakeDay(
			year, 
			MonthFromTime(t), 
			DateFromTime(t)),
			TimeWithinDay(t))
		       ));
	}
	SEE_SET_NUMBER(res, d->t);
}
