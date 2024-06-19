#pragma once

/* h264 type define */
//#define I_SLICE_PARA_SET_CODE               0x65
//#define I_SLICE_CODE                        0x67
//#define P_SLICE_CODE                        0x41

class CQStreamErrorConcealment
{
public:
    CQStreamErrorConcealment(void);
    ~CQStreamErrorConcealment(void);
    void    Reset();
    void    ShowError(bool enable);
    bool    OnErrConcealment(int sliceType,unsigned short frame_index, bool isPartial);

private:
 
    bool            mIsIFrameNeeded;
    
    unsigned short  mPFramePredictIndex;
    bool            mIsPFramePredicted;

    bool            mEnableShowError;
    pthread_mutex_t mPredictLock;
};
