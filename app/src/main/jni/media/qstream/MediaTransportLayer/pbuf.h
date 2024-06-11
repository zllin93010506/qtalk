/*
 * FILE:     pbuf.h
 * AUTHOR:   N.Cihan Tas
 * MODIFIED: Ladan Gharai
 *           Colin Perkins
 * 
 * Copyright (c) 2003-2004 University of Southern California
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
 * $Revision: 1.17 $
 * $Date: 2004/12/12 10:48:20 $
 *
 */

#ifndef _PBUF_H_
#define _PBUF_H_

#include  "config.h"
#ifdef WIN32
#include "config_w32.h"
#else
#include "config_unix.h"
#endif
#include "rtp.h"
/******************************************************************************/
/* The main playout buffer data structures. See "RTP: Audio and Video for the */
/* Internet" Figure 6.8 (page 167) for a diagram.                       [csp] */
/******************************************************************************/

#define RECVING_BUF_LOCK(x)		    pthread_mutex_lock(&(x->lock))
#define RECVING_BUF_UNLOCK(x)		pthread_mutex_unlock(&(x->lock))

/* The coded representation of a single frame */
typedef struct _coded_data 
{
        struct _coded_data            *next;
        struct _coded_data            *prev;
        uint16_t                      seqno;
        rtp_packet             		  *data;

}coded_data;

/* The playout buffer */
/*struct pbuf;*/

typedef struct _pbufnode {
        struct _pbufnode         *next;
        struct _pbufnode         *prev;
        uint32_t                 rtp_timestamp;	/* RTP timestamp for the frame           */
        struct timeval           arrival_time;	/* Arrival time of first packet in frame */
        struct timeval           playout_time;	/* Playout time for the frame            */
        struct timeval           discard_time;	/* Discard time for the frame            */
        coded_data               *cdata;		/*					 */
	unsigned                 cdata_count;
	uint16_t                 seqno_min;
	uint16_t                 seqno_max;
	int                      decoded;		/* Non-zero if we've decoded this frame  */
	int                      mbit;		/* Set if marker bit of frame seen       */
	uint8_t                  pt;			/* RTP payload type for the frame        */
        uint32_t                 magic;		/* For debugging                         */
        uint8_t                  resend_check_flag; /*reocrd that if we have checked necessary for reporte 
                                                                                resend message to sender for this frame*/
}pbuf_node;

typedef struct  
{
    pbuf_node               *frst;
    pbuf_node               *last;
    unsigned                size; /*record the number of frames in pbuf*/
    unsigned                pkt_count; /*record the number of packets in pbuf*/
    void                    *decoder_state;
    unsigned                decoder_id;
    pthread_mutex_t         lock;
    uint32_t                last_removed_timestamp; /*record the latest removed frame's timestamp, 
                                                 for not stroing the disrcarded frame data*/
    uint8_t                 removed_flag; /*check if the playout buffer has frame been removed*/
    int                      last_removed_frame_max_seq; /*record the last removed frame's max seq no*/
    uint8_t                 last_removed_frame_has_mbit; /*reocrd if the last removed frame has mark bit*/
    uint16_t                print_count;
}pbuf;

/* 
 * External interface: 
 */
pbuf	*pbuf_init(void);
void    pbuf_done(pbuf *playout_buf);
void	pbuf_insert(pbuf *playout_buf, rtp_packet *r);
/*
int 	 	 pbuf_decode(struct pbuf *playout_buf, struct timeval curr_time, struct display *display);
*/
void    pbuf_remove( pbuf *playout_buf,  struct timeval curr_time);

/*
 * Internal functions... may move elsewhere if generally useful:
 */
int     seq_lt(uint16_t s1, uint16_t s2);	/* Compare two RTP sequence numbers */ 

int     frame_complete(pbuf_node *frame, uint32_t *predict_frame_size, int prev_seqno_max, uint8_t* first_packet_data);

void    pbuf_remove_first(pbuf *playout_buf);
#endif

