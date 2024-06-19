APP_STL             := c++_static
APP_ABI             := armeabi-v7a x86
APP_PLATFORM        := android-16
APP_BUILD_SCRIPT    := $(call my-dir)/media/Android.mk
APP_BUILD_SCRIPT    += $(call my-dir)/logger/Android.mk
APP_BUILD_SCRIPT    += $(call my-dir)/mylibspeex/Android.mk
APP_MODULES         := media_aec_transform \
                       media_camera_source \
                       media_g7221_transform \
                       media_speex_transform \
                       media_openh264_transform \
                       media_opus_transform \
                       media_video_stream_engine_transform \
                       media_audio_stream_engine_transform \
                       media_x264_transform
                       
