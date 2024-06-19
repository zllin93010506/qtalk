LOCAL_PATH := $(call my-dir)
X264_GEN := /home/lawrence/workspace/FFMPEG_TEST/jni/ffmpeg/x264_link
X264_PATH :=  /home/lawrence/workspace/FFMPEG_TEST/jni/ffmpeg/x264
LIB_INC := $(X264_GEN)/include
#LIB_LIB := $(X264_GEN)/lib

include $(CLEAR_VARS)
#include $(LOCAL_PATH)/../av.mk
LOCAL_SRC_FILES := $(call all-subdir-c-files)
LOCAL_C_INCLUDES :=		\
	$(LOCAL_PATH)		\
	$(LOCAL_PATH)/..	\
LOCAL_CFLAGS += $(FFCFLAGS)
LOCAL_CFLAGS += $(LOCAL_C_INCLUDES:%=-I%)

#LOCAL_LDLIBS := -lz -lx264 -L$(LIB_LIB)
#LOCAL_STATIC_LIBRARIES +:= $(FFLIBS) \
#LOCAL_STATIC_LIBRARIES +:= $(X264_PATH)
LOCAL_MODULE := libx264
include $(BUILD_STATIC_LIBRARY)
