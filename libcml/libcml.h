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
 
#ifndef _libcml_h_
#define _libcml_h_ 1

#include <glib.h>
#include <memory.h>

typedef struct cml_location_s	    cml_location;
typedef struct cml_subrange_s	    cml_subrange;
typedef struct cml_atom_s   	    cml_atom;
typedef struct cml_expr_s   	    cml_expr;
typedef struct cml_node_s	    cml_node;
typedef struct cml_rule_s   	    cml_rule;
typedef struct cml_rulebase_s	    cml_rulebase;
typedef struct cml_binding_s   	    cml_binding;
typedef struct cml_transaction_s    cml_transaction;
typedef struct cml_enumdef_s	    cml_enumdef;
typedef struct cml_blob_s   	    cml_blob;
typedef struct cml_test_script_s    cml_test_script;

typedef enum
{
    CML_INFO,
    CML_WARNING,
    CML_ERROR,
    
    _CML_MAX_SEVERITY
} cml_severity;
typedef void (*cml_error_func)(
    cml_severity sev, const cml_location *loc, const char *fmt, va_list args);

typedef GList	    	  	    cml_range;	/* list of cml_subrange structs */

typedef enum 
{
    CML_N=0,
    CML_M=2,
    CML_Y=1
} cml_tritval;

typedef enum
{
    A_NONE,
    A_HEXADECIMAL,
    A_DECIMAL,
    A_STRING,
    A_NODE,    	    /* for CHOICES */
    A_BOOLEAN,
    A_TRISTATE
} cml_atom_type;
#define CML_MAX_ATOM_TYPE	(A_TRISTATE)

typedef enum
{
    CML_CONTINUE,   /* keep going with child nodes */
    CML_SKIP,	    /* skip children, continue with siblings */
    CML_RETURN	    /* skip all nodes, return immediately */
} cml_visit_result;

struct cml_atom_s
{
    cml_atom_type type;
    union
    {
    	long integer;	    	    	    /* A_DECIMAL, A_HEXADECIMAL */
	char *string;	    	    	    /* A_STRING */
	cml_node *node;     	    	    /* A_NODE */
	cml_tritval tritval;	    	    /* A_BOOLEAN, A_TRISTATE */
    } value;
};
/*
 * Set an atom's type to A_NONE and set all values to default 0
 */
#define cml_atom_init(a)	memset((a), 0, sizeof(cml_atom))

struct cml_blob_s
{
    unsigned long length;
    unsigned char *data;
};


struct cml_location_s
{
    const char *filename;
    int lineno;
};

struct cml_subrange_s
{
    unsigned long begin, end;
};

struct cml_enumdef_s
{
    cml_node *symbol;
    unsigned long value;
};



typedef enum 
{
    MN_UNKNOWN, 	    /* default, also for forward declarations */
    MN_SYMBOL,  	    /* leaf nodes are symbols */
    MN_DERIVED, 	    /* symbols derived from others; not declared */
    MN_MENU,	 	    /* body nodes are menus or choices */
    MN_EXPLANATION 	    /* banner used for enums and rule failures */
#define MN_MAX	MN_EXPLANATION
} cml_node_treetype;

/* atom.c */
char *cml_atom_value_as_string(const cml_atom *ap);
const char *cml_atom_type_as_string(const cml_atom *ap);
gboolean cml_atom_from_string(cml_atom *ap, const char *str);

/* node.c */
cml_node_treetype cml_node_get_treetype(const cml_node *);
cml_atom_type cml_node_get_value_type(const cml_node *);
gboolean cml_node_is_radio(const cml_node *);
gboolean cml_node_is_visible(const cml_node *);
GList *cml_node_get_children(const cml_node *);
const cml_atom *cml_node_get_value(cml_node *);
char *cml_node_get_value_as_string(cml_node *mn);
void cml_node_set_value(cml_node *mn, const cml_atom *ap);
gboolean cml_node_is_frozen(const cml_node *mn);
const char *cml_node_get_name(const cml_node *);
const char *cml_node_get_banner(const cml_node *);
void *cml_node_get_user_data(const cml_node *);
void cml_node_set_user_data(cml_node *, void *);
cml_node *cml_node_get_parent(const cml_node *);
const cml_range *cml_node_get_range(const cml_node *);
unsigned long cml_node_get_range_count(const cml_node *);
#define CML_NODE_MAX_STATES 	64
int cml_node_get_states(const cml_node *, int n, char **);
const char *cml_node_get_help_text(const cml_node *);
const GList *cml_node_get_enumdefs(const cml_node *mn);

/* rulebase.c */
cml_node *cml_rulebase_find_node(cml_rulebase *, const char *name);
const char *cml_rulebase_get_banner(const cml_rulebase *rb);
const cml_blob *cml_rulebase_get_icon(cml_rulebase*);
cml_node *cml_rulebase_get_start(const cml_rulebase *rb);
#if TESTSCRIPT
gboolean cml_rulebase_run_test(cml_rulebase *rb);
#endif

typedef cml_visit_result (*cml_rulebase_node_visitor_func)(
    	cml_rulebase *rb, cml_node *mn, int depth, void *user_data);

char *cml_rule_get_explanation(const cml_rule *rule);

cml_rulebase *cml_rulebase_new(void);
void cml_rulebase_set_arch(cml_rulebase *rb, const char *arch);
gboolean cml_rulebase_parse(cml_rulebase *, const char *filename);
/* these two only for the global rulebase checker */
void cml_rulebase_set_merge_mode(cml_rulebase *rb);
void cml_rulebase_set_xref_filename(cml_rulebase *rb, const char *filename);
gboolean cml_rulebase_post_parse(cml_rulebase *rb);
void cml_rulebase_delete(cml_rulebase *rb);
gboolean cml_rulebase_load_defconfig(cml_rulebase *, const char *filename);
void cml_rulebase_menu_apply(cml_rulebase *,
    	cml_rulebase_node_visitor_func visitor,
	void *user_data);
void cml_rulebase_derived_apply(cml_rulebase *,
    	cml_rulebase_node_visitor_func visitor,
	void *user_data);
gboolean cml_rulebase_save_defconfig(cml_rulebase *, const char *filename);
/* rulebase transactions */
gboolean cml_rulebase_commit(cml_rulebase *, gboolean freeze);
void cml_rulebase_abort(cml_rulebase *);
void cml_rulebase_undo(cml_rulebase *);
void cml_rulebase_redo(cml_rulebase *);
void cml_rulebase_clear(cml_rulebase *rb);
gboolean cml_rulebase_can_undo(const cml_rulebase *rb);
gboolean cml_rulebase_can_redo(const cml_rulebase *rb);
gboolean cml_rulebase_can_freeze(const cml_rulebase *rb);
GList *cml_rulebase_get_broken_rules(const cml_rulebase *rb);
void cml_rulebase_check_all_rules(cml_rulebase *rb);
void cml_rulebase_set_warning(cml_rulebase *rb, int id, gboolean enabled);


/* message.c */
void cml_set_error_func(cml_error_func fn);
/*
 * For controlling configurable warnings whose detection and
 * emission can be controlled at runtime.  This is explicitly
 * *not* an i18n interface.
 */
int cml_warning_get_num(void);
int cml_warning_id_by_name(const char *name);
const char *cml_warning_name_by_id(int id);



#endif /* _libcml_h_ */
