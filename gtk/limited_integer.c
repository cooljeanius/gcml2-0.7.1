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

CVSID("$Id: limited_integer.c,v 1.4 2002/06/07 15:39:19 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static void
on_limited_int_combo_changed(GtkWidget *w, void *ud)
{
    GtkWidget *listw;
    GtkWidget *selitem;
    unsigned long newvalue;
    cml_atom val;
    node_gui_t *ng = (node_gui_t *)ud;
    
    if (creating)
    	return;
	
    CALLED();

    listw = GTK_COMBO(ng->combo)->list;
    selitem = (GtkWidget *)GTK_LIST(listw)->selection->data;
    newvalue = (unsigned long)gtk_object_get_user_data(GTK_OBJECT(selitem));
    
    cml_atom_init(&val);
    val.type = cml_node_get_value(ng->node)->type;
    val.value.integer = newvalue;

    set_value(ng->node, &val);
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
add_list_entry(GtkWidget *listw, const char *str, unsigned long value)
{
    GtkWidget *item;

    item = gtk_list_item_new_with_label(str);
    gtk_object_set_user_data(GTK_OBJECT(item), (gpointer)value);
    gtk_widget_show(item);
    gtk_container_add(GTK_CONTAINER(listw), item);
}

static void
limited_int_build(node_gui_t *ng, gboolean ishex)
{
    unsigned long i;
    GtkWidget *listw;
    const GList *list;

    ng->combo = gtk_combo_new();
    gtk_combo_set_value_in_list(GTK_COMBO(ng->combo), TRUE, FALSE);
    gtk_combo_set_use_arrows(GTK_COMBO(ng->combo), FALSE);
    ng->entry = GTK_COMBO(ng->combo)->entry;
    gtk_entry_set_editable(GTK_ENTRY(ng->entry), FALSE);
    gtk_signal_connect(GTK_OBJECT(ng->entry), "changed",
    	GTK_SIGNAL_FUNC(on_limited_int_combo_changed), ng);

    listw = GTK_COMBO(ng->combo)->list;

    gtk_list_clear_items(GTK_LIST(listw), 0, -1);
    
    list = cml_node_get_enumdefs(ng->node);
    if (list != 0)
    {
	for ( ; list != 0 ; list = list->next)
	{
	    cml_enumdef *ed = (cml_enumdef *)list->data;
	    add_list_entry(listw, cml_node_get_banner(ed->symbol), ed->value);
	}
    }
    else
    {
	for (list = cml_node_get_range(ng->node) ; list != 0 ; list = list->next)
	{
	    cml_subrange *sr = (cml_subrange *)list->data;
	    for (i = sr->begin ; i <= sr->end ; i++)
	    {
		char valuebuf[32];
		sprintf(valuebuf, (ishex ? "0x%lX" : "%ld"), i);
		add_list_entry(listw, valuebuf, i);
	    }
	}
    }
    
    node_gui_attach_widget(ng, ng->combo, VALUE);
}

static void
limited_hex_build(node_gui_t *ng)
{
    limited_int_build(ng, TRUE);
}

static void
limited_dec_build(node_gui_t *ng)
{
    limited_int_build(ng, FALSE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
limited_int_update_value(node_gui_t *ng)
{
    GtkWidget *listw;
    GList *list;
    unsigned long value;

    value = cml_node_get_value(ng->node)->value.integer;
    
    listw = GTK_COMBO(ng->combo)->list;
    list = gtk_container_children(GTK_CONTAINER(listw));
    while (list != 0)
    {
    	GtkWidget *item = (GtkWidget *)list->data;
	unsigned long itemvalue;
	
	itemvalue = (unsigned long)gtk_object_get_user_data(GTK_OBJECT(item));
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

node_gui_ops_t limited_hex_ops = 
{
    node_gui_default_format_label,
    node_gui_default_build_arrow,
    limited_hex_build,
    limited_int_update_value
};

node_gui_ops_t limited_dec_ops = 
{
    node_gui_default_format_label,
    node_gui_default_build_arrow,
    limited_dec_build,
    limited_int_update_value
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
