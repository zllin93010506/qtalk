#pragma once

class CAudioData
{
public:
    CAudioData(void);
    ~CAudioData(void);
    unsigned char* GetOrNewData(const unsigned int size);
    void SetData(const unsigned char* pData,const unsigned int length,const unsigned int timestamp);
    unsigned char* GetData(unsigned int *pLength,unsigned int *pSize,unsigned int* pTimestamp);
private: 
    unsigned int    mTimeStamp;
    unsigned char*  mpData;
    unsigned int    mFrameSize;
    unsigned int    mDataLength;
};
