/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: dtoa_config.h 997 2006-01-31 21:10:59Z d $ */

/*
 * Configuration directives for dtoa when used by SEE
 */

#if STDC_HEADERS
#include <float.h>
#include <stdlib.h>
#endif

#include "see_type.h"

#if defined(__i386__) || defined(__amd64__) || defined(__ia64__) || defined(__alpha__)
#   define IEEE_8087
#endif

#if defined(__m68k__) || defined(__sparc__) || defined(__ppc__)
#   define IEEE_MC68k 
#endif

#if defined(__vax__) && !defined(VAX)
#   define VAX
#endif

/* #define IBM for IBM mainframe-style floating-point arithmetic. */

/* Use the types determined from <see/types.h> */
#define Long	SEE_int32_t
#define ULong	SEE_uint32_t
#define LLong	SEE_int64_t
#define ULLong	SEE_uint64_t

#if HAVE_MALLOC
#define MALLOC	malloc
#endif

#define NO_ERRNO
#define IEEE_Arith

/* Note that <see/config.h> will define 'const' as empty if it is unusable */
#define CONST	const

/* #define No_leftright to omit left-right logic in fast floating-point */

/* #define Honor_FLT_ROUNDS if FLT_ROUNDS can assume the values 2 or 3 */
/* #define Check_FLT_ROUNDS if FLT_ROUNDS can assume the values 2 or 3 */
/* #define RND_PRODQUOT to use rnd_prod and rnd_quot (assembly routines */
/* #define ROUND_BIASED for IEEE-format with biased rounding. */
/* #define Inaccurate_Divide for IEEE-format with correctly rounded */
/* #define KR_headers for old-style C function headers. */
/* #define Bad_float_h if your system lacks a float.h or if it does not */
/* #define INFNAN_CHECK on IEEE systems to cause strtod to check for */
/* #define MULTIPLE_THREADS if the system offers preemptively scheduled */
/* #define NO_IEEE_Scale to disable new (Feb. 1997) logic in strtod that */
/* #define YES_ALIAS to permit aliasing certain double values with */
/* #define USE_LOCALE to use the current locale's decimal_point value. */
/* #define SET_INEXACT if IEEE arithmetic is being used and extra */

/* Translate these symbols for SEE-only use */
#define strtod		SEE_strtod
#define dtoa		SEE_dtoa
#define freedtoa	SEE_freedtoa
