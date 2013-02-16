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

#include "cml1.h"
#if OVERLAP_DNF
#include "dnf.h"
#endif
#include "debug.h"

CVSID("$Id: dnf.c,v 1.2 2002/09/01 08:18:50 gnb Exp $");

#if OVERLAP_DNF

/*============================================================*/

static dnf_t *
dnf_leaf(const cml_expr *expr)
{
    return g_list_append(0, g_list_append(0, (void *)expr));
}

/*============================================================*/

void
dnf_free(dnf_t *dnf)
{
    while (dnf != 0)
    {
    	while (dnf->data != 0)
	    dnf->data = g_list_remove_link(dnf->data, dnf->data);
	dnf = g_list_remove_link(dnf, dnf);
    }
}

/*============================================================*/

static int
compare_strs(const void *v1, const void *v2)
{
    return strcmp(*(const char **)v1, *(const char **)v2);
}

static char *
dnf_clause_string(const GList *clause)
{
    GPtrArray *pa = g_ptr_array_new();

    for ( ; clause != 0 ; clause = clause->next)
    {
	cml_expr *expr = (cml_expr *)clause->data;
	cml_expr fake_expr;

	if (((unsigned long)expr)&1)
	{
	    expr = (cml_expr *)(((unsigned long)expr) & ~1);
	    fake_expr = *expr;
	    switch (expr->type)
	    {
	    case E_EQUALS: fake_expr.type = E_NOT_EQUALS; break;
	    case E_NOT_EQUALS: fake_expr.type = E_EQUALS; break;
	    default: assert(0);
	    }
	    expr = &fake_expr;
	}
    	g_ptr_array_add(pa, expr_as_string(expr));
    }
    g_ptr_array_add(pa, 0);
	
    qsort(pa->pdata, pa->len-1, sizeof(char*), compare_strs);
    /* TODO: major memleak */
    return g_strjoinv(" and ", (char **)pa->pdata);
}

/*============================================================*/

static dnf_t *
dnf_concatenate(dnf_t *d1, dnf_t *d2)
{
    if (d1 == 0 || d2 == 0)
    {
    	if (d1 != 0)
	    dnf_free(d1);
    	if (d2 != 0)
	    dnf_free(d2);
    	return 0;
    }

    return g_list_concat(d1, d2);
}

/*============================================================*/

static dnf_t *
dnf_convolve(dnf_t *d1, dnf_t *d2)
{
    GList *iter1, *iter2;
    dnf_t *dnf = 0;

    if (d1 == 0 || d2 == 0)
    {
    	if (d1 != 0)
	    dnf_free(d1);
    	if (d2 != 0)
	    dnf_free(d2);
    	return 0;
    }

    for (iter1 = d1 ; iter1 != 0 ; iter1 = iter1->next)
    {
	for (iter2 = d2 ; iter2 != 0 ; iter2 = iter2->next)
	{
	    dnf = g_list_append(dnf,
	    	    	g_list_concat(
			    g_list_copy(iter1->data),
			    g_list_copy(iter2->data)));
	}
    }

    dnf_free(d1);
    dnf_free(d2);
    
    return dnf;
}

/*============================================================*/
/*
 * Note that we use a truly blecherous data structure hack
 * for noting negation of atoms in dnf_t's: the low bit of
 * the cml_expr pointer in inverted.  This relies on the
 * cml_expr pointer from malloc being aligned to at least
 * a 2-byte boundary.
 */

static dnf_t *
dnf_invert(dnf_t *d)
{
    GList *citer, *aiter;
    dnf_t *res = 0;

    if (d == 0)
    	return 0;
	
    /* for each conjunctive clause */
    for (citer = d ; citer != 0 ; citer = citer->next)
    {
	GList *oldres = res;
	res = 0;

	/* for each atom in a conjunctive clause */
	for (aiter = (GList *)citer->data ; aiter != 0 ; aiter = aiter->next)
	{
	    if (oldres == 0)
	    {
	        GList *cc = 0;
		cc = g_list_append(cc,
		    	    (void *)(((unsigned long)aiter->data)^1));
	    	res = g_list_append(res, cc);
	    }
	    else
	    {
	    	GList *ociter;
		for (ociter = oldres ; ociter != 0 ; ociter = ociter->next)
		{
		    GList *cc = g_list_copy(ociter->data);
		    cc = g_list_append(cc,
		    	    	(void *)(((unsigned long)aiter->data)^1));
	    	    res = g_list_append(res, cc);
		}
	    }
	}
	if (oldres != 0)
	    dnf_free(oldres);
    }
    dnf_free(d);

    return res;
}

/*============================================================*/

static dnf_t *
dnf_from_expr_2(const cml_expr *expr)
{
    dnf_t *dleft, *dright, *dnf;

    if (expr == 0)
    	return 0;

    switch (expr->type)
    {
    case E_AND:
   	dleft = dnf_from_expr_2(expr->children[0]);
    	dright = dnf_from_expr_2(expr->children[1]);
    	dnf = dnf_convolve(dleft, dright);
	break;

    case E_OR:
   	dleft = dnf_from_expr_2(expr->children[0]);
    	dright = dnf_from_expr_2(expr->children[1]);
    	dnf = dnf_concatenate(dleft, dright);
	break;
	
    case E_NOT:
   	dleft = dnf_from_expr_2(expr->children[0]);
    	dnf = dnf_invert(dleft);
    	break;
    
    case E_EQUALS:
    case E_NOT_EQUALS:
    	/* time for a spot of paranoia */
#if 1
    	if (expr->children[0]->type == E_SYMBOL)
	{
	    assert(expr->children[1]->type == E_ATOM);
	    assert(!strcmp(expr->children[0]->symbol->name, "ARCH") || expr->children[1]->value.type == A_TRISTATE);
	}
	else if (expr->children[1]->type == E_SYMBOL)
	{
	    assert(expr->children[0]->type == E_ATOM);
	    assert(!strcmp(expr->children[1]->symbol->name, "ARCH") || expr->children[0]->value.type == A_TRISTATE);
	}
	else
	    /* this used to be an assert but the CONFIG_DECSTATION bug triggered it */
	    return 0;
#endif
	dnf = dnf_leaf(expr);
	break;
    
    default:
    	assert(0);
	break;
    }
    
    return dnf;
}

dnf_t *
dnf_from_expr(const cml_expr *expr)
{
    dnf_t *dnf = dnf_from_expr_2(expr);

#if DEBUG
    if ((debug & DEBUG_DNF))
    {
	char *s;
	GList *citer;

	s = expr_as_string(expr);
	DDPRINTF1(DEBUG_DNF, "expr = \"%s\" => dnf = \n", s);
	g_free(s);
	
	DDPRINTF0(DEBUG_DNF, "(\n");
	for (citer = dnf ; citer != 0 ; citer = citer->next)
	{
	    s = dnf_clause_string(citer->data);
	    DDPRINTF2(DEBUG_DNF, " %s (%s)\n", (citer == dnf ? "  " : "or"), s);
	    g_free(s);
	}
	DDPRINTF0(DEBUG_DNF, ")\n");
    }
#endif

    return dnf;
}

/*============================================================*/

GList *
dnf_overlap(const dnf_t *d1, const dnf_t *d2)
{
    const GList *iter1, *iter2;
    GList *varbs = 0;

    if (d1 == 0)
    {
	for (iter2 = d2 ; iter2 != 0 ; iter2 = iter2->next)
	    varbs = g_list_prepend(varbs, dnf_clause_string(iter2->data));
    }
    else if (d2 == 0)
    {
	for (iter1 = d1 ; iter1 != 0 ; iter1 = iter1->next)
	    varbs = g_list_prepend(varbs, dnf_clause_string(iter1->data));
    }
    else
    {
	for (iter1 = d1 ; iter1 != 0 ; iter1 = iter1->next)
	{
    	    char *s1 = dnf_clause_string(iter1->data);

	    for (iter2 = d2 ; iter2 != 0 ; iter2 = iter2->next)
	    {
    		char *s2 = dnf_clause_string(iter2->data);

    		if (!strcmp(s1, s2))
	    	    varbs = g_list_prepend(varbs, s2);
		else
		    g_free(s2);
	    }

	    g_free(s1);
    	}
    }
    
    return g_list_reverse(varbs);
}

/*============================================================*/
#endif /* OVERLAP_DNF */
/*END*/
