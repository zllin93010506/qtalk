//
//  itri_call.cpp
//  callengine
//
//  Created by Bill Tocute on 12/4/20.
//  Copyright (c) 2012年 Quanta. All rights reserved.
//

#include <iostream>
#include "itri_call.h"

#ifdef _ANDROID_APP
#include "logging.h"
#define TAG "SR2_CDRC"
#define LOGV(...) __logback_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __logback_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __logback_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __logback_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __logback_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)
#define printf LOGD
#endif


#if defined(__APPLE__) && defined(__MACH__)
#include "sip_log_ios.h"
#define printf CLog_sip

#endif

namespace itricall
{
    ItriCall::ItriCall()
    {
        memset((void*)&mCallData,0,sizeof(mCallData));
        mCallData.is_session_start      = false;
        mCallData.is_video_call         = false;
        mCallData.is_reinvite           = false;
        mCallData.is_session_change     = true;
        mCallData.is_caller             = false;
        mCallData.is_video_arch_caller  = false;
        mCallData.is_send_hold          = false;
        mCallData.is_recv_hold          = false;
        mCallData.enable_telephone_event= false;
        mCallData.is_mrf_server         = false;
        
        mCallData.call_state            = call::CALL_STATE_NA;
        mCallData.product_type          = call::PRODUCT_TYPE_NA;
        mCallData.mdp_status            = call::MDP_STATUS_INIT;
        /* creates a mutex */
        pthread_mutex_init(&mCallData.calldata_lock, NULL);
    }   

    ItriCall::~ItriCall()
    {
        empty_mdp_list(&remoteAudioMDP);
        empty_mdp_list(&remoteVideoMDP);
        /* destroys the mutex */
        pthread_mutex_destroy(&mCallData.calldata_lock);
    }
	
    const char* ItriCall::getRemoteID()
    {
        return mCallData.remote_id;
    }
    const char* ItriCall::getRemoteDomain()
    {
        return mCallData.remote_domain;
    }        
    const char* ItriCall::getRemoteAudioIP()
    {
         return mCallData.remote_audio_ip;
    }
    unsigned int ItriCall::getRemoteAudioPort()
    {
       return mCallData.remote_audio_port;
    }
    const char* ItriCall::getRemoteVideoIP()
    {
         return mCallData.remote_video_ip;
    }
    unsigned int ItriCall::getRemoteVideoPort()
    {
        return mCallData.remote_video_port;
    }
    
	const char* ItriCall::getRemoteDisplayName()
    {
         return mCallData.remote_display_name;
    }
    
    const char* ItriCall::getLocalID()
    {
        return mCallData.local_id;
    }
    
    const char* ItriCall::getLocalAudioIP()
    {
        return mCallData.local_audio_ip;
    }
    unsigned int ItriCall::getLocalAudioPort()
    {
       return mCallData.local_audio_port;
    }
    const char* ItriCall::getLocalVideoIP()
    {
        return mCallData.local_video_ip;
    }
    unsigned int ItriCall::getLocalVideoPort()
    {
        return mCallData.local_video_port;
    }
    
    const call::MDP ItriCall::getFinalAudioMDP()
    {
       return mCallData.audio_mdp;
    }
    const call::MDP ItriCall::getFinalVideoMDP()
    {
       return mCallData.video_mdp;
    }
    
    call::ProductType ItriCall::getProductType()
    {
		return mCallData.product_type;
    }
    unsigned long ItriCall::getStartTime()
    {
       return mCallData.start_time;
    }
    call::CallType ItriCall::getCallType()
    {
        return mCallData.call_type;
    }
    call::CallState ItriCall::getCallState()
    {
       return mCallData.call_state;
    }
    bool ItriCall::isVideoCall()
    {
         return mCallData.is_video_call;
    }
    const char* ItriCall::getCallID()
    {
        return mCallData.call_id;
    }
    
    unsigned int ItriCall::getBandwidthCT()
    {
        return mCallData.remote_ct;
    }
    unsigned int ItriCall::getBandwidthAS()
    {
         return mCallData.remote_as;
    }
    bool ItriCall::isReinvite()
    {
        return mCallData.is_reinvite;
    }
    bool ItriCall::isSendHold()
    {
       return mCallData.is_send_hold;
    }
    bool ItriCall::isRecvHold()
    {
         return mCallData.is_recv_hold;
    }
    bool ItriCall::isCaller()
    {
        return mCallData.is_video_arch_caller;
    }
    bool ItriCall::isSupportTelephoneEvent()
    {
        return mCallData.enable_telephone_event;
    }
	void ItriCall::requestIDR()
    {
        
    }
    void ItriCall::sendDTMFmessage(const char press)
    {  
      
    }
    
    call::NetworkInterfaceType ItriCall::getNetworkType(){
        return mCallData.network_interface_type;
    }
}
