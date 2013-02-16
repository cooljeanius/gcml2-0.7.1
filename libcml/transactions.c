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
 
/*
 * Low-level routines to manipulate bindings lists.  This data
 * structure is mostly translated directly from ESR's Python code into
 * C, without optimisation.  The primary difference is that instead
 * of two lists `newbindings' and `oldbindings' there is a single
 * list called `transactions' and the new ones are marked with the
 * TX_NEW flag.  Also, the `frozen' flag which is on nodes in ESR's
 * design is here moved to the cml_transaction; this allows nodes
 * to be unfrozen with an UNDO operation.  And, undo is supported
 * by maintaining attached to each node a most-recent first list of
 * not-UNDONE transactions guarded by that node; thus we can tell if
 * a transaction has been superceded with a single pointer comparison
 * in an UNDO-friendly manner.
 */
 
#include "private.h"
#include "util.h"
#include "debug.h"

CVSID("$Id: transactions.c,v 1.10 2002/09/01 08:23:40 gnb Exp $");

#define g_list_data(link) \
    ((link) == 0 ? 0 : (link)->data)
#define _cml_tx_is_superceded(tx) \
    ((tx) != (cml_transaction *)g_list_data((tx)->guard->transactions_guarded))
#define rb_first_tx(rb) \
    ((rb)->transactions == 0 ? 0 : (cml_transaction *)(rb)->transactions->data)
#define remove_head(l) \
    (l) = g_list_remove_link((l), (l))
    

/*============================================================*/

static cml_binding *
bd_new(const cml_atom *a)
{
    cml_binding *bd;
    
    bd = g_new(cml_binding, 1);
    if (bd == 0)
    	return 0;
	
    memset(bd, 0, sizeof(*bd));
    
    atom_assign(&bd->value, a);
    
    return bd;
}

static void
bd_delete(cml_binding *bd)
{
    atom_dtor(&bd->value);
    g_free(bd);
}

/*============================================================*/
/*
 * C'tor, d'tor
 */

static cml_transaction *
tx_new(cml_node *guard)
{
    cml_transaction *tx;
    static unsigned long last_uniqueid;
    
    tx = g_new(cml_transaction, 1);
    if (tx == 0)
    	return 0;
	
    memset(tx, 0, sizeof(*tx));
    
    tx->uniqueid = ++last_uniqueid;
    tx->flags = TX_NEW;
    tx->guard = guard;
    tx->bindings = g_hash_table_new(g_direct_hash, g_direct_equal);
    
    return tx;
}

static gboolean
_tx_delete_one_binding(gpointer key, gpointer value, gpointer user)
{
    cml_binding *bd = (cml_binding *)value;
    
    bd->node->bindings = g_list_remove(bd->node->bindings, bd);
    bd_delete(bd);
    
    return TRUE;    /* so remove it already */
}

static void
tx_delete(cml_transaction *tx)
{
    if (tx->guard != 0)
    	tx->guard->transactions_guarded = g_list_remove(tx->guard->transactions_guarded, tx);
    g_hash_table_foreach_remove(tx->bindings, _tx_delete_one_binding, 0);
    g_free(tx);
}

/*============================================================*/
/*
 * Get most current value of a node.
 */

const cml_binding *
_cml_tx_get(cml_rulebase *rb, const cml_node *mn)
{
    GList *list;
    
    for (list = mn->bindings ; list != 0 ; list = list->next)
    {
    	cml_binding *bd = (cml_binding *)list->data;
	
	if (!(bd->transaction->flags & TX_UNDONE) &&
	    !_cml_tx_is_superceded(bd->transaction))
	    return bd;
    }
    return 0;
}

/*============================================================*/

static void
_cml_tx_delete_first(cml_rulebase *rb)
{
    cml_transaction *tx = rb_first_tx(rb);
    
    remove_head(rb->transactions);
    if (tx == (cml_transaction *)g_list_data(tx->guard->transactions_guarded))
	remove_head(tx->guard->transactions_guarded);
    
    tx_delete(tx);
}

/*============================================================*/
/*
 * Bind a menunode to the given value
 */

void
_cml_tx_set(
    cml_rulebase *rb,
    cml_node *mn,
    const cml_atom *a,
    cml_node *source)
{
    cml_transaction *tx;
    cml_binding *bd;
    
    if (source == 0)
    	source = mn;

    if (rb->last_undo_id != rb->curr_undo_id)
    {
	/* delete UNDONE transactions now obsolete */
	while (rb_first_tx(rb) != 0 &&
    	       rb_first_tx(rb)->undo_id > rb->curr_undo_id)
	{
	    assert(rb_first_tx(rb)->flags & TX_UNDONE);
	    _cml_tx_delete_first(rb);
	}
	rb->last_undo_id = rb->curr_undo_id;
    }    
	
    /* possibly prepend a new transaction */
    tx = rb_first_tx(rb);
    if (tx == 0 ||
    	!(tx->flags & TX_NEW) ||
	tx->guard != source)
    {
    	if (tx == 0 || !(tx->flags & TX_NEW))
	    rb->last_undo_id = ++rb->curr_undo_id;
	tx = tx_new(source);
	tx->undo_id = rb->curr_undo_id;
	rb->transactions = g_list_prepend(rb->transactions, tx);
	tx->guard->transactions_guarded = g_list_prepend(
	    	    	tx->guard->transactions_guarded, tx);
    }


    bd = bd_new(a);

    bd->node = mn;
    mn->bindings = g_list_prepend(mn->bindings, bd);

    bd->transaction = tx;
    g_hash_table_insert(tx->bindings, mn, (gpointer)bd);

#if DEBUG
    if (debug & DEBUG_TXN)
    {
    	char *valstr = cml_node_get_value_as_string(mn);
	DDPRINTF2(DEBUG_TXN, "%s=\"%s\"\n", mn->name, valstr);
	g_free(valstr);
    }
#endif
}

/*============================================================*/
/*
 * Check if a menunode has binding in given list: new or old.
 */
 
const cml_atom *
_cml_tx_check(cml_rulebase *rb, const cml_node *mn, gboolean checknew)
{
    GList *list;

    for (list = mn->bindings ; list != 0 ; list = list->next)
    {
    	cml_binding *bd = (cml_binding *)list->data;
    	cml_transaction *tx = bd->transaction;
	
    	if ((tx->flags & TX_NEW) != (checknew ? TX_NEW : 0))
	    continue;
	
	if (!(tx->flags & TX_UNDONE) &&
	    !_cml_tx_is_superceded(tx))
	    return &bd->value;
    }
    return 0;
}

/*============================================================*/
/*
 * Commit new bindings.
 */

void
_cml_tx_commit(cml_rulebase *rb, gboolean freeze)
{
    GList *list;
    
    assert(rb->curr_undo_id == rb->last_undo_id);
    for (list = rb->transactions ; list != 0 ; list = list->next)
    {
    	cml_transaction *tx = (cml_transaction *)list->data;

    	if (!(tx->flags & TX_NEW))
	    break;
	    
	tx->flags &= ~TX_NEW;
	if (freeze)
	    tx->flags |= TX_FROZEN;
    }
}

/*============================================================*/
/*
 * Throw away all new bindings.
 */
 
void
_cml_tx_abort(cml_rulebase *rb)
{
    cml_transaction *tx;
    
    while (rb->transactions != 0 &&
    	   (tx = rb_first_tx(rb))->flags & TX_NEW)
    {
    	rb->transactions = g_list_remove_link(rb->transactions, rb->transactions);
    	tx_delete(tx);
    }
}

/*============================================================*/
/*
 * Dump a description of all transactions to the given file.
 * For debugging.
 */

static void
_cml_tx_dump_one(gpointer key, gpointer value, gpointer user)
{
    cml_node *mn = (cml_node *)key;
    cml_binding *bd = (cml_binding *)value;
    FILE *fp = (FILE *)user;
    char *s;
    
    s = cml_atom_value_as_string(&bd->value);
    fprintf(fp, " %s=%s", mn->name, s);
    g_free(s);
}

void
_cml_tx_dump(cml_rulebase *rb, FILE *fp)
{
    GList *list;
    GList *txlist;

    if (fp == 0)
    	fp = stderr;	/* make it easier to call in gdb */
	
    fprintf(fp, "curr_undo_id=%d last_undo_id=%d\n",
    	rb->curr_undo_id, rb->last_undo_id);
    for (list = rb->transactions ; list != 0 ; list = list->next)
    {
    	cml_transaction *tx = (cml_transaction *)list->data;
	
	fprintf(fp, "[%lu] guard= %s  flags= %s,%s,%s  undo_id= %d\n    bindings=",
	    tx->uniqueid,
	    tx->guard->name,
	    (tx->flags & TX_UNDONE ? "undone" : "-"),
	    (tx->flags & TX_NEW ? "new" : "-"),
	    (tx->flags & TX_FROZEN ? "frozen" : "-"),
	    tx->undo_id);
	g_hash_table_foreach(tx->bindings, _cml_tx_dump_one, fp);
	fprintf(fp, "\n    guard->transactions_guarded=");
	for (txlist = tx->guard->transactions_guarded ; txlist != 0 ; txlist = txlist->next)
	{
    	    cml_transaction *tx = (cml_transaction *)txlist->data;
	    fprintf(fp, " [%lu]", tx->uniqueid);
	}
	fprintf(fp, "\n");
    }
}

/*============================================================*/
/*
 * Undo the most recent transaction which is not already undone.
 */

void
_cml_tx_undo(cml_rulebase *rb)
{
    GList *list;

    if (rb->curr_undo_id == 0)
    	return;
	
    /* commit any uncommitted changes */
    if (rb->transactions != 0 &&
    	((cml_transaction *)rb->transactions->data)->flags & TX_NEW)
    	_cml_tx_commit(rb, /*freeze*/FALSE);

    for (list = rb->transactions ; list != 0 ; list = list->next)
    {
    	cml_transaction *tx = (cml_transaction *)list->data;
	
	if (tx->undo_id < rb->curr_undo_id)
	    break;
	if (tx->undo_id == rb->curr_undo_id)
	{
	    assert(!(tx->flags & TX_UNDONE));
	    assert(!(tx->flags & TX_NEW));
	    tx->flags |= TX_UNDONE;
	    assert(!_cml_tx_is_superceded(tx));
	    remove_head(tx->guard->transactions_guarded);
	}
    }
    rb->curr_undo_id--;
}

/*============================================================*/
/*
 * Redo the oldest undone transaction
 */

void
_cml_tx_redo(cml_rulebase *rb)
{
    GList *list;
    int redo_id;

    if (rb->curr_undo_id == rb->last_undo_id)
    	return;
    redo_id = rb->curr_undo_id+1;

    for (list = rb->transactions ; list != 0 ; list = list->next)
    {
    	cml_transaction *tx = (cml_transaction *)list->data;
	
	if (tx->undo_id < redo_id)
	    break;
	if (tx->undo_id == redo_id)
	{
	    assert((tx->flags & TX_UNDONE));
	    assert(!(tx->flags & TX_NEW));
	    tx->flags &= ~TX_UNDONE;
	    assert(g_list_find(tx->guard->transactions_guarded, tx) == 0);
	    tx->guard->transactions_guarded = g_list_prepend(
	    		tx->guard->transactions_guarded, tx);
	}
    }
    rb->curr_undo_id++;
}

/*============================================================*/
/*
 * Remove all transactions -- useful for regression testing.
 */

void
_cml_tx_clear(cml_rulebase *rb)
{
    while (rb_first_tx(rb) != 0)
	_cml_tx_delete_first(rb);
    rb->last_undo_id = rb->curr_undo_id = 0;
}

/*============================================================*/
/*
 * Support for greying-out Undo/Redo buttons.
 */
 
gboolean
cml_rulebase_can_undo(const cml_rulebase *rb)
{
    return (rb->curr_undo_id > 0);
}

gboolean
cml_rulebase_can_redo(const cml_rulebase *rb)
{
    return (rb->curr_undo_id < rb->last_undo_id);
}

gboolean
cml_rulebase_can_freeze(const cml_rulebase *rb)
{
    return (rb->transactions != 0 &&
    	    ((cml_transaction *)rb->transactions->data)->flags & TX_NEW);
}

/*============================================================*/

static void
g_hash_table_get_keys_1(gpointer key, gpointer value, gpointer userdata)
{
    GList **listp = (GList **)userdata;
    
    *listp = g_list_prepend(*listp, key);
}

static GList *
g_hash_table_get_keys(GHashTable *ht)
{
    GList *keys = 0;
    
    g_hash_table_foreach(ht, g_hash_table_get_keys_1, &keys);
    return keys;
}

GList *
_cml_tx_get_triggerable_nodes(cml_rulebase *rb, cml_node *mn, cml_node *source)
{
    GList *nodes = 0;
    cml_transaction *tx;
    
    /* check if `source' would start a new transaction */
    tx = rb_first_tx(rb);
    if (tx == 0 ||
        !(tx->flags & TX_NEW) ||
        tx->guard != source)
    {
        /* 
         * Include all nodes whose current bindings were set by
         * the transaction which is about to be superceded.
         */
        if (source->transactions_guarded != 0)
        {
            tx = (cml_transaction *)g_list_data(source->transactions_guarded);
            nodes = g_hash_table_get_keys(tx->bindings);
            assert(g_list_find(nodes, source) != 0);
        }
    }
    
    /* obviously, setting a node affects a node's own value */
    if (g_list_find(nodes, mn) == 0)
        nodes = g_list_prepend(nodes, mn);

#if DEBUG
    if (debug & DEBUG_TXN)
    {
        GList *iter;
        DDPRINTF2(DEBUG_TXN, "mn=%s source=%s -> ", mn->name, source->name);
        for (iter = nodes ; iter != 0 ; iter = iter->next)
            fprintf(stderr, " %s", ((cml_node *)iter->data)->name);
        fprintf(stderr, "\n");
    }
#endif

    return nodes;
}

/*============================================================*/
/*END*/
