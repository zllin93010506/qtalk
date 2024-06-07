package com.quanta.qtalk.call;
public class SRTPInfo
{
	 public boolean enableSRTP = false; 
	 public String   SRTPSendKey = null;
	 public String   SRTPReceiveKey = null;

     public SRTPInfo(){};
     
     public SRTPInfo(boolean enable, String sendKey, String receiveKey){
    	 enableSRTP = enable;
    	 SRTPSendKey = sendKey;
    	 SRTPReceiveKey = receiveKey;
     }
}
