/* Copyright 2006, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _SEE_h_platform_
#define _SEE_h_platform_

#include <stdarg.h>
#include "see_type.h"

/* Returns the time right now in milliseconds since Jan 1 1970 UTC 0:00 */
SEE_number_t _SEE_platform_time(struct SEE_interpreter *interp);

/* Returns the local timezone adjustment in milliseconds */
SEE_number_t _SEE_platform_tza(struct SEE_interpreter *interp);

/* Returns the daylight saving time adjustment in msec for a local time */
SEE_number_t _SEE_platform_dst(struct SEE_interpreter *interp,
		SEE_number_t ysec, int ily, int wstart);

/* Abort the current system with a message */
void _SEE_platform_abort(struct SEE_interpreter *interp, const char *msg);

#endif /* _SEE_h_platform_ */
