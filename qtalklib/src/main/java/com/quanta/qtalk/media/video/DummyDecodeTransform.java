package com.quanta.qtalk.media.video;

import java.nio.ByteBuffer;
import java.util.Vector;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;

import android.graphics.PixelFormat;
import android.os.ConditionVariable;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class DummyDecodeTransform implements IVideoTransform, IVideoSourcePollbased
{
    public  final static String CODEC_NAME = "H264";
    public  final static int    DEFAULT_ID = 109;
    private final static String TAG = "DummyDecodeTransform";
    private static final String DEBUGTAG = "QTFa_DummyDecodeTransform";
    private IVideoSink mVideoSink = null;
    private int        mWidth = 0;
    private int        mHeight = 0;
    private int        mInFormat = 0;
    private final int  mOutFormat = PixelFormat.RGB_565;
    private boolean    mIsFirst = false;
    private boolean    mIsStart = false;
    private final ConditionVariable  mOnNewData  = new ConditionVariable();
    private byte[]     mOutputBuffer = null;
    DummyDecodeTransform(){}
    @Override
    public void setSink(IVideoSink sink)  throws FailedOperateException ,UnSupportException
    {
        Log.d(DEBUGTAG, "setSink:"+mWidth+"x"+mHeight +" sink:"+sink);
        boolean restart = false;
        if (mIsStart)
        {
            restart = true;
            stop();
        }
        if(sink!=null && mWidth!=0 && mHeight!=0)
        {
        	Log.d(DEBUGTAG, "setSink:");
            sink.setFormat(mOutFormat, mWidth, mHeight);
        }
        mVideoSink = sink;
        if (restart)
        {
            start();
        }
    }

    @Override
    public boolean onMedia(ByteBuffer data,int length, long timestamp, short rotation) 
    {
    	
        boolean result = false;

        try
        {
            if (mIsStart && data!=null)
            {
                    synchronized(this)
                    {
                       result = mVideoSink.onMedia(data, length, timestamp,rotation);
                       if (StatisticAnalyzer.ENABLE)
                           StatisticAnalyzer.onDecodeSuccess();
                    }
            }else if (mIsStart)
            {
            	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"Failed on onMedia causes: input data is null");
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"onMedia", e);
        }
        return result;
    }

    @Override
    public void setFormat(int format, int width, int height)  throws FailedOperateException ,UnSupportException
    {
        //Define H264 Stream
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setFormat:"+format+" "+width+" "+height);
        switch(format)
        {
        case VideoFormat.H264:
            break;
        default:
            //throw new UnSupportException("Failed on setFormat: Not support video format type:"+format);
        }
        if (mInFormat!=format || mWidth!=width || mHeight!=height)
        {
            mInFormat = format;
            mWidth = width;
            mHeight = height;
            mOutputBuffer = new byte[width*height*2];
            synchronized(this)
            {    //Reset Sink
                setSink(mVideoSink);
            }
        }
    }

    @Override
    public void start() throws FailedOperateException 
    {    
        Log.d(DEBUGTAG, " ==>start");
        synchronized(this)
        {
            if (mVideoSink!=null && mHandle==0)
            {
                if(mWidth != 0 && mHeight != 0)
                 //   mHandle = _Start(mVideoSink, mWidth, mHeight);
                mIsFirst = false;
                mIsStart = true;
            }
        }
        Log.d(DEBUGTAG, " mIsStart="+mIsStart+", mHandle"+mHandle);
        //Log.d(DEBUGTAG, "<==start");
    }

    @Override
    public void stop()
    {    
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "stop");
    	//CameraSourcePlus.stop264Debug();
        mIsStart = false;
        mOnNewData.open();
        //synchronized to protect mHandle
        synchronized(this)
        {
            if (mHandle!=0)
            {
               // _Stop(mHandle);
                mHandle = 0;
            }
        }
    }
    private static void onMediaNative(final IVideoSink sink,final ByteBuffer input,final int length,final  long timestamp,final short rotation)
    {
        if (sink!=null)
        {
            try
            {
                sink.onMedia(input, length, timestamp,rotation);
                if (StatisticAnalyzer.ENABLE)
                    StatisticAnalyzer.onDecodeSuccess();
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"onMediaNative", e);
            }
        }
    }
    
    private int mHandle = 0;
    
    @Override
    public int getMedia(byte[] data, int length) throws UnSupportException
    {
        int result = 0;
        return result;
    }
    
    @Override
    public void setLowBandwidth(boolean showLowBandWidth)
    {
    	Log.d(DEBUGTAG,"StreamEngine mVideoSink:"+mVideoSink+", setLowBandwidth:"+showLowBandWidth);
        if (mVideoSink != null)
        {
            mVideoSink.setLowBandwidth(showLowBandWidth);
        }
    }

    @Override
    public void release()
    {
        mVideoSink = null;
        mOutputBuffer = null;
    }
    
    @Override
    public void destory()
    {
        ;
    }
}
