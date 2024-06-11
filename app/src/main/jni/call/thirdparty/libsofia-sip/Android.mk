LOCAL_PATH := $(call my-dir)
#TARGET_PLATFORM := 'android-8'
SOFIA_SIP := sofia-sip-1.12.11/libsofia-sip-ua

SOFIA_SIP_INCLUDES := $(LOCAL_PATH)/sofia-sip-1.12.11 \
					$(LOCAL_PATH)/$(SOFIA_SIP)/bnf \
					$(LOCAL_PATH)/$(SOFIA_SIP)/features \
					$(LOCAL_PATH)/$(SOFIA_SIP)/http \
					$(LOCAL_PATH)/$(SOFIA_SIP)/ipt \
					$(LOCAL_PATH)/$(SOFIA_SIP)/iptsec \
					$(LOCAL_PATH)/$(SOFIA_SIP)/msg \
					$(LOCAL_PATH)/$(SOFIA_SIP)/nea \
					$(LOCAL_PATH)/$(SOFIA_SIP)/nta \
					$(LOCAL_PATH)/$(SOFIA_SIP)/nth \
					$(LOCAL_PATH)/$(SOFIA_SIP)/nua \
					$(LOCAL_PATH)/$(SOFIA_SIP)/sdp \
					$(LOCAL_PATH)/$(SOFIA_SIP)/sip \
					$(LOCAL_PATH)/$(SOFIA_SIP)/soa \
					$(LOCAL_PATH)/$(SOFIA_SIP)/sresolv \
					$(LOCAL_PATH)/$(SOFIA_SIP)/stun \
					$(LOCAL_PATH)/$(SOFIA_SIP)/su \
					$(LOCAL_PATH)/$(SOFIA_SIP)/tport \
					$(LOCAL_PATH)/$(SOFIA_SIP)/url

SOFIA_SIP_SU_SRC := $(SOFIA_SIP)/su/addrinfo.c \
				$(SOFIA_SIP)/su/su_errno.c \
				$(SOFIA_SIP)/su/su_time0.c \
				$(SOFIA_SIP)/su/getopt.c \
				$(SOFIA_SIP)/su/su_global_log.c \
				$(SOFIA_SIP)/su/su_timer.c \
				$(SOFIA_SIP)/su/inet_ntop.c \
				$(SOFIA_SIP)/su/su_kqueue_port.c \
				$(SOFIA_SIP)/su/su_uniqueid.c \
				$(SOFIA_SIP)/su/inet_pton.c \
				$(SOFIA_SIP)/su/su_localinfo.c \
				$(SOFIA_SIP)/su/memcspn.c \
				$(SOFIA_SIP)/su/su_os_nw.c \
				$(SOFIA_SIP)/su/memmem.c \
				$(SOFIA_SIP)/su/memspn.c \
				$(SOFIA_SIP)/su/su_poll_port.c \
				$(SOFIA_SIP)/su/su_port.c \
				$(SOFIA_SIP)/su/smoothsort.c \
				$(SOFIA_SIP)/su/su_pthread_port.c \
				$(SOFIA_SIP)/su/string0.c \
				$(SOFIA_SIP)/su/su_root.c \
				$(SOFIA_SIP)/su/strtoull.c \
				$(SOFIA_SIP)/su/su_select_port.c \
				$(SOFIA_SIP)/su/su.c \
				$(SOFIA_SIP)/su/su_socket_port.c \
				$(SOFIA_SIP)/su/su_addrinfo.c \
				$(SOFIA_SIP)/su/su_sprintf.c \
				$(SOFIA_SIP)/su/su_alloc.c \
				$(SOFIA_SIP)/su/su_strdup.c \
				$(SOFIA_SIP)/su/su_alloc_lock.c \
				$(SOFIA_SIP)/su/su_strlst.c \
				$(SOFIA_SIP)/su/su_base_port.c \
				$(SOFIA_SIP)/su/su_tag.c \
				$(SOFIA_SIP)/su/su_bm.c \
				$(SOFIA_SIP)/su/su_tag_io.c \
				$(SOFIA_SIP)/su/su_default_log.c \
				$(SOFIA_SIP)/su/su_tag_ref.c \
				$(SOFIA_SIP)/su/su_devpoll_port.c \
				$(SOFIA_SIP)/su/su_taglist.c \
				$(SOFIA_SIP)/su/su_epoll_port.c \
				$(SOFIA_SIP)/su/su_time.c \
				$(SOFIA_SIP)/su/su_vector.c \
				$(SOFIA_SIP)/su/su_log.c \
				$(SOFIA_SIP)/su/su_wait.c \
				$(SOFIA_SIP)/su/memccpy.c \
				$(SOFIA_SIP)/su/su_md5.c \
				$(SOFIA_SIP)/su/su_string.c \
				$(SOFIA_SIP)/su/su_win32_port.c
				

SOFIA_SIP_FEATURES_SRC := $(SOFIA_SIP)/features/features.c

SOFIA_SIP_BNF_SRC := $(SOFIA_SIP)/bnf/bnf.c

SOFIA_SIP_RESOLV_SRC := $(SOFIA_SIP)/sresolv/sres.c \
				$(SOFIA_SIP)/sresolv/sres_cache.c \
				$(SOFIA_SIP)/sresolv/sres_blocking.c \
				$(SOFIA_SIP)/sresolv/sresolv.c
 
SOFIA_SIP_IPT_SRC := $(SOFIA_SIP)/ipt/base64.c \
				$(SOFIA_SIP)/ipt/token64.c

SOFIA_SIP_SDP_SRC := $(SOFIA_SIP)/sdp/sdp.c \
				$(SOFIA_SIP)/sdp/sdp_print.c \
				$(SOFIA_SIP)/sdp/sdp_tag_ref.c \
				$(SOFIA_SIP)/sdp/sdp_parse.c \
				$(SOFIA_SIP)/sdp/sdp_tag.c

SOFIA_SIP_URL_SRC := $(SOFIA_SIP)/url/url.c \
				$(SOFIA_SIP)/url/url_tag.c \
				$(SOFIA_SIP)/url/url_tag_ref.c \
				$(SOFIA_SIP)/url/urlmap.c

SOFIA_SIP_MSG_SRC := $(SOFIA_SIP)/msg/msg.c \
				$(SOFIA_SIP)/msg/msg_inlined.c \
				$(SOFIA_SIP)/msg/msg_tag.c \
				$(SOFIA_SIP)/msg/msg_auth.c \
				$(SOFIA_SIP)/msg/msg_mclass.c \
				$(SOFIA_SIP)/msg/msg_basic.c \
				$(SOFIA_SIP)/msg/msg_mime.c \
				$(SOFIA_SIP)/msg/msg_date.c \
				$(SOFIA_SIP)/msg/msg_mime_table.c \
				$(SOFIA_SIP)/msg/msg_generic.c \
				$(SOFIA_SIP)/msg/msg_header_copy.c \
				$(SOFIA_SIP)/msg/msg_parser.c \
				$(SOFIA_SIP)/msg/msg_header_make.c \
				$(SOFIA_SIP)/msg/msg_parser_util.c

SOFIA_SIP_SIP_SRC := $(SOFIA_SIP)/sip/sip_basic.c \
				$(SOFIA_SIP)/sip/sip_parser_table.c \
				$(SOFIA_SIP)/sip/sip_tag_class.c \
				$(SOFIA_SIP)/sip/sip_caller_prefs.c \
				$(SOFIA_SIP)/sip/sip_prack.c \
				$(SOFIA_SIP)/sip/sip_tag_ref.c \
				$(SOFIA_SIP)/sip/sip_event.c \
				$(SOFIA_SIP)/sip/sip_pref_util.c \
				$(SOFIA_SIP)/sip/sip_time.c \
				$(SOFIA_SIP)/sip/sip_extra.c \
				$(SOFIA_SIP)/sip/sip_reason.c \
				$(SOFIA_SIP)/sip/sip_util.c \
				$(SOFIA_SIP)/sip/sip_feature.c \
				$(SOFIA_SIP)/sip/sip_refer.c \
				$(SOFIA_SIP)/sip/sip_header.c \
				$(SOFIA_SIP)/sip/sip_security.c \
				$(SOFIA_SIP)/sip/sip_inlined.c \
				$(SOFIA_SIP)/sip/sip_session.c \
				$(SOFIA_SIP)/sip/sip_mime.c \
				$(SOFIA_SIP)/sip/sip_status.c \
				$(SOFIA_SIP)/sip/sip_parser.c \
				$(SOFIA_SIP)/sip/sip_tag.c

SOFIA_SIP_HTTP_SRC := $(SOFIA_SIP)/http/http_parser.c \
				$(SOFIA_SIP)/http/http_header.c \
				$(SOFIA_SIP)/http/http_basic.c \
				$(SOFIA_SIP)/http/http_extra.c \
				$(SOFIA_SIP)/http/http_inlined.c \
				$(SOFIA_SIP)/http/http_status.c \
				$(SOFIA_SIP)/http/http_tag_class.c \
				$(SOFIA_SIP)/http/http_tag.c \
				$(SOFIA_SIP)/http/http_parser_table.c \
				$(SOFIA_SIP)/http/http_tag_ref.c

SOFIA_SIP_STUN_SRC := $(SOFIA_SIP)/stun/stun_dns.c \
				$(SOFIA_SIP)/stun/stun_tag_ref.c \
				$(SOFIA_SIP)/stun/stun.c \
				$(SOFIA_SIP)/stun/stun_mini.c \
				$(SOFIA_SIP)/stun/stun_common.c \
				$(SOFIA_SIP)/stun/stun_tag.c

SOFIA_SIP_SOA_SRC := $(SOFIA_SIP)/soa/soa.c \
				$(SOFIA_SIP)/soa/soa_static.c \
				$(SOFIA_SIP)/soa/soa_tag.c \
				$(SOFIA_SIP)/soa/soa_tag_ref.c

SOFIA_SIP_TPORT_SRC := $(SOFIA_SIP)/tport/tport.c \
				$(SOFIA_SIP)/tport/tport_logging.c \
				$(SOFIA_SIP)/tport/tport_stub_sigcomp.c \
				$(SOFIA_SIP)/tport/tport_type_udp.c \
				$(SOFIA_SIP)/tport/tport_type_tcp.c \
				$(SOFIA_SIP)/tport/tport_type_sctp.c \
				$(SOFIA_SIP)/tport/tport_tag.c \
				$(SOFIA_SIP)/tport/tport_tag_ref.c \
				$(SOFIA_SIP)/tport/tport_type_connect.c \
				$(SOFIA_SIP)/tport/tport_type_stun.c \
				$(SOFIA_SIP)/tport/tport_stub_stun.c 

SOFIA_SIP_NTA_SRC := $(SOFIA_SIP)/nta/nta.c \
				$(SOFIA_SIP)/nta/nta_check.c \
				$(SOFIA_SIP)/nta/nta_tag.c \
				$(SOFIA_SIP)/nta/nta_tag_ref.c \
				$(SOFIA_SIP)/nta/sl_utils_print.c \
				$(SOFIA_SIP)/nta/sl_utils_log.c \
				$(SOFIA_SIP)/nta/sl_read_payload.c

SOFIA_SIP_NTH_SRC := $(SOFIA_SIP)/nth/http-client.c \
				$(SOFIA_SIP)/nth/nth_client.c \
				$(SOFIA_SIP)/nth/nth_tag.c \
				$(SOFIA_SIP)/nth/nth_server.c \
				$(SOFIA_SIP)/nth/nth_tag_ref.c

SOFIA_SIP_NEA_SRC := $(SOFIA_SIP)/nea/nea.c \
				$(SOFIA_SIP)/nea/nea_event.c \
				$(SOFIA_SIP)/nea/nea_tag.c \
				$(SOFIA_SIP)/nea/nea_debug.c \
				$(SOFIA_SIP)/nea/nea_server.c \
				$(SOFIA_SIP)/nea/nea_tag_ref.c

SOFIA_SIP_IPTSEC_SRC := $(SOFIA_SIP)/iptsec/auth_client.c \
				$(SOFIA_SIP)/iptsec/auth_common.c \
				$(SOFIA_SIP)/iptsec/auth_digest.c \
				$(SOFIA_SIP)/iptsec/auth_module.c \
				$(SOFIA_SIP)/iptsec/auth_tag.c \
				$(SOFIA_SIP)/iptsec/auth_tag_ref.c \
				$(SOFIA_SIP)/iptsec/auth_plugin.c \
				$(SOFIA_SIP)/iptsec/auth_plugin_delayed.c \
				$(SOFIA_SIP)/iptsec/auth_module_sip.c \
				$(SOFIA_SIP)/iptsec/iptsec_debug.c

SOFIA_SIP_NUA_SRC := $(SOFIA_SIP)/nua/nua_message.c \
				$(SOFIA_SIP)/nua/nua_stack.c \
				$(SOFIA_SIP)/nua/nua_notifier.c \
				$(SOFIA_SIP)/nua/nua_subnotref.c \
				$(SOFIA_SIP)/nua/nua_options.c \
				$(SOFIA_SIP)/nua/nua_tag.c \
				$(SOFIA_SIP)/nua/nua.c \
				$(SOFIA_SIP)/nua/nua_params.c \
				$(SOFIA_SIP)/nua/nua_tag_ref.c \
				$(SOFIA_SIP)/nua/nua_common.c \
				$(SOFIA_SIP)/nua/nua_publish.c \
				$(SOFIA_SIP)/nua/outbound.c \
				$(SOFIA_SIP)/nua/nua_dialog.c \
				$(SOFIA_SIP)/nua/nua_register.c \
				$(SOFIA_SIP)/nua/nua_event_server.c \
				$(SOFIA_SIP)/nua/nua_registrar.c \
				$(SOFIA_SIP)/nua/nua_extension.c \
				$(SOFIA_SIP)/nua/nua_session.c \
				$(SOFIA_SIP)/nua/nua_client.c \
				$(SOFIA_SIP)/nua/nua_server.c 
				


include $(CLEAR_VARS)
LOCAL_MODULE    := su
LOCAL_SRC_FILES := $(SOFIA_SIP_SU_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := features
LOCAL_SRC_FILES := $(SOFIA_SIP_FEATURES_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)
				
include $(CLEAR_VARS)
LOCAL_MODULE    := bnf
LOCAL_SRC_FILES := $(SOFIA_SIP_BNF_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)				

include $(CLEAR_VARS)
LOCAL_MODULE    := sresolv
LOCAL_SRC_FILES := $(SOFIA_SIP_RESOLV_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := ipt
LOCAL_SRC_FILES := $(SOFIA_SIP_IPT_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := sdp
LOCAL_SRC_FILES := $(SOFIA_SIP_SDP_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := url
LOCAL_SRC_FILES := $(SOFIA_SIP_URL_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := msg
LOCAL_SRC_FILES := $(SOFIA_SIP_MSG_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := sip
LOCAL_SRC_FILES := $(SOFIA_SIP_SIP_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := http
LOCAL_SRC_FILES := $(SOFIA_SIP_HTTP_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := stun
LOCAL_SRC_FILES := $(SOFIA_SIP_STUN_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := soa
LOCAL_SRC_FILES := $(SOFIA_SIP_SOA_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := tport
LOCAL_SRC_FILES := $(SOFIA_SIP_TPORT_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := nta
LOCAL_SRC_FILES := $(SOFIA_SIP_NTA_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := nth
LOCAL_SRC_FILES := $(SOFIA_SIP_NTH_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := nea
LOCAL_SRC_FILES := $(SOFIA_SIP_NEA_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := iptsec
LOCAL_SRC_FILES := $(SOFIA_SIP_IPTSEC_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := nua
LOCAL_SRC_FILES := $(SOFIA_SIP_NUA_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := sofia-sip
# su features bnf sresolv ipt sdp url msg sip http stun soa tport nta nth nea iptsec nua
LOCAL_SRC_FILES := 	$(SOFIA_SIP_SU_SRC) \
					$(SOFIA_SIP_FEATURES_SRC) \
					$(SOFIA_SIP_BNF_SRC) \
					$(SOFIA_SIP_RESOLV_SRC) \
					$(SOFIA_SIP_IPT_SRC) \
					$(SOFIA_SIP_SDP_SRC) \
					$(SOFIA_SIP_URL_SRC) \
					$(SOFIA_SIP_MSG_SRC) \
					$(SOFIA_SIP_SIP_SRC) \
					$(SOFIA_SIP_HTTP_SRC) \
					$(SOFIA_SIP_STUN_SRC) \
					$(SOFIA_SIP_SOA_SRC) \
					$(SOFIA_SIP_TPORT_SRC) \
					$(SOFIA_SIP_NTA_SRC) \
					$(SOFIA_SIP_NTH_SRC) \
					$(SOFIA_SIP_NEA_SRC) \
					$(SOFIA_SIP_IPTSEC_SRC) \
					$(SOFIA_SIP_NUA_SRC)
LOCAL_C_INCLUDES += $(SOFIA_SIP_INCLUDES)
include $(BUILD_STATIC_LIBRARY)
