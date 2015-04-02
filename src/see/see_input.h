/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: input.h 920 2005-12-24 10:30:35Z d $ */

#ifndef _SEE_h_input_
#define _SEE_h_input_

/*
 * UCS-32 character stream inputs,
 * used by the lexical analyser.
 *
 * Supported streams:
 *	- ASCII stdio file
 *	- SEE_string
 *	- UTF-8 C-strings
 *
 * Inputs can also implement filters.
 * Supported filters:
 *	- n-character lookahead
 *
 */

#include <stdio.h>

#include "see_type.h"

struct SEE_input;
struct SEE_string;
struct SEE_interpreter;

struct SEE_inputclass {
	/* Returns the next character on the stream. Invalid if !eof */
	SEE_unicode_t	(*next)(struct SEE_input *);
	/* Releases system resources allocated to input */
	void		(*close)(struct SEE_input *);
};

struct SEE_input {
	struct SEE_inputclass *inputclass;
	SEE_boolean_t	   eof;		/* True means next() will be invalid */
	SEE_unicode_t	   lookahead;	/* UCS-32 - what next() will return */
	struct SEE_string *filename;	/* source origin desc (or NULL) */
	int 		   first_lineno;
	struct SEE_interpreter *interpreter;
};

#define SEE_INPUT_NEXT(i)	(*(i)->inputclass->next)(i)
#define SEE_INPUT_CLOSE(i)	(*(i)->inputclass->close)(i)

/*
 * These input filters are intended for testing and demonstration.
 * Host applications will normally provide their own input class if they
 * want to support embedded JavaScript. Typically, you will have your
 * program text in a buffer that your application owns, and your
 * input class will return characters from that buffer. 
 * Remember: ASCII chars 0x80-0xff are not Unicode!
 */

/* Creates an input from a stdio file. Unicode markers are understood */
struct SEE_input   *SEE_input_file(struct SEE_interpreter *i, 
			FILE *f, const char *filename, const char *encoding);

/* Creates an input from an interpreter string */
struct SEE_input   *SEE_input_string(struct SEE_interpreter *i,
			struct SEE_string *s);

/* Creates an input from a C string */
struct SEE_input   *SEE_input_utf8(struct SEE_interpreter *i, const char *s);

/* Creates an input filter around another input, keeping a lookahead buffer */
struct SEE_input   *SEE_input_lookahead(struct SEE_input *i, int maxlookahead);

/* Copy out the lookahead buffer */
int SEE_input_lookahead_copy(struct SEE_input *li, 
			SEE_unicode_t *buf, int buflen);

/* Unicode character returned when source is malformed */
#define SEE_INPUT_BADCHAR	((SEE_unicode_t)0x100000)

#endif /* _SEE_h_input_ */
