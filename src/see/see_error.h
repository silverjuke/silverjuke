/* Automatically generated. Do not edit. */
/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: error.h.in 820 2005-12-05 13:57:36Z d $ */

#ifndef _SEE_h_error_
#define _SEE_h_error_

#include "see_type.h"
#include "see_native.h"

struct SEE_object;
struct SEE_string;
struct SEE_interpreter;

/*
 * Convenience error throwing.
 * These functions call the given object's constructor with
 * the string as a single argument, and then throws the 
 * resulting, constructed object.
 */
void SEE_error__throw_string(struct SEE_interpreter *i,
			struct SEE_object *errorobj, 
			const char *filename, int lineno, 
			struct SEE_string *message) SEE_dead;
void SEE_error__throw(struct SEE_interpreter *i,
			struct SEE_object *errorobj, 
			const char *filename, int lineno, 
			const char *fmt, ...) SEE_dead;
void SEE_error__throw_sys(struct SEE_interpreter *i,
			struct SEE_object *errorobj, 
			const char *filename, int lineno, 
			const char *fmt, ...) SEE_dead;

struct SEE_object * SEE_Error_make(struct SEE_interpreter *i,
			struct SEE_string *name);

#ifndef NDEBUG
# define SEE_error_throw_string(i, o, s) \
	 SEE_error__throw_string(i, o, __FILE__, __LINE__, s)
#else
# define SEE_error_throw_string(i, o, s) \
	 SEE_error__throw_string(i, o, 0, 0, s)
#endif

/* Determine how variadic macros are available */

#if __STDC_VERSION__ >= 199901L
/* C99-style variadic macros */

# ifndef NDEBUG
#  define SEE_error_throw(i, o, ...) \
	  SEE_error__throw(i, o, __FILE__, __LINE__, __VA_ARGS__)
#  define SEE_error_throw_sys(i, o, ...) \
	  SEE_error__throw_sys(i, o, __FILE__, __LINE__, __VA_ARGS__)
# else
#  define SEE_error_throw(i, o, ...) \
	  SEE_error__throw(i, o, 0, 0, __VA_ARGS__)
#  define SEE_error_throw_sys(i, o, ...) \
	  SEE_error__throw_sys(i, o, 0, 0, __VA_ARGS__)
# endif

#else
# if 0/*HAVE_VARIADIC_MACROS*/
/* GNU-style variadic macros */

#  ifndef NDEBUG
#   define SEE_error_throw(i, o, fmt, arg...) \
	   SEE_error__throw(i, o, __FILE__, __LINE__, fmt , ## arg)
#   define SEE_error_throw_sys(i, o, fmt, arg...) \
	  SEE_error__throw_sys(i, o, __FILE__, __LINE__, fmt , ## arg)
#  else
#   define SEE_error_throw(i, o, fmt, arg...) \
	   SEE_error__throw(i, o, 0, 0, fmt , ## arg)
#   define SEE_error_throw_sys(i, o, fmt, arg...) \
	   SEE_error__throw_sys(i, o, 0, 0, fmt , ## arg)
#  endif

# else
/*
 * Without variadic macros, we cannot insert file and line
 * information into a call to an intercepted variadic function call.
 */

#  define SEE_error_throw SEE_error__throw0
#  define SEE_error_throw_sys SEE_error__throw_sys0

# endif
#endif

/* 
 * The following functions are only useful when variadic macros are 
 * not available.
 */
void SEE_error__throw0(struct SEE_interpreter *i,
			struct SEE_object *errorobj, 
			const char *fmt, ...) SEE_dead;
void SEE_error__throw_sys0(struct SEE_interpreter *i,
			struct SEE_object *errorobj, 
			const char *fmt, ...) SEE_dead;

/*
 * An assertion macro.
 */
#ifndef NDEBUG
#  define SEE_ASSERT(i, x)						\
    do {								\
	if (!(x))							\
	    SEE_error_throw(i, (i)->Error,				\
		"%s:%d: assertion '%s' failed",				\
		__FILE__, __LINE__, #x);				\
    } while (0)
#else /* NDEBUG */
#  define SEE_ASSERT(i, x) /* ignore */
#endif /* NDEBUG */

#endif /* _SEE_h_error_ */
