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
 
#ifndef _node_gui_h_
#define _node_gui_h_

typedef struct node_gui_s   	node_gui_t;
typedef struct node_gui_ops_s   node_gui_ops_t;


struct node_gui_ops_s
{
    char *(*format_label)(node_gui_t *);
    void (*build_arrow)(node_gui_t *);
    void (*build_value)(node_gui_t *);
    void (*update_value)(node_gui_t *);
    void (*simulate)(node_gui_t *);
};

enum NodeGuiColumn { LABEL, ARROW, VALUE, HELP };
#define NUM_COLUMNS 4

struct node_gui_s
{
    node_gui_ops_t *ops;
    cml_node *node;
    GtkWidget *table;
    int row;
    GtkWidget *colwidgets[NUM_COLUMNS];
    GtkWidget *label;	    /* banner, bool, tristate, string, int, hex */
    GtkWidget *button;	    /* menu */
    GtkWidget *entry;	    /* string; or combo box' entry */
    GtkWidget *combo;	    /* radio, limited_int or limited_hex */
    GtkWidget *toggles[3];  /* bool, tristate */
    GtkWidget *child_page;  /* menu */
};



void node_gui_attach_widget(node_gui_t *ng, GtkWidget *child,
    	    	    	    enum NodeGuiColumn col);
node_gui_t *node_gui_new(cml_node *mn);
void node_gui_set_page(node_gui_t *ng, GtkWidget *page);
node_gui_t *node_gui_from_widget(GtkWidget *w);
node_gui_t *node_gui_get_current(void);
void node_gui_set_current(node_gui_t *ng);
char *node_gui_default_format_label(node_gui_t *ng);
void node_gui_default_build_arrow(node_gui_t *ng);
void show_node_help(cml_node *mn);
void node_gui_build(node_gui_t *ng, GtkWidget *table, int row);
void node_gui_update_label(node_gui_t *ng);
void node_gui_update(node_gui_t *ng);


#endif /* _node_gui_h_ */
