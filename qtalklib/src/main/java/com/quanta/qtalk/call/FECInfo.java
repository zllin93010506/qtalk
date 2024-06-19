package com.quanta.qtalk.call;
public class FECInfo
{
     public int    FECCtrl = 0;
     public int    FECSendPayloadType = 0;
     public int    FECRecvPayloadType = 0;

     public FECInfo(){};
     
     public FECInfo(int ctrl, int sPType, int rPType){
        FECCtrl = ctrl;
        FECSendPayloadType = sPType;
        FECRecvPayloadType = rPType;
     }
}
