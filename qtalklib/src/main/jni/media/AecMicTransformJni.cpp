/*******************************************************************************
* All Rights Reserved, Copyright @ Quanta Computer Inc. 2014
* File Name: AecTransformJni.cpp
* File Mark:
* Content Description:
* Other Description: None
* Version: 1.0
* Author: Iron Chang
* Date: 2014/06/11/
*
* History:
*   1.2014/06/11 Iron Chang: draft
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
#include "include/AecMicTransformJni.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "Sys_Software_AEC.h"

/* =============================================================================
                                    Macros
   ============================================================================= */
#define TAG "QSE_AecMicTransformJni"
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
	JavaVM 			*jvm;
	jobject 		jIAudioSink;
	jmethodID 		onMediaStatic;
	
	jint			frame_size;
	jlong                   timestamp;
} NativeParams;

/* =============================================================================
                                Implement Start
   ============================================================================= */

/*
 * Class:     com_quanta_qtalk_media_audio_AecMicTransform
 * Method:    _Start
 * Signature: (Lcom/quanta/qtalk/media/audio/IAudioSink;II)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_audio_AecMicTransform__1Start
 (JNIEnv * env, jclass jclass_caller, jobject jIAudioSink, jint sampleRate, jint numChannels){
NativeParams* result = (NativeParams*) malloc(sizeof(NativeParams));
	jobject buffer_output = NULL;
	LOGD("==>_Start sampleRate:%d numChannels:%d",sampleRate,numChannels);
	if (!result)
	{
		LOGD("Unable to malloc sizeof(NativeParams)");
		goto ERROR;
	}
	memset(result,0,sizeof(NativeParams));
	// Obtain a global ref to the Java VM
	env->GetJavaVM(&result->jvm);
	if ( result->jvm == 0)
	{
		LOGD("Unable to GetJavaVM");
		goto ERROR;
	}

	result->jIAudioSink = env->NewGlobalRef(jIAudioSink);
	if (result->jIAudioSink==0)
	{
		LOGD("Unable to get jIAudioSink");
		goto ERROR;
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

	/*if(result->alcProcess == NULL)
		result->alcProcess = new AlcProcess(sampleRate);*/
	//sw_aec_init(sampleRate);
	

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
 * Class:     com_quanta_qtalk_media_audio_AecMicTransform
 * Method:    _Stop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_AecMicTransform__1Stop
   (JNIEnv * env, jclass jclass_caller, jint params){
	NativeParams* handle = (NativeParams*)params;
	LOGD("==>_Stop");
	if (handle != NULL)
	{

		if (handle->jIAudioSink!=0)
		{
			env->DeleteGlobalRef(handle->jIAudioSink);
			handle->jIAudioSink = 0;
		}
		//sw_aec_exit();
		free(handle);
	}
	LOGD("<==_Stop");
}

/*
 * Class:     com_quanta_qtalk_media_audio_AecMicTransform
 * Method:    _OnMedia
 * Signature: (I[SIJ)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_AecMicTransform__1OnMedia
  (JNIEnv * env, jclass jclass_caller, jint params, jshortArray jobject_input, jint length, jlong timestamp){
	NativeParams* handle = (NativeParams*)params;
	if (handle != NULL && jobject_input != NULL)
	{
		//get directly access to the input bytebuffer
		jshort array_input[length];
		env->GetShortArrayRegion(jobject_input, 0, length, array_input);
		sw_aec_pop((char*)array_input, length*2);

		env->SetShortArrayRegion(jobject_input, 0, length, array_input);
		//callback to the Isink
		env->CallStaticVoidMethod(jclass_caller,
					handle->onMediaStatic,
					handle->jIAudioSink,
					jobject_input,//handle->buffer_output,
					length,
					timestamp);
	}

}

#ifdef __cplusplus
}
#endif 

