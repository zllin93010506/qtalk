#include "sip_call.h"
#include <string.h>

namespace sipcall
{
    
    SipCall::SipCall()
    {
        memset((void*)&mCallData,0,sizeof(mCallData));
        mCallData.is_session_start      = false;
        mCallData.is_video_call         = false;
        mCallData.is_reinvite           = false;
        mCallData.is_session_change     = true;
        mCallData.is_caller             = false;
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

    SipCall::~SipCall()
    {
        empty_mdp_list(&remoteAudioMDP);
        empty_mdp_list(&remoteVideoMDP);
        /* destroys the mutex */
        pthread_mutex_destroy(&mCallData.calldata_lock);
    }
    const char* SipCall::getRemoteID()
    {
        return mCallData.remote_id;
    }
    const char*   SipCall::getRemoteDomain()
    {
        return mCallData.remote_domain;
    }
    const char*   SipCall::getRemoteAudioIP()
    {
        return mCallData.remote_audio_ip;
    }
    unsigned int  SipCall::getRemoteAudioPort()
    {
        return mCallData.remote_audio_port;
    }
    const char*   SipCall::getRemoteVideoIP()
    {
        return mCallData.remote_video_ip;
    }
    unsigned int  SipCall::getRemoteVideoPort()
    {
        return mCallData.remote_video_port;
    }
    
    const char*  SipCall::getRemoteDisplayName()
    {
        return mCallData.remote_display_name;
    }

    const char*   SipCall::getLocalAudioIP()
    {
        return mCallData.local_audio_ip;
    }
    unsigned int  SipCall::getLocalAudioPort()
    {
        return mCallData.local_audio_port;
    }
    const char*   SipCall::getLocalVideoIP()
    {
        return mCallData.local_video_ip;
    }
    unsigned int  SipCall::getLocalVideoPort()
    {
        return mCallData.local_video_port;
    }

    const call::MDP SipCall::getFinalAudioMDP()
    {
        return mCallData.audio_mdp;
    }
    const call::MDP SipCall::getFinalVideoMDP()
    {
        return mCallData.video_mdp;
    }

    call::ProductType SipCall::getProductType()
    {
        return mCallData.product_type;
    }
    unsigned long SipCall::getStartTime()
    {
        return mCallData.start_time;
    }
    call::CallType SipCall::getCallType()
    {
        return mCallData.call_type;
    }
    call::CallState SipCall::getCallState()
    {
        return mCallData.call_state;
    }
    bool SipCall::isVideoCall()
    {
        return mCallData.is_video_call;
    }
    const char* SipCall::getCallID()
    {
        return mCallData.call_id;
    }
    unsigned int SipCall::getBandwidthCT()
    {
        return mCallData.remote_ct;
    }
    unsigned int SipCall::getBandwidthAS()
    {
        return mCallData.remote_as;
    }
    bool SipCall::isSendHold()
    {
        return mCallData.is_send_hold;
    }
    bool SipCall::isRecvHold()
    {
        return mCallData.is_recv_hold;
    }
    
    void SipCall::requestIDR()
    {
         const char *pl = "<media_control><vc_primitive><to_encoder><picture_fast_update/></to_encoder></vc_primitive></media_control>";
         
         nua_info(mCallData.call_handle, 
                  SIPTAG_CONTENT_TYPE_STR("application/media_control+xml"), 
                  SIPTAG_PAYLOAD_STR(pl), 
                  TAG_END());
    }
    void SipCall::sendDTMFmessage(const char press)
    {  
        /*在用sip info傳遞DTMF信號的時候要注意的問題：
        /*Content-Type: application/dtmf-relay 
        Message Body:
        Signal=5\r\n
        Duration=160\r\n*/
        char pl[256];
        snprintf(pl,256*sizeof(char),"Signal=%c\r\nDuration=500\r\n",press);
        nua_info(mCallData.call_handle,
                 SIPTAG_CONTENT_TYPE_STR("application/dtmf-info"),
                 SIPTAG_PAYLOAD_STR(pl),
                 TAG_END());
    }
    bool SipCall::isSupportTelephoneEvent()
    {
        return mCallData.enable_telephone_event;
    }
}