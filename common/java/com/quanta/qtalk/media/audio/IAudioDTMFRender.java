package com.quanta.qtalk.media.audio;

public interface IAudioDTMFRender
{
    public void onDTMFSend(final short[] data,final int length,final long timestamp);
}
