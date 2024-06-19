package com.quanta.qtalk.media.audio;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;

public interface IAudioRender
{
    public void start() throws FailedOperateException,UnSupportException;
    public void stop();
    public void setVolume(float leftVolumn, float rightVolumn)throws UnSupportException;
}
