/*
 *  gcml2 -- an implementation of Eric Raymond's CML2 in C
 *  Copyright (C) 2000-2001 Greg Banks
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
#ifndef _strvec_h_
#define _strvec_h_

#include <glib.h>

/* strvec.c */
typedef struct
{
    char **items;
    int nitems;
    int nalloc;
} strvec_t;

void strvec_init(strvec_t *sv);
void strvec_truncate(strvec_t *sv);
void strvec_free(strvec_t *sv);
void strvec_preallocate(strvec_t *sv, int n);
void strvec_append(strvec_t *sv, const char *str);
void strvec_appendm(strvec_t *sv, /*already saved*/char *str);
void strvec_appendf(strvec_t *sv, const char *fmt, ...);


#endif /* _strvec_h_ */
