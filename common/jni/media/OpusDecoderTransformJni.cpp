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
#include <jni.h>
#include "logging.h"
#include "include/OpusDecoderTransformJni.h"
#include "opus.h"
#include "opus_private.h"

#ifdef __cplusplus
extern "C" {
#endif
/* =============================================================================
                                    Macros
============================================================================= */
#define TAG "StreamEngine_OpusDecoderTransformJni"
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
	jobject 		jIAudioSink;
	jmethodID 		onMediaStatic;
	
	jint			frame_size;
	jlong                   timestamp;
	OpusDecoder*    	dec;
}NativeParams;

/* =============================================================================
                                Implement Start
============================================================================= */
/*
 * Class:     com_quanta_qtalk_media_audio_OpusDecoderTransform
 * Method:    _Start
 * Signature: (Lcom/quanta/qtalk/media/audio/IAudioSink;II)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_audio_OpusDecoderTransform__1Start
(JNIEnv * env, jclass jclass_caller, jobject  jIAudioSink, jint sampleRate, jint numChannels)
{
	NativeParams* result = (NativeParams*) malloc(sizeof(NativeParams));
	//return (jint)result;
	int error = 0;
	int size  = -1;
	LOGD("==>_Start");
	if (!result)
	{
		LOGD("Unable to malloc sizeof(NativeParams)");
		goto ERROR;
	}
	memset(result,0,sizeof(NativeParams));
	// Obtain a global ref to the Java VM

	if (jIAudioSink!=NULL)
	{
		result->jIAudioSink = env->NewGlobalRef(jIAudioSink);
		if (result->jIAudioSink==0)
		{
			LOGD("Unable to get jIAudioSink");
			goto ERROR;
		}
	}
	//Get the method for callback after on media process success
	#define METHOD_NAME "onMediaNative"
	#define METHOD_SIG  "(Lcom/quanta/qtalk/media/audio/IAudioSink;[SIJ)V"
	result->onMediaStatic = env->GetStaticMethodID(jclass_caller,	//Class
									   METHOD_NAME,					//Method
									   METHOD_SIG); 				//Signature
	if (result->onMediaStatic==0)
	{
		LOGD("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
		goto ERROR;
	}
	//Initialize Opus

	LOGD("Iron test Decoder sampleRate : %d , numChannels : %d",sampleRate, numChannels);
	result->dec = opus_decoder_create(sampleRate, numChannels, &error);
	if (error != OPUS_OK) {
		return error;
	}

	if (error != OPUS_OK) {
		return error;
	}

	LOGD("<==_Start:%d",result->frame_size);
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
 * Class:     com_quanta_qtalk_media_audio_OpusDecoderTransform
 * Method:    _Stop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_OpusDecoderTransform__1Stop
  (JNIEnv * env, jclass jclass_caller, jint params)
{
	//return;
	NativeParams* handle = (NativeParams*)params;
	LOGD("==>_Stop");
	if (handle != NULL)
	{
		if (handle->jIAudioSink!=0)
		{
			env->DeleteGlobalRef(handle->jIAudioSink);
			handle->jIAudioSink = NULL;
		}
		if(handle->dec){
			opus_decoder_destroy(handle->dec);	
		}

		free(handle);
		handle =NULL;
	}

	LOGD("<==_Stop");	
}

/*
 * Class:     com_quanta_qtalk_media_audio_OpusDecoderTransform
 * Method:    _OnMedia
 * Signature: (I[BIJ[S)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_OpusDecoderTransform__1OnMedia
(JNIEnv * env, jclass jclass_caller,
 jint params, jbyteArray jobjectInput, jint length, jlong timestamp, jshortArray outputArray)
{
	//return;
	NativeParams* handle = (NativeParams*)params;
	int dec_length;
	if (handle != NULL && jobjectInput != NULL && length != 0)
	{
        	jshort output_buffer[handle->frame_size];
		//get directly access to the input bytebuffer
		jbyte array_input[length];
		env->GetByteArrayRegion(jobjectInput, 0, length, array_input);

		if (handle->timestamp == 0 ){
			handle->timestamp = timestamp;
			return ;
		}

		if (handle->timestamp == 960 || timestamp - handle->timestamp >= 1920) {
			dec_length = opus_decode(handle->dec, NULL, 
		                      length, output_buffer, handle->frame_size, 1);	
			env->SetShortArrayRegion(outputArray, 0, handle->frame_size, output_buffer);
		 
			env->CallStaticVoidMethod(jclass_caller,
							      handle->onMediaStatic,
							      handle->jIAudioSink,
							      outputArray,
							      handle->frame_size,
							      timestamp);
		}
			dec_length = opus_decode(handle->dec, NULL, 
		                      length, output_buffer, handle->frame_size, 0);	
			env->SetShortArrayRegion(outputArray, 0, handle->frame_size, output_buffer);
		 
			env->CallStaticVoidMethod(jclass_caller,
							      handle->onMediaStatic,
							      handle->jIAudioSink,
							      outputArray,
							      handle->frame_size,
							      timestamp);

		env->SetShortArrayRegion(outputArray, 0, handle->frame_size, output_buffer);
		//callback to the Isink
		env->CallStaticVoidMethod(jclass_caller,
							handle->onMediaStatic,
							handle->jIAudioSink,
							outputArray,
							handle->frame_size,
							timestamp);
		
	}
	handle->timestamp = timestamp;
	return;
}

/*
 * Class:     com_quanta_qtalk_media_audio_OpusDecoderTransform
 * Method:    _OnMediaDirect
 * Signature: (ILjava/nio/ByteBuffer;IJ[S)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_OpusDecoderTransform__1OnMediaDirect
(JNIEnv * env, jclass jclass_caller,
 jint params, jobject jobjectInput, jint length, jlong timestamp, jshortArray outputArray)
{
	//return;
	NativeParams* handle = (NativeParams*)params;
	int dec_length ;
	handle->frame_size = 640;

	if(length == 0)
		LOGD("timestamp = %lld",timestamp);
	handle->timestamp = timestamp;

	if (handle != NULL && jobjectInput != NULL && length != 0)
	{
        jshort output_buffer[handle->frame_size];

        if (1 == length) {
        	//LOGD("%s %d: silence frame", __FUNCTION__, __LINE__);
        	memset(output_buffer, 0x00, handle->frame_size*sizeof(jshort));
        	env->SetShortArrayRegion(outputArray, 0, handle->frame_size, output_buffer);
        	env->CallStaticVoidMethod(jclass_caller,
        								handle->onMediaStatic,
        								handle->jIAudioSink,
        								outputArray,
        								handle->frame_size,
        								timestamp);
        	return ;
        }
		
		//get directly access to the input bytebuffer
		jobject buffer_input = env->NewLocalRef(jobjectInput);
		
		unsigned char *array_input = (unsigned char*) env->GetDirectBufferAddress(buffer_input);
		if (handle->timestamp == 0 ){
			handle->timestamp = timestamp;
			return ;
		}

		if (handle->timestamp == 960 || timestamp - handle->timestamp >= 1920) {

			dec_length = opus_decode(handle->dec, NULL, 
		                      length, output_buffer, 320, 1);
			LOGD("StreamEngine OPUS FEC recover : %d \n",dec_length);
			env->SetShortArrayRegion(outputArray, 0, handle->frame_size, output_buffer);
			if(dec_length > 0 && dec_length <= handle->frame_size ){
			env->CallStaticVoidMethod(jclass_caller,
							      handle->onMediaStatic,
							      handle->jIAudioSink,
							      outputArray,
							      dec_length,
							      timestamp);
			}
		}
		//LOGD("opus_decode length: %d, opus_decode handle->frame_size: %d",length, handle->frame_size);

		dec_length = opus_decode(handle->dec, array_input, length, 
					output_buffer, 320, 0);

		if (dec_length != 320) {
			LOGD("StreamEngine opus decode failure %d", dec_length);
		}
		if(dec_length == OPUS_OK){
			LOGD("OPUS_OK");
		}else if(dec_length==OPUS_BAD_ARG){
			LOGD("OPUS_BAD_ARG");
		}else if (dec_length == OPUS_BUFFER_TOO_SMALL){
			LOGD("OPUS_BUFFER_TOO_SMALL");
		}else if (dec_length == OPUS_INTERNAL_ERROR){
			LOGD("OPUS_INTERNAL_ERROR");
		}else if (dec_length == OPUS_INVALID_PACKET){
			LOGD("OPUS_INVALID_PACKET");
		}else if (dec_length == OPUS_UNIMPLEMENTED){
			LOGD("OPUS_UNIMPLEMENTED");
		}else if (dec_length == OPUS_INVALID_STATE){
			LOGD("OPUS_INVALID_STATE");
		}else if (dec_length == OPUS_ALLOC_FAIL){
			LOGD("OPUS_ALLOC_FAIL");
		}
	
		env->SetShortArrayRegion(outputArray, 0, handle->frame_size, output_buffer);
		//callback to the Isink
		if(dec_length > 0 && dec_length <= handle->frame_size ){
			env->CallStaticVoidMethod(jclass_caller,
								handle->onMediaStatic,
								handle->jIAudioSink,
								outputArray,
								dec_length,
								timestamp);
		}
		env->DeleteLocalRef(buffer_input);
	}

	return;
}

#ifdef __cplusplus
}
#endif
