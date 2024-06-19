package com.quanta.qtalk.ui;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.view.KeyEvent;
import android.view.SurfaceView;
import android.view.TextureView;
import android.widget.Toast;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.QtalkApplication;
import com.quanta.qtalk.QtalkEngine;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.call.ICallListener;
import com.quanta.qtalk.media.MediaEngine;
import com.quanta.qtalk.media.video.CameraSource;
import com.quanta.qtalk.media.video.IDemandIDRCB;
import com.quanta.qtalk.media.video.IVideoStreamReportCB;
import com.quanta.qtalk.media.video.VideoRenderMediaCodec;
import com.quanta.qtalk.media.video.VideoRenderPlus;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

public abstract class AbstractInVideoSessionActivity extends AbstractBaseCallActivity implements MediaEngine.VideoListener,MediaEngine.AudioListener, ICallListener, IDemandIDRCB, IVideoStreamReportCB
{
    private static final String DEBUGTAG = "QSE_AbstractInVideoSessionActivity";
    private String          mAudioDestIP = "127.0.0.1";//"192.168.18.142";
    private String          mVideoDestIP = "127.0.0.1";//"192.168.18.142";
    protected boolean       mEnableTFRC = true;
    protected int           mAudioListenPort = 6666;
    protected int           mVideoListenPort = 4444;
    protected int           mAudioDestPort = 6666;
    protected int           mVideoDestPort = 4444;
    protected String        mAudioMimeType = "SPEEX";//"PCMU";
    protected String        mVideoMimeType = "H264";//"MJPEG";
    protected int           mAudioPayloadId  = 97;//0;
    protected int           mVideoPayloadId  = 109;//110;
    protected int           mAudioSampleRate = 8000;
    protected int           mAudioBitRate    = 0;
    protected int           mVideoSampleRate = 90000;
    protected long          mAudioSSRC = 0;
    protected long          mVideoSSRC = 0;
    
    protected MediaEngine   mMediaEngine = null;
    protected VideoRenderMediaCodec   mVideoRender = null;
    protected int           mCameraIndex = 2;
    
    protected int           mLocalViewWidth  = 0;
    protected int           mLocalViewHeight = 0;
//    protected boolean       mIsShareMode     = false;
    protected long          mStartTime       = 0;
    protected int           mCallType = 0;
    protected int           mProductType    = 0;
 //   protected int           mCallMute    = 0;
    protected int           mWidth       = 0;
    protected int           mHeight      = 0;
    protected int           mFPS         = 0;
    protected int           mKbps        = 0;
    protected boolean       mEnablePLI   = false;
    protected boolean       mEnableFIR   = false;
    protected boolean       mLetGopEqFps = false;
    protected boolean       mEnableTelEvt = false;
    
    protected int			mAudioFECCtrl	= 0;			//eason - FEC info
    protected int			mAudioFECSendPType = 0;
    protected int			mAudioFECRecvPType = 0;
    protected int			mVideoFECCtrl	= 0;
    protected int			mVideoFECSendPType = 0;
    protected int			mVideoFECRecvPType = 0;
    
    protected boolean 	menableAudioSRTP = false;      //Iron  - SRTP info
    protected String   	mAudioSRTPSendKey = null;
    protected String   	mAudioSRTPReceiveKey = null;
    protected boolean 	menableVideoSRTP = false;
    protected String   	mVideoSRTPSendKey = null;
    protected String   	mVideoSRTPReceiveKey = null;
    
    //eason - video stream provision data
    protected int			mOutMaxBandwidth = 0;
    protected boolean		mAutoBandwidth = false;
    protected int			mVideoMTU = 0;
    protected int			mAudioRTPTOS = 0;
    protected int			mVideoRTPTOS = 0;
    
    protected boolean     mEnableCameraMute = false;
    
    
    @Override
    public void onConfigurationChanged(Configuration newConfig) 
    {
    	if (newConfig == null) {
    		return ;
    	}
        super.onConfigurationChanged(newConfig);
        if (mMediaEngine!=null)
            mMediaEngine.setDisplayRotation(getWindowManager().getDefaultDisplay().getRotation());
    }
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent msg) 
    {   
        if (mMediaEngine!=null)
        {
            if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) 
            {
                mMediaEngine.adjuestVolume(AudioManager.ADJUST_LOWER);
            }
            else if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) 
            {
                mMediaEngine.adjuestVolume(AudioManager.ADJUST_RAISE);
            }
        }
    /*	switch (keyCode) {
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
  	  }*/
        boolean result = super.onKeyDown(keyCode, msg);
        return result;
    }
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onCreate");
        QtalkApplication app = (QtalkApplication) getApplication();
        
      //eason - test getSettings
        QtalkSettings qtalkSetting = null;
        
        try {
			qtalkSetting = QTReceiver.engine(this, false).getSettings(this);
		} catch (FailedOperateException e) {
			// TODO Auto-generated catch block
			Log.e(DEBUGTAG,"onCreate",e);
		}
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"in AbstractVideo onCreate videoMTU = "+ qtalkSetting.videoMtu);
        
        if (mMediaEngine==null)
        {
            if(mCallID==null)
            	mCallID = "0";

        	mMediaEngine = app.getMediaEngine(mCallID);
            mMediaEngine.reset();

            //mStartTime = System.currentTimeMillis();
            // Set keep screen on

            if (savedInstanceState!=null)
            {
                mAudioDestIP        = savedInstanceState.getString( QTReceiver.KEY_AUDIO_DEST_IP );
                mAudioListenPort    = savedInstanceState.getInt(    QTReceiver.KEY_AUDIO_LOCAL_PORT );
                mAudioDestPort      = savedInstanceState.getInt(    QTReceiver.KEY_AUDIO_DEST_PORT );
                mAudioPayloadId     = savedInstanceState.getInt(    QTReceiver.KEY_AUDIO_PAYLOAD_TYPE );
                mAudioSampleRate    = savedInstanceState.getInt(    QTReceiver.KEY_AUDIO_SAMPLE_RATE );
                mAudioBitRate       = savedInstanceState.getInt(     QTReceiver.KEY_AUDIO_BITRATE );
                mAudioMimeType      = savedInstanceState.getString( QTReceiver.KEY_AUDIO_MIME_TYPE );
                mAudioSSRC          = savedInstanceState.getLong(   QTReceiver.KEY_AUDIO_SSRC);

                mVideoDestIP        = savedInstanceState.getString( QTReceiver.KEY_VIDEO_DEST_IP );
                mVideoListenPort    = savedInstanceState.getInt(    QTReceiver.KEY_VIDEO_LOCAL_PORT );
                mVideoDestPort      = savedInstanceState.getInt(    QTReceiver.KEY_VIDEO_DEST_PORT );
                mVideoPayloadId     = savedInstanceState.getInt(    QTReceiver.KEY_VIDEO_PAYLOAD_TYPE );
                mVideoSampleRate    = savedInstanceState.getInt(    QTReceiver.KEY_VIDEO_SAMPLE_RATE );
                mVideoMimeType      = savedInstanceState.getString( QTReceiver. KEY_VIDEO_MIME_TYPE );
                mVideoSSRC          = savedInstanceState.getLong(   QTReceiver.KEY_VIDEO_SSRC);
    
                mCameraIndex        = savedInstanceState.getInt(    QTReceiver.KEY_CAMERA_INDEX);
                mStartTime          = savedInstanceState.getLong(   QTReceiver.KEY_SESSION_START_TIME);
                mCallType           = savedInstanceState.getInt(    QTReceiver.KEY_CALL_TYPE);
                mProductType        = savedInstanceState.getInt(    QTReceiver.KEY_PRODUCT_TYPE);
                mEnableTelEvt       = savedInstanceState.getBoolean(QTReceiver.KEY_ENABLE_TEL_EVENT);
 //               mCallMute           = savedInstanceState.getInt(    QTReceiver.KEY_CALL_MUTE);
                
                mWidth              = savedInstanceState.getInt(    QTReceiver.KEY_VIDEO_WIDTH);
                mHeight             = savedInstanceState.getInt(    QTReceiver.KEY_VIDEO_HEIGHT);
                mFPS                = savedInstanceState.getInt(    QTReceiver.KEY_VIDEO_FPS);
                mKbps               = savedInstanceState.getInt(    QTReceiver.KEY_VIDEO_KBPS);
                mEnablePLI          = savedInstanceState.getBoolean( QTReceiver.KEY_ENABLE_PLI);
                mEnableFIR          = savedInstanceState.getBoolean( QTReceiver.KEY_ENABLE_FIR);
                
                mAudioFECCtrl 	    = savedInstanceState.getInt(    QTReceiver.KEY_AUDIO_FEC_CTRL);
                mAudioFECSendPType  = savedInstanceState.getInt(    QTReceiver.KEY_AUDIO_FEC_SEND_PTYPE);
                mAudioFECRecvPType  = savedInstanceState.getInt(    QTReceiver.KEY_AUDIO_FEC_RECV_PTYPE);
                mVideoFECCtrl 	    = savedInstanceState.getInt(    QTReceiver.KEY_VIDEO_FEC_CTRL);
                mVideoFECSendPType  = savedInstanceState.getInt(    QTReceiver.KEY_VIDEO_FEC_SEND_PTYPE);
                mVideoFECRecvPType  = savedInstanceState.getInt(    QTReceiver.KEY_VIDEO_FEC_RECV_PTYPE);
                
                menableAudioSRTP    = savedInstanceState.getBoolean(    QTReceiver.KEY_ENALBE_AUDIO_SRTP);
                mAudioSRTPSendKey   = savedInstanceState.getString(    QTReceiver.KEY_AUDIO_SRTP_SEND_KEY);
                mAudioSRTPReceiveKey= savedInstanceState.getString(    QTReceiver.KEY_AUDIO_SRTP_RECEIVE_KEY);
                menableVideoSRTP    = savedInstanceState.getBoolean(    QTReceiver.KEY_ENALBE_VIDEO_SRTP);
                mVideoSRTPSendKey   = savedInstanceState.getString(    QTReceiver.KEY_VIDEO_SRTP_SEND_KEY);
                mVideoSRTPReceiveKey= savedInstanceState.getString(    QTReceiver.KEY_VIDEO_SRTP_RECEIVE_KEY); 
                
                mEnableCameraMute   = savedInstanceState.getBoolean(    QTReceiver.KEY_VIDEO_ENABLE_CAMERA_MUTE); 
                
                if(mStartTime == 0)
                    mStartTime = System.currentTimeMillis();
                mEnableTFRC = mProductType!=0;
                mLetGopEqFps = mProductType==1;
                
                //eason - set provision data
                mOutMaxBandwidth = qtalkSetting.outMaxBandWidth;
                mAutoBandwidth = qtalkSetting.autoBandWidth;
                mVideoMTU = qtalkSetting.videoMtu;
                mAudioRTPTOS = qtalkSetting.audioRTPTos;
                mVideoRTPTOS = qtalkSetting.videoRTPTos;                
                
                mVideoRender = new VideoRenderMediaCodec(this);
                mMediaEngine.setupAudioEngine(this,mAudioSSRC,mAudioListenPort, mAudioDestIP, mAudioDestPort, mAudioPayloadId, mAudioMimeType, mAudioSampleRate,mAudioBitRate, mEnableTelEvt, 
                							  mAudioFECCtrl, mAudioFECSendPType, mAudioFECRecvPType, mAudioRTPTOS,
                							  menableAudioSRTP, mAudioSRTPSendKey, mAudioSRTPReceiveKey, mProductType);
                mMediaEngine.setupVideoEngine(this,mVideoSSRC, mEnableTFRC,mVideoListenPort, mVideoDestIP, mVideoDestPort, mVideoPayloadId, mVideoMimeType, 
                                                               mVideoSampleRate, mVideoRender, mCameraIndex, mWidth, mHeight, mFPS, mKbps, mEnablePLI, mEnableFIR,mLetGopEqFps,
                                                               mVideoFECCtrl, mVideoFECSendPType, mVideoFECRecvPType,
                                                               mOutMaxBandwidth, mAutoBandwidth, mVideoMTU, mVideoRTPTOS,
                                                               menableAudioSRTP, mAudioSRTPSendKey, mAudioSRTPReceiveKey, menableVideoSRTP, mVideoSRTPSendKey, mVideoSRTPReceiveKey,
                                                               mEnableCameraMute,mProductType);
                
            } 
            else {

                Bundle bundle = this.getIntent().getExtras();
                if (bundle!=null)
                {
                    mAudioDestIP     = bundle.getString(  QTReceiver.KEY_AUDIO_DEST_IP );
                    mAudioListenPort = bundle.getInt(     QTReceiver.KEY_AUDIO_LOCAL_PORT );
                    mAudioDestPort   = bundle.getInt(     QTReceiver.KEY_AUDIO_DEST_PORT );
                    mAudioPayloadId  = bundle.getInt(     QTReceiver.KEY_AUDIO_PAYLOAD_TYPE );
                    mAudioSampleRate = bundle.getInt(     QTReceiver.KEY_AUDIO_SAMPLE_RATE );
                    mAudioBitRate    = bundle.getInt(     QTReceiver.KEY_AUDIO_BITRATE );
                    mAudioMimeType   = bundle.getString(  QTReceiver.KEY_AUDIO_MIME_TYPE );

                    mVideoDestIP     = bundle.getString(  QTReceiver.KEY_VIDEO_DEST_IP );
                    mVideoListenPort = bundle.getInt(     QTReceiver.KEY_VIDEO_LOCAL_PORT );
                    mVideoDestPort   = bundle.getInt(     QTReceiver.KEY_VIDEO_DEST_PORT );
                    mVideoPayloadId  = bundle.getInt(     QTReceiver.KEY_VIDEO_PAYLOAD_TYPE );
                    mVideoSampleRate = bundle.getInt(     QTReceiver.KEY_VIDEO_SAMPLE_RATE );
                    mVideoMimeType   = bundle.getString(  QTReceiver.KEY_VIDEO_MIME_TYPE );
                    mCallType        = bundle.getInt(     QTReceiver.KEY_CALL_TYPE);
                    mProductType     = bundle.getInt(     QTReceiver.KEY_PRODUCT_TYPE);
                    mStartTime       = bundle.getLong(    QTReceiver.KEY_SESSION_START_TIME);
                    
                    mWidth           = bundle.getInt(     QTReceiver.KEY_VIDEO_WIDTH);
                    mHeight          = bundle.getInt(     QTReceiver.KEY_VIDEO_HEIGHT);
                    mFPS             = bundle.getInt(     QTReceiver.KEY_VIDEO_FPS);
                    mKbps            = bundle.getInt(     QTReceiver.KEY_VIDEO_KBPS);
                    mEnablePLI       = bundle.getBoolean( QTReceiver.KEY_ENABLE_PLI);
                    mEnableFIR       = bundle.getBoolean( QTReceiver.KEY_ENABLE_FIR);
                    mEnableTelEvt    = bundle.getBoolean( QTReceiver.KEY_ENABLE_TEL_EVENT);
                    
                    mAudioFECCtrl 	    = bundle.getInt(    QTReceiver.KEY_AUDIO_FEC_CTRL);
                    mAudioFECSendPType  = bundle.getInt(    QTReceiver.KEY_AUDIO_FEC_SEND_PTYPE);
                    mAudioFECRecvPType  = bundle.getInt(    QTReceiver.KEY_AUDIO_FEC_RECV_PTYPE);
                    mVideoFECCtrl 	    = bundle.getInt(    QTReceiver.KEY_VIDEO_FEC_CTRL);
                    mVideoFECSendPType  = bundle.getInt(    QTReceiver.KEY_VIDEO_FEC_SEND_PTYPE);
                    mVideoFECRecvPType  = bundle.getInt(    QTReceiver.KEY_VIDEO_FEC_RECV_PTYPE);
                    
                    menableAudioSRTP    = bundle.getBoolean(    QTReceiver.KEY_ENALBE_AUDIO_SRTP);
                    mAudioSRTPSendKey   = bundle.getString(    QTReceiver.KEY_AUDIO_SRTP_SEND_KEY);
                    mAudioSRTPReceiveKey= bundle.getString(    QTReceiver.KEY_AUDIO_SRTP_RECEIVE_KEY);
                    menableVideoSRTP    = bundle.getBoolean(    QTReceiver.KEY_ENALBE_VIDEO_SRTP);
                    mVideoSRTPSendKey   = bundle.getString(    QTReceiver.KEY_VIDEO_SRTP_SEND_KEY);
                    mVideoSRTPReceiveKey= bundle.getString(    QTReceiver.KEY_VIDEO_SRTP_RECEIVE_KEY); 
                    
                    mEnableCameraMute   = bundle.getBoolean(    QTReceiver.KEY_VIDEO_ENABLE_CAMERA_MUTE); 
                    
                    if(mStartTime == 0)
                        mStartTime = System.currentTimeMillis();
                    mEnableTFRC = mProductType!=0;
                    mLetGopEqFps = mProductType==1;

                    mCameraIndex     = 2;
                    
                    //eason - set provision data
                    mOutMaxBandwidth = qtalkSetting.outMaxBandWidth;
                    mAutoBandwidth = qtalkSetting.autoBandWidth;
                    mVideoMTU = qtalkSetting.videoMtu;
                    mAudioRTPTOS = qtalkSetting.audioRTPTos;
                    mVideoRTPTOS = qtalkSetting.videoRTPTos;

                    mVideoRender = new VideoRenderMediaCodec(this);
                    mMediaEngine.setupAudioEngine(this,mAudioSSRC,mAudioListenPort, mAudioDestIP, mAudioDestPort, mAudioPayloadId, mAudioMimeType, mAudioSampleRate, mAudioBitRate, mEnableTelEvt, 
                    							  mAudioFECCtrl, mAudioFECSendPType, mAudioFECRecvPType, mAudioRTPTOS,
                    							  menableAudioSRTP, mAudioSRTPSendKey, mAudioSRTPReceiveKey, mProductType);
                    mMediaEngine.setupVideoEngine(this,mVideoSSRC, mEnableTFRC,mVideoListenPort, mVideoDestIP, mVideoDestPort, mVideoPayloadId, mVideoMimeType,
                                                                   mVideoSampleRate, mVideoRender, mCameraIndex, mWidth, mHeight, mFPS, mKbps, mEnablePLI, mEnableFIR,mLetGopEqFps,
                                                                   mVideoFECCtrl, mVideoFECSendPType, mVideoFECRecvPType,
                                                                   mOutMaxBandwidth, mAutoBandwidth, mVideoMTU, mVideoRTPTOS,
                                                                   menableAudioSRTP, mAudioSRTPSendKey, mAudioSRTPReceiveKey, menableVideoSRTP, mVideoSRTPSendKey, mVideoSRTPReceiveKey,
                                                                   mEnableCameraMute, mProductType);
                }                
            }
        }

        if (Hack.canHandleRotation()) {
            if (mMediaEngine!=null)
                mMediaEngine.setDisplayRotation(getWindowManager().getDefaultDisplay().getRotation());
        }
        else {
            this.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        }
        sendHangOnBroadcast();
    }

    @Override
    protected void onSaveInstanceState (Bundle outState)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onSaveInstanceState");
        super.onSaveInstanceState(outState);
        if (outState!=null)
        {
            if (mMediaEngine!=null)
            {
                mAudioSSRC = mMediaEngine.getAudioSSRC();
                mVideoSSRC = mMediaEngine.getVideoSSRC();
            }
            outState.putString(QTReceiver.KEY_AUDIO_DEST_IP,      mAudioDestIP);
            outState.putInt(   QTReceiver.KEY_AUDIO_LOCAL_PORT,   mAudioListenPort);
            outState.putInt(   QTReceiver.KEY_AUDIO_DEST_PORT,    mAudioDestPort);
            outState.putInt(   QTReceiver.KEY_AUDIO_PAYLOAD_TYPE, mAudioPayloadId);
            outState.putInt(   QTReceiver.KEY_AUDIO_SAMPLE_RATE,  mAudioSampleRate);
            outState.putInt(   QTReceiver.KEY_AUDIO_BITRATE,      mAudioBitRate);
            outState.putString(QTReceiver.KEY_AUDIO_MIME_TYPE,    mAudioMimeType);
            outState.putLong(  QTReceiver.KEY_AUDIO_SSRC,         mAudioSSRC);

            outState.putString(QTReceiver.KEY_VIDEO_DEST_IP,      mVideoDestIP);
            outState.putInt(   QTReceiver.KEY_VIDEO_LOCAL_PORT,   mVideoListenPort);
            outState.putInt(   QTReceiver.KEY_VIDEO_DEST_PORT,    mVideoDestPort);
            outState.putInt(   QTReceiver.KEY_VIDEO_PAYLOAD_TYPE, mVideoPayloadId);
            outState.putInt(   QTReceiver.KEY_VIDEO_SAMPLE_RATE,  mVideoSampleRate);
            outState.putString(QTReceiver.KEY_VIDEO_MIME_TYPE,    mVideoMimeType);
            outState.putLong(  QTReceiver.KEY_VIDEO_SSRC,         mVideoSSRC);
            
            outState.putInt(   QTReceiver.KEY_CAMERA_INDEX,       mCameraIndex);
            outState.putLong(  QTReceiver.KEY_SESSION_START_TIME, mStartTime);
            outState.putInt(   QTReceiver.KEY_CALL_TYPE,          mCallType);
            outState.putInt(   QTReceiver.KEY_PRODUCT_TYPE,       mProductType);
 //           outState.putInt(   QTReceiver.KEY_CALL_MUTE,          mCallMute);
            
            outState.putInt(QTReceiver.KEY_VIDEO_WIDTH,  mWidth);
            outState.putInt(QTReceiver.KEY_VIDEO_HEIGHT,  mHeight);
            outState.putInt(QTReceiver.KEY_VIDEO_FPS,  mFPS);
            outState.putInt(QTReceiver.KEY_VIDEO_KBPS,  mKbps);
            
            outState.putBoolean(QTReceiver.KEY_ENABLE_PLI,  mEnablePLI);
            outState.putBoolean(QTReceiver.KEY_ENABLE_FIR,  mEnableFIR);
            outState.putBoolean(QTReceiver.KEY_ENABLE_TEL_EVENT,  mEnableTelEvt);
            
            outState.putInt(QTReceiver.KEY_AUDIO_FEC_CTRL,  mAudioFECCtrl);
            outState.putInt(QTReceiver.KEY_AUDIO_FEC_SEND_PTYPE,  mAudioFECSendPType);
            outState.putInt(QTReceiver.KEY_AUDIO_FEC_RECV_PTYPE,  mAudioFECRecvPType);
            outState.putInt(QTReceiver.KEY_VIDEO_FEC_CTRL,  mVideoFECCtrl);
            outState.putInt(QTReceiver.KEY_VIDEO_FEC_SEND_PTYPE,  mVideoFECSendPType);
            outState.putInt(QTReceiver.KEY_VIDEO_FEC_RECV_PTYPE,  mVideoFECRecvPType);

            outState.putBoolean(QTReceiver.KEY_ENALBE_AUDIO_SRTP,  menableAudioSRTP);
            outState.putString(QTReceiver.KEY_AUDIO_SRTP_SEND_KEY,    mAudioSRTPSendKey);
            outState.putString(QTReceiver.KEY_AUDIO_SRTP_RECEIVE_KEY,    mAudioSRTPReceiveKey);
            outState.putBoolean(QTReceiver.KEY_ENALBE_VIDEO_SRTP,  menableVideoSRTP);
            outState.putString(QTReceiver.KEY_VIDEO_SRTP_SEND_KEY,    mVideoSRTPSendKey);
            outState.putString(QTReceiver.KEY_VIDEO_SRTP_RECEIVE_KEY,    mVideoSRTPReceiveKey);
            outState.putBoolean(QTReceiver.KEY_VIDEO_ENABLE_CAMERA_MUTE, mEnableCameraMute);
           
        }
    }
    @Override
    protected void onResume()
    {
 //      Log.d(DEBUGTAG,"onResume:"+mCallMute);
        super.onResume();
        if(skipOnPause) {
            Log.d(DEBUGTAG, "skip onResume");
            return;
        }
        QTReceiver.engine(this,false).setCallListener(mCallID, this);
        if (mMediaEngine!=null)
        {
        	mMediaEngine.start(AbstractInVideoSessionActivity.this);
        }else
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG, "onStart: error mMediaEngine is null");

        try
        {
            if (mMediaEngine!=null)
            {
                mMediaEngine.SetDemandIDRCB(this);
            }
        }catch(Throwable e)
        {
            Log.e(DEBUGTAG, "onResume",e);
        }
        mHandler.postDelayed(mUpdateTimeTask,1000); 
    }
    protected boolean skipOnPause = false;
    @Override
    protected void onPause()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>onPause");
        super.onPause();
        if(skipOnPause) {
            Log.d(DEBUGTAG, "skip onPause");
            return;
        }
        mHandler.removeCallbacks(mUpdateTimeTask);
        if (mMediaEngine!=null)
        {
            mAudioSSRC = mMediaEngine.getAudioSSRC();
            mVideoSSRC = mMediaEngine.getVideoSSRC();
            mMediaEngine.stop(AbstractInVideoSessionActivity.this);
        }else 
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG, "onStop: error mMediaEngine is null");
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==onPause");
    }
    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        sendHangUpBroadcast();
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>onDestroy");
        QtalkApplication app = (QtalkApplication) getApplication();

    	app.destoryMediaEngine(mCallID);
        mMediaEngine.stop(AbstractInVideoSessionActivity.this);
        mMediaEngine = null;
        // Take advantage of that onSessionEnd is triggered slower.
        // if onDestory is running but current Activity is not myself >> skim setting Activity, new Activity has been set already.
        // if current Activity is myself ==> Activity from notification >> set Activity equals to null and redirect to DialerPad.
        Activity current = ViroActivityManager.getLastActivity();
        if(AbstractInVideoSessionActivity.class.isInstance(current))
        {
            //Log.d(DEBUGTAG,"Activity resumed from notification");
            //if(!QTReceiver.mEngine.isInSession())
                ViroActivityManager.setActivity(null);
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "current:"+current);
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==onDestroy");

    }
    
    protected SurfaceView getRemoteView()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"getRemoteView");
        return mVideoRender;
    }
    protected SurfaceView getLocalView()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"getLocalView");
        SurfaceView result = null;
        if (mMediaEngine!=null)
        {
            result = mMediaEngine.getLocalView(this);
        }else 
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG, "getLocalView: error mMediaEngine is null");
        return result;
    }
    
    protected SurfaceView getCameraView()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"getCameraView");
        SurfaceView result = null;
        if (mMediaEngine!=null)
        {
            result = mMediaEngine.getCameraView(this);
        }else 
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG, "getCameraView: error mMediaEngine is null");
        return result;
    }
    protected TextureView getH264View(Handler handler)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"getH264View");
    	TextureView result = null;
        if (mMediaEngine!=null)
        {
            result = mMediaEngine.getH264View(this, handler);
        }else 
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG, "getH264View: error mMediaEngine is null");
        return result;
    }
    
    protected void setCameraViewScale(float x_scale, float y_scale)
    {
    	if (mMediaEngine!=null)
        {
            mMediaEngine.setScale(x_scale, y_scale);
        }else 
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG, "setCameraViewScale: error mMediaEngine is null");
    }
    
    protected void setVideoMute(boolean flag, Bitmap bitmap)
    {
    	if (mMediaEngine!=null)
        {
            mMediaEngine.setVideoMute(flag, bitmap);
        }else 
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG, "setVideoMute: error mMediaEngine is null");
    }
    

    protected boolean reinvite(boolean enableVideo)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "reinvite: mCallID" + mCallID );
        boolean result = false;
        try {
            //Use engine to reject call
            if (mCallID!=null)
            {
                QTReceiver.engine(this,false).makeCall(mCallID,mRemoteID, enableVideo,false);
            }
            result = true;
        }catch (Throwable e) {
            Log.e(DEBUGTAG,"reinvite",e);
        }
        return result;
    }
    
    protected boolean accept(boolean enableVideo, boolean enableCamera)
    {
    	if(ProvisionSetupActivity.debugMode)  Log.d(DEBUGTAG, "accept: mCallID" + mCallID );
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
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"hangup");
        if(ProvisionSetupActivity.debugMode)  Log.d(DEBUGTAG, "hangup: mCallID" + mCallID + " mRemoteID:" + mRemoteID);
        boolean result = false;
        try {
            //Use engine to reject call
            if (mCallID!=null)
            {
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
    protected boolean setHold(boolean doHold)
    {
        Log.d(DEBUGTAG,"setHold");
        boolean result = false;
        if (mMediaEngine!=null)
        {
            try
            {
                mMediaEngine.setHold(doHold);
                result = true;
            }catch (Throwable e) {
                Log.e(DEBUGTAG,"setHold",e);
                //Toast.makeText(this, e.toString(), Toast.LENGTH_LONG);
            }
        }else 
            Log.e(DEBUGTAG, "setPause: error mMediaEngine is null");
        
        return result;
    }
    
    protected boolean setMute(boolean doMute, Bitmap bitmap)
    {
        Log.d(DEBUGTAG,"setMute");
        boolean result = false;
        if (mMediaEngine!=null)
        {
            try
            {
                mMediaEngine.setMute(doMute,bitmap);
                result = true;
            }catch (Throwable e) {
                Log.e(DEBUGTAG,"setMute",e);
                Toast.makeText(this, e.toString(), Toast.LENGTH_LONG);
            }
        }else 
            Log.e(DEBUGTAG, "setMute: error mMediaEngine is null");
        
        return result;
    }
    protected boolean enableLoudSpeaker(boolean enable)
    {
        return mMediaEngine.enableLoudeSpeaker(this,enable);
    }
   /* protected boolean switchCamera()
    {
        Log.d(DEBUGTAG,"switchCamera");
        boolean result = false;
        if (Hack.isSamsungTablet())//Disable table swap camera
            return result;
        mHandler.post(new Runnable()
        {
            public void run()
            {
                if (mCameraIndex == 1)
                {
                    mCameraIndex = 2;
                    mMediaEngine.setCameraIndex(mCameraIndex);
                }else
                {
                    mCameraIndex = 1;
                    mMediaEngine.setCameraIndex(mCameraIndex);
                }
                if (Hack.canHandleRotation())
                {
                    mMediaEngine.setDisplayRotation(getWindowManager().getDefaultDisplay().getRotation()); 
                }
            }
        });
        return result;
    }*/
    protected static Rect getPreferedWidthAndHeight(int resWidth,int resHeight,int desX,int desY, int desWidth, int desHeight)
    {
        Rect rect = new Rect();
        if (desWidth !=0 && desHeight != 0)
        {
            rect.left = desX;
            rect.top = desY;
            int resultW = ((desHeight-desY) * resWidth) / resHeight;
            int resultH = ((desWidth-desX) * resHeight) / resWidth;

            if (resultW < (desWidth-desX))
            {
                rect.left = desX + (desWidth - resultW) / 2;
                resultH = desHeight;
            }
            else if (resultH < (desHeight-desY))
            {
                resultW = desWidth;
                rect.top = desY + (desHeight - resultH) / 2;
            }
            rect.right = rect.left + resultW;
            rect.bottom = rect.top + resultH;
        }else
        {
            rect.right = 0;
            rect.bottom =0;
            rect.left =0;
            rect.top = 0;
        }
        return rect;
     }
    protected abstract void setSessionTime(final long session);
    protected abstract void setLocalViewResolution(final int width,final int height);
    protected abstract void onCameraModeChanged(final boolean isDualCamera,final boolean isShareMode);
    @Override
    public void setVideoSSRC(long ssrc) 
    {
        mVideoSSRC = ssrc;
    }
    @SuppressLint("ShowToast")
	@Override
    public void setLocalResolution(int width, int height) 
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"setLocalResolution:"+width+"X"+height);
        try
        {
            mLocalViewWidth = width;
            mLocalViewHeight = height;
            setLocalViewResolution(width,height);
        }catch (Throwable e) 
        {
            Log.e(DEBUGTAG,"setLocalResolution",e);
            Toast.makeText(this, e.toString(), Toast.LENGTH_LONG);
        }
    }
    @Override
    public void setCameraMode(boolean isDualCamera,short mode) {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"setCameraMode:"+mode);

        mMediaEngine.setDisplayRotation(getWindowManager().getDefaultDisplay().getRotation()); 
        if (mode == CameraSource.MODE_FACE)
            onCameraModeChanged(isDualCamera,false);
        else
            onCameraModeChanged(isDualCamera,true);
    }
    @Override
    public void setAudioSSRC(long ssrc) 
    {
        mAudioSSRC = ssrc;
    }
    public void setVideoReportListener(IVideoStreamReportCB cbListener)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setVideoReportListener:"+cbListener);
        mMediaEngine.setVideoReportListener(cbListener);
    }
        
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
                    finish();
                   // finishWithDialog("connection failed");
                }
            }catch (Throwable e) 
            {
                Log.e(DEBUGTAG,"UpdateTimeTask",e);
            }
            mHandler.postDelayed(mUpdateTimeTask,1000);
        }
     };
  /*   protected void sendDTMF(final char press)
     {
         if(mMediaEngine != null)
             mMediaEngine.sendDTMF(press);
     }*/
     
     @Override
     public void onRequestIDR()
     {
    	 if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onRequestIDR");
         if (mMediaEngine!=null)
         {
             mMediaEngine.RequestIDR();
         }
     }
     @Override
     public void onDemandIDR()
     {
    	 if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onDemandIDR");
         QtalkEngine engine = QTReceiver.engine(this,false);
         if (engine!=null)
         {
             //send IDR request to remote-side
             engine.demandIDR(mCallID);
         }
     }
/*     protected void sr2_switchcall()
     {
    	 //if (QTReceiver.ENABLE_DEBUG) Log.d(DEBUGTAG, "sr2_switchcall: IP:" + IP + " sip_port:"+sip_port);
    	
    	 //QTReceiver.engine(this,false).sr2_switch();
     }*/
}
