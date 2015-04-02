/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: native.h 920 2005-12-24 10:30:35Z d $ */

#ifndef _SEE_h_native_
#define _SEE_h_native_

/*
 * 'Native' is an object class that implements objects as
 * a hash table of name-value pairs called 'properties'. This
 * is the normal, expected behaviour of objects in ECMAScript.
 * Properties can have attributes that prevent them being deleted 
 * modified, or visible with the 'for prop in object' enumerator.
 */

#include "see_value.h"
#include "see_object.h"

/* Default attributes for the built-in properties described in the standard */
#define SEE_ATTR_DEFAULT	(SEE_ATTR_DONTENUM)	/* (see section 15) */

/* Default attributes for the 'length' property of functions */
#define SEE_ATTR_LENGTH		(SEE_ATTR_READONLY |	\
				 SEE_ATTR_DONTDELETE |	\
				 SEE_ATTR_DONTENUM)

struct SEE_interpreter;
struct SEE_property;

/* A native object is a primitive object plus a hash table of properties */
#define SEE_NATIVE_HASHLEN  257
struct SEE_native {
	struct SEE_object       object;
	struct SEE_property *   properties[SEE_NATIVE_HASHLEN];
};

/* Object class methods that assume the object is a struct SEE_native */
void SEE_native_get(struct SEE_interpreter *i, struct SEE_object *obj, 
	struct SEE_string *prop, struct SEE_value *res);
void SEE_native_put(struct SEE_interpreter *i, struct SEE_object *obj, 
	struct SEE_string *prop, struct SEE_value *val, int flags);
int  SEE_native_canput(struct SEE_interpreter *i, struct SEE_object *obj, 
	struct SEE_string *prop);
int  SEE_native_hasproperty(struct SEE_interpreter *i, struct SEE_object *obj, 
	struct SEE_string *prop);
int  SEE_native_hasownproperty(struct SEE_interpreter *i,
	struct SEE_object *obj, struct SEE_string *prop);
int  SEE_native_getownattr(struct SEE_interpreter *i, struct SEE_object *obj, 
	struct SEE_string *prop);
int  SEE_native_delete(struct SEE_interpreter *i, struct SEE_object *obj, 
	struct SEE_string *prop);
void SEE_native_defaultvalue(struct SEE_interpreter *i, struct SEE_object *obj,
	struct SEE_value *hint, struct SEE_value *res);
struct SEE_enum *SEE_native_enumerator(struct SEE_interpreter *i, 
	struct SEE_object *obj);

/* Allocate and initialise a new native object, with NULL prototype */
struct SEE_object *SEE_native_new(struct SEE_interpreter *i);

/* Initialise a SEE_native object. Useful when subtyping SEE_native */
void SEE_native_init(struct SEE_native *obj, struct SEE_interpreter *i,
		struct SEE_objectclass *obj_class, 
		struct SEE_object *prototype);

#endif /* _SEE_h_native_ */
