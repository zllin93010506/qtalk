package com.quanta.qtalk.media.video;

import java.util.ArrayList;


import android.app.Service;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Binder;
import android.os.IBinder;
import android.os.RemoteException;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class VideoLocalService extends Service 
{
    private static final String         DEBUGTAG      = "QSE_VideoLocalService";
    private IVideoSource                mSource       = null;
    private IVideoStreamEngine          mStreamEngine = null;
    private ArrayList<IVideoTransform>  mTransforms   = null;
    private IVideoSink                  mSink         = null;
    private IVideoEncoder               mEncoder      = null;
    private boolean                     mStarted      = false;
    private long                        mSSRC         = 0;
    
    public synchronized void setVideoSource(IVideoSource source) throws RemoteException 
    {
    	Log.d(DEBUGTAG,"=> setVideoSource()");
        synchronized(this)
        {
            mSource = source;
        }
    }
    public synchronized void setVideoSource(String className) throws RemoteException 
    {
    	Log.d(DEBUGTAG,"=> setVideoSource(): "+className);
        Object object = null;
        try
        {
            if (X264ServiceTransform.class.getName().compareTo(className)==0)
            {
                object = new X264ServiceTransform(this);
            }else if (H264DecodeServiceTransform.class.getName().compareTo(className)==0)
            {
                object = new H264DecodeServiceTransform(this);
            }else if (VideoStreamEngineTransform.class.getName().compareTo(className)==0)
            {
                object = new VideoStreamEngineTransform(this);
                mStreamEngine = (IVideoStreamEngine)object;
            } else
                object =  Class.forName(className).newInstance();
            mSource = (IVideoSource)object;
        }catch (Throwable e)
        {
            Log.e(DEBUGTAG,"setVideoSource",e);
            RemoteException ex = new RemoteException();
            ex.initCause(e);
            throw ex;
        }
    }
    public synchronized void addTransform(String className) throws RemoteException 
    {
        Log.d(DEBUGTAG, "=> addTransform(): "+className);
        try 
        {
            Object object = null;
            if (X264ServiceTransform.class.getName().compareTo(className)==0)
            {
                object = new X264ServiceTransform(this);
            }else if (H264DecodeServiceTransform.class.getName().compareTo(className)==0)
            {
                object = new H264DecodeServiceTransform(this);
            }else if (VideoStreamEngineTransform.class.getName().compareTo(className)==0)
            {
                object = new VideoStreamEngineTransform(this);
                mStreamEngine = (IVideoStreamEngine)object;
                try
                {
                    if (mTransforms.size()>0)
                        mStreamEngine.setSourceCallback((IVideoStreamSourceCB)mTransforms.get(mTransforms.size()-1));
                }catch (Throwable e)
                {
                    Log.e(DEBUGTAG,"addTransform",e);
                }
            }else
                object =  Class.forName(className).newInstance();
            //Get video encoder
            if (object instanceof IVideoEncoder){
                mEncoder = (IVideoEncoder) object;
                if(DummyEncodeTransform.class.getName().compareTo(className)==0){
                	mEncoder.setCameraCallback((IVideoEncoderCB)mSource);
                }
            }
            IVideoTransform transform = (IVideoTransform)object;
            if (mTransforms.size()==0)
            {
                mSource.setSink(transform);
                mSource.stop();
            }else
            {
                IVideoTransform last_tranform = mTransforms.get(mTransforms.size()-1);
                last_tranform.setSink(transform);
                last_tranform.stop();
            }
            mTransforms.add(transform);
        }catch (Throwable e)
        {
            Log.e(DEBUGTAG,"addTransform",e);
            RemoteException ex = new RemoteException();
            ex.initCause(e);
            throw ex;
        }
    }
    public synchronized void setVideoSink(IVideoSink sink) throws RemoteException 
    {
    	Log.d(DEBUGTAG,"=> setVideoSink");
        try
        {
            if (mTransforms.size()==0)
            {
                mSource.setSink(sink);
                mSource.stop();
            }else
            {
                IVideoTransform last_tranform = mTransforms.get(mTransforms.size()-1);
                last_tranform.setSink(sink);
                last_tranform.stop();
                if (mStreamEngine!=null && sink instanceof IVideoStreamReportCB)
                {
                    mStreamEngine.setReportCallback((IVideoStreamReportCB)sink);
                }
                if (IPollbased.ENABLE_POLL_BASED && 
                    last_tranform instanceof IVideoSourcePollbased &&
                    sink instanceof IVideoSinkPollbased)
                {
                    ((IVideoSinkPollbased)sink).setSource((IVideoSourcePollbased)last_tranform);
                }else if (IPollbased.ENABLE_POLL_BASED)
                	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"Failed on enable POLL_BASED media path");
            }
        }catch (Throwable e)
        {
            Log.e(DEBUGTAG,"setVideoSink",e);
            RemoteException ex = new RemoteException();
            ex.initCause(e);
            throw ex;
        }
        mSink = sink;
    }
    public void start(long ssrc,int listentPort,boolean enableTFRC,
                        String destIP, int destPort,
                        int sampleRate,int payloadID, String mimeType,
                        int fps,int kbps, boolean enablePLI, boolean enableFIR, boolean letGopEqFps, int fecCtrl, int fecSendPType, int fecRecvPType,
                        boolean enableVideoSRTP,String videoSRTPSendKey, String videoSRTPReceiveKey,
                        int outMaxBandwidth, boolean autoBandwidth, int videoMTU, 
                        int videoRTPTOS, int networkType, int productType) throws RemoteException 
    {
        // Start all components from the last to first
    	Log.d(DEBUGTAG, "=> start");
        try
        {
            if (mStarted)
            {
                synchronized(this) {
                    stop();
                }
            }
            synchronized(this)
            {
                if (!mStarted)
                {
                    if (mStreamEngine!=null)
                    {
                        mStreamEngine.setListenerInfo(listentPort, enableTFRC);
                        mStreamEngine.setRemoteInfo(ssrc,destIP, destPort, fps, kbps, enablePLI, enableFIR, letGopEqFps, productType);
                        mStreamEngine.setMediaInfo(sampleRate, payloadID, mimeType);
                        mStreamEngine.setFECInfo(fecCtrl, fecSendPType, fecRecvPType);		//eason - set FEC info
                        mStreamEngine.setSRTPInfo(enableVideoSRTP, videoSRTPSendKey, videoSRTPReceiveKey);     //Iron - set SRTP info
                        mStreamEngine.setStreamInfo(outMaxBandwidth, autoBandwidth, videoMTU, videoRTPTOS);
                        mStreamEngine.setNetworkType(networkType);
//                        mStreamEngine.setReportCallback(mReportCallback);
                    }
                    if (mTransforms.size()==0)
                    {
                        mSource.setSink(mSink);
                        mSource.stop();
                    }else
                    {
                        IVideoTransform last_tranform = mTransforms.get(mTransforms.size()-1);
                        last_tranform.setSink(mSink);
                        last_tranform.stop();
                    }
                    for(int i=(mTransforms.size()-1);i>=0;i--)
                    {
                    	Log.d(DEBUGTAG,"=> start(): "+mTransforms.get(i));
                        mTransforms.get(i).start();
                    }
                    mSource.start();
                    mStarted  = true;
                    if (StatisticAnalyzer.ENABLE)
                        StatisticAnalyzer.start();
                }
            }
        }catch (Throwable e)
        {
            Log.e(DEBUGTAG,"start",e);
            RemoteException ex = new RemoteException();
            ex.initCause(e);
            throw ex;
        }
    }
    public long getSSRC() throws RemoteException 
    {
        try
        {
            if (mStreamEngine!=null)
            {
                mSSRC = mStreamEngine.getSSRC();
            }
        }
        catch (Throwable e)
        {
            Log.e(DEBUGTAG,"getSSRC",e);
            RemoteException ex = new RemoteException();
            ex.initCause(e);
            throw ex;
        }
        return mSSRC;
    }
    public boolean isStarted()
    {
        return mStarted;
    }
    public void stop() throws RemoteException 
    {
    	Log.d(DEBUGTAG,"=> stop");
        try
        {
            if (mStarted)
            {
                if (mSource!=null) {
                    mSource.stop();
                    mSource.destory();
                }
                    
                for(int i=0;i<mTransforms.size();i++)
                {
                    mTransforms.get(i).stop();
                    mTransforms.get(i).release();
                }
                  
                if (mSink != null)
                {
                    mSink.release();
                }
                    
                
                if (StatisticAnalyzer.ENABLE)
                    StatisticAnalyzer.stop();
            }
        }
        catch (Throwable e)
        {
            Log.e(DEBUGTAG,"stop",e);
            RemoteException ex = new RemoteException();
            ex.initCause(e);
            throw ex;
        }
    }
    public void reset() throws RemoteException 
    {
    	Log.d(DEBUGTAG,"=> reset");
        try
        {
            synchronized(this)
            {
                stop();
                mSource = null;
                mTransforms.clear();
                mSink = null;
            }
            //System.runFinalization();
            //System.gc();
        }
        catch (Throwable e)
        {
            Log.e(DEBUGTAG,"reset",e);
            RemoteException ex = new RemoteException();
            ex.initCause(e);
            throw ex;
        }
    }
    @Override
    public void onCreate()
    {
    	Log.d(DEBUGTAG,"=> onCreate");
        super.onCreate();
        mTransforms = new ArrayList<IVideoTransform>();
    }
    @Override
    public void onDestroy()
    {
    	Log.d(DEBUGTAG,"=> onDestroy");
        try {
            reset();
        } catch (RemoteException e) {
            Log.e(DEBUGTAG,"onDestroy", e);
        }
        super.onDestroy();
    }
    public void setMute(boolean enable,Bitmap bitmap)
    {
        if(mSource != null && mSource instanceof IVideoMute)
        {
            ((IVideoMute) mSource).setMute(enable, bitmap);
        }
    }
    public void setHold(boolean enable)
    {
        try
        {
            if(mSource != null)
            {
                if (enable)
                    mSource.stop();
                else
                    mSource.start();
            }
            if(mStreamEngine != null)
            {
                mStreamEngine.setHold(enable);
            }
        }catch(Throwable e)
        {
            Log.e(DEBUGTAG,"setHold", e);
        }
    }
    // This is the object that receives interactions from clients.  See
    // RemoteService for a more complete example.
    private final IBinder mBinder = new LocalBinder();
    @Override
    public IBinder onBind(Intent intent) 
    {
        return mBinder;
    }
     /**
     * Class for clients to access.  Because we know this service always
     * runs in the same process as its clients, we don't need to deal with
     * IPC.
     */
    public class LocalBinder extends Binder
    {
        public VideoLocalService getService()
        {
            return VideoLocalService.this;
        }
    }
    public void requestIDR()
    {
        //call encoder requestIDR method
        if (mEncoder!=null)
        {
            mEncoder.requestIDR();
        }
    }
    public void SetDemandIDRCB(IDemandIDRCB callback) throws FailedOperateException
    {
        if (mStreamEngine!=null)
        {
            mStreamEngine.setDemandIDRCallBack(callback);
        }
    }
}
