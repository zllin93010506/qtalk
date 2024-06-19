LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_STATIC_LIBRARIES := libavformat libavcodec libavutil libpostproc libswscale
LOCAL_MODULE := ffmpeg
include $(BUILD_SHARED_LIBRARY)
include $(call all-makefiles-under,$(LOCAL_PATH))

#LOCAL_PATH := $(call my-dir)
#AVCODEC := libavcodec
#AVUTIL := libavutil
#AVFORMAT := libavformat
#POSTPROC := libpostproc
#SWSCALE := libswscale
#include $(CLEAR_VARS)
#LOCAL_STATIC_LIBRARIES := libavformat libavcodec libavutil libpostproc libswscale
#LOCAL_MODULE := ffmpeg
#LOCAL_C_INCLUDES := \
#        $(LOCAL_PATH)	\
#		$(LOCAL_PATH)/$(AVCODEC) \
#        $(LOCAL_PATH)/$(AVUTIL) \
#        $(LOCAL_PATH)/$(AVFORMAT) \
#		$(LOCAL_PATH)/$(POSTPROC) \
#		$(LOCAL_PATH)/$(SWSCALE)
		
#LOCAL_CFLAGS += $(LOCAL_C_INCLUDES:%=-I%)
#include $(BUILD_SHARED_LIBRARY)
#include $(call all-makefiles-under,$(LOCAL_PATH))
