#pragma once
#include "FrameData.h"

class CFrameDataQueue
{
public:
    CFrameDataQueue(const unsigned int size);
    ~CFrameDataQueue(void);
    unsigned int Count();
    unsigned int Size();
    bool PushData(const unsigned char* pData,const unsigned int dataLength, const unsigned int timestamp, const unsigned short rotation);
    CFrameData* NextData();
    void PopData();
    void Reset();
private:
    CFrameData  *mpFrameDataQueue;
    unsigned int mQueueSize;
    unsigned int mStart;
    unsigned int mEnd;
};
inline void CFrameDataQueue::Reset()
{
    mStart = 0;
    mEnd = 0;
}
inline unsigned int CFrameDataQueue::Size()
{
    return mQueueSize;
}
inline unsigned int CFrameDataQueue::Count()
{
    if (mEnd>=mStart)
    {
        return (mEnd - mStart);
    }else
    {
        return mEnd + (mQueueSize-mStart);
    }
}
