/* Copyright (c) 2005, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _SEE_h_cfunction_private_
#define _SEE_h_cfunction_private_

struct SEE_interpreter;
struct SEE_object;
struct SEE_value;
struct SEE_string;

struct SEE_string *SEE_cfunction_getname(struct SEE_interpreter *i,
        struct SEE_object *o);

void SEE_cfunction_toString(struct SEE_interpreter *,
    struct SEE_object *, struct SEE_object *,
    int, struct SEE_value **, struct SEE_value *);

#endif /* _SEE_h_cfunction_private_ */
