package com.quanta.qtalk.media.audio;

import java.nio.ByteBuffer;
import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.util.Log;

public class AecMicTransform implements IAudioTransform 
{
    private final static String TAG = "AecMicTransform";
    private static final String DEBUGTAG = "QTFa_AecMicTransform";
    private static final int  mInFormat = myAudioFormat.RAW16;
    private static final int  mOutFormat = myAudioFormat.RAW16;
    private IAudioSink mSink = null;
    private int        mSampleRate = 0;
    private int        mNumChannels = 0;
    
    protected AecMicTransform()
    {}
    @Override
    public void setSink(IAudioSink sink) throws FailedOperateException ,UnSupportException
    {
        boolean restart = false;
        if (mHandle!=0) restart = true;
        if (restart)
            stop();
        if(sink!=null && mSampleRate!=0 && mNumChannels!=0)
        {
            sink.setFormat(mOutFormat, mSampleRate, mNumChannels);
        }
        mSink = sink;
        if (restart)
        {
            start();
        }
    }
    @Override
    public void onMedia(final ByteBuffer data,final int length,final long timestamp) throws UnSupportException
    {
        throw new UnSupportException("onMedia(java.nio.ByteArray,int,int) isn't supported, please use onMedia(shrot[],int,long)");
    }
    @Override
    public void onMedia(final short[] data,final int length,final long timestamp)
    {
        try
        {
            if (mHandle != 0)
            	_OnMedia(mHandle, data , length, timestamp);
            	//sw_aec_pop(data, length, timestamp);
                
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"onMedia", e);
        }
    }
    @Override
    public void setFormat(int format, int sampleRate,int numChannels) throws FailedOperateException ,UnSupportException 
    {
        switch(format)
        {
        case mInFormat://AudioFormat.RAW16:
            break;
        default:
            throw new UnSupportException("Failed on setFormat: Not support format type:"+format);
        }
        switch(sampleRate)
        {
        case 8000:
            break;
        case 16000:
            break;
        case 48000:
        	break; 
        default:
            throw new UnSupportException("Failed on setFormat: Not support sample rate:"+sampleRate);
        }
        switch(numChannels)
        {
        case 1:
            break;
        default:
            throw new UnSupportException("Failed on setFormat: Not support number of channels:"+numChannels);
        }
        if (mInFormat!=format || mSampleRate!=sampleRate || mInFormat!=format)
        {
            mSampleRate = sampleRate;
            mNumChannels = numChannels;
            
            if (mHandle!=0)
            {
                stop();
            }
            if (mSink!=null)
            {
                setSink(mSink);        
            }
        }
    }

    @Override
    public void start() throws FailedOperateException 
    {    
        //synchronized to protect mHandle
        try
        {
            synchronized(this)
            {
                if (mSink!=null && mHandle==0)
                {
                    mHandle = _Start(mSink, mSampleRate, mNumChannels);
                }
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"start", e);
            throw new FailedOperateException(e);
        }
    }

    @Override
    public void stop()
    {    
        //synchronized to protect mHandle
        try
        {
            synchronized(this)
            {
                if (mHandle!=0)
                {
                    _Stop(mHandle);
                    mHandle = 0;
                }
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"stop", e);
        }
    }
    private static void onMediaNative(IAudioSink sink,final short[] input,final int length,final long timestamp)
    {
        if (sink!=null)
        {
            try
            {
                sink.onMedia(input, length, timestamp);
                if(StatisticAnalyzer.ENABLE)
                    StatisticAnalyzer.onEncodeSuccess();
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"onMediaNative", e);
            }
        }
    }
    static
    {
    	Log.d(DEBUGTAG,"media_aec_transform");
    	System.loadLibrary("media_aec_transform");
    }
    private int mHandle = 0;
    private static native int  _Start(IAudioSink sink,int samplerate, int channels);
    private static native void _Stop(int handle);
    private static native void _OnMedia(final int handle,final short[] input,final int length,final long timestamp);
}