#include <stdlib.h>
#include "AudioDataQueue.h"
//#include "DbgUtility.h"
//#define dxprintf
//#define trace_printf
CAudioDataQueue::CAudioDataQueue(const unsigned int size)
{
    mpFrameDataQueue = new CAudioData[size];
    mQueueSize = size;
    mStart = 0;
    mEnd = 0;
}

CAudioDataQueue::~CAudioDataQueue(void)
{
    delete [] mpFrameDataQueue;      
}

bool CAudioDataQueue::PushData(const unsigned char* pData,const unsigned int dataLength, const unsigned int timestamp)
{
    //trace_printf("CAudioDataQueue::PushData start:%d,end:%d",mStart,mEnd);
    if (Count()<mQueueSize)
    {
        mpFrameDataQueue[mEnd].SetData(pData,dataLength,timestamp);
        mEnd = (mEnd+1)%mQueueSize;
        return true;
    }else
        return false;
}
CAudioData* CAudioDataQueue::NextData()
{
    if (Count()>0)
    {
        return &mpFrameDataQueue[mStart];
    }else
        return NULL;
    
}
void CAudioDataQueue::PopData()
{
    //trace_printf(TEXT"CAudioDataQueue::PopData start:%d,end:%d",mStart,mEnd);
    if (Count()>0)
    {
        mStart = (mStart+1)%mQueueSize;
    }
}
