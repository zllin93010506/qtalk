/*
 * FILE:   rtp.h
 * AUTHOR: Colin Perkins <csp@csperkins.org>
 *
 * Copyright (c) 2004-2005 University of Glasgow
 * Copyright (c) 2000-2004 University of Southern California
 * Copyright (c) 1998-2000 University College London
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
 *      Department at University College London.
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
 * $Revision: 1.15 $
 * $Date: 2005/03/15 10:20:45 $
 * 
 */

#ifndef __RTP_H__
#define __RTP_H__

#include "config_unix.h"

#define RTP_VERSION                 2
#define RTP_MAX_PACKET_LEN          9000
#include <stdbool.h>



/*
#if !defined(WORDS_BIGENDIAN) && !defined(WORDS_SMALLENDIAN)
#error RTP library requires WORDS_BIGENDIAN or WORDS_SMALLENDIAN to be defined.
#endif
*/
struct rtp;

/* XXX gtkdoc doesn't seem to be able to handle functions that return
 * struct *'s. */
typedef struct rtp *rtp_t;

typedef struct {
	/* The following are pointers to the data in the packet as    */
	/* it came off the wire. The packet it read in such that the  */
	/* header maps onto the latter part of this struct, and the   */
	/* fields in this first part of the struct point into it. The */
	/* entire packet can be freed by freeing this struct, without */
	/* having to free the csrc, data and extn blocks separately.  */
	struct {
		uint32_t	*csrc;
		unsigned char	*data;
		int		 data_len;
		unsigned char	*extn;
		uint16_t	 extn_len;	/* Size of the extension in 32 bit words minus one */
		uint16_t	 extn_type;	/* Extension type field in the RTP packet header   */
	} meta;
	struct {
		/* The following map directly onto the RTP packet header...   */
#ifdef WORDS_BIGENDIAN
		unsigned short   v:2;		/* packet type                */
		unsigned short   p:1;		/* padding flag               */
		unsigned short   x:1;		/* header extension flag      */
		unsigned short   cc:4;		/* CSRC count                 */
		unsigned short   m:1;		/* marker bit                 */
		unsigned short   pt:7;		/* payload type               */
#else
		unsigned short   cc:4;		/* CSRC count                 */
		unsigned short   x:1;		/* header extension flag      */
		unsigned short   p:1;		/* padding flag               */
		unsigned short   v:2;		/* packet type                */
		unsigned short   pt:7;		/* payload type               */
		unsigned short   m:1;		/* marker bit                 */
#endif
		uint16_t          seq;		/* sequence number            */
		uint32_t          ts;		/* timestamp                  */
		uint32_t          ssrc;		/* synchronization source     */
                /* additional fixed fields for the TFRC AVPCC profile         */
                /* ian modified : move the tfrc related parameters to extend header*/
		/*uint32_t	  send_ts;*/
		/*uint32_t          rtt;*/
                /* The csrc list, header extension and data follow, but can't */
                /* be represented in the struct.                              */
	} fields;
} rtp_packet;

typedef struct 
{
     uint32_t rtt;
     uint32_t send_ts;
     
}rtp_tfrc_extn;

typedef struct {
	uint32_t         ssrc;
	uint32_t         ntp_sec;
	uint32_t         ntp_frac;
	uint32_t         rtp_ts;
	uint32_t         sender_pcount;
	uint32_t         sender_bcount;
} rtcp_sr;

typedef struct {
	uint32_t	ssrc;		/* The ssrc to which this RR pertains */
//#ifdef WORDS_BIGENDIAN
	uint32_t	fract_lost:8;
	uint32_t	total_lost:24;
//#else
//	uint32_t	total_lost:24;
//	uint32_t	fract_lost:8;
//#endif	
	uint32_t	last_seq;
	uint32_t	jitter;
	uint32_t	lsr;
	uint32_t	dlsr;
} rtcp_rr;

typedef struct {
        uint32_t	ts;
	uint32_t	tdelay;
	uint32_t	x_recv;
	uint32_t	p;
} rtcp_rx;				/* TFRC extensions */

typedef struct {
        uint32_t	ts;
	uint32_t	tdelay;
	uint32_t	x_recv;
	uint32_t	p;
  uint32_t    rtt;
  uint32_t  ipi;
} tfrc_report_t;				/* TFRC extensions */

typedef struct { 			/* used for pointer arithmetic */
        uint32_t        ssrc;           /* The ssrc to which this RR pertains */
//#ifdef WORDS_BIGENDIAN
        uint32_t        fract_lost:8;
        uint32_t        total_lost:24;
//#else
//        uint32_t        total_lost:24;
//        uint32_t        fract_lost:8;
//#endif
        uint32_t        last_seq;
        uint32_t        jitter;
        uint32_t        lsr;
        uint32_t        dlsr;
        uint32_t        ts;
        uint32_t        tdelay;
        uint32_t        x_recv;
        uint32_t        p;
} rtcp_rr_rx;                              /* TFRC extensions */


typedef struct {
#ifdef WORDS_BIGENDIAN
	unsigned short  version:2;	/* RTP version            */
	unsigned short  p:1;		/* padding flag           */
	unsigned short  subtype:5;	/* application dependent  */
#else
	unsigned short  subtype:5;	/* application dependent  */
	unsigned short  p:1;		/* padding flag           */
	unsigned short  version:2;	/* RTP version            */
#endif
	unsigned short  pt:8;		/* packet type            */
	uint16_t        length;		/* packet length          */
	uint32_t        ssrc;
	char            name[4];        /* four ASCII characters  */
	char            data[1];        /* variable length field  */
} rtcp_app;

/* rtp_event type values. */
typedef enum {
    RX_RTP,
    RX_RTP_IOV,
    RX_SR,
    RX_RR,
    RX_TFRC_RX,         /* received TFRC extended report */
    RX_SDES,
    RX_BYE,             /* Source is leaving the session, database entry is still valid                           */
    SOURCE_CREATED,
    SOURCE_DELETED,     /* Source has been removed from the database                                              */
    RX_RR_EMPTY,        /* We've received an empty reception report block                                         */
    RX_RTCP_START,      /* Processing a compound RTCP packet about to start. The SSRC is not valid in this event. */
    RX_RTCP_FINISH,     /* Processing a compound RTCP packet finished. The SSRC is not valid in this event.       */
    RR_TIMEOUT,
    RX_APP,
    PEEK_RTP,
    APP_SET_TIME_INFO,

    CHECK_SSRC,         /* for changing opposite_ssrc, add by willy, 2010-10-11 */
    CHECK_RESEND_PKT,   /* check this rtp packet is resend packet or not by rfc4585 generic nack request. by frank */
    RX_RTCP_PSFB,
} rtp_event_type;

typedef struct {
    uint32_t            ssrc;
    rtp_event_type      type;
    void                *data;
} rtp_event;

/* Callback types */
typedef int (*rtp_callback)(struct rtp *session, rtp_event *e);
typedef rtcp_app* (*rtcp_app_callback)(struct rtp *session, uint32_t rtp_ts, int max_size);

/* SDES packet types... */
typedef enum  {
        RTCP_SDES_END   = 0,
        RTCP_SDES_CNAME = 1,
        RTCP_SDES_NAME  = 2,
        RTCP_SDES_EMAIL = 3,
        RTCP_SDES_PHONE = 4,
        RTCP_SDES_LOC   = 5,
        RTCP_SDES_TOOL  = 6,
        RTCP_SDES_NOTE  = 7,
        RTCP_SDES_PRIV  = 8
} rtcp_sdes_type;

typedef struct {
	uint8_t		type;		/* type of SDES item              */
	uint8_t		length;		/* length of SDES item (in bytes) */
	char		data[1];	/* text, not zero-terminated      */
} rtcp_sdes_item;

typedef struct {
    uint32_t ntp_sec;
    uint32_t ntp_frac;
    uint32_t rtp_ts;
}ntp_rtpts_pair;

/* RTP options */
typedef enum {
    RTP_OPT_PROMISC 	  = 1,
    RTP_OPT_WEAK_VALIDATION	  = 2,
    RTP_OPT_FILTER_MY_PACKETS = 3,
	RTP_OPT_REUSE_PACKET_BUFS = 4,	/* Each data packet is written into the same buffer, */
	                                /* rather than malloc()ing a new buffer each time.   */
    RTP_OPT_PEEK              = 5
} rtp_option;

typedef struct {
#ifdef WORDS_BIGENDIAN
	unsigned short  version:2;	/* packet type            */
	unsigned short  p:1;		/* padding flag           */
	unsigned short  count:5;	/* varies by payload type */
	unsigned short  pt:8;		/* payload type           */
#else
	unsigned short  count:5;	/* varies by payload type */
	unsigned short  p:1;		/* padding flag           */
	unsigned short  version:2;	/* packet type            */
	unsigned short  pt:8;		/* payload type           */
#endif
	uint16_t        length;		/* packet length          */
} rtcp_common;

typedef struct {
	rtcp_common   common;	
	union {
		struct {
			rtcp_sr		    sr;
			rtcp_rr       	rr[1];		/* variable-length list */
		} sr;
        struct {
            rtcp_sr         sr;            
			rtcp_rr         rr[1];
            rtcp_rx         rx[1];          /* TFRC RR extensions   */
         } srx;
		struct {
			uint32_t        ssrc;		/* source this RTCP packet is coming from */
			rtcp_rr       	rr[1];		/* variable-length list */
		} rr;
	        struct {
            uint32_t      ssrc;           /* source this RTCP packet is coming from */
			rtcp_rr         rr[1];
			rtcp_rx		   rx[1];          /* TFRC RR extensions */
            } rrx;
		struct rtcp_sdes_t {
			uint32_t	            ssrc;
			rtcp_sdes_item 	item[1];	/* list of SDES */
		} sdes;
		struct {
			uint32_t        ssrc[1];	/* list of sources */
							/* can't express the trailing text... */
		} bye;
		struct {
			uint32_t        ssrc;           
			uint8_t          name[4];
			uint8_t          data[1];
		} app;
		struct {
		       uint32_t ssrc;
               uint32_t mssrc; /*media source*/
		       uint8_t  nack_data[1];
		}nack;
	} r;
} rtcp_t;

typedef struct {			// FEC parameters
    int ctrl;               // 0:disable, 1:enable 
    int fec_on;
    int send_payloadType;   // used by sender
    int receive_payloadType;// used by receiver
    unsigned long ea_id;
    unsigned long ecMod_id;
} RTP_ECControl_t;


/* API */
rtp_t		rtp_init(const char *addr, 
			 uint16_t rx_port, uint16_t tx_port, 
			 int ttl, double rtcp_bw, 
			 int tfrc_on,
			 RTP_ECControl_t ec_arg,
			 int resend_enable,
			 int tos,
			 rtp_callback callback,
			 uint8_t *userdata,
             const char* local_addr);
rtp_t		rtp_init_if(const char *addr, char *iface, 
			    uint16_t rx_port, uint16_t tx_port, 
			    int ttl, double rtcp_bw, 
			    int tfrc_on,
				RTP_ECControl_t ec_arg,
			    int resend_enable,
				int tos,
			    rtp_callback callback,
			    uint8_t *userdata,
                const char* local_addr);

void		 rtp_send_bye(struct rtp *session);
void		 rtp_done(struct rtp *session);

int 		 rtp_set_option(struct rtp *session, rtp_option optname, int optval);
int 		 rtp_get_option(struct rtp *session, rtp_option optname, int *optval);

int 		 rtp_recv(struct rtp *session, 
			  struct timeval *timeout, uint32_t curr_rtp_ts);

int 		 rtp_send_data(struct rtp *session, 
			       uint32_t rtp_ts, char pt, int m, 
			       int cc, uint32_t csrc[], 
                               unsigned char *data, int data_len, uint16_t *sent_seq,
			       unsigned char *extn, uint16_t extn_len, uint16_t extn_type);

int rtp_resend_data(struct rtp *session, uint16_t seq, uint32_t rtp_ts, char pt, int m, 
		  int cc, uint32_t* csrc, 
                  unsigned char *data, int data_len, 
		  unsigned char *extn, uint16_t extn_len, uint16_t extn_type);

int 		 rtp_send_data_hdr(struct rtp *session, long sequence_num,
			       uint32_t rtp_ts, char pt, int m, 
			       int cc, uint32_t csrc[], 
                               unsigned char *phdr, int phdr_len, 
                               unsigned char *data, int data_len, uint16_t *sent_seq,
			       unsigned char *extn, uint16_t extn_len, uint16_t extn_type);

void 		 rtp_send_ctrl(struct rtp *session, uint32_t rtp_ts, 
			       rtcp_app_callback appcallback, struct timeval curr_time);
void 		 rtp_update(struct rtp *session, struct timeval curr_time);

uint32_t	 rtp_my_ssrc(struct rtp *session);

int		 rtp_set_sdes(struct rtp *session, uint32_t ssrc, 
			      rtcp_sdes_type type, 
			      const char *value, int length);
const char	*rtp_get_sdes(struct rtp *session, uint32_t ssrc, rtcp_sdes_type type);

const rtcp_sr	*rtp_get_sr(struct rtp *session, uint32_t ssrc);
const rtcp_rr	*rtp_get_rr(struct rtp *session, uint32_t reporter, uint32_t reportee);

int              rtp_set_my_ssrc(struct rtp *session, uint32_t ssrc);

char 		*rtp_get_addr(struct rtp *session);
uint16_t	 rtp_get_rx_port(struct rtp *session);
uint16_t	 rtp_get_tx_port(struct rtp *session);
uint16_t     rtp_get_seq(struct rtp *session);
int		 rtp_get_ttl(struct rtp *session);
uint8_t		*rtp_get_userdata(struct rtp *session);
void 		 rtp_set_recv_iov(struct rtp *session, struct msghdr *m);

int rtp_set_rtcp_time_info(struct rtp *session, ntp_rtpts_pair *info);

void set_rtp_opposite_ssrc(struct rtp *session, uint32_t ssrc);
uint32_t rtp_opposite_ssrc(struct rtp *session);

int rtp_set_tfrc_min_sending_rate(struct rtp *session, uint32_t min_sending_rate);

//Corey - FEC 
int rtp_recover(struct rtp *session, uint16_t seq);


void rtp_check_rfc4585(struct rtp *session);

void send_rtpfb_generic_nack(struct rtp *session, int pid, int bitmask);
void send_psfb_slice_loss_indication(struct rtp *session, uint16_t first, uint16_t number, uint8_t picture_id);
void send_psfb_picture_loss_indication(struct rtp *session);
void send_psfb_full_intra_request(struct rtp *session);
int rtp_set_media_type(struct rtp *session, int media_type);


void diable_rtcp(struct rtp *session);
void enable_rtcp(struct rtp *session);

unsigned long rtp_get_tl_session(struct rtp *session);
void rtp_set_tl_session(struct rtp *session, unsigned long tl_session);
uint8_t rtp_fir_fci_seq_num(struct rtp *session);

#endif /* __RTP_H__ */
