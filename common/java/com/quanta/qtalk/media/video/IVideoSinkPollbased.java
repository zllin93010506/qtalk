package com.quanta.qtalk.media.video;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;

public interface IVideoSinkPollbased extends IPollbased
{
    public void setSource(IVideoSourcePollbased source) throws FailedOperateException,UnSupportException;
}
