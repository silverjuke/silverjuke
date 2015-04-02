/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: tokens.h 671 2004-03-30 12:30:19Z d $ */

#ifndef _SEE_h_tokens_
#define _SEE_h_tokens_

#include "see_type.h"

struct SEE_string;

/* Token tables */
struct strtoken {
	struct SEE_string *str;
	int token;
};
extern struct strtoken SEE_tok_keywords[];
extern int SEE_tok_nkeywords;

struct token {
	SEE_char_t identifier[4];
	int token;
};
extern struct token *SEE_tok_operators[];
extern int SEE_tok_noperators;

const char *SEE_tokenname(int token);
void SEE_tokenname_buf(int token, char *buf, int bufsz);

#endif /* _SEE_h_tokens_ */
