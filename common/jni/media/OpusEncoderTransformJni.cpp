/*******************************************************************************
* All Rights Reserved, Copyright @ Quanta Computer Inc. 2013
* File Name: OpusEncoderTransformJni.cpp
* File Mark:
* Content Description:
* Other Description: None
* Version: 1.0
* Author: Iron Chang
* Date: 2013/07/07/
*
* History:
*   1.2013/07/07 Iron Chang: draft
*******************************************************************************/
/* =============================================================================
                                Included headers
============================================================================= */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "logging.h"

#include "opus.h"
#include "opus_private.h"
#include "include/OpusEncoderTransformJni.h"



#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================================================
                                    Macros
============================================================================= */
#define TAG "OpusEncoderTransformJni"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)


#define ONE_OPUS_FRAME_NUM ((20*16000)/1000)
#define ONE_OPUS_ENCODE_BITRATE 24000

static unsigned char opus_buffer[128];
/* =============================================================================
                                    Types
============================================================================= */
typedef struct _NativeParams
{
	jobject			buffer_output;
	jobject 		jIAudioSink;
	jmethodID 		onMediaStatic;
	
	jint			frame_size;
	unsigned char*		array_output;
	OpusEncoder*    	enc;
}NativeParams;

/* =============================================================================
                                Implement Start
============================================================================= */
/*
 * Class:     com_quanta_qtalk_media_audio_OpusEncoderTransform
 * Method:    _Start
 * Signature: (Lcom/quanta/qtalk/media/audio/IAudioSink;III)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_audio_OpusEncoderTransform__1Start
  (JNIEnv *env, jclass jclass_caller, jobject jIAudioSink, jint compression, jint sampleRate, jint numChannels)
{

	NativeParams* result = (NativeParams*) malloc(sizeof(NativeParams));
	int error = OPUS_OK;
	int size  = -1;
	unsigned char*		ptr;
	result->frame_size = ONE_OPUS_ENCODE_BITRATE/50/8;
	jobject buffer_output = NULL;
	LOGD("==>_Start compression:%d sampleRate:%d numChannels:%d",compression,sampleRate,numChannels);
	if (!result){
		LOGD("Unable to malloc sizeof(NativeParams)");
		goto ERROR;
	}
	memset(result,0,sizeof(NativeParams));

	result->jIAudioSink = env->NewGlobalRef(jIAudioSink);
	if (result->jIAudioSink==0)
	{
		LOGD("Unable to get jIAudioSink");
		goto ERROR;
	}	
	#define METHOD_NAME "onMediaNative"
	#define METHOD_SIG  "(Lcom/quanta/qtalk/media/audio/IAudioSink;Ljava/nio/ByteBuffer;IJ)V"
	result->onMediaStatic = env->GetStaticMethodID(jclass_caller,	//Class
									   METHOD_NAME,					//Method
									   METHOD_SIG); 				//Signature
	if (result->onMediaStatic==0)
	{
		LOGD("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
		goto ERROR;
	}
	//Initialize Opus	
	//LOGD("Iron test Encoder sampleRate : %d , numChannels : %d",sampleRate, numChannels);
	result->enc = opus_encoder_create(sampleRate, numChannels, OPUS_APPLICATION_VOIP, &error);
	if(error != OPUS_OK){
		LOGD("Opus encoder create error");
		return error;
	}
	if(error != OPUS_OK){
		LOGD("Opus encoder init error");
		return error;
	}	
	opus_encoder_ctl(result->enc, OPUS_SET_BITRATE(ONE_OPUS_ENCODE_BITRATE));
	opus_encoder_ctl(result->enc, OPUS_SET_COMPLEXITY(compression));
	opus_encoder_ctl(result->enc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
	opus_encoder_ctl(result->enc, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_WIDEBAND));
	opus_encoder_ctl(result->enc, OPUS_SET_FORCE_CHANNELS(1)); 
	opus_encoder_ctl(result->enc, OPUS_SET_FORCE_MODE(MODE_SILK_ONLY));
	opus_encoder_ctl(result->enc, OPUS_SET_VBR(0));
	opus_encoder_ctl(result->enc, OPUS_SET_INBAND_FEC(1));
	opus_encoder_ctl(result->enc, OPUS_SET_PACKET_LOSS_PERC(25));
	opus_encoder_ctl(result->enc, OPUS_SET_LSB_DEPTH(16));
	
	result->array_output = opus_buffer;
	buffer_output = env->NewDirectByteBuffer( (void*)result->array_output, (result->frame_size));
	if (buffer_output==0)
	{
		LOGD("Unable to NewDirectByteBuffer");
		goto ERROR;
	}
	result->buffer_output = env->NewGlobalRef(buffer_output);

	LOGD("<==_Start frame_size:%d",result->frame_size);
	return (jint)result;


ERROR:
	LOGE("Error on _Start");
	if (result != NULL)
	{
		free(result);
	}
	LOGD("<==_Start");
	return 0;
}


/*
 * Class:     com_quanta_qtalk_media_audio_OpusEncoderTransform
 * Method:    _Stop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_OpusEncoderTransform__1Stop
(JNIEnv * env, jclass jclass_caller, jint params)
{
	//return;
	NativeParams* handle = (NativeParams*)params;
	LOGD("==>_Stop");
	if (handle != NULL)
	{
		if (handle->array_output)
		{
			//free(handle->array_output);
			handle->array_output = NULL;
		}
		if (handle->buffer_output)
		{
			env->DeleteGlobalRef(handle->buffer_output);
			handle->buffer_output = NULL;
		}
		if (handle->jIAudioSink)
		{
			env->DeleteGlobalRef(handle->jIAudioSink);
			handle->jIAudioSink = NULL;
		}
		if (handle->enc)
		{
			opus_encoder_destroy(handle->enc);
			handle->enc = NULL;
		}
		free(handle);
		handle = NULL;
	}
	LOGD("<==_Stop");
	return;
}





/*
 * Class:     com_quanta_qtalk_media_audio_OpusEncoderTransform
 * Method:    _OnMedia
 * Signature: (I[SIJ)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_OpusEncoderTransform__1OnMedia
(JNIEnv * env, jclass jclass_caller,
 jint params, jshortArray jobject_input, jint length, jlong timestamp)
{
	//return;
	NativeParams* handle = (NativeParams*)params;
	handle->frame_size = 60;
	if (handle != NULL && jobject_input != NULL)
	{
			//get directly access to the input bytebuffer
		    //LOGD("opus array input length : %d", length);
			jshort array_input[length];
			env->GetShortArrayRegion(jobject_input, 0, length, array_input);
			int length_output = opus_encode(handle->enc, array_input, ONE_OPUS_FRAME_NUM,
					handle->array_output, handle->frame_size);
			if(length_output!=60)
				LOGD("Iron length error : %d",length_output);
			//LOGD("opus encode length_output : %d",length_output);
			if(length_output == OPUS_OK){
				LOGD("OPUS_OK");
			}else if(length_output==OPUS_BAD_ARG){
				LOGD("OPUS_BAD_ARG");
			}else if (length_output == OPUS_BUFFER_TOO_SMALL){
				LOGD("OPUS_BUFFER_TOO_SMALL");
			}else if (length_output == OPUS_INTERNAL_ERROR){
				LOGD("OPUS_INTERNAL_ERROR");
			}else if (length_output == OPUS_INVALID_PACKET){
				LOGD("OPUS_INVALID_PACKET");
			}else if (length_output == OPUS_UNIMPLEMENTED){
				LOGD("OPUS_UNIMPLEMENTED");
			}else if (length_output == OPUS_INVALID_STATE){
				LOGD("OPUS_INVALID_STATE");
			}else if (length_output == OPUS_ALLOC_FAIL){
				LOGD("OPUS_ALLOC_FAIL");
			}
			if(length_output != handle->frame_size)
				return;
			//callback to the Isink
			env->CallStaticVoidMethod(jclass_caller,
								handle->onMediaStatic,
								handle->jIAudioSink,
								handle->buffer_output,
								length_output,
								timestamp);
	}


}

#ifdef __cplusplus
}
#endif 
