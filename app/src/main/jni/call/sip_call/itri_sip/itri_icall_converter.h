//
//  sofiasip_eventcallback.h
//  libsofia-sip
//
//  Created by Bill Tocute on 2011/6/24.
//  Copyright 2011�~ Quanta. All rights reserved.
//
#include "itri_call_engine.h"
//SOFIAPUBVAR tag_typedef_t soatag_local_sdp_str_ref;

//#include <sofia-sip/su.h>
//#include <sofia-sip/nua.h> 
//#include <sofia-sip/soa_tag.h>
//#include <sofia-sip/sip_extra.h>    
//#include "sdp_process.h"

#define SIP_PAI "P-Asserted-Identity"

#define SDP_STR_LENGTH 2048
using namespace call;
namespace itricall
{
    ItriCall* findCall(std::list<ItriCall*> callList ,const ICall* pcall)
    {
        ItriCall* internal_call = NULL;
        if (pcall!=NULL)
        {
            std::list<ItriCall*>::iterator it;
            for ( it=callList.begin() ; it != callList.end(); it++ )
            {
                if (*it == pcall)//Call existed in list
                {
                    internal_call = *it;
                    break;
                }
            }
        }
        return internal_call;
    }
    
}

