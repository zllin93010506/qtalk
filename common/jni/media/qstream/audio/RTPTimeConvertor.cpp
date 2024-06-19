
#include <pthread.h>
#include "RTPTimeConvertor.h"

CRTPTimeConvertor::CRTPTimeConvertor(void)
{
    mSampleRate = 0;
    mSrClockSec = 0;
    mSrClockUsec = 0;
    mSrTimestamp = 0;
    pthread_mutexattr_t mutexattr;   // Mutex attribute variable

    // Set the mutex as a recursive mutex
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);

    // create the mutex with the attributes set
    pthread_mutex_init(&mSrStampLock, &mutexattr);

    //After initializing the mutex, the thread attribute can be destroyed
    pthread_mutexattr_destroy(&mutexattr);
}

CRTPTimeConvertor::~CRTPTimeConvertor(void)
{
    pthread_mutex_destroy(&mSrStampLock);
}


void clock_time_minus(struct timeval *time1, struct timeval *time2, struct timeval *result)
{
    result->tv_sec = time1->tv_sec - time2->tv_sec;
    result->tv_usec = time1->tv_usec - time2->tv_usec;

    if (result->tv_usec < 0) {
        result->tv_sec -= 1;
        result->tv_usec += 1000000;
    }

    if (result->tv_usec >= 1000000) {
        result->tv_sec += 1;
        result->tv_usec -= 1000000;
    }
}

void clock_time_add(struct timeval *time1, struct timeval *time2, struct timeval *result)
{
    result->tv_sec = time1->tv_sec + time2->tv_sec;
    result->tv_usec = time1->tv_usec + time2->tv_usec;

    if (result->tv_usec < 0) {
    result->tv_sec -= 1;
    result->tv_usec += 1000000;
    }

    if (result->tv_usec >= 1000000) {
    result->tv_sec += 1;
    result->tv_usec -= 1000000;
    }
}

void convert_rtp_timestamp_offset_into_clock_time_offset(uint32_t sampling_rate,uint32_t rtp_timestamp_offset, struct timeval *clock_time_offset)
{
    clock_time_offset->tv_sec = rtp_timestamp_offset / sampling_rate;
    clock_time_offset->tv_usec = (long)((double)(rtp_timestamp_offset % sampling_rate) * 1.0e6 / (double)sampling_rate);
}


void CRTPTimeConvertor::SetReferenceTime(uint32_t ref_rtp_timestamp,
                                        long ref_clock_sec,
                                        long ref_clock_usec)
{
    
    pthread_mutex_lock(&mSrStampLock);
    mSrTimestamp = ref_rtp_timestamp;
    mSrClockSec = ref_clock_sec;
    mSrClockUsec = ref_clock_usec;
    pthread_mutex_unlock(&mSrStampLock);
}
uint32_t CRTPTimeConvertor::RtpTime2NTPClock(uint32_t rtp_timestamp,
                                        long *p_clock_sec,
                                        long *p_clock_usec)
{
    uint32_t rtp_timestamp_offset;
    struct timeval clock_time_offset; 
    struct timeval clock_time_result; 
    struct timeval clock_time_ref;
    pthread_mutex_lock(&mSrStampLock);
    clock_time_ref.tv_sec = mSrClockSec;
    clock_time_ref.tv_usec = mSrClockUsec;
    if (mSrClockSec!=0 || mSrClockUsec!=0 || mSrTimestamp!=0)
    {
        if ( ( (rtp_timestamp - mSrTimestamp) & 0x80000000 )!= 0) {/*rtp_timestamp < mSrTimestamp*/
            rtp_timestamp_offset = mSrTimestamp - rtp_timestamp;
            convert_rtp_timestamp_offset_into_clock_time_offset(mSampleRate,rtp_timestamp_offset, &clock_time_offset);
            clock_time_minus(&clock_time_ref, &clock_time_offset, &clock_time_result);
        }
        else {  /*rtp_timestamp >= mSrTimestamp*/
            rtp_timestamp_offset = rtp_timestamp - mSrTimestamp;
            convert_rtp_timestamp_offset_into_clock_time_offset(mSampleRate,rtp_timestamp_offset, &clock_time_offset);
            clock_time_add(&clock_time_ref, &clock_time_offset, &clock_time_result);
        }
    }else
    {
        clock_time_result.tv_sec = 0;
        clock_time_result.tv_usec = 0;
    }
    //dxprintf(TEXT("ts:%u=>%ld\t%ld\t[ref:ts:%u=>%ld\t%ld]\t[offset:%ld\t%ld]"),
    //                rtp_timestamp,
    //                clock_time_result.tv_sec,
    //                clock_time_result.tv_usec,
    //                mSrTimestamp,
    //                clock_time_ref.tv_sec,
    //                clock_time_ref.tv_usec,
    //                clock_time_offset.tv_sec,
    //                clock_time_offset.tv_usec);
    pthread_mutex_unlock(&mSrStampLock);
    *p_clock_sec = clock_time_result.tv_sec;
    *p_clock_usec = clock_time_result.tv_usec;
    return (clock_time_result.tv_sec*1000+clock_time_result.tv_usec/1000);
}
