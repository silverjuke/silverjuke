
#ifndef SEE_TYPE_MSW_H
#define SEE_TYPE_MSW_H


#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <string.h>


#pragma warning ( disable : 4018 ) //  Konflikt zwischen signed und unsigned
#pragma warning ( disable : 4244 ) // 'initializing' : Konvertierung von 'double ' in 'int ', moeglicher Datenverlust
#pragma warning ( disable : 4307 ) // Ueberlauf einer ganzzahligen Konstanten
#pragma warning ( disable : 4554 ) // Pruefen Sie Operatorvorrang auf moegliche Fehler; verwenden Sie runde Klammern, um den Vorrang zu verdeutlichen



#define __i386__ /*for dtoa.c*/
#define alloca					_alloca
#define snprintf				_snprintf
#define copysign				_copysign
#define isnan					_isnan
#define isinf(a)				(!_finite(a))
#define finite					_finite
#define rint(a)					floor((a)+0.5)

#define WITH_UNICODE_TABLES		1
#define HAVE_MEMCMP				1
#define HAVE_TIME				1
#define HAVE_LOCALTIME			1
#define HAVE_MKTIME				1


#define PACKAGE_VERSION			"2.0"


/* va_copy - see 
 * http://www.thescripts.com/forum/thread220717.html
 * http://www.codecomments.com/message415725.html (seems to be the same than above)
 */
//#define va_copy(DEST, SRC) memcpy((DEST), (SRC), sizeof(va_list))
#define va_copy(dst, src) ((void)((dst) = (src)))



/* Define misc values if they are not available from math.h. UNIX
 * systems typically have these defines, and MSWindows systems don't.
 */
#ifndef M_E
#define M_E 2.7182818284590452354
#endif /* !M_E */
#ifndef M_LOG2E
#define M_LOG2E 1.4426950408889634074
#endif /* !M_LOG2E */
#ifndef M_LOG10E
#define M_LOG10E 0.43429448190325182765
#endif /* !M_LOG10E */
#ifndef M_LN2
#define M_LN2 0.69314718055994530942
#endif /* !M_LN2 */
#ifndef M_LN10
#define M_LN10 2.30258509299404568402
#endif /* !M_LN10 */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif /* !M_PI */
#ifndef M_TWOPI
#define M_TWOPI (M_PI * 2.0)
#endif /* !M_TWOPI */
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif /* !M_PI_2 */
#ifndef M_PI_4
#define M_PI_4 0.78539816339744830962
#endif /* !M_PI_4 */
#ifndef M_3PI_4
#define M_3PI_4 2.3561944901923448370E0
#endif /* !M_3PI_4 */
#ifndef M_SQRTPI
#define M_SQRTPI 1.77245385090551602792981
#endif /* !M_SQRTPI */
#ifndef M_1_PI
#define M_1_PI 0.31830988618379067154
#endif /* !M_1_PI */
#ifndef M_2_PI
#define M_2_PI 0.63661977236758134308
#endif /* !M_2_PI */
#ifndef M_2_SQRTPI
#define M_2_SQRTPI 1.12837916709551257390
#endif /* !M_2_SQRTPI */
#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif /* !M_SQRT2 */
#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440
#endif /* !M_SQRT1_2 */
#ifndef M_LN2LO
#define M_LN2LO 1.9082149292705877000E-10
#endif /* !M_LN2LO */
#ifndef M_LN2HI
#define M_LN2HI 6.9314718036912381649E-1
#endif /* !M_LN2HI */
#ifndef M_SQRT3
#define M_SQRT3 1.73205080756887719000
#endif /* !M_SQRT3 */
#ifndef M_IVLN10
#define M_IVLN10 0.43429448190325182765 /* 1 / log(10) */
#endif /* !M_IVLN10 */
#ifndef M_LOG2_E
#define M_LOG2_E 0.693147180559945309417
#endif /* !M_LOG2_E */
#ifndef M_INVLN2
#define M_INVLN2 1.4426950408889633870E0 /* 1 / log(2) */
#endif /* !M_INVLN2 */


#define MAXMODULES 1 // save some memory, we do not used modues


/*

other, unused options:

WITH_PARSER_PRINT
WITH_PARSER_VISIT
WORDS_BIGENDIAN

*/


#endif /* SEE_TYPE_MSW_H */
