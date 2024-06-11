#include <stdio.h>
#include "QAudioStreamEngine.h"
#include <errno.h>            //errno, semaphone
#include <unistd.h>            //usleep
#include "logging.h"

#define TAG "CQAudioStreamEngine"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

#define hr_check(rtn,msg,...) if (rtn<0) LOGE(msg,__VA_ARGS__)
#define DEFAULT_WIDTH 320
#define DEFAULT_HEIGHT 240
#define SUCCEEDED(rtn) ((rtn>=0)?true:false)
#define S_OK 0
#define S_FALSE -1

extern void on_thread_begin();
extern void on_thread_end();

CQAudioStreamEngine::CQAudioStreamEngine(int rtpListPort,IEngineCB* pEngineCB)
{
    LOGD("==>CQAudioStreamEngine::CQAudioStreamEngine");
    mpEngineCB = pEngineCB;
    mpEngine = NULL;
    
    mRtpListPort = rtpListPort;

    
    mIsRunning = false;
    mClockRate                      = 0;
    mPayloadType                    = 0;
    memset(mMimeType,0,sizeof(char)*MAX_PATH);
    pthread_mutexattr_t mutexattr;   // Mutex attribute variable
    // Set the mutex as a recursive mutex
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    // create the mutex with the attributes set
    pthread_mutex_init(&mEngineLock, &mutexattr);
    //After initializing the mutex, the thread attribute can be destroyed
    pthread_mutexattr_destroy(&mutexattr);

    mPollReportThreadHandle = 0;
    mSessionStartTimesec = 0;
    mSessionStartTimeus = 0;
    mEncodeCodecType = CODEC_FFMPEG;
    mDecodeCodecType = CODEC_FFMPEG;
    LOGD("<==CQAudioStreamEngine::CQAudioStreamEngine");
}

CQAudioStreamEngine::~CQAudioStreamEngine(void)
{
    LOGD("==>CQAudioStreamEngine::~CQAudioStreamEngine");
    Stop();

    pthread_mutex_lock(&mEngineLock);
    if (mpEngine!=NULL)
    {
        delete mpEngine; 
        mpEngine = NULL;
    }
    pthread_mutex_unlock(&mEngineLock);
    
    pthread_mutex_destroy(&mEngineLock);

    LOGD("<==CQAudioStreamEngine::~CQAudioStreamEngine");
}

unsigned long CQAudioStreamEngine::GetSSRC()
{
    if (mpEngine!=NULL)
    {
        mSsrc = mpEngine->GetSSRC();
        return mSsrc;
    }
    return 0;
}
bool CQAudioStreamEngine::Start(unsigned long ssrc,const char* pRemoteIP, uint32_t rtpDestPort, uint32_t clockRate,uint32_t payLoadType,const char* pMimeType, int fecCtrl, int fecSendPType, int fecRecvPType, int audioRTPTOS)
{ 
    LOGD("==>CQAudioStreamEngine::Start");
    pthread_mutex_lock(&mEngineLock);
    mSsrc = ssrc;
    memset(mRemoteIP,0,sizeof(mRemoteIP));
    strcpy(mRemoteIP,pRemoteIP);
    mRtpDestPort = rtpDestPort;
    mClockRate                      = clockRate;
    mPayloadType                    = payLoadType;
    mMinGapTimeStamp = (int)mClockRate/50;
    memset(mMimeType,0,sizeof(mMimeType));
    strcpy(mMimeType,pMimeType);
    mFEC_arg.ctrl = fecCtrl;
    mFEC_arg.send_pt = fecSendPType;
    mFEC_arg.recv_pt = fecRecvPType;
    mAudioRTPTOS = audioRTPTOS;
   
    mpEngine = new QStreamEngine(this,
                                 mRtpListPort,                   //Recv Listen Port
                                 QS_MEDIATYPE_AUDIO,             //Media Type
                                 mFEC_arg
                                 );
    pthread_mutex_unlock(&mEngineLock);
    LOGD("<==CQAudioStreamEngine::Start");
   return InternalStart(false);
};
bool CQAudioStreamEngine::InternalStart(bool isRestart)
{
    LOGD("==>CQAudioStreamEngine::InternalStart %d ",isRestart);
    
    int hr = S_OK;
    mTimeStamp = 0;
    pthread_mutex_lock(&mEngineLock);
    if (mpEngine)
    {
        //Start Engine
        if (isRestart)
        {
            hr = mpEngine->Restart();
            hr_check(hr,"Faield on mpEngine->Restart %d",hr);
        }else
        {
            hr = mpEngine->Start(mSsrc, mRemoteIP,mRtpDestPort,mClockRate,mPayloadType,mMimeType, mAudioRTPTOS);
            hr_check(hr,"Faield on mpEngine->Start %d",hr);
        }
    }
    pthread_mutex_unlock(&mEngineLock);
   
    int   ret ;
    // Initialize event semaphore
    ret = sem_init(&mStopPollEvent,   // handle to the event semaphore
                       0,     // not shared
                       0);    // initially set to non signaled state
    if(ret)
        LOGE("sem_init mStopPollEvent error code=%d\n",ret);

    ret = sem_init(&mStopThreadSuccessEvent,   // handle to the event semaphore
                       0,     // not shared
                       0);    // initially set to non signaled state
    if(ret)
        LOGE("sem_init mStopThreadSuccessEvent error code=%d\n",ret);
//#ifdef ENABLE_STATISTIC_COUNTING
    // Create the threads
    ret = pthread_create(&mPollReportThreadHandle, NULL, PollReportThread, (void*)this);
    if(ret)
        LOGE("pthread_create mPollReportThreadHandle thread error code=%d\n",ret);
//#endif
    if (SUCCEEDED(hr))
        mIsRunning = true;
    else
        Stop();
    LOGD("<==CQAudioStreamEngine::InternalStart %d mClockRate=%d",hr,mClockRate);
    return SUCCEEDED(hr);
}

bool CQAudioStreamEngine::Restart(void)
{
    LOGD("CQAudioStreamEngine::Restart");
    Stop();
    return InternalStart(true);
};

int CQAudioStreamEngine::Stop(void)
{
    LOGD("==>CQAudioStreamEngine::Stop");
    int ret;

    sem_post(&mStopPollEvent);
    if (mPollReportThreadHandle!=0)
    {
        // Wait for the thread finished
        ret = pthread_join(mPollReportThreadHandle, NULL);
        if(ret)
            LOGE("Terminate mPollReportThreadHandle thread error code=%d\n",ret);
        mPollReportThreadHandle = 0;
    }
    ret = sem_destroy(&mStopPollEvent);
    if(ret)
        LOGE("Destroy mStopPollEvent error code=%d\n",errno);

    pthread_mutex_lock(&mEngineLock);
    if(mpEngine != NULL)
    {
        mpEngine->Stop();
    }
    pthread_mutex_unlock(&mEngineLock);
    mIsRunning = false;
    LOGD("<==CQAudioStreamEngine::Stop");
    return S_OK;
};

//Implement QStreamVideoCB ==========================================
void CQAudioStreamEngine::OnRecv(unsigned char * pData, uint32_t length, uint32_t timestamp)
{
    if (!mIsRunning)
    {
        LOGE(("OnRecv Engine Already Stop"));
        return ;
    }
    //Dump to File
//    FILE * pFile;
//    pFile = fopen ("/sdcard/audio_recv.g7221","a+b");
//    if (pFile!=NULL)
//    {
//        fwrite (pData , 1 , length , pFile );
//        fclose (pFile);
//    }
    //Send Data to the receiver
    if (mpEngineCB)
        mpEngineCB->OnRecv(pData,length,timestamp);

}

bool CQAudioStreamEngine::OnLipSyncCheck(long captureClockSec,long captureClockUsec)
{
    //LOGE(("VIDEO[RECV]:sec:%u\tusec:%u"),captureClockSec,captureClockUsec);
//    return mLipSyncControl.CheckBufferSize(currentRecvSize) &&
//           mLipSyncControl.CheckFrameTimeStamp(captureClockSec,captureClockUsec);
    return true;
}

void CQAudioStreamEngine::OnSend(uint8_t * pData, long length, double sampleTime, int dtmfKey)
{
    //Dump to File
//    FILE * pFile;
//    pFile = fopen ("/sdcard/audio_send.g7221","a+b");
//    if (pFile!=NULL)
//    {
//        fwrite (pData , 1 , length , pFile );
//        fclose (pFile);
//    }
//    LOGD("CQAudioStreamEngine::OnSend:%02X,%d",pData, length);
    pthread_mutex_lock(&mEngineLock);
    if (mpEngine!=NULL)
    {
        struct timeval clock;

        memset(&clock,0,sizeof(clock));
//        gettimeofday(&clock, 0);
//
//        uint32_t timestamp = clock.tv_sec * mClockRate +
//                           (uint32_t)((double)clock.tv_usec * (double)mClockRate * 1.0e-6);
        clock.tv_sec    = sampleTime/1000;
        clock.tv_usec   = (((long long)(sampleTime)) % 1000) *1000;
        uint32_t timestamp = clock.tv_sec * mClockRate +
                            (uint32_t)((double)clock.tv_usec * (double)mClockRate * 1.0e-6);
        int gap = 0;
        if (mTimeStamp!=0)
        {
            gap = (int)(timestamp - mTimeStamp);
            if (gap <= mMinGapTimeStamp)
            {
                timestamp = mTimeStamp + mMinGapTimeStamp;
                int rtn = mpEngine->SendData(pData,length,timestamp,clock,dtmfKey);
            }else
            {
                //LOGE("AUDIO Sender: Drop audio data for timeout: %d, %u - %u",gap,timestamp,mTimeStamp);
                //timestamp = 0;
                int rtn = mpEngine->SendData(pData,length,timestamp,clock,dtmfKey);
            }
        }else
        {
            int rtn = mpEngine->SendData(pData,length,timestamp,clock,dtmfKey);
        }
        //LOGD("CQAudioStreamEngine::OnSend:timestamp:%u,mTimeStamp:%u,gap:%d",timestamp,mTimeStamp, gap);
        //int rtn = mpEngine->SendData(pData,length,timestamp,clock);
        mTimeStamp = timestamp;
    }
    pthread_mutex_unlock(&mEngineLock);
}
bool WaitForSingleObject(sem_t* sem,uint32_t sec)
{
    bool             result = false;
    int              ret;
    uint32_t        timeout=0;

    while (timeout < sec )
    {
       // Wait for the event be signaled
       ret = sem_trywait(sem); // event semaphore handle
                              // non blocking call
       if (!ret)  {
            /* Event is signaled */
               result = true;
            break;
       }
       else {
           /* check whether somebody else has the mutex */
//           if (ret == EPERM ) {
                /* sleep for delay time */
                usleep(1000000);
                timeout++ ;/* 1 sec */
//           }
//           else{
//               /* error  */
//               break;
//           }
       }
    }
    return result;
}
void* CQAudioStreamEngine::PollReportThread(void* pParam)
{
    CQAudioStreamEngine* parent = (CQAudioStreamEngine*) pParam;
    //struct timespec timeout = {0, 0};
    //timeout.tv_sec = time(NULL) + 1;
    LOGD("==>PollReportThread");
    if (parent!=NULL)
    {
        memset(&(parent->mStreamReport),0,sizeof(parent->mStreamReport));
        while(!WaitForSingleObject(&parent->mStopPollEvent,3))
        //while(sem_timedwait(&parent->mStopPollEvent,&timeout))
        {
            pthread_mutex_lock(&parent->mEngineLock);
            if (parent->mpEngine)
            {
                parent->mpEngine->GetReport(&(parent->mStreamReport));
            }
            pthread_mutex_unlock(&parent->mEngineLock);
            
            if (parent->mpEngineCB!=NULL)
            {
                parent->mpEngineCB->OnReport(parent->mStreamReport.recv_bit_rate_kbs,
                                             parent->mStreamReport.send_bit_rate_kbs,
                                             parent->mStreamReport.recv_sample_rate_fbs,
                                             parent->mStreamReport.send_sample_rate_fbs,
                                             parent->mStreamReport.recv_gap_sec,
                                             parent->mStreamReport.send_gap_sec);
            }
        }
    }
    LOGD("<==PollReportThread");
    //pthread_exit(0);
    //pthread_exit Limitations
    //When using STL containers as local objects, the destructor will not get called when pthread_exit() is called, which leads to a memory leak. Do not call pthread_exit() from C++. Instead you must throw or return back to the thread's initial function. There you can do a return instead of pthread_exit().
    //Pthread library has no knowledge of C++ stack unwinding. Calling pthread_exit() for will terminate the thread without doing proper C++ stack unwind. That is, destructors for local objects will not be called. (This is analogous to calling exit() for single threaded program.)
    //This can be fixed by calling destructors explicitly right before calling pthread_exit().
    return 0;
}
