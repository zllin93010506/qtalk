#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "include/G7221DecoderTransformJni.h"
#include "G7221Decoder.h"
#include "logging.h"

#define TAG "G7221DecoderTransformJni"
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
	G7221Decoder	*mG7221Decoder;

	jmethodID 		onMediaStatic;
	jobject 		jIAudioSink;
}NativeParams;

/*
 * Class:     com_quanta_qtalk_media_audio_G7221DecoderTransform
 * Method:    _Start
 * Signature: (Lcom/quanta/qtalk/media/audio/IAudioSink;II)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_audio_G7221DecoderTransform__1Start
(JNIEnv * env, jclass jclass_caller, jobject  jIAudioSink, jint sampleRate, jint numChannels)
{
	NativeParams* result = (NativeParams*) malloc(sizeof(NativeParams));
	int enable_enhancer = true;
	LOGD("==>_Start");
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
	//Initialize G7221
	result->mG7221Decoder = new G7221Decoder(32000,7000);
	result->frame_size = 320;
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
 * Class:     com_quanta_qtalk_media_audio_G7221DecoderTransform
 * Method:    _Stop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_G7221DecoderTransform__1Stop
(JNIEnv * env, jclass jclass_caller, jint params)
{
	NativeParams* handle = (NativeParams*)params;
	LOGD("==>_Stop");
	if (handle != NULL)
	{
		if (handle->jIAudioSink!=0)
		{
			env->DeleteGlobalRef(handle->jIAudioSink);
			handle->jIAudioSink = 0;
		}
		if(handle->mG7221Decoder !=NULL )
		{
			delete handle->mG7221Decoder;
			handle->mG7221Decoder = NULL;
		}
		free(handle);
	}
	LOGD("<==_Stop");
}



/*
 * Class:     com_quanta_qtalk_media_audio_G7221DecoderTransform
 * Method:    _OnMedia
 * Signature: (ILjava/nio/ByteBuffer;IJ[S)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_G7221DecoderTransform__1OnMediaDirect
(JNIEnv * env, jclass jclass_caller,
 jint params, jobject jobjectInput, jint length, jlong timestamp, jshortArray outputArray)
{

	NativeParams* handle = (NativeParams*)params;
	if (handle != NULL && jobjectInput != NULL)
	{
        jshort output_buffer[handle->frame_size];
		//get directly access to the input bytebuffer
		jobject buffer_input = env->NewLocalRef(jobjectInput);
		unsigned char *array_input = (unsigned char*) env->GetDirectBufferAddress(buffer_input);
		int result = handle->mG7221Decoder->Decode(array_input, length, output_buffer, handle->frame_size);

		env->SetShortArrayRegion(outputArray, 0, handle->frame_size, output_buffer);

		//callback to the Isink
		env->CallStaticVoidMethod(jclass_caller,
							handle->onMediaStatic,
							handle->jIAudioSink,
							outputArray,
							handle->frame_size,
							timestamp);

		env->DeleteLocalRef(buffer_input);
	}
}


/*
 * Class:     com_quanta_qtalk_media_audio_G7221DecoderTransform
 * Method:    _OnMedia
 * Signature: (I[B;IJ[S)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_G7221DecoderTransform__1OnMedia
(JNIEnv * env, jclass jclass_caller,
 jint params, jbyteArray jobjectInput, jint length, jlong timestamp, jshortArray outputArray)
{
	NativeParams* handle = (NativeParams*)params;

	if (handle != NULL && jobjectInput != NULL)
	{
        jshort output_buffer[handle->frame_size];
		//get directly access to the input bytebuffer
		jbyte array_input[length];
		env->GetByteArrayRegion(jobjectInput, 0, length, array_input);
		int result = handle->mG7221Decoder->Decode((unsigned char*)array_input, length, output_buffer, handle->frame_size);

		env->SetShortArrayRegion(outputArray, 0, handle->frame_size, output_buffer);
		//callback to the Isink
		env->CallStaticVoidMethod(jclass_caller,
							handle->onMediaStatic,
							handle->jIAudioSink,
							outputArray,
							handle->frame_size,
							timestamp);
	}
}

#ifdef __cplusplus
}
#endif
