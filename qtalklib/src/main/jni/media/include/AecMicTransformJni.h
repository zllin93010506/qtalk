/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_quanta_qtalk_media_audio_AecMicTransform */

#ifndef _Included_com_quanta_qtalk_media_audio_AecMicTransform
#define _Included_com_quanta_qtalk_media_audio_AecMicTransform
#ifdef __cplusplus
extern "C" {
#endif
#undef com_quanta_qtalk_media_audio_AecMicTransform_mInFormat
#define com_quanta_qtalk_media_audio_AecMicTransform_mInFormat 255L
#undef com_quanta_qtalk_media_audio_AecMicTransform_mOutFormat
#define com_quanta_qtalk_media_audio_AecMicTransform_mOutFormat 255L
/*
 * Class:     com_quanta_qtalk_media_audio_AecMicTransform
 * Method:    _Start
 * Signature: (Lcom/quanta/qtalk/media/audio/IAudioSink;II)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_audio_AecMicTransform__1Start
  (JNIEnv *, jclass, jobject, jint, jint);

/*
 * Class:     com_quanta_qtalk_media_audio_AecMicTransform
 * Method:    _Stop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_AecMicTransform__1Stop
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_quanta_qtalk_media_audio_AecMicTransform
 * Method:    _OnMedia
 * Signature: (I[SIJ)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_audio_AecMicTransform__1OnMedia
  (JNIEnv *, jclass, jint, jshortArray, jint, jlong);

#ifdef __cplusplus
}
#endif
#endif
