/*
 * AUTHORS: N.Cihan Tas
 *          Ladan Gharai
 *          Colin Perkins
 * 
 * This file implements a linked list for the playout buffer.
 *
 * Copyright (c) 2003-2005 University of Southern California
 * Copyright (c) 2003-2005 University of Glasgow
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
 * $Revision: 1.92 $
 * $Date: 2005/04/26 12:04:10 $
 *
 */


#include "debug.h"
/*
#include "video_types.h"
#include "video_display.h"
#include "video_capture.h"
#include "video_convert.h"
*/
#include "rtp.h"
#include "pbuf.h"
#include "tv.h"
/*
#include "video_codec.h"
*/
#define PBUF_MAGIC	       0xcafebabe
#define PBUF_MAX_BUF_SIZE      60
#define PBUF_MAX_BUF_NODE_SIZE 500
/*********************************************************************************/

int
seq_lt(uint16_t s1, uint16_t s2)
{
	/* Return 1 if s1 < s2 modulo 65535; 0 otherwise     */
	/* Typically used for comparing RTP sequence numbers */
	/* and assumes s1 and s2 are within half the space.  */
	uint16_t   diff = s2 - s1;

	if (diff < 0x8000u) {
		return 1;
	}
	return 0;
}

int ts_lt(uint32_t t1, uint32_t t2)
{
	/* Return 1 if t1 < t2 modulo 65535; 0 otherwise     */
	/* Typically used for comparing RTP sequence numbers */
	/* and assumes t1 and t2 are within half the space.  */

    uint32_t diff =t2 - t1;
    if(diff < 0x80000000u) {
        return 1;
   }
   return 0;
}

static void
pbuf_validate(pbuf *playout_buf)
{
	/* Run through the entire playout buffer, checking pointers, etc.  */
	/* Only used in debugging mode, since it's a lot of overhead [csp] */
#ifdef DEBUG_PBUF
	 pbuf_node	*cpb, *ppb;
	 coded_data	*ccd, *pcd;
	unsigned		 cdc,  pbc;

	cpb = playout_buf->frst;
	ppb = NULL;
	pbc = 0;
	while (cpb != NULL) {
		assert(cpb->magic == PBUF_MAGIC);
		assert(cpb->prev == ppb);

		if (cpb->prev != NULL) {
			assert(cpb->prev->next == cpb);
			assert(cpb->rtp_timestamp > ppb->rtp_timestamp);     /* Nodes stored in RTP timestamp order */
			/* FIXME: previous assert will fail when timestamp wraps */
			assert(tv_gt(cpb->playout_time, ppb->playout_time)); /* Nodes stored in playout time order  */
		}

		if (cpb->next != NULL) {
			assert(cpb->next->prev == cpb);
		} else {
			assert(cpb = playout_buf->last);
		}

		if (cpb->cdata != NULL) {
			/* We have coded data... check all the pointers on that list too... */
			cdc = 0;
			ccd = cpb->cdata;
			pcd = NULL;
			while (ccd != NULL) {
				cdc++;
				assert(ccd->data != NULL);
				assert(ccd->prev == pcd);
				if (ccd->prev != NULL) {
					assert(ccd->prev->next == ccd);
					assert(seq_lt(ccd->seqno, pcd->seqno)); /* Coded data stored in descending order */
				}
				if (ccd->next != NULL) {
					assert(ccd->next->prev == ccd);
				}
				pcd = ccd;
				ccd = ccd->next;
			}
			assert(cpb->cdata_count == cdc);
		}

		/* Advance to next node in the playout buffer... */
		ppb = cpb;
		cpb = cpb->next;
		pbc++;
	}
	assert(pbc == playout_buf->size);
#else
	UNUSED(playout_buf);
#endif
}

pbuf *pbuf_init(void)
{  
	pbuf	*playout_buf = NULL;

	playout_buf = malloc(sizeof(pbuf));
	if (playout_buf != NULL) {
		playout_buf->frst=NULL;
		playout_buf->last=NULL;
	} else {
		TL_DEBUG_MSG(TL_PLAYOUT_BUF, DBG_MASK1, "Failed to allocate memory for playout buffer\n");
		return NULL;
	}
	playout_buf->size          = 0;
	playout_buf->pkt_count     = 0;
	playout_buf->decoder_state = NULL;
	playout_buf->decoder_id    =  1;
    playout_buf->last_removed_timestamp = 0;
    pthread_mutex_init(&(playout_buf->lock), NULL);
    playout_buf->removed_flag = FALSE;
    playout_buf->last_removed_frame_max_seq = -1;
    playout_buf->print_count = 0;
	return playout_buf;
}


/**
 *
 *Return Value: 1 -> inserted successfully   0 -> failed to insert
 *              -1 -> unexpected event occurs
 */
static int
add_coded_unit(pbuf_node *node, rtp_packet *pkt) 
{
	/* Add "pkt" to the frame represented by "node". The "node" has    */
	/* previously been created, and has some coded data already...     */
	/* Packets are stored in descending order of RTP sequence number.  */

	 coded_data *new_cdata, *curr; 

	assert(node->rtp_timestamp == pkt->fields.ts);
	assert(node->cdata != NULL);

	new_cdata = malloc(sizeof(coded_data));
	if (new_cdata == NULL) {
		/* Out of memory, drop the packet... */
		free(pkt);
		return 0;
	}

	if (node->pt != pkt->fields.pt) {
		TL_DEBUG_MSG(TL_PLAYOUT_BUF, DBG_MASK1, "Payload type changed within frame - packet discarded\n");
                free(new_cdata);
		free(pkt);
		return 0;
	}
	
#if 1
	// FIXME ignore the marker bit if the packet is parameter set or picture parameter set due to the capability of PL330, added by willy, 2012-01-30
	unsigned char type = pkt->meta.data[0] & 0x1f;
	if (type != 7 && type != 8) {
		node->mbit |= pkt->fields.m;
	}
#else
	node->mbit |= pkt->fields.m;
#endif
	node->cdata_count++;

	if (seq_lt(pkt->fields.seq, node->seqno_min)) node->seqno_min = pkt->fields.seq;
	if (seq_lt(node->seqno_max, pkt->fields.seq)) node->seqno_max = pkt->fields.seq;

    //if((node->seqno_max - node->seqno_min) > PBUF_MAX_BUF_NODE_SIZE)
    //     TL_DEBUG_MSG(TL_PLAYOUT_BUF, DBG_MASK1, "pbuf node has packets %u over size 500 \n", node->seqno_max - node->seqno_min);

	new_cdata->seqno = pkt->fields.seq;
	new_cdata->data  = pkt;
	new_cdata->prev  = NULL;
	new_cdata->next  = NULL;

	curr = node->cdata;
	while (curr != NULL) {
		if (seq_lt(curr->seqno, new_cdata->seqno)) {
			/* Insert into list before "curr".  In the usual case, when packets  */
			/* arrive in order, this inserts the packet at the head of the list. */
                       if(curr->seqno == new_cdata->seqno)
                       {
                             /*we have the duplicated packet, drop it*/
                             //TL_DEBUG_MSG(TL_PLAYOUT_BUF, DBG_MASK1, "add_coded_unit detect duplicate packet\n");
                             free(new_cdata);
                             free(pkt);
                             return 0;
                       }

			new_cdata->next = curr;
			new_cdata->prev = curr->prev;
			if (curr->prev != NULL) {
				curr->prev->next = new_cdata;
			}
			curr->prev = new_cdata;
			if (curr == node->cdata) {
				node->cdata = new_cdata;
			}
			return 1;
		}
		if (curr->next == NULL) {
			/* Got to the end of the list without inserting the newly arrived */
			/* packet. This can occur due to reordering. Add at end of list.  */
			curr->next = new_cdata;
			new_cdata->prev = curr;
			return 1;
		}
		curr = curr->next;
	}
	
	/*It's unexpected that going to here. becasue pkt is not inserted into pbuf, free it here*/
    free(new_cdata);
    free(pkt);
    return -1;
}

static pbuf_node * 
create_new_pnode(rtp_packet *pkt) 
{
	pbuf_node *tmp;

	tmp = malloc(sizeof(pbuf_node));
	if (tmp != NULL) {

		tmp->magic         = PBUF_MAGIC;
		tmp->next          = NULL;
		tmp->prev          = NULL;
		tmp->decoded       = 0;
		tmp->rtp_timestamp = pkt->fields.ts;
		
#if 1
		// FIXME ignore the marker bit if the packet is parameter set or picture parameter set due to the capability of PL330, added by willy, 2012-01-30
		unsigned char type = pkt->meta.data[0] & 0x1f;
		if (type == 7 || type == 8) {
			tmp->mbit = 0;
		}
		else 
			tmp->mbit = pkt->fields.m;
#else
		tmp->mbit	   = pkt->fields.m; 
#endif
		tmp->pt            = pkt->fields.pt; /* FIX ME: should we always and? - LG */
		tmp->cdata_count   = 1;
		tmp->seqno_min     = pkt->fields.seq;
		tmp->seqno_max     = pkt->fields.seq;
        tmp->resend_check_flag     = FALSE;

		gettimeofday(&(tmp->arrival_time), NULL);
		gettimeofday(&(tmp->playout_time), NULL);	
		gettimeofday(&(tmp->discard_time), NULL);	
		/* Playout delay... should really be adaptive, based on the */
		/* jitter, but we use a (conservative) fixed 250ms delay.   */
		tv_add(&(tmp->playout_time), 0.25);
		tv_add(&(tmp->discard_time), 0.50);	/* MUST be > playout time + maximum network queuing delay */

		tmp->cdata = malloc(sizeof(coded_data));
		if (tmp->cdata != NULL) {
			tmp->cdata->next   = NULL;
			tmp->cdata->prev   = NULL;
			tmp->cdata->seqno = pkt->fields.seq;
			tmp->cdata->data  = pkt;
		} else {
			free(pkt); 	
			free(tmp);
			return NULL;
		}
	} else {
		free(pkt);
		return NULL;
	}
	return tmp;
}


void
pbuf_insert( pbuf *playout_buf, rtp_packet *pkt) 
{
	 pbuf_node	*tmp, *tmp2;
     int    pkt_insert = 0;

	//pbuf_validate(playout_buf);

        RECVING_BUF_LOCK(playout_buf);
       
        if(pkt->fields.pt == 1)
       {
             free(pkt);
             RECVING_BUF_UNLOCK(playout_buf);
             return;     
       }

        if(pkt->fields.ts == playout_buf->last_removed_timestamp)
        {
             free(pkt);
             RECVING_BUF_UNLOCK(playout_buf);
             return;     
        }
        
	if (playout_buf->frst==NULL && playout_buf->last==NULL) {
	
		/* Playout buffer is empty - add the new frame */
		
		if( playout_buf->last_removed_frame_max_seq == -1 || !ts_lt(pkt->fields.ts, playout_buf->last_removed_timestamp) )  {
            /* only allow the new frame, which has the larger timestamp than the last removed frame */
			playout_buf->frst = create_new_pnode(pkt);
			playout_buf->last = playout_buf->frst;
			playout_buf->size++;
			playout_buf->pkt_count++;

        }else{
            free(pkt);
        }

        RECVING_BUF_UNLOCK(playout_buf);
		return;
	} 
        
       
	if (playout_buf->last->rtp_timestamp == pkt->fields.ts) {

		/* Packet belongs to last frame stored in the playout buffer. */
		/* This is the most likely scenario for video formats, which  */
		/* split each frame into multiple RTP packets.                */
		pkt_insert = add_coded_unit(playout_buf->last, pkt);
    }else if(ts_lt(playout_buf->last->rtp_timestamp, pkt->fields.ts)) { 
        /*fix a wrap around bug here, we must consider wrap around condition so use ts_lt function*/
		/* Packet belongs to a new frame. This is the most likely     */
		/* scenario for audio formats, where each frame is a single   */
		/* packet. Add a new node to the end of the playout buffer.   */

               if(playout_buf->size > PBUF_MAX_BUF_SIZE)
               {
                   free(pkt);
                   playout_buf->print_count++;

                   if(playout_buf->print_count > 20)
                   {
                       playout_buf->print_count = 0;
                       TL_DEBUG_MSG(TL_PLAYOUT_BUF, DBG_MASK1, "reciving buf has %u over maximu size 60!!!!!!!!!!!!!!!!!!! \n",playout_buf->size);
                   }

                   RECVING_BUF_UNLOCK(playout_buf);                   
                   return;
               }

		tmp = create_new_pnode(pkt);
		playout_buf->last->next = tmp;
		tmp->prev = playout_buf->last;
		playout_buf->last = tmp;
		playout_buf->size++;
		pkt_insert = 1;
	} else {
		/* Packet belongs to a previous frame. This is unlikely, and  */
		/* should only occur if there has been packet reordering.     */
                if(!ts_lt(playout_buf->frst->rtp_timestamp, pkt->fields.ts)) {
                //if(playout_buf->frst->rtp_timestamp > pkt->fields.ts) {
			TL_DEBUG_MSG(TL_PLAYOUT_BUF, DBG_MASK1,"A very old packet - discarded. first node ts %u last node ts %u buffer size %d packet ts %u\n", playout_buf->frst->rtp_timestamp, playout_buf->last->rtp_timestamp, playout_buf->size,pkt->fields.ts);
            pkt_insert = 0;
            free(pkt);
		} else {
			TL_DEBUG_MSG(TL_PLAYOUT_BUF, DBG_MASK2,"A packet for a previous frame, but might still be useful first node ts %u last node ts %u buffer size %d packet ts %u\n", playout_buf->frst->rtp_timestamp, playout_buf->last->rtp_timestamp, playout_buf->size,pkt->fields.ts);
			/* FIXME: should insert the packet into the playout buffer... */
            /*FIXED ? search the corresponding timestamp and insert it*/
            int found=0;
            for(tmp = playout_buf->frst; tmp != NULL; tmp = tmp->next){
                if(tmp->rtp_timestamp == pkt->fields.ts) {
                    TL_DEBUG_MSG(TL_PLAYOUT_BUF, DBG_MASK1,"Save a lost or out of order packet seq = %u\n",pkt->fields.seq);

                    found = 1;
                    pkt_insert = add_coded_unit(tmp, pkt);
                    break;
                }
                else if( tmp->next != NULL && 
                          ts_lt(tmp->rtp_timestamp, pkt->fields.ts) && 
                          ts_lt(pkt->fields.ts, tmp->next->rtp_timestamp) && 
                          pkt->fields.ts != tmp->next->rtp_timestamp) {
                    TL_DEBUG_MSG(TL_PLAYOUT_BUF, DBG_MASK1,"Create a pnode ,save a lost or out of order packet seq = %u\n",pkt->fields.seq);

                    found = 1;
                    tmp2 = create_new_pnode(pkt);
                    tmp2->prev = tmp;
                    tmp2->next = tmp->next;
                    tmp->next = tmp2;
                    tmp2->next->prev = tmp2;
                    playout_buf->size++;
                    pkt_insert = 1;
                    break;
                }
            }
            if(!found) free(pkt);
		}

	} 
    
    if(pkt_insert == 1)  playout_buf->pkt_count++;

    RECVING_BUF_UNLOCK(playout_buf);

	//pbuf_validate(playout_buf);
}

static void
free_cdata(coded_data *head)
{
	coded_data *tmp;

	while (head != NULL) {
		free(head->data);
		tmp  = head;
		head = head->next;
		free(tmp);
	}
}

void pbuf_remove_first(pbuf *playout_buf)
{
     pbuf_node *curr;  
     rtp_packet *packet;

     if(playout_buf->frst)
    {
         playout_buf->removed_flag = TRUE;
         
         curr = playout_buf->frst;

         playout_buf->last_removed_frame_max_seq = curr->seqno_max;

         if(curr->mbit)
             playout_buf->last_removed_frame_has_mbit = TRUE;
         else
             playout_buf->last_removed_frame_has_mbit = FALSE;

         if(curr->next)
        {
             playout_buf->frst = curr->next;
             curr->next->prev = NULL;
        }
        else
        {
            playout_buf->frst = NULL;
            playout_buf->last = NULL;
        }
        
        packet = curr->cdata->data;
        playout_buf->last_removed_timestamp = packet->fields.ts;
        
		playout_buf->pkt_count -= curr->cdata_count;
        free_cdata(curr->cdata);
        free(curr);
        playout_buf->size--;
         //TL_DEBUG_MSG(TL_PLAYOUT_BUF, DBG_MASK5,"pbuf_remove size=%d, frst=%x\n", playout_buf->size, playout_buf->frst);
        if(playout_buf->size && !(playout_buf->frst))
            TL_DEBUG_MSG(TL_PLAYOUT_BUF, DBG_MASK1,"playout buffer size does not match the playout buffer condition\n");
     }
}

void pbuf_flush(pbuf *playout_buf)
{
    
    
    while(playout_buf->frst)
   {
        pbuf_remove_first(playout_buf);
   }

   
    
}

void
pbuf_done(pbuf *playout_buf)
{
   

    /*
	if (playout_buf->size != 0) {
		TL_DEBUG_MSG(TL_PLAYOUT_BUF, DBG_MASK1,"ERROR: Destroying non-empty playout buffer\n");
		abort();
	}
    */
    TL_DEBUG_MSG(TL_PLAYOUT_BUF, DBG_MASK1, "playout buffer flush and destroy\n");
    RECVING_BUF_LOCK(playout_buf);
    pbuf_flush(playout_buf);
    RECVING_BUF_UNLOCK(playout_buf);
    
    free(playout_buf);    
}

void pbuf_remove(pbuf *playout_buf,  struct timeval curr_time)
{
	/* Remove previously decoded frames that have passed their playout  */
	/* time from the playout buffer. Incomplete frames that have passed */
	/* their playout time are also discarded.                           */

	pbuf_node *curr, *temp;

	pbuf_validate(playout_buf);

        curr=playout_buf->frst;
        while (curr != NULL) {
		temp = curr->next;
		if (tv_gt(curr_time, curr->discard_time)) {
			if (curr == playout_buf->frst) {
				playout_buf->frst = curr->next;
			}
			if (curr == playout_buf->last) {
				playout_buf->last = curr->prev;
			}
			if (curr->next != NULL) {
				curr->next->prev = curr->prev;
			}
			if (curr->prev != NULL) {
				curr->prev->next = curr->next;
			}
			assert(playout_buf->pkt_count - curr->cdata_count  >= 0);
            playout_buf->pkt_count -= curr->cdata_count;
			free_cdata(curr->cdata);
			free (curr);
			assert(playout_buf->size > 0);
			playout_buf->size--;
		} else {
			/* The playout buffer is stored in order, so once  */
			/* we see one frame that has not yet reached it's  */
			/* playout time, we can be sure none of the others */
			/* will have done so...                            */
			break;
		}
		curr = temp;
        }

	pbuf_validate(playout_buf);
	return;
}
/************************************************************************
* Funciton Name: frame_complete                                      
* Parameters: 
*                frame : the pointer of frame need to be verifyed
*                predict_frame_size : the predict frame size
* Return :       
*                1 frame complete                
*                0 frame not complete
*                
* Author:        Ian Tuan
* Description:
*                This function tells the upper layer if the selected frame
*                is complete 
*                
************************************************************************/
 int frame_complete(pbuf_node *frame, uint32_t *predict_frame_size, int prev_seqno_max, uint8_t* first_packet_data)
{
   /* Return non-zero if the list of coded_data represents a     */
  /* complete frame of video.                                   */
  /* A video-specific test: if we've received a packet with the */
  /* marker bit set or if we've started to received packets for */
  /* the next frame, we might have a complete packet. Otherwise */
  /* assume some data is missing.                               */

  coded_data	*curr, *prev;
  uint16_t      temp;
  int           jump = 0;
  uint32_t      frame_size = 0;
  rtp_packet    *packet;
  int           init_status;
  uint16_t      detect_seq;
  *first_packet_data = 0;
  if (frame->mbit) 
  {
    /* Does the number of packets in this frame match the gap between the */
    /* last sequence number of the previous frame and the first sequence  */
    /* number of the next frame? If so, we know we have a complete frame. */
    if ((frame->prev != NULL) && (frame->next != NULL)) 
    {
      unsigned gap = frame->next->seqno_min - frame->prev->seqno_max - 1;
      
      if (gap == frame->cdata_count) 
      {
        for(curr=frame->cdata;curr!=NULL;curr=curr->next)
        {
            packet = curr->data;
            frame_size += (packet->meta.data_len + 4); /*in case this is a single nalu*/
        }
        if (packet)
          *first_packet_data = *packet->meta.data;
        *predict_frame_size = frame_size;
        return 1;
      }
    }

	detect_seq  = prev_seqno_max % 65536 + 1u;
    init_status = prev_seqno_max == -1 ? 1 : 0;
    if( init_status != 1  &&  detect_seq != frame->seqno_min ){
        //printf("A packet loss location is at the head of frame,  %d ==== %d !!\n", detect_seq, frame->seqno_min);
        return 0;
    }
	
    /* Check for a complete frame by walking the list of packets... This  */
    /* is expensive, but only called if there's been reordering or packet */
    /* loss.                                                              */
    curr = frame->cdata;
    prev = NULL;
    while (curr != NULL) 
    {
      if (prev != NULL) 
      {
        temp = prev->seqno - 1u;
        if (curr->seqno != temp) 
        {
          jump = 1;  /* Frame is not complete... */
        }
      }
      packet = curr->data;
      frame_size += (packet->meta.data_len + 4);
      
      prev = curr;
      curr = curr->next;
    }

    if (packet)
      *first_packet_data = *packet->meta.data;
    *predict_frame_size = frame_size;
    if(jump)
        return 0;

    /* Frame complete but missing M bit... */
    return 1;
  }/*end if (frame->mbit || (frame->next != NULL))*/ 
        
  for(curr=frame->cdata;curr!=NULL;curr=curr->next)
  {
      packet = curr->data;
      frame_size += (packet->meta.data_len + 4); /*in case this is a single nalu*/
  }

  if (packet)
    *first_packet_data = *packet->meta.data;
  *predict_frame_size = frame_size;
  /* Frame is not complete... */
  return 0;
}
