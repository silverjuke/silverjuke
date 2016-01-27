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
/* $Id: parse.c 1126 2006-08-05 12:48:25Z d $ */

/*
 * Combined parser and evaluator.
 *
 * This file contains two threads (storylines): the LL(2) recursive
 * descent parser thread, and the semantic functions thread. The parsing and 
 * semantics stages are grouped together by their common productions in the
 * grammar, to facilitate reference to the ECMA-262 standard.
 *
 * The input to the parser is an instance of the lexical analyser.
 * The output of the parser is an abstract syntax tree (AST). The input to
 * the evaluator is the AST, and the output of the evaluator is a SEE_value.
 *
 * For each production PROD in the grammar, the function PROD_parse() 
 * allocates and populates a 'node' structure, representing the root
 * of the syntax tree that represents the parsed production. Each node
 * holds a 'nodeclass' pointer to semantics information as well as
 * production-specific information. 
 *
 * Names of structures and functions have been chosen to correspond 
 * with the production names from the standard's grammar.
 *
 * The semantic functions in each node class are the following:
 *
 *  - PROD_eval() functions are called at runtime to implement
 *    the behaviour of the program. They "evaluate" the program.
 *
 *  - PROD_fproc() functions are called at execution time to generate
 *    the name/value bindings between container objects and included
 *    function objects. (It finds functions and assigns them to properties.)
 *    They provide a parallel, but independent, recusive semantic operation
 *    described in the standard as "process[ing] for function 
 *    declarations".
 *
 *  - PROD_print() functions are used to print the abstract syntax tree 
 *    to stdout during debugging.
 *
 * TODO This file is far too big; need to split it up.
 * TODO Compact/bytecode intermediate form
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <string.h>
# include <stdlib.h>
#endif

#include "see_mem.h"
#include "see_string.h"
#include "see_value.h"
#include "see_intern.h"
#include "see_object.h"
#include "see_cfunction.h"
#include "see_input.h"
#include "see_eval.h"
#include "see_try.h"
#include "see_error.h"
#include "see_debug.h"
#include "see_interpreter.h"
#include "see_context.h"
#include "see_system.h"

#include "see_lex.h"
#include "see_parse.h"
#include "see_scope.h"
#include "see_function.h"
#include "see_enumerate.h"
#include "see_tokens.h"
#include "see_stringdefs.h"
#include "see_dtoa.h"
#include "see_dprint.h"

#ifndef NDEBUG
int SEE_parse_debug = 0;
int SEE_eval_debug = 0;
#endif

/*------------------------------------------------------------
 * structure types
 */

/*
 * Abstract syntax tree basic structure
 */
struct node;

#if WITH_PARSER_PRINT
struct printer;
#endif

#if WITH_PARSER_VISIT
typedef void (*visitor_fn_t)(struct node *, void *);
#endif

struct nodeclass {
#ifndef NDEBUG
	const char *decl_file; int decl_line;
	struct nodeclass *superclass;
# define SUPERCLASS(cls)	__FILE__, __LINE__, &cls##_nodeclass,
# define BASECLASS		__FILE__, __LINE__, NULL,
#else
# define SUPERCLASS(cls)
# define BASECLASS
#endif
	void (*eval)(struct node *, struct SEE_context *, struct SEE_value *);
	void (*fproc)(struct node *, struct SEE_context *);
#if WITH_PARSER_PRINT
	void (*print)(struct node *, struct printer *);
#endif
#if WITH_PARSER_VISIT
	void (*visit)(struct node *, visitor_fn_t, void *);
#endif
	int  (*isconst)(struct node *, struct SEE_interpreter *);
};

#if WITH_PARSER_PRINT
# define PARSER_PRINT(fn)	fn,
#else
# define PARSER_PRINT(fn)	/* empty */
#endif

#if WITH_PARSER_VISIT
# define PARSER_VISIT(fn)	fn,
#else
# define PARSER_VISIT(fn)	/* empty */
#endif

struct node {
	struct nodeclass *nodeclass;
	struct SEE_throw_location location;
	int isconst_valid : 1;		/* true if isconst is valid */
	int isconst : 1;		/* true if node is a constant eval */
};

struct label {
	struct SEE_string *name;	/* interned */
	struct SEE_string *labelset;
	struct SEE_throw_location location;
	struct label *next;
	int continuable;		/* can be target of continue */
};

#define COPY_LABELSET(dst, src) labelset_copy(dst, src);

#define UNGET_MAX 3
struct parser {
	struct SEE_interpreter *interpreter;
	struct lex 	 *lex;
	int		  unget, unget_end;
	struct SEE_value  unget_val[UNGET_MAX];
	int               unget_tok[UNGET_MAX];
	int               unget_lin[UNGET_MAX];
	SEE_boolean_t     unget_fnl[UNGET_MAX];
	int 		  noin;	  /* ignore 'in' in RelationalExpression */
	int		  is_lhs; /* derived LeftHandSideExpression */
	int		  funcdepth;
	struct SEE_string*current_labelset;
	struct label     *labels;
	struct var	**vars;		/* list of declared variables */
};

#if WITH_PARSER_PRINT
struct printerclass {
	void	(*print_string)(struct printer *, struct SEE_string *);
	void	(*print_char)(struct printer *, int);
	void	(*print_newline)(struct printer *, int);
	void	(*print_node)(struct printer *, struct node *);
};

struct printer {
	struct printerclass *printerclass;
	struct SEE_interpreter *interpreter;
	int	indent;
	int	bol;
};
#endif

struct Targetable_node {
	struct node node;
	void *target;
};

/*------------------------------------------------------------
 * function prototypes
 */

struct Arguments_node;
struct ArrayLiteral_node;
struct AssignmentExpression_node;
struct Binary_node;
struct BreakStatement_node;
struct CallExpression_node;
struct ConditionalExpression_node;
struct ContinueStatement_node;
struct Function_node;
struct IfStatement_node;
struct IterationStatement_for_node;
struct IterationStatement_forin_node;
struct IterationStatement_while_node;
struct Literal_node;
struct MemberExpression_bracket_node;
struct MemberExpression_dot_node;
struct MemberExpression_new_node;
struct ObjectLiteral_node;
struct PrimaryExpression_ident_node;
struct RegularExpressionLiteral_node;
struct ReturnStatement_node;
struct SourceElements_node;
struct StringLiteral_node;
struct SwitchStatement_node;
struct TryStatement_node;
struct Unary_node;
struct VariableDeclaration_node;

static struct nodeclass Unary_nodeclass,
                        Binary_nodeclass,
                        Literal_nodeclass,
                        StringLiteral_nodeclass,
                        RegularExpressionLiteral_nodeclass,
                        PrimaryExpression_this_nodeclass,
                        PrimaryExpression_ident_nodeclass,
                        ArrayLiteral_nodeclass,
                        ObjectLiteral_nodeclass,
                        Arguments_nodeclass,
                        MemberExpression_new_nodeclass,
                        MemberExpression_dot_nodeclass,
                        MemberExpression_bracket_nodeclass,
                        CallExpression_nodeclass,
                        PostfixExpression_inc_nodeclass,
                        PostfixExpression_dec_nodeclass,
                        UnaryExpression_delete_nodeclass,
                        UnaryExpression_void_nodeclass,
                        UnaryExpression_typeof_nodeclass,
                        UnaryExpression_preinc_nodeclass,
                        UnaryExpression_predec_nodeclass,
                        UnaryExpression_plus_nodeclass,
                        UnaryExpression_minus_nodeclass,
                        UnaryExpression_inv_nodeclass,
                        UnaryExpression_not_nodeclass,
                        MultiplicativeExpression_mul_nodeclass,
                        MultiplicativeExpression_div_nodeclass,
                        MultiplicativeExpression_mod_nodeclass,
                        AdditiveExpression_add_nodeclass,
                        AdditiveExpression_sub_nodeclass,
                        ShiftExpression_lshift_nodeclass,
                        ShiftExpression_rshift_nodeclass,
                        ShiftExpression_urshift_nodeclass,
                        RelationalExpression_lt_nodeclass,
                        RelationalExpression_gt_nodeclass,
                        RelationalExpression_le_nodeclass,
                        RelationalExpression_ge_nodeclass,
                        RelationalExpression_instanceof_nodeclass,
                        RelationalExpression_in_nodeclass,
                        EqualityExpression_eq_nodeclass,
                        EqualityExpression_ne_nodeclass,
                        EqualityExpression_seq_nodeclass,
                        EqualityExpression_sne_nodeclass,
                        BitwiseANDExpression_nodeclass,
                        BitwiseXORExpression_nodeclass,
                        BitwiseORExpression_nodeclass,
                        LogicalANDExpression_nodeclass,
                        LogicalORExpression_nodeclass,
                        ConditionalExpression_nodeclass,
                        AssignmentExpression_nodeclass,
                        AssignmentExpression_simple_nodeclass,
                        AssignmentExpression_muleq_nodeclass,
                        AssignmentExpression_diveq_nodeclass,
                        AssignmentExpression_modeq_nodeclass,
                        AssignmentExpression_addeq_nodeclass,
                        AssignmentExpression_subeq_nodeclass,
                        AssignmentExpression_lshifteq_nodeclass,
                        AssignmentExpression_rshifteq_nodeclass,
                        AssignmentExpression_urshifteq_nodeclass,
                        AssignmentExpression_andeq_nodeclass,
                        AssignmentExpression_xoreq_nodeclass,
                        AssignmentExpression_oreq_nodeclass,
                        Expression_comma_nodeclass,
                        Block_empty_nodeclass,
                        StatementList_nodeclass,
                        VariableStatement_nodeclass,
                        VariableDeclarationList_nodeclass,
                        VariableDeclaration_nodeclass,
                        EmptyStatement_nodeclass,
                        ExpressionStatement_nodeclass,
                        IfStatement_nodeclass,
                        IterationStatement_dowhile_nodeclass,
                        IterationStatement_while_nodeclass,
                        IterationStatement_for_nodeclass,
                        IterationStatement_forvar_nodeclass,
                        IterationStatement_forin_nodeclass,
                        IterationStatement_forvarin_nodeclass,
                        ContinueStatement_nodeclass,
                        BreakStatement_nodeclass,
                        ReturnStatement_nodeclass,
                        ReturnStatement_undef_nodeclass,
                        WithStatement_nodeclass,
                        SwitchStatement_nodeclass,
                        LabelledStatement_nodeclass,
                        ThrowStatement_nodeclass,
                        TryStatement_nodeclass,
                        TryStatement_catch_nodeclass,
                        TryStatement_finally_nodeclass,
                        TryStatement_catchfinally_nodeclass,
                        Function_nodeclass,
                        FunctionDeclaration_nodeclass,
                        FunctionExpression_nodeclass,
                        FunctionBody_nodeclass,
                        SourceElements_nodeclass;

static struct node *new_node(struct parser *parser, int sz, 
        struct nodeclass *nc, const char *dbg_nc);
static struct node *cast_node(struct node *na, struct nodeclass *nc, 
        const char *cname, const char *file, int line);
static void parser_init(struct parser *parser, 
        struct SEE_interpreter *interp, struct lex *lex);
static void label_push(struct parser *parser, struct SEE_string *name, 
	struct SEE_string *labelset, int continuable);
static void label_pop(struct parser *parser);
static void label_set_continuable(struct parser *parser,
	struct SEE_string *labelset);
static struct label *label_lookup(struct parser *parser, 
	struct SEE_string *name, int kind);
static int lookahead(struct parser *parser, int n);
static void trace_event(struct SEE_context *ctxt, enum SEE_trace_event);
static struct SEE_traceback *traceback_enter(struct SEE_interpreter *interp, 
        struct SEE_object *callee, struct SEE_throw_location *loc, 
        int call_type);
static void traceback_leave(struct SEE_interpreter *interp, 
        struct SEE_traceback *old_tb);
static void GetValue(struct SEE_context *context, struct SEE_value *v, 
        struct SEE_value *res);
static void PutValue(struct SEE_context *context, struct SEE_value *v, 
        struct SEE_value *w);
static struct SEE_string *error_at(struct parser *parser, const char *fmt, 
        ...);
static int Always_isconst(struct node *na, struct SEE_interpreter *interp);
static void Literal_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static struct node *Literal_parse(struct parser *parser);
static struct node *NumericLiteral_parse(struct parser *parser);
static void StringLiteral_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static struct node *StringLiteral_parse(struct parser *parser);
static void RegularExpressionLiteral_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *RegularExpressionLiteral_parse(struct parser *parser);
static void PrimaryExpression_this_eval(struct node *n, 
        struct SEE_context *context, struct SEE_value *res);
static void PrimaryExpression_ident_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *PrimaryExpression_parse(struct parser *parser);
static void ArrayLiteral_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static struct node *ArrayLiteral_parse(struct parser *parser);
static void ObjectLiteral_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static struct node *ObjectLiteral_parse(struct parser *parser);
static void Arguments_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static int Arguments_isconst(struct node *na, 
        struct SEE_interpreter *interp);
static struct Arguments_node *Arguments_parse(struct parser *parser);
static void MemberExpression_new_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void MemberExpression_dot_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void MemberExpression_bracket_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *MemberExpression_parse(struct parser *parser);
static void CallExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *LeftHandSideExpression_parse(struct parser *parser);
static int Unary_isconst(struct node *na, struct SEE_interpreter *interp);
static void PostfixExpression_inc_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void PostfixExpression_dec_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *PostfixExpression_parse(struct parser *parser);
static void UnaryExpression_delete_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_void_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_typeof_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_preinc_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_predec_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_plus_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_minus_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_inv_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void UnaryExpression_not_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *UnaryExpression_parse(struct parser *parser);
static int Binary_isconst(struct node *na, struct SEE_interpreter *interp);
static void MultiplicativeExpression_mul_common(struct SEE_value *r2, 
        struct node *bn, struct SEE_context *context, struct SEE_value *res);
static void MultiplicativeExpression_mul_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void MultiplicativeExpression_div_common(struct SEE_value *r2, 
        struct node *bn, struct SEE_context *context, struct SEE_value *res);
static void MultiplicativeExpression_div_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void MultiplicativeExpression_mod_common(struct SEE_value *r2, 
        struct node *bn, struct SEE_context *context, struct SEE_value *res);
static void MultiplicativeExpression_mod_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *MultiplicativeExpression_parse(struct parser *parser);
static void AdditiveExpression_add_common(struct SEE_value *r2, 
        struct node *bn, struct SEE_context *context, struct SEE_value *res);
static void AdditiveExpression_add_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AdditiveExpression_sub_common(struct SEE_value *r2, 
        struct node *bn, struct SEE_context *context, struct SEE_value *res);
static void AdditiveExpression_sub_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *AdditiveExpression_parse(struct parser *parser);
static void ShiftExpression_lshift_common(struct SEE_value *r2, 
        struct node *bn, struct SEE_context *context, struct SEE_value *res);
static void ShiftExpression_lshift_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void ShiftExpression_rshift_common(struct SEE_value *r2, 
        struct node *bn, struct SEE_context *context, struct SEE_value *res);
static void ShiftExpression_rshift_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void ShiftExpression_urshift_common(struct SEE_value *r2, 
        struct node *bn, struct SEE_context *context, struct SEE_value *res);
static void ShiftExpression_urshift_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *ShiftExpression_parse(struct parser *parser);
static void RelationalExpression_sub(struct SEE_interpreter *interp, 
        struct SEE_value *x, struct SEE_value *y, struct SEE_value *res);
static void RelationalExpression_lt_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void RelationalExpression_gt_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void RelationalExpression_le_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void RelationalExpression_ge_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void RelationalExpression_instanceof_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void RelationalExpression_in_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *RelationalExpression_parse(struct parser *parser);
static void EqualityExpression_eq(struct SEE_interpreter *interp, 
        struct SEE_value *x, struct SEE_value *y, struct SEE_value *res);
static void EqualityExpression_seq(struct SEE_context *context, 
        struct SEE_value *x, struct SEE_value *y, struct SEE_value *res);
static void EqualityExpression_eq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void EqualityExpression_ne_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void EqualityExpression_seq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void EqualityExpression_sne_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *EqualityExpression_parse(struct parser *parser);
static void BitwiseANDExpression_common(struct SEE_value *r2, 
        struct node *bn, struct SEE_context *context, struct SEE_value *res);
static void BitwiseANDExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *BitwiseANDExpression_parse(struct parser *parser);
static void BitwiseXORExpression_common(struct SEE_value *r2, 
        struct node *bn, struct SEE_context *context, struct SEE_value *res);
static void BitwiseXORExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *BitwiseXORExpression_parse(struct parser *parser);
static void BitwiseORExpression_common(struct SEE_value *r2, 
        struct node *bn, struct SEE_context *context, struct SEE_value *res);
static void BitwiseORExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *BitwiseORExpression_parse(struct parser *parser);
static void LogicalANDExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static int LogicalANDExpression_isconst(struct node *na, 
        struct SEE_interpreter *interp);
static struct node *LogicalANDExpression_parse(struct parser *parser);
static void LogicalORExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static int LogicalORExpression_isconst(struct node *na, 
        struct SEE_interpreter *interp);
static struct node *LogicalORExpression_parse(struct parser *parser);
static void ConditionalExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static int ConditionalExpression_isconst(struct node *na, 
        struct SEE_interpreter *interp);
static struct node *ConditionalExpression_parse(struct parser *parser);
static void AssignmentExpression_simple_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_muleq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_diveq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_modeq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_addeq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_subeq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_lshifteq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_rshifteq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_urshifteq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_andeq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_xoreq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void AssignmentExpression_oreq_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *AssignmentExpression_parse(struct parser *parser);
static void Expression_comma_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *Expression_parse(struct parser *parser);
static struct node *Statement_parse(struct parser *parser);
static void Block_empty_eval(struct node *n, struct SEE_context *context, 
        struct SEE_value *res);
static struct node *Block_parse(struct parser *parser);
static void StatementList_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static struct node *StatementList_parse(struct parser *parser);
static void VariableStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *VariableStatement_parse(struct parser *parser);
static void VariableDeclarationList_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *VariableDeclarationList_parse(struct parser *parser);
static void VariableDeclaration_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *VariableDeclaration_parse(struct parser *parser);
static void EmptyStatement_eval(struct node *n, struct SEE_context *context, 
        struct SEE_value *res);
static struct node *EmptyStatement_parse(struct parser *parser);
static void ExpressionStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *ExpressionStatement_parse(struct parser *parser);
static void IfStatement_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static int IfStatement_isconst(struct node *na, 
        struct SEE_interpreter *interp);
static struct node *IfStatement_parse(struct parser *parser);
static void IterationStatement_dowhile_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static int IterationStatement_dowhile_isconst(struct node *na, 
        struct SEE_interpreter *interp);
static void IterationStatement_while_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static int IterationStatement_while_isconst(struct node *na, 
        struct SEE_interpreter *interp);
static void IterationStatement_for_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static int IterationStatement_for_isconst(struct node *na, 
        struct SEE_interpreter *interp);
static void IterationStatement_forvar_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void IterationStatement_forin_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void IterationStatement_forvarin_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *IterationStatement_parse(struct parser *parser);
static void ContinueStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *ContinueStatement_parse(struct parser *parser);
static void BreakStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *BreakStatement_parse(struct parser *parser);
static void ReturnStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void ReturnStatement_undef_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *ReturnStatement_parse(struct parser *parser);
static void WithStatement_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static struct node *WithStatement_parse(struct parser *parser);
static void SwitchStatement_caseblock(struct SwitchStatement_node *n, 
        struct SEE_context *context, struct SEE_value *input, 
        struct SEE_value *res);
static void SwitchStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *SwitchStatement_parse(struct parser *parser);
static void LabelledStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *LabelledStatement_parse(struct parser *parser);
static void ThrowStatement_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *ThrowStatement_parse(struct parser *parser);
static void TryStatement_catch(struct TryStatement_node *n, 
        struct SEE_context *context, struct SEE_value *C, 
        struct SEE_value *res);
static void TryStatement_catch_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void TryStatement_finally_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void TryStatement_catchfinally_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *TryStatement_parse(struct parser *parser);
static void FunctionDeclaration_fproc(struct node *na, 
        struct SEE_context *context);
static struct node *FunctionDeclaration_parse(struct parser *parser);
static void FunctionExpression_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static struct node *FunctionExpression_parse(struct parser *parser);
static struct var *FormalParameterList_parse(struct parser *parser);
static void FunctionBody_eval(struct node *na, struct SEE_context *context, 
        struct SEE_value *res);
static struct node *FunctionBody_parse(struct parser *parser);
static struct node *FunctionStatement_parse(struct parser *parser);
static struct function *Program_parse(struct parser *parser);
static void SourceElements_eval(struct node *na, 
        struct SEE_context *context, struct SEE_value *res);
static void SourceElements_fproc(struct node *na, 
        struct SEE_context *context);
static struct node *SourceElements_parse(struct parser *parser);
static int FunctionBody_isempty(struct SEE_interpreter *interp, 
        struct node *body);
static void eval(struct SEE_context *context, struct SEE_object *thisobj, 
        int argc, struct SEE_value **argv, struct SEE_value *res);

#if WITH_PARSER_PRINT
static void Literal_print(struct node *na, struct printer *printer);
static void StringLiteral_print(struct node *na, struct printer *printer);
static void RegularExpressionLiteral_print(struct node *na, 
        struct printer *printer);
static void PrimaryExpression_this_print(struct node *n, 
        struct printer *printer);
static void PrimaryExpression_ident_print(struct node *na, 
        struct printer *printer);
static void ArrayLiteral_print(struct node *na, struct printer *printer);
static void ObjectLiteral_print(struct node *na, struct printer *printer);
static void Arguments_print(struct node *na, struct printer *printer);
static void MemberExpression_new_print(struct node *na, 
        struct printer *printer);
static void MemberExpression_dot_print(struct node *na, 
        struct printer *printer);
static void MemberExpression_bracket_print(struct node *na, 
        struct printer *printer);
static void CallExpression_print(struct node *na, struct printer *printer);
static void Unary_print(struct node *na, struct printer *printer);
static void PostfixExpression_inc_print(struct node *na, 
        struct printer *printer);
static void PostfixExpression_dec_print(struct node *na, 
        struct printer *printer);
static void UnaryExpression_delete_print(struct node *na, 
        struct printer *printer);
static void UnaryExpression_void_print(struct node *na, 
        struct printer *printer);
static void UnaryExpression_typeof_print(struct node *na, 
        struct printer *printer);
static void UnaryExpression_preinc_print(struct node *na, 
        struct printer *printer);
static void UnaryExpression_predec_print(struct node *na, 
        struct printer *printer);
static void UnaryExpression_plus_print(struct node *na, 
        struct printer *printer);
static void UnaryExpression_minus_print(struct node *na, 
        struct printer *printer);
static void UnaryExpression_inv_print(struct node *na, 
        struct printer *printer);
static void UnaryExpression_not_print(struct node *na, 
        struct printer *printer);
static void Binary_print(struct node *na, struct printer *printer);
static void MultiplicativeExpression_mul_print(struct node *na, 
        struct printer *printer);
static void MultiplicativeExpression_div_print(struct node *na, 
        struct printer *printer);
static void MultiplicativeExpression_mod_print(struct node *na, 
        struct printer *printer);
static void AdditiveExpression_add_print(struct node *na, 
        struct printer *printer);
static void AdditiveExpression_sub_print(struct node *na, 
        struct printer *printer);
static void ShiftExpression_lshift_print(struct node *na, 
        struct printer *printer);
static void ShiftExpression_rshift_print(struct node *na, 
        struct printer *printer);
static void ShiftExpression_urshift_print(struct node *na, 
        struct printer *printer);
static void RelationalExpression_lt_print(struct node *na, 
        struct printer *printer);
static void RelationalExpression_gt_print(struct node *na, 
        struct printer *printer);
static void RelationalExpression_le_print(struct node *na, 
        struct printer *printer);
static void RelationalExpression_ge_print(struct node *na, 
        struct printer *printer);
static void RelationalExpression_instanceof_print(struct node *na, 
        struct printer *printer);
static void RelationalExpression_in_print(struct node *na, 
        struct printer *printer);
static void EqualityExpression_eq_print(struct node *na, 
        struct printer *printer);
static void EqualityExpression_ne_print(struct node *na, 
        struct printer *printer);
static void EqualityExpression_seq_print(struct node *na, 
        struct printer *printer);
static void EqualityExpression_sne_print(struct node *na, 
        struct printer *printer);
static void BitwiseANDExpression_print(struct node *na, 
        struct printer *printer);
static void BitwiseXORExpression_print(struct node *na, 
        struct printer *printer);
static void BitwiseORExpression_print(struct node *na, 
        struct printer *printer);
static void LogicalANDExpression_print(struct node *na, 
        struct printer *printer);
static void LogicalORExpression_print(struct node *na, 
        struct printer *printer);
static void ConditionalExpression_print(struct node *na, 
        struct printer *printer);
static void AssignmentExpression_simple_print(struct node *na, 
        struct printer *printer);
static void AssignmentExpression_muleq_print(struct node *na, 
        struct printer *printer);
static void AssignmentExpression_diveq_print(struct node *na, 
        struct printer *printer);
static void AssignmentExpression_modeq_print(struct node *na, 
        struct printer *printer);
static void AssignmentExpression_addeq_print(struct node *na, 
        struct printer *printer);
static void AssignmentExpression_subeq_print(struct node *na, 
        struct printer *printer);
static void AssignmentExpression_lshifteq_print(struct node *na, 
        struct printer *printer);
static void AssignmentExpression_rshifteq_print(struct node *na, 
        struct printer *printer);
static void AssignmentExpression_urshifteq_print(struct node *na, 
        struct printer *printer);
static void AssignmentExpression_andeq_print(struct node *na, 
        struct printer *printer);
static void AssignmentExpression_xoreq_print(struct node *na, 
        struct printer *printer);
static void AssignmentExpression_oreq_print(struct node *na, 
        struct printer *printer);
static void Expression_comma_print(struct node *na, struct printer *printer);
static void Block_empty_print(struct node *n, struct printer *printer);
static void VariableStatement_print(struct node *na, 
        struct printer *printer);
static void VariableDeclarationList_print(struct node *na, 
        struct printer *printer);
static void VariableDeclaration_print(struct node *na, 
        struct printer *printer);
static void EmptyStatement_print(struct node *n, struct printer *printer);
static void ExpressionStatement_print(struct node *na, 
        struct printer *printer);
static void IfStatement_print(struct node *na, struct printer *printer);
static void IterationStatement_dowhile_print(struct node *na, 
        struct printer *printer);
static void IterationStatement_while_print(struct node *na, 
        struct printer *printer);
static void IterationStatement_for_print(struct node *na, 
        struct printer *printer);
static void IterationStatement_forvar_print(struct node *na, 
        struct printer *printer);
static void IterationStatement_forin_print(struct node *na, 
        struct printer *printer);
static void IterationStatement_forvarin_print(struct node *na, 
        struct printer *printer);
static void ContinueStatement_print(struct node *na, 
        struct printer *printer);
static void BreakStatement_print(struct node *na, struct printer *printer);
static void ReturnStatement_print(struct node *na, struct printer *printer);
static void ReturnStatement_undef_print(struct node *na, 
        struct printer *printer);
static void WithStatement_print(struct node *na, struct printer *printer);
static void SwitchStatement_print(struct node *na, struct printer *printer);
static void LabelledStatement_print(struct node *na, struct printer *printer);
static void ThrowStatement_print(struct node *na, struct printer *printer);
static void TryStatement_catch_print(struct node *na, 
        struct printer *printer);
static void TryStatement_finally_print(struct node *na, 
        struct printer *printer);
static void TryStatement_catchfinally_print(struct node *na, 
        struct printer *printer);
static void Function_print(struct node *na, struct printer *printer);
static void SourceElements_print(struct node *na, struct printer *printer);
static void printer_init(struct printer *printer, 
        struct SEE_interpreter *interp, struct printerclass *printerclass);
static void printer_atbol(struct printer *printer);
static void printer_print_newline(struct printer *printer, int indent);
static void printer_print_node(struct printer *printer, struct node *n);
static void print_hex(struct printer *printer, int i);
static void stdio_print_string(struct printer *printer, 
        struct SEE_string *s);
static void stdio_print_char(struct printer *printer, int c);
static void stdio_print_node(struct printer *printer, struct node *n);
static struct printer *stdio_printer_new(struct SEE_interpreter *interp, 
        FILE *output);
static void string_print_string(struct printer *printer, 
        struct SEE_string *s);
static void string_print_char(struct printer *printer, int c);
static struct printer *string_printer_new(struct SEE_interpreter *interp, 
        struct SEE_string *string);
static void print_functionbody(struct SEE_interpreter *interp, 
        struct function *f, FILE *fp);
#endif

#if WITH_PARSER_VISIT
static void ArrayLiteral_visit(struct node *na, visitor_fn_t v, void *va);
static void ObjectLiteral_visit(struct node *na, visitor_fn_t v, void *va);
static void Arguments_visit(struct node *na, visitor_fn_t v, void *va);
static void MemberExpression_new_visit(struct node *na, visitor_fn_t v, 
        void *va);
static void MemberExpression_dot_visit(struct node *na, visitor_fn_t v, 
        void *va);
static void MemberExpression_bracket_visit(struct node *na, visitor_fn_t v, 
        void *va);
static void CallExpression_visit(struct node *na, visitor_fn_t v, void *va);
static void Unary_visit(struct node *na, visitor_fn_t v, void *va);
static void Binary_visit(struct node *na, visitor_fn_t v, void *va);
static void ConditionalExpression_visit(struct node *na, visitor_fn_t v, 
        void *va);
static void AssignmentExpression_visit(struct node *na, visitor_fn_t v, 
        void *va);
static void VariableDeclaration_visit(struct node *na, visitor_fn_t v, 
        void *va);
static void IfStatement_visit(struct node *na, visitor_fn_t v, void *va);
static void IterationStatement_while_visit(struct node *na, visitor_fn_t v, 
        void *va);
static void IterationStatement_for_visit(struct node *na, visitor_fn_t v, 
        void *va);
static void IterationStatement_forin_visit(struct node *na, visitor_fn_t v, 
        void *va);
static void ReturnStatement_visit(struct node *na, visitor_fn_t v, void *va);
static void SwitchStatement_visit(struct node *na, visitor_fn_t v, void *va);
static void TryStatement_catch_visit(struct node *na, visitor_fn_t v, 
        void *va);
static void TryStatement_finally_visit(struct node *na, visitor_fn_t v, 
        void *va);
static void TryStatement_catchfinally_visit(struct node *na, visitor_fn_t v, 
        void *va);
static void Function_visit(struct node *na, visitor_fn_t v, void *va);
static void SourceElements_visit(struct node *na, visitor_fn_t v, void *va);
#endif

#define CONTINUABLE 1
static int target_matches(struct Targetable_node *, void *);
static void target_init(struct Targetable_node *, struct parser *, int);

/*------------------------------------------------------------
 * macros
 */

/*
 * Macros for accessing the tokeniser, with lookahead
 */
#define NEXT 						\
	(parser->unget != parser->unget_end		\
		? parser->unget_tok[parser->unget] 	\
		: parser->lex->next)
#define NEXT_VALUE					\
	(parser->unget != parser->unget_end		\
		? &parser->unget_val[parser->unget] 	\
		: &parser->lex->value)

#define NEXT_LINENO					\
	(parser->unget != parser->unget_end		\
		? parser->unget_lin[parser->unget] 	\
		: parser->lex->next_lineno)

#define NEXT_FILENAME					\
		  parser->lex->next_filename

#define NEXT_FOLLOWS_NL					\
	(parser->unget != parser->unget_end		\
		? parser->unget_fnl[parser->unget] 	\
		: parser->lex->next_follows_nl)

#define SKIP						\
    do {						\
	if (parser->unget == parser->unget_end)		\
		SEE_lex_next(parser->lex);		\
	else 						\
		parser->unget = 			\
		    (parser->unget + 1) % UNGET_MAX;	\
	SKIP_DEBUG					\
    } while (0)

#ifndef NDEBUG
#  define SKIP_DEBUG					\
    if (SEE_parse_debug)				\
      dprintf("SKIP: next = %s\n", SEE_tokenname(NEXT));
#else
#  define SKIP_DEBUG
#endif

/* Handy macros for describing syntax errors */
#define EXPECT(c) EXPECTX(c, SEE_tokenname(c))
#define EXPECTX(c, tokstr)				\
    do { 						\
	EXPECTX_NOSKIP(c, tokstr);			\
	SKIP;						\
    } while (0)
#define EXPECT_NOSKIP(c) EXPECTX_NOSKIP(c, SEE_tokenname(c))
#define EXPECTX_NOSKIP(c, tokstr)			\
    do { 						\
	if (NEXT != (c)) 				\
	    EXPECTED(tokstr);				\
    } while (0)
#define EXPECTED(tokstr)				\
    do { 						\
	    char nexttok[30];				\
	    SEE_tokenname_buf(NEXT, nexttok, 		\
		sizeof nexttok);			\
	    SEE_error_throw_string(			\
		parser->interpreter,			\
		parser->interpreter->SyntaxError,	\
		error_at(parser, 			\
		         "expected %s but got %s",	\
		         tokstr,			\
		         nexttok));			\
    } while (0)

#define EMPTY_LABEL		((struct SEE_string *)NULL)

/* 
 * Automatic semicolon insertion macros.
 *
 * Using these instead of NEXT/SKIP allows synthesis of
 * semicolons where they are permitted by the standard.
 */
#define NEXT_IS_SEMICOLON				\
	(NEXT == ';' || NEXT == '}' || NEXT_FOLLOWS_NL)
#define EXPECT_SEMICOLON				\
    do {						\
	if (NEXT == ';')				\
		SKIP;					\
	else if ((NEXT == '}' || NEXT_FOLLOWS_NL)) {	\
		/* automatic semicolon insertion */	\
	} else						\
		EXPECTX(';', "';', '}' or newline");	\
    } while (0)

/* Traces a statement-level event, or an eval() */
#define TRACE(node, ctxt, event)				\
    do {						\
	if (ctxt) {					\
            if (SEE_system.periodic)			\
	    	(*SEE_system.periodic)((ctxt)->interpreter); \
	    (ctxt)->interpreter->try_location =		\
		&(node)->location;			\
	    trace_event(ctxt, event);				\
	}						\
    } while (0)

/*
 * Macros for accessing the abstract syntax tree
 */

#ifndef NDEBUG

# if !HAVE___FUNCTION__
   /* Some trickery to stringize the __LINE__ macro */
#  define X_STR2(s) #s
#  define X_STR(s) X_STR2(s)
#  define __FUNCTION__   __FILE__ ":" X_STR(__LINE__)
# endif

# define EVAL_DEBUG_ENTER(node)				\
	if (SEE_eval_debug) 				\
	    dprintf("eval: %s enter %p\n", 		\
		__FUNCTION__, node);

# define EVAL_DEBUG_LEAVE(node, ctxt, res)		\
	if (SEE_eval_debug && (ctxt)) {			\
	    dprintf("eval: %s leave %p -> %p = ", 	\
		__FUNCTION__, node, (void *)(res));	\
	    dprintv((ctxt)->interpreter, res);		\
	    dprintf("\n");				\
	}

#else /* NDEBUG */

# define EVAL_DEBUG_ENTER(node)
# define EVAL_DEBUG_LEAVE(node, ctxt, res)

#endif /* NDEBUG */

# define EVAL(node, ctxt, res)				\
    do {						\
	struct SEE_throw_location * _loc_save = NULL;	\
	EVAL_DEBUG_ENTER(node)				\
	if (ctxt) {					\
	  _loc_save = (ctxt)->interpreter->try_location;\
	  (ctxt)->interpreter->try_location =		\
		&(node)->location;			\
	}						\
	(*(node)->nodeclass->eval)(node, ctxt, res);	\
	EVAL_DEBUG_LEAVE(node, ctxt, res)		\
    } while (0)
  /*
   * Note: there is no need to restore the _loc_save in
   * a try-finally block
   */

#define FPROC(node, ctxt)				\
    do {						\
	if ((node)->nodeclass->fproc)			\
	    (*(node)->nodeclass->fproc)((node), ctxt);	\
    } while (0)

#ifndef NDEBUG
#define NEW_NODE(t, nc)					\
	((t *)new_node(parser, sizeof (t), nc, #nc))
#else
#define NEW_NODE(t, nc)					\
	((t *)new_node(parser, sizeof (t), nc, NULL))
#endif

#ifdef NDEBUG
#define CAST_NODE(na, cls)				\
	((struct cls##_node *)(na))
#else
#define CAST_NODE(na, cls)				\
	((struct cls##_node *)cast_node(na, 		\
		&cls##_nodeclass, #cls, __FILE__, __LINE__))
static struct node *cast_node(struct node *, struct nodeclass *, 
	const char *, const char *, int);
#endif

#ifndef NDEBUG
#define PARSE(prod)					\
    ((void)(SEE_parse_debug ? 				\
	dprintf("parse %s next=%s\n", #prod,		\
	    SEE_tokenname(NEXT)) : (void)0),		\
        prod##_parse(parser))
#else
#define PARSE(prod)					\
        prod##_parse(parser)
#endif

/* Generates a generic parse error */
#define ERROR						\
	SEE_error_throw_string(				\
	    parser->interpreter,			\
	    parser->interpreter->SyntaxError,		\
	    error_at(parser, "parse error before %s",	\
	    SEE_tokenname(NEXT)))

/* Generates a specific parse error */
#define ERRORm(m)					\
	SEE_error_throw_string(				\
	    parser->interpreter,			\
	    parser->interpreter->SyntaxError,		\
	    error_at(parser, "%s, near %s",		\
	    m, SEE_tokenname(NEXT)))

#if WITH_PARSER_PRINT
/*
 * Printer macros
 */
# define PRINT(n)	 (*printer->printerclass->print_node)(printer, n)
# define PRINT_STRING(s) (*printer->printerclass->print_string)(printer, s)
# define PRINT_CSTRING(s)(*printer->printerclass->print_cstring)(printer, s)
# define PRINT_CHAR(c)	 (*printer->printerclass->print_char)(printer, c)
# define PRINT_NEWLINE(i)(*printer->printerclass->print_newline)(printer, i)
# define PRINTP(n) do {					\
	     PRINT_CHAR('(');				\
	     PRINT(n);					\
	     PRINT_CHAR(')');				\
    } while (0)
#endif

#if WITH_PARSER_VISIT
/*
 * Visitor macro
 */
# define VISIT(n, v, va)	do {			\
	if ((n)->nodeclass->visit)			\
	    (*(n)->nodeclass->visit)(n, v, va);		\
	(*(v))(n, va);					\
    } while (0)
#endif

/* Returns true if the node returns a constant expression */
#define ISCONST(n, interp) 				\
	((n)->isconst_valid ? (n)->isconst :		\
	  ((n)->isconst_valid = 1,			\
	   (n)->isconst =				\
	    ((n)->nodeclass->isconst  			\
		? (*(n)->nodeclass->isconst)(n, interp)	\
		: 0)))

/*------------------------------------------------------------
 * Allocators and initialisers
 */

/*
 * Creates a new AST node, initialising it with the 
 * given node class nc, and recording the current
 * line number as reported by the parser.
 */
static struct node *
new_node(parser, sz, nc, dbg_nc)
	struct parser *parser;
	int sz;
	struct nodeclass *nc;
	const char *dbg_nc;
{
	struct node *n;

	n = (struct node *)SEE_malloc(parser->interpreter, sz);
	n->nodeclass = nc;
	n->location.filename = NEXT_FILENAME;
	n->location.lineno = NEXT_LINENO;

#ifndef NDEBUG
	if (SEE_parse_debug) 
		dprintf("parse: %p %s (next=%s)\n", 
			n, dbg_nc, SEE_tokenname(NEXT));
#endif

	return n;
}

#ifndef NDEBUG
/*
 * Checks that the node pointer na has node class nc in its class chain.
 * Used by the CAST_NODE() macro to runtime check casts.
 */
static struct node *
cast_node(na, nc, cname, file, line)
	struct node *na;
	struct nodeclass *nc;
	const char *cname;
	const char *file;
	int line;
{
	if (na) {
		struct nodeclass *nac = na->nodeclass;
		while (nac && nac != nc)
		    nac = nac->superclass;
		if (!nac) {
		    dprintf("%s:%d: internal error: cast to %s failed (source class from %s:%d) [vers %s]\n",
			file, line, cname, na->nodeclass->decl_file,
		        na->nodeclass->decl_line, SEE_SJ_PACKAGE_VERSION);
		    abort();
		}
	}
	return na;
}
#endif

/*
 * Initialises a parser state.
 */
static void
parser_init(parser, interp, lex)
	struct parser *parser;
	struct SEE_interpreter *interp;
	struct lex *lex;
{
	parser->interpreter = interp;
	parser->lex = lex;
	parser->unget = 0;
	parser->unget_end = 0;
	parser->noin = 0;
	parser->is_lhs = 0;
	parser->labels = NULL;
	parser->vars = NULL;
	parser->funcdepth = 0;
}

/*------------------------------------------------------------
 * Labels
 *
 * LabelledStatements that appear consecutively are converted
 * into a 'label set' named by the first label. Each successive 
 * label becomes an alias for the first.
 *
 * Only Iteration/SwitchStatements that are immediately parented by 
 * a LabelledStatement are passed a 'current label set' consisting of 
 * either NULL or the first label. The Iteration/SwitchStatement then
 * translates break/continues to that labelset into the equivalent 
 * break/continues with an empty label. (All other statements ignore
 * their 'current label set', which is typically empty anyway.)
 *
 * During parse, the stack of currently active labels, is available
 * in parser->labels. This is used to check for duplicates, and
 * also to map identifier targets of 'break' and 'continue' to the
 * first label of a labelset.
 */

/*
 * Pushes a label onto the current label stack.
 * Checks for duplicate labels already in the current label
 * stack.
 */
static void
label_push(parser, name, labelset, continuable)
	struct parser *parser;
	struct SEE_string *name;
	struct SEE_string *labelset;
	int continuable;
{
	struct label *l;
	struct SEE_string *msg;
	struct SEE_throw_location location;

	location.lineno = NEXT_LINENO;
	location.filename = NEXT_FILENAME;

	/* check for duplicate labels */
	if (name)
	    for (l = parser->labels; l; l = l->next)
		if (l->name == name) {
		    msg = SEE_location_string(parser->interpreter, &location);
		    SEE_string_append(msg, STR(duplicate_label));
		    SEE_string_append(msg, name);
		    SEE_string_addch(msg, '\'');
		    SEE_string_addch(msg, ';');
		    SEE_string_addch(msg, ' ');
		    SEE_string_append(msg, 
			SEE_location_string(parser->interpreter, 
			    &l->location));
		    SEE_string_append(msg, STR(previous_definition));
		    SEE_error_throw_string(parser->interpreter,
			    parser->interpreter->SyntaxError, msg);
		}

	l = SEE_NEW(parser->interpreter, struct label);
	l->name = name;
	l->labelset = labelset;
	l->next = parser->labels;
	l->location.lineno = location.lineno;
	l->location.filename = location.filename;
	l->continuable = continuable;
	parser->labels = l;
}

/*
 * Pops the last pushed label from the current label stack.
 */
static void
label_pop(parser)
	struct parser *parser;
{
	struct label *l;

	if (!parser->labels)
	    SEE_error_throw_string(parser->interpreter,
		parser->interpreter->SyntaxError,
		STR(internal_error));
	l = parser->labels;
	parser->labels = l->next;
	l->next = NULL;
	SEE_free(parser->interpreter, (void **)&l);
}

/*
 * Marks a label as being a valid target for the continue statement.
 */
static void
label_set_continuable(parser, labelset)
	struct parser *parser;
	struct SEE_string *labelset;
{
	struct label *l;

	for (l = parser->labels; l; l = l->next)
	    if (l->labelset == labelset)
	    	break;
	SEE_ASSERT(parser->interpreter, l != NULL);
	l->continuable = 1;
}

/*
 * Looks for the given label name (or implied name), raising a
 * SyntaxError if it isn't found. Kind is a token indicates the kind of
 * statement using the label (tBREAK or tCONTINUE).
 */
static struct label *
label_lookup(parser, name, kind)
	struct parser *parser;
	struct SEE_string *name;
	int kind;
{
	struct SEE_string *msg;
	struct label *l;

	SEE_ASSERT(parser->interpreter, kind == tBREAK || kind == tCONTINUE);

#ifndef NDEBUG
	if (SEE_parse_debug) {
	    dprintf("label_find: searching for '");
	    if (name == EMPTY_LABEL)
	        dprintf("EMPTY_LABEL");
	    else
	    	dprints(name);
	    dprintf("\n");
	}
#endif
	for (l = parser->labels; l; l = l->next)
	    if (l->name == name) {
	        if (kind == tCONTINUE && !l->continuable) {
		    if (name == NULL)
		        continue;
		    msg = error_at(parser, "label '");
		    SEE_string_append(msg, name);
		    SEE_string_append(msg, 
			SEE_string_sprintf(parser->interpreter,
			"' not suitable for continue"));
		    SEE_error_throw_string(parser->interpreter,
			    parser->interpreter->SyntaxError, msg);
		}
		return l;
	    }

	if (name) {
	    msg = error_at(parser, "label '");
	    SEE_string_append(msg, name);
	    SEE_string_append(msg, 
		SEE_string_sprintf(parser->interpreter,
		"' not defined, or not reachable"));
	} else if (kind == tCONTINUE)
	    msg = error_at(parser,
		"continue statement not within a loop");
	else /* kind == tBREAK */
	    msg = error_at(parser,
		"break statement not within loop or switch");

	SEE_error_throw_string(parser->interpreter,
		parser->interpreter->SyntaxError, msg);
	/* NOTREACHED */
	return 0;
}

/*
 * Returns the labelset associated with a label.
 * The label argument may be NULL, in which case NULL is returned.
 */
static void *
label_get_target(label)
	struct label *label;
{
	return label ? label->labelset : NULL;
}

/*
 * Returns true if the node is a valid target of the labelset target.
 */
static int
target_matches(itn, ls_target)
	struct Targetable_node *itn;
	void *ls_target;
{
	return ls_target == NULL || ls_target == itn->target;
}

/*
 * Initialises a targetable node with the current label set,
 * and optionally modifies the labelset to be continuable.
 */
static void
target_init(itn, parser, continuable)
	struct Targetable_node *itn;
	struct parser *parser;
	int continuable;
{
	itn->target = (void *)parser->current_labelset;
	if (continuable && parser->current_labelset)
	    label_set_continuable(parser, parser->current_labelset);
}

/*------------------------------------------------------------
 * LL(2) lookahead implementation
 */

/*
 * Returns the token that is n tokens ahead. (0 is the next token.)
 */
static int
lookahead(parser, n)
	struct parser *parser;
	int n;
{
	int token;
	SEE_ASSERT(parser->interpreter, n < (UNGET_MAX - 1));

	while ((UNGET_MAX + parser->unget_end - parser->unget) % UNGET_MAX < n)
	{
	    SEE_VALUE_COPY(&parser->unget_val[parser->unget_end], 
		&parser->lex->value);
	    parser->unget_tok[parser->unget_end] =
		parser->lex->next;
	    parser->unget_lin[parser->unget_end] =
		parser->lex->next_lineno;
	    parser->unget_fnl[parser->unget_end] = 
		parser->lex->next_follows_nl;
	    SEE_lex_next(parser->lex);
	    parser->unget_end = (parser->unget_end + 1) % UNGET_MAX;
	}
	if ((parser->unget + n) % UNGET_MAX == parser->unget_end)
		token = parser->lex->next;
	else
		token = parser->unget_tok[(parser->unget + n) % UNGET_MAX];

#ifndef NDEBUG
	if (SEE_parse_debug)
	    dprintf("lookahead(%d) -> %s\n", n, SEE_tokenname(token));
#endif

	return token;
}

/*
 * Generates a trace event, giving the host application an opportunity to
 * step or trace execution.
 */
static void
trace_event(ctxt, event)
	struct SEE_context *ctxt;
	enum SEE_trace_event event;
{
	if (ctxt->interpreter->trace)
	    (*ctxt->interpreter->trace)(ctxt->interpreter,
		ctxt->interpreter->try_location, ctxt, event);
}

/*
 * Pushes a new call context entry onto the traceback stack.
 * Returns the old traceback stack.
 */
static struct SEE_traceback *
traceback_enter(interp, callee, loc, call_type)
	struct SEE_interpreter *interp;
	struct SEE_object *callee;
	struct SEE_throw_location *loc;
	int call_type;
{
	struct SEE_traceback *old_tb, *tb;

	old_tb = interp->traceback;

	tb = SEE_NEW(interp, struct SEE_traceback);
	tb->call_location = loc;
	tb->callee = callee;
	tb->call_type = call_type;
	tb->prev = old_tb;
	interp->traceback = tb;

	return old_tb;
}

/*
 * Restores the traceback list before a call context was entered.
 */
static void
traceback_leave(interp, old_tb)
	struct SEE_interpreter *interp;
	struct SEE_traceback *old_tb;
{
	interp->traceback = old_tb;
}

/*------------------------------------------------------------
 * GetValue/SetValue
 */

/* 8.7.1 */
static void
GetValue(context, v, res)
	struct SEE_context *context;
	struct SEE_value *v;
	struct SEE_value *res;
{
	struct SEE_interpreter *interp = context->interpreter;

	if (SEE_VALUE_GET_TYPE(v) != SEE_REFERENCE) {
		if (v != res)
			SEE_VALUE_COPY(res, v);
		return;
	}
	if (v->u.reference.base == NULL)
		SEE_error_throw_string(interp, interp->ReferenceError, 
		    v->u.reference.property);
	else
		SEE_OBJECT_GET(interp, v->u.reference.base, 
		    v->u.reference.property, res);
}

/* 8.7.2 */
static void
PutValue(context, v, w)
	struct SEE_context *context;
	struct SEE_value *v;
	struct SEE_value *w;
{
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_object *target;

	if (SEE_VALUE_GET_TYPE(v) != SEE_REFERENCE)
		SEE_error_throw_string(interp, interp->ReferenceError,
		    STR(bad_lvalue));
	target = v->u.reference.base;
	if (target == NULL)
		target = interp->Global;
	SEE_OBJECT_PUT(interp, target, v->u.reference.property, w, 0);
}

/*------------------------------------------------------------
 * Error handling
 */

/*
 * Generates an error string prefixed with the filename and 
 * line number of the next token. e.g. "foo.js:23: blah blah".
 * This is useful for error messages.
 */
static struct SEE_string *
error_at(struct parser *parser, const char *fmt, ...)
{
	va_list ap;
	struct SEE_throw_location here;
	struct SEE_string *msg;
	struct SEE_interpreter *interp = parser->interpreter;

	here.lineno = NEXT_LINENO;
	here.filename = NEXT_FILENAME;

	va_start(ap, fmt);
	msg = SEE_string_vsprintf(interp, fmt, ap);
	va_end(ap);

	return SEE_string_concat(interp,
	    SEE_location_string(interp, &here), msg);
}

/*------------------------------------------------------------
 * Constant subexpression reduction
 *
 *  A subtree is 'constant' iff it
 *	- has no side-effects; and
 *	- yields the same result independent of context
 *  It follows then that a constant subtree can be evaluated using 
 *  a NULL context. We can perform that eval, and then replace the
 *  subtree with a node that generates that expression statically.
 */

/* Always returns true to indicate this class of node is always constant. */
static int
Always_isconst(na, interp)
	struct node *na;
	struct SEE_interpreter *interp;
{
	return 1;
}

/*------------------------------------------------------------
 * Parser
 *
 * Each group of grammar productions is ordered:
 *   - production summary as a comment
 *   - node structure
 *   - evaluator function
 *   - function processor
 *   - node printer
 *   - recursive-descent parser
 */

/* -- 7.8
 *	Literal:
 *	 	NullLiteral
 *	 	BooleanLiteral
 *	 	NumericLiteral
 *	 	StringLiteral
 *
 *	NullLiteral:
 *		tNULL				-- 7.8.1
 *
 *	BooleanLiteral:
 *		tTRUE				-- 7.8.2
 *		tFALSE				-- 7.8.2
 */

struct Literal_node {
	struct node node;
	struct SEE_value value;
};

static void
Literal_eval(na, context, res)
	struct node *na; /* (struct Literal_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Literal_node *n = CAST_NODE(na, Literal);
	SEE_VALUE_COPY(res, &n->value);
}

#if WITH_PARSER_PRINT
static void
Literal_print(na, printer)
	struct node *na; /* (struct Literal_node) */
	struct printer *printer;
{
	struct Literal_node *n = CAST_NODE(na, Literal);
	struct SEE_value str;

	switch (SEE_VALUE_GET_TYPE(&n->value)) {
	case SEE_BOOLEAN:
		PRINT_STRING(n->value.u.boolean
			? STR(true)
			: STR(false));
		break;
	case SEE_NUMBER:
		SEE_ToString(printer->interpreter, &n->value, &str);
		PRINT_STRING(str.u.string);
		break;
	case SEE_NULL:
		PRINT_STRING(STR(null));
		break;
	default:
		PRINT_CHAR('?');
	}
	PRINT_CHAR(' ');
}
#endif

static struct nodeclass Literal_nodeclass 
	= { BASECLASS
	    Literal_eval, 0, 
	    PARSER_PRINT(Literal_print)
	    PARSER_VISIT(0)
	    Always_isconst };

static struct node *
Literal_parse(parser)
	struct parser *parser;
{
	struct Literal_node *n;

	/*
	 * Convert the next token into a regular expression
	 * if possible
	 */

	switch (NEXT) {
	case tNULL:
		n = NEW_NODE(struct Literal_node, &Literal_nodeclass);
		SEE_SET_NULL(&n->value);
		SKIP;
		return (struct node *)n;
	case tTRUE:
	case tFALSE:
		n = NEW_NODE(struct Literal_node,  &Literal_nodeclass);
		SEE_SET_BOOLEAN(&n->value, (NEXT == tTRUE));
		SKIP;
		return (struct node *)n;
	case tNUMBER:
		return PARSE(NumericLiteral);
	case tSTRING:
		return PARSE(StringLiteral);
	case tDIV:
	case tDIVEQ:
		SEE_lex_regex(parser->lex);
		return PARSE(RegularExpressionLiteral);
	default:
		EXPECTED("null, true, false, number, string, or regex");
	}
	/* NOTREACHED */
	return 0;
}

/*
 *	NumericLiteral:
 *		tNUMBER				-- 7.8.3
 */
static struct node *
NumericLiteral_parse(parser)
	struct parser *parser;
{
	struct Literal_node *n;

	EXPECT_NOSKIP(tNUMBER);
	n = NEW_NODE(struct Literal_node, &Literal_nodeclass);
	SEE_VALUE_COPY(&n->value, NEXT_VALUE);
	SKIP;
	return (struct node *)n;
}

/*
 *	StringLiteral:
 *		tSTRING				-- 7.8.4
 */

struct StringLiteral_node {
	struct node node;
	struct SEE_string *string;
};

static void
StringLiteral_eval(na, context, res)
	struct node *na; /* (struct StringLiteral_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct StringLiteral_node *n = CAST_NODE(na, StringLiteral);
	SEE_SET_STRING(res, n->string);
}

#if WITH_PARSER_PRINT
static void
StringLiteral_print(na, printer)
	struct node *na; /* (struct StringLiteral_node) */
	struct printer *printer;
{
	struct StringLiteral_node *n = CAST_NODE(na, StringLiteral);
	unsigned int i;

	PRINT_CHAR('"');
	for (i = 0; i < n->string->length; i ++) {
		SEE_char_t c = n->string->data[i];
		if (c == '\\' || c == '\"') {
			PRINT_CHAR('\\');
			PRINT_CHAR(c & 0x7f);
		} else if (c >= ' ' && c <= '~')
			PRINT_CHAR(c & 0x7f);
		else if (c < 0x100) {
			PRINT_CHAR('\\');
			PRINT_CHAR('x');
			PRINT_CHAR(SEE_hexstr_lowercase[ (c >> 4) & 0xf ]);
			PRINT_CHAR(SEE_hexstr_lowercase[ (c >> 0) & 0xf ]);
		} else {
			PRINT_CHAR('\\');
			PRINT_CHAR('u');
			PRINT_CHAR(SEE_hexstr_lowercase[ (c >>12) & 0xf ]);
			PRINT_CHAR(SEE_hexstr_lowercase[ (c >> 8) & 0xf ]);
			PRINT_CHAR(SEE_hexstr_lowercase[ (c >> 4) & 0xf ]);
			PRINT_CHAR(SEE_hexstr_lowercase[ (c >> 0) & 0xf ]);
		}
	}
	PRINT_CHAR('"');
	PRINT_CHAR(' ');
}
#endif

static struct nodeclass StringLiteral_nodeclass 
	= { BASECLASS
	    StringLiteral_eval, 0, 
	    PARSER_PRINT(StringLiteral_print)
	    PARSER_VISIT(0)
	    Always_isconst };

static struct node *
StringLiteral_parse(parser)
	struct parser *parser;
{
	struct StringLiteral_node *n;

	EXPECT_NOSKIP(tSTRING);
	n = NEW_NODE(struct StringLiteral_node, &StringLiteral_nodeclass);
	n->string = NEXT_VALUE->u.string;
	SKIP;
	return (struct node *)n;
}

/*
 *	RegularExpressionLiteral:
 *		tREGEX				-- 7.8.5
 */

struct RegularExpressionLiteral_node {
	struct node node;
	struct SEE_value pattern;
	struct SEE_value flags;
	struct SEE_value *argv[2];
};

static void
RegularExpressionLiteral_eval(na, context, res)
	struct node *na; /* (struct RegularExpressionLiteral_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct RegularExpressionLiteral_node *n = 
		CAST_NODE(na, RegularExpressionLiteral);
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_traceback *tb;

        tb = traceback_enter(interp, interp->RegExp, &n->node.location,
		SEE_CALLTYPE_CONSTRUCT);
	TRACE(na, context, SEE_TRACE_CALL);
	SEE_OBJECT_CONSTRUCT(interp, interp->RegExp, interp->RegExp, 
		2, n->argv, res);
	TRACE(na, context, SEE_TRACE_RETURN);
        traceback_leave(interp, tb);
}

#if WITH_PARSER_PRINT
static void
RegularExpressionLiteral_print(na, printer)
	struct node *na; /* (struct RegularExpressionLiteral_node) */
	struct printer *printer;
{
	struct RegularExpressionLiteral_node *n = 
		CAST_NODE(na, RegularExpressionLiteral);
	PRINT_CHAR('/');
	PRINT_STRING(n->pattern.u.string);
	PRINT_CHAR('/');
	PRINT_STRING(n->flags.u.string);
	PRINT_CHAR(' ');
}
#endif

static struct nodeclass RegularExpressionLiteral_nodeclass
	= { BASECLASS
	    RegularExpressionLiteral_eval, 0, 
	    PARSER_PRINT(RegularExpressionLiteral_print)
	    PARSER_VISIT(0)
	    0 };

static struct node *
RegularExpressionLiteral_parse(parser)
	struct parser *parser;
{
	struct RegularExpressionLiteral_node *n = NULL;
	struct SEE_string *s, *pattern, *flags;
	int p;

	if (NEXT == tREGEX)  {
	    /*
	     * Find the position after the regexp's closing '/'.
	     * i.e. the position of the regexp flags.
	     */
	    s = NEXT_VALUE->u.string;
	    for (p = s->length; p > 0; p--)
		    if (s->data[p-1] == '/')
			    break;
	    SEE_ASSERT(parser->interpreter, p > 1);

	    pattern = SEE_string_substr(parser->interpreter,
		s, 1, p - 2);
	    flags = SEE_string_substr(parser->interpreter,
		s, p, s->length - p);

	    n = NEW_NODE(struct RegularExpressionLiteral_node,
		&RegularExpressionLiteral_nodeclass);
	    SEE_SET_STRING(&n->pattern, pattern);
	    SEE_SET_STRING(&n->flags, flags);
	    n->argv[0] = &n->pattern;
	    n->argv[1] = &n->flags;

	}
	EXPECT(tREGEX);
	return (struct node *)n;
}

/*------------------------------------------------------------
 * -- 11.1
 *
 *	PrimaryExpression
 *	:	tTHIS				-- 11.1.1
 *	|	tIDENT				-- 11.1.2
 *	|	Literal				-- 11.1.3
 *	|	ArrayLiteral
 *	|	ObjectLiteral
 *	|	'(' Expression ')'		-- 11.1.6
 *	;
 */

/* 11.1.1 */
static void
PrimaryExpression_this_eval(n, context, res)
	struct node *n;
	struct SEE_context *context;
	struct SEE_value *res;
{
	SEE_SET_OBJECT(res, context->thisobj);
}

#if WITH_PARSER_PRINT
static void
PrimaryExpression_this_print(n, printer)
	struct printer *printer;
	struct node *n;
{
	PRINT_STRING(STR(this));
	PRINT_CHAR(' ');
}
#endif

static struct nodeclass PrimaryExpression_this_nodeclass
	= { BASECLASS
	    PrimaryExpression_this_eval, 0, 
	    PARSER_PRINT(PrimaryExpression_this_print)
	    PARSER_VISIT(0)
	    0 };


struct PrimaryExpression_ident_node {
	struct node node;
	struct SEE_string *string;
};

/* 11.1.2 */
static void
PrimaryExpression_ident_eval(na, context, res)
	struct node *na; /* (struct PrimaryExpression_ident_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct PrimaryExpression_ident_node *n = 
		CAST_NODE(na, PrimaryExpression_ident);
	SEE_scope_lookup(context->interpreter, context->scope, n->string, res);
}

#if WITH_PARSER_PRINT
static void
PrimaryExpression_ident_print(na, printer)
	struct node *na; /* (struct PrimaryExpression_ident_node) */
	struct printer *printer;
{
	struct PrimaryExpression_ident_node *n = 
		CAST_NODE(na, PrimaryExpression_ident);
	PRINT_STRING(n->string);
	PRINT_CHAR(' ');
}
#endif

static struct nodeclass PrimaryExpression_ident_nodeclass
	= { BASECLASS
	    PrimaryExpression_ident_eval, 0, 
	    PARSER_PRINT(PrimaryExpression_ident_print)
	    PARSER_VISIT(0)
	    0 };


static struct node *
PrimaryExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct PrimaryExpression_ident_node *i;

	switch (NEXT) {
	case tTHIS:
		n = NEW_NODE(struct node, &PrimaryExpression_this_nodeclass);
		SKIP;
		return n;
	case tIDENT:
		i = NEW_NODE(struct PrimaryExpression_ident_node,
			&PrimaryExpression_ident_nodeclass);
		i->string = NEXT_VALUE->u.string;
		SKIP;
		return (struct node *)i;
	case '[':
		return PARSE(ArrayLiteral);
	case '{':
		return PARSE(ObjectLiteral);
	case '(':
		SKIP;
		n = PARSE(Expression);
		EXPECT(')');
		return n;
	default:
		return PARSE(Literal);
	}
}

/*
 *	ArrayLiteral				-- 11.1.4
 *	:	'[' ']'
 *	|	'[' Elision ']'
 *	|	'[' ElementList ']'
 *	|	'[' ElementList ',' ']'
 *	|	'[' ElementList ',' Elision ']'
 *	;
 *
 *	ElementList
 *	:	Elision AssignmentExpression
 *	|	AssignmentExpression
 *	|	ElementList ',' Elision AssignmentExpression
 *	|	ElementList ',' AssignmentExpression
 *	;
 *
 *	Elision
 *	:	','
 *	|	Elision ','
 *	;
 *
 * NB: I ignore the above elision nonsense and just build a list of
 * (index,expr) nodes with an overall length. It is equivalent 
 * to that in the standard.
 */

struct ArrayLiteral_node {
	struct node node;
	int length;
	struct ArrayLiteral_element {
		int index;
		struct node *expr;
		struct ArrayLiteral_element *next;
	} *first;
};

/* 11.1.4 */
static void
ArrayLiteral_eval(na, context, res)
	struct node *na; /* (struct ArrayLiteral_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct ArrayLiteral_node *n = CAST_NODE(na, ArrayLiteral);
	struct ArrayLiteral_element *element;
	struct SEE_value expv, elv;
	struct SEE_string *ind;
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_traceback *tb;

	ind = SEE_string_new(interp, 16);

        tb = traceback_enter(interp, interp->Array, &n->node.location,
		SEE_CALLTYPE_CONSTRUCT);
	TRACE(na, context, SEE_TRACE_CALL);
	SEE_OBJECT_CONSTRUCT(interp, interp->Array, interp->Array, 
		0, NULL, res);
	TRACE(na, context, SEE_TRACE_RETURN);
        traceback_leave(interp, tb);

	for (element = n->first; element; element = element->next) {
		EVAL(element->expr, context, &expv);
		GetValue(context, &expv, &elv);
		ind->length = 0;
		SEE_string_append_int(ind, element->index);
		SEE_OBJECT_PUT(interp, res->u.object, 
		    SEE_intern(interp, ind), &elv, 0);
	}
	SEE_SET_NUMBER(&elv, n->length);
	SEE_OBJECT_PUT(interp, res->u.object, STR(length), &elv, 0);
}

#if WITH_PARSER_PRINT
static void
ArrayLiteral_print(na, printer)
	struct node *na; /* (struct ArrayLiteral_node) */
	struct printer *printer;
{
	struct ArrayLiteral_node *n = CAST_NODE(na, ArrayLiteral);
	struct ArrayLiteral_element *element;
	int pos;

	PRINT_CHAR('[');
	PRINT_CHAR(' ');
	for (pos = 0, element = n->first; element; element = element->next) {
		while (pos < element->index) {
			PRINT_CHAR(',');
			PRINT_CHAR(' ');
			pos++;
		}
		PRINT(element->expr);
	}
	while (pos < n->length) {
		PRINT_CHAR(',');
		PRINT_CHAR(' ');
		pos++;
	}
	PRINT_CHAR(']');
}
#endif

#if WITH_PARSER_VISIT
static void
ArrayLiteral_visit(na, v, va)
	struct node *na; /* (struct ArrayLiteral_node) */
	visitor_fn_t v;
	void *va;
{
	struct ArrayLiteral_node *n = CAST_NODE(na, ArrayLiteral);
	struct ArrayLiteral_element *element;

	for (element = n->first; element; element = element->next)
		VISIT(element->expr, v, va);
}
#endif

static struct nodeclass ArrayLiteral_nodeclass 
	= { BASECLASS
	    ArrayLiteral_eval, 0,
	    PARSER_PRINT(ArrayLiteral_print)
	    PARSER_VISIT(ArrayLiteral_visit)
	    0 };

static struct node *
ArrayLiteral_parse(parser)
	struct parser *parser;
{
	struct ArrayLiteral_node *n;
	struct ArrayLiteral_element **elp;
	int index;

	n = NEW_NODE(struct ArrayLiteral_node,
	    &ArrayLiteral_nodeclass);
	elp = &n->first;

	EXPECT('[');
	index = 0;
	while (NEXT != ']')
		if (NEXT == ',') {
			index++;
			SKIP;
		} else {
			*elp = SEE_NEW(parser->interpreter,
			    struct ArrayLiteral_element);
			(*elp)->index = index;
			(*elp)->expr = PARSE(AssignmentExpression);
			elp = &(*elp)->next;
			index++;
			if (NEXT != ']')
				EXPECTX(',', "',' or ']'");
		}
	n->length = index;
	*elp = NULL;
	EXPECT(']');
	return (struct node *)n;
}

/*
 *	ObjectLiteral				-- 11.1.5
 *	:	'{' '}'
 *	|	'{' PropertyNameAndValueList '}'
 *	;
 *
 *	PropertyNameAndValueList
 *	:	PropertyName ':' AssignmentExpression
 *	|	PropertyNameAndValueList ',' PropertyName ':' 
 *							AssignmentExpression
 *	;
 *
 *	PropertyName
 *	:	tIDENT
 *	|	StringLiteral
 *	|	NumericLiteral
 *	;
 */

struct ObjectLiteral_node {
	struct node node;
	struct ObjectLiteral_pair {
		struct node *value;
		struct ObjectLiteral_pair *next;
		struct SEE_string *name;
	} *first;
};

/* 11.1.5 */
static void
ObjectLiteral_eval(na, context, res)
	struct node *na; /* (struct ObjectLiteral_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct ObjectLiteral_node *n = CAST_NODE(na, ObjectLiteral);
	struct SEE_value valuev, v;
	struct SEE_object *o;
	struct ObjectLiteral_pair *pair;
	struct SEE_interpreter *interp = context->interpreter;

	o = SEE_Object_new(interp);
	for (pair = n->first; pair; pair = pair->next) {
		EVAL(pair->value, context, &valuev);
		GetValue(context, &valuev, &v);
		SEE_OBJECT_PUT(interp, o, pair->name, &v, 0);
	}
	SEE_SET_OBJECT(res, o);
}

#if WITH_PARSER_PRINT
static void
ObjectLiteral_print(na, printer)
	struct node *na; /* (struct ObjectLiteral_node) */
	struct printer *printer;
{
	struct ObjectLiteral_node *n = CAST_NODE(na, ObjectLiteral);
	struct ObjectLiteral_pair *pair;

	PRINT_CHAR('{');
	PRINT_CHAR(' ');
	for (pair = n->first; pair; pair = pair->next) {
		if (pair != n->first) {
			PRINT_CHAR(',');
			PRINT_CHAR(' ');
		}
		PRINT_STRING(pair->name);
		PRINT_CHAR(':');
		PRINT_CHAR(' ');
		PRINT(pair->value);
	}
	PRINT_CHAR('}');
}
#endif

#if WITH_PARSER_VISIT
static void
ObjectLiteral_visit(na, v, va)
	struct node *na; /* (struct ObjectLiteral_node) */
	visitor_fn_t v;
	void *va;
{
	struct ObjectLiteral_node *n = CAST_NODE(na, ObjectLiteral);
	struct ObjectLiteral_pair *pair;

	for (pair = n->first; pair; pair = pair->next)
		VISIT(pair->value, v, va);
}
#endif

static struct nodeclass ObjectLiteral_nodeclass = 
	{ BASECLASS
	  ObjectLiteral_eval, 0, 
	  PARSER_PRINT(ObjectLiteral_print)
	  PARSER_VISIT(ObjectLiteral_visit)
	  0 };

static struct node *
ObjectLiteral_parse(parser)
	struct parser *parser;
{
	struct ObjectLiteral_node *n;
	struct ObjectLiteral_pair **pairp;
	struct SEE_value sv;

	n = NEW_NODE(struct ObjectLiteral_node,
			&ObjectLiteral_nodeclass);
	pairp = &n->first;

	EXPECT('{');
	while (NEXT != '}') {
	    *pairp = SEE_NEW(parser->interpreter, struct ObjectLiteral_pair);
	    switch (NEXT) {
	    case tIDENT:
	    case tSTRING:
		(*pairp)->name = NEXT_VALUE->u.string;
		SKIP;
		break;
	    case tNUMBER:
		SEE_ToString(parser->interpreter, NEXT_VALUE, &sv);
		(*pairp)->name = sv.u.string;
		SKIP;
		break;
	    default:
		EXPECTED("string, identifier or number");
	    }
	    EXPECT(':');
	    (*pairp)->value = PARSE(AssignmentExpression);
	    if (NEXT != '}') {
		    /* XXX permits trailing comma e.g. {a:b,} */
		    EXPECTX(',', "',' or '}'"); 
	    }
	    pairp = &(*pairp)->next;
	}
	*pairp = NULL;
	EXPECT('}');
	return (struct node *)n;
}

/*
 *	-- 11.2
 *
 *	MemberExpression
 *	:	PrimaryExpression
 *	|	FunctionExpression				-- 11.2.5
 *	|	MemberExpression '[' Expression ']'		-- 11.2.1
 *	|	MemberExpression '.' tIDENT			-- 11.2.1
 *	|	tNEW MemberExpression Arguments			-- 11.2.2
 *	;
 *
 *	NewExpression
 *	:	MemberExpression
 *	|	tNEW NewExpression				-- 11.2.2
 *	;
 *
 *	CallExpression
 *	:	MemberExpression Arguments			-- 11.2.3
 *	|	CallExpression Arguments			-- 11.2.3
 *	|	CallExpression '[' Expression ']'		-- 11.2.1
 *	|	CallExpression '.' tIDENT			-- 11.2.1
 *	;
 *
 *	Arguments
 *	:	'(' ')'						-- 11.2.4
 *	|	'(' ArgumentList ')'				-- 11.2.4
 *	;
 *
 *	ArgumentList
 *	:	AssignmentExpression				-- 11.2.4
 *	|	ArgumentList ',' AssignmentExpression		-- 11.2.4
 *	;
 *
 *	LeftHandSideExpression
 *	:	NewExpression
 *	|	CallExpression
 *	;
 *
 * NOTE:  The standard grammar is complicated in order to resolve an 
 *        ambiguity in parsing 'new expr ( args )' as either
 *	  '(new  expr)(args)' or as 'new (expr(args))'. In fact, 'new'
 *	  is acting as both a unary and a binary operator. Yucky.
 *
 *	  Since recursive descent is single-token lookahead, we
 *	  can rewrite the above as the following equivalent grammar:
 *
 *	MemberExpression
 *	:	PrimaryExpression
 *	|	FunctionExpression		    -- lookahead == tFUNCTION
 *	|	MemberExpression '[' Expression ']'
 *	|	MemberExpression '.' tIDENT
 *	|	tNEW MemberExpression Arguments	    -- lookahead == tNEW
 *	|	tNEW MemberExpression 	            -- lookahead == tNEW
 *
 *	LeftHandSideExpression
 *	:	PrimaryExpression
 *	|	FunctionExpression		    -- lookahead == tFUNCTION
 *	|	LeftHandSideExpression '[' Expression ']'
 *	|	LeftHandSideExpression '.' tIDENT
 *	|	LeftHandSideExpression Arguments
 *	|	MemberExpression		    -- lookahead == tNEW
 *
 */

struct Arguments_node {				/* declare for early use */
	struct node node;
	int	argc;
	struct Arguments_arg {
		struct node *expr;
		struct Arguments_arg *next;
	} *first;
};

/* 11.2.4 */
static void
Arguments_eval(na, context, res)
	struct node *na; /* (struct Arguments_node) */
	struct SEE_context *context;
	struct SEE_value *res;		/* Assumed pointer to array */
{
	struct Arguments_node *n = CAST_NODE(na, Arguments);
	struct Arguments_arg *arg;
	struct SEE_value v;

	for (arg = n->first; arg; arg = arg->next) {
		EVAL(arg->expr, context, &v);
		GetValue(context, &v, res);
		res++;
	}
}

#if WITH_PARSER_VISIT
static void
Arguments_visit(na, v, va)
	struct node *na; /* (struct Arguments_node) */
	visitor_fn_t v;
	void *va;
{
	struct Arguments_node *n = CAST_NODE(na, Arguments);
	struct Arguments_arg *arg;

	for (arg = n->first; arg; arg = arg->next)
		VISIT(arg->expr, v, va);
}
#endif

static int
Arguments_isconst(na, interp)
	struct node *na; /* (struct Arguments_node) */
	struct SEE_interpreter *interp;
{
	struct Arguments_node *n = CAST_NODE(na, Arguments);
	struct Arguments_arg *arg;

	for (arg = n->first; arg; arg = arg->next)
		if (!ISCONST(arg->expr, interp))
			return 0;
	return 1;
}

#if WITH_PARSER_PRINT
static void
Arguments_print(na, printer)
	struct node *na; /* (struct Arguments_node) */
	struct printer *printer;
{
	struct Arguments_node *n = CAST_NODE(na, Arguments);
	struct Arguments_arg *arg;

	PRINT_CHAR('(');
	for (arg = n->first; arg; arg = arg->next) {
		if (arg != n->first) {
			PRINT_CHAR(',');
			PRINT_CHAR(' ');
		}
		PRINTP(arg->expr);
	}
	PRINT_CHAR(')');
}
#endif

static struct nodeclass Arguments_nodeclass 
	= { BASECLASS
	    Arguments_eval, 0, 
	    PARSER_PRINT(Arguments_print)
	    PARSER_VISIT(Arguments_visit)
	    Arguments_isconst };

static struct Arguments_node *
Arguments_parse(parser)
	struct parser *parser;
{
	struct Arguments_node *n;
	struct Arguments_arg **argp;

	n = NEW_NODE(struct Arguments_node,
			&Arguments_nodeclass);
	argp = &n->first;
	n->argc = 0;

	EXPECT('(');
	while (NEXT != ')') {
		n->argc++;
		*argp = SEE_NEW(parser->interpreter, struct Arguments_arg);
		(*argp)->expr = PARSE(AssignmentExpression);
		argp = &(*argp)->next;
		if (NEXT != ')')
			EXPECTX(',', "',' or ')'");
	}
	*argp = NULL;
	EXPECT(')');
	return n;
}


struct MemberExpression_new_node {
	struct node node;
	struct node *mexp;
	struct Arguments_node *args;
};

/* 11.2.2 */
static void
MemberExpression_new_eval(na, context, res)
	struct node *na; /* (struct MemberExpression_new_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct MemberExpression_new_node *n = 
		CAST_NODE(na, MemberExpression_new);
	struct SEE_value r1, r2, *args, **argv;
	struct SEE_interpreter *interp = context->interpreter;
	int argc, i;
	struct SEE_traceback *tb;

	EVAL(n->mexp, context, &r1);
	GetValue(context, &r1, &r2);
	if (n->args) {
		argc = n->args->argc;
		args = SEE_ALLOCA(interp, struct SEE_value, argc);
		argv = SEE_ALLOCA(interp, struct SEE_value *, argc);
		Arguments_eval((struct node *)n->args, context, args);
		for (i = 0; i < argc; i++)
			argv[i] = &args[i];
	} else {
		argc = 0;
		argv = NULL;
	}
	if (SEE_VALUE_GET_TYPE(&r2) != SEE_OBJECT)
		SEE_error_throw_string(interp, interp->TypeError,
			STR(new_not_an_object));
	if (!SEE_OBJECT_HAS_CONSTRUCT(r2.u.object))
		SEE_error_throw_string(interp, interp->TypeError,
			STR(not_a_constructor));
        tb = traceback_enter(interp, r2.u.object, &n->node.location,
		SEE_CALLTYPE_CONSTRUCT);
	TRACE(na, context, SEE_TRACE_CALL);
	SEE_OBJECT_CONSTRUCT(interp, r2.u.object, r2.u.object, 
		argc, argv, res);
	TRACE(na, context, SEE_TRACE_RETURN);
	traceback_leave(interp, tb);
}

#if WITH_PARSER_PRINT
static void
MemberExpression_new_print(na, printer)
	struct node *na; /* (struct MemberExpression_new_node) */
	struct printer *printer;
{
	struct MemberExpression_new_node *n = 
		CAST_NODE(na, MemberExpression_new);
	PRINT_STRING(STR(new));
	PRINT_CHAR(' ');
	PRINTP(n->mexp);
	if (n->args)
		PRINT((struct node *)n->args);
}
#endif

#if WITH_PARSER_VISIT
static void
MemberExpression_new_visit(na, v, va)
	struct node *na; /* (struct MemberExpression_new_node) */
	visitor_fn_t v;
	void *va;
{
	struct MemberExpression_new_node *n = 
		CAST_NODE(na, MemberExpression_new);
	VISIT(n->mexp, v, va);
	if (n->args)
		VISIT((struct node *)n->args, v, va);
}
#endif

static struct nodeclass MemberExpression_new_nodeclass = 
	{ BASECLASS
	  MemberExpression_new_eval, 0, 
	  PARSER_PRINT(MemberExpression_new_print)
	  PARSER_VISIT(MemberExpression_new_visit)
	  0 };


struct MemberExpression_dot_node {
	struct node node;
	struct node *mexp;
	struct SEE_string *name;
};

/* 11.2.1 */
static void
MemberExpression_dot_eval(na, context, res)
	struct node *na; /* (struct MemberExpression_dot_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct MemberExpression_dot_node *n = 
		CAST_NODE(na, MemberExpression_dot);
	struct SEE_value r1, r2, r5;
	struct SEE_interpreter *interp = context->interpreter;

	EVAL(n->mexp, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToObject(interp, &r2, &r5);
	_SEE_SET_REFERENCE(res, r5.u.object, n->name);
}

#if WITH_PARSER_PRINT
static void
MemberExpression_dot_print(na, printer)
	struct node *na; /* (struct MemberExpression_dot_node) */
	struct printer *printer;
{
	struct MemberExpression_dot_node *n = 
		CAST_NODE(na, MemberExpression_dot);
	PRINTP(n->mexp);
	PRINT_CHAR('.');
	PRINT_STRING(n->name);
	PRINT_CHAR(' ');
}
#endif

#if WITH_PARSER_VISIT
static void
MemberExpression_dot_visit(na, v, va)
	struct node *na; /* (struct MemberExpression_dot_node) */
	visitor_fn_t v;
	void *va;
{
	struct MemberExpression_dot_node *n = 
		CAST_NODE(na, MemberExpression_dot);
	VISIT(n->mexp, v, va);
}
#endif

static struct nodeclass MemberExpression_dot_nodeclass
	= { BASECLASS
	    MemberExpression_dot_eval, 0, 
	    PARSER_PRINT(MemberExpression_dot_print)
	    PARSER_VISIT(MemberExpression_dot_visit)
	    0 };


struct MemberExpression_bracket_node {
	struct node node;
	struct node *mexp, *name;
};

/* 11.2.1 */
static void
MemberExpression_bracket_eval(na, context, res)
	struct node *na; /* (struct MemberExpression_bracket_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct MemberExpression_bracket_node *n = 
		CAST_NODE(na, MemberExpression_bracket);
	struct SEE_value r1, r2, r3, r4, r5, r6;
	struct SEE_interpreter *interp = context->interpreter;

	EVAL(n->mexp, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->name, context, &r3);
	GetValue(context, &r3, &r4);
	SEE_ToObject(interp, &r2, &r5);
	SEE_ToString(interp, &r4, &r6);
	_SEE_SET_REFERENCE(res, r5.u.object, r6.u.string);
}

#if WITH_PARSER_PRINT
static void
MemberExpression_bracket_print(na, printer)
	struct node *na; /* (struct MemberExpression_bracket_node) */
	struct printer *printer;
{
	struct MemberExpression_bracket_node *n = 
		CAST_NODE(na, MemberExpression_bracket);
	PRINTP(n->mexp);
	PRINT_CHAR('[');
	PRINT(n->name);
	PRINT_CHAR(']');
}
#endif

#if WITH_PARSER_VISIT
static void
MemberExpression_bracket_visit(na, v, va)
	struct node *na; /* (struct MemberExpression_bracket_node) */
	visitor_fn_t v;
	void *va;
{
	struct MemberExpression_bracket_node *n = 
		CAST_NODE(na, MemberExpression_bracket);
	VISIT(n->mexp, v, va);
	VISIT(n->name, v, va);
}
#endif

static struct nodeclass MemberExpression_bracket_nodeclass
	= { BASECLASS
	    MemberExpression_bracket_eval, 0, 
	    PARSER_PRINT(MemberExpression_bracket_print)
	    PARSER_VISIT(MemberExpression_bracket_visit) 
	    0 };


static struct node *
MemberExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct MemberExpression_new_node *m;
	struct MemberExpression_dot_node *dn;
	struct MemberExpression_bracket_node *bn;

	switch (NEXT) {
        case tFUNCTION:
	    n = PARSE(FunctionExpression);
	    break;
        case tNEW:
	    m = NEW_NODE(struct MemberExpression_new_node,
	    	&MemberExpression_new_nodeclass);
	    SKIP;
	    m->mexp = PARSE(MemberExpression);
	    if (NEXT == '(')
		m->args = PARSE(Arguments);
	    else
		m->args = NULL;
	    n = (struct node *)m;
	    break;
	default:
	    n = PARSE(PrimaryExpression);
	}

	for (;;)
	    switch (NEXT) {
	    case '.':
		dn = NEW_NODE(struct MemberExpression_dot_node,
			&MemberExpression_dot_nodeclass);
		SKIP;
		if (NEXT == tIDENT) {
		    dn->mexp = n;
		    dn->name = NEXT_VALUE->u.string;
		    n = (struct node *)dn;
		}
	        EXPECT(tIDENT);
		break;
	    case '[':
		bn = NEW_NODE(struct MemberExpression_bracket_node,
			&MemberExpression_bracket_nodeclass);
		SKIP;
		bn->mexp = n;
		bn->name = PARSE(Expression);
		n = (struct node *)bn;
		EXPECT(']');
		break;
	    default:
		return n;
	    }
}


struct CallExpression_node {
	struct node node;
	struct node *exp;
	struct Arguments_node *args;
};

/* 11.2.3 */
static void
CallExpression_eval(na, context, res)
	struct node *na; /* (struct CallExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct CallExpression_node *n = CAST_NODE(na, CallExpression);
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_value r1, r3, *args, **argv;
	struct SEE_object *r6, *r7;
	struct SEE_traceback *tb;
	int argc, i;

	EVAL(n->exp, context, &r1);
	argc = n->args->argc;
	if (argc) {
		args = SEE_ALLOCA(interp, struct SEE_value, argc);
		argv = SEE_ALLOCA(interp, struct SEE_value *, argc);
		Arguments_eval((struct node *)n->args, context, args);
		for (i = 0; i < argc; i++)
			argv[i] = &args[i];
	} else 
		argv = NULL;
	GetValue(context, &r1, &r3);
	if (SEE_VALUE_GET_TYPE(&r3) == SEE_UNDEFINED)	/* nonstandard */
		SEE_error_throw_string(interp, interp->TypeError,
			STR(no_such_function));
	if (SEE_VALUE_GET_TYPE(&r3) != SEE_OBJECT)
		SEE_error_throw_string(interp, interp->TypeError,
			STR(not_a_function));
	if (!SEE_OBJECT_HAS_CALL(r3.u.object))
		SEE_error_throw_string(interp, interp->TypeError,
			STR(not_callable));
	if (SEE_VALUE_GET_TYPE(&r1) == SEE_REFERENCE)
		r6 = r1.u.reference.base;
	else
		r6 = NULL;
	if (r6 != NULL && IS_ACTIVATION_OBJECT(r6))
		r7 = NULL;
	else
		r7 = r6;
        tb = traceback_enter(interp, r3.u.object, &n->node.location,
		SEE_CALLTYPE_CALL);
	TRACE(na, context, SEE_TRACE_CALL);
	if (r3.u.object == interp->Global_eval) {
	    /* The special 'eval' function' */
	    eval(context, r7, argc, argv, res);
	} else {
#ifndef NDEBUG
	    SEE_SET_STRING(res, STR(internal_error));
#endif
	    SEE_OBJECT_CALL(interp, r3.u.object, r7, argc, argv, res);
	}
	TRACE(na, context, SEE_TRACE_RETURN);
        traceback_leave(interp, tb);
}

#if WITH_PARSER_PRINT
static void
CallExpression_print(na, printer)
	struct node *na; /* (struct CallExpression_node) */
	struct printer *printer;
{
	struct CallExpression_node *n = CAST_NODE(na, CallExpression);
	PRINTP(n->exp);
	PRINT((struct node *)n->args);
}
#endif

#if WITH_PARSER_VISIT
static void
CallExpression_visit(na, v, va)
	struct node *na; /* (struct CallExpression_node) */
	visitor_fn_t v;
	void *va;
{
	struct CallExpression_node *n = CAST_NODE(na, CallExpression);
	VISIT(n->exp, v, va);
	VISIT((struct node *)n->args, v, va);
}
#endif

static struct nodeclass CallExpression_nodeclass
	= { BASECLASS
	    CallExpression_eval, 0, 
	    PARSER_PRINT(CallExpression_print) 
	    PARSER_VISIT(CallExpression_visit)
	    0 };

static struct node *
LeftHandSideExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct CallExpression_node *cn;
	struct MemberExpression_dot_node *dn;
	struct MemberExpression_bracket_node *bn;

	switch (NEXT) {
        case tFUNCTION:
	    n = PARSE(FunctionExpression);	/* 11.2.5 */
	    break;
        case tNEW:
	    n = PARSE(MemberExpression);
	    break;
	default:
	    n = PARSE(PrimaryExpression);
	}

	for (;;)  {

#ifndef NDEBUG
	    if (SEE_parse_debug)
	        dprintf("LeftHandSideExpression: islhs = %d next is %s\n",
		    parser->is_lhs, SEE_tokenname(NEXT));
#endif

	    switch (NEXT) {
	    case '.':
	        dn = NEW_NODE(struct MemberExpression_dot_node,
		    &MemberExpression_dot_nodeclass);
		SKIP;
		if (NEXT == tIDENT) {
		    dn->mexp = n;
		    dn->name = NEXT_VALUE->u.string;
		    n = (struct node *)dn;
		}
	        EXPECT(tIDENT);
		break;
	    case '[':
		bn = NEW_NODE(struct MemberExpression_bracket_node,
			&MemberExpression_bracket_nodeclass);
		SKIP;
		bn->mexp = n;
		bn->name = PARSE(Expression);
		n = (struct node *)bn;
		EXPECT(']');
		break;
	    case '(':
		cn = NEW_NODE(struct CallExpression_node,
			&CallExpression_nodeclass);
		cn->exp = n;
		cn->args = PARSE(Arguments);
		n = (struct node *)cn;
		break;
	    default:
		/* Eventually we leave via this clause */
		parser->is_lhs = 1;
		return n;
	    }
	}
}

/*
 *	-- 11.3
 *
 *	PostfixExpression
 *	:	LeftHandSideExpression
 *	|	LeftHandSideExpression { NOLINETERM; } tPLUSPLUS    -- 11.3.1
 *	|	LeftHandSideExpression { NOLINETERM; } tMINUSMINUS  -- 11.3.2
 *	;
 */

struct Unary_node {
	struct node node;
	struct node *a;
};

#if WITH_PARSER_VISIT
static void
Unary_visit(na, v, va)
	struct node *na; /* (struct Unary_node) */
	visitor_fn_t v;
	void *va;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	VISIT(n->a, v, va);
}
#endif

#if WITH_PARSER_PRINT
static void
Unary_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT(n->a);
}
#endif

static int
Unary_isconst(na, interp)
	struct node *na; /* (struct Unary_node) */
	struct SEE_interpreter *interp;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	return ISCONST(n->a, interp);
}

static struct nodeclass Unary_nodeclass
	= { BASECLASS
	    0, 0, 
	    PARSER_PRINT(Unary_print)
	    PARSER_VISIT(Unary_visit)
	    Unary_isconst };

/* 11.3.1 */
static void
PostfixExpression_inc_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2, r3;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToNumber(context->interpreter, &r2, res);
	SEE_SET_NUMBER(&r3, res->u.number + 1);
	PutValue(context, &r1, &r3);
}

#if WITH_PARSER_PRINT
static void
PostfixExpression_inc_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINTP(n->a);
	PRINT_CHAR('+');
	PRINT_CHAR('+');
	PRINT_CHAR(' ');
}
#endif

static struct nodeclass PostfixExpression_inc_nodeclass
	= { SUPERCLASS(Unary)
	    PostfixExpression_inc_eval, 0, 
	    PARSER_PRINT(PostfixExpression_inc_print)
	    PARSER_VISIT(Unary_visit)
	    0 };


/* 11.3.2 */
static void
PostfixExpression_dec_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2, r3;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToNumber(context->interpreter, &r2, res);
	SEE_SET_NUMBER(&r3, res->u.number - 1);
	PutValue(context, &r1, &r3);
}

#if WITH_PARSER_PRINT
static void
PostfixExpression_dec_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINTP(n->a);
	PRINT_CHAR('-');
	PRINT_CHAR('-');
	PRINT_CHAR(' ');
}
#endif

static struct nodeclass PostfixExpression_dec_nodeclass
	= { SUPERCLASS(Unary)
	    PostfixExpression_dec_eval, 0, 
	    PARSER_PRINT(PostfixExpression_dec_print)
	    PARSER_VISIT(Unary_visit)
	    0 };

static struct node *
PostfixExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Unary_node *pen;

	n = PARSE(LeftHandSideExpression);
	if (!NEXT_FOLLOWS_NL && 
	    (NEXT == tPLUSPLUS || NEXT == tMINUSMINUS))
	{
		pen = NEW_NODE(struct Unary_node,
			NEXT == tPLUSPLUS
			    ? &PostfixExpression_inc_nodeclass
			    : &PostfixExpression_dec_nodeclass);
		pen->a = n;
		n = (struct node *)pen;
		SKIP;
		parser->is_lhs = 0;
	}
	return n;
}

/*
 *	-- 11.4
 *
 *	UnaryExpression
 *	:	PostfixExpression
 *	|	tDELETE UnaryExpression				-- 11.4.1
 *	|	tVOID UnaryExpression				-- 11.4.2
 *	|	tTYPEOF UnaryExpression				-- 11.4.3
 *	|	tPLUSPLUS UnaryExpression			-- 11.4.4
 *	|	tMINUSMINUS UnaryExpression			-- 11.4.5
 *	|	'+' UnaryExpression				-- 11.4.6
 *	|	'-' UnaryExpression				-- 11.4.7
 *	|	'~' UnaryExpression				-- 11.4.8
 *	|	'!' UnaryExpression				-- 11.4.9
 *	;
 */

/* 11.4.1 */
static void
UnaryExpression_delete_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1;
	struct SEE_interpreter *interp = context->interpreter;

	EVAL(n->a, context, &r1);
	if (SEE_VALUE_GET_TYPE(&r1) != SEE_REFERENCE) {
		SEE_SET_BOOLEAN(res, 0);
		return;
	}
	/*
	 * spec bug: if the base is null, it isn't clear what is meant 
	 * to happen. We return true as if the fictitous property 
	 * owner existed.
	 */
	if (!r1.u.reference.base || 
	    SEE_OBJECT_DELETE(interp, r1.u.reference.base, 
	    		      r1.u.reference.property))
		SEE_SET_BOOLEAN(res, 1);
	else
		SEE_SET_BOOLEAN(res, 0);
}

#if WITH_PARSER_PRINT
static void
UnaryExpression_delete_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_STRING(STR(delete));
	PRINT_CHAR(' ');
	PRINTP(n->a);
}
#endif

static struct nodeclass UnaryExpression_delete_nodeclass
	= { SUPERCLASS(Unary)
	    UnaryExpression_delete_eval, 0,
	    PARSER_PRINT(UnaryExpression_delete_print)
	    PARSER_VISIT(Unary_visit)
	    Unary_isconst };


/* 11.4.2 */
static void
UnaryExpression_void_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_SET_UNDEFINED(res);
}

#if WITH_PARSER_PRINT
static void
UnaryExpression_void_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_STRING(STR(void));
	PRINT_CHAR(' ');
	PRINTP(n->a);
}
#endif

static struct nodeclass UnaryExpression_void_nodeclass
	= { SUPERCLASS(Unary)
	    UnaryExpression_void_eval, 0, 
	    PARSER_PRINT(UnaryExpression_void_print)
	    PARSER_VISIT(Unary_visit)
	    Unary_isconst };


/* 11.4.3 */
static void
UnaryExpression_typeof_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r4;
	struct SEE_string *s;

	EVAL(n->a, context, &r1);
	if (SEE_VALUE_GET_TYPE(&r1) == SEE_REFERENCE && 
	    r1.u.reference.base == NULL) 
	{
		SEE_SET_STRING(res, STR(undefined));
		return;
	}
	GetValue(context, &r1, &r4);
	switch (SEE_VALUE_GET_TYPE(&r4)) {
	case SEE_UNDEFINED:	s = STR(undefined); break;
	case SEE_NULL:		s = STR(object); break;
	case SEE_BOOLEAN:	s = STR(boolean); break;
	case SEE_NUMBER:	s = STR(number); break;
	case SEE_STRING:	s = STR(string); break;
	case SEE_OBJECT:	s = SEE_OBJECT_HAS_CALL(r4.u.object)
				  ? STR(function)
				  : STR(object);
				break;
	default:		s = STR(unknown);
	}
	SEE_SET_STRING(res, s);
}

#if WITH_PARSER_PRINT
static void
UnaryExpression_typeof_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_STRING(STR(typeof));
	PRINT_CHAR(' ');
	PRINTP(n->a);
}
#endif

static struct nodeclass UnaryExpression_typeof_nodeclass
	= { SUPERCLASS(Unary)
	    UnaryExpression_typeof_eval, 0, 
	    PARSER_PRINT(UnaryExpression_typeof_print)
	    PARSER_VISIT(Unary_visit)
	    Unary_isconst };

/* 11.4.4 */
static void
UnaryExpression_preinc_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToNumber(context->interpreter, &r2, res);
	res->u.number++;
	PutValue(context, &r1, res);
}

#if WITH_PARSER_PRINT
static void
UnaryExpression_preinc_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_CHAR('+');
	PRINT_CHAR('+');
	PRINT_CHAR(' ');
	PRINTP(n->a);
}
#endif

static struct nodeclass UnaryExpression_preinc_nodeclass
	= { SUPERCLASS(Unary)
	    UnaryExpression_preinc_eval, 0, 
	    PARSER_PRINT(UnaryExpression_preinc_print)
	    PARSER_VISIT(Unary_visit)
	    0 };


/* 11.4.5 */
static void
UnaryExpression_predec_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToNumber(context->interpreter, &r2, res);
	res->u.number--;
	PutValue(context, &r1, res);
}

#if WITH_PARSER_PRINT
static void
UnaryExpression_predec_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_CHAR('-');
	PRINT_CHAR('-');
	PRINT_CHAR(' ');
	PRINTP(n->a);
}
#endif

static struct nodeclass UnaryExpression_predec_nodeclass
	= { SUPERCLASS(Unary)
	    UnaryExpression_predec_eval, 0, 
	    PARSER_PRINT(UnaryExpression_predec_print)
	    PARSER_VISIT(Unary_visit)
	    0 };


/* 11.4.6 */
static void
UnaryExpression_plus_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToNumber(context->interpreter, &r2, res);
}

#if WITH_PARSER_PRINT
static void
UnaryExpression_plus_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_CHAR('+');
	PRINT_CHAR(' ');
	PRINTP(n->a);
}
#endif

static struct nodeclass UnaryExpression_plus_nodeclass
	= { SUPERCLASS(Unary)
	    UnaryExpression_plus_eval, 0, 
	    PARSER_PRINT(UnaryExpression_plus_print)
	    PARSER_VISIT(Unary_visit)
	    Unary_isconst };


/* 11.4.7 */
static void
UnaryExpression_minus_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToNumber(context->interpreter, &r2, res);
	res->u.number = -(res->u.number);
}

#if WITH_PARSER_PRINT
static void
UnaryExpression_minus_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_CHAR('-');
	PRINT_CHAR(' ');
	PRINTP(n->a);
}
#endif

static struct nodeclass UnaryExpression_minus_nodeclass
	= { SUPERCLASS(Unary)
	    UnaryExpression_minus_eval, 0, 
	    PARSER_PRINT(UnaryExpression_minus_print)
	    PARSER_VISIT(Unary_visit)
	    Unary_isconst };

/* 11.4.8 */
static void
UnaryExpression_inv_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2;
	SEE_int32_t r3;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	r3 = SEE_ToInt32(context->interpreter, &r2);
	SEE_SET_NUMBER(res, ~r3);
}

#if WITH_PARSER_PRINT
static void
UnaryExpression_inv_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_CHAR('~');
	PRINT_CHAR(' ');
	PRINTP(n->a);
}
#endif

static struct nodeclass UnaryExpression_inv_nodeclass
	= { SUPERCLASS(Unary)
	    UnaryExpression_inv_eval, 0,
	    PARSER_PRINT(UnaryExpression_inv_print)
	    PARSER_VISIT(Unary_visit)
	    Unary_isconst };


/* 11.4.9 */
static void
UnaryExpression_not_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2, r3;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToBoolean(context->interpreter, &r2, &r3);
	SEE_SET_BOOLEAN(res, !r3.u.boolean);
}

#if WITH_PARSER_PRINT
static void
UnaryExpression_not_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_CHAR('!');
	PRINT_CHAR(' ');
	PRINTP(n->a);
}
#endif

static struct nodeclass UnaryExpression_not_nodeclass
	= { SUPERCLASS(Unary)
	    UnaryExpression_not_eval, 0, 
	    PARSER_PRINT(UnaryExpression_not_print)
	    PARSER_VISIT(Unary_visit)
	    Unary_isconst };

static struct node *
UnaryExpression_parse(parser)
	struct parser *parser;
{
	struct Unary_node *n;
	struct nodeclass *nc;

	switch (NEXT) {
	case tDELETE:
		nc = &UnaryExpression_delete_nodeclass;
		break;
	case tVOID:
		nc = &UnaryExpression_void_nodeclass;
		break;
	case tTYPEOF:
		nc = &UnaryExpression_typeof_nodeclass;
		break;
	case tPLUSPLUS:
		nc = &UnaryExpression_preinc_nodeclass;
		break;
	case tMINUSMINUS:
		nc = &UnaryExpression_predec_nodeclass;
		break;
	case '+':
		nc = &UnaryExpression_plus_nodeclass;
		break;
	case '-':
		nc = &UnaryExpression_minus_nodeclass;
		break;
	case '~':
		nc = &UnaryExpression_inv_nodeclass;
		break;
	case '!':
		nc = &UnaryExpression_not_nodeclass;
		break;
	default:
		return PARSE(PostfixExpression);
	}
	n = NEW_NODE(struct Unary_node, nc);
	SKIP;
	n->a = PARSE(UnaryExpression);
	parser->is_lhs = 0;
	return (struct node *)n;
}

/*
 *	-- 11.5
 *
 *	MultiplicativeExpression
 *	:	UnaryExpression
 *	|	MultiplicativeExpression '*' UnaryExpression	-- 11.5.1
 *	|	MultiplicativeExpression '/' UnaryExpression	-- 11.5.2
 *	|	MultiplicativeExpression '%' UnaryExpression	-- 11.5.3
 *	;
 */


struct Binary_node {
	struct node node;
	struct node *a, *b;
};

#if WITH_PARSER_VISIT
static void
Binary_visit(na, v, va)
	struct node *na; /* (struct Binary_node) */
	visitor_fn_t v;
	void *va;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	VISIT(n->a, v, va);
	VISIT(n->b, v, va);
}
#endif

#if WITH_PARSER_PRINT
static void
Binary_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINT(n->a);
	PRINT(n->b);
}
#endif

static int
Binary_isconst(na, interp)
	struct node *na; /* (struct Binary_node) */
	struct SEE_interpreter *interp;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	return ISCONST(n->a, interp) && ISCONST(n->b, interp);
}

static struct nodeclass Binary_nodeclass
	= { BASECLASS
	    0, 0, 
	    PARSER_PRINT(Binary_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };

/* 11.5.1 */
static void
MultiplicativeExpression_mul_common(r2, bn, context, res)
	struct SEE_value *r2, *res;
	struct node *bn;
	struct SEE_context *context;
{
	struct SEE_value r3, r4, r5, r6;

	EVAL(bn, context, &r3);
	GetValue(context, &r3, &r4);
	SEE_ToNumber(context->interpreter, r2, &r5);
	SEE_ToNumber(context->interpreter, &r4, &r6);
	SEE_SET_NUMBER(res, r5.u.number * r6.u.number);
}

static void
MultiplicativeExpression_mul_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	MultiplicativeExpression_mul_common(&r2, n->b, context, res);
}

#if WITH_PARSER_PRINT
static void
MultiplicativeExpression_mul_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('*');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass MultiplicativeExpression_mul_nodeclass
	= { SUPERCLASS(Binary)
	    MultiplicativeExpression_mul_eval, 0, 
	    PARSER_PRINT(MultiplicativeExpression_mul_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


/* 11.5.2 */
static void
MultiplicativeExpression_div_common(r2, bn, context, res)
	struct SEE_value *r2, *res;
	struct node *bn;
	struct SEE_context *context;
{
	struct SEE_value r3, r4, r5, r6;

	EVAL(bn, context, &r3);
	GetValue(context, &r3, &r4);
	SEE_ToNumber(context->interpreter, r2, &r5);
	SEE_ToNumber(context->interpreter, &r4, &r6);
	SEE_SET_NUMBER(res, r5.u.number / r6.u.number);
}

static void
MultiplicativeExpression_div_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
        MultiplicativeExpression_div_common(&r2, n->b, context, res);
}

#if WITH_PARSER_PRINT
static void
MultiplicativeExpression_div_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('/');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass MultiplicativeExpression_div_nodeclass
	= { SUPERCLASS(Binary)
	    MultiplicativeExpression_div_eval, 0, 
	    PARSER_PRINT(MultiplicativeExpression_div_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


/* 11.5.3 */
static void
MultiplicativeExpression_mod_common(r2, bn, context, res)
	struct SEE_value *r2, *res;
	struct node *bn;
	struct SEE_context *context;
{
	struct SEE_value r3, r4, r5, r6;

	EVAL(bn, context, &r3);
	GetValue(context, &r3, &r4);
	SEE_ToNumber(context->interpreter, r2, &r5);
	SEE_ToNumber(context->interpreter, &r4, &r6);
	SEE_SET_NUMBER(res, fmod(r5.u.number, r6.u.number));
}

static void
MultiplicativeExpression_mod_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	MultiplicativeExpression_mod_common(&r2, n->b, context, res);
}

#if WITH_PARSER_PRINT
static void
MultiplicativeExpression_mod_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('%');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass MultiplicativeExpression_mod_nodeclass
	= { SUPERCLASS(Binary)
	    MultiplicativeExpression_mod_eval, 0, 
	    PARSER_PRINT(MultiplicativeExpression_mod_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };

static struct node *
MultiplicativeExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct nodeclass *nc;
	struct Binary_node *m;

	n = PARSE(UnaryExpression);
	for (;;) {
	    /* Left-to-right associative */
	    switch (NEXT) {
	    case '*':
		nc = &MultiplicativeExpression_mul_nodeclass;
		break;
	    case '/':
		nc = &MultiplicativeExpression_div_nodeclass;
		break;
	    case '%':
		nc = &MultiplicativeExpression_mod_nodeclass;
		break;
	    default:
		return n;
	    }
	    SKIP;
	    m = NEW_NODE(struct Binary_node, nc);
	    m->a = n;
	    m->b = PARSE(UnaryExpression);
	    parser->is_lhs = 0;
	    n = (struct node *)m;
	}
}

/*
 *	-- 11.6
 *
 *	AdditiveExpression
 *	:	MultiplicativeExpression
 *	|	AdditiveExpression '+' MultiplicativeExpression	-- 11.6.1
 *	|	AdditiveExpression '-' MultiplicativeExpression	-- 11.6.2
 *	;
 */

/* 11.6.1 */
static void
AdditiveExpression_add_common(r2, bn, context, res)
	struct SEE_value *r2, *res;
	struct node *bn;
	struct SEE_context *context;
{
	struct SEE_value r3, r4, r5, r6,
			 r8, r9, r12, r13;
	struct SEE_string *s;

	EVAL(bn, context, &r3);
	GetValue(context, &r3, &r4);
	SEE_ToPrimitive(context->interpreter, r2, NULL, &r5);
	SEE_ToPrimitive(context->interpreter, &r4, NULL, &r6);
	if (!(SEE_VALUE_GET_TYPE(&r5) == SEE_STRING || 
	      SEE_VALUE_GET_TYPE(&r6) == SEE_STRING)) 
	{
		SEE_ToNumber(context->interpreter, &r5, &r8);
		SEE_ToNumber(context->interpreter, &r6, &r9);
		SEE_SET_NUMBER(res, r8.u.number + r9.u.number);
	} else {
		SEE_ToString(context->interpreter, &r5, &r12);
		SEE_ToString(context->interpreter, &r6, &r13);
		s = SEE_string_concat(context->interpreter, 
			r12.u.string, r13.u.string);
		SEE_SET_STRING(res, s);
	}
}

static void
AdditiveExpression_add_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	AdditiveExpression_add_common(&r2, n->b, context, res);
}

#if WITH_PARSER_PRINT
static void
AdditiveExpression_add_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('+');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass AdditiveExpression_add_nodeclass
	= { SUPERCLASS(Binary)
	    AdditiveExpression_add_eval, 0, 
	    PARSER_PRINT(AdditiveExpression_add_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


/* 11.6.2 */
static void
AdditiveExpression_sub_common(r2, bn, context, res)
	struct SEE_value *r2, *res;
	struct node *bn;
	struct SEE_context *context;
{
	struct SEE_value r3, r4, r5, r6;

	EVAL(bn, context, &r3);
	GetValue(context, &r3, &r4);
	SEE_ToNumber(context->interpreter, r2, &r5);
	SEE_ToNumber(context->interpreter, &r4, &r6);
	SEE_SET_NUMBER(res, r5.u.number - r6.u.number);
}

static void
AdditiveExpression_sub_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	AdditiveExpression_sub_common(&r2, n->b, context, res);
}

#if WITH_PARSER_PRINT
static void
AdditiveExpression_sub_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('-');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass AdditiveExpression_sub_nodeclass
	= { SUPERCLASS(Binary)
	    AdditiveExpression_sub_eval, 0, 
	    PARSER_PRINT(AdditiveExpression_sub_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };

static struct node *
AdditiveExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct nodeclass *nc;
	struct Binary_node *m;

	n = PARSE(MultiplicativeExpression);
	for (;;) {
	    switch (NEXT) {
	    case '+':
		nc = &AdditiveExpression_add_nodeclass;
		break;
	    case '-':
		nc = &AdditiveExpression_sub_nodeclass;
		break;
	    default:
		return n;
	    }
	    parser->is_lhs = 0;
	    SKIP;
	    m = NEW_NODE(struct Binary_node, nc);
	    m->a = n;
	    m->b = PARSE(MultiplicativeExpression);
	    n = (struct node *)m;
	}
	return n;
}

/*
 *	-- 11.7
 *
 *	ShiftExpression
 *	:	AdditiveExpression
 *	|	ShiftExpression tLSHIFT AdditiveExpression	-- 11.7.1
 *	|	ShiftExpression tRSHIFT AdditiveExpression	-- 11.7.2
 *	|	ShiftExpression tURSHIFT AdditiveExpression	-- 11.7.3
 *	;
 */

/* 11.7.1 */
static void
ShiftExpression_lshift_common(r2, bn, context, res)
	struct SEE_value *r2, *res;
	struct node *bn;
	struct SEE_context *context;
{
	struct SEE_value r3, r4;
	SEE_int32_t r5;
	SEE_uint32_t r6;

	EVAL(bn, context, &r3);
	GetValue(context, &r3, &r4);
	r5 = SEE_ToInt32(context->interpreter, r2);
	r6 = SEE_ToUint32(context->interpreter, &r4);
	SEE_SET_NUMBER(res, r5 << (r6 & 0x1f));
}

static void
ShiftExpression_lshift_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	ShiftExpression_lshift_common(&r2, n->b, context, res);
}

#if WITH_PARSER_PRINT
static void
ShiftExpression_lshift_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('<');
	PRINT_CHAR('<');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass ShiftExpression_lshift_nodeclass
	= { SUPERCLASS(Binary)
	    ShiftExpression_lshift_eval, 0, 
	    PARSER_PRINT(ShiftExpression_lshift_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


/* 11.7.2 */
static void
ShiftExpression_rshift_common(r2, bn, context, res)
	struct SEE_value *r2, *res;
	struct node *bn;
	struct SEE_context *context;
{
	struct SEE_value r3, r4;
	SEE_int32_t r5;
	SEE_uint32_t r6;

	EVAL(bn, context, &r3);
	GetValue(context, &r3, &r4);
	r5 = SEE_ToInt32(context->interpreter, r2);
	r6 = SEE_ToUint32(context->interpreter, &r4);
	SEE_SET_NUMBER(res, r5 >> (r6 & 0x1f));
}

static void
ShiftExpression_rshift_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	ShiftExpression_rshift_common(&r2, n->b, context, res);
}

#if WITH_PARSER_PRINT
static void
ShiftExpression_rshift_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('>');
	PRINT_CHAR('>');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass ShiftExpression_rshift_nodeclass
	= { SUPERCLASS(Binary)
	    ShiftExpression_rshift_eval, 0, 
	    PARSER_PRINT(ShiftExpression_rshift_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


/* 11.7.3 */
static void
ShiftExpression_urshift_common(r2, bn, context, res)
	struct SEE_value *r2, *res;
	struct node *bn;
	struct SEE_context *context;
{
	struct SEE_value r3, r4;
	SEE_uint32_t r5, r6;

	EVAL(bn, context, &r3);
	GetValue(context, &r3, &r4);
	r5 = SEE_ToUint32(context->interpreter, r2);
	r6 = SEE_ToUint32(context->interpreter, &r4);
	SEE_SET_NUMBER(res, r5 >> (r6 & 0x1f));
}

static void
ShiftExpression_urshift_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	ShiftExpression_urshift_common(&r2, n->b, context, res);
}

#if WITH_PARSER_PRINT
static void
ShiftExpression_urshift_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('>');
	PRINT_CHAR('>');
	PRINT_CHAR('>');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass ShiftExpression_urshift_nodeclass
	= { SUPERCLASS(Binary)
	    ShiftExpression_urshift_eval, 0, 
	    PARSER_PRINT(ShiftExpression_urshift_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


static struct node *
ShiftExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct nodeclass *nc;
	struct Binary_node *sn;

	n = PARSE(AdditiveExpression);
	for (;;) {
	    /* Left associative */
	    switch (NEXT) {
	    case tLSHIFT:
		nc = &ShiftExpression_lshift_nodeclass;
		break;
	    case tRSHIFT:
		nc = &ShiftExpression_rshift_nodeclass;
		break;
	    case tURSHIFT:
		nc = &ShiftExpression_urshift_nodeclass;
		break;
	    default:
		return n;
	    }
	    sn = NEW_NODE(struct Binary_node, nc);
	    SKIP;
	    sn->a = n;
	    sn->b = PARSE(AdditiveExpression);
	    parser->is_lhs = 0;
	    n = (struct node *)sn;
	}
}

/*
 *	-- 11.8
 *
 *	RelationalExpression
 *	:	ShiftExpression
 *	|	RelationalExpression '<' ShiftExpression	 -- 11.8.1
 *	|	RelationalExpression '>' ShiftExpression	 -- 11.8.2
 *	|	RelationalExpression tLE ShiftExpression	 -- 11.8.3
 *	|	RelationalExpression tGT ShiftExpression	 -- 11.8.4
 *	|	RelationalExpression tINSTANCEOF ShiftExpression -- 11.8.6
 *	|	RelationalExpression tIN ShiftExpression	 -- 11.8.7
 *	;
 *
 *	RelationalExpressionNoIn
 *	:	ShiftExpression
 *	|	RelationalExpressionNoIn '<' ShiftExpression	 -- 11.8.1
 *	|	RelationalExpressionNoIn '>' ShiftExpression	 -- 11.8.2
 *	|	RelationalExpressionNoIn tLE ShiftExpression	 -- 11.8.3
 *	|	RelationalExpressionNoIn tGT ShiftExpression	 -- 11.8.4
 *	|	RelationalExpressionNoIn tINSTANCEOF ShiftExpression -- 11.8.6
 *	;
 *
 * The *NoIn productions are implemented by the 'noin' boolean field
 * in the parser state.
 */

/* 
 * 11.8.5 Abstract relational comparison function.
 */
static void
RelationalExpression_sub(interp, x, y, res)
	struct SEE_interpreter *interp;
	struct SEE_value *x, *y, *res;
{
	struct SEE_value r1, r2, r4, r5;
	struct SEE_value hint;
	int k;

	SEE_SET_OBJECT(&hint, interp->Number);

	SEE_ToPrimitive(interp, x, &hint, &r1);
	SEE_ToPrimitive(interp, y, &hint, &r2);
	if (!(SEE_VALUE_GET_TYPE(&r1) == SEE_STRING && 
	      SEE_VALUE_GET_TYPE(&r2) == SEE_STRING)) 
	{
	    SEE_ToNumber(interp, &r1, &r4);
	    SEE_ToNumber(interp, &r2, &r5);
	    if (SEE_NUMBER_ISNAN(&r4) || SEE_NUMBER_ISNAN(&r5))
		SEE_SET_UNDEFINED(res);
	    else if (r4.u.number == r5.u.number)
		SEE_SET_BOOLEAN(res, 0);
	    else if (SEE_NUMBER_ISPINF(&r4))
		SEE_SET_BOOLEAN(res, 0);
	    else if (SEE_NUMBER_ISPINF(&r5))
		SEE_SET_BOOLEAN(res, 1);
	    else if (SEE_NUMBER_ISNINF(&r5))
		SEE_SET_BOOLEAN(res, 0);
	    else if (SEE_NUMBER_ISNINF(&r4))
		SEE_SET_BOOLEAN(res, 1);
	    else 
	        SEE_SET_BOOLEAN(res, r4.u.number < r5.u.number);
	} else {
	    for (k = 0; 
		 k < r1.u.string->length && k < r2.u.string->length;
		 k++)
		if (r1.u.string->data[k] != r2.u.string->data[k])
			break;
	    if (k == r2.u.string->length)
		SEE_SET_BOOLEAN(res, 0);
	    else if (k == r1.u.string->length)
		SEE_SET_BOOLEAN(res, 1);
	    else
		SEE_SET_BOOLEAN(res, r1.u.string->data[k] < 
				 r2.u.string->data[k]);
	}
}

/* 11.8.1 < */
static void
RelationalExpression_lt_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	RelationalExpression_sub(context->interpreter, &r2, &r4, res);
	if (SEE_VALUE_GET_TYPE(res) == SEE_UNDEFINED)
		SEE_SET_BOOLEAN(res, 0);
}

#if WITH_PARSER_PRINT
static void
RelationalExpression_lt_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('<');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass RelationalExpression_lt_nodeclass
	= { SUPERCLASS(Binary)
	    RelationalExpression_lt_eval, 0, 
	    PARSER_PRINT(RelationalExpression_lt_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


/* 11.8.2 > */
static void
RelationalExpression_gt_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	RelationalExpression_sub(context->interpreter, &r4, &r2, res);
	if (SEE_VALUE_GET_TYPE(res) == SEE_UNDEFINED)
		SEE_SET_BOOLEAN(res, 0);
}

#if WITH_PARSER_PRINT
static void
RelationalExpression_gt_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('>');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass RelationalExpression_gt_nodeclass
	= { SUPERCLASS(Binary)
	    RelationalExpression_gt_eval, 0, 
	    PARSER_PRINT(RelationalExpression_gt_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


/* 11.8.3 <= */
static void
RelationalExpression_le_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4, r5;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	RelationalExpression_sub(context->interpreter, &r4, &r2, &r5);
	if (SEE_VALUE_GET_TYPE(&r5) == SEE_UNDEFINED)
		SEE_SET_BOOLEAN(res, 0);
	else
		SEE_SET_BOOLEAN(res, !r5.u.boolean);
}

#if WITH_PARSER_PRINT
static void
RelationalExpression_le_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('<');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass RelationalExpression_le_nodeclass
	= { SUPERCLASS(Binary)
	    RelationalExpression_le_eval, 0, 
	    PARSER_PRINT(RelationalExpression_le_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


/* 11.8.4 >= */
static void
RelationalExpression_ge_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4, r5;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	RelationalExpression_sub(context->interpreter, &r2, &r4, &r5);
	if (SEE_VALUE_GET_TYPE(&r5) == SEE_UNDEFINED)
		SEE_SET_BOOLEAN(res, 0);
	else
		SEE_SET_BOOLEAN(res, !r5.u.boolean);
}

#if WITH_PARSER_PRINT
static void
RelationalExpression_ge_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('>');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass RelationalExpression_ge_nodeclass
	= { SUPERCLASS(Binary)
	    RelationalExpression_ge_eval, 0, 
	    PARSER_PRINT(RelationalExpression_ge_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


/* 11.8.6 */
static void
RelationalExpression_instanceof_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_value r1, r2, r3, r4;
	int r7;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	if (SEE_VALUE_GET_TYPE(&r4) != SEE_OBJECT)
		SEE_error_throw_string(interp, interp->TypeError,
		    STR(instanceof_not_object));
	if (!SEE_OBJECT_HAS_HASINSTANCE(r4.u.object))
		SEE_error_throw_string(interp, interp->TypeError,
		    STR(no_hasinstance));
	r7 = SEE_OBJECT_HASINSTANCE(interp, r4.u.object, &r2);
	SEE_SET_BOOLEAN(res, r7);
}

#if WITH_PARSER_PRINT
static void
RelationalExpression_instanceof_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_STRING(STR(instanceof));
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass RelationalExpression_instanceof_nodeclass
	= { SUPERCLASS(Binary)
	    RelationalExpression_instanceof_eval, 0, 
	    PARSER_PRINT(RelationalExpression_instanceof_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


/* 11.8.7 */
static void
RelationalExpression_in_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_value r1, r2, r3, r4, r6;
	int r7;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	if (SEE_VALUE_GET_TYPE(&r4) != SEE_OBJECT)
		SEE_error_throw_string(interp, interp->TypeError,
		    STR(in_not_object));
	SEE_ToString(interp, &r2, &r6);
	r7 = SEE_OBJECT_HASPROPERTY(interp, r4.u.object, 
		SEE_intern(interp, r6.u.string));
	SEE_SET_BOOLEAN(res, r7);
}

#if WITH_PARSER_PRINT
static void
RelationalExpression_in_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_STRING(STR(in));
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass RelationalExpression_in_nodeclass
	= { SUPERCLASS(Binary)
	    RelationalExpression_in_eval, 0, 
	    PARSER_PRINT(RelationalExpression_in_print)
	    PARSER_VISIT(Binary_visit) 
	    Binary_isconst };


static struct node *
RelationalExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct nodeclass *nc;
	struct Binary_node *rn;

	n = PARSE(ShiftExpression);
	for (;;) {
	    /* Left associative */
	    switch (NEXT) {
	    case '<':
		nc = &RelationalExpression_lt_nodeclass;
		break;
	    case '>':
		nc = &RelationalExpression_gt_nodeclass;
		break;
	    case tLE:
		nc = &RelationalExpression_le_nodeclass;
		break;
	    case tGE:
		nc = &RelationalExpression_ge_nodeclass;
		break;
	    case tINSTANCEOF:
		nc = &RelationalExpression_instanceof_nodeclass;
		break;
	    case tIN:
		if (!parser->noin) {
		    nc = &RelationalExpression_in_nodeclass;
		    break;
		} /* else Fallthrough */
	    default:
		return n;
	    }
	    rn = NEW_NODE(struct Binary_node, nc);
	    SKIP;
	    rn->a = n;
	    rn->b = PARSE(RelationalExpression);
	    parser->is_lhs = 0;
	    n = (struct node *)rn;
	}
}

/*
 *	-- 11.9
 *
 *	EqualityExpression
 *	:	RelationalExpression
 *	|	EqualityExpression tEQ RelationalExpression	-- 11.9.1
 *	|	EqualityExpression tNE RelationalExpression	-- 11.9.2
 *	|	EqualityExpression tSEQ RelationalExpression	-- 11.9.4
 *	|	EqualityExpression tSNE RelationalExpression	-- 11.9.5
 *	;
 *
 *	EqualityExpressionNoIn
 *	:	RelationalExpressionNoIn
 *	|	EqualityExpressionNoIn tEQ RelationalExpressionNoIn  -- 11.9.1
 *	|	EqualityExpressionNoIn tNE RelationalExpressionNoIn  -- 11.9.2
 *	|	EqualityExpressionNoIn tSEQ RelationalExpressionNoIn -- 11.9.4
 *	|	EqualityExpressionNoIn tSNE RelationalExpressionNoIn -- 11.9.5
 *	;
 */

/* 
 * 11.9.3 Abstract equality function.
 */
static void
EqualityExpression_eq(interp, x, y, res)
	struct SEE_interpreter *interp;
	struct SEE_value *x, *y, *res;
{
	struct SEE_value tmp;
	int xtype, ytype;

	if (SEE_VALUE_GET_TYPE(x) == SEE_VALUE_GET_TYPE(y))
	    switch (SEE_VALUE_GET_TYPE(x)) {
	    case SEE_UNDEFINED:
	    case SEE_NULL:
		SEE_SET_BOOLEAN(res, 1);
		return;
	    case SEE_NUMBER:
		if (SEE_NUMBER_ISNAN(x) || SEE_NUMBER_ISNAN(y))
		    SEE_SET_BOOLEAN(res, 0);
		else
		    SEE_SET_BOOLEAN(res, x->u.number == y->u.number);
		return;
	    case SEE_STRING:
		SEE_SET_BOOLEAN(res, SEE_string_cmp(x->u.string,
		    y->u.string) == 0);
		return;
	    case SEE_BOOLEAN:
		SEE_SET_BOOLEAN(res, !x->u.boolean == !y->u.boolean);
		return;
	    case SEE_OBJECT:
		SEE_SET_BOOLEAN(res, 
			SEE_OBJECT_JOINED(x->u.object, y->u.object));
		return;
	    default:
		SEE_error_throw_string(interp, interp->Error,
			STR(internal_error));
	    }
	xtype = SEE_VALUE_GET_TYPE(x);
	ytype = SEE_VALUE_GET_TYPE(y);
	if (xtype == SEE_NULL && ytype == SEE_UNDEFINED)
		SEE_SET_BOOLEAN(res, 1);
	else if (xtype == SEE_UNDEFINED && ytype == SEE_NULL)
		SEE_SET_BOOLEAN(res, 1);
	else if (xtype == SEE_NUMBER && ytype == SEE_STRING) {
		SEE_ToNumber(interp, y, &tmp);
		EqualityExpression_eq(interp, x, &tmp, res);
	} else if (xtype == SEE_STRING && ytype == SEE_NUMBER) {
		SEE_ToNumber(interp, x, &tmp);
		EqualityExpression_eq(interp, &tmp, y, res);
	} else if (xtype == SEE_BOOLEAN) {
		SEE_ToNumber(interp, x, &tmp);
		EqualityExpression_eq(interp, &tmp, y, res);
	} else if (ytype == SEE_BOOLEAN) {
		SEE_ToNumber(interp, y, &tmp);
		EqualityExpression_eq(interp, x, &tmp, res);
	} else if ((xtype == SEE_STRING || xtype == SEE_NUMBER) &&
		    ytype == SEE_OBJECT) {
		SEE_ToPrimitive(interp, y, x, &tmp);
		EqualityExpression_eq(interp, x, &tmp, res);
	} else if ((ytype == SEE_STRING || ytype == SEE_NUMBER) &&
		    xtype == SEE_OBJECT) {
		SEE_ToPrimitive(interp, x, y, &tmp);
		EqualityExpression_eq(interp, &tmp, y, res);
	} else
		SEE_SET_BOOLEAN(res, 0);
}

/*
 * 19.9.6 Strict equality function
 */
static void
EqualityExpression_seq(context, x, y, res)
	struct SEE_context *context;
	struct SEE_value *x, *y, *res;
{
	if (SEE_VALUE_GET_TYPE(x) != SEE_VALUE_GET_TYPE(y))
	    SEE_SET_BOOLEAN(res, 0);
	else
	    switch (SEE_VALUE_GET_TYPE(x)) {
	    case SEE_UNDEFINED:
		SEE_SET_BOOLEAN(res, 1);
		break;
	    case SEE_NULL:
		SEE_SET_BOOLEAN(res, 1);
		break;
	    case SEE_NUMBER:
		if (SEE_NUMBER_ISNAN(x) || SEE_NUMBER_ISNAN(y))
			SEE_SET_BOOLEAN(res, 0);
		else
			SEE_SET_BOOLEAN(res, x->u.number == y->u.number);
		break;
	    case SEE_STRING:
		SEE_SET_BOOLEAN(res, SEE_string_cmp(x->u.string, 
		    y->u.string) == 0);
		break;
	    case SEE_BOOLEAN:
		SEE_SET_BOOLEAN(res, !x->u.boolean == !y->u.boolean);
		break;
	    case SEE_OBJECT:
		SEE_SET_BOOLEAN(res, 
			SEE_OBJECT_JOINED(x->u.object, y->u.object));
		break;
	    default:
		SEE_SET_BOOLEAN(res, 0);
	    }
}

static void
EqualityExpression_eq_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	EqualityExpression_eq(context->interpreter, &r4, &r2, res);
}

#if WITH_PARSER_PRINT
static void
EqualityExpression_eq_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('=');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass EqualityExpression_eq_nodeclass
	= { SUPERCLASS(Binary)
	    EqualityExpression_eq_eval, 0, 
	    PARSER_PRINT(EqualityExpression_eq_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


static void
EqualityExpression_ne_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4, t;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	EqualityExpression_eq(context->interpreter, &r4, &r2, &t);
	SEE_SET_BOOLEAN(res, !t.u.boolean);
}

#if WITH_PARSER_PRINT
static void
EqualityExpression_ne_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('!');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass EqualityExpression_ne_nodeclass
	= { SUPERCLASS(Binary)
	    EqualityExpression_ne_eval, 0, 
	    PARSER_PRINT(EqualityExpression_ne_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


static void
EqualityExpression_seq_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	EqualityExpression_seq(context, &r4, &r2, res);
}

#if WITH_PARSER_PRINT
static void
EqualityExpression_seq_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('=');
	PRINT_CHAR('=');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass EqualityExpression_seq_nodeclass
	= { SUPERCLASS(Binary)
	    EqualityExpression_seq_eval, 0, 
	    PARSER_PRINT(EqualityExpression_seq_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


static void
EqualityExpression_sne_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3, r4, r5;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, &r4);
	EqualityExpression_seq(context, &r4, &r2, &r5);
	SEE_SET_BOOLEAN(res, !r5.u.boolean);
}

#if WITH_PARSER_PRINT
static void
EqualityExpression_sne_print(na, printer)
	struct printer *printer;
	struct node *na; /* (struct Binary_node) */
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('!');
	PRINT_CHAR('=');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass EqualityExpression_sne_nodeclass
	= { SUPERCLASS(Binary)
	    EqualityExpression_sne_eval, 0, 
	    PARSER_PRINT(EqualityExpression_sne_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


static struct node *
EqualityExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct nodeclass *nc;
	struct Binary_node *rn;

	n = PARSE(RelationalExpression);
	for (;;) {
	    /* Left associative */
	    switch (NEXT) {
	    case tEQ:
		nc = &EqualityExpression_eq_nodeclass;
		break;
	    case tNE:
		nc = &EqualityExpression_ne_nodeclass;
		break;
	    case tSEQ:
		nc = &EqualityExpression_seq_nodeclass;
		break;
	    case tSNE:
		nc = &EqualityExpression_sne_nodeclass;
		break;
	    default:
		return n;
	    }
	    rn = NEW_NODE(struct Binary_node, nc);
	    SKIP;
	    rn->a = n;
	    rn->b = PARSE(EqualityExpression);
	    parser->is_lhs = 0;
	    n = (struct node *)rn;
	}
}

/*
 *	-- 11.10
 *
 *	BitwiseANDExpression
 *	:	EqualityExpression
 *	|	BitwiseANDExpression '&' EqualityExpression
 *	;
 *
 *	BitwiseANDExpressionNoIn
 *	:	EqualityExpressionNoIn
 *	|	BitwiseANDExpressionNoIn '&' EqualityExpressionNoIn
 *	;
 */

/* 11.10 */
static void
BitwiseANDExpression_common(r2, bn, context, res)
	struct SEE_value *r2, *res;
	struct node *bn;
	struct SEE_context *context;
{
	struct SEE_value r3, r4;
	SEE_int32_t r5, r6;

	EVAL(bn, context, &r3);
	GetValue(context, &r3, &r4);
	r5 = SEE_ToInt32(context->interpreter, r2);
	r6 = SEE_ToInt32(context->interpreter, &r4);
	SEE_SET_NUMBER(res, r5 & r6);
}

static void
BitwiseANDExpression_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	BitwiseANDExpression_common(&r2, n->b, context, res);
}

#if WITH_PARSER_PRINT
static void
BitwiseANDExpression_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('&');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass BitwiseANDExpression_nodeclass
	= { SUPERCLASS(Binary)
	    BitwiseANDExpression_eval, 0, 
	    PARSER_PRINT(BitwiseANDExpression_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


static struct node *
BitwiseANDExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *m;

	n = PARSE(EqualityExpression);
	if (NEXT != '&') 
		return n;
	m = NEW_NODE(struct Binary_node,
			&BitwiseANDExpression_nodeclass);
	SKIP;
	m->a = n;
	m->b = PARSE(BitwiseANDExpression);
	parser->is_lhs = 0;
	return (struct node *)m;
}

/*
 *	BitwiseXORExpression
 *	:	BitwiseANDExpression
 *	|	BitwiseXORExpression '^' BitwiseANDExpression
 *	;
 *
 *	BitwiseXORExpressionNoIn
 *	:	BitwiseANDExpressionNoIn
 *	|	BitwiseXORExpressionNoIn '^' BitwiseANDExpressionNoIn
 *	;
 */

/* 11.10 */
static void
BitwiseXORExpression_common(r2, bn, context, res)
	struct SEE_value *r2, *res;
	struct node *bn;
	struct SEE_context *context;
{
	struct SEE_value r3, r4;
	SEE_int32_t r5, r6;

	EVAL(bn, context, &r3);
	GetValue(context, &r3, &r4);
	r5 = SEE_ToInt32(context->interpreter, r2);
	r6 = SEE_ToInt32(context->interpreter, &r4);
	SEE_SET_NUMBER(res, r5 ^ r6);
}

static void
BitwiseXORExpression_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	BitwiseXORExpression_common(&r2, n->b, context, res);
}

#if WITH_PARSER_PRINT
static void
BitwiseXORExpression_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('^');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass BitwiseXORExpression_nodeclass
	= { SUPERCLASS(Binary)
	    BitwiseXORExpression_eval, 0, 
	    PARSER_PRINT(BitwiseXORExpression_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


static struct node *
BitwiseXORExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *m;

	n = PARSE(BitwiseANDExpression);
	if (NEXT != '^') 
		return n;
	m = NEW_NODE(struct Binary_node,
			&BitwiseXORExpression_nodeclass);
	SKIP;
	m->a = n;
	m->b = PARSE(BitwiseXORExpression);
	parser->is_lhs = 0;
	return (struct node *)m;
}

/*
 *	BitwiseORExpression
 *	:	BitwiseXORExpression
 *	|	BitwiseORExpression '|' BitwiseXORExpression
 *	;
 *
 *	BitwiseORExpressionNoIn
 *	:	BitwiseXORExpressionNoIn
 *	|	BitwiseORExpressionNoIn '|' BitwiseXORExpressionNoIn
 *	;
 */

/* 11.10 */
static void
BitwiseORExpression_common(r2, bn, context, res)
	struct SEE_value *r2, *res;
	struct node *bn;
	struct SEE_context *context;
{
	struct SEE_value r3, r4;
	SEE_int32_t r5, r6;

	EVAL(bn, context, &r3);
	GetValue(context, &r3, &r4);
	r5 = SEE_ToInt32(context->interpreter, r2);
	r6 = SEE_ToInt32(context->interpreter, &r4);
	SEE_SET_NUMBER(res, r5 | r6);
}

static void
BitwiseORExpression_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	BitwiseORExpression_common(&r2, n->b, context, res);
}

#if WITH_PARSER_PRINT
static void
BitwiseORExpression_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('|');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static struct nodeclass BitwiseORExpression_nodeclass
	= { SUPERCLASS(Binary)
	    BitwiseORExpression_eval, 0, 
	    PARSER_PRINT(BitwiseORExpression_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


static struct node *
BitwiseORExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *m;

	n = PARSE(BitwiseXORExpression);
	if (NEXT != '|') 
		return n;
	m = NEW_NODE(struct Binary_node,
			&BitwiseORExpression_nodeclass);
	SKIP;
	m->a = n;
	m->b = PARSE(BitwiseORExpression);
	parser->is_lhs = 0;
	return (struct node *)m;
}

/*
 *	-- 11.11
 *
 *	LogicalANDExpression
 *	:	BitwiseORExpression
 *	|	LogicalANDExpression tANDAND BitwiseORExpression
 *	;
 *
 *	LogicalANDExpressionNoIn
 *	:	BitwiseORExpressionNoIn
 *	|	LogicalANDExpressionNoIn tANDAND BitwiseORExpressionNoIn
 *	;
 */

static void
LogicalANDExpression_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r3, r5;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, res);
	SEE_ToBoolean(context->interpreter, res, &r3);
	if (!r3.u.boolean)
		return;
	EVAL(n->b, context, &r5);
	GetValue(context, &r5, res);
}

#if WITH_PARSER_PRINT
static void
LogicalANDExpression_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('&');
	PRINT_CHAR('&');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static int
LogicalANDExpression_isconst(na, interp)
	struct node *na; /* (struct Binary_node) */
	struct SEE_interpreter *interp;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	if (ISCONST(n->a, interp)) {
		struct SEE_value r1, r3;
		EVAL(n->a, (struct SEE_context *)NULL, &r1);
		SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&r1) != SEE_REFERENCE);
		SEE_ToBoolean(interp, &r1, &r3);
		return r3.u.boolean ? ISCONST(n->b, interp) : 1;
	} else
		return 0;
}

static struct nodeclass LogicalANDExpression_nodeclass
	= { SUPERCLASS(Binary)
	    LogicalANDExpression_eval, 0, 
	    PARSER_PRINT(LogicalANDExpression_print)
	    PARSER_VISIT(Binary_visit)
	    LogicalANDExpression_isconst };


static struct node *
LogicalANDExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *m;

	n = PARSE(BitwiseORExpression);
	if (NEXT != tANDAND) 
		return n;
	m = NEW_NODE(struct Binary_node,
			&LogicalANDExpression_nodeclass);
	SKIP;
	m->a = n;
	m->b = PARSE(LogicalANDExpression);
	parser->is_lhs = 0;
	return (struct node *)m;
}

/*
 *	LogicalORExpression
 *	:	LogicalANDExpression
 *	|	LogicalORExpression tOROR LogicalANDExpression
 *	;
 *
 *	LogicalORExpressionNoIn
 *	:	LogicalANDExpressionNoIn
 *	|	LogicalORExpressionNoIn tOROR LogicalANDExpressionNoIn
 *	;
 */

static void
LogicalORExpression_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r3, r5;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, res);
	SEE_ToBoolean(context->interpreter, res, &r3);
	if (r3.u.boolean)
		return;
	EVAL(n->b, context, &r5);
	GetValue(context, &r5, res);
}

#if WITH_PARSER_PRINT
static void
LogicalORExpression_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINTP(n->a);
	PRINT_CHAR('|');
	PRINT_CHAR('|');
	PRINT_CHAR(' ');
	PRINTP(n->b);
}
#endif

static int
LogicalORExpression_isconst(na, interp)
	struct node *na; /* (struct Binary_node) */
	struct SEE_interpreter *interp;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	if (ISCONST(n->a, interp)) {
		struct SEE_value r1, r3;
		EVAL(n->a, (struct SEE_context *)NULL, &r1);
		SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&r1) != SEE_REFERENCE);
		SEE_ToBoolean(interp, &r1, &r3);
		return r3.u.boolean ? 1: ISCONST(n->b, interp);
	} else
		return 0;
}

static struct nodeclass LogicalORExpression_nodeclass
	= { SUPERCLASS(Binary)
	    LogicalORExpression_eval, 0, 
	    PARSER_PRINT(LogicalORExpression_print)
	    PARSER_VISIT(Binary_visit)
	    LogicalORExpression_isconst };


static struct node *
LogicalORExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *m;

	n = PARSE(LogicalANDExpression);
	if (NEXT != tOROR) 
		return n;
	m = NEW_NODE(struct Binary_node,
			&LogicalORExpression_nodeclass);
	SKIP;
	m->a = n;
	m->b = PARSE(LogicalORExpression);
	parser->is_lhs = 0;
	return (struct node *)m;
}

/*
 *	-- 11.12
 *
 *	ConditionalExpression
 *	:	LogicalORExpression
 *	|	LogicalORExpression '?' 
 *			AssignmentExpression ':' AssignmentExpression
 *	;
 *
 *	ConditionalExpressionNoIn
 *	:	LogicalORExpressionNoIn
 *	|	LogicalORExpressionNoIn '?' 
 *			AssignmentExpressionNoIn ':' AssignmentExpressionNoIn
 *	;
 */

struct ConditionalExpression_node {
	struct node node;
	struct node *a, *b, *c;
};

static void
ConditionalExpression_eval(na, context, res)
	struct node *na; /* (struct ConditionalExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct ConditionalExpression_node *n = 
		CAST_NODE(na, ConditionalExpression);
	struct SEE_value r1, r2, r3, t;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToBoolean(context->interpreter, &r2, &r3);
	if (r3.u.boolean)
		EVAL(n->b, context, &t);
	else
		EVAL(n->c, context, &t);
	GetValue(context, &t, res);
}

#if WITH_PARSER_PRINT
static void
ConditionalExpression_print(na, printer)
	struct node *na; /* (struct ConditionalExpression_node) */
	struct printer *printer;
{
	struct ConditionalExpression_node *n = 
		CAST_NODE(na, ConditionalExpression);
	PRINTP(n->a);
	PRINT_CHAR('?');
	PRINT_CHAR(' ');
	PRINTP(n->b);
	PRINT_CHAR(':');
	PRINT_CHAR(' ');
	PRINTP(n->c);
}
#endif

#if WITH_PARSER_VISIT
static void
ConditionalExpression_visit(na, v, va)
	struct node *na; /* (struct ConditionalExpression_node) */
	visitor_fn_t v;
	void *va;
{
	struct ConditionalExpression_node *n = 
		CAST_NODE(na, ConditionalExpression);
	VISIT(n->a, v, va);
	VISIT(n->b, v, va);
	VISIT(n->c, v, va);
}
#endif

static int
ConditionalExpression_isconst(na, interp)
	struct node *na; /* (struct ConditionalExpression_node) */
	struct SEE_interpreter *interp;
{
	struct ConditionalExpression_node *n = 
		CAST_NODE(na, ConditionalExpression);
	if (ISCONST(n->a, interp)) {
		struct SEE_value r1, r3;
		EVAL(n->a, (struct SEE_context *)NULL, &r1);
		SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&r1) != SEE_REFERENCE);
		SEE_ToBoolean(interp, &r1, &r3);
		return r3.u.boolean 
		    ? ISCONST(n->b, interp) 
		    : ISCONST(n->c, interp);
	} else
		return 0;
}

static struct nodeclass ConditionalExpression_nodeclass
	= { BASECLASS
	    ConditionalExpression_eval, 0, 
	    PARSER_PRINT(ConditionalExpression_print)
	    PARSER_VISIT(ConditionalExpression_visit)
	    ConditionalExpression_isconst };

static struct node *
ConditionalExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct ConditionalExpression_node *m;

	n = PARSE(LogicalORExpression);
	if (NEXT != '?') 
		return n;
	m = NEW_NODE(struct ConditionalExpression_node,
			&ConditionalExpression_nodeclass);
	SKIP;
	m->a = n;
	m->b = PARSE(AssignmentExpression);
	EXPECT(':');
	m->c = PARSE(AssignmentExpression);
	parser->is_lhs = 0;
	return (struct node *)m;
}

/*
 *	-- 11.13
 *
 *	AssignmentExpression
 *	:	ConditionalExpression
 *	|	LeftHandSideExpression AssignmentOperator AssignmentExpression
 *	;
 *
 *	AssignmentExpressionNoIn
 *	:	ConditionalExpressionNoIn
 *	|	LeftHandSideExpression AssignmentOperator 
 *						AssignmentExpressionNoIn
 *	;
 *
 *	AssignmentOperator
 *	:	'='				-- 11.13.1
 *	|	tSTAREQ				-- 11.13.2
 *	|	tDIVEQ
 *	|	tMODEQ
 *	|	tPLUSEQ
 *	|	tMINUSEQ
 *	|	tLSHIFTEQ
 *	|	tRSHIFTEQ
 *	|	tURSHIFTEQ
 *	|	tANDEQ
 *	|	tXOREQ
 *	|	tOREQ
 *	;
 */

struct AssignmentExpression_node {
	struct node node;
	struct node *lhs, *expr;
};

#if WITH_PARSER_VISIT
static void
AssignmentExpression_visit(na, v, va)
	struct node *na; /* (struct AssignmentExpression_node) */
	visitor_fn_t v;
	void *va;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	VISIT(n->lhs, v, va);
	VISIT(n->expr, v, va);
}
#endif

static struct nodeclass AssignmentExpression_nodeclass	/* Abstract */
	= { BASECLASS
	    0, 0, 
	    PARSER_PRINT(0)
	    PARSER_VISIT(AssignmentExpression_visit)
	    0 };

static void
AssignmentExpression_simple_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2;

	EVAL(n->lhs, context, &r1);
	EVAL(n->expr, context, &r2);
	GetValue(context, &r2, res);
	PutValue(context, &r1, res);
}

#if WITH_PARSER_PRINT
static void
AssignmentExpression_simple_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}
#endif

static struct nodeclass AssignmentExpression_simple_nodeclass
	= { SUPERCLASS(AssignmentExpression)
	    AssignmentExpression_simple_eval, 0, 
	    PARSER_PRINT(AssignmentExpression_simple_print)
	    PARSER_VISIT(AssignmentExpression_visit)
	    0 };


/* 11.13.2 */
static void
AssignmentExpression_muleq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	MultiplicativeExpression_mul_common(&r2, n->expr, context, res);
	PutValue(context, &r1, res);
}

#if WITH_PARSER_PRINT
static void
AssignmentExpression_muleq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('*');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}
#endif

static struct nodeclass AssignmentExpression_muleq_nodeclass
	= { SUPERCLASS(AssignmentExpression)
	    AssignmentExpression_muleq_eval, 0, 
	    PARSER_PRINT(AssignmentExpression_muleq_print)
	    PARSER_VISIT(AssignmentExpression_visit)
	    0 };


/* 11.13.2 */
static void
AssignmentExpression_diveq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	MultiplicativeExpression_div_common(&r2, n->expr, context, res);
	PutValue(context, &r1, res);
}

#if WITH_PARSER_PRINT
static void
AssignmentExpression_diveq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('/');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}
#endif

static struct nodeclass AssignmentExpression_diveq_nodeclass
	= { SUPERCLASS(AssignmentExpression)
	    AssignmentExpression_diveq_eval, 0, 
	    PARSER_PRINT(AssignmentExpression_diveq_print)
	    PARSER_VISIT(AssignmentExpression_visit)
	    0 };


/* 11.13.2 */
static void
AssignmentExpression_modeq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	MultiplicativeExpression_mod_common(&r2, n->expr, context, res);
	PutValue(context, &r1, res);
}

#if WITH_PARSER_PRINT
static void
AssignmentExpression_modeq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('%');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}
#endif

static struct nodeclass AssignmentExpression_modeq_nodeclass
	= { SUPERCLASS(AssignmentExpression)
	    AssignmentExpression_modeq_eval, 0, 
	    PARSER_PRINT(AssignmentExpression_modeq_print)
	    PARSER_VISIT(AssignmentExpression_visit)
	    0 };


/* 11.13.2 */
static void
AssignmentExpression_addeq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	AdditiveExpression_add_common(&r2, n->expr, context, res);
	PutValue(context, &r1, res);
}

#if WITH_PARSER_PRINT
static void
AssignmentExpression_addeq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('+');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}
#endif

static struct nodeclass AssignmentExpression_addeq_nodeclass
	= { SUPERCLASS(AssignmentExpression)
	    AssignmentExpression_addeq_eval, 0, 
	    PARSER_PRINT(AssignmentExpression_addeq_print)
	    PARSER_VISIT(AssignmentExpression_visit) 
	    0 };


/* 11.13.2 */
static void
AssignmentExpression_subeq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	AdditiveExpression_sub_common(&r2, n->expr, context, res);
	PutValue(context, &r1, res);
}

#if WITH_PARSER_PRINT
static void
AssignmentExpression_subeq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('-');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}
#endif

static struct nodeclass AssignmentExpression_subeq_nodeclass
	= { SUPERCLASS(AssignmentExpression)
	    AssignmentExpression_subeq_eval, 0, 
	    PARSER_PRINT(AssignmentExpression_subeq_print)
	    PARSER_VISIT(AssignmentExpression_visit) 
	    0 };


/* 11.13.2 */
static void
AssignmentExpression_lshifteq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	ShiftExpression_lshift_common(&r2, n->expr, context, res);
	PutValue(context, &r1, res);
}

#if WITH_PARSER_PRINT
static void
AssignmentExpression_lshifteq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('<');
	PRINT_CHAR('<');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}
#endif

static struct nodeclass AssignmentExpression_lshifteq_nodeclass
	= { SUPERCLASS(AssignmentExpression)
	    AssignmentExpression_lshifteq_eval, 0, 
	    PARSER_PRINT(AssignmentExpression_lshifteq_print)
	    PARSER_VISIT(AssignmentExpression_visit)
	    0 };


/* 11.13.2 */
static void
AssignmentExpression_rshifteq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	ShiftExpression_rshift_common(&r2, n->expr, context, res);
	PutValue(context, &r1, res);
}

#if WITH_PARSER_PRINT
static void
AssignmentExpression_rshifteq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('>');
	PRINT_CHAR('>');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}
#endif

static struct nodeclass AssignmentExpression_rshifteq_nodeclass
	= { SUPERCLASS(AssignmentExpression)
	    AssignmentExpression_rshifteq_eval, 0, 
	    PARSER_PRINT(AssignmentExpression_rshifteq_print)
	    PARSER_VISIT(AssignmentExpression_visit)
	    0 };


/* 11.13.2 */
static void
AssignmentExpression_urshifteq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	ShiftExpression_urshift_common(&r2, n->expr, context, res);
	PutValue(context, &r1, res);
}

#if WITH_PARSER_PRINT
static void
AssignmentExpression_urshifteq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('>');
	PRINT_CHAR('>');
	PRINT_CHAR('>');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}
#endif

static struct nodeclass AssignmentExpression_urshifteq_nodeclass
	= { SUPERCLASS(AssignmentExpression)
	    AssignmentExpression_urshifteq_eval, 0, 
	    PARSER_PRINT(AssignmentExpression_urshifteq_print)
	    PARSER_VISIT(AssignmentExpression_visit)
	    0 };


/* 11.13.2 */
static void
AssignmentExpression_andeq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	BitwiseANDExpression_common(&r2, n->expr, context, res);
	PutValue(context, &r1, res);
}

#if WITH_PARSER_PRINT
static void
AssignmentExpression_andeq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('&');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}
#endif

static struct nodeclass AssignmentExpression_andeq_nodeclass
	= { SUPERCLASS(AssignmentExpression)
	    AssignmentExpression_andeq_eval, 0, 
	    PARSER_PRINT(AssignmentExpression_andeq_print)
	    PARSER_VISIT(AssignmentExpression_visit) 
	    0 };


/* 11.13.2 */
static void
AssignmentExpression_xoreq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	BitwiseXORExpression_common(&r2, n->expr, context, res);
	PutValue(context, &r1, res);
}

#if WITH_PARSER_PRINT
static void
AssignmentExpression_xoreq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('^');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}
#endif

static struct nodeclass AssignmentExpression_xoreq_nodeclass
	= { SUPERCLASS(AssignmentExpression)
	    AssignmentExpression_xoreq_eval, 0, 
	    PARSER_PRINT(AssignmentExpression_xoreq_print)
	    PARSER_VISIT(AssignmentExpression_visit)
	    0 };


/* 11.13.2 */
static void
AssignmentExpression_oreq_eval(na, context, res)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	struct SEE_value r1, r2;

	EVAL(n->lhs, context, &r1);
	GetValue(context, &r1, &r2);
	BitwiseORExpression_common(&r2, n->expr, context, res);
	PutValue(context, &r1, res);
}

#if WITH_PARSER_PRINT
static void
AssignmentExpression_oreq_print(na, printer)
	struct node *na; /* (struct AssignmentExpression_node) */
	struct printer *printer;
{
	struct AssignmentExpression_node *n = 
		CAST_NODE(na, AssignmentExpression);
	PRINTP(n->lhs);
	PRINT_CHAR('|');
	PRINT_CHAR('=');
	PRINT_CHAR(' ');
	PRINTP(n->expr);
}
#endif

static struct nodeclass AssignmentExpression_oreq_nodeclass
	= { SUPERCLASS(AssignmentExpression)
	    AssignmentExpression_oreq_eval, 0, 
	    PARSER_PRINT(AssignmentExpression_oreq_print)
	    PARSER_VISIT(AssignmentExpression_visit)
	    0 };


static struct node *
AssignmentExpression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct nodeclass *nc;
	struct AssignmentExpression_node *an;

	/*
	 * If, while recursing we parse LeftHandSideExpression,
	 * then is_lhs will be set to 1. Otherwise, it is just a 
	 * ConditionalExpression, and we cannot derive the second
	 * production in this rule. So we just return.
	 */
	n = PARSE(ConditionalExpression);
	if (!parser->is_lhs)
		return n;

	switch (NEXT) {
	case '=':
		nc = &AssignmentExpression_simple_nodeclass;
		break;
	case tSTAREQ:
		nc = &AssignmentExpression_muleq_nodeclass;
		break;
	case tDIVEQ:
		nc = &AssignmentExpression_diveq_nodeclass;
		break;
	case tMODEQ:
		nc = &AssignmentExpression_modeq_nodeclass;
		break;
	case tPLUSEQ:
		nc = &AssignmentExpression_addeq_nodeclass;
		break;
	case tMINUSEQ:
		nc = &AssignmentExpression_subeq_nodeclass;
		break;
	case tLSHIFTEQ:
		nc = &AssignmentExpression_lshifteq_nodeclass;
		break;
	case tRSHIFTEQ:
		nc = &AssignmentExpression_rshifteq_nodeclass;
		break;
	case tURSHIFTEQ:
		nc = &AssignmentExpression_urshifteq_nodeclass;
		break;
	case tANDEQ:
		nc = &AssignmentExpression_andeq_nodeclass;
		break;
	case tXOREQ:
		nc = &AssignmentExpression_xoreq_nodeclass;
		break;
	case tOREQ:
		nc = &AssignmentExpression_oreq_nodeclass;
		break;
	default:
		return n;
	}
	an = NEW_NODE(struct AssignmentExpression_node, nc);
	an->lhs = n;
	SKIP;
	an->expr = PARSE(AssignmentExpression);
	parser->is_lhs = 0;
	return (struct node *)an;
}

/*
 *	-- 11.14
 *
 *	Expression
 *	:	AssignmentExpression
 *	|	Expression ',' AssignmentExpression
 *	;
 *
 *	ExpressionNoIn
 *	:	AssignmentExpressionNoIn
 *	|	ExpressionNoIn ',' AssignmentExpressionNoIn
 *	;
 */

/* 11.14 */
static void
Expression_comma_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value r1, r2, r3;

	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	EVAL(n->b, context, &r3);
	GetValue(context, &r3, res);
}

#if WITH_PARSER_PRINT
static void
Expression_comma_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINT(n->a);
	PRINT_CHAR(',');
	PRINT_CHAR(' ');
	PRINT(n->b);
}
#endif

static struct nodeclass Expression_comma_nodeclass
	= { SUPERCLASS(Binary)
	    Expression_comma_eval, 0, 
	    PARSER_PRINT(Expression_comma_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };


static struct node *
Expression_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *cn;

	n = PARSE(AssignmentExpression);
	if (NEXT != ',')
		return n;
	cn = NEW_NODE(struct Binary_node, &Expression_comma_nodeclass);
	SKIP;
	cn->a = n;
	cn->b = PARSE(Expression);
	parser->is_lhs = 0;
	return (struct node *)cn;
}

/*
 *
 * -- 12
 *
 *	Statement
 *	:	Block
 *	|	VariableStatement
 *	|	EmptyStatement
 *	|	ExpressionStatement
 *	|	IfStatement
 *	|	IterationStatement
 *	|	ContinueStatement
 *	|	BreakStatement
 *	|	ReturnStatement
 *	|	WithStatement
 *	|	LabelledStatement
 *	|	SwitchStatement
 *	|	ThrowStatement
 *	|	TryStatement
 *	;
 */

static struct node *
Statement_parse(parser)
	struct parser *parser;
{
	struct node *n;

	parser->current_labelset = NULL;

	switch (NEXT) {
	case '{':
		return PARSE(Block);
	case tVAR:
		return PARSE(VariableStatement);
	case ';':
		return PARSE(EmptyStatement);
	case tIF:
		return PARSE(IfStatement);
	case tDO:
	case tWHILE:
	case tFOR:
		n = PARSE(IterationStatement);
		return n;
	case tCONTINUE:
		return PARSE(ContinueStatement);
	case tBREAK:
		return PARSE(BreakStatement);
	case tRETURN:
		return PARSE(ReturnStatement);
	case tWITH:
		return PARSE(WithStatement);
	case tSWITCH:
		n = PARSE(SwitchStatement);
		return n;
	case tTHROW:
		return PARSE(ThrowStatement);
	case tTRY:
		return PARSE(TryStatement);
	case tFUNCTION:
		/* Conditional functions for JS1.5 compatibility */
		if (SEE_COMPAT_JS(parser->interpreter, >= ,JS15) &&
		    lookahead(parser, 1) != '(')
			return PARSE(FunctionStatement);
		ERRORm("function keyword not allowed here");
	case tIDENT:
		if (lookahead(parser, 1) == ':')
			return PARSE(LabelledStatement);
		/* FALLTHROUGH */
	default:
		return PARSE(ExpressionStatement);
	}
}

/*
 *	-- 12.1
 *
 *	Block
 *	:	'{' '}'					-- 12.1
 *	|	'{' StatementList '}'			-- 12.1
 *	;
 */

/* 12.1 */
static void
Block_empty_eval(n, context, res)
	struct node *n;
	struct SEE_context *context;
	struct SEE_value *res;
{
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, NULL, NULL);
}

#if WITH_PARSER_PRINT
static void
Block_empty_print(n, printer)
	struct node *n;
	struct printer *printer;
{
	PRINT_CHAR('{');
	PRINT_CHAR('}');
}
#endif

static struct nodeclass Block_empty_nodeclass
	= { BASECLASS
	    Block_empty_eval, 0, 
	    PARSER_PRINT(Block_empty_print)
	    PARSER_VISIT(0)
	    Always_isconst };

static struct node *
Block_parse(parser)
	struct parser *parser;
{
	struct node *n;

	EXPECT('{');
	if (NEXT == '}')
		n = NEW_NODE(struct node, &Block_empty_nodeclass);
	else
		n = PARSE(StatementList);
	EXPECT('}');
	return n;
}

/*
 *	StatementList
 *	:	Statement				-- 12.1
 *	|	StatementList Statement			-- 12.1
 *	;
 */

/* 12.1 */
static void
StatementList_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	struct SEE_value *val;

	EVAL(n->a, context, res);
	if (res->u.completion.type == SEE_COMPLETION_NORMAL) {
		val = res->u.completion.value;
		EVAL(n->b, context, res);
		if (res->u.completion.value == NULL)
			res->u.completion.value = val;
		else 
			SEE_free(context->interpreter, (void **)&val);
	}
}

static struct nodeclass StatementList_nodeclass
	= { SUPERCLASS(Binary)
	    StatementList_eval, 0, 
	    PARSER_PRINT(Binary_print)
	    PARSER_VISIT(Binary_visit)
	    Binary_isconst };

static struct node *
StatementList_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *ln;

	n = PARSE(Statement);
	switch (NEXT) {
	case '}':
	case tEND:
	case tFUNCTION:
	case tCASE:
	case tDEFAULT:
		return n;
	}
	ln = NEW_NODE(struct Binary_node, &StatementList_nodeclass);
	ln->a = n;
	ln->b = PARSE(StatementList);
	return (struct node *)ln;
}

/*
 *	-- 12.2
 *
 *	VariableStatement
 *	:	tVAR VariableDeclarationList ';'
 *	;
 *
 *	VariableDeclarationList
 *	:	VariableDeclaration
 *	|	VariableDeclarationList ',' VariableDeclaration
 *	;
 *
 *	VariableDeclarationListNoIn
 *	:	VariableDeclarationNoIn
 *	|	VariableDeclarationListNoIn ',' VariableDeclarationNoIn
 *	;
 *
 *	VariableDeclaration
 *	:	tIDENT
 *	|	tIDENT Initialiser
 *	;
 *
 *	VariableDeclarationNoIn
 *	:	tIDENT
 *	|	tIDENT InitialiserNoIn
 *	;
 *
 *	Initialiser
 *	:	'=' AssignmentExpression
 *	;
 *
 *	InitialiserNoIn
 *	:	'=' AssignmentExpressionNoIn
 *	;
 */

static void
VariableStatement_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value v;

	TRACE(na, context, SEE_TRACE_STATEMENT);
	EVAL(n->a, context, &v);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, NULL, NULL);
}

#if WITH_PARSER_PRINT
static void
VariableStatement_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_STRING(STR(var));
	PRINT_CHAR(' ');
	PRINT(n->a);
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}
#endif

static struct nodeclass VariableStatement_nodeclass
	= { SUPERCLASS(Unary)
	    VariableStatement_eval, 0, 
	    PARSER_PRINT(VariableStatement_print)
	    PARSER_VISIT(Unary_visit)
	    0 };

static struct node *
VariableStatement_parse(parser)
	struct parser *parser;
{
	struct Unary_node *n;

	n = NEW_NODE(struct Unary_node, &VariableStatement_nodeclass);
	EXPECT(tVAR);
	n->a = PARSE(VariableDeclarationList);
	EXPECT_SEMICOLON;
	return (struct node *)n;
}


/* 12.2 */
static void
VariableDeclarationList_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;		/* unused */
{
	struct Binary_node *n = CAST_NODE(na, Binary);

	EVAL(n->a, context, res);
	EVAL(n->b, context, res);
}

#if WITH_PARSER_PRINT
static void
VariableDeclarationList_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINT(n->a);
	PRINT_CHAR(',');
	PRINT_CHAR(' ');
	PRINT(n->b);
}
#endif

static struct nodeclass VariableDeclarationList_nodeclass
	= { SUPERCLASS(Binary)
	    VariableDeclarationList_eval, 0, 
	    PARSER_PRINT(VariableDeclarationList_print)
	    PARSER_VISIT(Binary_visit)
	    0 };

static struct node *
VariableDeclarationList_parse(parser)
	struct parser *parser;
{
	struct node *n;
	struct Binary_node *ln;

	n = PARSE(VariableDeclaration);
	if (NEXT != ',') 
		return n;
	ln = NEW_NODE(struct Binary_node, &VariableDeclarationList_nodeclass);
	SKIP;
	/* NB: IterationStatement_parse() also constructs a VarDeclList */
	ln->a = n;
	ln->b = PARSE(VariableDeclarationList);
	return (struct node *)ln;
}


struct VariableDeclaration_node {
	struct node node;
	struct var var;
	struct node *init;
};

static void
VariableDeclaration_eval(na, context, res)
	struct node *na; /* (struct VariableDeclaration_node) */
	struct SEE_context *context;
	struct SEE_value *res;		/* unused */
{
	struct VariableDeclaration_node *n = 
		CAST_NODE(na, VariableDeclaration);
	struct SEE_value r1, r2, r3;

	if (n->init) {
		SEE_scope_lookup(context->interpreter, context->scope, 
			n->var.name, &r1);
		EVAL(n->init, context, &r2);
		GetValue(context, &r2, &r3);
		PutValue(context, &r1, &r3);
	}
}

/*
 * Note: All declared vars end up attached to a function body's vars
 * list, and are set to undefined upon entry to that function.
 * See also:
 *	SEE_function_put_args()		- put args
 *	FunctionDeclaration_fproc()	- put func decls
 *	SourceElements_fproc()		- put vars
 */

#if WITH_PARSER_PRINT
static void
VariableDeclaration_print(na, printer)
	struct node *na; /* (struct VariableDeclaration_node) */
	struct printer *printer;
{
	struct VariableDeclaration_node *n = 
		CAST_NODE(na, VariableDeclaration);
	PRINT_STRING(n->var.name);
	PRINT_CHAR(' ');
	if (n->init) {
		PRINT_CHAR('=');
		PRINT_CHAR(' ');
		PRINT(n->init);
	}
}
#endif

#if WITH_PARSER_VISIT
static void
VariableDeclaration_visit(na, v, va)
	struct node *na; /* (struct VariableDeclaration_node) */
	visitor_fn_t v;
	void *va;
{
	struct VariableDeclaration_node *n = 
		CAST_NODE(na, VariableDeclaration);
	if (n->init)
		VISIT(n->init, v, va);
}
#endif

static struct nodeclass VariableDeclaration_nodeclass
	= { BASECLASS
	    VariableDeclaration_eval, 0, 
	    PARSER_PRINT(VariableDeclaration_print)
	    PARSER_VISIT(VariableDeclaration_visit)
	    0 };


static struct node *
VariableDeclaration_parse(parser)
	struct parser *parser;
{
	struct VariableDeclaration_node *v;

	v = NEW_NODE(struct VariableDeclaration_node, 
		&VariableDeclaration_nodeclass);
	if (NEXT == tIDENT)
		v->var.name = NEXT_VALUE->u.string;
	EXPECT(tIDENT);
	if (NEXT == '=') {
		SKIP;
		v->init = PARSE(AssignmentExpression);
	} else
		v->init = NULL;

	/* Record declared variables */
	if (parser->vars) {
		*parser->vars = &v->var;
		parser->vars = &v->var.next;
	}

	return (struct node *)v;
}

/*
 *	-- 12.3
 *
 *	EmptyStatement
 *	:	';'
 *	;
 */

/* 12.3 */
static void
EmptyStatement_eval(na, context, res)
	struct node *na;
	struct SEE_context *context;
	struct SEE_value *res;
{
	TRACE(na, context, SEE_TRACE_STATEMENT);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, NULL, NULL);
}

#if WITH_PARSER_PRINT
static void
EmptyStatement_print(n, printer)
	struct node *n;
	struct printer *printer;
{
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}
#endif

static struct nodeclass EmptyStatement_nodeclass
	= { BASECLASS
	    EmptyStatement_eval, 0, 
	    PARSER_PRINT(EmptyStatement_print)
	    PARSER_VISIT(0)
	    Always_isconst };

static struct node *
EmptyStatement_parse(parser)
	struct parser *parser;
{
	struct node *n;

	n = NEW_NODE(struct node, &EmptyStatement_nodeclass);
	EXPECT_SEMICOLON;
	return n;
}

/*
 *	-- 12.4
 *
 *	ExpressionStatement
 *	:	Expression ';'		-- lookahead != '{' or tFUNCTION
 *	;
 */

/* 12.4 */
static void
ExpressionStatement_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value *v = SEE_NEW(context->interpreter, struct SEE_value);

	TRACE(na, context, SEE_TRACE_STATEMENT);
	EVAL(n->a, context, v);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NULL);
}

#if WITH_PARSER_PRINT
static void
ExpressionStatement_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT(n->a);
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}
#endif

static struct nodeclass ExpressionStatement_nodeclass
	= { SUPERCLASS(Unary)
	    ExpressionStatement_eval, 0, 
	    PARSER_PRINT(ExpressionStatement_print)
	    PARSER_VISIT(Unary_visit)
	    Unary_isconst };

static struct node *
ExpressionStatement_parse(parser)
	struct parser *parser;
{
	struct Unary_node *n;

	n = NEW_NODE(struct Unary_node, &ExpressionStatement_nodeclass);
	n->a = PARSE(Expression);
	EXPECT_SEMICOLON;
	return (struct node *)n;
}

/*
 *	-- 12.5
 *
 *	IfStatement
 *	:	tIF '(' Expression ')' Statement tELSE Statement
 *	|	tIF '(' Expression ')' Statement
 *	;
 */

struct IfStatement_node {
	struct node node;
	struct node *cond, *btrue, *bfalse;
};

/* 12.5 */
static void
IfStatement_eval(na, context, res)
	struct node *na; /* (struct IfStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct IfStatement_node *n = CAST_NODE(na, IfStatement);
	struct SEE_value r1, r2, r3;

	TRACE(na, context, SEE_TRACE_STATEMENT);
	EVAL(n->cond, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToBoolean(context->interpreter, &r2, &r3);
	if (r3.u.boolean)
		EVAL(n->btrue, context, res);
	else if (n->bfalse)
		EVAL(n->bfalse, context, res);
	else
		_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, NULL, NULL);
}

#if WITH_PARSER_PRINT
static void
IfStatement_print(na, printer)
	struct node *na; /* (struct IfStatement_node) */
	struct printer *printer;
{
	struct IfStatement_node *n = CAST_NODE(na, IfStatement);
	PRINT_STRING(STR(if));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT(n->cond);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->btrue);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
	if (n->bfalse) {
	    PRINT_STRING(STR(else));
	    PRINT_CHAR('{');
	    PRINT_NEWLINE(1);
	    PRINT(n->bfalse);
	    PRINT_CHAR('}');
	    PRINT_NEWLINE(-1);
	}
}
#endif

#if WITH_PARSER_VISIT
static void
IfStatement_visit(na, v, va)
	struct node *na; /* (struct IfStatement_node) */
	visitor_fn_t v;
	void *va;
{
	struct IfStatement_node *n = CAST_NODE(na, IfStatement);
	VISIT(n->cond, v, va);
	VISIT(n->btrue, v, va);
	if (n->bfalse)
	    VISIT(n->bfalse, v, va);
}
#endif

static int
IfStatement_isconst(na, interp)
	struct node *na; /* (struct IfStatement_node) */
	struct SEE_interpreter *interp;
{
	struct IfStatement_node *n = CAST_NODE(na, IfStatement);
	if (ISCONST(n->cond, interp)) {
		struct SEE_value r1, r3;
		EVAL(n->cond, (struct SEE_context *)NULL, &r1);
		SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&r1) != SEE_REFERENCE);
		SEE_ToBoolean(interp, &r1, &r3);
		return r3.u.boolean 
		    ? ISCONST(n->btrue, interp) 
		    : (n->bfalse ? ISCONST(n->bfalse, interp) : 1);
	} else
		return 0;
}

static struct nodeclass IfStatement_nodeclass
	= { BASECLASS
	    IfStatement_eval, 0, 
	    PARSER_PRINT(IfStatement_print)
	    PARSER_VISIT(IfStatement_visit)
	    IfStatement_isconst };

static struct node *
IfStatement_parse(parser)
	struct parser *parser;
{
	struct node *cond, *btrue, *bfalse;
	struct IfStatement_node *n;

	n = NEW_NODE(struct IfStatement_node, &IfStatement_nodeclass);
	EXPECT(tIF);
	EXPECT('(');
	cond = PARSE(Expression);
	EXPECT(')');
	btrue = PARSE(Statement);
	if (NEXT != tELSE)
		bfalse = NULL;
	else {
		SKIP; /* 'else' */
		bfalse = PARSE(Statement);
	}
	n->cond = cond;
	n->btrue = btrue;
	n->bfalse = bfalse;
	return (struct node *)n;
}

/*
 *	-- 12.6
 *	IterationStatement
 *	:	tDO Statement tWHILE '(' Expression ')' ';'	-- 12.6.1
 *	|	tWHILE '(' Expression ')' Statement		-- 12.6.2
 *	|	tFOR '(' ';' ';' ')' Statement
 *	|	tFOR '(' ExpressionNoIn ';' ';' ')' Statement
 *	|	tFOR '(' ';' Expression ';' ')' Statement
 *	|	tFOR '(' ExpressionNoIn ';' Expression ';' ')' Statement
 *	|	tFOR '(' ';' ';' Expression ')' Statement
 *	|	tFOR '(' ExpressionNoIn ';' ';' Expression ')' Statement
 *	|	tFOR '(' ';' Expression ';' Expression ')' Statement
 *	|	tFOR '(' ExpressionNoIn ';' Expression ';' Expression ')'
 *			Statement
 *	|	tFOR '(' tVAR VariableDeclarationListNoIn ';' ';' ')' Statement
 *	|	tFOR '(' tVAR VariableDeclarationListNoIn ';'  
 *			Expression ';' ')' Statement
 *	|	tFOR '(' tVAR VariableDeclarationListNoIn ';' ';' 
 *			Expression ')' Statement
 *	|	tFOR '(' tVAR VariableDeclarationListNoIn ';' Expression ';' 
 *			Expression ')' Statement
 *	|	tFOR '(' LeftHandSideExpression tIN Expression ')' Statement
 *	|	tFOR '(' tVAR VariableDeclarationNoIn tIN Expression ')' 
 *			Statement
 *	;
 */

struct IterationStatement_while_node {
	struct Targetable_node targetable;
	struct node *cond, *body;
};

static void
IterationStatement_dowhile_eval(na, context, res)
	struct node *na; /* (struct IterationStatement_while_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct IterationStatement_while_node *n = 
		CAST_NODE(na, IterationStatement_while);
	struct SEE_value *v, r7, r8, r9;

	v = NULL;
step2:	EVAL(n->body, context, res);
	if (res->u.completion.value)
	    v = res->u.completion.value;
	if (res->u.completion.type == SEE_COMPLETION_CONTINUE &&
	    target_matches(&n->targetable, res->u.completion.target))
	    goto step7;
	if (res->u.completion.type == SEE_COMPLETION_BREAK &&
	    target_matches(&n->targetable, res->u.completion.target))
	{
	    _SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NULL);
	    return;
	}
	if (res->u.completion.type != SEE_COMPLETION_NORMAL)
	    return;
 step7: TRACE(na, context, SEE_TRACE_STATEMENT);
 	EVAL(n->cond, context, &r7);
	GetValue(context, &r7, &r8);
	SEE_ToBoolean(context->interpreter, &r8, &r9);
	if (r9.u.boolean)
		goto step2;
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NULL);
}

#if WITH_PARSER_PRINT
static void
IterationStatement_dowhile_print(na, printer)
	struct node *na; /* (struct IterationStatement_while_node) */
	struct printer *printer;
{
	struct IterationStatement_while_node *n = 
		CAST_NODE(na, IterationStatement_while);

	PRINT_STRING(STR(do));
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->body);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
	PRINT_STRING(STR(while));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT(n->cond);
	PRINT_CHAR(')');
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}
#endif

#if WITH_PARSER_VISIT
static void
IterationStatement_while_visit(na, v, va)
        struct node *na; /* (struct IterationStatement_while_node) */
	visitor_fn_t v;
	void *va;
{
        struct IterationStatement_while_node *n = 
		CAST_NODE(na, IterationStatement_while);

	VISIT(n->cond, v, va);
	VISIT(n->body, v, va);
}
#endif

static int
IterationStatement_dowhile_isconst(na, interp)
        struct node *na; /* (struct IterationStatement_while_node) */
	struct SEE_interpreter *interp;
{
        struct IterationStatement_while_node *n = 
		CAST_NODE(na, IterationStatement_while);

	if (ISCONST(n->cond, interp)) {
		struct SEE_value r1, r3;
		EVAL(n->cond, (struct SEE_context *)NULL, &r1);
		SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&r1) != SEE_REFERENCE);
		SEE_ToBoolean(interp, &r1, &r3);
		return r3.u.boolean ? 0 : ISCONST(n->body, interp);
	} else
		return 0;
}

static struct nodeclass IterationStatement_dowhile_nodeclass
	= { SUPERCLASS(IterationStatement_while)
	    IterationStatement_dowhile_eval, 0, 
	    PARSER_PRINT(IterationStatement_dowhile_print)
	    PARSER_VISIT(IterationStatement_while_visit)
	    IterationStatement_dowhile_isconst };


static void
IterationStatement_while_eval(na, context, res)
	struct node *na; /* (struct IterationStatement_while_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct IterationStatement_while_node *n = 
		CAST_NODE(na, IterationStatement_while);
	struct SEE_value *v, r2, r3, r4;

	v = NULL;
 step2: TRACE(na, context, SEE_TRACE_STATEMENT);
 	EVAL(n->cond, context, &r2);
	GetValue(context, &r2, &r3);
	SEE_ToBoolean(context->interpreter, &r3, &r4);
	if (!r4.u.boolean) {
	    _SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NULL);
	    return;
	}
	EVAL(n->body, context, res);
	if (res->u.completion.value)
		v = res->u.completion.value;
	if (res->u.completion.type == SEE_COMPLETION_CONTINUE &&
	    target_matches(&n->targetable, res->u.completion.target))
		goto step2;
	if (res->u.completion.type == SEE_COMPLETION_BREAK &&
	    target_matches(&n->targetable, res->u.completion.target))
	{
	    _SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NULL);
	    return;
	}
	if (res->u.completion.type != SEE_COMPLETION_NORMAL)
	    return;
	goto step2;
}

#if WITH_PARSER_PRINT
static void
IterationStatement_while_print(na, printer)
	struct node *na; /* (struct IterationStatement_while_node) */
	struct printer *printer;
{
	struct IterationStatement_while_node *n = 
		CAST_NODE(na, IterationStatement_while);

	PRINT_STRING(STR(while));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT(n->cond);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->body);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}
#endif

static int
IterationStatement_while_isconst(na, interp)
	struct node *na; /* (struct IterationStatement_while_node) */
	struct SEE_interpreter *interp;
{
	struct IterationStatement_while_node *n = 
		CAST_NODE(na, IterationStatement_while);
	if (ISCONST(n->cond, interp)) {
		struct SEE_value r1, r3;
		EVAL(n->cond, (struct SEE_context *)NULL, &r1);
		SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&r1) != SEE_REFERENCE);
		SEE_ToBoolean(interp, &r1, &r3);
		return r3.u.boolean ? ISCONST(n->body, interp) : 1;
	} else
		return 0;
}

static struct nodeclass IterationStatement_while_nodeclass
	= { BASECLASS
	    IterationStatement_while_eval, 0, 
	    PARSER_PRINT(IterationStatement_while_print)
	    PARSER_VISIT(IterationStatement_while_visit)
	    IterationStatement_while_isconst };


struct IterationStatement_for_node {
	struct Targetable_node targetable;
	struct node *init, *cond, *incr, *body;
};

/* 12.6.3 - "for (init; cond; incr) body" */
static void
IterationStatement_for_eval(na, context, res)
	struct node *na; /* (struct IterationStatement_for_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct IterationStatement_for_node *n = 
		CAST_NODE(na, IterationStatement_for);
	struct SEE_value *v, r2, r3, r6, r7, r8, r16, r17;

	if (n->init) {
	    TRACE(n->init, context, SEE_TRACE_STATEMENT);
	    EVAL(n->init, context, &r2);
	    GetValue(context, &r2, &r3);		/* r3 not used */
	}
	v = NULL;
 step5:	if (n->cond) {
	    TRACE(n->cond, context, SEE_TRACE_STATEMENT);
	    EVAL(n->cond, context, &r6);
	    GetValue(context, &r6, &r7);
	    SEE_ToBoolean(context->interpreter, &r7, &r8);
	    if (!r8.u.boolean) goto step19;
	} else
	    TRACE(na, context, SEE_TRACE_STATEMENT);
	EVAL(n->body, context, res);
	if (res->u.completion.value)
	    v = res->u.completion.value;
	if (res->u.completion.type == SEE_COMPLETION_BREAK &&
	    target_matches(&n->targetable, res->u.completion.target))
		goto step19;
	if (res->u.completion.type == SEE_COMPLETION_CONTINUE &&
	    target_matches(&n->targetable, res->u.completion.target))
		goto step15;
	if (res->u.completion.type != SEE_COMPLETION_NORMAL)
		return;
step15: if (n->incr) {
	    TRACE(n->incr, context, SEE_TRACE_STATEMENT);
	    EVAL(n->incr, context, &r16);
	    GetValue(context, &r16, &r17);	/* r17 not used */
	}
	goto step5;
step19:	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NULL);
}

#if WITH_PARSER_PRINT
static void
IterationStatement_for_print(na, printer)
	struct node *na; /* (struct IterationStatement_for_node) */
	struct printer *printer;
{
	struct IterationStatement_for_node *n = 
		CAST_NODE(na, IterationStatement_for);

	PRINT_STRING(STR(for));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	if (n->init) PRINT(n->init);
	PRINT_CHAR(';');
	PRINT_CHAR(' ');
	if (n->cond) PRINT(n->cond);
	PRINT_CHAR(';');
	PRINT_CHAR(' ');
	if (n->incr) PRINT(n->incr);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->body);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}
#endif

#if WITH_PARSER_VISIT
static void
IterationStatement_for_visit(na, v, va)
	struct node *na; /* (struct IterationStatement_for_node) */
	visitor_fn_t v;
	void *va;
{
	struct IterationStatement_for_node *n = 
		CAST_NODE(na, IterationStatement_for);

	if (n->init) VISIT(n->init, v, va);
	if (n->cond) VISIT(n->cond, v, va);
	if (n->incr) VISIT(n->incr, v, va);
	VISIT(n->body, v, va);
}
#endif

static int
IterationStatement_for_isconst(na, interp)
        struct node *na; /* (struct IterationStatement_for_node) */
	struct SEE_interpreter *interp;
{
        struct IterationStatement_for_node *n = 
		CAST_NODE(na, IterationStatement_for);

	if (n->cond && ISCONST(n->cond, interp)) {
		struct SEE_value r1, r3;
		EVAL(n->cond, (struct SEE_context *)NULL, &r1);
		SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&r1) != SEE_REFERENCE);
		SEE_ToBoolean(interp, &r1, &r3);
		if (r3.u.boolean)
			return 0;	/* Infinite loop */
		if (n->init && !ISCONST(n->init, interp))
			return 0;
		if (n->cond && !ISCONST(n->cond, interp))
			return 0;
		if (n->incr && !ISCONST(n->incr, interp))
			return 0;
		return ISCONST(n->body, interp);
	} else
		return 0;
}

static struct nodeclass IterationStatement_for_nodeclass
	= { BASECLASS
	    IterationStatement_for_eval, 0,
	    PARSER_PRINT(IterationStatement_for_print)
	    PARSER_VISIT(IterationStatement_for_visit)
	    IterationStatement_for_isconst };

/* 12.6.3 - "for (var init; cond; incr) body" */
static void
IterationStatement_forvar_eval(na, context, res)
	struct node *na; /* (struct IterationStatement_for_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct IterationStatement_for_node *n = 
		CAST_NODE(na, IterationStatement_for);
	struct SEE_value *v, r1, r4, r5, r6, r14, r15;

	TRACE(n->init, context, SEE_TRACE_STATEMENT);
	EVAL(n->init, context, &r1);
	v = NULL;
 step3: if (n->cond) {
	    TRACE(n->cond, context, SEE_TRACE_STATEMENT);
	    EVAL(n->cond, context, &r4);
	    GetValue(context, &r4, &r5);
	    SEE_ToBoolean(context->interpreter, &r5, &r6);
	    if (!r6.u.boolean) goto step17; /* XXX spec bug: says step 14 */
	} else
	    TRACE(na, context, SEE_TRACE_STATEMENT);
	EVAL(n->body, context, res);
	if (res->u.completion.value)
	    v = res->u.completion.value;
	if (res->u.completion.type == SEE_COMPLETION_BREAK &&
	    target_matches(&n->targetable, res->u.completion.target))
		goto step17;
	if (res->u.completion.type == SEE_COMPLETION_CONTINUE &&
	    target_matches(&n->targetable, res->u.completion.target))
		goto step13;
	if (res->u.completion.type != SEE_COMPLETION_NORMAL)
		return;
step13: if (n->incr) {
	    TRACE(n->incr, context, SEE_TRACE_STATEMENT);
	    EVAL(n->incr, context, &r14);
	    GetValue(context, &r14, &r15); 		/* value not used */
	}
	goto step3;
step17:	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NULL);
}

#if WITH_PARSER_PRINT
static void
IterationStatement_forvar_print(na, printer)
	struct node *na; /* (struct IterationStatement_for_node) */
	struct printer *printer;
{
	struct IterationStatement_for_node *n = 
		CAST_NODE(na, IterationStatement_for);
		
	PRINT_STRING(STR(for));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT_STRING(STR(var));
	PRINT_CHAR(' ');
	PRINT(n->init);
	PRINT_CHAR(';');
	PRINT_CHAR(' ');
	if (n->cond) PRINT(n->cond);
	PRINT_CHAR(';');
	PRINT_CHAR(' ');
	if (n->incr) PRINT(n->incr);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->body);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}
#endif

/* NB : the VarDecls of n->init are exposed through parser->vars */

static struct nodeclass IterationStatement_forvar_nodeclass
	= { SUPERCLASS(IterationStatement_for)
	    IterationStatement_forvar_eval, 0,
	    PARSER_PRINT(IterationStatement_forvar_print)
	    PARSER_VISIT(IterationStatement_for_visit)
	    IterationStatement_for_isconst };


struct IterationStatement_forin_node {
	struct Targetable_node targetable;
	struct node *lhs, *list, *body;
};

/* 12.6.3 - "for (lhs in list) body" */
static void
IterationStatement_forin_eval(na, context, res)
	struct node *na; /* (struct IterationStatement_forin_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct IterationStatement_forin_node *n = 
		CAST_NODE(na, IterationStatement_forin);
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_value *v, r1, r2, r3, r5, r6;
	struct SEE_string **props0, **props;

        TRACE(na, context, SEE_TRACE_STATEMENT);
	EVAL(n->list, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToObject(interp, &r2, &r3);
	v = NULL;
	for (props0 = props = SEE_enumerate(interp, r3.u.object); 
	     *props; 
	     props++)
	{
	    if (!SEE_OBJECT_HASPROPERTY(interp, r3.u.object, *props))
		    continue;	/* property was deleted! */
	    SEE_SET_STRING(&r5, *props);
	    EVAL(n->lhs, context, &r6);
	    PutValue(context, &r6, &r5);
	    EVAL(n->body, context, res);
	    if (res->u.completion.value)
		v = res->u.completion.value;
	    if (res->u.completion.type == SEE_COMPLETION_BREAK &&
		target_matches(&n->targetable, res->u.completion.target))
		    break;
	    if (res->u.completion.type == SEE_COMPLETION_CONTINUE &&
		target_matches(&n->targetable, res->u.completion.target))
		    continue;
	    if (res->u.completion.type != SEE_COMPLETION_NORMAL)
		    return;
	}
	SEE_enumerate_free(interp, props0);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NULL);
}

#if WITH_PARSER_PRINT
static void
IterationStatement_forin_print(na, printer)
	struct node *na; /* (struct IterationStatement_forin_node) */
	struct printer *printer;
{
	struct IterationStatement_forin_node *n = 
		CAST_NODE(na, IterationStatement_forin);

	PRINT_STRING(STR(for));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT(n->lhs);
	PRINT_STRING(STR(in));
	PRINT_CHAR(' ');
	PRINT(n->list);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->body);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}
#endif

#if WITH_PARSER_VISIT
static void
IterationStatement_forin_visit(na, v, va)
	struct node *na; /* (struct IterationStatement_forin_node) */
	visitor_fn_t v;
	void *va;
{
	struct IterationStatement_forin_node *n = 
		CAST_NODE(na, IterationStatement_forin);
	VISIT(n->lhs, v, va);
	VISIT(n->list, v, va);
	VISIT(n->body, v, va);
}
#endif

static struct nodeclass IterationStatement_forin_nodeclass
	= { SUPERCLASS(IterationStatement_forin)
	    IterationStatement_forin_eval, 0,
	    PARSER_PRINT(IterationStatement_forin_print)
	    PARSER_VISIT(IterationStatement_forin_visit)
	    0 };


static void
IterationStatement_forvarin_eval(na, context, res)
	struct node *na; /* (struct IterationStatement_forin_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct IterationStatement_forin_node *n = 
		CAST_NODE(na, IterationStatement_forin);
	struct SEE_interpreter *interp = context->interpreter;
	struct SEE_value *v, r2, r3, r4, r6, r7;
	struct SEE_string **props0, **props;
	struct VariableDeclaration_node *lhs 
		= CAST_NODE(n->lhs, VariableDeclaration);

	TRACE(na, context, SEE_TRACE_STATEMENT);
	EVAL(n->lhs, context, NULL);
	EVAL(n->list, context, &r2);
	GetValue(context, &r2, &r3);
	SEE_ToObject(interp, &r3, &r4);
	v = NULL;
	for (props0 = props = SEE_enumerate(interp, r4.u.object);
	     *props; 
	     props++)
	{
	    if (!SEE_OBJECT_HASPROPERTY(interp, r4.u.object, *props))
		    continue;	/* property was deleted! */
	    SEE_SET_STRING(&r6, *props);
	    SEE_scope_lookup(context->interpreter, context->scope, 
	    	lhs->var.name, &r7);
	    PutValue(context, &r7, &r6);
	    EVAL(n->body, context, res);
	    if (res->u.completion.value)
		v = res->u.completion.value;
	    if (res->u.completion.type == SEE_COMPLETION_BREAK &&
		target_matches(&n->targetable, res->u.completion.target))
		    break;
	    if (res->u.completion.type == SEE_COMPLETION_CONTINUE &&
		target_matches(&n->targetable, res->u.completion.target))
		    continue;
	    if (res->u.completion.type != SEE_COMPLETION_NORMAL)
		    return;
	}
	SEE_enumerate_free(interp, props0);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NULL);
}

#if WITH_PARSER_PRINT
static void
IterationStatement_forvarin_print(na, printer)
	struct node *na; /* (struct IterationStatement_forin_node) */
	struct printer *printer;
{
	struct IterationStatement_forin_node *n = 
		CAST_NODE(na, IterationStatement_forin);

	PRINT_STRING(STR(for));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT_STRING(STR(var));
	PRINT(n->lhs);
	PRINT_STRING(STR(in));
	PRINT_CHAR(' ');
	PRINT(n->list);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->body);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}
#endif

static struct nodeclass IterationStatement_forvarin_nodeclass
	= { SUPERCLASS(IterationStatement_forin)
	    IterationStatement_forvarin_eval, 0,
	    PARSER_PRINT(IterationStatement_forvarin_print)
	    PARSER_VISIT(IterationStatement_forin_visit)
	    0 };


static struct node *
IterationStatement_parse(parser)
	struct parser *parser;
{
	struct IterationStatement_while_node *w;
	struct IterationStatement_for_node *fn;
	struct IterationStatement_forin_node *fin;
	struct node *n;

	switch (NEXT) {
	case tDO:
		w = NEW_NODE(struct IterationStatement_while_node,
			&IterationStatement_dowhile_nodeclass);
		SKIP;
		target_init(&w->targetable, parser, CONTINUABLE);
		label_push(parser, EMPTY_LABEL, NULL, CONTINUABLE);
		w->body = PARSE(Statement);
		EXPECT(tWHILE);
		EXPECT('(');
		w->cond = PARSE(Expression);
		EXPECT(')');
		EXPECT_SEMICOLON;
		label_pop(parser);
		return (struct node *)w;
	case tWHILE:
		w = NEW_NODE(struct IterationStatement_while_node,
			&IterationStatement_while_nodeclass);
		SKIP;
		target_init(&w->targetable, parser, CONTINUABLE);
		label_push(parser, EMPTY_LABEL, NULL, CONTINUABLE);
		EXPECT('(');
		w->cond = PARSE(Expression);
		EXPECT(')');
		w->body = PARSE(Statement);
		label_pop(parser);
		return (struct node *)w;
	case tFOR:
		break;
	default:
		SEE_error_throw_string(
		    parser->interpreter,
		    parser->interpreter->Error,
		    STR(internal_error));
	}

	SKIP;		/* tFOR */
	EXPECT('(');

	if (NEXT == tVAR) {			 /* "for ( var" */
	    SKIP;	/* tVAR */
	    parser->noin = 1;
	    n = PARSE(VariableDeclarationList);	/* NB adds to parser->vars */
	    parser->noin = 0;
	    if (NEXT == tIN && 
		  n->nodeclass == &VariableDeclaration_nodeclass)
	    {					/* "for ( var VarDecl in" */
		fin = NEW_NODE(struct IterationStatement_forin_node,
		    &IterationStatement_forvarin_nodeclass);
		target_init(&fin->targetable, parser, CONTINUABLE);
		fin->lhs = n;
		SKIP;	/* tIN */
		fin->list = PARSE(Expression);
		EXPECT(')');
		label_push(parser, EMPTY_LABEL, NULL, CONTINUABLE);
		fin->body = PARSE(Statement);
		label_pop(parser);
		return (struct node *)fin;
	    }

	    /* Accurately describe possible tokens at this stage */
	    EXPECTX(';', 
	       (n->nodeclass == &VariableDeclaration_nodeclass
		  ? "';' or 'in'"
		  : "';'"));
					    /* "for ( var VarDeclList ;" */
	    fn = NEW_NODE(struct IterationStatement_for_node,
		&IterationStatement_forvar_nodeclass);
	    target_init(&fn->targetable, parser, CONTINUABLE);
	    label_push(parser, EMPTY_LABEL, NULL, CONTINUABLE);
	    fn->init = n;
	    if (NEXT != ';')
		fn->cond = PARSE(Expression);
	    else
		fn->cond = NULL;
	    EXPECT(';');
	    if (NEXT != ')')
		fn->incr = PARSE(Expression);
	    else
		fn->incr = NULL;
	    EXPECT(')');
	    fn->body = PARSE(Statement);
	    label_pop(parser);
	    return (struct node *)fn;
	}

	if (NEXT != ';') {
	    parser->noin = 1;
	    n = PARSE(Expression);
	    parser->noin = 0;
	    if (NEXT == tIN && parser->is_lhs) {   /* "for ( lhs in" */
		fin = NEW_NODE(struct IterationStatement_forin_node,
		    &IterationStatement_forin_nodeclass);
		target_init(&fin->targetable, parser, CONTINUABLE);
		label_push(parser, EMPTY_LABEL, NULL, CONTINUABLE);
		fin->lhs = n;
		SKIP;		/* tIN */
		fin->list = PARSE(Expression);
		EXPECT(')');
		fin->body = PARSE(Statement);
		label_pop(parser);
		return (struct node *)fin;
	    }
	} else
	    n = NULL;				/* "for ( ;" */

	fn = NEW_NODE(struct IterationStatement_for_node,
	    &IterationStatement_for_nodeclass);
	target_init(&fn->targetable, parser, CONTINUABLE);
	label_push(parser, EMPTY_LABEL, NULL, CONTINUABLE);
	fn->init = n;
	EXPECT(';');
	if (NEXT != ';')
	    fn->cond = PARSE(Expression);
	else
	    fn->cond = NULL;
	EXPECT(';');
	if (NEXT != ')')
	    fn->incr = PARSE(Expression);
	else
	    fn->incr = NULL;
	EXPECT(')');
	fn->body = PARSE(Statement);
	label_pop(parser);
	return (struct node *)fn;
}

/*
 *	-- 12.7
 *
 *	ContinueStatement
 *	:	tCONTINUE ';'
 *	|	tCONTINUE tIDENT ';'
 *	;
 */

struct ContinueStatement_node {
	struct node node;
	void *target;
};

static void
ContinueStatement_eval(na, context, res)
	struct node *na; /* (struct ContinueStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct ContinueStatement_node *n = CAST_NODE(na, ContinueStatement);

	TRACE(na, context, SEE_TRACE_STATEMENT);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_CONTINUE, NULL, n->target);
}

#if WITH_PARSER_PRINT
static void
ContinueStatement_print(na, printer)
	struct node *na; /* (struct ContinueStatement_node) */
	struct printer *printer;
{
	struct ContinueStatement_node *n = CAST_NODE(na, ContinueStatement);

	PRINT_STRING(STR(continue));
	PRINT_CHAR(' ');
	PRINT_CHAR('L');
	print_hex(printer, (int)n->target);
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}
#endif

static struct nodeclass ContinueStatement_nodeclass
	= { BASECLASS
	    ContinueStatement_eval, 0,
	    PARSER_PRINT(ContinueStatement_print)
	    PARSER_VISIT(0)
	    0
	    };


static struct node *
ContinueStatement_parse(parser)
	struct parser *parser;
{
	struct ContinueStatement_node *cn;
	struct label *label = NULL;

	cn = NEW_NODE(struct ContinueStatement_node,
		&ContinueStatement_nodeclass);
	EXPECT(tCONTINUE);
	if (NEXT_IS_SEMICOLON)
	    label = label_lookup(parser, EMPTY_LABEL, tCONTINUE);
	else {
	    if (NEXT == tIDENT)
		label = label_lookup(parser, NEXT_VALUE->u.string, tCONTINUE);
	    EXPECT(tIDENT);
	}
	EXPECT_SEMICOLON;
	cn->target = label_get_target(label);
	return (struct node *)cn;
}

/*
 *	-- 12.8
 *
 *	BreakStatement
 *	:	tBREAK ';'
 *	|	tBREAK tIDENT ';'
 *	;
 */

struct BreakStatement_node {
	struct node node;
	void *target;
};

static void
BreakStatement_eval(na, context, res)
	struct node *na; /* (struct BreakStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct BreakStatement_node *n = CAST_NODE(na, BreakStatement);

	TRACE(na, context, SEE_TRACE_STATEMENT);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_BREAK, NULL, n->target);
}

#if WITH_PARSER_PRINT
static void
BreakStatement_print(na, printer)
	struct node *na; /* (struct BreakStatement_node) */
	struct printer *printer;
{
	struct BreakStatement_node *n = CAST_NODE(na, BreakStatement);
	PRINT_STRING(STR(break));
	PRINT_CHAR(' ');
	PRINT_CHAR('L');
	print_hex(printer, (int)n->target);
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}
#endif

static struct nodeclass BreakStatement_nodeclass
	= { BASECLASS
	    BreakStatement_eval, 0,
	    PARSER_PRINT(BreakStatement_print)
	    PARSER_VISIT(0)
	    0 };

static struct node *
BreakStatement_parse(parser)
	struct parser *parser;
{
	struct BreakStatement_node *cn;
	struct label *label = NULL;

	cn = NEW_NODE(struct BreakStatement_node,
		&BreakStatement_nodeclass);
	EXPECT(tBREAK);
	if (NEXT_IS_SEMICOLON)
	    label = label_lookup(parser, EMPTY_LABEL, tBREAK);
	else {
	    if (NEXT == tIDENT)
		label = label_lookup(parser, NEXT_VALUE->u.string, tBREAK);
	    EXPECT(tIDENT);
	}
	EXPECT_SEMICOLON;
	cn->target = label_get_target(label);
	return (struct node *)cn;
}

/*
 *	-- 12.9
 *
 *	ReturnStatement
 *	:	tRETURN ';'
 *	|	tRETURN Expression ';'
 *	;
 */

struct ReturnStatement_node {
	struct node node;
	struct node *expr;
	void *target;
};

static void
ReturnStatement_eval(na, context, res)
	struct node *na; /* (struct ReturnStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct ReturnStatement_node *n = CAST_NODE(na, ReturnStatement);
	struct SEE_value r2, *v;

	TRACE(na, context, SEE_TRACE_STATEMENT);
	EVAL(n->expr, context, &r2);
	v = SEE_NEW(context->interpreter, struct SEE_value);
	GetValue(context, &r2, v);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_RETURN, v, NULL);
}

#if WITH_PARSER_PRINT
static void
ReturnStatement_print(na, printer)
	struct node *na; /* (struct ReturnStatement_node) */
	struct printer *printer;
{
	struct ReturnStatement_node *n = CAST_NODE(na, ReturnStatement);
	PRINT_STRING(STR(return));
	PRINT_CHAR(' ');
	PRINT(n->expr);
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}
#endif

#if WITH_PARSER_VISIT
static void
ReturnStatement_visit(na, v, va)
	struct node *na; /* (struct ReturnStatement_node) */
	visitor_fn_t v;
	void *va;
{
	struct ReturnStatement_node *n = CAST_NODE(na, ReturnStatement);
	VISIT(n->expr, v, va);
}
#endif

static struct nodeclass ReturnStatement_nodeclass
	= { BASECLASS
	    ReturnStatement_eval, 0,
	    PARSER_PRINT(ReturnStatement_print)
	    PARSER_VISIT(ReturnStatement_visit)
	    0 };


static void
ReturnStatement_undef_eval(na, context, res)
	struct node *na; /* (struct ReturnStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	static struct SEE_value undef = { SEE_UNDEFINED };

	TRACE(na, context, SEE_TRACE_STATEMENT);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_RETURN, &undef, NULL);
}

#if WITH_PARSER_PRINT
static void
ReturnStatement_undef_print(na, printer)
	struct node *na; /* (struct ReturnStatement_node) */
	struct printer *printer;
{
	PRINT_STRING(STR(return));
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}
#endif

static struct nodeclass ReturnStatement_undef_nodeclass
	= { BASECLASS
	    ReturnStatement_undef_eval, 0,
	    PARSER_PRINT(ReturnStatement_undef_print)
	    PARSER_VISIT(0)
	    0 };


static struct node *
ReturnStatement_parse(parser)
	struct parser *parser;
{
	struct ReturnStatement_node *rn;

	rn = NEW_NODE(struct ReturnStatement_node,
		    &ReturnStatement_undef_nodeclass);
	EXPECT(tRETURN);
	if (!parser->funcdepth)
		ERRORm("'return' statement not inside function");
	if (!NEXT_IS_SEMICOLON) {
	    rn->node.nodeclass = &ReturnStatement_nodeclass;
	    rn->expr = PARSE(Expression);
	}
	EXPECT_SEMICOLON;
	return (struct node *)rn;
}

/*
 *	-- 12.10
 *
 *	WithStatement
 *	:	tWITH '(' Expression ')' Statement
 *	;
 */

static void
WithStatement_eval(na, context, res)
	struct node *na; /* (struct Binary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	SEE_try_context_t ctxt;
	struct SEE_value r1, r2, r3;
	struct SEE_scope *s;

	TRACE(na, context, SEE_TRACE_STATEMENT);
	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);
	SEE_ToObject(context->interpreter, &r2, &r3);

	/* Insert r3 in front of current scope chain */
	s = SEE_NEW(context->interpreter, struct SEE_scope);
	s->obj = r3.u.object;
	s->next = context->scope;
	context->scope = s;
	SEE_TRY(context->interpreter, ctxt)
	    EVAL(n->b, context, res);
	context->scope = context->scope->next;
	SEE_DEFAULT_CATCH(context->interpreter, ctxt);
}

#if WITH_PARSER_PRINT
static void
WithStatement_print(na, printer)
	struct node *na; /* (struct Binary_node) */
	struct printer *printer;
{
	struct Binary_node *n = CAST_NODE(na, Binary);
	PRINT_STRING(STR(with));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT(n->a);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->b);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}
#endif

static struct nodeclass WithStatement_nodeclass
	= { SUPERCLASS(Binary)
	    WithStatement_eval, 0,
	    PARSER_PRINT(WithStatement_print)
	    PARSER_VISIT(Binary_visit)
	    0 };


static struct node *
WithStatement_parse(parser)
	struct parser *parser;
{
	struct Binary_node *n;

	n = NEW_NODE(struct Binary_node, &WithStatement_nodeclass);
	EXPECT(tWITH);
	EXPECT('(');
	n->a = PARSE(Expression);
	EXPECT(')');
	n->b = PARSE(Statement);
	return (struct node *)n;
}

/*
 *	-- 12.11
 *
 *	SwitchStatement
 *	:	tSWITCH '(' Expression ')' CaseBlock
 *	;
 *
 *	CaseBlock
 *	:	'{' '}'
 *	|	'{' CaseClauses '}'
 *	|	'{' DefaultClause '}'
 *	|	'{' CaseClauses DefaultClause '}'
 *	|	'{' DefaultClause '}'
 *	|	'{' CaseClauses DefaultClause CaseClauses '}'
 *	;
 *
 *	CaseClauses
 *	:	CaseClause
 *	|	CaseClauses CaseClause
 *	;
 *
 *	CaseClause
 *	:	tCASE Expression ':'
 *	|	tCASE Expression ':' StatementList
 *	;
 *
 *	DefaultClause
 *	:	tDEFAULT ':'
 *	|	tDEFAULT ':' StatementList
 *	;
 */

struct SwitchStatement_node {
	struct Targetable_node targetable;
	struct node *cond;
	struct case_list {
		struct node *expr;	/* NULL for default case */
		struct node *body;
		struct case_list *next;
	} *cases, *defcase;
};

static void
SwitchStatement_caseblock(n, context, input, res)
	struct SwitchStatement_node *n;
	struct SEE_context *context;
	struct SEE_value *input, *res;
{
	struct case_list *c;
	struct SEE_value cc1, cc2, cc3;

	/*
	 * Note, this should be functionally equivalent
	 * to the standard. We search through the in-order
	 * case statements to find an expression that is
	 * strictly equal to 'input', and then run all
	 * the statements from there till we break or reach
	 * the end. If no expression matches, we start at the
	 * default case, if one exists.
	 */
	for (c = n->cases; c; c = c->next) {
	    if (!c->expr) continue;
	    EVAL(c->expr, context, &cc1);
	    GetValue(context, &cc1, &cc2);
	    EqualityExpression_seq(context, input, &cc2, &cc3);
	    if (cc3.u.boolean)
		break;
	}
	if (!c)
	    c = n->defcase;	/* can be NULL, meaning no default */
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, NULL, NULL);
	for (; c; c = c->next) {
	    if (c->body)
		EVAL(c->body, context, res);
	    if (res->u.completion.type != SEE_COMPLETION_NORMAL)
		break;
	}
}

static void
SwitchStatement_eval(na, context, res)
	struct node *na; /* (struct SwitchStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct SwitchStatement_node *n = CAST_NODE(na, SwitchStatement);
	struct SEE_value *v, r1, r2;

	TRACE(na, context, SEE_TRACE_STATEMENT);
	EVAL(n->cond, context, &r1);
	GetValue(context, &r1, &r2);
	SwitchStatement_caseblock(n, context, &r2, res);
	if (res->u.completion.type == SEE_COMPLETION_BREAK &&
	    target_matches(&n->targetable, res->u.completion.target))
	{
		v = res->u.completion.value;
		_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, v, NULL);
	}
}

#if WITH_PARSER_PRINT
static void
SwitchStatement_print(na, printer)
	struct node *na; /* (struct SwitchStatement_node) */
	struct printer *printer;
{
	struct SwitchStatement_node *n = CAST_NODE(na, SwitchStatement);
	struct case_list *c;

	PRINT_STRING(STR(switch));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT(n->cond);
	PRINT_CHAR(')');
	PRINT_CHAR(' ');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);

	for (c = n->cases; c; c = c->next) {
		if (c == n->defcase) {
			PRINT_STRING(STR(default));
			PRINT_CHAR(':');
			PRINT_NEWLINE(0);
		}
		if (c->expr) {
			PRINT_STRING(STR(case));
			PRINT_CHAR(' ');
			PRINT(c->expr);
			PRINT_CHAR(':');
			PRINT_NEWLINE(0);
		}
		PRINT_NEWLINE(1);
		PRINT(c->body);
		PRINT_NEWLINE(-1);
	}

	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
	PRINT_NEWLINE(0);
}
#endif

#if WITH_PARSER_VISIT
static void
SwitchStatement_visit(na, v, va)
	struct node *na; /* (struct SwitchStatement_node) */
	visitor_fn_t v;
	void *va;
{
	struct SwitchStatement_node *n = CAST_NODE(na, SwitchStatement);
	struct case_list *c;

	VISIT(n->cond, v, va);
	for (c = n->cases; c; c = c->next) {
		if (c->expr) 
		    VISIT(c->expr, v, va);
		VISIT(c->body, v, va);
	}
}
#endif

static struct nodeclass SwitchStatement_nodeclass
	= { BASECLASS
	    SwitchStatement_eval, 0,
	    PARSER_PRINT(SwitchStatement_print)
	    PARSER_VISIT(SwitchStatement_visit)
	    0 };


static struct node *
SwitchStatement_parse(parser)
	struct parser *parser;
{
	struct SwitchStatement_node *n;
	struct case_list **cp, *c;
	int next;

	n = NEW_NODE(struct SwitchStatement_node,
		&SwitchStatement_nodeclass);
	target_init(&n->targetable, parser, 0);
	EXPECT(tSWITCH);
	label_push(parser, EMPTY_LABEL, NULL, 0);
	EXPECT('(');
	n->cond = PARSE(Expression);
	EXPECT(')');
	EXPECT('{');
	cp = &n->cases;
	n->defcase = NULL;
	while (NEXT != '}') {
	    c = SEE_NEW(parser->interpreter, struct case_list);
	    *cp = c;
	    cp = &c->next;
	    switch (NEXT) {
	    case tCASE:
		SKIP;
		c->expr = PARSE(Expression);
		break;
	    case tDEFAULT:
		SKIP;
		c->expr = NULL;
		if (n->defcase)
		    ERRORm("duplicate 'default' clause");
		n->defcase = c;
		break;
	    default:
		EXPECTED("'}', 'case' or 'default'");
	    }
	    EXPECT(':');
	    next = NEXT;
	    if (next != '}' && next != tDEFAULT && next != tCASE)
		c->body = PARSE(StatementList);
	    else
		c->body = NULL;
	}
	*cp = NULL;
	EXPECT('}');
	label_pop(parser);
	return (struct node *)n;
}

/*
 *	-- 12.12
 *
 *	LabelledStatement
 *	:	tIDENT
 *	|	Statement
 *	;
 */

struct LabelledStatement_node {
	struct Unary_node unary;
	struct SEE_string *labelset;
};

static void
LabelledStatement_eval(na, context, res)
	struct node *na; /* (struct LabelledStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct LabelledStatement_node *n = CAST_NODE(na, LabelledStatement);

	EVAL(n->unary.a, context, res);
	if (res->u.completion.type == SEE_COMPLETION_BREAK &&
		res->u.completion.target == n->labelset)
	{
	    res->u.completion.type = SEE_COMPLETION_NORMAL;
	    res->u.completion.target = NULL;
	}
}

#if WITH_PARSER_PRINT
static void
LabelledStatement_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct LabelledStatement_node *n = CAST_NODE(na, LabelledStatement);
	PRINT_STRING(n->labelset);
	PRINT_CHAR(':');
	PRINT(n->unary.a);
}
#endif

static struct nodeclass LabelledStatement_nodeclass
	= { SUPERCLASS(Unary)
	    LabelledStatement_eval, 0, 
	    PARSER_PRINT(LabelledStatement_print)
	    PARSER_VISIT(Unary_visit)
	    0 };

static struct node *
LabelledStatement_parse(parser)
	struct parser *parser;
{
	struct LabelledStatement_node *n;
	struct SEE_string *label, *labelset = NULL;
	unsigned int label_count = 0;

	n = NEW_NODE(struct LabelledStatement_node, 
			&LabelledStatement_nodeclass);
	do {
		/* Lookahead is IDENT ':' */
		label = NEXT_VALUE->u.string;
		if (!labelset)
			labelset = label;
		label_push(parser, label, labelset, 0);
		label_count++;
		EXPECT(tIDENT);
		EXPECT(':');
	} while (NEXT == tIDENT && lookahead(parser, 1) == ':');
	n->labelset = labelset;

	switch (NEXT) {
	case tDO:
	case tWHILE:
	case tFOR:
		parser->current_labelset = labelset;
		n->unary.a = PARSE(IterationStatement);
		break;
	case tSWITCH:
		parser->current_labelset = labelset;
		n->unary.a = PARSE(SwitchStatement);
		break;
	default:
		n->unary.a = PARSE(Statement);
	}

	while (label_count--)
		label_pop(parser);
	return (struct node *)n;
}

/*
 *	-- 12.13
 *
 *	ThrowStatement
 *	:	tTHROW Expression ';'
 *	;
 */

static void
ThrowStatement_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	struct SEE_value r1, r2;

	TRACE(na, context, SEE_TRACE_STATEMENT);
	EVAL(n->a, context, &r1);
	GetValue(context, &r1, &r2);

	TRACE(na, context, SEE_TRACE_THROW);
	SEE_THROW(context->interpreter, &r2);

	/* NOTREACHED */
}

#if WITH_PARSER_PRINT
static void
ThrowStatement_print(na, printer)
	struct node *na; /* (struct Unary_node) */
	struct printer *printer;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	PRINT_STRING(STR(throw));
	PRINT_CHAR(' ');
	PRINT(n->a);
	PRINT_CHAR(';');
	PRINT_NEWLINE(0);
}
#endif

static struct nodeclass ThrowStatement_nodeclass
	= { SUPERCLASS(Unary)
	    ThrowStatement_eval, 0, 
	    PARSER_PRINT(ThrowStatement_print)
	    PARSER_VISIT(Unary_visit)
	    0 };

static struct node *
ThrowStatement_parse(parser)
	struct parser *parser;
{
	struct Unary_node *n;

	n = NEW_NODE(struct Unary_node, &ThrowStatement_nodeclass);
	EXPECT(tTHROW);
	if (NEXT_FOLLOWS_NL)
		ERRORm("newline not allowed after 'throw'");
	n->a = PARSE(Expression);
	EXPECT_SEMICOLON;
	return (struct node *)n;
}

/*
 *	-- 12.14
 *
 *	TryStatement
 *	:	tTRY Block Catch
 *	|	tTRY Block Finally
 *	|	tTRY Block Catch Finally
 *	;
 *
 *	Catch
 *	:	tCATCH '(' tIDENT ')' Block
 *	;
 *
 *	Finally
 *	:	tFINALLY Block
 *	;
 */

struct TryStatement_node {
	struct node node;
	struct node *block, *bcatch, *bfinally;
	struct SEE_string *ident;
};

static struct nodeclass TryStatement_nodeclass
	= { BASECLASS
	    0, 0, 
	    PARSER_PRINT(0)
	    PARSER_VISIT(0)
	    0 };

/*
 * Helper function to evaluate the catch clause in a new scope.
 * Weill return a SEE_COMPLETION_THROW completion instead of directly throwing
 * an exception.
 */
static void
TryStatement_catch(n, context, C, res)
	struct TryStatement_node *n;
	struct SEE_context *context;
	struct SEE_value *C, *res;
{
	struct SEE_value *tcp;
	struct SEE_object *r2;
	SEE_try_context_t ctxt;
	struct SEE_scope *s;
	struct SEE_interpreter *interp = context->interpreter;

	r2 = SEE_Object_new(interp);
	SEE_OBJECT_PUT(interp, r2, n->ident, C, SEE_ATTR_DONTDELETE);
	s = SEE_NEW(interp, struct SEE_scope);
	s->obj = r2;
	s->next = context->scope;
	context->scope = s;
	SEE_TRY(interp, ctxt)
	    EVAL(n->bcatch, context, res);
	context->scope = context->scope->next;
	if (SEE_CAUGHT(ctxt)) {
	    tcp = SEE_NEW(interp, struct SEE_value);
	    SEE_VALUE_COPY(tcp, SEE_CAUGHT(ctxt));
	    _SEE_SET_COMPLETION(res, SEE_COMPLETION_THROW, tcp, NULL);
	}
}

static void
TryStatement_catch_eval(na, context, res)
	struct node *na; /* (struct TryStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	SEE_try_context_t ctxt;

	TRACE(na, context, SEE_TRACE_STATEMENT);
	SEE_TRY(context->interpreter, ctxt)
	    EVAL(n->block, context, res);
	if (SEE_CAUGHT(ctxt))
	    TryStatement_catch(n, context, SEE_CAUGHT(ctxt), res);
	if (res->u.completion.type == SEE_COMPLETION_THROW) {
	    TRACE(na, context, SEE_TRACE_THROW);
	    SEE_THROW(context->interpreter, res->u.completion.value);
	}
}

#if WITH_PARSER_PRINT
static void
TryStatement_catch_print(na, printer)
	struct node *na; /* (struct TryStatement_node) */
	struct printer *printer;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	PRINT_STRING(STR(try));
	PRINT_NEWLINE(1);
	PRINT(n->block);
	PRINT_NEWLINE(-1);
	PRINT_STRING(STR(catch));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT_STRING(n->ident);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->bcatch);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}
#endif

#if WITH_PARSER_VISIT
static void
TryStatement_catch_visit(na, v, va)
	struct node *na; /* (struct TryStatement_node) */
	visitor_fn_t v;
	void *va;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	VISIT(n->block, v, va);
	VISIT(n->bcatch, v, va);
}
#endif

static struct nodeclass TryStatement_catch_nodeclass
	= { SUPERCLASS(TryStatement)
	    TryStatement_catch_eval, 0, 
	    PARSER_PRINT(TryStatement_catch_print)
	    PARSER_VISIT(TryStatement_catch_visit)
	    0 };


static void
TryStatement_finally_eval(na, context, res)
	struct node *na; /* (struct TryStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	struct SEE_value r2;
	SEE_try_context_t ctxt;

	TRACE(na, context, SEE_TRACE_STATEMENT);
	SEE_TRY(context->interpreter, ctxt)
	    EVAL(n->block, context, res);
	if (SEE_CAUGHT(ctxt))
	    _SEE_SET_COMPLETION(res, SEE_COMPLETION_THROW, 
	    	SEE_CAUGHT(ctxt), NULL);
	EVAL(n->bfinally, context, &r2);
	if (r2.u.completion.type != SEE_COMPLETION_NORMAL)
	    SEE_VALUE_COPY(res, &r2); 		/* break, return etc */
	if (res->u.completion.type == SEE_COMPLETION_THROW) {
	    TRACE(na, context, SEE_TRACE_THROW);
	    SEE_THROW(context->interpreter, res->u.completion.value);
	}
}

#if WITH_PARSER_PRINT
static void
TryStatement_finally_print(na, printer)
	struct node *na; /* (struct TryStatement_node) */
	struct printer *printer;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	PRINT_STRING(STR(try));
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->block);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
	PRINT_STRING(STR(finally));
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->bfinally);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}
#endif

#if WITH_PARSER_VISIT
static void
TryStatement_finally_visit(na, v, va)
	struct node *na; /* (struct TryStatement_node) */
	visitor_fn_t v;
	void *va;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	VISIT(n->block, v, va);
	VISIT(n->bfinally, v, va);
}
#endif

static struct nodeclass TryStatement_finally_nodeclass
	= { SUPERCLASS(TryStatement)
	    TryStatement_finally_eval, 0, 
	    PARSER_PRINT(TryStatement_finally_print)
	    PARSER_VISIT(TryStatement_finally_visit)
	    0 };


static void
TryStatement_catchfinally_eval(na, context, res)
	struct node *na; /* (struct TryStatement_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	struct SEE_value r1, r4, r6, *C = NULL, *retv;
	SEE_try_context_t ctxt, ctxt2;
	struct SEE_interpreter *interp = context->interpreter;

	TRACE(na, context, SEE_TRACE_STATEMENT);
	SEE_TRY(interp, ctxt)
/*1*/	    EVAL(n->block, context, &r1);
	if (SEE_CAUGHT(ctxt)) 
	    _SEE_SET_COMPLETION(&r1, SEE_COMPLETION_THROW, 
	        SEE_CAUGHT(ctxt), NULL);

/*2*/	C = &r1;
	SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(C) == SEE_COMPLETION);

/*3*/	if (C->u.completion.type == SEE_COMPLETION_THROW) {
/*4*/	    TryStatement_catch(n, context, C->u.completion.value, &r4);
/*5*/	    if (r4.u.completion.type != SEE_COMPLETION_NORMAL) {
		C = &r4;
	    }
	}

	SEE_TRY(interp, ctxt2)
/*6*/	    EVAL(n->bfinally, context, &r6);
	if (SEE_CAUGHT(ctxt2))
	    _SEE_SET_COMPLETION(&r6, SEE_COMPLETION_THROW, 
	    	SEE_CAUGHT(ctxt2), NULL);

	if (r6.u.completion.type != SEE_COMPLETION_NORMAL)
		retv = C;
	else
		retv = &r6;
	SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(retv) == SEE_COMPLETION);
		
	if (retv->u.completion.type == SEE_COMPLETION_THROW) {
	    TRACE(na, context, SEE_TRACE_THROW);
	    SEE_THROW(interp, retv->u.completion.value);
	} else 
	    SEE_VALUE_COPY(res, retv);
}

#if WITH_PARSER_PRINT
static void
TryStatement_catchfinally_print(na, printer)
	struct node *na; /* (struct TryStatement_node) */
	struct printer *printer;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	PRINT_STRING(STR(try));
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->block);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
	PRINT_STRING(STR(catch));
	PRINT_CHAR(' ');
	PRINT_CHAR('(');
	PRINT_STRING(n->ident);
	PRINT_CHAR(')');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->bcatch);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
	PRINT_STRING(STR(finally));
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT(n->bfinally);
	PRINT_CHAR('}');
	PRINT_NEWLINE(-1);
}
#endif

#if WITH_PARSER_VISIT
static void
TryStatement_catchfinally_visit(na, v, va)
	struct node *na; /* (struct TryStatement_node) */
	visitor_fn_t v;
	void *va;
{
	struct TryStatement_node *n = CAST_NODE(na, TryStatement);
	VISIT(n->block, v, va);
	VISIT(n->bcatch, v, va);
	VISIT(n->bfinally, v, va);
}
#endif

static struct nodeclass TryStatement_catchfinally_nodeclass
	= { SUPERCLASS(TryStatement)
	    TryStatement_catchfinally_eval, 0, 
	    PARSER_PRINT(TryStatement_catchfinally_print)
	    PARSER_VISIT(TryStatement_catchfinally_visit)
	    0 };


static struct node *
TryStatement_parse(parser)
	struct parser *parser;
{
	struct TryStatement_node *n;
	struct nodeclass *nc;

	n = NEW_NODE(struct TryStatement_node, NULL);
	EXPECT(tTRY);
	n->block = PARSE(Block);
	if (NEXT == tCATCH) {
	    SKIP;
	    EXPECT('(');
	    if (NEXT == tIDENT)
		    n->ident = NEXT_VALUE->u.string;
	    EXPECT(tIDENT);
	    EXPECT(')');
	    n->bcatch = PARSE(Block);
	} else
	    n->bcatch = NULL;

	if (NEXT == tFINALLY) {
	    SKIP;
	    n->bfinally = PARSE(Block);
	} else
	    n->bfinally = NULL;

	if (n->bcatch && n->bfinally)
		nc = &TryStatement_catchfinally_nodeclass;
	else if (n->bcatch)
		nc = &TryStatement_catch_nodeclass;
	else if (n->bfinally)
		nc = &TryStatement_finally_nodeclass;
	else
		ERRORm("expected 'catch' or 'finally'");
	n->node.nodeclass = nc;

	return (struct node *)n;
}

/*
 *	-- 13
 *
 *	FunctionDeclaration
 *	:	tFUNCTION tIDENT '( ')' '{' FunctionBody '}'
 *	|	tFUNCTION tIDENT '( FormalParameterList ')' 
 *			'{' FunctionBody '}'
 *	;
 *
 *	FunctionExpression
 *	:	tFUNCTION '( ')' '{' FunctionBody '}'
 *	|	tFUNCTION '( FormalParameterList ')' '{' FunctionBody '}'
 *	|	tFUNCTION tIDENT '( ')' '{' FunctionBody '}'
 *	|	tFUNCTION tIDENT '( FormalParameterList ')' 
 *			'{' FunctionBody '}'
 *	;
 *
 *	FormalParameterList
 *	:	tIDENT
 *	|	FormalParameterList ',' tIDENT
 *	;
 *
 *	FunctionBody
 *	:	SourceElements
 *	;
 */

struct Function_node {
	struct node node;
	struct function *function;
};

#if 0
/* This is never called, but defined in the spec. (Spec bug?) */
static void
FunctionDeclaration_eval(na, context, res)
	struct node *na; /* (struct Function_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Function_node *n = CAST_NODE(na, Function);
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, NULL, NULL); /* 14 */
}
#endif

#if WITH_PARSER_PRINT
static void
Function_print(na, printer)
	struct node *na; /* (struct Function_node) */
	struct printer *printer;
{
	struct Function_node *n = CAST_NODE(na, Function);
	int i;

	PRINT_STRING(STR(function));
	PRINT_CHAR(' ');
	if (n->function->name) {
	    PRINT_STRING(n->function->name);
	    PRINT_CHAR(' ');
	}
	PRINT_CHAR('(');
	for (i = 0; i < n->function->nparams; i++) {
	    if (i) {
		PRINT_CHAR(',');
		PRINT_CHAR(' ');
	    }
	    PRINT_STRING(n->function->params[i]);
	}
	PRINT_CHAR(')');
	PRINT_CHAR(' ');
	PRINT_CHAR('{');
	PRINT_NEWLINE(1);
	PRINT((struct node *)n->function->body);
	PRINT_NEWLINE(-1);
	PRINT_CHAR('}');
	PRINT_NEWLINE(0);
}
#endif

#if WITH_PARSER_VISIT
static void
Function_visit(na, v, va)
	struct node *na; /* (struct Function_node) */
	visitor_fn_t v;
	void *va;
{
	struct Function_node *n = CAST_NODE(na, Function);
	VISIT((struct node *)n->function->body, v, va);
}
#endif

static struct nodeclass Function_nodeclass
	= { BASECLASS
	    0, 0, 
	    PARSER_PRINT(Function_print)
	    PARSER_VISIT(Function_visit)
	    0 };

static void
FunctionDeclaration_fproc(na, context)
	struct node *na; /* struct Function_node */
	struct SEE_context *context;
{
	struct Function_node *n = CAST_NODE(na, Function);
	struct SEE_object *funcobj;
	struct SEE_value   funcval;

	/* 10.1.3 */
	funcobj = SEE_function_inst_create(context->interpreter,
	    n->function, context->scope);
	SEE_SET_OBJECT(&funcval, funcobj);
	SEE_OBJECT_PUT(context->interpreter, context->variable,
	    n->function->name, &funcval, context->varattr);
}

static struct nodeclass FunctionDeclaration_nodeclass
	= { SUPERCLASS(Function)
	    0 /* FunctionDeclaration_eval */,
	    FunctionDeclaration_fproc,
	    PARSER_PRINT(Function_print)
	    PARSER_VISIT(Function_visit)
	    0 };

static struct node *
FunctionDeclaration_parse(parser)
	struct parser *parser;
{
	struct Function_node *n;
	struct node *body;
	struct var *formal;
	struct SEE_string *name = NULL;

	n = NEW_NODE(struct Function_node, &FunctionDeclaration_nodeclass);
	EXPECT(tFUNCTION);

	if (NEXT == tIDENT)
		name = NEXT_VALUE->u.string;
	EXPECT(tIDENT);

	EXPECT('(');
	formal = PARSE(FormalParameterList);
	EXPECT(')');

	EXPECT('{');
	parser->funcdepth++;
	body = PARSE(FunctionBody);
	parser->funcdepth--;
	EXPECT('}');

	n->function = SEE_function_make(parser->interpreter, 
		name, formal, body);

	return (struct node *)n;
}


static void
FunctionExpression_eval(na, context, res)
	struct node *na; /* (struct Function_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Function_node *n = CAST_NODE(na, Function);
	struct SEE_object *funcobj = NULL, *obj;
	struct SEE_value   v;
	struct SEE_scope  *scope;
	SEE_try_context_t  ctxt;
	struct SEE_interpreter *interp = context->interpreter;

	if (n->function->name == NULL) {
	    funcobj = SEE_function_inst_create(interp,
	        n->function, context->scope);
            SEE_SET_OBJECT(res, funcobj);
	} else {
	    /*
	     * Construct a single scope step that lets the
	     * function call itself recursively
	     */
	    obj = SEE_Object_new(interp);

	    scope = SEE_NEW(interp, struct SEE_scope);
	    scope->obj = obj;
	    scope->next = context->scope;
	    context->scope = scope;

	    /* Be careful to restore the scope on any exception! */
	    SEE_TRY(interp, ctxt) {
	        funcobj = SEE_function_inst_create(interp,
	            n->function, context->scope);
	        SEE_SET_OBJECT(&v, funcobj);
	        SEE_OBJECT_PUT(interp, obj, n->function->name, &v,
		    SEE_ATTR_DONTDELETE | SEE_ATTR_READONLY);
                SEE_SET_OBJECT(res, funcobj);
	    }
	    context->scope = context->scope->next;
	    SEE_DEFAULT_CATCH(interp, ctxt);	/* re-throw any exception */
	}
}

static struct nodeclass FunctionExpression_nodeclass
	= { SUPERCLASS(Function)
	    FunctionExpression_eval, 0,
	    PARSER_PRINT(Function_print)
	    PARSER_VISIT(Function_visit)
	    0 };

static struct node *
FunctionExpression_parse(parser)
	struct parser *parser;
{
	struct Function_node *n;
	struct var *formal;
	int noin_save, is_lhs_save;
	struct SEE_string *name;
	struct node *body;

	/* Save parser state */
	noin_save = parser->noin;
	is_lhs_save = parser->is_lhs;
	parser->noin = 0;
	parser->is_lhs = 0;

	n = NEW_NODE(struct Function_node, &FunctionExpression_nodeclass);
	EXPECT(tFUNCTION);
	if (NEXT == tIDENT) {
		name = NEXT_VALUE->u.string;
		SKIP;
	} else
		name = NULL;
	EXPECT('(');
	formal = PARSE(FormalParameterList);
	EXPECT(')');

	EXPECT('{');
	parser->funcdepth++;
	body = PARSE(FunctionBody);
	parser->funcdepth--;
	EXPECT('}');

	n->function = SEE_function_make(parser->interpreter,
		name, formal, body);

	/* Restore parser state */
	parser->noin = noin_save;
	parser->is_lhs = is_lhs_save;

	return (struct node *)n;
}


static struct var *
FormalParameterList_parse(parser)
	struct parser *parser;
{
	struct var **p;
	struct var *result;

	p = &result;

	if (NEXT == tIDENT) {
	    *p = SEE_NEW(parser->interpreter, struct var);
	    (*p)->name = NEXT_VALUE->u.string;
	    p = &(*p)->next;
	    SKIP;
	    while (NEXT == ',') {
		SKIP;
		if (NEXT == tIDENT) {
		    *p = SEE_NEW(parser->interpreter, struct var);
		    (*p)->name = NEXT_VALUE->u.string;
		    p = &(*p)->next;
		}
		EXPECT(tIDENT);
	    }
	}
	*p = NULL;
	return result;
}


static void
FunctionBody_eval(na, context, res)
	struct node *na; /* (struct Unary_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct Unary_node *n = CAST_NODE(na, Unary);
	FPROC(n->a, context);
	EVAL(n->a, context, res);
}

struct FunctionBody_node {
	struct Unary_node u;
};

static struct nodeclass FunctionBody_nodeclass
	= { SUPERCLASS(Unary)
	    FunctionBody_eval, 0, 
	    PARSER_PRINT(Unary_print)
	    PARSER_VISIT(Unary_visit)
	    0 };

static struct node *
FunctionBody_parse(parser)
	struct parser *parser;
{
	struct FunctionBody_node *n;

	n = NEW_NODE(struct FunctionBody_node, &FunctionBody_nodeclass);
	n->u.a = PARSE(SourceElements);
	return (struct node *)n;
}

/*
 * JavaScript 1.5 function statements. (Not part of ECMA-262, which
 * treats functions as declarations.) The statement 
 * 'function foo (args) { body };' is treated syntactically
 * equivalent to 'foo = function foo (args) { body };' The Netscape
 * documentation calls these 'conditional functions', as their intent
 * is to be used like this:
 *    if (0) function foo() { abc };
 *    else   function foo() { xyz };
 */
static struct node *
FunctionStatement_parse(parser)
	struct parser *parser;
{
	struct Function_node *f;
	struct PrimaryExpression_ident_node *i;
	struct AssignmentExpression_node *an;
	struct Unary_node *e;

	f = (struct Function_node *)FunctionExpression_parse(parser);

	i = NEW_NODE(struct PrimaryExpression_ident_node,
		&PrimaryExpression_ident_nodeclass);
	i->string = f->function->name;

	an = NEW_NODE(struct AssignmentExpression_node, 
			&AssignmentExpression_simple_nodeclass);
	an->lhs = (struct node *)i;
	an->expr = (struct node *)f;

	e = NEW_NODE(struct Unary_node, &ExpressionStatement_nodeclass);
	e->a = (struct node *)an;

	return (struct node *)e;
}

/*
 *	-- 14
 *
 *	Program
 *	:	SourceElements
 *	;
 *
 *
 *	SourceElements
 *	:	SourceElement
 *	|	SourceElements SourceElement
 *	;
 *
 *
 *	SourceElement
 *	:	Statement
 *	|	FunctionDeclaration
 *	;
 */

static struct function *
Program_parse(parser)
	struct parser *parser;
{
	struct node *body;

	/*
	 * NB: The semantics of Program are indistinguishable from that of
	 * a FunctionBody. Syntactically, the only difference is that
	 * Program must be followed by the tEND (end-of-input) token.
	 * Practically, a program does not have parameters nor a name,
	 * and its 'this' is always set to the Global object.
	 */
	body = PARSE(FunctionBody);
	if (NEXT == '}')
		ERRORm("unmatched '}'");
	if (NEXT == ')')
		ERRORm("unmatched ')'");
	if (NEXT == ']')
		ERRORm("unmatched ']'");
	if (NEXT != tEND)
		ERRORm("unexpected token");
	return SEE_function_make(parser->interpreter,
		NULL, NULL, body);
}


struct SourceElements_node {
	struct node node;
	struct SourceElement {
		struct node *node;
		struct SourceElement *next;
	} *statements, *functions;
	struct var *vars;
};

static void
SourceElements_eval(na, context, res)
	struct node *na; /* (struct SourceElements_node) */
	struct SEE_context *context;
	struct SEE_value *res;
{
	struct SourceElements_node *n = CAST_NODE(na, SourceElements);
	struct SourceElement *e;

	/*
	 * NB: strictly, this should 'evaluate' the
	 * FunctionDeclarations, but they only yield <NORMAL, NULL, NULL>
	 * so, we don't. We just run the non-functiondecl statements
	 * instead. It has the same result.
	 */
	_SEE_SET_COMPLETION(res, SEE_COMPLETION_NORMAL, NULL, NULL);
	for (e = n->statements; e; e = e->next) {
		EVAL(e->node, context, res);
		if (res->u.completion.type != SEE_COMPLETION_NORMAL)
			break;
	}
}

static void
SourceElements_fproc(na, context)
	struct node *na; /* struct SourceElements_node */
	struct SEE_context *context;
{
	struct SourceElements_node *n = CAST_NODE(na, SourceElements);
	struct SourceElement *e;
	struct var *v;
	struct SEE_value undefv;

	for (e = n->functions; e; e = e->next)
		FPROC(e->node, context);

	/*
	 * spec bug(?): although not mentioned in the spec, this
	 * is the place to set the declared variables
	 * to undefined. (10.1.3). 
	 * (I say 'spec bug' because there is partial overlap
	 * between 10.1.3 and the semantics of 13.)
	 */
	SEE_SET_UNDEFINED(&undefv);
	for (v = n->vars; v; v = v->next)
	    if (!SEE_OBJECT_HASPROPERTY(context->interpreter,
		context->variable, v->name))
	            SEE_OBJECT_PUT(context->interpreter, context->variable, 
			v->name, &undefv, context->varattr);
}

#if WITH_PARSER_PRINT
static void
SourceElements_print(na, printer)
	struct node *na; /* (struct SourceElements_node) */
	struct printer *printer;
{
	struct SourceElements_node *n = CAST_NODE(na, SourceElements);
	struct var *v;
	struct SourceElement *e;
        SEE_char_t c;

	if (n->vars) {
	    PRINT_CHAR('/');
	    PRINT_CHAR('*');
	    PRINT_CHAR(' ');
	    PRINT_STRING(STR(var));
	    c = ' ';
	    for (v = n->vars; v; v = v->next)  {
		PRINT_CHAR(c); c = ',';
		PRINT_STRING(v->name);
	    }
	    PRINT_CHAR(';');
	    PRINT_CHAR(' ');
	    PRINT_CHAR('*');
	    PRINT_CHAR('/');
	    PRINT_NEWLINE(0);
	}

	for (e = n->functions; e; e = e->next)
		PRINT(e->node);

	PRINT_NEWLINE(0);

	for (e = n->statements; e; e = e->next)
		PRINT(e->node);
}
#endif

#if WITH_PARSER_VISIT
static void
SourceElements_visit(na, v, va)
	struct node *na; /* (struct SourceElements_node) */
	visitor_fn_t v;
	void *va;
{
	struct SourceElements_node *n = CAST_NODE(na, SourceElements);
	struct SourceElement *e;

	for (e = n->functions; e; e = e->next)
		VISIT(e->node, v, va);
	for (e = n->statements; e; e = e->next)
		VISIT(e->node, v, va);
}
#endif

static struct nodeclass SourceElements_nodeclass
	= { BASECLASS
	    SourceElements_eval,
	    SourceElements_fproc,
	    PARSER_PRINT(SourceElements_print)
	    PARSER_VISIT(SourceElements_visit)
	    0 };

static struct node *
SourceElements_parse(parser)
	struct parser *parser;
{
	struct SourceElements_node *se;
	struct SourceElement **s, **f;
	struct var **vars_save;

	se = NEW_NODE(struct SourceElements_node, &SourceElements_nodeclass); 
	s = &se->statements;
	f = &se->functions;

	/* Whenever a VarDecl parses, it will get added to se->vars! */
	vars_save = parser->vars;
	parser->vars = &se->vars;

	for (;;) 
	    switch (NEXT) {
	    case tFUNCTION:
		if (lookahead(parser, 1) != '(') {
		    *f = SEE_NEW(parser->interpreter, struct SourceElement);
		    (*f)->node = PARSE(FunctionDeclaration);
		    f = &(*f)->next;
#ifndef NDEBUG
		    if (SEE_parse_debug) 
		        dprintf("SourceElements_parse: got function\n");
#endif
		    break;
		}
		/* else it's a function expression */
	    /* The 'first's of Statement */
	    case tTHIS: case tIDENT: case tSTRING: case tNUMBER:
	    case tNULL: case tTRUE: case tFALSE:
	    case '(': case '[': case '{':
	    case tNEW: case tDELETE: case tVOID: case tTYPEOF:
	    case tPLUSPLUS: case tMINUSMINUS:
	    case '+': case '-': case '~': case '!': case ';':
	    case tVAR: case tIF: case tDO: case tWHILE: case tFOR:
	    case tCONTINUE: case tBREAK: case tRETURN:
	    case tWITH: case tSWITCH: case tTHROW: case tTRY:
	    case tDIV: case tDIVEQ: /* in lieu of tREGEX */
		*s = SEE_NEW(parser->interpreter, struct SourceElement);
		(*s)->node = PARSE(Statement);
		s = &(*s)->next;
#ifndef NDEBUG
		if (SEE_parse_debug)
		    dprintf("SourceElements_parse: got statement\n");
#endif
		break;
	    case tEND:
	    default:
#ifndef NDEBUG
		if (SEE_parse_debug)
		    dprintf("SourceElements_parse: got EOF/other (%d)\n", 
		    	NEXT);
#endif
		*s = NULL;
		*f = NULL;
		*parser->vars = NULL;
		parser->vars = vars_save;

		return (struct node *)se;
	    }
}

/*------------------------------------------------------------
 * Public API
 */

/*
 * Parses a function declaration in two parts and
 * return a function structure, in a similar way to
 * FunctionDeclaration_parse() when called with the
 * right input.
 */
struct function *
SEE_parse_function(interp, name, paraminp, bodyinp)
	struct SEE_interpreter *interp;
	struct SEE_string *name;
	struct SEE_input *paraminp, *bodyinp;
{
	struct lex lex;
	struct parser parservar, *parser = &parservar;
	struct var *formal;
	struct node *body;

	if (paraminp) {
		SEE_lex_init(&lex, SEE_input_lookahead(paraminp, 6));
		parser_init(parser, interp, &lex);
		formal = PARSE(FormalParameterList);	/* handles "" too */
		EXPECT_NOSKIP(tEND);			/* uses parser var */
	} else
		formal = NULL;

	if (bodyinp) 
		SEE_lex_init(&lex, SEE_input_lookahead(bodyinp, 6));
	else {
		/* Set the lexer to EOF quickly */
		lex.input = NULL;
		lex.next = tEND;
	}
	parser_init(parser, interp, &lex);
	parser->funcdepth++;
	body = PARSE(FunctionBody);
	parser->funcdepth--;
	EXPECT_NOSKIP(tEND);

	return SEE_function_make(interp, name, formal, body);
}

/*
 * Parses a Program. 
 * Does not close the input, but may consume up to 6 characters.
 * lookahead. This is not usually a problem, because the input is
 * always read to EOF on normal completion.
 */
struct function *
SEE_parse_program(interp, inp)
	struct SEE_interpreter *interp;
	struct SEE_input *inp;
{
	struct lex lex;
	struct parser localparse, *parser = &localparse;
	struct function *f;

	SEE_lex_init(&lex, SEE_input_lookahead(inp, 6));
	parser_init(parser, interp, &lex);
	f = PARSE(Program);

#if !defined(NDEBUG) && WITH_PARSER_PRINT
	if (SEE_parse_debug) {
	    dprintf("parse Program result:\n");
	    print_functionbody(interp, f, stderr);
	    dprintf("<end>\n");
	}
#endif

	return f;
}

/* Evaluates the function body with the given execution context. */
void
SEE_eval_functionbody(f, context, res)
	struct function *f;
	struct SEE_context *context;
	struct SEE_value *res;
{
	EVAL((struct node *)f->body, context, res);
}

int
SEE_functionbody_isempty(interp, f)
	struct SEE_interpreter *interp;
	struct function *f;
{
	return FunctionBody_isempty(interp, (struct node *)f->body);
}

/* Returns true if the FunctionBody is empty. */
static int
FunctionBody_isempty(interp, body)
	struct SEE_interpreter *interp;
	struct node *body;
{
	struct SourceElements_node *se;
	struct FunctionBody_node *f;
	
	f = CAST_NODE(body, FunctionBody);
	se = CAST_NODE(f->u.a, SourceElements);
	return se->statements == NULL;
}

#if WITH_PARSER_PRINT
/*------------------------------------------------------------
 * Printer common code
 */

static void
printer_init(printer, interp, printerclass)
	struct printer *printer;
	struct SEE_interpreter *interp;
	struct printerclass *printerclass;
{
	printer->printerclass = printerclass;
	printer->interpreter = interp;
	printer->indent = 0;
	printer->bol = 0;
}

/* Called when the printer is at the beginning of a line. */
static void
printer_atbol(printer)
	struct printer *printer;
{
	int i;

	printer->bol = 0;		/* prevent recursion */
	PRINT_CHAR('\n');
	for (i = 0; i < printer->indent; i++) {
		PRINT_CHAR(' ');
		PRINT_CHAR(' ');
	}
}

static void
printer_print_newline(printer, indent)
	struct printer *printer;
	int indent;
{
	printer->bol = 1;
	printer->indent += indent;
}

static void
printer_print_node(printer, n)
	struct printer *printer;
	struct node *n;
{
	(*(n)->nodeclass->print)(n, printer);
}

static void
print_hex(printer, i)
	struct printer *printer;
	int i;
{
	if (i >= 16) print_hex(printer, i >> 4);
	PRINT_CHAR(SEE_hexstr_lowercase[i & 0xf]);
}

/*------------------------------------------------------------
 * Stdio printer - Prints each node in an AST to a stdio file.
 * So we can reconstruct parsed programs and print them to the screen.
 */

struct stdio_printer {
	struct printer printer;
	FILE *output;
};

static void
stdio_print_string(printer, s)
	struct printer *printer;
	struct SEE_string *s;
{
	struct stdio_printer *sp = (struct stdio_printer *)printer;

	if (printer->bol) printer_atbol(printer);
	SEE_string_fputs(s, sp->output);
}

static void
stdio_print_char(printer, c)
	struct printer *printer;
	int c;		/* SEE_char_t promoted to int */
{
	struct stdio_printer *sp = (struct stdio_printer *)printer;

	if (printer->bol) printer_atbol(printer);
	fputc(c & 0x7f, sp->output);
}

static void
stdio_print_node(printer, n)
	struct printer *printer;
	struct node *n;
{
	struct stdio_printer *sp = (struct stdio_printer *)printer;

	fprintf(sp->output, "(%d: ", n->location.lineno); 
	(*(n)->nodeclass->print)(n, printer);
	fprintf(sp->output, ")"); 
	fflush(sp->output);
}

static struct printerclass stdio_printerclass = {
	stdio_print_string,
	stdio_print_char,
	printer_print_newline,
	stdio_print_node, /* printer_print_node */
};

static struct printer *
stdio_printer_new(interp, output)
	struct SEE_interpreter *interp;
	FILE *output;
{
	struct stdio_printer *sp;

	sp = SEE_NEW(interp, struct stdio_printer);
	printer_init(&sp->printer, interp, &stdio_printerclass);
	sp->output = output;
	return (struct printer *)sp;
}

/*------------------------------------------------------------
 * String printer
 * Used to reconstruct parsed programs and save them in a string.
 */

struct string_printer {
	struct printer printer;
	struct SEE_string *string;
};

static void
string_print_string(printer, s)
	struct printer *printer;
	struct SEE_string *s;
{
	struct string_printer *sp = (struct string_printer *)printer;

	if (printer->bol) printer_atbol(printer);
	SEE_string_append(sp->string, s);
}

static void
string_print_char(printer, c)
	struct printer *printer;
	int c;		/* SEE_char_t promoted to int */
{
	struct string_printer *sp = (struct string_printer *)printer;

	if (printer->bol) printer_atbol(printer);
	SEE_string_addch(sp->string, c);
}

static struct printerclass string_printerclass = {
	string_print_string,
	string_print_char,
	printer_print_newline,
	printer_print_node
};

static struct printer *
string_printer_new(interp, string)
	struct SEE_interpreter *interp;
	struct SEE_string *string;
{
	struct string_printer *sp;

	sp = SEE_NEW(interp, struct string_printer);
	printer_init(&sp->printer, interp, &string_printerclass);
	sp->string = string;
	return (struct printer *)sp;
}


/*
 * Prints a function body to standard error.
 */
static void
print_functionbody(interp, f, fp)
	struct SEE_interpreter *interp;
	struct function *f;
	FILE *fp;
{
	struct printer *printer;

	printer = stdio_printer_new(interp, fp);
	PRINT((struct node *)f->body);
}
#endif /* WITH_PARSER_PRINT */

/* Returns the function body as a string */
struct SEE_string *
SEE_functionbody_string(interp, f)
	struct SEE_interpreter *interp;
	struct function *f;
{
	struct SEE_string *s = SEE_string_new(interp, 0);

#if WITH_PARSER_PRINT
	struct printer *printer = string_printer_new(interp, s);
	PRINT((struct node *)f->body);
#else
	SEE_string_addch(s, '/');
	SEE_string_addch(s, '*');
	SEE_string_append_int(s, (int)f);
	SEE_string_addch(s, '*');
	SEE_string_addch(s, '/');
#endif
	return s;
}

/*------------------------------------------------------------
 * eval
 *  -- 15.1.2.1
 */

/*
 * Global.eval()
 * 'Eval' is a special function (not a cfunction), because it accesses 
 * the execution context of the caller (which is not available to 
 * functions and methods invoked via SEE_OBJECT_CALL()).
 *
 * This normally only ever get called from CallExpression_eval().
 * A stub cfunction exists for Global.eval, but it is bypassed.
 */
static void
eval(context, thisobj, argc, argv, res)
	struct SEE_context *context;
	struct SEE_object *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_input *inp;
	struct function *f;
	struct SEE_value v;
	struct SEE_context evalcontext;
	struct SEE_interpreter *interp = context->interpreter;

	if (argc == 0) {
		SEE_SET_UNDEFINED(res);
		return;
	}
	if (SEE_VALUE_GET_TYPE(argv[0]) != SEE_STRING) {
		SEE_VALUE_COPY(res, argv[0]);
		return;
	}
	
	inp = SEE_input_string(interp, argv[0]->u.string);
	f = SEE_parse_program(interp, inp);
	SEE_INPUT_CLOSE(inp);

	/* 10.2.2 */
	evalcontext.interpreter = interp;
	evalcontext.activation = context->activation;	/* XXX */
	evalcontext.variable = context->variable;
	evalcontext.varattr = 0;
	evalcontext.thisobj = context->thisobj;
	evalcontext.scope = context->scope;

	if (SEE_COMPAT_JS(interp, >=, JS11)	/* EXT:23 */
	    && thisobj && thisobj != interp->Global) 
	{
		/*
		 * support eval() being called from something
		 * other than the global object, where the 'thisobj'
		 * becomes the scope chain and variable object
		 */
		evalcontext.thisobj = thisobj;
		evalcontext.variable = thisobj;
		evalcontext.scope = SEE_NEW(interp, struct SEE_scope);
		evalcontext.scope->next = context->scope;
		evalcontext.scope->obj = thisobj;
	}

	/* Set formal params to undefined, if any exist -- redundant? */
	SEE_function_put_args(context, f, 0, NULL);

	/* Evaluate the statement */
	SEE_eval_functionbody(f, &evalcontext, &v);

	if (SEE_VALUE_GET_TYPE(&v) != SEE_COMPLETION || 
	    v.u.completion.type != SEE_COMPLETION_NORMAL) 
	{
#ifndef NDEBUG
	    dprintf("eval'd string returned ");
	    dprintv(interp, &v);
	    dprintf("\n");
#endif
	    SEE_error_throw_string(
		interp,
		interp->EvalError,
		STR(internal_error));
        }
	if (v.u.completion.value == NULL)
	    SEE_SET_UNDEFINED(res);
	else
	    SEE_VALUE_COPY(res, v.u.completion.value);
}

/* 
 * Evaluates an expression in the given context.
 * This is a helper function intended for external debuggers wanting 
 * to evaluate user expressions in a given context.
 */
void
SEE_context_eval(context, expr, res)
	struct SEE_context *context;
	struct SEE_string *expr;
	struct SEE_value *res;
{
	struct SEE_value s, *argv[1];

	argv[0] = &s;
	SEE_SET_STRING(argv[0], expr);
	eval(context, context->interpreter->Global, 1, argv, res);
}

/*
 * Compares two value using ECMAScript == and > operator semantics.
 * Returns  0 if x == y,
 *          1 if x > y or indeterminate,
 *         -1 otherwise.
 * This could be used as a better comparsion function for Array.sort().
 * Currently only used by RegExp.prototype.test()
 */
int
SEE_compare(interp, x, y)
	struct SEE_interpreter *interp;
	struct SEE_value *x, *y;
{
	struct SEE_value v;

	EqualityExpression_eq(interp, x, y, &v);
	if (v.u.boolean)
		return 0;
	RelationalExpression_sub(interp, x, y, &v);
	if (SEE_VALUE_GET_TYPE(&v) == SEE_UNDEFINED || !v.u.boolean)
		return 1;
	else
		return -1;
}
