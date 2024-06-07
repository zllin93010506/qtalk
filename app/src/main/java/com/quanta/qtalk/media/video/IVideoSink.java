package com.quanta.qtalk.media.video;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.media.ISink;

public interface IVideoSink extends ISink {
    public void setFormat(int format, int width, int height)  throws FailedOperateException, UnSupportException;
    public boolean onMedia(java.nio.ByteBuffer data,int length, long timestamp, short rotation)throws UnSupportException;
    public void setLowBandwidth(boolean showLowBandwidth);
    public void release();    
}
