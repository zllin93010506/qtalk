/*
 * FILE:     rtp.c
 * AUTHOR:   Colin Perkins   <csp@csperkins.org>
 * MODIFIED: Orion Hodson    <o.hodson@cs.ucl.ac.uk>
 *           Markus Germeier <mager@tzi.de>
 *           Bill Fenner     <fenner@research.att.com>
 *           Timur Friedman  <timur@research.att.com>
 *           Ladan Gharai    <ladan@isi.edu>
 *           Steve Smith     <ssmith@vislab.usyd.edu.au>
 *
 * The routines in this file implement the Real-time Transport Protocol,
 * RTP, as specified in RFC 3550. Portions of the code are derived from 
 * the algorithms published in that specification.
 *
 * Copyright (c) 1998-2001 University College London
 * Copyright (c) 2001-2004 University of Southern California
 * Copyright (c) 2003-2005 University of Glasgow
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Computer Science
 *      Department at University College London and by the University of
 *      Southern California Information Sciences Institute.
 * 4. Neither the name of the University nor of the Department may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Revision: 1.301 $ 
 * $Date: 2005/07/23 17:07:49 $
 * 
 */
#include "logging.h"
#define TAG "SR2_CDRC"
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)

#ifdef WIN32
#include "config_w32.h"
#endif
#include "config.h"
#include "config_unix.h"
#include "memory.h"
#include "debug.h"
#include "net_udp.h"
/*
#include "crypto/random.h"
*/
#include "drand48.h"
#include "gettimeofday.h"
#include "tv.h"
/*
#include "crypto/md5.h"
*/
#include "ntp.h"
#include "tfrc.h"
#include "rtp.h"
#include "../GlobalDefine.h"
#include "pdb.h"
#include "ET1_QStream_RFC4585.h"
#include "rtp_payload_type_define.h"
#include <stdbool.h>

//Corey  FEC
#include "rtp_multiStream.h"
#include "ec_agent.h"

static void force_send_rtcp(struct rtp *session);

#define MAX_MISORDER   100
#define MIN_SEQUENTIAL 1  /* A-kuo intial:2  2011.01.12   */
/*
 * Definitions for the RTP/RTCP packets on the wire...
 */
#define RTP_SEQ_MOD        0x10000
#define RTP_LOWER_LAYER_OVERHEAD 28	/* IPv4 + UDP */

#define RTCP_SR   200
#define RTCP_RR   201
#define RTCP_SDES 202
#define RTCP_BYE  203
#define RTCP_APP  204

//Corey - used for debugging recvmsg
#define USE_RECVMSG 0

typedef struct _rtcp_rr_wrapper {
	struct _rtcp_rr_wrapper	*next;
	struct _rtcp_rr_wrapper	*prev;
        uint32_t                 reporter_ssrc;
	rtcp_rr			*rr;
	rtcp_rx 		*rx;    /* TFRC RXs */
	struct timeval		*ts;	/* Arrival time of this RR */
} rtcp_rr_wrapper;

/*
 * The RTP database contains source-specific information needed 
 * to make it all work. 
 */

static uint32_t	 tmp_sec;
static uint32_t	 tmp_frac;


typedef struct _source {
	struct _source	*next;
	struct _source	*prev;
	uint32_t	 ssrc;
	char		*sdes_cname;
	char		*sdes_name;
	char		*sdes_email;
	char		*sdes_phone;
	char		*sdes_loc;
	char		*sdes_tool;
	char		*sdes_note;
	char		*sdes_priv;
	rtcp_sr		*sr;
	uint32_t	 last_sr_sec;
	uint32_t	 last_sr_frac;
	struct timeval	 last_active;
	int		 sender;
	int		 got_bye;		/* TRUE if we've received an RTCP bye from this source */
	uint32_t	 base_seq;
	uint16_t	 max_seq;
	uint32_t	 bad_seq;
	uint32_t	 cycles;
	int		 received;
	int		 received_prior;
	int		 expected_prior;
	int		 probation;
	uint32_t	 jitter;
	uint32_t	 transit;
	uint32_t	 magic;			/* For debugging... */
} source;


/* The size of the hash table used to hold the source database. */
/* Should be large enough that we're unlikely to get collisions */
/* when sources are added, but not too large that we waste too  */
/* much memory. Sedgewick ("Algorithms", 2nd Ed, Addison-Wesley */
/* 1988) suggests that this should be around 1/10th the number  */
/* of entries that we expect to have in the database and should */
/* be a prime number. Everything continues to work if this is   */
/* too low, it just goes slower... for now we assume around 100 */
/* participants is a sensible limit so we set this to 11.       */   
#define RTP_DB_SIZE	11

/*
 *  Options for an RTP session are stored in the "options" struct.
 */

typedef struct {
	int 	promiscuous_mode;
	int	wait_for_rtcp;
	int	filter_my_packets;
	int	reuse_bufs;
} options;

/*
 * Encryption function types
 */
typedef int (*rtp_encrypt_func)(void *, unsigned char *data, unsigned int size, unsigned char *initvec);
typedef int (*rtp_decrypt_func)(void *, unsigned char *data, unsigned int size, unsigned char *initvec);

/*
 * The "struct rtp" defines an RTP session.
 */

struct rtp {
    socket_udp          *rtp_socket;
    socket_udp          *rtcp_socket;
    char                *addr;
    uint16_t            rx_port;
    uint16_t            tx_port;
    int                 ttl;
    uint32_t            my_ssrc;
    source              *db[RTP_DB_SIZE];
    rtcp_rr_wrapper     rr[RTP_DB_SIZE][RTP_DB_SIZE]; 	/* Indexed by [hash(reporter)][hash(reportee)] */
    options             *opt;
    uint8_t             *userdata;
    int                 invalid_rtp_count;
    int                 invalid_rtcp_count;
    int                 bye_count;
    int                 ssrc_count;
    int                 ssrc_count_prev;		/* ssrc_count at the time we last recalculated our RTCP interval */
    int                 sender_count;
    int                 initial_rtcp;
    int                 sending_bye;			/* TRUE if we're in the process of sending a BYE packet */
    double              avg_rtcp_size;
    int                 we_sent;
    double              rtcp_bw;			/* RTCP bandwidth fraction, in octets per second. */
    struct timeval      last_update;
    struct timeval      last_rtp_send_time;
    struct timeval      last_rtcp_send_time;
    struct timeval      next_rtcp_send_time;
    double              rtcp_interval;
    int                 sdes_count_pri;
    int                 sdes_count_sec;
    int                 sdes_count_ter;
    uint16_t            rtp_seq;
    uint32_t            rtp_pcount;
    uint32_t            rtp_bcount;
    int                 tfrc_on;			/* indicates TFRC congestion control */
    struct tfrc_snd     *tfrcs;				/* tfrc sender variables */
    struct tfrc_rcv     *tfrcr;				/* tfrc receiver variables */
    uint32_t            tfrc_reject_count;
	
	/* Corey - To estimate RTT */
    struct timeval RTP_now;
    struct timeval RTCP_now;

    unsigned long       ea_id;
    int                 fec_on;
    rtp_mulStream       *fec_stream; /*Corey  FEC*/
	

    char                *encryption_algorithm;
    int                 encryption_enabled;
    rtp_encrypt_func    encrypt_func;
    rtp_decrypt_func    decrypt_func;
    int                 encryption_pad_length;
    void                *crypto_state;
    rtp_callback        callback;
    struct msghdr       *mhdr;
    uint32_t            magic;				/* For debugging...  */
    ntp_rtpts_pair      ntp_time_info;
    
    uint32_t            opposite_ssrc;
    int                 has_opposite_ssrc;
    int                 resend_enable;
    unsigned long tl_session;

    bool                disable_rtcp;	/* it is a flag used to deside whether send the RTCP packet or not, add by Willy - 2011-11-07 */

    int                 media_type;    /* Sync with ET1_QStream_TransportLayer.h ( 0:PAYLOAD_AUDIO_CONTINUOUS ,  *
                                        * 1:PAYLOAD_AUDIO_PACKETIZED , 2:PAYLOAD_VIDEO , 3:PAYLOAD_OTHER)      */
    uint8_t             fir_fci_seq_num;
};

void set_rtp_opposite_ssrc(struct rtp *session, uint32_t ssrc)
{
    if(!(session->has_opposite_ssrc))
        session->has_opposite_ssrc = 1;

   session->opposite_ssrc = ssrc;
}

uint32_t rtp_opposite_ssrc(struct rtp *session)
{
    if(session->has_opposite_ssrc) {
        return session->opposite_ssrc;
    }
    return 0;
}

static inline int 
filter_event(struct rtp *session, uint32_t ssrc)
{
	return session->opt->filter_my_packets && (ssrc == rtp_my_ssrc(session));
}

static inline int 
ssrc_hash(uint32_t ssrc)
{
	/* Hash from an ssrc to a position in the source database.   */
	/* Assumes that ssrc values are uniformly distributed, which */
	/* should be true but probably isn't (Rosenberg has reported */
	/* that many implementations generate ssrc values which are  */
	/* not uniformly distributed over the space, and the H.323   */
	/* spec requires that they are non-uniformly distributed).   */
	/* This routine is written as a function rather than inline  */
	/* code to allow it to be made smart in future: probably we  */
	/* should run MD5 on the ssrc and derive a hash value from   */
	/* that, to ensure it's more uniformly distributed?          */
	return ssrc % RTP_DB_SIZE;
}

static void 
insert_rr(struct rtp *session, uint32_t reporter_ssrc, rtcp_rr *rr, rtcp_rx *rx)
{
    /* Insert the reception report into the receiver report      */
    /* database. This database is a two dimensional table of     */
    /* rr_wrappers indexed by hashes of reporter_ssrc and        */
    /* reportee_src.  The rr_wrappers in the database are        */
    /* sentinels to reduce conditions in list operations.        */
	/* The ts is used to determine when to timeout this rr.      */

    rtcp_rr_wrapper *cur, *start;

    start = &session->rr[ssrc_hash(reporter_ssrc)][ssrc_hash(rr->ssrc)];
    cur   = start->next;

    while (cur != start) {
        if (cur->reporter_ssrc == reporter_ssrc && cur->rr->ssrc == rr->ssrc) {
            /* Replace existing entry in the database  */
            free (cur->rr);
			if (cur->rx) {
                free (cur->rx);
			}
			free(cur->ts);
            cur->rr = rr;
			cur->rx = rx;
			cur->ts = malloc(sizeof(struct timeval));
			if (NULL == cur->ts)
			{
				return ;
			}
			gettimeofday(cur->ts, NULL);
            return;
        }
        cur = cur->next;
    }
        
    /* No entry in the database so create one now. */
    cur = (rtcp_rr_wrapper*)malloc(sizeof(rtcp_rr_wrapper));
    if (NULL == cur)
    {
        return ;
    }
    cur->reporter_ssrc = reporter_ssrc;
    cur->rr = rr;
	cur->rx = rx;
	cur->ts = malloc(sizeof(struct timeval));
	if (NULL == cur->ts)
	{
        free(cur);
		return ;
	}
	gettimeofday(cur->ts, NULL);
    /* Fix links */
    cur->next       = start->next;
    cur->next->prev = cur;
    cur->prev       = start;
    cur->prev->next = cur;

    TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Created new rr entry for 0x%08u from source 0x%08u\n", rr->ssrc, reporter_ssrc);
    return;
}

static void 
remove_rr(struct rtp *session, uint32_t ssrc)
{
	/* Remove any RRs from "s" which refer to "ssrc" as either   */
        /* reporter or reportee.                                     */
        rtcp_rr_wrapper *start, *cur, *tmp;
        int i;

        /* Remove rows, i.e. ssrc == reporter_ssrc                   */
        for(i = 0; i < RTP_DB_SIZE; i++) {
                start = &session->rr[ssrc_hash(ssrc)][i];
                cur   = start->next;
                while (cur != start) {
                        if (cur->reporter_ssrc == ssrc) {
                                tmp = cur;
                                cur = cur->prev;
                                tmp->prev->next = tmp->next;
                                tmp->next->prev = tmp->prev;
				free(tmp->ts);
                                free(tmp->rr);
				if (tmp->rx) free (tmp->rx);
                                free(tmp);
                        }
                        cur = cur->next;
                }
        }

        /* Remove columns, i.e.  ssrc == reporter_ssrc */
        for(i = 0; i < RTP_DB_SIZE; i++) {
                start = &session->rr[i][ssrc_hash(ssrc)];
                cur   = start->next;
                while (cur != start) {
                        if (cur->rr->ssrc == ssrc) {
                                tmp = cur;
                                cur = cur->prev;
                                tmp->prev->next = tmp->next;
                                tmp->next->prev = tmp->prev;
				free(tmp->ts);
                                free(tmp->rr);
				if (tmp->rx) free (tmp->rx);
                                free(tmp);
                        }
                        cur = cur->next;
                }
        }
}

static void 
timeout_rr(struct rtp *session, struct timeval *curr_ts)
{
	/* Timeout any reception reports which have been in the database for more than 3 */
	/* times the RTCP reporting interval without refresh.                            */
        rtcp_rr_wrapper *start, *cur, *tmp;
	rtp_event	 event;
        int 		 i, j;

        for(i = 0; i < RTP_DB_SIZE; i++) {
        	for(j = 0; j < RTP_DB_SIZE; j++) {
			start = &session->rr[i][j];
			cur   = start->next;
			while (cur != start) {
				if (tv_diff(*curr_ts, *(cur->ts)) > (session->rtcp_interval * 3)) {
					/* Signal the application... */
					if (!filter_event(session, cur->reporter_ssrc)) {
						event.ssrc = cur->reporter_ssrc;
						event.type = RR_TIMEOUT;
						event.data = cur->rr;              /* how to send the rx up? */
						session->callback(session, &event);
					}
					/* Delete this reception report... */
					tmp = cur;
					cur = cur->prev;
					tmp->prev->next = tmp->next;
					tmp->next->prev = tmp->prev;
					free(tmp->ts);
					free(tmp->rr);
					if (tmp->rx) free (tmp->rx);
					free(tmp);
				}
				cur = cur->next;
			}
		}
	}
}

static const rtcp_rr *
get_rr(struct rtp *session, uint32_t reporter_ssrc, uint32_t reportee_ssrc)
{
        rtcp_rr_wrapper *cur, *start;

        start = &session->rr[ssrc_hash(reporter_ssrc)][ssrc_hash(reportee_ssrc)];
        cur   = start->next;
        while (cur != start) {
                if (cur->reporter_ssrc == reporter_ssrc &&
                    cur->rr->ssrc      == reportee_ssrc) {
                        return cur->rr;
                }
                cur = cur->next;
        }
        return NULL;
}

static inline void 
check_source(source *s)
{
#ifdef _RTP_DEBUG
	assert(s != NULL);
	assert(s->magic == 0xc001feed);
#else
	UNUSED(s);
#endif
}

static inline void 
check_database(struct rtp *session)
{
	/* This routine performs a sanity check on the database. */
	/* This should not call any of the other routines which  */
	/* manipulate the database, to avoid common failures.    */
#ifdef _RTP_DEBUG_
	source 	 	*s;
	int	 	 source_count;
	int		 chain;

	assert(session != NULL);
	assert(session->magic == 0xfeedface);
	/* Check that we have a database entry for our ssrc... */
	/* We only do this check if ssrc_count > 0 since it is */
	/* performed during initialisation whilst creating the */
	/* source entry for my_ssrc.                           */
	if (session->ssrc_count > 0) {
		for (s = session->db[ssrc_hash(session->my_ssrc)]; s != NULL; s = s->next) {
			if (s->ssrc == session->my_ssrc) {
				break;
			}
		}
		assert(s != NULL);
	}

	source_count = 0;
	for (chain = 0; chain < RTP_DB_SIZE; chain++) {
		/* Check that the linked lists making up the chains in */
		/* the hash table are correctly linked together...     */
		for (s = session->db[chain]; s != NULL; s = s->next) {
			check_source(s);
			source_count++;
			if (s->prev == NULL) {
				assert(s == session->db[chain]);
			} else {
				assert(s->prev->next == s);
			}
			if (s->next != NULL) {
				assert(s->next->prev == s);
			}
			/* Check that the SR is for this source... */
			if (s->sr != NULL) {
				assert(s->sr->ssrc == s->ssrc);
			}
		}
	}
	/* Check that the number of entries in the hash table  */
	/* matches session->ssrc_count                         */
	assert(source_count == session->ssrc_count);
#else 
	UNUSED(session);
#endif
}

static inline source *
get_source(struct rtp *session, uint32_t ssrc)
{
	source *s;

	check_database(session);
	for (s = session->db[ssrc_hash(ssrc)]; s != NULL; s = s->next) {
		if (s->ssrc == ssrc) {
			check_source(s);
			return s;
		}
	}
	return NULL;
}

static source *
really_create_source(struct rtp *session, uint32_t ssrc, int probation, source *s)
{
	/* Create a new source entry, and add it to the database.    */
	/* The database is a hash table, using the separate chaining */
	/* algorithm.                                                */
	rtp_event	 event;
	int		 h;

	check_database(session);
	/* This is a new source, we have to create it... */
	h = ssrc_hash(ssrc);
	s = (source *) malloc(sizeof(source));
	if (NULL == s)
	{
		return NULL;
	}
	memset(s, 0, sizeof(source));
	s->magic          = 0xc001feed;
	s->next           = session->db[h];
	s->ssrc           = ssrc;
	if (probation) {
		/* This is a probationary source, which only counts as */
		/* valid once several consecutive packets are received */
		s->probation = -1;
	} else {
		s->probation = 0;
	}
	
	gettimeofday(&(s->last_active), NULL);
	/* Now, add it to the database... */
	if (session->db[h] != NULL) {
		session->db[h]->prev = s;
	}
	session->db[ssrc_hash(ssrc)] = s;
	session->ssrc_count++;
	check_database(session);

	TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "Created database entry 0x%08x (%d sources)\n", ssrc, session->ssrc_count);
        if (ssrc != session->my_ssrc) {
                /* Do not send during rtp_init since application cannot map the address */
                /* of the rtp session to anything since rtp_init has not returned yet.  */
		if (!filter_event(session, ssrc)) {
			event.ssrc = ssrc;
			event.type = SOURCE_CREATED;
			event.data = NULL;
			session->callback(session, &event);
		}
        }

	return s;
}

static inline source *
create_source(struct rtp *session, uint32_t ssrc, int probation)
{
	source		*s = get_source(session, ssrc);
	if (s == NULL) {
		return really_create_source(session, ssrc, probation, s);
	}
	/* Source is already in the database... Mark it as */
	/* active and exit (this is the common case...)    */
	gettimeofday(&(s->last_active), NULL);
	return s;
}

static void 
delete_source(struct rtp *session, uint32_t ssrc)
{
	/* Remove a source from the RTP database... */
	source		*s = get_source(session, ssrc);
	int		 h = ssrc_hash(ssrc);
	rtp_event	 event;
	struct timeval	 event_ts;

	assert(s != NULL);	/* Deleting a source which doesn't exist is an error... */

	gettimeofday(&event_ts, NULL);

	check_source(s);
	check_database(session);
	if (session->db[h] == s) {
		/* It's the first entry in this chain... */
		session->db[h] = s->next;
		if (s->next != NULL) {
			s->next->prev = NULL;
		}
	} else {
		assert(s->prev != NULL);	/* Else it would be the first in the chain... */
		s->prev->next = s->next;
		if (s->next != NULL) {
			s->next->prev = s->prev;
		}
	}
	/* Free the memory allocated to a source... */
	if (s->sdes_cname != NULL) free(s->sdes_cname);
	if (s->sdes_name  != NULL) free(s->sdes_name);
	if (s->sdes_email != NULL) free(s->sdes_email);
	if (s->sdes_phone != NULL) free(s->sdes_phone);
	if (s->sdes_loc   != NULL) free(s->sdes_loc);
	if (s->sdes_tool  != NULL) free(s->sdes_tool);
	if (s->sdes_note  != NULL) free(s->sdes_note);
	if (s->sdes_priv  != NULL) free(s->sdes_priv);
	if (s->sr    != NULL) free(s->sr);

        remove_rr(session, ssrc); 

	/* Reduce our SSRC count, and perform reverse reconsideration on the RTCP */
	/* reporting interval (draft-ietf-avt-rtp-new-05.txt, section 6.3.4). To  */
	/* make the transmission rate of RTCP packets more adaptive to changes in */
	/* group membership, the following "reverse reconsideration" algorithm    */
	/* SHOULD be executed when a BYE packet is received that reduces members  */
	/* to a value less than pmembers:                                         */
	/* o  The value for tn is updated according to the following formula:     */
	/*       tn = tc + (members/pmembers)(tn - tc)                            */
	/* o  The value for tp is updated according the following formula:        */
	/*       tp = tc - (members/pmembers)(tc - tp).                           */
	/* o  The next RTCP packet is rescheduled for transmission at time tn,    */
	/*    which is now earlier.                                               */
	/* o  The value of pmembers is set equal to members.                      */
	session->ssrc_count--;
	if (session->ssrc_count < session->ssrc_count_prev) {
		gettimeofday(&(session->next_rtcp_send_time), NULL);
		gettimeofday(&(session->last_rtcp_send_time), NULL);
		tv_add(&(session->next_rtcp_send_time), (session->ssrc_count / session->ssrc_count_prev) 
						     * tv_diff(session->next_rtcp_send_time, event_ts));
		tv_add(&(session->last_rtcp_send_time), - ((session->ssrc_count / session->ssrc_count_prev) 
						     * tv_diff(event_ts, session->last_rtcp_send_time)));
		session->ssrc_count_prev = session->ssrc_count;
	}

	/* Signal to the application that this source is dead... */
	if (!filter_event(session, ssrc)) {
		event.ssrc = ssrc;
		event.type = SOURCE_DELETED;
		event.data = NULL;
		session->callback(session, &event);
	}
        free(s);
	check_database(session);
}

static inline void 
init_seq(source *s, uint16_t seq)
{
	/* Taken from draft-ietf-avt-rtp-new-01.txt */
	check_source(s);
	s->base_seq = seq;
	s->max_seq = seq;
	s->bad_seq = RTP_SEQ_MOD + 1;
	s->cycles = 0;
	s->received = 0;
	s->received_prior = 0;
	s->expected_prior = 0;
}

static int 
update_seq(source *s, uint16_t seq)
{
	/* Taken from draft-ietf-avt-rtp-new-01.txt */
	uint16_t udelta = seq - s->max_seq;

	/*
	 * Source is not valid until MIN_SEQUENTIAL packets with
	 * sequential sequence numbers have been received.
	 */
	check_source(s);
	if (s->probation) {
		  /* packet is in sequence */
		  if (seq == s->max_seq + 1) {
				s->probation--;
				s->max_seq = seq;
				if (s->probation == 0) {
					 init_seq(s, seq);
					 s->received++;
					 return 1;
				}
		  } else {
				s->probation = MIN_SEQUENTIAL - 1;
				s->max_seq = seq;
		  }
		  return 0;
	} else if (udelta < TFRC_MAX_DROPOUT) {
		  /* in order, with permissible gap */
		  if (seq < s->max_seq) {
				/*
				 * Sequence number wrapped - count another 64K cycle.
				 */
				s->cycles += RTP_SEQ_MOD;
		  }
		  s->max_seq = seq;
	} else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
		  /* the sequence number made a very large jump */
		  if (seq == s->bad_seq) {
				/*
				 * Two sequential packets -- assume that the other side
				 * restarted without telling us so just re-sync
				 * (i.e., pretend this was the first packet).
				 */
				init_seq(s, seq);
		  } else {
				printf("<warning> receive an RTP packet %d whose sequence number made a very large jump, udelta=%d!\n",seq,udelta);
				s->bad_seq = (seq + 1) & (RTP_SEQ_MOD-1);
				return 0;
		  }
	} else {
		  /* duplicate or reordered packet */
	}
	s->received++;
	return 1;
}

static double 
rtcp_interval(struct rtp *session)
{
	/* Minimum average time between RTCP packets from this site (in   */
	/* seconds).  This time prevents the reports from `clumping' when */
	/* sessions are small and the law of large numbers isn't helping  */
	/* to smooth out the traffic.  It also keeps the report interval  */
	/* from becoming ridiculously small during transient outages like */
	/* a network partition.                                           */
	double const RTCP_MIN_TIME = 5.0;
	/* Fraction of the RTCP bandwidth to be shared among active       */
	/* senders.  (This fraction was chosen so that in a typical       */
	/* session with one or two active senders, the computed report    */
	/* time would be roughly equal to the minimum report time so that */
	/* we don't unnecessarily slow down receiver reports.) The        */
	/* receiver fraction must be 1 - the sender fraction.             */
	double const RTCP_SENDER_BW_FRACTION = 0.25;
	double const RTCP_RCVR_BW_FRACTION   = (1-RTCP_SENDER_BW_FRACTION);
	/* To compensate for "unconditional reconsideration" converging   */
	/* to a value below the intended average.                         */
	double const COMPENSATION            = 2.71828 - 1.5;

	double t;				              /* interval */
	double rtcp_min_time = RTCP_MIN_TIME;
	int n;			        /* no. of members for computation */
	double rtcp_bw = session->rtcp_bw;

	/* Very first call at application start-up uses half the min      */
	/* delay for quicker notification while still allowing some time  */
	/* before reporting for randomization and to learn about other    */
	/* sources so the report interval will converge to the correct    */
	/* interval more quickly.                                         */
	if (session->initial_rtcp) {
		rtcp_min_time /= 2;
	}

	/* If there were active senders, give them at least a minimum     */
	/* share of the RTCP bandwidth.  Otherwise all participants share */
	/* the RTCP bandwidth equally.                                    */
	if (session->sending_bye) {
		n = session->bye_count;
	} else {
		n = session->ssrc_count;
	}
	if (session->sender_count > 0 && session->sender_count < n * RTCP_SENDER_BW_FRACTION) {
		if (session->we_sent) {
			rtcp_bw *= RTCP_SENDER_BW_FRACTION;
			n = session->sender_count;
		} else {
			rtcp_bw *= RTCP_RCVR_BW_FRACTION;
			n -= session->sender_count;
		}
	}

	/* The effective number of sites times the average packet size is */
	/* the total number of octets sent when each site sends a report. */
	/* Dividing this by the effective bandwidth gives the time        */
	/* interval over which those packets must be sent in order to     */
	/* meet the bandwidth target, with a minimum enforced.  In that   */
	/* time interval we send one report so this time is also our      */
	/* average time between reports.                                  */
	t = session->avg_rtcp_size * n / rtcp_bw;
	if (t < rtcp_min_time) {
		t = rtcp_min_time;
	}
	session->rtcp_interval = t;

	/* To avoid traffic bursts from unintended synchronization with   */
	/* other sites, we then pick our actual next report interval as a */
	/* random number uniformly distributed between 0.5*t and 1.5*t.   */
	return (t * (drand48() + 0.5)) / COMPENSATION;
}

#define MAXCNAMELEN	255

static char *
get_cname(socket_udp *s)
{
        /* Set the CNAME. This is "user@hostname" or just "hostname" if the username cannot be found. */
        const char              *hname=NULL;
        char                    *uname;
        char                    *cname;
        struct passwd           *pwent;

        cname = (char *) malloc(MAXCNAMELEN + 1);
		if (NULL == cname)
		{
			return NULL;
		}
        cname[0] = '\0';

        /* First, fill in the username... */
	uname = NULL;
#ifndef WIN32
        pwent = getpwuid(getuid());
	if (pwent != NULL) {
		uname = pwent->pw_name;
	}
#endif
        if (uname != NULL) {
                strncpy(cname, uname, MAXCNAMELEN - 1);
                strcat(cname, "@");
        }

        /* Now the hostname. Must be dotted-quad IP address. */
        //hname = udp_host_addr(s);
	if (hname == NULL) {
		/* If we can't get our IP address we use the loopback address... */
		/* This is horrible, but it stops the code from failing.         */
		hname = "127.0.0.1";
	}
        strncpy(cname + strlen(cname), hname, MAXCNAMELEN - strlen(cname));
        return cname;
}

static void 
init_opt(struct rtp *session)
{
	/* Default option settings. */
	rtp_set_option(session, RTP_OPT_PROMISC,           FALSE);
	rtp_set_option(session, RTP_OPT_WEAK_VALIDATION,   FALSE);
	rtp_set_option(session, RTP_OPT_FILTER_MY_PACKETS, FALSE);
	rtp_set_option(session, RTP_OPT_REUSE_PACKET_BUFS, FALSE);
}

static void 
init_rng(const char *s)
{
        static uint32_t seed;
        if (seed == 0) {
                pid_t p = getpid();
		int32_t i, n;

                while (*s) {
                        seed += (uint32_t)*s++;
                        seed = seed * 31 + 1;
                }

                seed = 1 + seed * 31 + (uint32_t)p;
#ifdef WIN32
                srand(seed);
#else
                srand48(seed);
#endif
		UNUSED(i);
		UNUSED(n);
	}
}

/* See rtp_init_if(); calling rtp_init() is just like calling
 * rtp_init_if() with a NULL interface argument.
 */

/**
 * rtp_init:
 * @addr: IP destination of this session (unicast or multicast),
 * as an ASCII string.  May be a host name, which will be looked up,
 * or may be an IPv4 dotted quad or IPv6 literal adddress.
 * @rx_port: The port to which to bind the UDP socket
 * @tx_port: The port to which to send UDP packets
 * @ttl: The TTL with which to send multicasts
 * @rtcp_bw: The total bandwidth (in units of bytes per second) that is
 * allocated to RTCP.
 * @callback: See section on #rtp_callback.
 * @userdata: Opaque data associated with the session.  See
 * rtp_get_userdata().
 *
 *
 * Returns: An opaque session identifier to be used in future calls to
 * the RTP library functions, or NULL on failure.
 */
struct rtp *
rtp_init(const char *addr, uint16_t rx_port, uint16_t tx_port, int ttl, double rtcp_bw, 
         int tfrc_on, RTP_ECControl_t ec_arg, int resend_enable, int tos, rtp_callback callback, uint8_t *userdata, const char* local_addr)
{
	return rtp_init_if(addr, NULL, rx_port, tx_port, ttl, rtcp_bw, tfrc_on, ec_arg, resend_enable, tos, callback, userdata,local_addr);
}

/**
 * rtp_init_if:
 * @addr: IP destination of this session (unicast or multicast),
 * as an ASCII string.  May be a host name, which will be looked up,
 * or may be an IPv4 dotted quad or IPv6 literal adddress.
 * @iface: If the destination of the session is multicast,
 * the optional interface to bind to.  May be NULL, in which case
 * the default multicast interface as determined by the system
 * will be used.
 * @rx_port: The port to which to bind the UDP socket
 * @tx_port: The port to which to send UDP packets
 * @ttl: The TTL with which to send multicasts
 * @rtcp_bw: The total bandwidth (in units of ___) that is
 * allocated to RTCP.
 * @callback: See section on #rtp_callback.
 * @userdata: Opaque data associated with the session.  See
 * rtp_get_userdata().
 *
 * Creates and initializes an RTP session.
 *
 * Returns: An opaque session identifier to be used in future calls to
 * the RTP library functions, or NULL on failure.
 */
struct rtp *
rtp_init_if(const char *addr, char *iface, uint16_t rx_port, uint16_t tx_port, int ttl, 
            double rtcp_bw, int tfrc_on, RTP_ECControl_t ec_arg, int resend_enable, int tos, rtp_callback callback, uint8_t *userdata, const char* local_addr)
{
	struct rtp 	*session;
	struct timeval   now;
	int         	 i, j;
	char		*cname;

        if (ttl < 0) {
                TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "ttl must be greater than zero\n");
                return NULL;
        }
        if (rx_port % 2) {
                TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "rx_port must be even\n");
                return NULL;
        }
        if (tx_port % 2) {
                TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "tx_port must be even\n");
                return NULL;
        }

	session 		= (struct rtp *) malloc(sizeof(struct rtp));
	if (NULL == session)
	{
		return NULL;
	}
	memset (session, 0, sizeof(struct rtp));

	session->magic		= 0xfeedface;
	session->opt		= (options *) malloc(sizeof(options));
	if (NULL == session->opt)
	{
		free(session);
		session = NULL;
		return NULL;
	}
	session->userdata	= userdata;
	session->addr		= strdup(addr);
	session->rx_port	= rx_port;
	session->tx_port	= tx_port;
	session->ttl		= min(ttl, 127);
	session->rtp_socket	= udp_init_if(addr, iface, rx_port, tx_port, ttl, 0,local_addr, tos);
	session->rtcp_socket= udp_init_if(addr, iface, (uint16_t) (rx_port+1), (uint16_t) (tx_port+1), ttl, 0,local_addr, tos);

	init_opt(session);

	if (session->rtp_socket == NULL || session->rtcp_socket == NULL) {
		TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "Unable to open network\n");
		free(session);
		return NULL;
	}

//#if defined(ET1_QSTREAM_CROSS_COMPILE) || defined(QS_PX2_PLATFORM)
	init_rng("127.0.0.1");
//#else
//    init_rng(udp_host_addr(session->rtp_socket));
//#endif	

#ifdef WIN32
	session->my_ssrc            = (uint32_t) rand();
#else
	session->my_ssrc            = (uint32_t) lrand48();
#endif

    TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "session->my_ssrc = %d\n", session->my_ssrc);
	session->callback           = callback;
	session->invalid_rtp_count  = 0;
	session->invalid_rtcp_count = 0;
	session->bye_count          = 0;
	session->ssrc_count         = 0;
	session->ssrc_count_prev    = 0;
	session->sender_count       = 0;
	session->initial_rtcp       = TRUE;
	session->sending_bye        = FALSE;
	session->avg_rtcp_size      = -1;	/* Sentinal value: reception of first packet starts initial value... */
	session->we_sent            = FALSE;
	session->rtcp_bw            = rtcp_bw;
	session->sdes_count_pri     = 0;
	session->sdes_count_sec     = 0;
	session->sdes_count_ter     = 0;
#ifdef WIN32
	session->rtp_seq            = (uint16_t) rand();
#else
	session->rtp_seq            = (uint16_t) lrand48();
#endif
	session->rtp_pcount         = 0;
	session->rtp_bcount         = 0;
	session->tfrc_on            = tfrc_on; 
    session->ntp_time_info.ntp_sec = 0;
    session->ntp_time_info.ntp_frac = 0;
    session->ntp_time_info.rtp_ts = 0; 
	session->opposite_ssrc = 0;
    session->has_opposite_ssrc = 0;
    session->resend_enable = resend_enable;

    if (tfrc_on) {
		gettimeofday (&now, NULL);
		session->tfrcs=tfrc_sndr_init(now);
        /*add by iantuan*/
        session->tfrcr=tfrc_rcvr_init(now);
        session->tfrc_reject_count = 0;
	}
	else
		session->tfrcs=NULL;
	session->mhdr               = NULL;
	gettimeofday(&(session->last_update), NULL);
	gettimeofday(&(session->last_rtcp_send_time), NULL);
	gettimeofday(&(session->next_rtcp_send_time), NULL);
        session->encryption_enabled = 0;
	session->encryption_algorithm = NULL;
	session->disable_rtcp = false;
    session->fir_fci_seq_num = 0;

	/* Calculate when we're supposed to send our first RTCP packet... */
	tv_add(&(session->next_rtcp_send_time), rtcp_interval(session));

	/* FIX ME: when do we want the first rtcp sent if tfrc_on? - LG*/

	/* Initialise the source database... */
	for (i = 0; i < RTP_DB_SIZE; i++) {
		session->db[i] = NULL;
	}

        /* Initialize sentinels in rr table */
        for (i = 0; i < RTP_DB_SIZE; i++) {
                for (j = 0; j < RTP_DB_SIZE; j++) {
                        session->rr[i][j].next = &session->rr[i][j];
                        session->rr[i][j].prev = &session->rr[i][j];
                }
        }

	/* Create a database entry for ourselves... */
	create_source(session, session->my_ssrc, FALSE);
	cname = get_cname(session->rtp_socket);
	rtp_set_sdes(session, session->my_ssrc, RTCP_SDES_CNAME, cname, strlen(cname));
	free(cname);	/* cname is copied by rtp_set_sdes()... */
	
	/* Corey -EC init */
	if(ec_arg.ctrl){
        session->ea_id      = ec_arg.ea_id;
	}else{
        session->ea_id      = 0;
	}
    session->fec_on = ec_arg.fec_on;
    if(ec_arg.fec_on){
		session->fec_stream = rtp_mulStream_init( (unsigned long)session, ec_arg);
    }else{
		session->fec_stream = NULL;
    }

	return session;
}

/**
 * rtp_set_my_ssrc:
 * @session: the RTP session 
 * @ssrc: the SSRC to be used by the RTP session
 * 
 * This function coerces the local SSRC identifer to be ssrc.  For
 * this function to succeed it must be called immediately after
 * rtp_init or rtp_init_if.  The intended purpose of this
 * function is to co-ordinate SSRC's between layered sessions, it
 * should not be used otherwise.
 *
 * Returns: TRUE on success, FALSE otherwise.  
 */
int 
rtp_set_my_ssrc(struct rtp *session, uint32_t ssrc)
{
        //source *s;
        //uint32_t h;
        char *cname;

        if (session->ssrc_count != 1 && session->sender_count != 0) {
                return FALSE;
        }
        /* Remove existing source */
        //h = ssrc_hash(session->my_ssrc);
        //s = session->db[h];
        //session->db[h] = NULL;
        /* Fill in new ssrc       */
        //session->my_ssrc = ssrc;
        //s->ssrc          = ssrc;
        //h                = ssrc_hash(ssrc);
        /* Put source back        */
        //session->db[h]   = s;

        /*the original source has bug, so I use the function delete_source and create_source-- IanTuan */
        delete_source(session, session->my_ssrc);
        
        session->my_ssrc = ssrc; /*this line must before create_source, or there will be pdb error*/
        create_source(session, session->my_ssrc, FALSE);
        cname = get_cname(session->rtp_socket);
        rtp_set_sdes(session, session->my_ssrc, RTCP_SDES_CNAME, cname, strlen(cname));
        free(cname);

        return TRUE;
}

/**
 * rtp_set_option:
 * @session: The RTP session.
 * @optname: The option name, see #rtp_option.
 * @optval: The value to set.
 *
 * Sets the value of a session option.  See #rtp_option for
 * documentation on the options and their legal values.
 *
 * Returns: TRUE on success, else FALSE.
 */
int 
rtp_set_option(struct rtp *session, rtp_option optname, int optval)
{
	switch (optname) {
		case RTP_OPT_WEAK_VALIDATION:
			session->opt->wait_for_rtcp = optval;
			break;
	        case RTP_OPT_PROMISC:
			session->opt->promiscuous_mode = optval;
			break;
	        case RTP_OPT_FILTER_MY_PACKETS:
			session->opt->filter_my_packets = optval;
			break;
		case RTP_OPT_REUSE_PACKET_BUFS:
			session->opt->reuse_bufs = optval;
			break;
        	default:
			TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "Ignoring unknown option (%d) in call to rtp_set_option().\n", optname);
                        return FALSE;
	}
        return TRUE;
}

/**
 * rtp_get_option:
 * @session: The RTP session.
 * @optname: The option name, see #rtp_option.
 * @optval: The return value.
 *
 * Retrieves the value of a session option.  See #rtp_option for
 * documentation on the options and their legal values.
 *
 * Returns: TRUE and the value of the option in optval on success, else FALSE.
 */
int 
rtp_get_option(struct rtp *session, rtp_option optname, int *optval)
{
	switch (optname) {
		case RTP_OPT_WEAK_VALIDATION:
			*optval = session->opt->wait_for_rtcp;
                        break;
        	case RTP_OPT_PROMISC:
			*optval = session->opt->promiscuous_mode;
                        break;
	        case RTP_OPT_FILTER_MY_PACKETS:
			*optval = session->opt->filter_my_packets;
			break;
		case RTP_OPT_REUSE_PACKET_BUFS:
			*optval = session->opt->reuse_bufs;
			break;
        	default:
                        *optval = 0;
			TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "Ignoring unknown option (%d) in call to rtp_get_option().\n", optname);
                        return FALSE;
	}
        return TRUE;
}

/**
 * rtp_get_userdata:
 * @session: The RTP session.
 *
 * This function returns the userdata pointer that was passed to the
 * rtp_init() or rtp_init_if() function when creating this session.
 *
 * Returns: pointer to userdata.
 */
uint8_t *
rtp_get_userdata(struct rtp *session)
{
	check_database(session);
	return session->userdata;
}

/**
 * rtp_my_ssrc:
 * @session: The RTP Session.
 *
 * Returns: The SSRC we are currently using in this session. Note that our
 * SSRC can change at any time (due to collisions) so applications must not
 * store the value returned, but rather should call this function each time 
 * they need it.
 */
uint32_t 
rtp_my_ssrc(struct rtp *session)
{
	check_database(session);
	return session->my_ssrc;
}

uint8_t rtp_fir_fci_seq_num(struct rtp *session)
{
    uint8_t seq_num;

    seq_num = session->fir_fci_seq_num;
    session->fir_fci_seq_num++;
    session->fir_fci_seq_num%=256;

    return seq_num;
}

static int 
validate_rtp2(rtp_packet *packet, int len, int vlen)
{
	/* Check for valid payload types..... 72-76 are RTCP payload type numbers, with */
	/* the high bit missing so we report that someone is running on the wrong port. */
	if (packet->fields.pt >= 72 && packet->fields.pt <= 76) {
		TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "rtp_header_validation: payload-type invalid");
		if (packet->fields.m) {
			TL_DEBUG_MSG( TL_RTP, DBG_MASK3, " (RTCP packet on RTP port?)");
		}
		TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "\n");
		return FALSE;
	}
	/* Check that the length of the packet is sensible... */
	if (len < (vlen + (4 * packet->fields.cc))) {
		TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "rtp_header_validation: packet length is smaller than the header\n");
		return FALSE;
	}
	/* Check that the amount of padding specified is sensible. */
	/* Note: have to include the size of any extension header! */
	if (packet->fields.p) {
		int	payload_len = len - vlen - (packet->fields.cc * 4);
                if (packet->fields.x) {
                        /* extension header and data */
                        payload_len -= 4 * (1 + packet->meta.extn_len);
                }
                if (packet->meta.data[packet->meta.data_len - 1] > payload_len) {
                        TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "rtp_header_validation: padding greater than payload length\n");
                        return FALSE;
                }
                if (packet->meta.data[packet->meta.data_len - 1] < 1) {
			TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "rtp_header_validation: padding zero\n");
			return FALSE;
		}
        }
	return TRUE;
}

static inline int 
validate_rtp(struct rtp *session, rtp_packet *packet, int len, int vlen)
{
	/* This function checks the header info to make sure that the packet */
	/* is valid. We return TRUE if the packet is valid, FALSE otherwise. */
	/* See Appendix A.1 of the RTP specification.                        */

	/* We only accept RTPv2 packets... */
	if (packet->fields.v != 2) {
		TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "rtp_header_validation: v != 2\n");
		return FALSE;
	}

	if (!session->opt->wait_for_rtcp) {
		/* We prefer speed over accuracy... */
		return TRUE;
	}
	return validate_rtp2(packet, len, vlen);
}


static void 
process_rtp(struct rtp *session, uint32_t curr_rtp_ts, rtp_packet *packet, source *s)
{
    int             i, d, rtt_flag, transit, tfrc_process_flag = 0;
    double          p_prev, p_new;
    rtp_event       event;
    uint16_t        delta;
    uint32_t        now;
    struct timeval  tnow;
    rtp_tfrc_extn   *tfrc_info;
    uint32_t        rtt;
    uint32_t        send_ts;
    
    if (packet->fields.cc > 0) {
        for (i = 0; i < packet->fields.cc; i++) {
            create_source(session, packet->meta.csrc[i], FALSE);
        }
    }

    /* Update the source database... */
    if (s->sender == FALSE) {
        s->sender = TRUE;
        session->sender_count++;
    }

#if 0 
    if (session->tfrc_on) {
	
        if(!(packet->meta.extn))
        {
            TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "[rtp] process rtp error, tfrc on without exten header\n");
            return; 
        }

        if (session->tfrcr==NULL) {
            gettimeofday (&tnow, NULL);
            session->tfrcr=tfrc_rcvr_init(tnow);
		}
        now = get_local_time ();
		
        tfrc_info = (rtp_tfrc_extn*)(packet->meta.extn + 4);

        rtt = ntohl(tfrc_info->rtt);
        send_ts = ntohl(tfrc_info->send_ts);

        /*
        if(packet->fields.pt == 99)
        {
            rtt = tfrc_info->rtt;           
            send_ts = tfrc_info->send_ts;
            TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "recv seq = %d, rtt=%u(0x%08x), send_ts=%u(0x%08x)\n", packet->fields.seq, rtt, rtt, send_ts, send_ts);
        }
        */
        //rtt_flag = packet->fields.pt & 64;    /* disabled by Eric, correct the payload type */

        // Add by Eric
        if (rtt != 0) rtt_flag = 1;
		else rtt_flag = 0;
		    
        delta = tfrc_record_arrival ( session->tfrcr, now, 
                                      packet->fields.seq, 
                                      send_ts, rtt_flag, 
                                      rtt_flag ? rtt:0 );

        p_prev = tfrc_get_p (session->tfrcr);
        p_new  = tfrc_compute_p (session->tfrcr); 
        tfrc_set_p (session->tfrcr, p_new);	

        /* FIX ME: change 0.1 once we have a sensible way of testing */
        if (p_new - p_prev > 0.0000001)  {
            /* send feedback now */
            //printf("\np_new(%f)-p_prev(%f) = %f", p_new, p_prev, p_new - p_prev );
            force_send_rtcp (session);
        }
    }

#else

    /* filter tfrc process condition, frank */
    tfrc_process_flag = 0;
    if(session->tfrc_on) {
        tfrc_process_flag = 1;
        if(!(packet->meta.extn)) {
            //printf("[rtp] process rtp error, tfrc on without exten header\n");
            tfrc_process_flag = 0;
        }

        event.ssrc = packet->fields.ssrc;
        event.type = CHECK_RESEND_PKT;
        event.data = (void *) packet;
        if(session->callback(session, &event) > 0) {
            printf("[rtp.tfrc] it's resend packet, ignore this packet on tfrc\n");
            tfrc_process_flag = 0;
        } 
    }
    else {
        tfrc_process_flag = 0;
    }


    if (tfrc_process_flag) {

        if (session->tfrcr==NULL) {
            gettimeofday (&tnow, NULL);
            session->tfrcr=tfrc_rcvr_init(tnow);
		}
        now =  timeval_to_usec(session->RTP_now);
		
        tfrc_info = (rtp_tfrc_extn*)(packet->meta.extn + 4);

        rtt = ntohl(tfrc_info->rtt);
        send_ts = ntohl(tfrc_info->send_ts);

        // Add by Eric
        if (rtt != 0) rtt_flag = 1;
		else rtt_flag = 0;
		    
        delta = tfrc_record_arrival ( session->tfrcr, now, 
                                      packet->fields.seq, 
                                      send_ts, rtt_flag, 
                                      rtt_flag ? rtt:0 );

        p_prev = tfrc_get_p (session->tfrcr);
        p_new  = tfrc_compute_p (session->tfrcr); 
        tfrc_set_p (session->tfrcr, p_new);	

        /* FIX ME: change 0.1 once we have a sensible way of testing */
        if (p_new - p_prev > 0.0000001)  {
            force_send_rtcp (session);
        }
    }

#endif

    transit    = curr_rtp_ts - packet->fields.ts;
    d      	   = transit - s->transit;
    s->transit = transit;
    if (d < 0) {
        d = -d;
    }
    s->jitter += d - ((s->jitter + 8) / 16);
	
    /* Callback to the application to process the packet... */
    if (!filter_event(session, packet->fields.ssrc)) {
        event.ssrc = packet->fields.ssrc;
        event.type = RX_RTP;
        event.data = (void *) packet;	/* The callback function MUST free this! */
        session->callback(session, &event);
    }
}

void process_fec_rtp(struct rtp *session, uint32_t curr_rtp_ts, rtp_packet *packet, source *s)
{
		/*Corey -FEC, receive fec rtp packet */
	//  printf("->receive FEC packet, seq=%d, pt=%d,  SNBase=%d %d\n",packet->fields.seq, packet->fields.pt, packet->meta.data[2],  packet->meta.data[3]);
	
        rtp_mulStream_recvFec_in(session, session->fec_stream, packet, packet->meta.data, packet->meta.data_len);
		free(packet);
        return ;

}


void
rtp_set_recv_iov(struct rtp *session, struct msghdr *m)
{
	session->mhdr = m;
}

static void 
rtp_recv_data(struct rtp *session, uint32_t curr_rtp_ts)
{
	/* This routine preprocesses an incoming RTP packet, deciding whether to process it. */
	rtp_packet      *packet   = NULL;
	uint8_t	        *buffer   = NULL;
	uint8_t	        *buffer_vlen = NULL;
	int             vlen     = 12;  /* vlen = 12 | 16 | 20 */
	int             buflen = 0; 
	//	            max_buflen = 0; /*added by Eric, as a fixed packet length used in TFRC*/
	source          *s;
    //rtp_tfrc_extn   *tfrc_info;
	
	struct msghdr msg;
    struct iovec entry;
    struct sockaddr_in from_addr;
    struct msg_ctrl control;
    int res;
	
        
	if (!session->opt->reuse_bufs || (packet == NULL)) {
		packet   = (rtp_packet *) malloc(RTP_MAX_PACKET_LEN);
		if (NULL == packet)
		{
			return ;
		}
		buffer   = ((uint8_t *) packet) + offsetof(rtp_packet, fields);
	}
	
	#if USE_RECVMSG
    udp_msg_init(&msg, &entry, &from_addr, &control,buffer, RTP_MAX_PACKET_LEN - offsetof(rtp_packet, fields));
	buflen = udp_recvv(session->rtp_socket, &msg);
    #else
    buflen = udp_recv(session->rtp_socket, buffer, RTP_MAX_PACKET_LEN - offsetof(rtp_packet, fields));
    #endif
    
	
	if (buflen > 0) {
	
		#if USE_RECVMSG
        /* obtain the RTP received time */
        struct timeval RTP_now = udp_recvtime_obtain(&msg);
        #else
        struct timeval RTP_now;
        gettimeofday(&RTP_now,NULL);
        #endif
		
		
       /*
		if (session->encryption_enabled) {
			uint8_t 	 initVec[8] = {0,0,0,0,0,0,0,0};
			(session->decrypt_func)(session->crypto_state, buffer, buflen, initVec);
		}
        */
		/* figure out header lenght based on tfrc_on */
		/* might as well extract rtt and send_ts     */
		vlen = 12;
		/* ian modified : move tfrc related parameters to exten header*/
                //if (session->tfrc_on) {
		//	vlen+=4;
		//	packet->fields.send_ts = ntohl(packet->fields.send_ts);
                //	if (packet->fields.pt & 64) { 
				/* rtt is present in RTP packet */
                //        	vlen += 4; 
		//		packet->fields.rtt = ntohl(packet->fields.rtt);
		//	} else
		//		packet->fields.rtt = 0;	
		//}
		//if(packet->fields.pt == 99)
        //    TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "video packet\n");
        
		buffer_vlen = buffer + vlen;	

		/* Convert header fields to host byte order... */
		packet->fields.seq      = ntohs(packet->fields.seq);
		packet->fields.ts       = ntohl(packet->fields.ts);
		packet->fields.ssrc     = ntohl(packet->fields.ssrc);

		/* Setup internal pointers, etc... */
		if (packet->fields.cc) {
			int	i;
			packet->meta.csrc = (uint32_t *)(buffer_vlen);
			for (i = 0; i < packet->fields.cc; i++) {
				packet->meta.csrc[i] = ntohl(packet->meta.csrc[i]);
			}
		} else {
			packet->meta.csrc = NULL;
		}
		
		if (packet->fields.x) 
        {
			packet->meta.extn      = buffer_vlen + (packet->fields.cc * 4);
			packet->meta.extn_len  = (packet->meta.extn[2] << 8) | packet->meta.extn[3];
			packet->meta.extn_type = (packet->meta.extn[0] << 8) | packet->meta.extn[1];
			
			
		}
        else 
        {
			packet->meta.extn      = NULL;
			packet->meta.extn_len  = 0;
			packet->meta.extn_type = 0;
		}
		
		packet->meta.data     = buffer_vlen + (packet->fields.cc * 4);
		packet->meta.data_len = buflen -  (packet->fields.cc * 4) - vlen;
		
		if (packet->meta.extn != NULL) {
			packet->meta.data += ((packet->meta.extn_len + 1) * 4);
			packet->meta.data_len -= ((packet->meta.extn_len + 1) * 4);
		}
		
		session->RTP_now = RTP_now;
		
		if (validate_rtp(session, packet, buflen, vlen)) {

			if (session->opt->wait_for_rtcp) {
				s = create_source(session, packet->fields.ssrc, TRUE);
			} else {
				s = get_source(session, packet->fields.ssrc);
			}
			
			//Corey - FEC, filter fec stream. Since our fec stream has another sequence number space, we skip the step of sequence number checking. 
			//Otherwise, one media rtp packet will be dropped after one fec packet is received. The drop action is triggered in update_seq()     
			if(session->fec_on &&  packet->fields.pt == get_rtp_mulStream_recvpt(session->fec_stream) ){
				//LOGD("->check rtp packet, %d=%d\n",packet->fields.pt, get_rtp_mulStream_recvpt(session->fec_stream));
				process_fec_rtp(session, curr_rtp_ts, packet, s);
				return;
			}

            
            if(session->ea_id > 0 ){
                //Entering into here means EC module is on so that we should pass video rtp packets through EC module

                //printf("->receive rtp packet, seq=%d, pt=%d\n",packet->fields.seq, packet->fields.pt);
                ea_recvRtp_in(session, (struct ec_agent*) session->ea_id, packet, packet->meta.data, packet->meta.data_len);
            }
			
			

            if (session->tfrc_on) {
                /* FIX ME: this shouldn't change, or it should be avg. -LG */
                //Eric, use fixed packet size for TFRC
                //tfrc_set_rcvr_psize (session->tfrcr,buflen);
                //if(max_buflen < buflen)
                //	max_buflen = buflen;
                tfrc_set_rcvr_psize (session->tfrcr,1420);
            }

			if (session->opt->promiscuous_mode) {

				if (s == NULL) {
					create_source(session, packet->fields.ssrc, FALSE);
					s = get_source(session, packet->fields.ssrc);
				}

				process_rtp(session, curr_rtp_ts, packet, s);
                
                /* We don't free "packet", that's done by the callback function... */
				return; 
			} 

			if (s != NULL) {

				if (s->probation == -1) {
					s->probation = MIN_SEQUENTIAL;
					s->max_seq   = packet->fields.seq - 1;
				}

				if (update_seq(s, packet->fields.seq)) {

					process_rtp(session, curr_rtp_ts, packet, s);

                    /* we don't free "packet", that's done by the callback function... */
					return;	

				} else {

					/* This source is still on probation... */
					TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "RTP packet from probationary source ignored...\n");
				}
			} else {
				/* TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "RTP packet from unknown source ignored\n"); */
			}
		} else {
			session->invalid_rtp_count++;
			TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "Invalid RTP packet discarded\n");
		}

		if (!session->opt->reuse_bufs) {
			free(packet);
		}
	}
}

static int 
validate_rtcp(uint8_t *packet, int len)
{
	/* Validity check for a compound RTCP packet. This function returns */
	/* TRUE if the packet is okay, FALSE if the validity check fails.   */
        /*                                                                  */
	/* The following checks can be applied to RTCP packets [RFC1889]:   */
        /* o RTP version field must equal 2.                                */
        /* o The payload type field of the first RTCP packet in a compound  */
        /*   packet must be equal to SR or RR.                              */
        /* o The padding bit (P) should be zero for the first packet of a   */
        /*   compound RTCP packet because only the last should possibly     */
        /*   need padding.                                                  */
        /* o The length fields of the individual RTCP packets must total to */
        /*   the overall length of the compound RTCP packet as received.    */

	rtcp_t	*pkt  = (rtcp_t *) packet;
	rtcp_t	*end  = (rtcp_t *) (((char *) pkt) + len);
	rtcp_t	*r    = pkt;
	int	 l    = 0;
	int	 pc   = 1;
	int	 p    = 0;
	int      is_okay = TRUE;

	/* All RTCP packets must be compound packets (RFC1889, section 6.1) */
	if (((ntohs(pkt->common.length) + 1) * 4) == len) {
		TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: not a compound packet\n");
		is_okay = FALSE;
	}

	/* The RTCP packet must not be larger than the surrounding UDP packet */
	if (((ntohs(pkt->common.length) + 1) * 4) > len) {
		TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: length exceeds length of container UDP packet\n");
		return FALSE;
	}

	/* Check the RTCP version, payload type and padding of the first in  */
	/* the compund RTCP packet...                                        */
	if (pkt->common.version != 2) {
		TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: version number != 2 in the first sub-packet\n");
		is_okay = FALSE;
	}
	if (pkt->common.p != 0) {
		TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: padding bit is set on first packet in compound\n");
		is_okay = FALSE;
	}
	if ((pkt->common.pt != RTCP_SR) && (pkt->common.pt != RTCP_RR)) {
		if (pkt->common.pt == RTCP_SDES) {
			TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: compound packet starts with SDES not SR or RR\n");
		} else if (pkt->common.pt == RTCP_APP) {
			TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: compound packet starts with APP not SR or RR\n");
		} else if (pkt->common.pt == RTCP_BYE) {
			TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: compound packet starts with BYE not SR or RR\n");
		} else {
			TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: compound packet starts with packet type %d not SR or RR\n", pkt->common.pt);
		}
		is_okay = FALSE;
	}

	/* Check all following parts of the compund RTCP packet. The RTP version */
	/* number must be 2, and the padding bit must be zero on all apart from  */
	/* the last packet. Try to validate the format of each sub-packet.       */
	do {
		if (p == 1) {
			TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: padding bit set in sub-packet %d which is not final sub-packet\n", pc);
			is_okay = FALSE;
		}
		if (r->common.p) {
			p = 1;
		}
		if (r->common.version != 2) {
			TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: version number != 2 in sub-packet %d\n", pc);
			is_okay = FALSE;
		}

		if (pkt->common.pt == RTCP_SR) {
			if (((ntohs(pkt->common.length) + 1) * 4) < (28 + (pkt->common.count * 24))) {
				TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: SR packet is too short (length=%d)\n", ntohs(pkt->common.length));
				is_okay = FALSE;
			} 
		} else if (pkt->common.pt == RTCP_RR) {
			if (((ntohs(pkt->common.length) + 1) * 4) < (8 + (pkt->common.count * 24))) {
				TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: RR packet is too short (length=%d)\n", ntohs(pkt->common.length));
				is_okay = FALSE;
			} 
		} else if (pkt->common.pt == RTCP_SDES) {
			/* Parse and validate the SDES packet */
			int 			count = pkt->common.count;
			struct rtcp_sdes_t 	*sd   = &pkt->r.sdes;
			rtcp_sdes_item 		*rsp; 
			rtcp_sdes_item		*rspn;
			rtcp_sdes_item 		*sdes_end  = (rtcp_sdes_item *) ((uint32_t *)pkt + pkt->common.length + 1);

			while (--count >= 0) {
				rsp = &sd->item[0]; 
				if ((char *) rsp > (char *) end) {
					TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: SDES longer than UDP packet: no terminating null item?\n");
					is_okay = FALSE;
					break;
				}
				if (rsp >= sdes_end) {
					TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: too many SDES items for packet length\n");
					is_okay = FALSE;
					break;
				}
				for (; rsp->type; rsp = rspn ) {
					rspn = (rtcp_sdes_item *)((char*)rsp+rsp->length+2);
					if (rspn == sdes_end) {
						break;
					}
				}
				sd = (struct rtcp_sdes_t *) ((uint32_t *)sd + (((char *)rsp - (char *)sd) >> 2)+1);
			}
			if (count >= 0) {
				TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: malformed SDES packet\n");
			}
		} else if (pkt->common.pt == RTCP_APP) {
			/* No way to validate these? */
		} else if (pkt->common.pt == RTCP_BYE) {
			/* No way to validate these? */
		} else if (pkt->common.pt == PSFB ) {
		    is_okay = TRUE;     // FIXME: Allow PSFB (PLI, FIR,...)
		} 
		l += (ntohs(r->common.length) + 1) * 4;
		r  = (rtcp_t *) (((uint32_t *) r) + ntohs(r->common.length) + 1);
		pc++;	/* count of sub-packets, for debugging... */
	} while (r < end);

	/* Check that the length of the packets matches the length of the UDP */
	/* packet in which they were received...                              */
	if (l != len) {
		TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: RTCP packet length does not match UDP packet length (%d != %d)\n", l, len);
		is_okay = FALSE;
	}
	if (r != end) {
		TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Bogus RTCP packet: RTCP packet length does not match UDP packet length (%p != %p)\n", r, end);
		is_okay = FALSE;
	}

	if (!is_okay) {
		debug_dump(packet, len);
	}

	return is_okay;
}

static void 
process_report_blocks(struct rtp *session, rtcp_t *packet, uint32_t ssrc, rtcp_rr *rrp )
{
	int i;
	rtp_event	 event;
	rtcp_rr		*rr =NULL;
        rtcp_rx		*rx =NULL;
	rtcp_rr_rx      *rrxp=NULL;
    tfrc_report_t   *tfrc_report=NULL;
    uint32_t    rtt=0;


	/* FIX ME: does this make sense?       	     */
	/* I dont want to process my own RRs or SRs  */
	//if (ssrc==session->my_ssrc && session->tfrc_on) // Frank.T mask
	//	return;

	/* ...process RRs... */
	if (packet->common.count == 0) {
		if (!filter_event(session, ssrc)) {
			event.ssrc = ssrc;
			event.type = RX_RR_EMPTY;
			event.data = NULL;
			session->callback(session, &event);
		}
	} else {
		for (i = 0; i < packet->common.count; i++) {
			rr = (rtcp_rr *) malloc(sizeof(rtcp_rr));
			if (NULL == rr)
			{
				return ;
			}
			rr->ssrc          = ntohl(rrp->ssrc);
			rr->fract_lost    = rrp->fract_lost;	/* Endian conversion handled in the */
			//rr->total_lost    = rrp->total_lost;	/* definition of the rtcp_rr type.  */
			rr->total_lost    = ntohl(rrp->total_lost) >> 8;// total_lost is 3-byte but ntohl function converts a 4-byte value, so it's needed to shift one byte right. 	
			rr->last_seq      = ntohl(rrp->last_seq);
			rr->jitter        = ntohl(rrp->jitter);
			rr->lsr           = ntohl(rrp->lsr);
			rr->dlsr          = ntohl(rrp->dlsr);

			if (session->tfrc_on) {  
				/* process RR extensions */
				/* could also use rrp+4 if u think the typecasting is icky - LG */
				rrxp = (rtcp_rr_rx *) rrp;
				rx = (rtcp_rx *) malloc(sizeof(rtcp_rx));
				if (NULL == rx)
				{
					free(rr);
                    return ;
				}
				rx->ts = ntohl(rrxp->ts);
				rx->tdelay = ntohl(rrxp->tdelay);
				rx->x_recv = ntohl(rrxp->x_recv);
				rx->p = ntohl(rrxp->p);   /* i think ? -LG */
				/* update pointer */
				rrxp++;
				rrp = (rtcp_rr *) rrxp;
			} else
				rrp++;
                        

			/* Create a database entry for this SSRC, if one doesn't already exist... */
			create_source(session, rr->ssrc, FALSE);

			/* Store the RR for later use... */
			insert_rr(session, ssrc, rr, rx);

			/* process tfrc feedback */
			if (session->tfrc_on) {
			        struct timeval tnow;
                tfrc_report = (tfrc_report_t *) malloc(sizeof(tfrc_report_t));
				if (NULL == tfrc_report)
				{
                    /* NOTE: rr & rx will free by remove_rr() */
                    //free(rr);
                    //free(rx);
                    return ;
				}
				
				tnow = session->RTCP_now;
				
				rtt = tfrc_process_feedback ( session->tfrcs, tnow, 
							rx->tdelay, rx->x_recv, rx->ts, rx->p );
                tfrc_report->ts     = rx->ts;
                tfrc_report->tdelay = rx->tdelay;
                tfrc_report->x_recv = rx->x_recv;
                tfrc_report->p      = rx->p;
                tfrc_report->rtt    = rtt;
				tfrc_report->ipi    = tfrc_get_ipi(session->tfrcs);
			}

			/* Call the event handler... */
			if (!filter_event(session, ssrc)) {
				event.ssrc = ssrc;
				event.type = RX_RR;
				event.data = (void *) rr;  
				session->callback(session, &event);
			}

            /* Disable by Eric, the original code */
            /*
			if (session->tfrc_on) {
                    event.ssrc = ssrc;
                    event.type = RX_TFRC_RX;
                    event.data = (void *) rx; 
                    session->callback(session, &event);
            }
            */
            
			if (session->tfrc_on) {
                                event.ssrc = ssrc;
                                event.type = RX_TFRC_RX;
                                event.data = (void *) tfrc_report; 
                                session->callback(session, &event);
                                free(tfrc_report);
                                //printf("\nts=%d, tdelay=%d, x_recv=%d, p=%d, rtt=%d\n", tfrc_report->ts, tfrc_report->tdelay, tfrc_report->x_recv, tfrc_report->p, tfrc_report->rtt);
                        }

		}
	}
}

static void 
process_rtcp_sr(struct rtp *session, rtcp_t *packet)
{
	uint32_t	 ssrc;
	rtp_event	 event;
	rtcp_sr		*sr;
	source		*s;
	uint32_t 	tfrc_on=session->tfrc_on;

	ssrc = tfrc_on ? ntohl(packet->r.srx.sr.ssrc):ntohl(packet->r.sr.sr.ssrc);
	s = create_source(session, ssrc, FALSE);
	if (s == NULL) {
		TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Source 0x%08x invalid, skipping...\n", ssrc);
		return;
	}

	/* Mark as an active sender, if we get a sender report... */
	if (s->sender == FALSE) {
		s->sender = TRUE;
		session->sender_count++;
	}

	/* Process the SR... */
	sr = (rtcp_sr *) malloc(sizeof(rtcp_sr));
	if (NULL == sr)
	{
		return ;
	}
	sr->ssrc          = ssrc;
	sr->ntp_sec       = tfrc_on ? ntohl(packet->r.srx.sr.ntp_sec):
					ntohl(packet->r.sr.sr.ntp_sec);
	sr->ntp_frac      = tfrc_on ? ntohl(packet->r.srx.sr.ntp_frac):
					ntohl(packet->r.sr.sr.ntp_frac);
	sr->rtp_ts        = tfrc_on ? ntohl(packet->r.srx.sr.rtp_ts):
					ntohl(packet->r.sr.sr.rtp_ts);
	sr->sender_pcount = tfrc_on ? ntohl(packet->r.srx.sr.sender_pcount):
					ntohl(packet->r.sr.sr.sender_pcount);
	sr->sender_bcount = tfrc_on ? ntohl(packet->r.srx.sr.sender_bcount):
					ntohl(packet->r.sr.sr.sender_bcount);

	/* Store the SR for later retrieval... */
	if (s->sr != NULL) {
		free(s->sr);
	}
	s->sr = sr; 
	ntp64_time(&s->last_sr_sec, &s->last_sr_frac);
	s->last_sr_sec  = tmp_sec;
	s->last_sr_frac = tmp_frac;

	/* Call the event handler... */
	if (!filter_event(session, ssrc)) {
		event.ssrc = ssrc;
		event.type = RX_SR;
		event.data = (void *) sr;
		session->callback(session, &event);
	}

	if (session->tfrc_on) {
		/*rtcp_rx *rx= packet->r.srx.rx;*/
		process_report_blocks(session, packet, ssrc, packet->r.srx.rr);
		/*if (packet->common.count)*/
		/*TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "\nrcv_srx: %d %d %d %d", htonl(rx->ts), htonl(rx->tdelay), htonl(rx->x_recv), htonl(rx->p));*/
	}
	else
	        process_report_blocks(session, packet, ssrc, packet->r.sr.rr);

	/* these numbers must be different for tfrc_on? */
	if (((packet->common.count * 6) + 1) < (ntohs(packet->common.length) - 5)) {
		TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Profile specific SR extension ignored\n");
	}
}

static void 
process_rtcp_rr(struct rtp *session, rtcp_t *packet)
{
	uint32_t		 ssrc;
	source		*s;

	ssrc = ntohl(packet->r.rr.ssrc);
	s = create_source(session, ssrc, FALSE);
	if (s == NULL) {
		TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Source 0x%08x invalid, skipping...\n", ssrc);
		return;
	}

	if (session->tfrc_on) {
		/*rtcp_rx *rx= packet->r.rrx.rx;*/
		process_report_blocks(session, packet, ssrc, packet->r.rrx.rr);
		/*if (packet->common.count)*/
		/*TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "\nrcv_rrx: %d %d %d %d", htonl(rx->ts), htonl(rx->tdelay), htonl(rx->x_recv), htonl(rx->p));*/

	}
	else
		process_report_blocks(session, packet, ssrc, packet->r.rr.rr);

	/* these numbers must be different for tfrc_on? */
	if (((packet->common.count * 6) + 1) < ntohs(packet->common.length)) {
		TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Profile specific RR extension ignored\n");
	}
}


static void 
process_rtcp_sdes(struct rtp *session, rtcp_t *packet)
{
	int 			count = packet->common.count;
	struct rtcp_sdes_t 	*sd   = &packet->r.sdes;
	rtcp_sdes_item 		*rsp; 
	rtcp_sdes_item		*rspn;
	rtcp_sdes_item 		*end  = (rtcp_sdes_item *) ((uint32_t *)packet + packet->common.length + 1);
	source 			*s;
	rtp_event		 event;

	while (--count >= 0) {
		rsp = &sd->item[0];
		if (rsp >= end) {
			break;
		}
		sd->ssrc = ntohl(sd->ssrc);
		s = create_source(session, sd->ssrc, FALSE);
		if (s == NULL) {
			TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Can't get valid source entry for 0x%08x, skipping...\n", sd->ssrc);
		} else {
			for (; rsp->type; rsp = rspn ) {
				rspn = (rtcp_sdes_item *)((char*)rsp+rsp->length+2);
				if (rspn >= end) {
					rsp = rspn;
					break;
				}
				if (rtp_set_sdes(session, sd->ssrc, rsp->type, rsp->data, rsp->length)) {
					if (!filter_event(session, sd->ssrc)) {
						event.ssrc = sd->ssrc;
						event.type = RX_SDES;
						event.data = (void *) rsp;
						session->callback(session, &event);
					}
				} else {
					TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Invalid sdes item for source 0x%08x, skipping...\n", sd->ssrc);
				}
			}
		}
		sd = (struct rtcp_sdes_t *) ((uint32_t *)sd + (((char *)rsp - (char *)sd) >> 2)+1);
	}
	if (count >= 0) {
		TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Invalid RTCP SDES packet, some items ignored.\n");
	}
}

static void 
process_rtcp_bye(struct rtp *session, rtcp_t *packet)
{
	int		 i;
	uint32_t	 ssrc;
	rtp_event	 event;
	source		*s;

	for (i = 0; i < packet->common.count; i++) {
		ssrc = ntohl(packet->r.bye.ssrc[i]);
		/* This is kind-of strange, since we create a source we are about to delete. */
		/* This is done to ensure that the source mentioned in the event which is    */
		/* passed to the user of the RTP library is valid, and simplify client code. */
		create_source(session, ssrc, FALSE);
		/* Call the event handler... */
		if (!filter_event(session, ssrc)) {
			event.ssrc = ssrc;
			event.type = RX_BYE;
			event.data = NULL;
			session->callback(session, &event);
		}
		/* Mark the source as ready for deletion. Sources are not deleted immediately */
		/* since some packets may be delayed and arrive after the BYE...              */
		s = get_source(session, ssrc);
		s->got_bye = TRUE;
		check_source(s);
		session->bye_count++;
	}
}

static void 
process_rtcp_app(struct rtp *session, rtcp_t *packet)
{
	uint32_t          ssrc;
	rtp_event        event;
	rtcp_app         *app;
	source            *s;
	int                  data_len;
    uint8_t            buffer[RTP_MAX_PACKET_LEN];

	/* Update the database for this source. */
	ssrc = ntohl(packet->r.app.ssrc);
	create_source(session, ssrc, FALSE);
	s = get_source(session, ssrc);
	if (s == NULL) {
	        /* This should only occur in the event of database malfunction. */
	        TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Source 0x%08x invalid, skipping...\n", ssrc);
	        return;
	}
	check_source(s);

     memset(buffer, 0, RTP_MAX_PACKET_LEN);

	/* Copy the entire packet, converting the header (only) into host byte order. */
	app = (rtcp_app *) buffer;
	app->version        = packet->common.version;
	app->p              = packet->common.p;
	app->subtype        = packet->common.count;
	app->pt             = packet->common.pt;
	app->length         = ntohs(packet->common.length);
	app->ssrc           = ssrc;
	app->name[0]        = packet->r.app.name[0];
	app->name[1]        = packet->r.app.name[1];
	app->name[2]        = packet->r.app.name[2];
	app->name[3]        = packet->r.app.name[3];
	data_len            = (app->length - 2) * 4;
	memcpy(app->data, packet->r.app.data, data_len);

	/* Callback to the application to process the app packet... */
	if (!filter_event(session, ssrc)) {
		event.ssrc = ssrc;
		event.type = RX_APP;
		event.data = (void *) app;       /* The callback function MUST free this! */
		session->callback(session, &event);
	}
}


/*processing rtcp packets from the retransmission stream*/
static void 
rtp_process_ctrl_retrans(struct rtp *session, uint8_t *buffer, int buflen)
{
    
    uint32_t        ssrc;
	rtcp_t *packet = (rtcp_t *) buffer;
    

            while (packet < (rtcp_t *) (buffer + buflen)) {

				switch (packet->common.pt){
            case RTCP_SR:
                process_rtcp_sr(session, packet);
                break;

            case RTCP_RR:
                process_rtcp_rr(session, packet);
                break;

            case RTCP_SDES:
                process_rtcp_sdes(session, packet);
                break;

            case RTCP_BYE:
                process_rtcp_bye(session, packet);
                break;

            case RTCP_APP:
                process_rtcp_app(session, packet);
                break;

            default: 
                TL_DEBUG_MSG( TL_RTCP, DBG_MASK1, "<RTCP ignored> RTCP packet with unknown type (%d)\n", packet->common.pt);
                break;
        }
        packet = (rtcp_t *) ((char *) packet + (4 * (ntohs(packet->common.length) + 1)));
    }

    if (session->avg_rtcp_size < 0) {
        /* This is the first RTCP packet we've received, set our initial estimate */
        /* of the average  packet size to be the size of this packet.             */
        session->avg_rtcp_size = buflen + RTP_LOWER_LAYER_OVERHEAD;
    } else {
        /* Update our estimate of the average RTCP packet size. The constants are */
        /* 1/16 and 15/16 (section 6.3.3 of draft-ietf-avt-rtp-new-02.txt).       */
        session->avg_rtcp_size = (0.0625 * (buflen + RTP_LOWER_LAYER_OVERHEAD)) + (0.9375 * session->avg_rtcp_size);
    }

#if 0
    /* Signal that we've finished processing this packet */
    if (!filter_event(session, packet_ssrc)) {
        event.ssrc = packet_ssrc;
        event.type = RX_RTCP_FINISH;
        event.data = NULL;
        session->callback(session, &event);
    }
#endif

}

/*processing rtcp packets from the original stream*/
static  void
rtp_process_ctrl_ori(struct rtp *session, uint8_t *buffer, int buflen)
{
	int first = TRUE;
	uint32_t ssrc;
	uint32_t packet_ssrc = rtp_my_ssrc(session);
	rtp_event event;
	rtcp_t *packet = (rtcp_t *) buffer;
	uint8_t tfrc_on = session->tfrc_on;

	while (packet < (rtcp_t *) (buffer + buflen)) {

        switch (packet->common.pt){
                    case RTCP_SR:
                        ssrc = tfrc_on ? ntohl(packet->r.srx.sr.ssrc):ntohl(packet->r.sr.sr.ssrc);
                        if (first && !filter_event(session, ssrc)) {
                            event.ssrc  = ssrc; 
                            event.type  = RX_RTCP_START;
                            event.data  = &buflen;
                            packet_ssrc = event.ssrc;
                            session->callback(session, &event);
                        }
                        process_rtcp_sr(session, packet);
                        break;

                    case RTCP_RR:
                        ssrc = tfrc_on ? ntohl(packet->r.rrx.ssrc):ntohl(packet->r.rr.ssrc);
                        if (first && !filter_event(session, ssrc)) {
                            event.ssrc  = ssrc; 
                            event.type  = RX_RTCP_START;
                            event.data  = &buflen;
                            packet_ssrc = event.ssrc;
                            session->callback(session, &event);
                        }
                        process_rtcp_rr(session, packet);
                        break;

                    case RTCP_SDES:
                        if (first && !filter_event(session, ntohl(packet->r.sdes.ssrc))) {
                            event.ssrc  = ntohl(packet->r.sdes.ssrc);
                            event.type  = RX_RTCP_START;
                            event.data  = &buflen;
                            packet_ssrc = event.ssrc;
                            session->callback(session, &event);
                        }
                        process_rtcp_sdes(session, packet);
                        break;

                    case RTCP_BYE:
                        if (first && !filter_event(session, ntohl(packet->r.bye.ssrc[0]))) {
                            event.ssrc  = ntohl(packet->r.bye.ssrc[0]);
                            event.type  = RX_RTCP_START;
                            event.data  = &buflen;
                            packet_ssrc = event.ssrc;
                            session->callback(session, &event);
                        }
                        process_rtcp_bye(session, packet);
                        break;

                    case RTCP_APP:
                        if (first && !filter_event(session, ntohl(packet->r.app.ssrc))) {
                            event.ssrc  = ntohl(packet->r.app.ssrc);
                            event.type  = RX_RTCP_START;
                            event.data  = &buflen;
                            session->callback(session, &event);
                        }
                        process_rtcp_app(session, packet);
                        break;

#if LINUX_RFC4585_ENABLE
                    case RTPFB: /*RFC4585 Generic Nack message*/
                        TL_DEBUG_MSG(TL_RTCP, DBG_MASK1, "Got RFC4585 Generic Message\n");
                        RFC4585_ProcessGenricNACK(session,packet);
                        break;
#endif
                    case PSFB:
                        TL_DEBUG_MSG(TL_RTCP, DBG_MASK1, "Got PSFB, FMT:%d\n", packet->common.count);
                        event.ssrc  = ntohl(packet->r.app.ssrc);
                        event.type  = RX_RTCP_PSFB;
                        event.data  = (void *)packet;
                        TransportLayer_RTCPCallback(session, &event);
                        break;

                    default: 
                        TL_DEBUG_MSG( TL_RTCP, DBG_MASK1, "<RTCP ignored> RTCP packet with unknown type (%d)\n", packet->common.pt);
                        break;
                }
                packet = (rtcp_t *) ((char *) packet + (4 * (ntohs(packet->common.length) + 1)));
                first  = FALSE;
            }

            if (session->avg_rtcp_size < 0) {
                /* This is the first RTCP packet we've received, set our initial estimate */
                /* of the average  packet size to be the size of this packet.             */
                session->avg_rtcp_size = buflen + RTP_LOWER_LAYER_OVERHEAD;
            } else {
                /* Update our estimate of the average RTCP packet size. The constants are */
                /* 1/16 and 15/16 (section 6.3.3 of draft-ietf-avt-rtp-new-02.txt).       */
                session->avg_rtcp_size = (0.0625 * (buflen + RTP_LOWER_LAYER_OVERHEAD)) + (0.9375 * session->avg_rtcp_size);
            }

			/* Signal that we've finished processing this packet */
			if (!filter_event(session, packet_ssrc)) {
                event.ssrc = packet_ssrc;
                event.type = RX_RTCP_FINISH;
                event.data = NULL;
                session->callback(session, &event);
            }
        } 


static void 
rtp_process_ctrl(struct rtp *ori_session, uint8_t *buffer, int buflen, struct timeval RTCP_now)
{
    /* This routine processes incoming RTCP packets */
    rtcp_t          *packet;
//    uint8_t         initVec[8] = {0,0,0,0,0,0,0,0};
	

    if (buflen > 0) {
/*
		if (session->encryption_enabled)
		{
			(session->decrypt_func)(session->crypto_state, buffer, buflen, initVec);
			buffer += 4;	
			buflen -= 4;
		}
*/
        if (validate_rtcp(buffer, buflen)) 
        {
            packet = (rtcp_t *) buffer;
			
            ori_session->RTCP_now = RTCP_now;
            rtp_process_ctrl_ori(ori_session, buffer, buflen);
            
            
        } 
		else {
            /*all the invalid rtcp packets are counted in the original session*/
            TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Invalid RTCP packet discarded\n");
            ori_session->invalid_rtcp_count++;
        }
    }
}
		
		
/**
 * rtp_recv:
 * @session: the session pointer (returned by rtp_init())
 * @timeout: the amount of time that rtcp_recv() is allowed to block
 * @curr_rtp_ts: the current time expressed in units of the media
 * timestamp.
 *
 * Receive RTP packets and dispatch them.
 *
 * Returns: TRUE if data received, FALSE if the timeout occurred.
 */
int 
rtp_recv(struct rtp *session, struct timeval *timeout, uint32_t curr_rtp_ts)
{
	int	status;

	check_database(session);
	udp_fd_zero();
	udp_fd_set(session->rtp_socket);
	udp_fd_set(session->rtcp_socket);
	status = udp_select(timeout);
	if (status > 0) {
		if (udp_fd_isset(session->rtp_socket)) {
			rtp_recv_data(session, curr_rtp_ts);
		}
		if (udp_fd_isset(session->rtcp_socket)) {
            uint8_t		 buffer[RTP_MAX_PACKET_LEN];
            int		 buflen;
			struct msghdr msg;
            struct iovec entry;
            struct sockaddr_in from_addr;
            struct msg_ctrl control;
            int res;
            struct timeval RTCP_now;

            #if USE_RECVMSG
            udp_msg_init(&msg, &entry, &from_addr, &control,buffer, RTP_MAX_PACKET_LEN);
            buflen = udp_recvv(session->rtcp_socket, &msg);
            
            RTCP_now = udp_recvtime_obtain(&msg);
            #else
			buflen = udp_recv(session->rtcp_socket, buffer, RTP_MAX_PACKET_LEN);
			
			gettimeofday(&RTCP_now,NULL);
            #endif
			
			
			ntp64_time(&tmp_sec, &tmp_frac);
			rtp_process_ctrl(session, buffer, buflen, RTCP_now);
		}
		check_database(session);
                return TRUE;
	} else if (status == 0) {
		//TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "timeout\n");
	} else {
		perror("rtp_recv");
	}
	check_database(session);
        return FALSE;
}


/**
 * rtp_set_sdes:
 * @session: the session pointer (returned by rtp_init()) 
 * @ssrc: the SSRC identifier of a participant
 * @type: the SDES type represented by @value
 * @value: the SDES description
 * @length: the length of the description
 * 
 * Sets session description information associated with participant
 * @ssrc.  Under normal circumstances applications always use the
 * @ssrc of the local participant, this SDES information is
 * transmitted in receiver reports.  Setting SDES information for
 * other participants affects the local SDES entries, but are not
 * transmitted onto the network.
 * 
 * Return value: Returns TRUE if participant exists, FALSE otherwise.
 **/
int 
rtp_set_sdes(struct rtp *session, uint32_t ssrc, rtcp_sdes_type type, const char *value, int length)
{
	source	*s;
	char	*v;

	check_database(session);

	s = get_source(session, ssrc);
	if (s == NULL) {
		TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "Invalid source 0x%08x\n", ssrc);
		return FALSE;
	}
	check_source(s);

	v = (char *) malloc(length + 1);
	if (NULL == v)
	{
		return FALSE;
	}
	memset(v, '\0', length + 1);
	memcpy(v, value, length);

	switch (type) {
		case RTCP_SDES_CNAME: 
			if (s->sdes_cname) free(s->sdes_cname);
			s->sdes_cname = v; 
			break;
		case RTCP_SDES_NAME:
			if (s->sdes_name) free(s->sdes_name);
			s->sdes_name = v; 
			break;
		case RTCP_SDES_EMAIL:
			if (s->sdes_email) free(s->sdes_email);
			s->sdes_email = v; 
			break;
		case RTCP_SDES_PHONE:
			if (s->sdes_phone) free(s->sdes_phone);
			s->sdes_phone = v; 
			break;
		case RTCP_SDES_LOC:
			if (s->sdes_loc) free(s->sdes_loc);
			s->sdes_loc = v; 
			break;
		case RTCP_SDES_TOOL:
			if (s->sdes_tool) free(s->sdes_tool);
			s->sdes_tool = v; 
			break;
		case RTCP_SDES_NOTE:
			if (s->sdes_note) free(s->sdes_note);
			s->sdes_note = v; 
			break;
		case RTCP_SDES_PRIV:
			if (s->sdes_priv) free(s->sdes_priv);
			s->sdes_priv = v; 
			break;
		default :
			TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "Unknown SDES item (type=%d, value=%s)\n", type, v);
                        free(v);
			check_database(session);
			return FALSE;
	}
	check_database(session);
	return TRUE;
}

/**
 * rtp_get_sdes:
 * @session: the session pointer (returned by rtp_init()) 
 * @ssrc: the SSRC identifier of a participant
 * @type: the SDES information to retrieve
 * 
 * Recovers session description (SDES) information on participant
 * identified with @ssrc.  The SDES information associated with a
 * source is updated when receiver reports are received.  There are
 * several different types of SDES information, e.g. username,
 * location, phone, email.  These are enumerated by #rtcp_sdes_type.
 * 
 * Return value: pointer to string containing SDES description if
 * received, NULL otherwise.  
 */
const char *
rtp_get_sdes(struct rtp *session, uint32_t ssrc, rtcp_sdes_type type)
{
	source	*s;

	check_database(session);

	s = get_source(session, ssrc);
	if (s == NULL) {
		TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "Invalid source 0x%08x\n", ssrc);
		return NULL;
	}
	check_source(s);

	switch (type) {
        case RTCP_SDES_CNAME: 
                return s->sdes_cname;
        case RTCP_SDES_NAME:
                return s->sdes_name;
        case RTCP_SDES_EMAIL:
                return s->sdes_email;
        case RTCP_SDES_PHONE:
                return s->sdes_phone;
        case RTCP_SDES_LOC:
                return s->sdes_loc;
        case RTCP_SDES_TOOL:
                return s->sdes_tool;
        case RTCP_SDES_NOTE:
                return s->sdes_note;
	case RTCP_SDES_PRIV:
		return s->sdes_priv;	
        default:
                /* This includes RTCP_SDES_PRIV and RTCP_SDES_END */
                TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Unknown SDES item (type=%d)\n", type);
	}
	return NULL;
}

/**
 * rtp_get_sr:
 * @session: the session pointer (returned by rtp_init()) 
 * @ssrc: identifier of source
 * 
 * Retrieve the latest sender report made by sender with @ssrc identifier.
 * 
 * Return value: A pointer to an rtcp_sr structure on success, NULL
 * otherwise.  The pointer must not be freed.
 **/
const rtcp_sr *
rtp_get_sr(struct rtp *session, uint32_t ssrc)
{
	/* Return the last SR received from this ssrc. The */
	/* caller MUST NOT free the memory returned to it. */
	source	*s;

	check_database(session);

	s = get_source(session, ssrc);
	if (s == NULL) {
		return NULL;
	} 
	check_source(s);
	return s->sr;
}

/**
 * rtp_get_rr:
 * @session: the session pointer (returned by rtp_init())
 * @reporter: participant originating receiver report
 * @reportee: participant included in receiver report
 * 
 * Retrieve the latest receiver report on @reportee made by @reporter.
 * Provides an indication of other receivers reception service.
 * 
 * Return value: A pointer to a rtcp_rr structure on success, NULL
 * otherwise.  The pointer must not be freed.
 **/
const rtcp_rr *
rtp_get_rr(struct rtp *session, uint32_t reporter, uint32_t reportee)
{
	check_database(session);
        return get_rr(session, reporter, reportee);
}

/**
 * rtp_send_data:
 * @session: the session pointer (returned by rtp_init())
 * @rtp_ts: The timestamp reflects the sampling instant of the first octet of the RTP data to be sent.  The timestamp is expressed in media units.
 * @pt: The payload type identifying the format of the data.
 * @m: Marker bit, interpretation defined by media profile of payload.
 * @cc: Number of contributing sources (excluding local participant)
 * @csrc: Array of SSRC identifiers for contributing sources.
 * @data: The RTP data to be sent.
 * @data_len: The size @data in bytes.
 * @extn: Extension data (if present).
 * @extn_len: size of @extn in bytes.
 * @extn_type: extension type indicator.
 * 
 * Send an RTP packet.  Most media applications will only set the
 * @session, @rtp_ts, @pt, @m, @data, @data_len arguments.
 *
 * Mixers and translators typically set additional contributing sources 
 * arguments (@cc, @csrc).
 *
 * Extensions fields (@extn, @extn_len, @extn_type) are for including
 * application specific information.  When the widest amount of
 * inter-operability is required these fields should be avoided as
 * some applications discard packets with extensions they do not
 * recognize.
 * 
 * Return value: Number of bytes transmitted.
 **/
int 
rtp_send_data(struct rtp *session, uint32_t rtp_ts, char pt, int m, 
		  int cc, uint32_t* csrc, 
                  unsigned char *data, int data_len, uint16_t *sent_seq,
		  unsigned char *extn, uint16_t extn_len, uint16_t extn_type)
{
	return rtp_send_data_hdr(session, -1, rtp_ts, pt, m, cc, csrc, NULL, 0, data, data_len, sent_seq, extn, extn_len, extn_type);
}

int rtp_resend_data(struct rtp *session, uint16_t seq, uint32_t rtp_ts, char pt, int m, 
		  int cc, uint32_t* csrc, 
                  unsigned char *data, int data_len, 
		  unsigned char *extn, uint16_t extn_len, uint16_t extn_type)
{
       long sequence_num;
       
       sequence_num = seq;
       
       return rtp_send_data_hdr(session, sequence_num, rtp_ts, pt, m, cc, csrc, NULL, 0, data, data_len, NULL, extn, extn_len, extn_type);
		  
}

int 
rtp_send_data_hdr(struct rtp *session, long sequence_num,
                  uint32_t rtp_ts, char pt, int m, 
                  int cc, uint32_t csrc[], 
                  unsigned char *phdr, int phdr_len, 
                  unsigned char *data, int data_len, uint16_t *sent_seq,
                  unsigned char *extn, uint16_t extn_len, uint16_t extn_type)
{
	int			 	vlen, buffer_len, i, rc, pad, pad_len;
	//int			 	max_rc = 0; /*added by Eric, as a fixed packet length used in TFRC*/
	uint8_t		 	*buffer = NULL;
    rtp_packet	 	*packet = NULL;
	//uint8_t 	 	initVec[8] = {0,0,0,0,0,0,0,0};
	struct iovec 	send_vector[3];
	int			 	send_vector_len;
    rtp_tfrc_extn   tfrc_info;
    uint32_t 		ipi, send_time, now;

	static uint32_t last_now=0;  //debug
    uint32_t 		avg_rtt;
    double 			p = 0.0;
//Eric, one-way test for TFRC
//[Eric]return 0;
	
	if((extn && !extn_len) || (!extn && extn_len))
	{
	     TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "rtp send data parameters error\n");
	     return (0);
	}
	
	if (session->tfrc_on) {
                now = get_local_time();

		/* print message for debuging */
		if(last_now==0){
		    last_now = now;
		}
		if(now - last_now > 1000000) {
		    ipi = tfrc_get_ipi(session->tfrcs);
                    p = tfrc_get_sndr_p(session->tfrcs);
                    avg_rtt = tfrc_get_avg_rtt(session->tfrcs);
		    TL_DEBUG_MSG( TL_RTP, DBG_MASK2, "\n\t tfrc_ipi = %u (us), tfrc_send_rate = %u (kbps) avg_rtt=%u  p=%f\n", ipi, 11200000/ipi,avg_rtt, p);
		    last_now = now;
		}		


		if (!tfrc_send_ok (session->tfrcs,  now)){
                        session->tfrc_reject_count++;
                        if(session->tfrc_reject_count > 20)
                       {
                            send_time = tfrc_get_send_time (session->tfrcs);
                            ipi = tfrc_get_ipi(session->tfrcs);
                            TL_DEBUG_MSG( TL_RTP, DBG_MASK2, "tfrc sender ipi=%u send_time=%u now=%u\n",ipi, send_time, now);
                             session->tfrc_reject_count = 0;
                       }
			//Eric, do not send out
			//TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "X");	
			return (0);
		}
	}
        session->tfrc_reject_count = 0;

	//Eric, send a packet out
	//TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "P");
	check_database(session);

	//assert((data == NULL && data_len == 0) || (data != NULL && data_len > 0));
    if( (data == NULL) || (data_len == 0))
    {
         TL_DEBUG_MSG(TL_RTP, DBG_MASK1, "rtp_send_data_hdr data %s == NULL or data_len %d == 0\n", data, data_len);         
         return (0);
    }

	vlen=12;
	
	/* ian modified : we must move the TFRC related fields to the exten header*/
        //if (session->tfrc_on) {
	//	vlen+=4;
		//if (tfrc_rtt_flag(session->tfrcs))    /* Disabled by Eric Fang, correct the payload type */
	//		vlen+=4;
	//}
	
	buffer_len = vlen + (4 * cc);

    if((session->tfrc_on) || (extn != NULL))
    {
         buffer_len += 4;
    }
    
    if (session->tfrc_on) 
    {
        buffer_len += 8;  /*two 32-bit words*/
	}
    
	if (extn != NULL) 
    {
		buffer_len += (extn_len) * 4;
    }

	/* Do we need to pad this packet to a multiple of 64 bits? */
	/* This is only needed if encryption is enabled, since DES */
	/* only works on multiples of 64 bits. We just calculate   */
	/* the amount of padding to add here, so we can reserve    */
	/* space - the actual padding is added later.              */
#ifdef NDEF
	/* FIXME: This is broken, due to scatter send [csp]        */               /* FIXME */
	/*
   if ((session->encryption_enabled) &&
	    ((buffer_len % session->encryption_pad_length) != 0)) {
		pad         = TRUE;
		pad_len     = session->encryption_pad_length - (buffer_len % session->encryption_pad_length);
		buffer_len += pad_len; 
		assert((buffer_len % session->encryption_pad_length) == 0);
	} else {
		*/
        pad     = FALSE;
		pad_len = 0;
/*
	}
*/
#endif
	pad     = FALSE;	/* FIXME */
	pad_len = 0;
    //int test;
    //test = offsetof(rtp_packet, fields);
    
	/* Allocate memory for the packet... */
	if (buffer == NULL) {
		assert(buffer_len < RTP_MAX_PACKET_LEN);
		/* we dont always need 20 (12|16) but this seems to work. LG */
		buffer = (uint8_t *) malloc(40 + offsetof(rtp_packet, fields)+extn_len*4);
		if (NULL == buffer)
		{
			return FALSE;
		}
		packet = (rtp_packet *) buffer;
	}
        
        assert(buffer != NULL);
        
	send_vector[0].iov_base = buffer + offsetof(rtp_packet, fields);
	send_vector[0].iov_len  = buffer_len;
	send_vector_len = 1;

	/* These are internal pointers into the buffer... */
//#ifdef NDEF
	packet->meta.csrc = (uint32_t *) (buffer + offsetof(rtp_packet, fields) + vlen);
	packet->meta.extn = (uint8_t  *) (buffer + offsetof(rtp_packet, fields) + vlen + (4 * cc));
	packet->meta.data = (uint8_t  *) (buffer + offsetof(rtp_packet, fields) + vlen + (4 * cc));
	
	if(session->tfrc_on)
	    packet->meta.data += 12;
	
	if (extn != NULL) {
	        if(session->tfrc_on)
		    packet->meta.data += (extn_len) * 4;
		else
		    packet->meta.data += (extn_len + 1)*4;
	}
//#endif
	/* ...and the actual packet header... */
	packet->fields.v    = 2;
	packet->fields.p    = pad;
	packet->fields.x    = (session->tfrc_on ||(extn != NULL));/* ian modified : the tfrc fields also moved to exten header*/
	packet->fields.cc   = cc;
	packet->fields.m    = m;
	packet->fields.pt   = pt;
	
	if(sequence_num < 0)
   {
	    packet->fields.seq  = session->rtp_seq++;
        
        if(sent_seq)
            *sent_seq = packet->fields.seq;

        packet->fields.seq = htons(packet->fields.seq);
    }
	else
	    packet->fields.seq = htons(sequence_num);
    
	packet->fields.ts   = htonl(rtp_ts);
	packet->fields.ssrc = htonl(session->my_ssrc);
    
	
     /* ... do tfrc stuff... */
	//if (session->tfrc_on) {
	//	packet->fields.send_ts = htonl(get_local_time());
	//	if (tfrc_rtt_flag(session->tfrcs)) { 
	//		packet->fields.rtt = htonl(tfrc_get_avg_rtt(session->tfrcs));
	//		tfrc_set_rtt_flag (session->tfrcs, 0);
			//packet->fields.pt = packet->fields.pt | 64;   /* disabled by Eric, correct the payload type */
	//	}	
	//	else {
			//Added by Eric, do not change the RTP payload type
	//	    packet->fields.rtt = 0;
	//		packet->fields.rtt = htonl(packet->fields.rtt);	
		    //packet->fields.pt = packet->fields.pt & 63;   /* disabled by Eric, correct the payload type */
	//	}
	//}			
												
	/* ...now the CSRC list... */
	for (i = 0; i < cc; i++) {
		packet->meta.csrc[i] = htonl(csrc[i]);
	}
	/* ...a header extension? */
	/* ian modified : move the tfrc related parameters to the exten header*/
	if ((session->tfrc_on) || (extn != NULL)) {
		/* We don't use the packet->meta.extn_type field here, that's for receive only... */
		uint16_t *base = (uint16_t *) packet->meta.extn;
		
		if(session->tfrc_on)
		{
		     extn_len += 2; /*add two more words : send_ts and rtt*/  

             //TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "seq = %d\n", packet->fields.seq);
            
		     if(tfrc_rtt_flag(session->tfrcs))
		     {
                  tfrc_info.rtt = tfrc_get_avg_rtt(session->tfrcs);
                  //TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "send tfrc_info.rtt = %u\n", tfrc_info.rtt);
		          //tfrc_info.rtt = htonl(tfrc_info.rtt);
		          tfrc_set_rtt_flag (session->tfrcs, 0);
                  
		     }
		     else
		     {
		          tfrc_info.rtt = 0;
                  //TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "send tfrc_info.rtt = %u\n", tfrc_info.rtt);
			      //tfrc_info.rtt = htonl(tfrc_info.rtt);	
		     }
            
		     tfrc_info.rtt = htonl(tfrc_info.rtt);
		     tfrc_info.send_ts = htonl(get_local_time());
		      //tfrc_info.send_ts = get_local_time();

              //if(packet->fields.pt == 99)
              //    TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "send seq = %d, rtt=%u(0x%08x), send_ts=%u(0x%08x)\n", session->rtp_seq - 1, tfrc_info.rtt, tfrc_info.rtt, tfrc_info.send_ts, tfrc_info.send_ts);

              memcpy(packet->meta.extn + 4, &tfrc_info, sizeof(rtp_tfrc_extn));
		      //TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "(0x%08x) (0x%08x)\n", *(unsigned long *)(packet->meta.extn + 4), *(unsigned long *)(packet->meta.extn + 8));

		}
		
		base[0] = htons(extn_type);
		base[1] = htons(extn_len);
		
        if(extn)
       {
		    if(session->tfrc_on)
		        memcpy(packet->meta.extn + 12, extn, (extn_len-2)*4);
		       // memcpy(packet->meta.extn + 12, extn, extn_len*4);
		    else
		        memcpy(packet->meta.extn + 4, extn, extn_len*4);
        }
	}
	
	//Corey -- set correct extn_len
	packet->meta.extn_len = extn_len;
	
	/* ...the payload header... */
	if (phdr != NULL) {
		send_vector[send_vector_len].iov_base = phdr;
		send_vector[send_vector_len].iov_len  = phdr_len;
		send_vector_len++;
	}
	/* ...and the media data... */
	if (data_len > 0) {
		send_vector[send_vector_len].iov_base = data;
		send_vector[send_vector_len].iov_len  = data_len;
		send_vector_len++;
	}

#ifdef NDEF		/* FIXME */
	/* ...and any padding... */
	if (pad) {
		for (i = 0; i < pad_len; i++) {
			buffer[buffer_len + offsetof(rtp_packet, fields) - pad_len + i] = 0;
		}
		buffer[buffer_len + offsetof(rtp_packet, fields) - 1] = (char) pad_len;
	}
#endif
	
	/* Finally, encrypt if desired... */
   /*
	if (session->encryption_enabled) {
		assert((buffer_len % session->encryption_pad_length) == 0);
		(session->encrypt_func)(session->crypto_state, buffer + offsetof(rtp_packet, fields), buffer_len, initVec); 
	}
   */

 
    //TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "send rtp seq=%d,ts=%d, marker=%d\n",ntohs(packet->fields.seq), rtp_ts, m);

    /*if((pt > 105)||(pt == 99)) { // video only
        qdbg_monitor_cnt.video_before_network_pkt_cnt++;
        qdbg_monitor_cnt.video_before_network_len_cnt += data_len; 

        if(rtp_ts != qdbg_noclean_cnt.video_last_rtp_time) 
            qdbg_monitor_cnt.video_before_network_frame_cnt++;

        qdbg_noclean_cnt.video_last_rtp_time = rtp_ts;      
    }*/

	rc = udp_sendv(session->rtp_socket, send_vector, send_vector_len);
	if (rc == -1) {
		perror("sending RTP packet");
	}

	/* Update the RTCP statistics... */
	session->we_sent     = TRUE;
	session->rtp_pcount += 1;
	session->rtp_bcount += buffer_len;
	gettimeofday(&session->last_rtp_send_time, NULL);

	if (session->tfrc_on) {
		/* FIX ME: this shoule either not change or be averaged - LG */
		
	//Eric, use fixed packet size for TFRC
	//	tfrc_set_sndr_psize (session->tfrcs, rc);
		//if(max_rc < rc)
		//	max_rc = rc;
		tfrc_set_sndr_psize (session->tfrcs, 1420);
	//Eric, show the packet_size
	//[Eric] TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "(%lu)", max_rc);
	}
	
	//Corey - EC, pass  packets through EC module
	if( session->fec_on && get_rtp_mulStream_sendpt(session->fec_stream) != pt){
        ea_sendRtp_in(session, (struct agent*)session->ea_id, (rtp_packet *) buffer , data, data_len);
	}

	check_database(session);
    
        free(buffer);
	
	return rc;
}
/**
 *Corey -FEC, Send FEC RTP packet out
 *
 */
int 
rtp_send_fec_hdr(struct rtp *session, long sequence_num,
                  uint32_t rtp_ts, char pt, int m, 
                  int cc, uint32_t csrc[], 
                  unsigned char *phdr, int phdr_len, 
                  unsigned char *data, int data_len, uint16_t *sent_seq,
                  unsigned char *extn, uint16_t extn_len, uint16_t extn_type)
{
	int			 	vlen, buffer_len, i, rc, pad, pad_len;
	//int			 	max_rc = 0; /*added by Eric, as a fixed packet length used in TFRC*/
	uint8_t		 	*buffer = NULL;
    rtp_packet	 	*packet = NULL;
	//uint8_t 	 	initVec[8] = {0,0,0,0,0,0,0,0};
	struct iovec 	send_vector[3];
	int			 	send_vector_len;
    rtp_tfrc_extn   tfrc_info;
    uint32_t 		ipi, send_time, now;
	static uint32_t last_now=0;  //debug
    uint32_t 		avg_rtt;
    double 			p = 0.0;
	
	if((extn && !extn_len) || (!extn && extn_len))
	{
	     TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "rtp send data parameters error\n");
	     return (0);
	}
	

	//TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "P");
	check_database(session);

	//assert((data == NULL && data_len == 0) || (data != NULL && data_len > 0));
    if( (data == NULL) || (data_len == 0))
    {
         TL_DEBUG_MSG(TL_RTP, DBG_MASK1, "rtp_send_fec_hdr data %s == NULL or data_len %d == 0\n", data, data_len);         
         return (0);
    }

	vlen=12;
	
	
	buffer_len = vlen + (4 * cc);

    if((extn != NULL))
    {
        buffer_len += 4;
		buffer_len += (extn_len) * 4;
    }
    

	pad     = FALSE;	/* FIXME */
	pad_len = 0;
    
	/* Allocate memory for the packet... */
	if (buffer == NULL) {
		assert(buffer_len < RTP_MAX_PACKET_LEN);
		/* we dont always need 20 (12|16) but this seems to work. LG */
		buffer = (uint8_t *) malloc(40 + offsetof(rtp_packet, fields)+extn_len*4);
		if (NULL == buffer)
		{
			return FALSE;
		}
		packet = (rtp_packet *) buffer;
	}
        
        assert(buffer != NULL);
        
	send_vector[0].iov_base = buffer + offsetof(rtp_packet, fields);
	send_vector[0].iov_len  = buffer_len;
	send_vector_len = 1;

	/* These are internal pointers into the buffer... */
//#ifdef NDEF
	packet->meta.csrc = (uint32_t *) (buffer + offsetof(rtp_packet, fields) + vlen);
	packet->meta.extn = (uint8_t  *) (buffer + offsetof(rtp_packet, fields) + vlen + (4 * cc));
	packet->meta.data = (uint8_t  *) (buffer + offsetof(rtp_packet, fields) + vlen + (4 * cc));
	
	
	if (extn != NULL) {
		    packet->meta.data += (extn_len + 1)*4;
	}
//#endif
	/* ...and the actual packet header... */
	packet->fields.v    = 2;
	packet->fields.p    = pad;
	packet->fields.x    = ((extn != NULL));/* ian modified : the tfrc fields also moved to exten header*/
	packet->fields.cc   = cc;
	packet->fields.m    = m;
	packet->fields.pt   = pt;

	packet->fields.seq = htons(sequence_num);
    
	packet->fields.ts   = htonl(rtp_ts);
	packet->fields.ssrc = htonl(session->my_ssrc);
    
	

												
	/* ...now the CSRC list... */
	for (i = 0; i < cc; i++) {
		packet->meta.csrc[i] = htonl(csrc[i]);
	}
	/* ...a header extension? */
	/* ian modified : move the tfrc related parameters to the exten header*/
	if ( (extn != NULL)) {
		/* We don't use the packet->meta.extn_type field here, that's for receive only... */
		uint16_t *base = (uint16_t *) packet->meta.extn;
		
		
		
		base[0] = htons(extn_type);
		base[1] = htons(extn_len);
		
		//Corey -- set correct extn_len
		packet->meta.extn_len = extn_len;

        memcpy(packet->meta.extn + 4, extn, extn_len*4);
	}
	
	/* ...the payload header... */
	if (phdr != NULL) {
		send_vector[send_vector_len].iov_base = phdr;
		send_vector[send_vector_len].iov_len  = phdr_len;
		send_vector_len++;
	}
	/* ...and the media data... */
	if (data_len > 0) {
		send_vector[send_vector_len].iov_base = data;
		send_vector[send_vector_len].iov_len  = data_len;
		send_vector_len++;
	}

#ifdef NDEF		/* FIXME */
	/* ...and any padding... */
	if (pad) {
		for (i = 0; i < pad_len; i++) {
			buffer[buffer_len + offsetof(rtp_packet, fields) - pad_len + i] = 0;
		}
		buffer[buffer_len + offsetof(rtp_packet, fields) - 1] = (char) pad_len;
	}
#endif
	



	rc = udp_sendv(session->rtp_socket, send_vector, send_vector_len);
	if (rc == -1) {
		perror("sending RTP packet");
	}
	



	check_database(session);
    
    free(buffer);
	
	return rc;
}

static int 
format_report_blocks(rtcp_rr *rrp, int remaining_length, struct rtp *session)
{
	int nblocks = 0;
	int h;
	source *s;
	uint32_t	now_sec;
	uint32_t	now_frac;
	rtcp_rr_rx      *rrxp;
    uint32_t    rrxp_tdelay, rrxp_x_recv, rrxp_ts, rrxp_p;

	for (h = 0; h < RTP_DB_SIZE; h++) {
		for (s = session->db[h]; s != NULL; s = s->next) {
			check_source(s);
			if ((nblocks == 31) || (remaining_length < 24)) {
				break; /* Insufficient space for more report blocks... */
			}

			if (s->sender) {
				/* Much of this is taken from A.3 of draft-ietf-avt-rtp-new-01.txt */
				int	extended_max      = s->cycles + s->max_seq;
       				int	expected          = extended_max - s->base_seq + 1;
       				uint32_t	lost              = expected - s->received;
				int	expected_interval = expected - s->expected_prior;
       				int	received_interval = s->received - s->received_prior;
       				int 	lost_interval     = expected_interval - received_interval;
				uint32_t	fraction;
				uint32_t lsr;
				uint32_t dlsr;

				//TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "lost_interval %d\n", lost_interval);
       				s->expected_prior = expected;
       				s->received_prior = s->received;
       				if (expected_interval == 0 || lost_interval <= 0) {
					fraction = 0;
       				} else {
					fraction = (lost_interval << 8) / expected_interval;
				}

				ntp64_time(&now_sec, &now_frac);
				if (s->sr == NULL) {
					lsr = 0;
					dlsr = 0;
				} else {
					lsr = ntp64_to_ntp32(s->sr->ntp_sec, s->sr->ntp_frac);
					dlsr = ntp64_to_ntp32(now_sec, now_frac) - ntp64_to_ntp32(s->last_sr_sec, s->last_sr_frac);
				}
				rrp->ssrc       = htonl(s->ssrc);
				rrp->fract_lost = fraction;
				
				//rrp->total_lost = lost & 0x00ffffff;
				rrp->total_lost = htonl(lost) >> 8;
				

				rrp->last_seq   = htonl(extended_max);
				rrp->jitter     = htonl(s->jitter / 16);
				rrp->lsr        = htonl(lsr);
				rrp->dlsr       = htonl(dlsr);

				/* ... if tfrc_on generate an RX too ... */
                if (session->tfrc_on) {
					uint32_t now    = get_local_time();
					rrxp = (rtcp_rr_rx *) rrp;
					
					/* FIXME: add the needed ntohls */
                    /*
					tfrc_format_feedback ( session->tfrcr, 
								now,
								&(rrxp->tdelay),
								&(rrxp->x_recv),
								&(rrxp->ts),
								&(rrxp->p));
                    */
                    tfrc_format_feedback ( session->tfrcr,
                                now,
                                &rrxp_tdelay,
                                &rrxp_x_recv,
                                &rrxp_ts,
                                &rrxp_p);
                                
                    rrxp->tdelay = htonl(rrxp_tdelay);
					rrxp->x_recv = htonl(rrxp_x_recv);
					rrxp->ts = htonl(rrxp_ts);
					rrxp->p = htonl(rrxp_p);
                                	/* update pointer */
                                	rrxp++;
                                	rrp = (rtcp_rr *) rrxp;
					remaining_length -= (24+16);
                        	} else {
                           		rrp++;
					remaining_length -= 24;
				}

				nblocks++;

				/* comment following line since that will cause the the report block doesn't include in the RTCP receiver packet */ /*add by willy, 2010-10-01 */
				//s->sender = FALSE;
				session->sender_count--;
				if (session->sender_count == 0) {
					break; /* No point continuing, since we've reported on all senders... */
				}
			}
		}
	}
	return nblocks;
}

static uint8_t *
format_rtcp_sr(uint8_t *buffer, int buflen, struct rtp *session, uint32_t rtp_ts)
{
	/* Write an RTCP SR into buffer, returning a pointer to */
	/* the next byte after the header we have just written. */
	rtcp_t		*packet = (rtcp_t *) buffer;
	int		     remaining_length;
	//uint32_t	 ntp_sec, ntp_frac;
    rtp_event event;

	assert(buflen >= 28);	/* ...else there isn't space for the header and sender report */

	packet->common.version = 2;
	packet->common.p       = 0;
	packet->common.count   = 0;
	packet->common.pt      = RTCP_SR;
	packet->common.length  = htons(1);

    //ntp64_time(&ntp_sec, &ntp_frac);

	packet->r.sr.sr.ssrc          = htonl(rtp_my_ssrc(session));

/* 
	packet->r.sr.sr.ntp_sec       = htonl(ntp_sec);
	packet->r.sr.sr.ntp_frac      = htonl(ntp_frac);
	packet->r.sr.sr.rtp_ts        = htonl(rtp_ts);
*/
    /*ian modified : for willey's lip sync function*/
    /*use callback to trigger the intellegient control set rtp time info*/
    event.ssrc  = session->my_ssrc; 
	event.type  = APP_SET_TIME_INFO;
	event.data  = NULL;
    session->callback(session, &event);

    packet->r.sr.sr.ntp_sec       = htonl(session->ntp_time_info.ntp_sec);
	packet->r.sr.sr.ntp_frac      = htonl(session->ntp_time_info.ntp_frac);
	packet->r.sr.sr.rtp_ts        = htonl(session->ntp_time_info.rtp_ts);

	packet->r.sr.sr.sender_pcount = htonl(session->rtp_pcount);
	packet->r.sr.sr.sender_bcount = htonl(session->rtp_bcount);

	/* Add report blocks, until we either run out of senders */
	/* to report upon or we run out of space in the buffer.  */
	remaining_length = buflen - 28;
	
	if (session->tfrc_on) {
		packet->common.count = format_report_blocks(packet->r.srx.rr, remaining_length, session);
		packet->common.length = htons((uint16_t) (6 + (packet->common.count * (6+4))));
        	return buffer + 28 + ((24+16) * packet->common.count);
	} else {
		packet->common.count = format_report_blocks(packet->r.sr.rr, remaining_length, session);
		packet->common.length = htons((uint16_t) (6 + (packet->common.count * 6)));
		return buffer + 28 + (24 * packet->common.count);
	}
}

static uint8_t *
format_rtcp_rr(uint8_t *buffer, int buflen, struct rtp *session)
{
	/* Write an RTCP RR into buffer, returning a pointer to */
	/* the next byte after the header we have just written. */
	rtcp_t		*packet = (rtcp_t *) buffer;
	int		 remaining_length;

	assert(buflen >= 8);	/* ...else there isn't space for the header */

	packet->common.version = 2;
	packet->common.p       = 0;
	packet->common.count   = 0;
	packet->common.pt      = RTCP_RR;
	packet->common.length  = htons(1);
	packet->r.rr.ssrc      = htonl(session->my_ssrc);

	/* Add report blocks, until we either run out of senders */
	/* to report upon or we run out of space in the buffer.  */
	remaining_length = buflen - 8;
	if (session->tfrc_on) {
		packet->common.count = format_report_blocks(packet->r.rrx.rr, remaining_length, session);
		packet->common.length = htons((uint16_t) (1 + (packet->common.count * (6+4))));
		return buffer + 8 + ((24+16) * packet->common.count);
	} else {
		packet->common.count = format_report_blocks(packet->r.rr.rr, remaining_length, session);
		packet->common.length = htons((uint16_t) (1 + (packet->common.count * 6)));
		return buffer + 8 + (24 * packet->common.count);
	}
}

static int 
add_sdes_item(uint8_t *buf, int buflen, int type, const char *val)
{
	/* Fill out an SDES item. It is assumed that the item is a NULL    */
	/* terminated string.                                              */
        rtcp_sdes_item *shdr = (rtcp_sdes_item *) buf;
        int             namelen;

        if (val == NULL) {
                TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, "Cannot format SDES item. type=%d val=%s\n", type, val);
                return 0;
        }
        shdr->type = type;
        namelen = strlen(val);
        shdr->length = namelen;
        strncpy(shdr->data, val, buflen - 2); /* The "-2" accounts for the other shdr fields */
        return namelen + 2;
}

static uint8_t *
format_rtcp_sdes(uint8_t *buffer, int buflen, uint32_t ssrc, struct rtp *session)
{
        /* From draft-ietf-avt-profile-new-00:                             */
        /* "Applications may use any of the SDES items described in the    */
        /* RTP specification. While CNAME information is sent every        */
        /* reporting interval, other items should be sent only every third */
        /* reporting interval, with NAME sent seven out of eight times     */
        /* within that slot and the remaining SDES items cyclically taking */
        /* up the eighth slot, as defined in Section 6.2.2 of the RTP      */
        /* specification. In other words, NAME is sent in RTCP packets 1,  */
        /* 4, 7, 10, 13, 16, 19, while, say, EMAIL is used in RTCP packet  */
        /* 22".                                                            */
	uint8_t		*packet = buffer;
	rtcp_common	*common = (rtcp_common *) buffer;
	const char	*item;
	size_t		 remaining_len;
        int              pad;

	assert(buflen > (int) sizeof(rtcp_common));

	common->version = 2;
	common->p       = 0;
	common->count   = 1;
	common->pt      = RTCP_SDES;
	common->length  = 0;
	packet += sizeof(common);

	*((uint32_t *) packet) = htonl(ssrc);
	packet += 4;

	remaining_len = buflen - (packet - buffer);
	item = rtp_get_sdes(session, ssrc, RTCP_SDES_CNAME);
	if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len)) {
		packet += add_sdes_item(packet, remaining_len, RTCP_SDES_CNAME, item);
	}

	remaining_len = buflen - (packet - buffer);
	item = rtp_get_sdes(session, ssrc, RTCP_SDES_NOTE);
	if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len)) {
		packet += add_sdes_item(packet, remaining_len, RTCP_SDES_NOTE, item);
	}

	remaining_len = buflen - (packet - buffer);
	if ((session->sdes_count_pri % 3) == 0) {
		session->sdes_count_sec++;
		if ((session->sdes_count_sec % 8) == 0) {
			/* Note that the following is supposed to fall-through the cases */
			/* until one is found to send... The lack of break statements in */
			/* the switch is not a bug.                                      */
			switch (session->sdes_count_ter % 5) {
			case 0: item = rtp_get_sdes(session, ssrc, RTCP_SDES_TOOL);
				if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len)) {
					packet += add_sdes_item(packet, remaining_len, RTCP_SDES_TOOL, item);
					break;
				}
			case 1: item = rtp_get_sdes(session, ssrc, RTCP_SDES_EMAIL);
				if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len)) {
					packet += add_sdes_item(packet, remaining_len, RTCP_SDES_EMAIL, item);
					break;
				}
			case 2: item = rtp_get_sdes(session, ssrc, RTCP_SDES_PHONE);
				if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len)) {
					packet += add_sdes_item(packet, remaining_len, RTCP_SDES_PHONE, item);
					break;
				}
			case 3: item = rtp_get_sdes(session, ssrc, RTCP_SDES_LOC);
				if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len)) {
					packet += add_sdes_item(packet, remaining_len, RTCP_SDES_LOC, item);
					break;
				}
			case 4: item = rtp_get_sdes(session, ssrc, RTCP_SDES_PRIV);
				if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len)) {
					packet += add_sdes_item(packet, remaining_len, RTCP_SDES_PRIV, item);
					break;
				}
			}
			session->sdes_count_ter++;
		} else {
			item = rtp_get_sdes(session, ssrc, RTCP_SDES_NAME);
			if (item != NULL) {
				packet += add_sdes_item(packet, remaining_len, RTCP_SDES_NAME, item);
			}
		}
	}
	session->sdes_count_pri++;

	/* Pad to a multiple of 4 bytes... */
        pad = 4 - ((packet - buffer) & 0x3);
        while (pad--) {
               *packet++ = RTCP_SDES_END;
        }

	common->length = htons((uint16_t) (((int) (packet - buffer) / 4) - 1));

	return packet;
}

static uint8_t *
format_rtcp_app(uint8_t *buffer, int buflen, uint32_t ssrc, rtcp_app *app)
{
	/* Write an RTCP APP into the outgoing packet buffer. */
	rtcp_app    *packet       = (rtcp_app *) buffer;
	int          pkt_octets   = (app->length + 1) * 4;
	int          data_octets  =  pkt_octets - 12;

	assert(data_octets >= 0);          /* ...else not a legal APP packet.               */
	assert(buflen      >= pkt_octets); /* ...else there isn't space for the APP packet. */

	/* Copy one APP packet from "app" to "packet". */
	packet->version        =   RTP_VERSION;
	packet->p              =   app->p;
	packet->subtype        =   app->subtype;
	packet->pt             =   RTCP_APP;
	packet->length         =   htons(app->length);
	packet->ssrc           =   htonl(ssrc);
	memcpy(packet->name, app->name, 4);
	memcpy(packet->data, app->data, data_octets);

	/* Return a pointer to the byte that immediately follows the last byte written. */
	return buffer + pkt_octets;
}

void disable_rtcp(struct rtp *session)
{
	session->disable_rtcp = true;
	return;
}

void enable_rtcp(struct rtp *session)
{
	session->disable_rtcp = false;
	return;
}

static void 
send_rtcp_internal(struct rtp *session, uint32_t rtp_ts, rtcp_app_callback appcallback)
{
	/* Construct and send an RTCP packet. The order in which packets are packed into a */
	/* compound packet is defined by section 6.1 of draft-ietf-avt-rtp-new-03.txt and  */
	/* we follow the recommended order.                                                */
	uint8_t	   buffer[RTP_MAX_PACKET_LEN];	/* The +8 is to allow for padding when encrypting */
	uint8_t	  *ptr = buffer;
	uint8_t   *old_ptr;
	uint8_t   *lpt;		/* the last packet in the compound */
	rtcp_app  *app;
	//uint8_t    initVec[8] = {0,0,0,0,0,0,0,0};
	int	   rc;
    pdb	*participants;
    //pdb_e	*state;

	/* do not send rtcp if the flag disable_rtcp is true, add by willy, 2011-11-07 */
	if (session->disable_rtcp)
		return;

	check_database(session);
	/* If encryption is enabled, add a 32 bit random prefix to the packet */
    /*
	if (session->encryption_enabled)
	{
		*((uint32_t *) ptr) = lbl_random();
		ptr += 4;
	}
    */
	/* The first RTCP packet in the compound packet MUST always be a report packet...  */
	if (session->we_sent) {
		ptr = format_rtcp_sr(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), session, rtp_ts);
	} else {
		ptr = format_rtcp_rr(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), session);
	}
	/* Add the appropriate SDES items to the packet... This should really be after the */
	/* insertion of the additional report blocks, but if we do that there are problems */
	/* with us being unable to fit the SDES packet in when we run out of buffer space  */
	/* adding RRs. The correct fix would be to calculate the length of the SDES items  */
	/* in advance and subtract this from the buffer length but this is non-trivial and */
	/* probably not worth it.                                                          */
	lpt = ptr;
	ptr = format_rtcp_sdes(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), rtp_my_ssrc(session), session);

	/* Following that, additional RR packets SHOULD follow if there are more than 31   */
	/* senders, such that the reports do not fit into the initial packet. We give up   */
	/* if there is insufficient space in the buffer: this is bad, since we always drop */
	/* the reports from the same sources (those at the end of the hash table).         */
	while ((session->sender_count > 0)  && ((RTP_MAX_PACKET_LEN - (ptr - buffer)) >= 8)) {
		lpt = ptr;
		ptr = format_rtcp_rr(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), session);
	}

	/* Finish with as many APP packets as the application will provide. */
	old_ptr = ptr;
	if (appcallback) {
		while ((app = (*appcallback)(session, rtp_ts, RTP_MAX_PACKET_LEN - (ptr - buffer)))) {
			lpt = ptr;
			ptr = format_rtcp_app(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), rtp_my_ssrc(session), app);
			assert(ptr > old_ptr);
			old_ptr = ptr;
			assert(RTP_MAX_PACKET_LEN - (ptr - buffer) >= 0);
		}
	}

	/* And encrypt if desired... */
/*
	if (session->encryption_enabled)
	  {
		if (((ptr - buffer) % session->encryption_pad_length) != 0) {
			
			int	padlen = session->encryption_pad_length - ((ptr - buffer) % session->encryption_pad_length);
			int	i;

			for (i = 0; i < padlen-1; i++) {
				*(ptr++) = '\0';
			}
			*(ptr++) = (uint8_t) padlen;
			assert(((ptr - buffer) % session->encryption_pad_length) == 0); 

			((rtcp_t *) lpt)->common.p = TRUE;
			((rtcp_t *) lpt)->common.length = htons((int16_t)(((ptr - lpt) / 4) - 1));
		}
 		(session->encrypt_func)(session->crypto_state, buffer, ptr - buffer, initVec); 
	}
*/
	rc = udp_send(session->rtcp_socket, buffer, ptr - buffer);
	if (rc == -1) {
		perror("sending RTCP packet");
	}

        /* Loop the data back to ourselves so local participant can */
        /* query own stats when using unicast or multicast with no  */
        /* loopback.                                                */
        /*ian marked : this is why we get twice rtcp report */
        //rtp_process_ctrl(session, buffer, ptr - buffer);
	 
     check_database(session);
}

static void 
send_rtcp(struct rtp *session, uint32_t rtp_ts, rtcp_app_callback appcallback)
{
	/* do not send rtcp if the flag disable_rtcp is true, add by willy, 2011-11-07 */
	if (session && session->disable_rtcp)
		return;
    return send_rtcp_internal(session,rtp_ts,appcallback);
}
/**
 * rtp_send_ctrl:
 * @session: the session pointer (returned by rtp_init())
 * @rtp_ts: the current time expressed in units of the media timestamp.
 * @appcallback: a callback to create an APP RTCP packet, if needed.
 *
 * Checks RTCP timer and sends RTCP data when nececessary.  The
 * interval between RTCP packets is randomized over an interval that
 * depends on the session bandwidth, the number of participants, and
 * whether the local participant is a sender.  This function should be
 * called at least once per second, and can be safely called more
 * frequently.  
 */
void 
rtp_send_ctrl(struct rtp *session, uint32_t rtp_ts, rtcp_app_callback appcallback, struct timeval curr_time)
{
	int tfrc_feedback_due = 0;
	struct timeval now;
	/* Send an RTCP packet, if one is due... */
	check_database(session);

	if (session->tfrc_on) {
		gettimeofday (&now, NULL);
		tfrc_poll_rcvr_feedback(session->tfrcs,now);
		tfrc_feedback_due = tfrc_send_rcvr_feedback (session->tfrcr,now);
	}

	if (tv_gt(curr_time, session->next_rtcp_send_time) || tfrc_feedback_due) {
		/* The RTCP transmission timer has expired. The following */
		/* implements draft-ietf-avt-rtp-new-02.txt section 6.3.6 */
		//[Eric] if(tfrc_feedback_due)
		//[Eric] 	TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, " TFRC_Feedback_Due! ");
		//[Eric] else
		//[Eric] 	TL_DEBUG_MSG( TL_RTCP, DBG_MASK3, " Generic RTCP!!!!!! ");
		
		int		 h;
		source		*s;
		struct timeval	 new_send_time;
		double		 new_interval;

		new_interval  = rtcp_interval(session);
		new_send_time = session->last_rtcp_send_time;
		tv_add(&new_send_time, new_interval);
		if (tv_gt(curr_time, new_send_time) || tfrc_feedback_due) {
			send_rtcp(session, rtp_ts, appcallback);
			session->initial_rtcp        = FALSE;
			session->last_rtcp_send_time = curr_time;
			session->next_rtcp_send_time = curr_time; 
			tv_add(&(session->next_rtcp_send_time), rtcp_interval(session));
			/* We're starting a new RTCP reporting interval, zero out */
			/* the per-interval statistics.                           */
			session->sender_count = 0;
			for (h = 0; h < RTP_DB_SIZE; h++) {
				for (s = session->db[h]; s != NULL; s = s->next) {
					check_source(s);
					
					/* comment following line since that will cause the the report block doesn't include in the RTCP receiver packet */ /*add by willy, 2010-10-01 */
					//s->sender = FALSE;
				}
			}
		} else {
			session->next_rtcp_send_time = new_send_time;
		}
		session->ssrc_count_prev = session->ssrc_count;
	} 
	check_database(session);
}

#if 0
/*this is only for we have RFC4585 generic nack message need to send*/
void rtp_check_rfc4585(struct rtp *session)
{
  uint8_t   buffer[RTP_MAX_PACKET_LEN];	/* The +8 is to allow for padding when encrypting */
  uint8_t   *ptr = buffer;
  uint8_t   *old_ptr;
  uint8_t   *lpt;		/* the last packet in the compound */
  rtcp_app  *app;
  uint8_t   initVec[8] = {0,0,0,0,0,0,0,0};
  int       rc;
  pdb       *participants;
  pdb_e     *state;
  uint32_t  rtp_ts = get_local_mediatime();
  struct timeval   curr_time;
  int              h;
  source           *s;

  gettimeofday(&curr_time, NULL);

  /* The first RTCP packet in the compound packet MUST always be a report packet...  */
  if (session->we_sent) {
    ptr = format_rtcp_sr(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), session, rtp_ts);
  } else {
    ptr = format_rtcp_rr(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), session);
  }

    if(session->resend_enable)
    {
        participants = (pdb *) rtp_get_userdata(session);
        if(session->has_opposite_ssrc)
            state = pdb_get(participants, session->opposite_ssrc);
        else
            state = pdb_get(participants, rtp_my_ssrc(session));         
        
        old_ptr = ptr;

        ptr = RFC4585_FormatGenricNACK(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), session, state->playout_buffer);

        if(ptr == old_ptr)
        {
             /*no RFC4585 message , so give up this unregular rtcp message*/
             return;
        }
    }

  rc = udp_send(session->rtcp_socket, buffer, ptr - buffer);
  if (rc == -1) {
    perror("sending RTCP packet");
  }

  session->initial_rtcp        = FALSE;
  session->last_rtcp_send_time = curr_time;
  session->next_rtcp_send_time = curr_time;
  tv_add(&(session->next_rtcp_send_time), rtcp_interval(session));

  /* We're starting a new RTCP reporting interval, zero out */
  /* the per-interval statistics.                           */
  session->sender_count = 0;
  for (h = 0; h < RTP_DB_SIZE; h++) {
    for (s = session->db[h]; s != NULL; s = s->next) {
	check_source(s);
	/* comment following line since that will cause the the report block doesn't include in the RTCP receiver packet */ /*add by willy, 2010-10-01 */
	//s->sender = FALSE;
      }
  }
        
   check_database(session);
}
#endif

/**
 * force_send_rtcp:
 * @session: the session pointer (returned by rtp_init())
 *
 * send RTCP feedback NOW due to an increase in a TFRC's recivers
 * loss event rate, p.
 */
static void 
force_send_rtcp(struct rtp *session)
{
    /* Send an RTCP packet, NOW */
    int             h;
    source          *s;
    uint32_t        rtp_ts = get_local_mediatime ();
    struct timeval  curr_time;

    check_database(session);
    gettimeofday(&curr_time, NULL);
	
    send_rtcp(session, rtp_ts, NULL);

    session->initial_rtcp        = FALSE;
    session->last_rtcp_send_time = curr_time;
    session->next_rtcp_send_time = curr_time;
    tv_add(&(session->next_rtcp_send_time), rtcp_interval(session));

    /* We're starting a new RTCP reporting interval, zero out */
    /* the per-interval statistics.                           */
    session->sender_count = 0;
    for (h = 0; h < RTP_DB_SIZE; h++) {
        for (s = session->db[h]; s != NULL; s = s->next) {
            check_source(s);

            /* comment following line since that will cause the the report block doesn't include in the RTCP receiver packet */ /*add by willy, 2010-10-01 */
            //s->sender = FALSE;
        }
    }

    check_database(session);
}

/**
 * send_psfb_full_intra_request:
 * @session: the session pointer (returned by rtp_init())
 *
 * send Payload-Specific Feedback Messages for full_intra_request
 * PT=PSFB and FMT=4 (FIR)
 *
 * author: frank
 */
void send_psfb_full_intra_request(struct rtp *session)
{
    /* Send an generic nack RTCP packet, NOW */
    uint8_t             buffer[RTP_MAX_PACKET_LEN];	/* The +8 is to allow for padding when encrypting */
    uint8_t             *packet = buffer;
    uint32_t            rtp_ts = get_local_mediatime ();
    int                 h, rc;
    source              *s;
    struct timeval      curr_time;  

    check_database(session);
	gettimeofday(&curr_time, NULL);

#if 0
    /* The first RTCP packet in the compound packet MUST always be a report packet...  */
    if (session->we_sent) {
        packet = format_rtcp_sr(packet, RTP_MAX_PACKET_LEN - (packet - buffer), session, rtp_ts);
    } else {
        packet = format_rtcp_rr(packet, RTP_MAX_PACKET_LEN - (packet - buffer), session);
    }

    packet = format_rtcp_sdes(packet, RTP_MAX_PACKET_LEN-(packet-buffer), rtp_my_ssrc(session), session);
#endif    
    packet = rfc5104_format_psfb_full_intra_request(packet, RTP_MAX_PACKET_LEN-(packet-buffer), session);

    rc = udp_send(session->rtcp_socket, buffer, packet - buffer);
    if (rc == -1) {
        perror("sending rfc5104 full_intra_request");
    }

    session->initial_rtcp        = FALSE;
    session->last_rtcp_send_time = curr_time;
    session->next_rtcp_send_time = curr_time;
    tv_add(&(session->next_rtcp_send_time), rtcp_interval(session));

    /* We're starting a new RTCP reporting interval, zero out */
    /* the per-interval statistics.                           */
    session->sender_count = 0;
    for (h = 0; h < RTP_DB_SIZE; h++) {
        for (s = session->db[h]; s != NULL; s = s->next) {
            check_source(s);
        }
    }

    check_database(session);
}


/**
 * send_psfb_picture_loss_indication:
 * @session: the session pointer (returned by rtp_init())
 *
 * send Payload-Specific Feedback Messages for picture_loss_indication
 * PT=PSFB and FMT=1 (PLI)
 *
 * author: frank
 */
void send_psfb_picture_loss_indication(struct rtp *session)
{
    uint8_t             buffer[RTP_MAX_PACKET_LEN];	/* The +8 is to allow for padding when encrypting */
    uint8_t             *packet = buffer;
    uint32_t            rtp_ts = get_local_mediatime ();
    int                 h, rc;
    source              *s;
    struct timeval      curr_time;  

    check_database(session);
	gettimeofday(&curr_time, NULL);

    /* The first RTCP packet in the compound packet MUST always be a report packet...  */
    if (session->we_sent) {
        packet = format_rtcp_sr(packet, RTP_MAX_PACKET_LEN - (packet - buffer), session, rtp_ts);
    } else {
        packet = format_rtcp_rr(packet, RTP_MAX_PACKET_LEN - (packet - buffer), session);
    }

    packet = format_rtcp_sdes(packet, RTP_MAX_PACKET_LEN-(packet-buffer), rtp_my_ssrc(session), session);

    packet = rfc4585_format_psfb_picture_loss_indication(packet, RTP_MAX_PACKET_LEN-(packet-buffer), session);

    rc = udp_send(session->rtcp_socket, buffer, packet - buffer);
    if (rc == -1) {
        perror("sending rfc4585 picture_loss_indication");
    }

    session->initial_rtcp        = FALSE;
    session->last_rtcp_send_time = curr_time;
    session->next_rtcp_send_time = curr_time;
    tv_add(&(session->next_rtcp_send_time), rtcp_interval(session));

    /* We're starting a new RTCP reporting interval, zero out */
    /* the per-interval statistics.                           */
    session->sender_count = 0;
    for (h = 0; h < RTP_DB_SIZE; h++) {
        for (s = session->db[h]; s != NULL; s = s->next) {
            check_source(s);
        }
    }

    check_database(session);
}


/**
 * send_psfb_slice_loss_indication:
 * @session: the session pointer (returned by rtp_init())
 *
 * send Payload-Specific Feedback Messages for slice_loss_indication
 * PT=PSFB and FMT=2 (SLI)
 *
 * author: frank
 */
void send_psfb_slice_loss_indication(struct rtp *session, uint16_t first, uint16_t number, uint8_t picture_id)
{
    uint8_t             buffer[RTP_MAX_PACKET_LEN];	/* The +8 is to allow for padding when encrypting */
    uint8_t             *packet = buffer;
    uint32_t            rtp_ts = get_local_mediatime ();
    int                 h, rc;
    source              *s;
    struct timeval      curr_time;  

    check_database(session);
	gettimeofday(&curr_time, NULL);

    /* The first RTCP packet in the compound packet MUST always be a report packet...  */
    if (session->we_sent) {
        packet = format_rtcp_sr(packet, RTP_MAX_PACKET_LEN - (packet - buffer), session, rtp_ts);
    } else {
        packet = format_rtcp_rr(packet, RTP_MAX_PACKET_LEN - (packet - buffer), session);
    }

    packet = rfc4585_format_psfb_slice_loss_indication(packet, RTP_MAX_PACKET_LEN-(packet-buffer), first, number, picture_id, session);

    rc = udp_send(session->rtcp_socket, buffer, packet - buffer);
    if (rc == -1) {
        perror("sending rfc4585 slice_loss_indication");
    }

    session->initial_rtcp        = FALSE;
    session->last_rtcp_send_time = curr_time;
    session->next_rtcp_send_time = curr_time;
    tv_add(&(session->next_rtcp_send_time), rtcp_interval(session));

    /* We're starting a new RTCP reporting interval, zero out */
    /* the per-interval statistics.                           */
    session->sender_count = 0;
    for (h = 0; h < RTP_DB_SIZE; h++) {
        for (s = session->db[h]; s != NULL; s = s->next) {
            check_source(s);
        }
    }

    check_database(session);
}



/**
 * send_generic_nack_rtcp:
 * @session: the session pointer (returned by rtp_init())
 *
 * send Transport-Layer Feedback Message for generic_nack
 * PT=RTPFB and FMT=1
 *
 * author: frank
 */
void send_rtpfb_generic_nack(struct rtp *session, int pid, int bitmask)
{
    /* Send an generic nack RTCP packet, NOW */
    uint8_t             buffer[RTP_MAX_PACKET_LEN];	/* The +8 is to allow for padding when encrypting */
    uint8_t             *packet = buffer;
    uint32_t            rtp_ts = get_local_mediatime ();
    int                 h, rc;
    source              *s;
    struct timeval      curr_time;  

    check_database(session);
	gettimeofday(&curr_time, NULL);

    /* The first RTCP packet in the compound packet MUST always be a report packet...  */
    if (session->we_sent) {
        packet = format_rtcp_sr(packet, RTP_MAX_PACKET_LEN - (packet - buffer), session, rtp_ts);
    } else {
        packet = format_rtcp_rr(packet, RTP_MAX_PACKET_LEN - (packet - buffer), session);
    }

    packet = rfc4585_format_rtpfb_generic_nack(packet, RTP_MAX_PACKET_LEN-(packet-buffer), pid, bitmask, session);

    rc = udp_send(session->rtcp_socket, buffer, packet - buffer);
    if (rc == -1) {
        perror("sending rfc4585 generic nack packet");
    }

    session->initial_rtcp        = FALSE;
    session->last_rtcp_send_time = curr_time;
    session->next_rtcp_send_time = curr_time;
    tv_add(&(session->next_rtcp_send_time), rtcp_interval(session));

    /* We're starting a new RTCP reporting interval, zero out */
    /* the per-interval statistics.                           */
    session->sender_count = 0;
    for (h = 0; h < RTP_DB_SIZE; h++) {
        for (s = session->db[h]; s != NULL; s = s->next) {
            check_source(s);
        }
    }

    check_database(session);
}


/**
 * rtp_update:
 * @session: the session pointer (returned by rtp_init())
 *
 * Trawls through the internal data structures and performs
 * housekeeping.  This function should be called at least once per
 * second.  It uses an internal timer to limit the number of passes
 * through the data structures to once per second, it can be safely
 * called more frequently.
 */
void 
rtp_update(struct rtp *session, struct timeval curr_time)
{
	/* Perform housekeeping on the source database... */
	int	 	 h;
	source	 	*s, *n;
	double		 delay;

	if (tv_diff(curr_time, session->last_update) < 1.0) {
		/* We only perform housekeeping once per second... */
		return;
	}
	session->last_update = curr_time;

	/* Update we_sent (section 6.3.8 of RTP spec) */
	delay = tv_diff(curr_time, session->last_rtp_send_time);
	if (delay >= 2 * rtcp_interval(session)) {
		session->we_sent = FALSE;
	}

	check_database(session);

	for (h = 0; h < RTP_DB_SIZE; h++) {
		for (s = session->db[h]; s != NULL; s = n) {
			check_source(s);
			n = s->next;
			/* Expire sources which haven't been heard from for a long time.   */
			/* Section 6.2.1 of the RTP specification details the timers used. */

			/* How long since we last heard from this source?  */
			delay = tv_diff(curr_time, s->last_active);
			
			/* Check if we've received a BYE packet from this source.    */
			/* If we have, and it was received more than 2 seconds ago   */
			/* then the source is deleted. The arbitrary 2 second delay  */
			/* is to ensure that all delayed packets are received before */
			/* the source is timed out.                                  */
			if (s->got_bye && (delay > 2.0)) {
				TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "Deleting source 0x%08u due to reception of BYE %f seconds ago...\n", s->ssrc, delay);
				delete_source(session, s->ssrc);
			}

			/* Sources are marked as inactive if they haven't been heard */
			/* from for more than 2 intervals (RTP section 6.3.5)        */
			if ((s->ssrc != rtp_my_ssrc(session)) && (delay > (session->rtcp_interval * 2))) {
				if (s->sender) {
					/* comment following line since that will cause the the report block doesn't include in the RTCP receiver packet */ /*add by willy, 2010-10-01 */
					//s->sender = FALSE;
					session->sender_count--;
				}
			}

			/* If a source hasn't been heard from for more than 5 RTCP   */
			/* reporting intervals, we delete it from our database...    */
			if ((s->ssrc != rtp_my_ssrc(session)) && (delay > (session->rtcp_interval * 5))) {
				TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "Deleting source 0x%08u due to timeout...\n", s->ssrc);
				delete_source(session, s->ssrc);
			}
		}
	}

	/* Timeout those reception reports which haven't been refreshed for a long time */
	timeout_rr(session, &curr_time);
	check_database(session);
}

static void 
rtp_send_bye_now(struct rtp *session)
{
	/* Send a BYE packet immediately. This is an internal function,  */
	/* hidden behind the rtp_send_bye() wrapper which implements BYE */
	/* reconsideration for the application.                          */
	uint8_t	 	 buffer[RTP_MAX_PACKET_LEN];	/* + 8 to allow for padding when encrypting */
	uint8_t		*ptr = buffer;
	rtcp_common	*common;
	//uint8_t    	 initVec[8] = {0,0,0,0,0,0,0,0};

	check_database(session);
	/* If encryption is enabled, add a 32 bit random prefix to the packet */
    /*
	if (session->encryption_enabled) {
		*((uint32_t *) ptr) = lbl_random();
		ptr += 4;
	}
    */
	ptr    = format_rtcp_rr(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), session);    
	common = (rtcp_common *) ptr;
	
	common->version = 2;
	common->p       = 0;
	common->count   = 1;
	common->pt      = RTCP_BYE;
	common->length  = htons(1);
	ptr += sizeof(common);
	
	*((uint32_t *) ptr) = htonl(session->my_ssrc);  
	ptr += 4;
	/*
	if (session->encryption_enabled) {
		if (((ptr - buffer) % session->encryption_pad_length) != 0) {
			int	padlen = session->encryption_pad_length - ((ptr - buffer) % session->encryption_pad_length);
			int	i;

			for (i = 0; i < padlen-1; i++) {
				*(ptr++) = '\0';
			}
			*(ptr++) = (uint8_t) padlen;

			common->p      = TRUE;
			common->length = htons((int16_t)(((ptr - (uint8_t *) common) / 4) - 1));
		}
		assert(((ptr - buffer) % session->encryption_pad_length) == 0);
		(session->encrypt_func)(session->crypto_state, buffer, ptr - buffer, initVec);
	}
*/
	udp_send(session->rtcp_socket, buffer, ptr - buffer);
	/* Loop the data back to ourselves so local participant can */
	/* query own stats when using unicast or multicast with no  */
	/* loopback.                                                */
	struct timeval RTCP_now;
    gettimeofday(&RTCP_now, NULL);
	rtp_process_ctrl(session,buffer, ptr - buffer,RTCP_now);
	check_database(session);
}

/**
 * rtp_send_bye:
 * @session: The RTP session
 *
 * Sends a BYE message on the RTP session, indicating that this
 * participant is leaving the session. The process of sending a
 * BYE may take some time, and this function will block until 
 * it is complete. During this time, RTCP events are reported 
 * to the application via the callback function (data packets 
 * are silently discarded).
 */
void 
rtp_send_bye(struct rtp *session)
{
	struct timeval	curr_time, timeout, new_send_time;
	uint8_t		buffer[RTP_MAX_PACKET_LEN];
	int		buflen;
	double		new_interval;

	check_database(session);

	/* "...a participant which never sent an RTP or RTCP packet MUST NOT send  */
	/* a BYE packet when they leave the group." (section 6.3.7 of RTP spec)    */
	if ((session->we_sent == FALSE) && (session->initial_rtcp == TRUE)) {
		TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "Silent BYE\n");
		return;
	}

	/* If the session is small, send an immediate BYE. Otherwise, we delay and */
	/* perform BYE reconsideration as needed.                                  */
	if (session->ssrc_count < 50) {
		rtp_send_bye_now(session);
	} else {
		gettimeofday(&curr_time, NULL);
		session->sending_bye         = TRUE;
		session->last_rtcp_send_time = curr_time;
		session->next_rtcp_send_time = curr_time; 
		session->bye_count           = 1;
		session->initial_rtcp        = TRUE;
		session->we_sent             = FALSE;
		session->sender_count        = 0;
		session->avg_rtcp_size       = 70.0 + RTP_LOWER_LAYER_OVERHEAD;	/* FIXME */
		tv_add(&session->next_rtcp_send_time, rtcp_interval(session));

		TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "Preparing to send BYE...\n");
		while (1) {
			/* Schedule us to block in udp_select() until the time we are due to send our */
			/* BYE packet. If we receive an RTCP packet from another participant before   */
			/* then, we are woken up to handle it...                                      */
			timeout.tv_sec  = 0;
			timeout.tv_usec = 0;
			tv_add(&timeout, tv_diff(session->next_rtcp_send_time, curr_time));
			udp_fd_zero();
			udp_fd_set(session->rtcp_socket);
			if ((udp_select(&timeout) > 0) && udp_fd_isset(session->rtcp_socket)) {
				/* We woke up because an RTCP packet was received; process it... */
				buflen = udp_recv(session->rtcp_socket, buffer, RTP_MAX_PACKET_LEN);
				struct timeval RTCP_now;
                gettimeofday(&RTCP_now, NULL);
				rtp_process_ctrl(session, buffer, buflen,RTCP_now);
			}
			/* Is it time to send our BYE? */
			gettimeofday(&curr_time, NULL);
			new_interval  = rtcp_interval(session);
			new_send_time = session->last_rtcp_send_time;
			tv_add(&new_send_time, new_interval);
			if (tv_gt(curr_time, new_send_time)) {
				TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "Sent BYE...\n");
				rtp_send_bye_now(session);
				break;
			}
			/* No, we reconsider... */
			session->next_rtcp_send_time = new_send_time;
			TL_DEBUG_MSG( TL_RTP, DBG_MASK3, "Reconsidered sending BYE... delay = %f\n", tv_diff(session->next_rtcp_send_time, curr_time));
			/* ...and perform housekeeping in the usual manner */
			rtp_update(session, curr_time);
		}
	}
}

/**
 * rtp_done:
 * @session: the RTP session to finish
 *
 * Free the state associated with the given RTP session. This function does 
 * not send any packets (e.g. an RTCP BYE) - an application which wishes to
 * exit in a clean manner should call rtp_send_bye() first.
 */
void 
rtp_done(struct rtp *session)
{
	int i;
        source *s, *n;

	check_database(session);
        /* In delete_source, check database gets called and this assumes */
        /* first added and last removed is us.                           */
	for (i = 0; i < RTP_DB_SIZE; i++) {
                s = session->db[i];
                while (s != NULL) {
                        n = s->next;
                        if (s->ssrc != session->my_ssrc) {
                                delete_source(session,session->db[i]->ssrc);
                        }
                        s = n;
                }
	}

	delete_source(session, session->my_ssrc);

	/*
	 * Introduce a memory leak until we add algorithm-specific
	 * cleanup functions.
        if (session->encryption_key != NULL) {
                free(session->encryption_key);
        }
	*/

	udp_exit(session->rtp_socket);
	udp_exit(session->rtcp_socket);
	tfrc_sndr_release(session->tfrcs);
	tfrc_rcvr_release(session->tfrcr);
	free_rtp_mulStream(session->fec_stream);
	free(session->addr);
	free(session->opt);
	free(session);
}



/**
 * rtp_get_addr:
 * @session: The RTP Session.
 *
 * Returns: The session's destination address, as set when creating the
 * session with rtp_init() or rtp_init_if().
 */
char *
rtp_get_addr(struct rtp *session)
{
	check_database(session);
	return session->addr;
}

/**
 * rtp_get_rx_port:
 * @session: The RTP Session.
 *
 * Returns: The UDP port to which this session is bound, as set when
 * creating the session with rtp_init() or rtp_init_if().
 */
uint16_t 
rtp_get_rx_port(struct rtp *session)
{
	check_database(session);
	return session->rx_port;
}

/**
 * rtp_get_tx_port:
 * @session: The RTP Session.
 *
 * Returns: The UDP port to which RTP packets are transmitted, as set
 * when creating the session with rtp_init() or rtp_init_if().
 */
uint16_t 
rtp_get_tx_port(struct rtp *session)
{
	check_database(session);
	return session->tx_port;
}

/**
 * rtp_get_seq:
 * @session: The RTP Session.
 *
 * Returns:
 */
uint16_t 
rtp_get_seq(struct rtp *session)
{
	check_database(session);
	return session->rtp_seq;
}

/**
 * rtp_get_ttl:
 * @session: The RTP Session.
 *
 * Returns: The session's TTL, as set when creating the session with
 * rtp_init() or rtp_init_if().
 */
int 
rtp_get_ttl(struct rtp *session)
{
	check_database(session);
	return session->ttl;
}

int rtp_set_rtcp_time_info(struct rtp *session, ntp_rtpts_pair *info)
{
     session->ntp_time_info.ntp_sec = info->ntp_sec;
     session->ntp_time_info.ntp_frac = info->ntp_frac;
     session->ntp_time_info.rtp_ts = info->rtp_ts;
    
     return 1;
}

int rtp_set_tfrc_min_sending_rate(struct rtp *session, uint32_t min_sending_rate)
{
	tfrc_set_max_ipi(session->tfrcs , min_sending_rate);
	
	return 1;
}

int rtp_set_media_type(struct rtp *session, int media_type)
{
    session->media_type = media_type;

    return 1;
}
unsigned long rtp_get_tl_session(struct rtp *session)
{
    return session->tl_session;
}
void rtp_set_tl_session(struct rtp *session, unsigned long tl_session)
{
    session->tl_session = tl_session;
}

int rtp_recover(struct rtp *session, uint16_t seq)
{
  return rtp_mulStream_recover_rtp(session, session->fec_stream,seq);
}
