/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: parse.h 1004 2006-02-04 10:06:07Z d $ */

#ifndef _SEE_h_parse_
#define _SEE_h_parse_

struct SEE_string;
struct SEE_value;
struct SEE_interpreter;
struct SEE_context;
struct SEE_input;
struct function;

#include "see_eval.h"

struct function *SEE_parse_function(struct SEE_interpreter *i,
	struct SEE_string *name, struct SEE_input *param_input, 
	struct SEE_input *body_input);
void  SEE_eval_functionbody(struct function *f,
	struct SEE_context *context, struct SEE_value *res);
struct function *SEE_parse_program(struct SEE_interpreter *i, 
	struct SEE_input *input);

void  SEE_functionbody_print(struct SEE_interpreter *i, struct function *f);
struct SEE_string *SEE_functionbody_string(struct SEE_interpreter *i, 
	struct function *f);
int SEE_functionbody_isempty(struct SEE_interpreter *i, struct function *f);
int SEE_compare(struct SEE_interpreter *i, struct SEE_value *x, 
	struct SEE_value *y);

#endif /* _SEE_h_parse_ */
