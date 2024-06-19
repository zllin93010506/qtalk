package com.quanta.qtalk.call;


public interface ICall 
{
    public String getRemoteDisplayName();
    public String getRemoteID();
    public String getRemoteDomain();
    
    public String getRemoteAudioIP();
    public int getRemoteAudioPort();
    public int getRemoteVideoPort();

    public String getRemoteVideoIP();
    public int getLocalAudioPort();
    public int getLocalVideoPort();
    
    public MDP getFinalAudioMDP();
    public MDP getFinalVideoMDP();
    
    public int getProductType();
    public long getStartTime();
    public int getCallType();
    public boolean isVideoCall();
    public String getCallID();
    
    public void setListener(ICallListener listener);
    public int getCallState();
    public int getWidth();
    public int getHeight();
    public int getFPS();
    public int getKbps();
    public boolean getEnablePLI();
    public boolean getEnableFIR();
    public boolean getEnableTelEvt();
    
    public boolean getEnableCameraMute();
}
