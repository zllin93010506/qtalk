#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "include/SpeexEncoderTransformJni.h"
extern "C" {
    #include "thirdparty/speex-1.2rc1/include/speex/speex.h"
}
#include "logging.h"

#define TAG "SpeexEncoderTransformJni"
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
	SpeexBits 		speex_bits;
	void 			*speex_state;

	unsigned char*	array_output;
	jobject			buffer_output;
	jmethodID 		onMediaStatic;
	jobject 		jIAudioSink;
}NativeParams;

/*
 * Class:     com_quanta_qtalk_media_audio_SpeexEncoderTransform
 * Method:    _Start
 * Signature: (Lcom/quanta/qtalk/media/audio/IAudioSink;I)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_audio_SpeexEncoderTransform__1Start
(JNIEnv * env, jclass jclass_caller, jobject  jIAudioSink, jint compression, jint sampleRate, jint numChannels)
{
	NativeParams* result = (NativeParams*) malloc(sizeof(NativeParams));
	jobject buffer_output = NULL;
	LOGD("==>_Start compression:%d sampleRate:%d numChannels:%d",compression,sampleRate,numChannels);
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
	//Initialize Speex
	speex_bits_init(&result->speex_bits);
	result->speex_state = speex_encoder_init(&speex_nb_mode);
	speex_encoder_ctl(result->speex_state, SPEEX_SET_QUALITY, &compression);
    speex_encoder_ctl(result->speex_state, SPEEX_SET_SAMPLING_RATE, &sampleRate);
	speex_encoder_ctl(result->speex_state, SPEEX_GET_FRAME_SIZE, &result->frame_size);
    //speex_encoder_ctl(mpSpeexCodec->state_enc, SPEEX_SET_COMPLEXITY, &tmp);
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
 * Class:     com_quanta_qtalk_media_audio_SpeexEncoderTransform
 * Method:    _Stop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_SpeexEncoderTransform__1Stop
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
		speex_bits_destroy(&handle->speex_bits);
		speex_encoder_destroy(handle->speex_state);
		free(handle);
	}
	LOGD("<==_Stop");
}

/*
 * Class:     com_quanta_qtalk_media_audio_SpeexEncoderTransform
 * Method:    _OnMedia
 * Signature: (I[SIJ)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_SpeexEncoderTransform__1OnMedia
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

			speex_bits_reset(&handle->speex_bits);
			speex_encode_int(handle->speex_state, array_input, &handle->speex_bits);
			// SDP frame terminator
			//speex_bits_pack(&handle->speex_bits, 0xf, 5);
			int length_output = speex_bits_write(&handle->speex_bits, (char *)handle->array_output,length);

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
