#ifndef _CALL_CALL_ENGINE_H
#define _CALL_CALL_ENGINE_H
#include "icall_event_listener.h"
#include "call_result.h"
#include "provision.h"

namespace call
{
    
class ICallEngine {
public:
    ~ICallEngine(){};
  virtual CallResult login(const char*    outbound_proxy,//proxy
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
                           ProductType    product_type,
                           ICallEventListener* listener,
                           NetworkInterfaceType network_type) = 0;

  virtual CallResult logout()                            = 0;

  virtual CallResult setCapability(struct MDP audioMDPs[],int length_audio_mdps,
                                   struct MDP videoMDPs[],int length_video_mdps)= 0;

  virtual CallResult makeCall(const ICall* call,
                              const char* remoteID,
                              const char* domain,
                              const char* localIP,
                              unsigned int audioPort,
                              unsigned int videoPort)    = 0;

  virtual CallResult sequentialMakeCall(const ICall* call,
		  	  	  	  	  	  	const char* remoteUUID,
                                const char* remoteID,
                                const char* domain,
                                const char* localIP,
                                unsigned int audioPort,
                                unsigned int videoPort)    = 0;

  virtual CallResult acceptCall(const ICall* call,
                                const char* localIP,
                                unsigned int audioPort,
                                unsigned int videoPort)  = 0;
  
  virtual CallResult hangupCall(const ICall* call)       = 0;
  
  virtual CallResult keepAlive()                         = 0;
  
  virtual CallResult networkInterfaceSwitch(const char* localIP,
                                            unsigned int port,
                                            int interface_type)            = 0;
    
  virtual CallResult SwitchCallToPX(const ICall* call,const char* PX_Addr)            = 0;
  virtual CallResult SetQTDND(const int isDND) = 0 ;  
    
  virtual int getSocket(int index)                       = 0;
    
  virtual void requestIDR(const call::ICall* call)       = 0;
    
  virtual void sendDTMFmessage(const call::ICall* call,const char press)  = 0;
  virtual void setProvision(const call::Config* config)  = 0;

};
}
#endif //_CALL_CALL_ENGINE_H
