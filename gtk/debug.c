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
 
#include "common.h"
#include "node_gui.h"
#include "page_gui.h"
#include "private.h"

CVSID("$Id: debug.c,v 1.6 2002/06/07 15:47:24 gnb Exp $");


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if DEBUG
static cml_visit_result
value_dump_visitor(
    cml_rulebase *rb,
    cml_node *mn,
    int depth,
    void *user_data)
{
    char *valstr;
    
    switch (cml_node_get_treetype(mn))
    {
    case MN_MENU:
    	if (!cml_node_is_radio(mn))
	    break;
    case MN_SYMBOL:
    case MN_DERIVED:
	valstr = cml_node_get_value_as_string(mn);
    	fprintf(stderr, "%s=%s\n", cml_node_get_name(mn), valstr);
	g_free(valstr);
	break;
    default:
    	break;
    }
    return CML_CONTINUE;
}
#endif


GLADE_CALLBACK void
on_debug_show_all_values_activate(GtkWidget *w, void *ud)
{
#if DEBUG
    fprintf(stderr, "--- VALUE DUMP ---\n");
    cml_rulebase_menu_apply(rb, value_dump_visitor, 0);
    cml_rulebase_derived_apply(rb, value_dump_visitor, 0);
    fprintf(stderr, "--- ---\n");
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if DEBUG
static cml_visit_result
visibility_dump_visitor(
    cml_rulebase *rb,
    cml_node *mn,
    int depth,
    void *user_data)
{
    char *str;

    if (mn->visibility_expr == 0)
    	str = g_strdup("--no expression--");
    else
	str = expr_as_string(mn->visibility_expr);

    fprintf(stderr, "%s: %s\n", cml_node_get_name(mn), str);
    
    g_free(str);
    
    return CML_CONTINUE;
}
#endif

GLADE_CALLBACK void
on_debug_show_visibility_expressions_activate(GtkWidget *w, void *ud)
{
#if DEBUG
    fprintf(stderr, "--- VISIBILITY EXPRESSIONS ---\n");
    cml_rulebase_menu_apply(rb, visibility_dump_visitor, 0);
    cml_rulebase_derived_apply(rb, visibility_dump_visitor, 0);
    fprintf(stderr, "--- ---\n");
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if DEBUG
static cml_visit_result
saveability_dump_visitor(
    cml_rulebase *rb,
    cml_node *mn,
    int depth,
    void *user_data)
{
    char *str;

    if (mn->saveability_expr == 0)
    	str = g_strdup("--no expression--");
    else
	str = expr_as_string(mn->saveability_expr);

    fprintf(stderr, "%s: %s\n", cml_node_get_name(mn), str);
    
    g_free(str);
    
    return CML_CONTINUE;
}
#endif

GLADE_CALLBACK void
on_debug_show_saveability_expressions_activate(GtkWidget *w, void *ud)
{
#if DEBUG
    fprintf(stderr, "--- SAVEABILITY EXPRESSIONS ---\n");
    cml_rulebase_menu_apply(rb, saveability_dump_visitor, 0);
    cml_rulebase_derived_apply(rb, saveability_dump_visitor, 0);
    fprintf(stderr, "--- ---\n");
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if DEBUG
static cml_visit_result
dependency_dump_visitor(
    cml_rulebase *rb,
    cml_node *mn,
    int depth,
    void *user_data)
{
    GList *iter;

    fprintf(stderr, "%s:", cml_node_get_name(mn));
    for (iter = mn->dependees ; iter != 0 ; iter = iter->next)
    {
    	cml_node *dep = (cml_node *)iter->data;
	
    	fprintf(stderr, " %s", cml_node_get_name(dep));
    }
    fprintf(stderr, "\n");
    
    return CML_CONTINUE;
}
#endif

GLADE_CALLBACK void
on_debug_show_dependencies_activate(GtkWidget *w, void *ud)
{
#if DEBUG
    fprintf(stderr, "--- DEPENDENCIES ---\n");
    cml_rulebase_menu_apply(rb, dependency_dump_visitor, 0);
    cml_rulebase_derived_apply(rb, dependency_dump_visitor, 0);
    fprintf(stderr, "--- ---\n");
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


GLADE_CALLBACK void
on_debug_dump_transactions_activate(GtkWidget *w, void *ud)
{
#if DEBUG
    fprintf(stderr, "--- TRANSACTION DUMP ---\n");
    _cml_tx_dump(rb, stderr);
    fprintf(stderr, "--- ---\n");
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


GLADE_CALLBACK void
on_debug_run_test_activate(GtkWidget *w, void *ud)
{
#if TESTSCRIPT
    gboolean res;
    
    res = cml_rulebase_run_test(rb);
    fprintf(stderr, "Test %s\n", (res ? "PASSED" : "FAILED"));
    page_gui_update_current();
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if PROFILE

static GtkWidget *
gtk_0th_child(GtkWidget *con)
{
    GList *list;
    GtkWidget *kid0;
    
    list = gtk_container_children(GTK_CONTAINER(con));
    if (list == 0)
    	return 0;
    kid0 = (GtkWidget *)list->data;
    while (list != 0)
    	list = g_list_remove_link(list, list);
    return kid0;
}

static void
gtk_flush(void)
{
    gdk_flush();
    while (g_main_pending())
    	g_main_iteration(/*may_block*/FALSE);
	
#if DEBUG > 5
    usleep(300000);
#endif
}

static void
profile_visit_page(node_gui_t *pageng)
{
    node_gui_t **items;
    GtkWidget *table;
    GList *list;
    int row;
    
    table = gtk_0th_child(gtk_0th_child(pageng->child_page));
    
    /*
     * Build a lookup table of node_gui's indexed by row.
     */
    items = g_new(node_gui_t*, GTK_TABLE(table)->nrows);
    memset(items, 0, sizeof(node_gui_t*) * GTK_TABLE(table)->nrows);
    for (list = GTK_TABLE(table)->children ; list != 0 ; list = list->next)
    {
    	GtkTableChild *tc = (GtkTableChild *)list->data;
	
	if (tc->left_attach == LABEL)
	    items[tc->top_attach] = node_gui_from_widget(tc->widget);
    }
    
    /*
     * Iterate over all items.
     */
    for (row = 0 ; row < GTK_TABLE(table)->nrows ; row++)
    {
    	node_gui_t *ng = items[row];
	
    	assert(ng != 0);
	
	if (!GTK_WIDGET_VISIBLE(ng->colwidgets[LABEL]))
    	    continue;

	if (ng->colwidgets[ARROW] != 0)
	{
	    if (GTK_WIDGET_VISIBLE(ng->colwidgets[ARROW]))
	    {
#if DEBUG > 5
		fprintf(stderr, "Visiting complex %s\n",
	    	    	    cml_node_get_name(ng->node));
#endif
		push_page(ng->node);
		gtk_flush();
	    	profile_visit_page(ng);
    	    }
	}
	else
	{
#if DEBUG > 5
	    fprintf(stderr, "Visiting simple %s\n",
	    	    	cml_node_get_name(ng->node));
#endif
	    if (ng->ops->simulate != 0)
	    {
    		(*ng->ops->simulate)(ng);
    	    	gtk_flush();
	    }

	}
    }
    
    /*
     * Go back up the page stack
     */
    if (cml_node_get_parent(pageng->node) != 0)
    	push_page(cml_node_get_parent(pageng->node));
}

void
profile_exercise(void)
{
    profile_visit_page(node_gui_get_current());
    
    time_mark();
    cml_rulebase_save_defconfig(rb, "profile.config");
    fprintf(stderr, "Save took %g sec\n", time_elapsed());
}

#endif /* PROFILE */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
