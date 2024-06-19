package com.quanta.qtalk.media.video;


import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;
//import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.LinkedBlockingQueue;

import com.quanta.qtalk.ui.InVideoSessionActivity;

import android.annotation.SuppressLint;
import android.graphics.Canvas;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import com.quanta.qtalk.util.Log;
import android.view.Surface;
import android.view.SurfaceView;

@SuppressLint("NewApi")
public class H264Display 
{
	private static final String DEBUGTAG = "QSE_H264Display";
	private MediaCodec   mCodec = null;	
	private ByteBuffer[] mInputBuffers = null;
	private ByteBuffer[] mOutputBuffers = null;
	private boolean      mStart = false;
	private DecodeThread mThread = null;
	private byte[] stop_sig = {0,0,0,0};
	private Surface surface = null;
	private boolean isLocalView = false;
	private final Object mQueueLock = new Object();
	private IVideoStreamCTR ctrCallback = null;
	
	private boolean first = true;
	
	//private static ConcurrentLinkedQueue<byte[]> mQueue = new ConcurrentLinkedQueue<byte[]>();
	private LinkedBlockingQueue<byte[]> mQueue = null;
	
	public void Config(int width, int height, SurfaceView view, boolean localview, IVideoStreamCTR cb) {
		Log.e(DEBUGTAG, "Config, " + width + " x " + height+", localview:"+localview+" hashcode:"+this.hashCode());
		surface = view.getHolder().getSurface();
		isLocalView = localview;
		mQueue = new LinkedBlockingQueue<byte[]>(60);
		MediaFormat  fmt = MediaFormat.createVideoFormat("video/avc", width, height);
		
		// For Adaptive playback, setting MediaFormat.KEY_MAX_WIDTH and MediaFormat.KEY_MAX_HEIGHT 
		// will trigger for the "high latency" path.
		if(	Integer.valueOf(android.os.Build.VERSION.SDK_INT) >= 19) {
			//fmt.setInteger(MediaFormat.KEY_MAX_WIDTH, -1);
			//fmt.setInteger(MediaFormat.KEY_MAX_HEIGHT, -1);
		}
		//mCodec = MediaCodec.createByCodecName("OMX.google.h264.decoder");
		try {
			mCodec = MediaCodec.createDecoderByType("video/avc");
		} catch (IOException e) {
			Log.e(DEBUGTAG,"",e);
		}	
		//	selectCodec("");
		fmt.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 0);	
		mCodec.configure(fmt, surface, null, 0); //MediaCodec.VIDEO_SCALING_MODE_SCALE_TO_FIT
		mCodec.start();
		mInputBuffers = mCodec.getInputBuffers();
		mCodec.getOutputBuffers();
				
		mThread = new DecodeThread();
		
		Start();		
		
		if(cb!=null){
			ctrCallback = cb;
			ctrCallback.setRenderReady();
		}
	}
	public void Config(int width, int height, Surface view, boolean localview) {
		Log.e(DEBUGTAG, "Config on TextureView, " + width + " x " + height+", localview:"+localview+" hashcode:"+this.hashCode());
		surface = view;
		isLocalView = localview;
		mQueue = new LinkedBlockingQueue<byte[]>(60);
		MediaFormat  fmt = MediaFormat.createVideoFormat("video/avc", width, height);
		
		// For Adaptive playback, setting MediaFormat.KEY_MAX_WIDTH and MediaFormat.KEY_MAX_HEIGHT 
		// will trigger for the "high latency" path.
		if(	Integer.valueOf(android.os.Build.VERSION.SDK_INT) >= 19) {
			//fmt.setInteger(MediaFormat.KEY_MAX_WIDTH, -1);
			//fmt.setInteger(MediaFormat.KEY_MAX_HEIGHT, -1);
		}
		//mCodec = MediaCodec.createByCodecName("OMX.Intel.VideoDecoder.AVC");
		try {
			mCodec = MediaCodec.createDecoderByType("video/avc");
		} catch (IOException e) {
			Log.e(DEBUGTAG,"",e);
		}		
		fmt.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 0);
		mCodec.configure(fmt, surface, null, 0); //MediaCodec.VIDEO_SCALING_MODE_SCALE_TO_FIT
		mCodec.start();
		mInputBuffers = mCodec.getInputBuffers();
		mCodec.getOutputBuffers();
				
		mThread = new DecodeThread();
		
		Start();
	}
	
	private void Start() {
		
		Log.d(DEBUGTAG, "=> Start");
		
		if (mCodec != null) {			
			mStart = true;
			mThread.setPriority(Thread.MAX_PRIORITY);
			mThread.start();
		}
	}
	
	public void Stop() {
		
		Log.d(DEBUGTAG, "=> Stop: "+this.hashCode());
		
		if(mStart != true) 
			return;
		mStart = false;
		
		if(mThread != null) {
			Log.d(DEBUGTAG, "=> write stop signal: "+this.hashCode());
			Write(stop_sig, 4);
			try {
				Log.d(DEBUGTAG,"wait for thread join: "+this.hashCode());
				mThread.join();
				mThread = null;
			} catch (InterruptedException e) {
				Log.e(DEBUGTAG,"",e);
			}
		}
		
		synchronized (mQueueLock) {			
			Log.d(DEBUGTAG,"=> stop mQueue: "+this.hashCode());			
			if(mQueue != null)
				mQueue = null;
	
			if(mCodec != null) {
				mCodec.stop();
				mCodec.release();
				mCodec = null;
			}			
			mInputBuffers = null;
			first = true;
		}		
		surface.release();
		
		Log.d(DEBUGTAG, "<==Stop "+this.hashCode());
	}
	
	/* QF3 local view */
	public boolean Write(byte[] h264Frame, int sizeInBytes) {
		try {
			synchronized (mQueueLock) {
				if(mQueue != null) {
					if(first) {
						if(h264Frame.equals(stop_sig)) {
							Log.d(DEBUGTAG,"=> receive stop signal");
							mQueue.put(h264Frame);
							return true;
						}
						else {
							int type = get_nalu_type(h264Frame);
							if(type == 7) {
								Log.d(DEBUGTAG,"=> receive first IDR");
								first = false;
							}
							else {								
								return false;
							}
						}
					}
					mQueue.put(h264Frame);		// implement by LinkedBlockingQueue<>, blocking mode
					return true;
				}
			}
		} 
		catch (InterruptedException e) {
			Log.e(DEBUGTAG,"",e);
		}
		return false;
	}
	
	/* remote view */
	public boolean Write(byte[] h264Frame, int sizeInBytes, long timestamp) {
		try {			
			synchronized (mQueueLock) {
				if(mQueue != null) {
					if(first) {
						if(h264Frame.equals(stop_sig)) {
							Log.d(DEBUGTAG,"=> write2: "+mQueue.size());
							mQueue.put(h264Frame);
							return true;
						}
						else {
							int type = get_nalu_type(h264Frame);
							if(type == 7) {
								Log.d(DEBUGTAG,"=> write2: receive first IDR");
								first = false;
							}
							else {
								//sendRequestIDR();
								return false;
							}
						}
					}
					mQueue.put(h264Frame);		// implement by LinkedBlockingQueue<>, blocking mode
					return true;
				}
			}
		}
		catch (InterruptedException e) {
			Log.e(DEBUGTAG,"",e);
		}
		return false;
	}
    private void sendRequestIDR() {
    	//Log.d(DEBUGTAG,"sendRequestIDR");
    	if(ctrCallback != null) {
    		ctrCallback.onSendIDR();
    	}
    	else {
    		Log.d(DEBUGTAG,"send idr request fail");
    	}
		
	}
	class DecodeThread extends Thread {
		
		public void run() {
			int inIndex = 0;
			int outIndex = 0;
			MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
			
			Log.d(DEBUGTAG, "DecodeThread Run: "+this.hashCode());
    		
	    	while (true == mStart) {
	    		try {
	    			// Decode Input
	    			inIndex = mCodec.dequeueInputBuffer(1000);
		    		if (inIndex >= 0) {			    			
		    			//byte[] buf = mQueue.peek();	// implement by ConcurrentLinkedQueue<>, polling
		    			byte[] buf = mQueue.take();		// implement by LinkedBlockingQueue<>, blocking mode
		    			if(buf.equals(stop_sig)){
		    				Log.d(DEBUGTAG,"recv stop sig");
		    				continue;
		    			}
		    			if(buf != null) {
			    			ByteBuffer inputBuf = mInputBuffers[inIndex];					    			
				    		int capacity = inputBuf.capacity();
				    		if(capacity < buf.length) {
				    			Log.d(DEBUGTAG, "capacity " + capacity +  ", length " + buf.length);
				    		}	
				    		
				    		inputBuf.clear();	
				    		inputBuf.put(buf, 0, buf.length);
				    		mCodec.queueInputBuffer(inIndex, 0, buf.length, 0, info.flags);	
				    		mQueue.remove(buf);
				    		buf = null;
		    			}
		    		}
		    		
		    		// Decode Output 
		    		outIndex = mCodec.dequeueOutputBuffer(info, 0);
		    		while(outIndex >= 0) {
		    			mCodec.releaseOutputBuffer(outIndex, true);
		    			outIndex = mCodec.dequeueOutputBuffer(info, 0);
		    		}
		    		if (outIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
		    			mCodec.getOutputBuffers();    			
		    		} else if (outIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {	    			
		    			MediaFormat mMF = mCodec.getOutputFormat();
		    			int new_width = mMF.getInteger(MediaFormat.KEY_WIDTH);
		    			int new_height = mMF.getInteger(MediaFormat.KEY_HEIGHT);
		    			int new_cf = mMF.getInteger(MediaFormat.KEY_COLOR_FORMAT);
		    			Log.d(DEBUGTAG, "Output Format Changed: " + new_width + "x" + new_height + ", color:" + new_cf);
		    			if(ctrCallback != null){
		    				ctrCallback.updateResolutionToUI(new_width, new_height);
		    			}
		    			else{
		    				Log.d(DEBUGTAG,"ctrCallback = Null");
		    			}
		    		} else {
		    			//Log.d(TAG, "Output index = " + outIndex);
		    		}
		    		
//					Thread.sleep(5);
	    		
	    		} catch (InterruptedException e) {
	    			Log.e(DEBUGTAG,"MEDIACODEC ERROR!! "+ this.hashCode(), e);
					mStart = false;
					stopThread.start();
				} catch (IllegalStateException ie){
					Log.d(DEBUGTAG,"MEDIACODEC ERROR!! "+ this.hashCode(), ie);
					mStart = false;
					stopThread.start();
				}
	    		
	    	} // while() loop
	    	Log.d(DEBUGTAG, "DecodeThread Leave: "+ this.hashCode());
	    }
	}
	Thread stopThread = new Thread(){
		@Override
		public void run() {
			Stop();
		}
	};
    public static int get_nalu_type(byte[] src_data)
    {
        if (0x00 == src_data[0] && 0x00 == src_data[1]
            && 0x00 == src_data[2] && 0x01 == src_data[3]) {
            return src_data[4] & 0x1F;
        } else if (0x00 == src_data[0] && 0x00 == src_data[1] && 0x01 == src_data[2]){
            return src_data[3] & 0x1F;
        }
        return 0;
    }
    
    private static MediaCodecInfo selectCodec(String mimeType) {
        int numCodecs = MediaCodecList.getCodecCount();
        for (int i = 0; i < numCodecs; i++) {
            MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);
            Log.d(DEBUGTAG,"codecInfo:"+codecInfo.getName());
            if (!codecInfo.isEncoder()) {
                continue;
            }

            String[] types = codecInfo.getSupportedTypes();
            for (int j = 0; j < types.length; j++) 
            {
            	Log.d(DEBUGTAG,"types:"+types[j].toString());
                if (types[j].equalsIgnoreCase(mimeType)) {
                    return codecInfo;
                }
            }
        }
        return null;
    }
}
