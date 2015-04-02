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
/* $Id: lex.c 1126 2006-08-05 12:48:25Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#include "see_type.h"
#include "see_string.h"
#include "see_value.h"
#include "see_object.h"
#include "see_input.h"
#include "see_try.h"
#include "see_intern.h"
#include "see_error.h"
#include "see_debug.h"
#include "see_interpreter.h"
#include "see_tokens.h"
#include "see_lex.h"
#include "see_stringdefs.h"
#include "see_unicode.h"
#include "see_dtoa.h"
#include "see_dprint.h"
#include "see_nmath.h"

#ifndef NDEBUG
int SEE_lex_debug = 0;
#endif

/*
 * Lexical analyser.
 *
 * This is a lexical analyser for ECMAScript. It uses a 6-character
 * lookahead input 'filter' (to detect '\u####') and provides an
 * interface that reveals if the returned token was immediately
 * preceeded by a line terminator.
 *
 * The lexical analyser's behaviour when deciding a slash '/' as
 * a division or the start of a regular expression is determined by 
 * a flag. The parser is exepected to set it.
 *
 * NOTE: Although all strings generated for ECMAscript are UTF-16, 
 * this lexer requires UCS-32 input.
 */

/* Macros that assume local variable lex */
#define NEXT		lex->input->lookahead
#define SKIP		do { SEE_INPUT_NEXT(lex->input);		\
			} while (!ATEOF && is_FormatControl(NEXT))
#define UNGET(c)	do { lex->la[++lex->lalen]=(c); } while (0)
#define ATEOF		(lex->input->eof)
#define LOOKAHEAD(buf,len) SEE_input_lookahead_copy(lex->input, buf, len)
#define CONSUME(ch)							\
    do {								\
	if (ATEOF)							\
	    SYNTAX_ERROR(STR(unexpected_eof));				\
	if (NEXT != (ch))						\
	    SYNTAX_ERROR(SEE_string_sprintf(				\
		lex->input->interpreter, "expected '%c'", (ch)));	\
	SKIP;								\
    } while (0)

#define SYNTAX_ERROR(s)							\
	SEE_error_throw_string(lex->input->interpreter,			\
	    lex->input->interpreter->SyntaxError,			\
	    prefix_msg(s, lex))

/* Sign constants */
#define NEGATIVE	(-1)
#define POSITIVE	(1)

/* Prototypes */
static struct SEE_string *prefix_msg(struct SEE_string *s, struct lex *lex);
static void string_adducs32(struct SEE_string *s, SEE_unicode_t c);
static int is_FormatControl(SEE_unicode_t c);
static int is_WhiteSpace(SEE_unicode_t c);
static int is_LineTerminator(SEE_unicode_t c);
static int is_HexDigit(SEE_unicode_t c);
static int HexValue(SEE_unicode_t c);
static int is_HexEscape(struct lex *lex);
static int is_UnicodeEscape(struct lex *lex);
static int is_IdentifierStart(struct lex *lex);
static int is_IdentifierPart(struct lex *lex);
static SEE_unicode_t HexEscape(struct lex *lex);
static SEE_unicode_t UnicodeEscape(struct lex *lex);
static int DivPunctuator(struct lex *lex);
static int SGMLComment(struct lex *lex);
static int Punctuator(struct lex *lex);
static int StringLiteral(struct lex *lex);
static int RegularExpressionLiteral(struct lex *lex, int prev);
static int NumericLiteral(struct lex *lex);
static int CommentDiv(struct lex *lex);
static int Token(struct lex *lex);
static int lex0(struct lex *lex);

/* Returns ("line " + next_lineno + ": " + s) */
static struct SEE_string *
prefix_msg(s, lex)
	struct SEE_string *s;
	struct lex *lex;
{
	struct SEE_string *t;
	struct SEE_interpreter *interp = lex->input->interpreter;

	t = SEE_string_sprintf(interp, "line %d: ", lex->next_lineno);
	SEE_string_append(t, s);
	return t;
}

/* Adds unicode character c to the string s, using UTF-16 encoding */
static void
string_adducs32(s, c)
	struct SEE_string *s;
	SEE_unicode_t c;
{
	if (c < 0x10000)
		SEE_string_addch(s, (SEE_char_t)(c & 0xffff));
	else {
		/* RFC2781: UTF-16 encoding */
		c -= 0x10000;
		SEE_string_addch(s, (SEE_char_t)(0xd800 | (c >> 10 & 0x3ff)));
		SEE_string_addch(s, (SEE_char_t)(0xdc00 | (c       & 0x3ff)));
	}
		
}

static int
is_FormatControl(c)
	SEE_unicode_t c;			/* 7.1 */
{
	return UNICODE_IS_Cf(c);	/* category Cf or L or R */
}

static int
is_WhiteSpace(c)
	SEE_unicode_t c;			/* 7.2 */
{
	return (c == 0x0009 || c == 0x000B || c == 0x000C || c == 0x0020 
		|| c == 0x00A0 || UNICODE_IS_Zs(c));
}

static int
is_LineTerminator(c)
	SEE_unicode_t c;			/* 7.3 */
{
	return (c == 0x000A || c == 0x000D || c == 0x2028 || c == 0x2029);
}

static int
is_HexDigit(c)
	SEE_unicode_t c;
{
	return ((c >= '0' && c <= '9') ||
		(c >= 'A' && c <= 'F') ||
		(c >= 'a' && c <= 'f'));
}

/* Returns the hexadecimal value of a character. Assumes char is a hex digit */
static int
HexValue(c)
	SEE_unicode_t c;
{
	if (c >= '0' && c <= '9')		return c - '0';
	else if (c >= 'a' && c <= 'f')		return c - 'a' + 10;
	else /* (c >= 'A' && c <= 'F') */	return c - 'A' + 10;
}

static int
is_HexEscape(lex)
	struct lex *lex;			/* 7.6 */
{
	SEE_unicode_t lookahead[4];
	int lookahead_len;

	lookahead_len = LOOKAHEAD(lookahead, 4);
	return (lookahead_len >= 4 &&
		lookahead[0] == '\\' &&
		lookahead[1] == 'x' &&
		is_HexDigit(lookahead[2]) &&
		is_HexDigit(lookahead[3]));
}

static int
is_UnicodeEscape(lex)
	struct lex *lex;			/* 7.6 */
{
	SEE_unicode_t lookahead[6];
	int lookahead_len;

	lookahead_len = LOOKAHEAD(lookahead, 6);
	return (lookahead_len >= 6 &&
		lookahead[0] == '\\' &&
		lookahead[1] == 'u' &&
		is_HexDigit(lookahead[2]) &&
		is_HexDigit(lookahead[3]) &&
		is_HexDigit(lookahead[4]) &&
		is_HexDigit(lookahead[5]));
}

static int
is_IdentifierStart(lex)
	struct lex *lex;			/* 7.6 */
{
	SEE_unicode_t c;

	if (ATEOF)
		return 0;
	if (is_UnicodeEscape(lex))
		return 1;
	c = NEXT;
	return UNICODE_IS_IS(c);
}

static int
is_IdentifierPart(lex)
	struct lex *lex;			/* 7.6 */
{
	SEE_unicode_t c;

	if (ATEOF)
		return 0;
	if (is_UnicodeEscape(lex))
		return 1;
	c = NEXT;
	return UNICODE_IS_IP(c);
}

static SEE_unicode_t
HexEscape(lex)
	struct lex *lex;			/* 7.6 la \x */
{
	int i;
	SEE_unicode_t r = 0;
	CONSUME('\\'); CONSUME('x');
	for (i = 0; i < 2; i++) {
		if (ATEOF) SYNTAX_ERROR(STR(unexpected_eof));
		r = (r << 4) | HexValue(NEXT);
		SKIP;
	}
	return r;
}

static SEE_unicode_t
UnicodeEscape(lex)
	struct lex *lex;			/* 7.6 la \u */
{
	int i;
	SEE_unicode_t r = 0;
	CONSUME('\\'); CONSUME('u');
	for (i = 0; i < 4; i++) {
		if (ATEOF) SYNTAX_ERROR(STR(unexpected_eof));
		r = (r << 4) | HexValue(NEXT);
		SKIP;
	}
	return r;

	/*
	 * XXX NOTE: the \uxxxx escape can only encode characters
	 * up to 0xffff. To express unicode characters above this
	 * codepoint, you would have to use a UTF-16 surrogate, but
	 * this is problematic. Better would be to augment ECMA-262
	 * with a \Uxxxxxxxx escape, such as Python provides.
	 * (spec bug?)
	 */
}

static int
DivPunctuator(lex)
	struct lex *lex;			/* 7.7 la / */
{
	CONSUME('/');
	if (!ATEOF && NEXT == '=') { 
		SKIP; 
		return tDIVEQ;
	}
	return tDIV;
}

static int
SGMLComment(lex)
	struct lex *lex;			/* la <!-- */
{

	/*
	 * Treat SGML comment introducers the same as '//',
	 * i.e. to ignore everything up to the end of the line.
	 * The closing '-->' is assumed to be protected by an
	 * actual '//' comment leader. (Refer to Chapter 9 of
	 * 'Client-Side JavaScript Guide', by Netscape.
	 */
	while (!ATEOF && !is_LineTerminator(NEXT))
		SKIP;
	if (ATEOF)
		return tEND;
	SKIP; /* line terminator */
	return tLINETERMINATOR;
}

static int
Punctuator(lex)
	struct lex *lex;			/* 7.7 */
{
	SEE_unicode_t op[4];	/* ">>>=" is the longest punctuator */
	struct token *t;
	int j, len, oplen;
	struct SEE_interpreter *interp = lex->input->interpreter;

	if (ATEOF)
		return tEND;
	oplen = LOOKAHEAD(op, 4);
	len = SEE_tok_noperators - 1;
	if (len > oplen)
		len = oplen;
	for (; len > 0; len--)
		for (t = SEE_tok_operators[len]; t->token; t++) {
			for (j = 0; j < len; j++)
				if (t->identifier[j] != op[j])
					goto out;
			if (t->token == tSGMLCOMMENT) {
				if (interp->compatibility & SEE_COMPAT_SGMLCOM)
					return SGMLComment(lex);
				else
					goto out;
			}
			for (j = 0; j < len; j++)
				SKIP;
			return t->token;
	   out:
			/* continue */ ;
		}

	/*
	 * Throw a descriptive error message
	 */
	if (op[0] == SEE_INPUT_BADCHAR)
		SYNTAX_ERROR(SEE_string_sprintf(interp, 
			"malformed unicode input"));
	else if (op[0] >= ' ' && op[0] <= '~')
		SYNTAX_ERROR(SEE_string_sprintf(interp, 
			"unexpected character '%c'", op[0]));
	else
		SYNTAX_ERROR(SEE_string_sprintf(interp, 
			"unexpected character '\\u%04x'", op[0]));
	/* NOTREACHED */
	return 0;
}

static int
StringLiteral(lex)
	struct lex *lex;			/* 7.8.4 la ' " */
{
	SEE_unicode_t quote;
	SEE_unicode_t c = 0;
	struct SEE_string *s;
	struct SEE_interpreter *interp = lex->input->interpreter;

	s = SEE_string_new(interp, 0);
	quote = NEXT;
	SKIP;
	while (!ATEOF && NEXT != quote) {
		if (is_LineTerminator(NEXT))
			SYNTAX_ERROR(STR(broken_literal));
		else if (is_UnicodeEscape(lex))
			c = UnicodeEscape(lex);
		else if (is_HexEscape(lex))
			c = HexEscape(lex);
		else if (NEXT == '\\') {
			SKIP;
			if (ATEOF || is_LineTerminator(NEXT))
				SYNTAX_ERROR(STR(escaped_lit_nl));
			switch (NEXT) {
			case 'b':	c = 0x0008; SKIP; break;
			case 't':	c = 0x0009; SKIP; break;
			case 'n':	c = 0x000a; SKIP; break;
			case 'v':	c = 0x000b; SKIP; break;
			case 'f':	c = 0x000c; SKIP; break;
			case 'r':	c = 0x000d; SKIP; break;
			case '0': case '1': case '2': case '3':
				c = NEXT - '0'; SKIP;
				if (!ATEOF && NEXT >= '0' && NEXT <= '7') 
					{ c = (c << 3) | (NEXT - '0'); SKIP; }
				if (!ATEOF && NEXT >= '0' && NEXT <= '7') 
					{ c = (c << 3) | (NEXT - '0'); SKIP; }
				break;
			case '4': case '5': case '6': case '7':
				c = NEXT - '0'; SKIP;
				if (!ATEOF && NEXT >= '0' && NEXT <= '7') 
					{ c = (c << 3) | (NEXT - '0'); SKIP; }
				break;
			case 'x':
			case 'u':
				if (SEE_GET_JS_COMPAT(interp))
				    goto literal;
				/* Strict ECMA: */
				if (NEXT == 'x')
				     SYNTAX_ERROR(STR(invalid_esc_x));
				else
				     SYNTAX_ERROR(STR(invalid_esc_u));
				/* NOTREACHED */
			default:
	literal:
				c = NEXT; SKIP; break;
			}
		} else {
			c = NEXT;
			SKIP;
		}
		string_adducs32(s, c);
	}
	CONSUME(quote);
	SEE_SET_STRING(&lex->value, s);
	return tSTRING;
}

/*
 * 7.8.5 Scans for a regular expression token.
 * Assumes prev (immediately previous token) is either tDIV or tDIVEQ.
 * Returns tREGEX on success or throws an exception on failure.
 * The string in lex->value is of the form "/regex/flags"
 */
static int
RegularExpressionLiteral(lex, prev)
	struct lex *lex;
	int prev;
{
	struct SEE_string *s;
	int incc = 0;
	struct SEE_interpreter *interp = lex->input->interpreter;

	s = SEE_string_new(interp, 0);
	SEE_string_addch(s, '/');
	if (prev == tDIVEQ)
		SEE_string_addch(s, '=');
	while (!ATEOF) {
		if (NEXT == '/' && 
		    (!incc || !(SEE_GET_JS_COMPAT(interp))))	/* EXT:15 */
			break;
		if (NEXT == '\\') {
			SEE_string_addch(s, '\\');
			SKIP;
			if (ATEOF) break;
		} else {
			/* Track charclasses for JS_COMPAT */
			if (NEXT == '[') incc = 1;
			if (NEXT == ']') incc = 0;
		}
		if (is_LineTerminator(NEXT))
			SYNTAX_ERROR(STR(broken_regex));
		string_adducs32(s, NEXT);
		SKIP;
	}
	if (ATEOF)
		SYNTAX_ERROR(STR(eof_in_regex));
	CONSUME('/');

	SEE_string_addch(s, '/');
	while (!ATEOF && is_IdentifierPart(lex)) {
		string_adducs32(s, NEXT);
		SKIP;
	}

	SEE_SET_STRING(&lex->value, s);
	return tREGEX;
}

static int
NumericLiteral(lex)
	struct lex *lex;			/* 7.8.3 la [.0-9] */
{
	SEE_number_t n, e;
	int seendigit;
	unsigned int i;
	struct SEE_string *s;
	char *numbuf, *endstr;
	struct SEE_interpreter *interp = lex->input->interpreter;

	seendigit = 0;
	n = 0;
	s = SEE_string_new(interp, 0);

	if (NEXT == '0') {
	    SKIP;
	    if (!ATEOF && (NEXT == 'x' || NEXT == 'X')) {
		SKIP;
		if (ATEOF || !is_HexDigit(NEXT))
		    SYNTAX_ERROR(STR(hex_literal_detritus));
		while (!ATEOF && is_HexDigit(NEXT)) {
		    SEE_string_addch(s, (SEE_char_t)NEXT);
		    SKIP;
		}
		if (!ATEOF && is_IdentifierStart(lex))
		    SYNTAX_ERROR(STR(hex_literal_detritus));
		e = 1;
		for (i = 0; i < s->length; i++) {
		    n += e * HexValue(s->data[s->length - i - 1]);
		    e *= 16;
		}
		SEE_SET_NUMBER(&lex->value, n);
		return tNUMBER;
	    }
	    SEE_string_addch(s, '0');
	    seendigit = 1;
	}

	while (!ATEOF && '0' <= NEXT && NEXT <= '9') {
	    SEE_string_addch(s, (SEE_char_t)NEXT);
	    seendigit = 1;
	    SKIP;
	}

	/* Octal integers */
	if (SEE_GET_JS_COMPAT(interp)
	    && seendigit 
	    && (ATEOF || (NEXT != '.' && NEXT != 'e' && NEXT != 'E'))
	    && s->length > 1
	    && s->data[0] == '0')
	{
		/* Octal integers start with 0 and dont follow with . or e */
		n = 0;
		for (i = 1; i < s->length; i++) {
		    if (s->data[i] > '7')
			goto not_octal;
		    n = n * 8 + s->data[i] - '0';
		}
		if (!ATEOF && is_IdentifierStart(lex))
		    goto not_octal;
		SEE_SET_NUMBER(&lex->value, n);
		return tNUMBER;
	}
    not_octal:

	if (!ATEOF && NEXT == '.') {
	    SEE_string_addch(s, '.');
	    SKIP;
	    while (!ATEOF && '0' <= NEXT && NEXT <= '9') {
		seendigit = 1;
	        SEE_string_addch(s, (SEE_char_t)NEXT);
		SKIP;
	    }
	}
	if (!seendigit) {
	    /* free(s) */
	    return '.';		/* Actually matched Punctuator! */
	}

	if (!ATEOF && (NEXT == 'e' || NEXT == 'E')) {
	    SEE_string_addch(s, (SEE_char_t)NEXT);
	    SKIP;
	    seendigit = 0;
	    if (!ATEOF && NEXT == '-') {
	        SEE_string_addch(s, '-');
		SKIP;
	    } else if (!ATEOF && NEXT == '+') {
	        SEE_string_addch(s, '+');
		SKIP;
	    }
	    e = 0;
	    while (!ATEOF && '0' <= NEXT && NEXT <= '9') {
		seendigit = 1;
	        SEE_string_addch(s, (SEE_char_t)NEXT);
		SKIP;
	    }
	    if (!seendigit)
		SYNTAX_ERROR(STR(dec_literal_detritus));
	}

	numbuf = SEE_STRING_ALLOCA(interp, char, s->length + 1);
	for (i = 0; i < s->length; i++)
		numbuf[i] = s->data[i] & 0x7f;
	numbuf[i] = '\0';
	endstr = NULL;
	n = SEE_strtod(numbuf, &endstr);
	if (!endstr || *endstr) 		/* impossible condition? */
		SYNTAX_ERROR(STR(dec_literal_detritus));
	SEE_SET_NUMBER(&lex->value, n);
	return tNUMBER;
}

static int
CommentDiv(lex)
	struct lex *lex;			/* 7.4 la / */
{
	SEE_unicode_t lookahead[2];
	int lookahead_len;

	lookahead_len = LOOKAHEAD(lookahead, 2);

	if (lookahead_len >= 2 && lookahead[0] == '/' && lookahead[1] == '*') {
		int starprev = 0, contains_newline = 0;
		SKIP;
		SKIP;
		while (!ATEOF) {
			if (starprev && NEXT == '/') {
			    CONSUME('/');
			    return contains_newline 
				? tLINETERMINATOR 
				: tCOMMENT;
			}
			if (is_LineTerminator(NEXT)) {
				lex->next_lineno++;
				contains_newline = 1;
			}
			starprev = (NEXT == '*');
			SKIP;
		}
		SYNTAX_ERROR(STR(eof_in_c_comment));
	}
	if (lookahead_len >= 2 && lookahead[0] == '/' && lookahead[1] == '/') {
		while (!ATEOF && !is_LineTerminator(NEXT))
			SKIP;
		if (ATEOF)
			return tEND;
		SKIP; /* line terminator */
		return tLINETERMINATOR;
	}

	/*
	 * NB: This assumes regular expressions not wanted,
	 * and that the rest of the regex can be scanned later 
	 * if the parser wants it.
	 */
	return DivPunctuator(lex);
}

static int
Token(lex)
	struct lex *lex;				/* 7.5 */
{
	struct SEE_interpreter *interp = lex->input->interpreter;

	if (ATEOF)
		return tEND;

	if (NEXT == '\'' || NEXT == '\"')
		return StringLiteral(lex);

	if ((NEXT >= '0' && NEXT <= '9') || NEXT == '.')
		return NumericLiteral(lex);

	if (is_IdentifierStart(lex)) {
		int hasescape = 0, i;
		struct SEE_string *s;
		SEE_unicode_t c;

		s = SEE_string_new(interp, 0);
		do {
			if (is_UnicodeEscape(lex)) {
				c = UnicodeEscape(lex);
				hasescape = 1;
			} else  {
				c = NEXT;
				SKIP;
			}
			string_adducs32(s, c);
		} while (is_IdentifierPart(lex));

		/* match keywords */
		if (!hasescape)
		    for (i = 0; i < SEE_tok_nkeywords; i++)
			if (SEE_tok_keywords[i].str->length == s->length
		         && SEE_string_cmp(SEE_tok_keywords[i].str, s) 
			 == 0)
			 {
			    int token = SEE_tok_keywords[i].token;
			    if (token == tRESERVED &&
/* EXT:3 */			SEE_COMPAT_JS(interp, >=, JS11))
			    {
#ifndef NDEBUG
				dprintf("Warning: line %d: reserved token '",
				    lex->next_lineno);
				dprints(s);
				dprintf("' treated as identifier\n");
#endif
			        break;
			    }
			    return token;
			 }

		SEE_intern_and_free(interp, &s);
		SEE_SET_STRING(&lex->value, s);
		return tIDENT;
	}

	return Punctuator(lex);
}



/*
 * Scanner grammar goal. Scans lex->input for a token, and returns it.
 *
 * May return multiple tLINETERMINATORs, but will never return tCOMMENT.
 * Scans the InputElementDiv production (never InputElementRegex).
 * If this function returns tDIV or tDIVEQ, and a regular expression is wanted,
 * then SEE_lex_regex() should be called immediately.
 */
static int
lex0(lex)
	struct lex *lex;
{
	int ret;

    again:

	while (!ATEOF && is_WhiteSpace(NEXT) && !is_LineTerminator(NEXT)) 
		SKIP;
	if (ATEOF)
		return tEND;

	if (is_LineTerminator(NEXT)) {
		lex->next_lineno++;
		SKIP;
		return tLINETERMINATOR;
	}

	switch (NEXT) {
	case '/':
		ret = CommentDiv(lex);
		if (ret == tCOMMENT)
			goto again;	/* Discard tCOMMENTs */
		return ret;
	case '\"':
	case '\'':
		return StringLiteral(lex);
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return NumericLiteral(lex);
	case '.':
	    {
		SEE_unicode_t lookahead[2];
		int lookahead_len;

		lookahead_len = LOOKAHEAD(lookahead, 2);
		if (lookahead_len >= 2 
		 && lookahead[1] >= '0' 
		 && lookahead[1] <= '9')
			return NumericLiteral(lex);
		SKIP;
		return '.';
	    }
	default:
		return Token(lex);
	}
}

/*------------------------------------------------------------
 * Public API
 */

/*
 * Initialises a tokenizer structure
 */
void
SEE_lex_init(lex, inp)
	struct lex *lex;
	struct SEE_input *inp;
{
	lex->input = inp;
	SEE_SET_UNDEFINED(&lex->value);
	lex->next_lineno = inp->first_lineno;
	lex->next_filename = inp->filename;
	(void)SEE_lex_next(lex);
}

/*
 * Main interface to the lexical anaylser.
 *
 * We keep a one-token lookahead. 
 * Each call to this function generates a new lookahead token
 * (in lex->next) and returns the previous one, so
 * the lex flags apply to the scanning of the NEXT token, 
 * and NOT to the token being returned. (ie The caller should
 * generally refer to the resulting lex->next to make 
 * decisions. The value returned is merely a convenience.)
 *
 * On return, this function also sets (or clears) the 
 * lex->next_follows_nl flag when a newline is seen immediately 
 * before lex->next. The parser should use this information to 
 * perform automatic semicolon insertion. Note that the defined
 * tLINETERMINATOR token is an internal scanner pseudo-token and 
 * is never returned by this function. Use the next_follows_nl flag.
 *
 * As a special case, if end-of-file (tEND) does not follow 
 * a line terminator, then this function pretends that it does.
 *
 * The lex->next_lineno field reflects the line number of
 * lex->next.
 */
int
SEE_lex_next(lex)
	struct lex *lex;
{
	int next, token;

	lex->next_follows_nl = 0;
	next = lex->next;

  again:

	token = lex0(lex);

	if (token == tLINETERMINATOR) {
		lex->next_follows_nl = 1;
		goto again;
	}
	if (token == tEND)
		lex->next_follows_nl = 1;
	lex->next = token;

#ifndef NDEBUG
	if (SEE_lex_debug)
	    switch (lex->next) {
	    case tIDENT:  
		  dprintf("lex: tIDENT ");
		  dprintv(lex->input->interpreter, &lex->value);
		  dprintf("\n"); break;
	    case tSTRING: 
		  dprintf("lex: tSTRING ");
		  dprintv(lex->input->interpreter, &lex->value);
		  dprintf("\n"); break;
	    case tNUMBER: 
		  dprintf("lex: tNUMBER ");
		  dprintv(lex->input->interpreter, &lex->value);
		  dprintf("\n"); break;
	    default:
		  dprintf("lex: %s\n", SEE_tokenname(lex->next));
	}
#endif

	return next;
}

/*
 * Converts the next token (just scanned) into a regular expression, 
 * if possible.
 */
void
SEE_lex_regex(lex) 
	struct lex *lex;
{
	if (lex->next == tDIV || lex->next == tDIVEQ)
		lex->next = RegularExpressionLiteral(lex, lex->next);
}

/*
 * 9.3.1
 * Scans a SEE_string to convert it into a number.
 * On success, sets res to the resulting number and returns non-zero.
 *
 * This function is called by SEE_ToNumber().
 */
int
SEE_lex_number(interp, s, res)
	struct SEE_interpreter *interp;
	struct SEE_string *s;
	struct SEE_value *res;
{
	SEE_number_t n, sign;
	int seendig, hexok;
	int len = s->length;
	int i, pos;
	int start;
	char *numbuf, *endstr;

/* These work becuase we expect no Unicode surrogates in numbers */
#undef ATEOF
#undef NEXT
#undef SKIP
#define ATEOF	(pos >= len)
#define NEXT	(s->data[pos])
#define SKIP	pos++

	pos = 0;

	/* StrWhiteSpace */
	while (!ATEOF && (is_WhiteSpace(NEXT) || is_LineTerminator(NEXT)))
		SKIP;

	if (ATEOF) {
		SEE_SET_NUMBER(res, 0);		/* +0 */
		return 1;
	}

	sign = 0;
	if (NEXT == '-') {
		sign = NEGATIVE;
		SKIP;
	} else if (NEXT == '+') {
		sign = POSITIVE;
		SKIP;
	}

	/* Strict ECMA262-3 hex strings require no sign. Netscape relaxes this. */
	hexok = !sign || SEE_GET_JS_COMPAT(interp);

	if (ATEOF) goto fail;
	if (NEXT == 'I') {
		SKIP; if (ATEOF || NEXT != 'n') goto fail;
		SKIP; if (ATEOF || NEXT != 'f') goto fail;
		SKIP; if (ATEOF || NEXT != 'i') goto fail;
		SKIP; if (ATEOF || NEXT != 'n') goto fail;
		SKIP; if (ATEOF || NEXT != 'i') goto fail;
		SKIP; if (ATEOF || NEXT != 't') goto fail;
		SKIP; if (ATEOF || NEXT != 'y') goto fail;
		SKIP; n = SEE_Infinity;
	} else {
		n = 0;
		start = pos;

		/* Hexadecimal */
		if (hexok && pos + 1 < len && s->data[pos] == '0' &&
			(s->data[pos+1] == 'x' || s->data[pos+1] == 'X'))
		{
		    SKIP;
		    SKIP;
		    seendig = 0;
		    while (!ATEOF && is_HexDigit(NEXT)) {
			seendig = 1;
			n = 16 * n + HexValue(NEXT);
			SKIP;
		    }
		    if (!seendig) goto fail;
		    goto out;
		}

#if 0
		/* Octal */
		if (SEE_COMPAT_JS(interp, >=, JS11) && /* EXT:4 */
		    !ATEOF && NEXT == '0' &&
		    !(pos + 1 < len && (s->data[pos+1] == '.' ||
		      s->data[pos+1] == 'e' || s->data[pos+1] == 'E')))
		{
		    SKIP;
		    n = 0;
		    while (!ATEOF && NEXT >= '0' && NEXT <= '7') {
			n = 8 * n + NEXT - '0';
			SKIP;
		    }
		    goto out;
		}
#endif

		/*
		 * After this point, we expect to use strtod, so we
		 * just check for character validity, rather than computing n.
		 */
		seendig = 0;
		while (!ATEOF && NEXT >= '0' && NEXT <= '9') {
		    seendig = 1;
		    SKIP;
		}
		if (!ATEOF && NEXT == '.') {
		    SKIP; /* '.' */
		    while (!ATEOF && NEXT >= '0' && NEXT <= '9') {
			seendig = 1;
			SKIP;
		    }
		}
		if (!seendig) goto fail;	/* a lone dot is illegal */
		if (!ATEOF && (NEXT == 'e' || NEXT == 'E')) {
		    SKIP;
		    if (!ATEOF && NEXT == '-') {
			SKIP;
		    } else if (!ATEOF && NEXT == '+')
			SKIP;
		    seendig = 0;
		    while (!ATEOF && NEXT >= '0' && NEXT <= '9') {
			seendig = 1;
			SKIP;
		    }
		    if (!seendig) goto fail;
		}
		numbuf = SEE_STRING_ALLOCA(interp, char, pos - start + 1);
		for (i = 0; i < pos - start; i++)
			numbuf[i] = s->data[i + start] & 0x7f;
		numbuf[i] = '\0';
		endstr = NULL;
		n = SEE_strtod(numbuf, &endstr);
		if (!endstr || *endstr != '\0')
			goto fail;
	}

   out:
	if (!sign) sign = POSITIVE;

	/* trailing StrWhiteSpace */
	while (!ATEOF && (is_WhiteSpace(NEXT) || is_LineTerminator(NEXT)))
		SKIP;
	if (ATEOF) {
	    SEE_SET_NUMBER(res, NUMBER_copysign(n, sign));
	    return 1;
	}

    fail:
	return 0;
}

