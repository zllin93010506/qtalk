LOCAL_PATH := $(call my-dir) 

include $(CLEAR_VARS)

LOCAL_MODULE := libspeex
LOCAL_CFLAGS = -DFIXED_POINT -DEXPORT="" -UHAVE_CONFIG_H -I$(LOCAL_PATH)/../include

LOCAL_SRC_FILES := \
./bits.c \
./buffer.c \
./cb_search.c \
./exc_10_16_table.c \
./exc_10_32_table.c \
./exc_20_32_table.c \
./exc_5_256_table.c \
./exc_5_64_table.c \
./exc_8_128_table.c \
./filterbank.c \
./filters.c \
./gain_table.c \
./gain_table_lbr.c \
./hexc_10_32_table.c \
./hexc_table.c \
./high_lsp_tables.c \
./jitter.c \
./kiss_fft.c \
./kiss_fftr.c \
./lpc.c \
./lsp.c \
./lsp_tables_nb.c \
./ltp.c \
./mdf.c \
./modes.c \
./modes_wb.c \
./nb_celp.c \
./preprocess.c \
./quant_lsp.c \
./resample.c \
./sb_celp.c \
./scal.c \
./smallft.c \
./speex.c \
./speex_callbacks.c \
./speex_header.c \
./stereo.c \
./testdenoise.c \
./testecho.c \
./testenc.c \
./testenc_uwb.c \
./testenc_wb.c \
./testjitter.c \
./vbr.c \
./vq.c \
./window.c

include $(BUILD_STATIC_LIBRARY)
