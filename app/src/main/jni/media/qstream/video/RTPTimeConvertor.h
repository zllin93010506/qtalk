#pragma once

class CRTPTimeConvertor
{
public:
    CRTPTimeConvertor(void);
    ~CRTPTimeConvertor(void);
    void SetSampleRate(uint32_t sampleRate);
    uint32_t RtpTime2NTPClock(uint32_t rtp_timestamp,
                                long *p_clock_sec,
                                long *p_clock_usec);
    void SetReferenceTime(uint32_t ref_rtp_timestamp,
                        long ref_clock_sec,
                        long ref_clock_usec);

private:
    uint32_t        mSampleRate;

    pthread_mutex_t mSrStampLock;   // Mutex

    uint32_t        mSrTimestamp;
    long            mSrClockSec;
    long            mSrClockUsec;
};
inline void CRTPTimeConvertor::SetSampleRate(uint32_t sampleRate)
{
    mSampleRate = sampleRate;
}
