#ifndef _CALL_CALL_ENGINE_FACTORY_H
#define _CALL_CALL_ENGINE_FACTORY_H
#include "icall_engine.h"
namespace call
{
enum CallEngineType
{
    CALL_ENGINE_TYPE_NA = 0,
    CALL_ENGINE_TYPE_SIP = 1,
    CALL_ENGINE_TYPE_XMPP = 2,
    CALL_ENGINE_TYPE_CLOUD = 3
};
    
class CallEngineFactory 
{
public:
    static ICallEngine* createCallEngine(CallEngineType type,const char* localIP,unsigned int listenPort);
};
}
#endif //_CALL_CALL_ENGINE_FACTORY_H