/*
 * FILE:     ET1_QStream_RFC5104.c
 * AUTHOR:   Frank Ting   <frank.ting@quantatw.com>
 * MODIFIED: 
 *
 * The routines in this file implement the format and analysis funciton of 
 * RFC5104 Codec Control Messages in the RTP AVPF
 *
 * Copyright (c) Quanta Computer Inc.
 * All rights reserved.
 *
 */

#include "ET1_QStream_RFC5104.h"
#include "ET1_QStream_TransportLayer.h"
#include "pbuf.h"
#include "debug.h"
#include "ET1_QStream_SendingBuf.h"
#include "rtp_payload_type_define.h"

/**
 * rfc5104_format_full_intra_request:
 * @session: the session pointer (returned by rtp_init())
 *
 * format Payload-Specific Feedback Messages for full_intra_request
 * PT=PSFB and FMT=4
 *
 * author: frank
 */
uint8_t *rfc5104_format_psfb_full_intra_request(uint8_t *buffer, int buflen, struct rtp *session)
{
    uint8_t             *packet = buffer;
    int                 packed_nack_msg_count = 0;
    rtcp_common         *common;

    common = (rtcp_common *)packet;

    /* rtcp process */
    common->version = 2;
    common->p       = 0;
    common->count   = 4;        /*this field is the smae to FMT*/
    common->pt      = PSFB;
    common->length  = htons(4); /*temporarily give a length, this will may change after caculaing*/ 
     
    packet += sizeof(rtcp_common);

    *((uint32_t *) packet) = htonl(rtp_my_ssrc(session));
    packet += 4;
    *((uint32_t *) packet) = 0; //htonl(rtp_my_ssrc(session)); /*in one to one case, the ssrc of reporter is same to media source's ssrc*/
    packet += 4; 

    /* FCI, opposite ssrc (4 bytes) */
    *((uint32_t *) packet) = htonl(rtp_opposite_ssrc(session));
    packet += 4;

    /* seg num (1 byte) and reserved (3 bytes) */
    *((uint32_t *) packet) = 0;
    *((uint8_t *) packet) = rtp_fir_fci_seq_num(session);
    packet += 4;
    
    return packet;
}


