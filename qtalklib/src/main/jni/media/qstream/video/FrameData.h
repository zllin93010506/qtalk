#pragma once

class CFrameData
{
public:
    CFrameData(void);
    ~CFrameData(void);
    unsigned char* GetOrNewData(const unsigned int size);
    void SetData(const unsigned char* pData,const unsigned int length,const unsigned int timestamp,const unsigned short rotation);
    unsigned char* GetData(unsigned int *pLength,unsigned int *pSize,unsigned int* pTimestamp,unsigned short* pRotation);
private: 
    unsigned int    mTimeStamp;
    unsigned char*  mpData;
    unsigned int    mFrameSize;
    unsigned int    mDataLength;
    unsigned short  mRotation;
};
