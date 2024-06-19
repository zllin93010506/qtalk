package com.quanta.qtalk.media.video;

import java.nio.ByteBuffer;
import java.util.LinkedList;
import java.util.Queue;
import java.util.concurrent.LinkedBlockingQueue;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.os.Bundle;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.R;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.ui.InVideoSessionActivity;
import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

//class H264Frame{
//	byte[] frame;
//	int leagth;
//	long timestamp;
//}

public class VideoRenderMediaCodec extends SurfaceView implements SurfaceHolder.Callback
							,IVideoSink, IVideoSinkPollbased, IVideoStreamReportCB, IVideoStreamCTR
{
	private static final String DEBUGTAG = "QSE_VideoRenderMediaCodec";
	private VideoRenderMediaCodec   mVideoRender = null;
	private SurfaceHolder mHolder = null;
//	private IVideoSourcePollbased mSource = null;
	private H264Display mH264Display = new H264Display();
//	private boolean mShowLowBandwidth = false;
	private Handler mHandler = null;
	private boolean isfirstconfig = true;
	float x = 0, y = 0;
	private int            mWidth = 0;
	private int            mHeight = 0;
	
	private final Object mLock = new Object();
	
	//----------------Control command----------------------------
//	private boolean isRenderReady = false;
	private boolean firstSetRemoteView = true;
	private long lastIDRtimestamp = 0;
	private final long retryIDRinterval = 800;
//	private boolean      mStart = false;
//	private LinkedBlockingQueue<H264Frame> mQueue = new LinkedBlockingQueue<H264Frame>();
//	private final ConditionVariable mOnNewData = new ConditionVariable();
//	private byte[] stop_sig = {0,0,0,0};
//	private controlThread thread = null;
	
	
    /***************************************************************************
	 *  				Public function implement
	 **************************************************************************/
	public void Inintialize() {
		Log.d(DEBUGTAG, "Inintialize");
		return ;
	}

	public void Destroy() {
		Log.d(DEBUGTAG, "Destroy");
		mVideoRender = null;
	}

	public void Config(int width, int height) {
		Log.d(DEBUGTAG, "Config, " + width + " x " + height);
		_Config(width, height);
	}
	
    public void Start() {
    	Log.d(DEBUGTAG, "Start");
	}
    
    public void Stop() {
		Log.d(DEBUGTAG, "Stop");
		mHolder = null;
	}

    public void Pause() {
		Log.d(DEBUGTAG, "Pause");
	}

    public void Resume() {
		Log.d(DEBUGTAG, "Resume");
	}
	private static Handler timeoutHandler = new Handler();
	private static int TIMEOUT_MS = 10000;

	private void restartTimeoutHandler(String reason) {
		Log.d(DEBUGTAG, "restartTimeoutHandler:"+reason);
		timeoutHandler.removeCallbacksAndMessages(null);
		timeoutHandler.postDelayed(() -> {
			setHangup();
		}, TIMEOUT_MS);
	}
	public void setHangup() {		
        Message msg = mHandler.obtainMessage(1, "no streaming");
		msg.what = InVideoSessionActivity.NO_STEAM_HANGUP;
        mHandler.sendMessage(msg);
	}

    public VideoRenderMediaCodec(Context context, AttributeSet attrs) {
        super(context, attrs);
        Log.d(DEBUGTAG, "context, attrs");
        mHolder = this.getHolder();
        mHolder.addCallback(this);
        this.getResources();
    	mVideoRender = this;
		restartTimeoutHandler("VideoRenderMediaCodec(Context context, AttributeSet attrs)");
    }
    
    public VideoRenderMediaCodec(Context context) {
        super(context);
        Log.d(DEBUGTAG, "context");
        mHolder = this.getHolder();
        mHolder.addCallback(this);
        this.getResources();
    	mVideoRender = this;
		restartTimeoutHandler("VideoRenderMediaCodec(Context context)");
    }
    
    public SurfaceView GetRemoteView() {
		Log.e(DEBUGTAG, "GetRemoteView");
		return mVideoRender;
    }
    
    /***************************************************************************
	 *  			implement SurfaceHolder.Callback
	 **************************************************************************/
	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		Log.d(DEBUGTAG, "surfaceChanged, " + width + " x " + height + ", color:" + format + "suface:" + holder);
		VideoLevelID id = new VideoLevelID();
		mHolder = holder;
		if(isfirstconfig) {
			if(mH264Display != null) {
				mH264Display.Config(id.width, id.width, mVideoRender, false, this);
				isfirstconfig = false;
			}
		}
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		Log.d(DEBUGTAG, "surfaceCreated");
		mHolder = holder;
//		mStart = true;
//		if(thread==null){
//			thread = new controlThread();
//			thread.start();
//		}
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		Log.d(DEBUGTAG, "surfaceDestroyed");
//		release();
//		mHolder = null;
//		mStart = false;
	}
    
	/***************************************************************************
	 *  				Private function implement
	 **************************************************************************/
    private static void _Config(int width, int height) {
    	
	}

	@Override
	public void onReport(int kbpsRecv, byte fpsRecv, int kbpsSend,
			byte fpsSend, double pkloss, int usRtt) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setSource(IVideoSourcePollbased source)
			throws FailedOperateException, UnSupportException {
//		mSource = source;		
	}

	@Override
	public void setFormat(int format, int width, int height)
			throws FailedOperateException, UnSupportException {
		//Log.d(DEBUGTAG, "setFormat:"+format+" "+width+" "+height);
		restartTimeoutHandler("setFormat:"+format+" "+width+" "+height);
        mWidth = width;
        mHeight = height;
        if(firstSetRemoteView)
        	updateResolutionToUI(mWidth, mHeight);
	}

	@Override
	public boolean onMedia(ByteBuffer data, int length, long timestamp, short rotation) throws UnSupportException {
		restartTimeoutHandler("onMedia:"+length);
		boolean result = true;
		if(mH264Display != null) {
			byte[] byteFrame = new byte[data.remaining()];
			data.get(byteFrame);
			result = mH264Display.Write(byteFrame, length, timestamp);
		}
		return result;
		
//		boolean result = false;
//        if(data!=null && mH264Display!=null){
//        	byte[] b = new byte[data.remaining()];
//        	data.get(b);
//        	//result = mH264Display.Write(b, length, timestamp);
//        	H264Frame frame = new H264Frame();
//        	frame.frame = b;
//        	frame.leagth = length;
//        	frame.timestamp = timestamp;
//        	result = mQueue.offer(frame);
//        	if(isRenderReady)
//        		mOnNewData.open();
//        	return result;
//        }
//		
//		return result;
		
	}

	@Override
	public void setLowBandwidth(boolean showLowBandwidth) {		
		Log.d(DEBUGTAG,"showLowBandwidth:"+showLowBandwidth+" "+getResources().getConfiguration().locale.getDefault().getLanguage());
//        mShowLowBandwidth  = showLowBandwidth;	
        
        Message msg = mHandler.obtainMessage(1, "showLowBandwidth");
		if(showLowBandwidth) {
			msg.what = InVideoSessionActivity.LOWBANDWIDTH_ON;
		}
		else {
			msg.what = InVideoSessionActivity.LOWBANDWIDTH_OFF;
		}
        mHandler.sendMessage(msg);
	}

	@Override
	public void release() {
		Log.d(DEBUGTAG,"release");
		timeoutHandler.removeCallbacksAndMessages(null);
	    if(mH264Display != null) {
	    	mH264Display.Stop();
	    }
		mHolder = null;
	    isfirstconfig = true;
	    
//	    mSource = null;
//	    mStart = false;
//	    H264Frame stopFrame = new H264Frame();
//	    stopFrame.frame = stop_sig;
//	    mQueue.offer(stopFrame);
//	    if(thread!=null){
//		    try {
//				thread.join();
//				thread = null;
//			} catch (InterruptedException e) {
//				Log.e(DEBUGTAG,"",e);
//			}
//	    }
//	    isRenderReady = false;
	}

	public void setDisplayRotation(int rotation) {
		// TODO Auto-generated method stub
		
	}

	public void setVideoReportListener(IVideoStreamReportCB cbListener) {
		// TODO Auto-generated method stub
		
	}

	public void setHandler(Handler handler) {
		mHandler = handler;
	}
	
//	class controlThread extends Thread{
//		@Override
//		public void run() {
//			Log.d(DEBUGTAG,"==>controlThread:"+this.hashCode());
//			while (true == mStart){
//
//				if(isRenderReady){
//					H264Frame frame = null;
//					try {
//						frame = mQueue.take();
//						if(frame.frame.equals(stop_sig)){
//							Log.d(DEBUGTAG,"receive stop signal");
//							continue;
//						}
//						if(mH264Display!=null)
//							mH264Display.Write(frame.frame, frame.leagth, frame.timestamp);
//						
//					} catch (InterruptedException e) {
//						// TODO Auto-generated catch block
//						Log.e(DEBUGTAG,"",e);
//					}
//				}else{
//					boolean block = mOnNewData.block(5000);
//                    if (block==false){
//                    	mOnNewData.close();
//                    	continue;
//                    }else{
//                    	mOnNewData.close();
//                    }
//				}
//                
//			}
//			Log.d(DEBUGTAG,"<==controlThread:"+this.hashCode());
//		}		
//	}
	
	@Override
	public void onSendIDR() {
//		long nowTimestamp = System.currentTimeMillis();
//		if(nowTimestamp-lastIDRtimestamp>retryIDRinterval){
//			lastIDRtimestamp = nowTimestamp;
//			onDemandIDR();
//		}
	}
	
	@Override
	public void setRenderReady() {
//		isRenderReady = true;
//		mOnNewData.open();
	}
	
	private void onDemandIDR(){
//		if(mHandler!=null){
//			mHandler.sendEmptyMessage(InVideoSessionActivity.SEND_IDR_REQUEST);
//		}
	}
	
	@Override
	public void updateResolutionToUI(int width, int height){
        Log.d(DEBUGTAG,"updateResolutionToUI, width:"+width+", height:"+height);
        if(width>=50 && height>=50) {
        	firstSetRemoteView = false;
	        Message msg = new Message();
	        Bundle bundle = new Bundle();
	        bundle.putInt(InVideoSessionActivity.REMOTEVIEW_WIDTH, width);
	        bundle.putInt(InVideoSessionActivity.REMOTEVIEW_HEIGHT, height);
	        msg.what = InVideoSessionActivity.UPDATE_REMOTE_RESULOTION;
	        msg.setData(bundle);
	        mHandler.sendMessage(msg);
        }
        Message msg1 = new Message();
        Bundle bundle1 = new Bundle();
        if(width <= 640) {
            bundle1.putBoolean(InVideoSessionActivity.SHOW_REMOTE_SCALE, true);
        }
        else {
            bundle1.putBoolean(InVideoSessionActivity.SHOW_REMOTE_SCALE, false);
        }
        msg1.what = InVideoSessionActivity.SHOW_REMOTE_SCALE_BTN;
        msg1.setData(bundle1);
        mHandler.sendMessage(msg1);
	}
}


