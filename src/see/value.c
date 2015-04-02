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
/* $Id: value.c 1021 2006-02-08 15:40:35Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <string.h>
#endif

#include "see_mem.h"
#include "see_type.h"
#include "see_value.h"
#include "see_string.h"
#include "see_object.h"
#include "see_input.h"
#include "see_error.h"
#include "see_interpreter.h"

#include "see_lex.h"
#include "see_stringdefs.h"
#include "see_dtoa.h"
#include "see_nmath.h"

/*
 * Value type-converters and some numeric constants.
 */

/* IEEE-754 constants */
const unsigned char
# if WORDS_BIGENDIAN
 SEE_literal_NaN[8] = { 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
 SEE_literal_Inf[8] = { 0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
 SEE_literal_Max[8] = { 0x7f, 0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
 SEE_literal_Min[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
# else /* little endian */
 SEE_literal_NaN[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f },
 SEE_literal_Inf[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x7f },
 SEE_literal_Max[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0x7f },
 SEE_literal_Min[8] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
# endif /* little endian */

/* Convenience array for generating hexadecimal numbers */
char SEE_hexstr_lowercase[16] = {
	'0','1','2','3','4','5','6','7',
	'8','9','a','b','c','d','e','f'
};
char SEE_hexstr_uppercase[16] = {
	'0','1','2','3','4','5','6','7',
	'8','9','A','B','C','D','E','F'
};

/* 9.1 */
void
SEE_ToPrimitive(interp, val, type, res)
	struct SEE_interpreter *interp;
	struct SEE_value *val, *type, *res;
{
	if (SEE_VALUE_GET_TYPE(val) == SEE_OBJECT)
		SEE_OBJECT_DEFAULTVALUE(interp, val->u.object, type, res);
	else
		SEE_VALUE_COPY(res, val);
}

/* 9.2 */
void
SEE_ToBoolean(interp, val, res)
	struct SEE_interpreter *interp;
	struct SEE_value *val, *res;
{
	switch (SEE_VALUE_GET_TYPE(val)) {
	case SEE_UNDEFINED:
	case SEE_NULL:
		SEE_SET_BOOLEAN(res, 0);
		break;
	case SEE_BOOLEAN:
		SEE_VALUE_COPY(res, val);
		break;
	case SEE_NUMBER:
		if (val->u.number == 0 || SEE_NUMBER_ISNAN(val))
			SEE_SET_BOOLEAN(res, 0);
		else
			SEE_SET_BOOLEAN(res, 1);
		break;
	case SEE_STRING:
		if (val->u.string->length == 0)
			SEE_SET_BOOLEAN(res, 0);
		else
			SEE_SET_BOOLEAN(res, 1);
		break;
	case SEE_OBJECT:
		if (SEE_COMPAT_JS(interp, <=, JS12)) {
		    /* In JS <= 1.2, Boolean instances convert to bool */
		    extern struct SEE_objectclass _SEE_boolean_inst_class;
		    struct SEE_object *bo = val->u.object;
		    if (bo->objectclass == &_SEE_boolean_inst_class) {
		        struct SEE_value vo;
			SEE_OBJECT_GET(interp, bo, STR(valueOf), &vo);
			if (SEE_VALUE_GET_TYPE(&vo) == SEE_OBJECT &&
			    SEE_OBJECT_HAS_CALL(vo.u.object)) 
			{
			    SEE_OBJECT_CALL(interp, vo.u.object, bo,
			    	0, NULL, res);
			    if (SEE_VALUE_GET_TYPE(res) == SEE_BOOLEAN)
			        break;
			}
		    }
		}
		SEE_SET_BOOLEAN(res, 1);
		break;
	default:
		SEE_error_throw_string(interp, interp->TypeError, 
			STR(toboolean_bad));
	}
}

/* 9.3 */
void
SEE_ToNumber(interp, val, res)
	struct SEE_interpreter *interp;
	struct SEE_value *val, *res;
{
	switch (SEE_VALUE_GET_TYPE(val)) {
	case SEE_UNDEFINED:
		SEE_SET_NUMBER(res, SEE_NaN);
		break;
	case SEE_NULL:
		SEE_SET_NUMBER(res, 0.0);
		break;
	case SEE_BOOLEAN:
		SEE_SET_NUMBER(res, val->u.boolean ? 1.0 : 0.0);
		break;
	case SEE_NUMBER:
		SEE_VALUE_COPY(res, val);
		break;
	case SEE_STRING:
	    {
		/* Use the scanner to evaluate a StrNumericLiteral */
		if (!SEE_lex_number(interp, val->u.string, res))
			SEE_SET_NUMBER(res, SEE_NaN);
		break;
	    }
	case SEE_OBJECT:
	    {
		struct SEE_value hint, primitive;

		SEE_SET_OBJECT(&hint, interp->Number);
		SEE_ToPrimitive(interp, val, &hint, &primitive);
		SEE_ToNumber(interp, &primitive, res);
		break;
	    }
	default:
		SEE_error_throw_string(interp, interp->TypeError, 
			STR(tonumber_bad));
	}
}

/* 9.4 */
void
SEE_ToInteger(interp, val, res)
	struct SEE_interpreter *interp;
	struct SEE_value *val, *res;
{
	SEE_ToNumber(interp, val, res);
	if (SEE_NUMBER_ISNAN(res))
		res->u.number = 0.0;
	else if (SEE_NUMBER_ISINF(res) || res->u.number == 0.0)
		; /* nothing */
	else
		res->u.number = NUMBER_copysign(floor(
			NUMBER_copysign(res->u.number, 1.0)),
				  	 res->u.number);

}

/* 9.5 */
SEE_int32_t
SEE_ToInt32(interp, val)
	struct SEE_interpreter *interp;
	struct SEE_value *val;
{
	return (SEE_int32_t)SEE_ToUint32(interp, val);
}

/* 9.6 */
SEE_uint32_t
SEE_ToUint32(interp, val)
	struct SEE_interpreter *interp;
	struct SEE_value *val;
{
	struct SEE_value i;

	SEE_ToInteger(interp, val, &i);
	if (SEE_NUMBER_ISINF(&i) || i.u.number == 0.0)
		return 0;
	else {
		i.u.number = fmod(i.u.number, 4294967296.0); /* 2^32 */
		if (i.u.number < 0)
			i.u.number += 4294967296.0;
		return (SEE_uint32_t)i.u.number;
	}
}

/* 9.7 */
SEE_uint16_t
SEE_ToUint16(interp, val)
	struct SEE_interpreter *interp;
	struct SEE_value *val;
{
	struct SEE_value i;

	/* NB: slightly different to standard, but equivalent */
	SEE_ToInteger(interp, val, &i);
	if (SEE_NUMBER_ISINF(&i) || i.u.number == 0.0)
		return 0;
	else {
		i.u.number = fmod(i.u.number, 65536.0);	/* 2^16 */
		if (i.u.number < 0)
			return (SEE_uint16_t)(i.u.number + 65536.0);
		else
			return (SEE_uint16_t)i.u.number;
	}
}

/* 9.8 */
void
SEE_ToString(interp, val, res)
	struct SEE_interpreter *interp;
	struct SEE_value *val, *res;
{
	switch (SEE_VALUE_GET_TYPE(val)) {
	case SEE_UNDEFINED:
		SEE_SET_STRING(res, STR(undefined));
		break;
	case SEE_NULL:
		SEE_SET_STRING(res, STR(null));
		break;
	case SEE_BOOLEAN:
		SEE_SET_STRING(res, val->u.boolean ? STR(true) : STR(false));
		break;
	case SEE_NUMBER:					 /* 9.8.1 */
		if (SEE_NUMBER_ISNAN(val)) {
			SEE_SET_STRING(res, STR(NaN));
		} else if (val->u.number == 0) {
			SEE_SET_STRING(res, STR(zero_digit));
		} else if (val->u.number < 0) {
			struct SEE_value neg, negstr;
			SEE_SET_NUMBER(&neg, -(val->u.number));
			SEE_ToString(interp, &neg, &negstr);
			SEE_SET_STRING(res, SEE_string_concat(interp,
			    STR(minus), negstr.u.string));
		} else if (SEE_NUMBER_ISPINF(val)) {
			SEE_SET_STRING(res, STR(Infinity));
		} else {
			char *a, *endstr;
			struct SEE_string *s;
			int sign, k, n, i, exponent;

			a = SEE_dtoa(val->u.number, DTOA_MODE_SHORT_SW, 31, 
				&n, &sign, &endstr);
			k = (int)(endstr - a);
			s = SEE_string_new(interp, 0);
			SEE_ASSERT(interp, !sign);
			if (k <= n && n <= 21) {
			    for (i = 0; i < k; i++)
				SEE_string_addch(s, a[i]);
			    for (i = 0; i < n-k; i++)
				SEE_string_addch(s, '0');
			} else if (0 < n && n <= 21) {
			    for (i = 0; i < n; i++)
				SEE_string_addch(s, a[i]);
			    SEE_string_addch(s, '.');
			    for (; i < k; i++)
				SEE_string_addch(s, a[i]);
			} else if (-6 < n && n <= 0) {
			    SEE_string_addch(s, '0');
			    SEE_string_addch(s, '.');
			    for (i = 0; i < -n; i++)
				SEE_string_addch(s, '0');
			    for (i = 0; i < k; i++)
				SEE_string_addch(s, a[i]);
			} else if (k == 1) {
			    SEE_string_addch(s, a[0]);
			    goto add_exponent;
			} else {
			    SEE_string_addch(s, a[0]);
			    SEE_string_addch(s, '.');
			    for (i = 1; i < k; i++)
				SEE_string_addch(s, a[i]);
	    add_exponent:   SEE_string_addch(s, 'e');
			    exponent = n - 1;
			    if (exponent > 0)
				SEE_string_addch(s, '+');
			    SEE_string_append_int(s, exponent);
			}
			SEE_SET_STRING(res, s);
			SEE_freedtoa(a);
		} 
		break;
	case SEE_STRING:
		SEE_VALUE_COPY(res, val);
		break;
	case SEE_OBJECT:
	    {
		struct SEE_value prim;
		struct SEE_value hint;

		SEE_SET_OBJECT(&hint, interp->String);
		SEE_ToPrimitive(interp, val, &hint, &prim);
		SEE_ToString(interp, &prim, res);
		break;
	    }
	default:
		SEE_error_throw_string(interp, interp->TypeError, 
			STR(tostring_bad));
	}
}

/* 9.9 */
void
SEE_ToObject(interp, val, res)
	struct SEE_interpreter *interp;
	struct SEE_value *val, *res;
{
	struct SEE_object *obj;
	struct SEE_value *arg[1];

	switch (SEE_VALUE_GET_TYPE(val)) {
	case SEE_UNDEFINED:
		SEE_error_throw_string(interp, interp->TypeError, 
			STR(toobject_undefined));
	case SEE_NULL:
		SEE_error_throw_string(interp, interp->TypeError, 
			STR(toobject_null));
	case SEE_OBJECT:
		SEE_VALUE_COPY(res, val);
		return;
	case SEE_BOOLEAN:
		obj = interp->Boolean;
		break;
	case SEE_NUMBER:
		obj = interp->Number;
		break;
	case SEE_STRING:
		obj = interp->String;
		break;
	default:
		SEE_error_throw_string(interp, interp->TypeError, 
			STR(toobject_bad));
	}
	arg[0] = val;
	SEE_OBJECT_CONSTRUCT(interp, obj, obj, 1, arg, res);
}
