/*******************************************************************************
* All Rights Reserved, Copyright @ Quanta Computer Inc. 2014
* File Name: open_h264_enc.h
* File Mark:
* Content Description:
* Other Description: None
* Version: 1.0
* Author: Paul Weng
* Date: 2014/03/11
*
* History:
*   1.2014/03/11 Paul Weng: draft
*******************************************************************************/
#ifndef __OPEN_H264_ENC_H__
#define __OPEN_H264_ENC_H__

#ifdef __cplusplus
extern "C" {
#endif
/* =============================================================================
                                Included headers
============================================================================= */
#include "out/include/codec_api.h"
#include "out/include/codec_app_def.h"
#include "out/include/codec_def.h"
#include <pthread.h>

/* =============================================================================
                                    Type
============================================================================= */
typedef void (*H264DataFp)(unsigned char* frame_data, int frame_size,
                           unsigned int timestamp, struct timeval time_val);
typedef struct _H264Handle H264Handle;
typedef struct _YuvFrame YuvFrame;

struct _H264Handle {
    int                 width;
    int                 height;
    int                 bit_rate_bps;
    int                 gop;
    pthread_t           h264_tid;
    int                 h264_running;
};

struct _YuvFrame {
	unsigned char* frame_data;
	unsigned int   frame_size;
	unsigned int   timestamp;
	struct timeval time_val;
	YuvFrame*	   next;
};


/* =============================================================================
                                Implement Start
============================================================================= */
int open_h264_encorder_create();
void open_h264_encorder_destroy();

int open_h264_encorder_initialize(const int width, const int height,
                                  const int bit_rate_bps, const int fps,
                                  const int gop, H264DataFp cb);
int open_h264_encorder_uninitialize();

int open_h264_encorder_start();
int open_h264_encorder_stop();

int open_h264_encorder_encode_frame(SSourcePicture* pic,
                                    SFrameBSInfo*   bsInfo);

int open_h264_encodeframe2(unsigned char *yuvdata, int yuv_size, uint32_t timestamp, char *h264out);

int open_h264_encorder_set_bitrate(const int bps);
int open_h264_encorder_set_frame_rate(const int frame_rate);
int open_h264_encorder_idr_request();


#ifdef __cplusplus
}
#endif
#endif /* __OPEN_H264_ENC_H__ */
