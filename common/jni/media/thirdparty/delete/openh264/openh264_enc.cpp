/*******************************************************************************
* All Rights Reserved, Copyright @ Quanta Computer Inc. 2013
* File Name: open_h264_enc.c
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
/* =============================================================================
                                Included headers
============================================================================= */
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "logging.h"

#include "codec_api.h"
#include "openh264_enc.h"

/* =============================================================================
                                     Define
============================================================================= */
#define TAG "open_h264_enc"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG, __VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG, __VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG, __VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG, __VA_ARGS__)


/* =============================================================================
                        Private Data declarations
============================================================================= */
H264Handle*  handle = NULL;
ISVCEncoder* svc_encorder = NULL;
SEncParamExt   param;
int g_bitrate = 0;

/* =============================================================================
                                Implement Start
============================================================================= */
int open_h264_encorder_create()
{
    int ret = -1;
    if (svc_encorder) {
        WelsDestroySVCEncoder(svc_encorder);
    }

    handle = (H264Handle*)malloc(sizeof(H264Handle));
    if (NULL == handle) {
        LOGE("%s <error> H264Handle malloc failure", TAG);
        return -1;
    }
    else {
        memset(handle, 0x00, sizeof(H264Handle));
    }

    ret = WelsCreateSVCEncoder(&svc_encorder);

    LOGD("%s ret = %d", __FUNCTION__, ret);

    return ret;
}


void open_h264_encorder_destroy()
{
    if (handle) {
        free(handle);
        handle = NULL;
    }

    if (svc_encorder) {
        WelsDestroySVCEncoder(svc_encorder);
        svc_encorder = NULL;
    }

    return ;
}


int open_h264_encorder_initialize(const int width, const int height,
                                  const int bit_rate_bps, const int fps,
                                  const int gop, H264DataFp cb)
{
    int ret = 0;
    int i = 0 ;
    int videoFormat = videoFormatI420;

    LOGD("%s(), %d fps, width:%d, height:%d, bit_rate_bps:%d",  __FUNCTION__, fps, width, height, bit_rate_bps);

    if (handle) {
        handle->width = width;
        handle->height = height;
        handle->bit_rate_bps = bit_rate_bps;
        handle->gop = gop;
 //       handle->h264_data_cb = cb;

        handle->h264_tid = 0;
        handle->h264_running = 0;
    }
    g_bitrate = bit_rate_bps;
    memset (&param, 0, sizeof(SEncParamExt));
    svc_encorder->GetDefaultParams(&param);

    param.iUsageType = CAMERA_VIDEO_REAL_TIME;// 0: camera video signal; 1: screen content signal;

    param.iPicWidth = width;
    param.iPicHeight = height;
    param.iTargetBitrate = bit_rate_bps;
    param.iRCMode = RC_BITRATE_MODE; //0: Quality mode ; 1: Bitrate mode;
    param.fMaxFrameRate = fps;

    param.iTemporalLayerNum = 1;// layer number at temporal level
    param.iSpatialLayerNum = 1; // layer number at spatial level
    param.sSpatialLayers[0].iVideoWidth = param.iPicWidth;
    param.sSpatialLayers[0].iVideoHeight = param.iPicHeight;
    param.sSpatialLayers[0].fFrameRate = param.fMaxFrameRate;
    param.sSpatialLayers[0].iSpatialBitrate = param.iTargetBitrate;
    param.sSpatialLayers[0].uiProfileIdc = PRO_BASELINE;
    param.sSpatialLayers[0].iDLayerQp = 0;
    param.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_SINGLE_SLICE;
    param.iComplexityMode = LOW_COMPLEXITY;

    param.uiIntraPeriod = param.fMaxFrameRate*gop;
    param.iNumRefFrame = 1;
    param.bPrefixNalAddingCtrl = false;
    param.bEnableSSEI = false;
    param.iPaddingFlag = 0;
    param.iEntropyCodingModeFlag = 0;

    /* rc control */
    param.bEnableFrameSkip = false;
    //param.iMaxQp = 40;
    //param.iMinQp = 30;

    /*LTR settings*/
    param.bEnableLongTermReference = false;
    param.iLTRRefNum = 0;
    param.iLtrMarkPeriod = 0;

    /* multi-thread settings*/
    param.iMultipleThreadIdc = 4;

    /* Deblocking loop filter */
    param.iLoopFilterDisableIdc = 1;// 0: on, 1: off, 2: on except for slice boundaries
    param.iLoopFilterAlphaC0Offset = 0;
    param.iLoopFilterBetaOffset = 0;

    /*pre-processing feature*/
    param.bEnableDenoise = false;
    param.bEnableBackgroundDetection = false;
    param.bEnableAdaptiveQuant = true;
    param.bEnableFrameCroppingFlag = false;
    param.bEnableSceneChangeDetect = false;
    param.bIsLosslessLink = false;

    svc_encorder->SetOption(ENCODER_OPTION_DATAFORMAT, &videoFormat);
    ret = svc_encorder->InitializeExt(&param);
    return ret;
}


int open_h264_encorder_uninitialize()
{
    int ret = -1;
    if (svc_encorder) {
        ret = svc_encorder->Uninitialize();
    }
    LOGE("%s ret = %d", __FUNCTION__, ret);
    return ret;
}


int open_h264_encorder_start()
{
    int ret = -1;
    if (handle) {
        handle->h264_running = 1;
        //ret = pthread_create(&handle->h264_tid, NULL, h264_working_thread, handle);
    }
    LOGE("%s(), ret = %d", __FUNCTION__, ret);
    return ret;
}


int open_h264_encorder_stop()
{
    int ret = -1;

    if (handle) {
        handle->h264_running = 0;
        if (handle->h264_tid != 0) {
            pthread_join(handle->h264_tid, NULL);
            handle->h264_tid = 0;
            ret = 0;
        }
    }
    LOGE("%s(), ret = %d", __FUNCTION__, ret);

    return ret;
}


//int open_h264_encorder_encode_frame(SSourcePicture* pic, SFrameBSInfo*   bsInfo)
//{
//    int ret = -1;
//    if (svc_encorder) {
//        ret = svc_encorder->EncodeFrame(pic, bsInfo);
//    }
//    return ret;
//}

int open_h264_encodeframe(unsigned char *yuvdata, int yuv_size, uint32_t timestamp, unsigned char *h264out)
{
    int ret = -1, len = 0, i;
    SFrameBSInfo bsInfo = {0};
    SSourcePicture pic = {0};

    if (svc_encorder == NULL)
        return -1;

    pic.iColorFormat = videoFormatI420;
    pic.iPicWidth = handle->width;
    pic.iPicHeight = handle->height;
    pic.iStride[0] = pic.iPicWidth;
    pic.iStride[1] = pic.iStride[2] = pic.iPicWidth / 2;
    pic.uiTimeStamp = timestamp;
    pic.pData[0] = yuvdata;
    pic.pData[1] = yuvdata + handle->width * handle->height;
    pic.pData[2] = yuvdata + handle->width * handle->height * 5 / 4;

    //LOGD("%s yuv:%p, size:%d, width:%d, height:%d", TAG, yuvdata, yuv_size, handle->width, handle->height);

    ret = svc_encorder->EncodeFrame(&pic, &bsInfo);
    if((ret < 0) || (bsInfo.eFrameType == videoFrameTypeSkip) || (bsInfo.eFrameType == videoFrameTypeInvalid)) {
        LOGE("%s <error> EncodeFrame failed, ret:%d, eFrameType:%d", TAG, ret, bsInfo.eFrameType);
        return -1;
    }

#if 0
    if(ret == 0) {
        LOGD("%s iLayerNum:%d, iFrameSizeInBytes:%d, eFrameType:%d, uiTimeStamp:%lu",
        TAG,
        bsInfo.iLayerNum,
        bsInfo.iFrameSizeInBytes,
        bsInfo.eFrameType,
        bsInfo.uiTimeStamp);
    }
#endif

    //取数据方法,IDR帧会有2层,第一层为avc头,第二层为I帧数据
    for (int i = 0; i < bsInfo.iLayerNum; ++i) {
        SLayerBSInfo *pLayerBsInfo = &bsInfo.sLayerInfo[i];
        int frameType = pLayerBsInfo->eFrameType;

#if 0
        LOGD("%s-1 uiTemporalId:%d, uiSpatialId:%d, eFrameType:%d, uiLayerType:%d, iNalCount:%u",
            TAG,
            pLayerBsInfo->uiTemporalId,
            pLayerBsInfo->uiSpatialId,
            pLayerBsInfo->eFrameType,
            pLayerBsInfo->uiLayerType,
            pLayerBsInfo->iNalCount);
        LOGD("%s-2 uiTemporalId:%d, uiSpatialId:%d, eFrameType:%d, uiLayerType:%d, iNalCount:%u",
            TAG,
            bsInfo.sLayerInfo[i].uiTemporalId,
            bsInfo.sLayerInfo[i].uiSpatialId,
            bsInfo.sLayerInfo[i].eFrameType,
            bsInfo.sLayerInfo[i].uiLayerType,
            bsInfo.sLayerInfo[i].iNalCount);
#endif

        if (pLayerBsInfo != NULL) {
            int iLayerSize = 0;
            //IDR帧,NAL数据也会有2个
            int iNalIdx = pLayerBsInfo->iNalCount - 1;
            do {
                iLayerSize += pLayerBsInfo->pNalLengthInByte[iNalIdx];
                --iNalIdx;
            } while (iNalIdx >= 0);

            memcpy(h264out, (char *) ((*pLayerBsInfo).pBsBuf), iLayerSize);
            h264out += iLayerSize;
            len += iLayerSize;
        }
    }
    return len;
}

int open_h264_encorder_set_bitrate(const int bps)
{
    int ret = -1;
    SBitrateInfo Info;

    if (svc_encorder) {
    	Info.iLayer = SPATIAL_LAYER_ALL;
    	Info.iBitrate = bps;
    	g_bitrate = bps;
    	ret = svc_encorder->SetOption(ENCODER_OPTION_BITRATE, (void*)&Info);
    }
    //LOGD("%s(), bps = %d ret = %d", __FUNCTION__, bps, ret);
    return ret;
}


int open_h264_encorder_set_frame_rate(const int frame_rate)
{
    int ret = -1;
    if (svc_encorder) {
    	float get_fr = 0.0;
    	float set_fr = (float) frame_rate;
    	svc_encorder->GetOption(ENCODER_OPTION_FRAME_RATE, &get_fr);
        ret = svc_encorder->SetOption(ENCODER_OPTION_FRAME_RATE, &set_fr);
    	svc_encorder->GetOption(ENCODER_OPTION_FRAME_RATE, &get_fr);
    }
    //LOGD("%s(), fps = %d ret = %d", __FUNCTION__, frame_rate, ret);
    return ret;
}


int open_h264_encorder_idr_request()
{
    int ret = -1;
    if (svc_encorder) {
        ret = svc_encorder->ForceIntraFrame(true);
    }
    //LOGD("%s(), ret = %d", __FUNCTION__, ret);
    return ret;
}

