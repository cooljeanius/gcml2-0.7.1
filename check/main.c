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
#if PROFILE
#include <sys/time.h>
#endif

static char *argv0;
static char *arch = "i386";
static char **files;
static int nfiles;
static char *xref_filename = 0;

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
pre_parse(void)
{
#if PROFILE
    time_mark();
#endif /* PROFILE */
}

static void
post_parse(void)
{
#if PROFILE
    FILE *fp;
    int i;
    static const char filename[] = "cml-check.time";
	
    if ((fp = fopen(filename, "a")) == 0)
    {
	perror(filename);
	return;
    }
    fprintf(fp, "%g", time_elapsed());
    for (i = 0 ; i < nfiles ; i++)
    	fprintf(fp, " %s", files[i]);
    fprintf(fp, "\n");
    fclose(fp);
#endif /* PROFILE */
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const char usage_str[] = 
"Usage: %s [--arch arch] [--xref file] rulesfile [rulesfile...]\n"
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

static int num_warnings = -1;
static signed char *warnings;

static void
parse_warning_opt(const char *orig_opt)
{
    const char *opt = orig_opt;
    int dir = 1;
    int id;

    if (opt == 0 || *opt == '\0')
    	usagef(1, "expecting argument for -W or --warning\n");
    
    if (num_warnings < 0)
    {
    	num_warnings = cml_warning_get_num();
	assert(num_warnings > 0);
	warnings = g_malloc0(sizeof(signed char) * num_warnings);
    }
    
    if (!strcmp(opt, "default"))
    {
    	for (id = 0 ; id < num_warnings ; id++)
	    warnings[id] = 0;
    	return;
    }

    if (!strncmp(opt, "no-", 3))
    {
    	dir = -1;
	opt += 3;
    }
    if (!strcmp(opt, "all"))
    {
    	for (id = 0 ; id < num_warnings ; id++)
	    warnings[id] = dir;
    	return;
    }
    
    if ((id = cml_warning_id_by_name(opt)) < 0)
    	usagef(1, "unknown warning or error \"%s\"\n", orig_opt);

    warnings[id] = dir;
}

static void
parse_args(int argc, char **argv)
{
    int i;
    
    argv0 = argv[0];
    
    files = (char **)g_malloc0(sizeof(char *) * argc);
    
    for (i = 1 ; i < argc ; i++)
    {
    	if (argv[i][0] == '-')
	{
	    if (!strcmp(argv[i], "--help"))
	    {
	    	usagef(0, 0);
	    }
	    else if (!strcmp(argv[i], "--version"))
	    {
	    	printf("cml-check %s\n", VERSION);
    		exit(0);
	    }
	    else if (!strcmp(argv[i], "--debug"))
	    {
		if (++i == argc)
    		    usagef(1, "Expecting argument for --debug\n");
	    	debug_set(argv[i]);
	    }
	    else if (!strcmp(argv[i], "--arch"))
	    {
		if ((arch = argv[++i]) == 0)
    		    usagef(1, "Expecting argument for --arch\n");
	    }
	    else if (!strncmp(argv[i], "--arch=", 7))
	    {
		if (*(arch = argv[i]+7) == '\0')
    		    usagef(1, "Expecting argument for --arch\n");
	    }
	    else if (!strcmp(argv[i], "--xref"))
	    {
		if ((xref_filename = argv[++i]) == 0)
    		    usagef(1, "Expecting argument for --xref\n");
	    }
	    else if (!strncmp(argv[i], "--xref=", 7))
	    {
		if (*(xref_filename = argv[i]+7) == '\0')
    		    usagef(1, "Expecting argument for --xref\n");
	    }
	    else if (!strncmp(argv[i], "-W", 2))
	    {
	    	parse_warning_opt(argv[i]+2);
	    }
	    else if (!strcmp(argv[i], "--warning"))
	    {
	    	parse_warning_opt(argv[++i]);
	    }
	    else if (!strncmp(argv[i], "--warning=", 10))
	    {
	    	parse_warning_opt(argv[i]+10);
	    }
	    else
	    	usagef(1, "Unknown option \"%s\"", argv[i]);
	}
	else
	{
	    files[nfiles++] = argv[i];
	}
    }
    if (nfiles == 0)
    	usagef(1, "expecting at least one rulebase filename\n");
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
main(int argc, char **argv)
{
    cml_rulebase *rb;
    int i;
    int ret = 0;
    
    parse_args(argc, argv);
    
    rb = cml_rulebase_new();
    if (nfiles > 1)
	cml_rulebase_set_merge_mode(rb);
    cml_rulebase_set_arch(rb, arch);
    if (xref_filename != 0)
    	cml_rulebase_set_xref_filename(rb, xref_filename);
    for (i = 0 ; i < num_warnings ; i++)
    	if (warnings[i])
	    cml_rulebase_set_warning(rb, i, (warnings[i] > 0));

    pre_parse();

    for (i = 0 ; i < nfiles ; i++)
    {
	if (!cml_rulebase_parse(rb, files[i]))
	{
    	    fprintf(stderr, "%s: failed to load rulebase \"%s\"\n",
	    		argv0, files[i]);
	    ret = 2;
	}
    }
    
    if (nfiles > 1 && !cml_rulebase_post_parse(rb))
    	ret = 2;
    
    post_parse();
    
    return ret;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/
