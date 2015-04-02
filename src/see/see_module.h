/* Copyright (c) 2006, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _SEE_h_module_
#define _SEE_h_module_

struct SEE_interpreter;

struct SEE_module {
	SEE_uint32_t	  magic;
	const char 	 *name;
	const char 	 *version;
	unsigned int	  index;	/* Set by SEE_module_add() */
	int		(*mod_init)(void);
	void		(*alloc)(struct SEE_interpreter *);
	void		(*init)(struct SEE_interpreter *);
};
#define SEE_MODULE_MAGIC (SEE_uint32_t)0x5345456d

int SEE_module_add(struct SEE_module *module);

/* Reference to the per-module private field of an interpreter */
#define SEE_MODULE_PRIVATE(interp, module)	\
	((interp)->module_private[(module)->index])

#endif

