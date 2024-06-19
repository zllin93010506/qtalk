/*******************************************************************************
* All Rights Reserved, Copyright @ Quanta Computer Inc. 2016
* File Name: stream_engine_api.h
* File Mark:
* Content Description:
* Other Description: None
* Version: 1.0
* Author: Frank Ting
* Date: 2016/11/02
*
* History:
*   1.2016/11/02 Frank Ting: draft
*******************************************************************************/
#ifndef __STREAM_ENGINE_WRAPPER_H_
#define __STREAM_ENGINE_WRAPPER_H_


#ifdef __cplusplus
extern "C" {
#endif
/* =============================================================================
                                Included headers
============================================================================= */
#include <stdint.h>
#include <sys/time.h>

/* =============================================================================
                                Define
============================================================================= */
#ifdef IN
#undef IN 
#endif

#ifdef OUT
#undef OUT
#endif

#ifdef INOUT
#undef INOUT
#endif

#define IN 
#define OUT
#define INOUT

#ifdef API
#undef API
#endif

#ifdef WIN32DLL
    #ifdef DLL_EXPORT
        #define API __declspec(dllexport)
    #else
        #define API __declspec(dllimport)
    #endif
#else
    #define API extern
#endif
//#define API                                 extern

#define SE_FAILURE                          -1
#define SE_SUCCESS                          0
#define NEW_QSTREAMENGINE					1

#define SRTP_KEY_LENGTH                     40

#define NOT_QUANTA_PRODUCT                  0
#define PX1_PRODUCT                         1
#define QT15A_PRODUCT                       2
#define QT15W_PRODUCT                       3
#define QT15I_PRODUCT                       4
#define PX2_PRODUCT                         5
#define PX2_3G_PRODUCT                      6

/*=============================================================================
                                   Macro
==============================================================================*/

#ifdef DECLARE_HANDLE
#undef DECLARE_HANDLE
#endif

#define DECLARE_HANDLE(_handleName) \
    typedef struct { unsigned long unused; } _handleName##__ ;\
    typedef const _handleName##__* _handleName


/* =============================================================================
                                Type
============================================================================= */
typedef  enum _StreamType{
    SE_AUDIO_STREAM=1,
    SE_VIDEO_STREAM
} StreamType;

typedef  enum _NeworkType{
    SE_3G=1,
    SE_WiFi
} NetworkType;
        
typedef enum _LipSyncMode{
    LIP_SYNC_AUDIO_ONLY=0,
    LIP_SYNC_VIDEO_ONLY=1,
    LIP_SYNC_AUDIO_VIDEO=2
} LipSyncMode;

typedef struct product_type
{
    int local;
    int remote;
} product_type_t;

typedef enum _SessionStatus{
    SESSION_ACTIVE = 0,
    SESSION_RECV_IDLE = 1,
} SessionStatus;

typedef struct _Stream_Para_t{

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

    /* time calibrator parameters */
    int            is_caller;
    char           extend_str[256];

} Stream_Para_t;


DECLARE_HANDLE(SessionMgrHandle);
DECLARE_HANDLE(SessionHandle);

typedef void (*LogFp)(unsigned char *msg);
typedef int (*DecodeFp)(SessionHandle        sessio_handle, 
                        const unsigned char* frame_data, 
                        const int            size_byte, 
                        const uint32_t       timestamp);

typedef int (*VideoNewIdrFrameFp)(SessionHandle sessio_handle);    
typedef int (*VideoFrameRateControlFp)(SessionHandle sessio_handle, int frame_fps);   
typedef int (*VideoBitRateControlFp)(SessionHandle sessio_handle, int bit_rate_per_sec);
typedef int (*VideoPlaceHolderNotifyFp)(SessionHandle sesion_handle, int flag);
typedef int (*SipSendIdrRequestFp)(SessionHandle sessio_handle);      
typedef int (*VideoQualityFeedbackFp)(SessionHandle session_handle, int remote_product_type, int quality);

typedef struct _MediaEngineCallback MediaEngineCallback;
struct _MediaEngineCallback {   
    VideoNewIdrFrameFp       video_new_idr_frame_cb;        /* generate a new IDR frame */
    VideoFrameRateControlFp  video_frame_rate_control_cb;   /* frame rate setting. Note:unenalbed */
    VideoBitRateControlFp    video_bit_rate_control_cb;     /* encoder bit rate setting  */
    VideoPlaceHolderNotifyFp video_placeholder_notify_cb;   /* when video streaminig quality gets worse, stream engine will invoke this callback function */
    SipSendIdrRequestFp      sip_send_idr_request_cb;       /* send an IDR request via SIP Info */
    VideoQualityFeedbackFp   video_quality_feedback_cb;     /* quality => 0: poor, 1: good */
};


/* =============================================================================
                        Public function declarations
============================================================================= */
//Stream manager API
//

/* To initiate  streaming session manager.
 * Must be invoked before creating a streaming session
 * */
API int session_mgr_construct(void);

/* To destroy streaming session manager.  
 * Before invoking this function, make sure that all the sessions have been destructed */
API int session_mgr_destruct(void);

//Presession,  
//
/**
 * create sockets in advance
 *
 * @param call_id id for the call session 
 * @param lport the socket listen port
 * @return 0 on success, -1 on failure
 **/
API int session_presession_create(int call_id, int lport);

/**
 * destroy all the sockets created in advance for the call session 
 *
 * @param call_id id for the call session
 * @return 0 on success, -1 on failure
 **/
API int session_presession_destroy(int call_id);


//Stream API
//
/**
 * To create a streaming session
 *
 * @param call_id id for the call session
 * @param stream_type SE_AUDIO_STREAM(1) -> audio streaming session,
 *                    SE_VIDEO_STREAM(2) -> video streaming session
 * @param pSession_handle if the session is created successfully, 
 *        the session handle will be stored in it  
 * @return 0 on success, -1 on failure
 **/
API int session_construct(IN  const int         call_id,
                          IN  const int         stream_type,
                          OUT SessionHandle*    pSessio_handle);

/**
 * To destruct a streaming session
 *
 * @param session_handle the handle of a streaming session
 * @return 0 on success, -1 on failure
 *
 **/

API int session_destruct(IN const SessionHandle sessio_handle);


/**
 * OPTIONAL
 * Notify stream engine what network you are using 
 * and session_update will use this info to set streaming parameters
 *
 * @param session_handle the handle of a streaming session
 * @param network 3G, WiFi
 * @return 0 on success, -1 on failure
 **/
API int session_set_network(IN const SessionHandle   sessio_handle,
                            IN const int             network);
    
    
    
    
/**
 * To update settings for a streaming session
 *
 * @param session_handle the handle of a streaming session
 * @param stream_type SE_AUDIO_STREAM(1) -> audio streaming session,
 *                    SE_VIDEO_STREAM(2) -> video streaming session
 * @param paras setting parameter values
 * @return 0 on success, -1 on failure
 **/
API int session_update(IN const SessionHandle   sessio_handle,
                       IN const int             stream_type,
                       IN const Stream_Para_t*  paras);
    

    
    
/**
 * This API function just updates the clock rate of codec
 * Note: session_update also includes updating the clock rate
 *
 *
 * @param session_handle the handle of a streaming session
 * @clockRate codec clock rate, ex:H264->90000, PCMU->8000 
 * @return 0 on success, -1 on failure
**/
API int session_update_codecClockRate(IN const SessionHandle session_handle,
                                      IN const int clockRate);

/**
 * To start streaming 
 *
 * @param session_handle the handle of a streaming session
 * @return 0 on success, -1 on failure
 **/
API int session_start(IN const SessionHandle    sessio_handle);

/**
 * To stop streaming 
 *
 * @param session_handle the handle of a streaming session
 * @return 0 on success, -1 on failure
 **/
API int session_stop(IN const SessionHandle     sessio_handle);

/**
 * hold call: stop sending RTPs. continue sending RTCPs
 *
 * @param session_handle the handle of a streaming session
 * @return 0 on success, -1 on failure
 **/ 
API int session_hold_call(IN const SessionHandle);
API int session_unhold_call(IN const SessionHandle);



/**
 * StreamEngine will send an IDR request to the opposite 
 *
 * @param session_handle the handle of a streaming session
 * @return 0 on success, -1 on failure
 **/
API int session_request_idr(IN const SessionHandle);



//Stream lipsync API, 
/**
 * turn on lipsync. Must invoke session_lipsync_unbind to release resource in the end of streaming
 *
 * @param audio_session_handle the handle of an audio streaming session that wants to do lip sync 
 * @param video_session_handle the handle of a video streaming session that wants to do lip sync
 * @return 0 on success, -1 on failure
 **/
API int session_lipsync_bind(IN const SessionHandle   audio_sessio_handle,
                             IN const SessionHandle   video_sessio_handle);

/**
 * turn off lipsync and release resource
 *
 * @param audio_session_handle the handle of an audio streaming session that has lip sync 
 * @param video_session_handle the handle of a video streaming session that has lip sync
 * @return 0 on success, -1 on failure
 **/
API int session_lipsync_unbind(IN const SessionHandle audio_sessio_handle,
                               IN const SessionHandle video_sessio_handle);

//Stream I/O API
/**
 * To send an encoded frame to stream engine 
 *
 * @param session_handle the handle of a streaming session
 * @param frame_data an encoded frame 
 * @param size_byte frame size in byte
 * @param timestamp as RTP timestamp
 * @param clock capture time of the frame 
 * @return 0 on success, -1 on failure
 **/
API int send_encode_frame(IN const SessionHandle  session_handle,
                          IN const unsigned char* frame_data,
                          IN const int            size_byte,
                          IN const uint32_t       timestamp,
                          IN struct timeval       clock);

/**
 * Register a callback function used for obtaining video or audio encoded frames 
 *
 *
 * @param session_handle the handle of a streaming session
 * @param decode_cb callback function that will be invoked 
 *        after a frame goes to the end of procedures of stream engine
 * @return 0 on success, -1 on failure
 **/
API int register_decode_frame_callback(IN const SessionHandle sessio_handle,
                                       IN const DecodeFp      decode_cb);

/**
 * Register callback functions used for getting events and notifications from stream engine  
 *
 *
 * @param session_handle the handle of a streaming session
 * @param callback a bunch of callback functions 
 * @return 0 on success, -1 on failure
 **/

API int register_media_engine_callback(IN const SessionHandle       sessio_handle,
                                       IN const MediaEngineCallback* callback);


API int register_log_callback(IN LogFp callback);

//sip & framwwork callback
#ifdef __cplusplus
}
#endif

#endif /*  __STREAM_ENGINE_WRAPPER_H_ */
