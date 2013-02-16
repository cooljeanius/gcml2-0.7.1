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

CVSID("$Id: string.c,v 1.3 2002/06/07 15:38:58 gnb Exp $");

static void string_update_value(node_gui_t *ng);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define safe_str(s) \
    ((s) == 0 ? "" : (s))

static void
string_set(node_gui_t *ng)
{
    char *oldvalue;
    char *newvalue;
    
    oldvalue = cml_node_get_value_as_string(ng->node);
    newvalue = gtk_editable_get_chars(GTK_EDITABLE(ng->entry), 0, -1);
    
    if (strcmp(safe_str(oldvalue), safe_str(newvalue)))
    {
	cml_atom a;
	a.type = A_STRING;
	a.value.string = newvalue;
	set_value(ng->node, &a);
    }

    g_free(oldvalue);
    g_free(newvalue);
}

static void
on_string_entry_activate(GtkWidget *w, gpointer user)
{
    CALLED();
    string_set((node_gui_t *)user);
}

static void
on_string_entry_focus_out_event(GtkWidget *w, GdkEvent *event, gpointer user)
{
    CALLED();
    string_set((node_gui_t *)user);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
string_build_value(node_gui_t *ng)
{
    ng->entry = gtk_entry_new();
    gtk_signal_connect(GTK_OBJECT(ng->entry), "activate",
    	GTK_SIGNAL_FUNC(on_string_entry_activate), ng);
    gtk_signal_connect(GTK_OBJECT(ng->entry), "focus_out_event",
    	GTK_SIGNAL_FUNC(on_string_entry_focus_out_event), ng);
    
    string_update_value(ng);

    node_gui_attach_widget(ng, ng->entry, VALUE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
string_update_value(node_gui_t *ng)
{
    char *value;
    
    value = cml_node_get_value_as_string(ng->node);
    gtk_entry_set_text(GTK_ENTRY(ng->entry), value);
    g_free(value);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

node_gui_ops_t string_ops = 
{
    node_gui_default_format_label,
    node_gui_default_build_arrow,
    string_build_value,
    string_update_value
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
