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
#include "private.h"
#include <unistd.h>
#include <errno.h>

CVSID("$Id: message.c,v 1.7 2002/09/01 08:36:48 gnb Exp $");

#if TESTSCRIPT
static GList *error_log;
#endif

/*============================================================*/

static const char *severity_strings[_CML_MAX_SEVERITY] =
{
"info", "warning", "error"
};

int cml_message_count[_CML_MAX_SEVERITY];

static void
default_error_func(
    cml_severity sev,
    const cml_location *loc,
    const char *fmt,
    va_list args)
{
    fprintf(stderr, "%s:", severity_strings[sev]);
    if (loc != 0)
    {
    	if (loc->filename != 0 && *loc->filename != '\0')
	    fprintf(stderr, "%s:", loc->filename);
	if (loc->lineno > 0)
	    fprintf(stderr, "%d:", loc->lineno);
    }
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

static cml_error_func error_func = default_error_func;

void 
cml_set_error_func(cml_error_func fn)
{
    error_func = (fn == 0 ? default_error_func : fn);
}

static void
call_error_func(
    cml_severity sev,
    const cml_location *loc,
    const char *fmt,
    va_list args)
{
    (*error_func)(sev, loc, fmt, args);
    cml_message_count[sev]++;
#if TESTSCRIPT
    if (sev == CML_ERROR)
    	error_log = g_list_append(error_log, g_strdup_vprintf(fmt, args));
#endif
}

void
cml_infol(const cml_location *loc, const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    call_error_func(CML_INFO, loc, fmt, args);
    va_end(args);
}

void
cml_warningl(const cml_location *loc, const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    call_error_func(CML_WARNING, loc, fmt, args);
    va_end(args);
}

void
cml_errorl(const cml_location *loc, const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    call_error_func(CML_ERROR, loc, fmt, args);
    va_end(args);
}

void
cml_perror(const char *str)
{
    cml_location loc;

    loc.filename = str;
    loc.lineno = 0;
    cml_errorl(&loc, "%s", strerror(errno));
}

void
cml_messagel(
    cml_severity sev,
    const cml_location *loc,
    const char *fmt, ...)

{
    va_list args;
    
    va_start(args, fmt);
    call_error_func(sev, loc, fmt, args);
    va_end(args);
}

void
cml_messagelv(
    cml_severity sev,
    const cml_location *loc,
    const char *fmt,
    va_list args)
{
    call_error_func(sev, loc, fmt, args);
}

#if TESTSCRIPT
gboolean
cml_error_log_find(const char *str)
{
    GList *iter;
    
    for (iter = error_log ; iter != 0 ; iter = iter->next)
    {
    	const char *err = (const char *)iter->data;
	
	if (strstr(err, str) != 0)
	    return TRUE;
    }
    return FALSE;
}
#endif


/*============================================================*/

static const char *warning_names[CW_NUM_WARNINGS] =
{
/* warnings related to the (EXPERIMENTAL) tag in banners */
"missing-experimental-tag",
"spurious-experimental-tag",
"variant-experimental-tag",
"inconsistent-experimental-tag",

/* warnings related to the (OBSOLETE) tag in banners */
"missing-obsolete-tag",
"spurious-obsolete-tag",
"variant-obsolete-tag",
"inconsistent-obsolete-tag",

/* warnings related to statement syntax */
"spurious-dependencies",
"default-not-in-choices",
"empty-choices",
"nonliteral-define",
"unset-statement",

/* warnings related to the definitions of symbols */
"different-banner",
"different-parent",
"overlapping-definitions",
"overlapping-mixed-definitions",
"primitive-in-root",
"undeclared-symbol",

/* warnings related to expressions & symbol uses */
"forward-compared-to-n",
"symbol-arch",
"forward-reference",
"forward-dependency",
"undeclared-dependency",
"symbol-like-literal",
"constant-symbol-misuse",
"constant-symbol-dependency",
"condition-loop",
"dependency-loop"
};

int
cml_warning_get_num(void)
{
    return CW_NUM_WARNINGS;
}

int
cml_warning_id_by_name(const char *name)
{
    int id;
    
    for (id = 0 ; id < CW_NUM_WARNINGS ; id++)
    {
    	if (!strcmp(name, warning_names[id]))
	    return id;
    }
    return -1;
}

const char *
cml_warning_name_by_id(int id)
{
    if (id < 0 || id >= CW_NUM_WARNINGS)
    	return 0;
    return warning_names[id];
}

/*============================================================*/
/*END*/
