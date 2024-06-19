package com.quanta.qtalk;

import com.quanta.qtalk.call.MDP;

public interface IQtalkEngineListener {
    void onLogonStateChange(final boolean state,final String msg,final int expireTime);
	void onNewOutGoingCall(final String callID,final String remoteID, final int callState, final boolean enableVideo);
	void onNewIncomingCall(final String callID,final String remoteID, final boolean enableVideo);
	void onNewMCUIncomingCall(final String callID,final String remoteID, final boolean enableVideo);
	void onNewPTTIncomingCall(final String callID,final String remoteID, final boolean enableVideo);
	void onNewVideoSession(final String callID,final int callType,final String remoteID,final String audioDestIP,final int audioLocalPort,final int audioDestPort,final MDP audio,
	                       final String videoDestIP,final int videoLocalPort,final int videoDestPort,final MDP video, final int productType, final long startTime,
	                       final int width, final int height, final int fps, final int kbps, final boolean enablePLI, final boolean enableFIR, final boolean enableTelEvt, final boolean enableCameraMute);
    void onNewAudioSession(final String callID,final int callType,final String remoteID,final String remoteIP,final int audioLocalPort,final int audioDestPort,final MDP audio, final long startTime, final boolean enableTelEvt , final int productType);
	void onSessionEnd(final String callID);
	void onMCUSessionEnd(String callID);
//	void onRemoteHangup(final String callID, final String msg);
	void onLogCall(final String remoteID, final String remoteDisplayName, final int callType,final long startTime, final boolean isVideoCall);
	void onExit();
}
