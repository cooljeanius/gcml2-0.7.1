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
#include "debug.h"

CVSID("$Id: cml1pass2.c,v 1.7 2002/09/01 08:54:34 gnb Exp $");

static void pass2_node(cml_rulebase *rb, cml_node *mn);

/*============================================================*/

static cml_rule *
add_rule(cml_rulebase *rb, cml_expr *expr, const cml_location *loc)
{
    cml_rule *rule = rule_new_require(expr);
    rb_add_rule(rb, rule);
    if  (loc != 0)
	rule->location = *loc;
#if DEBUG
    if (debug & DEBUG_CONVERT)
    {
	char *s = expr_as_string(expr);
    	DDPRINTF1(DEBUG_CONVERT, "rule: expr=%s\n", s);
	g_free(s);
    }
#endif
    return rule;
}

/*============================================================*/

static const char *
branch_type_as_string(cml_branch_type_t type)
{
    static const char * const type_strs[N_NUMTYPES] = {
    	"none",
	"menu",
	"choice",
	"comment",
	"query",
	"dep_bool",
	"dep_mbool",
	"dep_tristate",
	"define"
    };
    return (type < N_NONE || type >= N_NUMTYPES ? "unknown" : type_strs[type]);
}

/*============================================================*/

static void
merge_visibility_expr_or(cml_node *mn, const cml_expr *cond, gboolean first)
{
    expr_merge_boolean_or(&mn->visibility_expr, cond, first);

#if DEBUG
    if (debug & DEBUG_CONVERT)
    {
    	char *s = expr_as_string(mn->visibility_expr);
	DDPRINTF2(DEBUG_CONVERT, "%s->visibility_expr = %s\n",
	    	mn->name, s);
	g_free(s);
    }
#endif
}

/*============================================================*/

static void
merge_saveability_expr_or(cml_node *mn, const cml_expr *cond, gboolean first)
{
    expr_merge_boolean_or(&mn->saveability_expr, cond, first);

#if DEBUG
    if (debug & DEBUG_CONVERT)
    {
    	char *s = expr_as_string(mn->saveability_expr);
	DDPRINTF2(DEBUG_CONVERT, "%s->saveability_expr = %s\n",
	    	mn->name, s);
	g_free(s);
    }
#endif
}

/*============================================================*/

/*
 * Consumes and merges the default/define expression from the branch.
 */
static void
merge_default_expr(cml_node *mn, cml_branch_t *branch)
{
    cml_expr *value;
    cml_expr *p = 0, *r = 0, *n;
    
    if (branch->exprs == 0)
    	return;
    value = (cml_expr *)branch->exprs->data;
    branch->exprs = g_list_remove_link(branch->exprs, branch->exprs);

    if (branch->type == N_DEFINE &&
    	branch->cond != 0 &&
	expr_contains_symbol(branch->cond, mn))
    {
	/*
	 * Branch is conditional on itself...cannot express
	 * this case with visibility/value expressions, need
	 * to define a rule.
	 */
	cml_expr *expr = expr_new_composite(E_IMPLIES,
	    	    	    expr_deep_copy(branch->cond),
			    expr_new_composite(E_EQUALS,
				expr_new_symbol(mn),
				value));
	add_rule(mn->rulebase, expr, &branch->location);
	if (mn->expr == 0)
	    mn->expr = expr_new_atom_v(A_NONE);
	return;
    }


    /* find rightmost child of the trinary tree = location of default value */	
    if (mn->expr != 0)
    {
	for (r = mn->expr ; r->type == E_TRINARY ; p = r, r = r->children[2])
	    ;
    }
    /* at this point: r=0 or rightmost child, p=0 or its parent */

    /* build a new expression node to stick at this site */
    if (branch->cond == 0)
    {
	n = value;  	/* new default value */
    }
    else
    {
    	if (r == 0)
	    r = expr_new_atom_v(A_NONE);    /* default default value */
	n = expr_new_trinary(expr_deep_copy(branch->cond), value, r);
	r = 0;	/* don't free r, its now a child of the new node */
    }
    /* at this point: n=new node, r=0 or node to free */
    
    /* replace the rightmost with the new subtree */
    if (p == 0)
    	mn->expr = n;
    else
    	p->children[2] = n;
	
    /* free an old node if necessary */
    if (r != 0)
	expr_destroy(r);

#if DEBUG
    if (debug & DEBUG_CONVERT)
    {
	char *s = expr_as_string(mn->expr);
	DDPRINTF2(DEBUG_CONVERT, "%s->expr=%s\n", mn->name, s);
	g_free(s);
    }
#endif
}

/*============================================================*/

static cml_branch_type_t
compound_treetype_upgrade[N_NUMTYPES][N_NUMTYPES];

static void
init_compound_treetype_upgrade(void)
{
    int i;
    static gboolean first = TRUE;
    
    if (!first)
    	return;
    first = FALSE;

    for (i = 0 ; i < N_NUMTYPES ; i++)
    	compound_treetype_upgrade[i][i] = i;

    compound_treetype_upgrade[N_MENU][N_MENU] = N_MENU;
    compound_treetype_upgrade[N_MENU][N_CHOICE] = N_MENU;
    compound_treetype_upgrade[N_MENU][N_COMMENT] = N_MENU;

    compound_treetype_upgrade[N_CHOICE][N_MENU] = N_MENU;
    compound_treetype_upgrade[N_CHOICE][N_CHOICE] = N_CHOICE;
    compound_treetype_upgrade[N_CHOICE][N_COMMENT] = N_CHOICE;

    compound_treetype_upgrade[N_COMMENT][N_MENU] = N_MENU;
    compound_treetype_upgrade[N_COMMENT][N_CHOICE] = N_CHOICE;
    compound_treetype_upgrade[N_COMMENT][N_COMMENT] = N_COMMENT;
}


static cml_branch_type_t
calc_compound_treetype(cml_node *mn)
{
    cml_branch_type_t type = N_NONE;
    cml_branch_type_t newtype;
    GList *iter;
    const cml_location *prev_loc = 0;
    
    for (iter = (GList *)mn->user_data ; iter != 0 ; iter = iter->next)
    {
    	cml_branch_t *branch = (cml_branch_t *)iter->data;
	
	switch (branch->type)
	{
	case N_COMMENT:
	case N_MENU:
	case N_CHOICE:
	    newtype = branch->type;
	    break;

	default:
	    continue;
	}

	if (type == N_NONE)
	    type = newtype;
	else if (type != newtype)
	{
	    cml_errorl(&branch->location,
	    	       "%s \"%s\" redefined as %s",
		        branch_type_as_string(type),
			mn->name,
			branch_type_as_string(newtype));
	    cml_errorl(prev_loc, "location of previous definition");
	    type = compound_treetype_upgrade[type][newtype];
	}
	else
	    type = newtype;
	    
	prev_loc = &branch->location;
    }
    
    DDPRINTF2(DEBUG_CONVERT, "compound \"%s\" is a %s\n",
	    mn->name,
	    branch_type_as_string(type));
    return type;
}

/*============================================================*/

static void
pass2_compound(cml_rulebase *rb, cml_node *mn)
{
    GList *iter;
    cml_branch_type_t type;
    char *deflt = 0;
    
    type = calc_compound_treetype(mn);
    switch (type)
    {
    case N_CHOICE:
    	mn->flags |= MN_FLAG_IS_RADIO;
	break;
    case N_COMMENT:
    case N_MENU:
    	break;
    case N_NONE:
    	assert(!strcmp(mn->name, "__start"));
    	break;
    default:
    	return;
    }

    for (iter = (GList *)mn->user_data ; iter != 0 ; iter = iter->next)
    {
    	cml_branch_t *branch = (cml_branch_t *)iter->data;

	merge_visibility_expr_or(mn, branch->cond, (iter->prev == 0));
	if (deflt == 0)
	    deflt = branch->choice_default;
    }
    
#if DEBUG
    if ((debug & DEBUG_CONVERT) &&
    	g_list_length((GList *)mn->user_data) > 1)
    {
    	DDPRINTF2(DEBUG_CONVERT, "%s \"%s\" has multiple branches:\n",
	    	branch_type_as_string(type),
		mn->name);

	for (iter = (GList *)mn->user_data ; iter != 0 ; iter = iter->next)
	{
    	    cml_branch_t *branch = (cml_branch_t *)iter->data;
    	    DDPRINTF2(DEBUG_CONVERT, "    %s:%d\n",
	    	branch->location.filename,
		branch->location.lineno);
	}
    }
#endif

    if (mn->flags & MN_FLAG_IS_RADIO)
    {
	/* setup default expression */
	cml_node *child;

    	assert(deflt != 0);	
    	child = cml_rulebase_find_node(rb, deflt);
	
	assert(child != 0);
	assert(child->parent == mn);

	/* TODO: need a cml_node_choice_set_default() */
	mn->expr = expr_new_atom_v(A_NODE, child);
    }

    for (iter = mn->children ; iter != 0 ; iter = iter->next)
    	pass2_node(rb, (cml_node *)iter->data);
}

/*============================================================*/

static cml_atom_type type_upgrade[CML_MAX_ATOM_TYPE+1][CML_MAX_ATOM_TYPE+1];

static void
init_type_upgrade(void)
{
    int i;
    static gboolean first = TRUE;
    
    if (!first)
    	return;
    first = FALSE;

    for (i = 0 ; i <= CML_MAX_ATOM_TYPE ; i++)
    	type_upgrade[i][i] = i;

    type_upgrade[A_DECIMAL][A_HEXADECIMAL] = A_HEXADECIMAL;
    type_upgrade[A_HEXADECIMAL][A_DECIMAL] = A_HEXADECIMAL;

    type_upgrade[A_BOOLEAN][A_TRISTATE] = A_TRISTATE;
    type_upgrade[A_TRISTATE][A_BOOLEAN] = A_TRISTATE;
}

static cml_atom_type
calc_value_type(cml_node *mn)
{
    cml_atom_type type = A_NONE;
    cml_atom_type newtype;
    GList *iter;
    const cml_location *prevloc = 0;
    
    for (iter = (GList *)mn->user_data ; iter != 0 ; iter = iter->next)
    {
    	cml_branch_t *branch = (cml_branch_t *)iter->data;
	
	switch (branch->type)
	{
	case N_DEP_BOOL:
	case N_DEP_MBOOL:
	    newtype = A_BOOLEAN;
	    break;
	case N_DEP_TRISTATE:
	    newtype = A_TRISTATE;
	    break;
	case N_QUERY:
	case N_DEFINE:
	    newtype = branch->value_type;
	    break;
	default:
	    continue;
	}

	if (type == A_NONE)
	    type = newtype;
	else if (type_upgrade[type][newtype] == A_NONE)
	{
	    cml_errorl(&branch->location,
	    	       "%s \"%s\" cannot be redefined as %s",
		        atom_type_as_string(type),
			mn->name,
		        atom_type_as_string(newtype));
	    cml_errorl(prevloc,
	    	       "location of previous definition");
	    return A_NONE;
	}
	else
	    type = newtype;
	    
	prevloc = &branch->location;
    }
    
    DDPRINTF2(DEBUG_CONVERT, "%s->value_type = %s\n",
	    mn->name,
	    atom_type_as_string(type));
    return type;
}

/*============================================================*/

static cml_node_treetype
calc_branch_treetype(const cml_branch_t *branch)
{
    switch (branch->type)
    {
    case N_QUERY:
    case N_DEP_BOOL:
    case N_DEP_MBOOL:
    case N_DEP_TRISTATE:
	return MN_SYMBOL;

    case N_DEFINE:
	return MN_DERIVED;

    default:
	return MN_UNKNOWN;
    }
}

static cml_node_treetype
calc_primitive_treetype(cml_node *mn)
{
    cml_node_treetype type = MN_UNKNOWN;
    cml_node_treetype newtype;
    GList *iter;
    
    for (iter = (GList *)mn->user_data ; iter != 0 ; iter = iter->next)
    {
    	cml_branch_t *branch = (cml_branch_t *)iter->data;
	
	if ((newtype = calc_branch_treetype(branch)) == MN_UNKNOWN)
	    continue;

	if (type == A_NONE)
	    type = newtype;
	else if (type != newtype)
	    type = MN_SYMBOL;
	else
	    type = newtype;
    }
    
    DDPRINTF2(DEBUG_CONVERT, "%s->treetype = %s\n",
	    mn->name,
	    mn_treetype_as_string(type));
    return type;
}

/*============================================================*/

/*
 * Report if the branch flags contains the given flag.  Returns:
 * BR_PLUS if the branch contains the flag, or
 * BR_MINUS if the branch does not contain the flag, or
 * BR_NEVER if the branch never happens (e.g. it's for another arch)
 */
#define BR_MINUS    0
#define BR_PLUS     1
#define BR_NEVER    2

static unsigned
branch_has_flag(cml_branch_t *branch, unsigned int flag)
{
    unsigned int ret;

    if (!(branch->flags & BR_HAS_BANNER))
    	return BR_NEVER;

    if (!(branch->flags & flag))
    	return BR_MINUS;

    if (branch->cond == 0)
    	ret = BR_PLUS;
    else
    {
	cml_expr *expr = expr_simplify(branch->cond);
	assert(expr->value.type == A_BOOLEAN);
	ret = (expr_is_constant(expr) && expr->value.value.tritval == CML_N ?
    	    		BR_NEVER : BR_PLUS);
	expr_destroy(expr);
    }

    return ret;
}

static const struct b2n_flag
{
    unsigned int branch_flag;
    unsigned int node_flag;
    char *tag;
    cml_warning_t warning;
}
branch_to_node_flags[] = 
{
    {
	BR_EXPERIMENTAL,
	MN_EXPERIMENTAL,
	"(EXPERIMENTAL)",
	CW_INCONSISTENT_EXPERIMENTAL_TAG
    },
    {
	BR_OBSOLETE,
	MN_OBSOLETE,
	"(OBSOLETE)",
	CW_INCONSISTENT_OBSOLETE_TAG
    },
    {0}
};

static unsigned int
calc_flags(cml_node *mn)
{
    unsigned int bflags[3];
    GList *iter;
    const struct b2n_flag *b2n;
    
    memset(bflags, 0, sizeof(bflags));

    for (iter = (GList *)mn->user_data ; iter != 0 ; iter = iter->next)
    {
    	cml_branch_t *branch = (cml_branch_t *)iter->data;

    	for (b2n = branch_to_node_flags ; b2n->branch_flag != 0 ; b2n++)
    	    bflags[branch_has_flag(branch, b2n->branch_flag)] |= b2n->node_flag;
    }
    
    for (b2n = branch_to_node_flags ; b2n->branch_flag != 0 ; b2n++)
    {
	if ((bflags[BR_PLUS] & b2n->node_flag) &&
    	    (bflags[BR_MINUS] & b2n->node_flag))
	    if (rb_warning_enabled(mn->rulebase, b2n->warning))
    		cml_warningl(0,
		    "symbol %s is declared both with and without %s",
		    mn->name, b2n->tag);
    }
    
    return bflags[BR_PLUS];
}

/*============================================================*/

/*
 * Add rules which constrain the value of the symbol.
 */	
static void
add_dependency_rules(
    cml_rulebase *rb,
    cml_node *mn,
    cml_branch_t *branch)
{
    cml_expr *expr;
    cml_expr_type req_type;
    GList *iter;
    
    /*
     * The three types of dependency differ in their treatment of 'm'
     * when it occurs in symbol and/or dep.  These effects are encoded
     * into an addition to the symbol's visibility expression and
     * a rule constraining the symbol's value.
     */
    switch (branch->type)
    {
    case N_DEP_BOOL:
	req_type = E_LESS_EQUALS;
	break;
    case N_DEP_MBOOL:
    	req_type = E_MDEP;
	break;
    case N_DEP_TRISTATE:
	req_type = E_LESS_EQUALS;
	break;
    default:
    	return;
    }

    for (iter = branch->exprs ; iter != 0 ; iter = iter->next)
    {
    	cml_expr *dep_expr = (cml_expr *)iter->data;

	expr = expr_new_composite(req_type,
	    	    expr_new_symbol(mn),
		    expr_deep_copy(dep_expr));
    	if (branch->cond != 0)
	    expr = expr_new_composite(E_IMPLIES,
	    	    	expr_deep_copy(branch->cond),
			expr);
	add_rule(rb, expr, &branch->location);
    }
}


/*
 * Calculate and return the branch visibility expression
 * for dep_* branches.  For most branch types this is just the
 * branch->cond from the parsetime cond stack and is handled
 * in pass2_primitive, but for dep_* we have to merge in
 * clauses which limit the visibility based on the dep.  For
 * example, with
 *
 * if [ EXPR1 ]; then
 *     bool '...' FOO
 * fi
 * if [ EXPR2 ]; then
 *     dep_bool '...' FOO $BAR
 * fi
 * 
 * we want the final visibility_expr=(EXPR1) or ((EXPR2) and (BAR==y))
 * so we return ((EXPR2) and (BAR==y)) for the second branch.
 */
static cml_expr *
calc_branch_visibility_expr(cml_rulebase *rb, cml_branch_t *branch)
{
    cml_expr *br_vis;
    cml_expr_type vis_type = E_NONE;
    cml_tritval vis_val;
    cml_expr *expr;
    GList *iter;
    
    br_vis = (branch->cond == 0 ? 0 : expr_deep_copy(branch->cond));
    
    switch (branch->type)
    {
    case N_DEP_BOOL:
	vis_type = E_EQUALS;
	vis_val = CML_Y;
	break;
    case N_DEP_MBOOL:
	vis_type = E_NOT_EQUALS;
	vis_val = CML_N;
	break;
    case N_DEP_TRISTATE:
	vis_type = E_NOT_EQUALS;
	vis_val = CML_N;
	break;
    default:
    	assert(0);
	break;
    }
    
    for (iter = branch->exprs ; iter != 0 ; iter = iter->next)
    {
    	cml_expr *dep_expr = (cml_expr *)iter->data;

	expr = expr_new_composite(vis_type,
				    expr_deep_copy(dep_expr),
				    expr_new_atom_v(A_TRISTATE, vis_val));
	expr_merge_boolean_and(&br_vis, expr, FALSE);
	/* TODO: destructive merge instead, replaces copies */
	expr_destroy(expr);
    }

#if DEBUG
    if (debug & DEBUG_CONVERT)
    {
    	char *s = expr_as_string(br_vis);
	DDPRINTF2(DEBUG_CONVERT, "%s branch visibility_expr = %s\n",
	    	    ((cml_node *)branch->node)->name, s);
	g_free(s);
    }
#endif

    return br_vis;
}

/*============================================================*/

#if OVERLAP_DNF

static gboolean
branches_overlap(cml_branch_t *branch1, cml_branch_t *branch2)
{
    GList *varbs;
    int i, n;
    
    if (branch1->cond_dnf == 0)
	branch1->cond_dnf = dnf_from_expr(branch1->cond);

    if (branch2->cond_dnf == 0)
	branch2->cond_dnf = dnf_from_expr(branch2->cond);

    if ((varbs = dnf_overlap(branch1->cond_dnf, branch2->cond_dnf)) == 0)
    	return FALSE;
	
    i = 0;
    n = g_list_length(varbs);
    while (varbs != 0)
    {
	cml_warningl(&branch1->location,
	    	   "overlap(%d/%d) is \"%s\"",
		    ++i, n, (char *)varbs->data);
	g_free(varbs->data);
    	varbs = g_list_remove_link(varbs, varbs);
    }
    return TRUE;
}

static void
branch_overlap_cleanup(cml_branch_t *branch)
{
    dnf_free(branch->cond_dnf);
    branch->cond_dnf = 0;
}

#else

static gboolean
branches_overlap(cml_branch_t *branch1, cml_branch_t *branch2)
{
    if (branch1->cond_str == 0)
	branch1->cond_str = expr_as_string(branch1->cond);
    if (branch2->cond_str == 0)
	branch2->cond_str = expr_as_string(branch2->cond);
	
    if (strstr(branch1->cond_str, branch2->cond_str) != 0)
    {
	cml_warningl(&branch1->location,
	    	   "overlap is \"%s\"",
		    branch2->cond_str);
    	return TRUE;
    }
    else if (strstr(branch2->cond_str, branch1->cond_str) != 0)
    {
	cml_warningl(&branch1->location,
	    	   "overlap is \"%s\"",
		    branch1->cond_str);
    	return TRUE;
    }
    return FALSE;
}

static void
branch_overlap_cleanup(cml_branch_t *branch)
{
    g_free(branch->cond_str);
    branch->cond_str = 0;
}

#endif
/*============================================================*/

/*
 * Type	Overlap Define	Conditional Menu
 * ---- ------- ------	----------- ----
 *  1	mixed	first	query	    same
 *  2	mixed	first	query	    different
 *  3	mixed	second	query	    same	
 *  4	mixed	second	query	    different
 *  5	mixed	first	define	    same	
 *  6	mixed	first	define	    different
 *  7	mixed	second	define	    same	
 *  8	mixed	second	define	    different
*/

static int
classify_mixed_overlap(cml_branch_t *branch1, cml_branch_t *branch2)
{
    /* confusingly, branch2 is defined before branch1 */
    int type = 0;
    cml_branch_t *definebr, *querybr;

    if (branch1->parent != branch2->parent)
    	type |= 1;
	
    if (branch2->type == N_DEFINE)
    {
	definebr = branch2;
	querybr = branch1;
    }
    else
    {
    	type |= 2;
    	definebr = branch1;
	querybr = branch2;
    }

    if (definebr->cond != 0)
    {
    	type |= 4;
	if (querybr->cond != 0)
	    type |= 16;     /* hack to show bad assumption */
    }
    
    return type+1;
}

static void
check_branches_are_disjoint(cml_node *mn)
{
    GList *iter1, *iter2;

    DDPRINTF1(DEBUG_DNF, "checking %s for disjoint branches\n", mn->name);
    
    for (iter1 = (GList *)mn->user_data ; iter1 != 0 ; iter1 = iter1->next)
    {
    	cml_branch_t *branch1 = (cml_branch_t *)iter1->data;

	for (iter2 = (GList *)mn->user_data ;
	     iter2 != 0 && iter2 != iter1;
	     iter2 = iter2->next)
	{
    	    cml_branch_t *branch2 = (cml_branch_t *)iter2->data;
	    
	    if (branches_overlap(branch1, branch2))
	    {
	    	cml_node_treetype treetype1 = calc_branch_treetype(branch1);
	    	cml_node_treetype treetype2 = calc_branch_treetype(branch2);

	    	if (treetype1 != treetype2)
		{
		    if (rb_warning_enabled(mn->rulebase,
		    	    	    	   CW_OVERLAPPING_MIXED_DEFINITIONS))
		    {
			cml_warningl(&branch1->location,
	    		       "\"%s\" has overlapping definitions as %s and %s (type %d)",
				mn->name,
				mn_treetype_as_string(treetype1),
				mn_treetype_as_string(treetype2),
				classify_mixed_overlap(branch1, branch2));
			cml_warningl(&branch2->location,
		    		"location of previous definition");
#if 0
			cml_warningl(&branch2->parent->location,
		    		"first parent is \"%s\"",
				branch2->parent->name);
			cml_warningl(&branch1->parent->location,
		    		"second parent is \"%s\"",
				branch1->parent->name);
#endif
		    }
    	    	}
		else
		{
		    if (rb_warning_enabled(mn->rulebase,
		    	    	    	   CW_OVERLAPPING_DEFINITIONS))
		    {
	    		cml_warningl(&branch1->location,
		    		"\"%s\" has overlapping definitions",
				mn->name);
			cml_warningl(&branch2->location,
		    		"location of previous definition");
		    }
		}
	    }
	}
    }


    for (iter1 = (GList *)mn->user_data ; iter1 != 0 ; iter1 = iter1->next)
    {
    	cml_branch_t *branch1 = (cml_branch_t *)iter1->data;

    	branch_overlap_cleanup(branch1);
    }
}

/*============================================================*/

static void
pass2_primitive(cml_rulebase *rb, cml_node *mn)
{
    cml_atom_type value_type;
    cml_node_treetype treetype;
    cml_expr *rexpr;
    GList *iter;
    
    DDPRINTF1(DEBUG_CONVERT, "%s\n", mn->name);

    if ((value_type = calc_value_type(mn)) == A_NONE)
    	return;

    if ((treetype = calc_primitive_treetype(mn)) == MN_UNKNOWN)
    	return;

    mn->treetype = treetype;
    mn->value_type = value_type;
    mn->flags |= calc_flags(mn);

    if (rb->xref_fp != 0)
    {
	for (iter = (GList *)mn->user_data ; iter != 0 ; iter = iter->next)
	{
    	    cml_branch_t *branch = (cml_branch_t *)iter->data;

    	    rb_add_xref(rb, mn, atom_type_as_string(mn->value_type),
	    	    	&branch->location);
    	}
    }
    
    if (rb_warning_enabled(rb, CW_OVERLAPPING_MIXED_DEFINITIONS) ||
    	rb_warning_enabled(rb, CW_OVERLAPPING_DEFINITIONS))
	check_branches_are_disjoint(mn);

    for (iter = (GList *)mn->user_data ; iter != 0 ; iter = iter->next)
    {
    	cml_branch_t *branch = (cml_branch_t *)iter->data;

	switch (branch->type)
	{
	case N_QUERY:
	    assert(mn->treetype == MN_SYMBOL);
	    merge_default_expr(mn, branch);
	    merge_visibility_expr_or(mn, branch->cond, (iter->prev == 0));
	    break;

	case N_DEP_BOOL:
	case N_DEP_MBOOL:
	case N_DEP_TRISTATE:
	    assert(mn->treetype == MN_SYMBOL);
    	    /* TODO: destructive merge to avoid copies */
	    rexpr = calc_branch_visibility_expr(rb, branch);
	    merge_visibility_expr_or(mn, rexpr, (iter->prev == 0));
    	    if (rexpr != 0)
		expr_destroy(rexpr);
	    break;

	case N_DEFINE:
	    merge_default_expr(mn, branch);
	    assert(branch->exprs == 0);
	    if (mn->treetype == MN_SYMBOL && iter->prev == 0)
	    {
		assert(mn->visibility_expr == 0);
		mn->visibility_expr = expr_new_atom_v(A_BOOLEAN, CML_N);
	    }
	    break;

	default:
	    continue;
	}

    	merge_saveability_expr_or(mn, branch->cond, (iter->prev == 0));
    }

    /* 
     * Add rules and modify branch->cond to implement dep_bool etc
     */
    for (iter = (GList *)mn->user_data ; iter != 0 ; iter = iter->next)
	add_dependency_rules(rb, mn, (cml_branch_t *)iter->data);
}

/*============================================================*/

static void
pass2_node(cml_rulebase *rb, cml_node *mn)
{
    if (mn->treetype == MN_MENU)
	pass2_compound(rb, mn);
    else
    	pass2_primitive(rb, mn);

    /* Detach and free the branch information */
    while (mn->user_data != 0)
    {
    	cml_branch_t *branch = (cml_branch_t *)((GList *)mn->user_data)->data;
	branch_delete(branch);
    	mn->user_data = g_list_remove_link((GList *)mn->user_data,
	    	    	    	    	   (GList *)mn->user_data);
    }
}

/*============================================================*/

static void
detach_derived_symbol(gpointer key, gpointer value, gpointer user_data)
{
    cml_node *mn = (cml_node *)value;

    if (mn->treetype == MN_DERIVED && mn->parent != 0)
    {
    	mn->parent->children = g_list_remove(mn->parent->children, mn);
	mn->parent = 0;
    }
}

/*============================================================*/

void
cml1_pass2(cml_rulebase *rb)
{
    init_type_upgrade();
    init_compound_treetype_upgrade();

    if (rb->banner != 0)
    {
	/*
	 * Kill the useless banner node, which exists solely to provide
	 * a banner string.  Move it's banner string into the root node,
	 * for which there is no useful banner string in CML1.  This code
	 * is necessary only because CML2's model differs from CML1 by
	 * having a banner for the root node and a separate banner for
	 * the rulebase as a whole.
	 */
	assert(rb->banner->banner != 0);
	assert(rb->start->banner == 0);
	rb->start->banner = rb->banner->banner;
	rb->banner->banner = 0;
	rb_remove_node(rb, rb->banner);
	mn_delete(rb->banner);
	rb->banner = rb->start;
    }
    
    pass2_node(rb, rb->start);

    if (rb->xref_fp != 0)
    	fflush(rb->xref_fp);

    /*
     * Temporary hack: remove all MN_DERIVED symbols from the menu
     * tree.  CML2 defines derived symbols to live outside the menu
     * tree in their own space, which has the unfortunate side effect
     * that their save order in unpredictable.
     */
    g_hash_table_foreach(rb->menu_nodes, detach_derived_symbol, rb);
}

/*============================================================*/
/*END*/
