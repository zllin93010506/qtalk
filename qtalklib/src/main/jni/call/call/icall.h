#ifndef _CALL_ICALL_H
#define _CALL_ICALL_H
#include "mdp.h"
#include "product_type.h"
#include "call_type.h"
#include "call_state.h"
#include "network_interface_type.h"

namespace call
{
class ICall {
public:
  virtual const char*   getRemoteDisplayName() = 0;
  virtual const char*   getRemoteID()       = 0;
  virtual const char*   getRemoteDomain()   = 0;

  virtual const char*   getRemoteAudioIP()  = 0;
  virtual unsigned int  getRemoteAudioPort()= 0;
  virtual const char*   getRemoteVideoIP()  = 0;
  virtual unsigned int  getRemoteVideoPort()= 0;

  virtual const char*   getLocalAudioIP()   = 0;
  virtual unsigned int  getLocalAudioPort() = 0;
  virtual const char*   getLocalVideoIP()   = 0;
  virtual unsigned int  getLocalVideoPort() = 0;

  virtual const MDP     getFinalAudioMDP()  = 0;
  virtual const MDP     getFinalVideoMDP()  = 0;

  virtual ProductType   getProductType()    = 0;
  virtual unsigned long getStartTime()      = 0;
  virtual CallType      getCallType()       = 0;
  virtual CallState     getCallState()      = 0;
  virtual bool          isVideoCall()       = 0;
  virtual const char*   getCallID()         = 0;
    
  virtual unsigned int  getBandwidthCT()    = 0;
  virtual unsigned int  getBandwidthAS()    = 0;
  virtual bool          isReinvite()        = 0;
  virtual bool          isSendHold()        = 0;
  virtual bool          isRecvHold()        = 0;
  virtual bool          isCaller()          = 0;

  virtual NetworkInterfaceType     getNetworkType()      = 0;
  virtual bool          isSupportTelephoneEvent() = 0;
  //virtual void          revertSendHold()    = 0;
  
};
}
#endif //_CALL_ICALL_H