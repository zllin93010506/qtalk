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
#include <netinet/in.h>
#define TAG "QStreamEngine"
#define TEST(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) //__logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

#define RTP_EXT_HEAD   0x93
#define RTP_EXT_TAIL   0x98

#pragma pack(push) /* push current alignment to stack */
#pragma pack(1) /* set alignment to 1 byte boundary */
/* struct size must be mutiple of 4 */
typedef struct {
    uint32_t frame_len;
    uint8_t  head;
    uint16_t rotation;
    uint8_t  tail;
} RTP_Eextern_Info_t;
#pragma pack(pop) /* restore original alignment from stack */

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
#define MAX_VIDEO_CODEC_BUFFER_SIZE         (256000)    // bytes

QStreamEngine::QStreamEngine(QStreamVideoCB* pStreamCB,int rtpListenPort, int qsMediaType, FEC_t fec_arg)
{
    LOGD("==>QStreamEngine Video");
    mpStreamVideoCB                 = pStreamCB;
    mMediaType                      = qsMediaType;
    mRtpListPort                    = rtpListenPort;
    
    mClockRate                      = 0;
    mPayloadType                    = 0;
    memset(mMimeType,0,sizeof(char)*MAX_PATH);
    
    mFEC_arg.ctrl                   = fec_arg.ctrl;
    mFEC_arg.send_pt                = fec_arg.send_pt;
    mFEC_arg.recv_pt                = fec_arg.recv_pt;
    
    memset(&mNetCallback,0,sizeof(mNetCallback));
    mNetCallback.rtcp_rr            = QStreamEngine::VideoRtcpRrCallback;
    mNetCallback.rtcp_sr            = QStreamEngine::VideoRtcpSrCallback;
    mNetCallback.tfrc_report        = QStreamEngine::VideoTfrcReportCallback;
    //mNetCallback.getBuf             = NULL;
    mNetCallback.timestamp_setting  = QStreamEngine::VideoRtcpTimestampSetting;
    mNetCallback.rtcp_app_set       = NULL;
    mNetCallback.rtcp_app_receive   = NULL;
    mNetCallback.dtmf_event         = NULL;
	
    mNetCallback.rtcp_psfb_receive  = QStreamEngine::VideoPsfbCallback;    //RTCP_FB_PLI, RTCP_FB_FIR (IDR request from remote)
    mNetCallback.rtp_rx             = QStreamEngine::VideoRtpReceive;

    //Intialize the parameter
    mEngineThreadHandle             = NULL;
    mIsEngineStop                   = true;
    mIsEngineReady                  = false;
     mRxBuffer = NULL;
    mRxBufferSize = 0;
    mSsrc = 0;
//    HANDLE hMutex = CreateMutexA(NULL, false, "QSCE_InitLibrary_Mutex");
//    if (hMutex != NULL && GetLastError() == ERROR_ALREADY_EXISTS)
//        LOGD("QStreamEngine QSCE_InitLibrary_Mutex Already on calling(Video)"));
//    else
//        QSCE_InitLibrary();
//    if (hMutex!=NULL)
//        CloseHandle(hMutex);    //the mutex
    pthread_mutexattr_t mutexattr;   // Mutex attribute variable

    // Set the mutex as a recursive mutex
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);

    // create the mutex with the attributes set
    pthread_mutex_init(&mSessionLock, &mutexattr);
    pthread_mutex_init(&mEngineThreadLock, &mutexattr);
    pthread_mutex_init(&mTimeStampLock, &mutexattr);

    //After initializing the mutex, the thread attribute can be destroyed
    pthread_mutexattr_destroy(&mutexattr);

    LOGD("<==QStreamEngine Video: FECCtrl: %d FECSPType:%d FECRPType:%d", mFEC_arg.ctrl, mFEC_arg.send_pt, mFEC_arg.recv_pt);
}

QStreamEngine::~QStreamEngine(void)
{
    LOGD("==>~QStreamEngine Video");
    if (!mIsEngineStop)
        Stop();
    pthread_mutex_destroy(&mSessionLock);
    pthread_mutex_destroy(&mTimeStampLock);
    pthread_mutex_destroy(&mEngineThreadLock);
    LOGD("<==~QStreamEngine Video");
}
void QStreamEngine::GetReport(StreamReport* pStreamReport)
{
//    LOGD("==>GetReport");
    if (pStreamReport!=NULL)
    {  
        pStreamReport->send_bit_rate_kbs = mStatistics.GetSendBitrate();
        pStreamReport->send_frame_rate_fps = mStatistics.GetSendFramerate();
        pStreamReport->recv_bit_rate_kbs = mStatistics.GetRecvBitrate();
        pStreamReport->recv_frame_rate_fps = mStatistics.GetRecvFramerate();
        pStreamReport->package_loss = mStatistics.GetRtpRecvLostRate();
        pStreamReport->rtt_us = mStatistics.GetRTTus();
    }
//    LOGD("<==GetReport");
}
int
QStreamEngine::Start(bool enableTfrc,unsigned long ssrc,const char* pRemoteIP, int rtpDestPort, uint32_t clockRate,uint32_t payLoadType,const char* pMimeType, int mtu, int tos)
{
    if (!pRemoteIP)
        return EINVAL;
    LOGD("==>Start Video ssrc:%d ip:%s,rtpDestPort:%d,clockRate:%d,payLoadType:%d,pMimeType:%s MTU:%d TOS:%d",ssrc,pRemoteIP,rtpDestPort,clockRate,payLoadType,pMimeType, mtu, tos);
    int rtn = FAIL;

    mEnableTfrc                     = enableTfrc;
    mClockRate                      = clockRate;
    mPayloadType                    = payLoadType;
    memset(mMimeType,0,sizeof(char)*MAX_PATH);
    strcpy(mMimeType,pMimeType);
    //Set Destination IP
    memset(mRemoteIP,0,MAX_PATH);
    strcpy(mRemoteIP,pRemoteIP);
    //Set Destination RTP Port
    mRtpDestPort = rtpDestPort;
    mTimeConvertor.SetSampleRate(clockRate);
    mSsrc = ssrc;

	mMTU = mtu;
	mTOS = tos;

    //Start StreamEngine
    if (mSsrc!=0)
        rtn = InternalStart(true);
    else
        rtn = InternalStart(false);
    if (rtn!=SUCCESS)
    {
        LOGE("Failed OnStart (Video)");
        Stop();
    }
    LOGD("<==Start Video");
    return rtn;
}
int
QStreamEngine::InternalStart(bool useSameSsrc)
{
    LOGD("==>InternalStart Video useSameSsrc:%d",useSameSsrc);
    int ret ;
    int result = FAIL;
    mRateControl.Reset();
    mStatistics.Reset();
    mLastRateControlTime=0;
    mLastRtpSeqNumber = 0;
    pthread_mutex_lock(&mEngineThreadLock);

    if (mEngineThreadHandle==0)
    {
        mIsEngineReady = false;
        mUseLastSsrc = useSameSsrc;
        mIsEngineStop = false;
        // Initialize event semaphore
        ret = sem_init(&mEngineReadyEvent,   // handle to the event semaphore
                           0,     // not shared
                           0);    // initially set to non signaled state
        if(ret)
            LOGE("sem_init mEngineReadyEvent error code=%d\n",ret);

        // Create the threads
        ret = pthread_create(&mEngineThreadHandle, 0, StreamEngineThread, (void*)this);

        if(ret)
            LOGE("pthread_create mEngineThreadHandle thread error code=%d\n",ret);

        pthread_mutex_unlock(&mEngineThreadLock);
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
        LOGE("InternalStart: Thread already on running Video");
        result = SUCCESS;
        pthread_mutex_unlock(&mEngineThreadLock);
    }
    LOGD("<==InternalStart Video");
    return result;
}
int
QStreamEngine::Restart(bool enableTfrc)
{
    Stop();
    mEnableTfrc = enableTfrc;
    int rtn = InternalStart(true);
    if (rtn!=SUCCESS)
    {
        LOGE("Failed On Restart (Video)");
        Stop();
    }
    return rtn;
}
int
QStreamEngine::Stop()
{   
    int ret;
    LOGD("==>Stop Video");
    //Stop Thread
    mpSession->session->on_stopping = 1;
    
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

    mStatistics.Reset();
    LOGD("<==Stop Video");
    return SUCCESS;
}
int
QStreamEngine::SendData(unsigned char * pData, uint32_t length, uint32_t timestamp, struct timeval clock, short rotation, bool toPx1)
{
    if (!pData && length>0)
    {
        LOGE("SendData -> INVALID pData:0x%x length:%d",pData,length);
        return EINVAL;
    }
//    if (timestamp==0)
//    {
//        LOGE("SendData -> INVALID timestamp:%d",timestamp);
//        return FAIL;
//    }
    if (!mpSession)
    {
        LOGE("SendData -> INVALID mpSession");
        return EINVAL;
    }
    if (mIsEngineStop)
    {
        LOGE("SendData -> Thread Stop");
        return FAIL;
    }
    if (!mIsEngineReady)
    {
        LOGE("SendData -> Engine is not ready");
        return FAIL;
    }
    int rtn = FAIL;

    pthread_mutex_lock(&mTimeStampLock);
    mRtpTimeStamp = timestamp;
    mNTPSec = (unsigned long)(clock.tv_sec + 0x83AA7E80);
    mNTPFrac = (unsigned long)((double)clock.tv_usec * (double)(1LL<<32) * 1.0e-6 );
    pthread_mutex_unlock(&mTimeStampLock);

    TL_Send_Para_t send_data;
    memset(&send_data,0,sizeof(send_data));
    send_data.frame_data = pData;
    send_data.length = length;
    send_data.timestamp = timestamp;
    if (mEnableTfrc==true)
    {
        RTP_Eextern_Info_t ext_hdr;
        ext_hdr.frame_len = length;//htonl(length);
        //ext_hdr.checksum = htons(rotation);//htonl(length);
        ext_hdr.head = RTP_EXT_HEAD;
        ext_hdr.rotation = htons(rotation);
        ext_hdr.tail = RTP_EXT_TAIL;
        
        send_data.ext_hdr_data = (uint8_t*)&ext_hdr;
        send_data.ext_len = sizeof(ext_hdr);
    }else
    {
        send_data.ext_hdr_data = NULL;
        send_data.ext_len = 0;
    }
    send_data.to_px1 = toPx1;
    send_data.rtp_event_data = -1;
    pthread_mutex_lock(&mSessionLock);
    if (mpSession && mIsEngineReady)
    {   
        rtn = mpSession->session->send_data(mpSession->session, &send_data);
        //SaveToFile("d:\\rx.h264",false,pData,length);
    }
    pthread_mutex_unlock(&mSessionLock);

    if (rtn<QS_TRUE)
    {
    }else
    {
        ++mExpectedTfrcCount;
        mStatistics.OnSendFrameOK(length);
    }
    //PerformRateControl
    if (mLastRateControlTime==0)
        mLastRateControlTime = mRtpTimeStamp;
//    if ((mRtpTimeStamp-mLastRateControlTime)>990000)
    if ((mRtpTimeStamp-mLastRateControlTime)>0)
    {
        PerformRateControl();
        mLastRateControlTime = mRtpTimeStamp;
    }
    return rtn;
}
void
QStreamEngine::PerformRateControl()
{
    uint32_t curr_frame_cnt = 0;
    uint32_t drop_frame_cnt = 0;
    //BitRate Control
    pthread_mutex_lock(&mSessionLock);
    if (mpSession && mIsEngineReady)
    {
        TransportLayer_GetSendingBufStatus(mpSession->session, &curr_frame_cnt, &drop_frame_cnt);
    }
    pthread_mutex_unlock(&mSessionLock);
    unsigned long kbit_rate = mRateControl.GetKbitRate(curr_frame_cnt,
                                                       drop_frame_cnt);
    kbit_rate = kbit_rate;// AP_CODEC_G711: 64kbps; AP_CODEC_G722: 64kbps; AP_CODEC_G722_1: 32kbps;
    unsigned char fps_rate = mRateControl.GetFrameRate(kbit_rate);
    if (mpStreamVideoCB)
    {
        mpStreamVideoCB->SetEncodeBitRate(kbit_rate);
        //mpStreamVideoCB->SetEncodeFrameRate(fps_rate);
        //TEST("PerformRateControl:kbit_rate=%d,fps_rate=%d",kbit_rate,fps_rate);
        LOGD("PerformRateControl-bitRate:%d frameRate:%d\n", kbit_rate, fps_rate);
    }
//    LOGD("<==PerformRateControl");
}
#define WAIT_BASE 1000
#define qsce_check(rtn,msg,...) if (rtn<0) LOGE(msg,__VA_ARGS__)
void*
QStreamEngine::StreamEngineThread(void* pParam)
{
    LOGD("==>StreamEngineThread Video");
    QStreamEngine* pParent = (QStreamEngine*)pParam;
    //Start Stream Engine
    int rtn = SUCCESS;
    pthread_mutex_lock(&pParent->mSessionLock);
    pParent->mNetId = 0;
    pParent->mpSession = NULL;
    if (SUCCESS == rtn)
    {   //Create Session
        rtn = QSCE_Create(&pParent->mNetId ,pParent->mMediaType);
        qsce_check(rtn,"QSCE_Create failure! %d",rtn);
    }
    if (SUCCESS == rtn)
    {   //Register Callbackup function
        rtn = QSCE_RegisterCallback(pParent->mNetId, &pParent->mNetCallback, (unsigned long)pParent);
        qsce_check(rtn,"QSCE_RegisterCallback failure! %d",rtn);
    }
    if (SUCCESS == rtn)
    {   //Setup Local Receiver
        rtn = QSCE_SetLocalReceiver(pParent->mNetId, pParent->mRtpListPort, "127.0.0.1");
        qsce_check(rtn,"QSCE_SetLocalReceiver failure! %d",rtn);
    }
    if (SUCCESS == rtn)
    {   //Setup TFRC Setting
        rtn = QSCE_SetTFRC(pParent->mNetId, pParent->mEnableTfrc?1:0); /* tfrc enable */
        qsce_check(rtn,"QSCE_SetLocalReceiver failure! %d",rtn);
    }
    if (SUCCESS == rtn)
    {   //Setup Network Payload
        rtn = QSCE_Set_Network_Payload(pParent->mNetId, pParent->mClockRate, pParent->mPayloadType, pParent->mMimeType , -1);
        qsce_check(rtn,"QSCE_Set_Network_Payload failure! %d",rtn);
    }
    if (SUCCESS == rtn)
    {   //Setup Destination
        rtn = QSCE_SetSendDestination(pParent->mNetId, pParent->mRtpDestPort, pParent->mRemoteIP);
        qsce_check(rtn,"QSCE_SetSendDestination failure! %d",rtn);
    }
    if (SUCCESS == rtn){
    	//Setup FEC info
    	rtn = QSCE_Set_FEC(pParent->mNetId, pParent->mFEC_arg.ctrl,pParent->mFEC_arg.send_pt, pParent->mFEC_arg.recv_pt);
    	qsce_check(rtn,"QSCE_Set_FEC failure! %d",rtn);
    }
    if (SUCCESS == rtn)
    {   //Setup TOS
        rtn = QSCE_Set_TOS(pParent->mNetId, pParent->mTOS);
        qsce_check(rtn,"QSCE_Set_TOS failure! %d",rtn);
    }
    if (SUCCESS == rtn)
    { 
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
        pParent->mGotTFRC = false;
        pParent->mpSession->session->on_stopping = 0;
        //LOGD("Audio QSCE_GetSessionSSRC:%d"),pParent->mSsrc);
        pParent->mIsEngineReady = true;
        pParent->mExpectedTfrcCount = 0;
        //Setup video MTU
//        QSCE_Set_Maximum_Payload_Size(pParent->mNetId,1180);
        QSCE_Set_Maximum_Payload_Size(pParent->mNetId,pParent->mMTU);
        //Setup min sending rate
        QSCE_SetTFRCMinSendingRate(pParent->mNetId,512);
    }else
    {
        LOGE("ERROR on Start RTP Layer ");
    }
    pthread_mutex_unlock(&pParent->mSessionLock);
    sem_post(&pParent->mEngineReadyEvent);
    if (SUCCESS == rtn)
    {   //Run Recving Thread
        RTP_Eextern_Info_t      ext_hdr;
        int                     rtn;
        TL_Poll_Para_t          poll_data;
        TL_Recv_Para_t          recv_data;
        long                    ntp_clock_sec;
        long                    ntp_clock_usec;
        uint32_t                this_frame_length = 0;
        uint32_t                rtp_timestamp_ms = 0;
        uint32_t                this_sender_timestamp_ms = 0;
        bool                    this_should_be_drop = false;
        uint32_t                first_sender_timestamp_ms = 0;
        uint16_t                rotation = 0;
        int                     tfrc_retry_count = 50;
        usleep(100000);
        pParent->mRxBuffer = (uint8_t*)malloc(MAX_VIDEO_CODEC_BUFFER_SIZE*sizeof(uint8_t));;
        pParent->mRxBufferSize = MAX_VIDEO_CODEC_BUFFER_SIZE;
        while(!pParent->mIsEngineStop && pParent->mpSession)
        {
            memset(&recv_data,0,sizeof(recv_data));
            //Polling Recv Data Buffer
            memset(&ext_hdr,0,sizeof(ext_hdr));
            memset(&poll_data,0,sizeof(poll_data));
            poll_data.ext_hdr_data = (uint8_t*)&ext_hdr;
            poll_data.ext_len = sizeof(ext_hdr);
            //Video Data

            rtn = pParent->mpSession->session->poll_data(pParent->mpSession->session,&poll_data);
            this_frame_length = 0;
            rtp_timestamp_ms = poll_data.timestamp;
            this_should_be_drop = false;
            //Get Recv Data Buffer
            if (rtn>=0)
            {
                if (poll_data.ext_len == sizeof(ext_hdr))
                {
                    this_frame_length = ext_hdr.frame_len;//ntohl(ext_hdr.frame_len);//poll_data.predict_length
                    //rotation = ntohs(ext_hdr.checksum);//htonl(length);
                    if (ext_hdr.head == RTP_EXT_HEAD &&
                        ext_hdr.tail == RTP_EXT_TAIL)
                    {
                        rotation = ntohs(ext_hdr.rotation);
                    }
                    if (!(rotation==0 || rotation==90 || rotation==180 || rotation==270))
                        rotation = 0;
                }
                if (this_frame_length == 0)
                    this_frame_length = poll_data.predict_length;

				if (poll_data.frame_buffer_size>5)
				{
					this_frame_length = MAX_VIDEO_CODEC_BUFFER_SIZE;
					this_should_be_drop = true;
				}else if (this_frame_length ==0)
                {
					//LOGE("ERROR StreamEngineThread polldata frame_length == 0");
					usleep(WAIT_BASE);
					continue;
                }
                if (this_frame_length > MAX_VIDEO_CODEC_BUFFER_SIZE)
                {   //Error
                    LOGE("ERROR StreamEngineThread this_frame_length>MAX :%d > %d",this_frame_length,MAX_VIDEO_CODEC_BUFFER_SIZE);
                    //Drop the received data
                    rtn = 0;
                    this_frame_length = MAX_VIDEO_CODEC_BUFFER_SIZE;
                    this_should_be_drop = true;
                }
                else if (this_frame_length > pParent->mRxBufferSize)
                {
                    LOGE("StreamEngineThread Re-allocate Rx_buffer poll size:%d > %d",this_frame_length,pParent->mRxBufferSize);
                    //Reallocate Rx_buffer
                    if (pParent->mRxBuffer)
                    {
                        free(pParent->mRxBuffer);
                        pParent->mRxBuffer = NULL;
                        pParent->mRxBufferSize = 0;
                    }
                    pParent->mRxBuffer = (uint8_t*)malloc(this_frame_length*sizeof(uint8_t));
                    //Allocate Success
                    if (pParent->mRxBuffer)
                        pParent->mRxBufferSize = this_frame_length;
                }
                recv_data.length =  pParent->mRxBufferSize;
                recv_data.frame_data = pParent->mRxBuffer;
                memset(pParent->mRxBuffer,0,pParent->mRxBufferSize);
                if (!this_should_be_drop && rtp_timestamp_ms!=0)
                {
                    ntp_clock_sec = 0;
                    ntp_clock_usec = 0;
                    this_sender_timestamp_ms = pParent->mTimeConvertor.RtpTime2NTPClock(rtp_timestamp_ms,
                                                       &ntp_clock_sec,
                                                       &ntp_clock_usec);
                    if (first_sender_timestamp_ms==0)
                        first_sender_timestamp_ms = this_sender_timestamp_ms;
                }
            }//End check poll_Data result

            //Get Data from Recv Buffer
            if (rtn == -1)
            {   //No Data
                //LOGE("QStreamEngine::StreamEngineThread Poll data result: No Data");
                usleep(WAIT_BASE);
                continue;
            }else if (rtn == 0)
            {   //At least a frame in the receive buffer, but not yet complete 
                
                //Check the lip_sync flow
                //if (rtp_timestamp_ms!=0)
                //    this_should_be_drop = pParent->mpStreamVideoCB->IsFrameTooLate(rtp_timestamp_ms);

                //Check how many frames in the buffer?
                //if(poll_data.frame_buffer_size > ((pParent->mStatistics.GetRecvFramerate()/2)+3) || this_should_be_drop) //Frank : 15 is recommended
                if (rtp_timestamp_ms!=0)
                {
                    if (this_should_be_drop || !pParent->mpStreamVideoCB->OnLipSyncCheck(poll_data.frame_buffer_size,ntp_clock_sec,ntp_clock_usec))
                    {    //if there are more than 10 frame in the buffer, drop the data
                        LOGE("StreamEngineThread drop uncomplete Data BS:%d DropFlag:%d",poll_data.frame_buffer_size,this_should_be_drop);

                        rtn = pParent->mpSession->session->recv_data(pParent->mpSession->session, &recv_data);
                        if (rtn<QS_TRUE) LOGE("StreamEngineThread 1 session->recv_data failure! %d",rtn);
                        else if (recv_data.frame_data!=NULL && recv_data.length>0)
                        {
                            //if (!recv_data.partial_flag)
                            //    pParent->mStatistics.OnRecvFrameOK(recv_data.length);
                            if (NULL != pParent->mpStreamVideoCB)
                            {
                                pParent->mpStreamVideoCB->OnRecv(recv_data.frame_data,
                                                                 recv_data.length,
                                                                 this_sender_timestamp_ms,
                                                                 rotation,
                                                                 recv_data.partial_flag!=0,
                                                                 recv_data.nal_type);
                            }
                        }
                    }else
                    {
                        if (this_should_be_drop)
                            LOGE("StreamEngineThread OnLipSyncCheck this_should_be_drop == true");
                    }
                }else
                    LOGE("StreamEngineThread poll data timestamp == 0");
                usleep(WAIT_BASE);
                continue;
            }else if (rtn == 1)
            {   //frame completed
                //Try Recving data
                rtn = pParent->mpSession->session->recv_data(pParent->mpSession->session, &recv_data);
                if (rtn<QS_TRUE) 
                    LOGD("StreamEngineThread 2 session->recv_data failure! %d",rtn);
                else if (recv_data.frame_data!=NULL && recv_data.length>0)
                {   //Get the recv data succussfully
                    pParent->mStatistics.OnRecvFrameOK(recv_data.length);
                    //Send video data to callback object
                    if (NULL != pParent->mpStreamVideoCB)
                    {
                        pParent->mpStreamVideoCB->OnRecv(recv_data.frame_data,
                                                         recv_data.length,
                                                         this_sender_timestamp_ms,
                                                         rotation,
                                                         recv_data.partial_flag!=0,
                                                         recv_data.nal_type);
                    }
                }else
                {
                    LOGE("StreamEngineThread session->recv_data!= predict %d != %d",recv_data.length,this_frame_length);
                }
            }else
            {   //Error occurred
                LOGE("StreamEngineThread session->poll_data failure! %d",rtn);
            }
            usleep(WAIT_BASE);
        }
    }
    pParent->mIsEngineReady = false;
    LOGD("StreamEngineThread : Exit Loop");
    //usleep(100000);
    pthread_mutex_lock(&pParent->mSessionLock);
    if (pParent->mpSession!=NULL)
    {        
        QSTL_CALLBACK_T net_null_callback;
        // un-register callback function 
        memset((char *)&net_null_callback, 0, sizeof(net_null_callback));
        rtn = QSCE_RegisterCallback(pParent->mNetId, &net_null_callback, (unsigned long)pParent);
        qsce_check(rtn,"QSCE_RegisterCallback failure! %d",rtn);

        rtn = QSCE_Stop(pParent->mNetId);
        qsce_check(rtn,"QSCE_Stop failure! %d",rtn);

        rtn = QSCE_Destory(pParent->mNetId);
        qsce_check(rtn,"QSCE_Destory failure! %d",rtn);
    }
    pParent->mNetId = 0;
    pParent->mpSession = NULL;
    pthread_mutex_unlock(&pParent->mSessionLock);

    LOGD("StreamEngineThread : mpSession stop");
    if (pParent->mRxBuffer)
    {
        free(pParent->mRxBuffer);
        pParent->mRxBuffer = NULL;
        pParent->mRxBufferSize = 0;
    }
    LOGD("<==StreamEngineThread Video");
    //pthread_exit Limitations
    //When using STL containers as local objects, the destructor will not get called when pthread_exit() is called, which leads to a memory leak. Do not call pthread_exit() from C++. Instead you must throw or return back to the thread's initial function. There you can do a return instead of pthread_exit().
    //Pthread library has no knowledge of C++ stack unwinding. Calling pthread_exit() for will terminate the thread without doing proper C++ stack unwind. That is, destructors for local objects will not be called. (This is analogous to calling exit() for single threaded program.)
    //This can be fixed by calling destructors explicitly right before calling pthread_exit().
    pthread_exit(0);
    return 0;
}
int 
QStreamEngine::VideoRtcpRrCallback(void *info)
{
    //LOGD("==>VideoRtcpRrCallback");
    if (info)
    {
        TL_Rtcp_RR_t *rr = (TL_Rtcp_RR_t *)info;
        //LOGD("[Video RR] total_lost=%lu, fract_lost=%lu, last_seq=%lu, jitter=%lu, lsr=%lu, dlsr=%lu\n"),
        //            (unsigned long)rr->total_lost,
        //            (unsigned long)rr->fract_lost,
        //            (unsigned long)rr->last_seq,
        //            (unsigned long)rr->jitter,
        //            (unsigned long)rr->lsr,
        //            (unsigned long)rr->dlsr);
    }
    //LOGD("<==VideoRtcpRrCallback");
    return QS_TRUE;
}
int
QStreamEngine::VideoRtcpSrCallback(void *info)
{
    if (info)
    {
        TL_Rtcp_SR_t *sr = (TL_Rtcp_SR_t *)info;
        //if (!mIsEngineStop && mIsEngineReady && mpSession && mpSession->session)
        //mTimeConvertor.SetReferenceTime(sr->rtp_ts,
        //                                sr->ntp_sec - 0x83AA7E80,
        //                                (long)( (double)sr->ntp_frac * 1.0e6 / (double)(1LL<<32)));
        //LOGD("[Video SR] ntp_sec=%lu, ntp_frac=%lu, rtp_ts=%lu, sender_pcount=%lu, sender_bcount=%lu\n"), (unsigned long)sr->ntp_sec,
        //                                                                                               (unsigned long)sr->ntp_frac,
        //                                                                                               (unsigned long)sr->rtp_ts,
        //                                                                                               (unsigned long)sr->sender_pcount,
        //                                                                                               (unsigned long)sr->sender_bcount);
    }
    return QS_TRUE;
}
int
QStreamEngine::VideoTfrcReportCallback(void *info)
{
    mGotTFRC = true;
    //LOGD("==>VideoTfrcReportCallback");
    if (info)
    {
        TL_Rtcp_Tfrc_RX_t *tfrc = (TL_Rtcp_Tfrc_RX_t *)info;
        if (!mIsEngineStop && mIsEngineReady && mpSession && mpSession->session)
        {   //Extract Package Loss
            double package_loss_rate = ((double)tfrc->p)/(double)4294967295.0;
            mRateControl.OnPackageLossUpdate(package_loss_rate);
            //mStatistics.OnPackageLossUpdate(package_loss_rate);
            //Extract RTT
            mRateControl.OnRTTUpdate(tfrc->rtt);
            mStatistics.OnRTTUpdate(tfrc->rtt);
            mRateControl.OnIPIUpdate(tfrc->ipi);
        }
        /*LOGD("[Video TFRC] ts=%7lu, tdelay=%7lu, x_recv=%5lukbps, pk_lost=%2.9f\n"),
                       (unsigned long)tfrc->ts, 
                       (unsigned long)tfrc->tdelay, 
                       (unsigned long)tfrc->x_recv*8/1000, 
                       (double)mPackageLostRate);*/
    }
    //LOGD("<==VideoTfrcReportCallback");
    return QS_TRUE;
}
void
QStreamEngine::VideoRtcpTimestampSetting()
{
    //LOGD("==>VideoRtcpTimestampSetting");
    uint32_t    rtp_time_stamp;
    uint32_t    ntp_sec;
    uint32_t    ntp_usec;
    pthread_mutex_lock(&mTimeStampLock);
    rtp_time_stamp = mRtpTimeStamp;
    ntp_sec = mNTPSec;
    ntp_usec = mNTPFrac;
    pthread_mutex_unlock(&mTimeStampLock);

    pthread_mutex_lock(&mSessionLock);
    if (mpSession && mpSession->session)
    {
        if (mIsEngineStop)
        {
            struct timeval clock;
            memset(&clock,0,sizeof(clock));
            gettimeofday(&clock, 0);
            ntp_sec = clock.tv_sec;
            ntp_usec = clock.tv_usec;
            rtp_time_stamp = clock.tv_sec * mClockRate +
            (uint32_t)((double)clock.tv_usec * (double)mClockRate * 1.0e-6);
        }
        TransportLayer_SetRTCPTimeIfno(mpSession->session, 
                                       rtp_time_stamp,
                                       ntp_sec,
                                       ntp_usec
                                       );
    }
    pthread_mutex_unlock(&mSessionLock);
    //LOGD("<==VideoRtcpTimestampSetting");
}
void
QStreamEngine::VideoPsfbCallback(void *info)
{ 
    TL_PSFB_Para_t *ptr = (TL_PSFB_Para_t *)info;
    if (ptr!=NULL && mIsEngineReady)
    {
        if (mpStreamVideoCB)
        {
            switch( ptr->fmt )
            {
                case 1:
                    mpStreamVideoCB->RequestIDR();
                    //LOGD("Send an IDR (I-frame) because getting RTCP_FB_PLI message.\n");
                    break;
                case 4:
                    mpStreamVideoCB->RequestIDR();
                    //LOGD("Send an IDR (I-frame) because getting RTCP_FB_FIR message.\n");
                    break;
                default:    
                    ;//printf("Do nothing.\n");
            }
        }
    }
}
void
QStreamEngine::VideoRtpReceive(uint16_t seq_num, unsigned long timestamp) // for seq_number checking, ex: packetlost counting
{
    if (mLastRtpSeqNumber!=0 && (mLastRtpSeqNumber+1)!=seq_num)
    {
        mStatistics.OnRecvLost(mLastRtpSeqNumber,(seq_num-mLastRtpSeqNumber));
        /*eason - mark; FEC function may recover the loss packet, besides, partial frame would be checked by upper layer 
        if (mIsEngineReady && mpStreamVideoCB)
        {
            mpStreamVideoCB->OnPacketLost();
        }
        */
    }
    mLastRtpSeqNumber = seq_num;
}
int 
QStreamEngine::VideoRtcpRrCallback(unsigned long mtl_id, void *info)
{
    QStreamEngine* pEngine = GetQStreamEngine(mtl_id);
    if (pEngine!=NULL)
        return pEngine->VideoRtcpRrCallback(info);
    return QS_FAIL;
}
int
QStreamEngine::VideoRtcpSrCallback(unsigned long mtl_id, void *info)
{
    QStreamEngine* pEngine = GetQStreamEngine(mtl_id);
    if (pEngine!=NULL)
        return pEngine->VideoRtcpSrCallback(info);
    return QS_FAIL;
}
int
QStreamEngine::VideoTfrcReportCallback(unsigned long mtl_id, void *info)
{
    QStreamEngine* pEngine = GetQStreamEngine(mtl_id);
    if (pEngine!=NULL)
        return pEngine->VideoTfrcReportCallback(info);
    return QS_FAIL;
}
void
QStreamEngine::VideoRtcpTimestampSetting(unsigned long mtl_id)
{
    QStreamEngine* pEngine = GetQStreamEngine(mtl_id);
    if (pEngine!=NULL)
        pEngine->VideoRtcpTimestampSetting();
}
void
QStreamEngine::VideoPsfbCallback(unsigned long parent_id, void *info)
{
    QStreamEngine* pEngine = GetQStreamEngine(parent_id);
    if (pEngine!=NULL)
        return pEngine->VideoPsfbCallback(info);
}

void
QStreamEngine::VideoRtpReceive(unsigned long parent_id, uint16_t seq_num, unsigned long timestamp) // for seq_number checking, ex: packetlost counting
{
    QStreamEngine* pEngine = GetQStreamEngine(parent_id);
    if (pEngine!=NULL)
        return pEngine->VideoRtpReceive(seq_num,timestamp);
}
QStreamEngine* 
QStreamEngine::GetQStreamEngine(unsigned long mtl_id)
{   
    //LOGD("==>GetQStreamEngine:%d",mtl_id);
    QStreamEngine* result = (QStreamEngine*)mtl_id;//(((MTL_SESSION_t *)mtl_id)->reserved);
    return result;
}

void
QStreamEngine::SetBitRateBound(int maxBitRate,int minBitRate,int normlBitRate, int initialBitRate)
{
    mRateControl.SetBitRateBound(maxBitRate,minBitRate,normlBitRate,initialBitRate);
}

void   
QStreamEngine::SendRtcpFbPli()
{
    if (mpSession && mpSession->session && mpSession->session->send_rfc4585_pli)
        mpSession->session->send_rfc4585_pli(mpSession->session);
}
void   
QStreamEngine::SendRtcpFbFir()
{
    if (mpSession && mpSession->session && mpSession->session->send_rfc5104_fir)
        mpSession->session->send_rfc5104_fir(mpSession->session);
}
