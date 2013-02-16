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
#include <unistd.h>
#include "node_gui.h"


static GtkWidget *node_help_shell = 0;
static GtkWidget *node_help_label;
static GtkWidget *node_help_text;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_node_help_close_clicked(GtkWidget *w, void *ud)
{
    gtk_widget_hide(node_help_shell);
}

GLADE_CALLBACK void
on_node_help_delete_event(GtkWidget *w, GdkEvent *event, void *user)
{
    gtk_widget_hide(node_help_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
show_node_help(cml_node *mn)
{
    const char *text;
    char *banner;
    
    if (node_help_shell == 0)
    {
    	GladeXML *xml = load_widget_tree("node_help");

    	node_help_shell = glade_xml_get_widget(xml, "node_help");
    	node_help_label = glade_xml_get_widget(xml, "node_help_label");
    	node_help_text = glade_xml_get_widget(xml, "node_help_text");
    }
    
    banner = g_strdup_printf("%s: %s",
    	    	    cml_node_get_name(mn), cml_node_get_banner(mn));
    gtk_label_set_text(GTK_LABEL(node_help_label), banner);
    g_free(banner);

    text = cml_node_get_help_text(mn);
    gtk_text_freeze(GTK_TEXT(node_help_text));
    gtk_text_set_point(GTK_TEXT(node_help_text), 0);
    gtk_text_forward_delete(GTK_TEXT(node_help_text),
    	gtk_text_get_length(GTK_TEXT(node_help_text)));
    if (text == 0)
    	text = _("No help available for this node.");
    gtk_text_insert(GTK_TEXT(node_help_text),
		    (GdkFont *)0,
		    (GdkColor *)0,  	/* fore */
		    (GdkColor *)0,  	/* back */
		    text, strlen(text));
    gtk_text_thaw(GTK_TEXT(node_help_text));
    
    gtk_widget_show(node_help_shell);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
