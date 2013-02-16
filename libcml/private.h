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
 
#ifndef _libcml_private_h_
#define _libcml_private_h_ 1

#include "libcml.h"
#include "common.h"


void atom_ctor(cml_atom *a);
void atom_dtor(cml_atom *a);
int _atom_compare(const cml_atom *left, const cml_atom *right);
cml_atom *atom_copy(const cml_atom *a);
void atom_free(cml_atom *a);
void atom_assign(cml_atom *to, const cml_atom *from);
const char *atom_type_as_string(cml_atom_type);

typedef enum
{
    E_NONE,

    /* arithmetic operators */
    E_PLUS,
    E_MINUS,
    E_TIMES,

    /* logical operators */
    E_OR,
    E_AND,
    E_IMPLIES,

    /* relational operators */
    E_EQUALS,
    E_NOT_EQUALS,
    E_LESS,
    E_LESS_EQUALS,
    E_GREATER,
    E_GREATER_EQUALS,
    E_MDEP, 	    /* special hacky operator to handle dep_mbool */
    E_NOT,

    /* ternary operators */
    E_MAXIMUM,
    E_MINIMUM,
    E_SIMILARITY,
    
    /* trinary ?: operator */
    E_TRINARY,

    /* atom */
    E_ATOM, 	    	/* constant */
    E_SYMBOL     	/* reference to MN_SYMBOL menu node */
} cml_expr_type;

#define EXPR_MAX_CHILDREN   3
struct cml_expr_s
{
    cml_expr_type type;
    cml_expr *children[EXPR_MAX_CHILDREN];  	    /* 3 allows for trinary expression */
    cml_atom value; 	    	    /* for E_ATOM, also later for lsee */
    cml_node *symbol;  	    /* for E_SYMBOL */
};

cml_expr *expr_new();
cml_expr *expr_copy(const cml_expr *);
cml_expr *expr_deep_copy(const cml_expr *);
cml_expr *expr_new_composite(cml_expr_type, cml_expr *left, cml_expr *right);
cml_expr *expr_new_trinary(cml_expr *cond, cml_expr *first, cml_expr *second);
cml_expr *expr_new_atom(const cml_atom *);
cml_expr *expr_new_atom_v(cml_atom_type type, ...);
cml_expr *expr_new_symbol(cml_node *);
cml_expr *expr_new_atom_from_string(cml_atom_type type, const char *s);
void expr_destroy(cml_expr *expr);  /* recursive */
void expr_set_left_child(cml_expr *e, cml_expr *child);
void expr_set_right_child(cml_expr *e, cml_expr *child);
void _expr_dump(const cml_expr *, FILE *, int depth);
gboolean expr_is_constant(cml_expr *expr);
cml_atom_type expr_get_value_type(const cml_expr *expr);
const char *_expr_get_type_as_string(const cml_expr *expr);
int expr_solve(const cml_expr *expr, const cml_atom *target, cml_node *source);
void expr_merge_boolean_or(cml_expr **ep, const cml_expr *newe, gboolean first);
void expr_merge_boolean_and(cml_expr **ep, const cml_expr *newe, gboolean first);
gboolean expr_contains_symbol(const cml_expr *expr, const cml_node *mn);

enum cml_transaction_flags
{
TX_UNDONE   	=(1<<0),	/* undo called */
TX_FROZEN   	=(1<<1),	/* all nodes bound by this txn are frozen */
TX_NEW   	=(1<<2)	    	/* not yet committed */
};

struct cml_transaction_s
{
    unsigned long uniqueid; /* for debugging only */
    cml_node *guard;
    GHashTable *bindings;	    /* key=cml_node, value=cml_binding */
    int flags;
    int undo_id;
};

struct cml_binding_s
{
    cml_node *node;
    cml_transaction *transaction;
    cml_atom value;
};

cml_enumdef *cml_enumdef_new(cml_node *symbol, unsigned long value);


struct cml_node_s
{
    char *name;
    char *banner;
    unsigned long uniqueid;
    cml_rulebase *rulebase;
    int visited;
    cml_location location;
    cml_node_treetype treetype;
    int expr_count; 	    	    	/* number of times used in expressions */
    
    unsigned int flags;
#define MN_FLAG_WARNDEPEND    	0x4 	/* warn user when nodes depend on this symbol */
#define MN_FLAG_IS_RADIO    	0x8 	/* an MN_MENU represents a radio def */
#define MN_FORWARD_WARNING    	0x20 	/* forward dec warning given */
#define MN_VITAL    	    	0x40 	/* symbol declared `vital' */
#define MN_SUBTREE_SEEN     	0x80 	/* used during { subtree } parsing */
#define MN_LOADED     	    	0x100 	/* value loaded from .config */
#define MN_EXPERIMENTAL     	0x200 	/* banner has (EXPERIMENTAL) tag */
#define MN_OBSOLETE     	0x400 	/* banner has (OBSOLETE) tag */
#define MN_CONSTANT     	0x800 	/* value never changes e.g. $ARCH */
#define MN_WEAK_POSITION     	0x1000 	/* tree location may be overriden later */
    /* TODO: enum status??? */
    GList *rules_using;    	    /* list of cml_rule */
    cml_expr *visibility_expr;	    /* merged visibility expression */
    cml_expr *saveability_expr;	    /* merged saveability expression */
    cml_node *parent;
    GList *children;	    	    /* even symbols can have children */
    
    /*
     * Used as:
     * MN_MENU if is_radio() default choice
     * MN_SYMBOL default expression
     * MN_DERIVED derivation expression
     */
    cml_expr *expr;
    cml_atom value; 	    	    /* for MN_DERIVED */
     
    GList *transactions_guarded;    /* txns which this node guards */
    GList *bindings;	    	    /* in txn order, i.e. most recent 1st */
    cml_atom_type value_type;	    /* type allowed in binding */
    	    	    	    	    /* MN_MENUs which is_radio have value */

    /* symbols */
    cml_range *range;
    GList *enumdefs;
    GList *dependants;	    	    /* symbols which depend on me */
    GList *dependees;	    	    /* symbols I depend on */
    
    char *help_text;
    
    void *user_data;
    GList *forward_refs;    	    /* list of forward reference cml_locations */
    GList *forward_deps;    	    /* list of forward dependence cml_locations */
};


struct cml_rule_s
{
    unsigned long uniqueid;
    cml_location location;
    cml_expr *expr;
    cml_node *explanation;
    cml_rulebase *rulebase;
};

gboolean rule_trigger(cml_rulebase *rb, cml_rule *rule, cml_node *source);

typedef enum
{
    TS_SET,
    TS_ASSERT,
    TS_SUCCEEDED,
    TS_CLEAR,
    TS_COMMIT,
    TS_FREEZE,
    TS_VISIBLE,
    TS_SAVEABLE,
    TS_ERROR,
    TS_NOERROR
} cml_test_script_op;

struct cml_test_script_s
{
    cml_test_script_op op;
    cml_location location;
    cml_node *symbol;
    cml_expr *expr;
};

typedef enum
{
    RBF_TRITS,
    RBF_NOHELP,
    
    RBF_NUM
} cml_rulebase_features;

typedef enum
{
    /* warnings related to the (EXPERIMENTAL) tag in banners */
    CW_MISSING_EXPERIMENTAL_TAG,
    CW_SPURIOUS_EXPERIMENTAL_TAG,
    CW_VARIANT_EXPERIMENTAL_TAG,
    CW_INCONSISTENT_EXPERIMENTAL_TAG,

    /* warnings related to the (OBSOLETE) tag in banners */
    CW_MISSING_OBSOLETE_TAG,
    CW_SPURIOUS_OBSOLETE_TAG,
    CW_VARIANT_OBSOLETE_TAG,
    CW_INCONSISTENT_OBSOLETE_TAG,

    /* warnings related to statement syntax */
    CW_CRUD_AFTER_LOGICAL_QUERY,
    CW_DEFAULT_NOT_IN_CHOICES,
    CW_EMPTY_CHOICES,
    CW_NONLITERAL_DEFINE,
    CW_UNSET_STATEMENT,

    /* warnings related to the definitions of symbols */
    CW_DIFFERENT_BANNER,
    CW_DIFFERENT_PARENT,
    CW_OVERLAPPING_DEFINITIONS,
    CW_OVERLAPPING_MIXED_DEFINITIONS,
    CW_PRIMITIVE_IN_ROOT,
    CW_UNDECLARED_SYMBOL,

    /* warnings related to expressions & variable uses */
    CW_FORWARD_COMPARED_TO_N,
    CW_SYMBOL_ARCH,
    CW_FORWARD_REFERENCE,
    CW_FORWARD_DEPENDENCY,
    CW_UNDECLARED_DEPENDENCY,
    CW_SYMBOL_LIKE_LITERAL,
    CW_CONSTANT_SYMBOL_MISUSE,
    CW_CONSTANT_SYMBOL_DEPENDENCY,
    CW_CONDITION_LOOP,
    CW_DEPENDENCY_LOOP,
    
    CW_NUM_WARNINGS
} cml_warning_t;


struct cml_rulebase_s
{
    struct
    {
	cml_atom value;
    	cml_node *tie;
	cml_location location;	/* location of "condition" statement */
    } features[RBF_NUM];
    gboolean cml1_default_vals; /* default value of nodes is {A_NONE,0} */
    gboolean merge_mode;    	/* merge multiple CML1 files */
    FILE *xref_fp;
    char *prefix;
    cml_node *banner;  	/* use the (l10n'ed) banner text for this node as global banner */
    cml_blob *icon;
    GList *rules;  	    	/* list of cml_rule* */
    cml_node *start;   	    	/* root of menu tree */
    cml_location start_loc; 	/* file location of "start" statement */
    GHashTable *menu_nodes;	/* hashtable of cml_node's */
    GList *filenames;	    	/* singular storage for filenames */
    GList *transactions;    	/* all transactions, most recent first */
    int last_visited;	    	/* used in topological sort of menu nodes */
    int last_undo_id;	    	/* largest undo_id of transactions */
    int curr_undo_id;	    	/* txns more recent than this are undone */
    GHashTable *broken_rules;	/* rules broken in this txn */
    GHashTable *chilled;    	/* chilled symbols key=cml_node value=cml_node */
    int num_failed_sets;    	/* number of failed cml_node_set_value() calls */
#if TESTSCRIPT
    GList *test_script;     	/* list of cml_test_script */
    gboolean parsetest;     	/* run test script after parse, even if failed */
#endif
    unsigned long warnings; 	/* bitfield of enabled warnings */
};

#define rb_is_ignored(rb, rt)	    	((rb)->ignore_rules & (1<<(rt)))

#define rb_warning_enabled(rb, w)   	((rb)->warnings & (1UL<<(w)))
#define _rb_enable_warning(rb, w)    	(rb)->warnings |= (1UL<<(w))
#define _rb_disable_warning(rb, w)    	(rb)->warnings &= ~(1UL<<(w))

/* range.c */
cml_range *range_new(unsigned long begin, unsigned long end);
void range_delete(cml_range *range);
cml_range *range_add(cml_range *, unsigned long begin, unsigned long end);
unsigned long range_count(const cml_range *range);
gboolean range_check(const cml_range *range, unsigned long x);
void range_dump(const cml_range *range, FILE *);

/* blob.c */
cml_blob *blob_new(unsigned char *data, unsigned long length);
cml_blob *blob_new_copy(unsigned char *data, unsigned long length);
void blob_delete(cml_blob *);

/* node.c */
cml_node *mn_new(const char *name);
void mn_delete(cml_node *mn);
void mn_set_children(cml_node *parent, GList *children);
void mn_add_child(cml_node *node, cml_node *child);
void mn_reparent(cml_node *newparent, cml_node *child);
const char *mn_get_treetype_as_string(const cml_node *mn);
const char *mn_treetype_as_string(cml_node_treetype);
gboolean mn_set_value(cml_node *mn, const cml_atom *ap, cml_node *source);
void _mn_add_using_rule(cml_node *mn, cml_rule *rule);
void mn_add_dependant(cml_node *dependee, cml_node *dependant);
void mn_add_visibility_expr(cml_node *mn, cml_expr *expr);
void mn_add_visibility_expr2(cml_node *mn, cml_expr *expr,  cml_expr_type join);
void mn_add_saveability_expr(cml_node *mn, cml_expr *expr);
void mn_chill(cml_node *mn);
gboolean mn_is_chilled(const cml_node *mn);
gboolean mn_is_saveable(const cml_node *mn);

/* expr.c */
void _expr_add_using_rule_recursive(cml_expr *expr, cml_rule *rule);
void expr_evaluate(const cml_expr *expr, cml_atom *val);
void _expr_add_dependant(const cml_expr *expr, cml_node *dependant);
cml_expr *expr_simplify(cml_expr *expr);
char *expr_as_string(const cml_expr *expr);

/* rule.c */
cml_rule *rule_new_require(cml_expr *);
cml_rule *rule_new_prohibit(cml_expr *);
void rule_delete(cml_rule *);
#if DEBUG
void rule_dump(const cml_rule *rule, FILE *fp);
#endif

/* rulebase.c */
#if DEBUG
void rb_dump_nodes(const cml_rulebase *rb, FILE *fp);
void rb_dump_rules(const cml_rulebase *rb, FILE *fp);
#endif
gboolean rb_trigger_rules(cml_rulebase *rb, GList *rules, cml_node *source);
void rb_add_rule(cml_rulebase *rb, cml_rule *rule);
cml_node *rb_add_node(cml_rulebase *, const char *name);
void rb_remove_node(cml_rulebase *rb, cml_node *mn);
void rb_unchill_all(cml_rulebase *rb);
#if TESTSCRIPT
void rb_add_test(cml_rulebase *rb, cml_location loc,
    	    	cml_test_script_op op, cml_node *symbol, cml_expr *expr);
#endif
void rb_add_xref(cml_rulebase *rb, const cml_node *mn, const char *usage,
    	    	 const cml_location*);

/* cml1_parser.y */
gboolean _cml_rulebase_parse_cml1(cml_rulebase *, const char *filename);

/* cml2_parser.y */
gboolean _cml_rulebase_parse_cml2(cml_rulebase *, const char *filename);

/* message.c */
#ifdef __GNUC__
#define PRINTF(n,m) 	__attribute__ (( format(printf,n,m) ))
#else
#define PRINTF(n,m)
#endif

void cml_infol(const cml_location *loc, const char *fmt, ...) PRINTF(2,3);
void cml_warningl(const cml_location *loc, const char *fmt, ...) PRINTF(2,3);
void cml_errorl(const cml_location *loc, const char *fmt, ...) PRINTF(2,3);
void cml_perror(const char *str);
void cml_messagel(cml_severity sev, const cml_location *loc, const char *fmt,
    	    	  ...) PRINTF(3,4);
void cml_messagelv(cml_severity sev, const cml_location *loc, const char *fmt,
		    va_list args) PRINTF(3,0);
extern int cml_message_count[_CML_MAX_SEVERITY];
#if TESTSCRIPT
gboolean cml_error_log_find(const char *str);
#endif

#undef PRINTF

/* transactions.c */
const cml_binding *_cml_tx_get(cml_rulebase *rb, const cml_node *mn);
void _cml_tx_set(cml_rulebase *rb, cml_node *mn, const cml_atom *a,
		    cml_node *source);
const cml_atom *_cml_tx_check(cml_rulebase *rb, const cml_node *mn,
    	    	    gboolean checknew);
void _cml_tx_commit(cml_rulebase *rb, gboolean freeze);
void _cml_tx_abort(cml_rulebase *rb);
void _cml_tx_dump(cml_rulebase *rb, FILE *fp);
void _cml_tx_undo(cml_rulebase *rb);
void _cml_tx_redo(cml_rulebase *rb);
void _cml_tx_clear(cml_rulebase *rb);
/*
 * Returns a new list, which needs to be freed, of nodes
 * whose value might be affected by adding a binding for
 * the given node, in indeterminate order.  Always includes
 * `node' itself.
 */
GList *_cml_tx_get_triggerable_nodes(cml_rulebase *rb, cml_node *node,
    	    	    	    	     cml_node *source);

#endif /* _libcml_private_h_ */
