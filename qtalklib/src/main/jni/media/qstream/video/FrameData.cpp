#include <stdlib.h>
#include <memory.h>
#include "FrameData.h"

CFrameData::CFrameData(void)
{
    mTimeStamp = 0;
    mpData     = NULL;
    mFrameSize = 0;
    mDataLength= 0;
    mRotation = 0;
}

CFrameData::~CFrameData(void)
{
    if (mpData!=NULL)
    {
        free(mpData);
    }
    mTimeStamp = 0;
    mpData     = NULL;
    mFrameSize = 0;
    mDataLength= 0;
    mRotation = 0;
}
unsigned char* CFrameData::GetOrNewData(const unsigned int size)
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
        mRotation = 0;
    }
    return mpData;
}
void CFrameData::SetData(const unsigned char* pData, const unsigned int length, const unsigned int timestamp,const unsigned short rotation)
{
    if (pData!=NULL && length>0 )
    {
        GetOrNewData(length);
        memcpy(mpData,pData,length);
    }
    mTimeStamp = timestamp;
    mDataLength = length;
    mRotation = rotation;
}
unsigned char* CFrameData::GetData(unsigned int *pLength,unsigned int *pSize,unsigned int* pTimestamp,unsigned short* pRotation)
{
    if (pLength!=NULL)
        *pLength = mDataLength;
    if (pSize!=NULL)
        *pSize = mFrameSize;
    if (pTimestamp!=NULL)
        *pTimestamp = mTimeStamp;
    if (pRotation!=NULL)
        *pRotation = mRotation;
    return mpData;
}
