#include <stdio.h>
#include "QVideoStreamEngine.h"
#include <errno.h>            //errno, semaphone
#include <unistd.h>            //usleep
#include "logging.h"

#define TAG "CQVideoStreamEngine"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) //__logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

#define hr_check(rtn,msg,...) if (rtn<0) LOGE(msg,__VA_ARGS__)

#define SUCCEEDED(rtn) ((rtn>=0)?true:false)
#define S_OK 0
#define S_FALSE -1

CQVideoStreamEngine::CQVideoStreamEngine(int rtpListPort,IEngineCB* pEngineCB)
{
    LOGD("==>CQVideoStreamEngine::CQVideoStreamEngine");
    mpEngineCB = pEngineCB;
    mpEngine = NULL;
    mSourceHeight = 0;
    mSourceWidth = 0;
    mSenderWidth = 0;
    mSenderHeight = 0;

    mRtpListPort = rtpListPort;

    mRemoteWidth = 0;
    mRemoteHeight = 0;
    
    mIsRunning = false;

    mGotFirstIFrame = false;
    mGetRecvH264ParaSet = false;
    mLastFrameIndex = 0;
    mMaxGopIndex    = -1;
    
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
    mRecvErrConcealment.ShowError(true);
    mPollReportThreadHandle = 0;
    mSessionStartTimesec = 0;
    mSessionStartTimeus = 0;
    mEncodeCodecType = CODEC_FFMPEG;
    mDecodeCodecType = CODEC_FFMPEG;
    
    mMaxBitRateKbps = mInitialBitRate = 512;
    mMinBitRateKbps = 50;
    mRecmdBitRateKbps = 100;
    LOGD("<==CQVideoStreamEngine::CQVideoStreamEngine");
}

CQVideoStreamEngine::~CQVideoStreamEngine(void)
{
    LOGD("==>CQVideoStreamEngine::~CQVideoStreamEngine");
    //Stop();

    pthread_mutex_lock(&mEngineLock);
    if (mpEngine!=NULL)
    {
        delete mpEngine; 
        mpEngine = NULL;
    }
    pthread_mutex_unlock(&mEngineLock);
    pthread_mutex_destroy(&mEngineLock);

    LOGD("<==CQVideoStreamEngine::~CQVideoStreamEngine");
}

static const struct
{
    unsigned int          nWidth;
    unsigned int          nMin;
    unsigned int          nMax;    
    unsigned int          nRecommended;
}
bitrate_ranges[] =
{
    { (176 + 352)/2,   60,      880/2,    400/2},
    { (352 + 640)/2,   150/2,     4000/5,   900/4},
    { (640 + 1280)/2,  500/5,     20000/20,  6000/10},
    { (1280 + 1920)/2, 1000/10,    30000/10,  9000/6},
    { 1920 * 2,        2000/10,    40000/10,  11500/6},
};

#define ARRAY_SIZE(_x) (sizeof(_x)/sizeof(_x[0]))
int GetBitrateRanges(unsigned int nWidth, unsigned int* nMin, unsigned int* nMax, unsigned int* nRecommended)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(bitrate_ranges); i++)
    {
        if (bitrate_ranges[i].nWidth > nWidth) 
        {
            *nMin            = bitrate_ranges[i].nMin;
            *nMax            = bitrate_ranges[i].nMax;
            *nRecommended    = bitrate_ranges[i].nRecommended;

            return 0;
        } 
    }
    return -3;
};
//#define MAXIMUM_BITRATE(w,h) ((w*h)/200)
//#define NORMAL_BITRATE(w,h)  ((w*h)/550+20)
//#define MINIMUM_BITRATE(w,h) ((w*h)/1200)
#define min_ex(x,y) (x>0&&y>0)?(x>y?y:x):(x<=0?y:x)
int CQVideoStreamEngine::UpdateMediaSource(int width, int height)
{
    LOGD("==>CQVideoStreamEngine::UpdateMediaSource");
    mSourceWidth    = width;
    mSourceHeight   = height;
    
    GetBitrateRanges(width,&mMinBitRateKbps,&mMaxBitRateKbps,&mRecmdBitRateKbps);
    mMaxBitRateKbps = min(mMaxBitRateKbps,mInitialBitRate);
    mMinBitRateKbps = min(mMaxBitRateKbps,mMinBitRateKbps);
    mRecmdBitRateKbps = min(mMaxBitRateKbps,mRecmdBitRateKbps);
/*    if (mInitialBitRate!=0 && mMaxBitRateKbps>0)
    {
        if (mInitialBitRate<mMinBitRateKbps)
        {
            mMinBitRateKbps = mInitialBitRate;
            mMaxBitRateKbps = mInitialBitRate;
            mRecmdBitRateKbps = mInitialBitRate;
        }else
        {
            mRecmdBitRateKbps = ((mRecmdBitRateKbps-mMinBitRateKbps)*(mInitialBitRate-mMinBitRateKbps)/(mMaxBitRateKbps-mMinBitRateKbps))+mMinBitRateKbps;
            mMinBitRateKbps = mMinBitRateKbps;
            mMaxBitRateKbps = mInitialBitRate;
        }
    }
*/
    mSenderHeight   = 0;
    mSenderWidth    = 0;
    pthread_mutex_lock(&mEngineLock);
    if (mpEngine)
    {
    	mMaxBitRateKbps = mMaxBitRateKbps < mMaxBitRateKbpsBound ? mMaxBitRateKbps : mMaxBitRateKbpsBound;
        mpEngine->SetBitRateBound(mMaxBitRateKbps,mMinBitRateKbps,mRecmdBitRateKbps,mRecmdBitRateKbps);
    }
    pthread_mutex_unlock(&mEngineLock);
    LOGD("<==CQVideoStreamEngine::UpdateMediaSource");
    return 0;
};

unsigned long CQVideoStreamEngine::GetSSRC()
{
    if (mpEngine!=NULL)
    {
        mSsrc = mpEngine->GetSSRC();
        return mSsrc;
    }
    return mSsrc;
}
bool CQVideoStreamEngine::Start(unsigned long ssrc,const char* pRemoteIP, uint32_t rtpDestPort, uint32_t clockRate,uint32_t payLoadType,const char* pMimeType, bool enableTFRC,uint32_t initialFramerate,uint32_t initialBitrate,bool letGopEqFps,bool enableRtcpFbPli,bool enableRtcpFbFir,
								int fecCtrl, int fecSendPType, int fecRecvPType, int maxBitRate, int videoMTU, int videoRTPTOS)
{ 
    LOGD("==>CQVideoStreamEngine::Start");
    LOGI("pli=%d, fir=%d",enableRtcpFbPli,enableRtcpFbFir);
    pthread_mutex_lock(&mEngineLock);
    memset(mRemoteIP,0,sizeof(mRemoteIP));
    strcpy(mRemoteIP,pRemoteIP);
    mRtpDestPort = rtpDestPort;
    mEnableTFRC = enableTFRC;
    
    mEnableRtcpFbPli = enableRtcpFbPli;
    mEnableRtcpFbFir = enableRtcpFbFir;
    mInitialBitRate = initialBitrate;
    mInitialFrameRate = initialFramerate;
    
    mFEC_arg.ctrl = fecCtrl;		//eason - FEC info
    mFEC_arg.send_pt = fecSendPType;
    mFEC_arg.recv_pt = fecRecvPType;
    mMaxBitRateKbpsBound = maxBitRate;
    mVideoMTU = videoMTU;
    mVideoRTPTOS = videoRTPTOS;
/*
    if (mInitialBitRate!=0 && mMaxBitRateKbps>0)
    {
        if (mInitialBitRate<mMinBitRateKbps)
        {
            mMinBitRateKbps = mInitialBitRate;
            mMaxBitRateKbps = mInitialBitRate;
            mRecmdBitRateKbps = mInitialBitRate;
        }else
        {
            mRecmdBitRateKbps = ((mRecmdBitRateKbps-mMinBitRateKbps)*(mInitialBitRate-mMinBitRateKbps)/(mMaxBitRateKbps-mMinBitRateKbps))+mMinBitRateKbps;
            mMinBitRateKbps = mMinBitRateKbps;
            mMaxBitRateKbps = mInitialBitRate;
        }
    }
*/
    mLetGopEqFps = letGopEqFps;
    LOGD(("[VIDEOENGINE]enableTFRC:%d initial=(%ukbps,%ufps) "),enableTFRC,initialBitrate,initialFramerate);
    mClockRate                      = clockRate;
    mPayloadType                    = payLoadType;
    memset(mMimeType,0,sizeof(mMimeType));
    strcpy(mMimeType,pMimeType);
    mIsH264 = (strcmp("H264",mMimeType)==0 || strcmp("h264",mMimeType)==0) ;
   
    mpEngine = new QStreamEngine(this,
                                 mRtpListPort,                   //Recv Listen Port
                                 QS_MEDIATYPE_VIDEO,             //Media Type
                                 mFEC_arg
                                 );
    
    pthread_mutex_unlock(&mEngineLock);
    mSsrc = ssrc;
    LOGD("<==CQVideoStreamEngine::Start");
   return InternalStart(false);
};
bool CQVideoStreamEngine::InternalStart(bool isRestart)
{
    LOGD("==>CQVideoStreamEngine::InternalStart %d",isRestart);
    int hr = S_OK;
    mRemoteWidth  = 0;
    mRemoteHeight = 0;
    mSenderWidth  = 0;
    mSenderHeight = 0;
    mSendFrameRate = 0;
    mSendGop = 0;
    mSendKbps = 0;
    mLastIDRDTimeMs = 0;
    mFirstDemandIDR = true;
    mDemandIDR = true;
    mGotFirstIFrame = false;
    mGetRecvH264ParaSet = false;
    mLastFrameIndex = -1;
    mMaxGopIndex    = 0;
    mRecvH264ParserCount = 0;
    mRecvErrConcealment.Reset();
    mRecvErrConcealment.ShowError(true);

    memset(&mRecvH264ParaSet,0,sizeof(mRecvH264ParaSet));
    pthread_mutex_lock(&mEngineLock);
    if (mpEngine)
    {
        mIsRunning = true;
        //if (mMaxBitRateKbps>0)
        //    mpEngine->SetBitRateBound(mMaxBitRateKbps,mMinBitRateKbps,mRecmdBitRateKbps,mMaxBitRateKbps);
        //Start Engine
        if (isRestart)
        {
            hr = mpEngine->Restart(mEnableTFRC);
            hr_check(hr,"Faield on mpEngine->Restart %d",hr);
        }else
        {
            hr = mpEngine->Start(mEnableTFRC,mSsrc,mRemoteIP,mRtpDestPort,mClockRate,mPayloadType,mMimeType, mVideoMTU, mVideoRTPTOS);
            hr_check(hr,"Faield on mpEngine->Start %d",hr);
        }
    }
    pthread_mutex_unlock(&mEngineLock);
    if (mpEngineCB)
    {
    	mMaxBitRateKbps = min(mMaxBitRateKbps,mInitialBitRate);
        mMinBitRateKbps = min(mMaxBitRateKbps,mMinBitRateKbps);
        mRecmdBitRateKbps = min(mMaxBitRateKbps,mRecmdBitRateKbps);
        mSendFrameRate = min(mInitialFrameRate,30);
//        mpEngineCB->SetFrameRate(mSendFrameRate);
        mSendKbps = min(mInitialBitRate,mMaxBitRateKbps);
//        mSendFrameRate = mInitialFrameRate;
        mpEngineCB->SetBitRate(mSendKbps);
        LOGD("mEnableTFRC:%d, mSendKbps:%d",mEnableTFRC,mSendKbps);
        if (mLetGopEqFps)
        {
            mSendGop = mSendFrameRate; 
            mpEngineCB->SetGop(mSendGop);
        }else
        {
            mpEngineCB->SetGop(65535);
        }
        
        pthread_mutex_lock(&mEngineLock);
        if (mpEngine)
        {
            mMaxBitRateKbps = mMaxBitRateKbps < mMaxBitRateKbpsBound ? mMaxBitRateKbps : mMaxBitRateKbpsBound;
            mpEngine->SetBitRateBound(mMaxBitRateKbps,mMinBitRateKbps,mRecmdBitRateKbps, mRecmdBitRateKbps);
        }
        pthread_mutex_unlock(&mEngineLock);
    }
    int   ret ;
    // Initialize event semaphore
    ret = sem_init(&mStopPollEvent,   // handle to the event semaphore
                       0,     // not shared
                       0);    // initially set to non signaled state
    if(ret)
        LOGE("sem_init mStopPollEvent error code=%d\n",ret);

    // Create the threads for polling report
    ret = pthread_create(&mPollReportThreadHandle, NULL, PollReportThread, (void*)this);
    if(ret)
        LOGE("pthread_create mPollReportThreadHandle thread error code=%d\n",ret);

    if (!SUCCEEDED(hr))
    {
        Stop();
    }
    LOGD("<==CQVideoStreamEngine::InternalStart %d",hr);
    return SUCCEEDED(hr);
}

bool CQVideoStreamEngine::Restart(void)
{
    LOGD("CQVideoStreamEngine::Restart");
    Stop();
    return InternalStart(true);
};

int CQVideoStreamEngine::Stop(void)
{
    LOGD("==>CQVideoStreamEngine::Stop");
    int ret;

    mIsRunning = false;
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
        delete mpEngine;
        mpEngine = NULL;
    }
    pthread_mutex_unlock(&mEngineLock);
    mRemoteWidth = 0;
    mRemoteHeight = 0;
    LOGD("<==CQVideoStreamEngine::Stop");
    return S_OK;
};

//Implement QStreamVideoCB ==========================================
void CQVideoStreamEngine::OnRecv(unsigned char * pData, uint32_t length, uint32_t timestamp, uint16_t rotation, bool isPartial,unsigned char nalType)
{
    if (!mIsRunning)
    {
        LOGE(("OnRecv Engine Already Stop"));
        return ;
    }
    if (isPartial)
        mDemandIDR = true;

    if(!isPartial && (nalType&NAL_TYPE_MASK) == NAL_TYPE_SEQ_PARA_SET) 
    {
        if (QS_TRUE == h264_get_parameter_set(pData, (int)length, &mRecvH264ParaSet))
        {
            LOGD("CQVideoStreamEngine::OnH264Recv Got IFrame");
            mDemandIDR = false;
            mGetRecvH264ParaSet = true;
            mGotFirstIFrame = true;
            int width = (mRecvH264ParaSet.pic_width_in_mbs_minus1+1)*16;
            int height = (mRecvH264ParaSet.pic_height_in_map_units_minus1+1)*16;
            //Set Resolution
            SetDecodeResolution(width,height);
        }
    }
    if (mGotFirstIFrame)
    {
        if (mRemoteWidth!=0 && mRemoteHeight!=0)
        {
            //OnErrConcealment(pData,length,timestamp,isPartial);
            if (OnErrConcealment(pData,length,timestamp,isPartial))
            {   //Save Data to buffer
                /*if (mpEngineCB)
                {
                    //LOGD(("mpEngineCB->OnRecv length=%d rotation=%d"),length,rotation);
                    if (!mpEngineCB->OnRecv(pData,length,timestamp,rotation)){
                        mDemandIDR = true;
                    }
                }else
                    LOGE("OnRecv: mpEngineCB is NULL");
                */
            }else
            {
                mDemandIDR = true;
            }
            if (mpEngineCB)
            {
                //LOGD(("mpEngineCB->OnRecv length=%d rotation=%d"),length,rotation);
                if (!mpEngineCB->OnRecv(pData,length,timestamp,rotation)){
                    mDemandIDR = true;
                }
            }else
                LOGE("OnRecv: mpEngineCB is NULL");
        }else
        {
            mDemandIDR = true;
            LOGE("OnRecv: mRemoteWidth:mRemoteHeight %dX%d dismatch, mGotFirstIFrame=%d",mRemoteWidth,mRemoteHeight, mGotFirstIFrame);
        }
    }else
    {
        mDemandIDR = true;
        LOGE("OnRecv didn't get first IFrame partial:%d mGotFirstIFrame=%d",isPartial,mGotFirstIFrame);
    }
    InternalDemandIDRCheck();
}
void CQVideoStreamEngine::SetDecodeResolution(int width,int height)
{
    LOGD("==>SetDecodeResolution:%dX%d source:%dX%d",width,height,mRemoteWidth,mRemoteHeight);
    if (mpEngineCB && (mRemoteWidth!=width || mRemoteHeight!=height))
    {
        mRemoteWidth = width;
        mRemoteHeight = height;
        mpEngineCB->OnResolutionChange(width, height);
    }
    LOGD("<==SetDecodeResolution");
}

void CQVideoStreamEngine::SetEncodeBitRate(unsigned long bitRate)
{
//    LOGD("==>SetEncodeBitRate:%u",bitRate);
    if (mpEngineCB && mEnableTFRC && mSendKbps!=bitRate)
    {
        LOGD("SetEncodeBitRate:%u old:%u, mEnableTFRC:%d",bitRate,mSendKbps, mEnableTFRC);
        mSendKbps = bitRate;
        mpEngineCB->SetBitRate(mSendKbps);
        
    }
//    LOGD("<==SetEncodeBitRate");
}
#define NEW_CONTROL
void CQVideoStreamEngine::SetEncodeFrameRate(unsigned char fps)
{
#ifdef NEW_CONTROL
    //minimize the gap between real framerate and desired framerate 
    if (fps>8)
    {
        if (abs(fps - mStreamReport.send_frame_rate_fps)>=2)
            fps = max(6,(mStreamReport.send_frame_rate_fps + 2));
    } 
    if (mpEngineCB && mEnableTFRC && abs(mSendFrameRate-fps)>1)
    {  
        mSendFrameRate = fps;
        mpEngineCB->SetFrameRate(mSendFrameRate);
        if (mLetGopEqFps)
        {
            mSendGop = mSendFrameRate; 
            mpEngineCB->SetGop(mSendFrameRate);
        }
    }
    /*int gop = (mSendGop*7+fps*3)/10;
    if (mStreamReport.send_frame_rate_fps>1)
    {
        gop = (mSendGop*5+fps*2+mStreamReport.send_frame_rate_fps*3)/10;
    }

    if (mpEngineCB && abs(mSendGop-gop)>1)  //try to avoid gop changed too frequently
    {
        //mpEngineCB->SetGop(gop);
        //mSendGop = gop;
    }*/
#else
    //LOGD("==>SetEncodeFrameRate:%u",frameRate);
    int tmp_frame_rate = mSendFrameRate;
    int tmp_gop        = mSendGop;
    if (tmp_frame_rate==0)
        tmp_frame_rate = fps;
    if (tmp_gop==0)
        tmp_gop = fps;
    int gop = fps;
    int frameRate = (tmp_frame_rate+fps+1)/2;
    if (mStreamReport.send_frame_rate_fps>1)
    {
        gop = (tmp_gop+fps+mStreamReport.send_frame_rate_fps+1)/3;
    }else
        gop = (tmp_gop+fps+1)/2;

    if (mpEngineCB && mEnableTFRC && mSendFrameRate!=frameRate)
    {
        mpEngineCB->SetFrameRate(frameRate);
        mSendFrameRate = frameRate;
    }

    if (mpEngineCB && mEnableTFRC && mSendGop!=gop)
    {
        mpEngineCB->SetGop(gop);
        mSendGop = gop;
    }
#endif
//    LOGD("<==SetEncodeFrameRate:%u",frameRate);
}

bool CQVideoStreamEngine::OnErrConcealment(unsigned char* pStreamData, unsigned int dataLength, uint32_t timestamp, bool isPartial)
{
    int slice_type = 0;
    unsigned short frame_index = h264_get_frame_num(pStreamData,dataLength,&slice_type,mRecvH264ParaSet.log2_max_frame_num_minus4,&mRecvH264ParserCount);
   
    bool result = mRecvErrConcealment.OnErrConcealment(slice_type,frame_index,isPartial);
    if (!result)
    {
        LOGE("CQVideoStreamEngine::OnErrConcealment:%d,index:%d PartialFrame:%d",result,frame_index,isPartial);
    }
    //Dump to File
    /*FILE * pFile;
    pFile = fopen ("/sdcard/recv.h264","a+b");
    if (pFile!=NULL)
    {
        fwrite (pStreamData , 1 , dataLength , pFile );
        fclose (pFile);
    }*/
    return result;
}
void CQVideoStreamEngine::SetLipSyncToken(LipSyncToken* pToken)
{
    mLipSyncControl.SetLipSyncToken(pToken);
}
bool CQVideoStreamEngine::OnLipSyncCheck(uint32_t currentRecvSize,long captureClockSec,long captureClockUsec)
{
    //LOGE(("VIDEO[RECV]:sec:%u\tusec:%u"),captureClockSec,captureClockUsec);
    bool result = mLipSyncControl.CheckBufferSize(mGotFirstIFrame,currentRecvSize,mpEngineCB->GetRecvBufferSize(),mLastFrameIndex==mMaxGopIndex) &&
                  mLipSyncControl.CheckFrameTimeStamp(captureClockSec,captureClockUsec);
    return result;
}

void CQVideoStreamEngine::OnFailedEnableTFRC()
{
    LOGD("==>OnFailedEnableTFRC:"+mEnableTFRC);
    //If the tfrc is enable
    if (mEnableTFRC)
    {   //disable TFRC and Restart stream engine
        LOGE("OnFailedEnableTFRC, disable TFRC");
        mEnableTFRC = false;
        // Create the threads for polling report
        pthread_t thread;
        int ret = pthread_create(&thread, NULL, RestartThread, (void*)this);
    }
    LOGD("<==OnFailedEnableTFRC:");
}

void CQVideoStreamEngine::OnSend(uint8_t * pData, long length, double sampleTime, short rotation)
{
    //LOGD("==>OnSend:");
    //Detect sender resolution changing
    if (mSenderHeight!=mSourceHeight ||
        mSenderWidth !=mSourceWidth)
    {
        if (mIsH264)
        {
            if (0==h264_get_resolution(pData,length,&mSenderHeight,&mSenderWidth))
            {
                //LOGE(("OnSend => %dX%d"),mSenderWidth,mSenderHeight);
                if (mSourceHeight%16!=0 && (mSenderHeight/16 == (mSourceHeight/16+1)))
                    mSourceHeight = mSenderHeight;
                if (mSourceWidth%16!=0 && (mSenderWidth/16 == (mSourceWidth/16+1)))
                    mSourceWidth = mSenderWidth;
            }
        }
        else
        {
            mSenderHeight = mSourceHeight;
            mSenderWidth = mSourceWidth;
        }
    }
    //Handle the resolution change, [If resolution changed waiting for the new first Iframe with new resolution]
    if ((mSenderHeight==mSourceHeight &&
        mSenderWidth ==mSourceWidth))
    {
        struct timeval clock;

        memset(&clock,0,sizeof(clock));
        //gettimeofday(&clock, 0);
        clock.tv_sec    = sampleTime/1000;
        clock.tv_usec   = (((long long)(sampleTime)) % 1000) *1000;
        uint32_t timestamp = clock.tv_sec * mClockRate +
                            (uint32_t)((double)clock.tv_usec * (double)mClockRate * 1.0e-6);
        //pthread_mutex_lock(&mEngineLock);
        if (mpEngine!=NULL && mIsRunning)
        {
            mpEngine->SendData(pData,length,timestamp,clock, rotation,mLetGopEqFps);
        }
        //pthread_mutex_unlock(&mEngineLock);
    }else
        LOGE(("OnSend => RS mismatched: %dX%d settings:%dX%d"),mSenderWidth,mSenderHeight,mSourceWidth,mSourceHeight);

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
void* CQVideoStreamEngine::RestartThread(void* pParam)
{   //Thread to restart streaming engine
    LOGD("==>RestartThread:");
    CQVideoStreamEngine* parent = (CQVideoStreamEngine*) pParam;
    if (parent!=NULL && parent->mpEngine!=NULL)
    {
        pthread_mutex_lock(&parent->mEngineLock);
        parent->mpEngine->Restart(parent->mEnableTFRC);
        pthread_mutex_unlock(&parent->mEngineLock);
    }
    LOGD("<==RestartThread:");
    return 0;
}
void* CQVideoStreamEngine::PollReportThread(void* pParam)
{
    CQVideoStreamEngine* parent = (CQVideoStreamEngine*) pParam;
    LOGD("==>PollReportThread");
    if (parent!=NULL)
    {
        memset(&(parent->mStreamReport),0,sizeof(parent->mStreamReport));
        while(!WaitForSingleObject(&parent->mStopPollEvent,2))
        //while(sem_timedwait(&parent->mStopPollEvent,&timeout))
        {
            if (!parent->mIsRunning)
                break;
            pthread_mutex_lock(&parent->mEngineLock);
            if (parent->mpEngine)
            {
                parent->mpEngine->GetReport(&(parent->mStreamReport));
            }
            pthread_mutex_unlock(&parent->mEngineLock);
            
            parent->mLipSyncControl.OnRTTUpdate(parent->mStreamReport.rtt_us/1000);
            parent->mLipSyncControl.OnUpdateFrameRate(parent->mStreamReport.recv_frame_rate_fps);
            if (parent->mStreamReport.recv_bit_rate_kbs==0)
                parent->mLipSyncControl.CheckFrameTimeStamp(0,0);   //Reset video timestamp @ LibSyncToken
            if (parent->mpEngineCB!=NULL)
            {
                parent->mpEngineCB->OnReport(parent->mStreamReport.recv_bit_rate_kbs,
                                             parent->mStreamReport.recv_frame_rate_fps,
                                             parent->mStreamReport.send_bit_rate_kbs,
                                             parent->mStreamReport.send_frame_rate_fps,
                                             parent->mStreamReport.package_loss,
                                             parent->mStreamReport.rtt_us);
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

unsigned long current_time_ms()
{
    struct timeval          curr_time;
    gettimeofday(&curr_time, NULL);
    return curr_time.tv_sec*1000+curr_time.tv_usec/1000;
}//This method send IDR request with SIP or RTCP to remote 
void CQVideoStreamEngine::InternalDemandIDRCheck()
{
    //LOGD("==>CQVideoStreamEngine::InternalDemandIDRCheck %d",mDemandIDR);
    if (mDemandIDR)
    {
        unsigned long gap = (current_time_ms()-mLastIDRDTimeMs);
        if (gap > 3000 ||
            (mFirstDemandIDR && gap>1000))
        {    
            LOGD("pli:%d, fir:%d", mEnableRtcpFbPli, mEnableRtcpFbFir);
            mFirstDemandIDR = false;
            if (mEnableRtcpFbPli)
            {
                LOGD("OnDemandIDR: Rtcp FB PLI");
                //Request IDR with RTCP-FB PLI
                if (mpEngine)
                {
                    mpEngine->SendRtcpFbPli();
                }
            }else if (mEnableRtcpFbFir)
            {
                LOGD("OnDemandIDR: Rtcp FB FIR");
                //Request IDR with RTCP-FB FIR
                if (mpEngine)
                {
                    mpEngine->SendRtcpFbFir();
                }
            }else
            {
                LOGD("OnDemandIDR: SIP INFO");
                //Request IDR with SIP INFO
                if (mpEngineCB!=NULL)
                    mpEngineCB->OnDemandIDR();
            }
            mLastIDRDTimeMs = current_time_ms();
        }
    }
    //LOGD("<==CQVideoStreamEngine::InternalDemandIDRCheck");
}

//This method generate a new IDR
void CQVideoStreamEngine::RequestIDR()
{
    //LOGD("==>CQVideoStreamEngine::RequestIDR");
    //Request IDR with SIP INFO
    if (mpEngineCB!=NULL)
        mpEngineCB->OnRequestIDR();
    //LOGD("<==CQVideoStreamEngine::RequestIDR");
}
//<===================RTP Packet Loss detection====================>
void CQVideoStreamEngine::OnPacketLost()
{
    mDemandIDR = true;
}
//</===================RTP Packet Loss detection====================>
