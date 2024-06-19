package com.quanta.qtalk.media.video;

import java.nio.ByteBuffer;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.media.ISink;

import android.graphics.PixelFormat;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class MjpegDecodeTransform implements IVideoTransform 
{
	public  final static String CODEC_NAME = "MJPEG";
	private final static String TAG = "MjpegDecodeTransform";
	private static final String DEBUGTAG = "QTFa_MjpegDecodeTransform";
	private IVideoSink mVideoSink = null;
	private int 	   mWidth = 0;
	private int 	   mHeight = 0;
	private int 	   mInFormat = 0;
	private final int  mOutFormat = PixelFormat.RGB_565;
	protected MjpegDecodeTransform(){}
	@Override
	public void setSink(IVideoSink sink)  throws FailedOperateException ,UnSupportException
	{
    	Log.d(DEBUGTAG, "setSink");
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
		//Native Layer decoder
		try
		{
			if (data!=null)
			{	//synchronized to protect mHandle
				if (mHandle!=0)
				{
					//Log.d(DEBUGTAG,"input size:"+data.capacity());
					_OnMedia(mHandle, data, length,timestamp,rotation);
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
	public void setFormat(int format, int width, int height)  throws FailedOperateException ,UnSupportException
	{
		//Define H264 Stream
		if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setFormat:"+format+" "+width+" "+height);
		switch(format)
		{
		case VideoFormat.MJPEG:
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
			{	//Reset Sink
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
				mHandle = _Start(mVideoSink, mWidth, mHeight);
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
	private static native int  _Start(final ISink sink,final  int width,final  int height);
	private static native void _Stop(final int handle);
	private static native void _OnMedia(final int handle,final  ByteBuffer input,final int length,final long timestamp,final short rotation);

    @Override
    public void setLowBandwidth(boolean showLowBandWidth)
    {
        if (mVideoSink != null)
        {
            mVideoSink.setLowBandwidth(showLowBandWidth);
        }
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
