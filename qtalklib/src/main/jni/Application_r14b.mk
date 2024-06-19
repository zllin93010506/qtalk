APP_STL          := stlport_static
APP_ABI          := armeabi-v7a x86
APP_PLATFORM     := android-12
APP_BUILD_SCRIPT := $(call my-dir)/media/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/mylibspeex/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/call/thirdparty/openssl/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/icl-sip/common/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/icl-sip/low/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/icl-sip/adt/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/icl-sip/sip/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/icl-sip/sdp/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/icl-sip/sipTx/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/icl-sip/DlgLayer/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/icl-sip/callcontrol/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/icl-sip/SessAPI/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/icl-sip/MsgSummary/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/call/Android.mk
APP_BUILD_SCRIPT += $(call my-dir)/logger/Android.mk
APP_MODULES      := media_aec_transform \
                    media_camera_source \
                    media_g7221_transform \
                    media_speex_transform \
                    media_openh264_transform \
                    media_opus_transform \
                    media_video_stream_engine_transform \
                    media_audio_stream_engine_transform \
                    media_x264_transform \
                    ssl cm low adt sipccl sdpccl sipTx DlgLayer callcontrol SessAPI MsgSummary call_engine
#                   media_ffmpeg_transform \
#                   mylibspeex
                   
