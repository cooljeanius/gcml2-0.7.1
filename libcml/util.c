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
 
#include "util.h"
#include "common.h"
#include "debug.h"
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

CVSID("$Id: util.c,v 1.6 2002/09/01 08:23:19 gnb Exp $");

/*============================================================*/

int
build_directories(const char *filename)
{
    int ret = 0;
    char *end, *buf = g_strdup(filename);
    struct stat sb;
    
    end = strchr(buf, '/');
    if (end == 0)
    {
    	g_free(buf);
	return 0;
    }
    
    do
    {
	*end = '\0';
	
	DDPRINTF1(DEBUG_SAVE, "    build_directories: checking %s\n", buf);
	if (stat(buf, &sb) < 0 && errno == ENOENT)
	{
	    DDPRINTF1(DEBUG_SAVE, "    build_directories: mkdir(%s)\n", buf);
	    if (mkdir(buf, 0777) < 0)
	    {
		ret = -1;
		break;
	    }
	}
    	
	*end = '/';
	end = strchr(end+1, '/');
    } while (end != 0);
    
    g_free(buf);
    return ret;
}

/*============================================================*/

#if SYLVESTER

#define CAGE	32
#define MAGIC1	0xdeadbeef
#define MAGIC2	0xbdefaced
#define TWEETY	0xa5

extern void *__libc_malloc(size_t);
extern void *__libc_realloc(void *, size_t);
extern void __libc_free(void *);

struct header
{
    uint32_t	magic;
    uint32_t	size;
};

void *
malloc(size_t sz)
{
    void *x;
    if ((x = __libc_malloc(sizeof(struct header) + sz + CAGE)) == 0)
    	return 0;
    ((struct header *)x)->magic = MAGIC1 ^ sz;
    ((struct header *)x)->size = sz;
    memset((char *)x + sizeof(struct header) + sz, TWEETY, CAGE);
    return ((char *)x + sizeof(struct header));
}

void *
calloc(size_t unit, size_t qty)
{
    size_t sz = unit * qty;
    void *x;
    if ((x = __libc_malloc(sizeof(struct header) + sz + CAGE)) == 0)
    	return 0;
    ((struct header *)x)->magic = MAGIC1 ^ sz;
    ((struct header *)x)->size = sz;
    memset((char *)x + sizeof(struct header), 0, sz);
    memset((char *)x + sizeof(struct header) + sz, TWEETY, CAGE);
    return ((char *)x + sizeof(struct header));
}

void *
realloc(void *x, size_t sz)
{
    if (x != 0)
    {
	sylvester(x);
    	x = (void *)((char *)x - sizeof(struct header));
    }
    if ((x = __libc_realloc(x, sizeof(struct header) + sz + CAGE)) == 0)
    	return 0;
    ((struct header *)x)->magic = MAGIC1 ^ sz;
    ((struct header *)x)->size = sz;
    memset((char *)x + sizeof(struct header) + sz, TWEETY, CAGE);
    return ((char *)x + sizeof(struct header));
}

void
free(void *x)
{
    sylvester(x);
    *(gulong *)x = MAGIC2;
    __libc_free((char *)x - sizeof(struct header));
}

void
sylvester(void *x)
{
    unsigned char *cage;
    struct header *h = ((struct header *)x) - 1;
    
    assert(x != 0);
    assert(*(gulong *)x != MAGIC2); 	    /* not already freed */
    assert((h->magic ^ h->size) == MAGIC1); /* header not corrupt */
    cage = (unsigned char *)x + h->size;
    assert(cage[0] == TWEETY);	    	    /* trailer corrupt */
    assert(cage[1] == TWEETY);
    assert(cage[2] == TWEETY);
    assert(cage[3] == TWEETY);
    assert(cage[4] == TWEETY);
    assert(cage[5] == TWEETY);
    assert(cage[6] == TWEETY);
    assert(cage[7] == TWEETY);
}

#endif

/*============================================================*/
/*END*/
