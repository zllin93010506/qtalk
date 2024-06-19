//
//  itri_call_engine.cpp
//  callengine
//
//  Created by Bill Tocute on 12/4/20.
//  Copyright (c) 2012年 Quanta. All rights reserved.
//

#include <iostream>
#include "itri_call.h"
#include "itri_call_engine.h"
#include "itri_icall_converter.h"
#include <string.h>

#include <callcontrol/callctrl.h>
#include <DlgLayer/dlglayer.h>
#include <SessAPI/sessapi.h>
#include <sdp/sdp.h>
#ifdef _ANDROID_APP
#include <MsgSummary/msgsummary.h>
#endif
#include <sipTx/sipTx.h>
#include "itri_sip.h"
#include <unistd.h>

#ifdef _WIN32
#include "DbgUtility.h"
#include<windows.h>
#define printf dxprintf
#endif

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

using namespace call;

#include "callmanager.h"
#include "itri_define.h"
extern QuCall qcall[NUMOFCALL];
extern bool thrd_reg;
bool b_itri_init = false;

extern int g_network_inetrface_type;
extern QTState qt_state;

extern Config sip_config;
extern QuRegister reg;
extern struct call::MDP    local_video_mdp;

extern pthread_mutex_t qu_free_mutex;
extern int g_register_progress;
extern int media_bandwidth;

pthread_mutex_t qu_callengine_mutex=PTHREAD_MUTEX_INITIALIZER;

namespace itricall
{	
	void empty_call_list(std::list<ItriCall*> *list)
    {
        while(!list->empty())
        {
            ItriCall* data = list->back();
            list->pop_back();
            delete data;
        }
        list->clear();
    }

	ItriCallEngine::ItriCallEngine(const char* localIP,unsigned int listenPort)
    {
		//OutputDebugStringA("ItriCallEngine");
        memset(mEngineData.local_ip,        0,sizeof(mEngineData.local_ip));
        memset(mEngineData.outbound_proxy,  0,sizeof(mEngineData.outbound_proxy));
        memset(mEngineData.proxy_server,    0,sizeof(mEngineData.proxy_server));
        memset(mEngineData.realm_server,    0,sizeof(mEngineData.realm_server));
        memset(mEngineData.user_name,       0,sizeof(mEngineData.user_name));
        memset(mEngineData.sip_id,          0,sizeof(mEngineData.sip_id));
        memset(mEngineData.password,        0,sizeof(mEngineData.password));
        if (localIP!=NULL)
            strcpy(mEngineData.local_ip,localIP);
        mEngineData.listen_port = listenPort;
        
        mEngineData.port = 0;
        mEngineData.expire_time = 60;
        mEngineData.product_type = PRODUCT_TYPE_NA;
        //mEngineData.root = NULL;  
        //mEngineData.nua  = NULL;  
        //mEngineData.regist_handle = NULL;
        mpThread = NULL;
        //// Initialize event semaphore
        mEngineData.finished_event = false;  // initially set to non signaled state
        
        memset(&qt_state, 0, sizeof(QTState));
        memset(&sip_config, 0, sizeof(Config));

        strcpy(sip_config.generic.sip.tls_enable,"0");
        strcpy(sip_config.generic.rtp.srtp_enable,"0");

        strcpy(sip_config.qtalk.sip.audio_codec_preference_1,"opus");
        strcpy(sip_config.qtalk.sip.audio_codec_preference_2,"g711u");
        strcpy(sip_config.qtalk.sip.audio_codec_preference_3,"g711a");

        strcpy(sip_config.generic.lines.registration_resend_time,"60");

    }
    
    ItriCallEngine::~ItriCallEngine()
    {
        // Destroy event semaphore
        empty_mdp_list(&mEngineData.local_audio_mdps);
        empty_mdp_list(&mEngineData.local_video_mdps);
        //empty_call_list(&mEngineData.call_list);
    }
    call::CallResult ItriCallEngine::login(const char*    outbound_proxy,//proxy
                                           const char*    proxy_server,  //domain
                                           const char*    realm_server,
                                           const char*    registrar_server,
                                           unsigned int   port,
                                           const char*    username,
                                           const char*    sipID,
                                           const char*    password,
                                           const char*    display_name,
                                           unsigned int   expire_time,
                                           const bool     enable_prack,
                                           call::ProductType    product_type,
                                           call::ICallEventListener* listener,
                                           call::NetworkInterfaceType network_type)
    {
        printf("**ItriCallEngine::login\n");
        
        /* Enter the critical section*/
        pthread_mutex_lock ( &qu_callengine_mutex );
        printf("**ItriCallEngine::start init\n");

        printf("listener=%x\n",listener);
        printf("**itri_init\n");
        
        int res;

        if (!b_itri_init){

        	res = itri_init(mEngineData.local_ip , mEngineData.listen_port);
        	if (res > 0){
        		b_itri_init = true;
        	}else{
        		return call::CALL_RESULT_OK;
        	}
        }

        thrd_reg = false;

        sleep(1);
        
      //Check input parameter
        if (outbound_proxy  == NULL ||
            proxy_server    == NULL ||
            realm_server    == NULL ||
            port            == 0    ||
            username        == NULL ||
            sipID           == NULL ||
            password        == NULL ||
            listener        == NULL){

        	/* Enter the critical section*/
        	pthread_mutex_unlock ( &qu_callengine_mutex );

            return CALL_RESULT_INVALID_INPUT;
        }
        //copy outbound_proxy string
        memset(mEngineData.outbound_proxy,0,sizeof(mEngineData.outbound_proxy));
        strcpy(mEngineData.outbound_proxy,outbound_proxy);
        
        //copy proxy_server string
        memset(mEngineData.proxy_server,0,sizeof(mEngineData.proxy_server));
        strcpy(mEngineData.proxy_server,proxy_server);
        
        //copy realm_server string
        memset(mEngineData.realm_server,0,sizeof(mEngineData.realm_server));
        strcpy(mEngineData.realm_server,realm_server);
        
        //copy registrar_server string
        memset(mEngineData.registrar_server,0,sizeof(mEngineData.registrar_server));
        if(registrar_server != NULL)
            strcpy(mEngineData.registrar_server,registrar_server);
        
        //copy port
        mEngineData.port = port;
        
        //copy user name
        memset(mEngineData.user_name,0,sizeof(mEngineData.user_name));
        strcpy(mEngineData.user_name,username);
        
        //copy login id
        memset(mEngineData.sip_id,0,sizeof(mEngineData.sip_id));
        strcpy(mEngineData.sip_id,sipID);
        
        //copy password
        memset(mEngineData.password,0,sizeof(mEngineData.password));
        strcpy(mEngineData.password,password);

        //copy display_name
        memset(mEngineData.display_name,0,sizeof(mEngineData.display_name));
        if (display_name == NULL || strlen(mEngineData.display_name) == 0) 
            strcpy(mEngineData.display_name,mEngineData.sip_id);
        else
            strcpy(mEngineData.display_name,display_name);
        
        //copy expire time
        mEngineData.expire_time = expire_time>0?expire_time:120;
        
        //copy enable_prack 100rel
        mEngineData.enable_prack = enable_prack;

        mEngineData.product_type = product_type;
        mEngineData.event_listner = listener;

		set_event_listner(listener);
		sip_make_registration(proxy_server, outbound_proxy, realm_server, sipID ,username, password,true);
		
        g_network_inetrface_type=network_type;
        
        /* Enter the critical section*/
        pthread_mutex_unlock ( &qu_callengine_mutex );

        return call::CALL_RESULT_OK;
    }
    
    
    
    call::CallResult ItriCallEngine::logout()
    {
      
      printf("**ItriCallEngine::logout\n");

      /* Enter the critical section*/
      pthread_mutex_lock ( &qu_callengine_mutex );
      printf("**ItriCallEngine::start logout\n");

#if 0

      /*if patient logout , sip account is inactvie. send expire 0 will be fail.
       * g_event_listner will crash
       */

      cleanRegisterRetry();
      set_event_listner(NULL);
      /*
      sip_make_registration(mEngineData.proxy_server, mEngineData.outbound_proxy,
    		  mEngineData.realm_server, mEngineData.sip_id ,mEngineData.user_name, mEngineData.password,false);
   	  */

#endif

#if 1
      if (!b_itri_init){

    	/* Enter the critical section*/
    	pthread_mutex_unlock ( &qu_callengine_mutex );
        return  call::CALL_RESULT_OK;
      }

      printf("**ItriCallEngine::logout b_itri_init = false\n");
      b_itri_init = false;

      int i = 0;

      set_event_listner(NULL);

      cleanCall();
      cleanRegisterRetry();
      cleanAllCallObject();

      /*
      cleanAllCallObject();
      printf("**CallCtrlClean\n");  
      CallCtrlClean();
	  */
#ifdef _WIN32
      Sleep(1000);
#else
      sleep(1);
#endif


#endif
      /* Enter the critical section*/
      pthread_mutex_unlock ( &qu_callengine_mutex );

        return call::CALL_RESULT_OK;
    }
    
    call::CallResult ItriCallEngine::setCapability(call::MDP audioMDPs[],int length_audio_mdps,
                                   call::MDP videoMDPs[],int length_video_mdps)
	{
        printf("**ItriCallEngine::setCapability!!!!!!!!!!!!!!!\n");
        empty_mdp_list(&mEngineData.local_audio_mdps);
        empty_mdp_list(&mEngineData.local_video_mdps);
        for (int i=0;i<length_audio_mdps;i++)
        {
            struct MDP* mdp = (struct MDP*)malloc(sizeof(audioMDPs[i]));
            init_mdp(mdp);
            copy_mdp(mdp, &audioMDPs[i]);
            
            free(mdp);
            //mEngineData.local_audio_mdps.push_back(mdp);
            //memcpy(&mEngineData.local_audio_mdps,&audioMDPs[i],sizeof(audioMDPs[i]));
        }
        
        for (int i=0;i<length_video_mdps;i++)
        {
            struct MDP* mdp = (struct MDP*)malloc(sizeof(videoMDPs[i]));
            init_mdp(mdp);
            copy_mdp(mdp, &videoMDPs[i]);
            
            //mEngineData.local_video_mdps.push_back(mdp);
            memcpy(&local_video_mdp,&videoMDPs[i],sizeof(videoMDPs[i]));
            
            free(mdp);
        }
        
        media_bandwidth = local_video_mdp.tias_kbps;

        return call::CALL_RESULT_OK;
	}
    call::CallResult ItriCallEngine::sequentialMakeCall(const call::ICall* call,
    		 	 	 	 	 	 	 	 	 	  const char* remoteUUID,
                                                  const char* remoteID,
                                                  const char* domain,
                                                  const char* localIP,
                                                  unsigned int audioPort,
                                                  unsigned int videoPort)
    {

    	printf("sequentialMakeCall\n");
    	printf("**remoteUUID=%s,remoteID=%s,domain=%s,localIP=%s,audioPort=%d,videoPort=%d\n",
    			remoteUUID,remoteID,domain,localIP,audioPort,videoPort);

    	int insession_index=0;

		//Check input parameter
		if (remoteID == NULL ||
			domain   == NULL ||
			localIP  == NULL ||
			(audioPort == 0 && videoPort == 0))
			return CALL_RESULT_INVALID_INPUT;

		int register_code;
		unsigned int mode;

		if (videoPort==0)
			mode = 0;
		else
			mode = 1;

		ItriCall* pcall = findInternalCall(call);

		if(pcall == NULL) // first invite
		{
			printf("**pcall=NULL\n");
			//terminate another call

			//Create new call
			pcall = new ItriCall();


			for(insession_index=0;insession_index<NUMOFCALL;insession_index++){
				pthread_mutex_lock ( &qu_free_mutex );
				if (qcall[insession_index].call_obj != NULL){
					printf("check call table, index = %d , state = %d , phone = %s\n",
							insession_index , qcall[insession_index].callstate , qcall[insession_index].phone);
					if (qcall[insession_index].callstate==ACTIVE){
						printf("sequentialMakeCall stop, phone = %s answer the call\n",
								qcall[insession_index].phone);

						if(mEngineData.event_listner   != NULL){
							pcall->mCallData.call_state = (call::CallState)RESULT_INVITE_INSESSION;
							mEngineData.event_listner->onCallOutgoing(pcall);
						}

						delete pcall;
						pthread_mutex_unlock ( &qu_free_mutex );
						return CALL_RESULT_FAIL;
					}else{
						sip_cancel_call(qcall[insession_index].phone);
					}
				}
				pthread_mutex_unlock ( &qu_free_mutex );
			}


			printf("icall_addr=%x\n",pcall);

			//copy remote id to call
			memset(pcall->mCallData.remote_id,0,sizeof(pcall->mCallData.remote_id));
			strcpy(pcall->mCallData.remote_id,remoteID);

			//copy remote domain to call
			memset(pcall->mCallData.remote_domain,0,sizeof(pcall->mCallData.remote_domain));
			strcpy(pcall->mCallData.remote_domain,domain);

			//copy local ip to call
			memset(pcall->mCallData.local_audio_ip,0,sizeof(pcall->mCallData.local_audio_ip));
			memset(pcall->mCallData.local_video_ip,0,sizeof(pcall->mCallData.local_video_ip));
			strcpy(pcall->mCallData.local_audio_ip,mEngineData.local_ip);
			strcpy(pcall->mCallData.local_video_ip,mEngineData.local_ip);

			//copy local infomation to call
			pcall->mCallData.old_remote_audio_port = 0;
			pcall->mCallData.old_remote_video_port = 0;
			pcall->mCallData.old_local_audio_port = 0;
			pcall->mCallData.old_local_video_port = 0;
			pcall->mCallData.local_audio_port = audioPort;
			pcall->mCallData.local_video_port = videoPort;

			pcall->mCallData.remote_audio_port = 0;
			pcall->mCallData.remote_video_port = 0;


			pcall->mCallData.call_type = CALL_TYPE_OUTGOING;
			pcall->mCallData.is_reinvite = false;

			pthread_mutex_lock ( &qu_free_mutex );

			if ((register_code = sip_make_call(pcall,mode,(char*)remoteUUID,(char*)remoteID,audioPort,videoPort)) < 0){
				if(mEngineData.event_listner   != NULL){
					pcall->mCallData.call_state = (call::CallState)register_code;
					mEngineData.event_listner->onCallOutgoing(pcall);
				}
				pthread_mutex_unlock ( &qu_free_mutex );
				return CALL_RESULT_FAIL;
			}

		}

	/* Enter the critical section*/
	#if MUTEX_DEBUG
		printf("pthread_mutex_lock for %s at %s Line:%d %s","pcall->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
	#endif
		pthread_mutex_lock(&pcall->mCallData.calldata_lock);

		pcall->mCallData.is_caller = true;
		pcall->mCallData.is_video_call  = false;
		if(pcall->mCallData.local_video_port != 0)
			pcall->mCallData.is_video_call = true;

		pcall->mCallData.mdp_status = call::MDP_STATUS_INIT;
		/*Leave the critical section  */
	#if MUTEX_DEBUG
		printf("pthread_mutex_unlock for %s at %s Line:%d %s","pcall->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
	#endif
		pthread_mutex_unlock(&pcall->mCallData.calldata_lock);


		if(mEngineData.event_listner   != NULL){
				//printf("*******************************=============%d\n",pcall);
				mEngineData.event_listner->onCallOutgoing(pcall);
		}

		pthread_mutex_unlock ( &qu_free_mutex );
		return call::CALL_RESULT_OK;

    }

    call::CallResult ItriCallEngine::makeCall(const call::ICall* call,
                                              const char* remoteID,
                                              const char* domain,
                                              const char* localIP,
                                              unsigned int audioPort,
                                              unsigned int videoPort)
    {
        
		printf("**ItriCallEngine::makeCall\n");
        printf("**loaclIP=%s,audioPort=%d,videoPort=%d\n",localIP,audioPort,videoPort);
		//Check input parameter
        if (remoteID == NULL ||
            domain   == NULL ||
            localIP  == NULL ||
            (audioPort == 0 && videoPort == 0)){
            return CALL_RESULT_INVALID_INPUT;
        }

        int register_code;
		unsigned int mode;

		if (videoPort==0)
			mode = 0;
		else
			mode = 1;

		ItriCall* pcall = findInternalCall(call);
        if(pcall == NULL) // first invite
        {
			
			printf("**pcall=NULL\n");
            //Create new call
			pcall = new ItriCall();
            
            printf("icall_addr=%x\n",pcall);
            
            //copy remote id to call
            memset(pcall->mCallData.remote_id,0,sizeof(pcall->mCallData.remote_id));
            strcpy(pcall->mCallData.remote_id,remoteID);
            
            //copy remote domain to call
            memset(pcall->mCallData.remote_domain,0,sizeof(pcall->mCallData.remote_domain));
            strcpy(pcall->mCallData.remote_domain,domain);
            
            //copy local ip to call
            memset(pcall->mCallData.local_audio_ip,0,sizeof(pcall->mCallData.local_audio_ip));
            memset(pcall->mCallData.local_video_ip,0,sizeof(pcall->mCallData.local_video_ip));
            strcpy(pcall->mCallData.local_audio_ip,mEngineData.local_ip);
            strcpy(pcall->mCallData.local_video_ip,mEngineData.local_ip);
            
            //copy local infomation to call
            pcall->mCallData.old_remote_audio_port = 0;
            pcall->mCallData.old_remote_video_port = 0;
            pcall->mCallData.old_local_audio_port = 0;
            pcall->mCallData.old_local_video_port = 0;
            pcall->mCallData.local_audio_port = audioPort;
            pcall->mCallData.local_video_port = videoPort;
            
            pcall->mCallData.remote_audio_port = 0;
            pcall->mCallData.remote_video_port = 0;
			

			pcall->mCallData.call_type = CALL_TYPE_OUTGOING;
            pcall->mCallData.is_reinvite = false;
            
            pthread_mutex_lock ( &qu_free_mutex );

			if ((register_code = sip_make_call(pcall,mode,NULL,(char*)remoteID,audioPort,videoPort)) < 0){
				if(mEngineData.event_listner   != NULL){
					pcall->mCallData.call_state = (call::CallState)register_code;
				    mEngineData.event_listner->onCallOutgoing(pcall);
				}

				delete pcall;
				pthread_mutex_unlock ( &qu_free_mutex );
				return CALL_RESULT_FAIL;
			}

		}else  //re-invite or hold
		{
			printf("**pcall!=NULL\n");
			printf("icall_addr=%x\n",call);
            /* Enter the critical section*/
#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","pcall->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
            pthread_mutex_lock(&pcall->mCallData.calldata_lock);
            printf("mCallData.is_reinvite = %d",pcall->mCallData.is_reinvite);
            if(pcall->mCallData.is_reinvite == true)//Already got an re-invite message
            {
                /*Leave the critical section  */

                
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","pcall->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                pthread_mutex_unlock(&pcall->mCallData.calldata_lock);
                printf("reinvite fail\n");
                if(mEngineData.event_listner   != NULL){
                	pcall->mCallData.call_state = CALL_STATE_FAILED;
                    mEngineData.event_listner->onCallOutgoing(pcall);
                }

                return CALL_RESULT_FAIL;
            }else
            {
                //copy local ip to call
                memset(pcall->mCallData.local_audio_ip,0,sizeof(pcall->mCallData.local_audio_ip));
                memset(pcall->mCallData.local_video_ip,0,sizeof(pcall->mCallData.local_video_ip));
                if(strcmp(localIP, "0.0.0.0") == 0) //make hold
                {
                    strcpy(pcall->mCallData.local_audio_ip,localIP);
                    strcpy(pcall->mCallData.local_video_ip,localIP);
                    pcall->mCallData.is_send_hold = true;
                    //pcall->mCallData.pre_hold_state = SEND_STATE_HOLD; //mark by 6F
					int call_id = findQCallID(call);
					printf("sip_hold_call id=%d,phone=%s\n",call_id, qcall[call_id].phone);
					if (sip_hold_call(qcall[call_id].phone)!=RC_OK){
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","pcall->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
						pthread_mutex_unlock(&pcall->mCallData.calldata_lock);

						if(mEngineData.event_listner   != NULL){
							pcall->mCallData.call_state = CALL_STATE_FAILED;
						    mEngineData.event_listner->onCallOutgoing(pcall);
						}
						printf("reinvite fail\n");

                        return CALL_RESULT_FAIL;
                    }
                }
                else 
                {
                    strcpy(pcall->mCallData.local_audio_ip,mEngineData.local_ip);
                    strcpy(pcall->mCallData.local_video_ip,mEngineData.local_ip);
                    
                    //pcall->mCallData.pre_hold_state = SEND_STATE_UNHOLD; //mark by 6F
					
					int call_id = findQCallID(call);
					printf("call id=%d\n",call_id);
					if (pcall->mCallData.is_send_hold){
						printf("sip_resume_call id=%d,phone=%s\n",call_id, qcall[call_id].phone);
						if (sip_resume_call(qcall[call_id].phone)!=RC_OK){
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","pcall->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
							pthread_mutex_unlock(&pcall->mCallData.calldata_lock);

							if(mEngineData.event_listner   != NULL){
								pcall->mCallData.call_state = CALL_STATE_FAILED;
							    mEngineData.event_listner->onCallOutgoing(pcall);
							}
							printf("reinvite fail\n");

                            return CALL_RESULT_FAIL;
                        }
					}else{
						printf("**mode=%d,id=%s\n",mode,qcall[call_id].phone);
						if (mode==0)
							qcall[call_id].audio_port = audioPort;
						else 
							qcall[call_id].video_port = videoPort;

						printf("sip_request_switch mode=%d,id=%d,phone=%s\n",mode,call_id, qcall[call_id].phone);
						if (sip_request_switch(mode,qcall[call_id].phone)!=RC_OK){
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","pcall->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
							pthread_mutex_unlock(&pcall->mCallData.calldata_lock);

							if(mEngineData.event_listner   != NULL){
								pcall->mCallData.call_state = CALL_STATE_FAILED;
							    mEngineData.event_listner->onCallOutgoing(pcall);
							}
							printf("reinvite fail\n");

                            return CALL_RESULT_FAIL;
                        }
					}

					pcall->mCallData.is_send_hold = false; 
                }
               
                //setup the hold state
                if (pcall->mCallData.is_send_hold || pcall->mCallData.is_recv_hold)
                    pcall->mCallData.call_state = CALL_STATE_HOLD;
                else if (pcall->mCallData.call_state == CALL_STATE_HOLD)
                    pcall->mCallData.call_state = CALL_STATE_UNHOLD;
                
                //backup local/remote port
                pcall->mCallData.old_remote_audio_port = pcall->mCallData.remote_audio_port;
                pcall->mCallData.old_remote_video_port = pcall->mCallData.remote_video_port;
                pcall->mCallData.old_local_audio_port  = pcall->mCallData.local_audio_port;
                pcall->mCallData.old_local_video_port  = pcall->mCallData.local_video_port;

                //copy local infomation to call
                pcall->mCallData.local_audio_port = audioPort;
                pcall->mCallData.local_video_port = videoPort;

                pcall->mCallData.is_reinvite = true;

                /*Leave the critical section  */
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","pcall->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                pthread_mutex_unlock(&pcall->mCallData.calldata_lock);
            }
		}

		/* Enter the critical section*/
#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","pcall->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
        pthread_mutex_lock(&pcall->mCallData.calldata_lock);

		pcall->mCallData.is_caller = true;
        pcall->mCallData.is_video_call  = false;
        if(pcall->mCallData.local_video_port != 0)
            pcall->mCallData.is_video_call = true;
		
		pcall->mCallData.mdp_status = call::MDP_STATUS_INIT;
		/*Leave the critical section  */
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","pcall->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
        pthread_mutex_unlock(&pcall->mCallData.calldata_lock);
		
		
		if(mEngineData.event_listner   != NULL){
				//printf("*******************************1=============%d\n",pcall);
				printf("g_event_listner=%x\n",mEngineData.event_listner);
				mEngineData.event_listner->onCallOutgoing(pcall);
		}

		pthread_mutex_unlock ( &qu_free_mutex );
		return call::CALL_RESULT_OK;
    }
    call::CallResult ItriCallEngine::acceptCall(const call::ICall* call,
                                                const char* localIP,
                                                unsigned int audioPort,
                                                unsigned int videoPort)
    {
        
		printf("**ItriCallEngine::acceptCall\n");
        printf("**loaclIP=%s,audioPort=%d,videoPort=%d\n",localIP,audioPort,videoPort);
        
		//Check input parameter
        if (call == NULL   ||
            localIP == NULL ||
            (audioPort == 0 && videoPort ==0) )
            return CALL_RESULT_INVALID_INPUT;
        
        printf("icall_addr=%x\n",call);
        
        /* Enter the critical section*/
        pthread_mutex_lock ( &qu_free_mutex );

        //Check call object
        ItriCall* internal_call = findInternalCall(call);
        if (internal_call==NULL){
        	pthread_mutex_unlock ( &qu_free_mutex );
            return CALL_RESULT_INVALID_INPUT;
        }

#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","internal_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
        pthread_mutex_lock(&internal_call->mCallData.calldata_lock);
        
        //copy local ip to call
        memset(internal_call->mCallData.local_audio_ip,0,sizeof(internal_call->mCallData.local_audio_ip));
        memset(internal_call->mCallData.local_video_ip,0,sizeof(internal_call->mCallData.local_video_ip));
        if(strcmp(localIP, "0.0.0.0") == 0) //answer hold
        {
            strcpy(internal_call->mCallData.local_audio_ip,localIP);
            strcpy(internal_call->mCallData.local_video_ip,localIP);
        }
        else 
        {
            strcpy(internal_call->mCallData.local_audio_ip,mEngineData.local_ip);
            strcpy(internal_call->mCallData.local_video_ip,mEngineData.local_ip);
        }
        //copy local infomation to call
        internal_call->mCallData.local_audio_port = audioPort;
        internal_call->mCallData.local_video_port = videoPort;

        //Send acceptCall message
        if(internal_call->mCallData.call_type == CALL_TYPE_INCOMING || internal_call->mCallData.is_reinvite == true)
        {
            char sdp_string[SDP_STR_LENGTH];
            char audio_mime[16];
            char video_mime[16];
            memset(audio_mime, 0, sizeof(audio_mime));
            memset(video_mime, 0, sizeof(video_mime));
            strcpy(audio_mime, internal_call->mCallData.audio_mdp.mime_type);
            strcpy(video_mime, internal_call->mCallData.video_mdp.mime_type);  

            //check if A->V or V->A
            internal_call->mCallData.is_session_change = false;
            if(internal_call->mCallData.is_reinvite)
            {
                /*
				if( strcasecmp( audio_mime ,internal_call->mCallData.audio_mdp.mime_type) ||
                   strcasecmp( video_mime ,internal_call->mCallData.video_mdp.mime_type))
                {
                    internal_call->mCallData.is_session_change = true;
                }
				*/
				internal_call->mCallData.is_session_change = true;

            }
            
            //printf("\"answerSDP\"\n%s\n",sdp_string);
            internal_call->mCallData.mdp_status = MDP_STATUS_FINAL;
            
			int call_id = findQCallID(call);
			int mode = 0 ;

			if (videoPort!=0)
				mode = 1;
			else 
				mode = 0;

			printf("**audio port=%d,video port=%d\n",audioPort,videoPort);
			
			if(internal_call->mCallData.is_reinvite){
				internal_call->mCallData.is_reinvite = false;
				qcall[call_id].audio_port = audioPort;
				qcall[call_id].video_port = videoPort;
				
				if (sip_confirm_switch(1,mode,qcall[call_id].phone)!=RC_OK){
			        /*Leave the critical section  */
#if MUTEX_DEBUG
					printf("pthread_mutex_unlock for %s at %s Line:%d %s","internal_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
			        pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
					pthread_mutex_unlock ( &qu_free_mutex );
					return CALL_RESULT_FAIL;
				}
				
				qcall[call_id].qt_call->mCallData.is_session_change = true;

				mEngineData.event_listner->onSessionEnd(internal_call);
				mEngineData.event_listner->onSessionStart(internal_call);
			}else{

				if (sip_answer_call(internal_call, mode , qcall[call_id].phone)!=RC_OK){
			        /*Leave the critical section  */
#if MUTEX_DEBUG
			    printf("pthread_mutex_unlock for %s at %s Line:%d %s","internal_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
			        pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
					pthread_mutex_unlock ( &qu_free_mutex );
					return CALL_RESULT_FAIL;
				}
				if ( (qcall[call_id].rsdp != NULL) && mEngineData.event_listner   != NULL){
					internal_call->mCallData.is_session_start=true;
					mEngineData.event_listner->onSessionStart(internal_call);
				}
			}
			 //internal_call->mCallData.is_reinvite = false;
			

        }

		internal_call->mCallData.call_state = CALL_STATE_ACCEPT;

        /*Leave the critical section  */
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","internal_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
        pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
        
        pthread_mutex_unlock ( &qu_free_mutex );
        return CALL_RESULT_OK;
    }
    call::CallResult ItriCallEngine::hangupCall(const call::ICall* call)
    {
       
		printf("**ItriCallEngine::hangupCal\n");

        
		
		//Check input parameter
        if (call == NULL)
            return CALL_RESULT_INVALID_INPUT;
        
        printf("icall_addr=%x\n",call);
        
        /* Enter the critical section*/
        pthread_mutex_lock ( &qu_free_mutex );

        //Check call object
        ItriCall* internal_call = findInternalCall(call);
        if (internal_call==NULL){
        	pthread_mutex_unlock ( &qu_free_mutex );
        	return CALL_RESULT_INVALID_INPUT;
        }

#if MUTEX_DEBUG
    printf("pthread_mutex_lock for %s at %s Line:%d %s","internal_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
        pthread_mutex_lock(&internal_call->mCallData.calldata_lock);
  
		printf("**call_state=%d,is_reinvite=%d\n",internal_call->mCallData.call_state,internal_call->mCallData.is_reinvite);

		int call_id = findQCallID(call);

		printf("qcall id=%d\n",call_id);

        //Send hangupCall message
        if((internal_call->mCallData.call_state  == CALL_STATE_ACCEPT ||
            internal_call->mCallData.call_state  == CALL_STATE_UNHOLD ||
            internal_call->mCallData.call_state  == CALL_STATE_HOLD ) &&
            internal_call->mCallData.is_reinvite == false)
        {   
            if (sip_terminate_call_by_id(call_id)!=RC_OK){
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","internal_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
           	   	internal_call = findInternalCall(call);
           	   	if (internal_call!=NULL){
           	   		pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
           	   	}
    			pthread_mutex_unlock ( &qu_free_mutex );
            	return CALL_RESULT_FAIL;
            }
        }
        else  // CALL_STATE_RINGING
        {
            if(internal_call->mCallData.is_caller == false)  //someone call me
            {
                if(internal_call->mCallData.is_reinvite == false)
                {
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","internal_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
                	pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
                	pthread_mutex_unlock ( &qu_free_mutex );
                   if (sip_reject_call_by_id(call_id)!=RC_OK)//486
					   return CALL_RESULT_FAIL;
                }
                else 
                {
					//mode = 0 still video
					//mode = 1 still audio
					internal_call->mCallData.is_reinvite = false;
					int mode ; 
					if (qcall[call_id].media_type == VIDEO)
						mode = 0;
					else
						mode = 1;

					if (sip_confirm_switch(0,mode,qcall[call_id].phone)!=RC_OK){
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","internal_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
    					internal_call = findInternalCall(call);
    					if (internal_call!=NULL){
    						pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
    					}
    					pthread_mutex_unlock ( &qu_free_mutex );
						return CALL_RESULT_FAIL;
					}

					mEngineData.event_listner->onReinviteFailed(qcall[call_id].qt_call);
                }
            }
            else if(internal_call->mCallData.is_caller == true)  //I call someone
            {
                if(internal_call->mCallData.is_reinvite == false)
                {
                    printf("**sip_cancel_call_by_id\n");
                    if (sip_cancel_call_by_id(call_id)!=RC_OK){
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","internal_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
    					internal_call = findInternalCall(call);
        				if (internal_call!=NULL){
        					pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
        				}
                		pthread_mutex_unlock ( &qu_free_mutex );
                    	return CALL_RESULT_FAIL;
                    }
                }
                else // end call on the re-invite process
                {
                	if (qcall[call_id].response_mode != RESPONSE_EMPTY){
                		int mode ;
                		if (qcall[call_id].media_type == VIDEO)
                			mode = 0;
                		else
                			mode = 1;

                		if (sip_confirm_switch(0,mode,qcall[call_id].phone)!=RC_OK){
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","internal_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
    						internal_call = findInternalCall(call);
        					if (internal_call!=NULL){
        						pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
        					}
                			pthread_mutex_unlock ( &qu_free_mutex );
                			return CALL_RESULT_FAIL;
                		}
                	}else{
                		if (sip_terminate_call_by_id(call_id)!=RC_OK){
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","internal_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
    						internal_call = findInternalCall(call);
        					if (internal_call!=NULL){
        						pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
        					}
                			pthread_mutex_unlock ( &qu_free_mutex );
                			return CALL_RESULT_FAIL;
                		}
                	}

                }
            }
        }
        internal_call = findInternalCall(call);
        if (internal_call!=NULL){

        	internal_call->mCallData.is_reinvite = false;
        	internal_call->mCallData.is_caller = false;
        
        /*Leave the critical section  */
#if MUTEX_DEBUG
    printf("pthread_mutex_unlock for %s at %s Line:%d %s","internal_call->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
        	pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
        }
        pthread_mutex_unlock ( &qu_free_mutex );
        
        return CALL_RESULT_OK;
    }
    call::CallResult ItriCallEngine::keepAlive()
    {
        return call::CALL_RESULT_OK;
    }
    
    call::CallResult ItriCallEngine::networkInterfaceSwitch(const char* localIP,
                                                            unsigned int port,
                                                            int interface_type)
    {
        printf("**ItriCallEngine::networkInterfaceSwitch ip=%s,port=%d,type=%d\n",localIP,port,interface_type);
        if(!b_itri_init)
		{
			printf("have not inited, return");
			return call::CALL_RESULT_FAIL;
		}

//        if (interface_type==NETWORK_NONE){
//        }else {
			NetworkInterfaceSwitch(localIP, port , interface_type);
//        }
        return call::CALL_RESULT_OK;
    }
    
    call::CallResult ItriCallEngine::SwitchCallToPX(const call::ICall* call,const char* PX_Addr)
    {
        
        printf("**ItriCallEngine::SwitchCallToPX , addr=%s\n",PX_Addr);
               
        if (call == NULL)
            return CALL_RESULT_INVALID_INPUT;
        
        printf("icall_addr=%x\n",call);
        
        int call_id = findQCallID(call);
        
        char* target;
        target = strdup(PX_Addr);
        sip_unattend_transfer(qcall[call_id].phone,target);
        
        return call::CALL_RESULT_OK;
    }
    
    call::CallResult ItriCallEngine::SetQTDND(const int isDND)
    {
        
        printf("**ItriCallEngine::SetQTDND , isDND=%d\n",isDND);
        qt_state.dnd = isDND;
        return call::CALL_RESULT_OK;
    }
    
    call::CallResult ItriCallEngine::startConference(const call::ICall* call,
                                     const char* domain,
                                     const char* localIP,
                                     unsigned int audioPort,
                                     unsigned int videoPort,
                                     const char* participant1,
                                     const char* participant2,
                                     const char* participant3)
    {
        printf("**ItriCallEngine::start conference\n");
        printf("**loaclIP=%s,audioPort=%d,videoPort=%d\n",localIP,audioPort,videoPort);
		//Check input parameter
        if (domain   == NULL ||
            localIP  == NULL ||
            (audioPort == 0 && videoPort == 0))
            return CALL_RESULT_INVALID_INPUT;
        
		unsigned int mode;
		if (videoPort==0)
			mode = 0;
		else
			mode = 1;
        
//        mode = 0;
        
        char conference_phone[64]="1000000069";
        char tmp_cp_phone[3][64];
        memset(tmp_cp_phone, 0, sizeof(tmp_cp_phone));
        if (participant1 != NULL){
            printf("tmp participant1 on qcall,phone=%s\n",participant1);
            strcpy(tmp_cp_phone[0],"111");
        }
        if (participant2 != NULL){
            printf("tmp participant2 on qcall,phone=%s\n",participant2);
            strcpy(tmp_cp_phone[1],participant2);
        }
        if (participant3 != NULL){
            printf("tmp participant3 on qcall,phone=%s\n",participant3);
            strcpy(tmp_cp_phone[2],participant3);
        }
		
        int free_id=0;
        ItriCall* pcall = findInternalCall(call);
        if(pcall == NULL) // first invite
        {
			
			printf("**pcall=NULL\n");
            //Create new call
			pcall = new ItriCall();
            
            printf("icall_addr=%x\n",pcall);
            
            //copy remote id to call
            memset(pcall->mCallData.remote_id,0,sizeof(pcall->mCallData.remote_id));
            strcpy(pcall->mCallData.remote_id,conference_phone);
            
            //copy remote domain to call
            memset(pcall->mCallData.remote_domain,0,sizeof(pcall->mCallData.remote_domain));
            strcpy(pcall->mCallData.remote_domain,domain);
            
            //copy local ip to call
            memset(pcall->mCallData.local_audio_ip,0,sizeof(pcall->mCallData.local_audio_ip));
            memset(pcall->mCallData.local_video_ip,0,sizeof(pcall->mCallData.local_video_ip));
            strcpy(pcall->mCallData.local_audio_ip,mEngineData.local_ip);
            strcpy(pcall->mCallData.local_video_ip,mEngineData.local_ip);
            
            //copy local infomation to call
            pcall->mCallData.old_remote_audio_port = 0;
            pcall->mCallData.old_remote_video_port = 0;
            pcall->mCallData.old_local_audio_port = 0;
            pcall->mCallData.old_local_video_port = 0;
            pcall->mCallData.local_audio_port = audioPort;
            pcall->mCallData.local_video_port = videoPort;
            
            pcall->mCallData.remote_audio_port = 0;
            pcall->mCallData.remote_video_port = 0;
			
            
			pcall->mCallData.call_type = CALL_TYPE_OUTGOING;
            pcall->mCallData.is_reinvite = false;
            
            free_id=sip_make_call(pcall,mode,NULL,(char*)conference_phone,audioPort,videoPort);
            
			if (free_id < 0)
				return CALL_RESULT_FAIL;
            
            
		}else
		{
			return CALL_RESULT_FAIL;
		}
        
		/* Enter the critical section*/
#if MUTEX_DEBUG
        printf("pthread_mutex_lock for %s at %s Line:%d %s","pcall->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
        pthread_mutex_lock(&pcall->mCallData.calldata_lock);
        
        printf("add participant on qcall,id=%d\n",free_id);
        if (participant1 != NULL){
            printf("add participant1 on qcall,phone=%s\n",tmp_cp_phone[0]);
            strcpy(qcall[free_id].cp_phone[0],tmp_cp_phone[0]);
        }
        if (participant2 != NULL){
            printf("add participant2 on qcall,phone=%s\n",tmp_cp_phone[1]);
            strcpy(qcall[free_id].cp_phone[1],tmp_cp_phone[1]);
        }
        if (participant3 != NULL){
            printf("add participant3 on qcall,phone=%s\n",tmp_cp_phone[2]);
            strcpy(qcall[free_id].cp_phone[2],tmp_cp_phone[2]);
        }
		pcall->mCallData.is_caller = true;
        pcall->mCallData.is_video_call  = false;
        if(pcall->mCallData.local_video_port != 0)
            pcall->mCallData.is_video_call = true;
		
		pcall->mCallData.mdp_status = call::MDP_STATUS_INIT;
		/*Leave the critical section  */
#if MUTEX_DEBUG
        printf("pthread_mutex_unlock for %s at %s Line:%d %s","pcall->mCallData.calldata_lock\n",__FILE__,__LINE__,__FUNCTION__);
#endif
        pthread_mutex_unlock(&pcall->mCallData.calldata_lock);
		
		
		if(mEngineData.event_listner  != NULL){
            //printf("*******************************1=============%d\n",pcall);
            /*check if in-session , if in-session do not call outgoing call*/
            
            if (!QuCheckInSession()){
                printf("[SIP]!!!!!!!!!!!!in session , call on call outgoing\n");
                mEngineData.event_listner->onCallOutgoing(pcall);
            }else{
                mEngineData.event_listner->onCallMCUOutgoing(pcall);
            }
		}
        return call::CALL_RESULT_OK;
    }
    
    int ItriCallEngine::getSocket(int index)
    {
        return 0;
    }
    
    void ItriCallEngine::requestIDR(const call::ICall* call)
    {
		printf("**ItriCallEngine::requestIDR\n");
        int call_id = findQCallID(call);
        sip_request_idr(call_id);
        return ;
    }
    
    void ItriCallEngine::sendDTMFmessage(const call::ICall* call,const char press)
    {
    }
	
    void ItriCallEngine::setProvision(const call::Config* config)
    {
    	printf("**ItriCallEngine::setProvision\n");
    	printf("audio codec 1=%s\n",config->qtalk.sip.audio_codec_preference_1);
    	printf("audio codec 2=%s\n",config->qtalk.sip.audio_codec_preference_2);
    	printf("audio codec 3=%s\n",config->qtalk.sip.audio_codec_preference_3);
    	memcpy(&sip_config,config,sizeof(Config));

    	strcpy(sip_config.generic.lines.registration_resend_time,"60");

    }

	ItriCall* ItriCallEngine::findInternalCall(const ICall* pcall)
    {
		
		int i=0;
		for (i=0;i<NUMOFCALL;i++){
			if (qcall[i].qt_call==pcall)
				return qcall[i].qt_call;
		}
		
		printf("[error] can't find qt internal call\n");
		return NULL;
		//return findCall(mEngineData.call_list,pcall);
    }
	
	int ItriCallEngine::findQCallID(const ICall* pcall)
    {
		
		int i=0;
		for (i=0;i<NUMOFCALL;i++){
			if (qcall[i].qt_call==pcall)
				return i;
		}
		
		printf("[error] can't find qt internal call\n");
		return -1;
		//return findCall(mEngineData.call_list,pcall);
    }

}
