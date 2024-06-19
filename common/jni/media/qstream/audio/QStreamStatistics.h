#pragma once

#include "List.h"

class CQStreamStatistics
{
public:
    CQStreamStatistics(void);
    ~CQStreamStatistics(void);
    void OnSendFrameOK(uint32_t length);
    void OnRecvFrameOK(uint32_t length);
    void OnPackageLossUpdate(double packageLossRate);
    void OnRTTUpdate(uint32_t rtt);
    void Reset();

    unsigned long GetRecvBitrate();
    unsigned char GetRecvFramerate();
    unsigned long GetSendBitrate();
    unsigned char GetSendFramerate();
    double        GetPackageLossRate();
    unsigned long GetRTTus();

private:
    //Statistical Report parameter
    //Recv
    List<unsigned long>    mListTimeRecvFrame;
    List<unsigned long>    mListLengthRecvFrame;
    pthread_mutex_t        mRecvLock;
    //Send
    List<unsigned long>    mListTimeSendFrame;
    List<unsigned long>    mListLengthSendFrame;
    pthread_mutex_t        mSendLock;
    //Package Loss
    double                 mPackageLostRate;   //Record the package loss rate for rate control
    //RTT
    List<unsigned long>    mListTimeRTT;
    List<unsigned long>    mListValueRTT;
    pthread_mutex_t        mRTTLock;
    unsigned long          mRTT;

    unsigned long          CurrentTimeMs();
};
