#include <sys/time.h>
#include <pthread.h>
#include "QStreamRateControl.h"

QStreamRateControl::QStreamRateControl(void)
{
    pthread_mutexattr_t mutexattr;   // Mutex attribute variable

    // Set the mutex as a recursive mutex
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);

    // create the mutex with the attributes set
    pthread_mutex_init(&mClassLock, &mutexattr);

    //After initializing the mutex, the thread attribute can be destroyed
    pthread_mutexattr_destroy(&mutexattr);

    mMaxBitRate = 512;
    mMinBitRate = 50;
    mNormalBitRate = 100;
    mInitialBitRate = mMaxBitRate;//(mNormalBitRate+mMinBitRate)/2;
    Reset();
}

QStreamRateControl::~QStreamRateControl(void)
{
    pthread_mutex_destroy(&mClassLock);
}

void QStreamRateControl::Reset(void)
{
    pthread_mutex_lock(&mClassLock);
    mKbitRate = mMaxBitRate;
    mDecreaseChangeCnt = 0;
    mStartTime = 0;
    mEndTime = 0;
    mTryBandwidthTime = 0;
    mLossEventStartTime = 0;
    mHasLostKbitRate=0;
    mLastLostKbitRate=0;
    mTouchRedLine = 0; 
    mSafeTimes = 0;
    mIsArbitrary = true;

    mPackageLostRate = 0; 
    mRoundTripTime = 0;   
    mPackageLostEvent = 0;
    mLongRTTEvent = 0;
    pthread_mutex_unlock(&mClassLock);
}

void QStreamRateControl::OnPackageLossUpdate(double packageLossRate)
{
    //pthread_mutex_lock(&mClassLock);
    if (mPackageLostEvent == 0 && 
        mPackageLostRate!=0 && 
        (packageLossRate-mPackageLostRate)> (double)0.0001)
    {
        mPackageLostEvent = 1;
        //dxprintf(TEXT("package_loss:%2.9f"),(double)mPackageLostRate); 
    }
    mPackageLostRate = packageLossRate;
    //pthread_mutex_unlock(&mClassLock);
}

void QStreamRateControl::OnRTTUpdate(unsigned long rttus)
{
    //pthread_mutex_lock(&mClassLock);
    if (mRoundTripTime==0)
        mRoundTripTime = rttus;
    else
        mRoundTripTime = 0.9*mRoundTripTime + 0.1*rttus;
    if (mLongRTTEvent == 0)
    {
        if( rttus > 10 * mRoundTripTime )
        {
            mLongRTTEvent = 1;
            //dxprintf(TEXT("RTT:%d"),mRoundTripTime); 
        }/*else
        {
            //Refine for 3.5G connection
            if (rttus>1000000)
            {
                mLongRTTEvent = 1;
            }
        }*/
    }
    //pthread_mutex_unlock(&mClassLock);
}

void QStreamRateControl::OnIPIUpdate(unsigned int ipi){
	mInterPacketInterval = ipi;
}

#define CTRL_PERIOD 400		//ms
#define BITRATE_CTRL_THRESHOLD 5
#define BITRATE_TIME_INTERVAL       200
#define DROP_RATE 0.85  //0.93
#define INCR_RATE 1.10   //1.07
unsigned int QStreamRateControl::GetKbitRate(unsigned char curr_frame_cnt, unsigned char drop_frame_cnt)
{
    pthread_mutex_lock(&mClassLock);
    int tfrc_kbitRate = 11200000 / mInterPacketInterval;
    int try_bandwidth_period = 15000;
    if (mIsArbitrary && (mPackageLostEvent!=0 || mLongRTTEvent!=0 || mKbitRate > mNormalBitRate))
    {
        //dxprintf(TEXT("RateControl: loss_flag:%d,rtt_flag:%d,bitrate:%d"),mPackageLostEvent,mLongRTTEvent,mKbitRate);
        mIsArbitrary = false;
    }
    
    if(mKbitRate == 0) {
        mKbitRate = mMaxBitRate;
    }
    
    if(mStartTime == 0) {
        mStartTime = CurrentTimeMs();
    } 

    if(mTryBandwidthTime == 0) {
        mTryBandwidthTime = CurrentTimeMs();
    }
    
    mEndTime = CurrentTimeMs();

    if( mKbitRate > mNormalBitRate)
        try_bandwidth_period = 30000;
    else
        try_bandwidth_period = 15000;

    if( (mEndTime - mStartTime) >=CTRL_PERIOD){
        /* use a counter to record the latest network situation  */
        if(  tfrc_kbitRate < mKbitRate){
            mCount_BRC_TFRC_Compare++;
            mCount_BRC_TFRC_Compare = mCount_BRC_TFRC_Compare > BITRATE_CTRL_THRESHOLD ? BITRATE_CTRL_THRESHOLD: mCount_BRC_TFRC_Compare;
        }else{
            mCount_BRC_TFRC_Compare--;
            mCount_BRC_TFRC_Compare = mCount_BRC_TFRC_Compare < 0 ? 0 : mCount_BRC_TFRC_Compare;
        }
    }

    if( (mEndTime - mStartTime) >= CTRL_PERIOD && tfrc_kbitRate < mKbitRate){
        mTouchRedLine = 0;
        //mKbitRate = (mKbitRate*DROP_RATE)-1;
        mKbitRate = tfrc_kbitRate;
        mDecreaseChangeCnt = 0;
        mSafeTimes = 0;
        mTryBandwidthTime = mEndTime;
        mCount_BRC_TFRC_Compare -= 2;
    }




    if((mEndTime - mStartTime) >= CTRL_PERIOD) {
        if( (mPackageLostEvent == 1) || (mLongRTTEvent == 1) ){
//            mLastLostKbitRate = mKbitRate;
//            mHasLostKbitRate = 1;
//            mTouchRedLine = 0;
            mKbitRate = (mKbitRate*DROP_RATE)-1;
//            mDecreaseChangeCnt = 0;
            if(mPackageLostEvent == 1){
                mPackageLostEvent = 2;   
#if INTELLIGENT_DEBUG_MSG            
                printf("\n[%u] ^_^ (%2.2f) [mKbitRate=%d]  ", mEndTime, mPackageLostEvent, mKbitRate);
#endif                
            }
            else {
                mLongRTTEvent = 2;
#if INTELLIGENT_DEBUG_MSG            
                printf("\n[%u] +_+ (long_rtt) [mKbitRate=%d]  ", mEndTime, mKbitRate);
#endif                
            }
            mSafeTimes = 0;
            mLossEventStartTime = CurrentTimeMs();

        }

	mStartTime = CurrentTimeMs();

        if((mEndTime - mTryBandwidthTime) > try_bandwidth_period) {
        mKbitRate = (mKbitRate * INCR_RATE)+10;
        if(mHasLostKbitRate){
            if(mKbitRate > mLastLostKbitRate){
                mTouchRedLine++;
                if(mTouchRedLine < 2)
                {
                    mKbitRate = (mLastLostKbitRate / INCR_RATE) - 1;
                }
                else
                {
                    mTouchRedLine = 0;
                    if(mSafeTimes < 5) {
                        mSafeTimes++;
                    }
                    mLastLostKbitRate =  mLastLostKbitRate * ( 1 + (0.1 * mSafeTimes) );
                }
           }
        }
#if INTELLIGENT_DEBUG_MSG
        //printf("raise the PL330's bit rate [mKbitRate=%d] (red_line=%d, %d, safe=%d)\n", mKbitRate, mLastLostKbitRate, mTouchRedLine, mSafeTimes);
#endif

        mTryBandwidthTime = CurrentTimeMs();
        if(mKbitRate > mMaxBitRate)
            mKbitRate = mMaxBitRate;
        if(mLastLostKbitRate > mMaxBitRate)
            mLastLostKbitRate = mMaxBitRate;
        }
    }


/*

        else if((drop_frame_cnt > 0)||(curr_frame_cnt > 4)) {
            mDecreaseChangeCnt++;
            if(mDecreaseChangeCnt >= 20) {
                mHasLostKbitRate = 1;
                mLastLostKbitRate = mKbitRate;
                mTouchRedLine = 0;
                mKbitRate = (mKbitRate*DROP_RATE)-1;
                mDecreaseChangeCnt = 0;
                mSafeTimes = 0;
#if INTELLIGENT_DEBUG_MSG                
                if(curr_frame_cnt > 4) printf("\n[%u] @_@ (%d) [mKbitRate=%d]  ", mEndTime, curr_frame_cnt, mKbitRate);
                else printf("\n[%u] >_< [mKbitRate=%d]  ", mEndTime, mKbitRate);
#endif
            }
        }
       
        mStartTime = CurrentTimeMs();

        if((mEndTime - mTryBandwidthTime) > try_bandwidth_period) {
        mKbitRate = (mKbitRate * INCR_RATE)+10;
        if(mHasLostKbitRate)
        {
            if(mKbitRate > mLastLostKbitRate)
           {
                mTouchRedLine++;
                if(mTouchRedLine < 2)
                {
                    mKbitRate = (mLastLostKbitRate / INCR_RATE) - 1;
                }
                else
                {
                    mTouchRedLine = 0;
                    if(mSafeTimes < 5) {
                        mSafeTimes++;
                    }
                    mLastLostKbitRate =  mLastLostKbitRate * ( 1 + (0.1 * mSafeTimes) );
                }
           }
        }
#if INTELLIGENT_DEBUG_MSG
        //printf("raise the PL330's bit rate [mKbitRate=%d] (red_line=%d, %d, safe=%d)\n", mKbitRate, mLastLostKbitRate, mTouchRedLine, mSafeTimes);
#endif

        mTryBandwidthTime = CurrentTimeMs();
        if(mKbitRate > mMaxBitRate)
            mKbitRate = mMaxBitRate;
        if(mLastLostKbitRate > mMaxBitRate)
            mLastLostKbitRate = mMaxBitRate;
        }
    }

//    if((mEndTime - mTryBandwidthTime) > 5000) {
//        mKbitRate = (mKbitRate*1.05);
//        //pl330_bit_rate_change_mode(imPtr, FAST_CHANGE_MODE);
//        printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> $_$ (x1.01) [mKbitRate=%d]\n", mKbitRate);
//        mTryBandwidthTime = CurrentTimeMs();
//    }
*/
    if(mPackageLostEvent == 2) {
        if((CurrentTimeMs() - mLossEventStartTime) > CTRL_PERIOD) {
            mPackageLostEvent = 0;
        }
    }
    
    if(mLongRTTEvent == 2) {
        if((CurrentTimeMs() - mLossEventStartTime) > CTRL_PERIOD) {
            mLongRTTEvent = 0;
        }
    }    
    
    //if(pl330_check_bit_rate_mode(imPtr) == FAST_CHANGE_MODE) {
    if (mIsArbitrary)
    {
        if((mEndTime - mTryBandwidthTime) > 5000)
        {
            mKbitRate = mKbitRate*INCR_RATE+10;

            mTryBandwidthTime = CurrentTimeMs();
        }
    }
    //}

    if(mKbitRate < mMinBitRate)
        mKbitRate = mMinBitRate;
    if (mKbitRate > mMaxBitRate)
        mKbitRate = mMaxBitRate;
    pthread_mutex_unlock(&mClassLock);
    return mKbitRate;
}
unsigned char QStreamRateControl::GetFrameRate(unsigned int encodeKbitRate)
{
    unsigned char result = 0;
    result = (15-3)*(encodeKbitRate-mMinBitRate)/((mMaxBitRate+mNormalBitRate+1)/2-mMinBitRate+1)+3;
    if (result < 3)
        result = 3;
    if (result > 15)
        result = 15;

    if (result>=3 && result<5)
        result = 3;
    else if (result>=5 && result<7)
        result = 5;
    else if (result>=7 && result<9)
        result = 7;
    else if (result>=9 && result<11)
        result = 9;
    else if (result>=11 && result<13)
        result = 11;
    else if (result>=13 && result<17)
        result = 13;
    else if (result>=17)
        result = 15;
    return result;
}
unsigned long QStreamRateControl::CurrentTimeMs()
{
    struct timeval          curr_time;
    gettimeofday(&curr_time, NULL);
    return curr_time.tv_sec*1000+curr_time.tv_usec/1000;
}
void QStreamRateControl::SetBitRateBound(int maxBitRate,int minBitRate,int normlBitRate, int initialBitrate)
{
    if (maxBitRate>0 && minBitRate>0 && normlBitRate>0)
    {
        mMaxBitRate = maxBitRate;
        mMinBitRate = minBitRate;
        mNormalBitRate = normlBitRate;
        mInitialBitRate = initialBitrate;
        Reset();
    }
}
