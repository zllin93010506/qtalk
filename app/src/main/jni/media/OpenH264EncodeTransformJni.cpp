/*******************************************************************************
* All Rights Reserved, Copyright @ Quanta Computer Inc. 2014
* File Name: OpenH264EncodeTransformJni.cpp
* File Mark:
* Content Description:
* Other Description: None
* Version: 1.0
*
* History:
*   1.2014/06/11 Iron Chang: draft
*   2.2022/3/30 Frank Ting: modify

*******************************************************************************/

/* =============================================================================
                                Included headers
   ============================================================================= */
#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "logging.h"
#include "include/OpenH264EncodeTransformJni.h"
#include "openh264_enc.h"

//#ifdef __cplusplus
//extern "C" {
//#endif

/* =============================================================================
                                    Macros
   ============================================================================= */
#define TAG "QSE_OpenH264EncodeTransformJni"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

/* =============================================================================
                                    Types
   ============================================================================= */
typedef struct _NativeParams
{
	JavaVM              *jvm;
	jobject             jISink;
	jmethodID           onMediaStatic;
    jint                width;
    jint                height;
} NativeParams;

/* =============================================================================
                                Implement Start
   ============================================================================= */
unsigned char* as_unsigned_char_array(JNIEnv *env, jbyteArray array) {
    int len = env->GetArrayLength (array);
    unsigned char* buf = new unsigned char[len];
    env->GetByteArrayRegion (array, 0, len, reinterpret_cast<jbyte*>(buf));
    return buf;
}

/*
 * Class:     com_quanta_qtalk_media_video_OpenH264EncodeTransform
 * Method:    _Start
 * Signature: (Lcom/quanta/qtalk/media/ISink;III)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_video_OpenH264EncodeTransform__1Start
 (JNIEnv * env, jclass jclass_caller, jobject  jISink, jint pixFormat, jint width, jint height, jint framerate, jint bitrate){
	NativeParams* result = (NativeParams*) malloc(sizeof(NativeParams));
	int ret = -1;

	if (!result) {
		LOGD("%s <error> Unable to malloc sizeof(NativeParams)", TAG);
		goto ERROR;
	}
	memset(result,0,sizeof(NativeParams));

	env->GetJavaVM(&result->jvm);
	if ( result->jvm == 0) {
		LOGD("%s <error> Unable to GetJavaVM", TAG);
		goto ERROR;
	}

	if (jISink!=NULL){
	    {
	        result->jISink = env->NewGlobalRef(jISink);
	        if (result->jISink==0)
	        {
	            LOGD("%s <error> Unable to get jISink", TAG);
	            goto ERROR;
	        }
	    }
	}
    //Get the method for callback after on media process success
    #define METHOD_NAME "onMediaNative"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/media/video/IVideoSink;Ljava/nio/ByteBuffer;IJS)V"
    result->onMediaStatic = env->GetStaticMethodID(jclass_caller,    //Class
                                       METHOD_NAME,                    //Method
                                       METHOD_SIG);                 //Signature
	if (result->onMediaStatic==0) {
		LOGE("%s <error> Unable to GetMethodID %s %s", TAG, METHOD_NAME, METHOD_SIG);
		goto ERROR;
	}
    result->width = width;
    result->height = height;

	ret = open_h264_encorder_create();
	if (ret != 0) {
		LOGE("<error> %s() open_h264_encorder_create failure, ret %d", __FUNCTION__, ret);
		return -1;
	}

	ret = open_h264_encorder_initialize(result->width, result->height, bitrate, framerate, 10, NULL);
	if (ret != 0) {
		open_h264_encorder_destroy();
		LOGE("<error> %s() open_h264_encorder_initialize failure, ret %d", __FUNCTION__, ret);
		return -1;
	}
	return (jint)result;

ERROR:
	if (result != NULL) {
		free(result);
	}
	return 0;
}

/*
 * Class:     com_quanta_qtalk_media_video_OpenH264EncodeTransform
 * Method:    _Stop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_OpenH264EncodeTransform__1Stop
   (JNIEnv * env, jclass jclass_caller, jint params){
	NativeParams* handle = (NativeParams*)params;
	int ret = -1;
	if (handle != NULL)	{
        ret = open_h264_encorder_uninitialize();
		if (ret != 0){
			LOGE("%s <error> open_h264_encorder_uninitialize failure, ret %d", TAG, ret);
		} else {
			LOGD("%s open_h264_encorder_uninitialize() success", TAG);
		}
		open_h264_encorder_destroy();
        if (handle->jISink!=0)
        {
            env->DeleteGlobalRef(handle->jISink);
            handle->jISink = 0;
        }
        free(handle);
	}
}

/*
 * Class:     com_quanta_qtalk_media_video_OpenH264EncodeTransform
 * Method:    _OnMedia
 * Signature: (ILjava/nio/ByteBuffer;IJS)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_OpenH264EncodeTransform__1OnMedia
 (JNIEnv * env, jclass jclass_caller, jint params, jbyteArray jbyteArray_input, jint length, jlong timestamp, jshort rotation)
{
    NativeParams *handle = (NativeParams *)params;
    unsigned char h264buf[1280*720*2];
    int len;

    if ((handle == NULL) || (jbyteArray_input == NULL))
        return;

    unsigned char *yuv_data = as_unsigned_char_array(env, jbyteArray_input);
    int yuv_len = strnlen((char *)yuv_data, static_cast<int>(length));

    len = open_h264_encodeframe(yuv_data, yuv_len, timestamp, h264buf);
    free(yuv_data);
    if(len > 0) {
        jobject jh264buf;
        jh264buf = env->NewDirectByteBuffer( (void *)h264buf, len);
        if (jh264buf == 0) {
            LOGE("%s <error> Unable to NewDirectByteBuffer", TAG);
            return;
        }
        env->CallStaticVoidMethod(jclass_caller,
                                  handle->onMediaStatic,
                                  handle->jISink,
                                  jh264buf,
                                  len,
                                  timestamp,
                                  rotation);
    }
}

/*
 * Class:     com_quanta_qtalk_media_video_OpenH264EncodeTransform
 * Method:    _SetGop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_OpenH264EncodeTransform__1SetGop
  (JNIEnv * env, jclass jclass_caller, jint gop){
	 LOGD("%s Set Gop is not implement now", TAG);
}

/*
 * Class:     com_quanta_qtalk_media_video_OpenH264EncodeTransform
 * Method:    _SetBitRate
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_OpenH264EncodeTransform__1SetBitRate
  (JNIEnv * env, jclass jclass_caller, jint bitrate){
	//LOGD("%s SetBitrate %d kbps (fk)", TAG, bitrate);
    open_h264_encorder_set_bitrate(bitrate);
}

/*
 * Class:     com_quanta_qtalk_media_video_OpenH264EncodeTransform
 * Method:    _SetFrameRate
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_OpenH264EncodeTransform__1SetFrameRate
  (JNIEnv * env, jclass jclass_caller, jint framerate){
	//LOGD("%s SetFrameRate %d fps (fk)", TAG, framerate);
	open_h264_encorder_set_frame_rate(framerate);
}

/*
 * Class:     com_quanta_qtalk_media_video_OpenH264EncodeTransform
 * Method:    _RequestIDR
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_OpenH264EncodeTransform__1RequestIDR
  (JNIEnv * env, jclass jclass_caller){
	//LOGD("%s Request IDR (fk)", TAG);
	open_h264_encorder_idr_request();
}
