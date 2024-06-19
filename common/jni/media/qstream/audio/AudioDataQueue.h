#pragma once
#include "AudioData.h"

class CAudioDataQueue
{
public:
    CAudioDataQueue(const unsigned int size);
    ~CAudioDataQueue(void);
    unsigned int Count();
    unsigned int Size();
    bool PushData(const unsigned char* pData,const unsigned int dataLength, const unsigned int timestamp);
    CAudioData* NextData();
    void PopData();
    void Reset();
private:
    CAudioData  *mpFrameDataQueue;
    unsigned int mQueueSize;
    unsigned int mStart;
    unsigned int mEnd;
};
inline void CAudioDataQueue::Reset()
{
    mStart = 0;
    mEnd = 0;
}
inline unsigned int CAudioDataQueue::Size()
{
    return mQueueSize;
}
inline unsigned int CAudioDataQueue::Count()
{
    if (mEnd>=mStart)
    {
        return (mEnd - mStart);
    }else
    {
        return mEnd + (mQueueSize-mStart);
    }
}
