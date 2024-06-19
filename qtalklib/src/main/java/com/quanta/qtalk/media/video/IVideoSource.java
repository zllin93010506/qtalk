package com.quanta.qtalk.media.video;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.media.ISource;

public interface IVideoSource extends ISource {
	public void setSink(IVideoSink sink) throws FailedOperateException,UnSupportException;
    public void destory() ;
}