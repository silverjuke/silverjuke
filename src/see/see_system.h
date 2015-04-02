/* Copyright (c) 2005, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _SEE_h_system_
#define _SEE_h_system_

struct SEE_interpreter;
struct SEE_throw_location;
struct SEE_context;

#include "see_interpreter.h"		/* for enum SEE_trace_event */
#include "see_type.h"

struct SEE_system {

	/* Interpreter field defaults */
	const char *default_locale;		/* default: NULL */
	int default_recursion_limit;		/* default: -1 (no limit) */
	void (*default_trace)(struct SEE_interpreter *, 
		struct SEE_throw_location *,
		struct SEE_context *,
		enum SEE_trace_event);		/* default: NULL */
	int default_compat_flags;

	unsigned int (*random_seed)(void);

	/* Fatal error handler */
	void (*abort)(struct SEE_interpreter *, 
		const char *) SEE_dead;

	/* Periodic execution callback */
	void (*periodic)(struct SEE_interpreter *);

	/* Memory allocator */
	void *(*malloc)(struct SEE_interpreter *, SEE_size_t);
	void *(*malloc_finalize)(struct SEE_interpreter *, SEE_size_t,
		void (*)(struct SEE_interpreter *, void *, void *), void *);
	void *(*malloc_string)(struct SEE_interpreter *, SEE_size_t);

	void (*free)(struct SEE_interpreter *, void *);
	void (*mem_exhausted)(struct SEE_interpreter *) SEE_dead;
	void (*gcollect)(struct SEE_interpreter *);

	/* Security domain tracking */
	void *(*transit_sec_domain)(struct SEE_interpreter *, void *);

};

extern struct SEE_system SEE_system;

#define SEE_ABORT(interp, msg) (*SEE_system.abort)(interp, msg)

#endif /* _SEE_h_interpreter_ */
