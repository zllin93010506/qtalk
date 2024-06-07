package com.quanta.qtalk.media.audio;

public interface IAudioDTMF 
{
    public void sendDTMF(final char press);
    public void setDTMFRender(IAudioDTMFRender render);
    public void setDTMFSender(IAudioDTMFSender sender);
}
