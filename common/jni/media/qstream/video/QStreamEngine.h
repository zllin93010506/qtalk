///QStreamEngine is the Wrapper for QStream
///Authors : Steven.Liao@AR1.Quanta
#ifndef __QStreamEngine_H
#define __QStreamEngine_H
//#pragma once
#include "stdlib.h"
#include <pthread.h>
#include <semaphore.h>
extern "C"{
//#include "QStream_MediaTransportLayer.h" // this include should before <Windows.h> because of <winsock2.h>
#include "QStream_MediaTransportLayer.h"
}
#include "QStreamRateControl.h"
#include "QStreamErrorConcealment.h"
#include "QStreamLipSyncControl.h"
#include "QStreamStatistics.h"
#include "RTPTimeConvertor.h"
#define MAX_PATH 256

//Codec Type Define
#define VIDEO_CODEC 0xFF
#define AUDIO_CODEC 0x00
#define SUCCESS 0
#define FAIL -1
#define INVALID_OPERATION -2
//#define EINVAL -3

#pragma pack(push, 1) //- packing is now 1
struct StreamReport {
    uint32_t         recv_bit_rate_kbs;
    unsigned char   recv_frame_rate_fps;
    uint32_t         send_bit_rate_kbs;
    unsigned char   send_frame_rate_fps;
    double          package_loss;
    uint32_t         rtt_us;
};
#pragma pack(pop) //- packing is 8

class QStreamVideoCB
{
public :    
    virtual void OnRecv(unsigned char * pData, uint32_t length, uint32_t msTimestamp,uint16_t rotation, bool isPartial,unsigned char nalType) = 0;
    virtual bool OnLipSyncCheck(uint32_t currentRecvSize, long captureClockSec,long captureClockUsec)= 0;
    virtual void SetEncodeBitRate(unsigned long bitRate) = 0;
    virtual void SetEncodeFrameRate(unsigned char frameRate) = 0;
    virtual void OnFailedEnableTFRC() = 0;
    virtual void RequestIDR() = 0;
    virtual void OnPacketLost() = 0;
};

class QStreamEngine
{
public:
    /* media type define */       
    //enum Result{SUCCESS=0,FAIL=-1,INVALID_OPERATION=-2,EINVAL=-3};

    QStreamEngine(QStreamVideoCB* pStreamCB,int rtpListenPort, int qsMediaType, FEC_t fec_arg);
    virtual ~QStreamEngine(void);
    int     Start(bool enableTfrc,unsigned long ssrc,const char* pRemoteIP, int rtpDestPort, uint32_t clockRate,uint32_t payLoadType,const char* pMimeType, int mtu, int tos);
    int     Restart(bool enableTfrc);
    int     Stop();
    int     SendData(unsigned char * pData, uint32_t length, uint32_t timestamp, struct timeval clock, short rotation, bool toPx1);
    void    GetReport(StreamReport* pReport);
    unsigned long GetSSRC();
    static void* StreamEngineThread(void* pParam);
    void SetBitRateBound(int maxBitRate,int minBitRate,int normlBitRate, int initialBitRate);
    void    SendRtcpFbPli();
    void    SendRtcpFbFir();
private:
    //QStream Result Output
    QStreamVideoCB*     mpStreamVideoCB;
    //Output Data Type
    uint32_t    mClockRate;
    uint32_t    mPayloadType;
    char                mMimeType[MAX_PATH];
    //Stream engine related parameters
    int                 mMediaType;
    bool                mEnableTfrc;
    char                mRemoteIP[MAX_PATH];
    int                 mRtpListPort;
    int                 mRtpDestPort;
    unsigned long       mNetId;
    unsigned long       mSsrc;
    MTL_SESSION_t       *mpSession;
    QSTL_CALLBACK_T     mNetCallback;
    pthread_mutex_t     mSessionLock;
    bool                mUseLastSsrc;
    bool                mGotTFRC;
    int                 mExpectedTfrcCount;
    
	FEC_t				mFEC_arg;			//eason - FEC info
	int					mMTU;
	int					mTOS;
	
    //Recving Thread
    pthread_t           mEngineThreadHandle;
    pthread_mutex_t     mEngineThreadLock;
    sem_t               mEngineReadyEvent;
    bool                mIsEngineReady;
    bool                mIsEngineStop;

    uint8_t*            mRxBuffer;
    uint32_t            mRxBufferSize;

    //Rate Control Objects
    QStreamRateControl      mRateControl;
    void                    PerformRateControl();
    long                    mLastRateControlTime;
    //TFRC/RTCP Control related parameter
    pthread_mutex_t         mTimeStampLock;
    uint32_t        mRtpTimeStamp;
    uint32_t        mNTPSec;
    uint32_t        mNTPFrac;
    CRTPTimeConvertor       mTimeConvertor;
    //Statistics Objects
    CQStreamStatistics      mStatistics;

    int                     InternalStart(bool useSameSsrc);
    
    uint32_t        mLastRtpSeqNumber;
    //Transport Layer TFRC Call back functions
    int             VideoRtcpRrCallback(void *info);
    int             VideoRtcpSrCallback(void *info);
    int             VideoTfrcReportCallback(void *info);
    void            VideoRtcpTimestampSetting();
    void            VideoPsfbCallback(void *info);  //RTCP_FB_PLI, RTCP_FB_FIR
    void            VideoRtpReceive(uint16_t seq_num, unsigned long timestamp); // for seq_number checking, ex: packetlost counting

    static int      VideoRtcpRrCallback(unsigned long mtl_id, void *info);
    static int      VideoRtcpSrCallback(unsigned long mtl_id, void *info);
    static int      VideoTfrcReportCallback(unsigned long mtl_id, void *info);
    static void     VideoRtcpTimestampSetting(unsigned long mtl_id);
    static void     VideoPsfbCallback(unsigned long parent_id, void *info);//RTCP_FB_PLI, RTCP_FB_FIR
    static QStreamEngine* GetQStreamEngine(unsigned long mtl_id);
    static void     VideoRtpReceive(unsigned long id, uint16_t seq_num, unsigned long timestamp); // for seq_number checking, ex: packetlost counting
  
};
inline unsigned long QStreamEngine::GetSSRC()
{
    return mSsrc;
}
#endif //__QStreamEngine_H
