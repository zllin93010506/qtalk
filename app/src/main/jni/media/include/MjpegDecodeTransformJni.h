/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_quanta_qtalk_media_video_MjpegEncodeTransform */

#ifndef _Included_com_quanta_qtalk_media_video_MjpegEncodeTransform
#define _Included_com_quanta_qtalk_media_video_MjpegEncodeTransform
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_quanta_qtalk_media_video_MjpegEncodeTransform
 * Method:    _Start
 * Signature: (Lcom/quanta/qtalk/media/ISink;III)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_video_MjpegEncodeTransform__1Start
  (JNIEnv *, jclass, jobject, jint, jint, jint);

/*
 * Class:     com_quanta_qtalk_media_video_MjpegEncodeTransform
 * Method:    _Stop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_MjpegEncodeTransform__1Stop
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_quanta_qtalk_media_video_MjpegEncodeTransform
 * Method:    _OnMedia
 * Signature: (ILjava/nio/ByteBuffer;IJS)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_MjpegEncodeTransform__1OnMedia
  (JNIEnv *, jclass, jint, jobject, jint, jlong, jshort);

#ifdef __cplusplus
}
#endif
#endif