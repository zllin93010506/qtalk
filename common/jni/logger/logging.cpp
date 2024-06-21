/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Logger implement, support writing into stdout and send for java logback
 */

#include "logging.h"


static JavaVM *		__logback_jvm = NULL;
static jclass		__logback_jclass_logger;
static jmethodID	__logback_jmethod_logWrite;
static jmethodID	__logback_jmethod_logv;
static jmethodID	__logback_jmethod_logd;
static jmethodID	__logback_jmethod_logi;
static jmethodID	__logback_jmethod_logw;
static jmethodID	__logback_jmethod_loge;

void
__stdout_log_print(const int level, const char * format, ...)
{
    va_list	args;
    static char tags[] = {'?', '?', 'V', 'D', 'I', 'W', 'E', 'F', '?'};

    va_start(args, format);

    printf("[%c]%s ", tags[level], LOG_TAG);
    vprintf(format, args);
    printf("\n");

    va_end(args);
}

void
__stdout_log_vprint(const int level, const char * format, va_list ap)
{
    static char tags[] = {'?', '?', 'V', 'D', 'I', 'W', 'E', 'F', '?'};
    printf("[%c]%s ", tags[level], LOG_TAG);
    vprintf(format, ap);
    printf("\n");
}

void
__stdout_log_write(const int level, const char * msg)
{
    static char tags[] = {'?', '?', 'V', 'D', 'I', 'W', 'E', 'F', '?'};
    printf("[%c]%s %s\n", tags[level], LOG_TAG, msg);
}



void
logback_init(JavaVM * jVM, JNIEnv * env)
{
    if(__logback_jvm != NULL)
	return;
    __logback_jvm = jVM;
    jclass jclass_logger = env->FindClass("com/quanta/qtalk/util/Log");
    if (! jclass_logger) {
	__android_log_write(LOG_LEVEL_ERROR, LOG_TAG, "logback_init failed to get logger class reference");
	return;
    }
    __logback_jclass_logger = (jclass) env->NewGlobalRef(jclass_logger);

    __logback_jmethod_logv = env->GetStaticMethodID(__logback_jclass_logger, "l", "(Ljava/lang/String;Ljava/lang/String;)V");
    if (! __logback_jmethod_logv) {
	__android_log_write(LOG_LEVEL_ERROR, LOG_TAG, "logback_init failed to get l method ID");
	return;
    }
    __logback_jmethod_logd = env->GetStaticMethodID(__logback_jclass_logger, "d", "(Ljava/lang/String;Ljava/lang/String;)V");
    if (! __logback_jmethod_logd) {
	__android_log_write(LOG_LEVEL_ERROR, LOG_TAG, "logback_init failed to get d method ID");
	return;
    }
    __logback_jmethod_logi = env->GetStaticMethodID(__logback_jclass_logger, "i", "(Ljava/lang/String;Ljava/lang/String;)V");
    if (! __logback_jmethod_logi) {
	__android_log_write(LOG_LEVEL_ERROR, LOG_TAG, "logback_init failed to get i method ID");
	return;
    }
    __logback_jmethod_logw = env->GetStaticMethodID(__logback_jclass_logger, "w", "(Ljava/lang/String;Ljava/lang/String;)V");
    if (! __logback_jmethod_logw) {
	__android_log_write(LOG_LEVEL_ERROR, LOG_TAG, "logback_init failed to get w method ID");
	return;
    }
    __logback_jmethod_loge = env->GetStaticMethodID(__logback_jclass_logger, "e", "(Ljava/lang/String;Ljava/lang/String;)V");
    if (! __logback_jmethod_loge) {
	__android_log_write(LOG_LEVEL_ERROR, LOG_TAG, "logback_init failed to get e method ID");
	return;
    }
}

void
logback_uninit(JNIEnv * env)
{
    if(__logback_jclass_logger != NULL) {
        env->DeleteGlobalRef(__logback_jclass_logger);
        __logback_jclass_logger = NULL;
	__logback_jvm = NULL;
    }
}

void
__logback_print(const int level, const char * tag, const char * format, ...)
{
    JNIEnv *	env = NULL;
    jint	err = JNI_OK;
    static char	buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    if (! __logback_jvm) {
	__android_log_write(level, LOG_TAG, buffer);
	va_end(args);
	return;
    }
    if (JNI_EDETACHED == (err = __logback_jvm->GetEnv((void**) &env, JNI_VERSION_1_4))) {
        if (__logback_jvm->AttachCurrentThread(&env, NULL) < 0) {
            __android_log_write(LOG_LEVEL_WARN, LOG_TAG, "__logback_write failed to get env neither attach current thread");
        }
    }

    

    
    jstring jmsg = env->NewStringUTF(buffer);
    jstring jtag = env->NewStringUTF(tag);
    switch(level) {
	case ANDROID_LOG_DEBUG:
	    env->CallStaticVoidMethod(__logback_jclass_logger, __logback_jmethod_logd, jtag, jmsg);
	    break;
	case ANDROID_LOG_INFO:
	    env->CallStaticVoidMethod(__logback_jclass_logger, __logback_jmethod_logi, jtag, jmsg);
	    break;
	case ANDROID_LOG_WARN:
	    env->CallStaticVoidMethod(__logback_jclass_logger, __logback_jmethod_logw, jtag, jmsg);
	    break;
	case ANDROID_LOG_ERROR:
	    env->CallStaticVoidMethod(__logback_jclass_logger, __logback_jmethod_loge, jtag, jmsg);
	    break;
	default:
	    env->CallStaticVoidMethod(__logback_jclass_logger, __logback_jmethod_logv, jtag, jmsg);
	    break;
	
    }
    env->DeleteLocalRef(jmsg);
    env->DeleteLocalRef(jtag);
    va_end(args);

    if (JNI_EDETACHED == err) __logback_jvm->DetachCurrentThread();
}

void
__logback_vprint(const int level, const char * format, va_list ap)
{
    JNIEnv *	env = NULL;
    jint	err = JNI_OK;
    if (! __logback_jvm) return;
    if (JNI_EDETACHED == (err = __logback_jvm->GetEnv((void**) &env, JNI_VERSION_1_4))) {
	if (__logback_jvm->AttachCurrentThread(&env, NULL) < 0) {
	    __android_log_write(LOG_LEVEL_WARN, LOG_TAG, "__logback_write failed to get env neither attach current thread");
	}
    }

    static char	buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, ap);
    jstring jmsg = env->NewStringUTF(buffer);
    env->CallStaticVoidMethod(__logback_jclass_logger, __logback_jmethod_logWrite, level, jmsg);
    env->DeleteLocalRef(jmsg);

    if (JNI_EDETACHED == err) __logback_jvm->DetachCurrentThread();
}

void
__logback_write(const int level, const char * tag, const char * msg)
{
    JNIEnv *	env = NULL;
    jint	err = JNI_OK;
    if (! __logback_jvm) return;
    if (JNI_EDETACHED == (err = __logback_jvm->GetEnv((void**) &env, JNI_VERSION_1_4))) {
	if (__logback_jvm->AttachCurrentThread(&env, NULL) < 0) {
	    __android_log_write(LOG_LEVEL_WARN, tag, "__logback_write failed to get env neither attach current thread");
	}
    }

    jstring jmsg = env->NewStringUTF(msg);
    env->CallStaticVoidMethod(__logback_jclass_logger, __logback_jmethod_logWrite, level, jmsg);
    env->DeleteLocalRef(jmsg);

    if (JNI_EDETACHED == err) __logback_jvm->DetachCurrentThread();
}

// vim:ts=8:sw=4:
