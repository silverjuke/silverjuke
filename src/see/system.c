/*
 * Copyright (c) 2005
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
/* $Id$ */

#include "see_type.h"

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
#endif

#if HAVE_TIME
# if TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
# else
#  if HAVE_SYS_TIME_H
#   include <sys/time.h>
#  else
#   include <time.h>
#  endif
# endif
#endif

#if HAVE_GC_H
# include "gc/gc.h"
#else
# if HAVE_GC_MALLOC
extern void *GC_malloc(int);
# endif
# if HAVE_GC_MALLOC_ATOMIC
extern void *GC_malloc_atomic(int);
# endif
# if HAVE_GC_FREE
extern void GC_free(void *);
# endif
#endif

#include "see_interpreter.h"
#include "see_system.h"

#include "see_dprint.h"
#include "see_platform.h"

/* Prototypes */
static unsigned int simple_random_seed(void);
#if HAVE_GC_MALLOC
static void *simple_gc_malloc(struct SEE_interpreter *, SEE_size_t);
static void *simple_gc_malloc_string(struct SEE_interpreter *, SEE_size_t);
static void *simple_gc_malloc_finalize(struct SEE_interpreter *, SEE_size_t,
        void (*)(struct SEE_interpreter *, void *, void *), void *);
static void simple_gc_free(struct SEE_interpreter *, void *);
static void simple_finalizer(GC_PTR, GC_PTR);
static void simple_gc_gcollect(struct SEE_interpreter *);
#else
static void *simple_malloc(struct SEE_interpreter *, SEE_size_t);
static void *simple_malloc_finalize(struct SEE_interpreter *, SEE_size_t,
        void (*)(struct SEE_interpreter *, void *, void *), void *);
static void simple_free(struct SEE_interpreter *, void *);
#endif
static void simple_mem_exhausted(struct SEE_interpreter *) SEE_dead;

/*
 * System defaults. This structure should be not be modified after
 * interpreters are created.
 */
struct SEE_system SEE_system = {
	NULL,				/* default_locale */
	-1,				/* default_recursion_limit */
	NULL,				/* default_trace */

	SEE_COMPAT_262_3B, 		/* default_compat_flags */

	simple_random_seed,		/* random_seed */

	_SEE_platform_abort,		/* abort */
	NULL,				/* periodic */

#if HAVE_GC_MALLOC
	simple_gc_malloc,		/* malloc */
	simple_gc_malloc_finalize,	/* malloc_finalize */
	simple_gc_malloc_string,	/* malloc_string */
	simple_gc_free,			/* free */
	simple_mem_exhausted,		/* mem_exhausted */
	simple_gc_gcollect,		/* gcollect */
#else
	simple_malloc,			/* malloc */
	simple_malloc_finalize,		/* malloc_finalize */
	simple_malloc,			/* malloc_string */
	simple_free,			/* free */
	simple_mem_exhausted,		/* mem_exhausted */
	NULL,				/* gcollect */
#endif
	NULL				/* transit_sec_domain */
};

/*
 * A simple random number seed generator. It is not thread safe.
 */
static unsigned int
simple_random_seed()
{
	static unsigned int counter = 0;
	unsigned int r;

	r = counter++;
#if HAVE_TIME
	r += (unsigned int)time(0);
#endif
	return r;
}

/*
 * A simple memory exhausted handler.
 */
static void
simple_mem_exhausted(interp)
	struct SEE_interpreter *interp;
{
	SEE_ABORT(interp, "memory exhausted");
}


#if HAVE_GC_MALLOC
/*
 * Memory allocator using Boehm GC
 */
static void *
simple_gc_malloc(interp, size)
	struct SEE_interpreter *interp;
	SEE_size_t size;
{
	return GC_malloc(size);
}

/*
 * A private structure to hold finalization info. This is appended onto
 * objects that are allocated with finalization requirements.
 */
struct finalize_info {
	struct SEE_interpreter *interp;
        void (*finalizefn)(struct SEE_interpreter *, void *, void *);
	void *closure;
};

static void
simple_finalizer(p, cd)
	GC_PTR p, cd;
{
	struct finalize_info *info = (struct finalize_info *)cd;
	
	(*info->finalizefn)(info->interp, (void *)p, info->closure);
}

static void *
simple_gc_malloc_finalize(interp, size, finalizefn, closure)
	struct SEE_interpreter *interp;
	SEE_size_t size;
        void (*finalizefn)(struct SEE_interpreter *, void *, void *);
	void *closure;
{
	SEE_size_t padsz;
	void *data;
	struct finalize_info *info;

	/* Round up to align the finalize_info */
	padsz = size;
	padsz += sizeof (struct finalize_info) - 1;
	padsz -= padsz % sizeof (struct finalize_info);

	/* Allocate, with space for the finalize_info */
	data = GC_malloc(padsz + sizeof (struct finalize_info));

	/* Fill in the finalize_info now */
	info = (struct finalize_info *)((char *)data + padsz);
	info->interp = interp;
	info->finalizefn = finalizefn;
	info->closure = closure;

	/* Ask the GC to call the finalizer when data is unreachable */
	GC_register_finalizer(data, simple_finalizer, info, NULL, NULL);

	return data;
}

/*
 * Non-pointer memory allocator using Boehm GC
 */
static void *
simple_gc_malloc_string(interp, size)
	struct SEE_interpreter *interp;
	SEE_size_t size;
{
# if HAVE_GC_MALLOC_ATOMIC
	return GC_malloc_atomic(size);
# else
	return GC_malloc(size);
# endif
}

/*
 * Non-pointer memory allocator using Boehm GC
 */
static void
simple_gc_free(interp, ptr)
	struct SEE_interpreter *interp;
	void *ptr;
{
# if HAVE_GC_FREE
	GC_free(ptr);
# endif
}

static void
simple_gc_gcollect(interp)
	struct SEE_interpreter *interp;
{
# if HAVE_GC_GCOLLECT
	GC_gcollect();
# endif
}

#else /* !HAVE_GC_MALLOC */


/*
 * Memory allocator using system malloc().
 * Note: most mallocs do not get freed! 
 * System strongly assumes a garbage collector.
 * This is a stub function.
 */
static void *
simple_malloc(interp, size)
	struct SEE_interpreter *interp;
	SEE_size_t size;
{
#ifndef NDEBUG
	static int warning_printed = 0;

	if (!warning_printed) {
		warning_printed++;
		dprintf("WARNING: SEE is using non-release malloc\n");
	}

#endif
	return malloc(size);
}

static void *
simple_malloc_finalize(interp, size, finalizefn, closure)
	struct SEE_interpreter *interp;
	SEE_size_t size;
        void (*finalizefn)(struct SEE_interpreter *, void *, void *);
	void *closure;
{
#ifndef NDEBUG
	static int warning_printed = 0;

	if (!warning_printed) {
		warning_printed++;
		dprintf("WARNING: SEE is using non-finalize malloc\n");
	}

#endif
	return malloc(size);
}

/*
 * Memory deallocator using system free().
 */
static void
simple_free(interp, ptr)
	struct SEE_interpreter *interp;
	void *ptr;
{
	free(ptr);
}

#endif /* !HAVE_GC_MALLOC */

