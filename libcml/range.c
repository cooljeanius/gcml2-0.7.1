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
 
#include "private.h"

CVSID("$Id: range.c,v 1.3 2001/04/05 21:18:11 gnb Exp $");

/*============================================================*/

#define in_subrange(sr, x) \
    ((x) >= (sr)->begin && (x) <= (sr)->end)

static cml_subrange *
subrange_new(unsigned long begin, unsigned long end)
{
    cml_subrange *sr = g_new(cml_subrange, 1);

    if (sr == 0)
    	return 0;
    sr->begin = begin;
    sr->end = end;

    return sr;
}

/*============================================================*/


cml_range *
range_new(unsigned long begin, unsigned long end)
{
    cml_subrange *sr = subrange_new(begin, end);
    return (sr == 0 ? 0 : g_list_append(0, sr));
}

void
range_delete(cml_range *range)
{
    while (range != 0)
    {
    	cml_subrange *sr = (cml_subrange *)range->data;
	g_free(sr);
    	range = g_list_remove_link(range, range);
    }
}

/*============================================================*/

#if 0
static GList *
g_list_insert_after(GList *list, gpointer data, GList *after)
{
    GList *link = g_list_alloc();

    link->data = data;
    
    if (after == 0)
    {
    	/* prepend */
	link->next = list;
	list = link;
    }
    else
    {
    	/* insert after `after' */
	link->next = after->next;
	after->next = link;
    }
    link->prev = after;
    if (link->next != 0)
	link->next->prev = link;
    
    return list;
}
#endif
static GList *
g_list_insert_before(GList *list, gpointer data, GList *before)
{
    GList *link = g_list_alloc();

    link->data = data;
    
    if (before == 0)
    {
    	/* append */
	link->prev = g_list_last(list);
    }
    else
    {
    	/* insert before `before' */
	link->prev = before->prev;
	before->prev = link;
    }
    link->next = before;
    if (link->prev != 0)
	link->prev->next = link;
    else
    	list = link;
    
    return list;
}

/*============================================================*/


cml_range *
range_add(cml_range *range, unsigned long begin, unsigned long end)
{
    GList *link, *next;
    cml_subrange *sr, *first;

    if (range == 0)
    	return range_new(begin, end);


    /* search for sr containing begin */
    first = 0;
    for (link=range ; link != 0 ; link=link->next)
    {
    	sr = (cml_subrange *)link->data;

	if (in_subrange(sr, begin))
	{
	    first = sr;
	    break;
	}
	if (begin == sr->end+1)
	{
	    first = sr;
	    first->end++;
	    break;
	}
	if (begin < sr->begin)
	{
	    first = subrange_new(begin, begin);
	    range = g_list_insert_before(range, first, link);
	    break;
	}
    }
    if (first == 0)
    {
    	/* append sr */
	return g_list_append(range, subrange_new(begin, end));
    }
    assert(first != 0);
    assert(in_subrange(first, begin));
    
    /* search for sr containing `end', merging into `first' as we go */
    for ( ; link != 0 ; link=next)
    {
    	sr = (cml_subrange *)link->data;

    	if (end+1 < sr->begin)
	{
	    /* found sr after `end' */
	    first->end = end;
	    return range;
	}
	/* merge this sr into first */
	next = link->next;
	if (sr != first)
	{
	    first->end = sr->end;
	    range = g_list_remove_link(range, link);
	    g_free(sr);
	}
    }
    if (first->end < end)
	first->end = end;
        
    return range;
}

/*============================================================*/

unsigned long
range_count(const cml_range *range)
{
    unsigned long count = 0;
    const GList *list;
    
    for (list=range ; list!=0 ; list=list->next)
    {
    	cml_subrange *sr = (cml_subrange *)list->data;
	count += sr->end - sr->begin + 1;
    }
    return count;
}

/*============================================================*/

gboolean
range_check(const cml_range *range, unsigned long x)
{
    const GList *list;
    
    for (list=range ; list!=0 ; list=list->next)
    {
    	cml_subrange *sr = (cml_subrange *)list->data;
	if (in_subrange(sr, x))
	    return TRUE;
    }
    return FALSE;
}

/*============================================================*/

void
range_dump(const cml_range *range, FILE *fp)
{
    const GList *list;
    
    fputc('{', fp);
    for (list=range ; list!=0 ; list=list->next)
    {
    	cml_subrange *sr = (cml_subrange *)list->data;
	fprintf(fp, "%s{%ld,%ld}",
	    (list == range ? "" : ","),
	    sr->begin,
	    sr->end);
    }
    fputc('}', fp);
}


/*============================================================*/
#ifdef RANGE_TEST

#include <string.h>
#include <stdlib.h>

#define MAX_NARGS 3

int
main(int argc, char **argv)
{
    cml_range *range = 0;
    unsigned long x, y;
    int nargs;
    char *args[MAX_NARGS];
    char buf[1024];
    
    while ((fgets(buf, sizeof(buf), stdin)) != 0)
    {
    	if (buf[0] == '#')
	{
	    /* echo all comments to output -- makes diffs more readable */
	    fputs(buf, stdout);
	    continue;
	}
	
    	/* parse line into cmdargs[] */
    	nargs = 0;
	while (nargs < MAX_NARGS &&
	       (args[nargs] = strtok((nargs ? 0 : buf), " \t\r\n")) != 0)
	    nargs++;

    	if (nargs == 0)
	    continue;	    /* empty line */

    	if (!strcmp(args[0], "delete"))
	{
	    if (nargs != 1)
	    {
	    	fprintf(stderr, "syntax: delete\n");
	    	continue;
	    }
	    if (range != 0)
	    {
	    	range_delete(range);
	    	range = 0;
	    }
	    printf("deleted\n");
	}
	else if (!strcmp(args[0], "add"))
	{
	    if (nargs != 3)
	    {
	    	fprintf(stderr, "syntax: add <begin> <end>\n");
	    	continue;
	    }
	    x = atoi(args[1]);
	    y = atoi(args[2]);
	    printf("added: {%ld,%ld} + ", x, y);
	    range_dump(range, stdout);
	    range = range_add(range, x, y);
	    fputs(" -> ", stdout);
	    range_dump(range, stdout);
	    fputs("\n", stdout);
	}
	else if (!strcmp(args[0], "check"))
	{
	    if (nargs != 2)
	    {
	    	fprintf(stderr, "syntax: check <num>\n");
	    	continue;
	    }
	    x = atoi(args[1]);
	    printf("check %ld: %s\n",
	    	x,
	    	(range_check(range, x) ? "true" : "false"));
	}
	else if (!strcmp(args[0], "count"))
	{
	    if (nargs != 1)
	    {
	    	fprintf(stderr, "syntax: count\n");
	    	continue;
	    }
	    printf("count: %ld\n", range_count(range));
	}
	else if (!strcmp(args[0], "help"))
	{
	    fputs("\
Commands are:\n\
delete	    	    delete current range\n\
add <begin> <end>   add to current range\n\
check <num> 	    check if num is in current range\n\
count	    	    show count of current range\n\
", stdout);
	}
	else
	{
	    fprintf(stderr, "unknown command `%s'\n", args[0]);
	}
    }
    return 0;
}

#endif

/*============================================================*/
/*END*/
