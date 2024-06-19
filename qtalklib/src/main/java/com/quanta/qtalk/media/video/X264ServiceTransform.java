package com.quanta.qtalk.media.video;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;

import org.x264.service.X264ServiceCommand;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.media.ISink;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Bundle;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class X264ServiceTransform implements IVideoTransform, IVideoStreamSourceCB, IVideoEncoder
{
    private final static boolean USE_LOCALSOCKET = true;
    public  final static String  CODEC_NAME      = "H264";
    public  final static int     DEFAULT_ID      = 109;
    private final static String  TAG             = "X264ServiceTransform";
    private static final String DEBUGTAG = "QTFa_X264ServiceTransform";
    private final int  mOutFormat = VideoFormat.H264;
    
    private IVideoSink mVideoSink = null;
    private int        mWidth = 0;
    private int        mHeight = 0;
    private int        mInFormat = 0;
    private int        mGapMediaTime = 100;
    private long       mLastMediaTime = 0;
    private long       mGapMediaTimeBank = 0;
    private int        mCounter = 0;
    private final Object mCounterLock = new Object();
    private Context    mContext = null;
    private boolean    mStopped = true;
    //LocalSocket to share data across process
    private static final String mLocalSocketName   = "com.quanta.qtalk.media.video.X264ServiceTransform";
    private LocalSocket         mLocalSocketOut    = null;
    private OutputStream        mOutputStream      = null;
    private LocalSocket         mLocalSocketIn     = null;
    private InputStream         mInputStream       = null;
    private byte[]              mLocalSocketBitStream = null;
    final ConditionVariable mOnStopLock = new ConditionVariable();
    X264ServiceTransform(Context context)
    {
        mContext = context;
        doBindService(mContext);
    }
    protected void finalize ()  {
        doUnbindService(mContext);
    }
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
    public boolean onMedia(final ByteBuffer data,final int length,final long timestamp,final short rotation) 
    {
        try
        {
            if (!mStopped && mHandle!=0 && data!=null && length>0)
            {
                //Log.d(DEBUGTAG, "==>onMedia:"+mStopped);
                if (mCounter>1)
                {
                    if (mCounterChanged!=null)
                    {
                        mCounterChanged.block(100);
                        mCounterChanged.close();
                    }
                }

                if (mCounter <= 1)
                {
                    boolean result = false;
                    //Frame rate control
                    long current = System.currentTimeMillis();
                    long left_time = (mLastMediaTime+mGapMediaTime-mGapMediaTimeBank) - (current);//mGapMediaTimeBank 
                    if (left_time<=5)
                    {
                        synchronized(this)
                        {
                            if (!mStopped && mHandle!=0)
                            {
                                result = _OnMedia(mHandle, data,length, timestamp, rotation);
                            }
                        }
                        if (StatisticAnalyzer.ENABLE)
                            StatisticAnalyzer.onControlSuccess();
                        mLastMediaTime = current;
                        if (left_time<0)
                            mGapMediaTimeBank = Math.abs(left_time);
                        else
                            mGapMediaTimeBank = 0;
                        if (mGapMediaTimeBank>mGapMediaTime)
                            mGapMediaTimeBank = mGapMediaTime;
                        long next_media_time = current+mGapMediaTime-System.currentTimeMillis()-mGapMediaTimeBank;
                        if (next_media_time>1){
                            Thread.sleep((next_media_time*4/5)+1);
                        }
                    }
                    else
                    {
                        Thread.sleep((left_time*4/5)+1);
                    }

                    if (result)
                    {
                        synchronized(mCounterLock)
                        {
                            ++mCounter;
                        }
                    }
                   // Log.d(DEBUGTAG, "<==onMedia:");
                }

            }else if (!mStopped && mHandle!=0)
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
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>start:"+mStopped);
        //synchronized to protect mHandle
        synchronized(this)
        {
            if (mVideoSink!=null && mHandle==0)
            {
                try {
                    mHandle = _Start(mVideoSink, mInFormat, mWidth, mHeight);
                    mCounter = 0;
                } catch (Exception e) {
                    throw new FailedOperateException(e);
                }
                mStopped = mHandle!=0?false:true;
            }
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==start:"+mStopped);
    }

    @Override
    public void stop()
    {    
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>stop");
        //synchronized to protect mHandle
        mStopped = true;
        mCounter = 0;
        if (mHandle!=0 && mService!=null)
        {
            mOnStopLock.close();
            synchronized(this)
            {
                try
                {
                    _Stop(mHandle);
                    mHandle = 0;
                }catch(java.lang.Throwable e)
                {
                    Log.e(DEBUGTAG,"stop", e);
                }
            }
            mOnStopLock.block(1000);
            _Destory();
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==stop");
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
    
    @Override
    public void setGop(final int gop) 
    {
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
    public void setFrameRate(byte fpsFramerate)
    {
        if (fpsFramerate>0)
            mGapMediaTime = (1000/fpsFramerate);
        try
        {
            if (mHandle!=0)
            {
                _SetFramerate(mHandle,fpsFramerate);
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
            if (mHandle!=0)
            {
                _requestIDR(mHandle);
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"requestIDR", e);
        }
    }
    
    //===================================Service
    /** Messenger for communicating with service. */
    private Messenger mService = null;
    /** Flag indicating whether we have called bind on the service. */
    private boolean mIsBound;
    private ConditionVariable mCounterChanged   = null;
    /**
     * Handler of incoming messages from service.
     */
    class IncomingHandler extends Handler 
    {
        @Override
        public void handleMessage(final Message msg) 
        {
            //Log.d(DEBUGTAG,"==>handleMessage:"+msg.what);
            Bundle bundle_input = null;
            if (msg.obj!=null)
                bundle_input = (Bundle)msg.obj;
            switch (msg.what)
            {
                case X264ServiceCommand.MSG_START:
                	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>MSG_START:"+msg.arg1);
                    if (USE_LOCALSOCKET)
                    {
                        final String socket_name = bundle_input.getString(X264ServiceCommand.KEY_LOCAL_SOCKET_NAME);
                        if (socket_name!=null)
                        {
                            new Thread(new Runnable() 
                            {
                                public void run() 
                                {
                                	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>mLocalSocketIn.run:"+socket_name);
                                    mLocalSocketIn = new LocalSocket();
                                    try
                                    {
                                        mLocalSocketIn.connect(new LocalSocketAddress(socket_name));
                                        if (mLocalSocketIn.isConnected())
                                            mInputStream = mLocalSocketIn.getInputStream();
                                        else
                                        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"Failed on build connection of mLocalSocketIn !");
                                    } catch (IOException e)
                                    {
                                        Log.e(DEBUGTAG,"LocalSocketIn.connect",e);
                                    }
                                    if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==mLocalSocketIn.run:"+mLocalSocketIn.isConnected());
                                }
                            }).start();
                            
                        }
                    }
                    mCounterChanged = new ConditionVariable();
                    mCounterChanged.close();
                    if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==MSG_START");
                    break;
                case X264ServiceCommand.MSG_STOP:
                	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"MSG_STOP");
                    mOnStopLock.open();
                    break;
                case X264ServiceCommand.MSG_SET_GOP:
                	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"MSG_SET_GOP");
                    break;
                case X264ServiceCommand.MSG_SET_BITRATE:
                	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"MSG_SET_BITRATE");
                    break;
                case X264ServiceCommand.MSG_ENCODE:
                    //Log.d(DEBUGTAG,"MSG_ENCODE");
                    synchronized(mCounterLock)
                    {
                        --mCounter;
                    }
                    if (mCounterChanged!=null)
                    {
                        mCounterChanged.open();
                    }
                    if (bundle_input!=null)
                    {
                        final int    length = bundle_input.getInt(X264ServiceCommand.KEY_BITSTREAM_LENGTH);
                        final long   sample_time  = bundle_input.getLong(X264ServiceCommand.KEY_SAMPLE_TIME);
                        final short  rotation  = bundle_input.getShort(X264ServiceCommand.KEY_ROTATION);
                        //Log.d(DEBUGTAG,"encode gap:"+(System.currentTimeMillis()-sample_time));
                        if (length>0)
                        {
                            try
                            {
                                 ByteBuffer bitstream = null;
                                 if (mInputStream!=null)
                                 {
                                     if (mLocalSocketBitStream==null || length>mLocalSocketBitStream.length)
                                         mLocalSocketBitStream = new byte[length];
                                     int read_length = 0;
                                     while(read_length<length)
                                     {
                                         int tmp_length = mInputStream.read(mLocalSocketBitStream, read_length, length-read_length);
                                         if (tmp_length>=0)
                                             read_length += tmp_length;
                                         else
                                             break;
                                     }
                                     
                                     if (read_length==length)
                                         bitstream = ByteBuffer.wrap(mLocalSocketBitStream);
                                     else
                                    	 if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"Failed on LocalSocket.readBytes");
                                 }
                                 else if (bundle_input.getByteArray(X264ServiceCommand.KEY_BITSTREAM)!=null)
                                 {
                                     bitstream = ByteBuffer.wrap(bundle_input.getByteArray(X264ServiceCommand.KEY_BITSTREAM));
                                 }
                                 if (!mStopped && bitstream!=null)
                                     onMediaNative(mVideoSink,
                                                    bitstream,
                                                    length,
                                                    sample_time,
                                                    rotation);
                            }catch(Exception e)
                            {
                                Log.e(DEBUGTAG,"handleMessage",e);
                            }
                        }
                    }
                    break;
                default:
                    super.handleMessage(msg);
            }
            //Log.d(DEBUGTAG,"<==handleMessage:"+msg.what);
        }
    }

    /**
     * Target we publish for clients to send messages to IncomingHandler.
     */
    final Messenger mMessenger = new Messenger(new IncomingHandler());

    /**
     * Class for interacting with the main interface of the service.
     */
    private ServiceConnection mConnection = new ServiceConnection() 
    {
        public void onServiceConnected(ComponentName className, IBinder service) 
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onServiceConnected");
            // This is called when the connection with the service has been
            // established, giving us the service object we can use to
            // interact with the service.  We are communicating with our
            // service through an IDL interface, so get a client-side
            // representation of that from the raw service object.
            mService = new Messenger(service);
            // We want to monitor the service for as long as we are
            // connected to it.
            try 
            {
                start();
            } catch (Exception e)
            {
                Log.e(DEBUGTAG,"onServiceConnected",e);
                // In this case the service has crashed before we could even
                // do anything with it; we can count on soon being
                // disconnected (and then reconnected if it can be restarted)
                // so there is no need to do anything here.
            }
        }

        public void onServiceDisconnected(ComponentName className) 
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onServiceDisconnected");
            // This is called when the connection with the service has been
            // unexpectedly disconnected -- that is, its process crashed.
            mService = null;
        }
    };

    void doBindService(Context context) 
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>doBindService");
        // Establish a connection with the service.  We use an explicit
        // class name because there is no reason to be able to let other
        // applications replace our component.
        if (!mIsBound && context!=null && mService==null)
        {
            //doUnbindService(context);
            try
            {
                context.bindService(new Intent(X264ServiceCommand.SERVICE_ACTION_NAME), mConnection, Context.BIND_AUTO_CREATE);
                mIsBound = true;
            }catch(Exception e)
            {
                Log.e(DEBUGTAG,"bindService",e);
            }
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==doBindService");
    }

    void doUnbindService(Context context) 
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>doUnbindService");
        if (mIsBound) 
        {
            // If we have received the service, and hence registered with
            // it, then now is the time to unregister.
            if (mService != null) 
            {
                try 
                {
                	if(!mStopped)
                		stop();
                } catch (Exception e)
                {
                    Log.e(DEBUGTAG,"doUnbindService",e);
                }
            }
            if (context!=null)
            {
                // Detach our existing connection.
                try 
                {
                    context.unbindService(mConnection);
                    context.stopService(new Intent(X264ServiceCommand.SERVICE_ACTION_NAME));
                } catch (Throwable e)
                {
                    Log.e(DEBUGTAG,"doUnbindService",e);
                }
            }
            mIsBound = false;
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==doUnbindService");
    }
    private int mHandle = 0;
    private int  _Start(final ISink sink,final  int pixformat,final int width,final  int height) throws Exception
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>_Start:"+USE_LOCALSOCKET);
        int handle = 0;
        try 
        {
            if (mService!=null)
            {
                final Message msg = Message.obtain(null,X264ServiceCommand.MSG_START,0,0);
                msg.replyTo = mMessenger;
                final Bundle bundle = new Bundle();
//                final ConditionVariable socketDone = new ConditionVariable();
//                socketDone.close();
                if (USE_LOCALSOCKET)
                {
                    final String socket_name = mLocalSocketName+System.currentTimeMillis();
                    bundle.putString(X264ServiceCommand.KEY_LOCAL_SOCKET_NAME,socket_name );
                    final LocalServerSocket tmpServer = new LocalServerSocket(socket_name);
                    new Thread(new Runnable() 
                    {
                        public void run() 
                        {
                        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>mLocalSocketOut.run:"+socket_name);
                            try
                            {
                                if (tmpServer!=null)
                                {   //Wait for socket connection from service 
                                    mLocalSocketOut = tmpServer.accept();
                                    //tmpServer.close();
                                    if (mLocalSocketOut!=null)
                                    {
                                        mOutputStream = mLocalSocketOut.getOutputStream();
                                    }
                                    else
                                        Log.e(DEBUGTAG,"Failed on initialize LocalSocketOut");
                                    tmpServer.close();
                                }
                            }catch (IOException e)
                            {
                                Log.e(DEBUGTAG,"LocalServerSocket",e);
                            }finally
                            {
//                                socketDone.open();
                            }
                            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==mLocalSocketOut.run:mLocalSocketOut="+mLocalSocketOut);
                        }
                    }).start();
                }
                bundle.putInt(X264ServiceCommand.KEY_FRAME_WIDTH, width);
                bundle.putInt(X264ServiceCommand.KEY_FRAME_HEIGHT, height);
                bundle.putInt(X264ServiceCommand.KEY_FRAME_FORMAT, pixformat);
                msg.obj = bundle;
                mService.send(msg);
                handle = 1;
            }
        } catch (Exception e)
        {
            Log.e(DEBUGTAG,"_Start",e);
            throw e;
            // In this case the service has crashed before we could even
            // do anything with it; we can count on soon being
            // disconnected (and then reconnected if it can be restarted)
            // so there is no need to do anything here.
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==_Start:"+handle);
        return handle;
    }
    private void _Stop(int handle)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>_Stop");
        try
        {
            if (handle!=0)
            {
                if (mService!=null)
                {
                    final Message msg = Message.obtain(null,X264ServiceCommand.MSG_STOP,0,0);
                    msg.replyTo = mMessenger;
                    mService.send(msg);
                }
            }
        } catch (Exception e)
        {
            Log.e(DEBUGTAG,"_Stop",e);
            // In this case the service has crashed before we could even
            // do anything with it; we can count on soon being
            // disconnected (and then reconnected if it can be restarted)
            // so there is no need to do anything here.
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==_Stop");
    }
    private void _Destory()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>_Destory");
        try
        {
//            if (mInputStream!=null)
//                mInputStream.close();
//            if (mOutputStream!=null)
//                mOutputStream.close();
//            if (mLocalSocketIn!=null)
//                mLocalSocketIn.close();
//            if (mLocalSocketOut!=null)
//                mLocalSocketOut.close();
            finalize();
            mLocalSocketOut = null;
            mLocalSocketIn = null;
            mOutputStream = null;
            mInputStream = null;
        } catch (Exception e)
        {
            Log.e(DEBUGTAG,"_Destory",e);
            // In this case the service has crashed before we could even
            // do anything with it; we can count on soon being
            // disconnected (and then reconnected if it can be restarted)
            // so there is no need to do anything here.
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==_Destory");
    }
    private boolean _OnMedia(final int handle,final ByteBuffer input,final int length,final long timestamp,final short rotation)
    {
        //Log.d(DEBUGTAG,"==>_OnMedia");
        boolean result = false;
        try
        {
            final Message msg = Message.obtain(null,X264ServiceCommand.MSG_ENCODE);
            msg.replyTo = mMessenger;
            final Bundle bundle = new Bundle();
            byte[] data = null;
            if (input.isDirect())
            {
                data = new byte[length];
                input.position(0);
                input.get(data,0,length);
                
            }else
            {
            	data = new byte[length];
            	System.arraycopy(input.array(), 0, data, 0, length);
                //data = input.array();
            }

            bundle.putInt(X264ServiceCommand.KEY_FRAME_LENGTH, length);
            bundle.putLong(X264ServiceCommand.KEY_SAMPLE_TIME, timestamp);
            bundle.putShort(X264ServiceCommand.KEY_ROTATION, rotation);
            if (USE_LOCALSOCKET)
            {
                if (mOutputStream!=null && mInputStream!=null)
                {
                    msg.obj = bundle;
                    if (mService!=null)
                    {
                        mService.send(msg);
                        
                        result = true;
                        int write_length = 0;
                        while(write_length<length)
                        {
                            //Log.d(DEBUGTAG,"write_length="+write_length+",length="+length);
                            if (mLocalSocketOut.getSendBufferSize()<(length-write_length))
                            {
                                mOutputStream.write(data, write_length , mLocalSocketOut.getSendBufferSize());
                                write_length+=mLocalSocketOut.getSendBufferSize();
                            }
                            else
                            {
                                mOutputStream.write(data, write_length , length-write_length);
                                write_length+=(length-write_length);
                            }
                        }
                    }
                }
            }else
            {
                bundle.putByteArray(X264ServiceCommand.KEY_FRAME_DATA, data);
                msg.obj = bundle;
                if (mService!=null)
                {
                    mService.send(msg);
                    result = true;
                }
            }
        } catch (Throwable e)
        {
            Log.e(DEBUGTAG,"_OnMedia",e);
            // In this case the service has crashed before we could even
            // do anything with it; we can count on soon being
            // disconnected (and then reconnected if it can be restarted)
            // so there is no need to do anything here.
        }
        //Log.d(DEBUGTAG,"<==_OnMedia:"+result);
        return result;
    }
    private void _SetGop(final int handle,final int gop)
    {
        //Log.d(DEBUGTAG,"==>_SetGop");
        try
        {
            final Message msg = Message.obtain(null,X264ServiceCommand.MSG_SET_GOP);
            msg.replyTo = mMessenger;
            final Bundle bundle = new Bundle();
            bundle.putInt(X264ServiceCommand.KEY_GOP, gop);
            msg.obj = bundle;
            if (mService!=null)
                mService.send(msg);
        } catch (Throwable e)
        {
            Log.e(DEBUGTAG,"_SetGop",e);
            // In this case the service has crashed before we could even
            // do anything with it; we can count on soon being
            // disconnected (and then reconnected if it can be restarted)
            // so there is no need to do anything here.
        }
        //Log.d(DEBUGTAG,"<==_SetGop");
    }
    private void _SetFramerate(final int handle,final byte fps)
    {
        Log.d(DEBUGTAG,"==>_SetFramerate:"+fps);
        try
        {
            final Message msg = Message.obtain(null,X264ServiceCommand.MSG_SET_FRAMERATE);
            msg.replyTo = mMessenger;
            final Bundle bundle = new Bundle();
            bundle.putInt(X264ServiceCommand.KEY_FRAMERATE, (int)fps);
            msg.obj = bundle;
            if (mService!=null)
                mService.send(msg);
        } catch (Throwable e)
        {
            Log.e(DEBUGTAG,"_SetFramerate",e);
            // In this case the service has crashed before we could even
            // do anything with it; we can count on soon being
            // disconnected (and then reconnected if it can be restarted)
            // so there is no need to do anything here.
        }
        //Log.d(DEBUGTAG,"<==_SetFramerate");
    }
    private void _SetBitRate(final int handle,final int kbsBitRate)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>_SetBitRate:"+kbsBitRate);
        try
        {
            final Message msg = Message.obtain(null,X264ServiceCommand.MSG_SET_BITRATE);
            msg.replyTo = mMessenger;
            final Bundle bundle = new Bundle();
            bundle.putInt(X264ServiceCommand.KEY_BITRATE, kbsBitRate);
            msg.obj = bundle;
            if (mService!=null)
                mService.send(msg);
        } catch (Throwable e)
        {
            Log.e(DEBUGTAG,"_SetBitRate",e);
            // In this case the service has crashed before we could even
            // do anything with it; we can count on soon being
            // disconnected (and then reconnected if it can be restarted)
            // so there is no need to do anything here.
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==_SetBitRate");
    }
    // remote request IDR
    private void _requestIDR(final int handle)  
    {
        //Log.d(DEBUGTAG,"==>_requestIDR");
        try
        {
            final Message msg = Message.obtain(null,X264ServiceCommand.MSG_SET_REQUESTIDR);
            msg.replyTo = mMessenger;
            final Bundle bundle = new Bundle();
            msg.obj = bundle;
            if (mService!=null)
                mService.send(msg);
        } catch (Throwable e)
        {
            Log.e(DEBUGTAG,"_requestIDR",e);
            // In this case the service has crashed before we could even
            // do anything with it; we can count on soon being
            // disconnected (and then reconnected if it can be restarted)
            // so there is no need to do anything here.
        }
        //Log.d(DEBUGTAG,"<==_requestIDR");
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
        mLocalSocketOut    = null;
        mOutputStream      = null;
        mLocalSocketIn     = null;
        mInputStream       = null;
        mLocalSocketBitStream = null;
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
