/*
 * FILE:     rtp_callback.c
 * AUTHOR:   Colin Perkins <csp@csperkins.org>
 *           Ladan Gharai  <ladan@isi.edu>
 *
 * Copyright (c) 2001-2003 University of Southern California
 * Copyright (c) 2003-2004 University of Glasgow
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
 * ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING,
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
 * $Revision: 1.49 $
 * $Date: 2005/04/25 08:38:28 $
 *
 */
#ifdef WIN32
#include "config_w32.h"
#endif
#include "config.h"
#include "config_unix.h"
#include "rtp_callback.h"
#include "debug.h"
#include "ntp.h"
#include "tv.h"
#include "pbuf.h"
#include "pdb.h"
#include "ET1_QStream_TransportLayer.h"
//#include "tfrc.h"

/*QS_DMSG_LATENCY -- video call latency message flag*/
#include "../GlobalDefine.h"
/*QS_DMSG_LATENCY -- video call latency message flag*/

//extern int		 should_exit;
//extern char		*frame;

//char		 	 hdr_buf[100];
//struct msghdr		 msg;
//struct iovec		 iov[10];
/*
extern uint32_t  RTT;
*/
static void
process_rr(struct rtp *session, rtp_event *e)
{
	float		fract_lost, tmp;
        uint32_t	ntp_sec, ntp_frac, now; 
	rtcp_rr		*r = (rtcp_rr *) e->data;

	if (e->ssrc == rtp_my_ssrc(session)) {
		/* Filter out loopback reports */
		return;
	}

	if (r->ssrc == rtp_my_ssrc(session)) {
		/* Received a reception quality report for data we are sending.  */
		/*                                                               */
		/* Check for excessive congestion: exit if it occurs, to protect */
		/* other users of the network.                                   */
		fract_lost = (r->fract_lost * 100.0) / 256.0;	/* percentage lost packets */
#ifdef NDEF
		if (fract_lost > 20) {	
			debug_msg("Receiver 0x%08x reports excessive congestion, exiting\n", e->ssrc);
			exit(1);
		} /* we want it to look bad! ha ha */
#endif
		//debug_msg("Fraction lost %0.2f\n", fract_lost);

		/* Compute network round-trip time:                              */
		if (r->lsr != 0) {
			ntp64_time(&ntp_sec, &ntp_frac);
			now = ntp64_to_ntp32(ntp_sec, ntp_frac); 

			if ((now < r->lsr) || (now - r->lsr < r->dlsr)) {
				/* Packet arrived before it was sent, ignore. This represents */
				/* a bug in the remote: either our timestamp was incorrectly  */
				/* echoed back to us, or the remote miscalculated in time for */
				/* which it held the packet.                                  */
				debug_msg("Bogus RTT from 0x%08x ignored\n", e->ssrc);
			} else {
				tmp = ((float) (now-r->lsr-r->dlsr))/65536.0; /* RTT in seconds */
				 /*
                   RTT = tmp * 1000000; 
                  */
			}
           /*
			debug_msg("  RTT=%d usec\n", RTT);
           */
		}
	}
}

static void
process_sdes(pdb *participants, uint32_t ssrc, rtcp_sdes_item *d)
{
	pdb_e	*e;
	char		*sdes_item;

	e = pdb_get(participants, ssrc);
	if (e == NULL) {
		/* We got an SDES packet for a participant we haven't previously seen. */
		/* This can happen, for example, in a partitioned multicast group. Log */
		/* the event, and add the new participant to the database.             */
		debug_msg("Added previously unseen participant 0x%08x due to receipt of SDES\n", ssrc);
		pdb_add(participants, ssrc);
		e = pdb_get(participants, ssrc);
	}

	sdes_item = calloc(d->length+1, 1);
	strncpy(sdes_item, d->data, d->length);

	switch (d->type) {
	case RTCP_SDES_END:
		/* This is the end of the SDES list of a packet. */ 
		/* Nothing for us to deal with.                  */                
		break;        
	case RTCP_SDES_CNAME:
		if (e->sdes_cname != NULL) free(e->sdes_cname);
		e->sdes_cname = sdes_item;
		break;
	case RTCP_SDES_NAME:
		if (e->sdes_name != NULL) free(e->sdes_name);
		e->sdes_name = sdes_item;
		break;
	case RTCP_SDES_EMAIL:
		if (e->sdes_email != NULL) free(e->sdes_email);
		e->sdes_email = sdes_item;
		break;
	case RTCP_SDES_PHONE:
		if (e->sdes_phone != NULL) free(e->sdes_phone);
		e->sdes_phone = sdes_item;
		break;
	case RTCP_SDES_LOC:
		if (e->sdes_loc != NULL) free(e->sdes_loc);
		e->sdes_loc = sdes_item;
		break;
       	case RTCP_SDES_TOOL:
		if (e->sdes_tool != NULL) free(e->sdes_tool);
		e->sdes_tool = sdes_item;
		break;
	case RTCP_SDES_NOTE:
		if (e->sdes_note != NULL) free(e->sdes_note);
		e->sdes_note = sdes_item;
		break;
	case RTCP_SDES_PRIV:
		break; /* Ignore private extensions */
	default:
        debug_msg("Ignored unknown SDES item (type=0x%02x) from 0x%08x\n", ssrc);
	}
}


int rtp_recv_callback(struct rtp *session,  rtp_event *e)
{
    //static int i=0;
    int ret = 0;
    rtp_packet *pckt_rtp = (rtp_packet *) e->data;
    pdb	*participants = (pdb *) rtp_get_userdata(session);
    pdb_e *state = pdb_get(participants, e->ssrc);
    int value;
    //uint8_t type;
     
    switch (e->type) 
    {
        case RX_RTP:
            if (pckt_rtp->meta.data_len > 0) 
            {		
                /* divite RX_RTP into RX_RTP and RX_DTMF(new) */ /* add by willy, 2010-10-11 */
                if(pckt_rtp->fields.pt == participants->allowed_pt)
                {
                    /* TransportLayer_RTCPCallback with event CHECK_SSRC */ /* add by willy, 2010-10-11 */
                    e->type = CHECK_SSRC;
                    TransportLayer_RTCPCallback(session, e);
                    
                    /* To filter out H.264 NAL type 9 , add by Eric, 2011.11.18 */
                    if( *(pckt_rtp->meta.data) == 0x9 ) {
                        TransportLayer_GetSessionInfo(session, MEDIA_TYPE, &value);
                        if( value == PAYLOAD_VIDEO){
                            //printf("\n>>>>>>>>>>>>> Free the H.264 NAL type 9 packet (nal_type=%d, media_type=%d)\n\n", *(pckt_rtp->meta.data), value);
                            free(pckt_rtp);  
                            break;
                        }
                    }       

                    pbuf_insert(state->playout_buffer, pckt_rtp);
                }
                else
                {
                    /* RTP event handle, ex.DTMF */
                    TransportLayer_RTCPCallback(session, e);
                    free(pckt_rtp);
                }
            }			
		    break;

        case RX_TFRC_RX:
		    /* compute TCP friendly data rate */
            TransportLayer_RTCPCallback(session, e);
            break;
        case RX_RTCP_START:
            break;
        case RX_RTCP_FINISH:
            break;
        case RX_SR:
            TransportLayer_RTCPCallback(session, e);
            break;
	    case RX_RR:
		    process_rr(session, e);
		    TransportLayer_RTCPCallback(session, e);
		    break;
	    case RX_RR_EMPTY:
		    break;
	    case RX_SDES:
		    process_sdes(participants, e->ssrc, (rtcp_sdes_item *) e->data);
		    break;
	    case RX_APP:
            TransportLayer_RTCPCallback(session, e);
		    break;
	    case RX_BYE:
		    break;
	    case SOURCE_DELETED:
		    break;
	    case SOURCE_CREATED:
            pdb_add(participants, e->ssrc);
            TransportLayer_RTCPCallback(session, e);
            break;
        case RR_TIMEOUT:
            break;
        case APP_SET_TIME_INFO:
            TransportLayer_RTCPCallback(session,e);
            break;
        case CHECK_RESEND_PKT:
            ret = TransportLayer_RTCPCallback(session,e);
            break;
        default:
            debug_msg("Unknown RTP event (type=%d)\n", e->type);
    }
    return ret;
}

