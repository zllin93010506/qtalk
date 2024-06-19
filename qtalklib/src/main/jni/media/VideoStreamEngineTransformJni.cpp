/*******************************************************************************
* All Rights Reserved, Copyright @ Quanta Computer Inc. 2013
* File Name: VideoStreamEngineTransformJni.cpp
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
/* =============================================================================
                                Included headers
============================================================================= */
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include "logging.h"

#include "include/H264Parser.h"
//#include "include/IpcCommon.h"
#include "include/VideoStreamEngineTransformJni.h"
#include "../StreamEngineService/StreamEngine/stream_engine_api.h"

/* =============================================================================
                                    Macros
============================================================================= */
#define TAG "QSE_VideoStreamEngineTransformJni"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

#define JNI(str) Java_com_quanta_qtalk_media_video_VideoStreamEngineTransform__1##str

#define METUX_INIT() \
    pthread_mutex_init(&g_lock, NULL);

#define METUX_LOCK() \
    pthread_mutex_lock(&g_lock);

#define METUX_UNLOCK() \
    pthread_mutex_unlock(&g_lock);


/* =============================================================================
                                    Types
============================================================================= */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NativeParams NativeParams;
struct _NativeParams {
    JavaVM*             jvm;
    jclass              jclass_caller;
    jobject             jvideo_sink;
    jobject             jsource_cb;
    jmethodID           on_media_native;
    jmethodID           on_resolution_change_native;
    jmethodID           set_bit_rate_native;
    jmethodID           set_frame_rate_native;
    jmethodID           on_request_idr_native;
    jmethodID           set_low_bandwidth_native;

    int                 format_index;
    int                 decode_width;
    int                 decode_height;
    int                 codec_clock_rate_hz;
    int                 first_received_frame;
    SessionHandle       session_handle;
};


/* =============================================================================
                                Private data
   ============================================================================= */
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static NativeParams *g_vHandle = NULL;

/* =============================================================================
                                Implement Start
   ============================================================================= */

static void free_native_params(JNIEnv* env, const char* remote_ip, jstring dest_ip)
{
    //LOGD("free_native_params()");

    if (g_vHandle->jclass_caller != 0) {
        env->DeleteGlobalRef(g_vHandle->jclass_caller);
        g_vHandle->jclass_caller = 0;
    }
    if (g_vHandle->jvideo_sink != 0) {
        env->DeleteGlobalRef(g_vHandle->jvideo_sink);
        g_vHandle->jvideo_sink = 0;
    }
    if (g_vHandle->jsource_cb != 0) {
        env->DeleteGlobalRef(g_vHandle->jsource_cb);
        g_vHandle->jsource_cb = 0;
    }
    free(g_vHandle);
    g_vHandle = NULL;

    if (remote_ip != NULL) {
        env->ReleaseStringUTFChars(dest_ip, remote_ip);
    }    

    return ;
}


int video_frame_callback(SessionHandle session_handle, const unsigned char* frame_data, const int size_byte, const uint32_t timestamp)
{
    JNIEnv*        env = NULL;
    jobject        byte_buffer = NULL;
    jobject        global_byte_buffer = NULL;
    int            decode_width = 0, decode_height = 0;
    bool           isAttached = false;

    //METUX_LOCK();
    if(g_vHandle == NULL) {
        //METUX_UNLOCK();
        return -1;
    }

    if (g_vHandle->jvm) {
        //g_vHandle->jvm->AttachCurrentThread((JNIEnv**)&env, NULL);
        //if (NULL == env) {
        //    //METUX_UNLOCK();
        //    return -1;
        //}
        if(g_vHandle->jvm->GetEnv((void**)&env, JNI_VERSION_1_4) < 0) {
            if(g_vHandle->jvm->AttachCurrentThread((JNIEnv**)&env, NULL)) {
                //METUX_UNLOCK();
                return -1;
            }
            isAttached = true;
        }
    }

    if (h264_get_resolution(frame_data, size_byte, &decode_height, &decode_width) == true) {
		if ((g_vHandle->decode_width != decode_width) ||
			(g_vHandle->decode_height != decode_height)) {
            LOGD("video resolution is change, %d x %d", decode_width, decode_height);
			g_vHandle->decode_width = decode_width;
			g_vHandle->decode_height = decode_height;
			env->CallStaticVoidMethod(g_vHandle->jclass_caller,
                                      g_vHandle->on_resolution_change_native,
                                      g_vHandle->jvideo_sink,
                                      g_vHandle->format_index,
                                      g_vHandle->decode_width,
                                      g_vHandle->decode_height);
		}
	}


    if(g_vHandle->first_received_frame == 0) {        
#if 0
        if (g_vHandle->jclass_caller && g_vHandle->on_request_idr_native && g_vHandle->jsource_cb) {
	        env->CallStaticVoidMethod(g_vHandle->jclass_caller,
							          g_vHandle->on_request_idr_native,
							          g_vHandle->jsource_cb);
        }
#endif
        g_vHandle->first_received_frame = 1;
    }

	byte_buffer = env->NewDirectByteBuffer((void*)frame_data, size_byte);
	if (byte_buffer) {
		global_byte_buffer = env->NewGlobalRef(byte_buffer);
	}

	if (global_byte_buffer) {
		jboolean retV = env->CallStaticBooleanMethod(g_vHandle->jclass_caller,
									                 g_vHandle->on_media_native,
									                 g_vHandle->jvideo_sink,
									                 global_byte_buffer,
									                 size_byte,
									                 timestamp,
									                 0);
        if(retV == 0) {
            //LOGD("video sink return: %d", retV);
            session_request_idr(g_vHandle->session_handle);
        }

        env->DeleteGlobalRef(global_byte_buffer);
        global_byte_buffer = NULL;
	}
    else {
        LOGE("global_byte_buffer is NULL");
    }

    if (byte_buffer) {
        env->DeleteLocalRef(byte_buffer);
        byte_buffer = NULL;
    }

    //if (g_vHandle->jvm) {
    //    g_vHandle->jvm->DetachCurrentThread();
    //}
    if(isAttached) {
        g_vHandle->jvm->DetachCurrentThread();
    }

    //METUX_UNLOCK();

    return 0;
}

int ServiceVideoNewIdrFrame(SessionHandle session_handle)
{
    JNIEnv *env = NULL;
    bool isAttached = false;

    //LOGD("ServiceVideoNewIdrFrame()");

    //METUX_LOCK();

    if(g_vHandle == NULL) {
        //METUX_UNLOCK();
        return -1;
    }
    
    if (g_vHandle->jvm) {
        if(g_vHandle->jvm->GetEnv((void**)&env, JNI_VERSION_1_4) < 0) {
            if(g_vHandle->jvm->AttachCurrentThread((JNIEnv**)&env, NULL)) {
                //METUX_UNLOCK();
                return -1;
            }
            isAttached = true;
        }

        //g_vHandle->jvm->AttachCurrentThread((JNIEnv**)&env, NULL);
        //if (NULL == env) {
            //METUX_UNLOCK();
        //    return -1;
        //}
    }
    if (g_vHandle->jclass_caller && g_vHandle->on_request_idr_native && g_vHandle->jsource_cb) {
	    env->CallStaticVoidMethod(g_vHandle->jclass_caller,
							      g_vHandle->on_request_idr_native,
							      g_vHandle->jsource_cb);
    }


    //if (g_vHandle->jvm) {
    //    g_vHandle->jvm->DetachCurrentThread();
    //}
    if(isAttached) {
        g_vHandle->jvm->DetachCurrentThread();
    }

    //METUX_UNLOCK();

    return 0;
}

int ServiceVideoFrameRateControl(SessionHandle session_handle, int frame_fps)
{
    JNIEnv *env = NULL;
    bool isAttached = false;


    //LOGD("ServiceVideoFrameRateControl(), %d fps", frame_fps);

    //METUX_LOCK();

    if(g_vHandle == NULL) {
        //METUX_UNLOCK();
        return -1;
    }
    if (g_vHandle->jvm) {
        //g_vHandle->jvm->AttachCurrentThread((JNIEnv**)&env, NULL);
        //if (NULL == env) {
        //    //METUX_UNLOCK();
        //    return -1;
        //}
        if(g_vHandle->jvm->GetEnv((void**)&env, JNI_VERSION_1_4) < 0) {
            if(g_vHandle->jvm->AttachCurrentThread((JNIEnv**)&env, NULL)) {
                //METUX_UNLOCK();
                return -1;
            }
            isAttached = true;
        }
    }
    if (g_vHandle->jclass_caller && g_vHandle->set_frame_rate_native && g_vHandle->jsource_cb) {
		env->CallStaticVoidMethod(g_vHandle->jclass_caller,
								  g_vHandle->set_frame_rate_native,
								  g_vHandle->jsource_cb,
								  frame_fps);
    }

    //if (g_vHandle->jvm) {
    //    g_vHandle->jvm->DetachCurrentThread();
    //}
    if(isAttached) {
        g_vHandle->jvm->DetachCurrentThread();
    }

    //METUX_UNLOCK();

    return 0;
}

int ServiceVideoBitRateControl(SessionHandle session_handle, int bit_rate_per_sec)
{
    JNIEnv *env = NULL;
    bool isAttached = false;

    //LOGD("ServiceVideoBitRateControl(), %d bps", bit_rate_per_sec);

    //METUX_LOCK();

    if(g_vHandle == NULL) {
        //METUX_UNLOCK();
        return -1;
    }

    if (g_vHandle->jvm) {
        //g_vHandle->jvm->AttachCurrentThread((JNIEnv**)&env, NULL);
        //if (NULL == env) {
        //    //METUX_UNLOCK();
        //    return -1;
        //}
        if(g_vHandle->jvm->GetEnv((void**)&env, JNI_VERSION_1_4) < 0) {
            if(g_vHandle->jvm->AttachCurrentThread((JNIEnv**)&env, NULL)) {
                //METUX_UNLOCK();
                return -1;
            }
            isAttached = true;
        }
    }
    if (g_vHandle->jclass_caller && g_vHandle->set_bit_rate_native && g_vHandle->jsource_cb) {
		env->CallStaticVoidMethod(g_vHandle->jclass_caller,
								  g_vHandle->set_bit_rate_native,
								  g_vHandle->jsource_cb,
								  bit_rate_per_sec/1000); //bps to Kbps
    }
    //if (g_vHandle->jvm) {
    //    g_vHandle->jvm->DetachCurrentThread();
    //}
    if(isAttached) {
        g_vHandle->jvm->DetachCurrentThread();
    }

    //METUX_UNLOCK();

    return 0;
}

int ServiceVideoPlaceHolderNotify(SessionHandle session_handle, int flag)
{
    JNIEnv *env = NULL;
    bool isAttached = false;

    //LOGD("ServiceVideoPlaceHolderNotify(), flag:%d", flag);

    //METUX_LOCK();

    if(g_vHandle == NULL) {
        //METUX_UNLOCK();
        return -1;
    }

    if (g_vHandle->jvm) {
        //g_vHandle->jvm->AttachCurrentThread((JNIEnv**)&env, NULL);
        //if (NULL == env) {
        //    //METUX_UNLOCK();
        //    return -1;
        //}
        if(g_vHandle->jvm->GetEnv((void**)&env, JNI_VERSION_1_4) < 0) {
            if(g_vHandle->jvm->AttachCurrentThread((JNIEnv**)&env, NULL)) {
                //METUX_UNLOCK();
                return -1;
            }
            isAttached = true;
        }
    }
    if (g_vHandle->jclass_caller && g_vHandle->set_low_bandwidth_native && g_vHandle->jsource_cb) {
		env->CallStaticVoidMethod(g_vHandle->jclass_caller,
								  g_vHandle->set_low_bandwidth_native,
								  (jboolean)flag);
    }
    //if (g_vHandle->jvm) {
    //    g_vHandle->jvm->DetachCurrentThread();
    //}
    if(isAttached) {
        g_vHandle->jvm->DetachCurrentThread();
    }
    
    //METUX_UNLOCK();

    return 0;
}

int ServiceSipSendIdrRequest(SessionHandle session_handle)
{
    JNIEnv *env = NULL;
    bool isAttached = false;

    //LOGD("ServiceSipSendIdrRequest()");

    //METUX_LOCK();

    if(g_vHandle == NULL) {
        //METUX_UNLOCK();
        return -1;
    }

    if (g_vHandle->jvm) {
        //g_vHandle->jvm->AttachCurrentThread((JNIEnv**)&env, NULL);
        //if (NULL == env) {
        //    //METUX_UNLOCK();
        //    return -1;
        //}
        if(g_vHandle->jvm->GetEnv((void**)&env, JNI_VERSION_1_4) < 0) {
            if(g_vHandle->jvm->AttachCurrentThread((JNIEnv**)&env, NULL)) {
                //METUX_UNLOCK();
                return -1;
            }
            isAttached = true;
        }
    }

    /* TODO */

    //if (g_vHandle->jvm) {
    //    g_vHandle->jvm->DetachCurrentThread();
    //}
    if(isAttached) {
        g_vHandle->jvm->DetachCurrentThread();
    }

    //METUX_UNLOCK();

    return 0;
}

JNIEXPORT jint JNICALL JNI(Start)(JNIEnv* env, jclass jclass_caller,
                                  jlong ssrc, jobject jvideo_sink,
                                  jint listenPort, jboolean enableTFRC,
                                  jstring dest_ip, jint destPort,
                                  jint formatIndex, jint sampleRate,
                                  jint payloadID, jstring mimeType,
                                  jint initFps, jint initialBitrate,
                                  jboolean enablePLI, jboolean enableFIR,
                                  jboolean letGopEqFps, jint fecCtrl,
                                  jint fecSendPType, jint fecRecvPType,
                                  jboolean enableVideoSRTP, jstring videoSRTPSendKey,
                                  jstring videoSRTPReceiveKey,
                                  jint outMaxBandwidth, jint videoMTU,
                                  jint videoRTPTOS, jint networkType,
                                  jint productType)
{
    const char*   remote_ip  = env->GetStringUTFChars(dest_ip, NULL);
    const char*   videoSRTP_SendKey = env->GetStringUTFChars(videoSRTPSendKey, NULL);
    const char*   videoSRTP_ReceiveKey = env->GetStringUTFChars(videoSRTPReceiveKey, NULL);

    JavaVM*       pJVM = NULL;
    int           ret = 0;

    //LOGD("Start(pt:%d,ip:%s,port:%d)", payloadID, remote_ip, destPort);

    METUX_LOCK();

    if(g_vHandle != NULL) {
        LOGE("<error> The video seesion was started");
        METUX_UNLOCK();
        return -1;
    }

    g_vHandle = (NativeParams *)malloc(sizeof(NativeParams));
    if (NULL == g_vHandle) {
        LOGE("<error> Unable to malloc sizeof(NativeParams)");
        METUX_UNLOCK();
        return 0;
    }
    memset(g_vHandle, 0x00, sizeof(NativeParams));

    g_vHandle->codec_clock_rate_hz = sampleRate;
    g_vHandle->format_index = formatIndex;
    g_vHandle->decode_width = 0;
    g_vHandle->decode_height = 0;
    g_vHandle->first_received_frame = 0;

    // Obtain a global ref to the Java VM
    env->GetJavaVM(&pJVM);
    if (NULL == pJVM) {
        LOGE("<error> Unable to GetJavaVM");
        free_native_params(env, remote_ip, dest_ip);
        METUX_UNLOCK();
        return 0;
    }
    g_vHandle->jvm = pJVM;

    if (jvideo_sink != NULL) {
        g_vHandle->jvideo_sink = env->NewGlobalRef(jvideo_sink);
        if (0 == g_vHandle->jvideo_sink) {
            LOGE("<error> Unable to get jvideo_sink");
            free_native_params(env, remote_ip, dest_ip);
            METUX_UNLOCK();
            return 0;
        }
    }

    g_vHandle->jclass_caller = (jclass)env->NewGlobalRef(jclass_caller);
    if (0 == g_vHandle->jclass_caller) {
        LOGE("<error> Unable to get jclass_caller");
        free_native_params(env, remote_ip, dest_ip);
        METUX_UNLOCK();
        return 0;
    }

    #define METHOD_NAME "onMediaNative"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/media/video/IVideoSink;Ljava/nio/ByteBuffer;III)Z"
    g_vHandle->on_media_native = env->GetStaticMethodID(g_vHandle->jclass_caller,
                                                       METHOD_NAME,
                                                       METHOD_SIG);
    if (g_vHandle->on_media_native == 0) {
        LOGE("<error> Unable to GetMethodID %s %s", METHOD_NAME, METHOD_SIG);
        free_native_params(env, remote_ip, dest_ip);
        METUX_UNLOCK();
        return 0;
    }

    #undef METHOD_NAME
    #undef METHOD_SIG
    #define METHOD_NAME "onResolutionChangeNative"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/media/video/IVideoSink;III)V"
    g_vHandle->on_resolution_change_native = env->GetStaticMethodID(g_vHandle->jclass_caller,
                                                                   METHOD_NAME,
                                                                   METHOD_SIG);
    if (g_vHandle->on_resolution_change_native == 0) {
        LOGE("<error> Unable to GetMethodID %s %s", METHOD_NAME, METHOD_SIG);
        free_native_params(env, remote_ip, dest_ip);
        METUX_UNLOCK();
        return 0;
    }

    #undef METHOD_NAME
    #undef METHOD_SIG
    #define METHOD_NAME "setFrameRateNative"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/media/video/IVideoStreamSourceCB;B)V"
    g_vHandle->set_frame_rate_native = env->GetStaticMethodID(g_vHandle->jclass_caller,
                                                             METHOD_NAME,
                                                             METHOD_SIG);
    if (g_vHandle->set_frame_rate_native == 0) {
        LOGE("<error> Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
        free_native_params(env, remote_ip, dest_ip);
        METUX_UNLOCK();
        return 0;
    }

    #undef METHOD_NAME
    #undef METHOD_SIG
    #define METHOD_NAME "setBitRateNative"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/media/video/IVideoStreamSourceCB;I)V"
    g_vHandle->set_bit_rate_native = env->GetStaticMethodID(g_vHandle->jclass_caller,
                                                           METHOD_NAME,
                                                           METHOD_SIG);
    if (g_vHandle->set_bit_rate_native == 0) {
        LOGD("<error> Unable to GetMethodID %s %s", METHOD_NAME, METHOD_SIG);
        free_native_params(env, remote_ip, dest_ip);
        METUX_UNLOCK();
        return 0;
    }

    #undef METHOD_NAME
    #undef METHOD_SIG
    #define METHOD_NAME "onRequestIDRNative"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/media/video/IVideoStreamSourceCB;)V"
    g_vHandle->on_request_idr_native = env->GetStaticMethodID(g_vHandle->jclass_caller,
                                                             METHOD_NAME,
                                                             METHOD_SIG);
    if (g_vHandle->on_request_idr_native == 0) {
        LOGD("<error> Unable to GetMethodID %s %s", METHOD_NAME, METHOD_SIG);
        free_native_params(env, remote_ip, dest_ip);
        METUX_UNLOCK();
        return 0;
    }

    #undef METHOD_NAME
    #undef METHOD_SIG
    #define METHOD_NAME "setLowBandwidthNative"
    #define METHOD_SIG  "(Z)V"
    g_vHandle->set_low_bandwidth_native = env->GetStaticMethodID(g_vHandle->jclass_caller,
                                                                METHOD_NAME,
                                                                METHOD_SIG);
    if (g_vHandle->set_low_bandwidth_native == 0) {
        LOGE("<error> Unable to GetMethodID %s %s", METHOD_NAME, METHOD_SIG);
        free_native_params(env, remote_ip, dest_ip);
        METUX_UNLOCK();
        return 0;
    }

#if 0
    //para.session_handle = handle->session_handle;
    strncpy((char*)para.paras.ip_v4, remote_ip, 20);
    para.paras.rtp_tx_port = destPort;
    para.paras.rtp_rx_port = listenPort;
    para.paras.tos = videoRTPTOS;
    para.paras.mtu = videoMTU;

    para.paras.payload_type = payloadID;
    para.paras.codec_clock_rate_hz = sampleRate;
    para.paras.codec_fps = initFps;

    para.paras.fec_support = fecCtrl;
    para.paras.fec_tx_payload_type = fecSendPType;
    para.paras.fec_rx_payload_type = fecRecvPType;

    para.paras.rfc4588_support = 0;
    para.paras.rfc4588_tx_payload_type = 0;
    para.paras.rfc4588_rx_payload_type = 0;
    para.paras.rfc4588_rtx_time_msec = 0;

    para.paras.rfc4585_rtcp_fb_pli_support = enablePLI;
    para.paras.rfc5104_rtcp_fb_fir_support = enableFIR;

    para.paras.product_type.local = QT1A;
    para.paras.product_type.remote = productType;
//    para.paras.auto_bandwidth_support = 0;
    para.paras.auto_bandwidth_support = enableTFRC;
    para.paras.private_header_support = 0;

    para.paras.max_outgoing_kbps = outMaxBandwidth;
    para.paras.min_outgoing_kbps = 300;
    para.paras.initial_outgoing_kbps = outMaxBandwidth*0.6;
    
    /* SRTP parameters */
    para.paras.srtp_support = enableVideoSRTP;
    strcpy(para.paras.srtp_tx_key, videoSRTP_SendKey);
    strcpy(para.paras.srtp_rx_key, videoSRTP_ReceiveKey);
#else
    int stype = SE_VIDEO_STREAM;
    Stream_Para_t para;
    session_construct(1, stype, &(g_vHandle->session_handle));

    /* Socket parameters */
    strncpy((char*)para.ip_v4, remote_ip, 20);
    para.rtp_tx_port   = destPort;
    para.rtp_rx_port   = listenPort;
    para.tos           = videoRTPTOS;
    para.mtu           = videoMTU;

    /* Codec parameters */
    para.payload_type = payloadID;
    para.codec_clock_rate_hz = g_vHandle->codec_clock_rate_hz;
    para.codec_fps = initFps;

    /* FEC parameters */
    para.fec_support = fecCtrl;
    para.fec_tx_payload_type = fecSendPType;
    para.fec_rx_payload_type = fecRecvPType;

    /* RFC4588 parameters */
    para.rfc4588_support = 0;
    para.rfc4588_tx_payload_type = 0;
    para.rfc4588_rx_payload_type = 0;
    para.rfc4588_rtx_time_msec = 0;

    /* Instantaneous Decoding Refresh frame request */
    para.rfc4585_rtcp_fb_pli_support = enablePLI;
    para.rfc5104_rtcp_fb_fir_support = enableFIR;

    /* private parameters */
    para.product_type.local = 5;//PX2_PRODUCT
    para.product_type.remote = productType;//PX2_PRODUCT
    para.auto_bandwidth_support = enableTFRC;
    para.private_header_support = 0;
    
    para.max_outgoing_kbps = outMaxBandwidth;
    para.min_outgoing_kbps = 300;
    para.initial_outgoing_kbps = outMaxBandwidth*0.6;;

    /* SRTP parameters */
    para.srtp_support = enableVideoSRTP;
    //strncpy(para.srtp_tx_key, videoSRTP_SendKey, SRTP_KEY_LENGTH+1);
    //strncpy(para.srtp_rx_key, videoSRTP_ReceiveKey, SRTP_KEY_LENGTH+1);
    strcpy(para.srtp_tx_key, videoSRTP_SendKey);
    strcpy(para.srtp_rx_key, videoSRTP_ReceiveKey);

    session_update(g_vHandle->session_handle, stype, &para);

    MediaEngineCallback cb;
    cb.video_new_idr_frame_cb = ServiceVideoNewIdrFrame;
    cb.video_frame_rate_control_cb = ServiceVideoFrameRateControl;
    cb.video_bit_rate_control_cb = ServiceVideoBitRateControl;
    cb.sip_send_idr_request_cb = ServiceSipSendIdrRequest;
    cb.video_placeholder_notify_cb = ServiceVideoPlaceHolderNotify;
    register_media_engine_callback(g_vHandle->session_handle, &cb);
    register_decode_frame_callback(g_vHandle->session_handle, video_frame_callback);    

    session_lipsync_bind(NULL, NULL);
    session_start(g_vHandle->session_handle);
#endif

    if (remote_ip != NULL) {
        env->ReleaseStringUTFChars(dest_ip, remote_ip);
    }

    METUX_UNLOCK();

    return (jint)g_vHandle;
}


JNIEXPORT void JNICALL JNI(Stop)(JNIEnv* env, jclass jclass_caller, jint params)
{
    NativeParams *handle = (NativeParams *)params;
    int           ret = 0;

    LOGD("Stop()");

    METUX_LOCK();

    if((g_vHandle == NULL) || (handle != g_vHandle)) {
        LOGE("<error> invailed native handle, Stop()");
        METUX_UNLOCK();
        return;
    }

    session_stop(g_vHandle->session_handle);
    session_lipsync_unbind(NULL, g_vHandle->session_handle);
    usleep(500000);
    session_destruct(g_vHandle->session_handle);

    free_native_params(env, NULL, NULL);

    METUX_UNLOCK();
    return ;
}


JNIEXPORT void JNICALL JNI(OnSend)(JNIEnv* env, jclass jclass_caller,
                                   jint params, jbyteArray jobject_input,
                                   jint length, jlong timestamp,
                                   jshort rotation)
{
    NativeParams*  handle = (NativeParams*)params;
    unsigned char* frame_data = NULL;
    int            ret = 0;
    uint32_t       ts;
    struct timeval clock;

    METUX_LOCK();

    if (handle && jobject_input) {
        frame_data = (unsigned char*)env->GetByteArrayElements(jobject_input, 0);

        gettimeofday(&clock, NULL);
        ts = (uint32_t)(clock.tv_sec * handle->codec_clock_rate_hz) + 
             (uint32_t)((double)clock.tv_usec * (double)handle->codec_clock_rate_hz*(double)1.0e-6);
        send_encode_frame(handle->session_handle, frame_data, length, ts, clock);
        //send_encode_frame(handle, timestamp, length, frame_data, rotation);

        env->ReleaseByteArrayElements(jobject_input,(jbyte*)frame_data, JNI_ABORT);
    } else {
        LOGE("OnSend(): NULL handle");
    }

    METUX_UNLOCK();

    return ;
}


JNIEXPORT void JNICALL JNI(OnSendDirect)(JNIEnv* env, jclass jclass_caller,
                                         jint params, jobject jobject_input,
                                         jint length, jlong timestamp,
                                         jshort rotation)
{
    NativeParams*   handle = (NativeParams*)params;
    jobject         buffer_input = NULL;
    unsigned char*  frame_data = NULL;
    int             ret = 0;
    uint32_t        ts;
    struct timeval  clock;

    METUX_LOCK();
    if (handle && jobject_input) {
        buffer_input = env->NewLocalRef(jobject_input);
        frame_data = (unsigned char*)env->GetDirectBufferAddress(buffer_input);

        gettimeofday(&clock, NULL);
        ts = (uint32_t)(clock.tv_sec * handle->codec_clock_rate_hz) + 
             (uint32_t)((double)clock.tv_usec * (double)handle->codec_clock_rate_hz*(double)1.0e-6); 
        send_encode_frame(handle->session_handle, frame_data, length, ts, clock);
        //send_encode_frame(handle, timestamp, length, frame_data, rotation);

        env->DeleteLocalRef(buffer_input);
    } else {
        LOGE("OnSendDirect(): NULL handle");
    }
    METUX_UNLOCK();

    return ;
}


JNIEXPORT void JNICALL JNI(SetHold)(JNIEnv* env, jclass jclass_caller,
                                    jint params, jboolean enable)
{
    NativeParams* handle = (NativeParams*)params;

    LOGD("SetHold(%d)", enable);

    METUX_LOCK();
    if (handle) {
        if (1 == enable) session_hold_call(handle->session_handle);
        else session_unhold_call(handle->session_handle);
    } else {
        LOGE("%s %d: NULL handle", __FUNCTION__, __LINE__);
    }
    METUX_UNLOCK();

    return ;
}


JNIEXPORT void JNI(SetDemandIDRCB)(JNIEnv * env, jclass jclass_caller,
                                   jint params,  jobject jIDemandIDRCB)
{
    LOGE("SetDemandIDRCB(): Not implement now");
    return ;
}


JNIEXPORT void JNICALL JNI(SetSourceCB)(JNIEnv* env, jclass jclass_caller,
                                        jint params,  jobject jISourceCB)
{
    NativeParams* handle = (NativeParams*)params;

    //LOGD("SetSourceCB()");

    METUX_LOCK();

    if (handle) {
        if (handle->jsource_cb != 0) {
            env->DeleteGlobalRef(handle->jsource_cb);
            handle->jsource_cb = 0;
        }

        if (jISourceCB != NULL) {
            handle->jsource_cb = env->NewGlobalRef(jISourceCB);
        }
    } else {
        LOGE("%s %d: NULL handle", __FUNCTION__, __LINE__);
    }

    METUX_UNLOCK();

    return ;
}


JNIEXPORT void JNICALL JNI(UpdateSource)(JNIEnv* env, jclass jclass_caller,
                                         jint params,  jint width, jint height)
{
    LOGE("UpdateSource(): Not implement now");
    return ;
}


JNIEXPORT void JNICALL JNI(SetReportCB)(JNIEnv* env, jclass jclass_caller,
                                        jint params,  jobject jIReportCB)
{
    /* Not implemet now */
    LOGE("SetReportCB(): Not implement now");
    return ;
}


JNIEXPORT jlong JNICALL JNI(GetSSRC)(JNIEnv* env, jclass jclass_caller, jint params)
{
    /* Not implemet now */
    LOGE("GetSSRC(): Not implement now");
    return 0;
}


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv * env = NULL;
     if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) == JNI_OK) {
        logback_init(vm, env);
    }
    LOGD("JNI_OnLoad()");

    METUX_INIT();

    session_mgr_construct();

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved)
{
    JNIEnv * env = NULL;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) == JNI_OK) {
        logback_uninit(env);
    }
    LOGD("JNI_OnUnload()");
    return ;
}


#ifdef __cplusplus
}
#endif
