#include "sip_call.h"
#include "sip_call_engine.h"
#include "sofiasip_eventcallback.h"
#ifndef WIN32
#include <unistd.h> //for usleep linux
#endif
#include <string.h>
#define ENABLE_SOFIA_LOG 0
#if ENABLE_SOFIA_LOG
#include "sofia-sip/su_log.h"
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

#define SIP_ADDRESS_LENGTH 256
using namespace call;
namespace sipcall
{           
    int SipCallEngine::getSocket(int index)
    {
        return 0;
        /*if (mEngineData.root == NULL)
        {
            return NULL;
        }
        return (int)myGetSocket(mEngineData.root,index);*/
    }
    
    void SipCallEngine::requestIDR(const call::ICall* call)
    {
        SipCall* pcall = findInternalCall(call);
        if(pcall != NULL)
        {
            pcall->requestIDR();
        }
    }
    
    void SipCallEngine::sendDTMFmessage(const call::ICall* call,const char press)
    {
        SipCall* pcall = findInternalCall(call);
        if(pcall != NULL)
        {
            pcall->sendDTMFmessage(press);
        }
    }
    
#if ENABLE_SOFIA_LOG
    static void
    __sofiasip_logger_func(void *stream, char const *fmt, va_list ap)
    {
        if((fmt != NULL) && (strcmp(fmt,"\n") != 0))
        {
            FILE * pFile;
            pFile = fopen ("sofia.txt","a");
            if (pFile!=NULL)
            {
                fprintf(pFile,fmt,ap);
                fclose (pFile);
            }
        }
    }
#endif
    
    void *SipCallEngine::sipThread(void *arg)
    {  
        /* Initialize Sofia-SIP library and create event loop */  
        su_init ();  
#if ENABLE_SOFIA_LOG
        su_log_set_level(NULL,3);
        su_log_redirect(NULL, __sofiasip_logger_func, NULL);
#endif
        EngineData *pEngineData = (EngineData*)arg;
        
        /* initialize memory handling */
        su_home_init(pEngineData->home);
        
        pEngineData->root = su_root_create (NULL); 
        if(pEngineData->root != NULL)
        {
            /* Create a user agent instance. The stack will call the 'event_callback()'   
             * callback when events such as succesful registration to network,   
             * an incoming call, etc, occur.   
             */
            char sip_from[SIP_ADDRESS_LENGTH];
            memset(sip_from,0,SIP_ADDRESS_LENGTH*sizeof(char));
            snprintf(sip_from, SIP_ADDRESS_LENGTH*sizeof(char),"\"%s\"<sip:%s@%s>",pEngineData->display_name,pEngineData->sip_id,pEngineData->proxy_server);
            
            char outbound_proxy[SIP_ADDRESS_LENGTH];
            memset(outbound_proxy,0,SIP_ADDRESS_LENGTH*sizeof(char));
            snprintf(outbound_proxy, SIP_ADDRESS_LENGTH*sizeof(char),"sip:%s:%d",pEngineData->outbound_proxy,pEngineData->port);
            char local_url[SIP_ADDRESS_LENGTH];
            memset(local_url,0,SIP_ADDRESS_LENGTH*sizeof(char));
            if (pEngineData->listen_port==0)
                snprintf(local_url, SIP_ADDRESS_LENGTH*sizeof(char),"sip:%s:*;transport=udp",pEngineData->local_ip);
            else
                snprintf(local_url, SIP_ADDRESS_LENGTH*sizeof(char),"sip:%s:%d;transport=udp",pEngineData->local_ip,pEngineData->listen_port);
            //pEngineData->root->sur_task->sut_port;
            bool set_registrar_server = true;
            if(strlen(pEngineData->registrar_server) == 0)
                set_registrar_server = false;
            else if(strcasecmp(pEngineData->registrar_server, pEngineData->proxy_server) == 0 ||
                    strcasecmp(pEngineData->registrar_server, pEngineData->outbound_proxy) == 0 )
                set_registrar_server = false;
            
            pEngineData->nua = nua_create(pEngineData->root, /* Event loop */  
                                          (nua_callback_f)event_callback, /* Callback for processing events */  
                                          pEngineData, /* Additional data to pass to callback */  
                                          SIPTAG_FROM_STR(sip_from),
                                          NUTAG_URL(local_url),
                                          NUTAG_PROXY(outbound_proxy), /* sip.vzalpha.comAddress to bind to */
                                          TAG_IF(!strchr(pEngineData->outbound_proxy, ':'),
                                                 SOATAG_AF(SOA_AF_IP4_ONLY)),
                                          TAG_IF(strchr(pEngineData->outbound_proxy, ':'),
                                                 SOATAG_AF(SOA_AF_IP6_ONLY)),
                                          TAG_IF(set_registrar_server,
                                                 NUTAG_REGISTRAR(pEngineData->registrar_server)),
                                          NTATAG_SERVER_RPORT(pEngineData->port),
                                          NTATAG_CLIENT_RPORT(pEngineData->listen_port),
                                          SIPTAG_ACCEPT_STR("application/sdp"),
                                          TAG_END()); /* Last tag should always finish the sequence */  
            //su_socket_t temp = su_base_port_mbox(pEngineData->root->sur_task[0]->sut_port);
            
            //Allow: INVITE,OPTIONS,BYE,CANCEL,REFER,SUBSCRIBE,NOTIFY,MESSAGE,UPDATE
            nua_set_params(pEngineData->nua, 
                           SIPTAG_ALLOW_STR("INVITE,ACK,BYE,CANCEL,OPTIONS,MESSAGE,UPDATE,INFO,NOTIFY"),
                           /*NUTAG_APPL_METHOD("OPTIONS"),
                            NUTAG_APPL_METHOD("REFER"),
                            NUTAG_APPL_METHOD("REGISTER"),//Indicate that a method (or methods) are handled by application.
                            NUTAG_APPL_METHOD("NOTIFY"), 
                            NUTAG_APPL_METHOD("INFO"), 
                            NUTAG_APPL_METHOD("ACK"), 
                            NUTAG_APPL_METHOD("SUBSCRIBE"),*/
                            NUTAG_APPL_METHOD("PRACK"), 
                           TAG_IF(pEngineData->enable_prack,SIPTAG_ALLOW_STR("PRACK")),
                           TAG_IF(!pEngineData->enable_prack,SIPTAG_SUPPORTED_STR((char*)NULL)),  //remove 100rel in invite support
                           TAG_IF(!pEngineData->enable_prack,NTATAG_REL100(0)),   //remove 100rel in ring
                           NUTAG_AUTOANSWER(0),
                           NUTAG_AUTOACK(0),
                           NUTAG_AUTOALERT(0),
                           TAG_END());
            if(pEngineData->nua != NULL)
            { 
                /* Run event loop */  
                su_root_run (pEngineData->root);  
                nua_destroy (pEngineData->nua);  
            }

            /* Destroy allocated resources */  
            su_root_destroy (pEngineData->root);
            pEngineData->nua = NULL;
            pEngineData->root = NULL;
        }
        /* deinitialize memory handling */
        su_home_deinit(pEngineData->home);
        su_deinit ();
        
        // Trigger finished event semaphore
        pEngineData->finished_event = true;
        pthread_exit(0);
        return 0;  
    }
    
    void empty_call_list(std::list<SipCall*> *list)
    {
        while(!list->empty())
        {
            SipCall* data = list->back();
            list->pop_back();
            delete data;
        }
        list->clear();
    }
    
    SipCallEngine::SipCallEngine(const char* localIP,unsigned int listenPort)
    {
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
        mEngineData.root = NULL;  
        mEngineData.nua  = NULL;  
        mEngineData.regist_handle = NULL;
        mpThread = NULL;
        // Initialize event semaphore
        mEngineData.finished_event = false;                       // initially set to non signaled state
    }
    
    SipCallEngine::~SipCallEngine()
    {
        // Destroy event semaphore
        empty_mdp_list(&mEngineData.local_audio_mdps);
        empty_mdp_list(&mEngineData.local_video_mdps);
        empty_call_list(&mEngineData.call_list);
    }
    CallResult SipCallEngine::login(const char*    outbound_proxy,
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
                                    ICallEventListener* listener)
    {
        //Check input parameter
        if (outbound_proxy  == NULL ||
            proxy_server    == NULL ||
            //realm_server    == NULL ||
            port            == 0    ||
            username        == NULL ||
            sipID           == NULL ||
            password        == NULL ||
            listener        == NULL)
            return CALL_RESULT_INVALID_INPUT;
        
        //copy outbound_proxy string
        memset(mEngineData.outbound_proxy,0,sizeof(mEngineData.outbound_proxy));
        strcpy(mEngineData.outbound_proxy,outbound_proxy);
        
        //copy proxy_server string
        memset(mEngineData.proxy_server,0,sizeof(mEngineData.proxy_server));
        strcpy(mEngineData.proxy_server,proxy_server);
        
        //copy realm_server string
        memset(mEngineData.realm_server,0,sizeof(mEngineData.realm_server));
        if(realm_server != NULL)
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
        
        // waiting for thread ready
        if(mpThread == 0)
        {
            mpThread = (pthread_t*) malloc(sizeof(pthread_t));
            int result = pthread_create(mpThread, NULL, sipThread, &mEngineData);
            if(result != 0)
            {
                free(mpThread);
                mpThread = 0;
                return CALL_RESULT_FAIL;
            }
            for (int i = 0; i<1000 && mEngineData.nua == NULL; i++)
            {
#ifdef WIN32
                Sleep(1);
#else
                usleep(1000);
#endif
            }
            if(mEngineData.nua == NULL)
            {
                free(mpThread);
                mpThread = 0;
                return CALL_RESULT_FAIL;
            }
        }
        
        if(mEngineData.regist_handle == NULL)
        {
            char sip_address[SIP_ADDRESS_LENGTH];
            snprintf(sip_address, SIP_ADDRESS_LENGTH*sizeof(char),"sip:%s@%s",mEngineData.sip_id,mEngineData.proxy_server); 
            mEngineData.regist_handle = nua_handle(mEngineData.nua, 
                                                   NULL,
                                                   //SIPTAG_FROM_STR(sip_address), 
                                                   SIPTAG_TO_STR(sip_address), 
                                                   TAG_END());
            int expire_time_str_length = 256;
            char expire_time_str[expire_time_str_length];
            memset(expire_time_str,0, expire_time_str_length*sizeof(char));
            snprintf(expire_time_str, expire_time_str_length*sizeof(char), "expires=%d",mEngineData.expire_time);
            char* pContact = NULL;
            char contact_url[SIP_ADDRESS_LENGTH];
            if (mEngineData.listen_port!=0 && strcmp(mEngineData.local_ip,"0.0.0.0")!=0)
            {
                memset(contact_url,0,SIP_ADDRESS_LENGTH*sizeof(char));
                if (mEngineData.listen_port==0)
                    snprintf(contact_url,SIP_ADDRESS_LENGTH*sizeof(char)
                            ,"<sip:%s@%s>;expires=%d"                            
                            mEngineData.sip_id,
                            mEngineData.local_ip,
                            mEngineData.expire_time);
                else
                    snprintf(contact_url,SIP_ADDRESS_LENGTH*sizeof(char),
                            "<sip:%s@%s:%u>;expires=%d",
                            mEngineData.sip_id,
                            mEngineData.local_ip,
                            mEngineData.listen_port,
                            mEngineData.expire_time);
                pContact = contact_url;
            }
            int sip_expire_str_length = 256;
            char sip_expire_str[sip_expire_str_length];
            memset(sip_expire_str,0,sip_expire_str_length*sizeof(char));
            snprintf(sip_expire_str,sip_expire_str_length*sizeof(char), "%d",mEngineData.expire_time);
            //Send login message
            if(nua_handle_has_registrations(mEngineData.regist_handle) != 0)
                nua_unregister(mEngineData.regist_handle,TAG_END());
            nua_register(mEngineData.regist_handle,
                         NUTAG_KEEPALIVE(0),//Disable auto keep alive
                         TAG_IF(pContact==NULL,NUTAG_M_USERNAME(mEngineData.sip_id)),
                         TAG_IF(pContact==NULL,NUTAG_M_FEATURES(expire_time_str)),
                         TAG_IF(pContact!=NULL,SIPTAG_CONTACT_STR(pContact)),
                         SIPTAG_EXPIRES_STR(sip_expire_str),
                         NUTAG_OUTBOUND("no-options-keepalive"), //for solve to send "nokia" to server
                         NUTAG_OUTBOUND("no-validate"),
                         TAG_END());
        }
        
        return CALL_RESULT_OK;
    }

    CallResult SipCallEngine::logout()
    {
        //Hangup all call in call_list
        if(mEngineData.call_list.size() > 0)
        {
            for (std::list<SipCall*>::iterator it=mEngineData.call_list.begin(); it!=mEngineData.call_list.end() ; it++)
            {
                /* Enter the critical section*/
                pthread_mutex_lock(&(*it)->mCallData.calldata_lock);
                
                //Send hangupCall message
                nua_bye((*it)->mCallData.call_handle, TAG_END());
                
                /*Leave the critical section  */
                pthread_mutex_unlock(&(*it)->mCallData.calldata_lock);
            }
        }
        
        //Send logout message
        if(mEngineData.regist_handle != NULL)
        {  
            if(nua_handle_has_registrations(mEngineData.regist_handle) != 0)
                nua_unregister(mEngineData.regist_handle,TAG_END());
            nua_handle_destroy (mEngineData.regist_handle);
            mEngineData.regist_handle = NULL;
        }

        if(mpThread != 0)
        {
            if(mEngineData.nua != NULL)
                nua_shutdown(mEngineData.nua);
            //Wiat for thread finish
            //pthread_join(*mpThread, NULL);
            //To avoid deadlock of this function,wait finish_envent with 2 sec timeout
            for(int i=0;i<20;i++)
            {
                if (mEngineData.finished_event) break;  // Successfully got semaphore.
#ifdef WIN32
                Sleep(100); //sem_trywait use WaitForSingleObject 1 msec
#else
                usleep(100000);
#endif 
            }
            free(mpThread);
            mpThread = 0;
        }
        
        empty_call_list(&mEngineData.call_list);
        //memset(&mEngineData,0,sizeof(mEngineData));
        
        memset(mEngineData.outbound_proxy, 0,sizeof(mEngineData.outbound_proxy));
        memset(mEngineData.proxy_server,   0,sizeof(mEngineData.proxy_server));
        memset(mEngineData.realm_server,   0,sizeof(mEngineData.realm_server));
        memset(mEngineData.user_name,      0,sizeof(mEngineData.user_name));
        memset(mEngineData.sip_id,         0,sizeof(mEngineData.sip_id));
        memset(mEngineData.password,       0,sizeof(mEngineData.password));
        mEngineData.port = 0;
        mEngineData.expire_time = 0;
        
        mEngineData.event_listner = NULL;
        
        return CALL_RESULT_OK;
    }

    CallResult SipCallEngine::setCapability(call::MDP audioMDPs[],int length_audio_mdps,
                                            call::MDP videoMDPs[],int length_video_mdps)
    {
        empty_mdp_list(&mEngineData.local_audio_mdps);
        empty_mdp_list(&mEngineData.local_video_mdps);
        for (int i=0;i<length_audio_mdps;i++)
        {
            struct MDP* mdp = (struct MDP*)malloc(sizeof(audioMDPs[i]));
            init_mdp(mdp);
            copy_mdp(mdp, &audioMDPs[i]);
            
            mEngineData.local_audio_mdps.push_back(mdp);
        }
        
        for (int i=0;i<length_video_mdps;i++)
        {
            struct MDP* mdp = (struct MDP*)malloc(sizeof(videoMDPs[i]));
            init_mdp(mdp);
            copy_mdp(mdp, &videoMDPs[i]);
            
            mEngineData.local_video_mdps.push_back(mdp);
        }
        return CALL_RESULT_OK;
    }

    CallResult SipCallEngine::makeCall(const call::ICall* call,
                                       const char* remoteID,
                                       const char* domain,
                                       const char* localIP,
                                       unsigned int audioPort,
                                       unsigned int videoPort)
    {
        if(mEngineData.nua == NULL) // does not register
            return CALL_RESULT_FAIL;
        
        //Check input parameter
        if (remoteID == NULL ||
            domain   == NULL ||
            localIP  == NULL ||
            (audioPort == 0 && videoPort == 0))
            return CALL_RESULT_INVALID_INPUT;
        
        SipCall* pcall = findInternalCall(call);
        if(pcall == NULL) // first invite
        {
            //Create new call
            pcall = new SipCall();
            
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
            /* Destination address */
            char sip_to_address[SIP_ADDRESS_LENGTH];
            memset(sip_to_address, 0, SIP_ADDRESS_LENGTH*sizeof(char));
            //Telephone numbers
            //The "user=phone" parameter indicates that the user portion of the URI (the part to the left of the @ sign) should be treated as a tel URI
            //"sip:+1555000000@domain.com;user=phone" should be treated as tel:+1555000000"
            if (pcall->mCallData.remote_id!=NULL && pcall->mCallData.remote_id[0]=='+')
                snprintf(sip_to_address, SIP_ADDRESS_LENGTH*sizeof(char),"sip:%s@%s;user=phone",
                                    pcall->mCallData.remote_id,pcall->mCallData.remote_domain);
            else
                snprintf(sip_to_address, SIP_ADDRESS_LENGTH*sizeof(char),"sip:%s@%s",pcall->mCallData.remote_id,pcall->mCallData.remote_domain);

            sip_to_t *to = sip_to_create(NULL, URL_STRING_MAKE(sip_to_address));
            if (!to)
            {
                delete pcall;
                pcall = NULL;
                return CALL_RESULT_FAIL;
            }
            //to->a_display = pcall->mCallData.remote_id;
            //char sip_from_address[SIP_ADDRESS_LENGTH];
            //memset(sip_from_address, 0, sizeof(sip_from_address));
            //sprintf(sip_from_address,"sip:%s@%s:%d",mEngineData.login_id,mEngineData.server,mEngineData.port);
            
            /* create operation handle */
            pcall->mCallData.call_handle = nua_handle(mEngineData.nua, 
                                                      NULL, 
                                                      //SIPTAG_FROM_STR(sip_from_address),
                                                      SIPTAG_TO(to), 
                                                      TAG_END()); 
            
            if (pcall->mCallData.call_handle == NULL) 
            {
                printf("cannot create operation handle\n");
                delete pcall;
                pcall = NULL;
                return CALL_RESULT_FAIL;
            }
            pcall->mCallData.call_type = CALL_TYPE_OUTGOING;
            pcall->mCallData.is_reinvite = false;
            
            //add to call list
            mEngineData.call_list.push_back(pcall);
        }
        else   //re-invite or hold
        {
            /* Enter the critical section*/
            pthread_mutex_lock(&pcall->mCallData.calldata_lock);
            if(pcall->mCallData.is_reinvite == true)//Already got an re-invite message
            {
                /*Leave the critical section  */
                pthread_mutex_unlock(&pcall->mCallData.calldata_lock);
                return CALL_RESULT_FAIL;
            }
            // The callee can not make call when hold 
            /*else if(pcall->mCallData.call_state == CALL_STATE_HOLD && pcall->mCallData.is_recv_hold == true) 
            {
                //Leave the critical section  
                pthread_mutex_unlock(&pcall->mCallData.calldata_lock);
                return CALL_RESULT_FAIL;
            }*/
            else
            {
                //copy local ip to call
                memset(pcall->mCallData.local_audio_ip,0,sizeof(pcall->mCallData.local_audio_ip));
                memset(pcall->mCallData.local_video_ip,0,sizeof(pcall->mCallData.local_video_ip));
                if(strcmp(localIP, "0.0.0.0") == 0) //make hold
                {
                    strcpy(pcall->mCallData.local_audio_ip,localIP);
                    strcpy(pcall->mCallData.local_video_ip,localIP);
                    pcall->mCallData.is_send_hold = true;
                    //pcall->mCallData.pre_hold_state = SEND_STATE_HOLD;
                }
                else 
                {
                    strcpy(pcall->mCallData.local_audio_ip,mEngineData.local_ip);
                    strcpy(pcall->mCallData.local_video_ip,mEngineData.local_ip);
                    pcall->mCallData.is_send_hold = false;  
                    //pcall->mCallData.pre_hold_state = SEND_STATE_UNHOLD;
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
                pthread_mutex_unlock(&pcall->mCallData.calldata_lock);
            }
        }
        /* Enter the critical section*/
        pthread_mutex_lock(&pcall->mCallData.calldata_lock);
        
        //Send make call message
        char sdp_string[SDP_STR_LENGTH];
        makeSDP(mEngineData,pcall->mCallData,sdp_string, SDP_STR_LENGTH*sizeof(char));
        printf("\"make call SDP\"\n%s\n",sdp_string);
        
        pcall->mCallData.is_caller = true;
        pcall->mCallData.is_video_call  = false;
        if(pcall->mCallData.local_video_port != 0)
            pcall->mCallData.is_video_call = true;
        
        pcall->mCallData.mdp_status = call::MDP_STATUS_INIT;
        /*Leave the critical section  */
        pthread_mutex_unlock(&pcall->mCallData.calldata_lock);
        
        /* The timeout for outstanding re-INVITE transactions in seconds.
         * Chosen to match the allowed cancellation timeout for proxies
         * described in RFC 3261 Section 13.3.1.1 */
        //Send MakeCall message
        if(mEngineData.event_listner   != NULL)
            mEngineData.event_listner->onCallOutgoing(pcall);
        
        nua_invite(pcall->mCallData.call_handle,
                   SOATAG_ADDRESS(pcall->mCallData.local_audio_ip),
                   SOATAG_USER_SDP_STR(sdp_string),
                   //SOATAG_USER_SDP(l_sdp),
                   SOATAG_RTP_SORT(SOA_RTP_SORT_REMOTE),
                   SOATAG_RTP_SELECT(SOA_RTP_SELECT_ALL),
                   NUTAG_AUTOACK(0),
                   NUTAG_AUTOANSWER(0),
                   TAG_IF(pcall->mCallData.is_send_hold==true ,SOATAG_HOLD("#")),
                   TAG_IF(pcall->mCallData.is_send_hold==false,SOATAG_HOLD((char*)NULL)),
                   //SOATAG_REUSE_REJECTED(0),
                   //TAG_IF(pcall->mCallData.is_reinvite, SIPTAG_ALLOW(NULL)),
                   //TAG_IF(pcall->mCallData.enable_telephone_event,SOATAG_AUDIO_AUX("telephone-event")),
                   TAG_END());
        return CALL_RESULT_OK;
    }

    CallResult SipCallEngine::acceptCall(const ICall* pcall,
                                         const char* localIP,
                                         unsigned int audioPort,
                                         unsigned int videoPort)
    {
        //Check input parameter
        if (pcall == NULL   ||
            localIP == NULL ||
            (audioPort == 0 && videoPort ==0) )
            return CALL_RESULT_INVALID_INPUT;
        
        //Check call object
        SipCall* internal_call = findInternalCall(pcall);
        if (internal_call==NULL)
            return CALL_RESULT_INVALID_INPUT;
        
        /* Enter the critical section*/
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
            
            //TODO answer invite without SDP
            /*if(internal_call->remoteAudioMDP.empty() && internal_call->remoteAudioMDP.empty())
            {
                char sdp_string[SDP_STR_LENGTH];
                makeSDP(mEngineData,internal_call->mCallData,sdp_string);
                printf("\"make call SDP\"\n%s\n",sdp_string);
                nua_respond(internal_call->mCallData.call_handle, SIP_200_OK, 
                            SIPTAG_CONTENT_TYPE_STR("application/sdp"),
                            SOATAG_USER_SDP_STR(sdp_string),
                            SOATAG_ADDRESS(internal_call->mCallData.local_audio_ip),
                            NUTAG_AUTOANSWER(0),
                            TAG_IF(internal_call->mCallData.call_state == CALL_STATE_HOLD,SOATAG_HOLD("#")),
                            TAG_IF(internal_call->mCallData.call_state != CALL_STATE_HOLD,SOATAG_HOLD((char*)NULL)),
                            TAG_IF(internal_call->mCallData.enable_telephone_event,SOATAG_AUDIO_AUX("telephone-event")),
                            TAG_END());
                
                //Leave the critical section 
                pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
                return CALL_RESULT_OK;
            }*/
            bool result = answerSDP(mEngineData,internal_call,sdp_string, SDP_STR_LENGTH*sizeof(char));
            //Doesn't mapping MDP
            if(result == false)
            {
                nua_respond(internal_call->mCallData.call_handle, SIP_415_UNSUPPORTED_MEDIA, TAG_END());
                if(mEngineData.event_listner != NULL)
                { 
                    mEngineData.event_listner->onCallHangup(internal_call,sip_415_Unsupported_media);
                }
                mEngineData.call_list.remove(internal_call);
                nua_handle_destroy(internal_call->mCallData.call_handle);
                internal_call->mCallData.call_handle = NULL;
                
                /*Leave the critical section  */
                pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
                
                delete internal_call;
                return CALL_RESULT_FAIL;
            }

            //check if A->V or V->A
            internal_call->mCallData.is_session_change = false;
            if(internal_call->mCallData.is_reinvite)
            {
                if( strcasecmp( audio_mime ,internal_call->mCallData.audio_mdp.mime_type) ||
                   strcasecmp( video_mime ,internal_call->mCallData.video_mdp.mime_type))
                {
                    internal_call->mCallData.is_session_change = true;
                }
            }
            LOGD("sdp_string:%s",sdp_string);
            printf("\"answerSDP\"\n%s\n",sdp_string);
            internal_call->mCallData.mdp_status = MDP_STATUS_FINAL;
            nua_respond(internal_call->mCallData.call_handle, SIP_200_OK, 
                        SIPTAG_CONTENT_TYPE_STR("application/sdp"),
                        SOATAG_USER_SDP_STR(sdp_string),
                        SOATAG_ADDRESS(internal_call->mCallData.local_audio_ip),
                        NUTAG_AUTOANSWER(0),
                        TAG_IF(internal_call->mCallData.call_state == CALL_STATE_HOLD,SOATAG_HOLD("#")),
                        TAG_IF(internal_call->mCallData.call_state != CALL_STATE_HOLD,SOATAG_HOLD((char*)NULL)),
                        TAG_IF(internal_call->mCallData.enable_telephone_event,SOATAG_AUDIO_AUX("telephone-event")),
                        TAG_END());
            
            //internal_call->mCallData.is_reinvite = false;
        }
        
        /*Leave the critical section  */
        pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
        
        return CALL_RESULT_OK;
    }

    CallResult SipCallEngine::hangupCall(const ICall* pcall)
    {
        //Check input parameter
        if (pcall == NULL)
            return CALL_RESULT_INVALID_INPUT;

        //Check call object
        SipCall* internal_call = findInternalCall(pcall);
        if (internal_call==NULL)
            return CALL_RESULT_INVALID_INPUT;
        
        /* Enter the critical section*/
        pthread_mutex_lock(&internal_call->mCallData.calldata_lock);
        
        //Send hangupCall message
        if((internal_call->mCallData.call_state  == CALL_STATE_ACCEPT ||
            internal_call->mCallData.call_state  == CALL_STATE_HOLD ) &&
            internal_call->mCallData.is_reinvite == false)
        {   
            nua_bye(internal_call->mCallData.call_handle, TAG_END());
        }
        else  // CALL_STATE_RINGING
        {
            if(internal_call->mCallData.is_caller == false)  //someone call me
            {
                if(internal_call->mCallData.is_reinvite == false)
                {
                    nua_respond(internal_call->mCallData.call_handle, 
                                SIP_486_BUSY_HERE, 
                                TAG_NULL());
                }
                else 
                {
                    nua_respond(internal_call->mCallData.call_handle, 
                                SIP_406_NOT_ACCEPTABLE, 
                                TAG_END());
                }
            }
            else if(internal_call->mCallData.is_caller == true)  //I call someone
            {
                if(internal_call->mCallData.is_reinvite == false)
                {
                    nua_cancel(internal_call->mCallData.call_handle, TAG_END());
                }
                else // end call on the re-invite process
                {
                    nua_bye(internal_call->mCallData.call_handle, TAG_END());
                }
            }
        }
        internal_call->mCallData.is_reinvite = false;
        internal_call->mCallData.is_caller = false;
        
        /*Leave the critical section  */
        pthread_mutex_unlock(&internal_call->mCallData.calldata_lock);
        
        return CALL_RESULT_OK;
    }

    CallResult SipCallEngine::keepAlive()
    {
        //Send KeepAlive message
        if(nua_handle_has_registrations(mEngineData.regist_handle) != 0)
        { 
            int expire_time_str_length = 256;
            char expire_time_str[expire_time_str_length];
            memset(expire_time_str,0,expire_time_str_length*sizeof(char));
            snprintf(expire_time_str, expire_time_str_length*sizeof(char), "expires=%d",mEngineData.expire_time);

            char* pContact = NULL;
            char contact_url[SIP_ADDRESS_LENGTH];
            if (mEngineData.listen_port!=0 && strcmp(mEngineData.local_ip,"0.0.0.0")!=0)
            {
                memset(contact_url,0,sizeof(contact_url));
                if (mEngineData.listen_port==0)
                    snprintf(contact_url,SIP_ADDRESS_LENGTH*sizeof(char),"<sip:%s@%s>;expires=%d",
                            mEngineData.sip_id,
                            mEngineData.local_ip,
                            mEngineData.expire_time);
                else
                    snprintf(contact_url,SIP_ADDRESS_LENGTH*sizeof(char),"<sip:%s@%s:%u>;expires=%d",
                            mEngineData.sip_id,
                            mEngineData.local_ip,
                            mEngineData.listen_port,
                            mEngineData.expire_time);
                pContact = contact_url;
            }
            int sip_expire_str_length = 256;
            char sip_expire_str[sip_expire_str_length];
            memset(sip_expire_str,0,sip_expire_str_length*sizeof(char));
            snprintf(sip_expire_str,sip_expire_str_length*sizeof(char), "%d",mEngineData.expire_time);
                         
            nua_register(mEngineData.regist_handle,
                         NUTAG_KEEPALIVE(0),//Disable auto keep alive
                         TAG_IF(pContact==NULL,NUTAG_M_USERNAME(mEngineData.sip_id)),
                         TAG_IF(pContact==NULL,NUTAG_M_FEATURES(expire_time_str)),
                         TAG_IF(pContact!=NULL,SIPTAG_CONTACT_STR(pContact)),
                         SIPTAG_EXPIRES_STR(sip_expire_str),
                         TAG_END());
            return CALL_RESULT_OK;
        }else
            return CALL_RESULT_FAIL;
    }
    //===============================
    SipCall* SipCallEngine::findInternalCall(const ICall* pcall)
    {
        return findCall(mEngineData.call_list,pcall);
    }
}

//SDP from linphone
/*
 m=video 10614 RTP/AVP 103 102
 a=rtpmap:103 H264/90000
 a=fmtp:103 packetization-mode=1;profile-level-id=428014
 a=rtpmap:102 H264/90000
 a=fmtp:102 profile-level-id=428014
 Hex[42] = 66 = profile base
 
 3.1 packetization-mode: 
 表示支持的封包模式. 
 當packetization-mode的值為0時或不存在時,必須使用單一NALU單元模式. 
 當packetization-mode的值為1時必須使用非交錯(non-interleaved)封包模式. 
 當packetization-mode的值為2時必須使用交錯(interleaved)封包模式. 
 這個參數不可以取其他的值.
 3.2 sprop-parameter-sets: 
 這個參數可以用於傳輸H.264的序列參數集和圖像參數NAL單元.這個參數的值採用Base64進行編碼.不同的參數集間用","號隔開. 
 3.3 profile -level-id:   
 這個參數用於指示H.264流的profile類型和級別.由Base16(十六進制)表示的3個字節.
 第一個字節表示H.264的Profile類型,
 第三個字節表示H.264的Profile級別: 
 */
