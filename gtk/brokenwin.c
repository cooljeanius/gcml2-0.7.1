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

CVSID("$Id: brokenwin.c,v 1.1 2001/12/05 07:39:44 gnb Exp $");

static GtkWidget *broken_list;
static GtkWidget *broken_window;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void brokenwin_init(void)
{
    GladeXML *xml;
    
    xml = load_widget_tree("broken");    
    broken_window = glade_xml_get_widget(xml, "broken");
    broken_list = glade_xml_get_widget(xml, "broken_list");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
brokenwin_update(cml_rulebase *rb)
{
    GList *list;
    gboolean showit;
    
    list = cml_rulebase_get_broken_rules(rb);
    showit = (list != 0);
    
    gtk_clist_freeze(GTK_CLIST(broken_list));
    gtk_clist_clear(GTK_CLIST(broken_list));
    while (list != 0)
    {
    	cml_rule *rule = (cml_rule *)list->data;
	char *txt;
	
	txt = cml_rule_get_explanation(rule);
	gtk_clist_append(GTK_CLIST(broken_list), &txt);
	
    	list = g_list_remove_link(list, list);
    }
    gtk_clist_thaw(GTK_CLIST(broken_list));
    
    if (showit)
    	gtk_widget_show(broken_window);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_broken_close_clicked(GtkWidget *w, void *ud)
{
    gtk_widget_hide(broken_window);
}

void
brokenwin_show(void)
{
    gtk_widget_show(broken_window);
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
