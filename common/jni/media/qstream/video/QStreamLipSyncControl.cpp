#include "stdlib.h"
#include "QStreamLipSyncControl.h"
#include "logging.h"

#define TAG "CQStreamLipSyncControl"
#define LOGV(...) //__logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) //__logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) //__logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) //__logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

CQStreamLipSyncControl::CQStreamLipSyncControl(void)
{
    mpExternLipSyncToken = 0;
    mFrameRatefps = 30;
    mRTTms = 500;//500ms
}

CQStreamLipSyncControl::~CQStreamLipSyncControl(void)
{
}

bool CQStreamLipSyncControl::CheckBufferSize(bool gotFirstIFrame,uint32_t recvBufferSize, unsigned int decodeBufferSize, bool thisIsIFrame)
{
    /*int bonus = 0;
    if (mFrameRatefps<1)
        bonus = 5;
    else if (mFrameRatefps<3)
        bonus = 3;
    else if (mFrameRatefps<5)
        bonus = 2;
    else
        bonus = 1;
    if (!gotFirstIFrame)
        bonus += 10;
    else if (thisIsIFrame)
        bonus += mFrameRatefps;

    if (decodeBufferSize>1 && recvBufferSize>(mFrameRatefps/4+bonus))// || currentRecvSize>(mFrameRatefps+10)))
    {
        LOGE("[VIDEOENGINE]LipSync Drop Video => RecvBuffer size : %d > 20+%d decodeBufferSize:%d",recvBufferSize,bonus,decodeBufferSize);
        return false;
    }else if (decodeBufferSize>0 && recvBufferSize>(mFrameRatefps/3+bonus))
    {
        LOGE("[VIDEOENGINE]LipSync Drop Video => RecvBuffer size : %d > 10+%d decodeBufferSize:%d",recvBufferSize,bonus,decodeBufferSize);
        return false;
    }else if (decodeBufferSize==0 && recvBufferSize>(mFrameRatefps/2+bonus))
    {
        LOGE("[VIDEOENGINE]LipSync Drop Video => RecvBuffer size : %d > 6+%d decodeBufferSize:%d",recvBufferSize,bonus,decodeBufferSize);
        return false;
    }*/
    if (recvBufferSize>5)
        return false;
    return true;
}
bool CQStreamLipSyncControl::CheckFrameTimeStamp(long captureClockSec,long captureClockUsec)
{
    bool result = true;
//    if (mpExternLipSyncToken)
//    {
//        mpExternLipSyncToken->video_new_rtp_ts = (uint64_t)(((double)captureClockSec*1000)+
//                                                     ((double)captureClockUsec/1000)); //tranform to minisec
//
//        long long gap_ms = mpExternLipSyncToken->video_new_rtp_ts - mpExternLipSyncToken->audio_new_rtp_ts;
//        //if (captureClockSec>mpExternLipSyncToken->audio_recv_clock_sec)
//        if (mpExternLipSyncToken->video_new_rtp_ts!=0 &&            //video_new_rtp_ts==0 means we don't recv any video data
//            mpExternLipSyncToken->audio_new_rtp_ts!=0 &&            //audio_new_rtp_ts==0 means we don't recv any audio data
//            gap_ms > 0)
//        {
//            /*unsigned __int32 gap = (unsigned __int32)(((double)(captureClockSec - mpExternLipSyncToken->audio_recv_clock_sec)*1000)+
//                                                     ((double)(captureClockUsec- mpExternLipSyncToken->audio_recv_clock_usec)/1000));*/
//            if ( gap_ms > (mRTTms+200))   //let gap behind RTT;
//            {
//                LOGD("[VIDEOENGINE]LipSync Drop Video => VideoTimeStamp > AudioTimeStamp : %u > %u %u>%u",mpExternLipSyncToken->video_new_rtp_ts,
//                                                                                                       mpExternLipSyncToken->audio_new_rtp_ts,
//                                                                                                       gap_ms,mRTTms);
//                return false;   //Drop Frame
//            }
//        }
//    }
    return result;
}
