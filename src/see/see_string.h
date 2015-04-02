/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: string.h 1098 2006-07-28 15:20:53Z d $ */

#ifndef _SEE_h_string_
#define _SEE_h_string_

/*
 * Strings are in-memory, sized arrays of 16-bit characters.
 * They have length and content, and may be 'owned' by the intern system.
 *
 * Like objects, a string class provides an implementation.
 * The implementation can optionally provide a 'growby' method that
 * is used by SEE_string_append() and SEE_string_addch() to ensure the
 * string length can be incremented by a given amount;
 * although care must be taken to ensure that the grown
 * string's data is not in use elsewhere. 
 * Practically there are only two string implementations: growable and 
 * non-growable.
 *
 * The lifetime of a string is as it passes through the following stages:
 *   1. new, mutable; can be modified, grown and kept private
 *   2. public, immutable; must not be changed, can be referenced anywhere
 *   3. unreachable; garbage collector determines when to deallocate string
 */

#include <stdio.h>
#include <stdarg.h>

#include "see_type.h"

struct SEE_stringclass;

struct SEE_string {
	unsigned int		 length;
	SEE_char_t		*data;
	struct SEE_stringclass	*stringclass;	/* NULL means static */
	struct SEE_interpreter	*interpreter;
	int 			 flags;
};
#define SEE_STRING_FLAG_INTERNED  1
#define SEE_STRING_FLAG_STATIC    2

#define SEE_STRING_DECL(chararray) \
	{ sizeof (chararray) / sizeof (SEE_char_t), (chararray), \
	  NULL, NULL, SEE_STRING_FLAG_STATIC }

struct SEE_stringclass {
	void (*growby)(struct SEE_string *, unsigned int);
};

void	SEE_string_addch(struct SEE_string *s, /* SEE_char_t */ int ch);
void	SEE_string_append(struct SEE_string *s, const struct SEE_string *sffx);
void	SEE_string_append_ascii(struct SEE_string *s, const char *ascii);
void	SEE_string_append_int(struct SEE_string *s, int i);
int	SEE_string_fputs(const struct SEE_string *s, FILE *file);
int	SEE_string_cmp(const struct SEE_string *s1,
			  const struct SEE_string *s2);

struct SEE_string *SEE_string_new(struct SEE_interpreter *i,
				unsigned int space);
struct SEE_string *SEE_string_dup(struct SEE_interpreter *i,
				const struct SEE_string *s);
struct SEE_string *SEE_string_substr(struct SEE_interpreter *i,
				struct SEE_string *s, int index, int length);
struct SEE_string *SEE_string_concat(struct SEE_interpreter *i,
				struct SEE_string *s1, struct SEE_string *s2);
struct SEE_string *SEE_string_sprintf(struct SEE_interpreter *i,
				const char *fmt, ...);
struct SEE_string *SEE_string_vsprintf(struct SEE_interpreter *i,
				const char *fmt, va_list ap);
struct SEE_string *SEE_string_literal(struct SEE_interpreter *i,
				const struct SEE_string *s);

void	SEE_string_free(struct SEE_interpreter *i, struct SEE_string **sp);

void	SEE_string_toutf8(struct SEE_interpreter *i, char *buf, 
			SEE_size_t buflen, const struct SEE_string *s);
SEE_size_t SEE_string_utf8_size(struct SEE_interpreter *interp,
			const struct SEE_string *s);

#endif /* _SEE_h_string_ */
