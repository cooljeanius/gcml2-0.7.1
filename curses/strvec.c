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
 
/*
 * Routines to implement a data structure comprising
 * an expanding array of strings.  This sacrifices
 * some efficiency (from many copies of strings,
 * and potentially resizing) for maintainability.
 */
 
#include "common.h"
#include <memory.h>

CVSID("$Id: strvec.c,v 1.1 2001/05/23 03:55:53 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
strvec_init(strvec_t *sv)
{
    sv->items = 0;
    sv->nitems = 0;
    sv->nalloc = 0;
}

void
strvec_truncate(strvec_t *sv)
{
    int i;
    
    for (i = 0 ; i < sv->nitems ; i++)
    	g_free(sv->items[i]);
	
    sv->nitems = 0;
}


void
strvec_free(strvec_t *sv)
{
    int i;
    
    for (i = 0 ; i < sv->nitems ; i++)
    	g_free(sv->items[i]);

    if (sv->items != 0)
    {
    	g_free(sv->items);
    	sv->items = 0;
    }
}

static void
_strvec_expand(strvec_t *sv, int n)
{
    char **old = sv->items;
    
    sv->nalloc = n;
    sv->items = g_new(char*, n);
    
    if (old != 0)
    {
    	if (sv->nitems > 0)
    	    memcpy(sv->items, old, sizeof(char*)*sv->nitems);
    	g_free(old);
    }
}

void
strvec_preallocate(strvec_t *sv, int n)
{
    if (n > sv->nalloc)
    	_strvec_expand(sv, n);
}


void
strvec_appendm(strvec_t *sv, char *str)
{
    if (sv->nitems == sv->nalloc)
    	_strvec_expand(sv, sv->nalloc+16);
    sv->items[sv->nitems++] = str;
}

void
strvec_append(strvec_t *sv, const char *str)
{
    strvec_appendm(sv, g_strdup(str));
}

void
strvec_appendf(strvec_t *sv, const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    strvec_appendm(sv, g_strdup_vprintf(fmt, args));
    va_end(args);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
