package com.quanta.qtalk.media.audio;

import java.nio.ByteBuffer;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.media.audio.codec.G711;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class G711DecoderTransform implements IAudioTransform 
{
    public  final static String CODEC_NAMES[] = {"PCMU","PCMA"};
    public  final static int    DEFAULT_IDS[] = {myAudioFormat.PCMU, myAudioFormat.PCMA};
    private final static String TAG = "G711DecoderTransform";
    private static final String DEBUGTAG = "QTFa_G711DecoderTransform";
    private static final int  mInFormats[] = {myAudioFormat.PCMU, myAudioFormat.PCMA};
    private static final int  mOutFormat = myAudioFormat.RAW16;
    
    private IAudioSink mSink = null;
    private int        mSampleRate = 0;
    private int        mNumChannels = 0;
    
    private short[]    mOutputArray = null;
    private int        mInputFormat = 0;
    
    protected G711DecoderTransform()
    {}
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
    public void onMedia(ByteBuffer data,int length,long timestamp) 
    {
    	//Log.d("AudioStreamEngineTransformJni","onMedia 1 length "+length);
    	if (length <= 0)
    		return ;
        if (mOutputArray==null)
        {
            mOutputArray = new short[160];
        }
        try
        {
        	
        	if (1 == length) {
        		//Log.e("AudioStreamEngineTransformJni","onMedia 2 length "+length);
        		short[] silenceArray = null;
        		silenceArray = new short[160];
        		if (silenceArray != null) {
        			for (int i=0; i<160; i++) {
            			silenceArray[i] = 0;
            		}
        			mSink.onMedia(silenceArray, 160, timestamp);
        		}        		
        		return ;
        	}
        	
            if (data.isDirect())
            {
            	//Log.d("AudioStreamEngineTransformJni","onMedia 3 length  "+length);
                if (mInputFormat == myAudioFormat.PCMU)
                    G711.ulaw2linear(data, mOutputArray, length);
                else if (mInputFormat == myAudioFormat.PCMA)
                    G711.alaw2linear(data, mOutputArray, length);
            }
            else
            {    
                G711.ulaw2linear(data.array(), mOutputArray, length);
            }
            mSink.onMedia(mOutputArray, length, timestamp);
            if(StatisticAnalyzer.ENABLE)
                StatisticAnalyzer.onDecodeSuccess();
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"onMedia", e);
        }
    }
    @Override
    public void onMedia(short[] data, int length, long timestamp) throws UnSupportException
    {
        throw new UnSupportException("This function is not support by the object, please call onMedia(java.nio.ByteBuffer,int,long) instead");
    }
    @Override
    public void setFormat(int format, int sampleRate,int numChannels) throws FailedOperateException ,UnSupportException 
    {
        switch(format)
        {
	        case myAudioFormat.PCMU://AudioFormat.PCMU
	        case myAudioFormat.PCMA://AudioFormat.PCMA
	            mInputFormat = format;
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
        if (mSampleRate!=sampleRate)
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