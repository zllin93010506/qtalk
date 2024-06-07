package com.quanta.qtalk.media.audio;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.media.ISink;

public interface IAudioSink extends ISink
{
    public void setFormat(int format, int sampleRate,int numChannels) throws FailedOperateException ,UnSupportException;
    public void onMedia(java.nio.ByteBuffer data,int length, long timestamp)throws UnSupportException;
    public void onMedia(short[] data,int length, long timestamp)throws UnSupportException;
}
