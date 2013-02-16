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
 
#include "private.h"
#include "util.h"
#include "debug.h"
#include <ctype.h>
#include <errno.h>

CVSID("$Id: save.c,v 1.14 2002/09/01 08:21:08 gnb Exp $");

#define safestr(s)  ((s) == 0 ? "" : (s))

/*============================================================*/

static cml_visit_result
save_defconfig_visitor(
    cml_rulebase *rb,
    cml_node *mn,
    int depth,
    void *user_data)
{
    FILE *fp = (FILE *)user_data;
    const cml_atom *ap;

    if (mn->treetype == MN_MENU && !cml_node_is_visible(mn))
    	return CML_SKIP;

    if (!mn_is_saveable(mn))
    	return CML_CONTINUE;
	
    if (mn->treetype == MN_MENU &&
    	!cml_node_is_radio(mn) &&
	mn->children != 0 &&
	mn->banner != 0)
    {
    	fprintf(fp, "\n#\n# %s\n#\n", mn->banner);
    	return CML_CONTINUE;
    }

    if (mn->treetype != MN_DERIVED && mn->treetype != MN_SYMBOL)
    	return CML_CONTINUE;

    if ((ap = cml_node_get_value(mn)) == 0 || ap->type == A_NONE)
    	return CML_CONTINUE;
	
    switch (cml_node_get_value_type(mn))
    {
    case A_HEXADECIMAL:
    	fprintf(fp, "%s%s=0x%lX\n",
	    safestr(rb->prefix),
	    mn->name,
	    ap->value.integer);
	break;
    case A_DECIMAL:
    	fprintf(fp, "%s%s=%ld\n",
	    safestr(rb->prefix),
	    mn->name,
	    ap->value.integer);
    	break;
    case A_STRING:
    	/* TODO: escapes */
    	fprintf(fp, "%s%s=\"%s\"\n",
	    safestr(rb->prefix),
	    mn->name,
	    safestr(ap->value.string));
    	break;
    case A_BOOLEAN:
    case A_TRISTATE:
    	switch (ap->value.tritval)
	{
	case CML_Y:
	    fprintf(fp, "%s%s=y\n",
		safestr(rb->prefix),
		mn->name);
	    break;
	case CML_M:
	    fprintf(fp, "%s%s=m\n",
		safestr(rb->prefix),
		mn->name);
	    break;
	case CML_N:
	    fprintf(fp, "# %s%s is not set\n",
		safestr(rb->prefix),
		mn->name);
	    break;
	}
	break;
    default:
    	break;
    }
    return CML_CONTINUE;
}

static const char banner[] = 
"#\n"
"# Automatically created by gcml2 " VERSION ": don't edit\n"
"#\n";


gboolean
cml_rulebase_save_defconfig(cml_rulebase *rb, const char *filename)
{
    /* TODO: write a tmp file, diff em, and replace only if different */
    /* TODO: escape strings properly */
    FILE *fp;
    char *bakfile;
    
    /* First, backup the file */
    bakfile = g_strconcat(filename, ".old", 0);
    if (rename(filename, bakfile) < 0 && errno != ENOENT)
    {
    	cml_perror(bakfile);
    	g_free(bakfile);
	return FALSE;
    }
    g_free(bakfile);
    
    if ((fp = fopen(filename, "w")) == 0)
    {
    	cml_perror(filename);
    	return FALSE;
    }
    DDPRINTF1(DEBUG_SAVE, "    Writing %s\n", filename);
    fputs(banner, fp);
    cml_rulebase_menu_apply(rb, save_defconfig_visitor, (void*)fp);
    fprintf(fp, "\n# Derived symbols\n");
    cml_rulebase_derived_apply(rb, save_defconfig_visitor, (void*)fp);
    fclose(fp);

    return TRUE;
}

/*============================================================*/
/*END*/
