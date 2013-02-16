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
#include "util.h"
#include "debug.h"

CVSID("$Id: rulebase.c,v 1.38 2002/09/01 08:43:06 gnb Exp $");

#if TESTSCRIPT
static void cml_test_script_delete(cml_test_script *ts);
#endif

/*============================================================*/

cml_rulebase *
cml_rulebase_new(void)
{
    cml_rulebase *rb = g_new(cml_rulebase, 1);
    if (rb == 0)
    	return 0;
    memset(rb, 0, sizeof(*rb));
    
    rb->menu_nodes = g_hash_table_new(g_str_hash, g_str_equal);
    rb->broken_rules = g_hash_table_new(g_direct_hash, g_direct_equal);
    rb->chilled = g_hash_table_new(g_direct_hash, g_direct_equal);
    
    /* setup default warnings for single mode */
    assert(sizeof(rb->warnings)*8 > CW_NUM_WARNINGS);
    rb->warnings = ~0UL;    /* enable all warnings by default */

    return rb;
}

/*============================================================*/

static gboolean
delete_one_node(gpointer key, gpointer value, gpointer userdata)
{
    mn_delete((cml_node *)value);
    return TRUE;    /* remove me please */
}

void
cml_rulebase_delete(cml_rulebase *rb)
{
    _cml_tx_clear(rb);
    
    strdelete(rb->prefix);
    if (rb->xref_fp != 0)
    {
    	fclose(rb->xref_fp);
    	rb->xref_fp = 0;
    }
    if (rb->icon != 0)
    	blob_delete(rb->icon);
    listdelete(rb->rules, cml_rule, rule_delete);
    g_hash_table_foreach_remove(rb->menu_nodes, delete_one_node, 0);
    g_hash_table_destroy(rb->menu_nodes);
    listdelete(rb->filenames, char *, g_free);
    assert(rb->transactions == 0);
    g_hash_table_destroy(rb->broken_rules);
    g_hash_table_destroy(rb->chilled);
#if TESTSCRIPT
    listdelete(rb->test_script, cml_test_script, cml_test_script_delete);
#endif
    g_free(rb);
}

/*============================================================*/

static int
str_has_suffix(const char *s, const char *p)
{
    int plen = strlen(p);
    int slen = strlen(s);
    
    if (slen < plen)
    	return -1;
    return strcmp(s+slen-plen, p);
}

gboolean
cml_rulebase_parse(cml_rulebase *rb, const char *filename)
{
    gboolean failed;
    const char *lang = 0;
    
    if (str_has_suffix(filename, "/Config.in") ||
    	str_has_suffix(filename, "/config.in") ||
	str_has_suffix(filename, ".cml1"))
    {
    	failed = !_cml_rulebase_parse_cml1(rb, filename);
	lang = "CML1";
    }
    else if (str_has_suffix(filename, ".cml"))
    {
    	failed = !_cml_rulebase_parse_cml2(rb, filename);
	lang = "CML2";
    }
    else
    {
    	cml_location loc;
	loc.filename = filename;
	loc.lineno = 0;
    	cml_errorl(&loc, "Cannot determine language from filename\n");
    	return FALSE;
    }
    
    if (!failed && !rb->merge_mode && !cml_rulebase_post_parse(rb))
    	failed = TRUE;

#if DEBUG
    if (debug & DEBUG_NODES)
	rb_dump_nodes(rb, stderr);
    if (debug & DEBUG_RULES)
	rb_dump_rules(rb, stderr);
#endif

#if TESTSCRIPT
    if (rb->parsetest)
    {
	gboolean res = cml_rulebase_run_test(rb);
	fprintf(stderr, "Test %s\n", (res ? "PASSED" : "FAILED"));
	if (!res)
	    failed = TRUE;
    }
#endif

    /* TODO: clean up `files' */
    if (cml_message_count[CML_ERROR] > 0)
    {
    	cml_errorl(0, "%s parser: %d errors\n",
	    	    	lang, cml_message_count[CML_ERROR]);
	failed = TRUE;
    }

    return !failed;
}


/*============================================================*/

const char *
cml_rulebase_get_banner(const cml_rulebase *rb)
{
    return (rb->banner == 0 ? 0 : rb->banner->banner);
}

/*============================================================*/

cml_node *
cml_rulebase_get_start(const cml_rulebase *rb)
{
    return rb->start;
}

/*============================================================*/

void
rb_add_rule(cml_rulebase *rb, cml_rule *rule)
{
    rb->rules = g_list_append(rb->rules, rule);
    rule->rulebase = rb;
}

/*============================================================*/

cml_node *
cml_rulebase_find_node(cml_rulebase *rb, const char *name)
{
    return (cml_node *)g_hash_table_lookup(rb->menu_nodes, name);
}


cml_node *
rb_add_node(cml_rulebase *rb, const char *name)
{
    cml_node *mn = mn_new(name);
    if (mn == 0)
    	return 0;
    g_hash_table_insert(rb->menu_nodes, mn->name, mn);
    mn->rulebase = rb;
    
    return mn;
}

void
rb_remove_node(cml_rulebase *rb, cml_node *mn)
{
    g_hash_table_remove(rb->menu_nodes, mn->name);
}

/*============================================================*/

static cml_visit_result
_cml_rulebase_menu_apply_recursive(
    cml_rulebase *rb,
    cml_rulebase_node_visitor_func visitor,
    cml_node *mn,
    int depth,
    void *user_data)
{
    GList *list;
    cml_visit_result res;
    
    res = (*visitor)(rb, mn, depth, user_data);
    if (res == CML_RETURN)
	return CML_RETURN;

    if (res == CML_CONTINUE)
    {	
	for (list = mn->children ; list != 0 ; list = list->next)
	{
    	    cml_node *child = (cml_node *)list->data;

	    res = _cml_rulebase_menu_apply_recursive(rb, visitor, child, depth+1, user_data);
	    if (res == CML_RETURN)
	    	return CML_RETURN;
	}
    }

    return CML_CONTINUE;
}

/* apply in tree in-order */
void
cml_rulebase_menu_apply(
    cml_rulebase *rb,
    cml_rulebase_node_visitor_func visitor,
    void *user_data)
{
    if (rb->start != 0)
	_cml_rulebase_menu_apply_recursive(rb, visitor, rb->start, 0, user_data);
}

/*============================================================*/

struct derived_apply_state_rec
{
    cml_rulebase *rulebase;
    cml_rulebase_node_visitor_func visitor;
    void *user_data;
};

static void
_rb_derived_apply(gpointer key, gpointer value, gpointer user_data)
{
    cml_node *mn = (cml_node *)value;
    struct derived_apply_state_rec *statep = (struct derived_apply_state_rec *)user_data;

    if (mn->treetype == MN_DERIVED)
	(*statep->visitor)(statep->rulebase, mn, 0, statep->user_data);
}

void
cml_rulebase_derived_apply(
    cml_rulebase *rb,
    cml_rulebase_node_visitor_func visitor,
    void *user_data)
{
    struct derived_apply_state_rec state;

    state.rulebase = rb;
    state.visitor = visitor;
    state.user_data = user_data;
    g_hash_table_foreach(rb->menu_nodes, _rb_derived_apply, &state);
}

/*============================================================*/

const cml_blob *
cml_rulebase_get_icon(cml_rulebase *rb)
{
    return rb->icon;
}

/*============================================================*/

gboolean
rb_trigger_rules(
    cml_rulebase *rb,
    GList *rules,
    cml_node *source)
{
    gboolean ret = TRUE;
    
    for ( ; rules != 0 ; rules = rules->next)
    {
    	cml_rule *rule = (cml_rule *)rules->data;
	
	if (!rule_trigger(rb, rule, source))
	    ret = FALSE;
    }
    
    return ret;
}

void
cml_rulebase_check_all_rules(cml_rulebase *rb)
{
    GList *list;
    
    for (list = rb->rules ; list != 0 ; list = list->next)
    {
    	cml_rule *rule = (cml_rule *)list->data;
	
	rule_trigger(rb, rule, /*source*/0);
    }
}


/*============================================================*/
#if DEBUG

static void
indent(int depth, FILE *fp)
{
    int i;
    
    for (i = 0 ; i < depth ; i++)
    	fputs("    ", fp);
}

static const struct
{
    const char *name;
    unsigned int flag;
} node_flag_strs[] = 
{
{"WARNDEPEND",	    	MN_FLAG_WARNDEPEND},
{"IS_RADIO",	    	MN_FLAG_IS_RADIO},
{"FORWARD_WARNING",	MN_FORWARD_WARNING},
{"VITAL",	    	MN_VITAL},
{"SUBTREE_SEEN",	MN_SUBTREE_SEEN},
{"LOADED",	    	MN_LOADED},
{"EXPERIMENTAL",	MN_EXPERIMENTAL},
{"OBSOLETE",	    	MN_OBSOLETE},
{"CONSTANT",	    	MN_CONSTANT},
{0}
};

static cml_visit_result
dump_rulebase_visitor(
    cml_rulebase *rb,
    cml_node *mn,
    int depth,
    void *user_data)
{
    FILE *fp = (FILE *)user_data;
    const cml_atom *val;
    char *s;
    
    indent(depth, fp);
    fprintf(fp, "%s \"%s\"\n", mn_get_treetype_as_string(mn), mn->name);

    indent(depth, fp);
    fprintf(stderr, "{\n");

    indent(depth, fp);
    if (mn->banner == 0)
	fprintf(fp, "    banner=null\n");
    else
	fprintf(fp, "    banner=\"%s\"\n", mn->banner);

    if (mn->visibility_expr != 0)
    {
    	s = expr_as_string(mn->visibility_expr);
	indent(depth, fp);
	fprintf(fp, "    visibility_expr=%s\n", s);
	g_free(s);
    }

    if (mn->saveability_expr != 0)
    {
    	s = expr_as_string(mn->saveability_expr);
	indent(depth, fp);
	fprintf(fp, "    saveability_expr=%s\n", s);
	g_free(s);
    }

    if (mn->treetype == MN_SYMBOL && mn->expr != 0)
    {
    	s = expr_as_string(mn->expr);
	indent(depth, fp);
	fprintf(fp, "    default_expr=%s\n", s);
	g_free(s);
    }
    if (mn->treetype == MN_DERIVED && mn->expr != 0)
    {
    	s = expr_as_string(mn->expr);
	indent(depth, fp);
	fprintf(fp, "    derived_expr=%s\n", s);
	g_free(s);
    }

    if (mn->treetype == MN_SYMBOL || mn->treetype == MN_DERIVED)
    {
    	int i;

	val = cml_node_get_value(mn);
    	s = cml_atom_value_as_string(val);
	
	indent(depth, fp);
	fprintf(fp, "    value_type=%s\n",
	    	atom_type_as_string(mn->value_type));

	indent(depth, fp);
	fprintf(fp, "    value=%s \"%s\"\n",
	    	cml_atom_type_as_string(val), s);
	g_free(s);

	indent(depth, fp);
	fprintf(fp, "    flags=0x%x", mn->flags);
	for (i = 0 ; node_flag_strs[i].name != 0 ; i++)
	{
	    if (mn->flags & node_flag_strs[i].flag)
	    	fprintf(fp, " %s", node_flag_strs[i].name);
	}
	fputs("\n", fp);

    }
    
    indent(depth, fp);
    fprintf(stderr, "}\n");
    
    return CML_CONTINUE;
}

void
rb_dump_nodes(const cml_rulebase *rb, FILE *fp)
{
    fprintf(stderr, "--dumping rulebase nodes--\n");
    cml_rulebase_menu_apply((cml_rulebase *)rb, dump_rulebase_visitor, fp);
    cml_rulebase_derived_apply((cml_rulebase *)rb, dump_rulebase_visitor, fp);
    fprintf(stderr, "--\n");
}

#endif
/*============================================================*/
#if DEBUG

void
rb_dump_rules(const cml_rulebase *rb, FILE *fp)
{
    GList *list;
    
    fprintf(stderr, "--dumping rulebase rules--\n");
    for (list = rb->rules ; list != 0 ; list = list->next)
    {
    	cml_rule *rule = (cml_rule *)list->data;
	rule_dump(rule, fp);
    }
    fprintf(stderr, "--\n");
}

#endif
/*============================================================*/

/* useful only for CML1 */
void
cml_rulebase_set_arch(cml_rulebase *rb, const char *arch)
{
    cml_node *mn;

    mn = rb_add_node(rb, "ARCH");
    /* TODO: set priority = immutable */
    mn->treetype = MN_DERIVED;
    mn->value_type = A_STRING;
    if (!rb->merge_mode)
	mn->flags |= MN_CONSTANT;
    mn->saveability_expr = expr_new_atom_v(A_BOOLEAN, CML_N);
    mn->expr = expr_new_atom_v(A_STRING, g_strdup(arch));
}

/* only useful for CML1 */
void
cml_rulebase_set_merge_mode(cml_rulebase *rb)
{
    rb->merge_mode = TRUE;

    /*
     * Setup default warnings for merge mode
     */
    rb->warnings = ~0UL;    /* enable all warnings */
    /* Disable warnings which depend on too much state for merge mode. */
    _rb_disable_warning(rb, CW_OVERLAPPING_DEFINITIONS);
    _rb_disable_warning(rb, CW_OVERLAPPING_MIXED_DEFINITIONS);
    _rb_disable_warning(rb, CW_INCONSISTENT_EXPERIMENTAL_TAG);
    _rb_disable_warning(rb, CW_INCONSISTENT_OBSOLETE_TAG);
    _rb_disable_warning(rb, CW_FORWARD_COMPARED_TO_N);
    _rb_disable_warning(rb, CW_FORWARD_REFERENCE);
    _rb_disable_warning(rb, CW_FORWARD_DEPENDENCY);
}

/*============================================================*/

/* most useful for cross-checking multiple branches with source */
void
cml_rulebase_set_xref_filename(cml_rulebase *rb, const char *filename)
{
    if (rb->xref_fp != 0)
    	fclose(rb->xref_fp);
    if ((rb->xref_fp = fopen(filename, "w")) == 0)
    {
    	perror(filename);
	return;
    }
}

void
rb_add_xref(
    cml_rulebase *rb,
    const cml_node *mn,
    const char *usage,
    const cml_location *loc)
{
    if (rb->xref_fp != 0)
    {
    	fprintf(rb->xref_fp, "%s:%s:%s:%d\n",
	    mn->name,
	    usage,
	    loc->filename,
	    loc->lineno);
    }
}

/*============================================================*/

/* commit new bindings */
gboolean
cml_rulebase_commit(cml_rulebase *rb, gboolean freeze)
{
    /* TODO: check feature ties here? */
    gboolean failed;
    
    failed = (rb->num_failed_sets > 0);
    if (failed)
	_cml_tx_abort(rb);
    else
	_cml_tx_commit(rb, freeze);
    
    rb_unchill_all(rb);
    rb->num_failed_sets = 0;
    
    /* TODO: notify front-end that values have changed */
    
    return !failed;
}

void
cml_rulebase_abort(cml_rulebase *rb)
{
    _cml_tx_abort(rb);

    /* TODO: notify front-end that values have changed */
    
    rb_unchill_all(rb);
    rb->num_failed_sets = 0;
}

void
cml_rulebase_undo(cml_rulebase *rb)
{
    _cml_tx_undo(rb);
    
    /* TODO: notify front-end that values have changed */
}

void
cml_rulebase_redo(cml_rulebase *rb)
{
    _cml_tx_redo(rb);
    
    /* TODO: notify front-end that values have changed */
}

void
cml_rulebase_clear(cml_rulebase *rb)
{
    _cml_tx_clear(rb);
}

/*============================================================*/

static gboolean
just_say_yes(gpointer key, gpointer value, gpointer user)
{
    return TRUE;    /* so remove me already */
}

void
rb_unchill_all(cml_rulebase *rb)
{
    g_hash_table_foreach_remove(rb->chilled, just_say_yes, 0);
}

/*============================================================*/

static void
ht_to_list2(gpointer key, gpointer value, gpointer user)
{
    GList **lp = (GList **)user;
    
    *lp = g_list_prepend(*lp, value);
}

static GList *
hashtable_to_list(GHashTable *ht)
{
    GList *list = 0;
    
    g_hash_table_foreach(ht, ht_to_list2, &list);
    
    return list;
}

static gint
rule_compare_by_id(gconstpointer p1, gconstpointer p2)
{
    const cml_rule *r1 = (const cml_rule *)p1;
    const cml_rule *r2 = (const cml_rule *)p2;
    
    if (r1->uniqueid > r2->uniqueid)
    	return 1;
    else if (r1->uniqueid < r2->uniqueid)
    	return -1;
    else
    	return 0;
}

GList *
cml_rulebase_get_broken_rules(const cml_rulebase *rb)
{
    return g_list_sort(
    	    	hashtable_to_list(rb->broken_rules),
		rule_compare_by_id);
}

/*============================================================*/

#if TESTSCRIPT

void
rb_add_test(
    cml_rulebase *rb,
    cml_location loc,
    cml_test_script_op op,
    cml_node *symbol,
    cml_expr *expr)
{
    cml_test_script *ts;
    
    ts = g_new(cml_test_script, 1);
    memset(ts, 0, sizeof(*ts));
    
    ts->op = op;
    ts->location = loc;
    ts->symbol = symbol;
    ts->expr = expr;
    
    rb->test_script = g_list_append(rb->test_script, ts);
}

static void
cml_test_script_delete(cml_test_script *ts)
{
    if (ts->expr != 0)
    	expr_destroy(ts->expr);
    g_free(ts);
}

gboolean
cml_rulebase_run_test(cml_rulebase *rb)
{
    GList *list;
    cml_atom a;
    gboolean v;
    gboolean success = TRUE;
    char *s;
    
    for (list = rb->test_script ; list != 0 ; list = list->next)
    {
    	cml_test_script *ts = (cml_test_script *)list->data;
	
	switch (ts->op)
	{
	case TS_SET:
	    cml_atom_init(&a);
	    expr_evaluate(ts->expr, &a);
#if DEBUG
    	    fprintf(stderr, "@set (%s:%d) %s=%s\n",
	    	ts->location.filename,
		ts->location.lineno,
	    	ts->symbol->name,
		cml_atom_value_as_string(&a)/*memleak*/);
#endif
	    cml_node_set_value(ts->symbol, &a);
	    break;
	
	case TS_ASSERT:
	    cml_atom_init(&a);
	    expr_evaluate(ts->expr, &a);
#if DEBUG
    	    {
	    	char *s1 = expr_as_string(ts->expr);
		char *s2 = cml_atom_value_as_string(&a);
    		fprintf(stderr, "@assert (%s:%d) %s => %s\n",
	    	    ts->location.filename,
		    ts->location.lineno,
		    s1, s2);
	    	g_free(s1);
	    	g_free(s2);
	    }
#endif
	    if (a.type != A_BOOLEAN)
	    {
	    	cml_errorl(&ts->location,
		    "@assert expression is type %d, expecting boolean", a.type);
	    	return FALSE;
	    }
	    if (a.value.tritval != CML_Y)
	    {
	    	s = expr_as_string(ts->expr);
	    	cml_errorl(&ts->location, "@assert %s failed", s);
		g_free(s);
	    	return FALSE;
	    }
	    break;
	
	case TS_SUCCEEDED:
#if DEBUG
    	    fprintf(stderr, "@succeeded (%s:%d) %s\n",
	    	ts->location.filename,
		ts->location.lineno,
		cml_atom_value_as_string(&ts->expr->value)/*memleak*/);
#endif
	    if (ts->expr->value.value.tritval == CML_Y && !success)
	    {
	    	cml_errorl(&ts->location, "Transaction failed, expected it to succeed.");
	    	return FALSE;
	    }
	    else if (ts->expr->value.value.tritval == CML_N && success)
	    {
	    	cml_errorl(&ts->location, "Transaction succeeded, expected it to fail.");
	    	return FALSE;
	    }
	    break;
	    
	case TS_CLEAR:
#if DEBUG
    	    fprintf(stderr, "@clear (%s:%d)\n",
	    	ts->location.filename,
		ts->location.lineno);
#endif
	    cml_rulebase_clear(rb);
	    break;
	    
	case TS_COMMIT:
#if DEBUG
    	    fprintf(stderr, "@commit (%s:%d)\n",
	    	ts->location.filename,
		ts->location.lineno);
#endif
	    success = cml_rulebase_commit(rb, /*freeze*/FALSE);
	    break;
	    
	case TS_FREEZE:
#if DEBUG
    	    fprintf(stderr, "@freeze (%s:%d)\n",
	    	ts->location.filename,
		ts->location.lineno);
#endif
	    success = cml_rulebase_commit(rb, /*freeze*/TRUE);

	case TS_ERROR:
	    s = ts->expr->value.value.string;
#if DEBUG
    	    fprintf(stderr, "@error (%s:%d) \"%s\"\n",
	    	ts->location.filename,
		ts->location.lineno,
		s);
#endif
    	    if (!cml_error_log_find(s))
	    {
	    	cml_errorl(&ts->location, "Did not find error \"%s\".", s);
	    	return FALSE;
	    }
	    break;
	    
	case TS_NOERROR:
#if DEBUG
    	    fprintf(stderr, "@noerror (%s:%d)\n",
	    	ts->location.filename,
		ts->location.lineno);
#endif
    	    if (cml_message_count[CML_ERROR] != 0)
	    {
	    	cml_errorl(&ts->location, "Found %d errors, expected none.",
		    	    cml_message_count[CML_ERROR]);
	    	return FALSE;
	    }
	    break;
	    
	case TS_VISIBLE:
#if DEBUG
    	    fprintf(stderr, "@visible (%s:%d) %s %s\n",
	    	ts->location.filename,
		ts->location.lineno,
		ts->symbol->name,
		cml_atom_value_as_string(&ts->expr->value));
#endif
    	    v = cml_node_is_visible(ts->symbol);
	    if (ts->expr->value.value.tritval == CML_Y && !v)
	    {
	    	cml_errorl(&ts->location, "Symbol %s invisible, expected it to be visible.",
		    	    ts->symbol->name);
	    	return FALSE;
	    }
	    else if (ts->expr->value.value.tritval == CML_N && v)
	    {
	    	cml_errorl(&ts->location, "Symbol %s visible, expected it to be invisible.",
		    	    ts->symbol->name);
	    	return FALSE;
	    }
	    break;

	case TS_SAVEABLE:
#if DEBUG
    	    fprintf(stderr, "@saveable (%s:%d) %s %s\n",
	    	ts->location.filename,
		ts->location.lineno,
		ts->symbol->name,
		cml_atom_value_as_string(&ts->expr->value));
#endif
    	    v = mn_is_saveable(ts->symbol);
	    if (ts->expr->value.value.tritval == CML_Y && !v)
	    {
	    	cml_errorl(&ts->location, "Symbol %s unsaveable, expected it to be saveable.",
		    	    ts->symbol->name);
	    	return FALSE;
	    }
	    else if (ts->expr->value.value.tritval == CML_N && v)
	    {
	    	cml_errorl(&ts->location, "Symbol %s saveable, expected it to be unsaveable.",
		    	    ts->symbol->name);
	    	return FALSE;
	    }
	    break;
	}
    }
    
    return TRUE;
}

#endif

/*============================================================*/

void
cml_rulebase_set_warning(cml_rulebase *rb, int id, gboolean enabled)
{
    if (enabled)
	_rb_enable_warning(rb, id);
    else
	_rb_disable_warning(rb, id);
}

/*============================================================*/
/*END*/
