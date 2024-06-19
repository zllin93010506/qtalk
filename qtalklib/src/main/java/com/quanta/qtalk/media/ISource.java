package com.quanta.qtalk.media;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;

public interface ISource 
{
	public void start() throws FailedOperateException,UnSupportException;
	public void stop();
}
