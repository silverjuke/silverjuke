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
/* $Id: mem.c 1065 2006-04-19 14:29:27Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "see_mem.h"
#include "see_system.h"

#include "see_dprint.h"

#ifndef NDEBUG
int SEE_mem_debug = 0;
#endif

#undef SEE_malloc
#undef SEE_malloc_finalize
#undef SEE_malloc_string
#undef SEE_free

/*------------------------------------------------------------
 * Wrappers around memory allocators that check for failure
 */

/*
 * Allocates size bytes of garbage-collected storage.
 */
void *
SEE_malloc(interp, size)
	struct SEE_interpreter *interp;
	SEE_size_t size;
{
	void *data;

	if (size == 0)
		return NULL;
	data = (*SEE_system.malloc)(interp, size);
	if (data == NULL) 
		(*SEE_system.mem_exhausted)(interp);
	return data;
}

/*
 * Allocates size bytes of garbage-collected storage, attaching
 * a finalizer function that will be called when the storage becomes
 * unreachable.
 */
void *
SEE_malloc_finalize(interp, size, finalizefn, closure)
	struct SEE_interpreter *interp;
	SEE_size_t size;
	void (*finalizefn)(struct SEE_interpreter *, void *, void *);
	void *closure;
{
	void *data;

	if (size == 0)
		return NULL;
	data = (*SEE_system.malloc_finalize)(interp, size, finalizefn, closure);
	if (data == NULL) 
		(*SEE_system.mem_exhausted)(interp);
	return data;
}

/*
 * Allocates size bytes of garbage-collected, string storage.
 * This function is just like SEE_malloc(), except that the caller
 * guarantees that no pointers will be stored in the data. This
 * improves performance with strings.
 */
void *
SEE_malloc_string(interp, size)
	struct SEE_interpreter *interp;
	SEE_size_t size;
{
	void *data;

	if (size == 0)
		return NULL;
	data = (*SEE_system.malloc_string)(interp, size);
	if (data == NULL) 
		(*SEE_system.mem_exhausted)(interp);
	return data;
}

/*
 * Releases memory that the caller *knows* is unreachable.
 */
void
SEE_free(interp, memp)
	struct SEE_interpreter *interp;
	void **memp;
{
	if (*memp) {
		(*SEE_system.free)(interp, *memp);
		*memp = NULL;
	}
}

/*
 * Forces a garbage collection, or does nothing if unsupported.
 */
void
SEE_gcollect(interp)
	struct SEE_interpreter *interp;
{
	if (SEE_system.gcollect)
		(*SEE_system.gcollect)(interp);
}

/*
 * The debug variants must not be protected by NDEBUG.
 * This is for the case when the library is compiled with NDEBUG,
 * but the library-user application is not.
 */

void *
_SEE_malloc_debug(interp, size, file, line, arg)
	struct SEE_interpreter *interp;
	SEE_size_t size;
	const char *file;
	int line;
	const char *arg;
{
	void *data;

#ifndef NDEBUG
	if (SEE_mem_debug)
		dprintf("malloc %u (%s:%d '%s')", size, file, line, arg);
#endif
	data = SEE_malloc(interp, size);
#ifndef NDEBUG
	if (SEE_mem_debug)
		dprintf(" -> %p\n", data);
#endif
	return data;
}

void *
_SEE_malloc_finalize_debug(interp, size, finalizefn, closure, file, line, arg)
	struct SEE_interpreter *interp;
	SEE_size_t size;
	void (*finalizefn)(struct SEE_interpreter *, void *, void *);
	void *closure;
	const char *file;
	int line;
	const char *arg;
{
	void *data;

#ifndef NDEBUG
	if (SEE_mem_debug)
		dprintf("malloc %u (%s:%d '%s')", size, file, line, arg);
#endif
	data = SEE_malloc_finalize(interp, size, finalizefn, closure);
#ifndef NDEBUG
	if (SEE_mem_debug)
		dprintf(" -> %p\n", data);
#endif
	return data;
}

void *
_SEE_malloc_string_debug(interp, size, file, line, arg)
	struct SEE_interpreter *interp;
	SEE_size_t size;
	const char *file;
	int line;
	const char *arg;
{
	void *data;

#ifndef NDEBUG
	if (SEE_mem_debug)
		dprintf("malloc_string %u (%s:%d '%s')", size, file, 
			line, arg);
#endif
	data = SEE_malloc_string(interp, size);
#ifndef NDEBUG
	if (SEE_mem_debug)
		dprintf(" -> %p\n", data);
#endif
	return data;
}

void
_SEE_free_debug(interp, memp, file, line, arg)
	struct SEE_interpreter *interp;
	void **memp;
	const char *file;
	int line;
	const char *arg;
{
#ifndef NDEBUG
	if (SEE_mem_debug)
		dprintf("free %p (%s:%d '%s')", *memp, file, line, arg);
#endif
	SEE_free(interp, memp);
}
