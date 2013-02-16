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
 
#ifndef _cml2_common_h_
#define _cml2_common_h_ 1

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>


#define CVSID(s) \
    static const char *__cvsid[2] = { (s), (const char *)__cvsid }
#define CVSID2(s) \
    static const char *__cvsid2[2] = { (s), (const char *)__cvsid2 }
    
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define strassign(v, s) \
    do { \
	if ((v) != 0) \
	    g_free((v)); \
	(v) = g_strdup((s)); \
    } while(0)
    
#define strdelete(v) \
    do { \
    	if ((v) != 0) \
	    g_free((v)); \
	(v) = 0; \
    } while(0)

#define strnullnorm(v) \
    do { \
	if ((v) != 0 && *(v) == '\0') \
	{ \
	    g_free((v)); \
	    (v) = 0; \
	} \
    } while(0)

#define listdelete(v,type,dtor) \
    do { \
	while ((v) != 0) \
	{ \
    	    dtor((type *)(v)->data); \
    	    (v) = g_list_remove_link((v), (v)); \
	} \
    } while(0)

#define listclear(v) \
    do { \
	while ((v) != 0) \
	{ \
    	    (v) = g_list_remove_link((v), (v)); \
	} \
    } while(0)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#ifndef SYLVESTER
#define SYLVESTER 0
#endif

#if SYLVESTER
extern void sylvester(void *);
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


#endif /* _cml2_common_h_ */
