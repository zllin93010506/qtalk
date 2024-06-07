package com.quanta.qtalk.media.audio;

interface IAudioService
{
	void setSource(in String className,in int foramt,in  int sampleRate,in int numChannels);
	void addTransform(in String className,in boolean isStreamTransform);
	void setSink(in String className);
	void start(in long ssrc,
	           in int listentPort,
			   in String destIP,in int destPort,
			   in int sampleRate,in int payloadID,in String mimeType, in boolean enableTelEvt,
			   in int fecCtrl, in int fecSendPType, in int fecRecvPType, 
			   in boolean enableAudioSRTP, in String AudioSRTPSendKey, in String AudioSRTPReceiveKey,
			   in int audioRTPTOS, 
			   in boolean hasVideo, in int networkType, in int productType);
	boolean isStarted();
	long getSSRC();
	void stop();
	void reset();
	void enableMic(in boolean enable);
	void enableLoudeSpeaker(in boolean enable);
	void adjectVolume(in float direction);
	boolean checkConnection();
	void setMute(in boolean enable);
	void setHold(in boolean enable);
    void sendDTMF(in char press);
}