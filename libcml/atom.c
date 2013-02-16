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

CVSID("$Id: atom.c,v 1.13 2002/09/01 08:12:06 gnb Exp $");

/*============================================================*/

void
atom_ctor(cml_atom *a)
{
    if (a->type == A_STRING && a->value.string != 0)
    	a->value.string = g_strdup(a->value.string);
}

cml_atom *
atom_copy(const cml_atom *a)
{
    cml_atom *newa = g_new(cml_atom, 1);
    
    *newa = *a;
    atom_ctor(newa);
	
    return newa;
}

void
atom_dtor(cml_atom *a)
{
    if (a->type == A_STRING && a->value.string != 0)
    {
    	g_free(a->value.string);
	a->value.string = 0;
    }
}

void
atom_assign(cml_atom *to, const cml_atom *from)
{
    atom_dtor(to);
    *to = *from;
    atom_ctor(to);
}

void
atom_free(cml_atom *a)
{
    atom_dtor(a);
    g_free(a);
}

/*============================================================*/

const char *
atom_type_as_string(cml_atom_type type)
{
    switch (type)
    {
    case A_NONE: return "none";
    case A_HEXADECIMAL: return "hex";
    case A_DECIMAL: return "int";
    case A_STRING: return "string";
    case A_NODE: return "node";
    case A_BOOLEAN: return "bool";
    case A_TRISTATE: return "tristate";
    }
    return 0;
}

const char *
cml_atom_type_as_string(const cml_atom *ap)
{
    if (ap == 0)
    	return 0;

    return atom_type_as_string(ap->type);
}

/*============================================================*/

char *
cml_atom_value_as_string(const cml_atom *ap)
{
    if (ap == 0)
    	return g_strdup("");
	
    switch (ap->type)
    {
    case A_HEXADECIMAL:
    	return g_strdup_printf("0x%lX", ap->value.integer);
    case A_DECIMAL:
    	return g_strdup_printf("%ld", ap->value.integer);
    case A_STRING:
    	return g_strdup(ap->value.string == 0 ? "" : ap->value.string);
    case A_NODE:
    	return g_strdup(ap->value.node == 0 ? "" : ap->value.node->banner);
    case A_BOOLEAN:
    case A_TRISTATE:
    	switch (ap->value.tritval)
	{
	case CML_Y: return g_strdup("y");
	case CML_M: return g_strdup("m");
	case CML_N: return g_strdup("n");
	}
	break;
    default:
    	break;
    }
    return g_strdup("");
}

/*============================================================*/

gboolean
cml_atom_from_string(cml_atom *ap, const char *str)
{
    char *end = 0;
    
    if (ap == 0 || ap->type == A_NONE)
    	return FALSE;
	
    switch (ap->type)
    {
    case A_HEXADECIMAL:
    	ap->value.integer = strtoul(str, &end, 16);
	return (end != 0 && end != str && *end == '\0');
    case A_DECIMAL:
    	ap->value.integer = strtoul(str, &end, 10);
	return (end != 0 && end != str && *end == '\0');
    case A_STRING:
    	ap->value.string = g_strdup(str);
	return TRUE;
    case A_BOOLEAN:
    case A_TRISTATE:
    	if (!strcmp(str, "y"))
	    ap->value.tritval = CML_Y;
	else if (!strcmp(str, "m") && ap->type == A_TRISTATE)
	    ap->value.tritval = CML_M;
	else if (!strcmp(str, "n"))
	    ap->value.tritval = CML_N;
	else
	    return FALSE;
	return TRUE;
    default:
    	break;
    }
    return FALSE;
}

/*============================================================*/

int
_atom_compare(const cml_atom *left, const cml_atom *right)
{
    cml_atom promoted;
    
    cml_atom_init(&promoted);

    /* Do runtime type promotion where necessary */    
    /* TODO: be more generic about it */
    if (left->type == A_BOOLEAN && right->type == A_TRISTATE)
    {
    	/* promote left side */
	promoted.type = A_TRISTATE;
	promoted.value.tritval = left->value.tritval;
	left = &promoted;
    }
    else if (left->type == A_TRISTATE && right->type == A_BOOLEAN)
    {
    	/* promote right side */
	promoted.type = A_TRISTATE;
	promoted.value.tritval = right->value.tritval;
	right = &promoted;
    }
    
    /* A_NONE is "greater than" anything not A_NONE for CML1 compatibility */
    if (left->type == A_NONE && right->type != A_NONE)
    	return 1;
    else if (left->type != A_NONE && right->type == A_NONE)
    	return -1;
    
    assert(left->type == right->type);
    
    switch (left->type)
    {
    case A_NONE:
    	return 0;	    	/* all A_NONEs are equal */
    case A_HEXADECIMAL:
    case A_DECIMAL:
    	if (left->value.integer < right->value.integer)
	    return -1;
    	if (left->value.integer > right->value.integer)
	    return 1;
    	return 0;
    case A_STRING:
    	{
	    char *ls = left->value.string;
	    char *rs = right->value.string;
	    if (ls == 0)
	    	ls = "";
	    if (rs == 0)
	    	rs = "";
	    return strcmp(ls, rs);
    	}
    case A_BOOLEAN:
    case A_TRISTATE:
    	{
	    static int comp[3] = {
	    	0,  /* N */
		2,  /* Y */
		1   /* M */
	    };
	    return comp[left->value.tritval] - comp[right->value.tritval];
	}
    default:
    	assert(0);  	    /* TODO: smarter runtime exception mechanism */
    }
    /*NOTREACHED*/
}

/*============================================================*/
/*END*/
