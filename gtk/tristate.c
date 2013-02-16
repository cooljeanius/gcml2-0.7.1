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

CVSID("$Id: tristate.c,v 1.6 2002/06/07 15:39:39 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char *
tristate_str(int b)
{
    static char *strs[3];
    int i;
    
    if (strs[0] == 0)
    {
	for (i=0 ; i<3 ; i++)
	{
    	    cml_atom val;

	    val.type = A_TRISTATE;
	    val.value.tritval = i;
	    strs[i] = cml_atom_value_as_string(&val);
	}
    }
    
    return strs[b];
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
on_tristate_toggle_toggled(GtkWidget *w, void *ud)
{
    cml_atom val;
    int i;
    node_gui_t *ng = (node_gui_t *)ud;
    cml_atom_type type;
    
    if (creating || !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
    	return;

    CALLED();
    type = cml_node_get_value(ng->node)->type;

    cml_atom_init(&val);
    for (i=CML_N ; i<= CML_M ; i++)
    {
	if (w == ng->toggles[i])
	{
	    val.type = type;
    	    val.value.tritval = i;
	}
	else if (ng->toggles[i] != 0)
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), FALSE);
    }
    
    set_value(ng->node, &val);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
build_toggle(GtkWidget *hbox, node_gui_t *ng, int i)
{
    GtkWidget *tog;

    tog = gtk_toggle_button_new_with_label(tristate_str(i));
    gtk_box_pack_start(GTK_BOX(hbox), tog,
    	    	    	/*expand*/TRUE, /*fill*/TRUE, /*padding*/0);
			
    gtk_signal_connect(GTK_OBJECT(tog), "toggled", 
    	    	    	GTK_SIGNAL_FUNC(on_tristate_toggle_toggled), ng);
			
    gtk_widget_show(tog);
    ng->toggles[i] = tog;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
boolean_build_value(node_gui_t *ng)
{
    GtkWidget *hbox;
    
    hbox = gtk_hbox_new(/*homogeneous*/TRUE, /*spacing*/0);
    
    build_toggle(hbox, ng, CML_Y);
    build_toggle(hbox, ng, CML_N);

    node_gui_attach_widget(ng, hbox, VALUE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
tristate_build_value(node_gui_t *ng)
{
    GtkWidget *hbox;

    hbox = gtk_hbox_new(/*homogeneous*/TRUE, /*spacing*/0);
    
    build_toggle(hbox, ng, CML_Y);
    build_toggle(hbox, ng, CML_M);   /* TODO: check tied */
    build_toggle(hbox, ng, CML_N);

    node_gui_attach_widget(ng, hbox, VALUE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
tristate_update_value(node_gui_t *ng)
{
    const cml_atom *ap = cml_node_get_value(ng->node);
    int i;
    
    for (i=0 ; i<3 ; i++)
    	if (ng->toggles[i] != 0)
    	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ng->toggles[i]),
	    	    	    	    	 (i == ap->value.tritval));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define FORCE_CHANGE 0

static void
tristate_simulate(node_gui_t *ng)
{
    int val;
    GtkWidget *tog;
    
    for (;;)
    {
    	val = gcml_random(0,2);
	/* fprintf(stderr, "gcml_random(0,2)=%d\n", val); */
	if ((tog = ng->toggles[val]) != 0
#if FORCE_CHANGE	
	     && !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tog))
#endif
	     )
	{
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tog), TRUE);
	    return;
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

node_gui_ops_t boolean_ops = 
{
    node_gui_default_format_label,
    node_gui_default_build_arrow,
    boolean_build_value,
    tristate_update_value,
    tristate_simulate
};

node_gui_ops_t tristate_ops = 
{
    node_gui_default_format_label,
    node_gui_default_build_arrow,
    tristate_build_value,
    tristate_update_value,
    tristate_simulate
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
