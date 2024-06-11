///QStreamEngine is the Wrapper for QStream
///Authors : Steven.Liao@AR1.Quanta
#ifndef __QStreamEngine_H
#define __QStreamEngine_H
#pragma once
#include "stdlib.h"
#include <pthread.h>
#include <semaphore.h>

#include "QStreamStatistics.h"
#include "RTPTimeConvertor.h"
extern "C"{
//#include "QStream_MediaTransportLayer.h" // this include should before <Windows.h> because of <winsock2.h>
#include "QStream_MediaTransportLayer.h"
}
#define MAX_PATH 256
//#define ENABLE_STATISTIC_COUNTING

#pragma pack(push, 1) //- packing is now 1
struct StreamReport {
    uint32_t recv_bit_rate_kbs;
    uint32_t send_bit_rate_kbs;
    uint32_t recv_sample_rate_fbs;
    uint32_t send_sample_rate_fbs;
    uint32_t recv_gap_sec;
    uint32_t send_gap_sec;
};
#pragma pack(pop) //- packing is 8

#define SUCCESS 0
#define FAIL -1
#define INVALID_OPERATION -2
//#define EINVAL -3
class IStreamEngineCB
{
public :
    virtual void OnRecv(unsigned char * pData, uint32_t length, uint32_t timestamp) = 0;
    virtual bool OnLipSyncCheck(long captureClockSec,long captureClockUsec) = 0;
};

class QStreamEngine
{
public:
    /* media type define */       
    //enum Result{SUCCESS=0,FAIL=-1,INVALID_OPERATION=-2,EINVAL=-3};

    QStreamEngine(IStreamEngineCB* audioHandler,int rtpListenPort, int qsMediaType, FEC_t fec_arg);
    virtual ~QStreamEngine(void);
    int     Start(unsigned long ssrc,const char* pRemoteIP, int rtpDestPort, uint32_t clockRate,uint32_t payLoadType,const char* pMimeType, int audioRTPTOS);
    int     Restart();
    int     Stop();
    int     SendData(unsigned char * pData, uint32_t length, uint32_t timestamp, struct timeval clock, int dtmfKey);
    void    GetReport(StreamReport* pReport);  
    unsigned long GetSSRC();
private:
    uint32_t    mSampleRate;
    uint32_t    mPayloadType;
    char                mMimeType[MAX_PATH];
    //QStream Statis
    IStreamEngineCB* mpAudioHandler;
    int                 mMediaType;
    char                mRemoteIP[MAX_PATH];
	int                 mRtpListPort;
	int                 mRtpDestPort;
	FEC_t				mFEC_arg;
	int					mTOS;

	unsigned long       mNetId;
    unsigned long       mSsrc;
    MTL_SESSION_t       *mpSession;
    QSTL_CALLBACK_T     mNetCallback;
    pthread_mutex_t     mSessionLock;
    //Recving Thread
    pthread_t           mEngineThreadHandle;
    pthread_mutex_t     mEngineThreadLock;
    sem_t               mEngineReadyEvent;
    bool                mIsEngineStop;
    bool                mIsEngineReady;
    bool                mUseLastSsrc;

    static void* StreamEngineThread(void* pParam);
    uint8_t*            mRxBuffer;
    uint32_t            mRxBufferSize;
    //QStream Call back
    static int          AudioRtcpRrCallback(unsigned long mtl_id, void *info);
    static int          AudioRtcpSrCallback(unsigned long mtl_id, void *info);
    static void         AudioRtcpTimestampSetting(unsigned long mtl_id);
    static QStreamEngine* GetQStreamEngine(unsigned long mtl_id);
    
    int                 AudioRtcpRrCallback(void *info);
    int                 AudioRtcpSrCallback(void *info);
    void                AudioRtcpTimestampSetting();
    //TimeStamp Control
    pthread_mutex_t    mTimeStampLock;
    uint32_t    mRtpTimeStamp;
    uint32_t    mNTPSec;
    uint32_t    mNTPFrac;
    //Statistics Objects
    CQStreamStatistics  mStatistics;
    
    CRTPTimeConvertor   mTimeConvertor;
    int                 InternalStart(bool useSameSsrc);
	
    long                mLastSendTimeSec;
    long                mLastRReportTimeSec;
    long                mLastSReportTimeSec;
    
    //Outband DTMF 
    int                 mLastDtmfKey;
    unsigned short      mDtmfDuration;
    uint32_t            mDtmfTimestamp;
    bool                mFirstMediaData;
    
    DTMF_Payload_t      mDtmf;
    TL_Send_Para_t      mSendData;
    int                 mDtmfEndResentCount;
};
inline unsigned long QStreamEngine::GetSSRC()
{
	return mSsrc;
}
#endif //__QStreamEngine_H
