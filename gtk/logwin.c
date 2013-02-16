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

CVSID("$Id: logwin.c,v 1.3 2001/12/10 08:31:39 gnb Exp $");

static GtkWidget *logwin_window;
static GtkWidget *logwin_text;
static gboolean logwin_stderr_flag;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char *severity_strings[_CML_MAX_SEVERITY] =
{
"info: ", "warning: ", "error: "
};

static void
logwin_error_func(
    cml_severity sev,
    const cml_location *loc,
    const char *fmt,
    va_list args)
{
    char *msg;
    const char *txt[16];
    char linenobuf[32];
    int i = 0;
    
    txt[i++] = severity_strings[sev];
    if (loc != 0)
    {
    	if (loc->filename != 0)
	    txt[i++] = loc->filename;
    	if (loc->filename != 0 && loc->lineno != 0)
	    txt[i++] = ":";
    	if (loc->lineno != 0)
	{
	    g_snprintf(linenobuf, sizeof(linenobuf), "%d", loc->lineno);
	    txt[i++] = linenobuf;
	}
    	if (loc->filename != 0 || loc->lineno != 0)
	    txt[i++] = ": ";
    }
    txt[i++] = msg = g_strdup_vprintf(fmt, args);
    txt[i++] = "\n";
    txt[i] = 0;

    logwin_insertv(txt);
        
    if (msg != 0)
	g_free(msg);
}

void
logwin_init(gboolean stderr_flag)
{
    GladeXML *xml;
    
    logwin_stderr_flag = stderr_flag;
    
    xml = load_widget_tree("log");    
    logwin_window = glade_xml_get_widget(xml, "log");
    logwin_text = glade_xml_get_widget(xml, "log_text");

    cml_set_error_func(logwin_error_func);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
logwin_insertv(const char **strp)
{
    if (logwin_text == 0)
    	return;
	
    for ( ; *strp != 0 ; strp++)
    {
	gtk_text_insert(GTK_TEXT(logwin_text),
    	    /*font*/0, /*fore*/0, /*back*/0,
	    *strp, strlen(*strp));
	if (logwin_stderr_flag)
	    fputs(*strp, stderr);
    }
    if (logwin_stderr_flag)
    	fflush(stderr);
	
    gtk_widget_show(logwin_window);
#if PROFILE
    /* keep main window on top */
    gdk_window_lower(logwin_window->window);
#endif
}

void
logwin_insert(const char *str)
{
    const char *txt[2];
    
    txt[0] = str;
    txt[1] = 0;
    logwin_insertv(txt);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_log_close_clicked(GtkWidget *w, void *ud)
{
    gtk_widget_hide(logwin_window);
}


void
logwin_show(void)
{
    gtk_widget_show(logwin_window);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
