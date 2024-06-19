#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include "include/G7221EncoderTransformJni.h"
#include "G7221Encoder.h"
#include "logging.h"

#define TAG "G7221EncoderTransformJni"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)


#ifdef __cplusplus
extern "C" {
#endif
typedef struct _NativeParams
{
	JavaVM 			*jvm;
	jint			frame_size;
	G7221Encoder	*mG7221Encoder;

	unsigned char*	array_output;
	jobject			buffer_output;
	jmethodID 		onMediaStatic;
	jobject 		jIAudioSink;
}NativeParams;

/*
 * Class:     com_quanta_qtalk_media_audio_G7221EncoderTransform
 * Method:    _Start
 * Signature: (Lcom/quanta/qtalk/media/audio/IAudioSink;I)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_audio_G7221EncoderTransform__1Start
(JNIEnv * env, jclass jclass_caller, jobject  jIAudioSink, jint sampleRate, jint numChannels)
{
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
	#define METHOD_SIG  "(Lcom/quanta/qtalk/media/audio/IAudioSink;Ljava/nio/ByteBuffer;IJ)V"
	result->onMediaStatic = env->GetStaticMethodID(jclass_caller,	//Class
									   METHOD_NAME,					//Method
									   METHOD_SIG); 				//Signature
	if (result->onMediaStatic==0)
	{
		LOGD("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
		goto ERROR;
	}
	if(result->mG7221Encoder == NULL)
		result->mG7221Encoder = new G7221Encoder(32000,7000);
	result->frame_size = 320;
	//allocate output buffer
	result->array_output = (unsigned char*)malloc(result->frame_size);
	buffer_output = env->NewDirectByteBuffer( (void*)result->array_output, result->frame_size);
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
 * Class:     com_quanta_qtalk_media_audio_G7221EncoderTransform
 * Method:    _Stop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_G7221EncoderTransform__1Stop
(JNIEnv * env, jclass jclass_caller, jint params)
{
	NativeParams* handle = (NativeParams*)params;
	LOGD("==>_Stop");
	if (handle != NULL)
	{
		if (handle->array_output!=0)
		{
			free(handle->array_output);
			handle->array_output = 0;
		}
		if (handle->buffer_output!=0)
		{
			env->DeleteGlobalRef(handle->buffer_output);
			handle->buffer_output = 0;
		}
		if (handle->jIAudioSink!=0)
		{
			env->DeleteGlobalRef(handle->jIAudioSink);
			handle->jIAudioSink = 0;
		}
		if (handle->mG7221Encoder!=NULL)
		{
			delete handle->mG7221Encoder;
			handle->mG7221Encoder = NULL;
		}
		free(handle);
	}
	LOGD("<==_Stop");
}

/*
 * Class:     com_quanta_qtalk_media_audio_G7221EncoderTransform
 * Method:    _OnMedia
 * Signature: (I[SIJ)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_G7221EncoderTransform__1OnMedia
(JNIEnv * env, jclass jclass_caller,
 jint params, jshortArray jobject_input, jint length, jlong timestamp)
{
	NativeParams* handle = (NativeParams*)params;
	if (handle != NULL && jobject_input != NULL)
	{
		//LOGD("OnMedia:%d/%d",length,handle->frame_size);
		if (length > handle->frame_size)
			LOGE("OnMedia: input length(%d) > frame size(%d)",length,handle->frame_size);
		else
		{
			//get directly access to the input bytebuffer
			jshort array_input[length];
			env->GetShortArrayRegion(jobject_input, 0, length, array_input);
			int length_output = handle->mG7221Encoder->Encode(array_input, length,(unsigned char *)handle->array_output, handle->frame_size);

			//callback to the Isink
			env->CallStaticVoidMethod(jclass_caller,
								handle->onMediaStatic,
								handle->jIAudioSink,
								handle->buffer_output,
								length_output,
								timestamp);
		}
	}
}


#ifdef __cplusplus
}
#endif
