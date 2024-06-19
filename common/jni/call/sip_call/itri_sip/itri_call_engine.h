//
//  itri_call_engine.h
//  callengine
//
//  Created by Bill Tocute on 12/4/20.
//  Copyright (c) 2012年 Quanta. All rights reserved.
//

#ifndef ITRI_CALL_ENGINE_H
#define ITRI_CALL_ENGINE_H
#include "icall_engine.h"
#include "itri_call.h"
#include <callcontrol/callctrl.h>

#include "pthread.h"
//#include "config.h" 
#include <stdio.h> 
#include <semaphore.h>

namespace call{class CallEngineFactory;};

namespace itricall
{

	struct EngineData
	{
		char         outbound_proxy[256]; //server
		char         proxy_server[256];   //domain
		char         realm_server[256];
		char         registrar_server[256];
		unsigned int port;
		char         sip_id[256];
		char         user_name[256];      //auth name
		char         password[256];
		char         display_name[128];
	    
		std::list<struct call::MDP*>   local_audio_mdps;
		std::list<struct call::MDP*>   local_video_mdps;
		std::list<ItriCall*> call_list;
		call::ICallEventListener* event_listner;
		enum call::ProductType  product_type;

	   // su_home_t               home[1];  /* memory home */
	  //  su_root_t               *root;  
	  //  nua_t                   *nua;  
	  //  nua_handle_t            *regist_handle;
		//enum nua_callstate call_state;
		int expire_time;
		unsigned int listen_port;
		char         local_ip[256];
		bool         finished_event;
		bool         enable_prack;
	};

	class ItriCallEngine : public call::ICallEngine
	{
		friend class call::CallEngineFactory;
		ItriCallEngine(const char* localIP,unsigned int listenPort);
	private:
		EngineData				mEngineData;
		ItriCall*				findInternalCall(const call::ICall* call);
		pthread_t*				mpThread;
		int						findQCallID(const call::ICall* call);

	public:
		virtual ~ItriCallEngine(void);
		 call::CallResult login(const char*    outbound_proxy,
						   const char*    proxy_server,
						   const char*    realm_server,
						   const char*    registrar_server,
						   unsigned int   port,
						   const char*    username,
						   const char*    sipID,
						   const char*    password,
						   const char*    display_name,
						   unsigned int   expire_time,
						   const bool     enable_prack,
						   call::ProductType  product_type,
						   call::ICallEventListener* listener,
                           call::NetworkInterfaceType network_type);

		call::CallResult logout();

		call::CallResult setCapability(call::MDP audioMDPs[],int length_audio_mdps,
									   call::MDP videoMDPs[],int length_video_mdps);

		call::CallResult sequentialMakeCall(const call::ICall* call,
											const char* remoteUUID,
											const char* remoteID,
											const char* domain,
											const char* localIP,
											unsigned int audioPort,
											unsigned int videoPort);

		call::CallResult makeCall(const call::ICall* call,
								  const char* remoteID,
								  const char* domain,
								  const char* localIP,
								  unsigned int audioPort,
								  unsigned int videoPort);

		call::CallResult acceptCall(const call::ICall* call,
									const char* localIP,
									unsigned int audioPort,
									unsigned int videoPort);

		call::CallResult hangupCall(const call::ICall* call);

		call::CallResult keepAlive();
	    
        call::CallResult networkInterfaceSwitch(const char* localIP,
                                                unsigned int port,
                                                int interface_type);
        
        call::CallResult SwitchCallToPX(const call::ICall* call,const char* PX_Addr);
        call::CallResult SetQTDND(const int isDND);
        
        call::CallResult startConference(const call::ICall* call,
                                   const char* domain,
                                   const char* localIP,
                                   unsigned int audioPort,
                                   unsigned int videoPort,
                                   const char* participant1,
                                   const char* participant2,
                                   const char* participant3);
        
		int getSocket(int index);

		void requestIDR(const call::ICall* call);
		void sendDTMFmessage(const call::ICall* call,const char press);
		void setProvision(const call::Config* config);
	};

}

#endif
