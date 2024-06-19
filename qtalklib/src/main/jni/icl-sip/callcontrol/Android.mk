LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
callctrl_call.c \
callctrl_Cont.c \
callctrl_msg.c \
callctrl_sipmsg.c \
callctrl_user.c \

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../common \
	$(LOCAL_PATH)/../adt \
	$(LOCAL_PATH)/../sip \
	$(LOCAL_PATH)/../sipTx \
	$(LOCAL_PATH)/../DlgLayer \

LOCAL_CFLAGS += -DUNIX
LOCAL_CFLAGS += -DPX2 -DUNIX -DICL_NO_MATCH_NOTIFY -DICL_TLS \
		 -Wcast-align -fno-short-enums \
		 -lc

LOCAL_SHARED_LIBRARIES := libcm libadt libsipccl libsipTx libDlgLayer \

LOCAL_MODULE:= callcontrol

include $(BUILD_SHARED_LIBRARY)



