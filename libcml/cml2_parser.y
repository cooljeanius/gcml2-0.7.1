
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
 */
 
#include "private.h"
#include "debug.h"
#include <malloc/malloc.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#define YYERROR_VERBOSE 1
static cml_rulebase *rb;    /* the rulebase currently being parsed */
static cml_location yylocation;
static char *find_file(const char *relfilename);

static void yyerror(const char *fmt, ...);
/*static int yywrap(void);*/
static int yylex(void);

CVSID("$Id: cml2_parser.y,v 1.2 2002/09/01 08:40:43 gnb Exp $");

/*============================================================*/

/* Simple 2-place circular buffer of location records,
 * needed to work around parse sequence.
 */
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

/*============================================================*/

static cml_expr *
expr_new_tristate_atom(cml_tritval v)
{
    return expr_new_atom_v((v == CML_M ? A_TRISTATE : A_BOOLEAN), v);
}

/*============================================================*/

static gboolean
check_mn_type(cml_node *mn, int typemask)
{
    char *expected, *p;
    int i, len;
    
    if ((1<<mn->treetype) & typemask)
    	return TRUE;

    /* build a string describing the expected types */	
    len = 0;	
    for (i=1 ; i<=MN_MAX ; i++)
	if (typemask & (1<<i))
	    len += 4 + strlen(mn_treetype_as_string(i));

    p = expected = g_malloc(len);

    for (i=1 ; i<=MN_MAX ; i++)
    {
	if (typemask & (1<<i))
	{
	    if (p > expected)
	    {
		strcpy(p, " or ");
		p += 4;
	    }
	    strcpy(p, mn_treetype_as_string(i));
	    p += strlen(p);
	}
    }
    *p = '\0';
    /* expected is now like "foo or bar or baz" */

    /* TODO: location */
    yyerror("Expecting %s, got %s `%s'",
	expected,
	mn_get_treetype_as_string(mn),
	mn->name);

    g_free(expected);
    return FALSE;
}

#define check_mn_is_menu(mn)	    	    	    check_mn_type(mn, 1<<MN_MENU)
#define check_mn_is_menu_or_unknown(mn)     	    check_mn_type(mn, (1<<MN_MENU)|(1<<MN_UNKNOWN))
#define check_mn_is_symbol_or_derived(mn)	    check_mn_type(mn, (1<<MN_SYMBOL)|(1<<MN_DERIVED))
#define check_mn_is_symbol_or_menu(mn)	    	    check_mn_type(mn, (1<<MN_SYMBOL)|(1<<MN_MENU))
#define check_mn_is_symbol_or_derived_or_unknown(mn) check_mn_type(mn, (1<<MN_SYMBOL)|(1<<MN_DERIVED)|(1<<MN_UNKNOWN))
#define check_mn_is_derived(mn)     	    	    check_mn_type(mn, 1<<MN_DERIVED)
#define check_mn_is_symbol(mn)     	    	    check_mn_type(mn, 1<<MN_SYMBOL)
#define check_mn_is_explanation(mn)     	    check_mn_type(mn, 1<<MN_EXPLANATION)
#define check_mn_is_not_unknown(mn) 	    	    check_mn_type(mn, ~(1<<MN_UNKNOWN))


static gboolean
set_mn_treetype(cml_node *mn, cml_node_treetype tt)
{
    if (mn->treetype != MN_UNKNOWN)
    {
	yyerror("%s `%s' already defined as %s",
	    mn_treetype_as_string(tt),
	    mn->name,
	    mn_get_treetype_as_string(mn));
	cml_errorl(&mn->location,
		"previous definition of `%s'",
		mn->name);
	return FALSE;
    }
    mn->treetype = tt;
    DDPRINTF3(DEBUG_PARSER, "Adding %s %ld `%s'\n",
	    mn_get_treetype_as_string(mn),
	    mn->uniqueid,
	    mn->name);
    return TRUE;
}

static void
set_value_type(cml_node *mn, int c)
{
    if (mn->treetype != MN_SYMBOL)
    {
    	if (c != ' ')
	    yyerror("setting value type on %s %s",
		mn_get_treetype_as_string(mn),
		mn->name);
    	return;
    }
    
    switch (c)
    {
    case ' ':
    	mn->value_type = A_BOOLEAN;
	break;
    case '?':
    	mn->value_type = A_TRISTATE;
	break;
    case '!':
    	mn->value_type = A_TRISTATE;
    	mn->flags |= MN_VITAL;
	break;
    case '%':
    	mn->value_type = A_DECIMAL;
	break;
    case '@':
    	mn->value_type = A_HEXADECIMAL;
	break;
    case '$':
    	mn->value_type = A_STRING;
	break;
    default:
    	yyerror("unknown suffix character `%c'", c);
	break;
    }
}


static gboolean
set_default_expr(cml_node *mn, cml_expr *expr)
{
    if (!check_mn_is_symbol(mn))
    	return FALSE;
	
    if (mn->expr != 0)
    {
	yyerror("symbol `%s' has two default statements", mn->name);
	/*TODO: remember location of previous one */
    	return FALSE;
    }
    
    mn->expr = expr;
    
    return TRUE;
}

/*============================================================*/

static gboolean
check_mn_is_orphan(const cml_node *mn)
{
    if (mn->parent == 0)
    	return TRUE;

    yyerror("menu node `%s' already has a parent `%s'",
    	mn->name,
	mn->parent->name);
        
    return FALSE;
}

/*============================================================*/

/*
 * Create and attach expressions which express the implicit
 * visibility rule for subtrees of the form SYM1 { SYM2 SYM3 }
 * and also dependencies.
 */
 
static void
handle_subtree(cml_node *guard, GList *list)
{
    cml_expr *expr;
    cml_atom a;
    
    assert(guard->treetype == MN_SYMBOL);
    
    switch (guard->value_type)
    {
    case A_BOOLEAN:
	expr = expr_new_symbol(guard);
    	break;
    case A_TRISTATE:
	cml_atom_init(&a);
	a.type = A_TRISTATE;
	a.value.tritval = CML_N;
	expr = expr_new_composite(E_NOT_EQUALS,
	    	    expr_new_symbol(guard),
		    expr_new_atom(&a));
    	break;
    /*
     * In a message on kbuild-devel, 23 Apr 2001, ESR clarified
     * that integral types can be used as the guards of subtrees.
     */
    case A_DECIMAL:
    case A_HEXADECIMAL:
	cml_atom_init(&a);
	a.type = guard->value_type;
	a.value.integer = 0;
	expr = expr_new_composite(E_NOT_EQUALS,
	    	    expr_new_symbol(guard),
		    expr_new_atom(&a));
    	break;
    default:
    	yyerror("can only use boolean, tristate, or integral variables as guard for subtree `%s'",
	    guard->name);
    	return;
    }
    
    
    for ( ; list != 0 ; list = list->next)
    {
    	cml_node *mn = (cml_node *)list->data;

    	if (mn->flags & MN_SUBTREE_SEEN)
	    continue;
	    
	if (mn->treetype == MN_SYMBOL)
	    mn_add_dependant(guard, mn);
	    
	mn_add_visibility_expr(mn, expr);

	mn->flags |= MN_SUBTREE_SEEN;
    }
}

/*============================================================*/

%}

%union
{
    cml_expr *expr;
    cml_node *node;
    char *string;
    unsigned long integer;
    cml_tritval tritval;
    cml_range *range;
    cml_subrange subrange;
    cml_enumdef *enumdef;
    GList *list;
    cml_blob *blob;
}

/* terminal symbols */
%token <node> SYMBOL
%token <string> STRING
%token <integer> DECIMAL
%token <integer> HEXADECIMAL
%token <blob> BINARYDATA
%token <tritval> TRITVAL

/* nonterminal symbols */
%type <expr> expr atom term relational logical ternary constant
%type <integer> integer feature_flag suffix
%type <range> subrange_list
%type <enumdef> enumdef
%type <list> enumdef_list
%type <subrange> subrange
%type <node> explanation
%type <list> name_or_subtree
%type <list> name_or_subtree_list symbol_list name_list
%type <integer> dependant_flag unless_when
%type <string> helptext

/* operators -- precedence according to CML2 0.6.1 spec */
%right '?' ':'
%left '+' '-'
%left '*'
%left K_IMPLIES
%left K_OR
%left K_AND
%left K_NOT
%left K_EQUALS K_NOT_EQUALS K_GREATER_EQUALS K_LESS_EQUALS '>' '<'
%left '&' '|' '$'

/* keywords with unspecified precedence */
%token K_BANNER
%token K_CHOICES
%token K_CONDITION
%token K_DEBUG
%token K_DEFAULT
%token K_DEPENDENT
%token K_DERIVE
%token K_ENUM
%token K_EXPLANATION
%token K_EXPLANATIONS
%token K_FROM
%token K_ICON
%token K_MENU
%token K_MENUS
%token K_NOHELP
%token K_ON
%token K_PREFIX
%token K_PROHIBIT
%token K_RANGE
%token K_REQUIRE
%token K_SAVE
%token K_SHOW
%token K_SOURCE
%token K_START
%token K_SUPPRESS
%token K_SYMBOLS
%token K_TEXT
%token K_TRITS
%token K_UNLESS
%token K_WARNDEPEND
%token K_WHEN
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

system:     	    	declaration_list ;
	    
declaration_list:    	  declaration
    	    		| declaration_list declaration
			;

declaration:	          symbols_declaration
			| menus_declaration
			| explanations_declaration
			| visibility_rule
			| visibility_rule2
			| saveability_rule
			| menu_definition
			| choices_definition
			| derive_definition
			| default_definition
			| requirement_definition
			| start_definition
			| prefix_definition
			| banner_definition
			| condition_declaration
			| warndepend_declaration
			| icon_definition
			| debug_definition
			| test_script_entry
    	    		;

/*
 * IV.3.1 Source statements
 *
 * A source statement declares a file to be inserted in place of the
 * source statement in the file, and treated as if the entire contents of
 * that file were present in the current file at the point of their source
 * statement.
 * 
 * Any implementation of CML2 must allow source statements to be nested
 * to a depth of at least 15 levels.  The reference implementation has no
 * hard limit.
 *
 * GNB Note: `source' is not really a statement because it does
 * lexical inclusion, and can be freely mixed with real declarations.
 * For this reason it's implemented in lexer.l.
 */

/*
 * IV.3.2. Symbol declarations
 * 
 * The body of a symbols section consists of pairs of tokens; a
 * configuration symbol and a prompt string.
 * 
 * Rationale: Having these name-to-prompt associations be separate from
 * the dependency rules will help make the text parts of the system
 * easier to localize for different languages.  Declaring all symbols
 * up front means we can do better and faster sanity checks.
 */
symbols_declaration:	  K_SYMBOLS symbol_def_list
    	    	    	;

symbol_def_list: 	  /* nothing */
    	    	    	| symbol_def_list symbol_def
			;

symbol_def: 	    	  SYMBOL STRING helptext
    	    	    	    {
			    	set_mn_treetype($1, MN_SYMBOL);
			    	$1->banner = $2;
				$1->help_text = $3;
			    }
			;
			
helptext:   	    	  /* nothing */
    	    	    	    {
				$$ = 0;
			    }
		    	| K_TEXT STRING
			    {
			    	$$ = $2;
			    }
			;

/*
 * IV.3.3. Menu declarations
 * 
 * The body of a menus section consists of pairs of tokens; a menu name
 * and a banner string.  The effect of each declaration is to declare an
 * empty menu (to be filled in later by a menu definition) and associate
 * a banner string with it.
 * 
 * Any implementation of CML2 must allow menus to be nested to a depth of
 * at least 15 levels.  The reference implementation has no hard limit.
 * 
 * Rationale: Having these menu-to-banner associations be separate from
 * the dependency rules will help make the text parts of the system
 * easier to localize for different languages.  Declaring all menu names
 * up front means we can do better and faster sanity checks.
 */
menus_declaration:  	  K_MENUS menuid_def_list
			;

menuid_def_list: 	  /* nothing */
    	    	    	| menuid_def_list menuid_def
			;

menuid_def: 	    	  SYMBOL STRING helptext
    	    	    	    {
			    	set_mn_treetype($1, MN_MENU);
			    	$1->banner = $2;
				$1->help_text = $3;
			    }
    	    	    	;


/*			
 * Explanation declarations
 * 
 * The body of an explanation section consists of pairs of tokens; an
 * explanation symbol and a banner string. Explanation symbols are used
 * to associate explanatory text with requirements.
 * 
 * Rationale: This declaration tells the compiler that it's OK for the
 * explanation symbol not to occur in a menu.
 */
explanations_declaration: K_EXPLANATIONS explan_def_list
			;

explan_def_list: 	  /* nothing */
    	    	    	| explan_def_list explan_def
			;

explan_def: 	    	  SYMBOL STRING
    	    	    	    {
			    	set_mn_treetype($1, MN_EXPLANATION);
			    	$1->banner = $2;
			    }

symbol_list:	    	  SYMBOL
    	    	    	    {
			    	$$ = g_list_append(0, $1);
			    }
    	    	    	| symbol_list SYMBOL
    	    	    	    {
			    	$$ = g_list_append($1, $2);
			    }
			;
			
/*
 * IV.3.6. Visibility rules
 * 
 * A visibility declaration associates a visibility predicate with a set of
 * configuration symbols.  The fact that several symbols may occur on the
 * right side of such a rule is just a notational convenience; the rule
 * 
 *       unless GUARD suppress SYMBOL1 SYMBOL2 SYMBOL3
 * 
 * is exactly equivalent to
 * 
 *       unless GUARD suppress SYMBOL1
 *       unless GUARD suppress SYMBOL2
 *       unless GUARD suppress SYMBOL3
 * 
 * Putting a menu on the right side of a visibility rule suppresses that
 * menu and all its children.
 * 
 * IV.3.6.1 Dependence
 * 
 * Optionally, a rule may declare that the suppressed symbols are constrained
 * in value by the predicate symbols.  That is, if there is a rule
 * 
 *       unless GUARD suppress dependent SYMBOL
 * 
 * then the value of SYMBOL is constrained by the value of GUARD in the 
 * following way: 
 * 
 * guard	trit	bool
 * -----	------	-------
 *   y      	y,m,n	y,n
 *   m      	m,n	y,n
 *   n      	n	n
 * 
 * The reason for this odd, type-dependent logic table is that we want to
 * be able to have boolean option symbols that configure options for
 * modular ancestors.  This is why the guard symbol value m permits a
 * dependent boolean symbol (but not a dependent modular symbol) to be y.
 * 
 * If the guard part is an expression, SYMBOL is made dependent on each
 * symbol that occurs in the guard.  Such guards may not contain
 * alternations or `implies'.  Thus, if FOO and BAR and BAZ are
 * trit symbols, 
 * 
 *      unless FOO!=n and BAR==m suppress dependent BAZ
 * 
 * is equivalent to the following rules:
 * 
 *      unless FOO!=n and BAR==m suppress BAZ
 *      require BAZ <= FOO and BAZ <= BAR 
 * 
 * Putting a menu on the right side of a visibility rule with `dependent'
 * puts the constraint on all the configuration symbols in that menu.
 * Any submenus will inherit the constraint and pass it downward to
 * their submenus.
 * 
 * Dependency works both ways.  If a dependent symbol is set y or m, the
 * value of the ancestor symbol may be forced; see also the section
 * `Symbol Assignment and Side Effects',
 * 
 * Rationale: The syntax is unless...suppress rather than if...query
 * because the normal state of a symbol or menu is visible.
 * The `dependent' construction replaces the dep_trival and dep_bool
 * constructs in CML1.
 *
 * TODO: check that `logical' does not contain alternations or `implies'
 */
visibility_rule:    	  unless_when logical K_SUPPRESS dependant_flag name_list
			    {
			    	GList *node = $5;
				cml_expr *expr = ($1 ? $2 : expr_new_composite(E_NOT, $2, 0));

				while (node != 0)
				{
				    cml_node *mn = (cml_node *)node->data;

    	    	    	    	    mn_add_visibility_expr(mn, expr);
				    if ($4)
					_expr_add_dependant(expr, mn);
				    node = g_list_remove_link(node, node);
				}
			    }
			;
			
visibility_rule2:    	  unless_when logical K_SHOW name_list
			    {
			    	/*
    	    	    	    	 * Same as standard visibility_rule but with
				 * logic that humans can understand.  Normal
				 * usage is "when logical show symbol"
				 */
			    	GList *node = $4;
				cml_expr *expr = ($1 ? expr_new_composite(E_NOT, $2, 0) : $2);

				while (node != 0)
				{
				    cml_node *mn = (cml_node *)node->data;

    	    	    	    	    mn_add_visibility_expr(mn, expr);
				    node = g_list_remove_link(node, node);
				}
			    }
			;
			
dependant_flag:     	  /* nothing */
    	    	    	    {
			    	$$ = FALSE;
			    }			
			| K_DEPENDENT
			    {
			    	$$ = TRUE;
			    }
			;
		    
saveability_rule:    	  unless_when logical K_SAVE name_list
			    {
			    	GList *node = $4;
				cml_expr *expr = ($1 ? expr_new_composite(E_NOT, $2, 0) : $2);

				while (node != 0)
				{
				    cml_node *mn = (cml_node *)node->data;

    	    	    	    	    mn_add_saveability_expr(mn, expr);
				    node = g_list_remove_link(node, node);
				}
			    }
			;
			
unless_when:	    	  K_UNLESS
    	    	    	    {
			    	$$ = TRUE;
			    }
			| K_WHEN
    	    	    	    {
			    	$$ = FALSE;
			    }
			;
			

name_list:	    	  SYMBOL
    	    	    	    {
			    	$$ = g_list_append(0, $1);
			    }
    	    	    	| name_list SYMBOL
    	    	    	    {
			    	$$ = g_list_append($1, $2);
			    }
			;
			
/*
 * IV.3.7. Menu definitions
 * 
 * A menu definition associates a sequence of configuration symbols and
 * (sub)menu identifiers with a menu identifier (and its banner string).
 * It is an error for any symbol or menu name to be referenced in more
 * than one menu.
 * 
 * Symbol references in menus may have suffixes which change the default
 * boolean type of the symbol.  The suffixes are as follows:
 * 
 * 	?      trit type
 * 	!      vital trit type
 * 	%      decimal type
 * 	@      hexadecimal type
 * 	$      string type
 * 
 * A choices definition associates a choice of boolean configuration
 * symbols with a menu identifier (and its banner string). It declares
 * a default symbol to be set to `y' at the time the menu is instantiated.
 * 
 * In a complete CML2 system, these definitions link all menus together
 * into a single big tree, which is normally traversed depth-first
 * (except that visibility predicates may suppress parts of it).
 * 
 * If the list of symbols has subtrees in it (indicated by curly braces)
 * then the symbol immediately before the opening curly brace is declared
 * a visibility and dependency guard for all symbols within the braces. 
 * That is, the menu declaration
 * 
 * 	menu foo 
 * 	     SYM1 SYM2 {SYM3 SYM4} SYM5
 * 
 * not only associates SYM[12345] with foo, it also registers rules 
 * equivalent to
 * 
 * 	   unless SYM2 suppress dependent SYM3 SYM4
 * 
 * Such subtree declarations may be nested to any depth.
 * 
 * It is perfectly legal for a menu-ID to have no child nodes.  In CML2,
 * this is how you embed text in menus -- by making it the banner of
 * of a symbol with no children.
 */
menu_definition:    	  K_MENU SYMBOL name_or_subtree_list
    	    	    	    {
			    	check_mn_is_menu($2);
			    	mn_set_children($2, $3);
			    }
    	    	    	;

name_or_subtree_list:	  name_or_subtree
    	    	    	    {
			    	$$ = $1;
			    }
    	    	    	| name_or_subtree_list name_or_subtree
    	    	    	    {
				$$ = g_list_concat($1, $2);
			    }
			;
			
name_or_subtree:    	  SYMBOL suffix
			    {
			    	check_mn_is_symbol_or_menu($1);
				check_mn_is_orphan($1);
				set_value_type($1, $2);
				$$ = g_list_append(0, $1);
			    }
    	    	    	| SYMBOL suffix '{' name_or_subtree_list '}'
			    {
			    	if (check_mn_is_symbol($1))
				{
				    check_mn_is_orphan($1);
				    set_value_type($1, $2);
				    handle_subtree($1, $4);
				}
				$$ = g_list_concat(g_list_append(0, $1), $4);
			    }
			;
			
suffix:     	    	  /* Empty suffix (boolean type for symbols) */
    	    	    	    {
			    	$$ = ' ';
			    }
    	    	    	| '?'	    /* declares trit type */
    	    	    	    {
			    	$$ = '?';
			    }
    	    	    	| '!'	    /* declares vital trit type */
    	    	    	    {
			    	$$ = '!';
			    }
    	    	    	| '%'	    /* declares decimal type */
    	    	    	    {
			    	$$ = '%';
			    }
    	    	    	| '@'	    /* declares hexadecimal type */
    	    	    	    {
			    	$$ = '@';
			    }
    	    	    	| '$'	    /* declares string type */
    	    	    	    {
			    	$$ = '$';
			    }
			;

choices_definition:   	  K_CHOICES SYMBOL symbol_list K_DEFAULT SYMBOL
    	    	    	    {
			    	GList *list;
				
    	    	    	    	check_mn_is_menu($2);
			    	$2->flags |= MN_FLAG_IS_RADIO;
				for (list = $3 ; list != 0 ; list = list->next)
				{
				    cml_node *mn = (cml_node *)list->data;
				    check_mn_is_symbol(mn);
				    mn->value_type = A_BOOLEAN;
				}
				mn_set_children($2, $3);
				
				if ($5->parent != $2)
				{
				    yyerror("Default choice `%s' is not present in choices list",
				    	$5->name);
				}
				else
				{
				    /*
				     * Build a simple expression which
				     * just returns the default node.
				     */
			    	    cml_atom a;
				    a.type = A_NODE;
				    a.value.node = $5;
				    $2->expr = expr_new_atom(&a);
				}
			    }
    	    	    	;

/*
 * IV.3.8. Derivations
 * 
 * A derivation binds a symbol to a formula, so the value of that
 * symbol is always the current value of the formula.  Symbols
 * may be evaluated either when a menu containing them is instantiated 
 * or at the time the final configuration file is written.
 * 
 * The compiler performs type inference to deduce the type of a derived
 * symbol.  In particular, derived symbols for which the top-level
 * expression is an arithmetic operator are deduced to be decimal.
 * Derived symbols for which the top level of the expression is a boolean
 * operator are deduced to be bool.  Derived symbols for which the top
 * level of the expression is a trit operator are deduced to be trit.
 * 
 * Derived symbols are never set directly by the user and have no
 * associated prompt string.
 */
derive_definition:  	  K_DERIVE SYMBOL K_FROM expr
    	    	    	    {
			    	/*
				 * Only one "derive" statement per derived
				 * symbol is allowed, so the symbol must
				 * be MN_UNKNOWN at this point.
				 */
			    	if ($2->treetype == MN_UNKNOWN)
			    	    set_mn_treetype($2, MN_DERIVED);
				else
				{
				    if (check_mn_is_derived($2))
				    {
					yyerror("derived symbol `%s' derived twice", $2->name);
					cml_errorl(&$2->location, "is previous definition");
				    }
				    break;
				}
				assert($2->treetype == MN_DERIVED);
				$2->expr = $4;
				$2->value_type = $4->value.type;
			    }
    	    	    	;

/*
 * IV.3.9. Defaults
 * 
 * A default definition sets the value a symbol will have until it is
 * explicitly set by the user's answer to a question.  The right-hand
 * side of a default is not limited to being a constant value; it
 * may be any valid expression.
 * 
 * Defaults may be evaluated either when a menu containing them is
 * instantiated or at the time the final configuration file is written.
 * 
 * If a symbol is not explicitly defaulted, it gets the zero value of its
 * type; n for bools and trits, 0 for decimal and hexadecimal symbols,
 * and the empty string for strings.
 * 
 * The optional range part may be used to set upper and lower bounds
 * for number-valued symbols.
 */
default_definition: 	  K_DEFAULT SYMBOL K_FROM expr
    	    	    	    {
				set_default_expr($2, $4);
			    }
    	    	    	| K_DEFAULT SYMBOL K_FROM expr K_RANGE subrange_list
    	    	    	    {
				set_default_expr($2, $4);
				$2->range = $6;
			    }
    	    	    	| K_DEFAULT SYMBOL K_FROM expr K_ENUM enumdef_list
    	    	    	    {
				set_default_expr($2, $4);
				$2->enumdefs = $6;
			    }
			;
			
subrange_list:	    	  subrange
    	    	    	    {
			    	$$ = range_add(0, $1.begin, $1.end);
			    }
			| subrange_list subrange
    	    	    	    {
			    	$$ = range_add($1, $2.begin, $2.end);
			    }
			;
			
subrange:   	    	  integer
    	    	    	    {
			    	$$.begin = $$.end = $1;
			    }
    	    	    	| integer '-' integer
    	    	    	    {
			    	$$.begin = $1;
				$$.end = $3;
			    }
			;

enumdef_list:  	    	  enumdef
    	    	    	    {
			    	$$ = g_list_append(0, $1);
			    }
			| enumdef_list enumdef
			    {
			    	$$ = g_list_append($1, $2);
			    }
			;
			
enumdef:  	    	  SYMBOL '=' integer
    	    	    	    {
			    	$$ = cml_enumdef_new($1, $3);
			    }

/*
 * IV.3.10.1 Requirements as sanity checks
 * 
 * A requirement definition constrains the value of one or more symbols 
 * by requiring that the given expression be true in any valid configuration.
 * All constraints involving a given configuration symbol are checked
 * each time that symbol is modified.  Every constraint is checked just
 * before the configuration file is written.
 * 
 * [...]
 * 
 * A `prohibit' definition requires that the attached predicate *not* be
 * true.  This is syntactic sugar, added to accomodate the fact that
 * human beings have troouble reasoning about the distribution of
 * negation in complex predicates.
 *
 * IV.3.10.2 Using requirements to force variables
 * 
 * Requirements have a second role.  Certain kinds of requirements can be used to
 * deduce values for variables the user has not yet set; the CML2
 * interpreter does this automatically.
 * 
 * Every time a symbol is changed, the change is tried on each declared
 * constraint.  The constraint is algebraicly simplified by substituting
 * in constant, derived and frozen symbols.  If the simplified constraint
 * forces an expression of the form A == B to be true, and either A is a
 * query symbol and B is a constant or the reverse, then the assignment
 * needed to make A == B true is forced.
 * 
 * Thus, given the rules
 * 
 * derive SPARC from SPARC32 or SPARC64
 * require SPARC implies ISA==n and PCMCIA==n and VT==y and VT_CONSOLE==y
 * 	and BUSMOUSE==y and SUN_MOUSE==y and SERIAL==y and SERIAL_CONSOLE==y
 * 	and SUN_KEYBOARD==y
 * 
 * when either SPARC32 or SPARC64 changes to Y, the nine assignments
 * implied by the right side of the second rule will be performed
 * automatically.  If this kind of requirement is triggered by a guard
 * consisting entirely of frozen or constant symbols, all the assigned
 * symbols become frozen.
 * 
 * If A is a boolean or trit symbol and B simplifies to a boolean or trit
 * constant (or vice-versa), assignments may be similarly forced by other
 * relationals (notably A != B, A< B, A > B, A <= B, and A >= B).  If
 * forcing the relational to be true implies only one possible value for
 * the symbol involved, then that assignment is forced.
 * 
 * Note that whether a relational forces a unique value may depend on
 * whether trits are enabled or not.
 */
requirement_definition:   K_REQUIRE logical explanation
    	    	    	    {
			    	cml_rule *rule = rule_new_require($2);
				rule->location = statement_get_location();
				rule->explanation = $3;
			    	rb_add_rule(rb, rule);
			    }
    	    	    	| K_PROHIBIT logical explanation
    	    	    	    {
			    	cml_rule *rule = rule_new_prohibit($2);
				rule->location = statement_get_location();
				rule->explanation = $3;
			    	rb_add_rule(rb, rule);
			    }
			;


explanation:        	 /* nothing */
    	    	    	    {
			    	$$ = 0;
			    }
    	    	    	| K_EXPLANATION SYMBOL
			    {
			    	check_mn_is_explanation($2);
				$$ = $2;
			    }
			
/*
 * IV.3.11. Start declaration 
 * 
 * The start definition specifies the name of the root menu of the 
 * hierarchy.  One such declaration is required per CML2 ruleset.
 */
start_definition:   	  K_START SYMBOL
    	    	    	    {
			    	if (rb->start != 0)
				{
				    yyerror("rulebase already has a \"start\" statement.");
				    cml_infol(&rb->start_loc, "location of previous \"start\" statement.");
				}
			    	check_mn_is_menu_or_unknown($2);
			    	rb->start = $2;
				rb->start_loc = statement_get_location();
			    }
    	    	    	;

/*
 * IV.3.12. Prefix declaration
 * 
 * A prefix declaration sets a string to be prepended to each symbol name
 * whenever it is written out to a result configuration file.  This
 * prefix is also stripped from symbol names read in in a defconfig file.
 * 
 * Rationale: This was added so the CML2 rule system for the Linux kernel
 * would not have to include the common CONFIG_ prefix. The alternative
 * of wiring that prefix into the code would compromise CML2's potential
 * usefulness for other applications.
 */
prefix_definition:  	  K_PREFIX STRING
    	    	    	    {
			    	rb->prefix = $2;
			    }
    	    	    	;

/*
 * IV.3.13 Banner declaration
 * 
 * A banner definition sets a string to used in the configurator
 * greeting line.  This string should identify the system being configured,
 *
 * Rationale: As for the prefix string.
 *
 * GNB Note: the spec is out of date, the language actually uses
 * a menuid to get i18n of the banner text for free (later).
 */
banner_definition:  	  K_BANNER SYMBOL
    	    	    	    {
			    	rb->banner = $2;
			    }
    	    	    	;


/*
 * IV.3.15 Condition statement
 * 
 * The condition statement ties a CML2 feature flag to a query symbol;
 * that is, the value of the feature flag is the value of the symbol.
 * The initial value of the flag when a rulebase is read in is simply
 * the associated symbol's default.  If there is no symbol associated
 * with the the flag, the flag's value is n.
 * 
 * At present only one flag, named "trits", is supported.  When this
 * flag is n, trit-valued symbols are treated as booleans and may only
 * assume the values y and n.
 * 
 * This flag may affect the front end's presentation of alternatives
 * for modular symbols.  It also affects forcing of ancestor symbols.
 * When the trits flag is on, setting a boolean symbol only forces its
 * trit ancestors to the value m; when trits is off, they are forced to y.
 * See IV.4 for discussion.
 */
condition_declaration:	  K_CONDITION feature_flag K_ON SYMBOL
    	    	    	    {
			    	if (rb->features[$2].location.lineno)
				{
				    yyerror("duplicate \"condition\" statement.");
				    cml_infol(&rb->features[$2].location,
				    	"location of previous statement.");
				}
				rb->features[$2].location = statement_get_location();

				check_mn_is_symbol($4);
				rb->features[$2].tie = $4;
			    }
			| K_CONDITION feature_flag K_ON constant
			    {
			    	if (rb->features[$2].location.lineno)
				{
				    yyerror("duplicate \"condition\" statement.");
				    cml_infol(&rb->features[$2].location,
				    	"location of previous statement.");
				}
				rb->features[$2].location = statement_get_location();

			    	assert($4->type == E_ATOM);
				atom_assign(&rb->features[$2].value, &$4->value);
				expr_destroy($4);
			    }
    	    	    	;

feature_flag:	    	  K_TRITS
    	    	    	    {
			    	$$ = RBF_TRITS;
			    }
    	    	    	|
			  K_NOHELP
			    {
			    	$$ = RBF_NOHELP;
			    }
    	    	    	;

/*
 * IV.3.16 Warndepend declaration
 * 
 * The warndepend declaration takes a list of symbol names.  All
 * dependents of each symbol have their prompts suffixed with the name of
 * the symbol in parentheses to indicate that they are dependent on it.
 * 
 * Rationale: Added to support the EXPERIMENTAL symbol in the Linux
 * lernel configuration.  This declaration is better than tracking 
 * experimental status by hand because it guarantees that subsidiary
 * symbols dependentent on an experimental feature will always be flagged
 * for the user.
 */
warndepend_declaration:   K_WARNDEPEND symbol_list
    	    	    	    {
			    	GList *node = $2;
				while (node != 0)
				{
				    cml_node *mn = (cml_node *)node->data;
				    mn->flags |= MN_FLAG_WARNDEPEND;
				    node = g_list_remove_link(node, node);
				}
			    }
    	    	    	;

/*
 * IV.3.16 Icon declaration
 * 
 * An icon declaration associates graphic data (encoded in RFC2045
 * base64) with the rulebase.  Front ends may use this data as an
 * identification icon.  All front ends are required to accept XPM data here.
 * 
 * The reference front-end implementation uses the image to iconify the
 * configurator when it is minimized while running in X mode.  The
 * reference front-end also accepts GIF data.
 * 
 * Note that parsing BINARYDATA is context-sensitive because (in general)
 * base64 data is indistinguishable from symbols/menuids.  Eric S Raymond
 * said on the linux-kbuild mailing list when a suggestion was made to
 * make parsing context-free by using quote characters:
 *
 * > I thought about this before I wrote the icon implementation.  Noise
 * > characters in language syntaxes really irritate me (my LISP bias shows
 * > here), and I think [quote characters] are noise in this context.
 * > Everybody knows what RFC2045 base-64 data looks like; those solid blocks
 * > of stuff are an unmistakeable clue.  Under these circumstances I'm not
 * > afraid of a little context sensitivity. 
 */
icon_definition:    	  K_ICON BINARYDATA
    	    	    	    {
			    	rb->icon = $2;
			    }
    	    	    	;

/*
 * IV.3.17. Debug
 * 
 * This declaration enables debugging output from the compiler (it has no
 * effect on front-end behavior).  It takes an integer value and uses it
 * to set the current debug level.  It may change or be removed in future
 * releases.
 */
debug_definition:   	  K_DEBUG DECIMAL ;

/*
 * These productions allow a test script to be provided in the
 * CML file, using a series of @something keywords.  This feature
 * is for regression testing the rulebase.
 */
test_script_entry:  	  K_TEST_ASSERT expr
			    {
#if TESTSCRIPT
    	    	    	    	if ($2->value.type != A_BOOLEAN)
				    yyerror("Expression for @assert must be boolean");
				rb_add_test(rb, statement_get_location(), TS_ASSERT, 0, $2);
#endif
			    }
    	    	    	| K_TEST_CLEAR
			    {
#if TESTSCRIPT
				rb_add_test(rb, statement_get_location(), TS_CLEAR, 0, 0);
#endif    
			    }
    	    	    	| K_TEST_COMMIT
			    {
#if TESTSCRIPT
				rb_add_test(rb, statement_get_location(), TS_COMMIT, 0, 0);
#endif    
			    }
    	    	    	| K_TEST_ERROR STRING
			    {
#if TESTSCRIPT
				rb_add_test(rb, statement_get_location(), TS_ERROR, 0,
				    	    	expr_new_atom_v(A_STRING, $2));
#endif    
			    }
    	    	    	| K_TEST_FREEZE
			    {
#if TESTSCRIPT
				rb_add_test(rb, statement_get_location(), TS_FREEZE, 0, 0);
#endif    
			    }
    	    	    	| K_TEST_NOERROR
			    {
#if TESTSCRIPT
				rb_add_test(rb, statement_get_location(), TS_NOERROR, 0, 0);
#endif    
			    }
    	    	    	| K_TEST_PARSETEST
			    {
#if TESTSCRIPT
    	    	    	    	rb->parsetest = TRUE;
#endif    
			    }
    	    	    	| K_TEST_SAVEABLE SYMBOL TRITVAL
			    {
#if TESTSCRIPT
    	    	    	    	check_mn_is_symbol($2);
				rb_add_test(rb, statement_get_location(),
				    TS_SAVEABLE, $2, expr_new_tristate_atom($3));
#endif
			    }
    	    	    	| K_TEST_SET SYMBOL expr
			    {
#if TESTSCRIPT
    	    	    	    	check_mn_is_symbol($2);
				rb_add_test(rb, statement_get_location(), TS_SET, $2, $3);
#endif    
			    }
    	    	    	| K_TEST_SUCCEEDED TRITVAL
			    {
#if TESTSCRIPT
				rb_add_test(rb, statement_get_location(),
				    TS_SUCCEEDED, 0, expr_new_tristate_atom($2));
#endif
			    }
    	    	    	| K_TEST_VISIBLE SYMBOL TRITVAL
			    {
#if TESTSCRIPT
    	    	    	    	check_mn_is_symbol($2);
				rb_add_test(rb, statement_get_location(),
				    TS_VISIBLE, $2, expr_new_tristate_atom($3));
#endif
			    }
			;
			
			

expr:	    	    	  expr '+' expr
    	    	    	    {
			    	$$ = expr_new_composite(E_PLUS, $1, $3);
			    }
    	    	    	| expr '-' expr
    	    	    	    {
			    	$$ = expr_new_composite(E_MINUS, $1, $3);
			    }
			| expr '*' expr
    	    	    	    {
			    	$$ = expr_new_composite(E_TIMES, $1, $3);
			    }
			| ternary
			    {
			    	$$ = $1;
			    }
			;
			
ternary:    	    	  expr '?' expr ':' expr
    	    	    	    {
			    	$$ = expr_new_trinary($1, $3, $5);
			    }
    	    	    	| logical
			;			
			
logical:    	    	  logical K_OR logical
    	    	    	    {
			    	$$ = expr_new_composite(E_OR, $1, $3);
			    }
    	    	    	| logical K_AND logical
    	    	    	    {
			    	$$ = expr_new_composite(E_AND, $1, $3);
			    }
			| logical K_IMPLIES logical
    	    	    	    {
			    	$$ = expr_new_composite(E_IMPLIES, $1, $3);
			    }
			| relational
			;
			
relational: 	    	  term K_EQUALS term
    	    	    	    {
			    	$$ = expr_new_composite(E_EQUALS, $1, $3);
			    }
    	    	    	| term K_NOT_EQUALS term
    	    	    	    {
			    	$$ = expr_new_composite(E_NOT_EQUALS, $1, $3);
			    }
			| term K_LESS_EQUALS term
    	    	    	    {
			    	$$ = expr_new_composite(E_LESS_EQUALS, $1, $3);
			    }
			| term K_GREATER_EQUALS term
    	    	    	    {
			    	$$ = expr_new_composite(E_GREATER_EQUALS, $1, $3);
			    }
			| term '<' term
    	    	    	    {
			    	$$ = expr_new_composite(E_LESS, $1, $3);
			    }
			| term '>' term
    	    	    	    {
			    	$$ = expr_new_composite(E_GREATER, $1, $3);
			    }
			| term
			| K_NOT relational  	    /* TODO prec */
    	    	    	    {
			    	$$ = expr_new_composite(E_NOT, $2, 0);
			    }
			;
			
term:	    	    	  term '|' term     	/* maximum or sum or union value */
    	    	    	    {
			    	$$ = expr_new_composite(E_MAXIMUM, $1, $3);
			    }
    	    	    	| term '&' term     	/* minimum or multiple or intersection value */
    	    	    	    {
			    	$$ = expr_new_composite(E_MINIMUM, $1, $3);
			    }
			| term '$' term     	/* similarity value */
    	    	    	    {
			    	$$ = expr_new_composite(E_SIMILARITY, $1, $3);
			    }
			| atom
			;

atom:	    	    	  SYMBOL
			    {
#if 0				
				if ($1->treetype == MN_UNKNOWN && !($1->flags & MN_FORWARD_WARNING))
				{
				    /*
				     * In CML2 version 0.7.6, ESR decided that
				     * forward-declared symbols were a poor CML
				     * coding practice, hence the warning.
				     */
				    cml_warningl(&$1->location,
				    	"Symbol `%s' is used in expression before it is defined\n",
					$1->name);
				    /* set_mn_treetype($1, MN_DERIVED); */
				    $1->flags |= MN_FORWARD_WARNING;
				}
#endif
				/* check_mn_is_symbol_or_derived($1); */
				$$ = expr_new_symbol($1);
			    }
			| constant
			    {
			    	$$ = $1;
			    }
			| '(' expr ')'
			    {
				$$ = $2;
			    }
			;
			

constant:   	    	  TRITVAL
			    {
			    	$$ = expr_new_tristate_atom($1);
			    }
			| STRING
			    {
			    	$$ = expr_new_atom_v(A_STRING, $1);
			    }
			| DECIMAL
			    {
			    	$$ = expr_new_atom_v(A_DECIMAL, $1);
			    }
			| HEXADECIMAL
			    {
			    	$$ = expr_new_atom_v(A_HEXADECIMAL, $1);
			    }
			;

integer:    	      	  DECIMAL
			    {
			    	$$ = (unsigned long)$1;
			    }
    	    		| HEXADECIMAL
			    {
			    	$$ = (unsigned long)$1;
			    }
	    		;
	    
%%

#include "cml2_lexer.c"

static void
yyerror(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    cml_messagelv(CML_ERROR, &yylocation, fmt, args);
    va_end(args);
}

/*============================================================*/

gboolean
_cml_rulebase_parse_cml2(cml_rulebase *rbi, const char *filename)
{
    gboolean failed = FALSE;

#if DEBUG
    cml2_yy_flex_debug = (debug & DEBUG_LEXER ? 1 : 0);
    cml2_yydebug = (debug & DEBUG_PARSER ? 1 : 0);
#endif

    if (!yylex_push_file(filename))
    	return FALSE;

    statement_init();
    cml_message_count[CML_ERROR] = 0;

    rb = rbi;
    if (yyparse())
    	failed = TRUE;
    rb = 0;

    return !failed;
}

/*============================================================*/
/*END*/
