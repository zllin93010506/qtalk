//
//  itri_call.h
//  callengine
//
//  Created by Bill Tocute on 12/4/20.
//  Copyright (c) 2012年 Quanta. All rights reserved.
//

#ifndef IRTI_CALL_H
#define IRTI_CALL_H

#define MUTEX_DEBUG 0

#include "icall.h"
#include "pthread.h"

namespace itricall
{
struct CallData
{
    char                    remote_id[128];
    char                    remote_domain[128];
    char                    remote_audio_ip[128];
    unsigned int            remote_audio_port;
    char                    remote_video_ip[128];
    unsigned int            remote_video_port;
    char                    remote_display_name[128];
    char                    local_id[128];
    char                    local_audio_ip[128];
    unsigned int            local_audio_port;
    char                    local_video_ip[128];
    unsigned int            local_video_port;
    struct call::MDP        audio_mdp;
    struct call::MDP        video_mdp;
    unsigned int            old_remote_audio_port;
    unsigned int            old_remote_video_port;
    unsigned int            old_local_audio_port;
    unsigned int            old_local_video_port;
    
    //IOT
    //bandwidth
    unsigned int local_ct;    //conference total
    unsigned int local_as;    //application specific maximun
    
    unsigned int remote_ct;
    unsigned int remote_as;
    
    unsigned long           start_time;
    enum call::ProductType  product_type;
    enum call::CallType     call_type;
    enum call::CallState    call_state;
    enum call::Mdp_Status   mdp_status;
    bool                    is_send_hold;       //no hold  / send hold      for make hold call 
    bool                    is_recv_hold;       //no hold  / receive hold   for receive hold call 
    bool                    is_video_call;      //audio call    / video call  for UI
    bool                    is_session_start;   //session start / session end for onSession End
    bool                    is_reinvite;        //first invite  / re-invite   for UI
    bool                    is_session_change;  //session does not change / a <-> v for UI
    bool                    is_caller;          //callee    / caller (press down make call) for HungupCall
    bool                    is_video_arch_caller;          // for video archive sync
    
    bool                    enable_telephone_event;//disable/ enable telephone-event (DTMF)
    bool                    is_mrf_server;
    char                    call_id[128];
    
    
    enum call::NetworkInterfaceType   network_interface_type;
    //nua_handle_t            *call_handle;
    pthread_mutex_t         calldata_lock;
};

    class ItriCall :public call::ICall
    {
        friend class ItriCallEngine;
	public:
		ItriCall(void);      //constructor;
		virtual ~ItriCall(void);
		CallData             mCallData;
		std::list<struct call::MDP*> remoteAudioMDP;
		std::list<struct call::MDP*> remoteVideoMDP;

		const char*   getRemoteDisplayName();
        const char*   getRemoteID();
        const char*   getRemoteDomain();
        
        const char*   getRemoteAudioIP();
        unsigned int  getRemoteAudioPort();
        const char*   getRemoteVideoIP();
        unsigned int  getRemoteVideoPort();
        
        const char*   getLocalID();
        const char*   getLocalAudioIP();
        unsigned int  getLocalAudioPort();
        const char*   getLocalVideoIP();
        unsigned int  getLocalVideoPort();
        
        const call::MDP     getFinalAudioMDP();
        const call::MDP     getFinalVideoMDP();
        
        call::ProductType   getProductType();
        unsigned long getStartTime();
        call::CallType      getCallType();
        call::CallState     getCallState();
        bool          isVideoCall();
        const char*   getCallID();
        
        unsigned int  getBandwidthCT();
        unsigned int  getBandwidthAS();
		bool          isReinvite();
        bool          isSendHold();
        bool          isRecvHold();
        bool          isCaller();

		void          requestIDR();
		void          sendDTMFmessage(const char press);
        bool          isSupportTelephoneEvent();
        call::NetworkInterfaceType     getNetworkType();
    };
}

#endif
