/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_quanta_qtalk_media_Yuv2RGBTransform */

#ifndef _Included_com_quanta_qtalk_media_Yuv2RGBTransform
#define _Included_com_quanta_qtalk_media_Yuv2RGBTransform
#ifdef __cplusplus
extern "C" {
#endif
#undef com_quanta_qtalk_media_Yuv2RGBTransform_mUseNative
#define com_quanta_qtalk_media_Yuv2RGBTransform_mUseNative 1L
/*
 * Class:     com_quanta_qtalk_media_Yuv2RGBTransform
 * Method:    _Start
 * Signature: (Lcom/quanta/qtalk/media/ISink;II)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_Yuv2RGBTransform__1Start
  (JNIEnv *, jclass, jobject, jint, jint);

/*
 * Class:     com_quanta_qtalk_media_Yuv2RGBTransform
 * Method:    _Stop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_Yuv2RGBTransform__1Stop
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_quanta_qtalk_media_Yuv2RGBTransform
 * Method:    _OnMedia
 * Signature: (ILjava/nio/ByteBuffer;IJ)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_Yuv2RGBTransform__1OnMedia
  (JNIEnv *, jclass, jint, jobject, jint, jlong);

#ifdef __cplusplus
}
#endif
#endif
