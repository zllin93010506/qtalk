#pragma once
#include "qstream/video/QStreamEngine.h"
#include "qstream/video/QStreamH264Parser.h"
#include "qstream/video/FrameDataQueue.h"
#define SUCCESS 0
#define FAIL          -1
#define CODEC_PL330     1
#define CODEC_FFMPEG    2
#define CODEC_DEFAULT   CODEC_PL330

typedef void (*resolution_change_cb)(int with,int height);

class IEngineCB
{
public :
    virtual bool OnRecv(const unsigned char * pData,const  uint32_t length,const  uint32_t msTimestamp,const uint16_t rotation) = 0;
    virtual void OnResolutionChange(const int width,const  int height) = 0;

    virtual void OnReport(const int kbpsRecv,const unsigned char fpsRecv,const int kbpsSend,const unsigned char fpsSend,const double pkloss,const int usRtt) = 0;
    virtual void SetBitRate(const unsigned long bitRate) = 0;
    virtual void SetFrameRate(const unsigned char frameRate) = 0;
    virtual void SetGop(const unsigned char gop) = 0;
    virtual unsigned int GetRecvBufferSize() = 0;
    virtual void OnRequestIDR() = 0;
    virtual void OnDemandIDR() = 0;
};
class CQVideoStreamEngine : public QStreamVideoCB
{
public:
    CQVideoStreamEngine(int rtpListPort,IEngineCB* pEngineCB);
    virtual ~CQVideoStreamEngine(void);
    int UpdateMediaSource(
                       int srcWidth,
                       int srcHeight);

    bool Start(unsigned long ssrc,const char* remoteIP,
              uint32_t rtpDestPort,
              uint32_t clockRate,
              uint32_t payLoadType,
              const char* pMimeType,
              bool enableTFRC,
              uint32_t initialFramerate,
              uint32_t initialBitrate,
              bool letGopEqFps,
              bool enableRtcpFbPli,
              bool enableRtcpFbFir,
              int fecCtrl,
              int fecSendPType,
              int fecRecvPType,
              int maxBitRate, int videoMTU, int videoRTPTOS);

    bool Restart(void);
    int Stop(void);

//    void RestartReceiver();
//    void GetReport(StreamReport* pStreamReport);
    void SetLipSyncToken(LipSyncToken* pToken);

    //Implement QStreamVideoCB
    void OnRecv(unsigned char * pData, uint32_t length, uint32_t timestamp,uint16_t rotation, bool isPartial,unsigned char nalType);
    void SetEncodeBitRate(const unsigned long bitRate);
    void SetEncodeFrameRate(const unsigned char frameRate);
    bool OnLipSyncCheck(const uint32_t currentRecvSize,const long captureClockSec,const long captureClockUsec);
    void OnFailedEnableTFRC();
    void RequestIDR();
    void OnPacketLost();
    
    //Implement GrabberOutputHandler
    void OnSend(uint8_t * pData, long length, double sampleTime, short rotation);

    unsigned long GetSSRC();

private :
    bool                mIsH264;
    unsigned long       mSsrc;
    IEngineCB           *mpEngineCB;
    //Main Components   ====================================
    QStreamEngine       *mpEngine;      //QStream Object Rtp/Rtcp Sender/Receiver
    pthread_mutex_t     mEngineLock;

    //State
    bool                mIsRunning;

    //Private Internal Functions
    bool                InternalStart(bool isRestart);

    //Sender Related    ====================================
    int                 mEncodeCodecType;
    int                 mSourceWidth;
    int                 mSourceHeight;
    unsigned int        mMaxBitRateKbps;
    unsigned int        mMinBitRateKbps;
    unsigned int        mRecmdBitRateKbps;

    int                 mSenderWidth;
    int                 mSenderHeight;
    int                 mSenderMaxBitRate;
    int                 mInitialBitRate;
    int                 mInitialFrameRate;
    //Receiver Related  ====================================
    int                 mDecodeCodecType;
    //Resolution of Remote 
    int                 mRemoteWidth;
    int                 mRemoteHeight;

    //QStream Related   ====================================
    char                mRemoteIP[MAX_PATH];
    int                 mRtpDestPort;
    int                 mRtpListPort;
    bool                mEnableTFRC;
    bool                mLetGopEqFps;
    
    FEC_t				mFEC_arg;
    int					mMaxBitRateKbpsBound;
    int					mVideoMTU;
    int					mVideoRTPTOS;    
    
    
    //For Sending H264 Stream to QStream
    uint32_t    mClockRate;
    uint32_t    mPayloadType;
    char                mMimeType[MAX_PATH];
    //Error Concealment
    bool                mGotFirstIFrame;
    int                 mGetRecvH264ParaSet;
    int                 mRecvH264ParserCodecType;
    h264_para_set_t     mRecvH264ParaSet;
    int                 mRecvH264ParserCount;
    CQStreamErrorConcealment mRecvErrConcealment;
    bool OnErrConcealment(unsigned char* pStreamData, unsigned int dataLength, uint32_t timestamp, bool isPartial);
    
    void SetDecodeResolution(int width,int height);
    //Lip Sync
    CQStreamLipSyncControl mLipSyncControl;
    //For sending timestamp sync
    uint32_t mSessionStartTimesec;
    uint32_t mSessionStartTimeus;
    //Threads to polling Streaming Report
    pthread_t            mPollReportThreadHandle;
    static void* PollReportThread(void* pParam);
    static void* RestartThread(void* pParam);
    StreamReport        mStreamReport;
    sem_t               mStopPollEvent;

    unsigned char       mSendFrameRate;
    unsigned char       mSendGop;
    unsigned long       mSendKbps;

    bool                mIsX264;
    int                 mH264ParserCount;
    int                 mMaxGopIndex;
    int                 mLastFrameIndex;
    
    //IDR loss detection
    bool                mDemandIDR;
    unsigned long       mLastIDRDTimeMs;
    void                InternalDemandIDRCheck();
    bool                mEnableRtcpFbPli;
    bool                mEnableRtcpFbFir;
    bool 				mFirstDemandIDR;
};
