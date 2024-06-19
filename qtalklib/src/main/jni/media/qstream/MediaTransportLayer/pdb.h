/*
 * FILE:   pdb.h - Participant database
 * AUTHOR: Colin Perkins <csp@csperkins.org>
 *
 * Copyright (c) 2002 University of Southern California
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
 * $Revision: 1.9 $
 * $Date: 2005/04/10 18:28:04 $
 *
 */
 
/*
 * A participant database entry. This holds (pointers to) all the
 * information about a particular participant in a teleconference.
 *
 */

#ifndef _PDB_H_
#define _PDB_H_
#include "pbuf.h"

typedef struct  {
	uint32_t		          ssrc;
	char			         *sdes_cname;
	char			         *sdes_name;
	char			         *sdes_email;
	char			         *sdes_phone;
	char			         *sdes_loc;
	char			         *sdes_tool;
	char			         *sdes_note;
	uint8_t			          pt;	/* Last seen RTP payload type for this participant */
	pbuf		                 *playout_buffer;
	struct tfrc		      *tfrc_state;
	struct timeval      creation_time;	/* Time this entry was created */
}pdb_e;

typedef struct s_pdb_node {
        uint32_t                         key;
        void                             *data;
        struct s_pdb_node      *parent;
        struct s_pdb_node      *left;
        struct s_pdb_node      *right;
        uint32_t                         magic;
} pdb_node_t;

typedef struct   
{
        pdb_node_t    *root;
	    pdb_node_t    *iter;
        uint32_t          magic;
        int                  count;
        uint8_t            allowed_pt; /*allowed payload type*/
}pdb;

 pdb   *pdb_init(uint8_t allowd_pt);
void          pdb_destroy(pdb **db);
int           pdb_add(pdb *db, uint32_t ssrc);
pdb_e *pdb_get(pdb *db, uint32_t ssrc);

/* Remove the entry indexed by "ssrc" from the database. If "item" is
 * non-null, return a pointer to the entry removed from the database. 
 * Returns zero on success, non-zero if the entry didn't exist.
 */
int           pdb_remove(pdb *db, uint32_t ssrc,  pdb_e **item);

/*
 * Iterator for the database. Supports only one accessor at once.
 */ 
pdb_e *pdb_iter_init(pdb *db);
pdb_e *pdb_iter_next(pdb *db);
void          pdb_iter_done(pdb *db);

#endif

