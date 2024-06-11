#include <string.h>
#include <stdlib.h>
#include <include/X264TransformJni.h>
#include "logging.h"
#include "X264Encoder.h"
#include <sys/time.h>
//#define COUNT_FRAME
#ifdef COUNT_FRAME
#include <pthread.h>
#include "media/qstream/video/H264Parser.h"
#endif

extern "C"
{
    #include "x264.h"
}

#define TAG "X264TransformJni"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

#ifndef max
#define max(a,b) ({typeof(a) _a = (a); typeof(b) _b = (b); _a > _b ? _a : _b; })
#define min(a,b) ({typeof(a) _a = (a); typeof(b) _b = (b); _a < _b ? _a : _b; })
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NativeParams
{
    JavaVM          *jvm;
    jint            pixFormat;
    jint            width;
    jint            height;
    unsigned char*  array_output;
    unsigned long   size_output;
    jobject         buffer_output;
    CX264Encoder    *mX264Encoder;
    jmethodID       onMediaStatic;
    jobject         jISink;        //Recv Listener
    unsigned char   *yuv_array;
#ifdef COUNT_FRAME
    CQStreamStatistics *pStatistics;
#endif
}NativeParams;

/*
   YUV 4:2:0 image with a plane of 8 bit Y samples followed by an interleaved
   U/V plane containing 8 bit 2x2 subsampled chroma samples.
   except the interleave order of U and V is reversed.

                        H V
   Y Sample Period      1 1
   U (Cb) Sample Period 2 2
   V (Cr) Sample Period 2 2
 */

static inline void pixel_convert_NV21toyuv420(
    unsigned char *pY, unsigned char *buffer,int width, int height)
{
    int size = height*width;
    int size2 = size*5/4;
    unsigned char *pUV = pY + size;
    unsigned char *out = buffer;

    memcpy(buffer ,pY      ,size); //Y
    for(int i = 0; i<size/4; i++)
    {
        out[i+size2] = pUV[2*i];   // U
        out[i+size]  = pUV[2*i+1]; // V
    }
}
static inline void pixel_convert_NV21toNV12(
    unsigned char *pY, unsigned char *buffer,int width, int height)
{
    int size = height*width;
    unsigned char *pUV = pY + size;
    unsigned char *out = buffer;

    memcpy(buffer ,pY      ,size); //Y
    for(int i = 0; i<size/4; i++)
    {
        out[size+2*i+1] = pUV[2*i];   // U
        out[size+2*i]   = pUV[2*i+1]; // V
    }
}
/*
 * Class:     com_quanta_qtalk_media_video_X264Transform
 * Method:    _Start
 * Signature: (Lcom/quanta/qtalk/media/ISink;II)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_video_X264Transform__1Start
(JNIEnv * env, jclass jclass_caller, jobject  jISink, jint pixFormat, jint width, jint height)
{
    NativeParams* result = (NativeParams*) malloc(sizeof(NativeParams));
    jobject buffer_output = NULL;
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

    result->jISink = env->NewGlobalRef(jISink);
    if (result->jISink==0)
    {
        LOGD("Unable to get jISink");
        goto ERROR;
    }
    //Get the method for callback after on media process success
    #define METHOD_NAME "onMediaNative"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/media/video/IVideoSink;Ljava/nio/ByteBuffer;IJS)V"
    result->onMediaStatic = env->GetStaticMethodID(jclass_caller,    //Class
                               METHOD_NAME,                            //Method
                               METHOD_SIG);                             //Signature
    if (result->onMediaStatic==0)
    {
        LOGD("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
        goto ERROR;
    }
    result->pixFormat = pixFormat;
    result->width = width;
    result->height = height;
    result->size_output = width*height;
    result->array_output = (unsigned char*)malloc(result->size_output);

    result->yuv_array = (unsigned char*)malloc((int)height*(int)width*1.5);

    buffer_output = env->NewDirectByteBuffer( (void*)result->array_output, result->size_output);
    if (buffer_output==0)
    {
        LOGD("Unable to NewDirectByteBuffer");
        goto ERROR;
    }
//    PixelFormat source_pix_format;
//    switch(pixFormat)
//    {
//    case 16:
//        source_pix_format = PIX_FMT_NV21; //PIX_FMT_YUYV422 PIX_FMT_YUVJ422P PIX_FMT_YUV422P PIX_FMT_UYVY422
//        break;
//    case 17:
//        source_pix_format = PIX_FMT_NV21;
//        break;
//    }
    result->buffer_output = env->NewGlobalRef(buffer_output);
    //TODO: Initial H264 Encoder HERE
    result->mX264Encoder = new CX264Encoder();
    result->mX264Encoder->InitX264Codec(width,height,10,25,3,0,X264_CSP_I420,true); //X264_CSP_NV12  X264_CSP_I420
#ifdef COUNT_FRAME
    result->pStatistics = new CQStreamStatistics();
#endif
    LOGD("<==_Start");
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
 * Class:     com_quanta_qtalk_media_video_X264Transform
 * Method:    _Stop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_X264Transform__1Stop
  (JNIEnv * env, jclass jclass_caller, jint params)
{
    NativeParams* handle = (NativeParams*)params;
    LOGD("==>_Stop");
    if (handle != NULL)
    {
        //Destroy H264 Encoder HERE
        if(handle->mX264Encoder != NULL)
        {
            handle->mX264Encoder->ReleaseX264Codec();
            handle->mX264Encoder = NULL;
        }

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
        if (handle->jISink!=0)
        {
            env->DeleteGlobalRef(handle->jISink);
            handle->jISink = 0;
        }
        if(handle->yuv_array != NULL)
        {
            free(handle->yuv_array);
            handle->yuv_array = NULL;
        }
#ifdef COUNT_FRAME
        if (handle->pStatistics)
        {
            delete handle->pStatistics;
            handle->pStatistics = NULL;
        }
#endif
        free(handle);
    }
    LOGD("<==_Stop");
}
/*
 * Class:     com_quanta_qtalk_media_video_X264Transform
 * Method:    _OnMedia
 * Signature: (ILjava/nio/ByteBuffer;J)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_X264Transform__1OnMedia
  (JNIEnv * env, jclass jclass_caller, jint params, jbyteArray jbyteArray_input, jint length, jlong timestamp, jshort rotation)
{
    NativeParams* handle = (NativeParams*)params;

    if (handle != NULL && jbyteArray_input != NULL)
    {
        //get directly access to the input bytebuffer
        unsigned char* array_input = (unsigned char*)env->GetByteArrayElements(jbyteArray_input, 0);
#if X264_BUILD > 103
        if(handle->mX264Encoder->m_colorSpace == X264_CSP_NV12)
            pixel_convert_NV21toNV12(array_input, handle->yuv_array, handle->width , handle->height);
        else if(handle->mX264Encoder->m_colorSpace == X264_CSP_I420)
#endif
            pixel_convert_NV21toyuv420(array_input, handle->yuv_array, handle->width , handle->height);

        env->ReleaseByteArrayElements(jbyteArray_input,(jbyte*)array_input, 0);
        bool isI = true;
        int length_output = handle->mX264Encoder->EncodeX264( handle->yuv_array, (int)length ,handle->array_output,isI,X264_TYPE_AUTO);//
#ifdef COUNT_FRAME
        if (handle->pStatistics)
        {
            int frame_type = 0;
            int frame_num = 0;
            handle->pStatistics->OnSendFrameOK(length_output);
            //frame_num = H264GetFrameNumber(handle->array_output,length_output);
            frame_num = H264_GetFrameNumber(handle->array_output,length_output,&frame_type);
            LOGD("%dkbps, %dfps size %d  t:%d num:%d",handle->pStatistics->GetSendBitrate(),handle->pStatistics->GetSendFramerate(),length_output,frame_type,frame_num);//
        }
#endif
        struct timeval clock;

        memset(&clock,0,sizeof(clock));
        gettimeofday(&clock, 0);

        timestamp = clock.tv_sec * 90000 +
                                   (uint32_t)((double)clock.tv_usec * (double)90000 * 1.0e-6);
        //callback to the Isink
        env->CallStaticVoidMethod(jclass_caller,
                            handle->onMediaStatic,
                            handle->jISink,
                            handle->buffer_output,
                            length_output,
                            timestamp,
                            rotation);

    }
}

/*
 * Class:     com_quanta_qtalk_media_video_X264Transform
 * Method:    _OnMediaDirect
 * Signature: (ILjava/nio/ByteBuffer;J)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_X264Transform__1OnMediaDirect
  (JNIEnv * env, jclass jclass_caller, jint params, jobject jobject_input, jint length, jlong timestamp, jshort rotation)
{
    NativeParams* handle = (NativeParams*)params;

    if (handle != NULL && jobject_input != NULL)
    {
        //get directly access to the input bytebuffer
        jobject buffer_input = env->NewLocalRef(jobject_input);
        unsigned char* array_input = (unsigned char*) env->GetDirectBufferAddress(buffer_input);
#if X264_BUILD > 103
        if(handle->mX264Encoder->m_colorSpace == X264_CSP_NV12)
            pixel_convert_NV21toNV12(array_input, handle->yuv_array, handle->width , handle->height);
        else if(handle->mX264Encoder->m_colorSpace == X264_CSP_I420)
#endif
            pixel_convert_NV21toyuv420(array_input, handle->yuv_array, handle->width , handle->height);
        bool isI = true;
        int length_output = handle->mX264Encoder->EncodeX264( handle->yuv_array, (int)length ,handle->array_output,isI,X264_TYPE_AUTO);//
#ifdef COUNT_FRAME
        if (handle->pStatistics)
        {
            int frame_type = 0;
            int frame_num = 0;
            handle->pStatistics->OnSendFrameOK(length_output);
            //frame_num = H264GetFrameNumber(handle->array_output,length_output);
            frame_num = H264_GetFrameNumber(handle->array_output,length_output,&frame_type);
            LOGD("%dkbps, %dfps size %d  t:%d num:%d",handle->pStatistics->GetSendBitrate(),handle->pStatistics->GetSendFramerate(),length_output,frame_type,frame_num);//
        }
#endif
        struct timeval clock;

        memset(&clock,0,sizeof(clock));
        gettimeofday(&clock, 0);

        timestamp = clock.tv_sec * 90000 +
                                   (uint32_t)((double)clock.tv_usec * (double)90000 * 1.0e-6);
        //callback to the Isink
        env->CallStaticVoidMethod(jclass_caller,
                            handle->onMediaStatic,
                            handle->jISink,
                            handle->buffer_output,
                            length_output,
                            timestamp,
                            rotation);

        env->DeleteLocalRef(buffer_input);
    }
}

/*
 * Class:     com_quanta_qtalk_media_video_X264Transform
 * Method:    _SetGop
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_X264Transform__1SetGop
(JNIEnv * env, jclass jclass_caller, jint params, jint gop)
{
    NativeParams* handle = (NativeParams*)params;
    if (handle != NULL)
    {
        if(handle->mX264Encoder != NULL)
        {
            handle->mX264Encoder->SetX264GOP(gop);
        }
    }
}
/*
 * Class:     com_quanta_qtalk_media_video_X264Transform
 * Method:    _SetBitRate
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_video_X264Transform__1SetBitRate
(JNIEnv * env, jclass jclass_caller, jint params, jint kbps)
{
    NativeParams* handle = (NativeParams*)params;
    if (handle != NULL)
    {
        if(handle->mX264Encoder != NULL)
        {
            handle->mX264Encoder->SetX264Bitrate(kbps);
        }
    }
}

#ifdef __cplusplus
}
#endif
