package com.quanta.qtalk.media.audio;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.media.ISource;

public interface IAudioSource extends ISource 
{
	public void setSink(IAudioSink sink) throws FailedOperateException,UnSupportException;
	public void setFormat(int foramt, int sampleRate,int numChannels) throws FailedOperateException,UnSupportException;	
}