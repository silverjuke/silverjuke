/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: mem.h 1065 2006-04-19 14:29:27Z d $ */

#ifndef _SEE_h_mem_
#define _SEE_h_mem_

#include "see_type.h"

struct SEE_interpreter;

void *	SEE_malloc(struct SEE_interpreter *i, SEE_size_t sz) 
		_SEE_malloc;
void *	SEE_malloc_string(struct SEE_interpreter *i, SEE_size_t sz) 
		_SEE_malloc;
void *	SEE_malloc_finalize(struct SEE_interpreter *i, SEE_size_t sz,
		void (*finalizefn)(struct SEE_interpreter *i, void *p,
			void *closure), void *closure) 
		_SEE_malloc;
void  	SEE_free(struct SEE_interpreter *i, void **memp);
void  	SEE_gcollect(struct SEE_interpreter *i);

#ifndef NDEBUG
/* Debugging variants */
void *	_SEE_malloc_debug(struct SEE_interpreter *i, SEE_size_t sz, 
		const char *file, int line, const char *arg);
void *	_SEE_malloc_string_debug(struct SEE_interpreter *i, SEE_size_t sz, 
		const char *file, int line, const char *arg);
void *	_SEE_malloc_finalize_debug(struct SEE_interpreter *i, SEE_size_t sz,
		void (*finalizefn)(struct SEE_interpreter *i, void *p,
			void *closure), void *closure, 
			const char *file, int line, const char *arg);
void 	_SEE_free_debug(struct SEE_interpreter *i, void **memp,
		const char *file, int line, const char *arg);
#define SEE_malloc(i,s) \
		_SEE_malloc_debug(i,s,__FILE__,__LINE__,#s)
#define SEE_malloc_string(i,s) \
		_SEE_malloc_string_debug(i,s,__FILE__,__LINE__,#s)
#define SEE_malloc_finalize(i,s,f,c) \
		_SEE_malloc_finalize_debug(i,s,f,c,__FILE__,__LINE__,\
			#s "," #f "," #c)
#define SEE_free(i,p) \
		_SEE_free_debug(i,p,__FILE__,__LINE__,#p)
#endif

/* Convenience macros */
#define SEE_NEW(i, t)		(t *)SEE_malloc(i, sizeof (t))
#define SEE_NEW_FINALIZE(i, t, f, c) \
				(t *)SEE_malloc_finalize(i, sizeof (t), f, c)
#define SEE_NEW_ARRAY(i, t, n)	(t *)SEE_malloc(i, (n) * sizeof (t))
#define SEE_NEW_STRING_ARRAY(i, t, n) \
				(t *)SEE_malloc_string(i, (n) * sizeof (t))

#endif /* _SEE_h_mem_ */
