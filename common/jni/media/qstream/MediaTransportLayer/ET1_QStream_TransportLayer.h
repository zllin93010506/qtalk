/*
 * FILE:     ET1_QStream_TransportLayer.h
 * AUTHOR:   Ian Tuan   <ian.tuan@quantatw.com>
 * MODIFIED: 
 *
 *
 * Copyright (c) Quanta Computer Inc.
 * All rights reserved.
 *
 */
 
#ifndef _ET1_QSTREAM_TRANSPORT_LAYER
#define _ET1_QSTREAM_TRANSPORT_LAYER

#include "config_unix.h"
#include "pdb.h"
#include "rtp.h"
#include "ET1_QStream_SendingBuf.h"
#include "../GlobalDefine.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//Corey -  FEC
#include "rtp_multiStream.h"
#include "ec_agent.h"

#ifndef WIN32
#include <semaphore.h>
#endif

#define MEDIA_TYPE               1

#define PAYLOAD_AUDIO_CONTINUOUS 0
#define PAYLOAD_AUDIO_PACKETIZED 1
#define PAYLOAD_VIDEO 2
#define PAYLOAD_OTHER 3           

#define TL_SUCCESS 1
#define TL_FAIL -1

#define RTP_TTL 255

#define CHANGE_OPPOSITE_SSRC_THRESHOLD  10	/* the threshold to change the opposite ssrc, add by willy, 2010-10-11 */

typedef struct
{
    int       type;                            /**< one of PAYLOAD_* macros*/
    int       clock_rate;                  /**< rtp clock rate*/
    char      bits_per_sample;	      /* in case of continuous audio data */
    char      zero_pattern[MAX_PATH];
    int       pattern_length;
    /* other useful information for the application*/
    int       normal_bitrate;	     /*in bit/s */
    char      mime_type[MAX_PATH];            /**<actually the submime, ex: pcm, pcma, gsm*/
    int       channels;                 /**< number of channels of audio */
    uint16_t  pt_num;
    uint16_t  rtp_event_pt_num;

}PayloadType_t;

#define   TYPE(val)                         (val)
#define	  CLOCK_RATE(val)                   (val)
#define	  BITS_PER_SAMPLE(val)              (val)
#define	  ZERO_PATTERN(val)                 (val)
#define	  PATTERN_LENGTH(val)               (val)
#define	  NORMAL_BITRATE(val)               (val)
#define	  MIME_TYPE(val)                    (val)
#define	  CHANNELS(val)                     (val)
#define   PAYLOAD_TYPE_NUM(val)             (val)  
#define   PAYLOAD_RTP_EVENT_TYPE_NUM(val)  (val)
/* rtp_event type values. */
typedef enum {
        TL_RX_RTP,
	    TL_RX_RTP_IOV,
        TL_RX_SR,
        TL_RX_RR,
    	TL_RX_TFRC_RX,	/* received TFRC extended report */
        TL_RX_SDES,
        TL_RX_BYE,         /* Source is leaving the session, database entry is still valid                           */
        TL_SOURCE_CREATED,
        TL_SOURCE_DELETED, /* Source has been removed from the database                                              */
        TL_RX_RR_EMPTY,    /* We've received an empty reception report block                                         */
        TL_RX_RTCP_START,  /* Processing a compound RTCP packet about to start. The SSRC is not valid in this event. */
        TL_RX_RTCP_FINISH,	/* Processing a compound RTCP packet finished. The SSRC is not valid in this event.       */
        TL_RR_TIMEOUT,
        TL_RX_APP,
	    TL_PEEK_RTP,
        TL_APP_SET_TIME_INFO,
        TL_APP_SET_RTCP_APP,

        TL_RX_RTP_EVENT,
        TL_CHECK_RESEND_PKT,
        TL_RX_RTCP_PSFB,
} TL_Rtcp_Event_Type;

typedef struct {
	uint32_t         ssrc;
	uint32_t         ntp_sec;
	uint32_t         ntp_frac;
	uint32_t         rtp_ts;
	uint32_t         sender_pcount;
	uint32_t         sender_bcount;
} TL_Rtcp_SR_t;

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
} TL_Rtcp_RR_t;

typedef struct {
        uint32_t	ts;
	uint32_t	tdelay;
	uint32_t	x_recv;
	uint32_t	p;
    uint32_t    rtt;
	uint32_t    ipi;
} TL_Rtcp_Tfrc_RX_t;


typedef struct {

    uint8_t  *frame_data;    
    uint32_t length;
    uint32_t timestamp;
    uint8_t  *ext_hdr_data;
    uint16_t ext_len;
    int      rtp_event_data;
    int      mark;
    uint8_t  to_px1;
}TL_Send_Para_t;

typedef struct {

    uint8_t  *frame_data;
    uint32_t length;
    uint32_t timestamp;
    uint8_t  partial_flag;
    uint8_t  nal_type;

}TL_Recv_Para_t;

typedef struct {

    uint32_t timestamp;
    uint32_t predict_length;
    uint32_t frame_buffer_size;
	uint32_t packet_buffer_size;
    uint8_t  *ext_hdr_data;
    uint16_t ext_len;
    uint8_t  nal_type;
}TL_Poll_Para_t;

typedef struct {
    int fmt;
}TL_PSFB_Para_t;

struct TL_Session_t;
struct TL_Rtcp_Event_t;


/*define some call back functions or common interface functions*/
typedef int (*tl_rtcp_callback)(struct TL_Session_t *session, struct TL_Rtcp_Event_t *e);
typedef int (*send_func)(struct TL_Session_t *session, TL_Send_Para_t *para);
typedef int (*recv_func)(struct TL_Session_t *session, TL_Recv_Para_t *para);
typedef int (*poll_func)(struct TL_Session_t *session, TL_Poll_Para_t *para);
typedef int (*send_generic_nack_func)(struct TL_Session_t *session, int pid, int bitmask);

typedef int (*send_pli_func)(struct TL_Session_t *session);
typedef int (*send_fir_func)(struct TL_Session_t *session);
typedef int (*send_sli_func)(struct TL_Session_t *session, uint16_t first, uint16_t number, uint8_t picture_id);

#define MAX_RTCP_APP_LEN 52

typedef struct TL_Session_t
{
    pdb                  		*participants;
                 
    struct rtp                  *rtp_session;       
     
    struct timeval              start_time;

    int                         tfrc_on;
	
	//Corey - EC flag
	int 						ec_on;
	//
	
    struct ec_agent             *ea;
     
    uint32_t                    opposite_ssrc;
    int                         has_opposite_ssrc;
 
    /* add to change opposite ssrc if need */ /* add by willy, 2010-10-11*/
    uint32_t                    change_opposite_ssrc;  /* if there are CHANGE_OPPOSITE_SSRC_THRESHOLD continues rtp which ssrc is not equal than current opposite_ssrc, change it */ /* add by willy, 2010-10-11 */



    /* Transmission and reception threads */
    pthread_t                   m_trans_thread_id;
    pthread_t                   m_recv_thread_id;
	
    /* Synchronization locks */
    pthread_mutex_t             m_network_device_lock;
    pthread_mutex_t             m_sending_buf_lock;
    pthread_mutex_t             m_receiving_buf_lock;
    sem_t                       trans_terminate_sem;
    sem_t                       recv_terminate_sem;

    unsigned int                maximum_payload_size;
    unsigned int                fps;
     
    SendingBuf_t                *sending_buffer;
     
    PayloadType_t               *payload_type;
     
    /* for rtcp app information */
    uint8_t                     app_buf[MAX_RTCP_APP_LEN];
    uint16_t                    app_len;
    uint32_t                    last_app_rtpts;
    
    /* function call */ 
    send_func                   send_data;
    recv_func                   recv_data;
    poll_func                   poll_data;
    send_generic_nack_func      send_generic_nack;

    send_pli_func               send_rfc4585_pli;
    send_sli_func               send_rfc4585_sli;
    send_fir_func               send_rfc5104_fir;

    /* others */ 
    tl_rtcp_callback            rtcp_callback;
    uint32_t                    dtmf_key_ts; //dtmf key first timestamp
    int                         dtmf_key;
    int                         get_dtmf_key;
    int                         dtmf_sent_cnt;

    /* send rtp when flag = 1, dont send rtp when flag = 0, initial value = 1, add by willy, 2010-09-21 */
    int                         send_rtp_flag;

    /* tos field in IP Header */
    int                         tos;

    /* inter packet interval (IPI) for video RTP packet */
    int                         video_ipi;
    int                         audio_ipi;

     int 				 on_stopping;

     unsigned long       mtl_id;
} TL_Session;


typedef struct TL_Rtcp_Event_t{
     struct TL_Session_t             *tl_session;   
     uint32_t                        ssrc;
     TL_Rtcp_Event_Type              type;
     void                            *data;
} TL_Rtcp_Event;

typedef struct {
    int ctrl;               // 0:disable, 1:enable (get it from provision file for enabling FEC or not)
    int send_payloadType;   // from opposite SDP. set '-1' if opposite doesn't support FEC
    int receive_payloadType;// from self SDP
} TL_FECControl_t;



/*for manage all the tl session*/
#define MAX_TL_SESSION_NUM      QS_MAX_SESSION_NUM //6
#define DTMF_DURATION           160
#define DTMF_PACKTS_PER_SEC     50
#define DTMF_EVENT_COUNTS       6
#define DTMF_EVENT_END_COUNTS   3
#define DTMF_FIXED_VOLUME       15

typedef struct
{
    unsigned char event;
#ifdef WORDS_BIGENDIAN
    unsigned char e:1;
    unsigned char r:1;
    unsigned char volume:6;
#else
    unsigned char volume:6;
    unsigned char r:1;
    unsigned char e:1;
#endif
    unsigned short duration;
} DTMF_Payload_t;

struct TL_Session_t *TransportLayer_InitwithSSRC(const char *addr, uint16_t rx_port, uint16_t tx_port, int tfrc_on, TL_FECControl_t fec, int tos,
                                                 PayloadType_t *payload_type,  tl_rtcp_callback callback, uint32_t ssrc,const char *local_addr);

struct TL_Session_t *TransportLayer_Init(const char *addr, uint16_t rx_port, uint16_t tx_port, int tfrc_on, TL_FECControl_t fec, int tos, 
                                         PayloadType_t *payload_type,  tl_rtcp_callback callback,const char *local_addr);

int TransportLayer_Start(struct TL_Session_t *session);

int TransportLayer_RTCPCallback(struct rtp *session,  rtp_event *e);
 
int TransportLayer_SetRTCPTimeIfno(struct TL_Session_t *session, uint32_t rtp_ts, 
                                   uint32_t ntp_sec, uint32_t ntp_frac);

int TransportLayer_SetTFRCMinSendingRate(struct TL_Session_t *session, uint32_t min_sending_rate);

int TransportLayer_GetSendingBufStatus(struct TL_Session_t *session, uint32_t *buf_frame_count, uint32_t *buf_drop_frame_count);

struct TL_Session_t  *TransportLayer_GetTLSession(struct rtp *session);

int TransportLayer_TerminateSession(struct TL_Session_t *session);

uint32_t TransportLayer_GetSessionSSRC(struct TL_Session_t *session);

int TransportLayer_Set_Video_InterPacketInterval(struct TL_Session_t *session, int ipi);

int TransportLayer_DisableRTCP(struct TL_Session_t *session);
int TransportLayer_EnableRTCP(struct TL_Session_t *session);
int TransportLayer_GetSeqNum(struct TL_Session_t *session, uint16_t *seq_num);
int TransportLayer_GetSessionInfo(struct rtp *session, int info_type, int* value);

#ifdef __cplusplus
}
#endif

 #endif
