package com.quanta.qtalk.call;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.call.engine.NativeCallEngine;

public class CallEngineFactory
{
    public static ICallEngine createCallEngine(int listenPort) throws FailedOperateException,UnSupportException
    {
        return new NativeCallEngine(listenPort);
    }
}
