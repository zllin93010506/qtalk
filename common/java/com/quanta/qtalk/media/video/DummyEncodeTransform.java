package com.quanta.qtalk.media.video;

import java.nio.ByteBuffer;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;

import android.content.Context;
import android.os.ConditionVariable;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class DummyEncodeTransform implements IVideoTransform, IVideoStreamSourceCB, IVideoEncoder
{
    public  final static String  CODEC_NAME      = "H264";
    public  final static int     DEFAULT_ID      = 109;
    private static final String DEBUGTAG = "QTFa_DummyEncodeTransform";
    private final int  mOutFormat = VideoFormat.H264;
    
    private IVideoSink mVideoSink = null;
    private int        mWidth = 0;
    private int        mHeight = 0;
    private int        mInFormat = 0;
    private int        mGapMediaTime = 100;
    private Context    mContext = null;
    //LocalSocket to share data across process
    private IVideoEncoderCB mVideoEncoderCB = null;
    final ConditionVariable mOnStopLock = new ConditionVariable();
    DummyEncodeTransform()
    {
    }
    DummyEncodeTransform(Context context)
    {
        mContext = context;
    }
    protected void finalize ()  {
    }
    @Override
    public void setSink(IVideoSink sink)  throws FailedOperateException ,UnSupportException
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setSink");
        boolean restart = false;
        if (restart)
            stop();
        if(sink!=null && mWidth!=0 && mHeight!=0)
        {
            sink.setFormat(mOutFormat, mWidth, mHeight);
        }
        mVideoSink = sink;
        if (restart)
        {
            start();
        }
    }

    @Override
    public boolean onMedia(final ByteBuffer data,final int length,final long timestamp,final short rotation) 
    {
    	if (mVideoSink!=null)
        {
            try
            {
            	mVideoSink.onMedia(data, length, timestamp,rotation);
                if (StatisticAnalyzer.ENABLE)
                    StatisticAnalyzer.onEncodeSuccess();
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"onMediaNative", e);
            }
        }
    	return true;
    }

    @Override
    public void setFormat(int format, int width, int height)  throws FailedOperateException, UnSupportException 
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setFormat:"+format+" "+width+" "+height);
        switch(format)
        {
        case VideoFormat.NV16:
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setFormat NV16");
            break;
        
        case VideoFormat.NV21:
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setFormat NV21");
            break;
            
        default:
            throw new UnSupportException("Failed on setFormat: Not support video format type:"+format);
        }
        if (mInFormat!=format || mWidth!=width || mHeight!=height)
        {
            mInFormat = format;
            mWidth = width;
            mHeight = height;
            if (mVideoSink!=null)
            {
                //Reset Sink
                setSink(mVideoSink);
            }
        }
    }

    @Override
    public void start() throws FailedOperateException 
    {    
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>start");
        //synchronized to protect mHandle
    }

    @Override
    public void stop()
    {    
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>stop");
        //synchronized to protect mHandle
    }
    
    @Override
    public void setGop(final int gop) 
    {
        try
        {
            if (mVideoEncoderCB!=null)
            {
            	mVideoEncoderCB.setGop(gop);
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"setGop", e);
        }
    }
    @Override
    public void setBitRate(final int kbsBitRate) 
    {
        try
        {
        	if (mVideoEncoderCB!=null)
            {
            	mVideoEncoderCB.setBitRate(kbsBitRate);
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"setBitRate", e);
        }
    }
    @Override
    public void setFrameRate(byte fpsFramerate)
    {
        if (fpsFramerate>0)
            mGapMediaTime = (1000/fpsFramerate);
        try
        {
        	if (mVideoEncoderCB!=null)
            {
            	mVideoEncoderCB.setFrameRate(fpsFramerate);
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"setFrameRate", e);
        }
    }
    @Override
    public void requestIDR()
    {
        try
        {
        	if (mVideoEncoderCB!=null)
            {
            	mVideoEncoderCB.requestIDR();
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"requestIDR", e);
        }
    }


    @Override
    public void setLowBandwidth(boolean showLowBandwidth)
    {
        return ;
    }

    @Override
    public void release()
    {    
        mVideoSink = null;
        mContext = null;
    }
    
    @Override
    public void destory()
    {
        ;
    }
	@Override
	public void setCameraCallback(IVideoEncoderCB callback)
			throws FailedOperateException {
		mVideoEncoderCB = callback;
	}
}
