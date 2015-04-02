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
/* $Id: obj_Error.c 991 2006-01-31 01:53:46Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <string.h>
# include <stdarg.h>
#endif

#include "see_value.h"
#include "see_object.h"
#include "see_cfunction.h"
#include "see_native.h"
#include "see_string.h"
#include "see_mem.h"
#include "see_debug.h"
#include "see_interpreter.h"
#include "see_error.h"

#include "see_stringdefs.h"
#include "see_init.h"
#include "see_dprint.h"

#ifndef NDEBUG
int SEE_Error_debug = 0;
#endif

/*
 * Error objects
 *
 * These objects are thrown by the runtime system in the event of
 * errors. User programs also use these as prototypes for their
 * own errors.
 *
 * -- 15.11
 */

/* Prototypes */
static struct SEE_object *init_error(struct SEE_interpreter *,
	struct SEE_string *, struct SEE_object *, struct SEE_object *);
static void error_proto_toString(struct SEE_interpreter *, 
	struct SEE_object *, struct SEE_object *, int, struct SEE_value **,
	struct SEE_value *);
static void error_construct(struct SEE_interpreter *, struct SEE_object *,
	struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

/* object class for Error constructors */
static struct SEE_objectclass error_const_class = {
	"ErrorConstructor",			/* Class */
	SEE_native_get,				/* Get */
	SEE_native_put,				/* Put */
	SEE_native_canput,			/* CanPut */
	SEE_native_hasproperty,			/* HasProperty */
	SEE_native_delete,			/* Delete */
	SEE_native_defaultvalue,		/* DefaultValue */
	SEE_native_enumerator,			/* enumerator */
	error_construct,			/* Call */
	error_construct,			/* Construct */
};

/* object class for Error.prototype */
static struct SEE_objectclass error_inst_class = {
	"Error",				/* Class */
	SEE_native_get,				/* Get */
	SEE_native_put,				/* Put */
	SEE_native_canput,			/* CanPut */
	SEE_native_hasproperty,			/* HasProperty */
	SEE_native_delete,			/* Delete */
	SEE_native_defaultvalue,		/* DefaultValue */
	SEE_native_enumerator,			/* enumerator */
};

/*
 * helper function to initialise the standard Error and 
 * NativeError objects
 */
static struct SEE_object *
init_error(interp, name, constructor, proto_proto)
	struct SEE_interpreter *interp;
	struct SEE_string *name;
	struct SEE_object *constructor;
	struct SEE_object *proto_proto;
{
	struct SEE_value v;
	struct SEE_object *proto;

	/* Construct the prototype object */
	proto = SEE_native_new(interp);
	proto->objectclass = &error_inst_class;
	proto->Prototype = proto_proto;			 /* 15.11.4 */

	/* 15.11.4.1 Error.prototype.constructor = Error */
	/* 15.11.7.8 NativeError.prototype.constructor = Error */
	SEE_SET_OBJECT(&v, constructor);
	SEE_OBJECT_PUT(interp, proto, STR(constructor), &v, SEE_ATTR_DEFAULT);

	/* 15.11.4.2 Error.prototype.name = "Error" */
	/* 15.11.7.9 NativeError.prototype.name = "NativeError" */
	SEE_SET_STRING(&v, name);
	SEE_OBJECT_PUT(interp, proto, STR(name), &v, SEE_ATTR_DEFAULT);

	/* 15.11.4.3 Error.prototype.message = "Error" */
	/* 15.11.7.10 NativeError.prototype.message = "NativeError" */
	SEE_SET_STRING(&v, name);
	SEE_OBJECT_PUT(interp, proto, STR(message), &v, SEE_ATTR_DEFAULT);

	/* 15.11.7.7 and 15.11.3 */
	SEE_native_init((struct SEE_native *)constructor, interp, 
	    &error_const_class, interp->Function_prototype);

	/* 15.11.3 Error.length = 1 */
	SEE_SET_NUMBER(&v, 1);
	SEE_OBJECT_PUT(interp, constructor, STR(length), &v, SEE_ATTR_LENGTH);

	/* 15.11.3.1 Error.prototype */
	/* 15.11.7.6 NativeError.prototype */
	SEE_SET_OBJECT(&v, proto);
	SEE_OBJECT_PUT(interp, constructor, STR(prototype), &v, 
		SEE_ATTR_DONTENUM | SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY);

	return proto;
}

void
SEE_Error_alloc(interp)
	struct SEE_interpreter *interp;
{
	interp->Error = 
		(struct SEE_object *)SEE_NEW(interp, struct SEE_native);
	interp->EvalError = 
		(struct SEE_object *)SEE_NEW(interp, struct SEE_native);
	interp->RangeError = 
		(struct SEE_object *)SEE_NEW(interp, struct SEE_native);
	interp->ReferenceError = 
		(struct SEE_object *)SEE_NEW(interp, struct SEE_native);
	interp->SyntaxError = 
		(struct SEE_object *)SEE_NEW(interp, struct SEE_native);
	interp->TypeError = 
		(struct SEE_object *)SEE_NEW(interp, struct SEE_native);
	interp->URIError = 
		(struct SEE_object *)SEE_NEW(interp, struct SEE_native);
}

void
SEE_Error_init(interp)
	struct SEE_interpreter *interp;
{
	struct SEE_value v;
	struct SEE_object *Error_prototype;

	Error_prototype = init_error(interp, STR(Error), interp->Error, 
		interp->Object_prototype);

	/* 15.11.4.4 Error.prototype.toString */
	SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, error_proto_toString,
		STR(toString), 0));
	SEE_OBJECT_PUT(interp, Error_prototype, STR(toString), &v, 
		SEE_ATTR_DEFAULT);

	init_error(interp, STR(EvalError), interp->EvalError,
		Error_prototype);			 /* 15.1.6.1 */
	init_error(interp, STR(RangeError), interp->RangeError,
		Error_prototype);			 /* 15.1.6.2 */
	init_error(interp, STR(ReferenceError), interp->ReferenceError, 
		Error_prototype);			 /* 15.1.6.3 */
	init_error(interp, STR(SyntaxError), interp->SyntaxError,
		Error_prototype);			 /* 15.1.6.4 */
	init_error(interp, STR(TypeError), interp->TypeError,
		Error_prototype);			 /* 15.1.6.5 */
	init_error(interp, STR(URIError), interp->URIError,
		Error_prototype);			 /* 15.1.6.6 */
}

/*
 * Host error constructor maker
 */
struct SEE_object *
SEE_Error_make(interp, name)
	struct SEE_interpreter *interp;
	struct SEE_string *name;
{
	struct SEE_object *Error_prototype;
	struct SEE_value v;
	struct SEE_object *new_error;

	SEE_OBJECT_GET(interp, interp->Error, STR(prototype), &v);
	Error_prototype = v.u.object;
	new_error = (struct SEE_object *)SEE_NEW(interp, struct SEE_native);
	init_error(interp, name, new_error, Error_prototype);
	return new_error;
}

/*
 * Return the string form of an error object.
 * -- 15.11.4.4
 * This implementation returns the error' class name, 
 * followed by a colon and the message text (or just the class name
 * if there is no message text).
 */
static void
error_proto_toString(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc; 
	struct SEE_value **argv, *res;
{
	struct SEE_value name, message;
	struct SEE_string *s;

	SEE_OBJECT_GET(interp, thisobj, STR(name), &name);
	SEE_OBJECT_GET(interp, thisobj, STR(message), &message);

#ifndef NDEBUG
	if (SEE_Error_debug) {
	    dprintf("Error.prototype.toString: self=%p this=%p name=",
		self,thisobj);
	    dprintv(interp, &name);
	    dprintf(", message=");
	    dprintv(interp, &message);
	    dprintf("\n");
	}
#endif

	s = SEE_string_new(interp, 0);
	if (SEE_VALUE_GET_TYPE(&name) == SEE_STRING) 
	    SEE_string_append(s, name.u.string);
	else
	    SEE_string_append(s, STR(Error));
	if (SEE_VALUE_GET_TYPE(&message) == SEE_STRING && 
	    message.u.string->length > 0) 
	{
	    SEE_string_addch(s, ':');
	    SEE_string_addch(s, ' ');
	    SEE_string_append(s, message.u.string);
	}

	SEE_SET_STRING(res, s);
}

/*
 * Construct an error object.
 * This implementation creates a new native object,
 * setting its [[Prototype]] to the self's 'prototype'
 * property. It then sets the message property of the new
 * object to the message text.
 * Extra arguments are ignored.
 *
 * -- 15.11.2, 15.11.7
 */
static void
error_construct(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc; 
	struct SEE_value **argv, *res;
{
	struct SEE_value msg, protov;
	struct SEE_object *proto;
	struct SEE_native *obj;

	SEE_OBJECT_GET(interp, self, STR(prototype), &protov);

#ifndef NDEBUG
	if (SEE_Error_debug) {
	    dprintf("error_construct: this.prototype=");
	    dprintv(interp, &protov);
	    dprintf("\n");
	}
#endif

	if (SEE_VALUE_GET_TYPE(&protov) == SEE_OBJECT)
		proto = protov.u.object;
	else
		proto = NULL;			/* XXX should abort? */

	obj = SEE_NEW(interp, struct SEE_native);
	SEE_native_init(obj, interp, &error_inst_class, proto);

	if (argc > 0 && SEE_VALUE_GET_TYPE(argv[0]) != SEE_UNDEFINED) {
	    SEE_ToString(interp, argv[0], &msg);
	    SEE_OBJECT_PUT(interp, (struct SEE_object *)obj, STR(message),
		    &msg, SEE_ATTR_DEFAULT);

#ifndef NDEBUG
	    if (SEE_Error_debug) {
		dprintf("error_construct: put obj.message=");
		dprintv(interp, &msg);
		dprintf("\n");
		SEE_OBJECT_GET(interp, (struct SEE_object *)obj, 
		    STR(message), &msg);
		dprintf("error_construct: get obj.message=");
		dprintv(interp, &msg);
		dprintf("\n");
		dprintf("error_construct: self=%p this=%p result=%p\n",
		    self, thisobj, obj);
	    }
#endif

	} 

	SEE_SET_OBJECT(res, (struct SEE_object *)obj);
}
