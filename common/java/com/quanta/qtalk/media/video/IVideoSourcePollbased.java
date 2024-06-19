package com.quanta.qtalk.media.video;

import com.quanta.qtalk.UnSupportException;

public interface IVideoSourcePollbased extends IPollbased
{
    public int getMedia(byte[] data,int length)throws UnSupportException;
}
