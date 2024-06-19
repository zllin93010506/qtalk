package com.quanta.qtalk.media.video;

import java.nio.ByteBuffer;
import java.util.Vector;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;

import android.graphics.PixelFormat;
import android.os.ConditionVariable;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class H264DecodeTransform implements IVideoTransform, IVideoSourcePollbased
{
    public  final static String CODEC_NAME = "H264";
    public  final static int    DEFAULT_ID = 109;
    private final static String TAG = "H264DecodeTransform";
    private static final String DEBUGTAG = "QTFa_H264DecodeTransform";
    private IVideoSink mVideoSink = null;
    private int        mWidth = 0;
    private int        mHeight = 0;
    private int        mInFormat = 0;
    private final int  mOutFormat = PixelFormat.RGB_565;
    private boolean    mIsFirst = false;
    private boolean    mIsStart = false;
    private final H264DataDepository mH264DataDepository = new H264DataDepository(10);
    private final Vector<H264Data>   mInputVector = new Vector<H264Data>();
    private final ConditionVariable  mOnNewData  = new ConditionVariable();
    private byte[]     mOutputBuffer = null;
    H264DecodeTransform(){}
    @Override
    public void setSink(IVideoSink sink)  throws FailedOperateException ,UnSupportException
    {
        //Log.d(DEBUGTAG, "setSink:"+mWidth+"x"+mHeight);
        boolean restart = false;
        if (mIsStart)
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
    public boolean onMedia(ByteBuffer data,int length, long timestamp, short rotation) 
    {
    	//Log.d(DEBUGTAG,"onMedia"+", mVideoSink="+mVideoSink
    	//		+",mIsStart ="+mIsStart+
    	//		", mHandle ="+mHandle
    	//		);
        int result = 0;
        //Log.d(DEBUGTAG, "==>onMedia:"+length+" "+timestamp+" "+rotation);
        try
        {
            if (mIsStart && data!=null)
            {
//                if (!mIsFirst)
//                {
//                    android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_BACKGROUND);
//                    mIsFirst = true;
//                }
                /*if (ENABLE_POLL_BASED)
                {   //Save data into queue
                    H264Data tmpData = mH264DataDepository.borrowData(500);
                    if (tmpData == null)
                    {   //Replace data
                        Log.e(DEBUGTAG,"Error mDepository is null, not data for borrow");
                        if (mInputVector.size()>0)
                            tmpData = mInputVector.remove(0);
                    }
                    if (tmpData!=null)
                    {
                        if (tmpData.size<length)
                        {
                            tmpData.buffer = new byte[length];
                            tmpData.size = length;
                        }
                        if (data.isDirect())
                        {
                            data.rewind();
                            data.get(tmpData.buffer, 0, length);
                        }else
                        {
                            System.arraycopy(data.array(), 0, tmpData.buffer, 0, length);
                        }
                        tmpData.length = length;
                        tmpData.sampleTime = timestamp;
                        tmpData.rotation = rotation;
                        mInputVector.add(tmpData);
                        mOnNewData.open();
                    }
                    if (null!=mVideoSink)
                    {   //Trigger the sink that new data coming
                        try
                        {
                            mVideoSink.onMedia((ByteBuffer)null, 0, timestamp,rotation);
                            if (StatisticAnalyzer.ENABLE)
                                StatisticAnalyzer.onDecodeSuccess();
                        }catch(java.lang.Throwable e)
                        {
                            Log.e(DEBUGTAG,"onMedia", e);
                        }
                    }
                }else
                {*/
                    //Run decoding process
                    synchronized(this)
                    {
                        if (mHandle!=0)
                        {
                            if (data.isDirect())
                            {
                               result = _OnMedia(mHandle, data, length,timestamp, rotation);  
                            }else
                            {
                               result = _Decode(mHandle,data.array(),length,mOutputBuffer,mOutputBuffer.length);
                               if (mVideoSink!=null)
                               {
                                   try
                                   {
                                       mVideoSink.onMedia(ByteBuffer.wrap(mOutputBuffer), result, timestamp,rotation);
                                       if (StatisticAnalyzer.ENABLE)
                                           StatisticAnalyzer.onDecodeSuccess();
                                   }catch(java.lang.Throwable e)
                                   {
                                       Log.e(DEBUGTAG,"onMediaNative", e);
                                   }
                               }
                            }
                        }
                        else
                        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG, "Failed on onMedia causes:mHandle == null");
                    }
                //}
            }else if (mIsStart)
            {
            	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"Failed on onMedia causes: input data is null");
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"onMedia", e);
        }
        //Log.d(DEBUGTAG, "<==onMedia: queue size:"+mInputVector.size());
        return result>0;
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
            throw new UnSupportException("Failed on setFormat: Not support video format type:"+format);
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
        //synchronized to protect mHandle
        synchronized(this)
        {
            if (mVideoSink!=null && mHandle==0)
            {
                //Log.d(DEBUGTAG, "start:"+mWidth+"x"+mHeight);
                if(mWidth != 0 && mHeight != 0)
                    mHandle = _Start(mVideoSink, mWidth, mHeight);
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
        mIsStart = false;
        mOnNewData.open();
        //synchronized to protect mHandle
        synchronized(this)
        {
            if (mHandle!=0)
            {
                _Stop(mHandle);
                mHandle = 0;
            }
            while(mInputVector.size()>0)
                mH264DataDepository.returnData(mInputVector.remove(0));
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
    static
    {
        System.loadLibrary("media_ffmpeg_transform");
    }
    private int mHandle = 0;
    private static native int  _Start(final IVideoSink sink,final int width,final int height);
    private static native void _Stop(final int handle);
    private static native int  _OnMedia(final int handle,final ByteBuffer input,final int length,final long timestamp,final short rotation);

    private static native int  _Decode(final int handle,final byte[] input,final int length,byte[] output,final int outputLength);
    //===========================Poll-based implementation
    static class H264Data
    {
        public byte[] buffer       = null;
        public int    length       = 0;
        public int    size         = 0;
        public long   sampleTime   = 0;
        public short  rotation     = 0;
    }
    static class H264DataDepository
    {
        final Vector<H264Data> mDataVector = new Vector<H264Data>();
        final ConditionVariable mOnDataReturn = new ConditionVariable();
        public H264DataDepository(int size)
        {
            for (int i=0;i<size;i++)
            {
                H264Data data = new H264Data();
                data.length = 0;
                data.size = 0;
                data.sampleTime = 0;
                data.rotation = 0;
                mDataVector.add(data);
            }
        }
        
        public H264Data borrowData(int timeout)
        {
            if (mDataVector.size()==0)
            {
                mOnDataReturn.block(timeout);
                mOnDataReturn.close();
            }
            if (mDataVector.size()>0)
                return mDataVector.remove(0);
            else
                return null;
        }
        public void returnData(H264Data data)
        {
            mDataVector.insertElementAt(data, 0);
            mOnDataReturn.open();
        }
    }
    @Override
    public int getMedia(byte[] data, int length) throws UnSupportException
    {
        //Log.d(DEBUGTAG, "==>getMedia:"+mInputVector.size());
        int result = 0;
        try
        {
            if (mIsStart && mHandle!=0)
            {
                if (mInputVector.size()==0)
                {
                    mOnNewData.block(50);
                    mOnNewData.close();
                }
                if (mInputVector.size()>0)
                {
                    synchronized(this)
                    {
                        if (mIsStart && mHandle!=0)
                        {
                            H264Data tmpData = mInputVector.remove(0);
                            result = _Decode(mHandle,tmpData.buffer,tmpData.length,data,length);
                            mH264DataDepository.returnData(tmpData);
                        }
                    }
                }
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"getMedia", e);
        }

        //Log.d(DEBUGTAG, "<==getMedia:"+result+" "+timestamp+" "+rotation);
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
