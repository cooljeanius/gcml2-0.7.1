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
 
#ifndef _common_h_
#define _common_h_

#include <stdio.h>
#include <stdlib.h>
#include "libcml.h"
#include "debug.h"
#include <gtk/gtk.h>
#include <libintl.h>
#include <assert.h>
#include <glade/glade.h>

#ifndef PROFILE
#define PROFILE 0
#endif

#define CVSID(s) \
    static const char *__cvsid[2] = { (s), (const char *)__cvsid }
    
/* main.c */
void grey_items(void);
void set_value(cml_node *mn, const cml_atom *valp);
GladeXML *load_widget_tree(const char *root);
int gcml_random(int minv, int maxv);
void push_page(cml_node *mn);
#if PROFILE
void profile_exercise(void);
void time_mark(void);
double time_elapsed(void);
#endif /* PROFILE */

/* logwin.c */
void logwin_init(gboolean stderr_flag);
void logwin_insertv(const char **strp);
void logwin_insert(const char *str);
void logwin_show(void);

/* brokenwin.c */
void brokenwin_init(void);
void brokenwin_update(cml_rulebase *rb);
void brokenwin_show(void);



/* global variables */
#ifdef _DEFINE_GLOBALS
#define EXTERN
#define EQUALS(x) = x
#else
#define EXTERN extern
#define EQUALS(x)
#endif

EXTERN cml_rulebase *rb;
EXTERN gboolean creating EQUALS( FALSE );
EXTERN GtkWidget *main_window;
EXTERN gboolean show_node_name EQUALS( FALSE );
EXTERN gboolean show_node_banner EQUALS( TRUE );
EXTERN gboolean show_node_warnings EQUALS( TRUE );
EXTERN gboolean show_node_status EQUALS( FALSE );
EXTERN gboolean show_suppressed EQUALS( FALSE );
EXTERN gboolean show_node_icons EQUALS( TRUE );



#ifndef _
#define _(x)	gettext(x)
#endif

#define SPACING 4

#define GLADE_CALLBACK extern

#define CALLED() DDPRINTF0(DEBUG_GUI, "called\n")

#endif /* _common_h_ */
