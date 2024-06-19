package com.quanta.qtalk.media.audio;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.nio.ByteBuffer;
import java.util.Enumeration;
import java.util.Vector;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;

import com.quanta.qtalk.media.audio.StatisticAnalyzer;
import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class AudioStreamEngineTransform implements IAudioTransform, IAudioStreamEngine, IAudioDTMFSender
{
     private final static String TAG = "AudioStreamEngineTransform";
     private static final String DEBUGTAG = "QTFa_AudioStreamEngineTransform";
     private final static int[]     SUPPORTED_FORMATS = {myAudioFormat.PCMU,
    		                                             myAudioFormat.PCMA,
    		                                             myAudioFormat.SPEEX,
    		                                             myAudioFormat.G7221,
    		                                             myAudioFormat.OPUS};
     private int         mFormatIndex = -1;
     private int         mSampleRate   = 0;
     private int         mNumChannels  = 0;
     
     private IAudioSink mAudioSink = null;
     
     private int         mListenPort= 0;
     private String     mDestIP    = "127.0.0.1";
     private int         mDestPort  = 0;
     private boolean    mEnableTFRC= false;
     private int        mProductType = 0;
     
     private int        mPayloadID = 0;
     private String     mMimeType  = null;
     private boolean    mEnableTelEvt = false;
     
     private int		mFECCtrl = 0;			//eason - FEC info
     private int		mFECSendPType = 0;
     private int		mFECRecvPType = 0;
     
     private boolean 	menableAudioSRTP = false;      //Iron  - SRTP info
     private String   	mAudioSRTPSendKey = null;
     private String   	mAudioSRTPReceiveKey = null;
     
     private int		mAudioRTPTOS = 0;		//eason - provision data
     private int		mNetworkType = 0;
     
     private long       mSSRC      = 0;
     
     private IAudioStreamReportCB mReportCB = null;
     
     private   Vector<Integer> mDTMFVector      = new Vector();
	 private boolean mHasVideo = false;
     AudioStreamEngineTransform(){}
     public static String getLocalIP()
     {
          try
          {
               for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();) 
             {
                    NetworkInterface intf = en.nextElement();
                    for (Enumeration<InetAddress> enumIpAddr =intf.getInetAddresses(); enumIpAddr.hasMoreElements();) 
                    {
                         InetAddress inetAddress = enumIpAddr.nextElement();
                         if (!inetAddress.isLoopbackAddress()) 
                         {
                        return inetAddress.getHostAddress().toString();
                     }
                    }
             }
          } 
          catch(java.lang.Throwable e)
          {
               Log.e(DEBUGTAG,"getLocalIP", e);
          }
          return null;
     }
     
     //=============Begin IAudioTransform=============
     @Override
     public void setSink(IAudioSink sink) throws FailedOperateException,UnSupportException 
     {
          boolean restart = false;
          try
          {
               //Check iif the stream engine is already on?
               if (mHandle!=0)
               {
//                    mSSRC = _GetSSRC(mHandle);
                    restart = true;
                    stop();
               }               
               // Set output format to the Sink
               if (sink!=null && mSampleRate!=0 && mNumChannels!=0 && mFormatIndex!=-1)
                    sink.setFormat(SUPPORTED_FORMATS[mFormatIndex], mSampleRate, mNumChannels);
               
               mAudioSink = sink;
               if (restart)
               {
                    start();
               }
          }catch(java.lang.Throwable e)
          {
               Log.e(DEBUGTAG,"setSink", e);
               throw new FailedOperateException(e);
          }
     }

     @Override
     public void setFormat(int format, int sampleRate, int channels) throws UnSupportException ,FailedOperateException
     {
          //Check iif input stream format
    	 if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setFormat:");
         boolean formatChanged = (mFormatIndex==-1);
         if (mFormatIndex!=-1 &&
              SUPPORTED_FORMATS[mFormatIndex]!=format)
         {
              formatChanged = true;
         }
         mFormatIndex = -1;
         for (int i=0;i<SUPPORTED_FORMATS.length;i++)
         {
              if (format == SUPPORTED_FORMATS[i])
              {
                   mFormatIndex = i;
              }
         }
         if (mFormatIndex==-1)
               throw new UnSupportException("Failed on setFormat: Not support video format type:"+format);

          if (formatChanged || mSampleRate != sampleRate || mNumChannels != channels)
          {     //Media format changed
               mSampleRate = sampleRate;
               mNumChannels = channels;
               //Trigger the sink to update the format information
               setSink(mAudioSink);
          }
     }

     @Override
     public void start() throws FailedOperateException 
     {     
    	 if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "start");
          //synchronized to protect mHandle
         try
          {
              synchronized(this)
               {
                    if (mAudioSink!=null && mHandle==0)
                    {
                         mHandle = _Start(mSSRC,
                                              mAudioSink,
                                              mListenPort, 
                                              mDestIP, mDestPort,
                                              mSampleRate,mPayloadID,mMimeType,mNumChannels,
                                              mFECCtrl, mFECSendPType, mFECRecvPType, 
                                              menableAudioSRTP, mAudioSRTPSendKey, mAudioSRTPReceiveKey,
                                              mAudioRTPTOS,
                                              mHasVideo, mNetworkType, mProductType);
//                         if (mReportCB!=null)
//                              _SetReportCB(mHandle,mReportCB);
                    }
               }
          }catch(java.lang.Throwable e)
          {
               stop();
               if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"Failed on Start :"+
                         "local port:"+mListenPort+", tfrc:"+ mEnableTFRC+
                         "remote port:"+mDestPort+", remote IP:"+ mDestIP+
                         "sampleRate :"+mSampleRate+", payloadID:"+mPayloadID+", Mime:"+mMimeType
                         , e);
               throw new FailedOperateException(e);
          }
          
     }

     @Override
     public void stop()
     {     
    	 if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "stop");
          //synchronized to protect mHandle
          try
          {
              synchronized(this)
               {
                    if (mHandle!=0)
                    {
                         _Stop(mHandle);
                         mHandle = 0;
                    }
               }
          }
          catch(java.lang.Throwable e)
          {
               Log.e(DEBUGTAG,"stop", e);
          }
     }

     @Override
     public void onMedia(short[] data, int length, long timestamp) throws UnSupportException {
          throw new UnSupportException("onMedia(short[],length,long) isn't supported, please use onMedia(java.nio.ByteBuffer,int,long)");
     }
     @Override
     public void onMedia(final ByteBuffer data,final int length,final long timestamp)
     {
          try
          {
               if (data!=null)
               {     //synchronized to protect mHandle
                    if (mHandle!=0)
                    {
                        int dtmf_key = -1;
                        if(mDTMFVector.size() > 0)
                        {
                            dtmf_key = mDTMFVector.firstElement();
                            mDTMFVector.removeElementAt(0);
                            //Log.d(DEBUGTAG, "onMedia - dtmf_key:"+dtmf_key);
                        }
                        if (data.isDirect())
                            _OnSendDirect(mHandle, data,length, System.currentTimeMillis(), dtmf_key);
                        else
                            _OnSend(mHandle, data.array(),length, System.currentTimeMillis(), dtmf_key);
                        if (StatisticAnalyzer.ENABLE)
                            StatisticAnalyzer.onSendSuccess();
                    }
                    else
                    	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG, "Failed on onMedia causes:mHandle == null");
               }else
               {
            	   if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"Failed on onMedia causes: input data is null");
               }
          }catch(java.lang.Throwable e)
          {
               Log.e(DEBUGTAG,"onMedia", e);
          }     
     }
     //=============End IAudioTransform=============
     //=============Begin IAudioStreamEngine=============     
     @Override
     public void setListenerInfo(int rtpListenPort) throws FailedOperateException 
     {
          if (mHandle!=0)
               throw new FailedOperateException("Please call this method before start method is call");
          mListenPort = rtpListenPort;
     }


     @Override
     public void setRemoteInfo(long ssrc,String destIP, int destPort, int productType) throws FailedOperateException 
     {
          if (mHandle!=0)
               throw new FailedOperateException("Please call this method before start method is call");
          mSSRC = ssrc;
          mDestIP = new String(destIP);
          mDestPort = destPort;
          mProductType = productType;
     }


     @Override
     public void setMediaInfo(int payloadID, String mimeType, boolean enableTelEvt) throws FailedOperateException 
     {
          if (mHandle!=0)
               throw new FailedOperateException("Please call this method before start method is call");
          mPayloadID  = payloadID;
          mMimeType   = new String(mimeType);
          mEnableTelEvt = enableTelEvt;
     }
     
     @Override
     public void setFECInfo(int fecCtrl, int fecSendPType, int fecRecvPType) throws FailedOperateException{
          if (mHandle!=0)
               throw new FailedOperateException("Please call this method before start method is call");
          mFECCtrl = fecCtrl;
          mFECSendPType = fecSendPType;
          mFECRecvPType = fecRecvPType;
     }
     
     public void setAudioRTPTOS(int audioRTPTOS) throws FailedOperateException{
    	 if (mHandle!=0)
             throw new FailedOperateException("Please call this method before start method is call");
    	 mAudioRTPTOS = audioRTPTOS;
     }

     @Override
     public void setReportCallback(IAudioStreamReportCB callback) throws FailedOperateException
     {
//          mReportCB = callback;
//          if (mHandle!=0)
//          {
//               try
//               {
//                    _SetReportCB(mHandle,mReportCB);
//               }catch(java.lang.Throwable e)
//               {
//                    Log.e(DEBUGTAG,"getSSRC", e);
//                    throw new FailedOperateException(e);
//               }
//          }
     }
     
     @Override
     public long getSSRC()  throws FailedOperateException
     {
//          if (mHandle==0)
//               throw new FailedOperateException("Please call this method after start method is call");
//          if (mHandle!=0)
//          {
//               try
//               {
//                    mSSRC = _GetSSRC(mHandle);
//               }catch(java.lang.Throwable e)
//               {
//                    Log.e(DEBUGTAG,"getSSRC", e);
//                    throw new FailedOperateException(e);
//               }
//          }
          return mSSRC;
     }
     
     @Override
     public void setHold(boolean enable)
     {
         if (mHandle!=0 )
         {
           	  _SetHold(mHandle, enable);
         } 	 
     }
     
     @Override
     public void setHasVideo(boolean hasVideo)
     {
          mHasVideo = hasVideo;
     }
     
     @Override
     public void setNetworkType(int networkType)
     {
          mNetworkType = networkType;
     }
     
     //=============End IAudioStreamEngine=============     
     //=============Start IAudioDTMFSender=============
     @Override
     public void onDTMFSend(int dtmfKey)
     {
         //add dtmf key to dtmf queue
         //Log.d(DEBUGTAG, "onDTMFSend:"+dtmfKey);
         if(mEnableTelEvt)
             mDTMFVector.addElement(dtmfKey);
     }
     //=============End IAudioDTMFSender=============

     private static void onMediaNative(final com.quanta.qtalk.media.audio.IAudioSink sink,final ByteBuffer input,final int length,final long timestamp)
     {
          //Log.d(DEBUGTAG,"onMediaNative length"+length);
          if (sink!=null)
          {
               try
               { 
                    sink.onMedia(input, length, timestamp);
                    if (StatisticAnalyzer.ENABLE)
                        StatisticAnalyzer.onRecvSuccess();
               }catch(java.lang.Throwable e)
               {
                    Log.e(DEBUGTAG,"onMediaNative", e);
               }
          }
     }
     private static void onReportNative(final IAudioStreamReportCB reportCB,
                                        final int kbpsRecv,final int kbpsSend,
                                        final int fpsRecv,final int fpsSend,
                                        final int secGapRecv,final int secGapSend)
     {
         //Log.d(DEBUGTAG,"onReportNative");
         if (reportCB!=null)
          {
               try
               {
                    reportCB.onReport(kbpsRecv, kbpsSend,fpsRecv,fpsSend,secGapRecv,secGapSend);
               }catch(java.lang.Throwable e)
               {
                    Log.e(DEBUGTAG,"onReportNative", e);
               }
          }
     }
     static
     {
        System.loadLibrary("media_audio_stream_engine_transform");
    }
     private int mHandle = 0;
     //private static native void _SetReportCB(int handle, IAudioStreamReportCB reportCB);
     private static native int  _Start(long ssrc,
                                      IAudioSink sink,
                                      int listentPort,
                                      String destIP, int destPort,
                                      int sampleRate,int payloadID, String mimeType,int channels,
                                      int fecCtrl, int fecSendPType, int fecRecvPType, 
                                      boolean enableAudioSRTP, String audioSRTPSendKey, String audioSRTPReceiveKey,
                                      int audioRTPTOS, boolean hasVideo,
                                      int networkType, int productType);			//eason - FEC info   //Iron - SRTP info
     private static native void _Stop(int handle);
     //private static native long _GetSSRC(int handle);
     private static native void _OnSendDirect(final int handle,final ByteBuffer input,final int length,final long timestamp,final int dtmfKey);
     private static native void _OnSend(final int handle,final byte[] input,final int length,final long timestamp,final int dtmfKey);
     private static native void _SetHold(final int handle,final boolean enable);
	@Override
	public void setSRTPInfo(boolean enableAudioSRTP, String AudioSRTPSendKey,
			String AudioSRTPReceiveKey) throws FailedOperateException {
		if (mHandle!=0)
            throw new FailedOperateException("Please call this method before start method is call");
		menableAudioSRTP = enableAudioSRTP;
		mAudioSRTPSendKey = AudioSRTPSendKey;
		mAudioSRTPReceiveKey = AudioSRTPReceiveKey;
   
	}
}
