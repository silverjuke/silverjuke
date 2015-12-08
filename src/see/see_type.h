/* Automatically generated. Do not edit. */
/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: type.h.in 993 2006-01-31 15:07:25Z d $ */

#ifndef _SEE_h_type_
#define _SEE_h_type_

/*
 * Machine-dependent types and definitions
 */

#if defined(WIN32)
	#include "see_type_msw_add.h"
#elif defined(__WXMAC__) || defined(__WXGTK__)
	#include "see_type_mac_add.h"
#else
	#error
#endif

#if 0/*!size_t*/
typedef SEE_size_t unsigned;
#else
# include <sys/types.h>
# define SEE_size_t size_t
#endif

#if 1/*HAVE_FLOAT_H*/
# include <float.h>
#endif

#if 0/*HAVE_INTTYPES_H*/
# include <inttypes.h>

typedef uint16_t SEE_uint16_t;
typedef uint32_t SEE_uint32_t;
typedef int32_t  SEE_int32_t;
typedef uint64_t SEE_uint64_t;
typedef int64_t  SEE_int64_t;

#else /* !HAVE_INTTYPES_H */

# if 0/*!HAVE_STDINT_H*/
#  include <stdint.h>

typedef uint16_t SEE_uint16_t;
typedef uint32_t SEE_uint32_t;
typedef int32_t  SEE_int32_t;

# else /* !HAVE_STDINT_H */

/* 16-bit unsigned integer */
#  if 2/*SIZEOF_UNSIGNED_SHORT*/ == 2
typedef unsigned short	  SEE_uint16_t;
#  else
#   if 4/*SIZEOF_UNSIGNED_INT*/ == 2
typedef unsigned int	  SEE_uint16_t;
#   else
 #   error "cannot provide type for SEE_uint16_t"
#   endif
#  endif

/* 32-bit signed integer */
#  if 2/*SIZEOF_SIGNED_SHORT*/ == 4
typedef signed short	  SEE_int32_t;
#  else
#   if 4/*SIZEOF_SIGNED_INT*/ == 4
typedef signed int	  SEE_int32_t;
#   else
#    if 4/*SIZEOF_SIGNED_LONG*/ == 4
typedef signed long	  SEE_int32_t;
#    else
 #    error "cannot provide type for SEE_int32_t"
#    endif
#   endif
#  endif

/* 32-bit unsigned integer */
#  if 2/*SIZEOF_UNSIGNED_SHORT*/ == 4
typedef unsigned short	  SEE_uint32_t;
#  else
#   if 4/*SIZEOF_UNSIGNED_INT*/ == 4
typedef unsigned int	  SEE_uint32_t;
#   else
#    if 4/*SIZEOF_UNSIGNED_LONG*/ == 4
typedef unsigned long	  SEE_uint32_t;
#    else
 #    error "cannot provide type for SEE_uint32_t"
#    endif
#   endif
#  endif

# endif /* !HAVE_STDINT_H */

/* 64-bit signed integer */
# if 4/*SIZEOF_SIGNED_INT*/ == 8
typedef signed int	  SEE_int64_t;
# else
#  if 4/*SIZEOF_SIGNED_LONG*/ == 8
typedef signed long	  SEE_int64_t;
#  else
#   if 0/*SIZEOF_SIGNED_LONGLONG*/ == 8
typedef signed LONGLONG   SEE_int64_t;
#   else
#    if 8/*SIZEOF_SIGNED_LONG_LONG*/ == 8
#ifdef WIN32
typedef signed __int64  SEE_int64_t;
#else
typedef signed long long  SEE_int64_t;
#endif
#    else
 #    error "cannot provide type for SEE_int64_t"
#    endif
#   endif
#  endif
# endif

/* 64-bit unsigned integer */
# if 4/*SIZEOF_UNSIGNED_INT*/ == 8
typedef unsigned int	  SEE_uint64_t;
# else
#  if 4/*SIZEOF_UNSIGNED_LONG*/ == 8
typedef unsigned long	  SEE_uint64_t;
#  else
#   if 0/*SIZEOF_UNSIGNED_LONGLONG*/ == 8
typedef unsigned LONGLONG  SEE_uint64_t;
#   else
#    if 8/*SIZEOF_UNSIGNED_LONG_LONG*/ == 8
#ifdef WIN32
typedef unsigned __int64 SEE_uint64_t;
#else
typedef unsigned long long SEE_uint64_t;
#endif
#    else
 #    error "cannot provide type for SEE_uint64_t"
#    endif
#   endif
#  endif
# endif

#endif /* !HAVE_INTTYPES_H */

/* 64-bit floating point */
#if 4/*SIZEOF_FLOAT*/ == 8
# define SEE_NUMBER_IS_FLOAT 1
typedef float SEE_number_t;
#else
# if 8/*SIZEOF_DOUBLE*/ == 8
#  define SEE_NUMBER_IS_DOUBLE 1
typedef double SEE_number_t;
# else
 # error "cannot provide 64-bit IEEE-754 type for SEE_number_t"
# endif
#endif

typedef unsigned char     SEE_boolean_t;  /* non-zero means true */

/* derived types */
typedef SEE_uint16_t	  SEE_char_t;     /* UTF-16 encoding */
typedef SEE_uint32_t	  SEE_unicode_t;  /* UCS-4 encoding */

/* attributes that assist the compiler */
#if __GNUC__
# define SEE_dead	__attribute__((__noreturn__))
# define _SEE_malloc	__attribute__((__malloc__))
#else
# define SEE_dead	/* nothing */
# define _SEE_malloc	/* nothing */
#endif

/* IEEE-754 constants defined in value.o */
extern const unsigned char
	SEE_literal_NaN[8],
	SEE_literal_Inf[8],
	SEE_literal_Max[8],
	SEE_literal_Min[8];

#if 0 //1/*HAVE_CONSTANT_HEX_FLOAT*/ && 1/*HAVE_CONSTANT_NAN_DIV*/ && 1/*HAVE_CONSTANT_INF_DIV*/
# define SEE_NaN		((SEE_number_t) (0.0 / 0.0))
# define SEE_Infinity		((SEE_number_t) (1.0 / 0.0))
# define SEE_MinNumber		((SEE_number_t) 0x1p-1074)
# define SEE_MaxNumber		((SEE_number_t) 0x1fffffffffffffp971)
#else
# define SEE_NaN		(*(SEE_number_t *)&SEE_literal_NaN)
# define SEE_Infinity		(*(SEE_number_t *)&SEE_literal_Inf)
# define SEE_MinNumber		(*(SEE_number_t *)&SEE_literal_Min)
# define SEE_MaxNumber		(*(SEE_number_t *)&SEE_literal_Max)
#endif

/* On-stack allocation */
#define SEE_ALLOCA(i,t,n)  (t *)((n) ? alloca((n) * sizeof (t)) : 0)
#define SEE_STRING_ALLOCA(i,t,n)  SEE_ALLOCA(i,t,n)
#if 1/*STDC_HEADERS*/
# include <stdlib.h>
#endif

#if 0
#ifndef __GNUC__
# if *@HAVE_ALLOCA_H@
#  include <alloca.h>
# else
#  ifdef _AIX
 #  pragma alloca
#  else
#   ifndef alloca 		/* predefined by HP cc +Olibcalls */
#    ifndef 1/*HAVE_ALLOCA*/
#     undef SEE_ALLOCA
#     define SEE_ALLOCA(i,t,n) SEE_NEW_ARRAY(i,t,n)
#     undef SEE_STRING_ALLOCA
#     define SEE_STRING_ALLOCA(i,t,n) SEE_NEW_STRING_ARRAY(i,t,n)
#    endif
#   endif
#  endif
# endif
#endif
#endif


#endif /* _SEE_h_type_ */
