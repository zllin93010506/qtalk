package com.quanta.qtalk.media.video;

import java.nio.ByteBuffer;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class H264EncodeTransform implements IVideoTransform, IVideoStreamSourceCB
{
    public  final static String CODEC_NAME = "H264";
    public  final static int    DEFAULT_ID = 109;
    private final static String TAG = "H264EncodeTransform";
    private static final String DEBUGTAG = "QTFa_H264EncodeTransform";
    private final int  mOutFormat = VideoFormat.H264;

    
    private IVideoSink mVideoSink = null;
    private int        mWidth = 0;
    private int        mHeight = 0;
    private int        mInFormat = 0;
    H264EncodeTransform(){}
    @Override
    public void setSink(IVideoSink sink)  throws FailedOperateException ,UnSupportException
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setSink");
        boolean restart = false;
        if (mHandle!=0)
        {
            restart = true;
            stop();
        }
        
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
        try
        {
            if (data!=null)
            {
                if (mHandle!=0)
                {
                    _OnMedia(mHandle, data,length, timestamp, rotation);
                }
                else
                	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG, "Failed on onMedia causes:mHandle == null");
            }else
            {
            	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"Failed on onMedia causes: input data is null");
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"onMedia", e);
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
            synchronized(this)
            {    //Reset Sink
                setSink(mVideoSink);
            }
        }
    }

    @Override
    public void start() throws FailedOperateException 
    {    
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "start");
        //synchronized to protect mHandle
        synchronized(this)
        {
            if (mVideoSink!=null && mHandle==0)
            {
                mHandle = _Start(mVideoSink, mInFormat, mWidth, mHeight);
            }
        }
    }

    @Override
    public void stop()
    {    
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "stop");
        //synchronized to protect mHandle
        synchronized(this)
        {
            if (mHandle!=0)
            {
                _Stop(mHandle);
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
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"onMediaNative", e);
            }
        }
    }
    static
    {
        System.loadLibrary("media_ffmpeg_transform");
    }
    private int mHandle = 0;
    private static native int  _Start(final IVideoSink sink,final  int pixformat,final int width,final  int height);
    private static native void _Stop(final int handle);
    private static native void _OnMedia(final int handle,final  ByteBuffer input,final int length,final long timestamp,final short rotation);
    @Override
    public void setGop(int gop) {
        // TODO Auto-generated method stub
        
    }
    @Override
    public void setBitRate(int kbsBitRate) {
        // TODO Auto-generated method stub
        
    }
    @Override
    public void setFrameRate(byte fpsFramerate) {
        // TODO Auto-generated method stub
        
    }
    @Override
    public void requestIDR()
    {
        // TODO Auto-generated method stub
        
    }
    
    @Override
    public void setLowBandwidth(boolean showLowBandWidth)
    {
        return ;
    }

    @Override
    public void release()
    {
        ;
    }
    
    @Override
    public void destory()
    {
        ;
    }
}
