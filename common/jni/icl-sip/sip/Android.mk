LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
base64.c \
md5c.c \
rfc822.c \
sip_cfg.c \
sip_hdr.c \
sip_int.c \
sip_req.c \
sip_rsp.c \
sip_tx.c \
sip_url.c \

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../common \
	$(LOCAL_PATH)/../adt \

LOCAL_CFLAGS += -DUNIX
LOCAL_CFLAGS += -DPX2 -DUNIX -DICL_NO_MATCH_NOTIFY -DICL_TLS \
		-Wcast-align -fno-short-enums \
		-lc

#LOCAL_CFLAGS += -march=armv6j
LOCAL_SHARED_LIBRARIES := libcm libadt libssl libcrypto

LOCAL_MODULE:= sipccl 

include $(BUILD_SHARED_LIBRARY)



