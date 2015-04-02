/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: lex.h 631 2004-03-29 20:13:45Z d $ */

#ifndef _SEE_h_lex_
#define _SEE_h_lex_

/*
 * The lexical analyser returns the next syntactic token from
 * the input stream.
 */

#include "see_value.h"

struct SEE_string;

struct lex {
	struct SEE_input  *input;
	struct SEE_value   value;
	int		   next;		/* single token lookahead */
	int		   next_lineno;		/* line number of next */
	struct SEE_string *next_filename;	/* source id for line number */
	SEE_boolean_t	   next_follows_nl;	/* next was preceeded by NL */
};

void SEE_lex_init(struct lex *lex, struct SEE_input *input); 
int  SEE_lex_next(struct lex *lex);
void SEE_lex_regex(struct lex *lex);

int SEE_lex_number(struct SEE_interpreter *i,
	struct SEE_string *s, struct SEE_value *res);

/*
 * tokens returned by SEE_lex_next()
 * (single-character tokens, like ';', are represented by their ASCII value)
 */

#define tEND		(-1)		/* end of file */
#define tANDAND		257
#define tANDEQ		258
#define tBREAK		259
#define tCASE		260
#define tCATCH		261
#define tCONTINUE	262
#define tDEFAULT	263
#define tDELETE		264
#define tDIV		'/'
#define tDIVEQ		266
#define tDO		267
#define tELSE		268
#define tEQ		269
#define tFINALLY	270
#define tFOR		271
#define tFUNCTION	272
#define tGE		273
#define tIF		274
#define tIN		275
#define tINSTANCEOF	276
#define tLE		277
#define tLSHIFT		278
#define tLSHIFTEQ	279
#define tMINUSEQ	280
#define tMINUSMINUS	281
#define tMODEQ		282
#define tNE		283
#define tNEW		284
#define tOREQ		285
#define tOROR		286
#define tPLUSEQ		287
#define tPLUSPLUS	288
#define tREGEX		289
#define tRESERVED	290	/* any "reserved" keyword */
#define tRETURN		291
#define tRSHIFT		292
#define tRSHIFTEQ	293
#define tSEQ		294
#define tSNE		295
#define tSTAREQ		296
#define tSWITCH		297
#define tTHIS		298
#define tTHROW		299
#define tTRY		300
#define tTYPEOF		301
#define tURSHIFT	302
#define tURSHIFTEQ	303
#define tVAR		304
#define tVOID		305
#define tWHILE		306
#define tWITH		307
#define tXOREQ		308
#define tNUMBER		309	/* numeric constant */
#define tSTRING		310	/* string constant */
#define tIDENT		311	/* non-keyword identifier */
#define tCOMMENT	312	/* internal: any kind of comment */
#define tLINETERMINATOR	313	/* internal: end of line */
#define tTRUE		314
#define tNULL		315
#define tFALSE		316
#define tSGMLCOMMENT	317	/* internal: '<!--' (treated like '//') */

#endif /* _SEE_h_lex_ */
