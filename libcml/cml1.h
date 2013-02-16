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
 
#ifndef _cml1_h_
#define _cml1_h_ 1

#ifndef OVERLAP_DNF
#define OVERLAP_DNF 1
#endif


#include "private.h"
#if OVERLAP_DNF
#include "dnf.h"
#endif

typedef struct cml_branch_s 	cml_branch_t;

typedef enum
{
    N_NONE,

    /* composite entities */
    N_MENU,
    N_CHOICE,
    
    /* menu entries */
    N_COMMENT,
    N_QUERY,
    N_DEP_BOOL,
    N_DEP_MBOOL,
    N_DEP_TRISTATE,

    N_DEFINE,
    
    N_NUMTYPES
} cml_branch_type_t;

/*
 * During CML1 parsing, a list of these are kept on cml_node->user_data.
 */
struct cml_branch_s
{
    cml_location location;
    cml_branch_type_t type;
    cml_atom_type value_type;	/* for N_QUERY, N_DEFINE */
    cml_expr *cond;
    GList *exprs;   	    	/* list of cml_expr */
    char *choice_default;
    cml_node *node; 	    	/* backpointer to menu node */
    cml_node *parent; 	    	/* menu in which branch is defined */

    unsigned int flags;
    /*
     * The convention for magic tag flags is (1<<N) for the tag and
     * (1<<(N+1)) for the variant versions of the tag.  This is why
     * there are gaps in the flags: don't fill them!!
     */
#define BR_VARIANT(x)     	((x)<<1)

#define BR_EXPERIMENTAL     	(1<<0) 	/* labelled (EXPERIMENTAL) in banner */
    	    	    	    /* BR_VARIANT(BR_EXPERIMENTAL) */
#define BR_OBSOLETE     	(1<<2) 	/* labelled (OBSOLETE) in banner */
    	    	    	    /* BR_VARIANT(BR_OBSOLETE) */
#define BR_HAS_BANNER	    	(1<<8)	/* a banner was defined for this branch */
#define BR_WAS_BANNER	    	(1<<9)	/* this branch's banner became the node's */


#if OVERLAP_DNF
    /* ***HACK*** used in branches_overlap */
    dnf_t *cond_dnf;	    	
#else
    char *cond_str;
#endif
};

void branch_delete(cml_branch_t *branch);

#endif /* _cml1_h_ */
