/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: interpreter.h 1126 2006-08-05 12:48:25Z d $ */

#ifndef _SEE_h_interpreter_
#define _SEE_h_interpreter_

#include "see_type.h"

struct SEE_object;
struct SEE_try_context;
struct SEE_throw_location;
struct SEE_context;
struct SEE_scope;

enum SEE_trace_event {
	SEE_TRACE_CALL,
	SEE_TRACE_RETURN,
	SEE_TRACE_STATEMENT,
	SEE_TRACE_THROW
};

/*
 * This is the place to stick things that, once created
 * for an interpreter instance, should be kept around
 * for easy access. (And cannot be replaced by scripts)
 */

struct SEE_interpreter {
	void *	host_data;		/* reserved for host app's use */
	int	compatibility;		/* compatibility flags (read-only?) */

	/* Built-in objects newly created for each interpreter instance */
	struct SEE_object *Global;
	struct SEE_object *Object;
	struct SEE_object *Object_prototype;
	struct SEE_object *Error;
	struct SEE_object *EvalError;
	struct SEE_object *RangeError;
	struct SEE_object *ReferenceError;
	struct SEE_object *SyntaxError;
	struct SEE_object *TypeError;
	struct SEE_object *URIError;
	struct SEE_object *String;
	struct SEE_object *String_prototype;
	struct SEE_object *Function;
	struct SEE_object *Function_prototype;
	struct SEE_object *Array;
	struct SEE_object *Array_prototype;
	struct SEE_object *Number;
	struct SEE_object *Number_prototype;
	struct SEE_object *Boolean;
	struct SEE_object *Boolean_prototype;
	struct SEE_object *Math;
	struct SEE_object *RegExp;
	struct SEE_object *RegExp_prototype;
	struct SEE_object *Date;
	struct SEE_object *Date_prototype;
	struct SEE_object *Global_eval;
	struct SEE_scope  *Global_scope;

	/* Current 'try-catch' context */
	volatile struct SEE_try_context * try_context;
	struct SEE_throw_location * try_location;

	/* Call traceback */
	struct SEE_traceback {
		struct SEE_throw_location *call_location;
		struct SEE_object *callee;
		int call_type;		/* SEE_CALLTYPE_* */
		struct SEE_traceback *prev;
	} *traceback;

	void **module_private;		/* private pointers for each module */
	void *intern_tab;		/* interned string table */
	unsigned int random_seed;	/* used by Math.random() */
	const char *locale;		/* current locale (may be NULL) */
	int recursion_limit;		/* -1 means don't care */
	void *sec_domain;		/* security domain inherited by new code */

	/* Trace hook, called by interpreter at each step if not NULL */
	void (*trace)(struct SEE_interpreter *, struct SEE_throw_location *,
			struct SEE_context *, enum SEE_trace_event);
};

/* Compatibility flags */
#define SEE_COMPAT_STRICT       0x0000  /* Strict ECMA-262 3rd ed. */
#define SEE_COMPAT_262_3B       (1<< 1) /* ECMA-262 3rd ed. Annex B */
#define SEE_COMPAT_UTF_UNSAFE   (1<< 2) /* accept 'valid but insecure' UTF */
#define SEE_COMPAT_SGMLCOM      (1<< 3) /* treat '<!--' as a '//' comment */
/* SEE_COMPAT_EXT1 deprecated; use SEE_COMPAT_JS11 instead */
#define SEE_COMPAT_JS_MASK      (7<< 5) /* mask for JS compat values  */
#define SEE_COMPAT_JS_NONE         0      /* no JS compat */
#define SEE_COMPAT_JS11           (1<< 5) /* JavaScript1.1 */
#define SEE_COMPAT_JS12           (2<< 5) /* JavaScript1.2 */
#define SEE_COMPAT_JS13           (3<< 5) /* JavaScript1.3 */
#define SEE_COMPAT_JS14           (4<< 5) /* JavaScript1.4 */
#define SEE_COMPAT_JS15           (5<< 5) /* JavaScript1.5 */

/* This macro is used to see if an ECMA deviation is required */
#define SEE_COMPAT_JS(i,cmp,jsnn) \
	((SEE_GET_JS_COMPAT(i) != SEE_COMPAT_JS_NONE) && \
	 (SEE_GET_JS_COMPAT(i) cmp SEE_COMPAT_##jsnn))
#define SEE_GET_JS_COMPAT(i) ((i)->compatibility & SEE_COMPAT_JS_MASK)
#define SEE_SET_JS_COMPAT(i,c) \
	(i)->compatibility = ((i)->compatibility & ~SEE_COMPAT_JS_MASK)|(c)

/* traceback call_type */
#define SEE_CALLTYPE_CALL	1
#define SEE_CALLTYPE_CONSTRUCT	2

/* Initialises an interpreter with default behaviour */
void SEE_interpreter_init(struct SEE_interpreter *i);

/* Initialises an interpreter with specific behaviour */
void SEE_interpreter_init_compat(struct SEE_interpreter *i, int compat_flags);

#endif /* _SEE_h_interpreter_ */
