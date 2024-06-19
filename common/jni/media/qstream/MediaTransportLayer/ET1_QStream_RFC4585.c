/*
 * FILE:     ET1_QStream_RFC4585.c
 * AUTHOR:   Ian Tuan   <ian.tuan@quantatw.com>
 * MODIFIED: 
 *
 * The routines in this file implement the format and analysis funciton of 
 *  RFC4585 AV Profile feedback message
 *
 * Copyright (c) Quanta Computer Inc.
 * All rights reserved.
 *
 */

#include "ET1_QStream_RFC4585.h"
#include "ET1_QStream_TransportLayer.h"
#include "pbuf.h"
#include "debug.h"
#include "ET1_QStream_SendingBuf.h"
#include "rtp_payload_type_define.h"


#define MAX_ONE_NACK_LOST_COUNT     (16+1)
#define MAX_LOST_DIFF               50

typedef struct 
{
    uint16_t packet_id;
    uint16_t bitmask;
} GenericNack_Msg_t;


/**
 * rfc4585_format_rtpfb_generic_nack:
 * @session: the session pointer (returned by rtp_init())
 *
 * format Transport-Layer Feedback Message for generic_nack
 * PT=RTPFB and FMT=1
 *
 * author: frank
 */
uint8_t *rfc4585_format_rtpfb_generic_nack(uint8_t *buffer, int buflen, int pid, int bitmask, struct rtp *session)
{
    uint8_t             *packet = buffer;
    int                 packed_nack_msg_count = 0;
    rtcp_common         *common;
    GenericNack_Msg_t   nack_msg;

    common = (rtcp_common *)packet;

    /* rtcp process ++ */
    common->version = 2;
    common->p       = 0;
    common->count   = 1;        /*this field is the smae to FMT*/
    common->pt      = RTPFB;
    common->length  = htons(2); /*temporarily give a length, this will may change after caculaing*/ 
     
    packet += sizeof(rtcp_common);

    *((uint32_t *) packet) = htonl(rtp_my_ssrc(session));
      packet += 4;
    *((uint32_t *) packet) = htonl(rtp_my_ssrc(session)); /*in one to one case, this the reporter's ssrc is same to media source's ssrc*/
      packet += 4; 

    nack_msg.packet_id = htons(pid); // current lost seq
    nack_msg.bitmask = htons(bitmask);

    packed_nack_msg_count = 0;
    packet += (packed_nack_msg_count * sizeof(GenericNack_Msg_t));
    
    memcpy(packet, &nack_msg, sizeof(GenericNack_Msg_t));

    packed_nack_msg_count++;    
    packet += (packed_nack_msg_count * sizeof(GenericNack_Msg_t));

    common->length = htons(packed_nack_msg_count + 2);

    return packet;
}

/**
 * rfc4585_format_psfb_picture_loss_indication:
 * @session: the session pointer (returned by rtp_init())
 *
 * format Payload-Specific Feedback Messages for picture_loss_indication
 * PT=PSFB and FMT=1
 *
 * author: frank
 */
uint8_t *rfc4585_format_psfb_picture_loss_indication(uint8_t *buffer, int buflen, struct rtp *session)
{
    uint8_t             *packet = buffer;
    int                 packed_nack_msg_count = 0;
    rtcp_common         *common;

    common = (rtcp_common *)packet;

    /* rtcp process ++ */
    common->version = 2;
    common->p       = 0;
    common->count   = 1;        /*this field is the smae to FMT*/
    common->pt      = PSFB;
    common->length  = htons(2); /*temporarily give a length, this will may change after caculaing*/ 
     
    packet += sizeof(rtcp_common);

    *((uint32_t *) packet) = htonl(rtp_my_ssrc(session));
      packet += 4;
    *((uint32_t *) packet) = htonl(rtp_my_ssrc(session)); /*in one to one case, this the reporter's ssrc is same to media source's ssrc*/
      packet += 4; 

    return packet;
}

/**
 * rfc4585_format_psfb_slice_loss_indication:
 * @session: the session pointer (returned by rtp_init())
 *
 * format Payload-Specific Feedback Messages for slice_loss_indication
 * PT=PSFB and FMT=2
 *
 * author: frank
 */
uint8_t *rfc4585_format_psfb_slice_loss_indication(uint8_t *buffer, int buflen, uint16_t first, uint16_t number, uint8_t picture_id, struct rtp *session)
{
    uint8_t             *packet = buffer;
    int                 packed_nack_msg_count = 0;
    rtcp_common         *common;

    common = (rtcp_common *)packet;

    /* rtcp process ++ */
    common->version = 2;
    common->p       = 0;
    common->count   = 2;        /*this field is the smae to FMT*/
    common->pt      = PSFB;
    common->length  = htons(2); /*temporarily give a length, this will may change after caculaing*/ 
     
    packet += sizeof(rtcp_common);

    *((uint32_t *) packet) = htonl(rtp_my_ssrc(session));
      packet += 4;
    *((uint32_t *) packet) = htonl(rtp_my_ssrc(session)); /*in one to one case, this the reporter's ssrc is same to media source's ssrc*/
      packet += 4; 

    return packet;
}


#if 0
 /************************************************************************
 * Funciton Name: RFC4585_FormatGenricNACK                                      
 * Parameters: 
 *                buffer : the caller should give a buffer address to fill the nack message
 *                buflen : the caller should give the total buffer space to prevent overflow
 *                session: we should get some rtp session information
 *                playout_buf : the caller should tell us which playout buffer need report nack message
 *            
 * Author:        Ian Tuan
 * Description:
 *     
 *                
 ************************************************************************/
uint8_t *RFC4585_FormatGenricNACK(uint8_t *buffer, int buflen, struct rtp *session, pbuf *playout_buf)
{
     //rtcp_t                          *packet = (rtcp_t *) buffer;   
	uint8_t		                    *packet = buffer;
	rtcp_common	            *common = (rtcp_common *) buffer;
     coded_data                *codeddata;
     pbuf_node                  *frame;
     uint16_t                       total_lost_count=0; /*record if we found the first lost seq*/
     uint16_t                       lost_seq;
     GenericNack_Msg_t    nack_msg;
     uint16_t                       bitmask;
     uint8_t                         is_lost = FALSE;
     uint16_t                       last_record_lost_seq;
     uint16_t                       curr_lost_seq, last_record_nack_lost_seq;
     int                               packed_nack_msg_count = 0;
     uint16_t                       temp, diff;
     int                               i = 0;

     common->version = 2;
     common->p       = 0;
     common->count   = 1; /*this field is the smae to FMT*/
     common->pt      = RTPFB;
     common->length  = htons(2); /*temporarily give a length, this will may change after caculaing*/ 
     
    packet += sizeof(common);

	*((uint32_t *) packet) = htonl(rtp_my_ssrc(session));
      packet += 4;
    *((uint32_t *) packet) = htonl(rtp_my_ssrc(session)); /*in one to one case, this the reporter's ssrc is same to media source's ssrc*/
      packet += 4; 

     /*check the packets seq needed resend*/
     RECVING_BUF_LOCK(playout_buf);
     
     /*check if the buffer is not empty first*/
     if(playout_buf->frst)
     {
         frame = playout_buf->frst;     
         
         while(frame)
         {      
             /*
              if we get the mark bit or next frame packet,
              then we can check if we lost packets further. 
              Otherwise, we can wait a moment
             */
             if((frame->mbit) || (frame->next != NULL))
             {
                 /*the playout buffer record the latest received packet in the buffer head
                   so we back to the oldest seq
                 */ 
                 if(frame->resend_check_flag)
                 {
                     /*if this frame we have checked before then pass it*/ 
                     frame = frame->next;
                     continue;
                 }

                 for(codeddata = frame->cdata; codeddata->next != NULL; codeddata = codeddata->next)
                 ;
             
                 /*
                 if (codeddata->prev != NULL)
                  codeddata = codeddata->prev;
                 */

                 while(codeddata)
                 {
                     if(codeddata->next)
                    {
                         temp = codeddata->next->seqno + 1u;
                     }
                     else
                    {
                         /*this is the first packet we received currently in this frame*/
                        if(frame->prev)
                        {
                               temp = frame->prev->seqno_max + 1u;
                         }
                         else
                        {
                             codeddata = codeddata->prev;
                             continue;
                         }
                         //else if((playout_buf->removed_flag) && (playout_buf->last_removed_frame_has_mbit))
                         //{
                         //      temp = playout_buf->last_removed_frame_max_seq + 1u;
                         //}
                     }

                     if (codeddata->seqno != temp) 
                     {
                          /*lost event found and count how many packets lost*/
                          diff = codeddata->seqno - temp;

                          if(diff > MAX_LOST_DIFF)
                          {
                               TL_DEBUG_MSG(TL_RFC4585, DBG_MASK1, " RFC lost too many packets seqno = %d last seqno = %d %d\n", codeddata->seqno, temp - 1 , diff );
                               break;
                          }

                          TL_DEBUG_MSG(TL_RFC4585, DBG_MASK1, " RFC4585 lost event detect  curr seq = %d, last seq + 1 = %d, diff = %d\n", codeddata->seqno, temp, diff );
                  
                         /*record all the lost packets in this lost event*/
                         for(i=0; i < diff; i++)
                         {
                             total_lost_count++;
                             if(!is_lost)
                            {
                                  /*this is the first lost seq we reocrd*/
                                  is_lost = TRUE;
                                  curr_lost_seq = temp + i;
                           
                           
                                  /*this is the first seq id of a nack msg*/
                                  nack_msg.packet_id = htons(curr_lost_seq);
                                  nack_msg.bitmask = htons(0);
                                  bitmask = 0;
                                  last_record_nack_lost_seq = curr_lost_seq;
                             }
                             else
                             {
                           
                                 curr_lost_seq = temp + i;
                           
                                 if((curr_lost_seq - last_record_lost_seq) >= MAX_ONE_NACK_LOST_COUNT)
                                 {
                                   nack_msg.bitmask = htons(bitmask);
                                   /*copy the last nack message*/
                                   memcpy(packet + (packed_nack_msg_count * sizeof(GenericNack_Msg_t)), &nack_msg, sizeof(GenericNack_Msg_t));
                                   packed_nack_msg_count ++;
                               
                                   nack_msg.packet_id = htons(curr_lost_seq);
                                   nack_msg.bitmask = htons(0);
                                   bitmask = 0;
                                   last_record_nack_lost_seq = curr_lost_seq;
                                 }
                                 else
                                 {
                                     /*this lost seq id can be record in the bitmask*/
                                     //nack_msg.bitmask = nack_msg.bitmask | (1 << (curr_lost_seq - last_record_nack_lost_seq - 1));
                                     bitmask = bitmask | (1 << (curr_lost_seq - last_record_nack_lost_seq - 1));
                                 }
                             } /*end if else*/
                         }/*end  for(i=0;*/
                     }/*if (codeddata->seqno != temp) */
                 
                     codeddata = codeddata->prev;

                 }/*end while(codeddata)*/

             }/*if((frame->mbit)*/

             frame->resend_check_flag = TRUE;

             frame = frame->next;

         }/*end while(frame)*/
          
         if(is_lost)
         {
             nack_msg.bitmask = htons(bitmask);
             memcpy(packet + (packed_nack_msg_count * sizeof(GenericNack_Msg_t)), &nack_msg, sizeof(GenericNack_Msg_t));

             packed_nack_msg_count++;
                  
             common->length = htons(packed_nack_msg_count + 2);
             
            TL_DEBUG_MSG(TL_RFC4585, DBG_MASK1, " RFC4585 Generic Nack total_lost_count = %d packed_nack_msg_count = %d\n", total_lost_count,packed_nack_msg_count);
     
             RECVING_BUF_UNLOCK(playout_buf);

             return (packet + packed_nack_msg_count * sizeof(GenericNack_Msg_t));
         }
         else
         {
             //TL_DEBUG_MSG(TL_RFC4585, DBG_MASK1, " RFC4585 no lost \n");
             RECVING_BUF_UNLOCK(playout_buf);
             return buffer;
         }

     }/*end if(playout_buf->...)*/

     //TL_DEBUG_MSG(TL_RFC4585, DBG_MASK1, " RFC4585 buffer empty \n");
     RECVING_BUF_UNLOCK(playout_buf);
     
     return buffer;

     
}
#endif

 /************************************************************************
 * Funciton Name:   RFC4585_ProcessGenricNACK                                      
 * Parameters:      *session: rtp session pointer
 *                  packet: rtcp packet to analyze the Generic Nack message           
 * 
 * Author:          Ian / Frank
 * Description:
 *                
 ************************************************************************/
int RFC4585_ProcessGenricNACK(struct rtp *session, rtcp_t *packet)
{
    struct TL_Session_t *tl_session;
    SendingBuf_t *sending_buffer;
    GenericNack_Msg_t *nack_msg;
    uint16_t resend_seq;
	uint16_t seq_base;
    uint16_t bitmask;
    int i = 0;
    int j = 0;
//
//    tl_session = TransportLayer_GetTLSession(session);
//    
//    if(tl_session)
//    {
//         sending_buffer = tl_session->sending_buffer;
//
//         /*the pli is start from the 3rd word, so pass the frist two send ssrc and media ssrc*/
//         for(i = 0; i < (ntohs(packet->common.length) - 2); i++)
//         {
//             nack_msg = (GenericNack_Msg_t*)(packet->r.nack.nack_data + (i * 4));
//            
//             /*check the first seq of one nack message*/
//             resend_seq = ntohs(nack_msg->packet_id);
//             TL_DEBUG_MSG(TL_RFC4585, DBG_MASK1, "[rfc4585,nack] re-send pid:%d, len:%d\n", resend_seq, ntohs(packet->common.length));
//             SendingBuf_Resend(sending_buffer, resend_seq);
//             /*check the bitmask*/
//             for(j=0; j < 16; j++)
//             {
//                 if(ntohs(nack_msg->bitmask) && (1 << j))
//                 {
//                     resend_seq = resend_seq + j + 1;
//                     TL_DEBUG_MSG(TL_RFC4585, DBG_MASK1, "[rfc4585,nack] re-send bitmask:%d\n", resend_seq);
//                     SendingBuf_Resend(sending_buffer, resend_seq);
//                 
//                 }
//             
//             }/*end for j*/
//         }/*end for i*/
//    
//         return (1);
//    } /*end if (tl_session)*/    

    return (0);
}


