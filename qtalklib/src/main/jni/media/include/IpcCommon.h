/*******************************************************************************
* All Rights Reserved, Copyright @ Quanta Computer Inc. 2013
* File Name: IpcCommon.h
* File Mark:
* Content Description:
* Other Description: None
* Version: 1.0
* Author: Paul Weng
* Date: 2013/03/05
*
* History:
*   1.2013/03/05 Paul Weng: draft
*******************************************************************************/
#ifndef __IPC_COMMON_H_
#define __IPC_COMMON_H_


#ifdef __cplusplus
extern "C" {
#endif
/* =============================================================================
                                    Constants
============================================================================= */
//#define MAX_FRAME_SIZE                  (16*4096-28)
#define MAX_FRAME_SIZE                  65000
#define OPUS_RTP_SAMPLERATE             48000

#define AUDIO_APP_TO_SRV_CTL_PORT       51000
#define AUDIO_SRV_TO_APP_CTL_ACK_PORT   51001
#define AUDIO_APP_TO_SRV_DATA_PORT      51002
#define AUDIO_SRV_TO_APP_DATA_PORT      51003

#define VIDEO_APP_TO_SRV_CTL_PORT       52000
#define VIDEO_SRV_TO_APP_CTL_ACK_PORT   52001
#define VIDEO_SRV_TO_APP_CTL_PORT       52002
#define VIDEO_APP_TO_SRV_CTL_ACK_PORT   52003
#define VIDEO_APP_TO_SRV_DATA_PORT      52004
#define VIDEO_SRV_TO_APP_DATA_PORT      52005


/* =============================================================================
                                    Types
============================================================================= */
typedef enum _NetworkType {
    TYPE_MOBILE = 0,
    TYPE_WIFI = 1,
    TYPE_MOBILE_MMS = 2,
    TYPE_MOBILE_SUPL = 3,
    TYPE_MOBILE_DUN = 4,
    TYPE_MOBILE_HIPRI = 5,
    TYPE_WIMAX = 6,
    TYPE_BLUETOOTH = 7,
    TYPE_DUMMY = 8,
    TYPE_ETHERNET = 9
} NetworkType;

typedef enum _QuantaProductType {
    NONE_QUANTA = 0,
    ET1,
    QT1A,
    QT1W,
    QT1I,
    QUANTA_PX2,
    QUANTA_FG6,
    QUANTA_OTHERS = 100,
} QuantaProductType;

typedef enum _StreamType {
    SE_AUDIO_STREAM = 1,
    SE_VIDEO_STREAM
} StreamType;

typedef struct product_type {
    int local;
    int remote;
} product_type_t;

/* SRTP */
#define SRTP_KEY_LENGTH                     40

typedef struct _Stream_Para {
    /* Socket parameters */
    unsigned char  ip_v4[20];               /* format: xxx.xxx.xxx.xxx */
    unsigned int   rtp_tx_port;             /* range: 10000~65535 */
    unsigned int   rtp_rx_port;             /* range: 10000~65535 */
    int            tos;
    int            mtu;

    /* Codec parameters */
    unsigned int   payload_type;            /* range: 0~127 */
    int            codec_clock_rate_hz;
    int            codec_fps;

    /* FEC parameters */
    int            fec_support;             /* 0: disable, 1: enable */
    unsigned int   fec_tx_payload_type;     /* range: 96~127 */
    unsigned int   fec_rx_payload_type;     /* range: 96~127 */

    /* RFC4588 parameters */
    int            rfc4588_support;         /* 0: disable, 1: enable */
    unsigned int   rfc4588_tx_payload_type; /* range: 96~127 */
    unsigned int   rfc4588_rx_payload_type; /* range: 96~127 */
    unsigned int   rfc4588_rtx_time_msec;   /* Corey TODO: add the opposite rtxTime*/

    /* Instantaneous Decoding Refresh frame request */
    int            rfc4585_rtcp_fb_pli_support; /* Picture Loss Indication, 0: disable, 1: enable */
    int            rfc5104_rtcp_fb_fir_support; /* Full Intra Request, 0: disable, 1: enable */

    /* private parameters */
    product_type_t product_type;
    int            auto_bandwidth_support;      /* 0: disable, 1: enable */
    int            private_header_support;      /* 0: disable, 1: enable */

    /* bit rate control parameters, only used by video stream */
    int            max_outgoing_kbps;
    int            min_outgoing_kbps;
    int            initial_outgoing_kbps;

    /* SRTP parameters */
    int            srtp_support;            /* 0:disable, 1:enable */
    char           srtp_tx_key[SRTP_KEY_LENGTH+1];
    char           srtp_rx_key[SRTP_KEY_LENGTH+1];

} Stream_Para;

typedef enum _SeCommnad {
    SE_CTL_SESSION_CONSTRUCT = 0,
    SE_CTL_SESSION_DESTRUCT = 1,
    SE_CTL_SESSION_UPDATE = 2,
    SE_CTL_SESSION_START = 3,
    SE_CTL_SESSION_STOP = 4,
    SE_CTL_SESSION_HOLD_CALL = 5,
    SE_CTL_SESSION_UNHOLD_CALL = 6,
    SE_CTL_SESSION_LIPSYNC_BIND = 7,
    SE_CTL_SESSION_LIPSYNC_UNBIND =8,
    SE_CTL_SESSION_SET_NETWORK_TYPE = 9,
    SE_CTL_SESSION_REQUEST_REMOTE_IDR = 10,

    SE_CTL_SESSION_CONSTRUCT_ACK = 100,
    SE_CTL_SESSION_DESTRUCT_ACK = 101,
    SE_CTL_SESSION_UPDATE_ACK = 102,
    SE_CTL_SESSION_START_ACK = 103,
    SE_CTL_SESSION_STOP_ACK = 104,
    SE_CTL_SESSION_HOLD_CALL_ACK = 105,
    SE_CTL_SESSION_UNHOLD_CALL_ACK = 106,
    SE_CTL_SESSION_LIPSYNC_BIND_ACK = 107,
    SE_CTL_SESSION_LIPSYNC_UNBIND_ACK = 108,
    SE_CTL_SESSION_SET_NETWORK_TYPE_ACK = 109,
    SE_CTL_SESSION_REQUEST_REMOTE_IDR_ACK = 110,

    SE_CTL_VIDEO_NEW_IDR_FRAME_CB = 1000,
    SE_CTL_VIDEO_FRAME_RATE_CONTROL_CB = 1001,
    SE_CTL_VIDEO_BIT_RATE_CONTROL_CB = 1002,
    SE_CTL_VIDEO_PLACEHOLDER_NOTIFY_CB = 1003,
    SE_CTL_SIP_SEND_IDR_REQUEST_CB = 1004,

    SE_CTL_VIDEO_NEW_IDR_FRAME_CB_ACK = 10000,
    SE_CTL_VIDEO_FRAME_RATE_CONTROL_CB_ACK = 10001,
    SE_CTL_VIDEO_BIT_RATE_CONTROL_CB_ACK = 10002,
    SE_CTL_VIDEO_PLACEHOLDER_NOTIFY_CB_ACK = 10003,
    SE_CTL_SIP_SEND_IDR_REQUEST_CB_ACK = 10004,

    SE_SEND_AUDIO_ENCODE_FRAME = 100000,
    SE_SEND_VIDEO_ENCODE_FRAME = 100001,

    SE_DUMMY = 0xFFFFFFFF
} SeCommnad;

typedef struct _SeHandle {
    int                  call_id;
    int                  stream_type;
    int                  network_type;
} SeHandle;

typedef struct _SeParameter {
    SeHandle             session_handle;
    Stream_Para          paras;
} SeParameter;

typedef struct  _SeCtlUnit {
    SeCommnad            command;
    unsigned char        data[1020];
} SeCtlUnit;

typedef struct  _SeCtlAckUnit {
    SeCommnad            command;
    int                  ret;
} SeCtlAckUnit;

typedef struct _SeEncodeFrame {
    SeCommnad            command;
    SeHandle             session_handle;
    unsigned long        timestamp;
    struct timeval       clock;
    unsigned int         size_byte;
    unsigned int         udp_index;
    unsigned int         frame_index;
    int                   next_frame;
    unsigned char        frame_data[MAX_FRAME_SIZE];
} SeEncodeFrame;

#ifdef __cplusplus
}
#endif

#endif /*  __IPC_COMMON_H_ */
