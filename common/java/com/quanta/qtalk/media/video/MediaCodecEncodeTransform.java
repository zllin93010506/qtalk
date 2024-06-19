package com.quanta.qtalk.media.video;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;

import org.x264.service.X264ServiceCommand;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;

import android.content.Context;
import android.os.Bundle;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.Message;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class MediaCodecEncodeTransform implements IVideoTransform, IVideoStreamSourceCB, IVideoEncoder
{
    public  final static String  CODEC_NAME      = "H264";
    public  final static int     DEFAULT_ID      = 109;
    public  final static int     MSG_ENCODE_COMPLETE     = 0x00000000;
    public  final static String  KEY_GET_FRAME       = "KEY_GET_FRAME" ;
    public  final static String  KEY_GET_LENGTH      = "KEY_GET_LENGTH";
    public  final static String  KEY_GET_TIMESTAMP   = "KEY_GET_TIMESTAMP";
    public  final static String  KEY_GET_ROTATION   = "KEY_GET_ROTATION";
    
    private static final String DEBUGTAG = "QTFa_MediaCodecEncodeTransform";
    private final int  mOutFormat = VideoFormat.H264;
    
    private IVideoSink mVideoSink = null;
    private int        mWidth = 0;
    private int        mHeight = 0;
    private int        mInFormat = 0;
    private Context    mContext = null;
    private boolean   restart = false;
    private boolean   bypass = false;
    
    //private MediaCodecEncoder mEncoder =null;
    private VideoEncoderFromBuffer mEncoder = null;
    
    
    private Handler mHandler = new Handler(){
    	
    	public void handleMessage(Message msg) {
    		switch(msg.what){
    			case MediaCodecEncodeTransform.MSG_ENCODE_COMPLETE:
    				Bundle bundle = msg.getData();
    				byte[] data =    bundle.getByteArray(KEY_GET_FRAME);
       				int length =     bundle.getInt(KEY_GET_LENGTH);
    				long timestamp = bundle.getLong(KEY_GET_TIMESTAMP);
    				short rotation = bundle.getShort(KEY_GET_ROTATION);
    				ByteBuffer input = ByteBuffer.wrap(data);
    				onMediaCallback(mVideoSink, input, length, timestamp, rotation);
    				break;
    			default:
    				super.handleMessage(msg);
    		}
    		
    	};
    };
    
    //LocalSocket to share data across process
    final ConditionVariable mOnStopLock = new ConditionVariable();
    MediaCodecEncodeTransform()
    {
    }
    MediaCodecEncodeTransform(Context context)
    {
        mContext = context;
    }
    protected void finalize ()  {
    }
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
    	if (mVideoSink!=null)
        {
            try
            {
            	if(bypass){
            		onMediaCallback(mVideoSink, data, length, timestamp, rotation);
            		return true;
            	}
            	byte[] clonedata = new byte[data.remaining()];
            	data.get(clonedata);
                //mEncoder.Write(clonedata, length, timestamp, rotation);
            	mEncoder.encodeFrame(clonedata);
            	if (StatisticAnalyzer.ENABLE)
                    StatisticAnalyzer.onEncodeSuccess();
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"onMedia", e);
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
            restart = true;
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
    	if(mEncoder==null){
    		//mEncoder = new MediaCodecEncoder(mHandler);
    		try {
				mEncoder = new VideoEncoderFromBuffer(mHandler,mWidth,mHeight);
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				Log.e(DEBUGTAG,"",e);
			}

    	}
//    	try {
////			mEncoder.Config(mWidth, mHeight, 30, 20000000, 10);
//			mEncoder.Config(mWidth, mHeight, 15, 1000000, 5);
//		} catch (IOException e) {
//			// TODO Auto-generated catch block
//			Log.e(DEBUGTAG,"",e);
//		}
    }
    

    @Override
    public void stop()
    {    
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>stop");
    	if(mEncoder!=null){
    		//mEncoder.Stop();
    		mEncoder.close();
    		mEncoder = null;
    	}
    }
    
    @Override
    public void setGop(final int gop) 
    {
    	Log.d(DEBUGTAG,"setGop:"+gop);
        if(mEncoder!=null){
        	mEncoder.setGop(gop);
        }
    }
    @Override
    public void setBitRate(final int kbsBitRate) 
    {
    	Log.d(DEBUGTAG,"setBitRate:"+kbsBitRate);
        if(mEncoder!=null){
        	mEncoder.setBitRate(kbsBitRate);
        }
    }
    @Override
    public void setFrameRate(byte fpsFramerate)
    {
    	Log.d(DEBUGTAG,"setFrameRate:"+fpsFramerate);
        if(mEncoder!=null){
        	mEncoder.setFrameRate(fpsFramerate);
        }
    }
    @Override
    public void requestIDR()
    {
    	Log.d(DEBUGTAG,"requestIDR");
        if(mEncoder!=null){
        	mEncoder.requestIDR();
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
	}
    private static void onMediaCallback(final IVideoSink sink,final ByteBuffer input,final int length,final long timestamp,final short rotation)
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
    public void setMute(boolean mute){
    	bypass = mute;
    }
}
