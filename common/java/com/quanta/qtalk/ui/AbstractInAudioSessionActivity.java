package com.quanta.qtalk.ui;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.call.ICallListener;
import com.quanta.qtalk.media.MediaEngine;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.media.AudioManager;
import android.os.Bundle;
import com.quanta.qtalk.util.Log;

import android.view.KeyEvent;
import android.widget.Toast;

public abstract class AbstractInAudioSessionActivity extends AbstractBaseCallActivity  implements MediaEngine.AudioListener, ICallListener
{
//    private static final String TAG = "AbstractInAudioSessionActivity";
    private static final String DEBUGTAG = "QTFa_AbstractInAudioSessionActivity";
    private String        mAudioDestIP = "127.0.0.1";//"192.168.18.142";
    protected boolean     mEnableTFRC = true;
    protected int         mAudioListenPort = 0;
    protected int         mAudioDestPort = 0;
    protected String      mAudioMimeType = null;//"PCMU";
    protected int         mAudioPayloadId = 0;//0;
    protected int         mAudioSampleRate = 0;
    protected int         mAudioBitRate = 0;
    protected long        mAudioSSRC = 0;
    protected MediaEngine mMediaEngine = null;
    private AudioManager mAudioManager = null;
    protected long        mStartTime        = 0;
    protected int         mCallHold    = 0;
    protected int         mSpeakerOn   = 0;
    protected int         mCallType = 0;
    protected int         mCallOnHold    = 0;
    protected boolean     mEnableTelEvt    = false;
    protected int			mAudioFECCtrl	= 0;			//eason - FEC info
    protected int			mAudioFECSendPType = 0;
    protected int			mAudioFECRecvPType = 0;
    protected int			mAudioRTPTOS = 0;
    
    protected boolean 	menableAudioSRTP = false;      //Iron  - SRTP info
    protected String   	mAudioSRTPSendKey = null;
    protected String   	mAudioSRTPReceiveKey = null;
    
    protected int         mProductType = 0;


    @Override
    public boolean onKeyDown(int keyCode, KeyEvent msg) 
    {   
    	switch (keyCode) {
  	  	case KeyEvent.KEYCODE_VOLUME_UP:
  		  mAudioManager.adjustStreamVolume(AudioManager.STREAM_VOICE_CALL,
  		  AudioManager.ADJUST_RAISE, AudioManager.FLAG_SHOW_UI);
  	      return true;
  	  	case KeyEvent.KEYCODE_VOLUME_DOWN:
  		  mAudioManager.adjustStreamVolume(AudioManager.STREAM_VOICE_CALL,
  	      AudioManager.ADJUST_LOWER, AudioManager.FLAG_SHOW_UI);
  	      return true;
  	  	default:
  	  	  break;
  	  }
        boolean result = super.onKeyDown(keyCode, msg);
        return result;
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) 
    {
        super.onCreate(savedInstanceState);
        
        //eason - test getSettings
        QtalkSettings qtalkSetting = null;
        
        try {
			qtalkSetting = QTReceiver.engine(this, false).getSettings(this);
		} catch (FailedOperateException e) {
			// TODO Auto-generated catch block
			Log.e(DEBUGTAG,"QTReceiver engine",e);
		}
        
        if (mMediaEngine==null)
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "onCreate:");
            mMediaEngine = getMediaEngine(mCallID);
//            mStartTime = System.currentTimeMillis();
            mMediaEngine.reset();
            // Set keep screen on
            if (savedInstanceState!=null)
            {
                mAudioDestIP        = savedInstanceState.getString( QTReceiver.KEY_AUDIO_DEST_IP );
                mAudioListenPort    = savedInstanceState.getInt(    QTReceiver.KEY_AUDIO_LOCAL_PORT );
                mAudioDestPort      = savedInstanceState.getInt(    QTReceiver.KEY_AUDIO_DEST_PORT );
                mAudioPayloadId     = savedInstanceState.getInt(    QTReceiver.KEY_AUDIO_PAYLOAD_TYPE );
                mAudioSampleRate    = savedInstanceState.getInt(    QTReceiver.KEY_AUDIO_SAMPLE_RATE );
                mAudioBitRate       = savedInstanceState.getInt(    QTReceiver.KEY_AUDIO_BITRATE );
                mAudioMimeType      = savedInstanceState.getString( QTReceiver.KEY_AUDIO_MIME_TYPE );
                mAudioSSRC          = savedInstanceState.getLong(   QTReceiver.KEY_AUDIO_SSRC);
                mStartTime          = savedInstanceState.getLong(   QTReceiver.KEY_SESSION_START_TIME);
                mCallType           = savedInstanceState.getInt(    QTReceiver.KEY_CALL_TYPE);
                mEnableTelEvt       = savedInstanceState.getBoolean(QTReceiver.KEY_ENABLE_TEL_EVENT);
                mCallHold           = savedInstanceState.getInt(    QTReceiver.KEY_CALL_HOLD);
                mSpeakerOn          = savedInstanceState.getInt(    QTReceiver.KEY_SPEAKER_ON);
                mCallOnHold         = savedInstanceState.getInt(    QTReceiver.KEY_CALL_ONHOLD);
                mAudioFECCtrl 	    = savedInstanceState.getInt(    QTReceiver.KEY_AUDIO_FEC_CTRL);
                mAudioFECSendPType  = savedInstanceState.getInt(    QTReceiver.KEY_AUDIO_FEC_SEND_PTYPE);
                mAudioFECRecvPType  = savedInstanceState.getInt(    QTReceiver.KEY_AUDIO_FEC_RECV_PTYPE);
                
                menableAudioSRTP    = savedInstanceState.getBoolean(    QTReceiver.KEY_ENALBE_AUDIO_SRTP);
                mAudioSRTPSendKey   = savedInstanceState.getString(    QTReceiver.KEY_AUDIO_SRTP_SEND_KEY);
                mAudioSRTPReceiveKey= savedInstanceState.getString(    QTReceiver.KEY_AUDIO_SRTP_RECEIVE_KEY);
                
                mProductType        = savedInstanceState.getInt(	QTReceiver.KEY_PRODUCT_TYPE);
                
                
                if(mStartTime == 0)
                    mStartTime = System.currentTimeMillis();
                //mMediaEngine = new MediaEngine();
                
                mAudioRTPTOS = qtalkSetting.audioRTPTos;
                
                mMediaEngine.setupAudioEngine(this,mAudioSSRC,mAudioListenPort, mAudioDestIP, mAudioDestPort, mAudioPayloadId, mAudioMimeType, mAudioSampleRate,mAudioBitRate, mEnableTelEvt, 
                							  mAudioFECCtrl, mAudioFECSendPType, mAudioFECRecvPType, mAudioRTPTOS,
                							  menableAudioSRTP, mAudioSRTPSendKey, mAudioSRTPReceiveKey, mProductType);
            }else
            {
                Bundle bundle = this.getIntent().getExtras();
                if (bundle!=null)
                {
                    mAudioDestIP        = bundle.getString( QTReceiver.KEY_AUDIO_DEST_IP );
                    mAudioListenPort    = bundle.getInt(    QTReceiver.KEY_AUDIO_LOCAL_PORT );
                    mAudioDestPort      = bundle.getInt(    QTReceiver.KEY_AUDIO_DEST_PORT );
                    mAudioPayloadId     = bundle.getInt(    QTReceiver.KEY_AUDIO_PAYLOAD_TYPE );
                    mAudioSampleRate    = bundle.getInt(    QTReceiver.KEY_AUDIO_SAMPLE_RATE );
                    mAudioBitRate       = bundle.getInt(    QTReceiver.KEY_AUDIO_BITRATE );
                    mAudioMimeType      = bundle.getString( QTReceiver.KEY_AUDIO_MIME_TYPE );
                    mCallType           = bundle.getInt(    QTReceiver.KEY_CALL_TYPE);
                    mStartTime          = bundle.getLong(   QTReceiver.KEY_SESSION_START_TIME);
                    mEnableTelEvt       = bundle.getBoolean(QTReceiver.KEY_ENABLE_TEL_EVENT);
                    
                    mAudioFECCtrl 	    = bundle.getInt(    QTReceiver.KEY_AUDIO_FEC_CTRL);
                    mAudioFECSendPType  = bundle.getInt(    QTReceiver.KEY_AUDIO_FEC_SEND_PTYPE);
                    mAudioFECRecvPType  = bundle.getInt(    QTReceiver.KEY_AUDIO_FEC_RECV_PTYPE);
                    
                    menableAudioSRTP    = bundle.getBoolean(    QTReceiver.KEY_ENALBE_AUDIO_SRTP);
                    mAudioSRTPSendKey   = bundle.getString(    QTReceiver.KEY_AUDIO_SRTP_SEND_KEY);
                    mAudioSRTPReceiveKey= bundle.getString(    QTReceiver.KEY_AUDIO_SRTP_RECEIVE_KEY);
                    
                    mProductType        = bundle.getInt(	QTReceiver.KEY_PRODUCT_TYPE);

                    mCallHold = 0;
                    mCallOnHold = 0;
                    mSpeakerOn = 0;
                    if(mStartTime == 0)
                        mStartTime = System.currentTimeMillis();
                    //mMediaEngine = new MediaEngine();
                    
                    mAudioRTPTOS = qtalkSetting.audioRTPTos;
                    
                    mMediaEngine.setupAudioEngine(this,mAudioSSRC,mAudioListenPort, mAudioDestIP, mAudioDestPort, 
                    							  mAudioPayloadId, mAudioMimeType, mAudioSampleRate,mAudioBitRate, mEnableTelEvt, 
                    							  mAudioFECCtrl, mAudioFECSendPType, mAudioFECRecvPType, mAudioRTPTOS,
                    							  menableAudioSRTP, mAudioSRTPSendKey, mAudioSRTPReceiveKey,mProductType);
                }
            }
        }
        /*
        QTReceiver.engine(this,false).setCallListener(mCallID, this);
        if (mMediaEngine!=null)
        {
            mMediaEngine.start(AbstractInAudioSessionActivity.this);
        }else
            Log.e(DEBUGTAG, "onStart: error mMediaEngine is null");
        mHandler.postDelayed(mUpdateTimeTask,1000);
        */
        sendHangOnBroadcast();
    }

    @Override
    protected void onSaveInstanceState (Bundle outState)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "onSaveInstanceState:");
        super.onSaveInstanceState(outState);
        if (outState!=null)
        {
            if (mMediaEngine!=null)
            {
                mAudioSSRC = mMediaEngine.getAudioSSRC();
            }
            outState.putString(QTReceiver.KEY_AUDIO_DEST_IP,  mAudioDestIP);
            outState.putInt(QTReceiver.KEY_AUDIO_LOCAL_PORT, mAudioListenPort);
            outState.putInt(QTReceiver.KEY_AUDIO_DEST_PORT, mAudioDestPort);
            outState.putInt(QTReceiver.KEY_AUDIO_PAYLOAD_TYPE, mAudioPayloadId);
            outState.putInt(QTReceiver.KEY_AUDIO_SAMPLE_RATE, mAudioSampleRate);
            outState.putInt(QTReceiver.KEY_AUDIO_BITRATE, mAudioBitRate);
            outState.putString(QTReceiver.KEY_AUDIO_MIME_TYPE, mAudioMimeType);
            outState.putLong(QTReceiver.KEY_AUDIO_SSRC, mAudioSSRC);
            outState.putLong(QTReceiver.KEY_SESSION_START_TIME, mStartTime);
            outState.putInt(QTReceiver.KEY_CALL_TYPE,mCallType);
            outState.putInt(QTReceiver.KEY_CALL_HOLD,mCallHold);
            outState.putInt(QTReceiver.KEY_SPEAKER_ON,mSpeakerOn);
            outState.putInt(QTReceiver.KEY_CALL_ONHOLD,mCallOnHold);
            outState.putBoolean(QTReceiver.KEY_ENABLE_TEL_EVENT, mEnableTelEvt);
            outState.putInt(QTReceiver.KEY_PRODUCT_TYPE, mProductType);
            outState.putBoolean(QTReceiver.KEY_ENALBE_AUDIO_SRTP, menableAudioSRTP);
            outState.putString(QTReceiver.KEY_AUDIO_SRTP_SEND_KEY, mAudioSRTPSendKey);
            outState.putString(QTReceiver.KEY_AUDIO_SRTP_RECEIVE_KEY, mAudioSRTPReceiveKey);
        }
    }

    @Override
    protected void onResume()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "onResume:"+mCallHold +", mCallOnHold:"+mCallOnHold);
        super.onResume();
        mAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE); 
        QTReceiver.engine(this,false).setCallListener(mCallID, this);
        if (mMediaEngine!=null)
        {
            mMediaEngine.start(AbstractInAudioSessionActivity.this);
        }else
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG, "onStart: error mMediaEngine is null");
        mHandler.postDelayed(mUpdateTimeTask,1000);
    }

    @Override
    protected void onPause()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "onPause:");
        super.onPause();
        mHandler.removeCallbacks(mUpdateTimeTask);
        if (mMediaEngine!=null)
        {
            mAudioSSRC = mMediaEngine.getAudioSSRC();
            mMediaEngine.stop(AbstractInAudioSessionActivity.this);
        }else 
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG, "onPause: error mMediaEngine is null");
    }
    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        sendHangUpBroadcast();
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>onDestroy:");
        /*
		mHandler.removeCallbacks(mUpdateTimeTask);
        if (mMediaEngine!=null)
        {
            mAudioSSRC = mMediaEngine.getAudioSSRC();
            mMediaEngine.stop(AbstractInAudioSessionActivity.this);
        }else 
            Log.e(DEBUGTAG, "onPause: error mMediaEngine is null");
        */
        destoryMediaEngine(mCallID);
        mMediaEngine = null;
        // Take advantage of that onSessionEnd is triggered slower.
        // if onDestory is running but current Activity is not myself >> skim setting Activity, new Activity has been set already.
        // if current Activity is myself ==> Activity from notification >> set Activity equals to null and redirect to DialerPad.
        Activity current = ViroActivityManager.getLastActivity();
        if(AbstractInAudioSessionActivity.class.isInstance(current))
        {
            //Log.d(DEBUGTAG,"Activity resumed from notification");
            //Log.d(DEBUGTAG,"still in session?:"+QTReceiver.mEngine.isInSession());
            //if(!QTReceiver.mEngine.isInSession()) //check if it is in session
                ViroActivityManager.setActivity(null);
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "current:"+current);
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==onDestroy");
    }
    @Override
    public void setAudioSSRC(long ssrc) 
    {
        mAudioSSRC = ssrc;
    }
    
    protected boolean reinvite(boolean enableVideo)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "reinvite: mCallID" + mCallID );
        boolean result = false;
        try {
            //Use engine to reject call
            if (mCallID!=null)
            {
                QTReceiver.engine(this,false).makeCall(mCallID,mRemoteID, enableVideo, true);
            }
            result = true;
        }catch (Throwable e) {
            Log.e(DEBUGTAG,"reinvite",e);
        }
        return result;
    }
    protected boolean accept(boolean enableVideo,boolean enableCamera)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "accept: mCallID" + mCallID );
        boolean result = false;
        try {
            //Use engine to reject call
            if (mCallID!=null)
            {
                QTReceiver.engine(this,false).acceptCall(mCallID,mRemoteID, enableVideo, enableCamera);
            }
            result = true;
        }catch (Throwable e) {
            Log.e(DEBUGTAG,"reinvite",e);
        }
        return result;
    }
    @SuppressLint("ShowToast")
	protected boolean hangup(boolean isReinvite)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "hangup: mCallID" + mCallID + " mRemoteID:" + mRemoteID);
        boolean result = false;
        try {
            //Use engine to reject call
            if (mCallID!=null)
            {
            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "isReinvite:" +isReinvite);
                QTReceiver.engine(this,false).hangupCall(mCallID, mRemoteID,isReinvite);
                if (!isReinvite)
                {
                    RingingUtil.playSoundHangup(this);
                }
                //LogManager.addCallLog(this, mRemoteID, mCallType, System.currentTimeMillis()-mStartTime);
            }
            result = true;
        }catch (Throwable e) {
            Log.e(DEBUGTAG,"hangup",e);
            Toast.makeText(this, e.toString(), Toast.LENGTH_LONG);
        }
        return result;
    }
    protected boolean enableLoudSpeaker(boolean enable)
    {
        return mMediaEngine.enableLoudeSpeaker(this,enable);
    }
/*    protected boolean setHold(boolean doHold)
    {
        Log.d(DEBUGTAG,"setHold:"+doHold);
        boolean result = false;
        if (mMediaEngine!=null)
        {
            try
            {
                mMediaEngine.setHold(doHold);
                result = true;
            }catch (Throwable e) {
                Log.e(DEBUGTAG,"setPause",e);
                Toast.makeText(this, e.toString(), Toast.LENGTH_LONG);
            }
        }else 
            Log.e(DEBUGTAG, "setPause: error mMediaEngine is null");
        
        return result;
    }
    
    @SuppressLint("ShowToast")
	protected boolean setMute(boolean doMute)
    {
        Log.d(DEBUGTAG,"setMute");
        boolean result = false;
        if (mMediaEngine!=null)
        {
            try
            {
                mMediaEngine.setMute(doMute, null);
                result = true;
            }catch (Throwable e) {
                Log.e(DEBUGTAG,"setMute",e);
                Toast.makeText(this, e.toString(), Toast.LENGTH_LONG);
            }
        }else 
            Log.e(DEBUGTAG, "setMute: error mMediaEngine is null");
        
        return result;
    }*/
    protected abstract void setSessionTime(final long session);
    private Runnable mUpdateTimeTask = new Runnable() 
    {
        public void run() 
        {
            try
            {
                //setLocalViewResolution(mLocalViewWidth,mLocalViewHeight);
                setSessionTime(System.currentTimeMillis()-mStartTime);
                //TODO check if session is still connected
                if(!mMediaEngine.checkConnection())
                {
                	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"no session connection detected ");
                    hangup(false);
                    //finishWithDialog("connection failed");
                    finish();
                }
            }catch (Throwable e) 
            {
                Log.e(DEBUGTAG,"UpdateTimeTask",e);
            }
            mHandler.postDelayed(mUpdateTimeTask,1000);
        }
     };
     
     protected boolean requestHoldCall(boolean isHold)
     {
    	 if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "hold: mCallID" + mCallID );
         boolean result = false;
         try {
             //Use engine to reject call
             if (mCallID!=null)
             {
                 QTReceiver.engine(this,false).holdCall(mCallID,mRemoteID, isHold);
             }
             result = true;
         }catch (Throwable e) {
             Log.e(DEBUGTAG,"setHold",e);
         }
         return result;
     }
     
     protected void sendDTMF(final char press)
     {
         if(mMediaEngine != null)
             mMediaEngine.sendDTMF(press);
     }


}
