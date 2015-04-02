/* Copyright (c) 2005, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _SEE_h_nmath
#define _SEE_h_nmath

#if STDC_HEADERS
# include <math.h>
#endif

#if SEE_NUMBER_IS_FLOAT
# define NUMBER_acos(a)		acosf(a)
# define NUMBER_asin(a)		asinf(a)
# define NUMBER_atan(a)		atanf(a)
# define NUMBER_atan2(a,b)	atan2f(a,b)
# define NUMBER_ceil(a)		ceilf(a)
# define NUMBER_cos(a)		cosf(a)
# define NUMBER_copysign(a,b)	copysignf(a,b)
# define NUMBER_exp(a)		expf(a)
# define NUMBER_floor(a)	floorf(a)
# define NUMBER_fmod(a,b)	fmodf(a,b)
# define NUMBER_log(a)		logf(a)
# define NUMBER_pow(a,b)	powf(a,b)
# define NUMBER_rint(a)		rintf(a)
# define NUMBER_sin(a)		sinf(a)
# define NUMBER_sqrt(a)		sqrtf(a)
# define NUMBER_tan(a)		tanf(a)
#endif

#if SEE_NUMBER_IS_DOUBLE
# define NUMBER_acos(a)		acos(a)
# define NUMBER_asin(a)		asin(a)
# define NUMBER_atan(a)		atan(a)
# define NUMBER_atan2(a,b)	atan2(a,b)
# define NUMBER_ceil(a)		ceil(a)
# define NUMBER_cos(a)		cos(a)
# define NUMBER_copysign(a,b)	copysign(a,b)
# define NUMBER_exp(a)		exp(a)
# define NUMBER_floor(a)	floor(a)
# define NUMBER_fmod(a,b)	fmod(a,b)
# define NUMBER_log(a)		log(a)
# define NUMBER_pow(a,b)	pow(a,b)
# define NUMBER_rint(a)		rint(a)
# define NUMBER_sin(a)		sin(a)
# define NUMBER_sqrt(a)		sqrt(a)
# define NUMBER_tan(a)		tan(a)
#endif

#ifndef MIN
# define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
# define MAX(a,b)	((a) < (b) ? (b) : (a))
#endif

#endif /* _SEE_h_nmath */
