package com.quanta.qtalk.media.audio;

import java.nio.ByteBuffer;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class SpeexDecoderTransform implements IAudioTransform 
{
    public  final static String CODEC_NAME = "SPEEX";
    public  final static int    DEFAULT_ID = 97;
    private final static String TAG = "SpeexDecoderTransform";
    private static final String DEBUGTAG = "QTFa_SpeexDecoderTransform";
    private static final int  mInFormat = myAudioFormat.SPEEX;
    private static final int  mOutFormat = myAudioFormat.RAW16;
    
    private IAudioSink mSink = null;
    private int        mSampleRate = 0;
    private int        mNumChannels = 0;
    
    private short[]    mOutputArray = null;
    
    protected SpeexDecoderTransform()
    {}
    @Override
    public void setSink(IAudioSink sink) throws FailedOperateException ,UnSupportException
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setSink");
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
    public void onMedia(final ByteBuffer data,final int length,final long timestamp) 
    {
        try
        {
            if (mHandle!=0)
            {
                if (data.isDirect())
                    _OnMediaDirect(mHandle,data, length, length,mOutputArray);
                else
                    _OnMedia(mHandle,data.array(), length, length,mOutputArray);
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"onMedia", e);
        }
    }
    @Override
    public void onMedia(final short[] data,final  int length,final  long timestamp) throws UnSupportException
    {
        throw new UnSupportException("This function is not support by the object, please call onMedia(java.nio.ByteBuffer,int,long) instead");
    }
    @Override
    public void setFormat(int format, int sampleRate,int numChannels) throws FailedOperateException ,UnSupportException 
    {
        switch(format)
        {
        case mInFormat://AudioFormat.SPEEX
            break;
        default:
            throw new UnSupportException("Failed on setFormat: Not support format type:"+format);
        }
        switch(sampleRate)
        {
        case 8000:
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
        if (mSampleRate!=sampleRate || mInFormat!=format)
        {
            mSampleRate = sampleRate;
            mNumChannels = numChannels;
            mOutputArray = new short[640];
            synchronized(this)
            {    //Reset Sink
                if (mSink!=null)
                {
                    setSink(mSink);        
                }
            }
        }
    }

    @Override
    public void start() throws FailedOperateException 
    {    //synchronized to protect mHandle
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
    {    //synchronized to protect mHandle
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

    private static void onMediaNative(final IAudioSink sink,final short[] input,final int length,final  long timestamp)
    {
        if (sink!=null)
        {
            try
            {
                sink.onMedia(input, length, timestamp);
                if(StatisticAnalyzer.ENABLE)
                    StatisticAnalyzer.onDecodeSuccess();
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"onMediaNative", e);
            }
        }
    }
    static
    {
        System.loadLibrary("media_speex_transform");
    }
    private int mHandle = 0;
    private static native int  _Start(IAudioSink sink, int samplerate, int channels);
    private static native void _Stop(int handle);
    private static native void _OnMedia(final int handle,final  byte[] input,final int length,final  long timestamp, short[] output);
    private static native void _OnMediaDirect(final int handle,final  java.nio.ByteBuffer input,final int length,final long timestamp, short[] output);
}