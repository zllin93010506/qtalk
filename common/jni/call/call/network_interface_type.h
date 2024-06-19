//
//  call_engine_factory.h
//  callengine
//
//  Created by nicky lin on 12/8/22.
//  Copyright (c) 2012年 Quanta. All rights reserved.
//

#ifndef _call_network_interface_type_h
#define _call_network_interface_type_h
namespace call
{
    enum NetworkInterfaceType{
        NETWORK_NONE =0,
        NETWORK_3G = 1,
        NETWORK_WIFI = 2,
        NETWORK_ETH = 3,
    };
}

#endif
