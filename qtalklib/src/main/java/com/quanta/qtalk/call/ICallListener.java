package com.quanta.qtalk.call;

public interface ICallListener
{
    public void onSelfCallChange(boolean enableVideo);  //Self do re-invite
    public void onRemoteCallChange(boolean enableVideo); //Remote do re-invite
    public void onReinviteFailed();
    public void onRemoteHangup(String message);
    public void onRemoteCallHold(boolean isHeld);
    public void onRequestIDR();
}
