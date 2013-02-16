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

CVSID("$Id: expr.c,v 1.28 2002/09/01 08:45:35 gnb Exp $");

/*============================================================*/

typedef struct
{
#define LOOP_CHECK_MAX	64
    cml_node *node;
    const char *func;
    int nexprs;
    const cml_expr *exprs[LOOP_CHECK_MAX];
} expr_loop_context_t;

static void
expr_loop_init(expr_loop_context_t *lc, const char *fn)
{
    lc->node = 0;
    lc->nexprs = 0;
    lc->func = fn;
}

/*
 * Pushes an expression into the loop context, returns TRUE if
 * that would create a loop.
 */
static gboolean
expr_loop_push(expr_loop_context_t *lc, const cml_expr *expr)
{
    int i;

    for (i = 0 ; i < lc->nexprs ; i++)
    {
    	if (lc->exprs[i] == expr)
    	{
	    if (lc->node == 0)
		cml_errorl(0,
		    "INTERNAL ERROR: expression loop in %s",
		    lc->func);
	    else
		cml_errorl(&lc->node->location,
		    "INTERNAL ERROR: expression loop expanding \"%s\" in %s",
		    lc->node->name,
		    lc->func);
	    return TRUE;
	}
    }
    assert(lc->nexprs < LOOP_CHECK_MAX);
    lc->exprs[lc->nexprs++] = expr;
    if (lc->node == 0 &&
    	expr->type == E_SYMBOL &&
	expr->symbol->treetype == MN_DERIVED)
    	lc->node = expr->symbol;
    return FALSE;
}

/*
 * Pops the last expression out of the loop context
 */
static void
expr_loop_pop(expr_loop_context_t *lc)
{
    const cml_expr *expr;
    
    assert(lc->nexprs > 0);
    if ((expr = lc->exprs[lc->nexprs-1])->type == E_SYMBOL &&
	expr->symbol->treetype == MN_DERIVED &&
	expr->symbol == lc->node)
    	lc->node = 0;

    lc->nexprs--;
}


/*============================================================*/

cml_expr *
expr_new(void)
{
    cml_expr *expr = g_new(cml_expr, 1);
    if (expr == 0)
    	return 0;
    memset(expr, 0, sizeof(*expr));
    return expr;
}

/* shallow clone */
cml_expr *
expr_copy(const cml_expr *expr)
{
    cml_expr *copy = g_new(cml_expr, 1);
    if (copy == 0)
    	return 0;
    *copy = *expr;
    atom_ctor(&copy->value);
    return copy;
}

/* deep clone */
cml_expr *
expr_deep_copy(const cml_expr *expr)
{
    int i;

    cml_expr *copy = g_new(cml_expr, 1);
    if (copy == 0)
    	return 0;
    *copy = *expr;
    atom_ctor(&copy->value);

    for (i=0 ; i<EXPR_MAX_CHILDREN ; i++)
    {
    	if (copy->children[i] != 0)
	    copy->children[i] = expr_deep_copy(copy->children[i]);
    }

    return copy;
}

/* do a Kronos */
static void
expr_destroy_children(cml_expr *expr)
{
    int i;
    
    for (i=0 ; i<EXPR_MAX_CHILDREN ; i++)
    {
    	if (expr->children[i] != 0)
	{
	    expr_destroy(expr->children[i]);
	    expr->children[i] = 0;
	}
    }
}

static void
expr_dtor(cml_expr *expr)
{
    atom_dtor(&expr->value);
    expr_destroy_children(expr);
}

void
expr_destroy(cml_expr *expr)
{
    expr_dtor(expr);
    g_free(expr);
}

/*============================================================*/

cml_expr *
expr_new_atom(const cml_atom *atom)
{
    cml_expr *expr;
    
    if ((expr = expr_new()) == 0)
    	return 0;
	
    expr->type = E_ATOM;
    expr->value = *atom;
    return expr;
}


cml_expr *
expr_new_atom_v(cml_atom_type type, ...)
{
    cml_expr *expr;
    va_list args;
    
    if ((expr = expr_new()) == 0)
    	return 0;
	
    expr->type = E_ATOM;
    expr->value.type = type;

    va_start(args, type);
    switch (type)
    {
    case A_NONE:
    	break;
    case A_HEXADECIMAL:
    case A_DECIMAL:
    	expr->value.value.integer = va_arg(args, long);
    	break;
    case A_STRING:
    	expr->value.value.string = va_arg(args, char*);
    	break;
    case A_NODE:
    	expr->value.value.node = va_arg(args, cml_node*);
    	break;
    case A_BOOLEAN:
    case A_TRISTATE:
    	expr->value.value.tritval = va_arg(args, cml_tritval);
    	break;
    }
    va_end(args);
    
    return expr;
}

cml_expr *
expr_new_atom_from_string(cml_atom_type type, const char *s)
{
    cml_atom a;

    cml_atom_init(&a);
    a.type = type;
    if (!cml_atom_from_string(&a, s))
    {
	/* special case for CML1: unparseable strings are A_NONE */
	a.type = A_NONE;
	a.value.tritval = 0;
    }

    return expr_new_atom(&a);
}


cml_expr *
expr_new_symbol(cml_node *mn)
{
    cml_expr *expr;
    
    if ((expr = expr_new()) == 0)
    	return 0;
	
    /* TODO: double-check that mn->treetype is SYMBOL or DERIVED */
    expr->type = E_SYMBOL;
    expr->value.type = mn->value_type;
    expr->symbol = mn;
    mn->expr_count++;

    return expr;
}

cml_expr *
expr_new_composite(cml_expr_type type, cml_expr *left, cml_expr *right)
{
    cml_expr *expr;
    
    if ((expr = expr_new()) == 0)
    	return 0;
	
    expr->type = type;
    expr->children[0] = left;
    expr->children[1] = right;
    
    /*
     * Compile-time type propagation.
     */
    switch (expr->type)
    {
    case E_PLUS:
    case E_MINUS:
    case E_TIMES:
    	expr->value.type = A_DECIMAL;
	break;

    /* logical operators */
    case E_OR:
    case E_AND:
    case E_IMPLIES:
    case E_EQUALS:
    case E_NOT_EQUALS:
    case E_LESS:
    case E_LESS_EQUALS:
    case E_GREATER:
    case E_GREATER_EQUALS:
    case E_MDEP:
    case E_NOT:
    	expr->value.type = A_BOOLEAN;
	break;

    /* ternary operators */
    case E_MAXIMUM:
    case E_MINIMUM:
    case E_SIMILARITY:
    	expr->value.type = A_TRISTATE;
	break;

    default:
    	/* TODO: internal error */
    	break;
    }
    
    return expr;
}

cml_expr *
expr_new_trinary(cml_expr *cond, cml_expr *first, cml_expr *second)
{
    cml_expr *expr;
    
    if ((expr = expr_new()) == 0)
    	return 0;
	
    expr->type = E_TRINARY;
    expr->children[0] = cond;
    expr->children[1] = first;
    expr->children[2] = second;
    
    /*
     * Compile-time type propagation.
     */
    expr->value.type = first->value.type;

    return expr;
}

static void
_expr_add_using_rule_recursive2(
    expr_loop_context_t *lc,
    cml_expr *expr,
    cml_rule *rule)
{
    int i;
    
    if (expr_loop_push(lc, expr))
    	return;

    if (expr->type == E_SYMBOL)
    {
	if (expr->symbol->treetype == MN_SYMBOL)
	    _mn_add_using_rule(expr->symbol, rule);
	else if (expr->symbol->treetype == MN_DERIVED)
	    _expr_add_using_rule_recursive2(lc, expr->symbol->expr, rule);
    }
    
    for (i=0 ; i<EXPR_MAX_CHILDREN ; i++)
	if (expr->children[i] != 0)
    	    _expr_add_using_rule_recursive2(lc, expr->children[i], rule);
	    
    expr_loop_pop(lc);
}

void
_expr_add_using_rule_recursive(cml_expr *expr, cml_rule *rule)
{
    expr_loop_context_t lc;
    
    expr_loop_init(&lc, "_expr_add_using_rule_recursive");
    
    _expr_add_using_rule_recursive2(&lc, expr, rule);
    
    assert(lc.nexprs == 0);
}

/*============================================================*/

/* TODO: mark derived symbols as not-to-be-visible */
/* TODO: implement frozen symbols */

/*============================================================*/

static void
promote_to_decimal(cml_atom *ap)
{
    switch (ap->type)
    {
    case A_HEXADECIMAL:
    	ap->type = A_DECIMAL;
	break;
    case A_DECIMAL:
    	return;
    case A_BOOLEAN:
    	ap->type = A_DECIMAL;
    	ap->value.integer = (ap->value.tritval == CML_N ? 0 : 1);
	break;
    case A_TRISTATE:
    	ap->type = A_DECIMAL;
    	ap->value.integer = (ap->value.tritval == CML_N ? 0 : 1);
	break;
    default:
    	{ int promotable_to_decimal = 0;  assert(promotable_to_decimal); }
    }
}

static void
promote_to_tristate(cml_atom *ap)
{
    switch (ap->type)
    {
    case A_BOOLEAN:
    	ap->type = A_TRISTATE;
	/* leave the binary value alone, just change the type */
	break;
    case A_TRISTATE:
	/* leave the binary value alone, just change the type */
	break;
    default:
    	{ int promotable_to_tristate = 0;  assert(promotable_to_tristate); }
    }
}

/*============================================================*/

/*
 *   RIGHT
 * L
 * E
 * F
 * T
 */
 
static const cml_tritval truth_equals[3][3] = {
       /* n    y      m */
/* n */{CML_Y, CML_N, CML_N},
/* y */{CML_N, CML_Y, CML_N},
/* m */{CML_N, CML_N, CML_Y}
};

static const cml_tritval truth_not_equals[3][3] = {
       /* n    y      m */
/* n */{CML_N, CML_Y, CML_Y},
/* y */{CML_Y, CML_N, CML_Y},
/* m */{CML_Y, CML_Y, CML_N}
};

/* for ordering,  y > m > n */

static const cml_tritval truth_less[3][3] = {
       /* n    y      m */
/* n */{CML_N, CML_Y, CML_Y},
/* y */{CML_N, CML_N, CML_N},
/* m */{CML_N, CML_Y, CML_N}
};

static const cml_tritval truth_less_equals[3][3] = {
       /* n    y      m */
/* n */{CML_Y, CML_Y, CML_Y},
/* y */{CML_N, CML_Y, CML_N},
/* m */{CML_N, CML_Y, CML_Y}
};

static const cml_tritval truth_greater[3][3] = {
       /* n    y      m */
/* n */{CML_N, CML_N, CML_N},
/* y */{CML_Y, CML_N, CML_Y},
/* m */{CML_Y, CML_N, CML_N}
};

static const cml_tritval truth_greater_equals[3][3] = {
       /* n    y      m */
/* n */{CML_Y, CML_N, CML_N},
/* y */{CML_Y, CML_Y, CML_Y},
/* m */{CML_Y, CML_N, CML_Y}
};

static const cml_tritval truth_mdep[3][3] = {
       /* n    y      m */
/* n */{CML_Y, CML_Y, CML_Y},
/* y */{CML_N, CML_Y, CML_Y},
/* m */{CML_N, CML_Y, CML_Y}
};

static const cml_tritval truth_maximum[3][3] = {
       /* n    y      m */
/* n */{CML_N, CML_Y, CML_M},
/* y */{CML_Y, CML_Y, CML_Y},
/* m */{CML_M, CML_Y, CML_M}
};

static const cml_tritval truth_minimum[3][3] = {
       /* n    y      m */
/* n */{CML_N, CML_N, CML_N},
/* y */{CML_N, CML_Y, CML_M},
/* m */{CML_N, CML_M, CML_M}
};

static const cml_tritval truth_similarity[3][3] = {
       /* n    y      m */
/* n */{CML_N, CML_N, CML_N},
/* y */{CML_N, CML_Y, CML_N},
/* m */{CML_N, CML_N, CML_M}
};

static const cml_tritval truth_or[3][3] = {
       /* n    y      m */
/* n */{CML_N, CML_Y,    -1},
/* y */{CML_Y, CML_Y,    -1},
/* m */{   -1,    -1,    -1}
};

static const cml_tritval truth_and[3][3] = {
       /* n    y      m */
/* n */{CML_N, CML_N,    -1},
/* y */{CML_N, CML_Y,    -1},
/* m */{   -1,    -1,    -1}
};

static const cml_tritval truth_implies[3][3] = {
       /* n    y      m */
/* n */{CML_Y, CML_Y,    -1},
/* y */{CML_N, CML_Y,    -1},
/* m */{   -1,    -1,    -1}
};

static void
expr_evaluate2(expr_loop_context_t *lc, const cml_expr *expr, cml_atom *val)
{
    cml_atom left, right;

    if (expr_loop_push(lc, expr))
    {
    	cml_atom_init(val);
    	return;
    }
    
    if (expr->type == E_TRINARY)
    {
    	/* different from all the others... */
	cml_atom cond;
	
	cml_atom_init(&cond);
    	expr_evaluate2(lc, expr->children[0], &cond);
    	assert(cond.type == A_BOOLEAN);
	expr_evaluate2(lc, expr->children[cond.value.tritval ? 1 : 2], val);
	expr_loop_pop(lc);
	return;
    }
    
    /* recursively evaluate child nodes if any */
    cml_atom_init(&left);
    if (expr->children[0] != 0)
    	expr_evaluate2(lc, expr->children[0], &left);
    
    cml_atom_init(&right);
    if (expr->children[1] != 0)
    	expr_evaluate2(lc, expr->children[1], &right);

    /* now handle this node */
    switch (expr->type)
    {
    case E_NONE:
    	break;
	
    /* arithmetic operators */
    case E_PLUS:
    	promote_to_decimal(&left);
    	promote_to_decimal(&right);
	val->type = A_DECIMAL;
	val->value.integer = left.value.integer + right.value.integer;
    	break;
    case E_MINUS:
    	promote_to_decimal(&left);
    	promote_to_decimal(&right);
	val->type = A_DECIMAL;
	val->value.integer = left.value.integer - right.value.integer;
    	break;
    case E_TIMES:
    	promote_to_decimal(&left);
    	promote_to_decimal(&right);
	val->type = A_DECIMAL;
	val->value.integer = left.value.integer * right.value.integer;
    	break;

    /* logical operators */
    case E_OR:
    	assert(left.type == A_BOOLEAN);     /* TODO: smarter check */
    	assert(right.type == A_BOOLEAN);     /* TODO: smarter check */
	val->type = A_BOOLEAN;
	val->value.tritval = left.value.tritval || right.value.tritval;
    	break;
    case E_AND:
    	assert(left.type == A_BOOLEAN);     /* TODO: smarter check */
    	assert(right.type == A_BOOLEAN);     /* TODO: smarter check */
	val->type = A_BOOLEAN;
	val->value.tritval = left.value.tritval && right.value.tritval;
    	break;
    case E_IMPLIES:
    	assert(left.type == A_BOOLEAN);     /* TODO: smarter check */
    	assert(right.type == A_BOOLEAN);     /* TODO: smarter check */
	val->type = A_BOOLEAN;
	val->value.tritval = !left.value.tritval || right.value.tritval;
    	break;

    /* relational operators */
    case E_EQUALS:
	val->type = A_BOOLEAN;
	val->value.tritval = (_atom_compare(&left, &right) == 0);
    	break;
    case E_NOT_EQUALS:
	val->type = A_BOOLEAN;
	val->value.tritval = (_atom_compare(&left, &right) != 0);
    	break;
    case E_LESS:
	val->type = A_BOOLEAN;
	val->value.tritval = (_atom_compare(&left, &right) < 0);
    	break;
    case E_LESS_EQUALS:
	val->type = A_BOOLEAN;
	val->value.tritval = (_atom_compare(&left, &right) <= 0);
    	break;
    case E_GREATER:
	val->type = A_BOOLEAN;
	val->value.tritval = (_atom_compare(&left, &right) > 0);
    	break;
    case E_GREATER_EQUALS:
	val->type = A_BOOLEAN;
	val->value.tritval = (_atom_compare(&left, &right) >= 0);
    	break;
    case E_MDEP:
	val->type = A_BOOLEAN;
    	if (left.type == A_NONE || right.type == A_NONE)
	{
	    /* allow MDEP comparison to A_NONE for CML1 compatibility */
	    val->value.tritval = (_atom_compare(&left, &right) <= 0);
	}
	else
	{
    	    promote_to_tristate(&left); 	    /* TODO: smarter check */
    	    promote_to_tristate(&right); 	    /* TODO: smarter check */
	    val->value.tritval = truth_mdep[left.value.tritval][right.value.tritval];
	}
    	break;
    case E_NOT:
    	assert(left.type == A_BOOLEAN);     /* TODO: smarter check */
    	assert(right.type == A_NONE);     /* TODO: smarter check */
	val->type = A_BOOLEAN;
	val->value.tritval = !left.value.tritval;
    	break;

    /* tristate operators */
    case E_MAXIMUM:
    	promote_to_tristate(&left); 	    /* TODO: smarter check */
    	promote_to_tristate(&right); 	    /* TODO: smarter check */
	val->type = A_TRISTATE;
	val->value.tritval = truth_maximum[left.value.tritval][right.value.tritval];
    	break;
    case E_MINIMUM:
    	promote_to_tristate(&left); 	    /* TODO: smarter check */
    	promote_to_tristate(&right); 	    /* TODO: smarter check */
	val->type = A_TRISTATE;
	val->value.tritval = truth_minimum[left.value.tritval][right.value.tritval];
    	break;
    case E_SIMILARITY:
    	promote_to_tristate(&left); 	    /* TODO: smarter check */
    	promote_to_tristate(&right); 	    /* TODO: smarter check */
	val->type = A_TRISTATE;
	val->value.tritval = truth_similarity[left.value.tritval][right.value.tritval];
    	break;

    /* atom */
    case E_ATOM:
    	*val = expr->value;
	break;
	
    case E_SYMBOL:
    	if (expr->symbol->treetype == MN_DERIVED)
	{
	    cml_node *mn = expr->symbol;
	    assert(mn->expr != 0);
	    cml_atom_init(&mn->value);
	    expr_evaluate2(lc, mn->expr, &mn->value);
	    *val = mn->value;
	}
	else
    	{
	    const cml_atom *v = cml_node_get_value(expr->symbol);
	    if (v == 0)
	    	cml_atom_init(val);
	    else
		*val = *v;
	}
    	break;
	
    case E_TRINARY:
    	/* suck this gcc */
	break;
    }
    expr_loop_pop(lc);
}

void
expr_evaluate(const cml_expr *expr, cml_atom *val)
{
    expr_loop_context_t lc;
    
    expr_loop_init(&lc, "expr_evaluate");
    expr_evaluate2(&lc, expr, val);
    assert(lc.nexprs == 0);
}

/*============================================================*/

const char *
_expr_get_type_as_string(const cml_expr *expr)
{
    switch (expr->type)
    {
    case E_NONE: return "<none>";
	
    /* arithmetic operators */
    case E_PLUS: return "+";
    case E_MINUS: return "-";
    case E_TIMES: return "*";

    /* logical operators */
    case E_OR: return "or";
    case E_AND: return "and";
    case E_IMPLIES: return "implies";

    /* relational operators */
    case E_EQUALS: return "==";
    case E_NOT_EQUALS: return "!=";
    case E_LESS: return "<";
    case E_LESS_EQUALS: return "<=";
    case E_GREATER: return ">";
    case E_GREATER_EQUALS: return ">=";
    case E_MDEP: return "mdep";
    case E_NOT: return "not";

    /* tristate operators */
    case E_MAXIMUM: return "|";
    case E_MINIMUM: return "&";
    case E_SIMILARITY: return "$";

    /* trinary operator */
    case E_TRINARY: return "?:";

    /* atom */
    case E_ATOM: return "atom";
    case E_SYMBOL: return "symbol";
    }
    return 0;
}

/*============================================================*/

cml_atom_type
expr_get_value_type(const cml_expr *expr)
{
    if (expr->type == E_SYMBOL)
    	return expr->symbol->value_type;
    else
    	return expr->value.type;
}

/*============================================================*/

void
_expr_add_dependant(
    const cml_expr *expr,
    cml_node *dependant)
{
    int i;
    
    if (expr->type == E_SYMBOL && expr->symbol != 0)
    	mn_add_dependant(expr->symbol, dependant);

    /* visit children */
    for (i=0 ; i<EXPR_MAX_CHILDREN ; i++)
	if (expr->children[i] != 0)
    	    _expr_add_dependant(expr->children[i], dependant);
}

/*============================================================*/
/*
 * Try to force bindings to make the expression equal
 * the target value.
 */
gboolean
expr_is_constant(cml_expr *expr)
{
    if (expr->type == E_ATOM)
    	return TRUE;
	
    if (expr->type == E_SYMBOL && (expr->symbol->flags & MN_CONSTANT))
    {
    	expr_evaluate(expr, &expr->value);
    	return TRUE;
    }
    
    return FALSE;
}
#define is_atom(e) \
    expr_is_constant(e)


#define is_boolean(e) \
    ((e)->type == E_ATOM && \
     (e)->value.type == A_BOOLEAN)
     
#define is_tristate(e) \
    ((e)->type == E_ATOM && \
     (e)->value.type == A_TRISTATE)
     
#define is_logical(e) \
    ((e)->value.type == A_BOOLEAN || (e)->value.type == A_TRISTATE)

#define is_boolean_val(e, v) \
    ((e)->type == E_ATOM && \
     (e)->value.type == A_BOOLEAN && \
     (e)->value.value.tritval == (v))
     
#define is_tristate_val(e, v) \
    ((e)->type == E_ATOM && \
     (e)->value.type == A_TRISTATE && \
     (e)->value.value.tritval == (v))
     
#define is_logical_val(e, v) \
    ((e)->type == E_ATOM && \
     is_logical(e) && \
     (e)->value.value.tritval == (v))
     
     
static int
expr_solve_tristate(
    const cml_expr *expr,
    const cml_atom *target,
    const cml_tritval truth[3][3],
    cml_node *source)
{
    int i, j;
    int starti = CML_N, endi = CML_M, startj = CML_N, endj = CML_M;
    int soli = 0, solj = 0, nsol = 0;
    cml_atom a;
    gboolean trits;
    
    /*
     * Search the truth table for solutions.
     */
    if (is_atom(expr->children[0]))
    	starti = endi = expr->children[0]->value.value.tritval;
    else if (is_atom(expr->children[1]))
    	startj = endj = expr->children[1]->value.value.tritval;
	
    trits = TRUE;
    /* TODO: implement condition "trits" here */
    
    
    for (i=starti ; i<=endi ; i++)
    {
    	if (i == CML_M && (!trits || expr->children[0]->value.type == A_BOOLEAN))
	    continue;
    	for (j=startj ; j<=endj ; j++)
	{
    	    if (j == CML_M && (!trits || expr->children[1]->value.type == A_BOOLEAN))
		continue;
	    if (truth[i][j] == target->value.tritval)
	    {
	    	soli = i;
	    	solj = j;
		nsol++;
	    }
	}
    }
    
    /* punt if there is more than one solution */
    if (nsol != 1)
    	return nsol;
	
    a.type = (truth[CML_M][CML_M] == -1 ? A_BOOLEAN : A_TRISTATE);

    a.value.tritval = soli;
    nsol = expr_solve(expr->children[0], &a, source);
    if (nsol != 1)
    	return nsol;
	
    a.value.tritval = solj;
    nsol = expr_solve(expr->children[1], &a, source);
    if (nsol != 1)
    	return nsol;
	
    return 1;
}

 
int
expr_solve(
    const cml_expr *expr,
    const cml_atom *target,
    cml_node *source)
{
    cml_atom a;
    
    switch (expr->type)
    {
    case E_NONE:
    case E_PLUS:
    case E_MINUS:
    case E_TIMES:
    	return 0;

    /* logical operators */
    case E_OR:
	return expr_solve_tristate(expr, target, truth_or, source);
	
    case E_AND:
	return expr_solve_tristate(expr, target, truth_and, source);
	
    case E_IMPLIES:
	return expr_solve_tristate(expr, target, truth_implies, source);

    /* relational operators */
    case E_EQUALS:
    	assert(target->type == A_BOOLEAN);
	if (target->value.tritval == CML_Y)
	{
	    /* force equality */
    	    if (is_atom(expr->children[0]))
		return expr_solve(expr->children[1], &expr->children[0]->value, source);
	    else if (is_atom(expr->children[1]))
		return expr_solve(expr->children[0], &expr->children[1]->value, source);
    	}
	else
	{
	    /* force inquality -- only get a single solution if boolean */
	    /* TODO: or if tristate and modules disabled */
	    a.type = A_BOOLEAN;
    	    if (is_boolean(expr->children[0]))
	    {
	    	a.value.tritval = !expr->children[0]->value.value.tritval;
		return expr_solve(expr->children[1], &a, source);
	    }
	    else if (is_boolean(expr->children[1]))
	    {
	    	a.value.tritval = !expr->children[1]->value.value.tritval;
		return expr_solve(expr->children[0], &a, source);
	    }
	}
	return 2;   /* more than 1 solution */
    
    case E_NOT_EQUALS:
    	assert(target->type == A_BOOLEAN);
	if (is_logical(expr->children[0]) &&
	    is_logical(expr->children[1]))
	    return expr_solve_tristate(expr, target, truth_not_equals, source);
	else
	    return 2;	/* TODO */
    
    case E_LESS:
    	assert(target->type == A_BOOLEAN);
	if (is_logical(expr->children[0]) &&
	    is_logical(expr->children[1]))
	    return expr_solve_tristate(expr, target, truth_less, source);
	else
	    return 2;	/* TODO */
	
    /* TODO: use truthtable */
    case E_LESS_EQUALS:
    	assert(target->type == A_BOOLEAN);
	if (is_logical(expr->children[0]) &&
	    is_logical(expr->children[1]))
	    return expr_solve_tristate(expr, target, truth_less_equals, source);
	else
	    return 2;	/* TODO */
	
    case E_GREATER:
    	assert(target->type == A_BOOLEAN);
	if (is_logical(expr->children[0]) &&
	    is_logical(expr->children[1]))
	    return expr_solve_tristate(expr, target, truth_greater, source);
	else
	    return 2;	/* TODO */
    
    case E_GREATER_EQUALS:
    	assert(target->type == A_BOOLEAN);
	if (is_logical(expr->children[0]) &&
	    is_logical(expr->children[1]))
	    return expr_solve_tristate(expr, target, truth_greater_equals, source);
	else
	    return 2;	/* TODO */

    case E_MDEP:
    	assert(target->type == A_BOOLEAN);
	if (is_logical(expr->children[0]) &&
	    is_logical(expr->children[1]))
	    return expr_solve_tristate(expr, target, truth_mdep, source);
	else
	    return 2;	/* TODO */

    case E_NOT:
    	assert(expr->type != E_ATOM);
    	assert(target->type == A_BOOLEAN);
	a.type = target->type;
	a.value.tritval = !target->value.tritval;
	return expr_solve(expr->children[0], &a, source);

    /* ternary operators */
    case E_MAXIMUM:
	return expr_solve_tristate(expr, target, truth_maximum, source);
	
    case E_MINIMUM:
	return expr_solve_tristate(expr, target, truth_minimum, source);
	
    case E_SIMILARITY:
	return expr_solve_tristate(expr, target, truth_similarity, source);
	
    
    /* trinary ?: operator */
    case E_TRINARY:
    	/* TODO */
    	return 0;

    /* atom */
    case E_ATOM:
    	return (_atom_compare(target, &expr->value) == 0 ? 1 : 0);
	
    case E_SYMBOL:
	if (_atom_compare(cml_node_get_value(expr->symbol), target))
	    return (mn_set_value(expr->symbol, target, source) ? 1 : 0);
	return 1;
    }
    
    return 0;
}

/*============================================================*/

/*
 * Create a new expression which is a simplified version of
 * the old expression, with constants and frozen/chilled
 * variables factored out.
 * TODO: mark derivations which only depend on frozen, as frozen
 */
 

static cml_expr *
expr_simplify_to_atom(cml_expr *expr)
{
    /* evaluate the operation on the 0 to 2 children */
    atom_dtor(&expr->value);
    expr_evaluate(expr, &expr->value);
    
    /* become an atomic node */
    expr->type = E_ATOM;
    expr_destroy_children(expr);
    
    return expr;
}

static cml_expr *
expr_simplify_to_logical_atom(cml_expr *expr, cml_tritval val)
{
    /* set the value */
    assert(is_logical(expr));
    expr->value.value.tritval = val;
    
    /* become an atomic node */
    expr->type = E_ATOM;
    expr_destroy_children(expr);
    
    return expr;
}

static cml_expr *
expr_simplify_to_child(cml_expr *expr, int child)
{
    cml_expr *tmp;
    
    /* first detach the favoured child from its parent */
    tmp = expr->children[child];
    expr->children[child] = 0;

    /* destroy the other children and any atom value */
    expr_dtor(expr);
    
    /* suck all the bits out of the child and destroy the lifeless husk */
    *expr = *tmp;
    g_free(tmp);
    
    return expr;
}


static cml_expr *
expr_simplify_to_logical_not_child(cml_expr *expr, int child)
{
    cml_expr *tmp;
    
    /* first detach the favoured child from its parent */
    tmp = expr->children[child];
    expr->children[child] = 0;

    /* destroy the other children and any atom value */
    expr_dtor(expr);
    
    /* become an E_NOT node */
    memset(expr, 0, sizeof(*expr));
    expr->type = E_NOT;
    expr->value.type = A_BOOLEAN;
    expr->children[0] = tmp;
    
    return expr;
}


static cml_expr *
expr_simplify_tristate(cml_expr *expr, const cml_tritval truth[3][3], gboolean istrit)
{
    int i;
    int nsame = 0, nopp = 0, counts[3];
    int non_atomic_child = 0;
    int maxval;
    static const gboolean is_opposite[3][3] = 
{
       /* n    y      m */
/* n */{FALSE,  TRUE, FALSE},
/* y */{ TRUE, FALSE, FALSE},
/* m */{FALSE, FALSE, FALSE}
};

#if 0
    /* TODO: implement condition "trits" here */
    if (!rulebase trits condition)
    	istrit = FALSE;
#endif

    memset(counts, 0, sizeof(counts));
    
    /*
     * Detect whether children are atomic
     */
#define LHS_ATOMIC  1
#define RHS_ATOMIC  2
    switch ((is_atom(expr->children[0]) ? LHS_ATOMIC : 0) |
    	    (is_atom(expr->children[1]) ? RHS_ATOMIC : 0))
    {
    case 0:
	/* no atomic children at all: not simplifiable */
    	return expr;
	
    case LHS_ATOMIC:
    	/* lhs atomic, rhs nonatomic: search truth table */
    	non_atomic_child = 1;
    	
    	for (i = CML_N ; i <= CML_M ; i++)
	{
	    cml_tritval res;
	    
	    if (i == CML_M && !istrit)
	    	continue;
		
	    res = truth[expr->children[0]->value.value.tritval][i];
	    assert(res != -1);
	    counts[res]++;
	    if (res == i)
	    	nsame++;
	    if (is_opposite[res][i])
	    	nopp++;
	}
    	break;
	
    case RHS_ATOMIC:
    	/* lhs nonatomic, rhs atomic: search truth table */
    	non_atomic_child = 0;
    	
    	for (i = CML_N ; i <= CML_M ; i++)
	{
	    cml_tritval res;
	    
	    if (i == CML_M && !istrit)
	    	continue;
		
	    res = truth[i][expr->children[1]->value.value.tritval];
	    assert(res != -1);
	    counts[res]++;
	    if (i == res)
	    	nsame++;
	    if (is_opposite[i][res])
	    	nopp++;
	}
    	break;
	
    case LHS_ATOMIC|RHS_ATOMIC:
	/* all children are atoms: just evaluate */
    	return expr_simplify_to_atom(expr);
    }
#undef LHS_ATOMIC
#undef RHS_ATOMIC
    
    assert(is_logical(expr->children[!non_atomic_child]));

    maxval = (istrit ? 3 : 2);
    assert(counts[CML_N] + counts[CML_Y] + counts[CML_M] == maxval);
	    
    for (i = CML_N ; i <= CML_M ; i++)
	if (counts[i] == maxval)
    	    return expr_simplify_to_logical_atom(expr, i);
    if (nsame == maxval)
    	return expr_simplify_to_child(expr, non_atomic_child);
    if (nopp == maxval)
    	return expr_simplify_to_logical_not_child(expr, non_atomic_child);
    return expr;    /* expression too complex to simplify even with an atomic child */
}


static cml_expr *
expr_simplify2(expr_loop_context_t *lc, cml_expr *expr)
{
    cml_expr *r;
    int i;
    int unsimplified = 0;

    if (expr_loop_push(lc, expr))
	return expr_new_atom_v(A_NONE);

    r = expr_copy(expr);
    
    /* 
     * First, attempt to simplify children.
     */
    for (i=0 ; i<EXPR_MAX_CHILDREN ; i++)
    {
    	if (expr->children[i] != 0)
	{
	    r->children[i] = expr_simplify2(lc, expr->children[i]);
	    if (!is_atom(r->children[i]))
	    	unsimplified++;
	}
    }

    /*
     * Check for potentially simplifiable.
     */
    switch (expr->type)
    {
    case E_OR:
	r = expr_simplify_tristate(r, truth_or, /*istrit*/FALSE);
	expr_loop_pop(lc);
	return r;
    case E_AND:
	r = expr_simplify_tristate(r, truth_and, /*istrit*/FALSE);
	expr_loop_pop(lc);
	return r;
    case E_IMPLIES:
	r = expr_simplify_tristate(r, truth_implies, /*istrit*/FALSE);
	expr_loop_pop(lc);
	return r;

    case E_EQUALS:
	if (is_logical(r->children[0]) &&
	    is_logical(r->children[1]))
	{
	    r = expr_simplify_tristate(r, truth_equals,
	    	    /*istrit*/r->children[0]->value.type == A_TRISTATE ||
		    	      r->children[1]->value.type == A_TRISTATE);
	    expr_loop_pop(lc);
	    return r;
    	}
	break;
    case E_NOT_EQUALS:
	if (is_logical(r->children[0]) &&
	    is_logical(r->children[1]))
	{
	    r = expr_simplify_tristate(r, truth_not_equals, /*istrit*/TRUE);
	    expr_loop_pop(lc);
	    return r;
	}
	break;
    case E_LESS:
	if (is_logical(r->children[0]) &&
	    is_logical(r->children[1]))
	{
	    r = expr_simplify_tristate(r, truth_less, /*istrit*/TRUE);
	    expr_loop_pop(lc);
	    return r;
	}
	break;
    case E_LESS_EQUALS:
	if (is_logical(r->children[0]) &&
	    is_logical(r->children[1]))
	{
	    r = expr_simplify_tristate(r, truth_less_equals, /*istrit*/TRUE);
	    expr_loop_pop(lc);
	    return r;
	}
	break;
    case E_GREATER:
	if (is_logical(r->children[0]) &&
	    is_logical(r->children[1]))
	{
	    r = expr_simplify_tristate(r, truth_greater, /*istrit*/TRUE);
	    expr_loop_pop(lc);
	    return r;
	}
	break;
    case E_GREATER_EQUALS:
	if (is_logical(r->children[0]) &&
	    is_logical(r->children[1]))
	{
	    r = expr_simplify_tristate(r, truth_greater_equals, /*istrit*/TRUE);
	    expr_loop_pop(lc);
	    return r;
	}
	break;
    case E_MDEP:
	if (is_logical(r->children[0]) &&
	    is_logical(r->children[1]))
	{
	    r = expr_simplify_tristate(r, truth_mdep, /*istrit*/TRUE);
	    expr_loop_pop(lc);
	    return r;
	}
	break;

    case E_MAXIMUM:
	r = expr_simplify_tristate(r, truth_maximum, /*istrit*/TRUE);
	expr_loop_pop(lc);
	return r;
    case E_MINIMUM:
	r = expr_simplify_tristate(r, truth_minimum, /*istrit*/TRUE);
	expr_loop_pop(lc);
	return r;
    case E_SIMILARITY:
	r = expr_simplify_tristate(r, truth_similarity, /*istrit*/TRUE);
	expr_loop_pop(lc);
	return r;
	
    case E_SYMBOL:
    	switch (expr->symbol->treetype)
	{
	case MN_MENU:
	    assert(cml_node_is_radio(expr->symbol));
	    /* fall through */
	case MN_SYMBOL:
    	    if (!cml_node_is_frozen(expr->symbol) && !mn_is_chilled(expr->symbol))
	    	unsimplified++;
	    break;
	case MN_DERIVED:
	    expr_destroy(r);
	    r = expr_simplify2(lc, expr->symbol->expr);
	    expr_loop_pop(lc);
	    return r;
	default:
	    break;
	}
	break;
	
    case E_TRINARY:
    	/* TODO: simplify when children[0] is an integral type */
	if (unsimplified)
	{
    	    if (is_logical_val(r->children[0], CML_Y))
	    {
		r = expr_simplify_to_child(r, 1);
		expr_loop_pop(lc);
		return r;
	    }
	    else if (is_logical_val(r->children[0], CML_N))
	    {
	    	r = expr_simplify_to_child(r, 2);
		expr_loop_pop(lc);
		return r;
	    }
	}
	break;
	
    case E_ATOM:
    	r = r;   	/* already atomic, just return it */
	expr_loop_pop(lc);
	return r;
	
    default:
    	break;
    }
    
    if (unsimplified > 0)
    {
    	/*
	 * `expr' is a composite, trinary, or symbol which depends on one
	 * or more unfrozen and unchilled symbols.  So return a duplicate.
	 */
	expr_loop_pop(lc);
	return r;
    }
    
    /*
     * All subexpressions simplify to atoms, so just evaluate.
     */
    r = expr_simplify_to_atom(r);
    
    expr_loop_pop(lc);
    return r;
}


cml_expr *
expr_simplify(cml_expr *expr)
{
    expr_loop_context_t lc;
    
    expr_loop_init(&lc, "expr_simplify");
    expr = expr_simplify2(&lc, expr);
    assert(lc.nexprs == 0);
    return expr;
}

/*============================================================*/
/*
 * Return a new string (which needs to be g_free()d)
 * describing the expression in a relatively human-
 * readable fashion.
 */

#define needs_bracket(e) \
    ((e)->type != E_ATOM && (e)->type != E_SYMBOL && (e)->type != E_NOT)
#define brackets(i) \
    (needs_bracket(expr->children[(i)]) ? "(" : ""), \
    kid[(i)], \
    (needs_bracket(expr->children[(i)]) ? ")" : "")
 
char *
expr_as_string(const cml_expr *expr)
{
    int i;
    char *s, *kid[EXPR_MAX_CHILDREN];
    
    if (expr == 0)
    	return g_strdup("");

    for (i=0 ; i<EXPR_MAX_CHILDREN ; i++)
    	if (expr->children[i] == 0)
	    kid[i] = 0;
	else
	    kid[i] = expr_as_string(expr->children[i]);
	    
	    
    switch (expr->type)
    {
    case E_NONE:
    	s = g_strdup("???");
	break;

    case E_NOT:
    	s = g_strdup_printf("!%s%s%s", brackets(0));
	break;

    case E_TRINARY:
    	s = g_strdup_printf("%s%s%s ? %s%s%s : %s%s%s", brackets(0), brackets(1), brackets(2));
	break;

    case E_ATOM:
    	if (expr->value.type == A_NONE)
	    s = g_strdup("null");
	else
    	    s = cml_atom_value_as_string(&expr->value);
	break;
	
    case E_SYMBOL:
    	s = g_strdup(expr->symbol->name);
	break;
	
    default:
    	s = g_strdup_printf("%s%s%s %s %s%s%s",
	    	    brackets(0),
		    _expr_get_type_as_string(expr),
		    brackets(1));
	break;
    }
    
    for (i=0 ; i<EXPR_MAX_CHILDREN ; i++)
    	if (kid[i] != 0)
	    g_free(kid[i]);
	        
    return s;
}

/*============================================================*/

/*
 * Support for efficiently building up expressions by merging
 * in a new expression in AND and OR modes where one or more of
 * the old and new expressions may be NULL or a boolean constant.
 * Attempts to build minimal expressions for runtime efficiency
 * and debugging clarity.
 */

enum _expr_boolconst
{
    EB_CONST_N,
    EB_CONST_Y,
    EB_NONCONST
};

enum _expr_side
{
    ES_LHS,
    ES_RHS,
    ES_LHS_RHS
};

static enum _expr_boolconst
_expr_bool_const(const cml_expr *expr)
{
    if (expr == 0)
    	return EB_CONST_Y;
    if (expr->type == E_ATOM && expr->value.type == A_BOOLEAN)
	return (expr->value.value.tritval == CML_N ? EB_CONST_N : EB_CONST_Y);
    return EB_NONCONST;
}

static void
_expr_merge_boolean(
    cml_expr **ep,
    const cml_expr *newe,
    gboolean first,
    const enum _expr_side table[3][3],
    cml_expr_type etype)
{
    enum _expr_side side;

    /*
     * When first==TRUE, we make no interpretation of the case *ep==0,
     * because it's just uninitialised.
     */
    if (first)
    {
    	assert(*ep == 0);
	side = ES_RHS;
    }
    else
    {
	enum _expr_boolconst lval = _expr_bool_const(*ep);
	enum _expr_boolconst rval = _expr_bool_const(newe);
	side = table[lval][rval];
    }
    
    switch (side)
    {
    case ES_LHS:
	/* do nothing */
	break;
    case ES_RHS:
	if ((*ep) != 0)
	    expr_destroy(*ep);
	*ep = (newe == 0 ? 0 : expr_deep_copy(newe));
	break;
    case ES_LHS_RHS:
	(*ep) = expr_new_composite(etype, (*ep), expr_deep_copy(newe));
	break;
    }
}

static const enum _expr_side _expr_or_merge_table[3][3] = 
{         /* RHS= CONST_N     CONST_Y    NONCONST */
/*L= CONST_N*/{    ES_LHS,     ES_RHS,     ES_RHS},
/*H= CONST_Y*/{    ES_LHS,     ES_LHS,     ES_LHS},
/*S=NONCONST*/{    ES_LHS,     ES_RHS, ES_LHS_RHS}
};

void
expr_merge_boolean_or(cml_expr **ep, const cml_expr *newe, gboolean first)
{
    _expr_merge_boolean(ep, newe, first, _expr_or_merge_table, E_OR);
}
    
static const enum _expr_side _expr_and_merge_table[3][3] = 
{         /* RHS= CONST_N     CONST_Y    NONCONST */
/*L= CONST_N*/{    ES_LHS,     ES_LHS,     ES_LHS},
/*H= CONST_Y*/{    ES_RHS,     ES_LHS,     ES_RHS},
/*S=NONCONST*/{    ES_RHS,     ES_LHS, ES_LHS_RHS}
};

void
expr_merge_boolean_and(cml_expr **ep, const cml_expr *newe, gboolean first)
{
    _expr_merge_boolean(ep, newe, first, _expr_and_merge_table, E_AND);
}

/*============================================================*/

gboolean
expr_contains_symbol(const cml_expr *expr, const cml_node *mn)
{
    int i;

    for (i=0 ; i<EXPR_MAX_CHILDREN ; i++)
    {
    	if (expr->children[i] != 0 && expr_contains_symbol(expr->children[i], mn))
	    return TRUE;
    }

    if (expr->type == E_SYMBOL)
    {
	if (expr->symbol == mn)
    	    return TRUE;
	if (expr->symbol->treetype == MN_DERIVED &&
	    expr->symbol->expr != 0 &&
	    expr_contains_symbol(expr->symbol->expr, mn))
	    return TRUE;
    }

    return FALSE;
}

/*============================================================*/
/*END*/
