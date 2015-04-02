/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: function.h 1108 2006-08-01 14:22:09Z d $ */

#ifndef _SEE_h_function_
#define _SEE_h_function_

struct SEE_interpreter;
struct SEE_string;
struct SEE_scope;
struct SEE_native;
struct SEE_object;
struct SEE_context;

/* Linked list of variable declarations, or formal parameter names */
struct var {
	struct SEE_string *name;
	struct var *next;		/* linked list of vars */
};

struct function {
	int nparams;
	struct SEE_string **params;
	void *body;			/* FunctionBody_node */
	struct SEE_string *name;	/* optional function name */
	struct SEE_object *common;	/* common to joined functions */
	struct SEE_object *cache;	/* used by SEE_Function_create() */
	struct function *next;		/* linked list of functions */
	int is_empty;			/* true if body is empty */
	void *sec_domain;		/* security domain active when defined */
};

struct function *SEE_function_make(struct SEE_interpreter *i,
	struct SEE_string *name, struct var *vars, void *node);
void SEE_function_put_args(struct SEE_context *i, struct function *func,
	int argc, struct SEE_value **argv);

extern struct SEE_objectclass SEE_activation_class;
#define IS_ACTIVATION_OBJECT(o) ((o)->objectclass == &SEE_activation_class)
struct SEE_object *SEE_activation_new(struct SEE_interpreter * i);

/* obj_Function.c */
struct SEE_object *SEE_function_inst_create(struct SEE_interpreter *i,
	struct function *func, struct SEE_scope *scope);
struct SEE_string *SEE_function_getname(struct SEE_interpreter * i,
        struct SEE_object *o);
/* cfunction.c */
struct SEE_string *SEE_cfunction_getname(struct SEE_interpreter *i,
        struct SEE_object *o);

/* cfunction.h */
void SEE_cfunction_toString(struct SEE_interpreter *,
    struct SEE_object *, struct SEE_object *,
    int, struct SEE_value **, struct SEE_value *);

#endif /* _SEE_h_function_ */
