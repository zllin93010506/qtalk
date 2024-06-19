LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#LOCAL_PRELINK_MODULE := false
#LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
ClientStateMachine.c \
CTransactionDatabase.c \
ServerStateMachine.c \
sipTx.c \
Transport.c \

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../common \
	$(LOCAL_PATH)/../adt \
	$(LOCAL_PATH)/../sip \

LOCAL_CFLAGS += -DUNIX
LOCAL_CFLAGS += -DPX2 -DUNIX -DICL_NO_MATCH_NOTIFY -DCCLSIP_ENABLE_TLS -DICL_TLS \
		-Wcast-align -fno-short-enums \
		-lc

LOCAL_SHARED_LIBRARIES := libcm libadt libsipccl libssl libcrypto

LOCAL_MODULE:= sipTx

include $(BUILD_SHARED_LIBRARY)



