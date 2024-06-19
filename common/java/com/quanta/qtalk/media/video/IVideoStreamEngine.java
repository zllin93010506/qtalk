package com.quanta.qtalk.media.video;

import com.quanta.qtalk.FailedOperateException;

public interface IVideoStreamEngine 
{
	public void setListenerInfo(int rtpListenPort, boolean enableTFRC) throws FailedOperateException;
	public void setRemoteInfo(long ssrc,String destIP, int destPort,int fps, int kbps, boolean enablePLI, boolean enableFIR, boolean letGopEqFps, int productType) throws FailedOperateException;
	public void setMediaInfo(int sampleRate,int payloadID, String mimeType) throws FailedOperateException;
	public long getSSRC()  throws FailedOperateException;
	public void setReportCallback(IVideoStreamReportCB callback)  throws FailedOperateException;
	public void setSourceCallback(IVideoStreamSourceCB callback)  throws FailedOperateException;
	public void setHold(boolean enable);
	public void setDemandIDRCallBack(IDemandIDRCB callback)  throws FailedOperateException;
	
	public void setFECInfo(int fecCtrl, int fecSendPType, int fecRecvPType) throws FailedOperateException;			//eason - set FEC info
	public void setSRTPInfo(boolean enableVideoSRTP,String VideoSRTPSendKey, String VideoSRTPReceiveKey) throws FailedOperateException;	//Iron - set SRTP info
	public void setStreamInfo(int outMaxBandwidth, boolean autoBandwidth, int videoMTU, int videoRTPTOS) throws FailedOperateException;
    public void setNetworkType(int networkType);
}
