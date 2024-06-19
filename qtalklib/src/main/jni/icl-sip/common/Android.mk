LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)


LOCAL_SRC_FILES := \
	../low/cx_event.c \
	../low/cx_misc.c \
	../low/cx_mutex.c \
	../low/cx_sock.c \
	../low/cx_thrd.c \
	../low/cx_timer.c \
	cm_trace.c \
	cm_utl.c 

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../ \

# the assembler is only for the ARM version, don't break the Linux sim

# temp fix until we understand why this broke cnn.com
#ANDROID_JPEG_NO_ASSEMBLER := true


LOCAL_CFLAGS += -DUNIX
LOCAL_CFLAGS += -DPX2 -DUNIX -DICL_NO_MATCH_NOTIFY \
					-Wcast-align -fno-short-enums \

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -lc
LOCAL_MODULE:= cm

include $(BUILD_SHARED_LIBRARY)

