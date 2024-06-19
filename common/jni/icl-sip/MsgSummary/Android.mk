LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
msgcount.c \
msgsummary.c \

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../common \
	$(LOCAL_PATH)/../adt \
	$(LOCAL_PATH)/../sip \
	$(LOCAL_PATH)/../sipTx \
	$(LOCAL_PATH)/../sdp \
	$(LOCAL_PATH)/../DlgLayer \

LOCAL_CFLAGS += -DUNIX
LOCAL_CFLAGS += -DPX2 -DUNIX -DICL_NO_MATCH_NOTIFY \
					-Wcast-align -fno-short-enums \
					-lc

LOCAL_SHARED_LIBRARIES := libcm libadt libsipccl libsdpccl libsipTx libDlgLayer \

LOCAL_MODULE:= MsgSummary

include $(BUILD_SHARED_LIBRARY)



