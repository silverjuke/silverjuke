/* Copyright (c) 2005, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _h_dprint_
#define _h_dprint_

#ifndef NDEBUG

struct SEE_string;
struct SEE_interpreter;
struct SEE_value;
struct SEE_object;

void SEE_dprintf(const char *fmt, ...);
void SEE_dprints(const struct SEE_string *s);
void SEE_dprintv(struct SEE_interpreter *interp, const struct SEE_value *v);
void SEE_dprinto(struct SEE_interpreter *interp, struct SEE_object *o);

# define dprintf SEE_dprintf
# define dprints SEE_dprints
# define dprintv SEE_dprintv
# define dprinto SEE_dprinto

#endif

#endif /* _h_dprint_ */

