package com.quanta.qtalk.media.video;

import com.quanta.qtalk.FailedOperateException;

public interface IVideoEncoder
{
    public void requestIDR();
    public void setCameraCallback(IVideoEncoderCB callback)  throws FailedOperateException;
}
