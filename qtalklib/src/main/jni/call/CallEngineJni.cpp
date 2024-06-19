
#include <stdlib.h>
#include "CallEngineJni.h"
#include "call_engine_factory.h"
#include "pthread.h"
#include <semaphore.h>
//#include <testmain.h>
#include "logging.h"
#define TAG "CallEngineJni"
//#define TAG "SR2_CDRC"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

using namespace call;


#ifdef __cplusplus
extern "C" {
#endif
class CEngineCB : public ICallEventListener
{
private:
    enum CBEvent
    {
        CB_EVENT_NA      = 0,
        CB_EVENT_ON_LOGON_STATE   = 1,
        CB_EVENT_ON_CALL_OUTGOING = 2,
        CB_EVENT_ON_CALL_INCOMING = 3,
        CB_EVENT_ON_CALL_HANGUP   = 4,
        CB_EVENT_ON_CALL_ANSWERED = 5,
        CB_EVENT_ON_CALL_CHANGED  = 6,
        CB_EVENT_ON_SESSION_START = 7,
        CB_EVENT_ON_SESSION_END   = 8,
        CB_EVENT_ON_REINVITE_FAILED = 9,
        CB_EVENT_ON_REQUEST_IDR = 10,
        CB_EVENT_ON_SR2_SWITCH = 11,
        CB_EVENT_ON_CALL_MCU_OUTGOING = 12 ,
        CB_EVENT_ON_CALL_MCU_INCOMING = 13 ,
        CB_EVENT_ON_MCUSESSION_START   = 14 ,
        CB_EVENT_ON_MCUSESSION_END   = 15,
        CB_EVENT_ON_CALL_PTT_INCOMING = 16 ,
        CB_EVENT_ON_MAX = 17 ,
    };
    JavaVM              *mpJVM;
    jclass              mJClassCaller;
    jobject             mJIListener;

    jmethodID           mOnLogonState;
    jmethodID           mOnCallOutgoing;
    jmethodID           mOnCallMCUOutgoing;
    jmethodID           mOnCallIncoming;
    jmethodID           mOnCallMCUIncoming;
    jmethodID           mOnCallPTTIncoming;
    jmethodID           mOnCallHangup;
    jmethodID           mOnCallAnswered;
    jmethodID           mOnCallStateChanged;
    jmethodID           mOnSessionStart;
    jmethodID           mOnSessionEnd;
    jmethodID           mOnMCUSessionStart;
    jmethodID           mOnMCUSessionEnd;
    jmethodID           mOnReinviteFailed;
    jmethodID           mOnRequestIDR;
    jmethodID			mOnCallTransferToPX;
    pthread_mutex_t     mCallbackLock;
    
    CBEvent             mCallbackValue;
    const char*         mCbMsg;
    bool                mCbLogonState;
    unsigned int        mCbExpireTime;
    const ICall*        mCbCall;
    CallState           mCbCallState;
    unsigned int        mCbPXSwitchSuccess;
    
    
    //Main Callback thread controls
    bool                mIsRunning;
    pthread_t           mCallbackThreadHandle;
    sem_t               mCBBeginEvent;
    sem_t               mCBFinishEvent;
    const char*         mCbLocalIP;
    
    static void*CallbackThread(void* pParam)
    {
        CEngineCB* parent = (CEngineCB*) pParam;
        LOGD("==>CallbackThread 0");
        JNIEnv *env = NULL;
        unsigned char  *mpData = NULL;
        jobject        mByteBuffer = NULL;
        unsigned long  mpDataSize = 0;
        if (parent!=NULL)
        {
            unsigned char *p_data = NULL;
            unsigned int length = 0;
            unsigned int size = 0;
            unsigned long this_paly_time = 0;
            unsigned int this_timestamp = 0;
            long long    paly_time_gap = 0;
            long long    timestamp_gap = 0;
            if(parent->mCallbackValue >= CB_EVENT_ON_MAX) {
                LOGD("<==CallbackThread 0 mCallbackValue %d invalid", parent->mCallbackValue);
                return ((void *)0);
            }
            if (parent->mpJVM!=NULL)
            {
                parent->mpJVM->AttachCurrentThread((JNIEnv**)&env,NULL);
                if(env == NULL) {
                    LOGD("<==CallbackThread 0 env null");
        	        return ((void *)0);
                }
            }
            //The callback thread process one event with FIFO
            while(parent->mIsRunning)
            {
                //Wait for callback begin event
                sem_wait(&parent->mCBBeginEvent);
                LOGD("==>CallbackThread 1 %d", parent->mCallbackValue);
                switch(parent->mCallbackValue)
                {
                    case CB_EVENT_ON_LOGON_STATE:
                        parent->onLogonStatePrivate(env,parent->mCbLogonState,parent->mCbMsg,parent->mCbExpireTime, parent->mCbLocalIP);
                        LOGD("%s", parent->mCbMsg);
                        break;
                    case CB_EVENT_ON_CALL_OUTGOING:
                        parent->onCallOutgoingPrivate(env,parent->mCbCall);
                        break;
                    case CB_EVENT_ON_CALL_MCU_OUTGOING:
                        parent->onCallMCUOutgoingPrivate(env,parent->mCbCall);
                        break;
                    case CB_EVENT_ON_CALL_INCOMING:
                        parent->onCallIncomingPrivate(env,parent->mCbCall);
                        break;
                    case CB_EVENT_ON_CALL_MCU_INCOMING:
                        parent->onCallMCUIncomingPrivate(env,parent->mCbCall);
                        break;
                    case CB_EVENT_ON_CALL_HANGUP:
                        parent->onCallHangupPrivate(env,parent->mCbCall,parent->mCbMsg);
                        break;
                    case CB_EVENT_ON_CALL_ANSWERED:
                        parent->onCallAnsweredPrivate(env,parent->mCbCall);
                        break;
                    case CB_EVENT_ON_CALL_CHANGED:
                        parent->onCallStateChangedPrivate(env,parent->mCbCall,parent->mCbCallState,parent->mCbMsg);
                        break;
                    case CB_EVENT_ON_SESSION_START:
                        parent->onSessionStartPrivate(env,parent->mCbCall);
                        break;
                    case CB_EVENT_ON_SESSION_END:
                        parent->onSessionEndPrivate(env,parent->mCbCall);
                        break;
                    case CB_EVENT_ON_MCUSESSION_START:
                        parent->onMCUSessionStartPrivate(env,parent->mCbCall);
                        break;
                    case CB_EVENT_ON_MCUSESSION_END:
                        parent->onMCUSessionEndPrivate(env,parent->mCbCall);
                        break;
                    case CB_EVENT_ON_REINVITE_FAILED:
                        parent->onReinviteFailedPrivate(env,parent->mCbCall);
                        break;
                    case CB_EVENT_ON_REQUEST_IDR:
                        parent->onRequestIDRPrivate(env,parent->mCbCall);
                        break;
                    case CB_EVENT_ON_SR2_SWITCH:
						parent->onCallTransferToPXPrivate(env,parent->mCbCall, parent->mCbPXSwitchSuccess);
						break;
                    case CB_EVENT_ON_CALL_PTT_INCOMING:
                        parent->onCallPTTIncomingPrivate(env,parent->mCbCall);
                        break;
                    default:
                        break;
                }
                parent->mCallbackValue = CB_EVENT_NA;
                //Signal Callback finish Event
                sem_post(&parent->mCBFinishEvent);
                LOGD("<==CallbackThread 1");
            }//end while(parent->mIsRunning)
            
            if (parent->mpJVM!=NULL)
            {
                parent->mpJVM->DetachCurrentThread();
            }
        }//end if (parent!=NULL)
        LOGD("<==CallbackThread 0");
        pthread_exit(NULL);
    }
    void onLogonStatePrivate(JNIEnv *env,
                      bool state,
                      const char* msg,
                      unsigned int expireTime,
                      const char* localIP)
    {
        int test_cnt = 0;
        if (env == NULL){
        	LOGD("==>onLogonStatePrivate env null");
        	return;
        }
        if (msg == NULL || localIP == NULL){
        	LOGD("==>onLogonStatePrivate parameter null");
        	return;
        }
        LOGD("==>onLogonStatePrivate");
        jobject msg_object = env->NewStringUTF(msg);
        jobject localIP_object = env->NewStringUTF(localIP);
        if (mOnLogonState!=NULL && mJIListener!=NULL)
        {
            env->CallStaticVoidMethod(mJClassCaller,
                        mOnLogonState,
                        mJIListener,
                        (jboolean)state,
                        msg_object,
                        (jint)expireTime,
                        localIP_object);
        }
        if(msg_object) {
            env->DeleteLocalRef(msg_object);
            msg_object = 0;
        }
        if(localIP_object) {
            env->DeleteLocalRef(localIP_object);
            localIP_object = 0;
        }
        LOGD("<==onLogonStatePrivate");
    }

    void onCallOutgoingPrivate(JNIEnv *env,const ICall* call)
    {
        LOGD("==>onCallOutgoingPrivate");

        if (mOnCallOutgoing!=NULL && call!=NULL && mJIListener!=NULL)
        {
           ICall* private_call = (ICall* ) call;
           int call_state = private_call->getCallState();

           if (private_call->getRemoteID() == NULL || private_call->getRemoteDomain() == NULL){
        	   LOGD("<==onCallOutgoingPrivate parameter NULL");
               return;
           }

           jobject remoteID_object = env->NewStringUTF(private_call->getRemoteID());
           jobject remote_domain_object = env->NewStringUTF(private_call->getRemoteDomain());
           env->CallStaticVoidMethod(mJClassCaller,
                        mOnCallOutgoing,
                        mJIListener,
                        (jint)call,
                        remoteID_object,
                        remote_domain_object,
                        (jboolean)private_call->isVideoCall(),
                        (jint)call_state);
            if(remoteID_object)
                env->DeleteLocalRef(remoteID_object);
            if(remote_domain_object)
                env->DeleteLocalRef(remote_domain_object);
        }
        LOGD("<==onCallOutgoingPrivate");
    }

    void onCallMCUOutgoingPrivate(JNIEnv *env,const ICall* call)
	{
		LOGD("==>onCallMCUOutgoingPrivate");

		if (mOnCallMCUOutgoing!=NULL && call!=NULL && mJIListener!=NULL)
		{
		   ICall* private_call = (ICall* ) call;
		   int call_state = private_call->getCallState();
		   if (private_call->getRemoteID() == NULL || private_call->getRemoteDomain() == NULL){
			   LOGD("<==onCallMCUOutgoingPrivate parameter NULL");
			   return;
		   }

		   jobject remoteID_object = env->NewStringUTF(private_call->getRemoteID());
		   jobject remote_domain_object = env->NewStringUTF(private_call->getRemoteDomain());
		   env->CallStaticVoidMethod(mJClassCaller,
						mOnCallMCUOutgoing,
						mJIListener,
						(jint)call,
						remoteID_object,
						remote_domain_object,
						(jboolean)private_call->isVideoCall(),
						(jint)call_state);
			if(remoteID_object)
				env->DeleteLocalRef(remoteID_object);
			if(remote_domain_object)
				env->DeleteLocalRef(remote_domain_object);
		}
		LOGD("<==onCallMCUOutgoingPrivate");
	}

    void onCallIncomingPrivate(JNIEnv *env,const ICall* call)
    {
       LOGD("==>onCallIncomingPrivate");

       if (mOnCallIncoming!=NULL && call!=NULL && mJIListener!=NULL)
       {
           ICall* private_call = (ICall* ) call;
           int call_state = private_call->getCallState();

           if (private_call->getRemoteID() == NULL || private_call->getRemoteDomain() == NULL
        		   || private_call->getRemoteDisplayName() == NULL)
           {
        	   LOGD("<==onCallIncomingPrivate parameter NULL");
        	   return;
           }

           jobject remoteID_object = env->NewStringUTF(private_call->getRemoteID());
           jobject remote_domain_object = env->NewStringUTF(private_call->getRemoteDomain());
           jobject remote_remote_display_name_object = env->NewStringUTF(private_call->getRemoteDisplayName());
           LOGD("remote display name:%s",private_call->getRemoteDisplayName());
           env->CallStaticVoidMethod(mJClassCaller,
                        mOnCallIncoming,
                        mJIListener,
                        (jint)call,
                        remoteID_object,
                        remote_domain_object,
                        remote_remote_display_name_object,
                        (jboolean)private_call->isVideoCall(),
                        (jint)call_state);
           if(remoteID_object)
               env->DeleteLocalRef(remoteID_object);
           if(remote_domain_object)
               env->DeleteLocalRef(remote_domain_object);
           if(remote_remote_display_name_object)
               env->DeleteLocalRef(remote_remote_display_name_object);
        }
        LOGD("<==onCallIncomingPrivate");
    }

    void onCallMCUIncomingPrivate(JNIEnv *env,const ICall* call)
	{
	   LOGD("==>onCallMCUIncomingPrivate");

	   if (mOnCallMCUIncoming!=NULL && call!=NULL && mJIListener!=NULL)
	   {
		   ICall* private_call = (ICall* ) call;
		   int call_state = private_call->getCallState();

		   if (private_call->getRemoteID() == NULL || private_call->getRemoteDomain() == NULL
		   		   || private_call->getRemoteDisplayName() == NULL)
		   {
			   LOGD("<==onCallMCUIncomingPrivate parameter NULL");
			   return;
		   }

		   jobject remoteID_object = env->NewStringUTF(private_call->getRemoteID());
		   jobject remote_domain_object = env->NewStringUTF(private_call->getRemoteDomain());
		   jobject remote_remote_display_name_object = env->NewStringUTF(private_call->getRemoteDisplayName());
		   LOGD("remote display name:%s",private_call->getRemoteDisplayName());
		   env->CallStaticVoidMethod(mJClassCaller,
						mOnCallMCUIncoming,
						mJIListener,
						(jint)call,
						remoteID_object,
						remote_domain_object,
						remote_remote_display_name_object,
						(jboolean)private_call->isVideoCall(),
						(jint)call_state);
		   if(remoteID_object)
			   env->DeleteLocalRef(remoteID_object);
		   if(remote_domain_object)
			   env->DeleteLocalRef(remote_domain_object);
		   if(remote_remote_display_name_object)
			   env->DeleteLocalRef(remote_remote_display_name_object);
		}
		LOGD("<==onCallMCUIncomingPrivate");
	}

    void onCallPTTIncomingPrivate(JNIEnv *env,const ICall* call)
	{
	   LOGD("==>onCallPTTIncomingPrivate");

	   if (mOnCallPTTIncoming!=NULL && call!=NULL && mJIListener!=NULL)
	   {
		   ICall* private_call = (ICall* ) call;
		   int call_state = private_call->getCallState();
		   if (private_call->getRemoteID() == NULL || private_call->getRemoteDomain() == NULL
		   		   || private_call->getRemoteDisplayName() == NULL)
		   {
			   LOGD("<==onCallMCUIncomingPrivate parameter NULL");
			   return;
		   }

		   jobject remoteID_object = env->NewStringUTF(private_call->getRemoteID());
		   jobject remote_domain_object = env->NewStringUTF(private_call->getRemoteDomain());
		   jobject remote_remote_display_name_object = env->NewStringUTF(private_call->getRemoteDisplayName());
		   LOGD("remote display name:%s",private_call->getRemoteDisplayName());
		   env->CallStaticVoidMethod(mJClassCaller,
						mOnCallPTTIncoming,
						mJIListener,
						(jint)call,
						remoteID_object,
						remote_domain_object,
						remote_remote_display_name_object,
						(jboolean)private_call->isVideoCall(),
						(jint)call_state);
		   if(remoteID_object)
			   env->DeleteLocalRef(remoteID_object);
		   if(remote_domain_object)
			   env->DeleteLocalRef(remote_domain_object);
		   if(remote_remote_display_name_object)
			   env->DeleteLocalRef(remote_remote_display_name_object);
		}
		LOGD("<==onCallPTTIncomingPrivate");
	}

    void onCallAnsweredPrivate(JNIEnv *env,const ICall* call)
    {
        LOGD("==>onCallAnsweredPrivate");
        if (mOnCallAnswered!=NULL && call!=NULL && mJIListener!=NULL)
        {
           ICall* private_call = (ICall* ) call;
           env->CallStaticVoidMethod(mJClassCaller,
                        mOnCallAnswered,
                        mJIListener,
                        (jint)call);
        }
        LOGD("<==onCallAnsweredPrivate");
    }

    void onCallHangupPrivate(JNIEnv *env,
                      const ICall* call,
                      const char* msg)
    {
       LOGD("==>onCallHangupPrivate");
       if (msg == NULL){
    	   LOGD("==>onCallHangupPrivate parameter null");
    	   return;
       }
       jobject msg_object = env->NewStringUTF(msg);
       if (mOnCallHangup!=NULL && call!=NULL && mJIListener!=NULL)
       {
           ICall* private_call = (ICall* ) call;
           env->CallStaticVoidMethod(mJClassCaller,
                        mOnCallHangup,
                        mJIListener,
                        (jint)call,
                        msg_object);
        }
        if(msg_object)
            env->DeleteLocalRef(msg_object);
        LOGD("<==onCallHangupPrivate");
    }

    void onCallStateChangedPrivate(JNIEnv *env,
                                  const ICall* call,
                                  CallState state,
                                  const char* msg)
    {
        //LOGD("==>onCallStateChangedPrivate");
    	if (msg == NULL){
    		LOGD("==>onCallStateChangedPrivate parameter null");
    		return;
    	}

        jobject msg_object = env->NewStringUTF(msg);

        if (mOnCallStateChanged!=NULL && call!=NULL && mJIListener!=NULL)
        {
           ICall* private_call = (ICall* ) call;
           env->CallStaticVoidMethod(mJClassCaller,
                        mOnCallStateChanged,
                        mJIListener,
                        (jint)call,
                        (jint)state,
                        msg_object);
        }
        if(msg_object)
            env->DeleteLocalRef(msg_object);
        //LOGD("<==onCallStateChangedPrivate");
    }

    void onSessionStartPrivate(JNIEnv *env,const ICall* call)
    {

    	if (mOnSessionStart!=NULL && call!=NULL && mJIListener!=NULL)
       {
           ICall* private_call = (ICall* ) call;
           const MDP audioMDP = private_call->getFinalAudioMDP();
           const MDP videoMDP = private_call->getFinalVideoMDP();
           unsigned int networkType = private_call->getNetworkType();

           if ( private_call->getRemoteAudioIP() == NULL 
        		|| private_call->getRemoteVideoIP() == NULL
        		// the following six condictions always false
				// || audioMDP.mime_type == NULL
				// || videoMDP.mime_type == NULL
				// || audioMDP.srtp.self_srtp_key.key_salt == NULL
				// || audioMDP.srtp.opposite_srtp_key.key_salt == NULL
				// || videoMDP.srtp.self_srtp_key.key_salt == NULL
				// || videoMDP.srtp.opposite_srtp_key.key_salt == NULL 
                 )
           {
        	   LOGD("<==onCallMCUIncomingPrivate parameter NULL");
			   return;
           }

           jobject remote_audioIP_object = env->NewStringUTF(private_call->getRemoteAudioIP());
           jobject remote_videoIP_object = env->NewStringUTF(private_call->getRemoteVideoIP());
           jobject localhost_object = env->NewStringUTF("127.0.0.1");
           jobject audioMDP_mine_type_object = env->NewStringUTF(audioMDP.mime_type);
           jobject videoMDP_mine_type_object = env->NewStringUTF(videoMDP.mime_type);
           //RSTP
           jobject audioMDP_srtp_sendkey_object = env->NewStringUTF(audioMDP.srtp.self_srtp_key.key_salt);
           jobject audioMDP_srtp_receivekey_object = env->NewStringUTF(audioMDP.srtp.opposite_srtp_key.key_salt);
           jobject videoMDP_srtp_sendkey_object = env->NewStringUTF(videoMDP.srtp.self_srtp_key.key_salt);
           jobject videoMDP_srtp_receivekey_object = env->NewStringUTF(videoMDP.srtp.opposite_srtp_key.key_salt);

           unsigned int local_width = 0;
           unsigned int local_height = 0;
           unsigned int local_fps = 0;
           unsigned int local_kbps = 0;

           bool local_enable_pli = false;
           bool local_enable_fir = false;
           bool local_enable_tel_evt = false;
           if (private_call->getBandwidthCT()>32)
               local_kbps = private_call->getBandwidthCT()-32; 
           if (private_call->getBandwidthAS()>32)
               local_kbps =private_call->getBandwidthAS()-32;
           iot_get_mdp(videoMDP, &local_width, &local_height, &local_fps, &local_kbps, &local_enable_pli, &local_enable_fir);
           local_enable_tel_evt = private_call->isSupportTelephoneEvent();
           LOGD("==>onSessionStartPrivate:%dx%d  ProductType:%d audioMDP.clock_rate:%d FECCtrl:%d FECSP:%d FECRP:%d",local_width,local_height, private_call->getProductType(), audioMDP.clock_rate, videoMDP.fec_ctrl.ctrl, videoMDP.fec_ctrl.send_payloadType, videoMDP.fec_ctrl.receive_payloadType);
           env->CallStaticVoidMethod(mJClassCaller,
                        mOnSessionStart,
                        mJIListener,
                        (jint)call,
                        remote_audioIP_object,
                        (jint)private_call->getRemoteAudioPort(),
                        remote_videoIP_object,
                        (jint)private_call->getRemoteVideoPort(),
                        localhost_object,
                        (jint)private_call->getLocalAudioPort(),
                        (jint)private_call->getLocalVideoPort(),
                        audioMDP_mine_type_object,
                        (jint)audioMDP.payload_id,
                        (jint)audioMDP.clock_rate,
                        (jint)audioMDP.bitrate,
                        videoMDP_mine_type_object,
                        (jint)videoMDP.payload_id,
                        (jint)videoMDP.clock_rate,
                        (jint)videoMDP.bitrate,
                        (jint)private_call->getProductType(),
                        (jboolean)private_call->isVideoCall(),
                        (jint)local_width,
                        (jint)local_height,
                        (jint)local_fps,
                        (jint)local_kbps,
                        (jlong)private_call->getStartTime(),
                        (jboolean)local_enable_pli,
                        (jboolean)local_enable_fir,
                        (jboolean)local_enable_tel_evt,
                        (jint)audioMDP.fec_ctrl.ctrl,
                        (jint)audioMDP.fec_ctrl.send_payloadType,
                        (jint)audioMDP.fec_ctrl.receive_payloadType,
                        (jboolean)audioMDP.srtp.enable_srtp,
                        audioMDP_srtp_sendkey_object ,
                        audioMDP_srtp_receivekey_object,
                        (jint)videoMDP.fec_ctrl.ctrl,
                        (jint)videoMDP.fec_ctrl.send_payloadType,
                        (jint)videoMDP.fec_ctrl.receive_payloadType,
                        (jboolean)videoMDP.srtp.enable_srtp,
                        videoMDP_srtp_sendkey_object,
                        videoMDP_srtp_receivekey_object);
                        
           LOGD("onSessionStartPrivate - %u",private_call->getStartTime());
           if(remote_audioIP_object)
                env->DeleteLocalRef(remote_audioIP_object);
           if(remote_videoIP_object)
                env->DeleteLocalRef(remote_videoIP_object);
           if(localhost_object)
                env->DeleteLocalRef(localhost_object);
           if(audioMDP_mine_type_object)
                env->DeleteLocalRef(audioMDP_mine_type_object);
           if(videoMDP_mine_type_object)
                env->DeleteLocalRef(videoMDP_mine_type_object);
           if(audioMDP_srtp_sendkey_object)
        	   env->DeleteLocalRef(audioMDP_srtp_sendkey_object);
           if(audioMDP_srtp_receivekey_object)
        	   env->DeleteLocalRef(audioMDP_srtp_receivekey_object);
           if(videoMDP_srtp_sendkey_object)
        	   env->DeleteLocalRef(videoMDP_srtp_sendkey_object);
           if(videoMDP_srtp_receivekey_object)
        	   env->DeleteLocalRef(videoMDP_srtp_receivekey_object);
        }
    }
    void onSessionEndPrivate(JNIEnv *env,const ICall* call)
    {
       if (mOnSessionEnd!=NULL && call!=NULL && mJIListener!=NULL)
       {
           ICall* private_call = (ICall* ) call;
           env->CallStaticVoidMethod(mJClassCaller,
                        mOnSessionEnd,
                        mJIListener,
                        (jint)call);
        }
    }

    void onMCUSessionStartPrivate(JNIEnv *env,const ICall* call)
    {

    	if (mOnMCUSessionStart!=NULL && call!=NULL && mJIListener!=NULL)
    	       {
    	           ICall* private_call = (ICall* ) call;
    	           const MDP audioMDP = private_call->getFinalAudioMDP();
    	           const MDP videoMDP = private_call->getFinalVideoMDP();
    	           unsigned int networkType = private_call->getNetworkType();

    	           if ( private_call->getRemoteAudioIP() == NULL
						 || private_call->getRemoteVideoIP() == NULL
                        //always false
                        // || audioMDP.mime_type == NULL
                        // || videoMDP.mime_type == NULL
						 )
					{
						   LOGD("<==onCallMCUIncomingPrivate parameter NULL");
						   return;
					}

    	           jobject remote_audioIP_object = env->NewStringUTF(private_call->getRemoteAudioIP());
    	           jobject remote_videoIP_object = env->NewStringUTF(private_call->getRemoteVideoIP());
    	           jobject localhost_object = env->NewStringUTF("127.0.0.1");
    	           jobject audioMDP_mine_type_object = env->NewStringUTF(audioMDP.mime_type);
    	           jobject videoMDP_mine_type_object = env->NewStringUTF(videoMDP.mime_type);
    	           unsigned int local_width = 0;
    	           unsigned int local_height = 0;
    	           unsigned int local_fps = 0;
    	           unsigned int local_kbps = 0;

    	           bool local_enable_pli = false;
    	           bool local_enable_fir = false;
    	           bool local_enable_tel_evt = false;
    	           if (private_call->getBandwidthCT()>32)
    	               local_kbps = private_call->getBandwidthCT()-32;
    	           if (private_call->getBandwidthAS()>32)
    	               local_kbps =private_call->getBandwidthAS()-32;
    	           iot_get_mdp(videoMDP, &local_width, &local_height, &local_fps, &local_kbps, &local_enable_pli, &local_enable_fir);
    	           local_enable_tel_evt = private_call->isSupportTelephoneEvent();
    	           LOGD("==>onMCUSessionStartPrivate:%dx%d  ProductType:%d audioMDP.clock_rate:%d FECCtrl:%d FECSP:%d FECRP:%d",local_width,local_height, private_call->getProductType(), audioMDP.clock_rate, videoMDP.fec_ctrl.ctrl, videoMDP.fec_ctrl.send_payloadType, videoMDP.fec_ctrl.receive_payloadType);
    	           env->CallStaticVoidMethod(mJClassCaller,
    	                        mOnMCUSessionStart,
    	                        mJIListener,
    	                        (jint)call,
    	                        remote_audioIP_object,
    	                        (jint)private_call->getRemoteAudioPort(),
    	                        remote_videoIP_object,
    	                        (jint)private_call->getRemoteVideoPort(),
    	                        localhost_object,
    	                        (jint)private_call->getLocalAudioPort(),
    	                        (jint)private_call->getLocalVideoPort(),
    	                        audioMDP_mine_type_object,
    	                        (jint)audioMDP.payload_id,
    	                        (jint)audioMDP.clock_rate,
    	                        (jint)audioMDP.bitrate,
    	                        videoMDP_mine_type_object,
    	                        (jint)videoMDP.payload_id,
    	                        (jint)videoMDP.clock_rate,
    	                        (jint)videoMDP.bitrate,
    	                        (jint)private_call->getProductType(),
    	                        (jboolean)private_call->isVideoCall(),
    	                        (jint)local_width,
    	                        (jint)local_height,
    	                        (jint)local_fps,
    	                        (jint)local_kbps,
    	                        (jlong)private_call->getStartTime(),
    	                        (jboolean)local_enable_pli,
    	                        (jboolean)local_enable_fir,
    	                        (jboolean)local_enable_tel_evt,
    	                        (jint)audioMDP.fec_ctrl.ctrl,
    	                        (jint)audioMDP.fec_ctrl.send_payloadType,
    	                        (jint)audioMDP.fec_ctrl.receive_payloadType,
    	                        (jint)videoMDP.fec_ctrl.ctrl,
    	                        (jint)videoMDP.fec_ctrl.send_payloadType,
    	                        (jint)videoMDP.fec_ctrl.receive_payloadType);

    	           LOGD("onMCUSessionStartPrivate - %u",private_call->getStartTime());
    	           if(remote_audioIP_object) {
    	                env->DeleteLocalRef(remote_audioIP_object);
    	                remote_audioIP_object = 0;
    	               }
    	           if(remote_videoIP_object) {
    	                env->DeleteLocalRef(remote_videoIP_object);
    	                remote_videoIP_object = 0;
    	                }
    	           if(localhost_object) {
    	                env->DeleteLocalRef(localhost_object);
    	                localhost_object = 0;
    	                }
    	           if(audioMDP_mine_type_object) {
    	                env->DeleteLocalRef(audioMDP_mine_type_object);
    	                audioMDP_mine_type_object = 0;
    	                }
    	           if(videoMDP_mine_type_object) {
    	                env->DeleteLocalRef(videoMDP_mine_type_object);
    	                videoMDP_mine_type_object = 0;
    	                }
    	        }
    }
    void onMCUSessionEndPrivate(JNIEnv *env,const ICall* call)
    {
    	if (mOnMCUSessionEnd!=NULL && call!=NULL && mJIListener!=NULL)
	   {
		   ICall* private_call = (ICall* ) call;
		   env->CallStaticVoidMethod(mJClassCaller,
						mOnMCUSessionEnd,
						mJIListener,
						(jint)call);
		}
    }

    void onReinviteFailedPrivate(JNIEnv *env,const ICall* call)
    {
       LOGD("==>onReinviteFailedPrivate");
       if (mOnReinviteFailed!=NULL && call!=NULL && mJIListener!=NULL)
       {
           ICall* private_call = (ICall* ) call;
           env->CallStaticVoidMethod(mJClassCaller,
                        mOnReinviteFailed,
                        mJIListener,
                        (jint)call);
        }
        LOGD("<==onReinviteFailedPrivate");

    }
    
    
    void onRequestIDRPrivate(JNIEnv *env,const ICall* call)
    {
       LOGD("==>onRequestIDRPrivate");
       if (mOnRequestIDR!=NULL && call!=NULL && mJIListener!=NULL)
       {
           ICall* private_call = (ICall* ) call;
           env->CallStaticVoidMethod(mJClassCaller,
                        mOnRequestIDR,
                        mJIListener,
                        (jint)call);
        }
        LOGD("<==onRequestIDRPrivate");

    }
    
    void onCallTransferToPXPrivate(JNIEnv *env, const ICall* call, unsigned int success)
    {
        LOGD("==>onCallTransferToPXPrivate");
        if (mOnCallTransferToPX!=NULL && mJIListener!=NULL)
        {
            env->CallStaticVoidMethod(mJClassCaller,
                        mOnCallTransferToPX,
                        mJIListener,
                        (jint)call,
                        (jint)success);
        }
        LOGD("<==onCallTransferToPXPrivate");
    }
    
    void requireCallbackToken()
    {
        /* Enter the critical section*/
        pthread_mutex_lock(&mCallbackLock);
    }
    void releaseCallbackToken()
    {
        //LOGD("==>releaseCallbackToken");
        if (mCallbackThreadHandle!=0)
        {
            //Signal Callback begin Event
            sem_post(&mCBBeginEvent);
            
            //Wait for Callback finish Event
            sem_wait(&mCBFinishEvent);
        }
        /*Leave the critical section  */
        pthread_mutex_unlock(&mCallbackLock);
        //LOGD("<==releaseCallbackToken");
    }
public :
    CEngineCB(JavaVM             *pJVM,
              jclass     jClassCaller,
              jmethodID  onLogonStateMethod,
              jmethodID  onCallOutgoingMethod,
              jmethodID  onCallMCUOutgoingMethod,
              jmethodID  onCallIncomingMethod,
              jmethodID  onCallMCUIncomingMethod,
              jmethodID  onCallPTTIncomingMethod,
              jmethodID  onCallHangupMethod,
              jmethodID  onCallAnsweredMethod,
              jmethodID  onCallStateChangedMethod,
              jmethodID  onSessionStartMethod,
              jmethodID  onSessionEndMethod,
              jmethodID  onMCUSessionStartMethod,
              jmethodID  onMCUSessionEndMethod,
              jmethodID  onReinviteFailed,
              jmethodID  onRequestIDR,
              jmethodID  onCallTransferToPX)
    {
        LOGD("==>CEngineCB");
        mpJVM = pJVM;
        mJClassCaller = jClassCaller;
        //CallBack methods
        mOnLogonState = onLogonStateMethod;
        mOnCallOutgoing = onCallOutgoingMethod;
        mOnCallMCUOutgoing = onCallMCUOutgoingMethod;
        mOnCallIncoming = onCallIncomingMethod;
        mOnCallMCUIncoming = onCallMCUIncomingMethod;
        mOnCallPTTIncoming = onCallPTTIncomingMethod;
        mOnCallHangup = onCallHangupMethod;
        mOnCallAnswered = onCallAnsweredMethod;
        mOnCallStateChanged = onCallStateChangedMethod;
        mOnSessionStart = onSessionStartMethod;
        mOnSessionEnd = onSessionEndMethod;
        mOnMCUSessionStart = onMCUSessionStartMethod;
        mOnMCUSessionEnd = onMCUSessionEndMethod;
        mOnReinviteFailed = onReinviteFailed;
        mOnRequestIDR = onRequestIDR;
        mOnCallTransferToPX = onCallTransferToPX;
        /* creates a mutex */
        pthread_mutex_init(&mCallbackLock, NULL);

        LOGD("<==CEngineCB");
    }
    ~CEngineCB()
    {
        /* destroys the mutex */
        pthread_mutex_destroy(&mCallbackLock);
        mpJVM = NULL; 
    }
    void Stop()
    {
        LOGD("==>CEngineCB.Stop");
        int ret;
        mIsRunning = false;
        if (mCallbackThreadHandle!=0)
        {
            //Signal Callback begin Event
            sem_post(&mCBBeginEvent);
            // Wait for the thread finished
            ret = pthread_join(mCallbackThreadHandle, NULL);
            if(ret)
                LOGE("Terminate mCallbackThreadHandle thread error code=%d\n",ret);
            mCallbackThreadHandle = 0;
            
            // Destroy begin & finish event semaphore
            sem_destroy(&mCBBeginEvent);
            sem_destroy(&mCBFinishEvent);
        }
        LOGD("<==CEngineCB.Stop");
    }
    void Start()
    {
        LOGD("==>CEngineCB.Start");
        int ret;
        mIsRunning = true;

        // Initialize event semaphore
        sem_init(&mCBBeginEvent,   // handle to the event semaphore
                 0,                 // not shared
                 0);                // initially set to non signaled state
        sem_init(&mCBFinishEvent,   // handle to the event semaphore
                 0,                 // not shared
                 0);                // initially set to non signaled state

        // Create the threads for decoding bitstream
        mCallbackValue = CB_EVENT_NA;
        ret = pthread_create(&mCallbackThreadHandle, NULL, CallbackThread, (void*)this);
        if(ret)
            LOGE("pthread_create mCallbackThreadHandle thread error code=%d\n",ret);

        LOGD("<==CEngineCB.Start");
    }
    void setListener(jobject jIListener)
    {
        mJIListener = jIListener;
    }
    void onLogonState(bool state,
                      const char* msg,
                      unsigned int expireTime,
                      const char* localIP)
    {
        LOGD("==>onLogonState:%s, expire time:%d ,local ip: %s\n",msg,expireTime,localIP);
        requireCallbackToken();
        mCallbackValue = CB_EVENT_ON_LOGON_STATE;
        LOGD("==>onLogonState mCallbackValue=%d, %p", mCallbackValue, &mCallbackValue);
        mCbMsg = msg;
        mCbLogonState = state;
        mCbExpireTime = expireTime;
        mCbLocalIP = localIP;
        releaseCallbackToken();
        LOGD("<==onLogonState");
    }

    void onCallOutgoing(const ICall* call)
    {
        requireCallbackToken();
        mCallbackValue = CB_EVENT_ON_CALL_OUTGOING;
        mCbCall = call;
        releaseCallbackToken();
    }

    void onCallMCUOutgoing(const ICall* call)
	{
		requireCallbackToken();
		mCallbackValue = CB_EVENT_ON_CALL_MCU_OUTGOING;
		mCbCall = call;
		releaseCallbackToken();
	}

    void onCallIncoming(const ICall* call)
    {
        requireCallbackToken();
        mCallbackValue = CB_EVENT_ON_CALL_INCOMING;
        mCbCall = call;
        releaseCallbackToken();
    }

    void onCallMCUIncoming(const ICall* call)
	{
		requireCallbackToken();
		mCallbackValue = CB_EVENT_ON_CALL_MCU_INCOMING;
		mCbCall = call;
		releaseCallbackToken();
	}

    void onCallPTTIncoming(const ICall* call)
	{
		requireCallbackToken();
		mCallbackValue = CB_EVENT_ON_CALL_PTT_INCOMING;
		mCbCall = call;
		releaseCallbackToken();
	}

    void onCallAnswered(const ICall* call)
    {
        requireCallbackToken();
        mCallbackValue = CB_EVENT_ON_CALL_ANSWERED;
        mCbCall = call;
        releaseCallbackToken();
    }

    void onCallHangup(const ICall* call,
                      const char* msg)
    {
        requireCallbackToken();
        mCallbackValue = CB_EVENT_ON_CALL_HANGUP;
        mCbCall = call;
        mCbMsg = msg;
        releaseCallbackToken();
    }

    void onCallStateChanged(const ICall* call,
                                  CallState state,
                                  const char* msg)
    {
        requireCallbackToken();
        mCallbackValue = CB_EVENT_ON_CALL_CHANGED;
        mCbCall = call;
        mCbCallState = state;
        mCbMsg = msg;
        releaseCallbackToken();
    }

    void onSessionStart(const ICall* call)
    {
        requireCallbackToken();
        mCallbackValue = CB_EVENT_ON_SESSION_START;
        mCbCall = call;
        releaseCallbackToken();
    }

    void onSessionEnd(const ICall* call)
    {
        requireCallbackToken();
        mCallbackValue = CB_EVENT_ON_SESSION_END;
        mCbCall = call;
        releaseCallbackToken();
    }
    
    void onMCUSessionStart(const ICall* call)
	{
		requireCallbackToken();
		mCallbackValue = CB_EVENT_ON_MCUSESSION_START;
		mCbCall = call;
		releaseCallbackToken();
	}

	void onMCUSessionEnd(const ICall* call)
	{
		requireCallbackToken();
		mCallbackValue = CB_EVENT_ON_MCUSESSION_END;
		mCbCall = call;
		releaseCallbackToken();
	}

    void onReinviteFailed(const ICall* call)
    {
        requireCallbackToken();
        mCallbackValue = CB_EVENT_ON_REINVITE_FAILED;
        mCbCall = call;
        releaseCallbackToken();
    }
    
    void onRequestIDR(const ICall* call)
    {
        requireCallbackToken();
        mCallbackValue = CB_EVENT_ON_REQUEST_IDR;
        mCbCall = call;
        releaseCallbackToken();
    }

    void onCallTransferToPX(const ICall* call, unsigned int success)
    {
        requireCallbackToken();
        mCallbackValue = CB_EVENT_ON_SR2_SWITCH;
        mCbPXSwitchSuccess = success;
        releaseCallbackToken();
    }
    
};
typedef struct _NativeParams
{
    jclass                jClassCaller;

    jobject               jIListener;

    CEngineCB*            pEngineCB;
    ICallEngine*          pEngine;

}NativeParams;

/*
 * Class:     com_quanta_qtalk_call_engine_NativeCallEngine
 * Method:    _createEngine
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_quanta_qtalk_call_engine_NativeCallEngine__1createEngine
 (JNIEnv * env, jclass jClassCaller,jstring localIP, jint listenPort)
{
    NativeParams* result = (NativeParams*) malloc(sizeof(NativeParams));
    JavaVM     *pJVM;
    CallResult  rtn;
    jmethodID   onLogonState;
    jmethodID   onCallOutgoing;
    jmethodID   onCallMCUOutgoing;
    jmethodID   onCallIncoming;
    jmethodID   onCallMCUIncoming;
    jmethodID   onCallPTTIncoming;
    jmethodID   onCallHangup;
    jmethodID   onCallAnswered;
    jmethodID   onCallStateChanged;
    jmethodID   onSessionStart;
    jmethodID   onSessionEnd;
    jmethodID   onMCUSessionStart;
    jmethodID   onMCUSessionEnd;
    jmethodID   onReinviteFailed;
    jmethodID   onRequestIDR;
    jmethodID	onCallTransferToPX;
    const char* local_local_ip = env->GetStringUTFChars(localIP, NULL);
    LOGD("==>createCallEngine");

    if (!result)
    {
        LOGE("Unable to malloc sizeof(NativeParams)");
        goto ERROR;
    }
    memset(result,0,sizeof(NativeParams));
    // Obtain a global ref to the Java VM
    env->GetJavaVM(&pJVM);
    if ( pJVM == 0)
    {
        LOGE("Unable to GetJavaVM");
        goto ERROR;
    }

    result->jClassCaller = (jclass)env->NewGlobalRef(jClassCaller);
    if (result->jClassCaller==0)
    {
        LOGE("Unable to get jClassCaller");
        goto ERROR;
    }
    
    //Loading onLogonState method
    #define METHOD_NAME "onLogonState"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;ZLjava/lang/String;ILjava/lang/String;)V"
    onLogonState = env->GetStaticMethodID(result->jClassCaller,    //Class
                                       METHOD_NAME,                //Method
                                       METHOD_SIG);                //Signature
    if (onLogonState==0)
    {
        LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
        goto ERROR;
    }
    //Loading onCallOutgoing method
    #undef METHOD_NAME
    #undef METHOD_SIG
    #define METHOD_NAME "onCallOutgoing"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;ILjava/lang/String;Ljava/lang/String;ZI)V"
    onCallOutgoing = env->GetStaticMethodID(result->jClassCaller,    //Class
                                       METHOD_NAME,                //Method
                                       METHOD_SIG);                //Signature
    if (onCallOutgoing==0)
    {
        LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
        goto ERROR;
    }

    //Loading onCallMCUOutgoing method
	#undef METHOD_NAME
	#undef METHOD_SIG
	#define METHOD_NAME "onCallMCUOutgoing"
	#define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;ILjava/lang/String;Ljava/lang/String;ZI)V"
	onCallMCUOutgoing = env->GetStaticMethodID(result->jClassCaller,    //Class
									   METHOD_NAME,                //Method
									   METHOD_SIG);                //Signature
	if (onCallMCUOutgoing==0)
	{
		LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
		goto ERROR;
	}

    //Loading onCallIncoming method
    #undef METHOD_NAME
    #undef METHOD_SIG
    #define METHOD_NAME "onCallIncoming"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;ZI)V"
    onCallIncoming = env->GetStaticMethodID(result->jClassCaller,    //Class
                                       METHOD_NAME,                //Method
                                       METHOD_SIG);                //Signature
    if (onCallIncoming==0)
    {
        LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
        goto ERROR;
    }
    
    //Loading onCallMCUIncoming method
   #undef METHOD_NAME
   #undef METHOD_SIG
   #define METHOD_NAME "onCallMCUIncoming"
   #define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;ZI)V"
   onCallMCUIncoming = env->GetStaticMethodID(result->jClassCaller,    //Class
									  METHOD_NAME,                //Method
									  METHOD_SIG);                //Signature
   if (onCallMCUIncoming==0)
   {
	   LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
	   goto ERROR;
   }

   //Loading onCallPTTIncoming method
   #undef METHOD_NAME
   #undef METHOD_SIG
   #define METHOD_NAME "onCallPTTIncoming"
   #define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;ZI)V"
   onCallPTTIncoming = env->GetStaticMethodID(result->jClassCaller,    //Class
								  METHOD_NAME,                //Method
								  METHOD_SIG);                //Signature
   if (onCallPTTIncoming==0)
   {
    LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
    goto ERROR;
   }

    //Loading onCallHangup method
    #undef METHOD_NAME
    #undef METHOD_SIG
    #define METHOD_NAME "onCallHangup"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;ILjava/lang/String;)V"
    onCallHangup = env->GetStaticMethodID(result->jClassCaller,    //Class
                                       METHOD_NAME,                //Method
                                       METHOD_SIG);                //Signature
    if (onCallHangup==0)
    {
        LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
        goto ERROR;
    }
    
    //Loading onCallAnswered method
    #undef METHOD_NAME
    #undef METHOD_SIG
    #define METHOD_NAME "onCallAnswered"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;I)V"
    onCallAnswered = env->GetStaticMethodID(result->jClassCaller,    //Class
                                       METHOD_NAME,                //Method
                                       METHOD_SIG);                //Signature
    if (onCallAnswered==0)
    {
        LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
        goto ERROR;
    }
    
    //Loading onCallStateChanged method
    #undef METHOD_NAME
    #undef METHOD_SIG
    #define METHOD_NAME "onCallStateChanged"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;IILjava/lang/String;)V"
    onCallStateChanged = env->GetStaticMethodID(result->jClassCaller,    //Class
                                       METHOD_NAME,                //Method
                                       METHOD_SIG);                //Signature
    if (onCallStateChanged==0)
    {
        LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
        goto ERROR;
    }
            
    //Loading onSessionStart method
    #undef METHOD_NAME
    #undef METHOD_SIG
    #define METHOD_NAME "onSessionStart"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;ILjava/lang/String;ILjava/lang/String;ILjava/lang/String;IILjava/lang/String;IIILjava/lang/String;IIIIZIIIIJZZZIIIZLjava/lang/String;Ljava/lang/String;IIIZLjava/lang/String;Ljava/lang/String;)V"	//eason - change method signature
    onSessionStart = env->GetStaticMethodID(result->jClassCaller,    //Class
                                       METHOD_NAME,                //Method
                                       METHOD_SIG);                //Signature
    if (onSessionStart==0)
    {
        LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
        goto ERROR;
    }
    //Loading onSessionEnd method
    #undef METHOD_NAME
    #undef METHOD_SIG
    #define METHOD_NAME "onSessionEnd"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;I)V"
    onSessionEnd = env->GetStaticMethodID(result->jClassCaller,    //Class
                                       METHOD_NAME,                //Method
                                       METHOD_SIG);                //Signature
    if (onSessionEnd==0)
    {
        LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
        goto ERROR;
    }
    
    //Loading onMCUSessionStart method
   #undef METHOD_NAME
   #undef METHOD_SIG
   #define METHOD_NAME "onMCUSessionStart"
   #define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;ILjava/lang/String;ILjava/lang/String;ILjava/lang/String;IILjava/lang/String;IIILjava/lang/String;IIIIZIIIIJZZZIIIIII)V"	//eason - change method signature
   onMCUSessionStart = env->GetStaticMethodID(result->jClassCaller,    //Class
									  METHOD_NAME,                //Method
									  METHOD_SIG);                //Signature
   if (onMCUSessionStart==0)
   {
	   LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
	   goto ERROR;
   }

   //Loading onMCUSessionEnd method
   #undef METHOD_NAME
   #undef METHOD_SIG
   #define METHOD_NAME "onMCUSessionEnd"
   #define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;I)V"
   onMCUSessionEnd = env->GetStaticMethodID(result->jClassCaller,    //Class
									  METHOD_NAME,                //Method
									  METHOD_SIG);                //Signature
   if (onMCUSessionEnd==0)
   {
	   LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
	   goto ERROR;
   }

    //Loading onReinviteFailed method
    #undef METHOD_NAME
    #undef METHOD_SIG
    #define METHOD_NAME "onReinviteFailed"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;I)V"
    onReinviteFailed = env->GetStaticMethodID(result->jClassCaller,    //Class
                                       METHOD_NAME,                //Method
                                       METHOD_SIG);                //Signature
    if (onReinviteFailed==0)
    {
        LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
        goto ERROR;
    }
    
    //Loading onRequestIDR method
    #undef METHOD_NAME
    #undef METHOD_SIG
    #define METHOD_NAME "onRequestIDR"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;I)V"
    onRequestIDR = env->GetStaticMethodID(result->jClassCaller,    //Class
                                       METHOD_NAME,                //Method
                                       METHOD_SIG);                //Signature
    if (onRequestIDR==0)
    {
        LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
        goto ERROR;
    }
    
     //Loading onCallTransferToPX method
    #undef METHOD_NAME
    #undef METHOD_SIG
    #define METHOD_NAME "onCallTransferToPX"
    #define METHOD_SIG  "(Lcom/quanta/qtalk/call/ICallEngineListener;II)V"
    onCallTransferToPX = env->GetStaticMethodID(result->jClassCaller,    //Class
                                       METHOD_NAME,                //Method
                                       METHOD_SIG);                //Signature
    if (onCallTransferToPX==0)
    {
        LOGE("Unable to GetMethodID %s %s",METHOD_NAME,METHOD_SIG);
        goto ERROR;
    }
    #undef METHOD_NAME
    #undef METHOD_SIG
    
    
    result->pEngineCB = new CEngineCB(
            pJVM,
            result->jClassCaller,
            onLogonState,
            onCallOutgoing,
            onCallMCUOutgoing,
            onCallIncoming,
            onCallMCUIncoming,
            onCallPTTIncoming,
            onCallHangup,
            onCallAnswered,
            onCallStateChanged,
            onSessionStart,
            onSessionEnd,
            onMCUSessionStart,
            onMCUSessionEnd,
            onReinviteFailed,
            onRequestIDR,
            onCallTransferToPX);
    result->pEngineCB->Start();
    result->pEngine = CallEngineFactory::createCallEngine(CALL_ENGINE_TYPE_SIP, local_local_ip, listenPort);
    if (result->pEngine == NULL)
    {
        LOGE("Unable to createCallEngine");
            goto ERROR;
    }
    //LOGD("<==createCallEngine:OK");
    if (local_local_ip) env->ReleaseStringUTFChars(localIP,local_local_ip);
    return (jint)result;
ERROR:
    if (result != NULL)
    {
        NativeParams* handle = result;
        if (handle->pEngine!=0)
        {
            //delete handle->pEngine;
            handle->pEngine = 0;
        }
        if (handle->pEngineCB!=0)
        {
            handle->pEngineCB->Stop();
            delete handle->pEngineCB;
            handle->pEngineCB = 0;
        }
        if (handle->jClassCaller!=0)
        {
            env->DeleteGlobalRef(handle->jClassCaller);
            handle->jClassCaller = 0;
        }
        free(handle);
    }
    if (local_local_ip) env->ReleaseStringUTFChars(localIP,local_local_ip);
    //LOGD("<==createCallEngine:ERROR");
    return 0;
}

/*
 * Class:     com_quanta_qtalk_call_engine_NativeCallEngine
 * Method:    _destroyEngine
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_call_engine_NativeCallEngine__1destroyEngine
 (JNIEnv * env, jclass jClassCaller, jint params)
{
    NativeParams* handle = (NativeParams*)params;
    //LOGD("==>destroyEngine");
    if (handle != NULL)
    {
        if (handle->pEngine!=0)
        {
            //delete handle->pEngine;
            handle->pEngine = 0;
        }
        if (handle->pEngineCB!=0)
        {
            handle->pEngineCB->Stop();
            delete handle->pEngineCB;
            handle->pEngineCB = 0;
        }
        if (handle->jClassCaller!=0)
        {
            //env->DeleteWeakGlobalRef(handle->jClassCaller);
            handle->jClassCaller = 0;
        }
        free(handle);
    }
    //LOGD("<==destroyEngine");
}


/*
 * Class:     com_quanta_qtalk_call_engine_NativeCallEngine
 * Method:    _login
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;ILcom/quanta/qtalk/call/ICallEngineListener;Ljava/lang/String;Ljava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_call_engine_NativeCallEngine__1login
  (JNIEnv * env, jclass jClassCaller, jint params, jstring server, jstring domain, jstring realm, jint port, jstring id, jstring name, jstring password, jint expireTime, jobject jIListener, jstring display_name, jstring registrar_server, jboolean enable_prack, jint network_type)
{
    NativeParams* handle = (NativeParams*) params;

    CallResult  rtn;
    const char* local_domain = env->GetStringUTFChars(domain, NULL);
    const char* local_realm = env->GetStringUTFChars(realm, NULL);
    const char* local_name = env->GetStringUTFChars(name, NULL);
    const char* local_id = env->GetStringUTFChars(id, NULL);
    const char* local_password = env->GetStringUTFChars(password, NULL);
    const char* local_server = env->GetStringUTFChars(server, NULL);
    const char* local_registrar_server = env->GetStringUTFChars(registrar_server, NULL);
    const char* local_display_name = env->GetStringUTFChars(display_name, NULL);
    LOGD("==>Login");
	//print_in_static_lib(1);
    if (!handle)
    {
        LOGE("Error on input parameter handler");
        goto ERROR;
    }
    
    if (jIListener!=NULL)
    {
        handle->jIListener = env->NewGlobalRef(jIListener);
        if (handle->jIListener==0)
        {
            LOGE("Unable to get jIListener");
            goto ERROR;
        }
    }
    if (handle->pEngineCB)
        handle->pEngineCB->setListener(handle->jIListener);
    LOGD("display name:%s",local_display_name);    
    call::NetworkInterfaceType type;
    type = NETWORK_NONE;
    switch( network_type )
    {
		default: break;
		case 1:
		type = NETWORK_3G;
		break;
		case 2:
		type = NETWORK_WIFI;
		break;
		case 9:
		type = NETWORK_ETH;
		break;

	}
    if(network_type == 1)
    {
		
	}
    rtn = handle->pEngine->login(local_server,
                                 local_domain,
                                 local_realm,
                                 local_registrar_server,
                                 port,
                                 local_name,
                                 local_id,
                                 local_password,
                                 local_display_name,
                                 expireTime,
                                 enable_prack,
                                 PRODUCT_TYPE_QTA,
                                 handle->pEngineCB,
                                 type);
    if (rtn != CALL_RESULT_OK)
    {
        LOGE("Unable to login");
        goto ERROR;
    }
    if (local_domain) env->ReleaseStringUTFChars(domain,local_domain);
    if (local_realm) env->ReleaseStringUTFChars(realm,local_realm);
    if (local_name) env->ReleaseStringUTFChars(name,local_name);
    if (local_id) env->ReleaseStringUTFChars(id,local_id);
    if (local_password) env->ReleaseStringUTFChars(password,local_password);
    if (local_server) env->ReleaseStringUTFChars(server,local_server);
    if (local_registrar_server) env->ReleaseStringUTFChars(registrar_server,local_registrar_server);
    if (local_display_name) env->ReleaseStringUTFChars(display_name,local_display_name);
    LOGD("<==Login:OK");
    return;
ERROR:
    if (handle != NULL)
    {
        if (handle->pEngine!=0)
        {
            handle->pEngine->logout();
        }
        if (handle->pEngineCB!=0)
        {
            handle->pEngineCB->setListener(NULL);
        }
        if (handle->jIListener!=0)
        {
            env->DeleteGlobalRef(handle->jIListener);
            handle->jIListener = 0;
        }
    }
    if (local_domain) env->ReleaseStringUTFChars(domain,local_domain);
    if (local_realm) env->ReleaseStringUTFChars(realm,local_realm);
    if (local_name) env->ReleaseStringUTFChars(name,local_name);
    if (local_id) env->ReleaseStringUTFChars(id,local_id);
    if (local_password) env->ReleaseStringUTFChars(password,local_password);
    if (local_server) env->ReleaseStringUTFChars(server,local_server);
    if (local_registrar_server) env->ReleaseStringUTFChars(registrar_server,local_registrar_server);
    if (local_display_name) env->ReleaseStringUTFChars(display_name,local_display_name);

    //LOGD("<==Login:ERROR");
    return;
}
/*
 * Class:     com_quanta_qtalk_call_engine_NativeCallEngine
 * Method:    _logout
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_call_engine_NativeCallEngine__1logout
  (JNIEnv * env, jclass jClassCaller, jint params)
{
    NativeParams* handle = (NativeParams*)params;
    //LOGD("==>Logout");
    if (handle != NULL)
    {
        if (handle->pEngine!=0)
        {
            handle->pEngine->logout();
        }
        if (handle->pEngineCB!=0)
        {
            handle->pEngineCB->setListener(NULL);
        }
        if (handle->jIListener!=0)
        {
            env->DeleteGlobalRef(handle->jIListener);
            handle->jIListener = 0;
        }
    }
    //LOGD("<==Logout");
}
/*
 * Class:     com_quanta_qtalk_call_engine_NativeCallEngine
 * Method:    _setCapability
 * Signature: (I[Lcom/quanta/qtalk/call/MDP;[Lcom/quanta/qtalk/call/MDP;)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_call_engine_NativeCallEngine__1setCapability
  (JNIEnv * env, jclass jClassCaller, jint params, jobjectArray audioMDPs, jobjectArray videoMDPs, jint width, jint height, jint fps)
{
    NativeParams* handle = (NativeParams*)params;
    //LOGD("==>setCapability");
    if (handle != NULL)
    {
        int audio_size = env->GetArrayLength(audioMDPs);
        int video_size = env->GetArrayLength(videoMDPs);
        MDP* audio_mdps = NULL;
        MDP* video_mdps = NULL;
        jclass java_mdp_class  = NULL;
        jfieldID mime_type_field = NULL;
        jfieldID payload_id_field = NULL;
        jfieldID clockrate_field = NULL;
        jfieldID bitrate_field = NULL;
        const char* local_str = NULL;
        jstring local_java_string;
        int i=0;
        unsigned int kbps = 512;
        bool enable_pli = true;
        bool enable_fir = true;
        
        if (audio_size>0) audio_mdps = (MDP*)malloc(audio_size*sizeof(MDP));
        if (video_size>0) video_mdps = (MDP*)malloc(video_size*sizeof(MDP));
        // get the class
        java_mdp_class = env->FindClass("com/quanta/qtalk/call/MDP");
        if (java_mdp_class==NULL)
        {
            LOGE("Error on FindClass com/quanta/qtalk/call/MDP");
            goto FREE;
        }
        // get the field id
        mime_type_field = env->GetFieldID(java_mdp_class, "MimeType", "Ljava/lang/String;");
        if (mime_type_field==NULL)
        {
            LOGE("Error on GetFieldID MimeType");
            goto FREE;
        }
        payload_id_field = env->GetFieldID(java_mdp_class, "PayloadID", "I");
        if (payload_id_field==NULL)
        {
            LOGE("Error on GetFieldID PayloadID");
            goto FREE;
        }
        clockrate_field = env->GetFieldID(java_mdp_class, "ClockRate", "I");
        if (clockrate_field==NULL)
        {
            LOGE("Error on GetFieldID ClockRate");
            goto FREE;
        }
        bitrate_field = env->GetFieldID(java_mdp_class, "BitRate", "I");
        if (bitrate_field==NULL)
        {
            LOGE("Error on GetFieldID BitRate");
            goto FREE;
        }
        // get the data from the field
        for (i=0;i<audio_size;i++)
        {
            init_mdp(&audio_mdps[i]);
            jobject local_audio_mdp_element = env->GetObjectArrayElement(audioMDPs,i);
            audio_mdps[i].payload_id = env->GetIntField(local_audio_mdp_element, payload_id_field);
            audio_mdps[i].clock_rate = env->GetIntField(local_audio_mdp_element, clockrate_field);
            audio_mdps[i].bitrate = env->GetIntField(local_audio_mdp_element, bitrate_field);
            local_java_string = (jstring) env->GetObjectField(local_audio_mdp_element,mime_type_field);
            local_str = env->GetStringUTFChars(local_java_string,NULL);
            if (local_str != NULL)
            {
                strncpy(audio_mdps[i].mime_type,local_str, MINE_TYPE_LENGTH*sizeof(char));
                env->ReleaseStringUTFChars(local_java_string,local_str);
            }
            if(local_audio_mdp_element != NULL)
                env->DeleteLocalRef(local_audio_mdp_element);
        }
        for (i=0;i<video_size;i++)
        {
            init_mdp(&video_mdps[i]);
            jobject local_video_mdp_element = env->GetObjectArrayElement(videoMDPs,i);
            video_mdps[i].payload_id = env->GetIntField(local_video_mdp_element, payload_id_field);
            video_mdps[i].clock_rate = env->GetIntField(local_video_mdp_element, clockrate_field);
            video_mdps[i].bitrate = env->GetIntField(local_video_mdp_element, bitrate_field);
            local_java_string = (jstring) env->GetObjectField(local_video_mdp_element,mime_type_field);
            local_str = env->GetStringUTFChars(local_java_string,NULL);
            if (local_str != NULL)
            {
                strncpy(video_mdps[i].mime_type,local_str, MINE_TYPE_LENGTH*sizeof(char));
                env->ReleaseStringUTFChars(local_java_string,local_str);
            }
            if(local_video_mdp_element != NULL)
                env->DeleteLocalRef(local_video_mdp_element);
                
            //refer to local parameter at begining of this method
            if(video_mdps[i].payload_id == 109)
                iot_set_mdp(width, height, fps, kbps, 1 , enable_pli, enable_fir, &video_mdps[i]);
            else if(video_mdps[i].payload_id == 110)
                iot_set_mdp(width, height, fps, kbps, 0 ,enable_pli, enable_fir, &video_mdps[i]);
        }
        if (handle->pEngine!=0)
        {
            handle->pEngine->setCapability(audio_mdps,audio_size,
                                           video_mdps,video_size);
        }
        FREE:
        if (audio_mdps) 
            free(audio_mdps);
        if (video_mdps) 
            free(video_mdps);
    }
    //LOGD("<==setCapability");
}

/*
 * Class:     com_quanta_qtalk_call_engine_NativeCallEngine
 * Method:    _makeCall
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;II)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_call_engine_NativeCallEngine__1makeCall
  (JNIEnv * env, jclass jClassCaller, jint params, jint call, jstring id, jstring domain, jstring localIP, jint audioPort, jint videoPort)
{
    NativeParams* handle = (NativeParams*)params;
    LOGD("==>makeCall- call:%d", call);
    
    if (handle != NULL)
    {
        const char* local_id = env->GetStringUTFChars(id, NULL);
        const char* local_domain = env->GetStringUTFChars(domain, NULL);
        const char* local_localIP = env->GetStringUTFChars(localIP, NULL);
        
        if (handle->pEngine!=0)
        {
            LOGD("local_id:%s, local_domain:%s, local_localIP:%s, audioPort:%d, videoPort:%d",local_id, local_domain, local_localIP, audioPort, videoPort);
            handle->pEngine->makeCall((ICall*)call,
                                  local_id,
                                  local_domain,
                                  local_localIP,
                                  audioPort,
                                  videoPort);
        }
        if (local_id) env->ReleaseStringUTFChars(id,local_id);
        if (local_domain) env->ReleaseStringUTFChars(domain,local_domain);
        if (local_localIP) env->ReleaseStringUTFChars(localIP,local_localIP);
    }
    LOGD("<==makeCall");
}

/*
 * Class:     com_quanta_qtalk_call_engine_NativeCallEngine
 * Method:    _makeCall
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;II)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_call_engine_NativeCallEngine__1sequentialMakeCall
  (JNIEnv * env, jclass jClassCaller, jint params, jint call, jstring ruuid,jstring id, jstring domain, jstring localIP, jint audioPort, jint videoPort)
{
    NativeParams* handle = (NativeParams*)params;
    LOGD("==>makeCall- call:%d", call);

    if (handle != NULL)
    {
    	const char* remote_id = env->GetStringUTFChars(ruuid, NULL);
    	const char* local_id = env->GetStringUTFChars(id, NULL);
        const char* local_domain = env->GetStringUTFChars(domain, NULL);
        const char* local_localIP = env->GetStringUTFChars(localIP, NULL);

        if (handle->pEngine!=0)
        {
            LOGD("remote_id:%s, local_id:%s, local_domain:%s, local_localIP:%s, audioPort:%d, videoPort:%d",remote_id,local_id, local_domain, local_localIP, audioPort, videoPort);
            handle->pEngine->sequentialMakeCall((ICall*)call,
            					  remote_id,
                                  local_id,
                                  local_domain,
                                  local_localIP,
                                  audioPort,
                                  videoPort);
        }
        if (remote_id) env->ReleaseStringUTFChars(ruuid,remote_id);
        if (local_id) env->ReleaseStringUTFChars(id,local_id);
        if (local_domain) env->ReleaseStringUTFChars(domain,local_domain);
        if (local_localIP) env->ReleaseStringUTFChars(localIP,local_localIP);
    }
    LOGD("<==makeCall");
}

/*
 * Class:     com_quanta_qtalk_call_engine_NativeCallEngine
 * Method:    _acceptCall
 * Signature: (ILjava/lang/String;II)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_call_engine_NativeCallEngine__1acceptCall
  (JNIEnv * env, jclass jClassCaller, jint params, jint call, jstring localIP, jint audioPort, jint videoPort)
{
    NativeParams* handle = (NativeParams*)params;
    //LOGD("==>acceptCall- call:%d", call);
    if (handle != NULL)
    {
        const char* local_localIP = env->GetStringUTFChars(localIP, NULL);
        
        if (handle->pEngine!=0)
        {
            handle->pEngine->acceptCall((ICall*)call,
                                        local_localIP,
                                        audioPort,
                                        videoPort);
        }
        if (local_localIP) env->ReleaseStringUTFChars(localIP,local_localIP);
    }
    //LOGD("<==acceptCall");
}

/*
 * Class:     com_quanta_qtalk_call_engine_NativeCallEngine
 * Method:    _hangupCall
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_call_engine_NativeCallEngine__1hangupCall
  (JNIEnv * env, jclass jClassCaller, jint params, jint call)
{
    NativeParams* handle = (NativeParams*)params;
    //LOGD("==>hangupCall- call:%d", call);
    if (handle != NULL)
    {
        if (handle->pEngine!=0)
        {
            handle->pEngine->hangupCall((ICall*)call);
        }
    }
    //LOGD("<==hangupCall");
}

/*
 * Class:     com_quanta_qtalk_call_engine_NativeCallEngine
 * Method:    _keepAlive
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_call_engine_NativeCallEngine__1keepAlive
  (JNIEnv * env, jclass jClassCaller, jint params)
{
    NativeParams* handle = (NativeParams*)params;
    LOGD("==>keepAlive");
    if (handle != NULL)
    {
        if (handle->pEngine!=0)
        {
            handle->pEngine->keepAlive();
        }
    }
    LOGD("<==keepAlive");
}

JNIEXPORT void JNICALL Java_com_quanta_qtalk_call_engine_NativeCallEngine__1requestIDR
  (JNIEnv * env, jclass jClassCaller, jint params, jint call)
{
    NativeParams* handle = (NativeParams*)params;
    LOGD("==>requestIDR");
    if (handle != NULL)
    {
        if (handle->pEngine!=0)
        {
            handle->pEngine->requestIDR((ICall*)call);
        }
    }
    LOGD("<==requestIDR");
}

JNIEXPORT void JNICALL Java_com_quanta_qtalk_call_engine_NativeCallEngine_networkInterfaceSwitch
  (JNIEnv * env, jclass jClassCaller, jint params, jstring localIP, jint sip_port, jint interface_type)
{
    NativeParams* handle = (NativeParams*)params;
    LOGD("==>sr2_switch");
    if (handle != NULL)
    {
        const char* local_localIP = env->GetStringUTFChars(localIP, NULL);
        
        if (handle->pEngine!=0)
        {
			LOGD("local_IP:%s sip_port:%d",local_localIP, sip_port);
			handle->pEngine->networkInterfaceSwitch(local_localIP, sip_port, interface_type);
        }
        if (local_localIP) env->ReleaseStringUTFChars(localIP,local_localIP);
    }
    LOGD("<==sr2_switch");
}

/*
 * Class:     com_quanta_qtalk_call_engine_NativeCallEngine
 * Method:    _SwitchCallToPX
 * Signature: (IILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_call_engine_NativeCallEngine__1SwitchCallToPX
  (JNIEnv *env, jclass jClassCaller, jint params, jint call, jstring px_sip_id)
{
    NativeParams* handle = (NativeParams*)params;
    LOGD("==>SwitchCallToPX- call:%d", call);
    
    if (handle != NULL)
    {
        
        const char* local_px_sip_id = env->GetStringUTFChars(px_sip_id, NULL);
        
        if (handle->pEngine!=0)
        {
            LOGD("local_px_sip_id:%s",local_px_sip_id);
            handle->pEngine->SwitchCallToPX((ICall*)call,
                                  local_px_sip_id);
        }
        if (local_px_sip_id) env->ReleaseStringUTFChars(px_sip_id,local_px_sip_id);
    }
    LOGD("<==SwitchCallToPX");
}


/*
 * Class:     com_quanta_qtalk_call_engine_NativeCallEngine
 * Method:    _SetQTDND
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_call_engine_NativeCallEngine__1SetQTDND
  (JNIEnv *env, jclass jClassCaller, jint params, jint isQTDND)
{
    NativeParams* handle = (NativeParams*)params;
    LOGD("==>SetQTDND isQTDND:%d", isQTDND);
    
    if (handle != NULL)
    {

        
        if (handle->pEngine!=0)
        {    
			handle->pEngine->SetQTDND( isQTDND);
        }
    }
    LOGD("<==SetQTDND");
}

/*
 * Class:     com_quanta_qtalk_call_engine_NativeCallEngine
 * Method:    _setProvision
 * Signature: (ILcom/quanta/qtalk/provision/ProvisionUtility/ProvisionInfo;)V
 */
JNIEXPORT void JNICALL Java_com_quanta_qtalk_call_engine_NativeCallEngine__1setProvision
  (JNIEnv * env, jclass jClassCaller, jint params, jobject j_provision)
{

	NativeParams* handle = (NativeParams*)params;

	Config config ;
	jclass java_provision_class  = NULL;
	//jclass java_provision_class  = jClassCaller;
	jfieldID audio_codec_preference_1_field = NULL;
	jfieldID audio_codec_preference_2_field = NULL;
	jfieldID audio_codec_preference_3_field = NULL;

	const char* local_str = NULL;
	jstring local_java_string;

	memset(&config,0,sizeof(Config));
	//strncpy(config.qtalk.sip.audio_codec_preference_1,"opus", CODEC_PREFERENCE_LENGTH*sizeof(char));
	//strncpy(config.qtalk.sip.audio_codec_preference_2,"g711u", CODEC_PREFERENCE_LENGTH*sizeof(char));
	//strncpy(config.qtalk.sip.audio_codec_preference_3,"g711a", CODEC_PREFERENCE_LENGTH*sizeof(char));

	 // get the class
	java_provision_class = env->FindClass("com/quanta/qtalk/provision/ProvisionUtility$ProvisionInfo");

	if (java_provision_class==NULL)
	{
		LOGE("Error on FindClass 1");
		goto FREE;
	}

	// get the field id
	audio_codec_preference_1_field = env->GetFieldID(java_provision_class, "audio_codec_preference_1", "Ljava/lang/String;");
	if (audio_codec_preference_1_field==NULL)
	{
		LOGE("Error on GetFieldID audio_codec_preference_1_field");
		goto FREE;
	}

	audio_codec_preference_2_field = env->GetFieldID(java_provision_class, "audio_codec_preference_2", "Ljava/lang/String;");
	if (audio_codec_preference_2_field==NULL)
	{
		LOGE("Error on GetFieldID audio_codec_preference_2_field");
		goto FREE;
	}

	audio_codec_preference_3_field = env->GetFieldID(java_provision_class, "audio_codec_preference_3", "Ljava/lang/String;");
	if (audio_codec_preference_3_field==NULL)
	{
		LOGE("Error on GetFieldID audio_codec_preference_3_field");
		goto FREE;
	}

	//set audio_codec_preference_1
	local_java_string = (jstring) env->GetObjectField(j_provision,audio_codec_preference_1_field);
	local_str = env->GetStringUTFChars(local_java_string,NULL);
	if (local_str != NULL)
	{
		LOGE("local_str_len=%d",strlen(local_str));
		if (strlen(local_str) > 0 ) {
			LOGE("set audio_codec_preference_1_field");
			strncpy(config.qtalk.sip.audio_codec_preference_1,local_str, CODEC_PREFERENCE_LENGTH*sizeof(char));
		}
		env->ReleaseStringUTFChars(local_java_string,local_str);
	}

	//set audio_codec_preference_2

	local_java_string = (jstring) env->GetObjectField(j_provision,audio_codec_preference_2_field);
	local_str = env->GetStringUTFChars(local_java_string,NULL);
	if (local_str != NULL)
	{
		LOGE("local_str_len=%d",strlen(local_str));
		if (strlen(local_str) > 0 ) {
			LOGE("set audio_codec_preference_2_field");
			strncpy(config.qtalk.sip.audio_codec_preference_2,local_str, CODEC_PREFERENCE_LENGTH*sizeof(char));
		}
		env->ReleaseStringUTFChars(local_java_string,local_str);
	}

	//set audio_codec_preference_3
	local_java_string = (jstring) env->GetObjectField(j_provision,audio_codec_preference_3_field);
	local_str = env->GetStringUTFChars(local_java_string,NULL);
	if (local_str != NULL)
	{
		LOGE("local_str_len=%d",strlen(local_str));
		if (strlen(local_str) > 0 ) {
			LOGE("set audio_codec_preference_3_field");
			strncpy(config.qtalk.sip.audio_codec_preference_3,local_str, CODEC_PREFERENCE_LENGTH*sizeof(char));
		}
		env->ReleaseStringUTFChars(local_java_string,local_str);
	}
	LOGE("setProvision=>sizeof(sip_config)=%d\n",sizeof(Config));
	if (handle->pEngine!=0)
	{
		handle->pEngine->setProvision(&config);
	}

	FREE:
	return;
}
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv * env = NULL;
     if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) == JNI_OK) {
        logback_init(vm, env);
    }
    LOGE("JNI_OnLoad");

    return JNI_VERSION_1_4;
}


JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved)
{
    JNIEnv * env = NULL;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) == JNI_OK) {
        logback_uninit(env);
    }
    LOGE("JNI_OnUnload");
    return ;
}

#ifdef __cplusplus
}
#endif
