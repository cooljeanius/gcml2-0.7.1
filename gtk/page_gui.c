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

CVSID("$Id: page_gui.c,v 1.2 2002/06/07 15:46:29 gnb Exp $");

static GList *page_stack;
static GtkWidget *notebook;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
page_gui_init(GtkWidget *nb)
{
    notebook = nb;
}
   
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cml_node *
page_gui_get_nth(int n)
{
    return node_gui_from_widget(
    	    	gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), n))->node;
}

cml_node *
page_gui_get_current(void)
{
    return page_gui_get_nth(
    	    	gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)));
}

cml_node *
page_gui_get_rootmost(void)
{
    return (cml_node *)g_list_last(page_stack)->data;
}

cml_node *
page_gui_get_leafmost(void)
{
    return (cml_node *)g_list_first(page_stack)->data;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
page_gui_move_rootmost(void)
{
    cml_node *mn;
    
    CALLED();
    mn = (cml_node *)g_list_last(page_stack)->data;
    page_gui_update(mn, /*make_current*/TRUE);
}

void
page_gui_move_up(void)
{
    cml_node *mn;
    
    CALLED();
    mn = page_gui_get_current();
    mn = (cml_node *)g_list_find(page_stack, mn)->next->data;
    page_gui_update(mn, /*make_current*/TRUE);
}

void
page_gui_move_down(void)
{
    cml_node *mn;
    
    CALLED();
    mn = page_gui_get_current();
    mn = (cml_node *)g_list_find(page_stack, mn)->prev->data;
    page_gui_update(mn, /*make_current*/TRUE);
}

void
page_gui_move_leafmost(void)
{
    cml_node *mn;
    
    CALLED();
    mn = (cml_node *)g_list_first(page_stack)->data;
    page_gui_update(mn, /*make_current*/TRUE);
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
page_gui_update(cml_node *mn, gboolean make_current)
{
    GtkWidget *table = 0;
    const GList *list;
    node_gui_t *ng;
    int row;
    
    ng = (node_gui_t *)cml_node_get_user_data(mn);
    if (ng == 0)
    	ng = node_gui_new(mn);

    list = cml_node_get_children(ng->node);
    
    if (ng->child_page == 0)
    {
	GtkWidget *sw;

    	creating = TRUE;
	/* create new table and scrolledwindow and populate it */
	table = gtk_table_new(g_list_length((GList *)list), NUM_COLUMNS, /*homogeneous*/FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 0);
	gtk_table_set_col_spacings(GTK_TABLE(table), 0);
	gtk_container_set_border_width(GTK_CONTAINER(table), SPACING);
	gtk_widget_show(table);
    
	sw = gtk_scrolled_window_new(0, 0);
	gtk_container_set_border_width(GTK_CONTAINER(sw), SPACING);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
    	    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), table);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw, 
    	    gtk_label_new(cml_node_get_banner(ng->node)));
	
	node_gui_set_page(ng, sw);
    	creating = FALSE;
    }
    
    
    for (row = 0 ;
    	 list != 0 ;
	 list = list->next, row++)
    {
    	cml_node *child = (cml_node *)list->data;
	node_gui_t *childng = (node_gui_t *)cml_node_get_user_data(child);
	
	if (childng == 0)
	{
	    childng = node_gui_new(child);
	    node_gui_build(childng, table, row);
	}

	node_gui_update(childng);
    }

    if (make_current)
    {
    	creating = TRUE;
	gtk_widget_show(ng->child_page);
	gtk_notebook_set_page(GTK_NOTEBOOK(notebook),
	    gtk_notebook_page_num(GTK_NOTEBOOK(notebook), ng->child_page));
    	creating = FALSE;
    }
}

void
page_gui_update_current(void)
{
    page_gui_update(page_gui_get_current(), FALSE);
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
page_gui_set_current(node_gui_t *ng)
{
    gtk_notebook_set_page(GTK_NOTEBOOK(notebook),
	gtk_notebook_page_num(GTK_NOTEBOOK(notebook), ng->child_page));
}


void
page_gui_push(cml_node *mn)
{
    GList *currlink = 0;

    if (page_stack != 0)
    	currlink = g_list_find(page_stack, page_gui_get_current());
    
    if (g_list_find(page_stack, mn) != 0)
    {
    	/* pushing a page already on the stack - just seek there. */
	page_gui_set_current((node_gui_t *)cml_node_get_user_data(mn));
    }
    else
    {
    	/* pushing a page not on the stack from the middle. */
	while (page_stack != currlink)
	{
	    cml_node *othermn = (cml_node *)page_stack->data;
	    node_gui_t *otherng = (node_gui_t *)cml_node_get_user_data(othermn);
	    
	    gtk_widget_hide(otherng->child_page);
	    
	    page_stack = g_list_remove_link(page_stack, page_stack);
	}
	page_stack = g_list_prepend(page_stack, mn);
	page_gui_update(mn, /*make_current*/TRUE);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_notebook_switch_page(GtkWidget *w, gpointer page, int page_num)
{
    CALLED();
    if (!creating)
    {
	page_gui_update(page_gui_get_nth(page_num), /*make_current*/FALSE);
	grey_items();
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
