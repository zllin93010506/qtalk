package com.quanta.qtalk.media.video;

import java.nio.ByteBuffer;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.media.ISink;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class X264Transform implements IVideoTransform, IVideoStreamSourceCB, IVideoEncoder
{
    public  final static String CODEC_NAME = "H264";
    public  final static int    DEFAULT_ID = 109;
    private final static String TAG = "X264Transform";
    private static final String DEBUGTAG = "QTFa_X264Transform";
    private final int  mOutFormat = VideoFormat.H264;

    private IVideoSink mVideoSink = null;
    private int        mWidth = 0;
    private int        mHeight = 0;
    private int        mInFormat = 0;
    private int        mGapMediaTime = 50;
    private long       mLastMediaTime = 0;
    private long       mGapMediaTimeBank = 0;
    private boolean    mFirstTime = false;
    X264Transform(){}
    @Override
    public void setSink(IVideoSink sink)  throws FailedOperateException ,UnSupportException
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setSink");
        boolean restart = false;
        if (mHandle!=0) restart = true;
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
    public boolean onMedia(final ByteBuffer data,final int length,final  long timestamp, final short rotation) 
    {
        try
        {
            if (mFirstTime)
            {
                mFirstTime = false;
                android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_BACKGROUND);
            }
            if (data!=null)
            {    //Frame rate control
                final long current = System.currentTimeMillis();
                final long current_gap = ((current-mLastMediaTime)+mGapMediaTimeBank)-mGapMediaTime; 
                if (current_gap>=0)
                {
                    mGapMediaTimeBank = current_gap;
                    //synchronized to protect mHandle
                    synchronized(this)
                    {
                        if (mHandle!=0 )
                        {
                            if (data.isDirect())
                                _OnMediaDirect(mHandle, data,length, timestamp,rotation);
                            else
                                _OnMedia(mHandle, data.array(),length, timestamp,rotation);
                        }
                    }
                    if (StatisticAnalyzer.ENABLE)
                        StatisticAnalyzer.onControlSuccess();
                    mLastMediaTime = current;
                    if (mGapMediaTimeBank>mGapMediaTime)
                        mGapMediaTimeBank = mGapMediaTime;
                }
                //else
                    //Log.e(DEBUGTAG, "FRANE RATE CONTROL : Drop frame");
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
    public void setFormat(final int format,final  int width,final  int height)  throws FailedOperateException, UnSupportException 
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
                mFirstTime = true;
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
    private static void onMediaNative(final IVideoSink sink,final ByteBuffer input,final int length,final long timestamp,final short rotation)
    {
        if (sink!=null)
        {
            try
            {
                sink.onMedia(input, length, timestamp,rotation);
                if (StatisticAnalyzer.ENABLE)
                    StatisticAnalyzer.onEncodeSuccess();
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"onMediaNative", e);
            }
        }
    }
    static
    {
        System.loadLibrary("media_x264_transform");
    }
    private int mHandle = 0;
    private static native int  _Start(ISink sink, int pixformat,int width, int height);
    private static native void _Stop(int handle);
    private static native void _OnMedia(final int handle,final byte[] input,final int length,final long timestamp,final short rotation);
    private static native void _OnMediaDirect(final int handle,final ByteBuffer input,final int length,final long timestamp,final short rotation);
    private static native void _SetGop(int handle,int gop);
    private static native void _SetBitRate(int handle,int kbsBitRate);
    
    @Override
    public void setGop(final int gop) 
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setGop:"+gop);
        try
        {
            if (mHandle!=0)
            {
                _SetGop(mHandle,gop);
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"setGop", e);
        }
    }
    @Override
    public void setBitRate(final int kbsBitRate) 
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setBitRate:"+kbsBitRate);
        try
        {
            if (mHandle!=0)
            {
                _SetBitRate(mHandle,kbsBitRate);
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"setBitRate", e);
        }
    }
    @Override
    public void setFrameRate(final byte fpsFramerate)
    {
        if (fpsFramerate>0)
            mGapMediaTime = (1000/fpsFramerate);
    }
    @Override
    public void requestIDR()
    {
        // TODO Auto-generated method stub
        
    }
    
    @Override
    public void setLowBandwidth(boolean showLowBandwidth)
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
	@Override
	public void setCameraCallback(IVideoEncoderCB callback)
			throws FailedOperateException {
		// TODO Auto-generated method stub
		
	}

}
