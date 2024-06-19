LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../av.mk
#Lawrence Added
#QRI_SRC = qri_ppo_qta15_ffmpeg_VideoProcess.c
#FFILES := $(filter-out qri_ppo_qta15_ffmpeg_VideoProcess.c,$(FFFILES))
#end


LOCAL_SRC_FILES := $(FFFILES)
LOCAL_C_INCLUDES :=		\
	$(LOCAL_PATH)		\
	$(LOCAL_PATH)/..
LOCAL_CFLAGS += $(FFCFLAGS)
LOCAL_STATIC_LIBRARIES := $(FFLIBS)
LOCAL_MODULE := $(FFNAME)
include $(BUILD_STATIC_LIBRARY)
