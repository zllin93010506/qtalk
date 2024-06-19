package com.quanta.qtalk.call;


import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.provision.ProvisionUtility.ProvisionInfo;

public interface ICallEngine 
{
    void login(String server,String domain,String realm, int port, String id, String name, String password,int expireTime, ICallEngineListener listerner, String displayName, String registrarServer,boolean enablePrack, int network_type) throws FailedOperateException,Exception;
    void logout();
    void setCapability(MDP[] audio, MDP[] video, int width, int height, int fps) throws FailedOperateException;
    void makeCall(ICall call,String id,String domain, int audioPort,int videoPort, boolean isHold, boolean enableCamera)throws FailedOperateException;
    void makeSeqCall(ICall call,String display, String id,String domain, int audioPort,int videoPort, boolean isHold, boolean enableCamera)throws FailedOperateException;
    void acceptCall(ICall call, int audioPort,int videoPort, boolean enableCameraMute)throws FailedOperateException;
    void hangupCall(ICall call, boolean isReinvite)throws FailedOperateException;
    void keepAlive()throws FailedOperateException;
    void requestIDR(ICall call);
    
    void sr2_switch_network_interface(int interface_type, String ip_addr);
    void qth_switch_call_to_px(ICall call, String PX_SIP_ID);
    void setProvision(ProvisionInfo config);
	
}
