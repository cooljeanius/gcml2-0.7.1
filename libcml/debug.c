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

#include "debug.h"

CVSID("$Id: debug.c,v 1.5 2002/09/01 08:39:36 gnb Exp $");


/*============================================================*/
#if DEBUG

unsigned int debug = 0;

static struct 
{
    const char *name;
    unsigned int value;
} const debug_tokens[] = {
{"lexer",   	DEBUG_LEXER},
{"parser",  	DEBUG_PARSER},
{"rules",  	DEBUG_RULES},
{"nodes",	DEBUG_NODES},
{"skipgui",	DEBUG_SKIPGUI},
{"convert",	DEBUG_CONVERT},
{"include",	DEBUG_INCLUDE},
{"gui",	    	DEBUG_GUI},
{"txn",	    	DEBUG_TXN},
{"lineno",  	DEBUG_LINENO},
{"load",  	DEBUG_LOAD},
{"dnf",  	DEBUG_DNF},
{"save",  	DEBUG_SAVE},
{"none",     	0},
{"all",     	~0},
{0, 0}
};

void
debug_set(const char *str)
{
    char *buf, *buf2, *x;
    int i;
    int dirn;
    
    buf = buf2 = g_strdup(str);
    debug = 0;
    
    while ((x = strtok(buf2, " \t\n\r,")) != 0)
    {
    	buf2 = 0;
	
    	if (*x == '+')
	{
	    dirn = 1;
	    x++;
	}
	else if (*x == '-')
	{
	    dirn = -1;
	    x++;
	}
	else
	    dirn = 1;

    	for (i = 0 ; debug_tokens[i].name ; i++)
	{
	    if (!strcmp(debug_tokens[i].name, x))
	    {
	    	if (dirn == 1)
		    debug |= debug_tokens[i].value;
		else
		    debug &= ~debug_tokens[i].value;
	    	break;
	    }
	}
	if (!debug_tokens[i].name)
	    fprintf(stderr, "Unknown debug token \"%s\", ignoring", x);
    }
    
    g_free(buf);
}

#else

void
debug_set(const char *str)
{
}

#endif /* !DEBUG */
/*============================================================*/
/*END*/
