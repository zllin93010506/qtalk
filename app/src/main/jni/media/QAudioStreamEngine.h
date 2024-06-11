#ifndef __QAudioStreamEngine_H
#define __QAudioStreamEngine_H
#include "qstream/audio/QStreamEngine.h"
#include "qstream/audio/AudioDataQueue.h"
#define SUCCESS 0
#define FAIL          -1
#define CODEC_PL330     1
#define CODEC_FFMPEG    2
#define CODEC_DEFAULT   CODEC_PL330

#ifndef LIP_SYNC_TOKEN
#define LIP_SYNC_TOKEN

//typedef unsigned long long int uint64_t;
//typedef unsigned long int uint32_t;
typedef struct{
	uint64_t audio_new_rtp_ts;
    uint64_t video_new_rtp_ts;
} LipSyncToken;
#endif

class IEngineCB
{
public :
    virtual void OnRecv(unsigned char * pData, uint32_t length, uint32_t msTimestamp) = 0;
    virtual void OnReport(uint32_t kbpsRecv,uint32_t kbpsSend, uint32_t fpsRecv, uint32_t fpsSend,uint32_t secGapRecv, uint32_t secGapSend ) = 0;
};
class CQAudioStreamEngine : public IStreamEngineCB
{
public:
    CQAudioStreamEngine(int rtpListPort,IEngineCB* pEngineCB);
    virtual ~CQAudioStreamEngine(void);

    bool Start(unsigned long ssrc,
              const char* remoteIP,
              uint32_t rtpDestPort,
              uint32_t clockRate,
              uint32_t payLoadType,
              const char* pMimeType,
              int fecCtrl,
              int fecSendPType,
              int fecRecvPType,
              int audioRTPTOS);

    bool Restart(void);
    int Stop(void);

    void SetLipSyncToken(LipSyncToken* pToken);

    //Implement QStreamVideoCB
    void OnRecv(unsigned char * pData, uint32_t length, uint32_t timestamp);
    bool OnLipSyncCheck(long captureClockSec,long captureClockUsec);
    
    //Implement GrabberOutputHandler
    void OnSend(uint8_t * pData, long length, double sampleTime, int dtmfKey);

    unsigned long       GetSSRC();
private :
    unsigned long       mSsrc;
    IEngineCB			*mpEngineCB;
    //Main Components   ====================================
    QStreamEngine       *mpEngine;      //QStream Object Rtp/Rtcp Sender/Receiver
    pthread_mutex_t     mEngineLock;

    //State
    bool                mIsRunning;

    //Private Internal Functions
    bool                InternalStart(bool isRestart);

    //Sender Related    ====================================
    int                 mEncodeCodecType;

    //Receiver Related  ====================================
    int                 mDecodeCodecType;
    //Resolution of Remote 

    //QStream Related   ====================================
    char                mRemoteIP[MAX_PATH];
    int                 mRtpDestPort;
    int                 mRtpListPort;
    //For Sending H264 Stream to QStream
    uint32_t    		mClockRate;
    uint32_t    		mPayloadType;
    char                mMimeType[MAX_PATH];
    int                 mMinGapTimeStamp;
	FEC_t				mFEC_arg;
    int					mAudioRTPTOS;

    //For sending timestamp sync
    uint32_t mSessionStartTimesec;
    uint32_t mSessionStartTimeus;
    //Threads to polling Streaming Report
    pthread_t            mPollReportThreadHandle;
    static void* PollReportThread(void* pParam);
    StreamReport        mStreamReport;
    sem_t               mStopPollEvent;
    sem_t               mStopThreadSuccessEvent;

    uint32_t            mTimeStamp;
};
#endif
