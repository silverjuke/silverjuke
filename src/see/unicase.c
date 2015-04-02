/*
 * Copyright (c) 2005
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
/* $Id$ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

# include "see_unicode.h"

#if !WITH_UNICODE_TABLES

 #warning "Unicode tables are omitted; library will not be ECMA-262 compliant"

#else /* WITH_UNICODE_TABLES */

struct case_map {
    SEE_char_t from, to;
};

/* Prototypes */
static SEE_char_t search(struct case_map *, unsigned int, unsigned int);

# include "see_unicase.inc"

# define lengthof(a) (sizeof a / sizeof (a)[0])

/* Binary search a map for the character. Usually done in 9 steps. */
static SEE_char_t
search(map, ch, maplen)
	struct case_map *map;
	unsigned int ch;		/* promoted from SEE_char_t */
	unsigned int maplen;
{
	unsigned int a, b, m;

	a = 0;
	b = maplen;
	for (;;) {
	    m = (a + b) / 2;
	    if (map[m].from == ch)
	    	return map[m].to;
	    if (map[m].from > ch) {
		if (b == m) break;
	    	b = m; 
	    } else  {
		if (a == m) break;
	    	a = m;
	    }
	}
	return ch;
}

SEE_char_t
SEE_unicase_tolower(ch)
	unsigned int ch;		/* promoted from SEE_char_t */
{
	return search(lowercase_map, ch, lengthof(lowercase_map));
}

SEE_char_t
SEE_unicase_toupper(ch)
	unsigned int ch;		/* promoted from SEE_char_t */
{
	return search(uppercase_map, ch, lengthof(uppercase_map));
}

#endif /* WITH_UNICODE_TABLES */
