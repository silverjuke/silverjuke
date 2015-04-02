/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: intern.h 992 2006-01-31 15:04:16Z d $ */

#ifndef _SEE_h_intern_
#define _SEE_h_intern_

struct SEE_interpreter;
struct SEE_string;

void _SEE_intern_init(struct SEE_interpreter *i);

/*
 * Internalises a string local to the intepreter. Returns a string
 * with the same content so that pointer inequality implies 
 * content inequality.
 */
struct SEE_string *SEE_intern(struct SEE_interpreter *i, struct SEE_string *s);

/*
 * Internalises a string, and frees the original string.
 * The string s must not be used by anywhere else at this stage.
 */
void SEE_intern_and_free(struct SEE_interpreter *i, struct SEE_string **s);

/*
 * Returns an interned SEE_string containing the ASCII string promoted to
 * unicode. All the characters in s must be ASCII (ie between 1 and 127 incl.)
 */
struct SEE_string *SEE_intern_ascii(struct SEE_interpreter *i, const char *s);

/*
 * Internalises an ASCII string into the global table. 
 * Invalid if interpreter instances exist.
 */
struct SEE_string *SEE_intern_global(const char *s);

#endif /* _SEE_h_intern_ */
