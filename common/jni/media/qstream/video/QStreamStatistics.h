#pragma once

#include "List.h"

class CQStreamStatistics
{
public:
    CQStreamStatistics(void);
    ~CQStreamStatistics(void);
    void OnSendFrameOK(uint32_t length);
    void OnRecvFrameOK(uint32_t length);
    void OnRecvLost(uint32_t seqNum,uint32_t count);
    void OnRTTUpdate(uint32_t rtt);
    void Reset();

    unsigned long GetRecvBitrate();
    unsigned char GetRecvFramerate();
    unsigned long GetSendBitrate();
    unsigned char GetSendFramerate();
    double        GetRtpRecvLostRate();
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
    
    //Packet Lost
    List<unsigned long>    mListTimeRtpRecvLost;
    List<unsigned long>    mListCountRtpRecvLost;
    List<unsigned long>    mListSeqNumRtpRecvLost;
    pthread_mutex_t        mRtpRecvLostLock;
    
    //RTT
    List<unsigned long>    mListTimeRTT;
    List<unsigned long>    mListValueRTT;
    pthread_mutex_t        mRTTLock;
    unsigned long          mRTT;

    unsigned long          CurrentTimeMs();
};
