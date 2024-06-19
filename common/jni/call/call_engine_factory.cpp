#include "call_engine_factory.h"
//#include "sip_call_engine.h"

#include "sip_call/itri_sip/itri_call_engine.h"
namespace call
{
ICallEngine* CallEngineFactory::createCallEngine(CallEngineType type,const char* localIP,unsigned int listenPort)
{
    ICallEngine* result = NULL;
    switch(type)
    {
    case CALL_ENGINE_TYPE_NA:
        break;
    case CALL_ENGINE_TYPE_SIP:
        //result = new sipcall::SipCallEngine(localIP,listenPort);
		result = new itricall::ItriCallEngine(localIP,listenPort);
        break;
    case CALL_ENGINE_TYPE_XMPP:
        break;
    case CALL_ENGINE_TYPE_CLOUD:
        break;
    }
    return result;
}
}
