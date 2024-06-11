LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
dlg_msg.c \
dlg_sipmsg.c \
dlg_util.c \

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../common \
	$(LOCAL_PATH)/../adt \
	$(LOCAL_PATH)/../sip \
	$(LOCAL_PATH)/../sipTx \


LOCAL_CFLAGS += -DUNIX
LOCAL_CFLAGS += -DPX2 -DUNIX -DICL_NO_MATCH_NOTIFY -DICL_TLS \
	         -Wcast-align -fno-short-enums \
		 -lc

LOCAL_SHARED_LIBRARIES := libcm libadt libsipccl libsipTx

LOCAL_MODULE:= DlgLayer

include $(BUILD_SHARED_LIBRARY)



