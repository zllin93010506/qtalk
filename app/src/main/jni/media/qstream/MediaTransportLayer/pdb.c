/*
 * FILE:   pdb.c - Participant database
 * AUTHOR: Colin Perkins <csp@csperkins.org>
 *         Orion Hodson
 *
 * Copyright (c) 2004-2005 University of Glasgow
 * Copyright (c) 2002-2003 University of Southern California
 * Copyright (c) 1999-2000 University College London
 *
 * Largely based on common/src/btree.c revision 1.7 from the UCL 
 * Robust-Audio Tool v4.2.25. Code is based on the algorithm in:
 *  
 *   Introduction to Algorithms by Corman, Leisserson, and Rivest,
 *   MIT Press / McGraw Hill, 1990.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 
 *      This product includes software developed by the University of Southern
 *      California Information Sciences Institute.
 * 
 * 4. Neither the name of the University nor of the Institute may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Revision: 1.18 $
 * $Date: 2005/04/25 08:38:26 $
 */

#include "config.h"
#include "config_unix.h"
#include "debug.h"
/*
#include "video_types.h"
#include "video_display.h"
*/
#include "rtp.h"		/* Needed by pbuf.h */
#include "pbuf.h"
//#include "tfrc.h"
#include "pdb.h"
 
#define PDB_MAGIC	0x10101010



/*****************************************************************************/
/* Debugging functions...                                                    */
/*****************************************************************************/

#define BTREE_MAGIC      0x10101010
#define BTREE_NODE_MAGIC 0x01010101

//static int pdb_count;

#if 0
static void
pdb_validate_node(pdb_node_t *node, pdb_node_t *parent)
{
        assert(node->magic  == BTREE_NODE_MAGIC);
        assert(node->parent == parent);
        pdb_count++;
        if (node->left != NULL) {
                pdb_validate_node(node->left, node);
        }
        if (node->right != NULL) {
                pdb_validate_node(node->right, node);
        }
}
#endif 

static void
pdb_validate(pdb *t)
{
        assert(t->magic == BTREE_MAGIC);
/*#ifdef DEBUG
        pdb_count = 0;
        if (t->root != NULL) {
                pdb_validate_node(t->root, NULL);
        }
        assert(pdb_count == t->count);
#endif*/
}


/*****************************************************************************/
/* Utility functions                                                         */
/*****************************************************************************/

static pdb_node_t*
pdb_min(pdb_node_t *x)
{
        if (x == NULL) {
                return NULL;
        }
        while(x->left) {
                x = x->left;
        }
        return x;
}

static pdb_node_t*
pdb_successor(pdb_node_t *x)
{
        pdb_node_t *y;

        if (x->right != NULL) {
                return pdb_min(x->right);
        }

        y = x->parent;
        while (y != NULL && x == y->right) {
                x = y;
                y = y->parent;
        }

        return y;
}

static pdb_node_t*
pdb_search(pdb_node_t *x, uint32_t key)
{
        while (x != NULL && key != x->key) {
                if (key < x->key) {
                        x = x->left;
                } else {
                        x = x->right;
                }
        }
        return x; 
}

static void
pdb_insert_node(pdb *tree, pdb_node_t *z) {
        pdb_node_t *x, *y;

	pdb_validate(tree);
        y = NULL;
        x = tree->root;
        while (x != NULL) {
                y = x;
                assert(z->key != x->key);
                if (z->key < x->key) {
                        x = x->left;
                } else {
                        x = x->right;
                }
        }

        z->parent = y;
        if (y == NULL) {
                tree->root = z;
        } else if (z->key < y->key) {
                y->left = z;
        } else {
                y->right = z;
        }
	tree->count++;
	pdb_validate(tree);
}

static pdb_node_t*
pdb_delete_node(pdb *tree, pdb_node_t *z)
{
        pdb_node_t *x, *y;

	pdb_validate(tree);
        if (z->left == NULL || z->right == NULL) {
                y = z;
        } else {
                y = pdb_successor(z);
        }

        if (y->left != NULL) {
                x = y->left;
        } else {
                x = y->right;
        }

        if (x != NULL) {
                x->parent = y->parent;
        }

        if (y->parent == NULL) {
                tree->root = x;
        } else if (y == y->parent->left) {
                y->parent->left = x;
        } else {
                y->parent->right = x;
        }

        z->key  = y->key;
        z->data = y->data;

	tree->count--;

	pdb_validate(tree);
        return y;
}

/*****************************************************************************/

pdb *pdb_init(uint8_t allowed_pt)
{
	pdb *db = malloc(sizeof(pdb));
	if (db != NULL) {
		db->magic = PDB_MAGIC;
		db->count = 0;
		db->root  = NULL;
		db->iter  = NULL;
        db->allowed_pt = allowed_pt;
	}
	else
	{
		return NULL;
	}
	
	return db;
}

void
pdb_destroy(pdb **db_p)
{
      pdb *db = *db_p;

	pdb_validate(db);
        if (db->root != NULL) {
		/* Log an error if the database isn't empty, but continue. */
		/* This leaks memory, but is otherwise harmless...         */
                debug_msg("ERROR: participant database not empty - cannot destroy\n");
        }

        free(db);
        *db_p = NULL;
}

static  pdb_e *pdb_create_item(uint32_t ssrc)
{
	pdb_e *p = malloc(sizeof(pdb_e));
	if (p != NULL) {
		gettimeofday(&(p->creation_time), NULL);
		p->ssrc                = ssrc;
		p->sdes_cname          = NULL;
		p->sdes_name           = NULL;
		p->sdes_email          = NULL;
		p->sdes_phone          = NULL;
		p->sdes_loc            = NULL;
		p->sdes_tool           = NULL;
		p->sdes_note           = NULL;
		p->pt                  = 255;
		p->playout_buffer      = pbuf_init();
		/*p->tfrc_state          = tfrc_init(p->creation_time);*/
	}
	else
	{
		return NULL;
	}
	
	return p;
}

int
pdb_add(pdb *db, uint32_t ssrc)
{
	/* Add an item to the participant database, indexed by ssrc. */
	/* Returns 0 on success, 1 if the participant is already in  */
	/* the database, 2 for other failures.                       */
	pdb_node_t	*x;
	pdb_e	*i;

	pdb_validate(db);
        x = pdb_search(db->root, ssrc);
        if (x != NULL) {
                debug_msg("Item already exists - ssrc %x\n", ssrc);
                return 1;
        }

	i = pdb_create_item(ssrc);
	if (i == NULL) {
		debug_msg("Unable to create database entry - ssrc %x\n", ssrc);
		return 2;
	}

        x = (pdb_node_t *) malloc(sizeof(pdb_node_t));
		if (NULL == x)
		{
			return 2;
		}
        x->key    = ssrc;
	x->data   = i;
        x->parent = NULL;
	x->left   = NULL;
	x->right  = NULL;
	x->magic  = BTREE_NODE_MAGIC;
        pdb_insert_node(db, x);
	debug_msg("Added participant %x\n", ssrc);
        return 0;
}

pdb_e *pdb_get(pdb *db, uint32_t ssrc)
{
	/* Return a pointer to the item indexed by ssrc, or NULL if   */
	/* the item is not present in the database.                   */
        pdb_node_t *x;

	pdb_validate(db);
        x = pdb_search(db->root, ssrc);
        if (x != NULL) {
                return x->data;
        }
        return NULL;
}

int
pdb_remove(pdb *db, uint32_t ssrc, pdb_e **item)
{
	/* Remove the item indexed by ssrc. Return zero on success.   */
        pdb_node_t *x;

	pdb_validate(db);
        x = pdb_search(db->root, ssrc);
        if (x == NULL) {
                debug_msg("Item not on tree - ssrc %ul\n", ssrc);
                return 1;
        }

	/* Note value that gets freed is not necessarily the the same as node
	 * that gets removed from tree since there is an optimization to avoid
	 * pointer updates in tree which means sometimes we just copy key and
	 * data from one node to another.  
         */
	if (item != NULL) {
		*item = x->data;
	}
        x = pdb_delete_node(db, x);
        free(x);
        return 0;
}


/* 
 * Iterator functions 
 */

pdb_e *pdb_iter_init(pdb *db)
{
	if (db->root == NULL) {
		return NULL; 	/* The database is empty */
	}
	db->iter = pdb_min(db->root);
	return db->iter->data;
}

pdb_e *pdb_iter_next(pdb *db)
{
	assert(db->iter != NULL);
	db->iter = pdb_successor(db->iter);
	if (db->iter == NULL) {
		return NULL;
	}
	return db->iter->data;
}

void
pdb_iter_done(pdb *db)
{
	db->iter = NULL;
}

