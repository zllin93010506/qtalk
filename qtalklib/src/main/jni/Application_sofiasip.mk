APP_STL:= stlport_static
APP_BUILD_SCRIPT := $(call my-dir)/media/Android.mk 
APP_BUILD_SCRIPT += $(call my-dir)/call/thirdparty/libsofia-sip/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/call/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/media/thirdparty/ffmpeg/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/media/thirdparty/libvpx/Android.mk 

APP_MODULES := ffmpeg \
               sofia-sip \
               call_engine \
               media_g7221_transform \
               media_speex_transform \
               qtransport \
               media_video_stream_engine_transform \
               media_audio_stream_engine_transform \
               media_ffmpeg_transform \
               media_x264_transform \
               media_vp8_transform \
               media_dip_transform \
               media_alc_transform
