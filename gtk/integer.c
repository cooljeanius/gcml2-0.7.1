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

CVSID("$Id: integer.c,v 1.4 2002/06/07 15:40:04 gnb Exp $");

static void integer_update_value(node_gui_t *ng);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
integer_set(node_gui_t *ng)
{
    unsigned long oldvalue;
    const cml_atom *oldvalue_a;
    unsigned long newvalue;
    char *newvalue_str;
    
    oldvalue_a = cml_node_get_value(ng->node);
    oldvalue = oldvalue_a->value.integer;
    newvalue_str = gtk_editable_get_chars(GTK_EDITABLE(ng->entry), 0, -1);
    newvalue = (unsigned long)strtoul(newvalue_str, 0, 0);
    
    if (oldvalue != newvalue)
    {
	cml_atom a;
	a.type = oldvalue_a->type;
	a.value.integer = newvalue;
	set_value(ng->node, &a);
    }
    integer_update_value(ng);

    g_free(newvalue_str);
}

static void
on_integer_entry_activate(GtkWidget *w, gpointer user)
{
    CALLED();
    integer_set((node_gui_t *)user);
}

static void
on_integer_entry_focus_out_event(GtkWidget *w, GdkEvent *event, gpointer user)
{
    CALLED();
    integer_set((node_gui_t *)user);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
integer_build_value(node_gui_t *ng)
{
    /* TODO: spinbox */
    
    ng->entry = gtk_entry_new_with_max_length(16);
    gtk_signal_connect(GTK_OBJECT(ng->entry), "activate",
    	GTK_SIGNAL_FUNC(on_integer_entry_activate), ng);
    gtk_signal_connect(GTK_OBJECT(ng->entry), "focus_out_event",
    	GTK_SIGNAL_FUNC(on_integer_entry_focus_out_event), ng);

    integer_update_value(ng);

    node_gui_attach_widget(ng, ng->entry, VALUE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
integer_update_value(node_gui_t *ng)
{
    char *value;
    
    value = cml_node_get_value_as_string(ng->node);
    gtk_entry_set_text(GTK_ENTRY(ng->entry), value);
    g_free(value);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

node_gui_ops_t hex_ops = 
{
    node_gui_default_format_label,
    node_gui_default_build_arrow,
    integer_build_value,
    integer_update_value
};

node_gui_ops_t dec_ops = 
{
    node_gui_default_format_label,
    node_gui_default_build_arrow,
    integer_build_value,
    integer_update_value
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
