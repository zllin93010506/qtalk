LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	cx_event.c \
	cx_misc.c \
	cx_mutex.c \
	cx_sock.c \
	cx_thrd.c \
	cx_timer.c \

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../common \

LOCAL_CFLAGS += -DUNIX
LOCAL_CFLAGS += -DPX2 -DUNIX -DICL_NO_MATCH_NOTIFY \
					-Wcast-align -fno-short-enums \
                    -lc

LOCAL_SHARED_LIBRARIES := libcm \

LOCAL_MODULE:= low

include $(BUILD_SHARED_LIBRARY)



