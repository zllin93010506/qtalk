package org.ffmpeg.service;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.app.Service;
import android.content.Intent;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class FFMpegService extends Service 
{
    private final static String TAG = "QTFa_FFMpegService";
    /**
     * Handler of incoming messages from clients.
     */
    class IncomingHandler extends Handler {
        private void handleBasicMessage(final Message msg) 
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(TAG,"==>handleBasicMessage:");
            final Messenger replyto = msg.replyTo;
            int result = FFMpegServiceCommand.RESULT_NA;
            Bundle bundle_input = null;
            Bundle bundle_output = null;
            try
            {
                if (msg.obj!=null)
                {
                    bundle_input = (Bundle)msg.obj;
                }
                switch (msg.what) {
                    case FFMpegServiceCommand.MSG_STOP:
                    	if(ProvisionSetupActivity.debugMode) Log.d(TAG,"MSG_STOP:"+replyto);
                        //Destroy X264 Encoder
                        //doDestroy();
                        result = FFMpegServiceCommand.RESULT_SUCCESS;
                        break;
                    default:
                        super.handleMessage(msg);
                }
            }catch(Exception e)
            {
                Log.e(TAG,"handleMessage",e);
                result = FFMpegServiceCommand.RESULT_EXCEPTION;
            }
            final Message msg_result = Message.obtain(null,msg.what, result, 0);
            msg_result.obj = bundle_output;
            if (replyto!=null)
            {
                try 
                {
                    replyto.send(msg_result);
                } catch (RemoteException e) {
                    Log.e(TAG,"handleMessage",e);
                    // The client is dead.  Remove it from the list;
                    // we are going through the list from back to front
                    // so this is safe to do inside the loop.
                }
            }
            if (msg.what==FFMpegServiceCommand.MSG_STOP)
            {
                if (mFFMpegHandler!=null)
                {
                    mFFMpegHandler.stop();
                    mFFMpegHandler = null;
                }
            }
            if(ProvisionSetupActivity.debugMode) Log.d(TAG,"<==handleBasicMessage:");
        }
        private void handleStartMessage(final Message msg)
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(TAG,"==>handleStartMessage:");
            final Messenger replyto = msg.replyTo;
            int result = FFMpegServiceCommand.RESULT_NA;
            Bundle bundle_input = null;
            Bundle bundle_output = null;
            if (msg.what==FFMpegServiceCommand.MSG_START)
            {
                if (msg.obj!=null)
                {
                    bundle_input = (Bundle)msg.obj;
                }
                try
                {
                    if (bundle_input!=null && replyto!=null)
                    {
                        final int width = bundle_input.getInt(FFMpegServiceCommand.KEY_FRAME_WIDTH);
                        final int height = bundle_input.getInt(FFMpegServiceCommand.KEY_FRAME_HEIGHT);
                        final int format = bundle_input.getInt(FFMpegServiceCommand.KEY_FRAME_FORMAT);
                        //Initial X264 Encoder
                        if (mFFMpegHandler!=null)
                        {
                            mFFMpegHandler.stop();
                            mFFMpegHandler = null;
                        }
                        mFFMpegHandler = new FFMpegHandler(format,width,height);
                        
                        final String socket_name = bundle_input.getString(FFMpegServiceCommand.KEY_LOCAL_SOCKET_NAME);
                        if (socket_name!=null)
                        {
                            bundle_output = new Bundle();
                            final String new_socket_name = FFMpegServiceCommand.SERVICE_ACTION_NAME+System.currentTimeMillis();//+socket_name;
                            bundle_output.putString(FFMpegServiceCommand.KEY_LOCAL_SOCKET_NAME, new_socket_name);
                            final LocalServerSocket tmpServer = new LocalServerSocket(new_socket_name);
                            if (tmpServer!=null)
                            {
                                new Thread(new Runnable() 
                                {
                                    public void run() 
                                    {
                                        LocalSocket localSocketIn = null;
                                        LocalSocket localSocketOut = null;
                                        InputStream inputStream = null;
                                        OutputStream outputStream = null;
                                        if(ProvisionSetupActivity.debugMode) Log.d(TAG,"==>localSocketIn.run:"+socket_name);
                                        try
                                        {
                                            localSocketIn = new LocalSocket();
                                            localSocketIn.connect(new LocalSocketAddress(socket_name));
                                            if (localSocketIn.isConnected())
                                            {
                                                inputStream = localSocketIn.getInputStream();
                                                if (tmpServer!=null)
                                                {
                                                	if(ProvisionSetupActivity.debugMode) Log.d(TAG,"==>localSocketOut:"+socket_name);
                                                    localSocketOut = tmpServer.accept();
                                                    if (localSocketOut!=null)
                                                        outputStream = localSocketOut.getOutputStream();
                                                    if(ProvisionSetupActivity.debugMode) Log.d(TAG,"<==localSocketOut:localSocketOut="+localSocketOut);
                                                    tmpServer.close();
                                                }
                                            }
                                        }catch (IOException e)
                                        {
                                            Log.e(TAG,"LocalServerSocket",e);
                                        }finally
                                        {
                                            if (mFFMpegHandler!=null)
                                                mFFMpegHandler.setConnection(localSocketIn, localSocketOut, inputStream, outputStream);
                                        }
                                        if(ProvisionSetupActivity.debugMode) Log.d(TAG,"<==localSocketIn.run:"+localSocketIn.isConnected());
                                    }
                                }).start();
                            }
                        }
                        result = FFMpegServiceCommand.RESULT_SUCCESS;
                        final Message msg_result = Message.obtain(null,msg.what, result, 0);
                        msg_result.obj = bundle_output;
                        replyto.send(msg_result);
                        mFFMpegHandler.start();
                    }
                }catch(Exception e)
                {
                    Log.e(TAG,"HandleStartMessage",e);
                }
            }
            if(ProvisionSetupActivity.debugMode) Log.d(TAG,"<==handleStartMessage");
        }
        private void handleDecodeMessage(final Message msg)
        {
            //long start = System.currentTimeMillis();
            //Log.d(TAG,"==>handleDecodeMessage:");
            final Messenger replyto = msg.replyTo;
            Bundle bundle_input = null;
            if (msg.what==FFMpegServiceCommand.MSG_DECODE)
            {
                if (msg.obj!=null)
                {
                    bundle_input = (Bundle)msg.obj;
                }
                try
                {
                    if (bundle_input!=null && replyto!=null)
                    {
                        final int    data_length = bundle_input.getInt(FFMpegServiceCommand.KEY_DATA_LENGTH);
                        final long   sample_time  = bundle_input.getLong(FFMpegServiceCommand.KEY_SAMPLE_TIME);
                        final short  rotation     = bundle_input.getShort(FFMpegServiceCommand.KEY_ROTATION);
                        //Read the data
                        if (mFFMpegHandler!=null)
                            mFFMpegHandler.onMedia(replyto, data_length, sample_time, rotation);
                    }
                }catch(Exception e)
                {
                    Log.e(TAG,"handleDecodeMessage",e);
                }
            }
            //Log.d(TAG,"<==handleDecodeMessage:");
        }
        @Override
        public void handleMessage(final Message msg) {
            //Log.d(TAG,"==>handleMessage:"+msg.what);
            switch (msg.what) {
                case FFMpegServiceCommand.MSG_START:
                    //android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_DISPLAY);
                    handleStartMessage(msg);
                    break;
                case FFMpegServiceCommand.MSG_DECODE:
                    handleDecodeMessage(msg);
                    break;
                default:
                    handleBasicMessage(msg);
                    break;
            }
            //Log.d(TAG,"<==handleMessage");
        }
    }
    FFMpegHandler mFFMpegHandler = null;
    /**
     * Target we publish for clients to send messages to IncomingHandler.
     */
    final Messenger mMessenger = new Messenger(new IncomingHandler());
    @Override
    public void onCreate() {
    	if(ProvisionSetupActivity.debugMode) Log.d(TAG, "onCreate");
    }
    @Override
    public void onDestroy() {
    	if(ProvisionSetupActivity.debugMode) Log.d(TAG, "onDestroy");
        //Destroy H264 Encoder
        if (mFFMpegHandler!=null)
        {
            mFFMpegHandler.stop();
            mFFMpegHandler = null;
        }
    }
    /**
     * When binding to the service, we return an interface to our messenger
     * for sending messages to the service.
     */
    @Override
    public IBinder onBind(Intent intent) {
    	if(ProvisionSetupActivity.debugMode) Log.d(TAG, "onBind");
        return mMessenger.getBinder();
    }
}
