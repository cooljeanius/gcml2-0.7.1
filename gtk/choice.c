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

CVSID("$Id: choice.c,v 1.5 2002/06/07 15:47:48 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
choice_build_arrow(node_gui_t *ng)
{
    /* no arrow for choices */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
on_choice_combo_changed(GtkWidget *w, void *ud)
{
    GtkWidget *listw;
    GtkWidget *selitem;
    gpointer user_data;
    cml_atom val;
    node_gui_t *ng = (node_gui_t *)ud;
    
    if (creating)
    	return;
	
    CALLED();
    listw = GTK_COMBO(ng->combo)->list;
    selitem = (GtkWidget *)GTK_LIST(listw)->selection->data;
    user_data = gtk_object_get_user_data(GTK_OBJECT(selitem));
    
    cml_atom_init(&val);

    val.type = A_NODE;
    val.value.node = (cml_node *)user_data;

    set_value(ng->node, &val);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
choice_build_value(node_gui_t *ng)
{
    GList *list;
    GtkWidget *listw;
    GtkWidget *selitem = 0;
    cml_node *selchild = cml_node_get_value(ng->node)->value.node;
        
    ng->combo = gtk_combo_new();
    gtk_combo_set_value_in_list(GTK_COMBO(ng->combo), TRUE, FALSE);
    gtk_combo_set_use_arrows(GTK_COMBO(ng->combo), FALSE);
    ng->entry = GTK_COMBO(ng->combo)->entry;
    gtk_entry_set_editable(GTK_ENTRY(ng->entry), FALSE);
    gtk_signal_connect(GTK_OBJECT(ng->entry), "changed", 
    	GTK_SIGNAL_FUNC(on_choice_combo_changed), ng);


    listw = GTK_COMBO(ng->combo)->list;
    
    gtk_list_clear_items(GTK_LIST(listw), 0, -1);
    for (list = cml_node_get_children(ng->node) ; list != 0 ; list = list->next)
    {
	cml_node *child = (cml_node *)list->data;
	GtkWidget *item = gtk_list_item_new_with_label(cml_node_get_banner(child));
	gtk_object_set_user_data(GTK_OBJECT(item), (gpointer)child);
	gtk_widget_show(item);
	gtk_container_add(GTK_CONTAINER(listw), item);
	if (child == selchild)
	    selitem = item;
    }
    if (selchild != 0)
	gtk_list_select_child(GTK_LIST(listw), selitem);
    
    node_gui_attach_widget(ng, ng->combo, VALUE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
choice_update_value(node_gui_t *ng)
{
    GtkWidget *listw;
    GList *list;
    cml_node *value;

    value = cml_node_get_value(ng->node)->value.node;
    
    listw = GTK_COMBO(ng->combo)->list;
    list = gtk_container_children(GTK_CONTAINER(listw));
    while (list != 0)
    {
    	GtkWidget *item = (GtkWidget *)list->data;
	cml_node *itemvalue;
	
	itemvalue = (cml_node *)gtk_object_get_user_data(GTK_OBJECT(item));
    	if (itemvalue == value)
	{
	    gtk_list_select_child(GTK_LIST(listw), item);
	    break;
	}
	
    	list = g_list_remove_link(list, list);
    }
    
    while (list != 0)
    	list = g_list_remove_link(list, list);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

node_gui_ops_t choice_ops = 
{
    node_gui_default_format_label,
    choice_build_arrow,
    choice_build_value,
    choice_update_value
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
