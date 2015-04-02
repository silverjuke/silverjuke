/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: dtoa.h 579 2004-02-10 08:54:36Z d $ */

#ifndef _SEE_h_dtoa_
#define _SEE_h_dtoa_

double	SEE_strtod(const char *s00, char **se);
char *	SEE_dtoa(double d, int mode, int ndigits, int *decpt, int *sign, 
		char **rve);
void	SEE_freedtoa(char *s);

#define DTOA_MODE_SHORT			0	/* shortest string */
#define DTOA_MODE_SHORT_SW		1	/* " w/ Steele & White rule */
#define DTOA_MODE_ECVT			2	/* max(1,ndigits) sig digits */
#define DTOA_MODE_FCVT			3	/* ndigits past decimal pt */
#define DTOA_MODE_SHORT_ECVT		4
#define DTOA_MODE_SHORT_FCVT		5
#define DTOA_MODE_ECVT_DEBUG		6
#define DTOA_MODE_FCVT_DEBUG		7
#define DTOA_MODE_SHORT_ECVT_DEBUG	8
#define DTOA_MODE_SHORT_FCVT_DEBUG	9

#endif /* _SEE_h_dtoa_ */
