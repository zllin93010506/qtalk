#include <stdlib.h>
#include "FrameDataQueue.h"
//#include "DbgUtility.h"
//#define dxprintf
//#define trace_printf
CFrameDataQueue::CFrameDataQueue(const unsigned int size)
{
    mpFrameDataQueue = new CFrameData[size];
    mQueueSize = size;
    mStart = 0;
    mEnd = 0;
}

CFrameDataQueue::~CFrameDataQueue(void)
{
    delete [] mpFrameDataQueue;      
}

bool CFrameDataQueue::PushData(const unsigned char* pData,const unsigned int dataLength, const unsigned int timestamp, const unsigned short rotation)
{
    //trace_printf("CFrameDataQueue::PushData start:%d,end:%d",mStart,mEnd);
    if (Count()<mQueueSize)
    {
        mpFrameDataQueue[mEnd].SetData(pData,dataLength,timestamp,rotation);
        mEnd = (mEnd+1)%mQueueSize;
        return true;
    }else
        return false;
}
CFrameData* CFrameDataQueue::NextData()
{
    if (Count()>0)
    {
        return &mpFrameDataQueue[mStart];
    }else
        return NULL;
    
}
void CFrameDataQueue::PopData()
{
    //trace_printf(TEXT"CFrameDataQueue::PopData start:%d,end:%d",mStart,mEnd);
    if (Count()>0)
    {
        mStart = (mStart+1)%mQueueSize;
    }
}
