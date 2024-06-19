#include <stdlib.h>
#include <memory.h>
#include "AudioData.h"

CAudioData::CAudioData(void)
{
    mTimeStamp = 0;
    mpData     = NULL;
    mFrameSize = 0;
    mDataLength= 0;
}

CAudioData::~CAudioData(void)
{
    if (mpData!=NULL)
    {
        free(mpData);
    }
    mTimeStamp = 0;
    mpData     = NULL;
    mFrameSize = 0;
    mDataLength= 0;
}
unsigned char* CAudioData::GetOrNewData(const unsigned int size)
{
    if (mpData!=NULL && size>mFrameSize)
    {
        free(mpData);
        mpData = NULL;
    }
    if (mpData == NULL)
    {
        mpData = (unsigned char*) malloc(size);
        mFrameSize = size;
        mDataLength = 0;
        mTimeStamp = 0;
    }
    return mpData;
}
void CAudioData::SetData(const unsigned char* pData, const unsigned int length, const unsigned int timestamp)
{
    if (pData!=NULL && length>0 )
    {
        GetOrNewData(length);
        memcpy(mpData,pData,length);
    }
    mTimeStamp = timestamp;
    mDataLength = length;
}
unsigned char* CAudioData::GetData(unsigned int *pLength,unsigned int *pSize,unsigned int* pTimestamp)
{
    if (pLength!=NULL)
        *pLength = mDataLength;
    if (pSize!=NULL)
        *pSize = mFrameSize;
    if (pTimestamp!=NULL)
        *pTimestamp = mTimeStamp;
    return mpData;
}
