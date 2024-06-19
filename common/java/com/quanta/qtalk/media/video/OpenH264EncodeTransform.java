package com.quanta.qtalk.media.video;


import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.media.ISink;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class OpenH264EncodeTransform implements IVideoTransform, IVideoStreamSourceCB
{
    private static final String DEBUGTAG = "QTFa_OpenH264EncodeTransform";
    private final int  mOutFormat = VideoFormat.MJPEG;
    
    private IVideoSink mVideoSink = null;
    private int        mWidth = 0;
    private int        mHeight = 0;
    private int        mInFormat = 0;
    private int        mFrameRate = 30;
    private int        mBitRate = 2000;
    private boolean    restart    = false;
    private boolean    bypass     = false;
    private long       startTime = 0;
    private int        frameCnt = 0;
    private int        TIMEGAP = 3000;
    protected          OpenH264EncodeTransform(){}

    @Override
    public void setSink(IVideoSink sink)  throws FailedOperateException ,UnSupportException
    {
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setSink");
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
        restart = false;
    }
    

    @Override
    public boolean onMedia(final ByteBuffer data,final int length,final long timestamp,final short rotation) 
    {
        try
        {
            if (data!=null)
            {    //synchronized to protect mHandle
                if (mHandle!=0)
                {
                	if(bypass){
                		onMediaNative(mVideoSink, data, length, timestamp, rotation);
                		return true;
                	}
                    frameCnt++;

                	long nowTime = System.currentTimeMillis();
                	if(startTime == 0){
                		startTime = nowTime;
                	}
                    
                    long diff = nowTime-startTime;
                    if(diff >= TIMEGAP){                		
                		adjustFrameRate(diff, frameCnt);
                        startTime = nowTime;
                		frameCnt = 0;
                	}    	
                	
                	byte[] n21_data = new byte[data.remaining()];
                	data.get(n21_data);
                	//byte[] yuv420_data = new byte[n21_data.length];
                	//nv21_to_yuv420(n21_data, yuv420_data, mHeight, mWidth);
                    //_OnMedia(mHandle, yuv420_data, yuv420_data.length, timestamp,rotation);

                    _OnMedia(mHandle, n21_data, n21_data.length, timestamp,rotation);
                }
                else
                	Log.e(DEBUGTAG, "Failed on onMedia causes:mHandle == null");
            }else
            {
            	Log.e(DEBUGTAG,"Failed on onMedia causes: input data is null");
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
            restart = true;
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
                mHandle = _Start(mVideoSink, mInFormat, mWidth, mHeight, mFrameRate, mBitRate);
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
            try {
                sink.onMedia(input, length, timestamp,rotation);
            }
            catch(java.lang.Throwable e) {
                Log.e(DEBUGTAG,"onMediaNative", e);
            }
        }
    }


	/***************************************************************************
	 *  				Public function implement
	 **************************************************************************/    
    static
    {
        System.loadLibrary("media_openh264_transform");
    }
    private int mHandle = 0;
    private static native int  _Start(final ISink sink,final int pixformat,final int width,final int height, final int framerate, int bitrate);
    private static native void _Stop(final int handle);
    private static native void _OnMedia(final int handle,final byte[] input,final int length,final long timestamp,final short rotation);
    private static native void _SetGop(final int gop);
    private static native void _SetBitRate(final int kbsBitRate);
    private static native void _SetFrameRate(final int fpsFrameRate);
    private static native void _RequestIDR();
    
    @Override
    public void setGop(int gop) {
    	Log.d(DEBUGTAG,"setGop "+gop);
    	_SetGop(gop);
    }
    @Override
    public void setBitRate(int kbsBitRate) {
    	Log.d(DEBUGTAG,"setBitRate "+kbsBitRate);
    	mBitRate = kbsBitRate;
    	_SetBitRate(kbsBitRate);
    }
    @Override
    public void setFrameRate(byte fpsFramerate) {
    	Log.d(DEBUGTAG,"setFrameRate "+fpsFramerate);
    	_SetFrameRate(fpsFramerate);
    }
    @Override
    public void requestIDR()
    {
    	Log.d(DEBUGTAG,"requestIDR");
    	_RequestIDR();
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
    
    public void setMute(boolean mute){
    	if(bypass != mute) {
    		requestIDR();
    	}
    	bypass = mute;
    }
    
    private void adjustFrameRate(long diff, int cnt) {
    	int frameRate = cnt/(int)(diff/1000) + (int)(1);
        if(frameRate>30) frameRate = 30;

    	//Log.d(DEBUGTAG,"adjust_fps: "+frameRate+"fps (cnt:"+cnt+", diff:"+diff+") (fk)");
        if(frameRate > 0) {
    	    _SetFrameRate(frameRate);
        }
    }

	private static void nv21_to_yuv420(byte[] n21_data, byte[] yuv420_data,
			int height, int width) {
		int y_plane_size = width * height;
		int u_plane_offset = width * height;
		int v_plane_offset = u_plane_offset + y_plane_size / 4;

		System.arraycopy(n21_data, 0, yuv420_data, 0, y_plane_size);

		for (int i = 0; i < y_plane_size / 4;) {
			yuv420_data[i + u_plane_offset] = n21_data[y_plane_size + 2 * i + 1];
			yuv420_data[i + 1 + u_plane_offset] = n21_data[y_plane_size + 2 * i
					+ 3];
			yuv420_data[i + 2 + u_plane_offset] = n21_data[y_plane_size + 2 * i
					+ 5];
			yuv420_data[i + 3 + u_plane_offset] = n21_data[y_plane_size + 2 * i
					+ 7];
			yuv420_data[i + 4 + u_plane_offset] = n21_data[y_plane_size + 2 * i
					+ 9];
			yuv420_data[i + 5 + u_plane_offset] = n21_data[y_plane_size + 2 * i
					+ 11];
			yuv420_data[i + 6 + u_plane_offset] = n21_data[y_plane_size + 2 * i
					+ 13];
			yuv420_data[i + 7 + u_plane_offset] = n21_data[y_plane_size + 2 * i
					+ 15];

			yuv420_data[i + v_plane_offset] = n21_data[y_plane_size + 2 * i];
			yuv420_data[i + 1 + v_plane_offset] = n21_data[y_plane_size + 2 * i
					+ 2];
			yuv420_data[i + 2 + v_plane_offset] = n21_data[y_plane_size + 2 * i
					+ 4];
			yuv420_data[i + 3 + v_plane_offset] = n21_data[y_plane_size + 2 * i
					+ 6];
			yuv420_data[i + 4 + v_plane_offset] = n21_data[y_plane_size + 2 * i
					+ 8];
			yuv420_data[i + 5 + v_plane_offset] = n21_data[y_plane_size + 2 * i
					+ 10];
			yuv420_data[i + 6 + v_plane_offset] = n21_data[y_plane_size + 2 * i
					+ 12];
			yuv420_data[i + 7 + v_plane_offset] = n21_data[y_plane_size + 2 * i
					+ 14];

			i += 8;
		}
}
}
