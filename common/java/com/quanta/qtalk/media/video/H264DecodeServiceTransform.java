package com.quanta.qtalk.media.video;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;

import org.ffmpeg.service.FFMpegService;
import org.ffmpeg.service.FFMpegServiceCommand;


import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.media.ISink;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.PixelFormat;
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

public class H264DecodeServiceTransform implements IVideoTransform
{
    public  final static String CODEC_NAME = "H264";
    public  final static int    DEFAULT_ID = 109;
    private final static String TAG = "H264DecodeServiceTransform";
    private static final String DEBUGTAG = "QTFa_H264DecodeServiceTransform";
    private IVideoSink mVideoSink = null;
    private int        mWidth = 0;
    private int        mHeight = 0;
    private int        mInFormat = 0;
    private final int  mOutFormat = PixelFormat.RGB_565;
    
    private Context    mContext = null;
    private boolean    mStopped = true;
    //LocalSocket to share data across process
    private static final String mLocalSocketName   = "com.quanta.qtalk.media.video.H264DecodeServiceTransform";
    private LocalSocket         mLocalSocketOut    = null;
    private OutputStream        mOutputStream      = null;
    private LocalSocket         mLocalSocketIn     = null;
    private InputStream         mInputStream       = null;
    private byte[]              mLocalSocketBitStream = null;
    final ConditionVariable mOnStopLock = new ConditionVariable();
    H264DecodeServiceTransform(Context context)
    {
        mContext = context;
        doBindService(mContext);
    }
    protected void finalize () 
    {
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
        boolean result = false;
        try
        {
            if (!mStopped && mHandle!=0 && data!=null && length>0)
            {
                    synchronized(this)
                    {
                        if (!mStopped && mHandle!=0)
                        {
                            result = _OnMedia(mHandle, data,length, timestamp, rotation);
                        }
                    }
            }else
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
    public void setFormat(int format, int width, int height)  throws FailedOperateException, UnSupportException 
    {
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
            synchronized(this)
            {    //Reset Sink
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
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"onMediaNative", e);
            }
        }
    }
   
    //===================================Service
    /** Messenger for communicating with service. */
    private Messenger mService = null;
    /** Flag indicating whether we have called bind on the service. */
    private boolean mIsBound;
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
                case FFMpegServiceCommand.MSG_START:
                	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>MSG_START:"+msg.arg1);
                    //android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_DISPLAY);
                    final String socket_name = bundle_input.getString(FFMpegServiceCommand.KEY_LOCAL_SOCKET_NAME);
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
                                Log.d(DEBUGTAG,"<==mLocalSocketIn.run:"+mLocalSocketIn.isConnected());
                            }
                        }).start();
                        
                    }
                    if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==MSG_START");
                    break;
                case FFMpegServiceCommand.MSG_STOP:
                	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"MSG_STOP");
                    mOnStopLock.open();
                    break;
                
                case FFMpegServiceCommand.MSG_DECODE:
                    //Log.d(DEBUGTAG,"MSG_ENCODE");
                    if (bundle_input!=null)
                    {
                        final int    length = bundle_input.getInt(FFMpegServiceCommand.KEY_DATA_LENGTH);
                        final long   sample_time  = bundle_input.getLong(FFMpegServiceCommand.KEY_SAMPLE_TIME);
                        final short  rotation  = bundle_input.getShort(FFMpegServiceCommand.KEY_ROTATION);
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
                                 if (!mStopped && bitstream!=null)
                                     onMediaNative(mVideoSink,
                                                    bitstream,
                                                    length,
                                                    sample_time,
                                                    rotation);
                            }catch(Exception e)
                            {
                                Log.e(DEBUGTAG,"handleMessage",e);

                                if (mInputStream!=null)
                                {
                                    try
                                    {
                                        mInputStream.close();
                                    } catch (IOException e1)
                                    {
                                    }finally
                                    {
                                        mInputStream = null;
                                    }
                                }
                                if (mLocalSocketIn!=null)
                                {
                                    try
                                    {
                                        mLocalSocketIn.close();
                                    } catch (IOException e1)
                                    {
                                    }finally
                                    {
                                        mLocalSocketIn = null;
                                    }
                                }
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
            try
            {
                Intent intent = new Intent();
                intent.setClass(context,FFMpegService.class);
                context.bindService(intent, mConnection, Context.BIND_AUTO_CREATE);
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
                    stop();
                } catch (Exception e)
                {
                    Log.e(DEBUGTAG,"doUnbindService",e);
                    // There is nothing special we need to do if the service
                    // has crashed.
                }
            }
            if (context!=null)
            {
                try
                {
                    // Detach our existing connection.
                    context.unbindService(mConnection);
                }catch(Exception e)
                {
                    Log.e(DEBUGTAG,"bindService",e);
                }
            }
            mIsBound = false;
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==doUnbindService");
    }
    private int mHandle = 0;
    private int  _Start(final ISink sink,final  int pixformat,final int width,final  int height) throws Exception
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>_Start:");
        int handle = 0;
        try 
        {
            if (mService!=null)
            {
                final Message msg = Message.obtain(null,FFMpegServiceCommand.MSG_START,0,0);
                msg.replyTo = mMessenger;
                final Bundle bundle = new Bundle();
//                final ConditionVariable socketDone = new ConditionVariable();
//                socketDone.close();
                final String socket_name = mLocalSocketName+System.currentTimeMillis();
                bundle.putString(FFMpegServiceCommand.KEY_LOCAL_SOCKET_NAME,socket_name );
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
                                	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"Failed on initialize LocalSocketOut");
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
                bundle.putInt(FFMpegServiceCommand.KEY_FRAME_WIDTH, width);
                bundle.putInt(FFMpegServiceCommand.KEY_FRAME_HEIGHT, height);
                bundle.putInt(FFMpegServiceCommand.KEY_FRAME_FORMAT, pixformat);
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
                    final Message msg = Message.obtain(null,FFMpegServiceCommand.MSG_STOP,0,0);
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
            final Message msg = Message.obtain(null,FFMpegServiceCommand.MSG_DECODE);
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
                data = input.array();
            }

            bundle.putInt(FFMpegServiceCommand.KEY_DATA_LENGTH, length);
            bundle.putLong(FFMpegServiceCommand.KEY_SAMPLE_TIME, timestamp);
            bundle.putShort(FFMpegServiceCommand.KEY_ROTATION, rotation);
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
        } catch (Throwable e)
        {
            Log.e(DEBUGTAG,"_OnMedia",e);
            if (mOutputStream!=null)
            {
                try
                {
                    mOutputStream.close();
                } catch (IOException e1)
                {
                }finally
                {
                    mOutputStream = null;
                }
            }
            if (mLocalSocketOut!=null)
            {
                try
                {
                    mLocalSocketOut.close();
                } catch (IOException e1)
                {
                }finally
                {
                    mLocalSocketOut = null;
                }
            }
            // In this case the service has crashed before we could even
            // do anything with it; we can count on soon being
            // disconnected (and then reconnected if it can be restarted)
            // so there is no need to do anything here.
        }
        //Log.d(DEBUGTAG,"<==_OnMedia:"+result);
        return result;
    }
    
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
