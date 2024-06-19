include $(CLEAR_VARS)
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -ldl -llog
#LOCAL_MODULE := media_speex_transform

LOCAL_SRC_FILES := addrinfo.c \
		getopt.c \
		inet_ntop.c \
		inet_pton.c \
		localinfo.c \
		memccpy.c \
		memcspn.c \
		memmem.c \
		memspn.c \
		poll.c \
		smoothsort.c \
		string0.c \
		strtoull.c \
		su_addrinfo.c \
		su_alloc.c \
		su_alloc_lock.c \
		su_base_port.c \
		su_bm.c \
		su.c \
		su_default_log.c \
		su_devpoll_port.c \
		su_epoll_port.c \
		su_errno.c \
		su_global_log.c \
		su_kqueue_port.c \
		su_localinfo.c \
		su_log.c \
		su_md5.c \
		su_os_nw.c \
		su_osx_runloop.c \
		su_poll_port.c \
		su_port.c \
		su_proxy.c \
		su_pthread_port.c \
		su_root.c \
		su_select_port.c \
		su_socket_port.c \
		su_sprintf.c \
		su_strdup.c \
		su_string.c \
		su_strlst.c \
		su_tag.c \
		su_tag_io.c \
		su_taglist.c \
		su_tag_ref.c \
		su_time0.c \
		su_time.c \
		su_timer.c \
		su_uniqueid.c \
		su_vector.c \
		su_wait.c \
		su_win32_port.c
         
LOCAL_CFLAGS = -DFIXED_POINT -DEXPORT="" -UHAVE_CONFIG_H -I$(LOCAL_PATH)/sofia-sip
include $(BUILD_STATIC_LIBRARY)
