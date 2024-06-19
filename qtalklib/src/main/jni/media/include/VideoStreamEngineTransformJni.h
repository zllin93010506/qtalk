/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_quanta_qtalk_media_video_VideoStreamEngineTransform */

#ifndef _Included_com_quanta_qtalk_media_video_VideoStreamEngineTransform
#define _Included_com_quanta_qtalk_media_video_VideoStreamEngineTransform
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_quanta_qtalk_media_video_VideoStreamEngineTransform
 * Method:    _UpdateSource
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_VideoStreamEngineTransform__1UpdateSource
  (JNIEnv *, jclass, jint, jint, jint);

/*
 * Class:     com_quanta_qtalk_media_video_VideoStreamEngineTransform
 * Method:    _SetReportCB
 * Signature: (ILcom/quanta/qtalk/media/video/IVideoStreamReportCB;)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_VideoStreamEngineTransform__1SetReportCB
  (JNIEnv *, jclass, jint, jobject);

/*
 * Class:     com_quanta_qtalk_media_video_VideoStreamEngineTransform
 * Method:    _SetSourceCB
 * Signature: (ILcom/quanta/qtalk/media/video/IVideoStreamSourceCB;)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_VideoStreamEngineTransform__1SetSourceCB
  (JNIEnv *, jclass, jint, jobject);

/*
 * Class:     com_quanta_qtalk_media_video_VideoStreamEngineTransform
 * Method:    _SetDemandIDRCB
 * Signature: (ILcom/quanta/qtalk/media/video/IDemandIDRCB;)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_VideoStreamEngineTransform__1SetDemandIDRCB
  (JNIEnv *, jclass, jint, jobject);

/*
 * Class:     com_quanta_qtalk_media_video_VideoStreamEngineTransform
 * Method:    _Start
 * Signature: (JLcom/quanta/qtalk/media/video/IVideoSink;IZLjava/lang/String;IIIILjava/lang/String;IIZZZIIIZLjava/lang/String;Ljava/lang/String;IIIII)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_video_VideoStreamEngineTransform__1Start
  (JNIEnv *, jclass, jlong, jobject, jint, jboolean, jstring, jint, jint, jint, jint, jstring, jint, jint, jboolean, jboolean, jboolean, jint, jint, jint, jboolean, jstring, jstring, jint, jint, jint, jint, jint);

/*
 * Class:     com_quanta_qtalk_media_video_VideoStreamEngineTransform
 * Method:    _Stop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_VideoStreamEngineTransform__1Stop
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_quanta_qtalk_media_video_VideoStreamEngineTransform
 * Method:    _GetSSRC
 * Signature: (I)J
 */
JNIEXPORT jlong JNICALL Java_com_quanta_qtalk_media_video_VideoStreamEngineTransform__1GetSSRC
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_quanta_qtalk_media_video_VideoStreamEngineTransform
 * Method:    _OnSendDirect
 * Signature: (ILjava/nio/ByteBuffer;IJS)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_VideoStreamEngineTransform__1OnSendDirect
  (JNIEnv *, jclass, jint, jobject, jint, jlong, jshort);

/*
 * Class:     com_quanta_qtalk_media_video_VideoStreamEngineTransform
 * Method:    _OnSend
 * Signature: (I[BIJS)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_VideoStreamEngineTransform__1OnSend
  (JNIEnv *, jclass, jint, jbyteArray, jint, jlong, jshort);

/*
 * Class:     com_quanta_qtalk_media_video_VideoStreamEngineTransform
 * Method:    _SetHold
 * Signature: (IZ)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_VideoStreamEngineTransform__1SetHold
  (JNIEnv *, jclass, jint, jboolean);

#ifdef __cplusplus
}
#endif
#endif
