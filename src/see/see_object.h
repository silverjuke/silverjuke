/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: object.h 1108 2006-08-01 14:22:09Z d $ */

#ifndef _SEE_h_object_
#define _SEE_h_object_

struct SEE_value;
struct SEE_object;
struct SEE_string;
struct SEE_scope;
struct SEE_enum;
struct SEE_interpreter;

/* 
 * Class method types. Even though ECMAScript uses a prototype
 * inheritance model, objects still have to carry something like
 * a vtbl.
 */
typedef void	(*SEE_get_fn_t)(struct SEE_interpreter *i,
			struct SEE_object *obj, struct SEE_string *prop,
			struct SEE_value *res);
typedef void	(*SEE_put_fn_t)(struct SEE_interpreter *i,
			struct SEE_object *obj, struct SEE_string *prop,
			struct SEE_value *res, int flags);
typedef int	(*SEE_boolean_fn_t)(struct SEE_interpreter *i,
			struct SEE_object *obj, struct SEE_string *prop);
typedef int	(*SEE_hasinstance_fn_t)(struct SEE_interpreter *i,
			struct SEE_object *obj, struct SEE_value *instance);
typedef void	(*SEE_default_fn_t)(struct SEE_interpreter *i,
			struct SEE_object *obj, struct SEE_value *hint, 
			struct SEE_value *res);
typedef void	(*SEE_call_fn_t)(struct SEE_interpreter *i, 
			struct SEE_object *obj, struct SEE_object *thisobj,
			int argc, struct SEE_value **argv,
			struct SEE_value *res);
typedef struct SEE_enum *(*SEE_enumerator_fn_t)(struct SEE_interpreter *i,
			struct SEE_object *obj);
typedef void *	(*SEE_get_sec_domain_fn_t)(struct SEE_interpreter *i,
			struct SEE_object *obj);

/*
 * Object classes: an object insatnce appears as a container of named
 * properties accessible through methods provided by its object class.
 * Object classes are a SEE implementation feature and not directly visible
 * to ECMAScript programs.
 *
 * All object classes must implement: Prototype, Class, Get, Put, CanPut,
 * HasProperty, Delete and DefaultValue. (DefaultValue may simply
 * throw a TypeError, and Proptype may be NULL)
 * Optionally, object classes can implement the enumerator, Construct, Call
 * or HasInstance. Unimplemented optional methods are indicated as NULL.
 */
struct SEE_objectclass {
	const char *		Class;			/* [[Class]] */
	SEE_get_fn_t		Get;			/* [[Get]] */
	SEE_put_fn_t		Put;			/* [[Put]] */
	SEE_boolean_fn_t	CanPut;			/* [[CanPut]] */
	SEE_boolean_fn_t	HasProperty;		/* [[HasProperty]] */
	SEE_boolean_fn_t	Delete;			/* [[Delete]] */
	SEE_default_fn_t	DefaultValue;		/* [[DefaultValue]] */
	SEE_enumerator_fn_t	enumerator;		/* enumerator */
	SEE_call_fn_t		Construct;		/* [[Construct]] */
	SEE_call_fn_t		Call;			/* [[Call]] */
	SEE_hasinstance_fn_t	HasInstance;		/* [[HasInstance]] */
	SEE_get_sec_domain_fn_t	get_sec_domain;		/* get_sec_domain */
};

/*
 * Base object structure. This structure is not generally useful
 * unless extended (see struct SEE_native in <see/native.h>).
 */
struct SEE_object {
	struct SEE_objectclass *objectclass;
	struct SEE_object *	Prototype;		/* [[Prototype]] */
};

#define SEE_OBJECT_GET(interp, obj, name, res)				\
	(*(obj)->objectclass->Get)(interp, obj, name, res)
#define SEE_OBJECT_PUT(interp, obj, name, val, attrs)			\
	(*(obj)->objectclass->Put)(interp, obj, name, val, attrs)
#define SEE_OBJECT_CANPUT(interp, obj, name)				\
	(*(obj)->objectclass->CanPut)(interp, obj, name)
#define SEE_OBJECT_HASPROPERTY(interp, obj, name)			\
	(*(obj)->objectclass->HasProperty)(interp, obj, name)
#define SEE_OBJECT_DELETE(interp, obj, name)				\
	(*(obj)->objectclass->Delete)(interp, obj, name)
#define SEE_OBJECT_DEFAULTVALUE(interp, obj, hint, res)			\
	(*(obj)->objectclass->DefaultValue)(interp, obj, hint, res)
#define SEE_OBJECT_CONSTRUCT(interp, obj, thisobj, argc, argv, res)	\
	SEE_object_construct(interp, obj, thisobj, argc, argv, res)
#define _SEE_OBJECT_CONSTRUCT(interp, obj, thisobj, argc, argv, res)	\
	(*(obj)->objectclass->Construct)(interp, obj, thisobj, argc, argv, res)
#define SEE_OBJECT_CALL(interp, obj, thisobj, argc, argv, res)		\
	SEE_object_call(interp, obj, thisobj, argc, argv, res)
#define _SEE_OBJECT_CALL(interp, obj, thisobj, argc, argv, res)		\
	(*(obj)->objectclass->Call)(interp, obj, thisobj, argc, argv, res)
#define SEE_OBJECT_HASINSTANCE(interp, obj, instance)			\
	(*(obj)->objectclass->HasInstance)(interp, obj, instance)
#define SEE_OBJECT_ENUMERATOR(interp, obj)				\
	(*(obj)->objectclass->enumerator)(interp, obj)
#define SEE_OBJECT_GET_SEC_DOMAIN(interp, obj)				\
	(*(obj)->objectclass->get_sec_domain)(interp, obj)

/* Convenience macros that use ASCII C strings for names */
struct SEE_string *SEE_intern_ascii(struct SEE_interpreter *, const char *);
#define SEE_OBJECT_GETA(interp, obj, name, res)				\
	SEE_OBJECT_GET(interp, obj, SEE_intern_ascii(interp, name), res)
#define SEE_OBJECT_PUTA(interp, obj, name, val, attrs)			\
	SEE_OBJECT_PUT(interp, obj, SEE_intern_ascii(interp, name), val, attrs)
#define SEE_OBJECT_CANPUTA(interp, obj, name)				\
	SEE_OBJECT_CANPUT(interp, obj, SEE_intern_ascii(interp, name))
#define SEE_OBJECT_HASPROPERTYA(interp, obj, name)			\
	SEE_OBJECT_HASPROPERTY(interp, obj, SEE_intern_ascii(interp, name))
#define SEE_OBJECT_DELETEA(interp, obj, name)				\
	SEE_OBJECT_DELETE(interp, obj, SEE_intern_ascii(interp, name))

#define SEE_OBJECT_HAS_CALL(obj)	((obj)->objectclass->Call)
#define SEE_OBJECT_HAS_CONSTRUCT(obj)	((obj)->objectclass->Construct)
#define SEE_OBJECT_HAS_HASINSTANCE(obj)	((obj)->objectclass->HasInstance)
#define SEE_OBJECT_HAS_ENUMERATOR(obj)	((obj)->objectclass->enumerator)
#define SEE_OBJECT_HAS_GET_SEC_DOMAIN(obj) ((obj)->objectclass->get_sec_domain)

/* [[Put]] attributes */
#define SEE_ATTR_READONLY   0x01
#define SEE_ATTR_DONTENUM   0x02
#define SEE_ATTR_DONTDELETE 0x04
#define SEE_ATTR_INTERNAL   0x08

/* Enumerator class. */
struct SEE_enumclass {
	void *unused; 		/* XXX leftover from deprecated reset method */
	struct SEE_string *(*next)(struct SEE_interpreter *i,
			struct SEE_enum *e, int *flags_return);
};

/*
 * Enumerator instance. This structure is generally subclassed to
 * hold enumeration state.
 */
struct SEE_enum {
	struct SEE_enumclass *enumclass;
};

#define SEE_ENUM_NEXT(i,e,dep) ((e)->enumclass->next)(i,e,dep)

/*
 * This macro tests to see if two objects are "joined", i.e. a change to one
 * is reflected in the other. This is only useful with function 
 * objects that share a common function implementation.
 */
#define SEE_OBJECT_JOINED(a,b)					\
	((a) == (b) || 						\
	  ((a)->objectclass == (b)->objectclass &&		\
	   SEE_function_is_joined(a,b)))
int SEE_function_is_joined(struct SEE_object *a, struct SEE_object *b);

/* A convenience function equivalent to "new Object()" */
struct SEE_object *SEE_Object_new(struct SEE_interpreter *);

/*
 * Wrappers around [[Call]] and [[Construct]] that check for 
 * recursion limits being reached and to keep track of the security
 * domains.
 */
void SEE_object_call(struct SEE_interpreter *, struct SEE_object *,
	struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
void SEE_object_construct(struct SEE_interpreter *, struct SEE_object *,
	struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

#endif /* _SEE_h_object_ */
