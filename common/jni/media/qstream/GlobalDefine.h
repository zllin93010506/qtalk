// GlobalDefine.h : Global Define
// -----------------------------------------------------------------------
// Quanta Computer, QRI
// =======================================================================
// 2009/11/20   Frank.Ting Create
// 2010/01/22	Vicky Liao Added Cross Compile Definition -- ET1_QSTREAM_CROSS_COMPILE
// 2010/10/25   Vicky Liao Added 4 counter -- 
//              unsigned short audio_speaker_playout_dect_frame_cnt;
//              unsigned short audio_speaker_playout_hdmi_frame_cnt;
//              unsigned short audio_slience_dect_cnt;
//              unsigned short audio_slience_hdmi_cnt;
// =======================================================================

#ifndef _GLOBAL_DEFINE_H_
#define _GLOBAL_DEFINE_H_


#ifdef __cplusplus
extern "C" {
#endif

#define QS_TRUE                             (0)
#define QS_FAIL                             (-1)

#define QS_ENABLE                           (1)
#define QS_DISABLE                          (0)
    
#define QS_PROCESSING                       (2)
#define QS_NULL_STATE                       0xff
 


/* ---------------------------------------
   Function Control (0:disable, 1:enable)
   ---------------------------------------      
 */         
#define LINUX_DEBUGMSG                      1
#define LINUX_DEBUG_LEVEL_MSG               1
#define LINUX_LIP_SYNC                      1
#define LINUX_GSTREAMER_ENABLE              0
#define LINUX_PL330_DECODER_ENABLE          0
#define LINUX_RFC4585_ENABLE                0

//#define QS_SR1_PLATFORM


/* ---------------------------------------
   debug daemon define
   ---------------------------------------      
 */  
#define QDBG_MSG_QUEUE_ID                   19000 /* control debug message by message queue */
#define QDBG_SOCKET_PORT                    16133

#define QDBG_MAX_CMD                        5
typedef struct {
    long int msg_type;                      /* A field of type long int that specifies the type of the message and must be greater than zero */
    int cmd_number;                         /* 0:off, 1:on */
    char cmd_buf[QDBG_MAX_CMD][50];
} qdbg_cmd_msg_t;

#define QDBG_MAX_VIDEO_ALIVE_THREAD         6
#define QDBG_MAX_AUDIO_ALIVE_THREAD         6

//typedef struct {
//
//    /* video session counter */
//    int video_session_cnt;
//    int audio_session_cnt;
//    int dump_audio_pcm;
//} qdbg_stream_engine_info_t;
//qdbg_stream_engine_info_t qdbg_info;
//
//
///* counter will self clean after read */
//typedef struct {
//    
//    /* timestamp for rate calculate */
//    unsigned long start_time_msec;
//    unsigned long end_time_msec;
//
//    /* video generic counter */    
//    unsigned short video_thread_alive_cnt[QDBG_MAX_VIDEO_ALIVE_THREAD];
//    unsigned short video_capture_frame_cnt;
//    unsigned short video_encodein_frame_cnt;
//    unsigned short video_encodeout_frame_cnt;
//    unsigned short video_receive_frame_cnt;
//    unsigned short video_decodeout_frame_cnt;
//    unsigned short video_playout_frame_cnt;
//    unsigned long video_rtp_pkt_cnt;
//
//    /* video exception counter */
//    unsigned short video_partial_frame_cnt;
//    unsigned short video_loss_event_cnt;
//    unsigned short video_long_rtt_event_cnt;
//
//    /* audio generic counter */
//    unsigned short audio_thread_alive_cnt[QDBG_MAX_AUDIO_ALIVE_THREAD];
//    unsigned short audio_capture_frame_cnt;
//    unsigned short audio_encodein_frame_cnt;
//    unsigned short audio_sending_frame_cnt;
//    unsigned short audio_receive_frame_cnt;
//    unsigned short audio_playout_frame_cnt;    
//    
//    unsigned short audio_mixer_frame_cnt;
//    unsigned short audio_mic_capture_frame_cnt;
//    unsigned short audio_speaker_playout_frame_cnt; 
//    unsigned short audio_speaker_playout_dect_frame_cnt;
//    unsigned short audio_speaker_playout_hdmi_frame_cnt;    
//    unsigned short audio_slience_dect_cnt;
//    unsigned short audio_slience_hdmi_cnt;
//    unsigned short audio_alsa_write_to_dect_cnt;
//    unsigned short audio_alsa_write_to_hdmi_cnt;
//    unsigned long audio_rtp_pkt_cnt;
//    
//    /* audio exeption counter */
//    unsigned short audio_dect_buffer_underun_cnt;
//    unsigned short audio_hdmi_buffer_underun_cnt;        
//    unsigned short audio_buffer_drop_cnt;
//    unsigned short audio_lipsync_drop_cnt;
//
//    /* lip sync statistics */
//    unsigned short video_lip_sync_drop_cnt;
//
//} qdbg_counter_t;
//qdbg_counter_t qdbg_cnt;
//
//
///* counter always keep in this structure */
//typedef struct {
//
//    /* video frame counter */
//    unsigned short video_frame_cnt_in_receiving_buf;
//    unsigned short video_frame_cnt_in_decoder;
//    unsigned short video_frame_cnt_in_playout_buf;
//
//    // audio, common variable
//    unsigned short audio_last_rtp_sequence_number;   
//
//    // video, common variable
//    unsigned short video_last_rtp_sequence_number;
//    unsigned long  video_last_rtp_time;
//
//    // call latency
//    unsigned long timestamp_create_session;
//    unsigned long timestamp_start_session;
//    unsigned long timestamp_first_playout;
//
//    // video, camera
//    unsigned long before_start_camera_timestamp_msec;
//    unsigned long after_start_camera_timestamp_msec;
//
//} qdbg_no_clean_counter_t;
//qdbg_no_clean_counter_t qdbg_noclean_cnt;


///* counter for traffic monitor                              */
///* the report will update to file and UI will show it on TV */
//typedef struct {
//
//    /* timestamp for rate calculate */
//    unsigned long start_time_msec;
//    unsigned long end_time_msec;
//    
//    // ------------------------------------------------------------------    
//
//    // video, receiver after network layer (real received pkt)    
//    unsigned long video_after_network_pkt_cnt;
//    unsigned long video_after_network_len_cnt; 
//    unsigned long video_after_network_pkt_lost_cnt;
//    
//    // video, receiver after transport layer (packet loss drop)
//    unsigned long video_after_transport_frame_cnt;
//    unsigned long video_after_transport_len_cnt;
//
//    // video, receiver after session layer (error concealment drop)
//    unsigned long video_after_session_frame_cnt;
//    unsigned long video_after_session_len_cnt;
//
//    // video, receiver after decode output (decoder drop)
//    unsigned long video_after_decodeout_frame_cnt;
//    
//    // video, receiver after application layer (lipsync drop)
//    unsigned long video_after_application_frame_cnt;
//    
//    // ------------------------------------------------------------------
//
//    // video, sender before network layer
//    unsigned long video_before_network_pkt_cnt;
//    unsigned long video_before_network_len_cnt;
//    unsigned long video_before_network_frame_cnt;
//
//    // video, sender before transport layer
//    unsigned long video_before_transport_frame_cnt;
//    unsigned long video_before_transport_len_cnt;
//
//    // ------------------------------------------------------------------
//
//    // audio, debug info
//    unsigned long audio_sw_aec_process_time;
//    unsigned long audio_sw_aec_process_cnt;
//
//    unsigned long audio_sw_aec_max_process_time;
//    unsigned long audio_sw_aec_min_process_time;
//    unsigned long audio_buf_drop_cnt;
//    unsigned long audio_lipsync_drop_cnt;
//
//    // ------------------------------------------------------------------    
//
//    // audio, after network layer (real received pkt)    
//    unsigned long audio_after_network_pkt_cnt;
//    unsigned long audio_after_network_len_cnt; 
//    unsigned long audio_after_network_pkt_lost_cnt;     
//
//    unsigned long video_get_decode_buf_cnt;
//
//} qdbg_monitor_counter_t;
//qdbg_monitor_counter_t qdbg_monitor_cnt;


/* -------------------------------------------------------------
   Share memroy global define
   -------------------------------------------------------------      
 */
#define SW_DECODER_TIMESTAMP_FIFO_SIZE      100
#define SW_DECODER_HEALTH_CHECK_CNT         12

#define SW_DECODER_ID                       (key_t)19910     // for share memory
#define SW_DECODER_SHM_FLAG                 (0666|IPC_CREAT)
#define SW_DECODER_SHM_ADDR                 0x0//0x55000000

typedef struct sw_decoder_timestamp_fifo {
    unsigned long timestamp[SW_DECODER_TIMESTAMP_FIFO_SIZE];
    int timestamp_input_idx;
    int timestamp_output_idx;
} timestamp_fifo_t;

/* -------------------------------------------------------------
   Share memroy command set / status 
   -------------------------------------------------------------      
 */
#define SHM_CMD_NULL                        0x0
#define SHM_CMD_RESET_SW_DECODER            0x01


#define SHM_STATUS_READY_TO_WRITE           0x0
#define SHM_STATUS_READY_TO_READ            0x1

/* -------------------------------------------------------------
   Share memroy control for h264 software decoder h264 input
   -------------------------------------------------------------      
 */
#define SW_DECODER_H264_SHM_ENTRY_NUM       5               // frame buffer number
#define SW_DECODER_H264_SHM_SLICE_NUM       60              // slice number in a frame
#define SW_DECODER_H264_SHM_BUFFER_SIZE     (256*1024)      // buffer size

#pragma pack(push)                                          /* push current alignment to stack */
#pragma pack(1)                                             /* set alignment to 4 byte boundary */  
typedef struct {
    volatile int offset;                                             /* share memory addressing is by virtual memroy, 
                                                               so we save offset, not address in the slice arrary */
    volatile int len;
} slice_arrary_t;

typedef struct {
    volatile int len;                                                /* if len equal "0" indicate this is empty entry and can be written    */
    volatile unsigned char buf[SW_DECODER_H264_SHM_BUFFER_SIZE]; 
    volatile unsigned long timestamp;

    volatile int status;

    volatile int slice_num_in_frame;
    volatile int slice_read_index;
    slice_arrary_t slice_ptr[SW_DECODER_H264_SHM_SLICE_NUM];
} frame_buf_entry_t;

typedef struct {
    volatile int command;
} shm_cmd_t;

typedef struct {
    shm_cmd_t cmd; 

    volatile int read_index;
    volatile int write_index;      
    frame_buf_entry_t sw_h264[SW_DECODER_H264_SHM_ENTRY_NUM];
 
} shm_table_t;
#pragma pack(pop)                                           /* restore original alignment from stack */
#define SW_DECODER_H264_SHM_MAX_SIZE        ((size_t)(sizeof(shm_table_t)))


/* ------------------------------------------------------------
   Share memroy control for h264 software decoder yuv output
   ------------------------------------------------------------
 */
#define SW_DECODER_YUV_SHM_ENTRY_NUM        12              // frame buffer number
#define SW_DECODER_YUV_SHM_BUFFER_SIZE      (614400)        // max buffer size 1280x720, (Ravesion, 848x480 yuv420)

#pragma pack(push)                          /* push current alignment to stack */
#pragma pack(1)                             /* set alignment to 4 byte boundary */  
typedef struct {
    volatile int len;                                /* if len equal "0" indicate this is empty entry and can be written    */
    volatile unsigned char buf[SW_DECODER_YUV_SHM_BUFFER_SIZE];     
    volatile unsigned long timestamp;

    volatile int playing;
    volatile int status;
} framebuf_yuv_entry_t;

typedef struct {
    volatile int read_index;
    volatile int write_index;
    framebuf_yuv_entry_t sw_yuv[SW_DECODER_YUV_SHM_ENTRY_NUM];
} shm_yuv_table_t;
#pragma pack(pop)                           /* restore original alignment from stack */
#define SW_DECODER_SHM_YUV_MAX_SIZE         ((size_t)(sizeof(shm_yuv_table_t)))


/* -----------------------------------------
   Debug Control on the x86 Linux platform
   (In the ARM, all configs must be set to 0)
   -----------------------------------------
 */   
#ifndef WIN32
#define X86_LINUX_PLATFORM                  1
#define X86_LINUX_BYPASS_NETWORK            0

#if defined(ET1_QSTREAM_CROSS_COMPILE) || defined(QS_PX2_PLATFORM)		
#define LINUX_STATISTICS                    1
#define X86_LINUX_SDL_RENDER                0
#else
#define LINUX_STATISTICS                    1
#define X86_LINUX_SDL_RENDER                1
#endif

#endif

        
/* -----------------------------
   Debug message define
   -----------------------------      
 */    
#define MAX_DEBUG_MSG_LEVEL                 5
#define DEBUG_MSG_DISABLE                   0
#define DEBUG_MSG_LEVEL1                    1
#define DEBUG_MSG_LEVEL2                    2
#define DEBUG_MSG_LEVEL3                    3
#define DEBUG_MSG_LEVEL4                    4
#define DEBUG_MSG_LEVEL5                    5

#ifdef WIN32
#define QS_DMSG(input,check,fmt,...)
#else

#if LINUX_DEBUG_LEVEL_MSG
#define QS_DMSG(input,check,fmt,arg...)     if((input)) { \
                                                if((input)==(check)) { \
                                                    printf(fmt,##arg); \
                                                }else if((input)==MAX_DEBUG_MSG_LEVEL) { \
                                                    printf(fmt,##arg); \
                                                } \
                                            }

#else
#define QS_DMSG(input,check,fmt,arg...)
#endif

#endif


/* -----------------------------
   Analyse Video Latency
   -----------------------------      
 */ 
/* Vicky Liao added for getting latency during a video call */
#define QS_DMSG_LATENCY                             0
#define QS_DMSG_LATENCY_PARALLEL                    1

#define QS_LATENCY_VIDEO_INIT_CALL_BEGIN            1
#define QS_LATENCY_VIDEO_INIT_START                 2
#define QS_LATENCY_VIDEO_SET_CAMERA_BEGIN           3
#define QS_LATENCY_VIDEO_SET_CAMERA_END             4
#define QS_LATENCY_VIDEO_SET_RENDERING_BEGIN        5
#define QS_LATENCY_VIDEO_SET_RENDERING_END          6
#define QS_LATENCY_VIDEO_START_RENDERING_BEGIN      7
#define QS_LATENCY_VIDEO_START_RENDERING_END        8
#define QS_LATENCY_VIDEO_START_CAMERA_BEGIN         9
#define QS_LATENCY_VIDEO_START_CAMERA_END           10
#define QS_LATENCY_VIDEO_SET_DECODER_BEGIN          11
#define QS_LATENCY_VIDEO_SET_DECODER_END            12
#define QS_LATENCY_VIDEO_SET_ENCODER_BEGIN          13
#define QS_LATENCY_VIDEO_SET_ENCODER_END            14
#define QS_LATENCY_VIDEO_START_NET_BEGIN            15
#define QS_LATENCY_VIDEO_START_NET_END              16
#define QS_LATENCY_VIDEO_START_DECODER_BEGIN        17
#define QS_LATENCY_VIDEO_START_DECODER_END          18
#define QS_LATENCY_VIDEO_START_ENCODER_BEGIN        19
#define QS_LATENCY_VIDEO_START_ENCODER_END          20
#define QS_LATENCY_VIDEO_START_CALL_CAPTURE         21
#define QS_LATENCY_VIDEO_CAPTURE_ENCODE             22  
#define QS_LATENCY_VIDEO_ENCODE_FRAME_SEND          23
#define QS_LATENCY_VIDEO_FRAME_PACKET_SEND          24
#define QS_LATENCY_VIDEO_PACKET_RECEIVE_FRAME       25
#define QS_LATENCY_VIDEO_FRAME_RECEIVE_DECODE       26
#define QS_LATENCY_VIDEO_DECODE_PLAYOUT             27
#define QS_LATENCY_VIDEO_PLAYOUT                    28

//typedef struct {
//    int g_qstream_initCall_flag;
//    int g_qstream_startCall_flag;
//    int g_qstream_videoSetCamera_flag;
//    int g_qstream_videoSetRendering_flag;
//    int g_qstream_videoSetDecoder_flag;
//    int g_qstream_videoSetEncoder_flag;
//    int g_qstream_videoCaptureEncode_flag;
//    int g_qstream_videoEncodeFrame_flag;
//    int g_qstream_videoFrameDecode_flag;
//    int g_qstream_videoDecodePlayout_flag;
//    int g_qstream_videoPlayout_flag;
//    int g_qstream_videoPktSend_flag;
//    int g_qstream_videoStartRendering_flag;
//    int g_qstream_videoStartCamera_flag;
//    int g_qstream_videoStartNet_flag;
//    int g_qstream_videoStartDecoder_flag;
//    int g_qstream_videoStartEncoder_flag;
//    int g_qstream_videoPktReceive_flag;
//} qdbg_video_latency_t;
//qdbg_video_latency_t qdbg_video_latency;    
/* Vicky Liao added for getting latency during a video call */


/* -------------------------------------
   Frame Rate / Bit Rate Control define
   -------------------------------------      
 */
#define DEFAULT_AUTO_FRAME_RATE_CONTROL     QS_ENABLE
#define DEFAULT_AUTO_KBIT_RATE_CONTROL      QS_ENABLE
    
#define QS_CONSTANT_KBIT_RATE               1000
#define QS_MAX_KBIT_RATE                    2500
#define QS_MIN_KBIT_RATE                    400    
#define QS_NORMAL_KBIT_RATE                 2000

#define QS_FRC_LEVEL1_KBIT                  1500    /* bit rate > 1500kbps          , 30fps */
#define QS_FRC_LEVEL2_KBIT                  700     /* 1500kbps > bit rate > 700kbps, 15fps */
#define QS_FRC_LEVEL3_KBIT                  400     /* 700 kbps > bit rate > 400kbps, 10fps */
                                                    /* bit rate < 400kbps           , 05fps */                                                    
#define QS_FRC_CHECK_COUNT                  60      /* frame count */
#define QS_FRC_TOLERANCE_KBIT               100     

#define QS_ALLOW_DECODE_PARTIAL_CASE        QS_DISABLE



/* -------------------------------------
   IDR period define
   -------------------------------------      
 */
 #define IDR_REQUEST_PERIOD		            30000		//msec
 #define TIME_DIFF_SINCE_LAST_IDR_REQUEST   5000        //msec
 #define TIME_DIFF_SINCE_LOST_DETECTED      1500        //msec


/* -----------------------------
   Feature Define
   -----------------------------      
 */   
#define QS_RECORD_PATH                      "/tmp/"     // NULL or "/home/frank/", ...
#define QS_SNAPSHOT_PATH                    "/tmp/"     // NULL or "/tmp", ...

#define QS_DTMF_NULL_MODE                   0    
#define QS_DTMF_IN_BAND_MODE                1
#define QS_DTMF_OUT_BAND_MODE               2
#define QS_DTMF_NULL_KEY                    (-1)
#define QS_DTMF_END_SENDING                 (-2)

#define QS_VIDEO_QUALITY_AUTO               0 // auto frame rate control
#define QS_VIDEO_QUALITY_SHARPNESS          1 // manual frame rate control

#define QS_PHOTO_STICKER_WIDTH              160
#define QS_PHOTO_STICKER_HEIGHT             90


#define QS_AUDIO_SRC_DECT                   0
#define QS_AUDIO_SRC_USBMIC                 1


/* -----------------------------------
   command that report to remote side
   -----------------------------------  
 */   
#define CMD_INDEX_FIELD                     0
#define CMD_ID_FIELD                        1
#define CMD_NEW_GOP_EVENT                   ((unsigned short)(0x1))
#define CMD_RX_BYTE_RATE                    ((unsigned short)(0x2))
#define CMD_VIDEO_QUALITY_SETTING           ((unsigned short)(0x3))

    
    
/* -----------------------------
   Parameters define
   -----------------------------      
 */   
//#define QS_MAX_SESSION_NUM                  8

/* media type define */        
#define QS_MEDIATYPE_AUDIO                  1
#define QS_MEDIATYPE_VIDEO                  2   


/* h264 type define */
#define I_SLICE_TYPE                        2
#define P_SLICE_TYPE                        0

/* h264 NALU define */
#define NAL_TYPE_MASK                       0x1F
#define NAL_TYPE_UNDEFINE                   0
#define NAL_TYPE_NON_IDR                    1
#define NAL_TYPE_SLICE_A                    2
#define NAL_TYPE_SLICE_B                    3
#define NAL_TYPE_SLICE_C                    4
#define NAL_TYPE_IDR                        5
#define NAL_TYPE_SEI                        6
#define NAL_TYPE_SEQ_PARA_SET               7
#define NAL_TYPE_PIC_PARA_SET               8
#define NAL_TYPE_ACCESS_UNIT_DELIMITER      9

#define QS_FREE_BUF_DIRECTLY                1
#define QS_FREE_BUF_INDIRECTLY              0                    

/* bit rate control */
#define FAST_CHANGE_MODE                    0
#define SLOW_CHANGE_MODE                    1

/* video decoder solution */
#define V_DECODER_PL330_HW_DECODER          0
#define V_DECODER_ASPEN_IPP_SW_DECODER      1
#define V_DECODER_IPP_VMETA_HW_DECODER      2


/* -----------------------------
   Type/Structure define
   -----------------------------      
 */            
#pragma pack(push)      /* push current alignment to stack */
#pragma pack(1)         /* set alignment to 1 byte boundary */    
typedef struct {
    union {
        unsigned char u_byte[4];
        unsigned short u_short[2];
        unsigned long u_long;
    } u_type;
} union_type_t;
#pragma pack(pop)       /* restore original alignment from stack */


        
#ifdef __cplusplus
}
#endif

#endif // _GLOBAL_DEFINE_H_
