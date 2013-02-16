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
 
#include <stdio.h>
#include "libcml.h"
#include "debug.h"
#include <stdlib.h>
#include <unistd.h>

static char *argv0;
static char *arch = "i386";
static char *rulebase_filename = "rules.cml";
static char *defconfig_filename = ".config";
static gboolean freeze_flag = TRUE;

#define INDENT 4

#if !PROFILE && !COVERAGE
#define NORMAL 1
#endif

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

static cml_visit_result
profile_visitor(cml_rulebase *rb, cml_node *mn, int depth, void *user_data)
{
    static int bools[2] = { BV_Y, BV_Y };
    static int trits[3] = { TV_Y, TV_M, TV_N };
    const cml_atom *val;
    cml_atom a;
    gboolean doit = FALSE;
    
    if (!cml_node_is_visible(mn) || cml_node_is_radio(mn))
    	return CML_SKIP;
    
    switch (cml_node_get_treetype(mn))
    {
    case MN_SYMBOL:
    	val = cml_node_get_value(mn);
	a = *val;
    	switch (a.type)
	{
	case A_BOOLEAN:
	    a.value.boolean = bools[gcml_random(0,1)];
	    doit = (a.value.boolean != val->value.boolean);
	    break;
	case A_TRISTATE:
	    a.value.tritval = trits[gcml_random(0,2)];
	    doit = (a.value.tritval != val->value.tritval);
	    break;
	default:
	    break;
	}
    	break;
    default:
	break;
    }
    
    if (doit)
    {
    	DDPRINTF2(DEBUG_GUI, "Setting %s=%s\n",
	    cml_node_get_name(mn),
	    cml_atom_value_as_string(&a)/*LEAK*/);
	cml_node_set_value(mn, &a);
	cml_rulebase_commit(rb, /*freeze*/FALSE);
    }
    
    return CML_CONTINUE;
}

#endif
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#if NORMAL

typedef struct
{
    char *name;
    cml_atom value;
} choice_t;

static GList *choices = 0;  /* list of choice_t */
static choice_t *choice_current = 0;

static void
choice_clear(void)
{
    while (choices != 0)
    {
    	choice_t *ch = (choice_t *)choices->data;
	
	g_free(ch->name);
	g_free(ch);
	choices = g_list_remove_link(choices, choices);
    }
    choice_current = 0;
}


static choice_t *
choice_add(const char *name, const cml_atom *value)
{
    choice_t *ch;
    
    ch = g_new(choice_t, 1);
    
    ch->name = g_strdup(name);
    ch->value = *value;
    
    DDPRINTF2(DEBUG_GUI, "choice_add(\"%s\", %s)\n",
    	    	name, cml_atom_value_as_string(value));
    
    choices = g_list_append(choices, ch);
    
    return ch;
}

static choice_t *
choice_lookup(const char *str)
{
    GList *list;
    
    if (str == 0 || *str == '\0')
    	return choice_current != 0 ?
	    	    choice_current : (choice_t *)choices->data;

    for (list = choices ; list != 0 ; list = list->next)
    {
    	choice_t *ch = (choice_t *)list->data;
	
	if (!strcasecmp(ch->name, str))
	    return ch;
    }
    
    return 0;
}

static void
choice_set_current(choice_t *ch)
{
    choice_current = ch;
}

#define CHOICE_LENGTH_THRESHOLD     32
static gboolean
choice_is_long(void)
{
    int len;
    GList *list;
    
    len = 0;
    for (list = choices ; list != 0 ; list = list->next)
    {
    	choice_t *ch = (choice_t *)list->data;
	
	if (strpbrk(ch->name, " \t\n\r"))
	    return TRUE;
	len += strlen(ch->name);
    }

    return (len > CHOICE_LENGTH_THRESHOLD);
}

static char *
choice_summary(void)
{
    GList *list;
    char *p, *str;
    int len;
    choice_t *def;
    
    if (choices == 0)
    	return 0;
	
    def = choice_current;
    if (def == 0)
    	def = (choice_t *)choices->data;
    
    /* calculate length */
    len = 0;
    for (list = choices ; list != 0 ; list = list->next)
    {
    	choice_t *ch = (choice_t *)list->data;
	
	len += strlen(ch->name) + /* | or \0 */1;
    }
    len += 2; /* [ ] */
    
    p = str = g_malloc(len);
    
    for (list = choices ; list != 0 ; list = list->next)
    {
    	choice_t *ch = (choice_t *)list->data;
	
	if (p != str)
	    *p++ = ' ';
	if (ch == def)
	    *p++ = '[';
	strcpy(p, ch->name);
	p += strlen(p);
	if (ch == def)
	    *p++ = ']';
    }
    *p = '\0';
    
    return str;
}

static char *
choice_menu(int depth)
{
    GList *list;
    char *p, *str;
    int len;
    int n, i;
    choice_t *def;
    
    if (choices == 0)
    	return 0;
	
    def = choice_current;
    if (def == 0)
    	def = (choice_t *)choices->data;
    
    /* calculate length */
    len = 1; /* \n */
    for (list = choices ; list != 0 ; list = list->next)
    {
    	choice_t *ch = (choice_t *)list->data;
	
	len += 4*INDENT + /* [n] */5 + strlen(ch->name) + /* \n */1;
    }
    len += 1; /* \0 */
    
    p = str = g_malloc(len);
    
    n = 0;
    *p++ = '\n';
    for (list = choices ; list != 0 ; list = list->next)
    {
    	choice_t *ch = (choice_t *)list->data;
	
	for (i=0 ; i<depth*INDENT ; i++)
	    *p++ = ' ';
	    
	*p++ = (ch == def ? '[' : ' ');
	sprintf(p, "%2d", ++n); p += strlen(p);
	*p++ = (ch == def ? ']' : ' ');
	*p++ = ' ';
	
	strcpy(p, ch->name); p += strlen(p);
	*p++ = '\n';
    }
    *p = '\0';
    
    return str;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
get_value(cml_rulebase *rb, cml_node *mn, gboolean islong)
{
    cml_atom a;
    GList *list;
    char *p, buf[2048];
    
    if (fgets(buf, sizeof(buf), stdin) == 0)
    {
    	fprintf(stderr, "\n%s: quitting\n", argv0);
	exit(0);
    }
    if ((p = strrchr(buf, '\n')) != 0)
    	*p = '\0';
    if ((p = strrchr(buf, '\r')) != 0)
    	*p = '\0';
    
    /* TODO: strip spurious whitespace */
    
    if (choices != 0)
    {
    	/* handle limited range of values */
	choice_t *ch;
	
	if ((ch = choice_lookup(buf)) == 0)
	{
	    int n;
	    
	    if (!islong)
		return FALSE;
	    if (sscanf(buf, "%d", &n) != 1)
		return FALSE;
	    if ((ch = g_list_nth_data(choices, n-1)) == 0)
		return FALSE;
	}
	a = ch->value;
    }
    else
    {
    	if (buf[0] == '\0')
	    a = *cml_node_get_value(mn);
	else
	{
	    a.type = cml_node_get_value(mn)->type;
	    if (!cml_atom_from_string(&a, buf))
    		return FALSE;
    	}
    }
    
    cml_node_set_value(mn, &a);
    	
    if (cml_rulebase_commit(rb, freeze_flag))
    	return TRUE;
	
    list = cml_rulebase_get_broken_rules(rb);
    if (list != 0)
    {
    	printf("--> %d broken rules:\n", g_list_length(list));
	while (list != 0)
	{
	    cml_rule *rule = (cml_rule *)list->data;
	    char *explanation = cml_rule_get_explanation(rule);
	    printf("    %s\n", explanation);
	    g_free(explanation);
	    list = g_list_remove_link(list, list);
	}
	return FALSE;
    }
    
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static cml_visit_result
query_visitor(cml_rulebase *rb, cml_node *mn, int depth, void *user_data)
{
    int i;
    cml_atom a;
    const cml_atom *ap;
    const GList *list;
    char *choicestr;
    gboolean islong;
    gboolean hasval = TRUE;
    gboolean done;
    choice_t *ch, *trit_ch[3];
    cml_visit_result res = CML_CONTINUE;
    const char *banner;
    
    if (!cml_node_is_visible(mn))
    	return CML_SKIP;
	
    choice_clear();
    
    switch (cml_node_get_treetype(mn))
    {
    case MN_SYMBOL:
    	ap = cml_node_get_value(mn);
	a.type = cml_node_get_value_type(mn);
    	switch (a.type)
	{
	case A_BOOLEAN:
	    a.value.tritval = CML_Y;
	    trit_ch[CML_Y] = choice_add("y", &a);
	    a.value.tritval = CML_N;
	    trit_ch[CML_N] = choice_add("n", &a);
	    choice_set_current(trit_ch[ap->value.tritval]);
	    break;
	case A_TRISTATE:
	    a.value.tritval = CML_Y;
	    trit_ch[CML_Y] = choice_add("y", &a);
	    a.value.tritval = CML_M;
	    trit_ch[CML_M] = choice_add("m", &a);
	    a.value.tritval = CML_N;
	    trit_ch[CML_N] = choice_add("n", &a);
	    choice_set_current(trit_ch[ap->value.tritval]);
	    break;
	case A_DECIMAL:
	case A_HEXADECIMAL:
	    list = cml_node_get_enumdefs(mn);
	    if (list != 0)
	    {
		for ( ; list != 0 ; list = list->next)
		{
		    cml_enumdef *ed = (cml_enumdef *)list->data;
		    a.value.integer = ed->value;
		    ch = choice_add(cml_node_get_banner(ed->symbol), &a);
		    if (ap->value.integer == ed->value)
		    	choice_set_current(ch);
		}
	    }
	    else
	    {
		for (list = cml_node_get_range(mn) ; list != 0 ; list = list->next)
		{
		    cml_subrange *sr = (cml_subrange *)list->data;
		    for (i = sr->begin ; i <= sr->end ; i++)
		    {
			char valuebuf[32];
			sprintf(valuebuf, (ap->type == A_DECIMAL ? "%ld" : "0x%lX"), i);
    	    	    	a.value.integer = i;
			ch = choice_add(valuebuf, &a);
			if (ap->value.integer == i)
		    	    choice_set_current(ch);
		    }
		}
	    }
	    break;
	default:
	    break;
	}
    	break;
    case MN_MENU:
    	if (cml_node_get_children(mn) == 0)
	    hasval = FALSE;
	else if (cml_node_is_radio(mn))
	{
	    res = CML_SKIP;
	    for (list = cml_node_get_children(mn) ; list != 0 ; list = list->next)
	    {
	    	cml_node *child = (cml_node *)list->data;
		a.type = ap->type;
		a.value.node = child;
		ch = choice_add(cml_node_get_banner(child), &a);
		if (ap->value.node == child)
		    choice_set_current(ch);
	    }
	}
	else
	    hasval = FALSE;
	break;
    default:
	break;
    }
    
    islong = choice_is_long();
    choicestr = (islong ? choice_menu(depth) : choice_summary());
    
    banner = cml_node_get_banner(mn);
    if (banner == 0)
    	banner = cml_node_get_name(mn);

    done = FALSE;
    do
    {
	for (i=0 ; i<depth*INDENT ; i++)
    	    putchar(' ');
	fputs(banner, stdout);
	if (hasval)
	{
	    if (islong)
	    {
	    	fputs(choicestr, stdout);
		for (i=0 ; i<depth*INDENT ; i++)
    		    putchar(' ');
	    }
	    else if (choicestr != 0)
	    {
    		printf(" (%s)", choicestr);
	    }
	    else
	    {
	    	char *str = cml_atom_value_as_string(ap);
	    	printf(" [%s]", str);
		g_free(str);
	    }
	    fputs(" ? ", stdout);
	    fflush(stdout);

	    if (get_value(rb, mn, islong))
	    	done = TRUE;
	}
	else
	{
	    fputc('\n', stdout);
	    done = TRUE;
	}
    }
    while (!done);
    
    if (choicestr != 0) 
    	g_free(choicestr);
    
    return res;
}

#endif /* NORMAL */
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
    cml_rulebase *rb;
    const char *banner;
    
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
    	fprintf(stderr, "%s: failed to load defconfig \"%s\"\n",
	    	    argv0, defconfig_filename);
	exit(1);
    }
    cml_rulebase_commit(rb, /*freeze*/FALSE);
    
    if ((banner = cml_rulebase_get_banner(rb)) != 0)
    	printf("%s\n", banner);
	
#if PROFILE
    cml_rulebase_menu_apply(rb, profile_visitor, 0);
    cml_rulebase_save_defconfig(rb, "profile.config");
#elif COVERAGE
    if (!cml_rulebase_run_test(rb))
    	exit(1);
#else

#if DEBUG
    if ((debug & DEBUG_SKIPGUI))
    	return 0;
#endif

    cml_rulebase_menu_apply(rb, query_visitor, 0);

    if (!cml_rulebase_save_defconfig(rb, defconfig_filename))
    {
    	fprintf(stderr, "%s: failed to save defconfig \"%s\"\n",
	    	    argv0, defconfig_filename);
	exit(1);
    }
#endif
    
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
