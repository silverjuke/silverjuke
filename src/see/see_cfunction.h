/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: cfunction.h 1098 2006-07-28 15:20:53Z d $ */

#ifndef _SEE_h_cfunction_
#define _SEE_h_cfunction_

#include "see_object.h"

struct SEE_interpeter;
struct SEE_string;

/* Creates a new function object, implemented by func */
struct SEE_object *SEE_cfunction_make(struct SEE_interpreter *i,
	SEE_call_fn_t func, struct SEE_string *name, int length);

#define SEE_CFUNCTION_PUTA(interp, obj, name, func, length, attr) \
	do { 							\
		struct SEE_value _SEE_v;			\
		struct SEE_object *_SEE_obj;			\
		struct SEE_string *_SEE_name;			\
		_SEE_name = SEE_intern_ascii(interp, name);	\
		_SEE_obj = SEE_cfunction_make(interp, func, 	\
			_SEE_name, length);			\
		SEE_SET_OBJECT(&_SEE_v, _SEE_obj);		\
		SEE_OBJECT_PUT(interp, obj, _SEE_name, &_SEE_v, \
			attr);					\
	} while (0)

void SEE_parse_args(struct SEE_interpreter *i, int argc, 
	struct SEE_value **argv, const char *fmt, ...);

#endif /* _SEE_h_cfunction_ */
