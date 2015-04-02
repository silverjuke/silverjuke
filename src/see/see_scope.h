/* Copyright (c) 2005, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _SEE_h_scope_
#define _SEE_h_scope_

struct SEE_interpreter;
struct SEE_object;
struct SEE_string;
struct SEE_value;

/* Execution scope chain */
struct SEE_scope {
	struct SEE_scope *next;
	struct SEE_object *obj;
};

void SEE_scope_lookup(struct SEE_interpreter *, struct SEE_scope *scope,
	struct SEE_string *name, struct SEE_value *res);
int SEE_scope_eq(struct SEE_scope *scope1, struct SEE_scope *scope2);


#endif /* _SEE_h_context_ */
