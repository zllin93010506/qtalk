#pragma once
//
//#define QS_MAX_KBIT_RATE                    3000
//#define QS_MIN_KBIT_RATE                    200
//#define QS_NORMAL_KBIT_RATE                 2000
class QStreamRateControl
{
public:
    QStreamRateControl(void);
    ~QStreamRateControl(void);
    void OnPackageLossUpdate(double pkLoss);
    void OnRTTUpdate(unsigned long rttus);
    void OnIPIUpdate(unsigned int);
    void SetBitRateBound(int maxBitRate,int minBitRate,int normlBitRate, int initialBitRate);
    

    unsigned int GetKbitRate(unsigned char currFrameCnt,
                             unsigned char dropFrameCnt);

    unsigned char GetFrameRate(unsigned int encodeKbitRate);
    void Reset();
private:
    pthread_mutex_t         mClassLock;
    //Rate Control
    double                  mPackageLostRate;   //Record the package loss rate for rate control
    unsigned long           mRoundTripTime;     //Record the round trip time for rate control
    unsigned int            mInterPacketInterval;//Record the Inter-packet interval for rate control
    int                     mCount_BRC_TFRC_Compare;//counter used in bit rate control algorithm
    unsigned char           mPackageLostEvent;
    unsigned char           mLongRTTEvent;
    //Flag be enabled in the begining of Session, which enable the bitrate increase quickly
    bool                    mIsArbitrary;
    //Variable for Rate Control 
    int                     mKbitRate; 
    int                     mDecreaseChangeCnt;
    unsigned long           mStartTime;
    unsigned long           mEndTime;
    unsigned long           mTryBandwidthTime;
    unsigned long           mLossEventStartTime;
	int                     mHasLostKbitRate;
    int                     mLastLostKbitRate;
    int                     mTouchRedLine; 
	int                     mSafeTimes;

    //Function to get current timestamp
    unsigned long           CurrentTimeMs();

    int                     mMaxBitRate;
    int                     mMinBitRate;
    int                     mNormalBitRate;
    int                     mInitialBitRate;
};
