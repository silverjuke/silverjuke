/* Automatically generated. Do not edit. */
/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: try.h.in 920 2005-12-24 10:30:35Z d $ */

#ifndef _SEE_h_try_
#define _SEE_h_try_

/*
 * Exception handling (try/catch)
 *
 * Usage example:
 *
 *	SEE_try_context_t c;
 *
 *	SEE_TRY(interp, c) {
 *		-- do code here that may call SEE_THROW()
 *		-- the try block breaks as soon as SEE_THROW happens
 *		-- NEVER 'return' or 'break' from inside a SEE_TRY!
 *		-- use TRY_BREAK or 'continue' to exit the SEE_TRY block.
 *	}
 *	-- 'finally' code gets run here always
 *	if (SEE_CAUGHT(c)) {
 *		-- this code run if a SEE_THROW was called during SEE_TRY 
 *		-- the exception thrown is SEE_CAUGHT(c)
 *		-- do a SEE_THROW(interp, SEE_CAUGHT(c)) if you can't handle it
 *	}
 *
 * If you only want a finally, and don't want to catch anything, use:
 *
 *	SEE_TRY(interp, c) {
 *		...
 *	}
 *	-- 'finally' code goes here
 *	SEE_DEFAULT_CATCH(interp, c);
 *
 * The signatures for the macros are:
 *
 *      struct SEE_value *SEE_CAUGHT(SEE_try_context_t);
 *      void SEE_THROW(struct SEE_interpreter *, struct SEE_value *);
 *      void SEE_DEFAULT_CATCH(SEE_interpreter *, SEE_try_context_t);
 *
 */

#include "see_type.h"
#include "see_value.h"

/*
 * Determine which setjmp/longjmp interface to use
 */

#if 0/*HAVE__LONGJMP*/
# include <setjmp.h>
# define _SEE_LONGJMP(buf, val)	_longjmp(buf, val)
# define _SEE_SETJMP(buf)	_setjmp(buf)
# define _SEE_JMPBUF		jmp_buf
#else
# if 1/*HAVE_LONGJMP*/
#  include <setjmp.h>
#  define _SEE_LONGJMP(buf, val)	longjmp(buf, val)
#  define _SEE_SETJMP(buf)	setjmp(buf)
#  define _SEE_JMPBUF		jmp_buf
# else
 # error "exception handling features requires longjmp/setjmp"
# endif
#endif

/*
 * Exception handling macros
 */

#define SEE_TRY(interp, c) 					\
	    for ((c).previous = (interp)->try_context,		\
		 (interp)->try_context = &(c),			\
		 (c).interpreter = (interp),			\
		 SEE_SET_NULL(&(c).thrown),			\
		 (c).done = 0;					\
		 !(c).done && (_SEE_SETJMP(			\
		    ((struct SEE_try_context *)&(c))->env) 	\
		   ? ((c).interpreter->try_context = 		\
		     (c).previous, 0)   			\
		   : 1);					\
		 (c).done = 1,					\
		 (c).interpreter->try_context = (c).previous)

#define TRY_BREAK						\
	    continue

#define SEE_CAUGHT(c)						\
	    ((struct SEE_value *)((c).done ? 0 : &(c).thrown))

#define SEE_THROW(interp, v) SEE__THROW(interp, v, __FILE__, __LINE__)

#define SEE__THROW(interp, v, _file, _line)			\
	    do {						\
		if (!(interp)->try_context)			\
			SEE_throw_abort(interp, v, _file, 	\
				_line);	\
		SEE_VALUE_COPY((struct SEE_value *)		\
			&(interp)->try_context->thrown, 	\
			(struct SEE_value *)v);			\
		(interp)->try_context->throw_file = _file;	\
		(interp)->try_context->throw_line = _line;	\
		SEE_throw();	/* debugger hook */		\
		_SEE_LONGJMP(((struct SEE_try_context *)	\
		      (interp)->try_context)->env, 1);		\
		/* NOTREACHED */				\
	    } while (0)

/* convenience macro */
#define SEE_DEFAULT_CATCH(interp, c)				\
	    do {						\
		if (!(c).done) 					\
		    SEE__THROW(interp, 				\
			    (struct SEE_value *)&(c).thrown,	\
			    (c).throw_file, (c).throw_line);	\
	    } while (0)

/*------------------------------------------------------------
 * private functions and externs used by the above macros
 */

struct SEE_throw_location {
	struct SEE_string *filename;		/* source location */
	int lineno;
};

struct SEE_try_context {
	struct SEE_interpreter *interpreter;
	volatile struct SEE_try_context *previous; /* try chain */
	struct SEE_value thrown;		/* value thrown during try */
	int done;				/* true if try completed */
	_SEE_JMPBUF env;			/* setjmp storage */
	const char *throw_file;			/* (debugging) */
	int throw_line;				/* (debugging) */
};

typedef struct SEE_try_context volatile SEE_try_context_t; 

struct SEE_interpreter;
void	SEE_throw_abort(struct SEE_interpreter *, const struct SEE_value *,
		const char *, int) SEE_dead;
struct SEE_string *SEE_location_string(struct SEE_interpreter *i,
		struct SEE_throw_location *loc);

/* Debugger hook for exceptions */
#ifndef NDEBUG
void	SEE_throw(void);
#else
# define SEE_throw() /* nothing */
#endif

#endif /* _SEE_h_try_ */
