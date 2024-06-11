/*
 */
#include <jni.h>
#include "com_example_hellojni_HelloJni.h"
#include <string.h>
//#include <jni.h>
#include <stdio.h>

#include "logging.h"
#define LOG_TAG "kakasi"
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , "LOG_DEBUG",__VA_ARGS__)


jstring
Java_com_example_hellojni_HelloJni_stringFromJNI( JNIEnv* env,
                                                  jobject thiz )
{
    return (*env)->NewStringUTF(env, "Hello from JNI ! QQ orz 1");
}
JNIEXPORT void JNICALL Java_com_example_hellojni_HelloJni_Test(JNIEnv *env, jobject thiz)
{
	LOGD("QQ i cannot use NDK LOGD");
}

JNIEXPORT jbyte JNICALL Java_com_example_hellojni_HelloJni_PassPointer(JNIEnv *env, jobject thiz, jobject remote_data, jbyteArray record_data, jobject three_small)
{
	//jbyte* c_remote_data = (jbyte*)(*env)->GetDirectBufferAddress(env, remote_datakakasi);
	//jbyte* c_record_data = (jbyte*)(*env)->GetDirectBufferAddress(env, record_data);
	long a = (*env)->GetDirectBufferCapacity(env,remote_data);
	long b = (*env)->GetDirectBufferCapacity(env,record_data);
	long c = (*env)->GetDirectBufferCapacity(env,three_small);
	jbyte* jbyter = (*env)->GetByteArrayElements(env, record_data, NULL);
	int co;
	LOGD("size_1:%s", jbyter);
	//sprintf( jbyter," kakasi kakasi\n");
	return jbyter;
}
