
#include <sys/time.h>
#include <pthread.h>
#include "QStreamStatistics.h"

const int STATISTICS_LIST_SIZE = 88;
const int STATISTICS_GAP_MS = 1000;
CQStreamStatistics::CQStreamStatistics(void)
{
	pthread_mutexattr_t mutexattr;   // Mutex attribute variable

	// Set the mutex as a recursive mutex
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);

	// create the mutex with the attributes set
	pthread_mutex_init(&mSendLock, &mutexattr);
	pthread_mutex_init(&mRecvLock, &mutexattr);
//	pthread_mutex_init(&mRTTLock, &mutexattr);

	//After initializing the mutex, the thread attribute can be destroyed
	pthread_mutexattr_destroy(&mutexattr);
    Reset();
}

CQStreamStatistics::~CQStreamStatistics(void)
{
    pthread_mutex_destroy(&mSendLock);
    pthread_mutex_destroy(&mRecvLock);
//    pthread_mutex_destroy(&mRTTLock);
}
unsigned long CQStreamStatistics::CurrentTimeMs()
{
    struct timeval          curr_time;
    gettimeofday(&curr_time, NULL);
    return curr_time.tv_sec*1000+curr_time.tv_usec/1000;
}
void CQStreamStatistics::OnSendFrameOK(uint32_t length)
{
    //Calculate the sending frame rate
    pthread_mutex_lock(&mSendLock);
    if (mListTimeSendFrame.size()>=STATISTICS_LIST_SIZE)
        mListTimeSendFrame.pop_back();
    if (mListLengthSendFrame.size()>=STATISTICS_LIST_SIZE)
    {
       mListLengthSendFrame.pop_back();
    }
    mListLengthSendFrame.push_front(length*8);    //convert to bit
    mListTimeSendFrame.push_front(CurrentTimeMs());
    pthread_mutex_unlock(&mSendLock);
}
void CQStreamStatistics::OnRecvFrameOK(uint32_t length)
{
     //Calculate the recving frame rate
    pthread_mutex_lock(&mRecvLock);
    if (mListTimeRecvFrame.size()>=STATISTICS_LIST_SIZE)
        mListTimeRecvFrame.pop_back();
    if (mListLengthRecvFrame.size()>=STATISTICS_LIST_SIZE)
        mListLengthRecvFrame.pop_back();
    mListLengthRecvFrame.push_front(length*8);  //convert to bit
    mListTimeRecvFrame.push_front(CurrentTimeMs());
    pthread_mutex_unlock(&mRecvLock);

}
void CQStreamStatistics::OnRTTUpdate(uint32_t rttus)
{
     //Calculate the recving frame rate
    if (mRTT==0)
        mRTT = rttus;
    else
        mRTT = 0.9 * mRTT + 0.1 * rttus;
//    pthread_mutex_lock(&mRTTLock);
//    if (mListValueRTT.size()>=STATISTICS_LIST_SIZE/2)
//        mListValueRTT.pop_back();
//    if (mListTimeRTT.size()>=STATISTICS_LIST_SIZE/2)
//        mListTimeRTT.pop_back();
//    mListValueRTT.push_front(rtt);
//    mListTimeRTT.push_front(CurrentTimeMs());
//    pthread_mutex_unlock(&mRTTLock);
}
void CQStreamStatistics::OnPackageLossUpdate(double packageLossRate)
{
    mPackageLostRate = packageLossRate;
}
double CQStreamStatistics::GetPackageLossRate()
{
    return mPackageLostRate;
}

void CQStreamStatistics::Reset()
{
    pthread_mutex_lock(&mRecvLock);
    mListTimeRecvFrame.clear();
    mListLengthRecvFrame.clear();
    pthread_mutex_unlock(&mRecvLock);
    pthread_mutex_lock(&mSendLock);
    mListTimeSendFrame.clear();
    mListLengthSendFrame.clear();
    pthread_mutex_unlock(&mSendLock);
//    pthread_mutex_lock(&mRTTLock);
//    mListValueRTT.clear();
//    mListTimeRTT.clear();
//    pthread_mutex_unlock(&mRTTLock);
    mPackageLostRate = 0;
    mRTT = 0;
}
unsigned long CQStreamStatistics::GetRecvBitrate()
{
    unsigned long result = 0;
    unsigned long now_ms = CurrentTimeMs();
    unsigned long gap = 1;
    pthread_mutex_lock(&mRecvLock);
    if (!mListLengthRecvFrame.empty())
    {   //Remove the data in queue more than three seconds ago
        unsigned long accepted_ms = now_ms-STATISTICS_GAP_MS;
        while(accepted_ms>mListTimeRecvFrame.back())
        {
            mListTimeRecvFrame.pop_back();
            mListLengthRecvFrame.pop_back();
            if (mListTimeRecvFrame.empty())
                break;
        }
        if (!mListTimeRecvFrame.empty())
        {
            unsigned long oldest_ms = mListTimeRecvFrame.back();
            gap = (now_ms - oldest_ms + 1);
            for(List<unsigned long>::iterator list_iter = mListLengthRecvFrame.begin();
                list_iter != mListLengthRecvFrame.end(); 
                list_iter++)
            {
                result += (*list_iter);
            }
        }
    }
    pthread_mutex_unlock(&mRecvLock);
    return result/gap;
}
unsigned char CQStreamStatistics::GetRecvFramerate()
{
    unsigned char result = 0;
    unsigned long now_ms = CurrentTimeMs();
    pthread_mutex_lock(&mRecvLock);
    if (!mListTimeRecvFrame.empty())
    {   //Remove the data in queue more than three seconds ago
        unsigned long accepted_ms = now_ms-STATISTICS_GAP_MS;
        while(accepted_ms>mListTimeRecvFrame.back())
        {
            mListTimeRecvFrame.pop_back();
            mListLengthRecvFrame.pop_back();
            if (mListTimeRecvFrame.empty())
                break;
        }
        if (!mListTimeRecvFrame.empty())
        {
            unsigned long oldest_ms = mListTimeRecvFrame.back();
            unsigned long gap = (now_ms - oldest_ms + 1);
            result = (mListTimeRecvFrame.size()*1000)/gap;
        }
    }
    pthread_mutex_unlock(&mRecvLock);
    return result;
}
unsigned long CQStreamStatistics::GetSendBitrate()
{
    unsigned long result = 0;
    unsigned long now_ms = CurrentTimeMs();
    unsigned long gap = 1;
    pthread_mutex_lock(&mSendLock);
    if (!mListLengthSendFrame.empty())
    {   //Remove the data in queue more than three seconds ago
        unsigned long accepted_ms = now_ms-STATISTICS_GAP_MS;
        while(accepted_ms>mListTimeSendFrame.back())
        {
            mListTimeSendFrame.pop_back();
            mListLengthSendFrame.pop_back();
            if (mListTimeSendFrame.empty())
                break;
        }
        if (!mListTimeSendFrame.empty())
        {
            unsigned long oldest_ms = mListTimeSendFrame.back();
            gap = (now_ms - oldest_ms + 1);
            for(List<unsigned long>::iterator list_iter = mListLengthSendFrame.begin();
                list_iter != mListLengthSendFrame.end(); 
                list_iter++)
            {
                result += (*list_iter);
            }
        }
    }
    pthread_mutex_unlock(&mSendLock);
    
    return result/gap;
}
unsigned char CQStreamStatistics::GetSendFramerate()
{
    unsigned char result = 0;
    unsigned long now_ms = CurrentTimeMs();
    pthread_mutex_lock(&mSendLock);
    if (!mListTimeSendFrame.empty())
    {   //Remove the data in queue more than three seconds ago
        unsigned long accepted_ms = now_ms-STATISTICS_GAP_MS;
        while(accepted_ms>mListTimeSendFrame.back())
        {
            mListTimeSendFrame.pop_back();
            mListLengthSendFrame.pop_back();
            if (mListTimeSendFrame.empty())
                break;
        }
        if (!mListTimeSendFrame.empty())
        {
            unsigned long oldest_ms = mListTimeSendFrame.back();
            unsigned long gap = (now_ms - oldest_ms + 1);
            result = (mListTimeSendFrame.size()*1000)/gap;
        }
    }
    pthread_mutex_unlock(&mSendLock);
    return result;
}

unsigned long CQStreamStatistics::GetRTTus()
{
    return mRTT;
//    unsigned long result = 0;
//    unsigned long now_ms = CurrentTimeMs();
//    unsigned long gap = 1;
//    pthread_mutex_lock(&mRTTLock);
//    if (!mListValueRTT.empty())
//    {
//        //Remove the data in queue more than three seconds ago
//        unsigned long accepted_ms = now_ms-STATISTICS_GAP_MS;
//        while(accepted_ms>mListTimeRTT.back())
//        {
//            mListTimeRTT.pop_back();
//            mListValueRTT.pop_back();
//            if (mListTimeRTT.empty())
//                break;
//        }
//        if (!mListTimeRTT.empty())
//        {
//            gap = mListValueRTT.size();
//            for(List<unsigned long>::iterator list_iter = mListValueRTT.begin();list_iter != mListValueRTT.end(); list_iter++)
//            {
//                result += (*list_iter);
//            }
//        }
//    }
//    pthread_mutex_unlock(&mRTTLock);
//
//    return result/gap;
}
