package com.quanta.qtalk.media;

import java.net.SocketException;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Bitmap;
import android.graphics.PixelFormat;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.view.SurfaceView;
import android.view.TextureView;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.call.MDP;
import com.quanta.qtalk.call.engine.NativeCallEngine;
import com.quanta.qtalk.media.audio.AecMicTransform;
import com.quanta.qtalk.media.audio.AecSpeakerTransform;
import com.quanta.qtalk.media.audio.AudioRender;
import com.quanta.qtalk.media.audio.AudioService;
import com.quanta.qtalk.media.audio.AudioStreamEngineTransform;
import com.quanta.qtalk.media.audio.G711DecoderTransform;
import com.quanta.qtalk.media.audio.G711EncoderTransform;
import com.quanta.qtalk.media.audio.G7221DecoderTransform;
import com.quanta.qtalk.media.audio.G7221EncoderTransform;
import com.quanta.qtalk.media.audio.IAudioService;
import com.quanta.qtalk.media.audio.MicSource;
import com.quanta.qtalk.media.audio.OpusDecoderTransform;
import com.quanta.qtalk.media.audio.OpusEncoderTransform;
import com.quanta.qtalk.media.audio.PCMAEncoderTransform;
import com.quanta.qtalk.media.audio.PCMUEncoderTransform;
import com.quanta.qtalk.media.audio.SpeexDecoderTransform;
import com.quanta.qtalk.media.audio.SpeexEncoderTransform;
import com.quanta.qtalk.media.audio.AlcTransform;
import com.quanta.qtalk.media.video.CameraSourcePlus;
import com.quanta.qtalk.media.video.CameraSourcePlusWith1832;
import com.quanta.qtalk.media.video.DummyEncodeTransform;
import com.quanta.qtalk.media.video.DummyDecodeTransform;
import com.quanta.qtalk.media.video.H264DecodeServiceTransform;
import com.quanta.qtalk.media.video.H264DecodeTransform;
import com.quanta.qtalk.media.video.IDemandIDRCB;
import com.quanta.qtalk.media.video.IVideoStreamReportCB;
//import com.quanta.qtalk.media.video.MediaCodecEncodeTransform;
//import com.quanta.qtalk.media.video.MjpegDecodeTransform;
//import com.quanta.qtalk.media.video.MjpegEncodeTransform;
import com.quanta.qtalk.media.video.OpenH264EncodeTransform;
import com.quanta.qtalk.media.video.VideoLevelID;
import com.quanta.qtalk.media.video.VideoLocalService;
import com.quanta.qtalk.media.video.VideoRenderMediaCodec;
import com.quanta.qtalk.media.video.VideoRenderPlus;
import com.quanta.qtalk.media.video.VideoStreamEngineTransform;
import com.quanta.qtalk.media.video.X264ServiceTransform;
import com.quanta.qtalk.media.video.X264Transform;
import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.ui.QTReceiver;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.IniFile;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.util.PortManager;

public class MediaEngine 
{
    private static final boolean ENABLE_OPENH264 = true;
    private boolean mMuteState = false;
    private boolean mVideoMuteState = false;
    private boolean mHoldState = false;
    private boolean mSpeakerState = false;
    private Bitmap  mMuteBitmap = null;
    private VideoLevelID levelid = new VideoLevelID();
    public interface VideoListener
    {
        void setVideoSSRC(long ssrc);
        void setCameraMode(boolean isDualCamera,short mode);
        void setLocalResolution(int width,int height);
    }
    public interface AudioListener
    {
        void setAudioSSRC(long ssrc);
    }
    public static final int DEFAULT_AUDIO_PORT = 6666;
    public static final int DEFAULT_VIDEO_PORT = 4444;
    public static MDP[] getAudioMDP()
    {
        MDP[] result = {//new MDP(G7221EncoderTransform.CODEC_NAME,G7221EncoderTransform.DEFAULT_ID,16000, 32000), //G7221
                        new MDP(SpeexEncoderTransform.CODEC_NAME,SpeexEncoderTransform.DEFAULT_ID,8000),  // SPEEX
                        new MDP(G711EncoderTransform.CODEC_NAMES[0],G711EncoderTransform.DEFAULT_IDS[0],8000), // PCMU
                        new MDP(G711EncoderTransform.CODEC_NAMES[1],G711EncoderTransform.DEFAULT_IDS[1],8000),// PCMA
                        new MDP(OpusEncoderTransform.CODEC_NAME,OpusEncoderTransform.DEFAULT_ID,16000),
                        new MDP("telephone-event", 101, 8000) // outband DTMF support
                        };
        return result;
    }
    public static int getAudioPort()
    {
        return PortManager.getAudioPort();//DEFAULT_AUDIO_PORT;
    }
    public static int getNextAudioPort()
    {
        return PortManager.getNextAudioPort();//DEFAULT_AUDIO_PORT;
    }
    public static MDP[] getVideoMDP()
    {
        MDP[] result = { 
                         new MDP(X264Transform.CODEC_NAME,X264Transform.DEFAULT_ID,90000)//, // must be assigned 1 in native layer, packtize-mode:0
                         //new MDP(X264Transform.CODEC_NAME,X264Transform.DEFAULT_ID+1,90000)  //packtize-mode:1
                        };
        return result;
    }
    public static int getVideoPort()
    {
        return PortManager.getVideoPort();//DEFAULT_VIDEO_PORT;
    }
    
    public static int getNextVideoPort()
    {
        return PortManager.getNextVideoPort();//DEFAULT_VIDEO_PORT;
    }
    
    public MediaEngine()
    {
    }

    private static final String DEBUGTAG		  = "QSE_MediaEngine";
    private static final int AUDIO_FORMATE_TYPE   = AudioFormat.ENCODING_PCM_16BIT; 
    private static final int NUM_CHANNELS         = AudioFormat.CHANNEL_CONFIGURATION_MONO;

    private static final int VIDEO_FORMATE_TYPE   = PixelFormat.YCbCr_420_SP;
    private static final int CIF_WIDTH            = 352;
    private static final int CIF_HEIGHT           = 288;
    private static final int QVGA_WIDTH           = 320;
    private static final int QVGA_HEIGHT          = 240;
    private static final int QCIF_WIDTH           = 176;
    private static final int QCIF_HEIGHT          = 144;
    private static final int VGA_WIDTH            = 640;
    private static final int VGA_HEIGHT           = 480;
    private static final int WIDTH                = QVGA_WIDTH;
    private static final int HEIGHT               = QVGA_HEIGHT; 
    private String             mAudioDestIP       = "127.0.0.1";//"192.168.18.142";
    private boolean            mEnableTFRC        = true;
    private int                mAudioListenPort   = 6666;
    private int                mAudioDestPort     = 6666;
    private String             mAudioMimeType     = "SPEEX";//"PCMU";
    private int                mAudioPayloadId    = 97;//0;
    private int                mAudioSampleRate   = 8000;
    private int                mAudioBitRate      = 0;
    private boolean            mEnableAudioEngine = false;

    private String             mVideoDestIP       = "127.0.0.1";//"192.168.18.142";
    private int                mVideoDestPort     = 4444;
    private int                mVideoListenPort   = 4444;
    private String             mVideoMimeType     = "H264";//"MJPEG";
    private int                mVideoPayloadId    = 109;//110;
    private int                mVideoSampleRate   = 90000;
    private boolean            mEnableVideoEngine = false;

    private VideoLocalService			mVideoService      = null;
    private IAudioService				mAudioService      = null;
    
    private boolean						mStartAudio        = false;
    private boolean						mStartVideo        = false;

    private long						mAudioSSRC         = 0;
    private long						mVideoSSRC         = 0;

    private VideoRenderMediaCodec		mRemoteRender      = null;
    private SurfaceView					mLocalView         = null;
    private CameraSourcePlus			mCamerSource       = new CameraSourcePlus();
    private CameraSourcePlusWith1832	mCameraSource1832 = new CameraSourcePlusWith1832();
    private int							mCameraIndex       = 1;
    
    private VideoListener		mVideoListener     = null;
    private AudioListener		mAudioListener     = null;
    private boolean				mIsDualCamera      = false;
    private Intent				mAudioServiceIntent= null;
    private Intent				mVideoServiceIntent= null;
    private IDemandIDRCB		mDemandIDRCB       = null;
    
    private int					mWidth             = 0;
    private int					mHeight            = 0;
    private int					mFPS               = 15;
    private int					mKbps              = 350;
    private boolean				mEnablePLI         = false;
    private boolean				mEnableFIR         = false;
    private boolean				mLetGopEqFps       = false;
    private boolean				mEnableTelEvt      = false;
    
    private int					mAudioFECCtrl			= 0;		//eason - FEC info
    private int					mAudioFECSendPType		= 0;
    private int					mAudioFECRecvPType		= 0;
    private int					mVideoFECCtrl			= 0;
    private int					mVideoFECSendPType		= 0;
    private int					mVideoFECRecvPType		= 0;
    
    private boolean 			menableAudioSRTP = false;      //Iron  - SRTP info
    private String   			mAudioSRTPSendKey = null;
    private String   			mAudioSRTPReceiveKey = null;
    private boolean 			menableVideoSRTP = false;
    private String   			mVideoSRTPSendKey = null;
    private String   			mVideoSRTPReceiveKey = null;
    
    private int					mProductType        = 0;
    
  //eason - video stream provision data
    private int					mOutMaxBandwidth = 0;
    private boolean				mAutoBandwidth = false;
    private int					mVideoMTU = 0;
    private int					mAudioRTPTOS = 0;
    private int					mVideoRTPTOS = 0;
    private int					mNetworkType = ConnectivityManager.TYPE_DUMMY;
    
    private boolean				mEnableCameraMute = false;
    private Activity			mActivity = null;
    private Context				mContext = null;
    
    private ServiceConnection mAudioConnection = new ServiceConnection() 
    {
        public void onServiceConnected(ComponentName className, IBinder service) {
            mAudioService = IAudioService.Stub.asInterface(service);
            if (mStartAudio == true) {
                _startAudioEngine();
            }
            else {
                _stopAudioEngine();
            }
        }

        public void onServiceDisconnected(ComponentName className) {
            _stopAudioEngine();
            mAudioService = null;
        }
    };
    
    private ServiceConnection mVideoConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
            mVideoService = ((VideoLocalService.LocalBinder)service).getService();
            if (mStartVideo == true) {
                _startVideoEngine();
            }
            else {
                _stopVideoEngine();
            }
        }

        public void onServiceDisconnected(ComponentName className) {
            _stopVideoEngine();
            mVideoService = null;
        }
    };    

    private synchronized void _startAudioEngine()
    {
    	Log.d(DEBUGTAG,"_startAudioEngine");
        try
        {
            if (mAudioService!=null && !mAudioService.isStarted())
            {          	
                mAudioService.reset();
                mAudioService.setSource(MicSource.class.getName(), AUDIO_FORMATE_TYPE, mAudioSampleRate, NUM_CHANNELS);

                if(Hack.isEnableSWAEC() == true) {
                	Log.d(DEBUGTAG,"AecMicTransform");
                	mAudioService.addTransform(AecMicTransform.class.getName(), false);
                }
                
                if (mAudioMimeType.compareToIgnoreCase(PCMUEncoderTransform.CODEC_NAME)==0)
                    mAudioService.addTransform(PCMUEncoderTransform.class.getName(),false);
                else if (mAudioMimeType.compareToIgnoreCase(PCMAEncoderTransform.CODEC_NAME)==0)
                    mAudioService.addTransform(PCMAEncoderTransform.class.getName(),false);
                else if (mAudioMimeType.compareToIgnoreCase(SpeexEncoderTransform.CODEC_NAME)==0)
                    mAudioService.addTransform(SpeexEncoderTransform.class.getName(),false);
                else if (mAudioMimeType.compareToIgnoreCase(G7221EncoderTransform.CODEC_NAME)==0)
                    mAudioService.addTransform(G7221EncoderTransform.class.getName(),false);
                else if (mAudioMimeType.compareToIgnoreCase(OpusEncoderTransform.CODEC_NAME)==0)
                    mAudioService.addTransform(OpusEncoderTransform.class.getName(),false);
                else ;
                
                mAudioService.addTransform(AudioStreamEngineTransform.class.getName(),true);

                if (mAudioMimeType.compareToIgnoreCase(G711DecoderTransform.CODEC_NAMES[0])==0 || 
                    mAudioMimeType.compareToIgnoreCase(G711DecoderTransform.CODEC_NAMES[1])==0)
                    mAudioService.addTransform(G711DecoderTransform.class.getName(),false);
                else if (mAudioMimeType.compareToIgnoreCase(SpeexDecoderTransform.CODEC_NAME)==0)
                    mAudioService.addTransform(SpeexDecoderTransform.class.getName(),false);
                else if (mAudioMimeType.compareToIgnoreCase(G7221DecoderTransform.CODEC_NAME)==0)
                    mAudioService.addTransform(G7221DecoderTransform.class.getName(),false);
                else if (mAudioMimeType.compareToIgnoreCase(OpusDecoderTransform.CODEC_NAME)==0)
                    mAudioService.addTransform(OpusDecoderTransform.class.getName(),false);
                else ;
                
                if(Hack.isEnableSWAEC() == true) {
                	Log.d(DEBUGTAG,"AecSpeakerTransform");
                	mAudioService.addTransform(AecSpeakerTransform.class.getName(), false);
                }
                
                mAudioService.setSink(AudioRender.class.getName());

                ConnectivityManager connMgr = null;
                NetworkInfo         networkInfo = null;
                
                if (mActivity != null) {
                	connMgr = (ConnectivityManager)mActivity.getSystemService(Context.CONNECTIVITY_SERVICE);
                }
                
                if (connMgr != null) {
                    networkInfo = connMgr.getActiveNetworkInfo();
                    if (networkInfo != null) {
                        mNetworkType = networkInfo.getType();
                    }
                }

                mAudioService.start(mAudioSSRC,
                                    mAudioListenPort, 
                                    mAudioDestIP,mAudioDestPort,
                                    mAudioSampleRate,mAudioPayloadId,
                                    mAudioMimeType,mEnableTelEvt,
                                    mAudioFECCtrl, mAudioFECSendPType, mAudioFECRecvPType, 
                                    menableAudioSRTP, mAudioSRTPSendKey, mAudioSRTPReceiveKey,
                                    mAudioRTPTOS, mEnableVideoEngine, mNetworkType, mProductType);
                mAudioSSRC = mAudioService.getSSRC();
                if (mAudioListener!=null) {
                    mAudioListener.setAudioSSRC(mAudioSSRC);
                }
                
                if (mAudioService!=null) {
                    mAudioService.setHold(mHoldState);
                    mAudioService.setMute(mMuteState);
                    mAudioService.enableLoudeSpeaker(mSpeakerState);
                }
            }  
        }
        catch(java.lang.Throwable e)
        {
        	Log.e(DEBUGTAG,"_startAudioEngine", e);
            if (mAudioService!=null)
            {
                try {
                    mAudioService.stop();
                } catch (java.lang.Throwable e1) {
                    Log.e(DEBUGTAG,"_StartAudio", e1);
                }
            }
        }
    }

    public synchronized void setCameraIndex(int index)
    {
        if (mCamerSource!=null && mIsDualCamera)
        {
            mCameraIndex = index;
            try {
                if(mWidth != 0 && mHeight != 0)
                    mCamerSource.setupCamera(VIDEO_FORMATE_TYPE, 
                                             mWidth,
                                             mHeight,
                                             mCameraIndex,
                                             mFPS,
                                             mEnableCameraMute);
                else
                    mCamerSource.setupCamera(VIDEO_FORMATE_TYPE, 
                                             WIDTH,
                                             HEIGHT,
                                             mCameraIndex,
                                             mFPS,
                                             mEnableCameraMute);
            } 
            catch (FailedOperateException e) {
                Log.e(DEBUGTAG,"_startVideoEngine", e);
            }
            finally {
                if (mIsDualCamera)
                    mVideoListener.setCameraMode(mIsDualCamera,(short)mCameraIndex);
                else
                    mVideoListener.setCameraMode(mIsDualCamera, (short)CameraSourcePlus.MODE_BACK);
            }
        }
    }
    
    private synchronized void _startVideoEngine()
    {
    	Log.d(DEBUGTAG,"_startVideoEngine");
        try
        {
            if (mVideoService!=null && !mVideoService.isStarted())
            {
                mVideoService.reset();
                
                //TODO check workable resolution of device
                //if resolution <= workable resolution --> apply
                Log.d(DEBUGTAG,"mWidth: "+mWidth+", mHeight: "+mHeight);
                
                if(mWidth != 0 && mHeight != 0){
                	if((Hack.isK72()|| Hack.isQF3())&& QtalkSettings.enable1832){
	                    mCameraSource1832.setupCamera(VIDEO_FORMATE_TYPE, 
					                                mWidth,
					                                mHeight,
					                                mCameraIndex,
					                                mFPS,
					                                mKbps,
					                                mEnableCameraMute);
	                    if (mCameraSource1832!=null)
	                    	mCameraSource1832.setDisplayRotation(mRotation);
                		
                	}
                	else {
	                    mCamerSource.setupCamera(VIDEO_FORMATE_TYPE, 
	                                             mWidth,
	                                             mHeight,
	                                             mCameraIndex,
	                                             mFPS,
	                                             mEnableCameraMute);
	                    if (mCamerSource!=null)
	                        mCamerSource.setDisplayRotation(mRotation);
                	}
                }
                else {
                	if((Hack.isK72()|| Hack.isQF3())&& QtalkSettings.enable1832){
	                    mCameraSource1832.setupCamera(VIDEO_FORMATE_TYPE, 
	                                             WIDTH,
	                                             HEIGHT,
	                                             mCameraIndex,
	                                             mFPS,
	                                             mKbps,
	                                             mEnableCameraMute);
	                    if (mCameraSource1832!=null)
	                    	mCameraSource1832.setDisplayRotation(mRotation);
                	}
                	else {
                		 mCamerSource.setupCamera(VIDEO_FORMATE_TYPE, 
				                                 WIDTH,
				                                 HEIGHT,
				                                 mCameraIndex,
				                                 mFPS,
				                                 mEnableCameraMute);
                         if (mCamerSource!=null)
                             mCamerSource.setDisplayRotation(mRotation);
                	}
                }
                
                if (mRemoteRender!=null)
                    mRemoteRender.setDisplayRotation(mRotation);
                
                if((Hack.isK72()|| Hack.isQF3())&& QtalkSettings.enable1832) {
                	mVideoService.setVideoSource(mCameraSource1832);
                }
                else {
                	mVideoService.setVideoSource(mCamerSource);
                }
                
//                if (mVideoMimeType.compareToIgnoreCase(MjpegEncodeTransform.CODEC_NAME)==0)
//                {
//                    mVideoService.addTransform(MjpegEncodeTransform.class.getName());
//                }
//                else if (mVideoMimeType.compareToIgnoreCase(X264ServiceTransform.CODEC_NAME)==0) {
//                    if (ENABLE_OPENH264) {
//                    	if((Hack.isK72()|| Hack.isQF3())&& QtalkSettings.enable1832) {
//                    		mVideoService.addTransform(DummyEncodeTransform.class.getName());
//                    	}
//                    	else {
//                    		mVideoService.addTransform(OpenH264EncodeTransform.class.getName());
//                    		//mVideoService.addTransform(X264ServiceTransform.class.getName());
//                    		//mVideoService.addTransform(MediaCodecEncodeTransform.class.getName());
//                    	}
//                    }
//                    else {
//                        mVideoService.addTransform(X264Transform.class.getName());
//                    }
//                }

                if (mVideoMimeType.compareToIgnoreCase(X264ServiceTransform.CODEC_NAME)==0) {
                    if (ENABLE_OPENH264) {
                        if((Hack.isK72()|| Hack.isQF3())&&QtalkSettings.enable1832) {
                            mVideoService.addTransform(DummyEncodeTransform.class.getName());
                        }
                        else {
                            mVideoService.addTransform(OpenH264EncodeTransform.class.getName());
                        }
                    }
                    else {
                        mVideoService.addTransform(X264Transform.class.getName());
                    }
                }
                mVideoService.addTransform(VideoStreamEngineTransform.class.getName());
                mVideoService.addTransform(DummyDecodeTransform.class.getName());
                mVideoService.setVideoSink(mRemoteRender);

//                mVideoService.addTransform(VideoStreamEngineTransform.class.getName());
//                if (mVideoMimeType.compareToIgnoreCase(MjpegDecodeTransform.CODEC_NAME)==0) {
//                    mVideoService.addTransform(MjpegDecodeTransform.class.getName());
//                }
//                else if (mVideoMimeType.compareToIgnoreCase(H264DecodeTransform.CODEC_NAME)==0) {
//                    if (ENABLE_FFMPEGERVICE) {
//                        mVideoService.addTransform(H264DecodeServiceTransform.class.getName());
//                    }
//                    else {
//                    	mVideoService.addTransform(DummyDecodeTransform.class.getName());
////                    	mVideoService.addTransform(H264DecodeTransform.class.getName());
//                    }
//                }
//                mVideoService.setVideoSink(mRemoteRender);

                ConnectivityManager connMgr = null;
                NetworkInfo         networkInfo = null;
                
                if (mActivity != null) {
                	connMgr = (ConnectivityManager)mActivity.getSystemService(Context.CONNECTIVITY_SERVICE);
                }
                
                if (connMgr != null) {
                    networkInfo = connMgr.getActiveNetworkInfo();
                    if (networkInfo != null) {
                        mNetworkType = networkInfo.getType();
                    }
                }
                
                mVideoService.start(mVideoSSRC,
                                    mVideoListenPort, mEnableTFRC,
                                    mVideoDestIP,mVideoDestPort,
                                    mVideoSampleRate,mVideoPayloadId,mVideoMimeType,
                                    mFPS,mKbps, mEnablePLI, mEnableFIR, mLetGopEqFps,
                                    mVideoFECCtrl, mVideoFECSendPType, mVideoFECRecvPType,
                                    menableVideoSRTP, mVideoSRTPSendKey, mVideoSRTPReceiveKey,
                                    mOutMaxBandwidth, mAutoBandwidth, mVideoMTU, 
                                    mVideoRTPTOS, mNetworkType, mProductType);
                mVideoSSRC = mVideoService.getSSRC();
                if (mVideoListener!=null) {
                    updateLocalResolution();
                    mVideoListener.setVideoSSRC(mVideoSSRC);
                    mCameraIndex = mCamerSource.getCameraMode();
                    mIsDualCamera = mCamerSource.isDualCamera();
                    mVideoListener.setCameraMode(mIsDualCamera,(short)mCameraIndex);
                }
                
                if(mVideoService != null) {
                    if (mDemandIDRCB!=null)
                        mVideoService.SetDemandIDRCB(mDemandIDRCB);
                }
            }
        }
        catch(java.lang.Throwable e) {
            Log.e(DEBUGTAG,"_startVideoEngine", e);
            if (mVideoService!=null) {
                try {
                    mVideoService.stop();
                } 
                catch (RemoteException e1) {
                    Log.e(DEBUGTAG,"Start", e);
                }
            }
        }
    }
    
    private synchronized void _stopAudioEngine()
    {
        try
        {
            if (mAudioService!=null)
                mAudioService.stop();
        }
        catch(java.lang.Throwable e) {
            Log.e(DEBUGTAG,"Stop", e);
        } 
    }
    
    private synchronized void _stopVideoEngine()
    {
        try
        {
            if (mVideoService!=null)
                mVideoService.stop();
        }
        catch(java.lang.Throwable e) {
            Log.e(DEBUGTAG,"Stop", e);
        } 
    }
    
    private synchronized void startAudioEngine(Context context)
    {
    	Log.d(DEBUGTAG,"startAudioEngine:"+!mStartAudio);
        if(!mStartAudio) {

            if (mAudioServiceIntent == null) {
                mAudioServiceIntent = new Intent();
                mAudioServiceIntent.setClass(context, AudioService.class);
            }
            context.bindService(mAudioServiceIntent, mAudioConnection, Context.BIND_AUTO_CREATE);
        }
        mStartAudio = true;
    }
    
    private synchronized void startVideoEngine(Context context)
    {
    	Log.d(DEBUGTAG,"startVideoEngine:"+!mStartVideo);

        if(!mStartVideo) {
            if (mVideoServiceIntent == null) {
                mVideoServiceIntent = new Intent();
                mVideoServiceIntent.setClass(context, VideoLocalService.class);
            }

            context.bindService(mVideoServiceIntent, mVideoConnection, Context.BIND_AUTO_CREATE);
        }
        mStartVideo = true;
    }

    private synchronized void stopAudioEngine(Context context)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"stopAudioEngine:"+mStartAudio);

        if(mStartAudio) {
            try {
                if (mAudioServiceIntent != null)
                    context.stopService(mAudioServiceIntent);
                context.unbindService(mAudioConnection);
            } catch (Throwable e) {
            }
            new Thread(new Runnable() {

                @Override
                public void run() {
                    _stopAudioEngine();
                }
            }).start();
        }
        mStartAudio = false;
    }
    
    private synchronized void stopVideoEngine(Context context)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"stopVideoEngine:"+mStartVideo);

        if(mStartVideo) {
            try {
                if (mVideoServiceIntent != null)
                    context.stopService(mVideoServiceIntent);

                context.unbindService(mVideoConnection);

            } catch (Throwable e) {
            }

            new Thread(new Runnable() {
                @Override
                public void run() {
                    _stopVideoEngine();
                }
            }).start();
        }
        mStartVideo = false;
    }
    
    public void setVideoMute(boolean doMute,Bitmap bitmap)
    {
        try
        {
            mVideoMuteState = doMute;
            if(bitmap != null)
                mMuteBitmap = bitmap;
            if (doMute)
            {
                if (mStartVideo && mVideoService != null)
                {
                	mVideoService.setMute(doMute, bitmap);
                }
            }
            else
            {
                if (mStartVideo && mVideoService != null)
                {
                    mVideoService.setMute(doMute, bitmap);
                }
            }
        }
        catch(Throwable err)
        {
            Log.e(DEBUGTAG,"setMute - ",err);
        }
    }
    
    public void setMute(boolean doMute,Bitmap bitmap)
    {
        try
        {
        	if(mMuteState == doMute)
        		return;
        	
            mMuteState = doMute;
            if(bitmap != null)
                mMuteBitmap = bitmap;
            
            if (doMute)
            {
                if (mStartAudio && mAudioService != null)
                {
                    mAudioService.setMute(doMute);
                }
                if (mStartVideo && mVideoService != null)
                {
                	mVideoService.setMute(doMute, bitmap);                	
                }
            }
            else
            {
                if (mStartAudio && mAudioService != null)
                {
                    mAudioService.setMute(doMute);
                }
                if (mStartVideo && mVideoService != null)
                {
                    mVideoService.setMute(doMute, bitmap);
                }
            }
        }
        catch(Throwable err)
        {
            Log.e(DEBUGTAG,"setMute - ",err);
        }
    }
    
    public void setHold(boolean doHold)
    {
        mHoldState = doHold;
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>setHold:"+mHoldState);

        try 
        {
            if (mStartAudio && mAudioService != null)
                mAudioService.setHold(doHold);
                
            if (mStartVideo && mVideoService != null)
                mVideoService.setHold(doHold);
        }
        catch(Throwable err)
        {
            Log.e(DEBUGTAG,"setHold - ",err);
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==setHold");
    }
    
    public void sendDTMF(final char press)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>pressDTMF:"+press);
        try 
        {
            if (mStartAudio && mAudioService != null)
                mAudioService.sendDTMF(press);
        }
        catch(Throwable err)
        {
            Log.e(DEBUGTAG,"pressDTMF - ",err);
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==pressDTMF");
    }
    
    public SurfaceView getLocalView(Context context)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"getLocalView");
        mLocalView  = mCamerSource.getPreview(context);
        return mLocalView;
    }
    
    public SurfaceView getCameraView(Context context)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"getCameraView");
        if((Hack.isK72()|| Hack.isQF3())&& QtalkSettings.enable1832){
        	return mCameraSource1832.getCameraView(context);
        }
        else {
        	return mCamerSource.getCameraView(context);
        }
    }
    
    public TextureView getH264View(Context context, Handler handler)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"getCameraView");
    	return mCameraSource1832.getH264View(context, handler);
    }
    
    public void setScale(float x_scale, float y_scale)
    {
    	if(mCamerSource!=null)
    		mCamerSource.setScale(x_scale,y_scale);
    }
    
    public void setupAudioEngine(AudioListener listener,long ssrc,int localPort,String destIP,int destPort,
    							  int payloadID, String mimeType, int sampleRate, int bitrate, boolean enableTelEvt, 
    							  int fecCtrl, int fecSendPType, int fecRecvPType, int audioRTPTOS,
    							  boolean enableAudioSRTP, String audioSRTPSendKey, String audioSRTPReceiveKey, int productType)
    {
    	// Frank Test ++
//    	sampleRate = 8000;
//    	mimeType = "PCMU";
//    	payloadID = 0;
    	// Frank Test --
    	
    	Log.l(DEBUGTAG,"setupAudioEngine(), "+"ssrc:"+ssrc+
                                      ", localPort:"+localPort+
                                      ", destIP:"+destIP+
                                      ", destPort:"+destPort+
                                      ", payloadID:"+payloadID+
                                      ", mimeType:"+mimeType+
                                      ", sampleRate:"+sampleRate+
                                      ", enableTelEvt:"+enableTelEvt+
                                      ", fecCtrl:"+fecCtrl+
                                      ", fecSendPType:"+fecSendPType+
                                      ", fecRecvPType:"+fecRecvPType+
                                      ", audioRTPTOS:"+audioRTPTOS+
                                      ", enableAudioSRTP:"+enableAudioSRTP+
                                      ", AudioSRTPSendKey:"+audioSRTPSendKey+
                                      ", AudioSRTPReceiveKey:"+audioSRTPReceiveKey+
                                      ", productType:"+productType);
    	
        mEnableAudioEngine = (localPort!=0 && destPort!=0);
        if (mEnableAudioEngine)
        {
            mAudioListenPort = localPort;
            mAudioDestIP = new String(destIP);
            mAudioDestPort = destPort;

            mAudioPayloadId = payloadID;
            mAudioMimeType = new String(mimeType);
            mAudioSampleRate = sampleRate;
            
            mAudioBitRate = bitrate;
            mAudioSSRC = ssrc;
            mAudioListener = listener;
            mEnableTelEvt = enableTelEvt;
            mAudioFECCtrl = fecCtrl;
            mAudioFECSendPType = fecSendPType;
            mAudioFECRecvPType = fecRecvPType;
            mAudioRTPTOS = audioRTPTOS;
            menableAudioSRTP = enableAudioSRTP;      
            mAudioSRTPSendKey = audioSRTPSendKey;
            mAudioSRTPReceiveKey = audioSRTPReceiveKey;
            mProductType         = productType;
            
        }
    }
    
    public void setupVideoEngine(VideoListener listener,long ssrc,boolean enableTFRC,int localPort,String destIP,int destPort,
                                 int payloadID, String mimeType, int sampleRate,VideoRenderMediaCodec remoteRender, int cameraIndex,
                                 int width,int height,int fps,int kbps, boolean enablePLI, boolean enableFIR, boolean letGopEqFps,
                                 int fecCtrl, int fecSendPType, int fecRecvPType,
                                 int outMaxBandwidth, boolean autoBandwidth, int videoMTU, int videoRTPTOS,
                                 boolean enableAudioSRTP, String audioSRTPSendKey, String audioSRTPReceiveKey,
                                 boolean enableVideoSRTP, String videoSRTPSendKey, String videoSRTPReceiveKey,
                                 boolean enableCameraMute, int productType)
    {
    	Log.l(DEBUGTAG,"setupVideoEngine(), "+"ssrc:"+ssrc+
                                    ", localPort:"+localPort+
                                    ", destIP:"+destIP+
                                    ", destPort:"+destPort+
                                    ", payloadID:"+payloadID+
                                    ", mimeType:"+mimeType+
                                    ", sampleRate:"+sampleRate+
                                    ", width:"+width+
                                    ", height:"+height+
                                    ", fps:"+fps+
                                    ", kbps:"+kbps+
                                    ", enablePLI:"+enablePLI+
                                    ", enableFIR:"+enableFIR+
                                    ", letGopEqFps:"+letGopEqFps+
                                    ", enableTFRC:"+enableTFRC+
                                    ", cameraIndex:"+cameraIndex+
                                    ", fecCtrl:"+fecCtrl+
                                    ", fecSendPType:"+fecSendPType+
                                    ", fecRecvPType:"+fecRecvPType+
                                    ", outMaxBandwidth:"+outMaxBandwidth+
                                    ", autoBandwidth:"+autoBandwidth+
                                    ", videoMTU:"+videoMTU+
                                    ", videoRTPTOS:"+videoRTPTOS+
                                    ", enableAudioSRTP:"+enableAudioSRTP+
                                    ", AudioSRTPSendKey:"+audioSRTPSendKey+
                                    ", AudioSRTPReceiveKey:"+audioSRTPReceiveKey+
                                    ", enableVideoSRTP:"+enableVideoSRTP+
                                    ", VideoSRTPSendKey:"+videoSRTPSendKey+
                                    ", VideoSRTPReceiveKey:"+videoSRTPReceiveKey+
                                    ", enableCameraMute:"+enableCameraMute+
                                    ", productType:"+productType);
        
        mEnableVideoEngine = (localPort!=0 && destPort!=0);
        if(mEnableVideoEngine)
        {
            mVideoListenPort = localPort;
            mVideoDestIP = new String(destIP);
            mVideoDestPort = destPort;
            mVideoPayloadId = payloadID;
            mVideoMimeType = new String(mimeType);
            mVideoSampleRate = sampleRate;
            mRemoteRender = remoteRender;
            mVideoSSRC = ssrc;
            mEnableTFRC = enableTFRC;
            mCameraIndex = cameraIndex;
            mVideoListener = listener;
            mWidth = width;
            mHeight = height;
            mFPS = fps;
            mKbps = kbps;
            mEnablePLI = enablePLI;
            mEnableFIR = enableFIR;
            mLetGopEqFps = letGopEqFps; // if remote is PX1 mLetGopEqFps = true;
            mVideoFECCtrl = fecCtrl;
            mVideoFECSendPType = fecSendPType;
            mVideoFECRecvPType = fecRecvPType;
            //mOutMaxBandwidth = outMaxBandwidth;
            mOutMaxBandwidth = kbps;
            mAutoBandwidth = autoBandwidth;
            mVideoMTU = videoMTU;
            mVideoRTPTOS = videoRTPTOS;
            menableAudioSRTP = enableAudioSRTP;      
            mAudioSRTPSendKey = audioSRTPSendKey;
            mAudioSRTPReceiveKey = audioSRTPReceiveKey;
            menableVideoSRTP = enableVideoSRTP;      
            mVideoSRTPSendKey = videoSRTPSendKey;
            mVideoSRTPReceiveKey = videoSRTPReceiveKey;
            mEnableCameraMute = enableCameraMute;
            mProductType = productType;
            
//            if(mOutMaxBandwidth>levelid.bit_rate){
//            	mOutMaxBandwidth = levelid.bit_rate;
//            }

//            if(mWidth==1280 && levelid.width == 1280){
//            	mOutMaxBandwidth = 2000;
//            }
        }
    }
    
    public void start(Activity activity)
    {	
    	mActivity = activity;
    	
    	//_Start();
    	
        mMuteState = false;
        mHoldState = false;	    
        mRotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();
        
        if (mEnableAudioEngine)
        	startAudioEngine(mActivity);

        if (mEnableVideoEngine)
            startVideoEngine(mActivity);        
        
        // Frank Add ++       
        Log.d(DEBUGTAG, "setVolumeControlStream: "+Hack.getAudioStreamType());
        mActivity.setVolumeControlStream(Hack.getAudioStreamType());
        if ((mEnableVideoEngine)||(mProductType == 8)) { // mProductType 8 (Push to talk), need enable loud speaker
        	enableLoudeSpeaker(mActivity, true);
        }
        else {
        	if(mSpeakerState)
        		enableLoudeSpeaker(mActivity, true);
        	else
        		enableLoudeSpeaker(mActivity, false);
        }
        // Frank Add --
    }
    
    public boolean enableLoudeSpeaker(Activity activity,final boolean enable)
    {
    	Log.d(DEBUGTAG, "enableLoudeSpeaker: "+enable);
        mSpeakerState = enable;
        try
        {
            if(mAudioService != null)
                mAudioService.enableLoudeSpeaker(enable);
            else
            	Log.d(DEBUGTAG,"mAudioService is null");
        }
        catch(RemoteException e)
        {
            Log.e(DEBUGTAG,"enableLoudeSpeaker", e);
        }
        return false;
    }
    public void stop(Context context)
    {
    	mActivity = null;
    	mContext = context;
    	
    	//_Stop();


        if (mEnableAudioEngine)
            stopAudioEngine(mContext);

        if (mEnableVideoEngine)
            stopVideoEngine(mContext);
    }
    
    public void adjuestVolume(int direction)
    {
    	// Frank test ++
//        try {
//            final float current = mAudioManager.getStreamVolume(Hack.getAudioStreamType());
//            final float max = mAudioManager.getStreamMaxVolume(Hack.getAudioStreamType());
//            final float new_volume = current/max;
//            
//            Log.d(DEBUGTAG,"adjuestVolume:"+current+"/"+max+" = "+new_volume);
//            if(mAudioService != null)
//                mAudioService.adjectVolume(new_volume);
//        }catch (RemoteException e) {
//            Log.e(DEBUGTAG,"Start", e);
//        }
    	// Frank test --
    }
    
    public long getAudioSSRC()
    {
        return mAudioSSRC;
    }
    public long getVideoSSRC()
    {
        return mVideoSSRC;
    }
    private void updateLocalResolution()
    {
//        if (mCamerSource!=null)
//        {
//            Size size = mCamerSource.getResolution();
//            int degree = mCamerSource.getRotationDegree();
//            if (size!=null && mVideoListener!=null)
//            {
//                if(degree == 90 || degree == 270)
//                {
//                    mVideoListener.setLocalResolution(size.height,size.width);
//                }         
//                else
//                {
//                    mVideoListener.setLocalResolution(size.width,size.height);
//                }
//            }
//        }
    }
    private int mRotation = 0;
    public void setDisplayRotation(int rotation) 
    {
        if (mRotation!=rotation)
        {
            mRotation = rotation;
            if (mRemoteRender!=null)
                mRemoteRender.setDisplayRotation(rotation);
            if (mCamerSource!=null)
            {
                mCamerSource.setDisplayRotation(rotation);
                updateLocalResolution();
            }
        }
    }
    public void setVideoReportListener( IVideoStreamReportCB cbListener)
    {
        if (mRemoteRender!=null)
            mRemoteRender.setVideoReportListener(cbListener);
    }
    public void reset()
    {
        mEnableAudioEngine = false;
        mEnableVideoEngine = false;
    }
    public boolean checkConnection()
    {
        if(mAudioService != null)
        {
            try
            {
                if (!mHoldState)
                    return mAudioService.checkConnection();
            } catch (RemoteException e)
            {
                Log.e(DEBUGTAG,"Start", e);
            }
        }
        return true;
    }
    public void RequestIDR()
    {
        //remote side request new IDR
        if(mVideoService != null)
        {
            mVideoService.requestIDR();
        }
    }
    public void SetDemandIDRCB(IDemandIDRCB callback) throws FailedOperateException
    {
        mDemandIDRCB = callback;
        //setup the callback to video service as IDR demand listener
        if(mVideoService != null)
        {
            mVideoService.SetDemandIDRCB(callback);
        }
    }
    
/*
    static
    {
        System.loadLibrary("stream_engine_service");
    }
    
    private static native void _Start();
    private static native void _Stop();
*/
}
