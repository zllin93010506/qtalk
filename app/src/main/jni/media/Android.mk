# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# the purpose of this sample is to demonstrate how one can
# generate two distinct shared libraries and have them both
# uploaded in
#
LOCAL_PATH      := $(call my-dir)

# ====================================================================
# Prebuild Static Library
# ====================================================================
include $(CLEAR_VARS)
LOCAL_MODULE            := qStreamEngine
ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_SRC_FILES         := ../StreamEngineService/StreamEngine/libqStream_x86.a
else
    LOCAL_SRC_FILES         := ../StreamEngineService/StreamEngine/libqStream.a
endif
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE    := q_x264
ifeq ($(TARGET_PLATFORM),android-12)
    LOCAL_SRC_FILES := ./thirdparty/x264.2245/out/$(TARGET_ARCH_ABI)/lib/libx264.a
    X264_C_INCLUDES := $(LOCAL_PATH)/thirdparty/x264.2245/out/$(TARGET_ARCH_ABI)/include
else
    LOCAL_SRC_FILES := ./thirdparty/x264.2245/android/$(TARGET_ARCH_ABI)/lib/libx264.a
    X264_C_INCLUDES := $(LOCAL_PATH)/thirdparty/x264.2245/android/$(TARGET_ARCH_ABI)/include
endif
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE    := q_openh264
#ifeq ($(TARGET_PLATFORM),android-12)
#    LOCAL_SRC_FILES := ./thirdparty/openh264/out/$(TARGET_ARCH_ABI)/libopenh264.a
#    OPENH264_C_INCLUDES := $(LOCAL_PATH)/thirdparty/openh264/out/include
#else
    LOCAL_SRC_FILES := ./thirdparty/openh264/android/lib/$(TARGET_ARCH_ABI)/libopenh264.a
    OPENH264_C_INCLUDES := $(LOCAL_PATH)/thirdparty/openh264/android/include
#endif
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE    := q_avcodec
ifeq ($(TARGET_PLATFORM),android-12)
    LOCAL_SRC_FILES := ./thirdparty/ffmpeg-1.2.1/out/$(TARGET_ARCH_ABI)/lib/libavcodec.a
else
    LOCAL_SRC_FILES := ./thirdparty/ffmpeg-5.1/android/$(TARGET_ARCH_ABI)/lib/libavcodec.a
endif
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := q_avformat
ifeq ($(TARGET_PLATFORM),android-12)
    LOCAL_SRC_FILES := ./thirdparty/ffmpeg-1.2.1/out/$(TARGET_ARCH_ABI)/lib/libavformat.a
else
    LOCAL_SRC_FILES := ./thirdparty/ffmpeg-5.1/android/$(TARGET_ARCH_ABI)/lib/libavformat.a
endif
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := q_avutil
ifeq ($(TARGET_PLATFORM),android-12)
    LOCAL_SRC_FILES := ./thirdparty/ffmpeg-1.2.1/out/$(TARGET_ARCH_ABI)/lib/libavutil.a
else
    LOCAL_SRC_FILES := ./thirdparty/ffmpeg-5.1/android/$(TARGET_ARCH_ABI)/lib/libavutil.a
endif
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := q_swresample
ifeq ($(TARGET_PLATFORM),android-12)
    LOCAL_SRC_FILES := ./thirdparty/ffmpeg-1.2.1/out/$(TARGET_ARCH_ABI)/lib/libswresample.a
else
    LOCAL_SRC_FILES := ./thirdparty/ffmpeg-5.1/android/$(TARGET_ARCH_ABI)/lib/libswresample.a
endif
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := q_swscale
ifeq ($(TARGET_PLATFORM),android-12)
    LOCAL_SRC_FILES := ./thirdparty/ffmpeg-1.2.1/out/$(TARGET_ARCH_ABI)/lib/libswscale.a
else
    LOCAL_SRC_FILES := ./thirdparty/ffmpeg-5.1/android/$(TARGET_ARCH_ABI)/lib/libswscale.a
endif
include $(PREBUILT_STATIC_LIBRARY)


#==================== Opus Tranform =================
OPUS_PATH             := $(LOCAL_PATH)/thirdparty/opus-1.0.3
include $(CLEAR_VARS)
    LOCAL_MODULE      := opus.r103
    OPUS_SOURCES      := $(OPUS_PATH)/src/opus.c \
                         $(OPUS_PATH)/src/opus_decoder.c \
                         $(OPUS_PATH)/src/opus_encoder.c \
                         $(OPUS_PATH)/src/opus_multistream.c \
                         $(OPUS_PATH)/src/repacketizer.c
    CELT_SOURCES      := $(OPUS_PATH)/celt/bands.c \
                         $(OPUS_PATH)/celt/celt.c \
                         $(OPUS_PATH)/celt/cwrs.c \
                         $(OPUS_PATH)/celt/entcode.c \
                         $(OPUS_PATH)/celt/entdec.c \
                         $(OPUS_PATH)/celt/entenc.c \
                         $(OPUS_PATH)/celt/kiss_fft.c \
                         $(OPUS_PATH)/celt/laplace.c \
                         $(OPUS_PATH)/celt/mathops.c \
                         $(OPUS_PATH)/celt/mdct.c \
                         $(OPUS_PATH)/celt/modes.c \
                         $(OPUS_PATH)/celt/pitch.c \
                         $(OPUS_PATH)/celt/celt_lpc.c \
                         $(OPUS_PATH)/celt/quant_bands.c \
                         $(OPUS_PATH)/celt/rate.c \
                         $(OPUS_PATH)/celt/vq.c
    SILK_SOURCES      := $(OPUS_PATH)/silk/CNG.c \
                         $(OPUS_PATH)/silk/code_signs.c \
                         $(OPUS_PATH)/silk/init_decoder.c \
                         $(OPUS_PATH)/silk/decode_core.c \
                         $(OPUS_PATH)/silk/decode_frame.c \
                         $(OPUS_PATH)/silk/decode_parameters.c \
                         $(OPUS_PATH)/silk/decode_indices.c \
                         $(OPUS_PATH)/silk/decode_pulses.c \
                         $(OPUS_PATH)/silk/decoder_set_fs.c \
                         $(OPUS_PATH)/silk/dec_API.c \
                         $(OPUS_PATH)/silk/enc_API.c \
                         $(OPUS_PATH)/silk/encode_indices.c \
                         $(OPUS_PATH)/silk/encode_pulses.c \
                         $(OPUS_PATH)/silk/gain_quant.c \
                         $(OPUS_PATH)/silk/interpolate.c \
                         $(OPUS_PATH)/silk/LP_variable_cutoff.c \
                         $(OPUS_PATH)/silk/NLSF_decode.c \
                         $(OPUS_PATH)/silk/NSQ.c \
                         $(OPUS_PATH)/silk/NSQ_del_dec.c \
                         $(OPUS_PATH)/silk/PLC.c \
                         $(OPUS_PATH)/silk/shell_coder.c \
                         $(OPUS_PATH)/silk/tables_gain.c \
                         $(OPUS_PATH)/silk/tables_LTP.c \
                         $(OPUS_PATH)/silk/tables_NLSF_CB_NB_MB.c \
                         $(OPUS_PATH)/silk/tables_NLSF_CB_WB.c \
                         $(OPUS_PATH)/silk/tables_other.c \
                         $(OPUS_PATH)/silk/tables_pitch_lag.c \
                         $(OPUS_PATH)/silk/tables_pulses_per_block.c \
                         $(OPUS_PATH)/silk/VAD.c \
                         $(OPUS_PATH)/silk/control_audio_bandwidth.c \
                         $(OPUS_PATH)/silk/quant_LTP_gains.c \
                         $(OPUS_PATH)/silk/VQ_WMat_EC.c \
                         $(OPUS_PATH)/silk/HP_variable_cutoff.c \
                         $(OPUS_PATH)/silk/NLSF_encode.c \
                         $(OPUS_PATH)/silk/NLSF_VQ.c \
                         $(OPUS_PATH)/silk/NLSF_unpack.c \
                         $(OPUS_PATH)/silk/NLSF_del_dec_quant.c \
                         $(OPUS_PATH)/silk/process_NLSFs.c \
                         $(OPUS_PATH)/silk/stereo_LR_to_MS.c \
                         $(OPUS_PATH)/silk/stereo_MS_to_LR.c \
                         $(OPUS_PATH)/silk/check_control_input.c \
                         $(OPUS_PATH)/silk/control_SNR.c \
                         $(OPUS_PATH)/silk/init_encoder.c \
                         $(OPUS_PATH)/silk/control_codec.c \
                         $(OPUS_PATH)/silk/A2NLSF.c \
                         $(OPUS_PATH)/silk/ana_filt_bank_1.c \
                         $(OPUS_PATH)/silk/biquad_alt.c \
                         $(OPUS_PATH)/silk/bwexpander_32.c \
                         $(OPUS_PATH)/silk/bwexpander.c \
                         $(OPUS_PATH)/silk/debug.c \
                         $(OPUS_PATH)/silk/decode_pitch.c \
                         $(OPUS_PATH)/silk/inner_prod_aligned.c \
                         $(OPUS_PATH)/silk/lin2log.c \
                         $(OPUS_PATH)/silk/log2lin.c \
                         $(OPUS_PATH)/silk/LPC_analysis_filter.c \
                         $(OPUS_PATH)/silk/LPC_inv_pred_gain.c \
                         $(OPUS_PATH)/silk/table_LSF_cos.c \
                         $(OPUS_PATH)/silk/NLSF2A.c \
                         $(OPUS_PATH)/silk/NLSF_stabilize.c \
                         $(OPUS_PATH)/silk/NLSF_VQ_weights_laroia.c \
                         $(OPUS_PATH)/silk/pitch_est_tables.c \
                         $(OPUS_PATH)/silk/resampler.c \
                         $(OPUS_PATH)/silk/resampler_down2_3.c \
                         $(OPUS_PATH)/silk/resampler_down2.c \
                         $(OPUS_PATH)/silk/resampler_private_AR2.c \
                         $(OPUS_PATH)/silk/resampler_private_down_FIR.c \
                         $(OPUS_PATH)/silk/resampler_private_IIR_FIR.c \
                         $(OPUS_PATH)/silk/resampler_private_up2_HQ.c \
                         $(OPUS_PATH)/silk/resampler_rom.c \
                         $(OPUS_PATH)/silk/sigm_Q15.c \
                         $(OPUS_PATH)/silk/sort.c \
                         $(OPUS_PATH)/silk/sum_sqr_shift.c \
                         $(OPUS_PATH)/silk/stereo_decode_pred.c \
                         $(OPUS_PATH)/silk/stereo_encode_pred.c \
                         $(OPUS_PATH)/silk/stereo_find_predictor.c \
                         $(OPUS_PATH)/silk/stereo_quant_pred.c
SILK_SOURCES_FIXED    := $(OPUS_PATH)/silk/fixed/LTP_analysis_filter_FIX.c \
                         $(OPUS_PATH)/silk/fixed/LTP_scale_ctrl_FIX.c \
                         $(OPUS_PATH)/silk/fixed/corrMatrix_FIX.c \
                         $(OPUS_PATH)/silk/fixed/encode_frame_FIX.c \
                         $(OPUS_PATH)/silk/fixed/find_LPC_FIX.c \
                         $(OPUS_PATH)/silk/fixed/find_LTP_FIX.c \
                         $(OPUS_PATH)/silk/fixed/find_pitch_lags_FIX.c \
                         $(OPUS_PATH)/silk/fixed/find_pred_coefs_FIX.c \
                         $(OPUS_PATH)/silk/fixed/noise_shape_analysis_FIX.c \
                         $(OPUS_PATH)/silk/fixed/prefilter_FIX.c \
                         $(OPUS_PATH)/silk/fixed/process_gains_FIX.c \
                         $(OPUS_PATH)/silk/fixed/regularize_correlations_FIX.c \
                         $(OPUS_PATH)/silk/fixed/residual_energy16_FIX.c \
                         $(OPUS_PATH)/silk/fixed/residual_energy_FIX.c \
                         $(OPUS_PATH)/silk/fixed/solve_LS_FIX.c \
                         $(OPUS_PATH)/silk/fixed/warped_autocorrelation_FIX.c \
                         $(OPUS_PATH)/silk/fixed/apply_sine_window_FIX.c \
                         $(OPUS_PATH)/silk/fixed/autocorr_FIX.c \
                         $(OPUS_PATH)/silk/fixed/burg_modified_FIX.c \
                         $(OPUS_PATH)/silk/fixed/k2a_FIX.c \
                         $(OPUS_PATH)/silk/fixed/k2a_Q16_FIX.c \
                         $(OPUS_PATH)/silk/fixed/pitch_analysis_core_FIX.c \
                         $(OPUS_PATH)/silk/fixed/vector_ops_FIX.c \
                         $(OPUS_PATH)/silk/fixed/schur64_FIX.c \
                         $(OPUS_PATH)/silk/fixed/schur_FIX.c
    LOCAL_SRC_FILES   := $(CELT_SOURCES) $(SILK_SOURCES) $(SILK_SOURCES_FIXED) $(OPUS_SOURCES)
    LOCAL_LDLIBS      := -lm -llog
    LOCAL_STATIC_LIBRARIES := liblogger
    LOCAL_C_INCLUDES  := $(OPUS_PATH)/include \
                         $(OPUS_PATH)/silk \
                         $(OPUS_PATH)/silk/fixed \
                         $(OPUS_PATH)/celt \
                         $(LOCAL_PATH)/../logger/ \

    LOCAL_CFLAGS      := -DNULL=0 -DSOCKLEN_T=socklen_t -DLOCALE_NOT_USED -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64
    LOCAL_CFLAGS      += -Drestrict='' -D__EMX__ -DOPUS_BUILD -DFIXED_POINT=1 -DDISABLE_FLOAT_API -DUSE_ALLOCA -DHAVE_LRINT -DHAVE_LRINTF  -DAVOID_TABLES
    LOCAL_CFLAGS      += -w -std=gnu99 -O3 -fno-strict-aliasing -fprefetch-loop-arrays  -fno-math-errno
    LOCAL_CPPFLAGS    := -DBSD=1
    LOCAL_CPPFLAGS    += -ffast-math -O3 -funroll-loops
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
    LOCAL_MODULE      := media_opus_transform
    OPUS_PATH         := $(LOCAL_PATH)/thirdparty/opus-1.0.3
    LOCAL_LDLIBS      := -L$(SYSROOT)/usr/lib -ldl -llog    
    
    LOCAL_SRC_FILES   := OpusEncoderTransformJni.cpp \
                         OpusDecoderTransformJni.cpp

    LOCAL_C_INCLUDES  := $(LOCAL_PATH) \
                         $(LOCAL_PATH)/thirdparty \
                         $(OPUS_PATH)/src \
                         $(OPUS_PATH)/celt \
                         $(OPUS_PATH)/silk \
                         $(OPUS_PATH)/include \
                         $(LOCAL_PATH)/../logger/ \

    LOCAL_STATIC_LIBRARIES := opus.r103 liblogger
include $(BUILD_SHARED_LIBRARY)


#==================== speexdsp-1.2rc3 library ====================
include $(CLEAR_VARS)
    LOCAL_MODULE     := speexdsp1.2rc3
    SPEEXDSP_PATH    := $(LOCAL_PATH)/thirdparty/speexdsp-1.2rc3
    LOCAL_CFLAGS     := -DFIXED_POINT -DUSE_KISS_FFT -DEXPORT="" -UHAVE_CONFIG_H -DHAVE_INTTYPES_H
    #LOCAL_LDLIBS    := -L$(SYSROOT)/usr/lib -llog
    LOCAL_STATIC_LIBRARIES := liblogger
    LOCAL_C_INCLUDES := $(SPEEXDSP_PATH)/include \
                        $(LOCAL_PATH)/../logger/ \

    LOCAL_SRC_FILES  := $(SPEEXDSP_PATH)/libspeexdsp/buffer.c \
                        $(SPEEXDSP_PATH)/libspeexdsp/fftwrap.c \
                        $(SPEEXDSP_PATH)/libspeexdsp/filterbank.c \
                        $(SPEEXDSP_PATH)/libspeexdsp/jitter.c \
                        $(SPEEXDSP_PATH)/libspeexdsp/kiss_fft.c \
                        $(SPEEXDSP_PATH)/libspeexdsp/kiss_fftr.c \
                        $(SPEEXDSP_PATH)/libspeexdsp/mdf.c \
                        $(SPEEXDSP_PATH)/libspeexdsp/preprocess.c \
                        $(SPEEXDSP_PATH)/libspeexdsp/resample.c \
                        $(SPEEXDSP_PATH)/libspeexdsp/scal.c \
                        $(SPEEXDSP_PATH)/libspeexdsp/smallft.c \
                        $(SPEEXDSP_PATH)/libspeexdsp/testdenoise.c \
                        $(SPEEXDSP_PATH)/libspeexdsp/testecho.c \
                        $(SPEEXDSP_PATH)/libspeexdsp/testjitter.c \
                        $(SPEEXDSP_PATH)/libspeexdsp/testresample.c
include $(BUILD_STATIC_LIBRARY)

#====================Camera Source====================
include $(CLEAR_VARS)
    LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -ldl -llog
    LOCAL_MODULE := media_camera_source
    LOCAL_SRC_FILES := CameraSourceJni.cpp
    LOCAL_STATIC_LIBRARIES := liblogger
    LOCAL_C_INCLUDES := $(LOCAL_PATH)/../logger/
    LOCAL_CFLAGS = $(common_cflags) -g
include $(BUILD_SHARED_LIBRARY)

#====================AEC Tranform====================
include $(CLEAR_VARS)
    AEC := thirdparty/SoftwareAEC
    #SPEEX:= ../mylibspeex/_speex/include
    SPEEXLIB_PATH := thirdparty/speexdsp-1.2rc3/include
    LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -ldl -llog
    LOCAL_MODULE := media_aec_transform
    LOCAL_CFLAGS = $(common_cflags) -DFIXED_POINT -DEXPORT="" -UHAVE_CONFIG_H -g -I$(LOCAL_PATH)/$(AEC)

    LOCAL_SRC_FILES := $(AEC)/agc.c \
				       $(AEC)/division_operations.c \
				       $(AEC)/energy.c \
				       $(AEC)/get_scaling_square.c \
				       $(AEC)/Sys_Software_AEC.c \
				       $(AEC)/vad_core.c \
				       $(AEC)/vad_filterbank.c \
				       $(AEC)/vad_gmm.c \
				       $(AEC)/vad_sp.c \
				       $(AEC)/webrtc_vad.c \
					    AecSpeakerTransformJni.cpp \
					    AecMicTransformJni.cpp
    LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SPEEXLIB_PATH) $(AEC) $(LOCAL_PATH)/../logger/
    LOCAL_STATIC_LIBRARIES := speexdsp1.2rc3 liblogger
include $(BUILD_SHARED_LIBRARY)


#====================OpenH264 Tranform===============
include $(CLEAR_VARS)
    OPENH264         := $(LOCAL_PATH)/thirdparty/openh264
    LOCAL_LDLIBS     := -L$(SYSROOT)/usr/lib -ldl -llog
    LOCAL_MODULE     := media_openh264_transform
    LOCAL_SRC_FILES  := $(OPENH264)/openh264_enc.cpp \
                        OpenH264EncodeTransformJni.cpp

    LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
                        $(OPENH264) \
                        $(OPENH264_C_INCLUDES) \
                        $(LOCAL_PATH)/../logger/
    LOCAL_CFLAGS     := $(LOCAL_C_INCLUDES:%=-I%) -g
    LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
    LOCAL_STATIC_LIBRARIES := liblogger q_openh264
include $(BUILD_SHARED_LIBRARY)

#====================SPEEX Tranform====================
include $(CLEAR_VARS)
SPEEX1.2RC1 := thirdparty/speex-1.2rc1
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -ldl -llog
LOCAL_MODULE := media_speex_transform
LOCAL_STATIC_LIBRARIES := liblogger

LOCAL_SRC_FILES := SpeexEncoderTransformJni.cpp \
                   SpeexDecoderTransformJni.cpp \
                   $(SPEEX1.2RC1)/libspeex/speex.c \
                   $(SPEEX1.2RC1)/libspeex/speex_callbacks.c \
                   $(SPEEX1.2RC1)/libspeex/bits.c \
                   $(SPEEX1.2RC1)/libspeex/modes.c \
                   $(SPEEX1.2RC1)/libspeex/nb_celp.c \
                   $(SPEEX1.2RC1)/libspeex/exc_20_32_table.c \
                   $(SPEEX1.2RC1)/libspeex/exc_5_256_table.c \
                   $(SPEEX1.2RC1)/libspeex/exc_5_64_table.c \
                   $(SPEEX1.2RC1)/libspeex/exc_8_128_table.c \
                   $(SPEEX1.2RC1)/libspeex/exc_10_32_table.c \
                   $(SPEEX1.2RC1)/libspeex/exc_10_16_table.c \
                   $(SPEEX1.2RC1)/libspeex/filters.c \
                   $(SPEEX1.2RC1)/libspeex/quant_lsp.c \
                   $(SPEEX1.2RC1)/libspeex/ltp.c \
                   $(SPEEX1.2RC1)/libspeex/lpc.c \
                   $(SPEEX1.2RC1)/libspeex/lsp.c \
                   $(SPEEX1.2RC1)/libspeex/vbr.c \
                   $(SPEEX1.2RC1)/libspeex/gain_table.c \
                   $(SPEEX1.2RC1)/libspeex/gain_table_lbr.c \
                   $(SPEEX1.2RC1)/libspeex/lsp_tables_nb.c \
                   $(SPEEX1.2RC1)/libspeex/cb_search.c \
                   $(SPEEX1.2RC1)/libspeex/vq.c \
                   $(SPEEX1.2RC1)/libspeex/window.c \
                   $(SPEEX1.2RC1)/libspeex/high_lsp_tables.c
LOCAL_CFLAGS = -DFIXED_POINT -DEXPORT="" -UHAVE_CONFIG_H -g -I$(LOCAL_PATH)/$(SPEEX1.2RC1)/include -I$(LOCAL_PATH)/../logger
include $(BUILD_SHARED_LIBRARY)

#====================G7221 Transfrom====================
include $(CLEAR_VARS)
G7221 := thirdparty/G7221

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -ldl -llog
LOCAL_MODULE    := media_g7221_transform
LOCAL_SRC_FILES := G7221EncoderTransformJni.cpp \
                   G7221DecoderTransformJni.cpp \
                   G7221Decoder.cpp \
                   G7221Encoder.cpp \
                   $(G7221)/coef2sam.cpp \
                   $(G7221)/sam2coef.cpp \
                   $(G7221)/dct4_a.cpp \
                   $(G7221)/dct4_s.cpp \
                   $(G7221)/decoder.cpp \
                   $(G7221)/encoder.cpp \
                   $(G7221)/common/basop32.cpp \
                   $(G7221)/common/common.cpp \
                   $(G7221)/common/count.cpp \
                   $(G7221)/common/huff_tab.cpp \
                   $(G7221)/common/tables.cpp
LOCAL_LDLIBS += -lm -lz -I$(LOCAL_PATH)/$(G7221)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../logger/
LOCAL_STATIC_LIBRARIES := liblogger
include $(BUILD_SHARED_LIBRARY)

#====================Audio StreamEngine Transfrom====================
include $(CLEAR_VARS)
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -ldl -llog
LOCAL_MODULE    := media_audio_stream_engine_transform
LOCAL_STATIC_LIBRARIES := qStreamEngine liblogger
LOCAL_SRC_FILES := AudioStreamEngineTransformJni.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/../logger/
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -g
include $(BUILD_SHARED_LIBRARY)

#====================Video StreamEngine Transfrom====================
include $(CLEAR_VARS)
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -ldl -llog
LOCAL_MODULE    := media_video_stream_engine_transform
LOCAL_SRC_FILES := VideoStreamEngineTransformJni.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/../logger/
LOCAL_STATIC_LIBRARIES := qStreamEngine liblogger
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -g
include $(BUILD_SHARED_LIBRARY)


#====================X264 Transfrom====================
include $(CLEAR_VARS)
#X264            := $(LOCAL_PATH)/thirdparty/x264.2245/out/$(TARGET_ARCH_ABI)
LOCAL_MODULE     := media_x264_transform
LOCAL_SRC_FILES  := X264TransformJni.cpp \
                    qstream/video/QStreamStatistics.cpp \
                    X264Encoder.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(X264_C_INCLUDES) \
                    $(LOCAL_PATH)/../logger/

LOCAL_CFLAGS     += $(LOCAL_C_INCLUDES:%=-I%) -g
LOCAL_LDLIBS     := -L$(SYSROOT)/usr/lib -llog -lm -lz
LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
LOCAL_STATIC_LIBRARIES := liblogger q_x264
include $(BUILD_SHARED_LIBRARY)

#====================FFMPEG: H264 & Mjpeg encoder+decoder  Transfrom====================
#include $(CLEAR_VARS)
#LOCAL_MODULE     := media_ffmpeg_transform
#ifeq ($(TARGET_PLATFORM),android-12)
#	FFMPEG       := $(LOCAL_PATH)/thirdparty/ffmpeg-1.2.1/out/$(TARGET_ARCH_ABI)
#else
#	FFMPEG       := $(LOCAL_PATH)/thirdparty/ffmpeg-5.1/android/$(TARGET_ARCH_ABI)
#endif
#
#LOCAL_SRC_FILES  := H264EncodeTransformJni.cpp \
#                    CffmpegEncoder.cpp \
#                    H264DecodeTransformJni.cpp \
#                    CffmpegDecoder.cpp \
#                    MjpegEncodeTransformJni.cpp \
#                    MjpegDecodeTransformJni.cpp \
#                    FFMpegHandlerJni.cpp
#
#LOCAL_C_INCLUDES := $(LOCAL_PATH) \
#                    $(FFMPEG)/include \
#                    $(LOCAL_PATH)/../logger/
#
#LOCAL_CFLAGS     += $(LOCAL_C_INCLUDES:%=-I%)
#LOCAL_LDLIBS     := -L$(SYSROOT)/usr/lib -llog -lm -lz
#LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
#LOCAL_STATIC_LIBRARIES := liblogger q_avcodec q_avformat q_avutil q_swscale
#include $(BUILD_SHARED_LIBRARY)

