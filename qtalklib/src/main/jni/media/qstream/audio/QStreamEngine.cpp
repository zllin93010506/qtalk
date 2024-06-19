/*==================================================================================================
File name       : QStreamEngine.cpp
Description     : QStreamEngine is the Wrapper for QStream
Global function :
Input           :
Output          :
Crated date     : 2010.06.11
Last date       : 2010.06.11
Author          : Steven.Liao@AR1.Quanta
Tester          : Steven.Liao@AR1.Quanta
Test results       :
History            : #2010.06.11    Steven.Liao Create
                  #2010.09.29   Steven.Liao Modify to Andorid Version
==================================================================================================*/
#include "QStreamEngine.h"
#include "List.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>            //errno, semaphone
#include "logging.h"
#include <stdio.h>
#define TAG "QStreamEngine"

#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

extern void on_thread_begin();
extern void on_thread_end();

class CCritLock {
    private:
        pthread_mutex_t m_CritSec;
    public:
        CCritLock() {
            pthread_mutexattr_t mutexattr;   // Mutex attribute variable
            // Set the mutex as a recursive mutex
            pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
            // create the mutex with the attributes set
            pthread_mutex_init(&m_CritSec, &mutexattr);
            //After initializing the mutex, the thread attribute can be destroyed
            pthread_mutexattr_destroy(&mutexattr);
        };

        ~CCritLock() {
            pthread_mutex_destroy(&m_CritSec);
        };

        void Lock() {
            pthread_mutex_lock(&m_CritSec);
        };

        void Unlock() {
            pthread_mutex_unlock(&m_CritSec);
        };
};

#define MAX_AUDIO_BUF                       1280

QStreamEngine::QStreamEngine(IStreamEngineCB* audioHandler,int rtpListenPort, int qsMediaType, FEC_t fec_arg)
{
    mpAudioHandler                  = audioHandler;
    mMediaType                      = qsMediaType;
    mRtpListPort                    = rtpListenPort;
    mRtpDestPort                    = 0;
    mUseLastSsrc                    = false;
    
    mFEC_arg.ctrl                   = fec_arg.ctrl;
    mFEC_arg.send_pt                = fec_arg.send_pt;
    mFEC_arg.recv_pt                = fec_arg.recv_pt;
    mTOS							= 0;
    
    memset(&mNetCallback,0,sizeof(mNetCallback));
    
    mNetCallback.rtcp_rr            = QStreamEngine::AudioRtcpRrCallback;
    mNetCallback.rtcp_sr            = QStreamEngine::AudioRtcpSrCallback;
    mNetCallback.tfrc_report        = NULL;
    //mNetCallback.getBuf             = NULL;
    mNetCallback.timestamp_setting  = QStreamEngine::AudioRtcpTimestampSetting;
    mNetCallback.rtcp_app_set       = NULL;
    mNetCallback.rtcp_app_receive   = NULL;
    mNetCallback.dtmf_event         = NULL;

    //Intialize the parameter
    mEngineThreadHandle             = NULL;
//    mEngineReadyEvent               = NULL;
    mIsEngineStop                   = true;
    mIsEngineReady                  = false;

    mRxBuffer = NULL;
    mRxBufferSize = 0;
//    HANDLE hMutex = CreateMutexA(NULL, false, "QSCE_InitLibrary_Mutex");
//    if (hMutex != NULL && GetLastError() == ERROR_ALREADY_EXISTS)
//        LOGE("[AUDIOENGINE]QStreamEngine QSCE_InitLibrary_Mutex Already on calling(Audio)"));
//    else
//    if (hMutex!=NULL)
//        CloseHandle(hMutex);// close the mutex

    pthread_mutexattr_t mutexattr;   // Mutex attribute variable

    // Set the mutex as a recursive mutex
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);

    // create the mutex with the attributes set
    pthread_mutex_init(&mSessionLock, &mutexattr);
    pthread_mutex_init(&mEngineThreadLock, &mutexattr);
    pthread_mutex_init(&mTimeStampLock, &mutexattr);

    //After initializing the mutex, the thread attribute can be destroyed
    pthread_mutexattr_destroy(&mutexattr);
}

QStreamEngine::~QStreamEngine(void)
{
    LOGD("==>~QStreamEngine");
    if (!mIsEngineStop)
        Stop();
    pthread_mutex_destroy(&mSessionLock);
    pthread_mutex_destroy(&mTimeStampLock);
    pthread_mutex_destroy(&mEngineThreadLock);
    LOGD("<==~QStreamEngine");
}

int
QStreamEngine::Start(unsigned long ssrc,const char* pRemoteIP, int rtpDestPort, uint32_t clockRate,uint32_t payLoadType,const char* pMimeType, int audioRTPTOS)
{
    if (!pRemoteIP)
        return EINVAL;
    LOGD("==>Start Audio ssrc:%d ip:%s,mRtpListPort%d,rtpDestPort:%d,clockRate:%d,payLoadType:%d,pMimeType:%s",ssrc,pRemoteIP,mRtpListPort,rtpDestPort,clockRate,payLoadType,pMimeType);
    int rtn = FAIL;
    mSsrc = ssrc;
    mSampleRate                      = clockRate;
    mPayloadType                    = payLoadType;
    memset(mMimeType,0,sizeof(char)*MAX_PATH);
    mTimeConvertor.SetSampleRate(mSampleRate);
    strcpy(mMimeType,pMimeType);
    //Set Destination IP
    memset(mRemoteIP,0,MAX_PATH);
    strcpy(mRemoteIP,pRemoteIP);
    //Set Destination RTP Port
    mRtpDestPort = rtpDestPort;
    
	mTOS = audioRTPTOS;
	
    //Start StreamEngine
    if (mSsrc!=0)
        rtn = InternalStart(true);
    else
        rtn = InternalStart(false);
    if (rtn != SUCCESS)
    {
        LOGE("[AUDIOENGINE]Failed On Start (Audio)");
        Stop();
    }
    LOGD("<==Start Audio");
    return rtn;
}
int
QStreamEngine::InternalStart(bool useSameSsrc)
{
    int result = FAIL;
    LOGD("==>InternalStart Audio");
#ifdef ENABLE_STATISTIC_COUNTING
    mStatistics.Reset();
#endif
    mNTPSec = 0;
    mNTPFrac = 0;
    mRtpTimeStamp = 0;
    mLastSendTimeSec = 0;
    mLastRReportTimeSec = 0;
    mLastSReportTimeSec = 0;
    mDtmfDuration = 0;//reset dtmf duration
    mLastDtmfKey = -1;//reset last dtmf key
    mFirstMediaData = true;
    
    mDtmfEndResentCount = 0;//reset the resend counter
    pthread_mutex_lock(&mEngineThreadLock);
    if (mEngineThreadHandle==0)
    {
        mIsEngineReady = false;
        int   ret ;
        // Initialize event semaphore
        ret = sem_init(&mEngineReadyEvent,   // handle to the event semaphore
                           0,     // not shared
                           0);    // initially set to non signaled state
        if(ret)
            LOGE("sem_init mEngineReadyEvent error code=%d\n",ret);

        mUseLastSsrc = useSameSsrc;
        mIsEngineStop = false;
        // Create the threads
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

        ret = pthread_create(&mEngineThreadHandle, &attr, StreamEngineThread, (void*)this);
        if(ret)
            LOGE("pthread_create mEngineThreadHandle thread error code=%d\n",ret);
        if (mEngineThreadHandle != 0)
        {
            // Wait for the event be signaled
            ret = sem_wait(&mEngineReadyEvent); // event semaphore handle
            if(ret)
                LOGE("sem_wait mEngineReadyEvent error code=%d\n",ret);

            result = SUCCESS;
        }
    }else
    {   
        LOGE("[AUDIOENGINE]InternalStart: Thread already on running Audio");
        result = SUCCESS;
    }
    pthread_mutex_unlock(&mEngineThreadLock);
    LOGD("<==InternalStart Audio");
    return result;
}
int
QStreamEngine::Restart()
{  
    Stop();
    int rtn = InternalStart(true);
    if (rtn!=SUCCESS)
    {
        LOGE("[AUDIOENGINE]Failed On Restart (Audio)");
        Stop();
    }
    return rtn;
}
int
QStreamEngine::Stop()
{   
    LOGD("==>Stop Audio");
    int ret;
    if (!mIsEngineStop)
    {
        //Stop Thread
        mIsEngineStop = true;
        pthread_mutex_lock(&mEngineThreadLock);
        if (mEngineThreadHandle!=0)
        {
            ret = pthread_join(mEngineThreadHandle, 0); //<== Hangup Here , remark: don't call thread_exit on C++
            if(ret)
                LOGE("Terminate mEngineThreadHandle thread error code=%d\n",ret);
            mEngineThreadHandle = NULL;
        }
        ret = sem_destroy(&mEngineReadyEvent);
        if(ret)
            LOGE("Destroy mEngineReadyEvent error code=%d\n",errno);
        pthread_mutex_unlock(&mEngineThreadLock);
#ifdef ENABLE_STATISTIC_COUNTING
        mStatistics.Reset();
#endif
    }
    LOGD("<==Stop Audio");
    return SUCCESS;
}
int
QStreamEngine::SendData(unsigned char * pData, uint32_t length, uint32_t timestamp, struct timeval clock,int dtmfKey)
{
    if (!pData)
    {
        LOGE("[AUDIOENGINE]SendData -> INVALID pData");
        return EINVAL;
    }
    if (!mpSession)
    {
        LOGE("[AUDIOENGINE]SendData -> INVALID mpSession");
        return EINVAL;
    }
    if (mIsEngineStop)
    {
        LOGE("[AUDIOENGINE]SendData -> Thread Stop");
        return FAIL;
    }
    if (!mIsEngineReady)
    {
        LOGE("[AUDIOENGINE]SendData -> Engine is not ready");
        return FAIL;
    }
    int rtn = FAIL;
    pthread_mutex_lock(&mTimeStampLock);
    mRtpTimeStamp = clock.tv_sec * mSampleRate +
                    (uint32_t)((double)clock.tv_usec * (double)mSampleRate * 1.0e-6);;
    mNTPSec = (unsigned long)(clock.tv_sec + 0x83AA7E80);
    mNTPFrac = (unsigned long)((double)clock.tv_usec * (double)(1LL<<32) * 1.0e-6 );
    pthread_mutex_unlock(&mTimeStampLock);
    mLastSendTimeSec = clock.tv_sec;
    if (mDtmfEndResentCount>0)
    {   //Downgree the resend count of dtmf end
        --mDtmfEndResentCount;
    }else
    {
        memset(&mSendData,0,sizeof(mSendData));
        //Process outband DTMF====================
        if (dtmfKey!=-1 || mLastDtmfKey!=-1)
        {
            bool dtmf_end = (dtmfKey!= mLastDtmfKey && mLastDtmfKey!=-1 && dtmfKey==-1);
            bool dtmf_start = (dtmfKey!= mLastDtmfKey && mLastDtmfKey==-1 && dtmfKey!=-1);
            unsigned char dtmf_evnet = (unsigned char)((dtmfKey!=-1)?dtmfKey:mLastDtmfKey);
            if (dtmf_end)
                mDtmfDuration += 3*mSampleRate/DTMF_PACKTS_PER_SEC;
            else
                mDtmfDuration += mSampleRate/DTMF_PACKTS_PER_SEC;
            memset(&mDtmf,0,sizeof(mDtmf));
            mDtmf.event = dtmf_evnet;
            mDtmf.volume = DTMF_FIXED_VOLUME;
            mDtmf.r = 0;
            mDtmf.e = dtmf_end;
            mDtmf.duration = htons(mDtmfDuration);
            if (dtmf_start)
                mDtmfTimestamp = timestamp;
            if (dtmf_end)
            {
                mDtmfDuration = 0;//reset dtmf duration
                mLastDtmfKey = -1;//reset last dtmf key
                mFirstMediaData = true;
                mDtmfEndResentCount = 2;//reset the resend counter
            }else
            {
                mLastDtmfKey = dtmfKey;
            }
            
            mSendData.frame_data = (uint8_t*)&mDtmf;
            mSendData.length = sizeof(mDtmf);
            mSendData.rtp_event_data = QS_DTMF_OUT_BAND_MODE;
            mSendData.mark = dtmf_start; //first dtmf
            mSendData.timestamp = mDtmfTimestamp;
        }else
        {
            mSendData.frame_data = pData;
            mSendData.length = length;
            mSendData.rtp_event_data = QS_DTMF_NULL_MODE;
            if (mFirstMediaData)
            {
                mFirstMediaData = false;
                mSendData.mark = 1;
            }else
                mSendData.mark = 0;
            mSendData.timestamp = timestamp;
        }
        mSendData.ext_hdr_data = NULL;
        mSendData.ext_len = 0;
    }
    
    //Begin=================Dump to File
//    FILE * pFile;
//    pFile = fopen ("/sdcard/qtalk.send.audio","a+b");
//    if (pFile!=NULL)
//    {
//        fwrite (pData , 1 , length , pFile );
//        fclose (pFile);
//    }
    //End=================Dump to File
    pthread_mutex_lock(&mSessionLock);
    if (mpSession && !mIsEngineStop)
        rtn = mpSession->session->send_data(mpSession->session, &mSendData);
    pthread_mutex_unlock(&mSessionLock);

    if (rtn<QS_TRUE) 
        LOGE("[AUDIOENGINE]SendData -> send_data failed %d",rtn);
    else
    {
#ifdef ENABLE_STATISTIC_COUNTING
        mStatistics.OnSendFrameOK(length);
#endif
    }
    return rtn;
}

#define qsce_check(rtn,msg,...) if (rtn<0) LOGE(msg,__VA_ARGS__)
void*
QStreamEngine::StreamEngineThread(void* pParam)
{
    LOGD("==>StreamEngineThread Audio");
    QStreamEngine* pParent = (QStreamEngine*)pParam;
    int rtn = SUCCESS;
    pthread_mutex_lock(&pParent->mSessionLock);
    pParent->mNetId = 0;
    pParent->mpSession = NULL;
    //Start Stream Engine
    if (SUCCESS == rtn)
    {   //Create Session
        rtn = QSCE_Create(&pParent->mNetId ,pParent->mMediaType);
        qsce_check(rtn,"QSCE_Create failure! %d",rtn);
    }
    
    if (SUCCESS == rtn)
    {   //Register Callbackup function
        rtn = QSCE_RegisterCallback(pParent->mNetId, &pParent->mNetCallback,(unsigned long) pParent);
        qsce_check(rtn,"QSCE_RegisterCallback failure! %d",rtn);
    }
    
    if (SUCCESS == rtn)
    {   //Setup Local Receiver
        rtn = QSCE_SetLocalReceiver(pParent->mNetId, pParent->mRtpListPort, "127.0.0.1");
        qsce_check(rtn,"QSCE_SetLocalReceiver failure! %d",rtn);
    }
    //Setup TFRC Setting
    //rtn = QSCE_SetTFRC(mNetId, mEnableTfrc?1:0); /* tfrc enable */
    //qsce_check(rtn,"QSCE_SetLocalReceiver failure! %d",rtn);
    
    if (SUCCESS == rtn)
    {   //Setup Network Payload
        rtn = QSCE_Set_Network_Payload(pParent->mNetId, pParent->mSampleRate, pParent->mPayloadType, pParent->mMimeType, 101);
        qsce_check(rtn,"QSCE_Set_Network_Payload failure! %d",rtn);
    }
    if (SUCCESS == rtn)
    {   //Setup Destination
        rtn = QSCE_SetSendDestination(pParent->mNetId, pParent->mRtpDestPort, pParent->mRemoteIP);
        qsce_check(rtn,"QSCE_SetSendDestination failure! %d",rtn);
    }
    if (SUCCESS == rtn){
    	//Setup FEC info
    	rtn = QSCE_Set_FEC(pParent->mNetId, pParent->mFEC_arg.ctrl, pParent->mFEC_arg.send_pt, pParent->mFEC_arg.recv_pt);
    	qsce_check(rtn,"QSCE_Set_FEC failure! %d",rtn);
    }
    if (SUCCESS == rtn)
    {   //Setup RTP TOS
        rtn = QSCE_Set_TOS(pParent->mNetId, pParent->mTOS);
        qsce_check(rtn,"QSCE_Set_TOS failure! %d",rtn);
    }
    
    if (SUCCESS == rtn)
    {   //Start the sending and recving thread
        if (pParent->mUseLastSsrc)
        {
            rtn = QSCE_StartWithSSRC(pParent->mNetId,pParent->mSsrc);
            qsce_check(rtn,"QSCE_StartWithSSRC failure! %d",rtn);
        }else
        {
            rtn = QSCE_Start(pParent->mNetId);
            qsce_check(rtn,"QSCE_Start failure! %d",rtn);
        }
    }
    if (SUCCESS == rtn)
    {
        pParent->mpSession = (MTL_SESSION_t *)pParent->mNetId;
        pParent->mSsrc = QSCE_GetSessionSSRC(pParent->mNetId);
        //LOGE("[AUDIOENGINE]Audio QSCE_GetSessionSSRC:%d",pParent->mSsrc);
    }
    pthread_mutex_unlock(&pParent->mSessionLock);
    sem_post(&pParent->mEngineReadyEvent);
    pParent->mIsEngineReady = true;
    if (SUCCESS == rtn)
    {   //Running Recving Thread
        TL_Recv_Para_t  recv_data;

        usleep(100000);
        long ntp_clock_sec;
        long ntp_clock_usec;
        while(!pParent->mIsEngineStop && pParent->mpSession)
        {
            memset(&recv_data,0,sizeof(recv_data));
            //Audio Data
            if (!pParent->mRxBuffer)
            {
                pParent->mRxBuffer = (uint8_t*)malloc(MAX_AUDIO_BUF*sizeof(uint8_t));
                pParent->mRxBufferSize = MAX_AUDIO_BUF;
            }

            recv_data.frame_data    = pParent->mRxBuffer;
            recv_data.length        = pParent->mRxBufferSize;
            memset(recv_data.frame_data,0,recv_data.length);
            //Get Data from Recv Buffer, Try Recving data
            rtn = pParent->mpSession->session->recv_data(pParent->mpSession->session, &recv_data);
            if (!pParent->mIsEngineStop && rtn >= QS_TRUE && recv_data.frame_data!=NULL && recv_data.length>0 && pParent->mpAudioHandler)
            {
#ifdef ENABLE_STATISTIC_COUNTING
                pParent->mStatistics.OnRecvFrameOK(recv_data.length);
#endif
                if (!pParent->mIsEngineStop && recv_data.timestamp!=0)
                {
                    pParent->mTimeConvertor.RtpTime2NTPClock(recv_data.timestamp,
                                                            &ntp_clock_sec,
                                                            &ntp_clock_usec);
                    if (!pParent->mIsEngineStop && pParent->mpAudioHandler->OnLipSyncCheck(ntp_clock_sec,ntp_clock_usec))
                    {
                        pParent->mpAudioHandler->OnRecv(recv_data.frame_data,recv_data.length,(ntp_clock_sec*1000+ntp_clock_usec/1000));
                        //Begin=================Dump to File
//                        FILE * pFile;
//                        pFile = fopen ("/sdcard/qtalk.recv.audio","a+b");
//                        if (pFile!=NULL)
//                        {
//                            fwrite (recv_data.frame_data , 1 , recv_data.length , pFile );
//                            fclose (pFile);
//                        }
                        //End=================Dump to File
                    }
                }else
                    LOGE("[AUDIOENGINE]StreamEngineThread Audio recv_data.timestamp is Zero %d",recv_data.length);
                
            }
            if (!pParent->mIsEngineStop)
                usleep(1000);
        }
    }
    pParent->mIsEngineReady = false;
    //Stop Stream Engine    
    pthread_mutex_lock(&pParent->mSessionLock);
    if (pParent->mNetId!=0)
    {
        QSTL_CALLBACK_T net_null_callback;
        // un-register callback function 
        memset((char *)&net_null_callback, 0x0, sizeof(net_null_callback));
        rtn = QSCE_RegisterCallback(pParent->mNetId, &net_null_callback,(unsigned long) pParent);
        qsce_check(rtn,"QSCE_RegisterCallback failure! %d",rtn);

        rtn = QSCE_Stop(pParent->mNetId);
        qsce_check(rtn,"QSCE_Stop failure! %d",rtn);

        rtn = QSCE_Destory(pParent->mNetId);
        qsce_check(rtn,"QSCE_Destory failure! %d",rtn);
    }
    pParent->mNetId = 0;
    pParent->mpSession = NULL;
    pthread_mutex_unlock(&pParent->mSessionLock);
    if (pParent->mRxBuffer)
    {
        free(pParent->mRxBuffer);
        pParent->mRxBuffer = NULL;
        pParent->mRxBufferSize = 0;
    }

    LOGD("<==StreamEngineThread Audio");
    //pthread_exit Limitations
    //When using STL containers as local objects, the destructor will not get called when pthread_exit() is called, which leads to a memory leak. Do not call pthread_exit() from C++. Instead you must throw or return back to the thread's initial function. There you can do a return instead of pthread_exit().
    //Pthread library has no knowledge of C++ stack unwinding. Calling pthread_exit() for will terminate the thread without doing proper C++ stack unwind. That is, destructors for local objects will not be called. (This is analogous to calling exit() for single threaded program.)
    //This can be fixed by calling destructors explicitly right before calling pthread_exit().
    pthread_exit(0);
}


void QStreamEngine::GetReport(StreamReport* pStreamReport)
{
    if (pStreamReport!=NULL)
    {
#ifdef ENABLE_STATISTIC_COUNTING
        pStreamReport->send_bit_rate_kbs = mStatistics.GetSendBitrate();
        pStreamReport->recv_bit_rate_kbs = mStatistics.GetRecvBitrate();
        pStreamReport->recv_sample_rate_fbs = mStatistics.GetRecvFramerate();
        pStreamReport->send_sample_rate_fbs = mStatistics.GetSendFramerate();
#endif
        pStreamReport->send_gap_sec = mLastSendTimeSec - mLastRReportTimeSec;
        pStreamReport->recv_gap_sec = mLastSendTimeSec - mLastSReportTimeSec;
    }
}

int 
QStreamEngine::AudioRtcpRrCallback(void *info)
{
    //LOGD("==>AudioRtcpRrCallback");
    TL_Rtcp_RR_t *rr = (TL_Rtcp_RR_t *)info;
    mLastRReportTimeSec = mLastSendTimeSec;
    //LOGD("<==AudioRtcpRrCallback");
    return SUCCESS;
}
int
QStreamEngine::AudioRtcpSrCallback(void *info)
{
    //LOGD("==>AudioRtcpSrCallback");
    TL_Rtcp_SR_t *sr = (TL_Rtcp_SR_t *)info;
    mLastSReportTimeSec = mLastSendTimeSec;
    //if (mIsEngineReady && mpSession && !mIsEngineStop)
    //mTimeConvertor.SetReferenceTime(sr->rtp_ts,
    //                                sr->ntp_sec - 0x83AA7E80,
    //                                (long)( (double)sr->ntp_frac * 1.0e6 / (double)(1LL<<32)));
    //LOGE("[Audio SR] ntp_sec=%lu, ntp_frac=%lu, rtp_ts=%lu, sender_pcount=%lu, sender_bcount=%lu\n", (unsigned long)sr->ntp_sec,
    //                                                                                                   (unsigned long)sr->ntp_frac,
    //                                                                                                   (unsigned long)sr->rtp_ts,
    //                                                                                                   (unsigned long)sr->sender_pcount,
    //                                                                                                   (unsigned long)sr->sender_bcount);
    //LOGD("<==AudioRtcpSrCallback");
    return SUCCESS;
}
void
QStreamEngine::AudioRtcpTimestampSetting()
{
    //LOGD("==>AudioRtcpTimestampSetting");
    pthread_mutex_lock(&mSessionLock);
    if (mIsEngineReady && mpSession && !mIsEngineStop)
    {
        //struct timeval clock;
        //unsigned long timestamp;
        //gettimeofday(&clock, 0);

        //timestamp = clock.tv_sec*1000 + clock.tv_usec/1000;
        //TransportLayer_SetRTCPTimeIfno(mpSession->session, 
        //                               timestamp,
        //                               (unsigned long)(clock.tv_sec + 0x83AA7E80),
        //                               (unsigned long)((double)clock.tv_usec * (double)(1LL<<32) * 1.0e-6 )
        //                               );
        pthread_mutex_lock(&mTimeStampLock);
        TransportLayer_SetRTCPTimeIfno(mpSession->session, 
                                       mRtpTimeStamp,
                                       mNTPSec,
                                       mNTPFrac
                                       );
        pthread_mutex_unlock(&mTimeStampLock);
    }
    pthread_mutex_unlock(&mSessionLock);
    //LOGD("<==AudioRtcpTimestampSetting");
}

int 
QStreamEngine::AudioRtcpRrCallback(unsigned long mtl_id, void *info)
{
    QStreamEngine* pEngine = GetQStreamEngine(mtl_id);
    if (pEngine!=NULL)
        return pEngine->AudioRtcpRrCallback(info);
    return SUCCESS;
}
int
QStreamEngine::AudioRtcpSrCallback(unsigned long mtl_id, void *info)
{
    QStreamEngine* pEngine = GetQStreamEngine(mtl_id);
    if (pEngine!=NULL)
        return pEngine->AudioRtcpSrCallback(info);
    return SUCCESS;
}
void
QStreamEngine::AudioRtcpTimestampSetting(unsigned long mtl_id)
{
    QStreamEngine* pEngine = GetQStreamEngine(mtl_id);
    if (pEngine!=NULL)
        pEngine->AudioRtcpTimestampSetting();
}
QStreamEngine* 
QStreamEngine::GetQStreamEngine(unsigned long mtl_id)
{
    QStreamEngine* result = (QStreamEngine*)mtl_id;
    return result;
}
