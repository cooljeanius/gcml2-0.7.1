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
#include <malloc/malloc.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

CVSID("$Id: postparse.c,v 1.4 2002/09/01 08:22:01 gnb Exp $");

/*============================================================*/

static void
check_menu_node(gpointer key, gpointer value, gpointer user_data)
{
    cml_node *mn = (cml_node *)value;
    cml_rulebase *rb = (cml_rulebase *)user_data;
    
    if (mn->treetype == MN_UNKNOWN)
    {
    	if (mn->forward_refs != 0)
	{
    	    while (mn->forward_refs != 0)
	    {
		cml_location *loc = (cml_location *)mn->forward_refs->data;

		cml_errorl(loc,
		    "symbol `%s' used but not declared or derived.",
		    mn->name);
		g_free(loc);
		mn->forward_refs = g_list_remove_link(mn->forward_refs, mn->forward_refs);
	    }
	}
	else
	{
	    cml_errorl(&mn->location,
		    "symbol `%s' used but not declared or derived.",
		    mn->name);
	}
    }
    
    if (mn->treetype == MN_MENU)
    {
    	if (mn->expr_count > 0)
	    cml_errorl(&mn->location,
		"menu `%s' used in expressions.",
		mn->name);
	if (mn != rb->start && mn != rb->banner && mn->parent == 0)
	    cml_errorl(&mn->location,
		"menu `%s' not used in a menu.",
		mn->name);
	if (mn->saveability_expr != 0)
	    cml_errorl(&mn->location,
		"menu `%s' may not have saveability expression.",
		mn->name);
    }
    
    if (mn->treetype == MN_SYMBOL)
    {
	if (mn->parent == 0)
	    cml_errorl(&mn->location,
		"symbol `%s' not used in a menu.",
		mn->name);
		
	if (mn->visibility_expr != 0 &&
    	    expr_get_value_type(mn->visibility_expr) != A_BOOLEAN)
	{
	    cml_errorl(&mn->location,
		     "visibility expression for symbol `%s' must be boolean.",
		     mn->name);
	}
	
    }
    
    if (mn->treetype == MN_EXPLANATION)
    {
    	if (mn->expr_count > 0)
	    cml_errorl(&mn->location,
		"explanation `%s' used in expressions.",
		mn->name);
	if (mn->parent != 0)
	    cml_errorl(&mn->location,
		"explanation `%s' used in a menu.",
		mn->name);
	if (mn->saveability_expr != 0)
	    cml_errorl(&mn->location,
		"explanation `%s' may not have saveability expression.",
		mn->name);
    }
    
    if (mn->treetype == MN_DERIVED)
    {    
    	/* these only detect parser internal errors and so represent paranoia */
    	if (mn->expr == 0)
	    cml_errorl(&mn->location,
		"no derivation expression for derived symbol %s\n",
		mn->name);
	if (mn->visibility_expr != 0)
	    cml_errorl(&mn->location,
		"derived symbol %s may not have visibility expression",
		mn->name);
		
    	if (mn->value_type == A_NONE)
	{
	    /* TODO: this is a hack which doesn't handle the case of
	     *       multiple levels of derivation from a forward-
	     *       declared symbol.  To handle that would require
	     *       a full transitive closure of the derivation network.
	     */
	    cml_atom_type ty = expr_get_value_type(mn->expr);
	    if (ty != A_NONE)
	    {
	    	mn->value_type = ty;
	    }
	    else
	    {
		cml_errorl(&mn->location,
		    "sorry, derived symbol %s is derived from forward-declared derived symbol.\n",
		    mn->name);
	    }
	}
	
	if (mn->parent != 0)
	    cml_errorl(&mn->location,
		"derived symbol `%s' is used in a menu.",
		mn->name);
    }

#if 0
    /*
     * This test made sense when saveability was a fixed flag,
     * but with the general saveability predicate it's less useful.
     */
    if ((mn->treetype == MN_DERIVED || mn->treetype == MN_SYMBOL) &&
        mn->saveability_expr != 0 &&
        mn->expr_count == 0)
    {
	cml_errorl(&mn->location,
		"%s `%s' potentially unsaveable and not used in any expression or used as subtree guard",
		mn_get_treetype_as_string(mn),
		mn->name);
    }
#endif
}

#define is_logical(ty) \
    ((ty) == A_BOOLEAN || (ty) == A_TRISTATE)
#define is_integral(ty) \
    ((ty) == A_HEXADECIMAL || (ty) == A_DECIMAL)
#define are_comparable(ty1, ty2) \
    ((ty1) == (ty2) || \
     (is_logical(ty1) && is_logical(ty2)) || \
     (is_integral(ty1) && is_integral(ty2)))
     

/* TODO: this needs to be done in a more predictable order for error reporting */
static void
conv_dep_to_rule_2(cml_node *mn, GList *deelist)
{
    GList *kidlist;
    cml_expr *expr;
    cml_rule *rule;

    switch (mn->treetype)
    {
    case MN_MENU:
    	if (!cml_node_is_radio(mn))
	{
	    /* recurse */
	    for (kidlist = mn->children ; kidlist != 0 ; kidlist = kidlist->next)
		conv_dep_to_rule_2((cml_node *)kidlist->data, deelist);
	}
	break;
    case MN_SYMBOL:
	for ( ; deelist != 0 ; deelist = deelist->next)
	{
    	    cml_node *dee = (cml_node *)deelist->data;
	    
	    if (are_comparable(mn->value_type, dee->value_type))
	    {
    		expr = expr_new_composite(
	    		E_LESS_EQUALS,
	    		expr_new_symbol(mn),
	    		expr_new_symbol(dee));
		DDPRINTF1(DEBUG_CONVERT, "conv_dep_to_rule_2: %s\n", expr_as_string(expr)/*memleak*/);
    		rule = rule_new_require(expr);
		rb_add_rule(mn->rulebase, rule);
	    }
    	}
    	break;
    default:
    	break;
    }
}

static void
conv_dep_to_rule(gpointer key, gpointer value, gpointer user_data)
{
    cml_node *mn = (cml_node *)value;
    
    conv_dep_to_rule_2(mn, mn->dependees);
}

/* TODO: record the fact that errors have happened!!! */

static void
check_expr(const cml_location *loc, const cml_expr *expr)
{
    cml_atom_type at0, at1;
    int i;
    	
    switch (expr->type)
    {
    case E_OR:
    case E_AND:
    case E_IMPLIES:
	/*conv_dep_to_rule
	 * IV.3.18 Expressions
	 * [...]
	 * It is a compile-time error to apply the logical operators
	 * or/and/implies to trit or numeric values.  [...] The compiler does
	 * type propagation in expressions to check these constraints.
	 */
    	at0 = expr_get_value_type(expr->children[0]);
    	at1 = expr_get_value_type(expr->children[1]);
	if (at0 != A_BOOLEAN || at1 != A_BOOLEAN)
	{
	    cml_errorl(loc,
		     "operands to `%s' must be boolean%s.",
		     _expr_get_type_as_string(expr),
		     (at0 == A_TRISTATE || at1 == A_TRISTATE ?
		     	"; use (VAR!=n) for tristate variables" : ""));
	}
    	break;
    default:
    	break;
    }
	
    for (i=0 ; i<EXPR_MAX_CHILDREN ; i++)
	if (expr->children[i] != 0)
    	    check_expr(loc, expr->children[i]);
}


static void
check_rule(const cml_rule *rule)
{
    check_expr(&rule->location, rule->expr);

    if (expr_get_value_type(rule->expr) != A_BOOLEAN)
    {
	cml_errorl(&rule->location,
		 "expression in rule must be boolean.");
    }
}


#define rb_condition_type(rb, feature) \
    ((rb)->features[(feature)].tie != 0 ? \
    	(rb)->features[(feature)].tie->value_type : \
	(rb)->features[(feature)].value.type)
	
#define rb_condition_is_tied(rb, feature) \
    ((rb)->features[(feature)].tie != 0)

extern void cml1_pass2(cml_rulebase *rb);

gboolean
cml_rulebase_post_parse(cml_rulebase *rb)
{
    GList *list;
    cml_atom_type atype;
    int old_nerrs = cml_message_count[CML_ERROR];

    if (rb->merge_mode)
    	cml1_pass2(rb);     /* HACK HACK HACK */

    
    /* check start symbol */
    if (rb->start == 0)
    	cml_errorl(0, "rulebase contains no \"start\" statement.");
    else if (rb->start->treetype != MN_MENU)
	cml_errorl(&rb->start_loc, "expecting menu argument to \"start\" statement, got %s.",
	    	    mn_get_treetype_as_string(rb->start));


    /* check specific conditions */
    atype = rb_condition_type(rb, RBF_TRITS);
    if (atype != A_NONE && !is_logical(atype))
    {
    	cml_errorl(&rb->features[RBF_TRITS].location,
	    	   "%s for condition trits must be boolean or tristate.",
		   (rb_condition_is_tied(rb, RBF_TRITS) ? "symbol" : "constant"));
    }
    
    atype = rb_condition_type(rb, RBF_NOHELP);
    if (atype != A_NONE && !is_logical(atype))
    {
    	cml_errorl(&rb->features[RBF_NOHELP].location,
	    	   "%s for condition nohelp must be boolean or tristate.",
		   (rb_condition_is_tied(rb, RBF_NOHELP) ? "symbol" : "constant"));
    }

    
    /* check menu nodes */
    g_hash_table_foreach(rb->menu_nodes, check_menu_node, rb);
    
    /* convert dependencies into rules */
    g_hash_table_foreach(rb->menu_nodes, conv_dep_to_rule, rb);
    
    /* check rules */
    for (list = rb->rules ; list != 0 ; list = list->next)
    {
    	cml_rule *rule = (cml_rule *)list->data;
	
	check_rule(rule);

	/*
	 * This has to be done in pass 2 to allow rules
	 * to use forward-declared derived symbols.
	 */
	_expr_add_using_rule_recursive(rule->expr, rule);
    }
    
    return (cml_message_count[CML_ERROR] == old_nerrs);
}

/*============================================================*/
/*END*/
