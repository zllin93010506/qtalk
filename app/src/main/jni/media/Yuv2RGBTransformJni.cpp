
#include <stdlib.h>
#include <include/Yuv2RGBTransformJni.h>
#include "logging.h"

#define min(x1,x2) ((x1) > (x2) ? (x2):(x1))
#define max(x1,x2) ((x1) > (x2) ? (x1):(x2))

#define TAG "Yuv2RGBTransformJNI"
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
	jint			width;
	jint			height;
	unsigned char*	array_output;
	unsigned long   size_output;
	jobject			buffer_output;

	jmethodID 		onMedia;
	jobject 		jISink;		//Recv Listener
}NativeParams;

static inline void yuv2rgb(const int width,const int height,const unsigned char *in,unsigned char *out)
{
    int sz = width * height;
    int i, j;
    int Y, Cr = 0, Cb = 0;
	for(j = 0; j < height; j++)
	{
		 int pixPtr = j * width;
		 int jDiv2 = j >> 1;
		 for(i = 0; i < width; i++)
		 {
			 Y = in[pixPtr];
			 if(Y < 0) Y += 255;
			 if((i & 0x1) != 1)
			 {
				 int cOff = sz + jDiv2 * width + (i >> 1) * 2;
				 Cb = in[cOff];
				 if(Cb < 0) Cb += 127; else Cb -= 128;
				 Cr = in[cOff + 1];
				 if(Cr < 0) Cr += 127; else Cr -= 128;
			 }
			 int R = Y + Cr + (Cr >> 2) + (Cr >> 3) + (Cr >> 5);
			 if(R < 0) R = 0; else if(R > 255) R = 255;
			 int G = Y - (Cb >> 2) + (Cb >> 4) + (Cb >> 5) - (Cr >> 1) + (Cr >> 3) + (Cr >> 4) + (Cr >> 5);
			 if(G < 0) G = 0; else if(G > 255) G = 255;
			 int B = Y + Cb + (Cb >> 1) + (Cb >> 2) + (Cb >> 6);
			 if(B < 0) B = 0; else if(B > 255) B = 255;
			 out[pixPtr++] = 0xff000000 + (B << 16) + (G << 8) + R;
		 }
	}
}

//  refer to http://download-llnw.oracle.com/javase/1.5.0/docs/guide/jni/spec/types.html#wp276
//  The jvalue union type is used as the element type in argument arrays. It is declared as follows:
//	typedef union jvalue {
//		jboolean z;
//		jbyte    b;
//		jchar    c;
//		jshort   s;
//		jint     i;
//		jlong    j;
//		jfloat   f;
//		jdouble  d;
//		jobject  l;
//	} jvalue;
//	Table 3-2 Java VM Type Signatures
//	Type Signature
//	Java Type
//	Z							boolean
//	B							byte
//	C							char
//	S							short
//	I							int
//	J							long
//	F							float
//	D							double
//	L fully-qualified-class ;	fully-qualified-class
//	[ type						type[]
//	( arg-types ) ret-type		method type
//	For example, the Java method:
//
//	long f (int n, String s, int[] arr);
//	has the following type signature:
//
//	(ILjava/lang/String;[I)J

/*
 * Class:     com_quanta_qtalk_media_Yuv2RGBTransform
 * Method:    _Start
 * Signature: (Lcom/quanta/qtalk/media/ISink;)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_media_Yuv2RGBTransform__1Start
(JNIEnv * env, jclass jclass_caller, jobject  jISink, jint width, jint height)
{
	NativeParams* result = (NativeParams*) malloc(sizeof(NativeParams));
	jobject buffer_output = NULL;
	LOGD("==>Yuv2RGBTransform__1Start");
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
	#define METHOD_NAME "onMedia"
	#define METHOD_SIG  "(Ljava/nio/ByteBuffer;IJ)V"
	result->onMedia = env->GetMethodID(env->GetObjectClass(jISink),	//Class
									   METHOD_NAME,					//Method
									   METHOD_SIG); 				//Signature
	if (result->onMedia==0)
	{
		LOGD("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
		goto ERROR;
	}
	result->width = width;
	result->height = height;
	result->size_output = width*height*sizeof(int);
	result->array_output = (unsigned char*)malloc(result->size_output);
	buffer_output = env->NewDirectByteBuffer( (void*)result->array_output, result->size_output);
	if (buffer_output==0)
	{
		LOGD("Unable to NewDirectByteBuffer");
		goto ERROR;
	}
	result->buffer_output = env->NewGlobalRef(buffer_output);
	LOGD("<==Yuv2RGBTransform__1Start");
	return (jint)result;
ERROR:
	if (result != NULL)
	{
		free(result);
	}
	return 0;
}

/*
 * Class:     com_quanta_qtalk_media_Yuv2RGBTransform
 * Method:    _Stop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_Yuv2RGBTransform__1Stop
  (JNIEnv * env, jclass jclass_caller, jint params)
{
	NativeParams* handle = (NativeParams*)params;
	LOGD("==>Yuv2RGBTransform__1Stop");
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
		if (handle->jISink!=0)
		{
			env->DeleteGlobalRef(handle->jISink);
			handle->jISink = 0;
		}
		free(handle);
	}
	LOGD("<==Yuv2RGBTransform__1Stop");
}

/*
 * Class:     com_quanta_qtalk_media_Yuv2RGBTransform
 * Method:    _OnMedia
 * Signature: (ILjava/nio/ByteBuffer;IJ)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_media_Yuv2RGBTransform__1OnMedia
  (JNIEnv * env, jclass jclass_caller, jint params, jobject jobject_input, jint length, jlong timestamp)
{
	NativeParams* handle = (NativeParams*)params;
	//LOGD("==>Yuv2RGBTransform__1OnMedia");
	if (handle != NULL && jobject_input != NULL)
	{
		//get directly access to the input bytebuffer
		jobject buffer_input = env->NewGlobalRef(jobject_input);

		long size_input = env->GetDirectBufferCapacity(buffer_input);
		if (size_input>0 )
		{
			unsigned char *array_input = (unsigned char*) env->GetDirectBufferAddress(buffer_input);
			//decode
			//LOGD("yuv2rgb:%dX%d in:%d out:%d ",handle->width,handle->height,size_input,handle->size_output);

			yuv2rgb(handle->width,handle->height,array_input,handle->array_output);//array_output
			//callback to the Isink
			//LOGD("CallVoidMethod %x %x",handle->jISink,handle->onMedia);
			env->CallVoidMethod(handle->jISink,
								handle->onMedia,
								handle->buffer_output,
								timestamp);
		}else
			LOGE("OnMedia input size = %d array_output size=%d",size_input,handle->size_output);

		env->DeleteGlobalRef(buffer_input);
	}
	//LOGD("<==Yuv2RGBTransform__1OnMedia");
}

#ifdef __cplusplus
}
#endif
