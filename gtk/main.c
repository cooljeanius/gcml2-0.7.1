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
 
#define _DEFINE_GLOBALS
#include "common.h"
#undef _DEFINE_GLOBALS
#include "node_gui.h"
#include "page_gui.h"
#include <gdk_imlib.h>
#include <unistd.h>
#if PROFILE
#include <sys/time.h>
#endif
#include <sys/stat.h>


CVSID("$Id: main.c,v 1.47 2002/09/03 14:17:48 gnb Exp $");

typedef struct
{
    GList *widgets;
} WidgetSet;

static const char *rulebase_filename;
static const char *arch = "i386";
static const char *defconfig_filename;
static gboolean changed = FALSE;
static GtkWidget *edit_undo;
static GtkWidget *edit_redo;
static GtkWidget *edit_freeze_changes;
static GtkWidget *show_node_name_check;
static GtkWidget *show_node_banner_check;
static GtkWidget *show_node_status_check;
static GtkWidget *show_node_icons_check;
static GtkWidget *show_suppressed_check;
static WidgetSet widgets_up;
static WidgetSet widgets_down;
static WidgetSet widgets_save;
static WidgetSet widgets_save_as;
static GtkWidget *load_window;
static GtkWidget *save_as_window;
static GtkWidget *save_query_window;
static GtkWidget *about_window;
static GtkWidget *licence_window;
static gboolean stderr_flag = FALSE;
static gboolean freeze_changes_flag = TRUE;
static char *argv0;


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
gcml_random(int minv, int maxv)
{
    static gboolean init = FALSE;
    int v;
    
    if (!init)
    {
    	srand48(0xdeadbeef);
    	init = TRUE;
    }
    
    v = (int)(drand48() * (maxv - minv) + 0.5) + minv;

    return v;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if PROFILE

static struct timeval mark;

void
time_mark(void)
{
    gettimeofday(&mark, 0);
}

double
time_elapsed(void)
{
    struct timeval now, delta;
    
    gettimeofday(&now, 0);
    
    delta.tv_sec = now.tv_sec - mark.tv_sec;
    if (now.tv_usec > mark.tv_usec)
    {
    	delta.tv_usec = now.tv_usec - mark.tv_usec;
    }
    else
    {
    	delta.tv_usec = 1000000 - now.tv_usec + mark.tv_usec;
	delta.tv_sec--;
    }
    
    return (double)delta.tv_sec + (double)delta.tv_usec / 1.0e6;
}

#endif /* PROFILE */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
widgetset_set_sensitive(WidgetSet *ws, gboolean b)
{
    GList *list;
    
    for (list = ws->widgets ; list != 0 ; list = list->next)
    	gtk_widget_set_sensitive((GtkWidget *)list->data, b);
}

static void
widgetset_add(WidgetSet *ws, GtkWidget *w)
{
    ws->widgets = g_list_prepend(ws->widgets, w);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
grey_items(void)
{
    cml_node *curr = page_gui_get_current();
    cml_node *rootmost = page_gui_get_rootmost();
    cml_node *leafmost = page_gui_get_leafmost();

    DDPRINTF3(DEBUG_GUI, "grey_items: curr=`%s' rootmost=`%s' leafmost=`%s'\n",
    	cml_node_get_name(curr), cml_node_get_name(rootmost), cml_node_get_name(leafmost));
    
    gtk_widget_set_sensitive(edit_undo, cml_rulebase_can_undo(rb));
    gtk_widget_set_sensitive(edit_redo, cml_rulebase_can_redo(rb));
    gtk_widget_set_sensitive(edit_freeze_changes, cml_rulebase_can_freeze(rb));
    
    widgetset_set_sensitive(&widgets_up, (curr != rootmost));
    widgetset_set_sensitive(&widgets_down, (curr != leafmost));
    widgetset_set_sensitive(&widgets_save, changed && (defconfig_filename != 0));
    widgetset_set_sensitive(&widgets_save_as, changed);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
do_load(void)
{
    cml_rulebase_clear(rb);
    if (!cml_rulebase_load_defconfig(rb, defconfig_filename))
    {
    	/* TODO: alert */
    	fprintf(stderr, "Failed to load defconfig \"%s\"\n",
	    	    defconfig_filename);
    	return;
    }
    cml_rulebase_commit(rb, /*freeze*/TRUE);
    changed = FALSE;
    page_gui_update_current();
    grey_items();
}

GLADE_CALLBACK void
on_load_ok_clicked(GtkWidget *w, void *ud)
{
    defconfig_filename = gtk_file_selection_get_filename(
    	    	    	    	GTK_FILE_SELECTION(load_window));
    do_load();
    gtk_widget_hide(load_window);
}

GLADE_CALLBACK void
on_load_cancel_clicked(GtkWidget *w, void *ud)
{
    gtk_widget_hide(load_window);
}

GLADE_CALLBACK void
on_file_load_activate(GtkWidget *w, void *ud)
{
    if (load_window == 0)
    {
	GladeXML *xml = load_widget_tree("load");    
	load_window = glade_xml_get_widget(xml, "load");
    }
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(load_window),
			(defconfig_filename == 0 ? "" : defconfig_filename));
    gtk_widget_show(load_window);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
do_actual_save(void)
{
    cml_rulebase_save_defconfig(rb, defconfig_filename);
    changed = FALSE;
    grey_items();
}

GLADE_CALLBACK void
on_save_as_ok_clicked(GtkWidget *w, void *ud)
{
    defconfig_filename = gtk_file_selection_get_filename(
    	    	    	    	GTK_FILE_SELECTION(save_as_window));
    do_actual_save();
    gtk_widget_hide(save_as_window);
    gtk_main_quit();	/* break from subsidiary event loop */
}

GLADE_CALLBACK void
on_save_as_cancel_clicked(GtkWidget *w, void *ud)
{
    gtk_widget_hide(save_as_window);
    gtk_main_quit();	/* break from subsidiary event loop */
}

static void
do_save_as(void)
{
    if (save_as_window == 0)
    {
	GladeXML *xml = load_widget_tree("save_as");    
	save_as_window = glade_xml_get_widget(xml, "save_as");
    }
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(save_as_window),
			(defconfig_filename == 0 ? "" : defconfig_filename));
    gtk_widget_show(save_as_window);
    
    /*
     * Run a subsidiary event loop to handle the case
     * where we are called from the File->Exit handler.
     */
    gtk_main();
}

static void
do_save(void)
{
    if (defconfig_filename == 0)
    	do_save_as();
    else
    	do_actual_save();
}

GLADE_CALLBACK void
on_file_save_activate(GtkWidget *w, void *ud)
{
    do_save();
}

GLADE_CALLBACK void
on_file_save_as_activate(GtkWidget *w, void *ud)
{
    do_save_as();
}

GLADE_CALLBACK void
on_tool_save_clicked(GtkWidget *w, void *ud)
{
    do_save();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Called from File->Exit and WM_DELETE event.
 */
 
static void
do_quit(void)
{
    creating = TRUE;	/* suppress callbacks */
    gtk_main_quit();	/* break main event loop */
}
 
GLADE_CALLBACK void
on_save_query_save_clicked(GtkWidget *w, void *ud)
{
    do_save();
    if (!changed)
	do_quit();
}

GLADE_CALLBACK void
on_save_query_exit_clicked(GtkWidget *w, void *ud)
{
    do_quit();
}

GLADE_CALLBACK void
on_save_query_cancel_clicked(GtkWidget *w, void *ud)
{
    gtk_widget_hide(save_query_window);
}

GLADE_CALLBACK void
file_exit_cb(GtkWidget *w, void *ud)
{
    if (!changed)
    {
    	do_quit();
    }
    else
    {
	if (save_query_window == 0)
	{
	    GladeXML *xml = load_widget_tree("save_query");    
	    save_query_window = glade_xml_get_widget(xml, "save_query");
	}
	gtk_widget_show(save_query_window);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_edit_undo_activate(GtkWidget *w, void *ud)
{
    cml_rulebase_undo(rb);
    page_gui_update_current();
    grey_items();
}

GLADE_CALLBACK void
on_edit_redo_activate(GtkWidget *w, void *ud)
{
    cml_rulebase_redo(rb);
    page_gui_update_current();
    grey_items();
}

GLADE_CALLBACK void
on_edit_freeze_changes_activate(GtkWidget *w, void *ud)
{
    cml_rulebase_commit(rb, /*freeze*/TRUE);
    changed = TRUE;
    page_gui_update_current();
    grey_items();
}

GLADE_CALLBACK void
on_edit_check_all_rules_activate(GtkWidget *w, void *ud)
{
    cml_rulebase_check_all_rules(rb);
    brokenwin_update(rb);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_licence_close_clicked(GtkWidget *w, void *ud)
{
    gtk_widget_hide(licence_window);
}

static const char licence_txt[] =
#include "licence_txt.c"
;

GLADE_CALLBACK void
on_about_licence_clicked(GtkWidget *w, void *ud)
{
    if (licence_window == 0)
    {
	GladeXML *xml;
	GtkWidget *text;
	
	xml = load_widget_tree("licence");    
	licence_window = glade_xml_get_widget(xml, "licence");
	text = glade_xml_get_widget(xml, "licence_text");
	
	gtk_text_freeze(GTK_TEXT(text));
    	gtk_text_set_point(GTK_TEXT(text), 0);
    	gtk_text_insert(GTK_TEXT(text),
			(GdkFont *)0,
			(GdkColor *)0 /*fore*/,
			(GdkColor *)0 /*back*/,
			licence_txt, strlen(licence_txt)),
	gtk_text_thaw(GTK_TEXT(text));
    }
    gtk_widget_show(licence_window);
}

GLADE_CALLBACK void
on_about_close_clicked(GtkWidget *w, void *ud)
{
    gtk_widget_hide(about_window);
}

GLADE_CALLBACK void
on_help_about_activate(GtkWidget *w, void *ud)
{
    if (about_window == 0)
    {
	GladeXML *xml = load_widget_tree("about");    
	about_window = glade_xml_get_widget(xml, "about");
    }
    gtk_widget_show(about_window);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
set_value(cml_node *mn, const cml_atom *valp)
{
    cml_node_set_value(mn, valp);
    cml_rulebase_commit(rb, freeze_changes_flag);
    page_gui_update_current();
    changed = TRUE;
    grey_items();
    brokenwin_update(rb);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_view_rootmost_activate(GtkWidget *w, gpointer data)
{
    page_gui_move_rootmost();
    grey_items();    
}

GLADE_CALLBACK void
on_view_up_activate(GtkWidget *w, gpointer data)
{
    page_gui_move_up();
    grey_items();    
}

GLADE_CALLBACK void
on_view_down_activate(GtkWidget *w, gpointer data)
{
    page_gui_move_down();
    grey_items();    
}

GLADE_CALLBACK void
on_view_leafmost_activate(GtkWidget *w, gpointer data)
{
    page_gui_move_leafmost();
    grey_items();    
}


GLADE_CALLBACK void
on_tool_rootmost_clicked(GtkWidget *w, gpointer data)
{
    page_gui_move_rootmost();
    grey_items();    
}

GLADE_CALLBACK void
on_tool_up_clicked(GtkWidget *w, gpointer data)
{
    page_gui_move_up();
    grey_items();    
}

GLADE_CALLBACK void
on_tool_down_clicked(GtkWidget *w, gpointer data)
{
    page_gui_move_down();
    grey_items();    
}

GLADE_CALLBACK void
on_tool_leafmost_clicked(GtkWidget *w, gpointer data)
{
    page_gui_move_leafmost();
    grey_items();    
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_show_log_activate(GtkWidget *w, gpointer data)
{
    logwin_show();
}

GLADE_CALLBACK void
on_show_broken_activate(GtkWidget *w, gpointer data)
{
    brokenwin_show();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_show_node_name_check_activate(GtkWidget *w, gpointer data)
{
    if (!creating)
    {
	show_node_name = GTK_CHECK_MENU_ITEM(w)->active;
	page_gui_update_current();
    }
}

GLADE_CALLBACK void
on_show_node_banner_check_activate(GtkWidget *w, gpointer data)
{
    if (!creating)
    {
	show_node_banner = GTK_CHECK_MENU_ITEM(w)->active;
	page_gui_update_current();
    }
}

GLADE_CALLBACK void
on_show_node_status_check_activate(GtkWidget *w, gpointer data)
{
    if (!creating)
    {
	show_node_status = GTK_CHECK_MENU_ITEM(w)->active;
	page_gui_update_current();
    }
}

GLADE_CALLBACK void
on_show_node_icons_check_activate(GtkWidget *w, gpointer data)
{
    if (!creating)
    {
	show_node_icons = GTK_CHECK_MENU_ITEM(w)->active;
	page_gui_update_current();
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_show_suppressed_check_activate(GtkWidget *w, gpointer data)
{
    if (!creating)
    {
	show_suppressed = GTK_CHECK_MENU_ITEM(w)->active;
	page_gui_update_current();
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

GLADE_CALLBACK void
on_rulebase_fail_close_clicked(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#ifndef UI_DEBUG
#define UI_DEBUG 1
#endif

const char *ui_glade_path = 
#if DEBUG || UI_DEBUG
"ui:../gtk/ui:"
#endif
PKGDATADIR;

static char *
find_file(const char *path, const char *base)
{
    char *path_save, *dir, *buf, *file;
    struct stat sb;
    
    buf = path_save = g_strdup(path);
    while ((dir = strtok(buf, ":")) != 0)
    {
    	buf = 0;
	file = g_strconcat(dir, "/", base, 0);

	if (stat(file, &sb) == 0 && S_ISREG(sb.st_mode))
	{
	    g_free(path_save);
	    return file;
	}
	g_free(file);
    }
    
    g_free(path_save);
    return 0;
}

GladeXML *
load_widget_tree(const char *root)
{
    GladeXML *xml = 0;
    static char *filename = 0;
    static const char gladefile[] = PACKAGE ".glade";
    
    
    /* load & create the interface */
    if (filename == 0)
    {
	filename = find_file(ui_glade_path, gladefile);
	if (filename == 0)
	{
	    fprintf(stderr, "gcml2: can't find %s in path %s\n",
	    	gladefile, ui_glade_path);
	    exit(1);
	}
	DDPRINTF1(DEBUG_GUI, "Loading Glade UI from file \"%s\"\n", filename);
    }

    xml = glade_xml_new(filename, root);
    if (xml == 0)
    {
    	fprintf(stderr, "gcml2: can't load Glade UI from file \"%s\"\n",
	    filename);
    	exit(1);
    }
    
    /* connect the signals in the interface */
    glade_xml_signal_autoconnect(xml);

    return xml;
}


static void
set_icon(void)
{
    const cml_blob *blob;
    static char tmpfile[] = "/tmp/gcml2-iconXXXXXX";
    int tmpfd;
    GdkImlibImage *im;
    
    if ((blob = cml_rulebase_get_icon(rb)) == 0)
    	return;
	
    tmpfd = mkstemp(tmpfile);
    write(tmpfd, blob->data, blob->length);
    close(tmpfd);
    
    im = gdk_imlib_load_image(tmpfile);
    gdk_imlib_render(im, im->rgb_width, im->rgb_height);
    gdk_window_set_icon(main_window->window, 0,
    	gdk_imlib_move_image(im),
    	gdk_imlib_move_mask(im));
    gdk_imlib_destroy_image(im);
    
    unlink(tmpfile);
}


static gboolean
delayed_initialise(void *data)
{
    const char *banner;
    char *title;
    

#if PROFILE
    time_mark();
#endif /* PROFILE */
    rb = cml_rulebase_new();
    cml_rulebase_set_arch(rb, arch);
    if (!cml_rulebase_parse(rb, rulebase_filename))
    {
    	GtkWidget *errorwin;
	GladeXML *xml;
	
	xml = load_widget_tree("rulebase_fail");
	errorwin = glade_xml_get_widget(xml, "rulebase_fail");
    	gtk_widget_show(errorwin);
	return FALSE;   /* stop calling me already */
    }
#if PROFILE
    fprintf(stderr, "Parse took %g sec\n", time_elapsed());
#endif /* PROFILE */
    
    gtk_widget_show(main_window);
    if (defconfig_filename != 0)
    {
	if (cml_rulebase_load_defconfig(rb, defconfig_filename))
	    cml_rulebase_commit(rb, /*freeze*/TRUE);
    }
    

    creating = TRUE;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(show_node_name_check),
    	    	    	    	    show_node_name);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(show_node_banner_check),
    	    	    	    	    show_node_banner);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(show_node_status_check),
    	    	    	    	    show_node_status);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(show_node_icons_check),
    	    	    	    	    show_node_icons);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(show_suppressed_check),
    	    	    	    	    show_suppressed);

    if ((banner = cml_rulebase_get_banner(rb)) == 0)
    	banner = rulebase_filename;
    title = g_strdup_printf(_("GCML2: %s"), banner);
    gtk_window_set_title(GTK_WINDOW(main_window), title);
    g_free(title);
    
    set_icon();
    
    page_gui_push(cml_rulebase_get_start(rb));
    grey_items();

    creating = FALSE;
    
#if PROFILE
    profile_exercise();
    creating = TRUE;	/* suppress callbacks */
    gtk_main_quit();
#endif /* PROFILE */

    return FALSE;   /* stop calling me already */
}


static void
ui_init(int *argcp, char ***argvp)
{
    gtk_init(argcp, argvp);
    gdk_imlib_init();
    gtk_widget_push_visual(gdk_imlib_get_visual());
    gtk_widget_push_colormap(gdk_imlib_get_colormap());
}

static void
ui_create(void)
{
    GladeXML *xml;
    GtkWidget *w;
    
    /* initialise libGlade */
    glade_init();
    
    /* load the interface & connect signals */
    xml = load_widget_tree("main");    

    /* extract some widgets we will want to use later */
    main_window = glade_xml_get_widget(xml, "main");
#if PROFILE
    gtk_widget_set_uposition(main_window, 0, 0);
    gtk_widget_set_usize(main_window, gdk_screen_width(), gdk_screen_height());
#endif /* PROFILE */

    widgetset_add(&widgets_save, glade_xml_get_widget(xml, "file_save"));
    widgetset_add(&widgets_save_as, glade_xml_get_widget(xml, "file_save_as"));
    widgetset_add(&widgets_save, glade_xml_get_widget(xml, "tool_save"));
    edit_undo = glade_xml_get_widget(xml, "edit_undo");
    edit_redo = glade_xml_get_widget(xml, "edit_redo");
    edit_freeze_changes = glade_xml_get_widget(xml, "edit_freeze_changes");
    
    /* HACK to work around bug in Mandrake 7.1 libglade */
    w = glade_xml_get_widget(xml, "toolbar");
    gtk_toolbar_set_button_relief(GTK_TOOLBAR(w), GTK_RELIEF_NONE);
    
    /* destroy the dummy page we get from libglade */
    w = glade_xml_get_widget(xml, "notebook");
    page_gui_init(w);
    w = gtk_notebook_get_nth_page(GTK_NOTEBOOK(w), 0);
    gtk_widget_destroy(w);

#if DEBUG
    if (!(debug & DEBUG_GUI))
#endif
    {
	GtkWidget *debug_menu = glade_xml_get_widget(xml, "debug_menu");
	if (debug_menu != 0)
	    gtk_widget_hide(debug_menu);
    }
#if TESTSCRIPT - 0 == 0
    w = glade_xml_get_widget(xml, "debug_run_test");
    if (w != 0)
	gtk_widget_hide(w);
#endif

    show_node_name_check = glade_xml_get_widget(xml, "show_node_name_check");
    show_node_banner_check = glade_xml_get_widget(xml, "show_node_banner_check");
    show_node_status_check = glade_xml_get_widget(xml, "show_node_status_check");
    show_node_icons_check = glade_xml_get_widget(xml, "show_node_icons_check");
    show_suppressed_check = glade_xml_get_widget(xml, "show_suppressed_check");

    widgetset_add(&widgets_up, glade_xml_get_widget(xml, "view_rootmost"));
    widgetset_add(&widgets_up, glade_xml_get_widget(xml, "view_up"));
    widgetset_add(&widgets_down, glade_xml_get_widget(xml, "view_down"));
    widgetset_add(&widgets_down, glade_xml_get_widget(xml, "view_leafmost"));

    widgetset_add(&widgets_up, glade_xml_get_widget(xml, "tool_rootmost"));
    widgetset_add(&widgets_up, glade_xml_get_widget(xml, "tool_up"));
    widgetset_add(&widgets_down, glade_xml_get_widget(xml, "tool_down"));
    widgetset_add(&widgets_down, glade_xml_get_widget(xml, "tool_leafmost"));

    logwin_init(stderr_flag);
    brokenwin_init();

    delayed_initialise(0);
}


#if 0
static void
parse_cml2_options(int argc, char **argv)
{
    int i;
    
    for (i=1 ; i<argc ; i++)
    {
    	if (argv[i][0] == '-')
	{
	    switch (argv[i][1])
	    {
	    case 'h':
	    	/* TODO: proper filename search semantics */
	    	rb->output_header_file = g_strdup(argv[++i]);
		break;
	    case 's':
	    	/* TODO: proper filename search semantics */
	    	rb->output_defconfig = g_strdup(argv[++i]);
		break;
	    }
	}
    }
}
#endif

static const char usage_str[] = 
"Usage: cml-gtk [options] filename.cml\n"
"  options are:\n"
"--arch ARCH   	for CML1, set the $ARCH variable\n"
"--help   	print this message and exit\n"
"--version   	print version number and exit\n"
"--stderr   	log all messages to stderr as well as log window\n"
"plus all standard GTK options\n"
;

static void
usagef(int ec, const char *fmt, ...)
{
    if (fmt != 0)
    {
	va_list args;

    	fprintf(stderr, "%s: ", argv0);	
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
	va_end(args);
    }

    fputs(usage_str, stderr);
    fflush(stderr); /* JIC */
    exit(ec);
}


static void
parse_args(int argc, char **argv)
{
    int i;
    int nfiles = 0;
    
    argv0 = argv[0];
    
    for (i=1 ; i<argc ; i++)
    {
    	if (argv[i][0] == '-')
	{
	    if (!strcmp(argv[i], "--version"))
	    {
	    	printf("GCML2 version %s\n", VERSION);
	    	exit(0);
	    }
	    else if (!strcmp(argv[i], "--help"))
	    {
	    	usagef(0, 0);
	    }
	    else if (!strcmp(argv[i], "--stderr"))
	    {
	    	stderr_flag = TRUE;
	    }
	    else if (!strcmp(argv[i], "--arch"))
	    {
		if (++i == argc)
    		    usagef(1, "Expecting argument for --arch\n");
	    	arch = argv[i];
	    }
	    else if (!strcmp(argv[i], "--debug"))
	    {
		if (++i == argc)
    		    usagef(1, "Expecting argument for --debug\n");
	    	debug_set(argv[i]);
	    }
	    else
	    {
	    	usagef(1, "unknown option \"%s\"", argv[i]);
	    }
	}
	else
	{
	    switch (++nfiles)
	    {
	    case 1:
	    	rulebase_filename = argv[i];
		break;
	    case 2:
	    	defconfig_filename = argv[i];
		break;
	    default:
	    	usagef(1, "too many filenames at \"%s\"", argv[i]);
		break;
	    }
	}
    }

    if (rulebase_filename == 0)
	usagef(1, "a rulebase filename must be specified");
}


int
main(int argc, char **argv)
{
    ui_init(&argc, &argv);
    parse_args(argc, argv);
    ui_create();
    gtk_main();
    return 0;
}
