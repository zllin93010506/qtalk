package com.quanta.qtalk.media.video;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class VideoStreamEngineTransform implements IVideoTransform, IVideoStreamEngine
{
    private final static String TAG = "VideoStreamEngineTransform";
    private static final String DEBUGTAG = "QTFa_VideoStreamEngineTransform";
    private final static int[]  SUPPORTED_FORMATS = {VideoFormat.H264,VideoFormat.MJPEG};
    private int        mFormatIndex = -1;
    private int        mInWidth   = 0;
    private int        mInHeight  = 0;
    
    private IVideoSink mVideoSink = null;
    private static IVideoSink mNativeVideoSink = null;
    private int        mOutWidth  = 0;
    private int        mOutHeight = 0;
    
    private int        mListenPort= 0;
    private String     mDestIP    = "127.0.0.1";
    private int        mDestPort  = 0;
    private boolean    mEnableTFRC= false;
    
    private int        mSampleRate= 0;
    private int        mPayloadID = 0;
    private String     mMimeType  = null;
    
    private long       mSSRC      = 0;
    private boolean    mStop      = true;
    private IVideoStreamReportCB mReportCB = null;
    private IVideoStreamSourceCB mSourceCB = null;
    private IDemandIDRCB mDemandIDRCB = null;
    private int        mInitialBitrate = 350;
    private int        mFPS       = 15;
    private boolean    mEnablePLI = false;
    private boolean    mEnableFIR = false;
    private boolean    mLetGopEqFps = false;
    private int        mProductType = 0;
    
    private int		mFECCtrl = 0;			//eason - FEC info
    private int		mFECSendPType = 0;
    private int		mFECRecvPType = 0;
    
    private boolean 	menableVideoSRTP = false;      //Iron  - SRTP info
    private String   	mVideoSRTPSendKey = null;
    private String   	mVideoSRTPReceiveKey = null;
    
    private int	    mOutMaxBandwidth = 700;
    private static boolean		mAutoBandwidth = false;
    private int	    mVideoMTU = 0;
    private int		mVideoRTPTOS = 0;
    private int		mNetworkType = 0;
    
    private static boolean first = true ; 
    
    VideoStreamEngineTransform(Context context)
    {
        if (context!=null)
        {
            ConnectivityManager conm = (ConnectivityManager)context.getApplicationContext().getSystemService(Context.CONNECTIVITY_SERVICE);
            if (conm!=null)
            {
                NetworkInfo ni = conm.getActiveNetworkInfo();
                if (ni!=null && ni.isConnected() && (ni.getType() == ConnectivityManager.TYPE_WIFI))
                    mInitialBitrate = mInitialBitrate*2;
            }
        }
    }
    
    //=============Begin IVideoTransform=============
    @Override
    public void setSink(IVideoSink sink) throws FailedOperateException,UnSupportException 
    {
        boolean restart = false;
        try
        {
            //Check iif the stream engine is already on?
            if (mHandle!=0)
            {
                mSSRC = _GetSSRC(mHandle);
                restart = true;
                stop();
            }
            
            // Set output format to the Sink
            if (sink!=null && mOutWidth!=0 && mOutHeight!=0 && mFormatIndex!=-1)
                sink.setFormat(SUPPORTED_FORMATS[mFormatIndex], mOutWidth, mOutHeight);
            
            mVideoSink = sink;
            mNativeVideoSink = sink;
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
    public void setFormat(final int format,final  int width,final  int height) throws UnSupportException ,FailedOperateException
    {
        //Check iif input stream format
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setFormat");
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

        if (formatChanged)
        {    //Media format changed
            mInWidth = width;
            mInHeight = height;
            //Trigger the sink to update the format information
            setSink(mVideoSink);
        }else if (mInWidth!=width || mInHeight!=height)
        {    //Resolution changed
            mInWidth = width;
            mInHeight = height;
//            try
//            {    //
//                if (mHandle!=0)
//                    _UpdateSource(mHandle, mInWidth, mInHeight);
//            }catch(java.lang.Throwable e)
//            {
//                Log.e(DEBUGTAG,"setFormat", e);
//                throw new FailedOperateException(e);
//            }
        }
    }
    
    @Override
    public void start() throws FailedOperateException 
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"VideoStreamEngine Start :"+
									                "local port:"+mListenPort+", tfrc:"+ mEnableTFRC+
									                "remote port:"+mDestPort+", remote IP:"+ mDestIP+
									                "sampleRate :"+mSampleRate+", payloadID:"+mPayloadID+", Mime:"+mMimeType + 
									                "mFPS :"+mFPS+", mInitialBitrate:"+mInitialBitrate+", mLetGopEqFps:"+mLetGopEqFps+
									                "mFECCtrl:"+mFECCtrl+ "mFECSendPType:"+mFECSendPType+ "mFECRecvPType:"+mFECRecvPType+
									                "menableVideoSRTP:"+menableVideoSRTP+"mVideoSRTPSendKey:"+mVideoSRTPSendKey+"mVideoSRTPReceiveKey:"+mVideoSRTPReceiveKey);
        //synchronized to protect mHandle
        try
        {
            synchronized(this)
            {
                if (mHandle==0)
                {
                    mStop = false;
                    mHandle = _Start(mSSRC,
                                     mVideoSink,
                                     mListenPort, mEnableTFRC,
                                     mDestIP, mDestPort,
                                     mFormatIndex,mSampleRate,mPayloadID,mMimeType,
                                     mFPS, mInitialBitrate, mEnablePLI, mEnableFIR, mLetGopEqFps,
                                     mFECCtrl, mFECSendPType, mFECRecvPType,
                                     menableVideoSRTP, mVideoSRTPSendKey, mVideoSRTPReceiveKey,
                                     mOutMaxBandwidth, mVideoMTU, mVideoRTPTOS, mNetworkType, mProductType);
                    if (mHandle == 0)
                        throw new Exception("Failed on Start VideoStreamEngine");
                    //_UpdateSource(mHandle, mInWidth, mInHeight);
                    //if (mReportCB!=null)
                    //    _SetReportCB(mHandle,mReportCB);
                    if (mSourceCB!=null)
                        _SetSourceCB(mHandle,mSourceCB);
                    if (mDemandIDRCB!=null)
                        _SetDemandIDRCB(mHandle,mDemandIDRCB);
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
            mStop = true;
            synchronized(this)
            {
                if (mHandle!=0)
                {
                    _Stop(mHandle);
                    mHandle = 0;
                }
                first = true;
            }
        }
        catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"stop", e);
        }
    }

    @Override
    public boolean onMedia(final ByteBuffer data,final int length,final long timestamp,final short rotation)
    {
    	//Log.d(DEBUGTAG,"==> [Iron_Test] onMedia");
        try
        {
            if (data!=null)
            {    //synchronized to protect mHandle
                if (mHandle!=0)
                {
                    if (data.isDirect())
                        _OnSendDirect(mHandle, data,length, timestamp,rotation);//Pass address
                    else
                        _OnSend(mHandle, data.array(),length, timestamp,rotation);//Pass data array

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
        //Log.d(DEBUGTAG,"<== [Iron_Test] onMedia");
        return true;
    }
    //=============End IVideoTransform=============
    //=============Begin IVideoStreamEngine=============    
    @Override
    public void setListenerInfo(int rtpListenPort, boolean enableTFRC)  throws FailedOperateException 
    {
        if (mHandle!=0)
            throw new FailedOperateException("Please call this method before start method is call");
        mListenPort = rtpListenPort;
        mEnableTFRC = enableTFRC;
    }


    @Override
    public void setRemoteInfo(long ssrc,String destIP, int destPort ,int fps, int kbps, boolean enablePLI, boolean enableFIR, boolean letGopEqFps, int productType) throws FailedOperateException 
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"setRemoteInfo :"+
									                "ssrc:"+ssrc+", destIP:"+ destIP+
									                "destPort:"+destPort+", fps:"+ fps+
									                "kbps :"+kbps+", enablePLI:"+enablePLI+", enableFIR:"+enableFIR + 
									                "letGopEqFps :"+letGopEqFps);
        
        if (mHandle!=0)
            throw new FailedOperateException("Please call this method before start method is call");
        mSSRC = ssrc;
        mDestIP = destIP;
        mDestPort = destPort;
        mFPS = fps;
        mInitialBitrate = kbps;
        mEnablePLI = enablePLI;
        mEnableFIR = enableFIR;
        mLetGopEqFps = letGopEqFps;
        mProductType = productType;
    }


    @Override
    public void setMediaInfo(int sampleRate, int payloadID, String mimeType) throws FailedOperateException 
    {
        if (mHandle!=0)
            throw new FailedOperateException("Please call this method before start method is call");
        mSampleRate = sampleRate;
        mPayloadID  = payloadID;
        mMimeType   = new String(mimeType);
    }
    
    public void setFECInfo(int fecCtrl, int fecSendPType, int fecRecvPType) throws FailedOperateException{
        if (mHandle!=0)
             throw new FailedOperateException("Please call this method before start method is call");
        mFECCtrl = fecCtrl;
        mFECSendPType = fecSendPType;
        mFECRecvPType = fecRecvPType;
    }
    
    public void setStreamInfo(int outMaxBandwidth, boolean autoBandwidth, int videoMTU, int videoRTPTOS) throws FailedOperateException{
        if (mHandle!=0)
             throw new FailedOperateException("Please call this method before start method is call");
        mOutMaxBandwidth = outMaxBandwidth;
        mAutoBandwidth =  autoBandwidth;
        mVideoMTU = videoMTU;
        mVideoRTPTOS = videoRTPTOS;
    }
   
    public void setNetworkType(int networkType)
    {
        mNetworkType = networkType;
    }
    
    @Override
    public void setReportCallback(IVideoStreamReportCB callback) throws FailedOperateException
    {
//        mReportCB = callback;
//        if (mHandle!=0)
//        {
//            try
//            {
//                _SetReportCB(mHandle,mReportCB);
//            }catch(java.lang.Throwable e)
//            {
//                Log.e(DEBUGTAG,"getSSRC", e);
//                throw new FailedOperateException(e);
//            }
//        }
    }
    @Override
    public void setSourceCallback(IVideoStreamSourceCB callback) throws FailedOperateException
    {
        mSourceCB = callback;
        if (mHandle!=0)
        {
            try
            {
                _SetSourceCB(mHandle,mSourceCB);
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"getSSRC", e);
                throw new FailedOperateException(e);
            }
        }
    }

    @Override
    public void setDemandIDRCallBack(IDemandIDRCB callback)
            throws FailedOperateException
    {
        mDemandIDRCB = callback;
        if (mHandle!=0)
        {
            try
            {
                _SetDemandIDRCB(mHandle,mDemandIDRCB);
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"getSSRC", e);
                throw new FailedOperateException(e);
            }
        }
    }
    
    @Override
    public long getSSRC()  throws FailedOperateException
    {
        if (mHandle==0)
            throw new FailedOperateException("Please call this method after start method is call");
        if (mHandle!=0)
        {
            try
            {
                mSSRC = _GetSSRC(mHandle);
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"getSSRC", e);
                throw new FailedOperateException(e);
            }
        }
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


	/***************************************************************************
	 *  				Public function implement
	 **************************************************************************/   
    private final static boolean onMediaNative(final com.quanta.qtalk.media.video.IVideoSink sink,final ByteBuffer input,final int length,final int timestamp,final int rotation)
    {
        boolean result = false;
        int type = -1;
        long timestamp_long = 0;
        if (sink!=null)
        {
            try
            {
                result = sink.onMedia(input, length, timestamp_long, (short)rotation);
                if (StatisticAnalyzer.ENABLE)
                    StatisticAnalyzer.onRecvSuccess();
            }
            catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"onMediaNative", e);
            }
        }
        return result;
    }
    private final static void onResolutionChangeNative(com.quanta.qtalk.media.video.IVideoSink sink,int formatIndex,int width, int height)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"setRemoteResolution:"+width+"X"+height);
        try
        {
            if (sink!=null)
            {
                sink.setFormat(SUPPORTED_FORMATS[formatIndex], width, height);
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"setResolutionNative", e);
        }
    }

    private final static void onReportNative(IVideoStreamReportCB reportCB,int kbpsRecv,byte fpsRecv,int kbpsSend,byte fpsSend,double pkloss,int usRtt)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"onReport:recv:("+kbpsRecv+"kbps,"+fpsRecv+"fps)"+
								                    "send:("+kbpsSend+"kbps,"+fpsSend+"fps)"+
								                    "pkloss:"+pkloss+",rtt:"+usRtt);
        if (reportCB!=null)
        {
            try
            {
                reportCB.onReport(kbpsRecv, fpsRecv, kbpsSend, fpsSend, pkloss, usRtt);
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"onReportNative", e);
            }
        }
    }
    private final static void setGopNative(IVideoStreamSourceCB source,byte gop)
    {
        if (source!=null)
        {
            try
            {
            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"setGopNative:"+gop);
                source.setGop(gop);
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"setGopNative", e);
            }
        }
    }
    private final static void setBitRateNative(IVideoStreamSourceCB source,int kbsBitRate)
    {
        if (source!=null && mAutoBandwidth)		
        {
            try
            {
            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"setBitRateNative:"+kbsBitRate);
                source.setBitRate(kbsBitRate);
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"setBitRateNative", e);
            }
        }
    }
    private final static void setFrameRateNative(IVideoStreamSourceCB source,byte fpsFramerate)
    {
        if (source!=null)
        {
            try
            {
            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"setFrameRateNative:"+fpsFramerate);
                source.setFrameRate(fpsFramerate);
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"setFrameRateNative", e);
            }
        }
    }
    private final static void onRequestIDRNative(IVideoStreamSourceCB source)
    {
        if (source!=null)
        {
            try
            {
            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "onRequestIDRNative");
                source.requestIDR();
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"onRequestIDRNative", e);
            }
        }
    }
    private final static void onDemandIDRNative(IDemandIDRCB callback)
    {
        if (callback!=null)
        {
            try
            {
            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "onDemandIDRNative");
                callback.onDemandIDR();
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG,"onDemandIDRNative", e);
            }
        }
    }

    private final static void setLowBandwidthNative(boolean showLowBandwidth)
    {
    	Log.d(DEBUGTAG,"StreamEngine mNativeVideoSink:"+mNativeVideoSink+", showLowBandwidth:"+showLowBandwidth);
        if (mNativeVideoSink != null)
        {
        	mNativeVideoSink.setLowBandwidth(showLowBandwidth);
        }
    }
    
    static
    {
        System.loadLibrary("media_video_stream_engine_transform");
    }
    private int mHandle = 0;
    //private static native void _UpdateSource(int handle, int width, int height);
    //private static native void _SetReportCB(int handle, IVideoStreamReportCB reportCB);
    private static native void _SetSourceCB(int handle, IVideoStreamSourceCB sourceCB);
    private static native void _SetDemandIDRCB(int handle, IDemandIDRCB sourceCB);//TODO
    private static native int  _Start(long ssrc,
                               IVideoSink sink,
                               int listentPort,boolean enableTFRC,
                               String destIP, int destPort,
                               int formatIndex,int sampleRate,int payloadID, String mimeType,
                               int initFps, int initialBitrate, boolean enablePLI, boolean enableFIR, boolean letGopEqFps,
                               int fecCtrl, int fecSendPType, int fecRecvPType,
                               boolean enableVideoSRTP, String mVideoSRTPSendKey, String mVideoSRTPReceiveKey, 
                               int outMaxBandwidth, int videoMTU, 
                               int videoRTPTOS, int networkType, int productType);			//eason - FEC info and provision data  //Iron - SRTP info
    private static native void _Stop(int handle);
    private static native long _GetSSRC(int handle);
    private static native void _OnSendDirect(final int handle,final ByteBuffer input,final int length,final long timestamp,final short rotation);
    private static native void _OnSend(final int handle,final byte[] input,final int length,final long timestamp,final short rotation);
    private static native void _SetHold(final int handle,final boolean enable);
    /*
    public void onMedia(byte[] frameData, int frameLength, long sampleTime, short rotation)
    {
        try
        {
            if (mStop && frameData!=null)
            {    //synchronized to protect mHandle
                if (mHandle!=0)
                {
                    _OnSend(mHandle, frameData,frameLength, sampleTime,rotation);//Pass data array
                }
                else
                    Log.e(DEBUGTAG, "Failed on onMedia causes:mHandle == null");
            }else
            {
                Log.e(DEBUGTAG,"Failed on onMedia causes: input data is null");
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG,"onMedia", e);
        } 
    }*/

    @Override
    public void setLowBandwidth(boolean showLowBandWidth)
    {
        return ;
    }

    @Override
    public void release()
    {
        mNativeVideoSink = null;

        mMimeType  = null;

        mReportCB = null;
        mSourceCB = null;
        mDemandIDRCB = null;
    }
    
    @Override
    public void destory()
    {
        ;
    }

	@Override
	public void setSRTPInfo(boolean enableVideoSRTP, String VideoSRTPSendKey, String VideoSRTPReceiveKey) throws FailedOperateException{
        if (mHandle!=0)
            throw new FailedOperateException("Please call this method before start method is call");
       menableVideoSRTP = enableVideoSRTP;
       mVideoSRTPSendKey = VideoSRTPSendKey;
       mVideoSRTPReceiveKey = VideoSRTPReceiveKey;
   }
}
