/*******************************************************************************
* All Rights Reserved, Copyright @ Quanta Computer Inc. 2013
* File Name: AudioStreamEngineTransformJni.cpp
* File Mark:
* Content Description:
* Other Description: None
* Version: 1.0
* Author: Paul Weng
* Date: 2013/03/04
*
* History:
*   1.2013/03/04 Paul Weng: draft
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

//#include "include/IpcCommon.h"
#include "include/AudioStreamEngineTransformJni.h"
#include "../StreamEngineService/StreamEngine/stream_engine_api.h"

/* =============================================================================
                                    Macros
============================================================================= */
#define TAG "QSE_AudioStreamEngineTransformJni"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

#define JNI(str) Java_com_quanta_qtalk_media_audio_AudioStreamEngineTransform__1##str

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

typedef struct _NativeParams {
    JavaVM*             jvm;
    jclass              jclass_caller;
    jobject             jaudio_sink;
    jmethodID           on_media_native;

    int                 codec_clock_rate_hz;
    SessionHandle       session_handle;
} NativeParams;


/* =============================================================================
                                Private data
   ============================================================================= */
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static NativeParams *g_aHandle = NULL;

/* =============================================================================
                                Implement Start
   ============================================================================= */
static void free_native_params(JNIEnv* env, const char* remote_ip, jstring dest_ip)
{
    //LOGD("free_native_params()");

    if (g_aHandle->jclass_caller != 0) {
        env->DeleteGlobalRef(g_aHandle->jclass_caller);
        g_aHandle->jclass_caller = 0;
    }

    if (g_aHandle->jaudio_sink != 0) {
        env->DeleteGlobalRef(g_aHandle->jaudio_sink);
        g_aHandle->jaudio_sink = 0;
    }
    free(g_aHandle);
    g_aHandle = NULL;
    if (remote_ip != NULL) {
        env->ReleaseStringUTFChars(dest_ip, remote_ip);
    }
    return;
}

int audio_frame_callback(SessionHandle session_handle, const unsigned char* frame_data, const int size_byte, const uint32_t timestamp)
{
    JNIEnv*        env = NULL;
    jobject        byte_buffer = NULL;
    jobject        global_byte_buffer = NULL;
    bool           isAttached = false;

    //METUX_LOCK();

    if(g_aHandle == NULL) {
        //METUX_UNLOCK();
        return -1;
    }

    if (g_aHandle->jvm) {
        //g_aHandle->jvm->AttachCurrentThread((JNIEnv**)&env, NULL);
        //if (NULL == env) {
        //    //METUX_UNLOCK();
        //    return -1;
        //}
        if(g_aHandle->jvm->GetEnv((void**)&env, JNI_VERSION_1_4) < 0) {
            if(g_aHandle->jvm->AttachCurrentThread((JNIEnv**)&env, NULL)) {
                //METUX_UNLOCK();
                return -1;
            }
            isAttached = true;
        }
    }

    byte_buffer = env->NewDirectByteBuffer((void*)frame_data,
												  size_byte);
    if (byte_buffer) {
        global_byte_buffer = env->NewGlobalRef(byte_buffer);
    }

    if (global_byte_buffer) {
		env->CallStaticVoidMethod(g_aHandle->jclass_caller,
								  g_aHandle->on_media_native,
								  g_aHandle->jaudio_sink,
								  global_byte_buffer,
								  size_byte,
								  timestamp);

        env->DeleteGlobalRef(global_byte_buffer);
        global_byte_buffer = NULL;
    }

    if (byte_buffer) {
        env->DeleteLocalRef(byte_buffer);
        byte_buffer = NULL;
    }

    //if (g_aHandle->jvm) {
    //    g_aHandle->jvm->DetachCurrentThread();
    //}
    if(isAttached) {
        g_aHandle->jvm->DetachCurrentThread();
    }

    //METUX_UNLOCK();

    return 0;
}

JNIEXPORT jint JNICALL JNI(Start)(JNIEnv* env, jclass jclass_caller,
                                  jlong ssrc, jobject jaudio_sink,
                                  jint listenPort, jstring dest_ip,
                                  jint destPort, jint sampleRate,
                                  jint payloadID, jstring mimeType,
                                  jint channels, jint fecCtrl,
                                  jint fecSendPType, jint fecRecvPType,
                                  jboolean enableAudioSRTP, jstring audioSRTPSendKey,
                                  jstring audioSRTPReceiveKey,
                                  jint audioRTPTOS, jboolean hasVideo,
                                  jint networkType, jint productType)
{
    //NativeParams* handle = (NativeParams*)malloc(sizeof(NativeParams));
    const char*   remote_ip  = env->GetStringUTFChars(dest_ip, NULL);
    const char*   audioSRTP_SendKey = env->GetStringUTFChars(audioSRTPSendKey, NULL);
    const char*   audioSRTP_ReceiveKey = env->GetStringUTFChars(audioSRTPReceiveKey, NULL);
    JavaVM*       pJVM = NULL;
    int           ret = 0;


    //LOGD("Start(pt:%d,ip:%s,port:%d)", payloadID, remote_ip, destPort);

    METUX_LOCK();  

    if(g_aHandle != NULL) {
        LOGE("<error> The audio seesion was started");
        METUX_UNLOCK();
        return -1;
    }

    g_aHandle = (NativeParams *)malloc(sizeof(NativeParams));
    if (NULL == g_aHandle) {
        LOGE("<error> Unable to malloc sizeof(NativeParams)");
        METUX_UNLOCK();
        return 0;
    }
    memset(g_aHandle, 0x00, sizeof(NativeParams));

    if(payloadID == 111) g_aHandle->codec_clock_rate_hz = 48000;
    else g_aHandle->codec_clock_rate_hz = sampleRate;


    // Obtain a global ref to the Java VM
    env->GetJavaVM(&pJVM);
    if (NULL == pJVM) {
        LOGE("<error> Unable to GetJavaVM");
        free_native_params(env, remote_ip, dest_ip);
        METUX_UNLOCK();
        return 0;
    }
    g_aHandle->jvm = pJVM;

    g_aHandle->jclass_caller = (jclass)env->NewGlobalRef(jclass_caller);
    if (0 == g_aHandle->jclass_caller) {
        LOGE("<error> Unable to get jclass_caller");
        free_native_params(env, remote_ip, dest_ip);
        METUX_UNLOCK();
        return 0;
    }

    if (jaudio_sink != NULL) {
        g_aHandle->jaudio_sink = env->NewGlobalRef(jaudio_sink);
        if (0 == g_aHandle->jaudio_sink) {
            LOGE("<error> Unable to get jaudio_sink");
            free_native_params(env, remote_ip, dest_ip);
            METUX_UNLOCK();
            return 0;
        }
    }

    #define METHOD_NAME "onMediaNative"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/media/audio/IAudioSink;Ljava/nio/ByteBuffer;IJ)V"
    g_aHandle->on_media_native = env->GetStaticMethodID(g_aHandle->jclass_caller,
                                                       METHOD_NAME,
                                                       METHOD_SIG);
    if (0 == g_aHandle->on_media_native) {
        LOGE("<error> Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
        free_native_params(env, remote_ip, dest_ip);
        METUX_UNLOCK();
        return 0;
    }

#if 0
    para.session_handle = handle->session_handle;
    strncpy((char*)para.paras.ip_v4, remote_ip, 20);
    para.paras.rtp_tx_port = destPort;
    para.paras.rtp_rx_port = listenPort;
    para.paras.tos = audioRTPTOS;
    para.paras.mtu = 1380;
    para.paras.codec_fps = 50;
    para.paras.payload_type = payloadID;
    para.paras.fec_support = fecCtrl;
    para.paras.fec_tx_payload_type = fecSendPType;
    para.paras.fec_rx_payload_type = fecRecvPType;

    para.paras.rfc4588_support = 0;
    para.paras.rfc4588_tx_payload_type = 0;
    para.paras.rfc4588_rx_payload_type = 0;
    para.paras.rfc4588_rtx_time_msec = 0;

    para.paras.rfc4585_rtcp_fb_pli_support = 0;
    para.paras.rfc5104_rtcp_fb_fir_support = 0;

    para.paras.product_type.local = QT1A;
    para.paras.product_type.remote = productType;
    para.paras.auto_bandwidth_support = 0;
    para.paras.private_header_support = 0;

    para.paras.max_outgoing_kbps = 0;
    para.paras.min_outgoing_kbps = 0;
    para.paras.initial_outgoing_kbps = 0;

    /* SRTP parameters */
    para.paras.srtp_support = enableAudioSRTP;
    strcpy(para.paras.srtp_tx_key, audioSRTP_SendKey);
    strcpy(para.paras.srtp_rx_key, audioSRTP_ReceiveKey);
#else
    int stype = SE_AUDIO_STREAM;
    Stream_Para_t para;
    session_construct(1, stype, &(g_aHandle->session_handle));

    /* Socket parameters */
    strncpy((char*)para.ip_v4, remote_ip, 20);
    para.rtp_tx_port   = destPort;
    para.rtp_rx_port   = listenPort;
    para.tos           = audioRTPTOS;
    para.mtu           = 1380;    

    /* Codec parameters */
    para.payload_type = payloadID;
    para.codec_clock_rate_hz = g_aHandle->codec_clock_rate_hz;
    para.codec_fps = 50;

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
    para.rfc4585_rtcp_fb_pli_support = 0;
    para.rfc5104_rtcp_fb_fir_support = 0;

    /* private parameters */
    para.product_type.local = 5;//PX2_PRODUCT
    para.product_type.remote = productType;//PX2_PRODUCT
    para.auto_bandwidth_support = 0;
    para.private_header_support = 0;
    
    para.max_outgoing_kbps = 0;
    para.min_outgoing_kbps = 0;
    para.initial_outgoing_kbps = 0;

    /* SRTP parameters */
    para.srtp_support = enableAudioSRTP;
    //strncpy(para.srtp_tx_key, audioSRTP_SendKey, SRTP_KEY_LENGTH+1);
    //strncpy(para.srtp_rx_key, audioSRTP_ReceiveKey, SRTP_KEY_LENGTH+1);
    strcpy(para.srtp_tx_key, audioSRTP_SendKey);
    strcpy(para.srtp_rx_key, audioSRTP_ReceiveKey);

    session_update(g_aHandle->session_handle, stype, &para);
    register_decode_frame_callback(g_aHandle->session_handle, audio_frame_callback);

    session_lipsync_bind(NULL, NULL);
    session_start(g_aHandle->session_handle);
#endif

    if (remote_ip != NULL) {
        env->ReleaseStringUTFChars(dest_ip, remote_ip);
    }

    METUX_UNLOCK();

    return (jint)g_aHandle;
}


JNIEXPORT void JNICALL JNI(Stop)(JNIEnv* env, jclass jclass_caller, jint params)
{
    NativeParams* handle = (NativeParams*)params;
    int           ret = 0;

    LOGD("Stop()");

    METUX_LOCK();
    
    if((g_aHandle == NULL) || (handle != g_aHandle)) {
        LOGE("<error> invailed native handle, Stop()");
        METUX_UNLOCK();
        return;
    }

    session_stop(g_aHandle->session_handle);
    session_lipsync_unbind(g_aHandle->session_handle, NULL);
    usleep(500000);
    session_destruct(g_aHandle->session_handle);

    if (g_aHandle->jclass_caller != 0) {
        env->DeleteGlobalRef(g_aHandle->jclass_caller);
        g_aHandle->jclass_caller = 0;
    }

    if (g_aHandle->jaudio_sink != 0) {
        env->DeleteGlobalRef(g_aHandle->jaudio_sink);
        g_aHandle->jaudio_sink = 0;
    }

    free(g_aHandle);
    g_aHandle = NULL;

    METUX_UNLOCK();

    return ;
}


JNIEXPORT void JNICALL JNI(OnSend)(JNIEnv* env, jclass jclass_caller,
                                   jint params, jbyteArray jobject_input,
                                   jint length, jlong timestamp, jint dtmfKey)
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
                //send_encode_frame(handle, timestamp, length, frame_data, dtmfKey);

        env->ReleaseByteArrayElements(jobject_input,(jbyte*)frame_data, JNI_ABORT);
    } else {
        LOGE("<error> OnSend(): NULL handle");
    }

    METUX_UNLOCK();

    return ;
}


JNIEXPORT void JNICALL JNI(OnSendDirect)(JNIEnv* env, jclass jclass_caller,
                                         jint params, jobject jobject_input,
                                         jint length, jlong timestamp,
                                         jint dtmfKey)
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
        //send_encode_frame(handle, timestamp, length, frame_data, dtmfKey);

        env->DeleteLocalRef(buffer_input);
    } else {
        LOGE("<error> OnSendDirect(): NULL handle");
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


JNIEXPORT jlong JNICALL JNI(GetSSRC)(JNIEnv* env, jclass jclass_caller, jint params)
{
    /* Not implemet now */
    LOGE("GetSSRC(): Not implement now");
    return 0;
}


JNIEXPORT void JNICALL JNI(SetReportCB)(JNIEnv* env, jclass jclass_caller,
                                        jint params,  jobject jIReportCB)
{
    /* Not implemet now */
    LOGE("SetReportCB(): Not implement now");
    return;
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

