package com.quanta.qtalk.media.audio;

import com.quanta.qtalk.FailedOperateException;

public interface IAudioStreamEngine 
{
	public void setListenerInfo(int rtpListenPort)  throws FailedOperateException;
	public void setRemoteInfo(long ssrc,String destIP, int destPort, int productType) throws FailedOperateException;
	public void setMediaInfo(int payloadID, String mimeType, boolean enableTelEvt) throws FailedOperateException;
	public void setFECInfo(int fecCtrl, int fecSendPType, int fecRecvPType) throws FailedOperateException;			//eason - set FEC info
	public void setSRTPInfo(boolean enableAudioSRTP, String AudioSRTPSendKey, String AudioSRTPReceiveKey) throws FailedOperateException;         //Iron  - set SRTP info
	public void setAudioRTPTOS(int audioRTPTOS) throws FailedOperateException;
	public long getSSRC() throws FailedOperateException;
	
	public void setReportCallback(IAudioStreamReportCB callback) throws FailedOperateException;
	public void setHold(boolean enable);
	public void setHasVideo(boolean hasVideo);
    public void setNetworkType(int networkType);
}
