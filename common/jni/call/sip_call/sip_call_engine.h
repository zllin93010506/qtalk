#ifndef _CALL_SIP_CALL_ENGINE_H
#define _CALL_SIP_CALL_ENGINE_H
#include "icall_engine.h"
#include "sip_call.h"

#include "pthread.h" 
#include "config.h" 
#include <stdio.h>  
#include <sofia-sip/nua.h>  
#include <sofia-sip/sip_header.h>
#include <sofia-sip/sdp.h>
#include <sofia-sip/sip_status.h>
#include <semaphore.h>
#define printf(...)
#define nua_magic_t EngineData

namespace call{class CallEngineFactory;};
namespace sipcall
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
    std::list<SipCall*> call_list;
    call::ICallEventListener* event_listner;
    enum call::ProductType  product_type;
    
    su_home_t               home[1];  /* memory home */
    su_root_t               *root;  
    nua_t                   *nua;  
    nua_handle_t            *regist_handle;
    //enum nua_callstate call_state;
    int expire_time;
    unsigned int listen_port;
    char         local_ip[256];
    bool         finished_event;
    bool         enable_prack;
};
    
class SipCallEngine : public call::ICallEngine 
{
    friend class call::CallEngineFactory;
    SipCallEngine(const char* localIP,unsigned int listenPort);
private:
    EngineData             mEngineData;
    SipCall*               findInternalCall(const call::ICall* call);
    pthread_t*             mpThread;

public:
    virtual ~SipCallEngine(void);
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
                           call::ICallEventListener* listener);

    call::CallResult logout();

    call::CallResult setCapability(call::MDP audioMDPs[],int length_audio_mdps,
                                   call::MDP videoMDPs[],int length_video_mdps);

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
    
    int getSocket(int index);

    void requestIDR(const call::ICall* call);
    void sendDTMFmessage(const call::ICall* call,const char press);
    
    static void * sipThread(void *arg);
};
}
#endif //_CALL_SIP_CALL_ENGINE_H
