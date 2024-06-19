LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#LOCAL_PRELINK_MODULE := false
#LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
sdp_mses.c \
sdp_sess.c \
sdp_ta.c \
sdp_tb.c \
sdp_tc.c \
sdp_tk.c \
sdp_tm.c \
sdp_to.c \
sdp_tok.c \
sdp_tr.c \
sdp_tses.c \
sdp_tt.c \
sdp_tz.c \
sdp_utl.c \

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
LOCAL_SHARED_LIBRARIES := libcm libadt

LOCAL_MODULE:= sdpccl

include $(BUILD_SHARED_LIBRARY)



