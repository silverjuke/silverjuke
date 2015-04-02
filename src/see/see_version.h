/* Copyright (c) 2005, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _SEE_h_version_
#define _SEE_h_version_

/*
 * Returns a static version string consisting of identifiers separated by
 * a single space character. 
 * The first word is the library package name (usually "see").
 * The second word is the library package version (e.g. "1.3").
 * Successive words are feature identifiers that indicate compile-time
 * options used when building the library.
 * This function is useful for end-users to determine the characteristics
 * of the library their application is using.
 */
const char *SEE_version(void);

/*
 * The major and minor version numbers indicate backward-compatible
 * and backward-incompatible changes to the API (i.e this set of
 * header files.). They are *indepenent* of the package and library
 * implementation version numbers.
 *
 * Practically, developers should use the following code to signal the 
 * rare case of major API changes when compiling:
 *    #if SEE_VERSION_API_MAJOR > 1
 *     #warning "SEE API major version mismatch"
 *    #endif
 * This precaution will signal when past use of older API may be
 * incompatible with newer APIs.
 *
 * The API versioning rules are:
 * + If a function, type, extern or macro has been changed since last
 *   release so that previous use is incompatible with future use, then 
 *   the major version number is incremented and the minor version number 
 *   is reset to zero. 
 * + If a function, type, extern or macro has been added since last
 *   release such that its future use is incompatible with previous headers
 *   with the same API major version number, then the minor version number
 *   is incremented.
 *
 * The API documentation should indicate at what API version new API
 * elements are added, defaulting to "1.0". 
 *
 */
#define SEE_VERSION_API_MAJOR	2
#define SEE_VERSION_API_MINOR	0

#endif /* _SEE_h_version */
