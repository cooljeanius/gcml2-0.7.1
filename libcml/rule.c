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

CVSID("$Id: rule.c,v 1.21 2002/09/01 08:19:34 gnb Exp $");

/*============================================================*/

static cml_rule *
rule_new(cml_expr *expr)
{
    cml_rule *rule = g_new(cml_rule, 1);
    static unsigned long last_uniqueid = 0;
    
    if (rule == 0)
    	return 0;
    memset(rule, 0, sizeof(*rule));
    rule->uniqueid = ++last_uniqueid;
    
    rule->expr = expr;
    
    return rule;
}


cml_rule *
rule_new_require(cml_expr *expr)
{
    return rule_new(expr);
}

cml_rule *
rule_new_prohibit(cml_expr *expr)
{
    return rule_new(expr_new_composite(E_NOT, expr, 0));
}

/*============================================================*/

void
rule_delete(cml_rule *rule)
{
    expr_destroy(rule->expr);
    g_free(rule);
}

/*============================================================*/

gboolean
rule_trigger(cml_rulebase *rb, cml_rule *rule, cml_node *source)
{
    cml_atom val;
    gboolean broken;

    DDPRINTF3(DEBUG_RULES, "triggering rule %ld (%s:%d)\n",
	rule->uniqueid,
	rule->location.filename,
	rule->location.lineno);
	
    cml_atom_init(&val);
    expr_evaluate(rule->expr, &val);
#if DEBUG
    if (debug & DEBUG_RULES)
    {
	char *valstr = cml_atom_value_as_string(&val);
	DDPRINTF1(DEBUG_RULES, "rule expression is %s\n",
	    (valstr == 0 ? "(null)" : valstr));
	g_free(valstr);
    }
#endif
    assert(val.type == A_BOOLEAN);
    
    broken = !val.value.tritval;
    if (broken && source != 0)
    {
    	/* attempt to find a set of bindings which will please the rule */
	cml_atom yes;
	cml_expr *simple;
	
	/* TODO: need a less malloc-intensive single-pass approach */
	simple = expr_simplify(rule->expr);
	
#if DEBUG
    	if (debug & DEBUG_RULES)
	{
	    char *s1 = expr_as_string(rule->expr);
	    char *s2 = expr_as_string(simple);
	    DDPRINTF2(DEBUG_RULES, "Simplified expression: `%s' to `%s'\n", s1, s2);
    	    g_free(s1);
	    g_free(s2);
	}
#endif
	
	yes.type = A_BOOLEAN;
	yes.value.tritval = CML_Y;
	if (expr_solve(simple, &yes, source) == 1)
	    broken = FALSE;
	    
	expr_destroy(simple);
    }
    
    if (broken)
	g_hash_table_insert(rb->broken_rules, rule, rule);
    else
	g_hash_table_remove(rb->broken_rules, rule);
	
    return !broken;
}

/*============================================================*/
#if DEBUG

void
rule_dump(const cml_rule *rule, FILE *fp)
{
    char *estr = expr_as_string(rule->expr);
    
    fprintf(fp, "rule %ld (%s:%d) %s\n",
    	rule->uniqueid,
	rule->location.filename,
	rule->location.lineno,
	estr);
}

#endif
/*============================================================*/

char *
cml_rule_get_explanation(const cml_rule *rule)
{
    char *expr_str, *str;
    
    if (rule->explanation != 0)
    	return g_strdup(rule->explanation->banner);
	
    expr_str = expr_as_string(rule->expr);
    str = g_strdup_printf("Rule %ld (%s:%d) require %s",
    		rule->uniqueid,
		rule->location.filename,
		rule->location.lineno,
		expr_str);
    g_free(expr_str);
    return str;
}

/*============================================================*/
/*END*/
