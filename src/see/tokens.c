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
/* $Id: tokens.c 988 2006-01-31 01:51:50Z d $ */

/*
 * Token tables for the lexical analyser.
 * 
 * The textual form of keywords and punctuators are
 * kept here.  See lex.c for how the tables are used.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#include "see_type.h"
#include "see_string.h"

#include "see_stringdefs.h"
#include "see_lex.h"
#include "see_tokens.h"

#define lengthof(a) (sizeof a / sizeof a[0])

struct strtoken SEE_tok_keywords[] = {
	{ STR(synchronized), tRESERVED },
	{ STR(eleven_filler), tRESERVED },	/* hack */
	{ STR(implements), tRESERVED },
	{ STR(instanceof), tINSTANCEOF },
	{ STR(transient), tRESERVED },
	{ STR(protected), tRESERVED },
	{ STR(interface), tRESERVED },
	{ STR(volatile), tRESERVED },
	{ STR(debugger), tRESERVED },
	{ STR(function), tFUNCTION },
	{ STR(continue), tCONTINUE },
	{ STR(abstract), tRESERVED },
	{ STR(private), tRESERVED },
	{ STR(package), tRESERVED },
	{ STR(extends), tRESERVED },
	{ STR(boolean), tRESERVED },
	{ STR(finally), tFINALLY },
	{ STR(default), tDEFAULT },
	{ STR(native), tRESERVED },
	{ STR(export), tRESERVED },
	{ STR(typeof), tTYPEOF },
	{ STR(switch), tSWITCH },
	{ STR(return), tRETURN },
	{ STR(throws), tRESERVED },
	{ STR(import), tRESERVED },
	{ STR(static), tRESERVED },
	{ STR(delete), tDELETE },
	{ STR(public), tRESERVED },
	{ STR(double), tRESERVED },
	{ STR(float), tRESERVED },
	{ STR(super), tRESERVED },
	{ STR(short), tRESERVED },
	{ STR(const), tRESERVED },
	{ STR(class), tRESERVED },
	{ STR(while), tWHILE },
	{ STR(final), tRESERVED },
	{ STR(throw), tTHROW },
	{ STR(catch), tCATCH },
	{ STR(break), tBREAK },
	{ STR(false), tFALSE },
	{ STR(with), tWITH },
	{ STR(long), tRESERVED },
	{ STR(null), tNULL },
	{ STR(true), tTRUE },
	{ STR(void), tVOID },
	{ STR(else), tELSE },
	{ STR(goto), tRESERVED },
	{ STR(enum), tRESERVED },
	{ STR(this), tTHIS },
	{ STR(byte), tRESERVED },
	{ STR(case), tCASE },
	{ STR(char), tRESERVED },
	{ STR(new), tNEW },
	{ STR(try), tTRY },
	{ STR(int), tRESERVED },
	{ STR(for), tFOR },
	{ STR(var), tVAR },
	{ STR(in), tIN },
	{ STR(do), tDO },
	{ STR(if), tIF },
};
int SEE_tok_nkeywords = lengthof(SEE_tok_keywords);

static struct token operators1[] = {
	{ {'?'}, '?' },
	{ {'{'}, '{' },
	{ {'}'}, '}' },
	{ {'('}, '(' },
	{ {')'}, ')' },
	{ {'['}, '[' },
	{ {']'}, ']' },
	{ {'.'}, '.' },
	{ {';'}, ';' },
	{ {','}, ',' },
	{ {'<'}, '<' },
	{ {'>'}, '>' },
	{ {':'}, ':' },
	{ {'='}, '=' },
	{ {'|'}, '|' },
	{ {'^'}, '^' },
	{ {'!'}, '!' },
	{ {'~'}, '~' },
	{ {'*'}, '*' },
	{ {'-'}, '-' },
	{ {'+'}, '+' },
	{ {'%'}, '%' },
	{ {'&'}, '&' },
	{ {0}, 0 }
}, operators2[] = {
	{ {'-','-'}, tMINUSMINUS },
	{ {'<','<'}, tLSHIFT },
	{ {'>','>'}, tRSHIFT },
	{ {'&','&'}, tANDAND },
	{ {'|','|'}, tOROR },
	{ {'+','='}, tPLUSEQ },
	{ {'-','='}, tMINUSEQ },
	{ {'*','='}, tSTAREQ },
	{ {'%','='}, tMODEQ },
	{ {'&','='}, tANDEQ },
	{ {'|','='}, tOREQ },
	{ {'^','='}, tXOREQ },
	{ {'<','='}, tLE },
	{ {'>','='}, tGE },
	{ {'=','='}, tEQ },
	{ {'!','='}, tNE },
	{ {'+','+'}, tPLUSPLUS },
	{ {0}, 0 }
}, operators3[] = {
	{ {'>','>','='}, tRSHIFTEQ },
	{ {'<','<','='}, tLSHIFTEQ },
	{ {'>','>','>'}, tURSHIFT },
	{ {'=','=','='}, tSEQ },
	{ {'!','=','='}, tSNE },
	{ {0}, 0 }
}, operators4[] = {
	{ {'<','!','-','-'}, tSGMLCOMMENT },
	{ {'>','>','>','='}, tURSHIFTEQ },
	{ {0}, 0 }
};

struct token *SEE_tok_operators[] = {
	NULL, operators1, operators2, operators3, operators4
};
int SEE_tok_noperators = lengthof(SEE_tok_operators);


static struct {
	const char *name;
	int token;
} tok_names[] = {
	{ "end of file",	tEND },
	{ "a comment",		tCOMMENT },
	{ "a line break",	tLINETERMINATOR },
	{ "'/='",		tDIVEQ },
	{ "'/'",		tDIV },
	{ "a number",		tNUMBER },
	{ "a string",		tSTRING },
	{ "an identifier",	tIDENT },
	{ "a regular expression",tREGEX },
	{ "a reserved word",	tRESERVED },
	{ "'instanceof'",	tINSTANCEOF },
	{ "'function'",		tFUNCTION },
	{ "'continue'",		tCONTINUE },
	{ "'finally'",		tFINALLY },
	{ "'default'",		tDEFAULT },
	{ "'typeof'",		tTYPEOF },
	{ "'switch'",		tSWITCH },
	{ "'return'",		tRETURN },
	{ "'delete'",		tDELETE },
	{ "'while'",		tWHILE },
	{ "'throw'",		tTHROW },
	{ "'catch'",		tCATCH },
	{ "'break'",		tBREAK },
	{ "'with'",		tWITH },
	{ "'void'",		tVOID },
	{ "'else'",		tELSE },
	{ "'this'",		tTHIS },
	{ "'case'",		tCASE },
	{ "'new'",		tNEW },
	{ "'try'",		tTRY },
	{ "'for'",		tFOR },
	{ "'var'",		tVAR },
	{ "'in'",		tIN },
	{ "'do'",		tDO },
	{ "'if'",		tIF },
	{ "'>>>='",		tURSHIFTEQ },
	{ "'<!--'",		tSGMLCOMMENT },
	{ "'>>='",		tRSHIFTEQ },
	{ "'<<='",		tLSHIFTEQ },
	{ "'>>>'",		tURSHIFT },
	{ "'==='",		tSEQ },
	{ "'!=='",		tSNE },
	{ "'--'",		tMINUSMINUS },
	{ "'<<'",		tLSHIFT },
	{ "'>>'",		tRSHIFT },
	{ "'&&'",		tANDAND },
	{ "'||'",		tOROR },
	{ "'+='",		tPLUSEQ },
	{ "'-='",		tMINUSEQ },
	{ "'*='",		tSTAREQ },
	{ "'%='",		tMODEQ },
	{ "'&='",		tANDEQ },
	{ "'|='",		tOREQ },
	{ "'^='",		tXOREQ },
	{ "'<='",		tLE },
	{ "'>='",		tGE },
	{ "'=='",		tEQ },
	{ "'!='",		tNE },
	{ "'++'",		tPLUSPLUS },
	{ "'true'",		tTRUE },
	{ "'false'",		tFALSE },
	{ "'null'",		tNULL }
};

const char *
SEE_tokenname(token)
	int token;
{
	static char buf[30];

	SEE_tokenname_buf(token, buf, sizeof buf);
	return buf;
}

void
SEE_tokenname_buf(token, buf, buflen)
	int token;
	char *buf;
	int buflen;
{
	int i;
	char tokch[4];
	const char *name;
        int namelen;

	for (i = 0; i < lengthof(tok_names); i++)
		if (tok_names[i].token == token) {
		    name = tok_names[i].name;
		    break;
		}
	else if (token >= ' ' && token <= '~') {
		tokch[0] = '\'';
		tokch[1] = (unsigned char)token;
		tokch[2] = '\'';
		tokch[3] = 0;
		name = tokch;
	} else 
		name = "<bad token>";
	namelen = strlen(name);
	if (namelen > buflen - 1)
		namelen = buflen - 1;
        memcpy(buf, name, namelen);
	buf[namelen] = 0;
        return;
}
