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
/* $Id$ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "see_interpreter.h"
#include "see_module.h"
#include "see_mem.h"

#include "see_init.h"

#ifndef MAXMODULES
# define MAXMODULES 256
#endif

struct SEE_module *_SEE_modules[MAXMODULES];
unsigned int _SEE_nmodules;

/*
 * Calls a modules mod_init, and if it returns 0, adds to the module list.
 * Returns -1 if out of memory, or the return code of mod_init().
 */
int
SEE_module_add(module)
	struct SEE_module *module;
{
	int ret;
	unsigned int i, index;

	for (i = 0; i < _SEE_nmodules; i++)
		if (_SEE_modules[i] == module)
			return i;

	if (_SEE_nmodules >= MAXMODULES)
		return -1;
	
	index = _SEE_nmodules++;
	_SEE_modules[index] = module;
	module->index = index;
	ret = (*module->mod_init)();
	if (ret != 0)
		_SEE_nmodules = index;
	return ret;
}

/* Calls each module's alloc() */
void
_SEE_module_alloc(interp)
	struct SEE_interpreter *interp;
{
	unsigned int i;

	interp->module_private = SEE_NEW_ARRAY(interp, void *, _SEE_nmodules);
	for (i = 0; i < _SEE_nmodules; i++)
		if (_SEE_modules[i]->alloc)
			(*_SEE_modules[i]->alloc)(interp);
}

/* Calls each module's init() */
void
_SEE_module_init(interp)
	struct SEE_interpreter *interp;
{
	unsigned int i;

	for (i = 0; i < _SEE_nmodules; i++)
		if (_SEE_modules[i]->init)
			(*_SEE_modules[i]->init)(interp);
}
