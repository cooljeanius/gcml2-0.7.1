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

CVSID("$Id: node.c,v 1.18 2002/09/01 08:24:36 gnb Exp $");

/*============================================================*/

cml_enumdef *
cml_enumdef_new(cml_node *symbol, unsigned long value)
{
    cml_enumdef *ed;
    
    ed = g_new(cml_enumdef, 1);
    memset(ed, 0, sizeof(*ed));
    
    ed->symbol = symbol;
    ed->value = value;
    
    return ed;
}

void
cml_enumdef_delete(cml_enumdef *ed)
{
    g_free(ed);
}

/*============================================================*/

cml_node *
mn_new(const char *name)
{
    cml_node *mn = g_new(cml_node, 1);
    static unsigned long last_uniqueid = 0;
    
    if (mn == 0)
    	return 0;
    memset(mn, 0, sizeof(*mn));
    mn->treetype = MN_UNKNOWN;
    mn->flags = 0;
    mn->name = g_strdup(name);
    mn->uniqueid = ++last_uniqueid;
    return mn;
}

void
mn_delete(cml_node *mn)
{
    strdelete(mn->name);
    strdelete(mn->banner);
    listclear(mn->rules_using);
    if (mn->visibility_expr != 0)
    	expr_destroy(mn->visibility_expr);
    if (mn->saveability_expr != 0)
    	expr_destroy(mn->saveability_expr);
    /* only remove the list structure, all nodes are deleted seperately */
    listclear(mn->children);
    if (mn->expr != 0)
    	expr_destroy(mn->expr);
    atom_dtor(&mn->value);
    listclear(mn->transactions_guarded);
    listclear(mn->bindings);
    range_delete(mn->range);
    listdelete(mn->enumdefs, cml_enumdef, cml_enumdef_delete);
    listclear(mn->dependants);
    listclear(mn->dependees);
    strdelete(mn->help_text);
    g_free(mn);
}

/*============================================================*/

cml_node_treetype
cml_node_get_treetype(const cml_node *mn)
{
    return mn->treetype;
}

const char *
mn_get_treetype_as_string(const cml_node *mn)
{
    return mn_treetype_as_string(mn->treetype);
}

const char *
mn_treetype_as_string(cml_node_treetype tt)
{
    switch (tt)
    {
    case MN_UNKNOWN: return "unknown";
    case MN_SYMBOL: return "symbol";
    case MN_DERIVED: return "derived symbol";
    case MN_MENU: return "menu";
    case MN_EXPLANATION: return "explanation";
    }
    return "<unknown>";
}

gboolean
cml_node_is_radio(const cml_node *mn)
{
    return (mn->treetype == MN_MENU && (mn->flags & MN_FLAG_IS_RADIO));
}

/*============================================================*/

cml_atom_type
cml_node_get_value_type(const cml_node *mn)
{
    return mn->value_type;
}

/*============================================================*/

int
cml_node_get_states(const cml_node *mn, int maxn, char **states)
{
    int n = 0;
    GList *iter;
    
    for (iter = mn->dependees ; iter != 0 ; iter = iter->next)
    {
    	cml_node *dee = (cml_node *)iter->data;
	
	if (n < maxn && (dee->flags & MN_FLAG_WARNDEPEND))
    	    states[n++] = dee->name;
    }

    if (n < maxn && (mn->flags & MN_EXPERIMENTAL))
    	states[n++] = "EXPERIMENTAL";

    if (n < maxn && (mn->flags & MN_OBSOLETE))
    	states[n++] = "OBSOLETE";

    if (n < maxn && !(mn->flags & MN_LOADED))
    	states[n++] = "NEW";

    return n;
}

/*============================================================*/

void
mn_add_visibility_expr2(cml_node *mn, cml_expr *expr, cml_expr_type join)
{
    /* 
     * Merge new visibility expr into existing visibility expr
     * by grafting the two expressions together with an `or' node.
     * TODO: check against latest Reference.
     */
    if (mn->visibility_expr == 0)
    {
	mn->visibility_expr = expr;
    }
    else
    {
	mn->visibility_expr = expr_new_composite(join, mn->visibility_expr, expr);
	DDPRINTF1(DEBUG_PARSER, "Merging visibility expressions for `%s'\n", mn->name);
    }
}

void
mn_add_visibility_expr(cml_node *mn, cml_expr *expr)
{
    mn_add_visibility_expr2(mn, expr, E_OR);
}

gboolean
cml_node_is_visible(const cml_node *mn)
{
    cml_atom a;
    GList *iter;
    
    if (mn->visibility_expr == 0)
    	return TRUE;	/* default is to be visible always */
    
    cml_atom_init(&a);
    expr_evaluate(mn->visibility_expr, &a);
    if (a.value.tritval != CML_Y)
    	return FALSE;
    
    for (iter = mn->dependees ; iter != 0 ; iter = iter->next)
    {
    	cml_node *dep = (cml_node *)iter->data;
	
	if (!cml_node_is_visible(dep))
	    return FALSE;
    }

    return TRUE;    
}

/*============================================================*/

void
mn_add_saveability_expr(cml_node *mn, cml_expr *expr)
{
    /* 
     * Merge new saveability expr into existing saveability expr
     * by grafting the two expressions together with an `or' node.
     * TODO: check against latest Reference.
     */
    if (mn->saveability_expr == 0)
    {
	mn->saveability_expr = expr;
    }
    else
    {
	mn->saveability_expr = expr_new_composite(E_OR, mn->saveability_expr, expr);
	DDPRINTF1(DEBUG_PARSER, "Merging saveability expressions for `%s'\n", mn->name);
    }
}

gboolean
mn_is_saveable(const cml_node *mn)
{
    cml_atom a;
    
    if (mn->saveability_expr == 0)
    	return cml_node_is_visible(mn);
    
    cml_atom_init(&a);
    expr_evaluate(mn->saveability_expr, &a);
    assert(a.type == A_BOOLEAN);
    return (a.value.tritval == CML_Y);
}

/*============================================================*/

/*
 * TODO: detect expression loops at parse time!!!!
 */
 
static cml_atom_type
basic_type(cml_atom_type t)
{
    switch (t)
    {
    case A_HEXADECIMAL:
    	return A_DECIMAL;
    case A_TRISTATE:
    	return A_BOOLEAN;
    default:
    	return t;
    }
} 
 
static gboolean
mn_eval_default_expr(cml_node *mn, cml_atom *val)
{
    if (mn->expr == 0)
    	return FALSE; /* no explicit expression for default value */
	
    cml_atom_init(val);
    expr_evaluate(mn->expr, val);
    
    if (basic_type(val->type) != basic_type(mn->value_type) &&
    	mn->value_type != A_NONE)
	return FALSE;
	
    return TRUE;
}


const cml_atom *
cml_node_get_value(cml_node *mn)
{
    const cml_binding *bd;

    switch (mn->treetype)
    {
    case MN_DERIVED:
	assert(mn->expr != 0);
	cml_atom_init(&mn->value);
	expr_evaluate(mn->expr, &mn->value);
	return &mn->value;
	
    case MN_MENU:
    	if (!cml_node_is_radio(mn))
	    return 0;
	/* fall through */
    case MN_SYMBOL:
    	if ((bd = _cml_tx_get(mn->rulebase, mn)) == 0)
	{
	    cml_atom_init(&mn->value);
	    
	    if (cml_node_is_radio(mn->parent))
	    {
	    	mn->value.type = A_BOOLEAN;
		mn->value.value.tritval =
		    (cml_node_get_value(mn->parent)->value.node == mn
		    	? CML_Y : CML_N);
	    }
	    else if (!mn_eval_default_expr(mn, &mn->value))
	    {
    		/*
		 * Failed to set default value from expression,
		 * so use fallback defaults.
		 */
		cml_atom_init(&mn->value);  	/* zero value */
		
    	    	if (mn->value_type == A_NODE)
		{
		    mn->value.type = A_NODE;
		    mn->value.value.node = (mn->children == 0 ? 0 : mn->children->data);
		}
		else if (!mn->rulebase->cml1_default_vals)
		{
		    /* CML2 default value is a zero value of the right type */
		    mn->value.type = mn->value_type;
		}
		/* CML1 default is type A_NONE, i.e. a null */
	    }
	    return &mn->value;
    	}
	return &bd->value;

    case MN_UNKNOWN:
    case MN_EXPLANATION:
    	return 0;
    }
    return 0;
}

char *
cml_node_get_value_as_string(cml_node *mn)
{
    const cml_atom *a = cml_node_get_value(mn);
    
    return (a == 0 ? g_strdup("") : cml_atom_value_as_string(a));
}

/*============================================================*/

static gboolean
mn_set_value2(
    cml_node *mn,
    const cml_atom *ap,
    cml_node *source)
{
    gboolean ret = TRUE;
    GList *nodes;
    
    if (mn_is_chilled(mn))
    {
    	cml_errorl(0, "Ruleset found unsatisfiable while setting %s", mn->name);
    	return FALSE;
    }

    nodes = _cml_tx_get_triggerable_nodes(mn->rulebase, mn, source);

    _cml_tx_set(mn->rulebase, mn, ap, source);
    mn_chill(mn);

    while (nodes != 0)
    {
    	cml_node *trig = (cml_node *)nodes->data;

    	DDPRINTF1(DEBUG_RULES, "triggering rules for node %s\n", trig->name);
	if (!rb_trigger_rules(mn->rulebase, trig->rules_using, source))
    	    ret = FALSE;
    	nodes = g_list_remove_link(nodes, nodes);
    }
    
    return ret;
}

gboolean
mn_set_value(
    cml_node *mn,
    const cml_atom *ap,
    cml_node *source)
{
    if (cml_node_is_radio(mn))
    {
	/* if the menunode is a `choices', set the children using radio behaviour */
	GList *list;
	cml_atom childval;
	
	if (!mn_set_value2(mn, ap, source))
	    return FALSE;

	cml_atom_init(&childval);
	childval.type = A_BOOLEAN;
	
	for (list = mn->children ; list != 0 ; list = list->next)
	{
	    cml_node *child = (cml_node *)list->data;

	    assert(child->treetype == MN_SYMBOL);
	    assert(child->value_type == A_BOOLEAN);
	    childval.value.tritval = (child == ap->value.node ? CML_Y : CML_N);
	    if (!mn_set_value2(child, &childval, source))
	    	return FALSE;
	}
    }
    else if (cml_node_is_radio(mn->parent) && ap->value.tritval == CML_Y)
    {
    	/* happens during defconfig load */
	cml_atom me;
	
	cml_atom_init(&me);
	me.type = A_NODE;
	me.value.node = mn;
	
	return mn_set_value(mn->parent, &me, source);
    }
    else
    {
	return mn_set_value2(mn, ap, source);
    }
    return TRUE;
}

void
cml_node_set_value(cml_node *mn, const cml_atom *ap)
{
    if (mn->treetype == MN_SYMBOL || cml_node_is_radio(mn))
    {
	if (!mn_set_value(mn, ap, mn))
	    mn->rulebase->num_failed_sets++;
    }
}

/*============================================================*/

void
mn_chill(cml_node *mn)
{
    g_hash_table_insert(mn->rulebase->chilled, mn, mn);
}

gboolean
mn_is_chilled(const cml_node *mn)
{
    return (g_hash_table_lookup(mn->rulebase->chilled, mn) != 0);
}

/*============================================================*/

gboolean
cml_node_is_frozen(const cml_node *mn)
{
    const cml_binding *bd;
    
    bd = _cml_tx_get(mn->rulebase, mn);
    return (bd != 0 && (bd->transaction->flags & TX_FROZEN));
}

const char *
cml_node_get_name(const cml_node *mn)
{
    return mn->name;
}

const char *
cml_node_get_banner(const cml_node *mn)
{
    return mn->banner;
}

cml_node *
cml_node_get_parent(const cml_node *mn)
{
    return mn->parent;
}

GList *
cml_node_get_children(const cml_node *mn)
{
    return mn->children;
}

const cml_range *
cml_node_get_range(const cml_node *mn)
{
    return mn->range;
}

unsigned long
cml_node_get_range_count(const cml_node *mn)
{
    return (mn->range == 0 ? 0 : range_count(mn->range));
}

const GList *
cml_node_get_enumdefs(const cml_node *mn)
{
    return mn->enumdefs;
}

/*============================================================*/

void *
cml_node_get_user_data(const cml_node *mn)
{
    return mn->user_data;
}

void
cml_node_set_user_data(cml_node *mn, void *ud)
{
    mn->user_data = ud;
}

/*============================================================*/

void
mn_set_children(cml_node *node, GList *children)
{
    GList *list;
    
    /* TODO: issue error if this node already has children */
    /* TODO: issue error if node->type != MN_MENU */
    node->children = children;
    for (list = children ; list != 0 ; list = list->next)
    {
    	cml_node *child = (cml_node *)list->data;
	/* TODO: issue error if child node already has a parent */
	child->parent = node;
    }
}

void
mn_add_child(cml_node *node, cml_node *child)
{
    assert(child->parent == 0);
    assert(node->treetype == MN_MENU);
    node->children = g_list_append(node->children, child);
    child->parent = node;
}

void
mn_reparent(cml_node *newparent, cml_node *child)
{
    assert(newparent->treetype == MN_MENU);
    if (child->parent != 0)
    {
	assert(child->parent->treetype == MN_MENU);
    	child->parent->children = g_list_remove(child->parent->children, child);
    }
    newparent->children = g_list_append(newparent->children, child);
    child->parent = newparent;
}

void
_mn_add_using_rule(cml_node *mn, cml_rule *rule)
{
    DDPRINTF2(DEBUG_RULES, "Rule %ld uses symbol %s\n",
	rule->uniqueid,
	mn->name);
    mn->rules_using = g_list_prepend(mn->rules_using, rule);
}

/*============================================================*/

void
mn_add_dependant(
    cml_node *dependee,
    cml_node *dependant)
{
    DDPRINTF2(DEBUG_NODES, "mn_add_dependant: %s depends on %s\n",
    	dependant->name,
	dependee->name);

    assert(dependee->treetype == MN_SYMBOL || dependee->treetype == MN_DERIVED || dependee->treetype == MN_UNKNOWN);
    assert(dependant->treetype == MN_MENU || dependant->treetype == MN_SYMBOL);

    dependee->dependants = g_list_prepend(dependee->dependants, dependant);
    dependant->dependees = g_list_prepend(dependant->dependees, dependee);
}

GList *
cml_node_get_warn_dependees(const cml_node *mn)
{
    GList *ret = 0, *list;
    
    for (list = mn->dependees ; list != 0 ; list = list->next)
    {
    	cml_node *dee = (cml_node *)list->data;
	
	if (dee->flags & MN_FLAG_WARNDEPEND)
	    ret = g_list_prepend(ret, dee);
    }

    return ret;
}

/*============================================================*/

const char *
cml_node_get_help_text(const cml_node *mn)
{
    return mn->help_text;
}

/*============================================================*/
/*END*/
