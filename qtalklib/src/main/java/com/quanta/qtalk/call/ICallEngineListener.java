package com.quanta.qtalk.call;


public interface ICallEngineListener 
{
    void onLogonState(final boolean state,final String msg, final int expire);
    void onCallOutgoing(ICall call);
    void onCallMCUOutgoing(ICall call);
    void onCallIncoming(ICall call);
    void onCallMCUIncoming(ICall call);
    void onCallPTTIncoming(ICall call);
    void onCallAnswered(ICall call);
    void onCallHangup(ICall call,String msg);
    void onCallStateChange(ICall call, int state, String msg);
    void onSessionStart(final String callID,final int callType,final String remoteID,final String audioDestIP,final int audioLocalPort,final int audioDestPort,final MDP audio,
                        final String videoDestIP,final int videoLocalPort,final int videoDestPort,final MDP video, final int productType, final long startTime,
                        final int width, final int height, final int fps, final int kbps, final boolean enablePLI, final boolean enableFIR, final boolean enableTelEvt, 
                        final boolean enableCamera);
    void onMCUSessionStart(final String callID,final int callType,final String remoteID,final String audioDestIP,final int audioLocalPort,final int audioDestPort,final MDP audio,
            final String videoDestIP,final int videoLocalPort,final int videoDestPort,final MDP video, final int productType, final long startTime,
            final int width, final int height, final int fps, final int kbps, final boolean enablePLI, final boolean enableFIR, final boolean enableTelEvt);
    void onSessionEnd(ICall call);
    void onMCUSessionEnd(ICall call);
    void onCallTransferToPX(ICall call, int success);
}
