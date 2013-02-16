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
#include "debug.h"
#include "util.h"
#include <ctype.h>

CVSID("$Id: load.c,v 1.7 2002/09/01 08:19:14 gnb Exp $");

/*============================================================*/

static gboolean
rb_parse(cml_rulebase *rb, FILE *fp)
{
    char *p, *x;
    char *name;
    char *value;
    int preflen = (rb->prefix == 0 ? 0 : strlen(rb->prefix));
    cml_atom a;
    cml_node *mn;
    char buf[1024];
    
    while (fgets(buf, sizeof(buf), fp) != 0)
    {
    	/* strip off trailing newline */
	if ((p = strrchr(buf, '\n')) != 0)
	    *p = '\0';
	if ((p = strrchr(buf, '\r')) != 0)
	    *p = '\0';
	    
	/* skip any initial whitespace */
    	for (p = buf ; *p && isspace(*p) ; p++)
	    ;
	if (!*p)
	    continue;	/* ignore empty lines */

    	if (*p == '#')
	{
	    if (strstr(p, "is not set") == 0)
	    	continue;   /* normal comment */
	    if (preflen)
	    {
	    	/* use the prefix to find the symbol name */
		if ((name = strstr(p, rb->prefix)) == 0)
		    continue;
		name += preflen;
	    }
	    else
	    {
	    	/* assume the symbol name is after leading '#' and whitespace */
    		for (p++ ; *p && isspace(*p) ; p++)
		    ;
		if (!isalpha(*p) && *p != '_')
		    continue;
		name = p;
	    }
	    /* scan for end of name */
	    for (x = name ; *x && !isspace(*x) ; x++)
	    	;
	    if (!*x)
	    	continue;
	    *x = '\0';	/* terminate name */
	    value = "n";
	}
	else
	{
	    /* CONFIG_FOO=value */
	    if (preflen)
	    {
	    	/* check the symbol has the prefix */
		if (strncmp(p, rb->prefix, preflen))
		    continue;
		name = p + preflen;
	    }
	    else
	    {
	    	name = p;
	    }
	    if ((value = strchr(name, '=')) == 0)
	    	continue;
	    /* trim whitespace off end of name */
    	    for (x = value - 1 ; x > name && isspace(*x) ; --x)
	    	*x = '\0';
	    /* seperate name and value */
	    *value++ = '\0';
	    /* trim whitespace off start of value */
	    while (*value && isspace(*value))
	    	value++;
	    if (!*value)
	    	continue;
	    /* handle quoted value */
	    if (*value == '"')
	    {
	    	value++;
		if ((x = strchr(value, '"')) == 0)
		    continue;
		*x = '\0';
	    }
	    else
	    {
		/* trim whitespace off end of value */
    		for (x = value + strlen(value) ; x != value && isspace(*x) ; --x)
	    	    *x = '\0';
    	    }
	}
	
	/* add binding */
	if ((mn = cml_rulebase_find_node(rb, name)) == 0)
	    continue;
	if (cml_node_get_treetype(mn) != MN_SYMBOL)
	    continue;

	cml_atom_init(&a);
	a.type = cml_node_get_value_type(mn);
	if (!cml_atom_from_string(&a, value))
	    continue;

	DDPRINTF2(DEBUG_LOAD, "`%s'=`%s'\n", name, value);
	mn_set_value(mn, &a, rb->start);
	mn->flags |= MN_LOADED;
    }
    return TRUE;
}


gboolean
cml_rulebase_load_defconfig(cml_rulebase *rb, const char *filename)
{
    FILE *fp;
    gboolean ret;
    
    if ((fp = fopen(filename, "r")) == 0)
    {
    	cml_perror(filename);
    	return FALSE;
    }

    ret = rb_parse(rb, fp);
        
    fclose(fp);
    
    return ret;
}

/*============================================================*/
/*END*/
