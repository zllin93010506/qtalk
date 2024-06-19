package com.quanta.qtalk.media.audio;

//import java.io.File;
import java.util.ArrayList;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;
import android.os.RemoteException;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

public class AudioService extends Service implements IAudioStreamReportCB
{
    private static final String DEBUGTAG = "QSE_AudioService";
    private static boolean isLoudeSpeaker = false;
    private static boolean isHeadsetPlugin = false;
    private BroadcastReceiver mBroadcastRecv = null;
    private int mCurrSystemVolume = 0;
    
    private synchronized void updateAudioManager()
    {
    	Log.d(DEBUGTAG, "=> isLoudeSpeaker:"+isLoudeSpeaker+", isHeadsetPlugin:"+isHeadsetPlugin);
//    	mAudioManager.setMode(Hack.getAudioMode(isHeadsetPlugin, isLoudeSpeaker));
    	    	
        if (isHeadsetPlugin == true){
        	Log.d(DEBUGTAG, "=> Headset Mode");
            if(mAudioManager != null)
            	mAudioManager.setSpeakerphoneOn(false);
        } else if (isHeadsetPlugin == false){
        	if(mAudioManager != null) {
        		if(isLoudeSpeaker == true) {
            		mAudioManager.setMode(Hack.getAudioMode(isHeadsetPlugin, isLoudeSpeaker));
            		if(Hack.isQF3()||Hack.isK72()||Hack.isQF3a()||Hack.isQF5()) {
            			Log.d(DEBUGTAG, "=> LoudeSpeaker Mode: setSpeakerphoneOn(false)");
            			mAudioManager.setSpeakerphoneOn(false);
            		}
            		else {
            			Log.d(DEBUGTAG, "=> LoudeSpeaker Mode: setSpeakerphoneOn(true)");
            			mAudioManager.setSpeakerphoneOn(true);
            		}
        		}
        		else {
        			Log.d(DEBUGTAG, "=> Normal Speaker Mode");
        			mAudioManager.setSpeakerphoneOn(false);
        			mAudioManager.setMode(Hack.getAudioMode(isHeadsetPlugin, isLoudeSpeaker));
        		}
        	}
        }
    }
    
	class HeadsetReceiver extends BroadcastReceiver {
		@Override
		public void onReceive(Context context, final Intent intent) {
			new Thread(new Runnable() {
				@Override
				public void run() {
					String action = intent.getAction();
					switch (action) {
					// 處理耳機插拔Event
					case Intent.ACTION_HEADSET_PLUG:
						int state = intent.getIntExtra("state", 0);
						Log.d(DEBUGTAG, "=> ACTION_HEADSET_PLUG: " + state);
						isHeadsetPlugin = (state == 1) ? true : false;
						updateAudioManager();
						break;
					default:
						break;
					}
				}
			}).start();
		}
	}
    
    /**
    * Class for clients to access.  Because we know this service always
    * runs in the same process as its clients, we don't need to deal with
    * IPC.
    */
    public class LocalBinder extends Binder
    {
        public AudioService getService()
        {
            return AudioService.this;
        }
    }
    // This is the object that receives interactions from clients.  See
    // RemoteService for a more complete example.
    private static IAudioService.Stub mRemoteBinder = null;//Remote Service
    private final IBinder mLocalBinder = new LocalBinder();//Local Service
    private long mLastConnectionTime;
	private boolean mMuteState = false;
    private boolean mHoldState = false;

	@Override
	public int onStartCommand(Intent intent, int flags, int startId)
    {
        super.onStart(intent,startId);
        Log.d(DEBUGTAG, "=> onStart(), startId:" + startId + ", intent:" + intent);
        return START_REDELIVER_INTENT;
    }
    
    @Override
    public IBinder onBind(Intent intent)
    {
    	Log.d(DEBUGTAG, "onBind");
        synchronized(AudioService.class)
        {
            if (mRemoteBinder == null)
                mRemoteBinder = new AudioServiceInternal(this);
            return mRemoteBinder;
        }
        //return mLocalBinder;
    }
    
    @Override
    public void onCreate()
    {
        super.onCreate();
        Log.d(DEBUGTAG, "=> onCreate()");
        try
        {
            mAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE); 
            IntentFilter myIntenFilter = new IntentFilter();
            myIntenFilter.addAction(Intent.ACTION_HEADSET_PLUG);
            mBroadcastRecv = new HeadsetReceiver();
            registerReceiver(mBroadcastRecv, myIntenFilter);            
            
            // Initial isHeadsetPlugin to read current Headset status.
            if(mAudioManager.isWiredHeadsetOn() == true)
            	isHeadsetPlugin = true;
        }
        catch (Throwable e) {
            Log.e(DEBUGTAG,"onStart",e);
        }
    }
    @Override
    public void onDestroy()
    {
    	Log.d(DEBUGTAG,"=> onDestory");
    	
    	if(mBroadcastRecv != null)
    		unregisterReceiver(mBroadcastRecv);
    	
    	try {
            reset();
        } catch (Throwable e) {
            Log.e(DEBUGTAG,"onStart",e);
        }
        System.runFinalization();
        super.onDestroy();
        
    	if(mAudioManager != null) {
    		Log.d(DEBUGTAG, "=> Normal Speaker Mode");
        	mAudioManager.setMode(AudioManager.MODE_NORMAL);
    		mAudioManager.setSpeakerphoneOn(false);
    		mAudioManager = null;
    	}
    	System.gc();
    }
    private IAudioSource               mSource = null;
    private IAudioStreamEngine         mStreamEngine = null;
    private final ArrayList<IAudioTransform> mTransforms = new ArrayList<IAudioTransform>();
    private IAudioSink                 mSink = null;
    private boolean                    mStarted = false;
    public static AudioManager         mAudioManager = null;
    
    //set the source components for the media service
    public void setSource(final String className,final int format,final int sampleRate,
                          final int numChannels) throws RemoteException 
    {
    	Log.d(DEBUGTAG, "=> setSource(), className:"+className+", format:"+format+", sampleRate:"+sampleRate+", numChannels:"+numChannels);
        //Create Instance from class name
        try 
        {
            synchronized(this)
            {
                if (!mStarted) {
                    IAudioSource source = (IAudioSource)Class.forName(className).newInstance();
                    source.setFormat(format, sampleRate, numChannels);
                    mSource = source;
                }
                else {
                    throw new FailedOperateException("The Audio Source is already running, please call stop() to stop audio source before replace it");
                }
            }
        }
        catch (Throwable e) {
            Log.e(DEBUGTAG,"setSource",e);
            RemoteException ex = new RemoteException();
            ex.initCause(e);
            throw ex;
        }
    }
    
    //Add new transform component to the media service
    public void addTransform(String className, boolean isStreamTransform) throws RemoteException 
    {
    	Log.d(DEBUGTAG, "=> addTransform(), className:"+className);
        try 
        {
            synchronized(mTransforms)
            {
                Object object =  Class.forName(className).newInstance();
                if (object instanceof IAudioStreamEngine) // obtain stream engine
                    mStreamEngine = (IAudioStreamEngine)object;
                
                IAudioTransform transform = (IAudioTransform)object;
                if (mTransforms.size() == 0) {
                    mSource.setSink(transform);
                    mSource.stop();
                }
                else {
                    IAudioTransform last_tranform = mTransforms.get(mTransforms.size()-1);
                    last_tranform.setSink(transform);
                    last_tranform.stop();
                }
                mTransforms.add(transform);
            }
        }
        catch (Throwable e) {
            Log.e(DEBUGTAG,"addTransform",e);
            RemoteException ex = new RemoteException();
            ex.initCause(e);
            throw ex;
        }
    }

    //set the render component for the media service
    public void setSink(String className) throws RemoteException 
    {
        //SetSink Component
    	Log.d(DEBUGTAG, "=> setSink(), className:"+className);
        try 
        {
            synchronized(this)
            {
                IAudioSink sink = (IAudioSink)Class.forName(className).newInstance();
                if (mTransforms.size()==0) {
                    mSource.setSink(sink);
                    mSource.stop();
                }
                else {
                    IAudioTransform last_tranform = mTransforms.get(mTransforms.size()-1);
                    last_tranform.setSink(sink);
                    last_tranform.stop();
                }
                mSink = sink;
            }
        }
        catch (Throwable e)
        {
            Log.e(DEBUGTAG,"setSink",e);
            RemoteException ex = new RemoteException();
            ex.initCause(e);
            throw ex;
        }
    }
    
    public void start(long ssrc,int listentPort,
                      String destIP, int destPort,
                      int sampleRate,int payloadID, String mimeType, boolean enableTelEvt,
                      int fecCtrl, int fecSendPType, int fecRecvPType, 
                      boolean enableAudioSRTP, String audioSRTPSendKey, String audioSRTPReceiveKey,
                      int audioRTPTOS, 
                      boolean hasVideo, int networkType, int productType) throws RemoteException		//eason - FEC info  //Iron - STRP info
    {
    	Log.d(DEBUGTAG, "=> start(), ssrc:"+ssrc+", listentPort"+listentPort+", destIP:"+destIP+", destPort:"+destPort+", sampleRate:"+sampleRate+", payloadID:"+payloadID+", mimeType:"+mimeType +", enableTelEvt:"+enableTelEvt);
        try
        {
            if (mStarted) {
                stop();
            }
            
            if(mAudioManager != null) {
            	/* keep current volume and restore on stop() */
            	if(AudioManager.STREAM_SYSTEM == Hack.getAudioStreamType()) {
            		mCurrSystemVolume = mAudioManager.getStreamVolume(Hack.getAudioStreamType());
            	}
            	/* set to max volume on PTT case */
//                if(productType == 8) {
//                	int maxVolume = mAudioManager.getStreamMaxVolume(Hack.getAudioStreamType());
//                	mAudioManager.setStreamVolume(Hack.getAudioStreamType(), maxVolume, AudioManager.FLAG_REMOVE_SOUND_AND_VIBRATE);
//                	Log.d(DEBUGTAG, "setStreamVolume(), volume:"+maxVolume);
//                }                
            }

            synchronized(this)
            {
                if (!mStarted)
                {
                    mMuteState = false;
                    mHoldState = false;
                    if (mStreamEngine!=null) {
                        mStreamEngine.setListenerInfo(listentPort);
                        mStreamEngine.setRemoteInfo(ssrc,destIP, destPort, productType);
                        mStreamEngine.setMediaInfo(payloadID, mimeType, enableTelEvt);
                        mStreamEngine.setFECInfo(fecCtrl, fecSendPType, fecRecvPType);		//eason - sef FEC info
                        mStreamEngine.setSRTPInfo(enableAudioSRTP, audioSRTPSendKey, audioSRTPReceiveKey);	    //Iron - set FEC info
                        mStreamEngine.setAudioRTPTOS(audioRTPTOS);
                        mStreamEngine.setReportCallback(AudioService.this);
                        mStreamEngine.setHasVideo(hasVideo);
                        mStreamEngine.setNetworkType(networkType);
                    }                    
                    
                    if (mSink!=null && mSink instanceof IAudioRender)
                        ((IAudioRender)mSink).start();
                    
                    for(int i=(mTransforms.size()-1);i>=0;i--) {
                        mTransforms.get(i).start();
                    }
                    
                    mSource.start();
                    
                    mLastConnectionTime = System.currentTimeMillis();
                    mStarted  = true;
                    if (StatisticAnalyzer.ENABLE)
                        StatisticAnalyzer.start();
                }
            }
        }
        catch (Throwable e) {
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
                return mStreamEngine.getSSRC();
            }
        }catch (Throwable e)
        {
            Log.e(DEBUGTAG,"getSSRC",e);
            RemoteException ex = new RemoteException();
            ex.initCause(e);
            throw ex;
        }
        return 0;
    }
    
    //query the state of media service
    public boolean isStarted() throws RemoteException 
    {
        return mStarted;
    }    
    
    //stop all running components
    public void stop() throws RemoteException 
    {
    	Log.d(DEBUGTAG, "=> stop(), "+mStarted);
        try
        {
            mLastConnectionTime = System.currentTimeMillis();
            if (!mStarted)
                return;
            
            /* restore volume */
            if((mAudioManager != null) && (mCurrSystemVolume != 0)) {
            	mAudioManager.setStreamVolume(Hack.getAudioStreamType(), mCurrSystemVolume, AudioManager.FLAG_REMOVE_SOUND_AND_VIBRATE);
            	mCurrSystemVolume = 0;
            }
            
            synchronized(this)
            {
                if (mStarted)
                {
                    if (mSource!=null)
                        mSource.stop();
                    for(int i=0;i<mTransforms.size();i++) {
                        mTransforms.get(i).stop();
                    }

                    if (mSink!=null && mSink instanceof IAudioRender)
                        ((IAudioRender)mSink).stop();
                    mStarted = false;
                    if (StatisticAnalyzer.ENABLE)
                        StatisticAnalyzer.stop();
                }
            }
        }catch (Throwable e)
        {
            Log.e(DEBUGTAG,"stop",e);
            RemoteException ex = new RemoteException();
            ex.initCause(e);
            throw ex;
        }
    }
    
    //stop all running components and remove them from the media service
    public void reset() throws RemoteException 
    {
    	Log.d(DEBUGTAG, "=> reset()");
        try
        {
            stop();
            synchronized(this)
            {
                mSource = null;
                mTransforms.clear();
                mSink = null;
            }
        }catch (Throwable e)
        {
            Log.e(DEBUGTAG,"reset",e);
            RemoteException ex = new RemoteException();
            ex.initCause(e);
            throw ex;
        }
    }
    
    public void enableMic(boolean enable) throws RemoteException 
    {
    	Log.d(DEBUGTAG, "=> enableMic()");
        try
        {
            mAudioManager.setMicrophoneMute(!enable);
            if (mSource!=null)
            {
                if (enable)
                    mSource.start();
                else
                    mSource.stop();
            }
        }catch (Throwable e)
        {
            Log.e(DEBUGTAG,"reset",e);
            RemoteException ex = new RemoteException();
            ex.initCause(e);
            throw ex;
        }
    }
    
    public void enableLoudeSpeaker(boolean enable) throws RemoteException 
    {
    	Log.d(DEBUGTAG, "=> enableLoudeSpeaker(), "+enable);
    	
    	isLoudeSpeaker = enable;
    	updateAudioManager();
    }
    
	public void adjectVolume(float volume) throws RemoteException
    {

	}

    public boolean checkConnection()
    {
        return true;
    }

    public void setMute(boolean enable) throws RemoteException
    {
        if (mSource!=null && mSource instanceof IAudioMute)
        {
            if (mMuteState!=enable)
            {
                ((IAudioMute)mSource).setMute(enable);
                mMuteState=enable;
            }
        }
    }

    public void setHold(boolean enable) throws RemoteException
    {
        try
        {
            if (mStreamEngine!=null && mHoldState!=enable)
            {
                mHoldState = enable;
                mLastConnectionTime = System.currentTimeMillis();
                mStreamEngine.setHold(enable);

                Log.d(DEBUGTAG,"=> setHold(), enable:"+enable+", mLastConnectionTime:"+mLastConnectionTime+", mSink:"+mSink);

                if (mSource!=null) {
                    if (enable)
                        mSource.stop();
                    else
                        mSource.start();
                }
                
                if (mSink!=null) {
                    if (enable)
                        ((IAudioRender)mSink).stop();
                    else
                        ((IAudioRender)mSink).start();
                }
            }
        }
        catch (Throwable e)
        {
            Log.e(DEBUGTAG,"setHold",e);
            RemoteException ex = new RemoteException();
            ex.initCause(e);
            throw ex;
        }
    }
    
    public void sendDTMF(final char press)
    {
        if (mSource!=null && mSource instanceof IAudioDTMF)
        {
            ((IAudioDTMF)mSource).sendDTMF(press);
        }
    }
    
    @Override
    public void onReport(final int kbpsRecv,final int kbpsSend,
            final int fpsRecv,final int fpsSend,
            final int secGapRecv,final int secGapSend)
    {
        //Log.d(DEBUGTAG,"onReport-"+" kbpsRecv:"+kbpsRecv+", fpsRecv:"+fpsRecv+", kbpsSend"+kbpsSend+", fpsSend:"+fpsSend+", secGapSend:"+secGapSend+", secGapRecv:"+secGapRecv);
        // update mLastConnectiontime
        if (secGapSend < 6)
            mLastConnectionTime = System.currentTimeMillis();
    }
    
    static class AudioServiceInternal extends IAudioService.Stub
    {
        private final Context mContext;
        AudioServiceInternal(Context context)
        {
            //Create ArrayList for Transform
            mContext = context;
        }
        @Override
        public void setSource(String className, int format, int sampleRate,
                int numChannels) throws RemoteException {
            ((AudioService)mContext).setSource(className, format, sampleRate, numChannels);
        }

        @Override
        public void addTransform(String className, boolean isStreamTransform)
                throws RemoteException {
            ((AudioService)mContext).addTransform(className, isStreamTransform);
        }

        @Override
        public void setSink(String className) throws RemoteException {
            ((AudioService)mContext).setSink(className);
            
        }

        @Override
        public void start(long ssrc,int listentPort, String destIP, int destPort,
                int sampleRate, int payloadID, String mimeType, boolean enableTelEvt,
                int fecCtrl, int fecSendPType, int fecRecvPType, 
                boolean enableAudioSRTP, String audioSRTPSendKey, String audioSRTPReceiveKey,
                int audioRTPTOS, boolean hasVideo, int networkType, int productType)
                throws RemoteException {
            ((AudioService)mContext).start(ssrc,listentPort, destIP, destPort, sampleRate, 
            							   payloadID, mimeType, enableTelEvt, fecCtrl, 
            							   fecSendPType, fecRecvPType, enableAudioSRTP,audioSRTPSendKey, audioSRTPReceiveKey,
            							   audioRTPTOS, hasVideo, networkType, productType);
        }

        @Override
        public boolean isStarted() throws RemoteException {
            return ((AudioService)mContext).isStarted();
        }

        @Override
        public void stop() throws RemoteException {
            ((AudioService)mContext).stop();
        }

        @Override
        public void reset() throws RemoteException {
            ((AudioService)mContext).reset();
        }
        
        @Override
        public long getSSRC() throws RemoteException {
            return ((AudioService)mContext).getSSRC();
        }
        
        @Override
        public void enableMic(boolean enable) throws RemoteException {
            ((AudioService)mContext).enableMic(enable);
        }
        
        @Override
        public void enableLoudeSpeaker(boolean enable) throws RemoteException {
            ((AudioService)mContext).enableLoudeSpeaker(enable);
        }    
        
        @Override 
        public void adjectVolume(float direction) throws RemoteException{
            ((AudioService)mContext).adjectVolume(direction);
        }
        
        @Override
        public boolean checkConnection() throws RemoteException
        {
            return ((AudioService)mContext).checkConnection();
        }

        @Override
        public void setMute(boolean enable) throws RemoteException
        {
            ((AudioService)mContext).setMute(enable);
        }

        @Override
        public void setHold(boolean enable) throws RemoteException
        {
            ((AudioService)mContext).setHold(enable);
        }
        
        @Override
        public  void sendDTMF(char press) throws RemoteException
        {
            ((AudioService)mContext).sendDTMF(press);
        }
    }
}