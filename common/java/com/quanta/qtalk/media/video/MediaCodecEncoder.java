package com.quanta.qtalk.media.video;

/**
 * Created by xr705 on 15/11/16.
 */

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.LinkedBlockingQueue;

import android.annotation.SuppressLint;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import com.quanta.qtalk.util.Log;

class encodeData{
    byte[] rawdata;
    int size;
    long timestamp;
};

@SuppressLint("NewApi")
public class MediaCodecEncoder
{
    private static final String 	DEBUGTAG = "QTFa_MediaCodecEncoder";
    private static final String 	MIME = "video/avc";
    private static MediaCodec   	mCodec = null;
    private static ByteBuffer[] 	mInputBuffers = null;
    private static ByteBuffer[] 	mOutputBuffers = null;
    private static int          	mWidth = 0;
    private static int          	mHeight = 0;
    private static int          	mColorFormat = 0;
    private static int          	mFrameCnt = 0;
    private static int				mBps = 0;
    private EncodeThread            mThread = null;
    private boolean                 mStart = false;
    private LinkedBlockingQueue<encodeData> mQueue = null;
    private static Object           mQueueLock = null;
    private byte[]                  mStopSignal = {0,0,0,0};
    private short                     mRotation = 0;
    private Handler                 mHandler = null;
    private int                    counter = 0;
    static FileOutputStream         mH264File = null;
    private byte[]              spsPpsBuffer = null;
    private boolean             spsPpsFlag = false;

    public MediaCodecEncoder(Handler handler) {
    	mHandler = handler;
		// TODO Auto-generated constructor stub
	}

    public static void Inintialize() {
        Log.d(DEBUGTAG, "Inintialize");
        return ;
    }

    public boolean Write(byte[] rawdata, int size, long timestamp, short rotation){
        try{
            synchronized (mQueueLock){
                if(mQueue!=null){
                    encodeData data = new encodeData();
                    data.rawdata = rawdata;
                    data.size = size;
                    data.timestamp = timestamp;
                    mQueue.put(data);
                    
                    mRotation = rotation;
                    return true;
                }
            }
        }catch(InterruptedException e){
            Log.e(DEBUGTAG,"",e);
        }
        return false;
    }

//    public void Write(byte[] rawdata, int size, long timestamp) {
//        Log.d(TAG,"Write, mCodec:"+mCodec+",mInputBuffers:"+mInputBuffers);
//        if (mCodec != null && mInputBuffers != null) {
//            int index = mCodec.dequeueInputBuffer(0);
//            if (index >= 0) {
//                if (MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar == mColorFormat) {
//                    // FIXME: need to improve nv21_to_yuv420() performance.
//                    byte[] yuv420_data = new byte[rawdata.length];
//                    nv21_to_yuv420(rawdata, yuv420_data, mWidth, mHeight);
//
//                    mInputBuffers[index].clear();
//                    mInputBuffers[index].put(yuv420_data);
//                    mCodec.queueInputBuffer(index, 0, size, timestamp, 0);
//                } else if (MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar == mColorFormat) {
//                    byte[] nv12_data = new byte[rawdata.length];
//                    nv21_to_nv12(rawdata, nv12_data, mWidth, mHeight);
//
//                    mInputBuffers[index].clear();
//                    mInputBuffers[index].put(nv12_data);
//                    mCodec.queueInputBuffer(index, 0, size, timestamp, 0);
//                } else {
//                    mInputBuffers[index].clear();
//                    mInputBuffers[index].put(rawdata);
//                    mCodec.queueInputBuffer(index, 0, size, timestamp, 0);
//                }
////					mInputBuffers[index].clear();
////					mInputBuffers[index].put(rawdata);
////					mCodec.queueInputBuffer(index, 0, size, timestamp, 0);
//            } else {
////					Log.e(TAG, "queueInputBuffer index = " + index);
//            }
//
//            getH264Frame();
//
//        } else if (mCodec == null)  {
//            Log.e(TAG, "mCodec == null");
//        } else if (mInputBuffers == null) {
//            Log.e(TAG, "mInputBuffers == null");
//        } else {
//            Log.e(TAG, "unknown error");
//        }
//    }

	public  void Config(int width, int height, int fps, int bps, int gop) throws IOException {
        MediaFormat  fmt = MediaFormat.createVideoFormat(MIME, width, height);

        MediaCodecInfo codecInfo = selectCodec(MIME);
        //int colorFormat = selectColorFormat(codecInfo, MIME);
        int colorFormat = MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar;
        if(colorFormat == 0) {
            Log.e(DEBUGTAG, "Invaild Color Format for Encoder");
            return;
        }
        Log.d(DEBUGTAG, "Config, " + width + " x " + height + ", " +
                fps + "fps, " + bps + "bps, color:" + colorFormat);
        mQueue = new LinkedBlockingQueue<encodeData>(60);
        mQueueLock = new Object();
        fmt.setInteger(MediaFormat.KEY_BIT_RATE, bps);
        fmt.setInteger(MediaFormat.KEY_FRAME_RATE, fps);
        fmt.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, gop);
        fmt.setInteger(MediaFormat.KEY_COLOR_FORMAT, colorFormat);

        mWidth = width;
        mHeight = height;
        mColorFormat = colorFormat;
        mBps = bps;

        mCodec = MediaCodec.createEncoderByType(MIME);
        mCodec.configure(fmt, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
        mCodec.start();
        mInputBuffers = mCodec.getInputBuffers();
        mOutputBuffers = mCodec.getOutputBuffers();
       
        mH264File = new FileOutputStream("/sdcard/test264");
        
        mThread = new EncodeThread();
        Start();
    }

    public void Start() {
        Log.d(DEBUGTAG, "Start");

    	if(mCodec!=null){
            mStart = true;
            mThread.setPriority(Thread.MAX_PRIORITY);
            mThread.start();
        }

    }

	public void Stop() {
        Log.d(DEBUGTAG, "Stop");

        if (mCodec != null) {
            Write(mStopSignal,4,0,(short)0);
            try{
                mThread.join();
                mThread = null;
            }catch (InterruptedException e){
                Log.e(DEBUGTAG,"",e);
            }
            synchronized (mQueueLock){
                if(mQueue!=null){
                    mQueue = null;
                }
            }
            try {
                if(mH264File != null)
                    mH264File.close();
            } catch (IOException e) {
                Log.e(DEBUGTAG,"",e);
            }
            
            mCodec.stop();
            mCodec.release();
            mCodec = null;
        }

    }

    private static MediaCodecInfo selectCodec(String mime) {
        int numCodecs = MediaCodecList.getCodecCount();
        for (int i = 0; i < numCodecs; i++) {
            MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);

            if (!codecInfo.isEncoder()) {
                continue;
            }

            String[] types = codecInfo.getSupportedTypes();
            for (int j = 0; j < types.length; j++) {
                if (types[j].equalsIgnoreCase(mime)) {
                    return codecInfo;
                }
            }
        }
        return null;
    }

    private static boolean isRecognizedFormat(int format) {
        switch (format) {
            //case MediaCodecInfo.CodecCapabilities.COLOR_Format16bitRGB565:
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar:
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedPlanar:
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar:
            case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedSemiPlanar:
            case MediaCodecInfo.CodecCapabilities.COLOR_TI_FormatYUV420PackedSemiPlanar:
                return true;
            default:
                return false;
        }
    }

    private static int selectColorFormat(MediaCodecInfo codecInfo, String mime) {
        MediaCodecInfo.CodecCapabilities capabilities = codecInfo.getCapabilitiesForType(mime);
        int colorFormat = 0;
        for (int i=0; i<capabilities.colorFormats.length; i++) {
            int color = capabilities.colorFormats[i];
            Log.d(DEBUGTAG, "color format: " + color);
            if (isRecognizedFormat(color)) {
                colorFormat = color;
            }
        }
        return colorFormat;
    }

    private static void nv21_to_yuv420(byte[] n21_data, byte[] yuv420_data,
                                       int height, int width) {
        int y_plane_size = width * height;
        int u_plane_offset = width * height;
        int v_plane_offset = u_plane_offset + y_plane_size/4 ;

        System.arraycopy(n21_data, 0, yuv420_data, 0, y_plane_size);

        for (int i=0; i<y_plane_size/4;) {
            yuv420_data[i+u_plane_offset] = n21_data[y_plane_size+2*i+1];
            yuv420_data[i+1+u_plane_offset] = n21_data[y_plane_size+2*i+3];
            yuv420_data[i+2+u_plane_offset] = n21_data[y_plane_size+2*i+5];
            yuv420_data[i+3+u_plane_offset] = n21_data[y_plane_size+2*i+7];
            yuv420_data[i+4+u_plane_offset] = n21_data[y_plane_size+2*i+9];
            yuv420_data[i+5+u_plane_offset] = n21_data[y_plane_size+2*i+11];
            yuv420_data[i+6+u_plane_offset] = n21_data[y_plane_size+2*i+13];
            yuv420_data[i+7+u_plane_offset] = n21_data[y_plane_size+2*i+15];

            yuv420_data[i+v_plane_offset] = n21_data[y_plane_size+2*i];
            yuv420_data[i+1+v_plane_offset] = n21_data[y_plane_size+2*i+2];
            yuv420_data[i+2+v_plane_offset] = n21_data[y_plane_size+2*i+4];
            yuv420_data[i+3+v_plane_offset] = n21_data[y_plane_size+2*i+6];
            yuv420_data[i+4+v_plane_offset] = n21_data[y_plane_size+2*i+8];
            yuv420_data[i+5+v_plane_offset] = n21_data[y_plane_size+2*i+10];
            yuv420_data[i+6+v_plane_offset] = n21_data[y_plane_size+2*i+12];
            yuv420_data[i+7+v_plane_offset] = n21_data[y_plane_size+2*i+14];

            i += 8;
        }
    }

    private static void nv21_to_nv12(byte[] n21_data, byte[] nv12_data,
                                     int height, int width) {
        int y_plane_size = width * height;
        int uv_plane_size = y_plane_size/2;

        System.arraycopy(n21_data, 0, nv12_data, 0, y_plane_size);
        for (int i=0; i<uv_plane_size; ) {
            nv12_data[y_plane_size+i] = n21_data[y_plane_size+i+1];
            nv12_data[y_plane_size+i+1] = n21_data[y_plane_size+i];
            i += 2;
        }
    }

    private static void InSessionTest(byte[] h264Frame, int naluType) {
        mFrameCnt++;
        if (0 == mFrameCnt%900) {
            setIdrRequest();
        }
        if (0 == mFrameCnt%300) {
            mBps += 100000;
            setBitRate(mBps);
        }

        // Debug: show NALU header
//		Log.d(TAG, "NALU: " + Integer.toString(h264Frame[0], 16) + " "
//	             + Integer.toString(h264Frame[1], 16) + " "
//	             + Integer.toString(h264Frame[2], 16) + " "
//	             + Integer.toString(h264Frame[3], 16) + " "
//	             + Integer.toString(h264Frame[4], 16));

//		// DEBUG: output h264 stream to file (/sdcard/mediacodec.h264)
    }

    private static void getH264Frame() {
        MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
        int 					 naluType = 0;

        int outputBufferIndex = mCodec.dequeueOutputBuffer(info, 0);
        while (outputBufferIndex >= 0) {
            ByteBuffer buf = mOutputBuffers[outputBufferIndex];
            byte[] h264Frame = new byte[info.size];

            buf.position(info.offset);
            buf.limit(info.offset + info.size);
            buf.get(h264Frame);

            naluType = h264Frame[4]&0x0F;
            if(naluType==5 || naluType==1) {
                // only counting I-slice and P-slice.
//                long timestamp = System.currentTimeMillis();
//                TestParameter.EncodeOUT(timestamp);
//                TestParameter.DecodeIN(timestamp);
            }

            InSessionTest(h264Frame, naluType);
            //VideoDecoder.Write(h264Frame, h264Frame.length);

            mCodec.releaseOutputBuffer(outputBufferIndex, false);
            outputBufferIndex = mCodec.dequeueOutputBuffer(info, 0);
        }
    }

    private static void setIdrRequest() {

        // IDR Request only supports the API 19 or later
        if(	Integer.valueOf(android.os.Build.VERSION.SDK_INT) < 19) {
            Log.d(DEBUGTAG, "Invaild command - PARAMETER_KEY_REQUEST_SYNC_FRAME, API < 19");
            return;
        }

        if (mCodec != null) {
            Bundle para = new Bundle();
            para.putInt(MediaCodec.PARAMETER_KEY_REQUEST_SYNC_FRAME, 0);
            mCodec.setParameters(para);

            Log.e(DEBUGTAG, "setIdrRequest");
        }
    }

    class EncodeThread extends Thread{

        public void run(){
            int inIndex = 0;
            int outIndex = 0;
            Message msg = null;
            Bundle bundle = null;
            MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();         
            Log.d(DEBUGTAG,"EncodeThread Run: "+this.hashCode());

            while(true == mStart){
                try {
                inIndex = mCodec.dequeueInputBuffer(-1);
                    if(inIndex>=0){
                        encodeData buf = mQueue.take();
                        if (buf.rawdata.equals(mStopSignal)){
                            mStart = false;
                            continue;
                        }

                        if(buf!=null){
                            if (MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar == mColorFormat) {
                                // FIXME: need to improve nv21_to_yuv420() performance.
                                byte[] yuv420_data = new byte[buf.rawdata.length];
                                nv21_to_yuv420(buf.rawdata, yuv420_data, mWidth, mHeight);

                                mInputBuffers[inIndex].clear();
                                mInputBuffers[inIndex].put(yuv420_data);
                                mCodec.queueInputBuffer(inIndex, 0, buf.size, buf.timestamp, 0);
                            } else if (MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar == mColorFormat) {
                                byte[] nv12_data = new byte[buf.rawdata.length];
                                nv21_to_nv12(buf.rawdata, nv12_data, mWidth, mHeight);

                                mInputBuffers[inIndex].clear();
                                mInputBuffers[inIndex].put(nv12_data);
                                mCodec.queueInputBuffer(inIndex, 0, buf.size, buf.timestamp, 0);
                            } else {
                                mInputBuffers[inIndex].clear();
                                mInputBuffers[inIndex].put(buf.rawdata);
                                mCodec.queueInputBuffer(inIndex, 0, buf.size, buf.timestamp, 0);
                            }
                        }

                    }
                    outIndex = mCodec.dequeueOutputBuffer(info, 0);
                    while (outIndex >= 0) {
                        ByteBuffer buf = mOutputBuffers[outIndex];
                        byte[] h264Frame = new byte[info.size];
                        
                        buf.position(info.offset);
                        buf.limit(info.offset + info.size);
                        buf.get(h264Frame);
                                             
                        try {
                            mH264File.write(h264Frame);
                        } catch (IOException e) {
                            Log.e(DEBUGTAG,"",e);
                        }
                        
                        int type = H264Display.get_nalu_type(h264Frame);
                        Log.d(DEBUGTAG,"type:"+type+", lenght:"+info.size+", timestamp:"+info.presentationTimeUs);
                        if(type==7){
                        	spsPpsFlag = true;
                        	spsPpsBuffer = new byte[info.size];
                        	System.arraycopy(h264Frame, 0, spsPpsBuffer, 0, info.size);
                        }else{
	                        bundle= new Bundle();
	                        msg = new Message();
	                        if(type == 5){
	                        	byte[] h264FrameAppend = new byte[spsPpsBuffer.length+h264Frame.length];
	                        	System.arraycopy(spsPpsBuffer, 0, h264FrameAppend, 0, spsPpsBuffer.length);
	                        	System.arraycopy(h264Frame, 0, h264FrameAppend, spsPpsBuffer.length, h264Frame.length);
	                        	//spsPpsFlag = false;
	                        	//spsPpsBuffer = null;
	                        	bundle.putByteArray(MediaCodecEncodeTransform.KEY_GET_FRAME, h264FrameAppend);
	                        }else{
	                        	bundle.putByteArray(MediaCodecEncodeTransform.KEY_GET_FRAME, h264Frame);
	                        }
	                        bundle.putLong(MediaCodecEncodeTransform.KEY_GET_TIMESTAMP, info.presentationTimeUs);
	                        bundle.putInt(MediaCodecEncodeTransform.KEY_GET_LENGTH, info.size);
	                        bundle.putShort(MediaCodecEncodeTransform.KEY_GET_ROTATION, mRotation);
	                        msg.what = MediaCodecEncodeTransform.MSG_ENCODE_COMPLETE;
	                        msg.setData(bundle);
	                        mHandler.sendMessage(msg);       
                        }
                        mCodec.releaseOutputBuffer(outIndex, false);
                        outIndex = mCodec.dequeueOutputBuffer(info, 0);
                        
                        bundle = null;
                        msg = null;
                    }



                } catch (InterruptedException e) {
                Log.e(DEBUGTAG,"",e);
            }
            }

        }
    }
    
    public static void setGop(final int gop) 
    {
        
    }

    public static void setBitRate(int bps) {
        // Dynamic bit-rate set only supports the API 19 or later
//        if(	Integer.valueOf(android.os.Build.VERSION.SDK_INT) < 19) {
//            Log.d(DEBUGTAG, "Invaild command - PARAMETER_KEY_VIDEO_BITRATE, API < 19");
//            return;
//        }
//
//        if (mCodec != null) {
//            Bundle para = new Bundle();
//            para.putInt(MediaCodec.PARAMETER_KEY_VIDEO_BITRATE, bps*1000);
//            mCodec.setParameters(para);
//
//            Log.e(DEBUGTAG, "setBitRate bps = " + bps);
//        }
    }
    public static void setFrameRate(byte fpsFramerate)
    {
        
    }
    public static void requestIDR()
    {
        // IDR Request only supports the API 19 or later
        if(	Integer.valueOf(android.os.Build.VERSION.SDK_INT) < 19) {
            Log.d(DEBUGTAG, "Invaild command - PARAMETER_KEY_REQUEST_SYNC_FRAME, API < 19");
            return;
        }

        if (mCodec != null) {
            Bundle para = new Bundle();
            para.putInt(MediaCodec.PARAMETER_KEY_REQUEST_SYNC_FRAME, 0);
            mCodec.setParameters(para);

            Log.e(DEBUGTAG, "setIdrRequest");
        }
    }
}