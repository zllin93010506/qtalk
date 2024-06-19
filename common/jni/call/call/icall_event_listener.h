#ifndef _CALL_CALL_EVENT_LISTENER_H
#define _CALL_CALL_EVENT_LISTENER_H
#include "icall.h"
namespace call
{
class ICallEventListener {
public:
  virtual void onLogonState(bool state,
                            const char* msg,
                            unsigned int expireTime,
                            const char* localIP)  = 0;

  virtual void onCallOutgoing(const ICall* call)      = 0;
  virtual void onCallIncoming(const ICall* call)      = 0;

  virtual void onCallMCUOutgoing(const ICall* call)   = 0;
  virtual void onCallMCUIncoming(const ICall* call)   = 0;

  virtual void onCallPTTIncoming(const ICall* call)   = 0;

  virtual void onCallAnswered(const ICall* call)      = 0;

  virtual void onCallHangup(const ICall* call,
                            const char* msg)          = 0;

  virtual void onCallStateChanged(const ICall* call,
                                  CallState state,
                                  const char* msg)    = 0;

  virtual void onSessionStart(const ICall* call)      = 0;
  virtual void onSessionEnd(const ICall* call)        = 0;

  virtual void onMCUSessionStart(const ICall* call)   = 0;
  virtual void onMCUSessionEnd(const ICall* call)     = 0;
    
  virtual void onReinviteFailed(const ICall* call)    = 0;
    
  virtual void onRequestIDR(const ICall* call)        = 0;
    
  // 1 for success, 0 for fail
  virtual void onCallTransferToPX(const ICall* call,
                                  unsigned int success) = 0;
};
}
#endif //_CALL_CALL_EVENT_LISTENER_H
