
%{

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
 *
 * 
 *
 * 
 * The first pass through the CML1 nodes builds a basic menu
 * structure of cml_node's in the rulebase.  Each body node
 * has treetype MN_MENU (or is_radio) or MN_UNKNOWN, no visibility
 * expression, no default/derive expression, no value_type and
 * has no rules associated with it.  They are all placeholders
 * for the menu structure.  Each cml_node->user_data is a GList*
 * of the cml1node_t's whose name matched it.  Each cml1node_t
 * has their own copy of a merged cond expression.
 *
 * In the second pass we go down the cml_node tree and try to
 * resolve the various different attempts at defining that node
 * in the CML1 corpus.  For each node, the result is a treetype,
 * (MN_UNKNOWN -> MN_DERIVED or MN_SYMBOL), a value_type, a
 * visibility expression, a default/derive expression, and some
 * number of rules.
 *
 * The second pass simplifies the logic somewhat, and makes it
 * possible to handle certain cases like:
 *
 * define_bool CONFIG_FOO y
 * if [ "$CONFIG_BAR" = "y" ]; then
 *     define CONFIG_FOO n
 * fi
 * if [ "$CONFIG_BAZ" = "y" ]; then
 *     bool 'yadda yadda yadda' CONFIG_FOO
 * fi
 *
 * Here the rule that corresponds to the first "define_bool"
 * statement is of the form (EXPR implies (CONFIG_FOO == y))
 * where EXPR is true only none of the other branches are
 * taken.  In general this rule cannot be created until all
 * the possible branches are known.
 */
 
#include "cml1.h"
#include "debug.h"
#include <malloc/malloc.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>

#define YYERROR_VERBOSE 1
static cml_rulebase *rb;
static cml_location yylocation;

static void yyerror(const char *fmt, ...);
static int yylex(void);


typedef struct
{
    const char *tag;	    /* literal tag string in banner */

    /* known variant forms of the tag */
#define MAX_MAGIC_TAG_VARIANTS	3
    const char *variants[MAX_MAGIC_TAG_VARIANTS];

    unsigned int flag;	    /* will appear in BRANCH flags */
    const char *symbol;     /* name of required symbol to depend on */
    cml_warning_t base_warning; /* +0=missing, +1=spurious, +2=variant */
} cml_magic_tag_t;

static const cml_magic_tag_t magic_tags[] =
{
    {"(EXPERIMENTAL)",
	{"(experimental)", "(Experimental)", "(DANGEROUS)"},
	BR_EXPERIMENTAL,
	"CONFIG_EXPERIMENTAL",
	CW_MISSING_EXPERIMENTAL_TAG
    },
    {"(OBSOLETE)",
    	{"(obsolete)", "[Obsolete]"},
	BR_OBSOLETE,
	"CONFIG_OBSOLETE",
	CW_MISSING_OBSOLETE_TAG
    },
    {0}
};


CVSID("$Id: cml1_parser.y,v 1.8 2002/09/01 09:01:21 gnb Exp $");

/*============================================================*/

/* Simple 2-place circular buffer of location records,
 * needed to work around parse sequence.
 */
#if 0
#define STATEMENT_FIFO_MAX  2
static cml_location statement_locations[STATEMENT_FIFO_MAX];
static int statement_in, statement_out;

static void
statement_init(void)
{
    statement_in = 0;
    statement_out = 0;
}

static void
statement_start(void)
{
    statement_locations[statement_in] = yylocation;
    if (++statement_in == STATEMENT_FIFO_MAX)
    	statement_in = 0;
}

static cml_location
statement_get_location(void)
{
    int i = statement_out;
    if (++statement_out == STATEMENT_FIFO_MAX)
    	statement_out = 0;
    return statement_locations[i];
}
#else
static cml_location statement_location;

static void
statement_init(void)
{
}

static void
statement_start(void)
{
    statement_location = yylocation;
}

static cml_location
statement_get_location(void)
{
    return statement_location;
}
#endif

/*============================================================*/

static GList *cond_stack;

static const cml_expr *
cond_top(void)
{
    return (cond_stack == 0 ? 0 : (cml_expr *)cond_stack->data);
}

static cml_expr *
cond_copy_top(void)
{
    return (cond_stack == 0 ? 0 : expr_deep_copy((cml_expr *)cond_stack->data));
}

static void
cond_push_m(cml_expr *expr/*consumed*/)
{
    cml_expr *nexpr;
    
    if (cond_stack != 0)
	nexpr = expr_new_composite(E_AND,
	    	    expr_deep_copy(cond_top()),
		    expr);
    else
    	nexpr = expr;
    
    cond_stack = g_list_prepend(cond_stack, nexpr);
}

static void
cond_else(void)
{
    assert(cond_stack != 0);

    if (cond_stack->next == 0)
    {
	cml_expr *top = (cml_expr *)cond_stack->data;
	cond_stack->data = expr_new_composite(E_NOT, top, 0);
    }
    else
    {
	cml_expr *top = (cml_expr *)cond_stack->data;
	assert(top != 0);
	assert(top->type == E_AND);
	top->children[1] = expr_new_composite(E_NOT, top->children[1], 0);
    }
}

static void
cond_pop(void)
{
    assert(cond_stack != 0);
    expr_destroy((cml_expr *)cond_top());
    cond_stack = g_list_remove_link(cond_stack, cond_stack);
}

/*============================================================*/

static GList *menu_stack;

static void
menu_push(cml_node *mn)
{
    assert(mn != 0);
    menu_stack = g_list_prepend(menu_stack, mn);
}

static void
menu_pop(void)
{
    assert(menu_stack != 0);
    menu_stack = g_list_remove_link(menu_stack, menu_stack);
}

static cml_node *
menu_top(void)
{
    assert(menu_stack != 0);
    return (cml_node *)menu_stack->data;
}

static gboolean
menu_is_root(void)
{
    assert(menu_stack != 0);
    return (menu_stack->next == 0);
}

/*============================================================*/

cml_branch_t *
branch_new(cml_branch_type_t type)
{
    cml_branch_t *branch;
    
    branch = g_new0(cml_branch_t, 1);
    branch->type = type;

    return branch;
}

void
branch_delete(cml_branch_t *branch)
{
    if (branch->cond != 0)
    	expr_destroy(branch->cond);

    while (branch->exprs != 0)
    {
    	expr_destroy((cml_expr *)branch->exprs->data);
	branch->exprs = g_list_remove_link(branch->exprs, branch->exprs);
    }
    strdelete(branch->choice_default);
    
    g_free(branch);
}

void
branch_add_expr(cml_branch_t *branch, cml_expr *expr)
{
    branch->exprs = g_list_append(branch->exprs, expr);
}

/*============================================================*/

static gboolean
parse_hex(const char *str, unsigned int *valp)
{
    char *end = 0;
    unsigned int val;
    
    if (!strncmp(str, "0x", 2) || !strncmp(str, "0X", 2))
    	str += 2;
    val = strtoul(str, &end, 16);
    if (end == 0 || end == str || *end != '\0')
    	return FALSE;
	
    if (valp != 0)
    	*valp = val;
    return TRUE;
}

/*============================================================*/

static gboolean
is_constant_symbol(const char *name)
{
    static GHashTable *syms_hash = 0;
    static const char * const syms[] = {
    "CONFIG_ALPHA",	/* alpha */
    "CONFIG_ARM",	/* arm */
    /*TODO*/	    	/* cris */
    "CONFIG_X86",	/* i386 */
    "CONFIG_IA64",	/* ia64 */
    /*TODO*/	    	/* m68k */
    /*TODO*/	    	/* mips64 */
    "CONFIG_MIPS",	/* mips */
    "CONFIG_PARISC",    /* parisc */
    "CONFIG_PPC64",	/* ppc64 */
    "CONFIG_PPC32",	/* ppc */
    "CONFIG_ARCH_S390", /* s390 */
    "CONFIG_ARCH_S390X",/* s390x */
    "CONFIG_SUPERH",	/* sh */
    "CONFIG_SPARC64",	/* sparc64 */
    "CONFIG_SPARC32",	/* sparc */
    "CONFIG_X86_64",	/* x86_64 */
    0};
    
    if (syms_hash == 0)
    {
    	const char * const *p;
    	syms_hash = g_hash_table_new(g_str_hash, g_str_equal);
	for (p = syms ; *p != 0 ; p++)
	    g_hash_table_insert(syms_hash, (gpointer)*p, (gpointer)*p);
    }
    return (g_hash_table_lookup(syms_hash, name) != 0);
}

/*============================================================*/
/* 
 * Remove leading spaces from the prompt string.  CML1 spuriously
 * encodes display menu depth in the prompt strings themselves;
 * as we can calculate that information more accurately, there's
 * no point keeping it.  While we're at it, remove trailing
 * whitespace too.
 */

static gboolean
replace(char *str, const char *expendable)
{
    char *x;
    int len, elen = strlen(expendable);
    
    if ((x = strstr(str, expendable)) != 0)
    {
	if ((len = strlen(x)) > elen)
	    memmove(x, x+elen, len-elen+1);
	else if (len == elen)
	    *x = '\0';
	return TRUE;
    }
    return FALSE;
}

static char *
normalise_prompt(char *str, unsigned int *flagsp)
{
    char *x;
    const cml_magic_tag_t *mt;
    
#if DEBUG > 5
    fprintf(stderr, "normalise_prompt(\"%s\")", str);
#endif

    for (x = str ; *x && isspace(*x) ; x++)
    	;
    if (*x)
    	memmove(str, x, strlen(x)+1);

    for (mt = magic_tags ; mt->tag != 0 ; mt++)
    {
    	int i;
	
	if (replace(str, mt->tag) && flagsp != 0)
	    *flagsp |= mt->flag;
	for (i = 0 ; i < MAX_MAGIC_TAG_VARIANTS && mt->variants[i] != 0 ; i++)
	    if (replace(str, mt->variants[i]) && flagsp != 0)
		*flagsp |= BR_VARIANT(mt->flag);
    }

    for (x = str + strlen(str) - 1 ; x != str && isspace(*x) ; x--)
    	;
    x[1] = '\0';

#if DEBUG > 5
    fprintf(stderr, " = \"%s\"\n", str);
#endif    
    return str;
}

/*============================================================*/

/*
 * Expand backslash escape sequences.  Actually, the resulting
 * string only ever contracts, so "expand" is a misnomer.
 * Operates in-place and returns its argument.
 */
static char *
expand_escapes(char *str)
{
    char *in, *out;
    
#if DEBUG > 4
    fprintf(stderr, "expand_escapes(\"%s\")", str);
#endif

    for (in = out = str ; *in ; in++)
    {
    	if (*in == '\\')
	{
	    switch (*(++in))
	    {
	    case 't': *out++ = '\t'; break;
	    case 'r': *out++ = '\r'; break;
	    case 'n': *out++ = '\n'; break;
	    case '\r': break;
	    case '\n': break;
	    default:  *out++ = *in; break;
	    }
	}
	else
	    *out++ = *in;
    }
    *out = '\0';

#if DEBUG > 4
    fprintf(stderr, " = \"%s\"\n", str);
#endif    

    return str;
}


/*============================================================*/
/*
 * Note that in CML1 menus have prompts but not names, and 
 * that multiple menus with the same prompt are really the
 * same menu under different conditions.  Hence we are forced
 * to use the banner as a name.  Takes over the `banner'.
 */

static cml_node *
add_compound_branch(char *banner, cml_branch_t *branch)
{
    cml_node *mn;

    if ((mn = cml_rulebase_find_node(rb, banner)) != 0)
    {
    	g_free(banner);
    }
    else
    {
	mn = rb_add_node(rb, banner);
	mn->treetype = MN_MENU;
	if (branch->type == N_CHOICE)
	    mn->flags |= MN_FLAG_IS_RADIO;
	mn_add_child(menu_top(), mn);
	/* TODO: check that normalised banners are the same */
	mn->banner = banner;
    }

    mn->location = branch->location;
    branch->node = mn;
    mn->user_data = g_list_append((GList *)mn->user_data, branch);
    
    DDPRINTF1(DEBUG_CONVERT, "\"%s\"\n", mn->name);

    return mn;
}

/*============================================================*/

static gboolean
is_sym_and_y(const cml_expr *e1, cml_node *mn, const cml_expr *e2)
{
    return (e1->type == E_SYMBOL &&
    	    e1->symbol == mn &&
    	    e2->type == E_ATOM &&
	    e2->value.type == A_TRISTATE &&
	    e2->value.value.tritval == CML_Y);
}

static gboolean
expr_depends_on(const cml_expr *expr, cml_node *mn)
{
    int i;
    
    switch (expr->type)
    {
    case E_EQUALS:
    	return (is_sym_and_y(expr->children[0], mn, expr->children[1]) ||
	    	is_sym_and_y(expr->children[1], mn, expr->children[0]));
    case E_SYMBOL:
    	return (expr->symbol == mn);
    default:
    	/*
	 * This is slightly wonky in that some expressions which logically
	 * result in no actual dependence on the given symbol can be spuriously
	 * detected, for example
	 * if [ "$CONFIG_EXPERIMENTAL" = "y" -o "$CONFIG_EXPERIMENTAL" != "y" ]
	 */
    	for (i = 0 ; i < EXPR_MAX_CHILDREN ; i++)
	    if (expr->children[i] != 0 && expr_depends_on(expr->children[i], mn))
	    	return TRUE;
    	break;
    }
    return FALSE;
}

static gboolean
branch_depends_on(const cml_branch_t *branch, cml_node *mn)
{
    GList *iter;
    
    switch (branch->type)
    {
    case N_DEP_BOOL:
    case N_DEP_MBOOL:
    case N_DEP_TRISTATE:
    	for (iter = branch->exprs ; iter != 0 ; iter = iter->next)
	{
	    cml_expr *expr = (cml_expr *)iter->data;
	    
	    if (expr_depends_on(expr, mn))
	    	return TRUE;
	}
    	break;
    default:
    	break;
    }
    if (branch->cond != 0 && expr_depends_on(branch->cond, mn))
    	return TRUE;
    return FALSE;    
}

/*
 * Check for correctness of the magic (EXPERIMENTAL) and (OBSOLETE)
 * tags in banners.  If we are going to live with this horrendous
 * hack then we may as well get it right.
 */
static void
check_magic_tags(cml_branch_t *branch)
{
    const cml_magic_tag_t *mt;
    cml_node *depmn;
    gboolean needs;
    gboolean says;

    for (mt = magic_tags ; mt->tag != 0 ; mt++)
    {
	if ((depmn = cml_rulebase_find_node(rb, mt->symbol)) == 0)
	{
	    /*
	     * Because of the way forward references need to work,
	     * we know that if the symbol is not present in the
	     * rulebase then no symbols can depend on it.
	     */
	    needs = FALSE;
	}
	else
	    needs = branch_depends_on(branch, depmn);
	    
	if ((branch->flags & BR_VARIANT(mt->flag)) &&
	    rb_warning_enabled(rb, mt->base_warning+2))
    	    cml_warningl(&branch->location,
		"banner for %s contains variant form of %s",
		branch->node->name, mt->tag);

    	says = (branch->flags & (mt->flag|BR_VARIANT(mt->flag)));

	if (says && !needs)
	{
	    if (rb_warning_enabled(rb, mt->base_warning+1))
    		cml_warningl(&branch->location,
		    "banner for %s contains %s but symbol does not depend on %s",
		    branch->node->name, mt->tag, mt->symbol);
	}
	if ((branch->flags & BR_HAS_BANNER) && !says && needs)
	{
	    if (rb_warning_enabled(rb, mt->base_warning))
    		cml_warningl(&branch->location,
		    "symbol %s depends on %s but banner does not contain %s",
		    branch->node->name, mt->symbol, mt->tag);
	}
		
	/* normalise the flags to what the actual dependence relation indicates */
    	branch->flags &= ~(mt->flag|BR_VARIANT(mt->flag));
	if (needs)
	    branch->flags |= mt->flag;
    }
}

/*============================================================*/
/*
 * Find the last location at which the node's banner was defined by a branch.
 */
static const cml_location *
find_banner_location(const cml_node *mn)
{
    GList *iter;
    const cml_location *loc = 0;
    
    for (iter = (GList *)mn->user_data ; iter != 0 ; iter = iter->next)
    {
    	cml_branch_t *branch = (cml_branch_t *)iter->data;
	
	if ((branch->flags & BR_WAS_BANNER))
	    loc = &branch->location;
    }
    return loc;
}

/* Takes over `name' and `banner' */
static cml_node *
add_primitive_branch(char *name, char *banner, cml_branch_t *branch)
{
    cml_node *mn;

    branch->parent = menu_top();

    if ((mn = cml_rulebase_find_node(rb, name)) != 0)
    {
	g_free(name);
    	if (mn->treetype != MN_UNKNOWN)
	{
	    cml_errorl(&branch->location,
	    	       "%s \"%s\" redefined as primitive",
		        mn_get_treetype_as_string(mn),
		       mn->name);
	    cml_errorl(&mn->location,
	    	       "location of previous definition");
	    return 0;
	}
	if (mn->parent == 0)
	{
	    /* result of a forward reference */
	    mn_add_child(menu_top(), mn);

    	    while (mn->forward_refs != 0)
	    {
	    	cml_location *loc = (cml_location *)mn->forward_refs->data;
		
		if (mn->expr_count > 0 &&
	    	    rb_warning_enabled(rb, CW_FORWARD_REFERENCE))
		    cml_warningl(loc,
			"forward reference to \"%s\"", mn->name);
		g_free(loc);
	    	mn->forward_refs = g_list_remove_link(mn->forward_refs, mn->forward_refs);
	    }
	    if (mn->expr_count > 0 &&
	    	rb_warning_enabled(rb, CW_FORWARD_REFERENCE))
		cml_warningl(&branch->location,
	    		   "location of definition");
    	    while (mn->forward_deps != 0)
	    {
	    	cml_location *loc = (cml_location *)mn->forward_deps->data;
		
		if (rb_warning_enabled(rb, CW_FORWARD_DEPENDENCY))
		    cml_warningl(loc,
	    		"forward declared symbol \"%s\" used in dependency list",
			mn->name);
		g_free(loc);
	    	mn->forward_deps = g_list_remove_link(mn->forward_deps, mn->forward_deps);
	    }
	}
	else
	{
	    /* symbol was previously defined in the menu tree. */
	    if (mn->parent != menu_top() &&
	    	branch->type != N_DEFINE &&
		rb_warning_enabled(rb, CW_DIFFERENT_PARENT))
	    {
	    	/*
		 * Symbol occurs in two different places in the menu tree,
		 * which will result in confusing presentation in xconfig.
		 */
		cml_warningl(&branch->location, 
	    		   "symbol \"%s\" redefined with different parent",
			   mn->name);
		cml_warningl(&mn->location,
	    		   "location of previous definition");
	    }
	    if (branch->type != N_DEFINE && (mn->flags & MN_WEAK_POSITION))
	    {
	    	/*
		 * Symbol was a define earlier in the corpus, now it's a
		 * user visible symbol.  Move it to the place where the
		 * user will expect to see it.
		 */
	    	mn->flags &= ~MN_WEAK_POSITION;
		mn_reparent(menu_top(), mn);
	    }
	}
	if (banner != 0)
	{
	    branch->flags |= BR_HAS_BANNER;
	    banner = normalise_prompt(banner, &branch->flags);
	    if (mn->banner == 0)
	    {
		mn->banner = banner;
		branch->flags |= BR_WAS_BANNER;
	    }
	    else
	    {
		if (strcmp(mn->banner, banner))
		{
		    if (rb_warning_enabled(rb, CW_DIFFERENT_BANNER))
		    {
			cml_warningl(&branch->location, 
	    		       "symbol \"%s\" redefined with different banner",
			       mn->name);
			cml_warningl(find_banner_location(mn),
	    		       "location of previous definition");
		    }
		}
		else
		    branch->flags |= BR_WAS_BANNER;
		g_free(banner);
	    }
	}
    }
    else
    {
    	/* new item */
	mn = rb_add_node(rb, name);
	g_free(name);
	/* treetype = MN_UNKNOWN is the default */
	mn_add_child(menu_top(), mn);
	if (banner != 0)
	{
	    banner = normalise_prompt(banner, &branch->flags);
	    mn->banner = banner;
	    branch->flags |= BR_WAS_BANNER|BR_HAS_BANNER;
	}
	if (branch->type == N_DEFINE)
	    mn->flags |= MN_WEAK_POSITION;
	if (is_constant_symbol(mn->name))
	    mn->flags |= MN_CONSTANT;
    }

    if (menu_is_root() && rb_warning_enabled(rb, CW_PRIMITIVE_IN_ROOT))
    {
    	switch (branch->type)
	{
	case N_CHOICE:
	case N_COMMENT:
	case N_QUERY:
	case N_DEP_BOOL:
	case N_DEP_MBOOL:
	case N_DEP_TRISTATE:
	    cml_warningl(&branch->location,
	    	    "primitive symbol \"%s\" in root menu",
		    mn->name);
	    break;
	default:
	    break;
	}
    }
    
    if (rb_warning_enabled(rb, CW_FORWARD_DEPENDENCY) ||
    	rb_warning_enabled(rb, CW_CONSTANT_SYMBOL_DEPENDENCY))
    {
    	GList *iter;
	
    	switch (branch->type)
	{
	case N_DEP_BOOL:
	case N_DEP_MBOOL:
	case N_DEP_TRISTATE:
	    assert(branch->exprs != 0);
	    for (iter = branch->exprs ; iter != 0 ; iter = iter->next)
	    {
	    	cml_expr *expr = (cml_expr *)iter->data;
		
		assert(expr->type == E_ATOM || expr->type == E_SYMBOL);
		if (expr->type != E_SYMBOL)
		    continue;
		if (rb_warning_enabled(rb, CW_CONSTANT_SYMBOL_DEPENDENCY) &&
		    (expr->symbol->flags & MN_CONSTANT))
		    cml_warningl(&branch->location,
	    		    "constant symbol \"%s\" used in dependency list for \"%s\"",
			    expr->symbol->name,
			    mn->name);
		if (rb_warning_enabled(rb, CW_FORWARD_DEPENDENCY) &&
		    expr->symbol->parent == 0)
		    /* Remember location for reporting later. */
		    expr->symbol->forward_deps = g_list_append(expr->symbol->forward_deps,
				    	g_memdup(&branch->location,
					    sizeof(branch->location)));
#if 0
		    cml_warningl(&branch->location,
	    		    "forward declared symbol \"%s\" used in dependency list for \"%s\"",
			    expr->symbol->name,
			    mn->name);
#endif
	    }
	    break;
	default:
	    break;
	}
    }
    
    if (rb_warning_enabled(rb, CW_CONSTANT_SYMBOL_MISUSE) &&
    	(mn->flags & MN_CONSTANT) && 
	(branch->type != N_DEFINE ||
	 branch->value_type != A_BOOLEAN ||
	 ((cml_expr *)branch->exprs->data)->type != E_ATOM ||
	 ((cml_expr *)branch->exprs->data)->value.value.tritval != CML_Y ||
	 mn->user_data != 0))
    {
    	cml_warningl(&branch->location,
	    "misuse of constant symbol \"%s\"",
	    mn->name);
    }

    if (rb_warning_enabled(rb, CW_CONDITION_LOOP) &&
	branch->cond != 0 &&
	expr_contains_symbol(branch->cond, mn))
    {
	cml_warningl(&branch->location,
	    "symbol \"%s\" is conditional (eventually) on itself",
	    mn->name);
    }
    
    if (rb_warning_enabled(rb, CW_DEPENDENCY_LOOP))
    {
    	GList *iter;
	    
	for (iter = (GList *)branch->exprs ; iter != 0 ; iter = iter->next)
	{
    	    cml_expr *expr = (cml_expr *)iter->data;

    	    if (expr_contains_symbol(expr, mn))
	    {
		cml_warningl(&branch->location,
	    	    "symbol \"%s\" depends (eventually) on itself",
		    mn->name);
	    }
	}
    }    
    

    assert(mn->parent != 0);
    mn->location = branch->location;
    branch->node = mn;
    mn->user_data = g_list_append((GList *)mn->user_data, branch);
    
    if (branch->flags & BR_HAS_BANNER)
	check_magic_tags(branch);
    
    if (branch->type == N_DEFINE &&
    	rb_warning_enabled(rb, CW_NONLITERAL_DEFINE))
    {
    	cml_expr *def = (cml_expr *)branch->exprs->data;
	
	if (def->type != E_ATOM)
	{
    	    /*
	     * This reportedly confuses xconfig, and is not part of
	     * the language anyway, so warn about it.
	     */
	    char *s = expr_as_string(def);
    	    cml_warningl(&branch->location,
		"symbol \"%s\" defined to non-literal expression \"%s\"",
		mn->name, s);
	    g_free(s);
	}
    }

    DDPRINTF1(DEBUG_CONVERT, "%s\n", mn->name);

    return mn;
}

/*============================================================*/

/* Takes over `str' */
static gboolean
parse_choice_list_m(char *str, char **def_promptp, const cml_location *loc)
{
    char *word, *buf2;
    int n = 0;
    char *prompt = 0;
    cml_branch_t *child;
    char *def_symbol = 0;
    cml_node *mn;
    
    buf2 = str = expand_escapes(str);
    while ((word = strtok(buf2, " \t\r\n")) != 0)
    {
    	buf2 = 0;
	
	if (++n & 1)
	{
	    prompt = word;
	}
	else
	{
	    child = branch_new(N_QUERY);
	    child->value_type = A_BOOLEAN;
	    child->location = *loc;
    	    mn = add_primitive_branch(
	    	    g_strdup(word),
	    	    g_strdup(prompt),
		    child);

	    if (def_symbol == 0 &&
	    	!strncmp(*def_promptp, prompt, strlen(*def_promptp)))
	    	def_symbol = mn->name;
	}
    }
    
    /* TODO: issue error if odd number of words */
    
    g_free(str);
    *def_promptp = def_symbol;
    return TRUE;
}

/*============================================================*/

#define BLACKHOLE   "_blackhole"

static cml_node *
blackhole(cml_rulebase *rb)
{
    cml_node *blackhole;

    if ((blackhole = cml_rulebase_find_node(rb, BLACKHOLE)) == 0)
    {
	blackhole = rb_add_node(rb, BLACKHOLE);
	blackhole->treetype = MN_MENU;
	mn_add_child(rb->start, blackhole);
	/* invisible, hence unsaveable */
	/* TODO: make them saveable */
	blackhole->visibility_expr = expr_new_atom_v(A_BOOLEAN, CML_N);
    }
    
    return blackhole;
}

static void
check_undefined_symbol(gpointer key, gpointer value, gpointer user_data)
{
    cml_node *mn = (cml_node *)value;
    cml_rulebase *rb = (cml_rulebase *)user_data;

    if (mn->treetype == MN_UNKNOWN)
    {
    	while (mn->forward_refs != 0)
	{
	    cml_location *loc = (cml_location *)mn->forward_refs->data;

	    if (rb_warning_enabled(rb, CW_UNDECLARED_SYMBOL))
		cml_warningl(loc,
		    "symbol \"%s\" used but not declared, defaults to \"\"",
		    mn->name);
	    g_free(loc);
	    mn->forward_refs = g_list_remove_link(mn->forward_refs, mn->forward_refs);
	}
    	while (mn->forward_deps != 0)
	{
	    cml_location *loc = (cml_location *)mn->forward_deps->data;

	    if (rb_warning_enabled(rb, CW_UNDECLARED_DEPENDENCY))
		cml_warningl(loc,
		    "symbol \"%s\" used in dependency list but not declared",
		    mn->name);
	    g_free(loc);
	    mn->forward_deps = g_list_remove_link(mn->forward_deps, mn->forward_deps);
	}
	
	mn->treetype = MN_SYMBOL;
	mn->value_type = A_TRISTATE;
	mn->expr = expr_new_atom_v(A_NONE);
	mn_add_child(blackhole(rb), mn);
    }
}

/*============================================================*/

static gboolean
is_forward_sym_and_n(const cml_expr *e1, const cml_expr *e2)
{
    return (e1->type == E_SYMBOL &&
    	    e1->symbol->parent == 0 &&
    	    e2->type == E_ATOM &&
	    e2->value.type == A_TRISTATE &&
	    e2->value.value.tritval == CML_N);
}

/*============================================================*/

#if TESTSCRIPT

static void
add_test(
    cml_test_script_op op,
    char *symbol,
    cml_expr *expr)
{
    cml_node *mn = 0;

    if (symbol != 0 && (mn = cml_rulebase_find_node(rb, symbol)) == 0)
    {
	yyerror("no such symbol \"%s\"", symbol);
	g_free(symbol);
	return;
    }

    if (symbol != 0)
    	g_free(symbol);    
	
    rb_add_test(rb, statement_get_location(), op, mn, expr);
}

#endif /*TESTSCRIPT*/
/*============================================================*/

%}

%union
{
    char *string;
    GList *list;
    cml_expr *expr;
}

/* terminal symbols */
%token <string> PROMPT
%token <string> WORD
%token <string> SYMBOL
%token <string> SYMBOLREF
%token <string> DECIMAL
%token <string> HEXADECIMAL
%token <string> TRISTATE

/* nonterminal symbols */
%type <list> logical_atoms
%type <list> logical_atoms_or_nul
%type <expr> expr
%type <expr> primitive_expr
%type <expr> atom
%type <expr> logical_atom
%type <expr> integer
%type <string> string
%type <string> word_or_symbol
%type <list> symbols

/* operators -- precedence according to CML2 0.6.1 spec */
%left K_OR
%left K_AND
%left K_NOT
%left K_EQUALS K_NOT_EQUALS

/* keywords with unspecified precedence */
%token EOL
%token K_BOOL
%token K_CHOICE
%token K_COMMENT
%token K_DEFINE_BOOL
%token K_DEFINE_HEX
%token K_DEFINE_INT
%token K_DEFINE_STRING
%token K_DEFINE_TRISTATE
%token K_DEP_BOOL
%token K_DEP_HEX
%token K_DEP_INT
%token K_DEP_MBOOL
%token K_DEP_STRING
%token K_DEP_TRISTATE
%token K_ELSE
%token K_ENDMENU
%token K_FI
%token K_HEX
%token K_IF
%token K_INT
%token K_MAINMENU_NAME
%token K_MAINMENU_OPTION
%token K_NEXT_COMMENT
%token K_NULL
%token K_STRING
%token K_TEXT
%token K_THEN
%token K_TRISTATE
%token K_TRITS
%token K_UNSET
/* test script keywords */
%token K_TEST_ASSERT
%token K_TEST_CLEAR
%token K_TEST_COMMIT
%token K_TEST_ERROR
%token K_TEST_FREEZE
%token K_TEST_NOERROR
%token K_TEST_PARSETEST
%token K_TEST_SAVEABLE
%token K_TEST_SET
%token K_TEST_SUCCEEDED
%token K_TEST_VISIBLE

%%

corpus:        	    	  toplevel_block
    	    	    	;

/*
 * "block" and "toplevel_block" are identical except for error
 * detection and recovery.  For example, we cannot attempt to
 * detect an extra endmenu in "block" because it could be a
 * legitimate end of a menu.
 */

toplevel_block:     	  toplevel_statement
    	    		| toplevel_block toplevel_statement
    	    	    	| toplevel_block K_ENDMENU EOL
			    {
			    	cml_location loc = yylocation;
				loc.lineno--;
			    	cml_errorl(&loc, "unbalanced endmenu");
			    }
			| toplevel_block error EOL
			;

toplevel_statement: 	  mainmenu_name_statement
    	    	    	| statement
			| test_script_statement
			;

block:	    	    	  statement
    	    		| block statement
			| block error EOL
			;

statement:  	    	  menu_definition
			| basic_statement
			| conditional
			| EOL
    	    		;

/*
 * TODO: mainmenu_name_statement should only be legal at the top level
 */
mainmenu_name_statement:  K_MAINMENU_NAME PROMPT EOL
			    {
			    	if (rb->banner != 0)
				{
				    if (!rb->merge_mode)
					yyerror("repeated mainmenu_name");
				    g_free($2);
				}
				else
				{
    				    rb->banner = rb_add_node(rb, "__banner");
				    rb->banner->banner = $2;
				}
			    }
    	    	    	;

menu_definition:	  menu_def_header block K_ENDMENU EOL
			    {
			    	menu_pop();
			    }
/*
			| menu_def_header block error EOL
			    {
			    	cml_errorl(&menu_top()->location,
				    	    "unbalanced mainmenu_option");
			    	YYABORT;
			    }
*/
    	    	    	;

menu_def_header:    	  K_MAINMENU_OPTION K_NEXT_COMMENT eols K_COMMENT PROMPT EOL
    	    	    	    {
			    	cml_branch_t *branch = branch_new(N_MENU);
				cml_node *mn;
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
    	    	    	    	if ((mn = add_compound_branch($5, branch)) == 0)
				    YYERROR;
    	    	    	    	menu_push(mn);
			    }
			;
			
/* 1 or more EOLs */
eols:	    	    	  EOL
    	    	    	| eols EOL
			;

basic_statement:    	  comment_statement
    	    	    	| define_bool_statement
    	    	    	| define_hex_statement
			| define_int_statement
			| define_string_statement
			| define_tristate_statement
    	    	    	| bool_statement
			| dep_bool_statement
			| dep_mbool_statement
			| hex_statement
			| int_statement
			| string_statement
			| tristate_statement
			| dep_tristate_statement
			| choice_statement
			| unset_statement
			;

conditional:	    	  K_IF '[' expr ']' { cond_push_m($3); } prethen K_THEN EOL block else_block K_FI EOL
			    {
			    	cond_pop();
			    }
			;
			
prethen:    	    	  ';'
    	    	    	| EOL
			;

else_block: 	    	  /* nothing */
    	    	    	| K_ELSE { cond_else(); } block
			;


comment_statement:  	  K_COMMENT PROMPT EOL
			    {
			    	cml_branch_t *branch = branch_new(N_COMMENT);
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
    	    	    	    	add_compound_branch(normalise_prompt($2, 0), branch);
			    }
    	    	    	;
			
bool_statement:     	  K_BOOL PROMPT SYMBOL logical_atoms_or_nul EOL
			    {
			    	cml_branch_t *branch = branch_new(N_QUERY);
				cml_node *mn;
				branch->value_type = A_BOOLEAN;
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				mn = add_primitive_branch($3, $2, branch);

				if ($4 != 0)
				{
				    if (rb_warning_enabled(rb, CW_CRUD_AFTER_LOGICAL_QUERY))
				    {
					cml_warningl(&branch->location,
				    	    "spurious dependencies after bool \"%s\"",
					    mn->name);
					cml_warningl(&branch->location,
					    "did you mean to use dep_bool?");
    	    	    	    	    }
				    while ($4 != 0)
				    {
					expr_destroy((cml_expr *)$4->data);
					$4 = g_list_remove_link($4, $4);
				    }
				}
			    }
    	    	    	;

/* 
 * CML1's idea of what constitutes a hexadecimal number is sufficiently 
 * flexible that we can't handle it completely at the lexical level.
 */
hex_statement:	    	  K_HEX PROMPT SYMBOL HEXADECIMAL EOL
			    {
			    	cml_branch_t *branch;
			    	if (!parse_hex($4, 0))
				{
				    yyerror("\"%s\" is not a hexadecimal number", $4);
    	    	    	    	    g_free($2);
    	    	    	    	    g_free($3);
    	    	    	    	    g_free($4);
				    YYERROR;
				}
			    	branch = branch_new(N_QUERY);
				branch->value_type = A_HEXADECIMAL;
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				branch_add_expr(branch, expr_new_atom_from_string(A_HEXADECIMAL, $4));
				g_free($4);
				add_primitive_branch($3, $2, branch);
			    }
			| K_HEX PROMPT SYMBOL DECIMAL EOL
			    {
			    	cml_branch_t *branch;
			    	if (!parse_hex($4, 0))
				{
				    yyerror("\"%s\" is not a hexadecimal number", $4);
    	    	    	    	    g_free($2);
    	    	    	    	    g_free($3);
    	    	    	    	    g_free($4);
				    YYERROR;
				}
			    	branch = branch_new(N_QUERY);
				branch->value_type = A_HEXADECIMAL;
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				branch_add_expr(branch, expr_new_atom_from_string(A_HEXADECIMAL, $4));
				g_free($4);
				add_primitive_branch($3, $2, branch);
			    }
			| K_HEX PROMPT SYMBOL WORD EOL
			    {
			    	cml_branch_t *branch;
			    	if (!parse_hex($4, 0))
				{
				    yyerror("\"%s\" is not a hexadecimal number", $4);
    	    	    	    	    g_free($2);
    	    	    	    	    g_free($3);
    	    	    	    	    g_free($4);
				    YYERROR;
				}
			    	branch = branch_new(N_QUERY);
				branch->value_type = A_HEXADECIMAL;
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				branch_add_expr(branch, expr_new_atom_from_string(A_HEXADECIMAL, $4));
				g_free($4);
				add_primitive_branch($3, $2, branch);
			    }
    	    	    	;

int_statement:	    	  K_INT PROMPT SYMBOL DECIMAL EOL
			    {
			    	cml_branch_t *branch = branch_new(N_QUERY);
				branch->value_type = A_DECIMAL;
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				branch_add_expr(branch, expr_new_atom_from_string(A_DECIMAL, $4));
				g_free($4);
				add_primitive_branch($3, $2, branch);
			    }
    	    	    	;

string_statement:   	  K_STRING PROMPT SYMBOL string EOL
			    {
			    	cml_branch_t *branch = branch_new(N_QUERY);
				branch->value_type = A_STRING;
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				branch_add_expr(branch, expr_new_atom_from_string(A_STRING, $4));
				g_free($4);
				add_primitive_branch($3, $2, branch);
			    }
    	    	    	;

tristate_statement:	  K_TRISTATE PROMPT SYMBOL logical_atoms_or_nul EOL
			    {
			    	cml_branch_t *branch = branch_new(N_QUERY);
				cml_node *mn;
				branch->value_type = A_TRISTATE;
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				mn = add_primitive_branch($3, $2, branch);
				
				if ($4 != 0)
				{
				    if (rb_warning_enabled(rb, CW_CRUD_AFTER_LOGICAL_QUERY))
				    {
					cml_warningl(&branch->location,
				    	    "spurious dependencies after tristate \"%s\"",
					    mn->name);
					cml_warningl(&branch->location,
					    "did you mean to use dep_tristate?");
    	    	    	    	    }
				    while ($4 != 0)
				    {
					expr_destroy((cml_expr *)$4->data);
					$4 = g_list_remove_link($4, $4);
				    }
				}
			    }
    	    	    	;

/*
 * According to Documentation/kbuild/config-language.txt, the last
 * argument of a define_bool should be a TRISTATE, but I have extended
 * it to "logical_atom" by analogy with define_tristate.
 */
define_bool_statement:	  K_DEFINE_BOOL SYMBOL logical_atom EOL
			    {
			    	cml_branch_t *branch = branch_new(N_DEFINE);
				branch->value_type = A_BOOLEAN;
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				branch_add_expr(branch, $3);
				add_primitive_branch($2, 0, branch);
			    }
    	    	    	;

define_hex_statement:	  K_DEFINE_HEX SYMBOL DECIMAL EOL
			    {
			    	cml_branch_t *branch;
			    	if (!parse_hex($3, 0))
				{
				    yyerror("\"%s\" is not a hexadecimal number", $3);
    	    	    	    	    g_free($2);
    	    	    	    	    g_free($3);
				    YYERROR;
				}
			    	branch = branch_new(N_DEFINE);
				branch->value_type = A_HEXADECIMAL;
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				branch_add_expr(branch, expr_new_atom_from_string(A_HEXADECIMAL, $3));
				g_free($3);
				add_primitive_branch($2, 0, branch);
			    }
			| K_DEFINE_HEX SYMBOL HEXADECIMAL EOL
			    {
			    	cml_branch_t *branch;
			    	if (!parse_hex($3, 0))
				{
				    yyerror("\"%s\" is not a hexadecimal number", $3);
    	    	    	    	    g_free($2);
    	    	    	    	    g_free($3);
				    YYERROR;
				}
			    	branch = branch_new(N_DEFINE);
				branch->value_type = A_HEXADECIMAL;
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				branch_add_expr(branch, expr_new_atom_from_string(A_HEXADECIMAL, $3));
				g_free($3);
				add_primitive_branch($2, 0, branch);
			    }
			| K_DEFINE_HEX SYMBOL WORD EOL
			    {
			    	cml_branch_t *branch;
			    	if (!parse_hex($3, 0))
				{
				    yyerror("\"%s\" is not a hexadecimal number", $3);
    	    	    	    	    g_free($2);
    	    	    	    	    g_free($3);
				    YYERROR;
				}
			    	branch = branch_new(N_DEFINE);
				branch->value_type = A_HEXADECIMAL;
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				branch_add_expr(branch, expr_new_atom_from_string(A_HEXADECIMAL, $3));
				g_free($3);
				add_primitive_branch($2, 0, branch);
			    }
    	    	    	;

define_int_statement:	  K_DEFINE_INT SYMBOL integer EOL
			    {
			    	cml_branch_t *branch = branch_new(N_DEFINE);
				branch->value_type = A_DECIMAL;
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				branch_add_expr(branch, $3);
				add_primitive_branch($2, 0, branch);
			    }
    	    	    	;

define_string_statement:  K_DEFINE_STRING SYMBOL WORD EOL
			    {
			    	cml_branch_t *branch = branch_new(N_DEFINE);
				branch->value_type = A_STRING;
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				branch_add_expr(branch, expr_new_atom_v(A_STRING, $3));
				add_primitive_branch($2, 0, branch);
			    }
    	    	    	;

/*
 * According to Documentation/kbuild/config-language.txt, the last
 * argument of a define_tristate should be a TRISTATE, but I have extended
 * it to "logical_atom" to handle the CML1 corpus where SYMBOLREFs occur
 * in that position.
 */
define_tristate_statement:K_DEFINE_TRISTATE SYMBOL logical_atom EOL
			    {
			    	cml_branch_t *branch = branch_new(N_DEFINE);
				branch->value_type = A_TRISTATE;
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				branch_add_expr(branch, $3);
				add_primitive_branch($2, 0, branch);
			    }
    	    	    	;

dep_bool_statement:	  K_DEP_BOOL PROMPT SYMBOL logical_atoms EOL
			    {
			    	cml_branch_t *branch = branch_new(N_DEP_BOOL);
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				branch->exprs = $4;
				add_primitive_branch($3, $2, branch);
			    }
    	    	    	;

dep_mbool_statement:	  K_DEP_MBOOL PROMPT SYMBOL logical_atoms EOL
			    {
			    	cml_branch_t *branch = branch_new(N_DEP_MBOOL);
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				branch->exprs = $4;
				add_primitive_branch($3, $2, branch);
			    }
    	    	    	;

dep_tristate_statement:	  K_DEP_TRISTATE PROMPT SYMBOL logical_atoms EOL
			    {
			    	cml_branch_t *branch = branch_new(N_DEP_TRISTATE);
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();
				branch->exprs = $4;
				add_primitive_branch($3, $2, branch);
			    }
    	    	    	;

/* choice prompt choices_in_string default */
choice_statement:   	  K_CHOICE PROMPT PROMPT word_or_symbol EOL
    	    	    	    {
			    	char *deflt;
				cml_node *mn;
				cml_branch_t *branch;

			    	branch = branch_new(N_CHOICE);
				branch->location = statement_get_location();
				branch->cond = cond_copy_top();

    	    	    	    	mn = add_compound_branch(normalise_prompt($2, 0), branch);
    	    	    	    	if (mn == 0)
				    YYERROR;

				menu_push(mn);
				deflt = $4;
				parse_choice_list_m($3, &deflt, &branch->location);
				menu_pop();

				if (mn->children == 0 &&
				    rb_warning_enabled(rb, CW_EMPTY_CHOICES))
				{
				    cml_warningl(&branch->location,
					"empty \"choices\" statement");
				}

    	    	    	    	/* TODO: build the default value into an expr */
				if (deflt == 0 && mn->children != 0)
				{
				    /* TODO: upgrade this to an ERROR */
				    cml_node *defchild = (cml_node *)mn->children->data;
				    deflt = defchild->name;
				    if (rb_warning_enabled(rb, CW_DEFAULT_NOT_IN_CHOICES))
					cml_warningl(&branch->location,
				    	    "default \"%s\" not in choices list, using \"%s\"",
						$4, defchild->banner);
				}
				if (deflt != 0)
				    branch->choice_default = g_strdup(deflt);
    	    	    	    	g_free($4);
			    }

unset_statement:    	  K_UNSET symbols
    	    	    	    {
			    	cml_location loc = statement_get_location();
				if (rb_warning_enabled(rb, CW_UNSET_STATEMENT))
			    	    cml_warningl(&loc,
					"unsupported \"unset\" statement used");
				    
				while ($2 != 0)
				{
				    g_free($2->data);
				    $2 = g_list_remove_link($2, $2);
				}
			    }

/*
 * These productions allow a test script to be provided in the
 * CML1 file, using a series of #@something keywords.  This feature
 * is for regression testing the rulebase.  Note that the leading
 * # character makes the test script invisible to the other parsers.
 */
test_script_statement:	  K_TEST_ASSERT expr
			    {
#if TESTSCRIPT
				add_test(TS_ASSERT, 0, $2);
#endif
			    }
    	    	    	| K_TEST_CLEAR
			    {
#if TESTSCRIPT
				add_test(TS_CLEAR, 0, 0);
#endif    
			    }
    	    	    	| K_TEST_COMMIT
			    {
#if TESTSCRIPT
				add_test(TS_COMMIT, 0, 0);
#endif    
			    }
    	    	    	| K_TEST_ERROR PROMPT
			    {
#if TESTSCRIPT
				add_test(TS_ERROR, 0,
					    expr_new_atom_v(A_STRING, $2));
#endif    
			    }
    	    	    	| K_TEST_FREEZE
			    {
#if TESTSCRIPT
				add_test(TS_FREEZE, 0, 0);
#endif    
			    }
    	    	    	| K_TEST_NOERROR
			    {
#if TESTSCRIPT
				add_test(TS_NOERROR, 0, 0);
#endif    
			    }
    	    	    	| K_TEST_PARSETEST
			    {
#if TESTSCRIPT
    	    	    	    	rb->parsetest = TRUE;
#endif    
			    }
    	    	    	| K_TEST_SAVEABLE SYMBOL TRISTATE
			    {
#if TESTSCRIPT
				add_test(TS_SAVEABLE, $2,
				    expr_new_atom_from_string(A_TRISTATE, $3));
#endif
			    }
    	    	    	| K_TEST_SET SYMBOL logical_atom
			    {
#if TESTSCRIPT
				add_test(TS_SET, $2, $3);
#endif    
			    }
    	    	    	| K_TEST_SUCCEEDED TRISTATE
			    {
#if TESTSCRIPT
				add_test(TS_SUCCEEDED, 0,
				    expr_new_atom_from_string(A_TRISTATE, $2));
#endif
			    }
    	    	    	| K_TEST_VISIBLE SYMBOL TRISTATE
			    {
#if TESTSCRIPT
				add_test(TS_VISIBLE, $2,
				    expr_new_atom_from_string(A_TRISTATE, $3));
#endif
			    }
			;



/*
 * Note that a naked /atom/ is not a valid /expr/. 
 */
expr: 	    	    	  primitive_expr
			| expr K_OR expr
    	    	    	    {
			    	assert($1->value.type == A_BOOLEAN);
			    	assert($3->value.type == A_BOOLEAN);
			    	$$ = expr_new_composite(E_OR, $1, $3);
			    }
			| expr K_AND expr
    	    	    	    {
			    	assert($1->value.type == A_BOOLEAN);
			    	assert($3->value.type == A_BOOLEAN);
			    	$$ = expr_new_composite(E_AND, $1, $3);
			    }
			| K_NOT expr
    	    	    	    {
			    	assert($2->value.type == A_BOOLEAN);
			    	$$ = expr_new_composite(E_NOT, $2, 0);
			    }
			;

primitive_expr:     	  atom K_EQUALS atom
    	    	    	    {
			    	$$ = expr_new_composite(E_EQUALS, $1, $3);
			    }
			| atom K_NOT_EQUALS atom
    	    	    	    {
			    	cml_expr *e = 0;
			    	$$ = expr_new_composite(E_NOT_EQUALS, $1, $3);
				if (rb_warning_enabled(rb, CW_FORWARD_COMPARED_TO_N) &&
    	    	    	    	    (is_forward_sym_and_n(e = $1, $3) ||
				     is_forward_sym_and_n(e = $3, $1)))
				{
				    cml_warningl(&yylocation,
				    	"forward declared symbol \"%s\" compared ambiguously to \"n\"",
					e->symbol->name);
				}
			    }
			;

atom:	    	    	  PROMPT
    	    	    	    {
			    	$$ = expr_new_atom_from_string(A_STRING, $1);
				g_free($1);
			    }
    	    	    	| logical_atom
			;


logical_atoms_or_nul:	  /* nothing */
    	    	    	    {
			    	$$ = 0;
			    }
    	    	    	| logical_atoms
			;

logical_atoms:	    	  logical_atom
    	    	    	    {
			    	$$ = g_list_append(0, $1);
			    }
			| logical_atoms logical_atom
			    {
			    	$$ = g_list_append($1, $2);
			    }
			;

logical_atom:	    	  SYMBOLREF
    	    	    	    {
			    	cml_node *mn;
				
				if ((mn = cml_rulebase_find_node(rb, $1)) == 0)
				{
				    /* handle forward references */
				    mn = rb_add_node(rb, $1);
				    mn->location = yylocation;
				    if (is_constant_symbol($1))
				    	mn->flags |= MN_CONSTANT;
				}
				if (mn->parent == 0)
				{
				    /* Remember location for reporting later. */
				    mn->forward_refs = g_list_append(mn->forward_refs,
				    	    	    	g_memdup(&yylocation,
							    sizeof(yylocation)));
				}
				rb_add_xref(rb, mn, "kcexpr", &yylocation);
				$$ = expr_new_symbol(mn);
				g_free($1);
			    }
    	    	    	| TRISTATE
    	    	    	    {
			    	$$ = expr_new_atom_from_string(A_TRISTATE, $1);
				g_free($1);
			    }
    	    	    	| K_NULL
    	    	    	    {
			    	/*
				 * The "null" keyword is an extension to CML1
				 * which allows expressions to be constructed
				 * which explicitly test against the default
				 * A_NONE value.  This is useful for test
				 * scripts.
				 */
			    	$$ = expr_new_atom_v(A_NONE);
			    }
			;

integer:    	    	  DECIMAL
    	    	    	    {
			    	$$ = expr_new_atom_from_string(A_DECIMAL, $1);
				g_free($1);
			    }
    	    	    	| HEXADECIMAL
    	    	    	    {
			    	$$ = expr_new_atom_from_string(A_HEXADECIMAL, $1);
				g_free($1);
			    }
			;

string:     	    	  WORD
    	    	    	| PROMPT
			;

/* special hack to handle a common `choice' syntax error */
word_or_symbol:     	  WORD
    	    	    	| SYMBOL
			;

symbols:    	    	  SYMBOL
			    {
			    	$$ = g_list_append(0, $1);
			    }
    	    	    	| symbols SYMBOL
			    {
			    	$$ = g_list_append($1, $2);
			    }
			;

%%

#include "cml1_lexer.c"

static void
yyerror(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    cml_messagelv(CML_ERROR, &yylocation, fmt, args);
    va_end(args);
}

/*============================================================*/

extern void cml1_pass2(cml_rulebase *rb);

static gboolean
str_has_prefix(const char *s, const char *pref)
{
    return (strncmp(s, pref, strlen(pref)) == 0);
}

static gboolean
str_has_suffix(const char *s, const char *suff)
{
    int s_len = strlen(s);
    int suff_len = strlen(suff);
    if (s_len < suff_len)
    	return FALSE;
    return (strcmp(s+s_len-suff_len, suff) == 0);
}

static char *
intuit_arch(const char *filename)
{
    char *str;
    static const char prefix[] = "arch/";
    static const char suffix[] = "/config.in";
 
    if (!str_has_prefix(filename, prefix) ||
    	!str_has_suffix(filename, suffix))
	return g_strdup("unknown");
    str = g_strdup(filename+(sizeof(prefix)-1));
    str[(sizeof(suffix)-1)-(sizeof(prefix)-1)] = '\0';
    DDPRINTF2(DEBUG_PARSER, "intuited $ARCH=\"%s\" from filename \"%s\"\n",
    	    	    str, filename);
    return str;
}

gboolean
_cml_rulebase_parse_cml1(cml_rulebase *rbi, const char *filename)
{
    gboolean failed = FALSE;
    
#if DEBUG
    cml1_yy_flex_debug = (debug & DEBUG_LEXER ? 1 : 0);
    cml1_yydebug = (debug & DEBUG_PARSER ? 1 : 0);
#endif

    rb = rbi;
    if (!yylex_push_file(filename))
    {    
    	rb = 0;
    	return FALSE;
    }

#if DEBUG
    if (debug & DEBUG_PARSER)
    {
    	int i;
	
    	DDPRINTF0(DEBUG_PARSER, "parser warning options:\n");
	for (i = 0 ; i < CW_NUM_WARNINGS ; i++)
    	    DDPRINTF2(DEBUG_PARSER, "    -W%s%s\n",
	    	(rb_warning_enabled(rb, i) ? "" : "no-"),
		cml_warning_name_by_id(i));
    }
#endif

    statement_init();
    cml_message_count[CML_ERROR] = 0;

    if (rb->start == 0)
    {
	rb->start = rb_add_node(rb, "__start");
	rb->start->treetype = MN_MENU;
    }
    menu_push(rb->start);
    rb->cml1_default_vals = TRUE;

    if (rb->merge_mode)
    	cond_push_m(expr_new_composite(E_EQUALS,
	    	    	expr_new_symbol(cml_rulebase_find_node(rb, "ARCH")),
			expr_new_atom_v(A_STRING, intuit_arch(filename))));
    
    if (yyparse())
    {
    	/* clean up possibly unbalanced parsetime stacks */
    	while (cond_stack != 0)
	    cond_pop();
    	while (menu_stack != 0)
	    menu_pop();
    	failed = TRUE;
    }
    else
    {
    	if (rb->merge_mode)
	    cond_pop();
    	assert(cond_stack == 0);
    	assert(menu_top() == rb->start);
	menu_pop();
    }
    if (cml_message_count[CML_ERROR] > 0)
	failed = TRUE;
    assert(cond_stack == 0);
    assert(menu_stack == 0);

    if (!failed && !rb->merge_mode)
	cml1_pass2(rb);

    /*
     * Find all symbols which have been referenced but not defined,
     * and provide invisible, unsaveable, default definitions for
     * them.  These symbols could be either defined by other arches
     * or simply not defined anywhere; without parsing and merging
     * all arch trees there is simply no way to tell.  So we follow
     * the undocumented effective semantics of CML1.
     * Except of course in merge mode, where the check is delayed
     * to cml_rulebase_post_parse() and becomes an error, because
     * we *are* parsing all the arch trees.
     */
    if (!failed && !rb->merge_mode)
	g_hash_table_foreach(rb->menu_nodes, check_undefined_symbol, rb);

    rb = 0;
    return !failed;
}

/*============================================================*/
/*END*/
