package com.quanta.qtalk.media.video;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;

import android.annotation.SuppressLint;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.media.MediaCodec.BufferInfo;
import android.media.MediaMuxer;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import com.quanta.qtalk.util.Log;

public class VideoEncoderFromBuffer {
	private static final String TAG = "QTFa_VideoEncoderFromBuffer";
	private static final String DEBUGTAG = "QTFa_VideoEncoderFromBuffer";
	private static final boolean VERBOSE = true; // lots of logging
	private static final String DEBUG_FILE_NAME_BASE = "/sdcard/Movies/h264";
	// parameters for the encoder
	private static final String MIME_TYPE = "video/avc"; // H.264 Advanced Video
	private static final int FRAME_RATE = 25; // 15fps
	private static final int IFRAME_INTERVAL = 5; // 10 between
															// I-frames
	private static final int TIMEOUT_USEC = 0;
	private static final int COMPRESS_RATIO = 256;
	//private static final int BIT_RATE = CameraWrapper.IMAGE_HEIGHT * CameraWrapper.IMAGE_WIDTH * 3 * 8 * FRAME_RATE / COMPRESS_RATIO; // bit rate CameraWrapper.
	private static int BIT_RATE = 720000;
	private int mWidth;
	private int mHeight;
	private static MediaCodec mMediaCodec;
	private BufferInfo mBufferInfo;
	byte[] mFrameData;
	FileOutputStream mFileOutputStream = null;
	private int mColorFormat;
	private long mStartTime = 0;
	
	
	
	private Handler                 mHandler = null;
    private byte[]              spsPpsBuffer = null;
    private boolean             spsPpsFlag = false;
    
	@SuppressLint("NewApi")
	public VideoEncoderFromBuffer(Handler handler, int width, int height) throws FileNotFoundException {
		Log.i(TAG, "VideoEncoder()");
		this.mWidth = width;
		this.mHeight = height;
		this.mHandler = handler;
		mFrameData = new byte[this.mWidth * this.mHeight * 3 / 2];
		
		mBufferInfo = new MediaCodec.BufferInfo();
		MediaCodecInfo codecInfo = selectCodec(MIME_TYPE);
		if (codecInfo == null) {
			// Don't fail CTS if they don't have an AVC codec (not here,
			// anyway).
			Log.e(TAG, "Unable to find an appropriate codec for " + MIME_TYPE);
			return;
		}
		if (VERBOSE)
			Log.d(TAG, "found codec: " + codecInfo.getName());
		mColorFormat = selectColorFormat(codecInfo, MIME_TYPE);
		if (VERBOSE)
			Log.d(TAG, "found colorFormat: " + mColorFormat);
		BIT_RATE =  this.mWidth * this.mHeight * 3 * 8 * FRAME_RATE / COMPRESS_RATIO; // bit rate CameraWrapper.
		MediaFormat mediaFormat = MediaFormat.createVideoFormat(MIME_TYPE,
				this.mWidth, this.mHeight);
		Log.d(TAG, "BIT_RATE: " + BIT_RATE);
		mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, BIT_RATE);
		mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, FRAME_RATE);
		mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, mColorFormat);
		mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL,
				IFRAME_INTERVAL);
		if (VERBOSE)
			Log.d(TAG, "format: " + mediaFormat);
		try {
			mMediaCodec = MediaCodec.createByCodecName(codecInfo.getName());
		} catch (IOException e) {
			Log.e(DEBUGTAG,"",e);
		}
		mMediaCodec.configure(mediaFormat, null, null,
				MediaCodec.CONFIGURE_FLAG_ENCODE);
		mMediaCodec.start();

		mStartTime = System.nanoTime();

	}
	long startTime = 0;
	@SuppressLint("NewApi")
	public void encodeFrame(byte[] input /*, byte[] output*/ ) {
		Log.i(TAG, "encodeFrame()");
        Message msg = null;
        Bundle bundle = null;
		long encodedSize = 0;
		NV21toI420SemiPlanar(input, mFrameData, this.mWidth, this.mHeight);
		System.arraycopy(input, 0, mFrameData, 0, input.length);
		ByteBuffer[] inputBuffers = mMediaCodec.getInputBuffers();
		ByteBuffer[] outputBuffers = mMediaCodec.getOutputBuffers();
		int inputBufferIndex = mMediaCodec.dequeueInputBuffer(TIMEOUT_USEC);
		//if (VERBOSE)
		//	Log.i(TAG, "inputBufferIndex-->" + inputBufferIndex);
		if (inputBufferIndex >= 0) {
			long endTime = System.nanoTime();
			long ptsUsec = (endTime - mStartTime) / 1000;
			Log.i(TAG, "resentationTime: " + ptsUsec);
			ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
			inputBuffer.clear();
			inputBuffer.put(mFrameData);
			mMediaCodec.queueInputBuffer(inputBufferIndex, 0,
					mFrameData.length, System.nanoTime() / 1000, 0);
		} else {
			// either all in use, or we timed out during initial setup
			if (VERBOSE)
				Log.d(TAG, "input buffer not available");
		}

		int outputBufferIndex = mMediaCodec.dequeueOutputBuffer(mBufferInfo, TIMEOUT_USEC);
		//Log.i(TAG, "outputBufferIndex-->" + outputBufferIndex);
		do {
			if (outputBufferIndex == MediaCodec.INFO_TRY_AGAIN_LATER) {
				// no output available yet
				if (VERBOSE)
					Log.d(TAG, "no output from encoder available");
			} else if (outputBufferIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
				// not expected for an encoder
				outputBuffers = mMediaCodec.getOutputBuffers();
				if (VERBOSE)
					Log.d(TAG, "encoder output buffers changed");
			} else if (outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
				// not expected for an encoder
				
				MediaFormat newFormat = mMediaCodec.getOutputFormat();
                Log.d(TAG, "encoder output format changed: " + newFormat);

			} else if (outputBufferIndex < 0) {
				Log.w(TAG, "unexpected result from encoder.dequeueOutputBuffer: " +
						outputBufferIndex);
                // let's ignore it
			} else {
				if (VERBOSE)
					Log.d(TAG, "perform encoding");
				ByteBuffer outputBuffer = outputBuffers[outputBufferIndex];
				if (outputBuffer == null) {
                    throw new RuntimeException("encoderOutputBuffer " + outputBufferIndex +
                            " was null");
                }
				
				if ((mBufferInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
                    // The codec config data was pulled out and fed to the muxer when we got
                    // the INFO_OUTPUT_FORMAT_CHANGED status.  Ignore it.
                    if (VERBOSE) Log.d(TAG, "ignoring BUFFER_FLAG_CODEC_CONFIG");
                   // mBufferInfo.size = 0;
                }

				if (mBufferInfo.size != 0) {
					// adjust the ByteBuffer values to match BufferInfo (not needed?)
					outputBuffer.position(mBufferInfo.offset);
					outputBuffer.limit(mBufferInfo.offset + mBufferInfo.size);
					byte[] h264Frame = new byte[mBufferInfo.size];
					outputBuffer.get(h264Frame);
					
					
					int type = H264Display.get_nalu_type(h264Frame);
					Log.d(TAG,"VideoStreamEngineTransformJni type = "+type);
                    if(type==7){
                    	spsPpsFlag = true;
                    	spsPpsBuffer = new byte[mBufferInfo.size];
                    	Log.d(TAG,"mBufferInfo.size:"+mBufferInfo.size+", h264Frame.length:"+h264Frame.length);
                    	System.arraycopy(h264Frame, 0, spsPpsBuffer, 0, mBufferInfo.size);
                    }else{
                        bundle= new Bundle();
                        msg = new Message();
                        if(type == 5){
                        	byte[] h264FrameAppend = new byte[spsPpsBuffer.length+h264Frame.length];
                        	System.arraycopy(spsPpsBuffer, 0, h264FrameAppend, 0, spsPpsBuffer.length);
                        	System.arraycopy(h264Frame, 0, h264FrameAppend, spsPpsBuffer.length, h264Frame.length);
                        	bundle.putByteArray(MediaCodecEncodeTransform.KEY_GET_FRAME, h264FrameAppend);
                        }else{
                        	bundle.putByteArray(MediaCodecEncodeTransform.KEY_GET_FRAME, h264Frame);
                        }
                        bundle.putLong(MediaCodecEncodeTransform.KEY_GET_TIMESTAMP, mBufferInfo.presentationTimeUs);
                        bundle.putInt(MediaCodecEncodeTransform.KEY_GET_LENGTH, mBufferInfo.size);
                        bundle.putShort(MediaCodecEncodeTransform.KEY_GET_ROTATION, (short)1);
                        msg.what = MediaCodecEncodeTransform.MSG_ENCODE_COMPLETE;
                        msg.setData(bundle);
                        mHandler.sendMessage(msg);       
                    }
                    bundle = null;
                    msg = null;
					
					
					if (VERBOSE) {
						Log.d(TAG, "sent " + mBufferInfo.size + " bytes to muxer");
					}
				}

				mMediaCodec.releaseOutputBuffer(outputBufferIndex, false);
			}
			outputBufferIndex = mMediaCodec.dequeueOutputBuffer(mBufferInfo, TIMEOUT_USEC);
		} while (outputBufferIndex >= 0);
	}

	@SuppressLint("NewApi")
	public void close() {
		try {
			if(mMediaCodec != null){
				mMediaCodec.stop();
				mMediaCodec.release();
				mMediaCodec = null;
			}
		} catch (Exception e) {
			Log.e(DEBUGTAG,"",e);
		}
	}

	/**
	 * NV21 is a 4:2:0 YCbCr, For 1 NV21 pixel: YYYYYYYY VUVU I420YUVSemiPlanar
	 * is a 4:2:0 YUV, For a single I420 pixel: YYYYYYYY UVUV Apply NV21 to
	 * I420YUVSemiPlanar(NV12) Refer to https://wiki.videolan.org/YUV/
	 */
	private void NV21toI420SemiPlanar(byte[] nv21bytes, byte[] i420bytes,
			int width, int height) {
		System.arraycopy(nv21bytes, 0, i420bytes, 0, width * height);
		for (int i = width * height; i < nv21bytes.length; i += 2) {
			i420bytes[i] = nv21bytes[i + 1];
			i420bytes[i + 1] = nv21bytes[i];
		}
	}

	/**
	 * Returns a color format that is supported by the codec and by this test
	 * code. If no match is found, this throws a test failure -- the set of
	 * formats known to the test should be expanded for new platforms.
	 */
	@SuppressLint("NewApi")
	private static int selectColorFormat(MediaCodecInfo codecInfo,
			String mimeType) {
		MediaCodecInfo.CodecCapabilities capabilities = codecInfo
				.getCapabilitiesForType(mimeType);
		for (int i = 0; i < capabilities.colorFormats.length; i++) {
			int colorFormat = capabilities.colorFormats[i];
			Log.d(TAG,"support color format:"+colorFormat);
			if (isRecognizedFormat(colorFormat)) {
				return colorFormat;
			}
		}
		Log.e(TAG,
				"couldn't find a good color format for " + codecInfo.getName()
						+ " / " + mimeType);
		return 0; // not reached
	}

	/**
	 * Returns true if this is a color format that this test code understands
	 * (i.e. we know how to read and generate frames in this format).
	 */
	private static boolean isRecognizedFormat(int colorFormat) {
		switch (colorFormat) {
		// these are the formats we know how to handle for this test
		//case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar:
		//case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedPlanar:
		case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar:
		//case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedSemiPlanar:
		//case MediaCodecInfo.CodecCapabilities.COLOR_TI_FormatYUV420PackedSemiPlanar:
			return true;
		default:
			return false;
		}
	}

	/**
	 * Returns the first codec capable of encoding the specified MIME type, or
	 * null if no match was found.
	 */
	@SuppressLint("NewApi")
	private static MediaCodecInfo selectCodec(String mimeType) {
		int numCodecs = MediaCodecList.getCodecCount();
		for (int i = 0; i < numCodecs; i++) {
			MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);
			if (!codecInfo.isEncoder()) {
				continue;
			}
			String[] types = codecInfo.getSupportedTypes();
			for (int j = 0; j < types.length; j++) {
				if (types[j].equalsIgnoreCase(mimeType)) {
					return codecInfo;
				}
			}
		}
		return null;
	}

	/**
	 * Generates the presentation time for frame N, in microseconds.
	 */
	private static long computePresentationTime(int frameIndex) {
		return 132 + frameIndex * 1000000 / FRAME_RATE;
	}

	/**
	 * Returns true if the specified color format is semi-planar YUV. Throws an
	 * exception if the color format is not recognized (e.g. not YUV).
	 */
	private static boolean isSemiPlanarYUV(int colorFormat) {
		switch (colorFormat) {
		case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar:
		case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedPlanar:
			return false;
		case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar:
		case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420PackedSemiPlanar:
		case MediaCodecInfo.CodecCapabilities.COLOR_TI_FormatYUV420PackedSemiPlanar:
			return true;
		default:
			throw new RuntimeException("unknown format " + colorFormat);
		}
	}
	
    public static void setGop(final int gop) 
    {
        
    }

    @SuppressLint("NewApi")
	public static void setBitRate(int bps) {
        // Dynamic bit-rate set only supports the API 19 or later
        if(	Integer.valueOf(android.os.Build.VERSION.SDK_INT) < 19) {
            Log.d(DEBUGTAG, "Invaild command - PARAMETER_KEY_VIDEO_BITRATE, API < 19");
            return;
        }

        if (mMediaCodec != null) {
            Bundle para = new Bundle();
            para.putInt(MediaCodec.PARAMETER_KEY_VIDEO_BITRATE, bps*1000);
            mMediaCodec.setParameters(para);

            Log.e(DEBUGTAG, "setBitRate bps = " + bps);
        }
    }
    public static void setFrameRate(byte fpsFramerate)
    {
        
    }
    @SuppressLint("NewApi")
	public static void requestIDR()
    {
        // IDR Request only supports the API 19 or later
        if(	Integer.valueOf(android.os.Build.VERSION.SDK_INT) < 19) {
            Log.d(DEBUGTAG, "Invaild command - PARAMETER_KEY_REQUEST_SYNC_FRAME, API < 19");
            return;
        }

        if (mMediaCodec != null) {
            Bundle para = new Bundle();
            para.putInt(MediaCodec.PARAMETER_KEY_REQUEST_SYNC_FRAME, 0);
            mMediaCodec.setParameters(para);

            Log.e(DEBUGTAG, "setIdrRequest");
        }
    }
}
