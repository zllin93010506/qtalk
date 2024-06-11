#pragma once

#ifndef LIP_SYNC_TOKEN 
#define LIP_SYNC_TOKEN

//typedef unsigned long long int uint64_t;
//typedef unsigned long int uint32_t;
typedef struct{
	uint64_t audio_new_rtp_ts;
    uint64_t video_new_rtp_ts;
} LipSyncToken;
#endif

class CQStreamLipSyncControl
{
public:
    CQStreamLipSyncControl(void);
    ~CQStreamLipSyncControl(void);
    void SetLipSyncToken(LipSyncToken* pToken);

    void OnRTTUpdate(unsigned long msRTT);
    void OnUpdateFrameRate(unsigned char fpsFrameRate);

    bool CheckBufferSize(bool gotFirstIFrame,uint32_t recvBufferSize, unsigned int decodeBufferSize, bool thisIsIFrame);
    bool CheckFrameTimeStamp(long captureClockSec,long captureClockUsec);
private:
    unsigned long mRTTms;
    unsigned char mFrameRatefps;
    
    //LipSync
    LipSyncToken* mpExternLipSyncToken;
};
inline void CQStreamLipSyncControl::SetLipSyncToken(LipSyncToken* pToken)
{
    mpExternLipSyncToken = pToken;
}
inline void CQStreamLipSyncControl::OnRTTUpdate(unsigned long rttms)
{
    if (mRTTms==0)
        mRTTms = rttms;
    else if (rttms)
        mRTTms = 0.9 * mRTTms + 0.1 * rttms;
    //mRTTms = msRTT;
}
inline void CQStreamLipSyncControl::OnUpdateFrameRate(unsigned char fpsFrameRate)
{
    mFrameRatefps = fpsFrameRate;
}
