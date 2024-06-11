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
LOCAL_PATH := $(call my-dir)
#LOCAL_CFLAGS := -g
include $(CLEAR_VARS)
LOCAL_MODULE    := mylibspeex
LOCAL_CFLAGS = -DFIXED_POINT -DUSE_KISS_FFT -DEXPORT="" -UHAVE_CONFIG_H -I$(LOCAL_PATH)/_speex/include
#LOCAL_CFLAGS = -DFIXED_POINT -DUSE_SMALLFT -DEXPORT="" -UHAVE_CONFIG_H -I$(LOCAL_PATH)/_speex/include
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog
LOCAL_SRC_FILES := \
					capture.c \
					playback.c \
					msgque.c \
					_speex/libspeex/bits.c \
					_speex/libspeex/buffer.c \
					_speex/libspeex/cb_search.c \
					_speex/libspeex/exc_10_16_table.c \
					_speex/libspeex/exc_10_32_table.c \
					_speex/libspeex/exc_20_32_table.c \
					_speex/libspeex/exc_5_256_table.c \
					_speex/libspeex/exc_5_64_table.c \
					_speex/libspeex/exc_8_128_table.c \
					_speex/libspeex/filterbank.c \
					_speex/libspeex/filters.c \
					_speex/libspeex/gain_table.c \
					_speex/libspeex/gain_table_lbr.c \
					_speex/libspeex/hexc_10_32_table.c \
					_speex/libspeex/hexc_table.c \
					_speex/libspeex/high_lsp_tables.c \
					_speex/libspeex/jitter.c \
					_speex/libspeex/kiss_fft.c \
					_speex/libspeex/kiss_fftr.c \
					_speex/libspeex/lpc.c \
					_speex/libspeex/lsp.c \
					_speex/libspeex/lsp_tables_nb.c \
					_speex/libspeex/ltp.c \
					_speex/libspeex/mdf.c \
					_speex/libspeex/modes.c \
					_speex/libspeex/modes_wb.c \
					_speex/libspeex/nb_celp.c \
					_speex/libspeex/preprocess.c \
					_speex/libspeex/quant_lsp.c \
					_speex/libspeex/resample.c \
					_speex/libspeex/sb_celp.c \
					_speex/libspeex/scal.c \
					_speex/libspeex/smallft.c \
					_speex/libspeex/speex.c \
					_speex/libspeex/speex_callbacks.c \
					_speex/libspeex/speex_header.c \
					_speex/libspeex/stereo.c \
					_speex/libspeex/vbr.c \
					_speex/libspeex/vq.c \
					_speex/libspeex/window.c \
					_speex/libspeex/fftwrap.c

include $(BUILD_STATIC_LIBRARY)
