LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
dx_buf.c \
dx_hash.c \
dx_lst.c \
dx_msgq.c \
dx_str.c \
dx_vec.c \

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../common \

# the assembler is only for the ARM version, don't break the Linux sim

# temp fix until we understand why this broke cnn.com
#ANDROID_JPEG_NO_ASSEMBLER := true


LOCAL_CFLAGS += -DUNIX
LOCAL_CFLAGS += -DPX2 -DUNIX -DICL_NO_MATCH_NOTIFY \
					-Wcast-align -fno-short-enums \
					-lc

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog
LOCAL_SHARED_LIBRARIES := libcm \

LOCAL_MODULE:= adt

include $(BUILD_SHARED_LIBRARY)



