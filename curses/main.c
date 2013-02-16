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
#undef MIN  	/* fmeh */
#undef MAX
#include "strvec.h"
#include "dialog.h"
#include "libcml.h"
#include "debug.h"
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/errno.h>

CVSID("$Id: main.c,v 1.17 2002/07/22 13:58:58 gnb Exp $");

static char *argv0;
static char *arch = "i386";
static char *rulebase_filename = "rules.cml";
static char *defconfig_filename = ".config";
static gboolean freeze_flag = TRUE;
static cml_rulebase *rb;
static gboolean changed = FALSE;


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define height	    	    (LINES-4)
#define width	    	    (COLS-5)
#define list_height	    (LINES-14)
#define menu_height	    (LINES-14)
#define input_height	    (10)
#define input_width	    (COLS-5)
#define load_height	    (11)
#define load_width	    (55)
#define save_height	    (5)
#define save_width	    (60)
#define saveas_height	    (10)
#define saveas_width	    (55)
#define error_height        (3)
#define error_width         (38)

#if DEBUG
#define DEBUG_RET() \
    if (debug & DEBUG_GUI) \
    { \
	fprintf(stderr, "\n\nRETURN=%d\nSTDERR=\"%s\"\n", retcode, retstr); \
	fflush(stderr); \
    }
#else
#define DEBUG_RET()
#endif

static void capture_init(void);
static void capture_begin(void);
static char *capture_end(void);
static void capture_to_file(const char *which, char **filenamep);
static void capture_to_file_end(void);

static void show_menu(cml_node *mn);
static void show_radio(cml_node *mn);
static void show_string(cml_node *mn);
static void show_int(cml_node *mn);
static void show_text_help(const char *title, const char *helptext);
static void show_node_help(cml_node *mn);

static const char inputbox_instructions_string[] = "\
Please enter a string value. \
Use the <TAB> key to move from the input field to the buttons below it.";

static const char inputbox_instructions_dec[] = "\
Please enter a decimal value. \
Fractions will not be accepted.  \
Use the <TAB> key to move from the input field to the buttons below it.";

static const char inputbox_instructions_hex[] = "\
Please enter a hexadecimal value. \
Use the <TAB> key to move from the input field to the buttons below it.";

static const char radio_instructions[] = "\
Use the arrow keys to navigate this window or \
press the hotkey of the item you wish to select \
followed by the <SPACE BAR>. \
Press <?> for additional information about this option.";

static const char menu_instructions[] = "\
Arrow keys navigate the menu.  \
<Enter> selects submenus --->.  \
Highlighted letters are hotkeys.  \
Pressing <Y> includes, <N> excludes, <M> modularizes features.  \
Press <Esc><Esc> to exit, <?> for Help.  \
Legend: [*] built-in  [ ] excluded  <M> module  < > module capable";

static const char load_instructions[] = "\
Enter the name of the configuration file you wish to load.  \
Accept the name shown to restore the configuration you \
last retrieved.  Leave blank to abort.";

static const char load_help[] = "\
\n\
For various reasons, one may wish to keep several different kernel\n\
configurations available on a single machine.  \n\
\n\
If you have saved a previous configuration in a file other than the\n\
kernel's default, entering the name of the file here will allow you\n\
to modify that configuration.\n\
\n\
If you are uncertain, then you have probably never used alternate \n\
configuration files.  You should therefor leave this blank to abort.\n\
\n\
";

static const char saveas_instructions[] = "\
Enter a filename to which this configuration should be saved \
as an alternate.  Leave blank to abort.\
";

static const char save_help[] = "\
\n\
For various reasons, one may wish to keep different kernel\n\
configurations available on a single machine.  \n\
\n\
Entering a file name here will allow you to later retrieve, modify\n\
and use the current configuration as an alternate to whatever \n\
configuration options you have selected at that time.\n\
\n\
If you are uncertain what all this means then you should probably\n\
leave this blank.\n\
";

static const char menuconfig_help[] = 
#include "README.Menuconfig.h"
;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static cml_node *
find_child(cml_node *mn, const char *name)
{
    GList *list;
    
    for (list = cml_node_get_children(mn) ; list != 0 ; list = list->next)
    {
    	cml_node *child = (cml_node *)list->data;
	
    	if (!strcmp(name, cml_node_get_name(child)))
	    return child;
    }
    
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static FILE *
fopen_tmp(const char *which, char **filenamep)
{
    char *tmpdir = "/tmp";
    char *filename;
    int fd;
    FILE *fp;
    
    filename = g_strconcat(tmpdir, "/gcml2-curses-", which, "XXXXXXX", 0);
    
    if ((fd = mkstemp(filename)) < 0)
    {
    	perror(filename);
	g_free(filename);
	return 0;
    }
    
    if ((fp = fdopen(fd, "w")) == 0)
    {
    	perror(filename);
	close(fd);
	g_free(filename);
	return 0;
    }

    *filenamep = filename;
    return fp;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
show_broken_rules(void)
{
    FILE *fp;
    GList *list;
    char *retstr;
    char *filename = 0;
    int retcode;
    
    list = cml_rulebase_get_broken_rules(rb);
    if (list == 0)
    	return;

    fp = fopen_tmp("rules", &filename);
    if (fp == 0)
    	return;
    while (list != 0)
    {
    	cml_rule *rule = (cml_rule *)list->data;
    	char *p, *text = cml_rule_get_explanation(rule);
	int n;
	
	for (p = text, n = 0 ; *p ; p++, n++)
	{
	    if (n == width - 2)
	    {
	    	n = 0;
		fputc('\n', fp);
	    }
	    fputc(*p, fp);
	}
	if (n)
	    fputc('\n', fp);
	fputc('\n', fp);
	g_free(text);
    	list = g_list_remove_link(list, list);
    }
    fclose(fp);
    
    capture_begin();
    dialog_clear();
    retcode = dialog_textbox(
    	"Broken Rules",    	    	/* title */
	filename,   	    	    	/* file */
	height, width); 	    	/* height, width */
    retstr = capture_end();

    DEBUG_RET();

    unlink(filename);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
set_value(cml_node *mn, const cml_atom *ap)
{
#if DEBUG
    fprintf(stderr, "set_value(%s, %s)\n",
    	cml_node_get_name(mn),
	cml_atom_value_as_string(ap)/*memleak*/);
#endif

    cml_node_set_value(mn, ap);
    
    if (!cml_rulebase_commit(rb, freeze_flag))
    {
    	fprintf(stderr, "Waaaaaah! Rulebase kacked itself.\n");
	show_broken_rules();
    }
    else
	changed = TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static char *
trim(char *s)
{
    char *p;
    
    for ( ; *s && isspace(*s) ; s++)
    	;
	
    for (p = s+strlen(s)-1 ; p != s && isspace(*p) ; --p)
    	;
    p[1] = '\0';
    
    return s;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
show_error(const char *error)
{
    beep();
    dialog_clear();
    dialog_msgbox(
	0,	    	    	    /* title */
	error,	    	    	    /* prompt */
	error_height, strlen(error)+7,  /* height, width */
	0);     	    	    /* pause */
    sleep(2);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
show_load(void)
{
    char *retstr;
    int retcode;
    gboolean done = FALSE;
    char *filename;

    do
    {
	capture_begin();
	dialog_clear();
	retcode = dialog_inputbox(
    	    0,	    	    	    	    	/* title */
	    load_instructions,	    	    	/* prompt */
	    load_height, load_width,	    	/* height, width */
	    defconfig_filename);    	    	/* string */
	retstr = capture_end();

    	DEBUG_RET();

	switch (retcode)
	{
	case 0: /* `Ok' button */
	    filename = trim(dialog_input_result);
	    if (filename[0] == '\0')
	    {
	    	/* empty is cancel. */
	    	done = TRUE;
		break;
	    }
	    if (!cml_rulebase_load_defconfig(rb, filename))
	    {
	    	show_error("File does not exist!"/*TODO*/);
		continue;
	    }
	    if (!cml_rulebase_commit(rb, /*freeze*/FALSE))
	    {
	    	show_error("File has errors!"/*TODO*/);
		continue;
	    }
	    done = TRUE;
    	    break;
	case 1: /* `Help' button */
	    show_text_help("Load Alternate Configuration", load_help);
    	    break;
	}
    }
    while (!done);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
show_saveas(void)
{
    char *retstr;
    int retcode;
    gboolean done = FALSE;
    char *filename;

    do
    {
	capture_begin();
	dialog_clear();
	retcode = dialog_inputbox(
    	    0,	    	    	    	    	/* title */
	    saveas_instructions,    	    	/* prompt */
	    saveas_height, saveas_width,    	/* height, width */
	    "");     	    	    	    	/* string */
	retstr = capture_end();

    	DEBUG_RET();

	switch (retcode)
	{
	case 0: /* `Ok' button */
	    filename = trim(dialog_input_result);
	    if (filename[0] == '\0')
	    {
	    	/* empty is cancel. */
	    	done = TRUE;
		break;
	    }
	    if (!cml_rulebase_save_defconfig(rb, filename))
	    {
	    	show_error("Can't create file!  Probably a nonexistent directory.");
		continue;
	    }
	    changed = FALSE;
	    done = TRUE;
    	    break;
	case 1: /* `Help' button */
	    show_text_help("Save Alternate Configuration", save_help);
    	    break;
	}
    }
    while (!done);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if TESTSCRIPT

static void
show_runtest(void)
{
    gboolean success;
    char *filename = 0;
    int retcode;
    char *retstr;

    capture_to_file("test", &filename);
    success = cml_rulebase_run_test(rb);
    fprintf(stderr, "\nRulebase test script %s\n",
    	    	    	(success ? "PASSED" : "**FAILED**"));
    capture_to_file_end();


    if (!success)
	beep();

    capture_begin();
    dialog_clear();
    retcode = dialog_textbox(
    	"Rulebase Test Script",     	/* title */
	filename,   	    	    	/* file */
	height, width);     	    	/* height, width */
    retstr = capture_end();

    DEBUG_RET();

    unlink(filename);
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
show_text_help(const char *title, const char *helptext)
{
    FILE *fp;
    char *retstr;
    int retcode;
    char *filename = 0;
    
    if (helptext == 0)
    	return;
    
    fp = fopen_tmp("help", &filename);
    if (fp == 0)
    	return;
    fputs(helptext, fp);
    fclose(fp);
    
    capture_begin();
    retcode = dialog_textbox(
    	title,    	    	    	/* title */
	filename,   	    	    	/* file */
	height, width);	    	    	/* height, width */
    retstr = capture_end();

    DEBUG_RET();


    unlink(filename);
}

static void
show_node_help(cml_node *mn)
{
    show_text_help(cml_node_get_banner(mn), cml_node_get_help_text(mn));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
show_string(cml_node *mn)
{
    const cml_atom *ap;
    char *retstr;
    int retcode;
    cml_atom a;

    ap = cml_node_get_value(mn);

    for (;;)
    {
	capture_begin();
	dialog_clear();
	retcode = dialog_inputbox(
    	    cml_node_get_banner(mn),	    	/*title*/
	    inputbox_instructions_string,   	/*prompt*/
	    input_height, input_width,	    	/* height, width */
	    ap->value.string);	    	    	/* string */
	retstr = capture_end();

    	DEBUG_RET();

	switch (retcode)
	{
	case 0: /* `Ok' button */
	    a.type = A_STRING;
	    a.value.string = dialog_input_result;
	    set_value(mn, &a);
    	    return;
	case 1: /* `Help' button */
	    show_node_help(mn);
    	    break;
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
show_simple_int(cml_node *mn)
{
    const cml_atom *ap;
    char *valstr;
    char *retstr;
    int retcode;
    cml_atom a;
    gboolean done = FALSE;
    gboolean ishex;
    
    ap = cml_node_get_value(mn);
    ishex = (cml_node_get_value_type(mn) == A_HEXADECIMAL);
    valstr = cml_atom_value_as_string(ap);

    do
    {
	capture_begin();
	dialog_clear();
	retcode = dialog_inputbox(
    	    cml_node_get_banner(mn),	    	/*title*/
	    (ishex ?
	    	inputbox_instructions_hex :
		inputbox_instructions_dec),   	/*prompt*/
	    input_height, input_width,	    	/* height, width */
	    valstr);	    	    	    	/* string */
	retstr = capture_end();

    	DEBUG_RET();

	switch (retcode)
	{
	case 0: /* `Ok' button */
	    a.type = cml_node_get_value_type(mn);
	    /* TODO: format check */
	    a.value.integer = strtol(dialog_input_result, 0, (ishex ? 16 : 10));
	    set_value(mn, &a);
	    done = TRUE;
    	    break;
	case 1: /* `Help' button */
	    show_node_help(mn);
    	    break;
	}
    }
    while (!done);
    
    g_free(valstr);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define LIMITED_MAX 32

static void
show_int(cml_node *mn)
{
    const GList *list;
    strvec_t menu;
    int nitems;
    const cml_atom *ap;
    char *retstr;
    int retcode;
    cml_atom a;
    gboolean done = FALSE;
    gboolean ishex;

    strvec_init(&menu);
    
    ap = cml_node_get_value(mn);
    ishex = (cml_node_get_value_type(mn) == A_HEXADECIMAL);

    if ((list = cml_node_get_enumdefs(mn)) != 0)
    {
	strvec_preallocate(&menu, 3*g_list_length((GList *)list));

	for ( ; list != 0 ; list = list->next)
	{
	    cml_enumdef *ed = (cml_enumdef *)list->data;

	    strvec_appendf(&menu, (ishex ? "0x%lX" : "%ld"), ed->value);
	    strvec_append(&menu, cml_node_get_banner(ed->symbol));
	    /* TODO: deal with alias problem */
	    strvec_append(&menu, ap->value.integer == ed->value ? "ON" : "OFF");
	}
    }
    else if ((nitems = cml_node_get_range_count(mn)) > 0 &&
             nitems < LIMITED_MAX)
    {
	strvec_preallocate(&menu, 3*nitems);
	
	for (list = cml_node_get_range(mn) ; list != 0 ; list = list->next)
    	{
	    cml_subrange *sr = (cml_subrange *)list->data;
	    int value;
	    
	    for (value = sr->begin ; value <= sr->end ; value++)
	    {
	    	char *valstr = g_strdup_printf((ishex ? "0x%lX" : "%ld"), value);
		strvec_append(&menu, valstr);
		strvec_appendm(&menu, valstr);
		/* TODO: deal with alias problem */
		strvec_append(&menu, (ap->value.integer == value ? "ON" : "OFF"));
	    }
	}
    }
    else
    {
    	/* TODO: something needs to check long ranges */
    	show_simple_int(mn);
	return;
    }
    
    
    do
    {

	capture_begin();
	retcode = dialog_checklist(
    	    cml_node_get_banner(mn),    	/*title*/
	    radio_instructions,  	    	/*prompt*/
	    height, width, list_height,	    	/* height, width, list_height */
	    menu.nitems/3, (const char * const *)menu.items, /* item_no, items */
	    FLAG_CHECK);
	retstr = capture_end();

    	DEBUG_RET();

	switch (retcode)
	{
	case 0: /* `Select' button */
	    if (retstr != 0)
	    {
		a.type = cml_node_get_value_type(mn);
		/*
		 * Stupid bloody output is the old node name and the
		 * new node name, quoted, in their order of appearance
		 * in the menu.  So have to futz about to figure out
		 * the new one.
		 */
		a.value.integer = strtol(strtok(retstr, "\""), 0, (ishex ? 16 : 10));
		if (a.value.integer == ap->value.integer)
		{
	    	    /* second name output is the new one */
		    strtok(0, "\"");    /* skip the space */
		    a.value.integer = strtol(strtok(0, "\""), 0, (ishex ? 16 : 10));
		}
		set_value(mn, &a);
    	    }
    	    done = TRUE;
	    break;
	case 1: /* `Help' button */
	    show_node_help(mn);
    	    break;
	}
    }
    while (!done);
    
    strvec_free(&menu);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
show_radio(cml_node *mn)
{
    GList *list;
    strvec_t menu;
    const cml_atom *ap;
    char *retstr;
    int retcode;
    cml_atom a;
    cml_node *child;
    gboolean done = FALSE;

    list = cml_node_get_children(mn);
    
    strvec_init(&menu);
    strvec_preallocate(&menu, 3*g_list_length(list));
    
    for ( ; list != 0 ; list = list->next)
    {
    	child = (cml_node *)list->data;

    	/* Note: we deliberately don't allow radio children to be invisible */
	/* TODO: ask esr about that */	
	ap = cml_node_get_value(child);
    	assert(ap->type == A_BOOLEAN);
	
	strvec_append(&menu, cml_node_get_name(child));
	strvec_append(&menu, cml_node_get_banner(child));
	strvec_append(&menu, (ap->value.tritval ? "ON" : "OFF"));
    }
    
    do
    {

	capture_begin();
	retcode = dialog_checklist(
    	    cml_node_get_banner(mn),    	/*title*/
	    radio_instructions,  	    	/*prompt*/
	    height, width, list_height,	    	/* height, width, list_height */
	    menu.nitems/3, (const char * const *)menu.items, /* item_no, items */
	    0);
	retstr = capture_end();

    	DEBUG_RET();

	switch (retcode)
	{
	case 0: /* `Select' button */
	    if (retstr != 0)
	    {
		a.type = A_BOOLEAN;
		a.value.tritval = CML_Y;
		/*
		 * Stupid bloody output is the old node name and the
		 * new node name, quoted, in their order of appearance
		 * in the menu.  So have to futz about to figure out
		 * the new one.
		 */
		child = find_child(mn, strtok(retstr, "\""));
		assert(child != 0);
		if (child != cml_node_get_value(mn)->value.node)
		{
	    	    /* first name output is the new one */
		    set_value(child, &a);
		}
		else
		{
	    	    /* second name output is the new one */
		    strtok(0, "\"");    /* skip the space */
		    set_value(find_child(mn, strtok(0, "\"")), &a);
		}
	    }
    	    done = TRUE;
	    break;
	case 1: /* `Help' button */
	    show_node_help(mn);
    	    break;
	}
    }
    while (!done);
    
    strvec_free(&menu);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
build_node_state_string(const cml_node *mn)
{
    int i, n, len;
    char *str, *p;
    char *states[CML_NODE_MAX_STATES];
    
    n = cml_node_get_states(mn, CML_NODE_MAX_STATES, states);
    if (n == 0)
    	return g_strdup("");

    len = 1;	/* allow for trailing \0 */
    for (i = 0 ; i < n ; i++)
    	len += strlen(states[i])+3;

    p = str = g_malloc(len);
    for (i = 0 ; i < n ; i++)
    {
    	sprintf(p, " (%s)", states[i]);
	p += strlen(p);
    }
    
    return str;
}

#define safestr(s)  ((s) == 0 ? "" : (s))

static char *
build_menustring(cml_node *mn)
{
    const cml_atom *ap;
    char *valstr;
    char *menustring = 0;
    const char *banner;
    static const char *bool_strs[3] = {"[ ]", "[*]", "???"};
    static const char *trit_strs[3] = {"< >", "<*>", "<M>"};
    char *statestr;
    
    banner = cml_node_get_banner(mn);
    ap = cml_node_get_value(mn);
    valstr = cml_atom_value_as_string(ap);
    statestr = build_node_state_string(mn);

    switch (cml_node_get_treetype(mn))
    {
    case MN_SYMBOL:

    	switch (cml_node_get_value_type(mn))
	{
	case A_BOOLEAN:
	    menustring = g_strdup_printf("%s %s%s",
	    	    bool_strs[ap->value.tritval], banner, statestr);
	    break;
	case A_TRISTATE:
	    menustring = g_strdup_printf("%s %s%s",
	    	    trit_strs[ap->value.tritval], banner, statestr);
	    break;
	case A_STRING:
	    menustring = g_strdup_printf("    %s: \"%s\"%s",
	    	    banner, safestr(ap->value.string), statestr);
	    break;

	case A_DECIMAL:
	case A_HEXADECIMAL:
	default:
	    menustring = g_strdup_printf("(%s) %s%s",
	    	    valstr, banner, statestr);
	    break;
	}

	break;
    case MN_MENU:
    	if (cml_node_get_children(mn) == 0)
	{
	    /* banner */
	    menustring = g_strdup_printf("--- %s", banner);
	}
	else if (cml_node_is_radio(mn))
	{
	    /* choices */
	    menustring = g_strdup_printf("(%s) %s", valstr, banner);
	}
	else
	{
	    /* menu */
	    menustring = g_strdup_printf("%s --->", banner);
    	    /* TODO: handle the case of no visible children */
	}
	break;
    default:
	break;
    };

    if (valstr != 0)
	g_free(valstr);
    g_free(statestr);
    return menustring;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
handle_command_event(const char *command, char event)
{
    if (event == '?')
    {
	show_text_help("Menuconfig", menuconfig_help);
	return;
    }
    
    if (!strcmp(command, "#load"))
    {
	if (event == ' ' || event == '\n')
    	    show_load();
    }
    else if (!strcmp(command, "#saveas"))
    {
	if (event == ' ' || event == '\n')
    	    show_saveas();
    }
#if TESTSCRIPT
    else if (!strcmp(command, "#runtest"))
    {
	if (event == ' ' || event == '\n')
    	    show_runtest();
    }
#endif    
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
handle_event(cml_node *mn, char event)
{
    const cml_atom *ap;
    cml_atom a;
    static const cml_tritval trit_next[3] = { CML_M, CML_N, CML_Y };
    
    if (event == '?')
    {
	show_node_help(mn);
    	return;
    }
    
    ap = cml_node_get_value(mn);

    switch (cml_node_get_treetype(mn))
    {
    case MN_SYMBOL:

    	switch (cml_node_get_value_type(mn))
	{
	case A_BOOLEAN:
	    a.type = A_BOOLEAN;
	    switch (event)
	    {
	    case '\n':
	    case ' ':
	    	a.value.tritval = !ap->value.tritval;
		set_value(mn, &a);
		break;
	    case 'y':
	    	a.value.tritval = CML_Y;
		set_value(mn, &a);
		break;
	    case 'n':
	    	a.value.tritval = CML_N;
		set_value(mn, &a);
		break;
	    }
	    break;
	    
	case A_TRISTATE:
	    a.type = A_TRISTATE;
	    switch (event)
	    {
	    case '\n':
	    case ' ':
	    	a.value.tritval = trit_next[ap->value.tritval];
		set_value(mn, &a);
		break;
	    case 'y':
	    	a.value.tritval = CML_Y;
		set_value(mn, &a);
		break;
	    case 'n':
	    	a.value.tritval = CML_N;
		set_value(mn, &a);
		break;
	    case 'm':
	    	a.value.tritval = CML_M;
		set_value(mn, &a);
		break;
	    }
	    break;

	case A_STRING:
	    if (event == '\n' || event == ' ')
		show_string(mn);
	    break;
	    
	case A_DECIMAL:
	case A_HEXADECIMAL:
	    if (event == '\n' || event == ' ')
		show_int(mn);
	    break;

	default:
	    break;
	}

	break;
    case MN_MENU:
    	if (cml_node_get_children(mn) == 0)
	{
	    /* banner */
	}
	else if (cml_node_is_radio(mn))
	{
	    /* choices */
	    if (event == '\n' || event == ' ')
		show_radio(mn);
	}
	else
	{
	    /* menu */
	    if (event == '\n' || event == ' ')
		show_menu(mn);
	}
	break;
    default:
	break;
    };
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
show_menu(cml_node *mn)
{
    GList *list;
    strvec_t menu;
    char *retstr;
    int retcode;
    cml_node *selected = 0;
    static char retcode2key[] = {
	'\n', /* 0: `Select' button */
	'q',  /* 1: `Exit' button */
	'?',  /* 2: `Help' button */
	'y',  /* 3: `y' key */
	'n',  /* 4: `n' key */
	'm',  /* 5: `m' key */
	' ',  /* 6: ` ' key (toggle) */
    };

    strvec_init(&menu);
    strvec_preallocate(&menu, g_list_length(cml_node_get_children(mn)));
    
    for (;;)
    {
	strvec_truncate(&menu);
	for (list = cml_node_get_children(mn) ; list != 0 ; list = list->next)
	{
	    char *menustring;
    	    cml_node *child = (cml_node *)list->data;

	    if (!cml_node_is_visible(child))
		continue;

	    if ((menustring = build_menustring(child)) == 0)
	    	continue;

    	    strvec_append(&menu, cml_node_get_name(child));
    	    strvec_appendm(&menu, menustring);
	}
	
	if (mn == cml_rulebase_get_start(rb))
	{
	    /* root menu bletchery */
	    strvec_append(&menu, "#sep");
	    strvec_append(&menu, "--- ");

	    strvec_append(&menu, "#load");
	    strvec_append(&menu, "Load an Alternate Configuration File");

	    strvec_append(&menu, "#saveas");
	    strvec_append(&menu, "Save Configuration to an Alternate File");

#if TESTSCRIPT
	    strvec_append(&menu, "#runtest");
	    strvec_append(&menu, "Run Test Script");
#endif
	}
	
	if (menu.nitems == 0)
	    break;  	/* empty menu */

	capture_begin();
	retcode = dialog_menu(
    	    cml_node_get_banner(mn),    	/*title*/
	    menu_instructions,  	    	/*prompt*/
	    height, width, menu_height,	    	/* height, width, menu_height */
	    (selected == 0 ? "" : cml_node_get_name(selected)),/* default */
	    menu.nitems/2, (const char * const *)menu.items); /* item_no, items */
	retstr = capture_end();

    	DEBUG_RET();

    	if (retcode == -1/* ESC key */ || retcode == 1/* `Exit' button */)
	    break;
	    
    	if (retstr[0] == '#')
	    handle_command_event(retstr, retcode2key[retcode]);
	else
	{
    	    selected = find_child(mn, trim(retstr));
	    handle_event(selected, retcode2key[retcode]);
	}
    }
    
    strvec_free(&menu);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Initialise the godawful hack required to get output from
 * lxdialog, which comes out on stderr.  No, really.
 */
 
static int cap_fd;
static FILE *cap_real_stderr, *cap_fake_stderr;
static char cap_buf[4096/*TODO*/];

static int
set_nonblocking(int fd)
{
    int flags;
    
    if ((flags = fcntl(fd, F_GETFL)) < 0)
    	return -1;
    if (fcntl(fd, F_SETFL, (long)(flags | O_NONBLOCK)) < 0)
    	return -1;
    return 0;
}

static void
capture_init(void)
{
#define PIPEREAD  0
#define PIPEWRITE 1
    int pipefds[2];
    
    if (pipe(pipefds) < 0)
    {
    	perror("pipe");
	exit(1);
    }
    
    cap_fd = pipefds[PIPEREAD];
    if (set_nonblocking(cap_fd) < 0)
    {
    	perror("nonblocking");
	exit(1);
    }
    
    
    cap_fake_stderr = fdopen(pipefds[PIPEWRITE], "w");
#undef PIPEREAD
#undef PIPEWRITE
}

static void
capture_begin(void)
{
    cap_real_stderr = stderr;
    stderr = cap_fake_stderr;
}

static char *
capture_end(void)
{
    int n;
    char *p;
    
    fflush(stderr);
    stderr = cap_real_stderr;
    n = read(cap_fd, cap_buf, sizeof(cap_buf)-1);
    if (n < 0)
    {
    	if (errno == EWOULDBLOCK)
	    return 0;
    	fprintf(stderr, "Godammit, lxdialog output failed\n");
    	exit(1);
    }
    cap_buf[n] = '\0';
    if ((p = strrchr(cap_buf, '\r')) != 0)
    	*p = '\0';
    if ((p = strrchr(cap_buf, '\n')) != 0)
    	*p = '\0';
    return cap_buf;
}


static void
capture_to_file(const char *which, char **filenamep)
{
    cap_real_stderr = stderr;
    stderr = fopen_tmp(which, filenamep);
}

static void
capture_to_file_end(void)
{
    fclose(stderr);
    stderr = cap_real_stderr;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
ui_run(void)
{
    gboolean saved = FALSE;
    int retcode;

    /*
     * Initialise the UI
     */    
    backtitle = cml_rulebase_get_banner(rb);
    init_dialog();
    capture_init();
    
    /*
     * Show the root menu
     */
    show_menu(cml_rulebase_get_start(rb));
    
    /*
     * Deal with saving any changes.
     */
    
    if (changed)
    {
	dialog_clear();
	retcode = dialog_yesno(
    	    0,  	    	    	    /* title */
	    "Do you wish to save your new kernel configuration?",	/* prompt */
	    save_height, save_width);   /* height, width */
	    
	if (retcode == 0/* `Yes' button */)
	{
	    if (!(saved = cml_rulebase_save_defconfig(rb, defconfig_filename)))
	    	perror(defconfig_filename); 	/* TODO */
	}
    }

    dialog_clear();
    end_dialog();
    
    /*
     * Give the user an idea of what just happened.
     */
    
    if (changed && !saved)
    {
    	printf("\n\nYour kernel configuration changes were NOT saved.\n\n");
    }
    else if (!changed)
    {
    	printf("\n\nYou made no changes to `%s'.\n\n", defconfig_filename);
    }
    else
    {
    	cml_node *modversions;
	
    	printf("\n\n*** End of Linux kernel configuration.\n");
    	printf("*** Check the top-level Makefile for additional configuration.\n");
    	/* TODO: wtf is .hdepend */
	modversions = cml_rulebase_find_node(rb, "MODVERSIONS");
	if (modversions != 0 && cml_node_get_value(modversions)->value.tritval)
	    printf("*** Next, you must run 'make dep'.\n");
	else
	    printf("*** Next, you may run 'make bzImage', 'make bzdisk', or 'make install'.\n");
	printf("\n");
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char usage_str[] = 
"Usage: %s [--arch arch] [rules-file [defconfig-file]]\n"
;

static void
usagef(int ec, const char *fmt, ...)
{
    if (fmt != 0)
    {
	va_list args;

	va_start(args, fmt);
	fprintf(stderr, "%s: ", argv0);
	vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
	va_end(args);
    }
    
    fprintf(stderr, usage_str, argv0);

    fflush(stderr); /* JIC */
    
    exit(ec);
}

static void
parse_args(int argc, char **argv)
{
    int i;
    int nfiles = 0;
    
    argv0 = argv[0];
    
    for (i = 1 ; i < argc ; i++)
    {
    	if (argv[i][0] == '-')
	{
	    if (!strcmp(argv[i], "--help"))
	    {
	    	usagef(0, 0);
	    }
	    else if (!strcmp(argv[i], "--debug"))
	    {
		if (++i == argc)
    		    usagef(1, "Expecting argument for --debug\n");
	    	debug_set(argv[i]);
	    }
	    else if (!strcmp(argv[i], "--arch"))
	    {
		if (++i == argc)
    		    usagef(1, "Expecting argument for --arch\n");
	    	arch = argv[i];
	    }
	    else
	    	usagef(1, "Unknown option \"%s\"", argv[i]);
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
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
    parse_args(argc, argv);
    
    rb = cml_rulebase_new();
    cml_rulebase_set_arch(rb, arch);
    if (!cml_rulebase_parse(rb, rulebase_filename))
    {
    	fprintf(stderr, "%s: failed to load rulebase \"%s\"\n",
	    	    argv0, rulebase_filename);
	exit(1);
    }
    
    if (!cml_rulebase_load_defconfig(rb, defconfig_filename))
    {
    	fprintf(stderr, "%s: failed to load \"%s\"\n",
	    	    argv0, defconfig_filename);
    }
    cml_rulebase_commit(rb, /*freeze*/FALSE);
	
#if DEBUG
    if ((debug & DEBUG_SKIPGUI))
    	return 0;
#endif

    ui_run();
    
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
