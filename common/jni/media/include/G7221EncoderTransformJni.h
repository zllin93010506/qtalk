/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_quanta_qtalk_media_audio_G7221EncoderTransform */

#ifndef _Included_com_quanta_qtalk_media_audio_G7221EncoderTransform
#define _Included_com_quanta_qtalk_media_audio_G7221EncoderTransform
#ifdef __cplusplus
extern "C" {
#endif
#undef com_quanta_qtalk_media_audio_G7221EncoderTransform_DEFAULT_ID
#define com_quanta_qtalk_media_audio_G7221EncoderTransform_DEFAULT_ID 102L
#undef com_quanta_qtalk_media_audio_G7221EncoderTransform_mInFormat
#define com_quanta_qtalk_media_audio_G7221EncoderTransform_mInFormat 255L
#undef com_quanta_qtalk_media_audio_G7221EncoderTransform_mOutFormat
#define com_quanta_qtalk_media_audio_G7221EncoderTransform_mOutFormat 102L
/*
 * Class:     com_quanta_qtalk_media_audio_G7221EncoderTransform
 * Method:    _Start
 * Signature: (Lcom/quanta/qtalk/media/audio/IAudioSink;II)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_audio_G7221EncoderTransform__1Start
  (JNIEnv *, jclass, jobject, jint, jint);

/*
 * Class:     com_quanta_qtalk_media_audio_G7221EncoderTransform
 * Method:    _Stop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_G7221EncoderTransform__1Stop
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_quanta_qtalk_media_audio_G7221EncoderTransform
 * Method:    _OnMedia
 * Signature: (I[SIJ)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_G7221EncoderTransform__1OnMedia
  (JNIEnv *, jclass, jint, jshortArray, jint, jlong);

#ifdef __cplusplus
}
#endif
#endif