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
/* $Id: obj_Boolean.c 1021 2006-02-08 15:40:35Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "see_mem.h"
#include "see_value.h"
#include "see_string.h"
#include "see_object.h"
#include "see_native.h"
#include "see_cfunction.h"
#include "see_error.h"
#include "see_interpreter.h"

#include "see_stringdefs.h"
#include "see_init.h"

/*
 * 15.6 The Boolean object.
 */

/* structure of boolean instances */
struct boolean_object {
	struct SEE_native native;
	SEE_boolean_t boolean;		/* Value */
};

static struct boolean_object *toboolean(struct SEE_interpreter *,
	struct SEE_object *);

static void boolean_construct(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);
static void boolean_call(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);

static void boolean_proto_toString(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *,
	int, struct SEE_value **, struct SEE_value *);
static void boolean_proto_valueOf(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);

/* object class for Boolean constructor */
static struct SEE_objectclass boolean_const_class = {
	"BooleanConstructor",		/* Class */
	SEE_native_get,			/* Get */
	SEE_native_put,			/* Put */
	SEE_native_canput,		/* CanPut */
	SEE_native_hasproperty,		/* HasProperty */
	SEE_native_delete,		/* Delete */
	SEE_native_defaultvalue,	/* DefaultValue */
	SEE_native_enumerator,		/* DefaultValue */
	boolean_construct,		/* Construct */
	boolean_call			/* Call */
};

/* object class for Boolean.prototype and number instances */
struct SEE_objectclass _SEE_boolean_inst_class = {
	"Boolean",			/* Class */
	SEE_native_get,			/* Get */
	SEE_native_put,			/* Put */
	SEE_native_canput,		/* CanPut */
	SEE_native_hasproperty,		/* HasProperty */
	SEE_native_delete,		/* Delete */
	SEE_native_defaultvalue,	/* DefaultValue */
	SEE_native_enumerator		/* enumerator */
};

void
SEE_Boolean_alloc(interp)
	struct SEE_interpreter *interp;
{
	interp->Boolean = 
	    (struct SEE_object *)SEE_NEW(interp, struct SEE_native);
	interp->Boolean_prototype = 
	    (struct SEE_object *)SEE_NEW(interp, struct boolean_object);
}

void
SEE_Boolean_init(interp)
	struct SEE_interpreter *interp;
{
	struct SEE_object *Boolean;		/* struct SEE_native */
	struct SEE_object *Boolean_prototype;	/* struct boolean_object */
	struct SEE_value v;

	Boolean = interp->Boolean;
	SEE_native_init((struct SEE_native *)Boolean, interp,
		&boolean_const_class, interp->Function_prototype);

	Boolean_prototype = interp->Boolean_prototype;

	/* 15.6.3 Boolean.length = 1 */
	SEE_SET_NUMBER(&v, 1);
	SEE_OBJECT_PUT(interp, Boolean, STR(length), &v, SEE_ATTR_LENGTH);

	/* 15.6.3.1 Boolean.prototype */
	SEE_SET_OBJECT(&v, Boolean_prototype);
	SEE_OBJECT_PUT(interp, Boolean, STR(prototype), &v,
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY);

	SEE_native_init((struct SEE_native *)Boolean_prototype, interp,
		&_SEE_boolean_inst_class, interp->Object_prototype); /* 15.6.4 */
	((struct boolean_object *)Boolean_prototype)->boolean = 0; /* 15.6.4 */

	/* 15.6.4.1 Boolean.prototype.constructor */
	SEE_SET_OBJECT(&v, Boolean);
	SEE_OBJECT_PUT(interp, Boolean_prototype, STR(constructor), &v, 
		SEE_ATTR_DEFAULT);

#define PUTFUNC(name, len)						    \
	SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, boolean_proto_##name, \
		STR(name), len));					    \
	SEE_OBJECT_PUT(interp, Boolean_prototype, STR(name), &v, 	    \
		SEE_ATTR_DEFAULT);

	PUTFUNC(toString, 0)
	PUTFUNC(valueOf, 0)
}

static struct boolean_object *
toboolean(interp, o)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
{
	if (o->objectclass != &_SEE_boolean_inst_class)
		SEE_error_throw_string(interp, interp->TypeError, 
		   STR(not_boolean));
	return (struct boolean_object *)o;
}

/* 15.6.2.1 */
static void
boolean_construct(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	struct boolean_object *bo;
	struct SEE_value v;

	if (argc == 0)
		SEE_SET_BOOLEAN(&v, 0);
	else
		SEE_ToBoolean(interp, argv[0], &v);

	bo = SEE_NEW(interp, struct boolean_object);
	SEE_native_init(&bo->native, interp, &_SEE_boolean_inst_class,
		interp->Boolean_prototype);
	bo->boolean = v.u.boolean;

	SEE_SET_OBJECT(res, (struct SEE_object *)bo);
}

/* 15.6.1.1 */
static void
boolean_call(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	if (argc == 0)
		SEE_SET_BOOLEAN(res, 0);
	else 
		SEE_ToBoolean(interp, argv[0], res);
}

/* 15.6.4.2 Boolean.prototype.toString() */
static void
boolean_proto_toString(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct boolean_object *bo;

	bo = toboolean(interp, thisobj);
	SEE_SET_STRING(res, bo->boolean ? STR(true) : STR(false));
}

/* 15.6.4.3 Boolean.prototype.valueOf() */
static void
boolean_proto_valueOf(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct boolean_object *bo;

	bo = toboolean(interp, thisobj);
	SEE_SET_BOOLEAN(res, bo->boolean);
}
