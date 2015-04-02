/*
 * Copyright (c) 2006
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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "see_type.h"
#include "see_error.h"
#include "see_interpreter.h"

#include "see_dprint.h"
#include "see_platform.h"

/* Returns the current right now in milliseconds since Jan 1 1970 UTC 0:00 */
SEE_number_t
_SEE_platform_time(interp)
	struct SEE_interpreter *interp;
{
#if HAVE_GETTIMEOFDAY
	struct timeval tv;

	if (gettimeofday(&tv, NULL) < 0)
		SEE_error_throw_sys(interp, interp->Error, "gettimeofday");
	return tv.tv_sec * 1e3 + tv.tv_usec * 1e-3;
#else
# if HAVE_TIME
	return time(0) * 1000.0;
# else
 # warning "no time() or gettimeofday(); accessing time will throw an error"
	SEE_error_throw_sys(interp, interp->Error, "System time unavailable");
# endif
#endif
}

SEE_number_t
_SEE_platform_tza(interp)
	struct SEE_interpreter *interp;
{
#if HAVE_LOCALTIME
	static int initialized = 0;
	static SEE_number_t tza;

	if (!initialized) {	/* XXX not thread safe */
		time_t time0 = 0, diff;
		struct tm *tm;
		
		#pragma warning(disable : 4996) // disable the warning "localtime is unsecure"
		tm = localtime(&time0);
	        diff = tm->tm_sec + 60 * (tm->tm_min + tm->tm_hour * 60);
		if (tm->tm_year < 0)
			diff = diff - 24 * 60 * 60;
		tza = diff * 1000.0;
		initialized = 1;
	}
	return tza;
#else
 # warning "no localtime(); effective timezone has been set to UTC"
 	return 0;
#endif
}

/* 
 * Compute the daylight savings adjustment.
 * Because of standards madness (15.9.1.9[8])
 * we must translate the date in question to an
 * 'equivalent' year of a fixed era. I've chosen the 
 * fourteen years near the current year.
 * Once the translation is done, we then figure out what
 * the difference between dst and non-dst times are, using the
 * system's timezone databases.
 */
SEE_number_t
_SEE_platform_dst(interp, ysec, ily, wstart)
	struct SEE_interpreter *interp;
	SEE_number_t ysec;
	int ily, wstart;
{
#if HAVE_MKTIME
	struct tm tm;
	time_t dst_time, nodst_time;
	time_t s = ysec / 1000.0;
	int jday, mon, mday;

	static unsigned int yearmap[2][7] = {
	    { 2006, 2007, 2002, 2003, 2009, 1999, 2005 },
	    { 1984, 1996, 2008, 1992, 2004, 1988, 2000 }
	};

	memset(&tm, 0, sizeof tm);
	tm.tm_sec = s % 60;
	tm.tm_min = (s / 60) % 60;
	tm.tm_hour = (s / (60 * 60)) % 24;
	jday = s / (60 * 60 * 24);

	if (jday < 31)           { mon =  0; mday = jday + 1; }
	else if (jday < 59+ily)  { mon =  1; mday = jday - 30; }
	else if (jday < 90+ily)  { mon =  2; mday = jday - 58 - ily; }
	else if (jday < 120+ily) { mon =  3; mday = jday - 89 - ily; }
	else if (jday < 151+ily) { mon =  4; mday = jday - 119 - ily; }
	else if (jday < 181+ily) { mon =  5; mday = jday - 150 - ily; }
	else if (jday < 212+ily) { mon =  6; mday = jday - 180 - ily; }
	else if (jday < 243+ily) { mon =  7; mday = jday - 211 - ily; }
	else if (jday < 273+ily) { mon =  8; mday = jday - 242 - ily; }
	else if (jday < 304+ily) { mon =  9; mday = jday - 272 - ily; }
	else if (jday < 334+ily) { mon = 10; mday = jday - 303 - ily; }
	else if (jday < 365+ily) { mon = 11; mday = jday - 334 - ily; }
	else
	    SEE_error_throw_sys(interp, interp->Error, "_SEE_platform_dst");

	tm.tm_mday = mday;
	tm.tm_mon = mon;
	tm.tm_year = yearmap[ily][wstart] - 1900;

	tm.tm_isdst = -1;
	dst_time = mktime(&tm);

	tm.tm_isdst = 0;
	nodst_time = mktime(&tm);

	return (dst_time - nodst_time) * 1000;
#else
 # warning "no mktime(); daylight savings adjustments have been disabled"
 	return 0;
#endif
}

/* Abort the current system with a message */
void
_SEE_platform_abort(interp, msg)
	struct SEE_interpreter *interp;
	const char *msg;
{
#if STDC_HEADERS
	if (msg)
		fprintf(stderr, "fatal error: %s\n", msg);
	fflush(stderr);
	fflush(stdout);
#endif
#if HAVE_ABORT
	abort();
#else
# ifndef NDEBUG
	dprintf("fatal error: %s\n", msg);
# endif
	exit(1);
#endif
}
