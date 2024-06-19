package org.ffmpeg.service;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Vector;

import android.net.LocalSocket;
import android.os.Bundle;
import android.os.ConditionVariable;
import android.os.Message;
import android.os.Messenger;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class FFMpegHandler
{
    private final static String DEBUGTAG = "QTFa_FFMpegService";
    static class H264Data
    {
        public byte[] buffer       = null;
        public int    bufferSize       = 0;
        public int    inputLength     = 0;
        public long   sampleTime      = 0;
        public int    ouputLength = 0;
        public short  rotation        = 0;
        public Messenger replyTo      = null;
    }
    static class H264DataDepository
    {
        final Vector<H264Data> mDataVector = new Vector<H264Data>();
        final ConditionVariable mOnDataReturn = new ConditionVariable();
        public H264DataDepository(int size, int width,int height)
        {
            for (int i=0;i<size;i++)
            {
                H264Data data = new H264Data();
                data.bufferSize = width*height*2;
                data.buffer = new byte[data.bufferSize];
                data.inputLength = 0;
                data.ouputLength = 0;
                mDataVector.add(data);
            }
        }
        
        public H264Data borrowData()
        {
            if (mDataVector.size()==0)
            {
                mOnDataReturn.block();
                mOnDataReturn.close();
            }
            if (mDataVector.size()>0)
                return mDataVector.remove(0);
            else
                return null;
        }
        public void returnData(H264Data data)
        {
            mDataVector.add(data);
            mOnDataReturn.open();
        }
    }
    //Thread to read the data from localsocket
    class ReadThread extends Thread 
    {
        private final Vector<H264Data> mInputVector = new Vector<H264Data>();
        private final ConditionVariable mOnNewData = new ConditionVariable();
        final ConditionVariable mOnStopLock = new ConditionVariable();
        boolean mExit = false;
        
        public void addData(H264Data data)
        {
            if (data!=null)
                mInputVector.add(data);
            mOnNewData.open();
        }
        @Override
        public void run()
        {
            DecodeThread decode_thread = new DecodeThread();
            decode_thread.start();
            mOnStopLock.close();
            while (!mExit || mInputVector.size()>0)
            {
                if (mInputVector.size()==0)
                {
                    mOnNewData.block();
                    mOnNewData.close();
                }
                if (mInputVector.size()>0)
                {
                    final H264Data data= (H264Data)mInputVector.remove(0);
                    try
                    {
                        int read_length = 0;
                        while(read_length<data.inputLength)
                        {
                            int tmp_length = mInputStream.read(data.buffer, read_length, data.inputLength-read_length);
                            if (tmp_length>=0)
                                read_length += tmp_length;
                            else
                                break;
                        }
                    }catch(Exception e)
                    {
                        Log.e(DEBUGTAG,"ReadThread",e);
                        mExit = true;
                    }finally
                    {
                        decode_thread.addData(data);
                    }
                }
            }
            decode_thread.mExit = true;
            decode_thread.addData(null);
            decode_thread.mOnStopLock.block();
            mOnStopLock.open();
            if (mInputStream!=null)
                try {
                    mInputStream.close();
                } catch (IOException e) {
                    Log.e(DEBUGTAG,"doDestroy",e);
                }
            if (mLocalSocketIn!=null)
                try {
                    mLocalSocketIn.close();
                } catch (IOException e) {
                    Log.e(DEBUGTAG,"doDestroy",e);
                }
        }
    }
    //Thread to decode the h264 data with ffmpeg
    class DecodeThread extends Thread 
    {
        private final Vector<H264Data> mInputVector = new Vector<H264Data>();
        private final ConditionVariable mOnNewData = new ConditionVariable();
        final ConditionVariable mOnStopLock = new ConditionVariable();
        boolean mExit = false;
        public void addData(H264Data data)
        {
            if (data!=null)
                mInputVector.add(data);
            mOnNewData.open();
        }
        @Override
        public void run()
        {
            //android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_DISPLAY);
            //android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_DISPLAY);
            android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_BACKGROUND);
            //this.setPriority(MAX_PRIORITY);
            ReplyThread reply_thread = new ReplyThread();
            reply_thread.start();
            mOnStopLock.close();
            while (!mExit || mInputVector.size()>0)
            {
                if (mInputVector.size()==0)
                {
                    //Log.e(TAG,"DecodeThread.block");
                    mOnNewData.block();
                    mOnNewData.close();
                }
                if (mInputVector.size()>0)
                {
                    //long start = System.currentTimeMillis();
                    final H264Data data= (H264Data)mInputVector.remove(0);
                    try
                    {
                        data.ouputLength = 0;
                        if(mHandle != 0)
                        {
                            data.ouputLength = _OnMedia(mHandle,data.buffer, data.inputLength,data.rotation);
                            if (COUNT_FRAME)
                            {
                                mCounter++;
                                if (mStartTime==0)
                                    mStartTime = System.currentTimeMillis();
                                else
                                    Log.d(DEBUGTAG,"MSG_ENCODE fps="+mCounter*1000/(System.currentTimeMillis()-mStartTime));
                            }
                        }
                    }catch(Exception e)
                    {
                        Log.e(DEBUGTAG,"DecodeThread",e);
                        mExit = true;;
                    }finally
                    {
                        reply_thread.addData(data);
                        //Log.d(TAG,"DecodeThread:"+(System.currentTimeMillis()-start));
                    }
                }
            }
            reply_thread.mExit = true;
            reply_thread.addData(null);
            reply_thread.mOnStopLock.block();
            mOnStopLock.open();
        }
    }
    class ReplyThread extends Thread
    {
        private final Vector<H264Data> mInputVector = new Vector<H264Data>();
        private final ConditionVariable mOnNewData = new ConditionVariable();
        final ConditionVariable mOnStopLock = new ConditionVariable();
        boolean mExit = false;
        public void addData(H264Data data)
        {
            if (data!=null)
                mInputVector.add(data);
            mOnNewData.open();
        }
        @Override
        public void run()
        {
            mOnStopLock.close();
            while (!mExit || mInputVector.size()>0)
            {
                if (mInputVector.size()==0)
                {
                    mOnNewData.block();
                    mOnNewData.close();
                }
                if (mInputVector.size()>0)
                {
                    final H264Data data= (H264Data)mInputVector.remove(0);
                    final Bundle bundle_output = new Bundle();
                    bundle_output.putInt(FFMpegServiceCommand.KEY_DATA_LENGTH, data.ouputLength);
                    bundle_output.putLong(FFMpegServiceCommand.KEY_SAMPLE_TIME, data.sampleTime);
                    bundle_output.putShort(FFMpegServiceCommand.KEY_ROTATION, data.rotation);
                    final Message msg_result = Message.obtain(null,FFMpegServiceCommand.MSG_DECODE, 0, 0);
                    msg_result.obj = bundle_output;
                    try
                    {
                        data.replyTo.send(msg_result);
                        if (mOutputStream!=null && data.ouputLength>0)
                        {
                            int write_length = 0;
                            while(write_length<data.ouputLength)
                            {
                                if (mLocalSocketOut.getSendBufferSize()<(data.ouputLength-write_length))
                                {
                                    mOutputStream.write(data.buffer, write_length , mLocalSocketOut.getSendBufferSize());
                                    write_length+=mLocalSocketOut.getSendBufferSize();
                                }
                                else
                                {
                                    mOutputStream.write(data.buffer, write_length , data.ouputLength-write_length);
                                    write_length+=(data.ouputLength-write_length);
                                }
                            }
                        }
                    }catch(Exception e)
                    {
                        Log.e(DEBUGTAG,"ReplyThread",e);
                        mExit = true;
                    }finally
                    {
                        mDataRepository.returnData(data);
                    }
                }
            }
            mOnStopLock.open();
            if (mOutputStream!=null)
                try {
                    mOutputStream.close();
                } catch (IOException e) {
                    Log.e(DEBUGTAG,"doDestroy",e);
                }
            if (mLocalSocketOut!=null)
                try {
                    mLocalSocketOut.close();
                } catch (IOException e) {
                    Log.e(DEBUGTAG,"doDestroy",e);
                }
        }
    }
    class DestoryThread extends Thread
    {
        public void run()
        {
            doDestroy();
        }
    }
    private void doDestroy()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>doDestroy:");
        try
        {
            mStopped = true;
            if (mReadThread!=null)
            {
                mReadThread.mExit = true;
                mReadThread.addData(null);//Wakeup the block
                mReadThread.mOnStopLock.block();
                mReadThread = null;
            }
            if(mHandle !=0)
            {
                _Stop(mHandle);
                mHandle = 0;
            }
            if (mInputStream!=null)
                try {
                    mInputStream.close();
                } catch (IOException e) {
                    Log.e(DEBUGTAG,"doDestroy",e);
                }
            if (mOutputStream!=null)
                try {
                    mOutputStream.close();
                } catch (IOException e) {
                    Log.e(DEBUGTAG,"doDestroy",e);
                }
            if (mLocalSocketIn!=null)
                try {
                    mLocalSocketIn.close();
                } catch (IOException e) {
                    Log.e(DEBUGTAG,"doDestroy",e);
                }
            if (mLocalSocketOut!=null)
                try {
                    mLocalSocketOut.close();
                } catch (IOException e) {
                    Log.e(DEBUGTAG,"doDestroy",e);
                }
            mLocalSocketIn = null;
            mLocalSocketOut = null;
            mInputStream = null;
            mOutputStream = null;
        }catch(Exception e)
        {
            Log.e(DEBUGTAG, "doDestroy",e);
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==doDestroy:");
    }
    //Frame rate count
    private static final boolean COUNT_FRAME = false;
    private long mStartTime = 0;
    private long mCounter = 0;
    //LocalSocket related File
    private LocalSocket     mLocalSocketOut    = null;
    private OutputStream    mOutputStream      = null;
    private LocalSocket     mLocalSocketIn     = null;
    private InputStream     mInputStream       = null;
    private boolean         mStopped           = true;
    private ReadThread      mReadThread        = null;

    private int             mFormat            = 0;
    private int             mWidth             = 0;
    private int             mHeight            = 0;
    private H264DataDepository mDataRepository = null;

    public FFMpegHandler(int format,int width,int height)
    {
        mFormat = format;
        mWidth  = width;
        mHeight = height;
        mDataRepository = new H264DataDepository(3,width,height);
        mHandle = _Start(mFormat,mWidth,mHeight);
    }
    public void setConnection(  LocalSocket localSocketIn,LocalSocket localSocketOut,InputStream inputStream ,OutputStream outputStream)
    {
        mLocalSocketIn = localSocketIn;
        mLocalSocketOut = localSocketOut;
        mInputStream = inputStream;
        mOutputStream = outputStream;
    }
    public void start()
    {
        mReadThread = new ReadThread();
        mReadThread.start();
    }
    public void stop()
    {
        (new DestoryThread()).start();
    }
    public void onMedia(final Messenger replyto,final int inputLength,final long sampleTime,final short rotation)
    {
        if (mInputStream!=null && inputLength>0)
        {
            H264Data data = null;
            while ((data = mDataRepository.borrowData())==null)
                ;
            data.inputLength = inputLength;
            data.ouputLength = 0;
            data.sampleTime = sampleTime;
            data.replyTo = replyto;
            data.rotation = rotation;
            mReadThread.addData(data);
        }
    }
    static
    {
        System.loadLibrary("media_ffmpeg_transform");
    }
    private int mHandle = 0;
    private static native int  _Start(final int format,final int width,final int height);
    private static native void _Stop(final int handle);
    private static native int _OnMedia(final int handle,final byte [] buffer,final int length,final short rotation);
}
