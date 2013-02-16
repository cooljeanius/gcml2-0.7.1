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

CVSID("$Id: node_gui.c,v 1.10 2002/08/18 00:49:24 gnb Exp $");

#define NODE_GUI_KEY	"node_gui_key"

static GtkTooltips *tooltips;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern node_gui_ops_t boolean_ops;
extern node_gui_ops_t tristate_ops;
extern node_gui_ops_t choice_ops;
extern node_gui_ops_t hex_ops;
extern node_gui_ops_t dec_ops;
extern node_gui_ops_t limited_hex_ops;
extern node_gui_ops_t limited_dec_ops;
extern node_gui_ops_t string_ops;
extern node_gui_ops_t banner_ops;
extern node_gui_ops_t menu_ops;
extern node_gui_ops_t unknown_ops;

#define LIMITED_MAX 32

static gboolean
is_limited(cml_node *mn)
{
    unsigned count;

    if (cml_node_get_enumdefs(mn) != 0)
    	return TRUE;
	
    count = cml_node_get_range_count(mn);
    return (count > 0 && count < LIMITED_MAX);
}

static node_gui_ops_t *
node_gui_classify(cml_node *mn)
{
    const cml_atom *ap;

    switch (cml_node_get_treetype(mn))
    {
    case MN_SYMBOL:
        ap = cml_node_get_value(mn);
    	switch (cml_node_get_value_type(mn))
	{
	case A_BOOLEAN:
	    return &boolean_ops;
	case A_TRISTATE:
	    return &tristate_ops;
	case A_DECIMAL:
	    return (is_limited(mn) ? &limited_dec_ops : &dec_ops);
	case A_HEXADECIMAL:
	    return (is_limited(mn) ? &limited_hex_ops : &hex_ops);
	case A_STRING:
	    return &string_ops;
	default:
	    return &unknown_ops;
	}
    	break;
    case MN_MENU:
    	if (cml_node_get_children(mn) == 0)
	    return &banner_ops;
	else if (cml_node_is_radio(mn))
	    return &choice_ops;
	else
	    return &menu_ops;
	break;
    default:
	return &unknown_ops;
    }
    return &unknown_ops;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

node_gui_t *
node_gui_new(cml_node *mn)
{
    node_gui_t *ng;
    
    ng = g_new(node_gui_t, 1);
    memset(ng, 0, sizeof(*ng));
    
    ng->node = mn;
    cml_node_set_user_data(mn, ng);
    
    ng->ops = node_gui_classify(ng->node);
    
    return ng;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
node_gui_set_page(node_gui_t *ng, GtkWidget *page)
{
    ng->child_page = page;
    gtk_object_set_data(GTK_OBJECT(page), NODE_GUI_KEY, ng);
}

node_gui_t *
node_gui_from_widget(GtkWidget *w)
{
    while (w != 0)
    {
	node_gui_t *ng = (node_gui_t *)gtk_object_get_data(GTK_OBJECT(w), NODE_GUI_KEY);
	if (ng != 0)
	    return ng;
	w = w->parent;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
node_gui_default_format_label(node_gui_t *ng)
{
    char *txt;
    int n = 0;
    char *strs[32+2*CML_NODE_MAX_STATES];

    if (show_node_name)
    {
    	strs[n++] = "[";
    	strs[n++] = (char *)cml_node_get_name(ng->node);
    	strs[n++] = "]";
    }
	
    if (show_node_banner)
    	strs[n++] = (char *)cml_node_get_banner(ng->node);
    
    if (show_node_status)
    {
    	int i;
	char *states[CML_NODE_MAX_STATES];
	int nstates = cml_node_get_states(ng->node, CML_NODE_MAX_STATES, states);

    	strs[n++] = "{";
	for (i = 0 ; i < nstates ; i++)
	{
	    strs[n++] = " ";
	    strs[n++] = states[i];
	}

    	if (cml_node_is_frozen(ng->node))
	    strs[n++] = " FROZEN";
    	if (!cml_node_is_visible(ng->node))
	    strs[n++] = " SUPPRESSED";
    	strs[n++] = " }";
    }
    
    strs[n++] = ":";
    strs[n] = 0;
    
    txt = g_strjoinv(" ", strs);

    return txt;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
node_gui_attach_widget(
    node_gui_t *ng,
    GtkWidget *child,
    enum NodeGuiColumn col)
{
    static const int xoptions[NUM_COLUMNS] = { GTK_FILL, GTK_FILL, GTK_EXPAND|GTK_FILL, GTK_FILL };
    static const int xpadding[NUM_COLUMNS] = { SPACING, 0, SPACING, 0 };
    
    if (col == VALUE)
    {
	GtkWidget *fixed = gtk_fixed_new();
	gtk_fixed_put(GTK_FIXED(fixed), child, 0, 0);
	gtk_widget_show(child);
	child = fixed;
    }
    
    gtk_table_attach(GTK_TABLE(ng->table), child,
    	col, col+1, ng->row, ng->row+1,
	xoptions[col], 0,
	xpadding[col], (SPACING/2));
    gtk_widget_show(child);
    
    ng->colwidgets[col] = child;
    
    gtk_object_set_data(GTK_OBJECT(child), NODE_GUI_KEY, ng);
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#include "ui/right_arrow_clip.xpm"
static GdkPixmap *right_pm;
static GdkBitmap *right_mask;

static void
on_menu_button_clicked(GtkWidget *w, void *ud)
{
    node_gui_t *ng = (node_gui_t *)ud;
    
    if (creating)
    	return;

    CALLED();
    page_gui_push(ng->node);
    grey_items();
}


void
node_gui_default_build_arrow(node_gui_t *ng)
{
    GtkWidget *pixmap;
    GtkWidget *button;
    char *tip;
    
    button = gtk_button_new();

    if (cml_node_get_children(ng->node) != 0)
    {
	if (right_pm == 0)
	    right_pm = gdk_pixmap_create_from_xpm_d(main_window->window,
	    	    		&right_mask, 0, right_arrow_clip_xpm);

	pixmap = gtk_pixmap_new(right_pm, right_mask);
	gtk_widget_show(pixmap);
	gtk_container_add(GTK_CONTAINER(button), pixmap);

	gtk_signal_connect(GTK_OBJECT(button), "clicked", 
    	    	    	    GTK_SIGNAL_FUNC(on_menu_button_clicked), ng);

	node_gui_attach_widget(ng, button, ARROW);
	
	if (tooltips == 0)
	    tooltips = gtk_tooltips_new();
	tip = g_strdup_printf(_("Enter \"%s\" menu"),
	    cml_node_get_banner(ng->node));
	gtk_tooltips_set_tip(tooltips, button, tip, 0);
	g_free(tip);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
on_help_button_clicked(GtkWidget *w, gpointer user)
{
    node_gui_t *ng = (node_gui_t *)user;
    
    show_node_help(ng->node);
}

#include "ui/help.xpm"
static GdkPixmap *help_pm;
static GdkBitmap *help_mask;

static void
node_gui_build_help(node_gui_t *ng)
{
    GtkWidget *pixmap;
    GtkWidget *button;
    char *tip;
    
    button = gtk_button_new();

    if (cml_node_get_help_text(ng->node) != 0)
    {
	if (help_pm == 0)
	    help_pm = gdk_pixmap_create_from_xpm_d(main_window->window,
	    	    		&help_mask, 0, help_xpm);

	pixmap = gtk_pixmap_new(help_pm, help_mask);
	gtk_widget_show(pixmap);
	gtk_container_add(GTK_CONTAINER(button), pixmap);

	gtk_signal_connect(GTK_OBJECT(button), "clicked", 
    	    	    	    GTK_SIGNAL_FUNC(on_help_button_clicked), ng);

	node_gui_attach_widget(ng, button, HELP);
	
	if (tooltips == 0)
	    tooltips = gtk_tooltips_new();
	tip = g_strdup_printf(_("Get help on \"%s\""),
	    cml_node_get_banner(ng->node));
	gtk_tooltips_set_tip(tooltips, button, tip, 0);
	g_free(tip);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
node_gui_build(node_gui_t *ng, GtkWidget *table, int row)
{
    char *txt;
    
    ng->table = table;
    ng->row = row;
    
    creating = TRUE;
    
    txt = (*ng->ops->format_label)(ng);
    ng->label = gtk_label_new(txt);
    gtk_label_set_justify(GTK_LABEL(ng->label), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(ng->label), 1.0, 0.5);
    node_gui_attach_widget(ng, ng->label, LABEL);

    (*ng->ops->build_arrow)(ng);
    node_gui_build_help(ng);
    (*ng->ops->build_value)(ng);
    
    creating = FALSE;

    g_free(txt);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
node_has_visible_children(cml_node *mn)
{
    const GList *list;
    
    for (list = cml_node_get_children(mn) ; list != 0 ; list = list->next)
    {
    	cml_node *child = (cml_node *)list->data;
	
	if (cml_node_is_visible(child))
	    return TRUE;
    }
    return FALSE;
}

static void
gtk_widget_set_visible(GtkWidget *w, gboolean vis)
{
    if (vis)
	gtk_widget_show(w);
    else
	gtk_widget_hide(w);
}


static gboolean
node_is_visible(node_gui_t *ng)
{
    if (!show_suppressed && !cml_node_is_visible(ng->node))
    	return FALSE;
	
    if (ng->colwidgets[VALUE] == 0 &&
    	!node_has_visible_children(ng->node))
    	return FALSE;
	
    return TRUE;
}

void
node_gui_update(node_gui_t *ng)
{
    int i;
    char *label;
    
    if (!node_is_visible(ng))
    {
    	/* nothing to see here, move along */
	for (i=0 ; i<NUM_COLUMNS ; i++)
	{
    	    if (ng->colwidgets[i] != 0)
    		gtk_widget_set_visible(ng->colwidgets[i], FALSE);
	}
    	return;
    }
    
    /* node is visible: widgets are potentially visible */
    gtk_widget_set_visible(ng->colwidgets[LABEL], TRUE);
    if (ng->colwidgets[ARROW] != 0)
	gtk_widget_set_visible(ng->colwidgets[ARROW],
	    	    	       node_has_visible_children(ng->node));
    if (ng->colwidgets[VALUE] != 0)
	gtk_widget_set_visible(ng->colwidgets[VALUE], TRUE);
    if (ng->colwidgets[HELP] != 0)
	gtk_widget_set_visible(ng->colwidgets[HELP], TRUE);

    label = (*ng->ops->format_label)(ng);
    gtk_label_set_text(GTK_LABEL(ng->label), label);
    g_free(label);
    
    creating = TRUE;
    (*ng->ops->update_value)(ng);
    creating = FALSE;
}



/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
