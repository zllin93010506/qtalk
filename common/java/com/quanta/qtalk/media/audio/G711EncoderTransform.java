package com.quanta.qtalk.media.audio;

import java.nio.ByteBuffer;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.media.audio.codec.G711;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class G711EncoderTransform implements IAudioTransform 
{
    public  final static String CODEC_NAMES[] = {"PCMU","PCMA"};
    public  final static int    DEFAULT_IDS[] = {myAudioFormat.PCMU, myAudioFormat.PCMA};
    private final static String TAG = "G711EncoderTransform";
    private static final String DEBUGTAG = "QTFa_G711EncoderTransform";
    private static final int  mInFormat = myAudioFormat.RAW16;
    private static final int  mOutFormats[] = {myAudioFormat.PCMU, myAudioFormat.PCMA};
    
    private IAudioSink mSink = null;
    private int        mSampleRate = 0;
    private int        mNumChannels = 0;
    
    private ByteBuffer mOutputBuffer = null;
    private byte[]     mOutputArray = null;
    private int  mOutFormat = 0;
    
    protected G711EncoderTransform(int outputFormat)
    {
        mOutFormat = outputFormat;
    }
    @Override
    public void setSink(IAudioSink sink) throws FailedOperateException ,UnSupportException
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setSink");
        if(sink!=null && mSampleRate!=0 && mNumChannels!=0)
        {
            sink.setFormat(mOutFormat, mSampleRate, mNumChannels);
        }
        mSink = sink;
    }
    @Override
    public void onMedia(ByteBuffer data,int length,long timestamp) throws UnSupportException
    {
        throw new UnSupportException("onMedia(java.nio.ByteArray,int,int) isn't supported, please use onMedia(shrot[],int,long)");
    }
    @Override
    public void onMedia(short[] data, int length, long timestamp)
    {
        if (mOutputArray==null)
        {
            mOutputArray = new byte[length*2];
            mOutputBuffer = ByteBuffer.wrap(mOutputArray);
        }
        try
        {
            if (mOutFormat == myAudioFormat.PCMU)
                G711.linear2ulaw(data, 0, mOutputArray, length);
            else if (mOutFormat == myAudioFormat.PCMA)
                G711.linear2alaw(data, 0, mOutputArray, length);
            mSink.onMedia(mOutputBuffer, length, timestamp);
            if(StatisticAnalyzer.ENABLE)
                StatisticAnalyzer.onEncodeSuccess();
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
        case mInFormat://AudioFormat.RAW16
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
        if (mInFormat!=format || mSampleRate!=sampleRate)
        {
            mSampleRate = sampleRate;
            mNumChannels = numChannels;
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
    {    
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "start");
    }

    @Override
    public void stop()
    {    
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "stop");
    }
}