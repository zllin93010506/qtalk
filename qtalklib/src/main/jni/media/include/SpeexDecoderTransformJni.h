/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_quanta_qtalk_media_audio_SpeexDecoderTransform */

#ifndef _Included_com_quanta_qtalk_media_audio_SpeexDecoderTransform
#define _Included_com_quanta_qtalk_media_audio_SpeexDecoderTransform
#ifdef __cplusplus
extern "C" {
#endif
#undef com_quanta_qtalk_media_audio_SpeexDecoderTransform_DEFAULT_ID
#define com_quanta_qtalk_media_audio_SpeexDecoderTransform_DEFAULT_ID 97L
#undef com_quanta_qtalk_media_audio_SpeexDecoderTransform_mInFormat
#define com_quanta_qtalk_media_audio_SpeexDecoderTransform_mInFormat 97L
#undef com_quanta_qtalk_media_audio_SpeexDecoderTransform_mOutFormat
#define com_quanta_qtalk_media_audio_SpeexDecoderTransform_mOutFormat 255L
/*
 * Class:     com_quanta_qtalk_media_audio_SpeexDecoderTransform
 * Method:    _Start
 * Signature: (Lcom/quanta/qtalk/media/audio/IAudioSink;II)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_audio_SpeexDecoderTransform__1Start
  (JNIEnv *, jclass, jobject, jint, jint);

/*
 * Class:     com_quanta_qtalk_media_audio_SpeexDecoderTransform
 * Method:    _Stop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_SpeexDecoderTransform__1Stop
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_quanta_qtalk_media_audio_SpeexDecoderTransform
 * Method:    _OnMedia
 * Signature: (I[BIJ[S)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_SpeexDecoderTransform__1OnMedia
  (JNIEnv *, jclass, jint, jbyteArray, jint, jlong, jshortArray);

/*
 * Class:     com_quanta_qtalk_media_audio_SpeexDecoderTransform
 * Method:    _OnMediaDirect
 * Signature: (ILjava/nio/ByteBuffer;IJ[S)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_SpeexDecoderTransform__1OnMediaDirect
  (JNIEnv *, jclass, jint, jobject, jint, jlong, jshortArray);

#ifdef __cplusplus
}
#endif
#endif
