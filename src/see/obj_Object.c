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
/* $Id: obj_Object.c 1092 2006-06-21 14:46:28Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
#endif

#include "see_mem.h"
#include "see_native.h"
#include "see_string.h"
#include "see_intern.h"
#include "see_cfunction.h"
#include "see_interpreter.h"

#include "see_stringdefs.h"
#include "see_init.h"

/*
 * Object objects.
 *
 * This module declares two objects:
 *   - the 'object constructor object' (a.k.a Object)
 *   - the 'object prototype object' (a.k.a. Object.prototype)
 *
 * 'new Object(o)' returns new objects, whose prototype is 
 * set to Object.prototype. Note that 'Object' can be called
 * either as a function, or as a constructor with the same
 * result.
 */

extern struct SEE_native SEE_obj_Function_prototype;
extern struct SEE_native SEE_obj_Object;

/* Prototypes */
static void object_construct(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void object_proto_toString(struct SEE_interpreter *, 
        struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
        struct SEE_value *);
static void object_proto_toLocaleString(struct SEE_interpreter *, 
        struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
        struct SEE_value *);
static void object_proto_valueOf(struct SEE_interpreter *, 
        struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
        struct SEE_value *);
static void object_proto_hasOwnProperty(struct SEE_interpreter *, 
        struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
        struct SEE_value *);
static void object_proto_isPrototypeOf(struct SEE_interpreter *, 
        struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
        struct SEE_value *);
static void object_proto_propertyIsEnumerable(struct SEE_interpreter *, 
        struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
        struct SEE_value *);

static struct SEE_objectclass object_const_class = {
	"ObjectConstructor",			/* Class */
	SEE_native_get,				/* Get */
	SEE_native_put,				/* Put */
	SEE_native_canput,			/* CanPut */
	SEE_native_hasproperty,			/* HasProperty */
	SEE_native_delete,			/* Delete */
	SEE_native_defaultvalue,		/* DefaultValue */
	SEE_native_enumerator,			/* enumerator */
	object_construct,			/* Construct */
	object_construct,			/* Call */
};

static struct SEE_objectclass object_inst_class = {
	"Object",				/* Class */
	SEE_native_get,				/* Get */
	SEE_native_put,				/* Put */
	SEE_native_canput,			/* CanPut */
	SEE_native_hasproperty,			/* HasProperty */
	SEE_native_delete,			/* Delete */
	SEE_native_defaultvalue,		/* DefaultValue */
	SEE_native_enumerator,			/* enumerator */
};

void
SEE_Object_alloc(interp)
	struct SEE_interpreter *interp;
{
	interp->Object =
		(struct SEE_object *)SEE_NEW(interp, struct SEE_native);
	interp->Object_prototype =
		(struct SEE_object *)SEE_NEW(interp, struct SEE_native);
}

void
SEE_Object_init(interp)
	struct SEE_interpreter *interp;
{
	struct SEE_object *Object;			/* struct SEE_native */
	struct SEE_object *Object_prototype;		/* struct SEE_native */
	struct SEE_value v;

	Object = interp->Object;
	Object_prototype = interp->Object_prototype;

	SEE_native_init((struct SEE_native *)Object, interp, 
		&object_const_class, interp->Function_prototype);

	SEE_native_init((struct SEE_native *)Object_prototype, interp, 
		&object_inst_class, NULL);

	/* Object.prototype.constructor */
	SEE_SET_OBJECT(&v, Object);
	SEE_OBJECT_PUT(interp, Object_prototype, STR(constructor), &v, 
		SEE_ATTR_DEFAULT);

#define PUTFUNC(name, len) \
    SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, object_proto_##name, 	\
	STR(name), len)); 						\
    SEE_OBJECT_PUT(interp, Object_prototype, STR(name), &v, 		\
	SEE_ATTR_DEFAULT);

	PUTFUNC(toString, 0)
	PUTFUNC(toLocaleString, 0)
	PUTFUNC(valueOf, 0)
	PUTFUNC(hasOwnProperty, 1)
	PUTFUNC(isPrototypeOf, 1)
	PUTFUNC(propertyIsEnumerable, 1)

	/* 15.2.3.1 Object.prototype */
	SEE_SET_OBJECT(&v, Object_prototype);
	SEE_OBJECT_PUT(interp, Object, STR(prototype), &v, 
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY);

	/* Object.length */
	SEE_SET_NUMBER(&v, 1);
	SEE_OBJECT_PUT(interp, Object, STR(length), &v, SEE_ATTR_LENGTH);
}

/* Convenience routine to simulare 'new Object()' */
struct SEE_object *
SEE_Object_new(interp)
	struct SEE_interpreter *interp;
{
	struct SEE_value v;

	SEE_OBJECT_CONSTRUCT(interp, interp->Object, interp->Object, 0, 
		NULL, &v);
	return v.u.object;
}

/* new Object  (15.2.2.1) */
static void
object_construct(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	struct SEE_object *obj;

	if (argc) {
		if (SEE_VALUE_GET_TYPE(argv[0]) == SEE_OBJECT) {
			/*
			 * TODO optional step 4 - handling host objects.
			 * Should we add a hook into the objectclass structure
			 * to allow hosts to trap this new Object(hostobj)
			 * situation? Is it really that important?? Hmm.
			 */
			SEE_SET_OBJECT(res, argv[0]->u.object);
			return;
		}
		if (SEE_VALUE_GET_TYPE(argv[0]) == SEE_STRING ||
		    SEE_VALUE_GET_TYPE(argv[0]) == SEE_BOOLEAN ||
		    SEE_VALUE_GET_TYPE(argv[0]) == SEE_NUMBER)
		{
			SEE_ToObject(interp, argv[0], res);
			return;
		}
	}

	obj = SEE_native_new(interp);
	obj->objectclass = &object_inst_class;
	obj->Prototype = interp->Object_prototype;
	SEE_SET_OBJECT(res, obj);
}


/* Object.prototype.toString (15.2.4.2) */
static void
object_proto_toString(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	struct SEE_string *s;

        if (SEE_COMPAT_JS(interp, ==, JS12)) {
                struct SEE_string *s = SEE_string_new(interp, 0);
                struct SEE_string *prop;
                struct SEE_value v, vs;
                unsigned int j;
		int flag;
		int first = 1;
		struct SEE_enum *e = NULL;

                SEE_string_addch(s, '{');
		if (SEE_OBJECT_HAS_ENUMERATOR(thisobj)) {
		    e = SEE_OBJECT_ENUMERATOR(interp, thisobj);
		    while ((prop = SEE_ENUM_NEXT(interp, e, &flag)) != NULL) {
			SEE_OBJECT_GET(interp, thisobj, prop, &v);
			if (SEE_VALUE_GET_TYPE(&v) == SEE_UNDEFINED)
			    continue;
			if (!first) {
			    SEE_string_addch(s, ',');
			    SEE_string_addch(s, ' ');
			} else
			    first = 0;
			SEE_string_append(s, prop);
			SEE_string_addch(s, ':');
			switch (SEE_VALUE_GET_TYPE(&v)) {
			case SEE_STRING:
			    SEE_string_addch(s, '"');
			    for (j = 0; j < v.u.string->length; j++) {
				if (v.u.string->data[j] == '\"' ||
				    v.u.string->data[j] == '\\')
					SEE_string_addch(s, '\\');
				SEE_string_addch(s, v.u.string->data[j]);
			    }
			    SEE_string_addch(s, '"');
			    break;
			default:
			    SEE_ToString(interp, &v, &vs);
			    SEE_string_append(s, vs.u.string);
			    break;
			}
		    }
		}
                SEE_string_addch(s, '}');
                SEE_SET_STRING(res, s);
        } else {
		s = SEE_string_sprintf(interp, "[object %s]",
			(thisobj && thisobj->objectclass && 
			 thisobj->objectclass->Class)
			? thisobj->objectclass->Class 
			: "(null)");
		SEE_SET_STRING(res, s);
	}
}

/* Object.prototype.toLocaleString (15.2.4.3) */
static void
object_proto_toLocaleString(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	struct SEE_value v1, v2;

	/* "return the result of calling toString()" */
	SEE_OBJECT_GET(interp, thisobj, STR(toString), &v1);
	SEE_ToObject(interp, &v1, &v2);
	SEE_OBJECT_CALL(interp, v2.u.object, thisobj, argc, argv, res);
}

/* Object.prototype.valueOf (15.2.4.4) */
static void
object_proto_valueOf(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	if (!thisobj)
		SEE_SET_NULL(res);
	else
		SEE_SET_OBJECT(res, thisobj);
}

/* Object.prototype.hasOwnProperty (15.2.4.5) */
static void
object_proto_hasOwnProperty(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	struct SEE_value v;
	struct SEE_string *is;
	int hasit;

	/* XXX - should be a nicer way of determining how to do this: */
	if (argc > 0 && 
	    thisobj->objectclass->HasProperty == SEE_native_hasproperty)
	{
		SEE_ToString(interp, argv[0], &v);
		is = SEE_intern(interp, v.u.string);
		hasit = SEE_native_hasownproperty(interp, thisobj, is);
	} else
		hasit = 0;
	SEE_SET_BOOLEAN(res, hasit);
}

/* Object.prototype.isPrototypeOf (15.2.4.6) Pp97 */
static void
object_proto_isPrototypeOf(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	struct SEE_object *v;

	if (argc == 0 || SEE_VALUE_GET_TYPE(argv[0]) != SEE_OBJECT) {
		SEE_SET_BOOLEAN(res, 0);
		return;
	}
	v = argv[0]->u.object->Prototype;
	if (v == NULL)
		SEE_SET_BOOLEAN(res, 0);
	else if (SEE_OBJECT_JOINED(thisobj, v)) 
		SEE_SET_BOOLEAN(res, 1);
	else
		SEE_SET_BOOLEAN(res, 0);
}

/* Object.prototype.propertyIsEnumerable (15.2.4.7) */
static void
object_proto_propertyIsEnumerable(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv;
	struct SEE_value *res;
{
	struct SEE_value v;
	struct SEE_string *is;
	int isenum = 0;

	if (argc > 0 &&
	    thisobj->objectclass->HasProperty == SEE_native_hasproperty)
	{
		SEE_ToString(interp, argv[0], &v);
		is = SEE_intern(interp, v.u.string);
		isenum = SEE_native_hasownproperty(interp, thisobj, is)
		    && (SEE_native_getownattr(interp, thisobj, is) & 
			SEE_ATTR_DONTENUM) == 0;
	}
	SEE_SET_BOOLEAN(res, isenum);
}

