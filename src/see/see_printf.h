/* Copyright (c) 2006, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _SEE_h_printf
#define _SEE_h_printf

#include <stdarg.h>
struct SEE_string;
struct SEE_interpreter;

void _SEE_vsprintf(struct SEE_interpreter *interp, struct SEE_string *,
			  const char *fmt, va_list ap);

#endif /* _SEE_h_printf */
