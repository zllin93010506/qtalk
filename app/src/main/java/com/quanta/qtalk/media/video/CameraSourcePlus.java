package com.quanta.qtalk.media.video;

import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.StringTokenizer;
import java.util.Vector;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.ImageFormat;
import android.graphics.Matrix;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.Size;
import android.os.Build;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.Looper;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.Surface.OutOfResourcesException;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;
import android.widget.RelativeLayout;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.R;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

@TargetApi(Build.VERSION_CODES.GINGERBREAD)
public class CameraSourcePlus implements IVideoSource, IVideoMute, Camera.PreviewCallback
{	
    private static final String DEBUGTAG = "QSE_CameraSourcePlus";
    private Context mContext = null;
//    private LocalLayout mLocalLayout = null;
    private LocalPreview mLocalPreview = null;
    private CameraPreview mCameraPreview = null;
    private Camera mCamera = null;
    private static final Object mCameraLock = new Object();
    private static final Object mSurfaceLock = new Object();
    private static final Object mLocalViewLock = new Object();
    private final ConditionVariable mLocalOnNewData = new ConditionVariable();
    private int mFormat = ImageFormat.NV21;
    private int mWidth = 0;
    private int mHeight = 0;
    private int mCameraID = 2;
    private int mRotationDegree = 0;
    private int mDisplayRotation = 0;
    private float m_x_scale=1.0f,m_y_scale=1.0f;
    private int thisThread = 0;
    private int thisLocalThread = 0;
    private static final int MINIMUM_FPS = 15;
    private int mFrameRate = MINIMUM_FPS;
    private boolean mOnPreviewing = false;
    private long mStartTime = 0;
//    private LocalViewThread mLocalViewThread = null;
    private boolean UpdateLocalView = true;   
    private IVideoSink mSink = null;
    private boolean mIsStarted = false;
    private boolean mOnSurfaceChanging = false;
    private FrameDataDepository mFrameDataDepository = null;
    private static final int DEPOSITORY_SIZE = 1;
    private final Handler mHandler = new Handler();
    private static boolean mIsDualCamera = true;
    
    //mute Camera
    private boolean mIsSilence = false;
    private byte [] mpI420 = null;
    private Bitmap mMuteCameraBitmap = null;
    private Bitmap muteBmp = null;
    private Handler mTimerHandler = null;
    public static final short MODE_NA = -1;
    public static final short MODE_ONE = 0;
    public static final short MODE_BACK = 1;
    public static final short MODE_FACE = 2;
    public boolean  isScreenPortrait=true;
    public static int xx = 0;
    private int targetW = 0;
    private int targetH = 0;
    private byte[] LocalViewData = null;    
    private boolean mEnableCameraMute = true;    
    private boolean isPreviewRotate = true;    
    private int frameCnt        = 0;    
    private VideoLevelID levelid = new VideoLevelID();

    static class FrameData
    {
        public byte[] frameData = null;
        public int frameSize = 0;
        public long sampleTime = 0;
        public ByteBuffer frameBuffer = null;
    }

    static class FrameDataDepository
    {
        final Vector<FrameData> mDataVector = new Vector<FrameData>();
        final ConditionVariable mOnDataReturn = new ConditionVariable();

        public FrameDataDepository(int size, int frameSize)
        {
            for (int i = 0; i < size; i++)
            {
                FrameData tmpFrameData = new FrameData();
                tmpFrameData.frameData = new byte[frameSize];
                tmpFrameData.frameSize = frameSize;
                tmpFrameData.sampleTime = 0;
                tmpFrameData.frameBuffer = ByteBuffer.wrap(tmpFrameData.frameData);
                mDataVector.add(tmpFrameData);
            }
        }

        public FrameData borrowData()
        {
            if (mDataVector.size() == 0)
            {
                mOnDataReturn.block();
                mOnDataReturn.close();
            }
            return mDataVector.remove(0);
        }

        public void returnData(FrameData data)
        {
            mDataVector.add(data);
            mOnDataReturn.open();
        }
    }

    // Method to show current properties
    private String propertiesToString()
    {
        StringBuffer sb = new StringBuffer();
        sb.append("mCamera:" + mCamera);
        sb.append(",mFormat:" + mFormat);
        sb.append(",mWidth:" + mWidth);
        sb.append(",mHeight:" + mHeight);
        sb.append(",mStartTime:" + mStartTime);
        return sb.toString();
    }

	public CameraSourcePlus()
    {

    }
	
	public CameraSourcePlus(Context context)
	{
		mContext = context;
	}
	
	byte[] getNV21(int inputWidth, int inputHeight, Bitmap scaled) {
		
		int [] argb = new int[inputWidth*inputHeight];
		scaled.getPixels(argb, 0, inputWidth, 0, 0, inputWidth, inputHeight);
		
		byte [] yuv = new byte[inputWidth*inputHeight*3/2];
		encodeYUV420P(yuv, argb, inputWidth, inputHeight);

		return yuv;
	}

	byte[] getEncodedMute() throws IOException {
		InputStream input = null;
		if(targetH >= 720) {
			input = mContext.getResources().openRawResource(R.raw.mute_1280x720);
		}
		else {
			if(mRotationDegree==90 || mRotationDegree == 270) {
				input = mContext.getResources().openRawResource(R.raw.mute_480x640);
			}
			else {
				input = mContext.getResources().openRawResource(R.raw.mute_640x480);
			}
		}
		byte[] b = new byte[input.available()];
		input.read(b);
		return b;
	}
	
	void encodeYUV420SP(byte[] yuv420sp, int[] argb, int width, int height) {
        final int frameSize = width * height;

        int yIndex = 0;
        int uvIndex = frameSize;

        int a, R, G, B, Y, U, V;
        int index = 0;
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {

                a = (argb[index] & 0xff000000) >> 24; // a is not used obviously
                R = (argb[index] & 0xff0000) >> 16;
                G = (argb[index] & 0xff00) >> 8;
                B = (argb[index] & 0xff) >> 0;

                // well known RGB to YUV algorithm
                Y = ( (  66 * R + 129 * G +  25 * B + 128) >> 8) +  16;
                U = ( ( -38 * R -  74 * G + 112 * B + 128) >> 8) + 128;
                V = ( ( 112 * R -  94 * G -  18 * B + 128) >> 8) + 128;

                // NV21 has a plane of Y and interleaved planes of VU each sampled by a factor of 2
                //    meaning for every 4 Y pixels there are 1 V and 1 U.  Note the sampling is every other
                //    pixel AND every other scanline.
                yuv420sp[yIndex++] = (byte) ((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));
                if (j % 2 == 0 && index % 2 == 0) { 
                    yuv420sp[uvIndex++] = (byte)((V<0) ? 0 : ((V > 255) ? 255 : V));
                    yuv420sp[uvIndex++] = (byte)((U<0) ? 0 : ((U > 255) ? 255 : U));
                }

                index ++;
            }
        }
    }
	
	void encodeYUV420P(byte[] yuv420sp, int[] argb, int width, int height) {
        final int frameSize = width * height;

        int yIndex = 0;
        int uIndex = frameSize;
        int vIndex = frameSize*5/4;
        int a, R, G, B, Y, U, V;
        int index = 0;
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {

                a = (argb[index] & 0xff000000) >> 24; // a is not used obviously
                R = (argb[index] & 0xff0000) >> 16;
                G = (argb[index] & 0xff00) >> 8;
                B = (argb[index] & 0xff) >> 0;

                // well known RGB to YUV algorithm
                Y = ( (  66 * R + 129 * G +  25 * B + 128) >> 8) +  16;
                U = ( ( -38 * R -  74 * G + 112 * B + 128) >> 8) + 128;
                V = ( ( 112 * R -  94 * G -  18 * B + 128) >> 8) + 128;

                // NV21 has a plane of Y and interleaved planes of VU each sampled by a factor of 2
                //    meaning for every 4 Y pixels there are 1 V and 1 U.  Note the sampling is every other
                //    pixel AND every other scanline.
                yuv420sp[yIndex++] = (byte) ((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));
                if (j % 2 == 0 && index % 2 == 0) { 
                    yuv420sp[vIndex++] = (byte)((V<0) ? 0 : ((V > 255) ? 255 : V));
                    yuv420sp[uIndex++] = (byte)((U<0) ? 0 : ((U > 255) ? 255 : U));
                }

                index ++;
            }
        }
    }
    //raw data RGB to I420
    private void BGRAtoI420 (byte[] data ,byte[]I420 , int layer)
    {
        if(data == null || I420 == null)
            return;
        
        int ui = mHeight*mWidth;
        int vi = mHeight*mWidth*5/4;
        
        for (int j = 0; j < mHeight; j++)
            for (int k = 0; k < mWidth; k++)
            {
                byte B = data[j*mWidth*layer+k*layer+0];
                byte G = data[j*mWidth*layer+k*layer+1];
                byte R = data[j*mWidth*layer+k*layer+2];
                
                I420[j*mWidth+k] = (byte)(((66*R + 129*G + 25*B + 128) >> 8)+ 16);
                if (j%2 == 0 && k%2 == 0)
                {
                    I420[ui++] =(byte)(((-38*R - 74*G + 112*B + 128) >> 8) + 128);
                    I420[vi++] =(byte)(((112*R - 94*G - 18*B + 128) >> 8) + 128); 
                }
            }
    }
    
    //Bitmap -> RGB raw data
    private void setMuteCameraImage(byte rgbData[] ,int layer)
    {
        if(rgbData == null || mMuteCameraBitmap == null)
            return;
        
        //put the image of mute camera into the center of screen
        int muteImageSize_W = (int)mMuteCameraBitmap.getWidth();
        int muteImageSize_H = (int)mMuteCameraBitmap.getHeight();
        if(muteImageSize_W > mWidth)
            muteImageSize_W = mWidth;
        if(muteImageSize_H > mHeight)
            muteImageSize_H = mHeight;
        
        int start_x = (mWidth  - muteImageSize_W)/2;
        int start_y = (mHeight - muteImageSize_H)/2;
        
        int[] pix = new int[mMuteCameraBitmap.getWidth() * mMuteCameraBitmap.getHeight()];
        mMuteCameraBitmap.getPixels(pix, 0, mMuteCameraBitmap.getWidth(), 0, 0, mMuteCameraBitmap.getWidth(), mMuteCameraBitmap.getHeight());
        
        for (int y=0; y < muteImageSize_H; y++)
        {
            for (int x=0; x < muteImageSize_W; x++)
            {
                int index = y * mMuteCameraBitmap.getWidth() + x;
                int R = (pix[index] >> 16) & 0xff;     //bitwise shifting
                int G = (pix[index] >> 8) & 0xff;
                int B = pix[index] & 0xff;
                
                rgbData[(y+start_y)*mWidth*layer+(x+start_x)*layer+0] = (byte)R;
                rgbData[(y+start_y)*mWidth*layer+(x+start_x)*layer+1] = (byte)G;
                rgbData[(y+start_y)*mWidth*layer+(x+start_x)*layer+2] = (byte)B;
            }
        }
    }

    //Mute Camera timer
//    private Runnable muteTimer = new Runnable() 
//    {    	
//        public void run()
//        {
//            // stop timer
//            if(!mIsSilence){
//                return;
//            }
//
//            int size = (int) (targetW*targetH*1.5f);
//            if(mpI420==null)
//            {  
//                mpI420 = new byte[size];
//                if(mMuteCameraBitmap == null) {
//                	//Log.d(DEBUGTAG,"Doesn't find the image of mute camera");
//                }
//                else {
//                    //pixel convert process
//                	mpI420 = getNV21(mMuteCameraBitmap.getWidth(), mMuteCameraBitmap.getHeight(),mMuteCameraBitmap);
//                }
//            }
//            
//            try
//            {
//                if(mSink != null)
//                    mSink.onMedia(ByteBuffer.wrap(mpI420),
//                                  size,
//                                  System.currentTimeMillis(), 
//                                  (short) 0);
//            } 
//            catch (Throwable e) {
//                Log.e(DEBUGTAG, "handleMessage", e);
//            }
//            //recall timer
//            mTimerHandler.postDelayed(this, 1000);
//        }
//    };
    
    public void setLocalMute(){
    	synchronized (mSurfaceLock) {
	    	if(mCameraPreview!=null && mCameraPreview.mSurface!=null && mCameraPreview.mIsReady && mMuteCameraBitmap!=null){
		    	Canvas canvas  = mCameraPreview.mSurface.lockCanvas(null);
		    	Matrix matrix = _GetPreferedWidthAndHeightForLBW(mMuteCameraBitmap.getWidth(), mMuteCameraBitmap.getHeight(), mCameraPreview.getHeight(), mCameraPreview.getWidth());
		    	canvas.drawBitmap(mMuteCameraBitmap, matrix, null);
		    	mCameraPreview.mSurface.unlockCanvasAndPost(canvas);
	    	}
    	}
    }
    private Matrix _GetPreferedWidthAndHeightForLBW(int resWidth,int resHeight, int desHeight, int desWidth)
    {
    	Matrix mtx = new Matrix();
        float scale_x  = (float)desWidth/(float)resWidth;
        float scale_y  = (float)desHeight/(float)resHeight;

        mtx.reset();
        mtx.postScale(scale_x, scale_y);
        return mtx;
    }
        
    
    public void setMute (boolean enable, Bitmap bitmap)
    {
    	Log.d(DEBUGTAG,"setMute "+ enable);
    	
        if(mIsSilence == enable)
            return;
        
        frameCnt = 0;
    	mEnableCameraMute = enable;
        mIsSilence = enable;
        
        if(mIsSilence) {   
            mMuteCameraBitmap = bitmap;
                        
            mpI420 = null;
            if(mTimerHandler == null)
            	mTimerHandler = new Handler(Looper.getMainLooper());
            
//            mTimerHandler.removeCallbacks(muteTimer); // Frank: remove unused timer
            startMuteTimer();
            
            internalStop();
        }
        else {
            try {
                start();
            } 
            catch (FailedOperateException e) {
                Log.e(DEBUGTAG, "setMute", e);
            }
        }
        
        if(mSink instanceof OpenH264EncodeTransform) {
        	((OpenH264EncodeTransform) mSink).setMute(mIsSilence);
        }
    }
    
    private void startMuteTimer() { //some devices cant postdelayed. so use this method to workaround.
        Runnable r = new Runnable() {			
			@Override
			public void run() {
	            if(!mIsSilence) {
	                return;
	            }

	            int size = (int) (targetW*targetH*1.5f);
	            if(mpI420 == null) {
	                if(mMuteCameraBitmap == null) {
	                	Log.d(DEBUGTAG,"Doesn't find the image of mute camera");
	                }
	                else {
	                	try {
	                		mpI420 = getEncodedMute();
						} 
	                	catch (IOException e) {
							Log.e(DEBUGTAG,"",e);
						}
	                }
	            }
	            
	            try {
	            	setLocalMute();
	                if(mSink != null)
	                    mSink.onMedia(ByteBuffer.wrap(mpI420),
	                                  mpI420.length,
	                                  System.currentTimeMillis(), 
	                                  (short) 0);
	            } 
	            catch (Throwable e) {
	                Log.e(DEBUGTAG, "handleMessage", e);
	            }
	            
	            if(mTimerHandler==null) {
	            	mTimerHandler = new Handler(Looper.getMainLooper());
	            }
	            
	            if(frameCnt < 150) {
	            	frameCnt ++;
	            	mTimerHandler.postDelayed(this, 33);
	            }
	            else {
	            	mTimerHandler.postDelayed(this, 2000);
	            }
			}
		};
		
        if(mTimerHandler==null)
        	mTimerHandler = new Handler(Looper.getMainLooper());
        
		mTimerHandler.post(r);
	}

	public Size getResolution()
    {
        Size result = null;
        synchronized (mCameraLock)
        {
            if (mCamera != null)
            {
                Camera.Parameters parameters = mCamera.getParameters();
                result = parameters.getPreviewSize();
            }
        }
        return result;
    }

    public int getRotationDegree()
    {
        return mRotationDegree;
    }

    public boolean isDualCamera()
    {
        return mIsDualCamera;
    }

    public short getCameraMode()
    {
        return (short)mCameraID;
    }

    public SurfaceView getPreview(Context context)
    {
    	Log.d(DEBUGTAG,"getPreview (for camera preview)");
    	
    	if(mLocalPreview != null) {
    		mLocalPreview = null;
    	}    	
    	mContext = context;
    	mLocalPreview = new LocalPreview(context);
   		mLocalPreview.setAspectRatio(mHeight, mWidth);
    	
        return mLocalPreview;
    }
    
    public SurfaceView getCameraView(Context context)
    {
    	Log.d(DEBUGTAG, "getCameraView (for mute image)");
    	
    	if(mCameraPreview == null) {
    		mContext = context;
    		mCameraPreview = new CameraPreview(context);
    	}
        return mCameraPreview;
    }
    
    public void setScale(float x_scale, float y_scale)
    {
    	Log.d(DEBUGTAG,"setScale(), x:"+x_scale+" y:"+y_scale);
        m_x_scale = x_scale;
        m_y_scale = y_scale;
    }

    
    public void setupCamera(int format, int width, int height, int camera_id, int frameRate, boolean enableCamera)
            throws FailedOperateException
    {
    	Log.d(DEBUGTAG, "setupCamera(), mCameraID:" + mCameraID + 
			    			         ", " + width + "x" + height + 
			    		             ", id:" + camera_id + 
			    		             ", frameRate:" + frameRate + 
			    		             ", enableCamera:" + enableCamera);
    	
        boolean format_changed = false;
        if (mFormat != format || mWidth != width || mHeight != height || mFrameRate != frameRate || mEnableCameraMute != enableCamera)
        {
            mFormat = format;
            mWidth = width;
            mHeight = height;
            targetW = width;
            targetH = height;
            mFrameRate = frameRate;
            mEnableCameraMute = enableCamera;
            format_changed = true;
            if((width > levelid.width) || (height > levelid.height)){
            	mWidth = levelid.width;
            	mHeight = levelid.height;
                targetW = levelid.width;
                targetH = levelid.height;   	
            }
            if(frameRate > levelid.frame_rate){
            	mFrameRate = levelid.frame_rate;
            }
        }
        
        if (mCameraID != camera_id) {
            mCameraID = camera_id;
            if (mCamera != null && mIsStarted)
            {
                if (Hack.isSamsungI9K())
                {
                    if (!format_changed)
                        cameraRestart();
                }else
                {
                    format_changed = true;
                }
            }
        }
        if (format_changed)
        {
            if (mCamera != null && mIsStarted) {
                internalStop();
                start();
            }
        }
    }

    private void setResolution(int width, int height)
    {
    	Log.d(DEBUGTAG, "setResolution(), " + width + "x" + height);
        try
        {
            mWidth = width;
            mHeight = height;
            if (mCamera != null && mIsStarted) {
                stop();
                start();
            }
            
            if (mLocalPreview != null) {
                mLocalPreview.setAspectRatio(mHeight, mWidth);
            }
        } 
        catch (FailedOperateException e) {
            Log.e(DEBUGTAG, "setResolution", e);
        }
    }

    private void setFrameRate(int framerate)
    {
    	Log.d(DEBUGTAG,"setFrameRate(), " + framerate);
        try
        {
            mFrameRate = framerate;
            if (mCamera != null && mIsStarted) {
                internalStop();
                start();
            }
        } 
        catch (FailedOperateException e) {
            Log.e(DEBUGTAG, "setFrameRate", e);
        }
    }

    private void cameraRestart() throws FailedOperateException
    {
        mHandler.post(new Runnable()
        {
            @SuppressLint("NewApi")
			public void run()
            {
            	Log.d(DEBUGTAG, "cameraRestart(), " + mWidth + "x" + mHeight);

                synchronized (mCameraLock)
                {
                	mCamera = openFrontFacingCamera2();
                    if (mCamera != null)
                    {
                        mCamera.stopPreview();
                        mOnPreviewing = false;
                        Camera.Parameters parameters = mCamera.getParameters();
                        List<Camera.Size> previewSizes = parameters.getSupportedPreviewSizes();
                        Collections.sort(previewSizes, new Comparator<Camera.Size>() {
                        	public int compare(final Camera.Size a, final Camera.Size b) {
                      		  	return b.width * b.height - a.width * a.height;
                        	}
                      	});
                        
                        int target_width = targetW;
                        int target_height = targetH;
                        mRotationDegree = getRotationDegree(mCameraID, mDisplayRotation);
                        for(int i = 0 ;i < previewSizes.size();i++){
                        	Camera.Size previewSize = previewSizes.get(i);
                        	if(isPreviewRotate){
                        		if(previewSize.width>=target_width && previewSize.height>=target_height)
    	                    	{
    	                    		mWidth = previewSize.width;
    	                    		mHeight = previewSize.height;	
    	                    	}
                        	}
                        	else{
                        		if(mRotationDegree == 270 || mRotationDegree == 90){
	    	                       	if(previewSize.width>=target_height && previewSize.height>=target_width)
	    	                    	{
	    	                    		mWidth = previewSize.width;
	    	                    		mHeight = previewSize.height;	
	    	                    	}else{
	    	                    		//break;
	    	                    	}                	
	                        	}
                        		else if (mRotationDegree ==0 || mRotationDegree == 180){
	                        		if(previewSize.width>=target_width && previewSize.height>=target_height)
	    	                    	{
	    	                    		mWidth = previewSize.width;
	    	                    		mHeight = previewSize.height;	
	    	                    	}else{
	    	                    		//break;
	    	                    	}
	                        	}     
                        	}
                        }                        
                        getSupportedPreviewFramerate(parameters);
                        parameters.setPreviewSize(mWidth, mHeight);
                        parameters.setPreviewFormat(mFormat);                   
                        mCamera.setParameters(parameters);
                    }
                    else{
                    	Log.d(DEBUGTAG,"mCamera is null");
                    }
                }
                
                try { 
                    postStart();
                } 
                catch (Throwable e) {
                    Log.e(DEBUGTAG, "postStart", e);
                }
            }
        });
    }
    
    public static void dumpParameters(Parameters parameters) {
        String flattened = parameters.flatten();
        StringTokenizer tokenizer = new StringTokenizer(flattened, ";");
        Log.d(DEBUGTAG, "Dump all camera parameters:");
        while (tokenizer.hasMoreElements()) {
            Log.d(DEBUGTAG, tokenizer.nextToken());
        }
    }
    
    public static boolean mEnableCameraRotation = false;
    public static void setEnableCameraRotation(boolean enableRoatation)
    {
        mEnableCameraRotation = enableRoatation;
    }
    
    private void postStart() throws IOException, FailedOperateException,
            UnSupportException
    {
        Camera.Parameters parameters = null;
        Size size = null;
        
        Log.d(DEBUGTAG,"postStart(), enter");
        
        try
        {
            synchronized (mCameraLock)
            {
                if (mCamera != null && !mOnPreviewing)
                {      
                	frameCnt = 0;
                    mRotationDegree = getRotationDegree(mCameraID, mDisplayRotation);
                    if (mLocalPreview != null && mLocalPreview.mIsReady) {
                    	mCamera.setPreviewDisplay(mLocalPreview.mHolder);
                    }
                        
                    if(mEnableCameraRotation)
                    {
                        //Setup the roatation degree
                        if (MODE_FACE == mCameraID && Build.VERSION.SDK_INT >= 9) //for 2.3+
                        {   // compensate the mirror
                            mCamera.setDisplayOrientation((360 - mRotationDegree) % 360);
                            Camera.Parameters param = mCamera.getParameters();
                            param.setRotation((360 - mRotationDegree) % 360);
                        }
                        else {
                            mCamera.setDisplayOrientation(mRotationDegree);
                        }
                    }
                    else
                        mCamera.setDisplayOrientation(0);
                    
                    PixelFormat pixelinfo = new PixelFormat();
                    int pixelformat = mCamera.getParameters().getPreviewFormat();
                    PixelFormat.getPixelFormatInfo(pixelformat, pixelinfo);

                    int bufSize = mWidth * mHeight * pixelinfo.bitsPerPixel / 8;                    
                    byte[] buffer = new byte[bufSize]; 
                    for(int i = 0; i<1; i++) {
                    	mCamera.addCallbackBuffer(buffer); 
                    	buffer = new byte[bufSize];
                    }
                    mCamera.setPreviewCallbackWithBuffer(this);
                    mCamera.startPreview();
                    
                    parameters = mCamera.getParameters();
                    size = parameters.getPreviewSize();                    
                    //dumpParameters(parameters);
                    
                    mIsStarted = true;
                    mOnPreviewing = true;
                }
            }
        } 
        catch (Throwable err) {
            Log.e(DEBUGTAG, "postStart", err);
            mHandler.post(new Runnable()
            {
                public void run()
                {
                    internalStop();
                    try {
                        start();
                    } 
                    catch (FailedOperateException e) {
                        Log.e(DEBUGTAG, "postStart", e);
                    }
                }
            });
            throw new FailedOperateException(err);
        }

        if (size != null && (size.width > mWidth || size.height > mHeight))
        {
            mHandler.post(new Runnable()
            {
                public void run()
                {
                    Camera.Parameters parameters = null;
                    synchronized (mCameraLock)
                    {
                        if (mCamera != null)
                            parameters = mCamera.getParameters();
                    }
                    if (parameters != null)
                    {
                        List<Size> size_list = parameters.getSupportedPreviewSizes();
                        Size result_size = parameters.getPreviewSize();
                        result_size.width = mWidth;
                        result_size.height = mHeight;
                        int result_dist = 3000;
                        for (int i = 0; i < size_list.size(); i++)
                        {
                            Size tmp_size = size_list.get(i);
                            int tmp_dist = Math.abs(tmp_size.width - result_size.width )+ Math.abs( tmp_size.height - result_size.height);
                            if (tmp_size.width < result_size.width && 
                                tmp_size.height < result_size.height &&
                                tmp_dist < result_dist)
                            {
                                result_size.width = tmp_size.width;
                                result_size.height = tmp_size.height;
                                result_dist = tmp_dist;                                
                            }
                        }
                        setResolution(result_size.width, result_size.height);
                    }
                }
            });
        } 
        else {
            if (parameters != null && size != null){
            	if(mSink instanceof OpenH264EncodeTransform || mSink instanceof MediaCodecEncodeTransform){
            		if(mRotationDegree == 270 || mRotationDegree == 90){
            			mSink.setFormat(parameters.getPreviewFormat(),mHeight, mWidth);
            		}
            		else{
            			mSink.setFormat(parameters.getPreviewFormat(),mWidth, mHeight);
            		}
            	}
            	else{
            		Log.d(DEBUGTAG,"is not mediacodec/openh264");
            		mSink.setFormat(parameters.getPreviewFormat(), targetW, targetH);
            	}
            }
            onDegreeChanged();
        }
        
        Log.d(DEBUGTAG,"postStart(), exit");
    }

	@Override
    public void start() throws FailedOperateException
    {
		boolean result = false;
		
		Log.d(DEBUGTAG,"start(), enter");
		
    	if(Hack.isNEC()) {
    		targetW = 320;
    		targetH = 240;
    	}
        
        if (mCamera != null) {
            internalStop();
        }
        
        mIsSilence = false;
        if(mEnableCameraMute) {
        	mIsSilence = true;

        	if(muteBmp == null) {
        		mRotationDegree = getRotationDegree(mCameraID, mDisplayRotation);
        		if(mRotationDegree==90 || mRotationDegree == 270)
        			muteBmp = Hack.getBitmap(mContext, R.drawable.bg_vc_audio_vga2); 
        		else
        			muteBmp = Hack.getBitmap(mContext, R.drawable.bg_vc_audio_vga); 
        	}
            if(mMuteCameraBitmap == null)
        		mMuteCameraBitmap = getMuteImage();
            
            mpI420 = null;
            if(mTimerHandler == null) {
            	mTimerHandler = new Handler(Looper.getMainLooper());
            }
//            mTimerHandler.removeCallbacks(muteTimer); // Frank: remove unused timer

            startMuteTimer();
            internalStop();
            if(mSink instanceof OpenH264EncodeTransform) {
            	((OpenH264EncodeTransform) mSink).setMute(mIsSilence);
            }
            
            Log.d(DEBUGTAG,"start(), exit");
            
        	return;
        }
        
        synchronized (mCameraLock)
        {       	
        	mCamera = openFrontFacingCamera2();
        	Camera.Parameters parameters = mCamera.getParameters();
            List<Camera.Size> previewSizes = parameters.getSupportedPreviewSizes();

            Collections.sort(previewSizes, new Comparator<Camera.Size>() {
          	  public int compare(final Camera.Size a, final Camera.Size b) {
          	    return b.width * b.height - a.width * a.height;
          	  }
          	});

            int target_width = targetW;
            int target_height = targetH;
            for(int i = 0 ;i < previewSizes.size();i++){
            	Camera.Size previewSize = previewSizes.get(i);
            	//Log.d(DEBUGTAG,"CameraSize at start: "+previewSize.width+"x"+previewSize.height );
            }
            mRotationDegree = getRotationDegree(mCameraID, mDisplayRotation);
            for(int i = 0 ;i < previewSizes.size();i++){
            	Camera.Size previewSize = previewSizes.get(i);
            	if(isPreviewRotate){
            		if(previewSize.width>=target_width && previewSize.height>=target_height)
                	{
                		mWidth = previewSize.width;
                		mHeight = previewSize.height;	
                	}
            	}
            	else{
	            	if(mRotationDegree == 270 || mRotationDegree == 90) {
	                   	if(previewSize.width>=target_height && previewSize.height>=target_width)
	                	{
	                		mWidth = previewSize.width;
	                		mHeight = previewSize.height;	
	                	}              	
	            	}
	            	else if(mRotationDegree==0 || mRotationDegree==180) {
	            		if(previewSize.width>=target_width && previewSize.height>=target_height) {
	                		mWidth = previewSize.width;
	                		mHeight = previewSize.height;	
	                	}
	            	}
            	}
            }   	
        	
            getSupportedPreviewFramerate(parameters);
            parameters.setPreviewSize(mWidth, mHeight);
            parameters.setPreviewFormat(mFormat);
            //parameters.setPreviewFpsRange(10000, 10000);
            
            //----------------------------
            mCamera.setParameters(parameters);

            // mCamera.setPreviewCallback(CameraSource.this);

            // Add buffers to the buffer queue. I re-queue them once
            // they are
            // used in onPreviewFrame, so we should not need many of
            // them.
//            PixelFormat pixelinfo = new PixelFormat();
//            int pixelformat = mCamera.getParameters().getPreviewFormat();
//            PixelFormat.getPixelFormatInfo(pixelformat, pixelinfo);
//            int frameSize = mWidth * mHeight * pixelinfo.bitsPerPixel / 8;
//            mFrameDataDepository = new FrameDataDepository(DEPOSITORY_SIZE, frameSize);
//            mCamera.setPreviewCallbackWithBuffer(this);           
            
            mIsStarted = true;
            result = true;
//            if(QtalkSettings.ScreenSize < 4){
//            	mLocalViewThread = new LocalViewThread();
//            	mLocalViewThread.start();
//            }
        }
        try
        {
        	if (!(mLocalPreview != null && !mLocalPreview.mIsReady))
                postStart();
        } 
        catch (Throwable e) {
            Log.e(DEBUGTAG, "start", e);
            throw new FailedOperateException(e);
        }
        if (!result)
            throw new FailedOperateException("Failed on start CameraSource:"
                    + propertiesToString());
        
        Log.d(DEBUGTAG,"start(), exit");
    }

    private WindowManager getSystemService(String windowService) {
		return null;
	}

	@Override
    public void stop()
    {
        internalStop();
        mIsSilence = false;
    }
	
    public void internalStop()
    {
    	
    	Log.d(DEBUGTAG,"internalStop(), enter");
    	
        synchronized (mCameraLock)
        {
            if (mCamera != null)
            {
                mCamera.setPreviewCallback(null);
                mCamera.stopPreview();
                mCamera.release();
                mCamera = null;
                mOnPreviewing = false;
            }
        }
        
        mIsStarted = false;
//        if (mLocalViewThread != null) {
//        	mLocalViewThread = null;
//        }

        // Frank: remove unused timer
//        if (mTimerHandler != null) {
//        	mTimerHandler.removeCallbacks(muteTimer);
//        }
        
        Log.d(DEBUGTAG,"internalStop(), exit");
    }

    @Override
    public void setSink(IVideoSink sink) throws FailedOperateException,
            UnSupportException
    {
        try
        {
            if (mCamera != null) {
                Camera.Parameters parameters = mCamera.getParameters();
                Size size = parameters.getPreviewSize();
                sink.setFormat(parameters.getPreviewFormat(), targetW, targetH);
            }
            else {
            	sink.setFormat(mFormat, targetW, targetH);
            }
            mSink = sink;
        } 
        catch (Throwable e) {
            Log.e(DEBUGTAG, "setSink", e);
            internalStop();
            throw new FailedOperateException("Failed on CameraSource.SetSink:" + propertiesToString());
        }
    }
    
    long startTime_msec = 0;
    long previewframeCnt = 0;
    
    @Override
    public void onPreviewFrame(final byte[] data, Camera camera)
    {
        byte[] output = null;
    	long curTime_msec = System.currentTimeMillis();

    	previewframeCnt++;
    	if((curTime_msec - startTime_msec) > 3000) {
    		long fps = previewframeCnt / ((curTime_msec-startTime_msec)/1000);
    		Log.d(DEBUGTAG, "=> "+fps+"fps (fk)");
    		startTime_msec = curTime_msec;
    		previewframeCnt = 0;
    	}
    	
    	
//        if(LocalViewData==null){
//            LocalViewData = new byte[targetW*targetH*3/2];
//        }
		
        //Log.d(DEBUGTAG,"targetW:"+targetW+",targetH:"+targetH+",mWidth:"+mWidth+",mHeight:"+mHeight+",mRotationDegree:"+mRotationDegree);
		//output = _rotateAndCropAndtoYuv420(data, targetW, targetH, mWidth, mHeight, mRotationDegree); 
		//output = _rotateAndCropAndtoYuv420SP(data, targetW, targetH, mWidth, mHeight, mRotationDegree); 
		//output = _rotateAndCrop(data, targetW, targetH, mWidth, mHeight, mRotationDegree);    
		//output = _rotate(data, mWidth, mHeight, mRotationDegree);
		//output = _rotateAndtoYuv420SP(data, mWidth, mHeight, mRotationDegree);
    	
		output = _rotateAndtoYuv420(data, mWidth, mHeight, mRotationDegree);
		try {
			mSink.onMedia(ByteBuffer.wrap(output), output.length, System.currentTimeMillis(), (short) mRotationDegree);
		} catch (UnSupportException e) {
			Log.e(DEBUGTAG,"",e);
		}
    	camera.addCallbackBuffer(data);
    }
    
    
//    class LocalViewThread extends Thread
//    {
//    	private int[] rgbArray = new int[targetW*targetH];
//    	private Canvas canvas = null;
//    	
//    	public void decodeN21toRGB565(int[] rgb, byte[] yuv420sp, int width, int height){
//    		
//    		Log.d(DEBUGTAG, "LocalViewThread(), decodeN21toRGB565: "+width+"x"+height);
//    		
//        	int frameSize = width * height;
//        	int rgb_index = 0;
//        	for (int j = 0, yp = 0; j < height; j++) {
//        		int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;
//        		for (int i = 0; i < width; i++, yp++) {
//        			int y = (0xff & ((int) yuv420sp[yp])) - 16;
//        			if (y < 0) y = 0;
//        			if ((i & 1) == 0) {
//        				v = (0xff & yuv420sp[uvp++]) - 128;
//        				u = (0xff & yuv420sp[uvp++]) - 128;
//        			}
//        			
//        			int y1192 = 1192 * y;
//        			int r = (y1192 + 1634 * v);
//        			int g = (y1192 - 833 * v - 400 * u);
//        			int b = (y1192 + 2066 * u);
//        			
//        			if (r < 0) r = 0; else if (r > 262143) r = 262143;
//        			if (g < 0) g = 0; else if (g > 262143) g = 262143;
//        			if (b < 0) b = 0; else if (b > 262143) b = 262143;
//        			
//        			//mirror the image
//        			rgb_index = (width-1-i) + j*width;
//        			rgb[rgb_index] = 0xff000000 | ((r << 6) & 0xff0000) | ((g >> 2) & 0xff00) | ((b >> 10) & 0xff);
//        		}
//        	}
//        }
//    	
//    	@Override
//    	public void run() {
//    		    		
//    		thisLocalThread = this.hashCode();
//    		android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_DEFAULT);
//    		
//    		Log.d(DEBUGTAG, "LocalViewThread(), run(), start: "+thisLocalThread);
//    		
//    		while(mIsStarted && thisLocalThread == this.hashCode()) {
//    			if (UpdateLocalView){
//    				boolean block = mLocalOnNewData.block(5000);
//                    if (block==false && thisLocalThread != this.hashCode()) 
//                    	break;
//                   	mLocalOnNewData.close();
//                   	Log.d(DEBUGTAG, "UpdateLocalView");
//                } 
//    			else {
//        			synchronized (mSurfaceLock) {        				
//        				if (mCameraPreview != null && mCameraPreview.mSurface != null) {
//        					try {
//                                Bitmap bmp = null;
//                                Bitmap reBmp = null;
//                                synchronized (mLocalViewLock) {
//                                	if(LocalViewData!=null){
//                                		decodeN21toRGB565(rgbArray, LocalViewData, targetW, targetH);
//                                	}
//                            		UpdateLocalView =true;
//                            	}
//                                
//                                Log.d(DEBUGTAG, "drawBitmap");
//            					canvas = mCameraPreview.mSurface.lockCanvas(null);            					
//                                if (canvas != null) {	
//                                	 Matrix matrix = new Matrix();
//                                     matrix.postScale(m_x_scale, m_y_scale);
//                                     bmp = Bitmap.createBitmap(rgbArray, targetW, targetH, Bitmap.Config.RGB_565);
//                                     reBmp = Bitmap.createBitmap(bmp, 0, 0, targetW, targetH, matrix, true);
//                                    canvas.drawBitmap(reBmp, 0, 0, null);
//                                }
//                                mCameraPreview.mSurface.unlockCanvasAndPost(canvas);
//                                
//                                if (bmp != null) {
//                                    bmp.recycle();
//                                    bmp = null;
//                                }
//                                
//                                if (reBmp != null) {
//                                	reBmp.recycle();
//                                	reBmp = null;
//                                }
//
//            				} catch (IllegalArgumentException e) {
//            					Log.e(DEBUGTAG,"",e);
//            				} catch (OutOfResourcesException e) {
//            					Log.e(DEBUGTAG,"",e);
//            				}                   				
//        				}                             
//        			}
//                }
//    		}
//    		Log.d(DEBUGTAG, "LocalViewThread(), run(), exit");
//    		UpdateLocalView =true;
//    	}    
//    }

    class VideoPreview extends SurfaceView
    {
        private float mAspectRatio;
        private int mHorizontalTileSize = 1;
        private int mVerticalTileSize = 1;

        /**
         * Setting the aspect ratio to this value means to not enforce an aspect
         * ratio.
         */
        public  float DONT_CARE = 0.0f;

        public VideoPreview(Context context)
        {
            super(context);
        }

        public VideoPreview(Context context, AttributeSet attrs)
        {
            super(context, attrs);
        }

        public VideoPreview(Context context, AttributeSet attrs, int defStyle)
        {
            super(context, attrs, defStyle);
        }

        public void setTileSize(int horizontalTileSize, int verticalTileSize)
        {
            if ((mHorizontalTileSize != horizontalTileSize)
                    || (mVerticalTileSize != verticalTileSize))
            {
                mHorizontalTileSize = horizontalTileSize;
                mVerticalTileSize = verticalTileSize;
                requestLayout();
                invalidate();
            }
        }

        public void setAspectRatio(int width, int height)
        {
            setAspectRatio(((float) width) / ((float) height));
        }

        public void setAspectRatio(float aspectRatio)
        {
            if (mAspectRatio != aspectRatio)
            {
                mAspectRatio = aspectRatio;
                requestLayout();
                invalidate();
            }
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
        {
            if (mAspectRatio != DONT_CARE)
            {
                int widthSpecSize = MeasureSpec.getSize(widthMeasureSpec);
                int heightSpecSize = MeasureSpec.getSize(heightMeasureSpec);

                int width = widthSpecSize;
                int height = heightSpecSize;

                if (width > 0 && height > 0)
                {
                    float defaultRatio = ((float) width) / ((float) height);
                    if (defaultRatio < mAspectRatio)
                    {
                        // Need to reduce height
                        height = (int) (width / mAspectRatio);
                    } else if (defaultRatio > mAspectRatio)
                    {
                        width = (int) (height * mAspectRatio);
                    }
                    width = roundUpToTile(width, mHorizontalTileSize, widthSpecSize);
                    height = roundUpToTile(height, mVerticalTileSize, heightSpecSize);
                    setMeasuredDimension(width, height);
                    return;
                }
            }
            super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        }

        private int roundUpToTile(int dimension, int tileSize, int maxDimension)
        {
            return Math.min(((dimension + tileSize - 1) / tileSize) * tileSize,
                    maxDimension);
        }
    }

//    class LocalLayout extends android.widget.RelativeLayout
//    {
//        private LocalPreview _mView = null;
//        private int _mWidth = 0;
//        private int _mHeight = 0;
//        private float mAspectRatio;
//
//        public LocalLayout(LocalPreview localView, Context context)
//        {
//            super(context);
//            _mView = localView;
//        }
//
//        public void setAspectRatio(int width, int height)
//        {
//            setAspectRatio(((float) width) / ((float) height));
//        }
//
//        public void setAspectRatio(float aspectRatio)
//        {
//            if (mAspectRatio != aspectRatio)
//            {
//                mAspectRatio = aspectRatio;
//                relayout(_mWidth, _mHeight);
//            }
//        }
//
//        private void relayout(int des_width, int des_height)
//        {
//            int width = des_width;
//            int height = des_height;
//            int left = 0;
//            int top = 0;
//            if (width > 0 && height > 0)
//            {
//                float defaultRatio = ((float) width) / ((float) height);
//                if (defaultRatio < mAspectRatio)
//                {// Need to reduce height
//                    height = (int) (width / mAspectRatio);
//                } else if (defaultRatio > mAspectRatio)
//                {
//                    width = (int) (height * mAspectRatio);
//                }
//                width = roundUpToTile(width, 1, des_width);
//                height = roundUpToTile(height, 1, des_height);
//                left = (des_width - width) / 2;
//                top = (des_height - height) / 2;
//            }
//            RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) _mView
//                    .getLayoutParams();
//
//            // _mView.layout(left, top , left+width, top+height);
//            params.setMargins(left, top, left, top);
//            _mView.setLayoutParams(params);
//        }
//
//        private int roundUpToTile(int dimension, int tileSize, int maxDimension)
//        {
//            return Math.min(((dimension + tileSize - 1) / tileSize) * tileSize,
//                    maxDimension);
//        }
//
//        @Override
//        protected void onSizeChanged(int w, int h, int oldw, int oldh)
//        {
//            if (_mWidth != w || _mHeight != h)
//            {
//                _mWidth = w;
//                _mHeight = h;
//                relayout(_mWidth, _mHeight);
//            }
//        }
//    }

    class LocalPreview extends VideoPreview implements SurfaceHolder.Callback
    {
        SurfaceHolder mHolder = null;
        boolean mIsReady = false;

        LocalPreview(Context context)
        {
            super(context);
            
            // Install a SurfaceHolder.Callback so we get notified when the
            // underlying surface is created and destroyed.
            Log.d(DEBUGTAG, "==> LocalPreview(), Context");
            mHolder = getHolder();
            mHolder.addCallback(this);
            mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
            mIsReady = false;
       }

        public void surfaceCreated(SurfaceHolder holder)
        {
        	Log.d(DEBUGTAG, "==> LocalPreview(), surfaceCreated");
            mHolder = holder;
            mIsReady = true;
        }

        public void surfaceDestroyed(SurfaceHolder holder)
        {
        	Log.d(DEBUGTAG, "==> LocalPreview(), surfaceDestroyed "+this.hashCode());
            // Surface will be destroyed when we return, so stop the preview.
            // Because the CameraDevice object is not a shared resource, it's
            // very
            // important to release it when the activity is paused.
            mHolder = null;
            mIsReady = false;
            mHandler.post(new Runnable()
            {
                public void run()
                {
                    if (mIsStarted)
                    {
                        try {
                        	/* DX80 transfer get some problem, dont stop camera here */
                            //internalStop(); 
                        } 
                        catch (Throwable e) {
                            Log.e(DEBUGTAG, "surfaceDestroyed", e);
                        }
                        mIsStarted = false; //true;
                    }
                }
            });
        }

        public void surfaceChanged(SurfaceHolder holder, final int format, final int w, final int h)
        {
        	Log.d(DEBUGTAG, "==> LocalPreview(), surfaceChanged:" + w + "x" + h);
        	
            if (mOnSurfaceChanging)
                return;
            
            mOnSurfaceChanging = true;
            mHandler.post(new Runnable()
            {
                public void run()
                {               		
                	try
                    {
                        if (mIsStarted)
                        {
                        	if(Hack.isQF5() || Hack.isQF3a() || Hack.isQF3() || Hack.isK72()) {
	                            if (mCamera == null) {
	                                start();
	                            } 
	                            else {
	                                synchronized (mCameraLock)
	                                {
	                                    if (mCamera != null) {
	                                        mCamera.stopPreview();
	                                        mOnPreviewing = false;
	                                    }
	                                }
	                                postStart();
	                            }
                        	}
                        }
                    } 
                	catch (Throwable e) {
                        Log.e(DEBUGTAG, "surfaceChanged", e);
                    }
                	mOnSurfaceChanging = false;
                }
            });
        }
    }
        
    class CameraPreview extends VideoPreview
    				    implements SurfaceHolder.Callback
    {
    	SurfaceHolder mHolder;
        boolean mIsReady = false;
        private Surface mSurface = null;

		CameraPreview(Context context)
        {
            super(context);
            
            Log.d(DEBUGTAG, "==> CameraPreview(), Context");
            synchronized (mSurfaceLock) {
            	mHolder = getHolder();
                mHolder.addCallback(this);
                mHolder.setType(SurfaceHolder.SURFACE_TYPE_NORMAL);
                mIsReady = false;
            	mSurface = null;
            }
        }

        public void surfaceCreated(SurfaceHolder holder)
        {
        	Log.d(DEBUGTAG, "==> CameraPreview(), surfaceCreated");
        	synchronized (mSurfaceLock) {
	    		mHolder = holder;
	            mIsReady = true;
	            mSurface = holder.getSurface();
        	}
        }

        public void surfaceDestroyed(SurfaceHolder holder)
        {
        	Log.d(DEBUGTAG, "==> CameraPreview(), surfaceDestroyed");
        	synchronized (mSurfaceLock) {   
        		mHolder = null;
                mIsReady = false;
                mSurface = null;
        	}
        }

        public void surfaceChanged(SurfaceHolder holder, final int format,
                                   final int w, final int h)
        {
        	Log.d(DEBUGTAG, "==> CameraPreview(), surfaceChanged:"+w+"x"+h);
        }
    }

    private static final int getRotationDegree(int cameraID, int rotation)
    {    	
    	Log.d(DEBUGTAG, "getRotationDegree(), "+cameraID+", "+rotation);
    	
        int result = getRotationDegree2_3(cameraID,rotation);
        if (result >= 0)
        {
            return result;
        }
        if (Hack.isSamsungI9K() || Hack.isSamsungTablet())
        {
            return getRotationDegreeSamsung(cameraID,rotation);
        }
        return 0;
    }

    /**
     * Open the camera.  First attempt to find and open the front-facing camera.
     * If that attempt fails, then fall back to whatever camera is available.
     *
     * @return aCamera object
     */
    private static final int getRotationDegree2_3(int cameraID, int rotation)
    {
    	Log.d(DEBUGTAG, "getRotationDegree2_3(), "+cameraID+", "+rotation);
        try 
        {
            int degrees = 0;
            Class<?> cameraClass = Class.forName("android.hardware.Camera");
            Object cameraInfo = null;
            Field field = null;

            int cameraCount = 0;
            Method getNumberOfCamerasMethod = cameraClass.getMethod( "getNumberOfCameras" );
            if ( getNumberOfCamerasMethod != null ) 
            {
                cameraCount = (Integer) getNumberOfCamerasMethod.invoke( null, (Object[]) null );
            }
            Class<?> cameraInfoClass = Class.forName("android.hardware.Camera$CameraInfo");
            if ( cameraInfoClass != null ) 
            {
                cameraInfo = cameraInfoClass.newInstance();
            }
            if ( cameraInfo != null )
            {
                field = cameraInfo.getClass().getField( "orientation" );
            }
            Method getCameraInfoMethod = cameraClass.getMethod( "getCameraInfo", Integer.TYPE, cameraInfoClass );
            if ( getCameraInfoMethod != null && cameraInfoClass != null && field != null ) 
            {
                if(Camera.getNumberOfCameras() >= 2)
                    getCameraInfoMethod.invoke( null, cameraID==MODE_FACE?1:0, cameraInfo );//CameraInfo.CAMERA_FACING_BACK:0,CameraInfo.CAMERA_FACING_FRONT,1
                else if(cameraID!=MODE_FACE)
                    getCameraInfoMethod.invoke( null, 0, cameraInfo );
                else {
                    Log.d(DEBUGTAG, "skip do getCameraInfo");
                    return -1;// only one camera and cameraID==MODE_FACE
                }
                switch (rotation) 
                {
                    case Surface.ROTATION_0: degrees = 0; break;
                    case Surface.ROTATION_90: degrees = 90; break;
                    case Surface.ROTATION_180: degrees = 180; break;
                    case Surface.ROTATION_270: degrees = 270; break;
                }
                int result;
                if (MODE_FACE == cameraID)
                {
                    result = (field.getInt( cameraInfo )+ degrees) % 360;
                    //result = (360 - result) % 360;  // compensate the mirror
                } else 
                {  // back-facing
                    result = (field.getInt( cameraInfo )-degrees + 360) % 360;
                }
                return result;
            }
        }
        // Ignore the bevy of checked exceptions the Java Reflection API throws - if it fails, who cares.
        catch ( ClassNotFoundException e        ) {Log.e(DEBUGTAG, "ClassNotFoundException" + e.getLocalizedMessage());}
        catch ( NoSuchMethodException e         ) {Log.e(DEBUGTAG, "NoSuchMethodException" + e.getLocalizedMessage());}
        catch ( NoSuchFieldException e          ) {Log.e(DEBUGTAG, "NoSuchFieldException" + e.getLocalizedMessage());}
        catch ( IllegalAccessException e        ) {Log.e(DEBUGTAG, "IllegalAccessException" + e.getLocalizedMessage());}
        catch ( InstantiationException e        ) {Log.e(DEBUGTAG, "InstantiationException" + e.getLocalizedMessage());}
        catch ( InvocationTargetException e     ) {Log.e(DEBUGTAG, "InvocationTargetException" + e.getLocalizedMessage());}
        catch ( SecurityException e             ) {Log.e(DEBUGTAG, "SecurityException" + e.getLocalizedMessage());}
     
        return -1;
    }
    
    private static final int getRotationDegreeSamsung(int cameraID, int rotation)
    {
    	Log.d(DEBUGTAG, "getRotationDegreeSamsung(), "+cameraID+", "+rotation);
        int degrees = 0;
        switch (rotation)
        {
        case Surface.ROTATION_0:
            degrees = 0;
            break;
        case Surface.ROTATION_90:
            degrees = 90;
            break;
        case Surface.ROTATION_180:
            degrees = 180;
            break;
        case Surface.ROTATION_270:
            degrees = 270;
            break;
        }
        int result = degrees;
        if (cameraID == MODE_FACE)// Camera.CameraInfo.CAMERA_FACING_FRONT
        {
            result = (360 - result) % 360; // compensate the mirror
        } 
        else // Camera.CameraInfo.CAMERA_FACING_BACK
        {
            // result = (270 + result) % 360;
            if (degrees == 90)
                result = 0;
            else if (degrees == 0)
                result = 90;
            else if (degrees == 270)
                result = 180;
            else if (degrees == 180)
                result = 270;
            // Log.d(DEBUGTAG, "CAMERA_FACING_BACK: result:"+result+
            // " rotation:"+rotation+" degrees:"+degrees);
        }
        return result;
    }

    public void setDisplayRotation(int rotation)
    {
    	Log.d(DEBUGTAG, "setDisplayRotation(), "+rotation);
        if (mDisplayRotation != rotation)
        {
            mDisplayRotation = rotation;
            try
            {
                if (mCamera != null && mIsStarted)
                {
                	cameraRestart();
                	internalStop();
                	mIsStarted = true;
                }
            } catch (Throwable e)
            {
                Log.e(DEBUGTAG, "setDisplayRotation", e);
            }
        }
    }

    private void onDegreeChanged()
    {
    	Log.d(DEBUGTAG, "onDegreeChanged()");
    	mHandler.post(new Runnable()
        {
            public void run()
            {
                if (mRotationDegree == 90 || mRotationDegree == 270)
                {
                    if (mLocalPreview != null)
                        mLocalPreview.setAspectRatio(mHeight, mWidth);
//                    if (mLocalLayout != null)
//                        mLocalLayout.setAspectRatio(mHeight, mWidth);
                } else
                {
                    if (mLocalPreview != null)
                        mLocalPreview.setAspectRatio(mWidth, mHeight);
//                    if (mLocalLayout != null)
//                        mLocalLayout.setAspectRatio(mWidth, mHeight);
                }
            }
        });
    }

    // obtain supported frame rate list for camera and pick proper frame rate from the list
    private void getSupportedPreviewFramerate(final Camera.Parameters parameters)
    {
    	Log.d(DEBUGTAG, "getSupportedPreviewFramerate(), mFrameRate "+mFrameRate+"fps");
    	
        @SuppressWarnings("deprecation")
		List<Integer> support_framerate_list = parameters.getSupportedPreviewFrameRates(); // get supported frame rate list
        Collections.sort(support_framerate_list); // sort from smallest one

        int temp_fps = 0;
        for(int i = 0 ; i< support_framerate_list.size(); i++)
        {
            Log.d(DEBUGTAG,"support frame rate["+i+"]:"+temp_fps);
            temp_fps = support_framerate_list.get(i);
            if(mFrameRate < temp_fps)
                break;
        }
        if(temp_fps > MINIMUM_FPS && temp_fps > mFrameRate) 
            mFrameRate = temp_fps;
        else if(support_framerate_list.contains(MINIMUM_FPS)) // temp_fps is smaller than MINIMUM_FPS or mFrameRate, set mFrameRate as MINIMUM_FPS
            mFrameRate = MINIMUM_FPS;
        else // mFrameRate is not supported and MINIMUM_FPS is not contained in list
            mFrameRate = temp_fps;
        
        Log.d(DEBUGTAG,"==> findSupportPreviewFramerate:"+mFrameRate);
    }
    
    private Camera openFrontFacingCamera2() {
    	
    	Log.d(DEBUGTAG, "openFrontFacingCamera2()");
    	
    	int cameraCount = 0;
    	Camera cam = null;
    	Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
    	
    	cameraCount = Camera.getNumberOfCameras();
    	for (int camIdx = 0; camIdx<cameraCount; camIdx++) {
    	    Camera.getCameraInfo(camIdx, cameraInfo);
    	    if (cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
    	        try {
    	            cam = Camera.open(camIdx);
    	            if(Hack.isQF3()){
    	            	cam = Camera.open(0);
    	            }
    	            Log.d(DEBUGTAG, "Choose camera index : "+camIdx);
    	            break;
    	        } catch (RuntimeException e) {
    	            Log.e(DEBUGTAG, "Facing Camera failed to open: " + e.getLocalizedMessage());
    	        }
    	    }
    	}
    	if(cam == null){
    		try{
    			Log.d(DEBUGTAG, "This device may has not facing camera, open others");
    			cam = Camera.open();
    		}
    		catch (RuntimeException e) {
	            Log.e(DEBUGTAG, "Facing Camera failed to open: " + e.getLocalizedMessage());
	        }
    	}
    	return cam;
    }
    
    Bitmap getMuteImage(){
    	
    	Log.d(DEBUGTAG, "getMuteImage(), "+targetW+"x"+targetH+", "+mRotationDegree);
    	
    	int width = muteBmp.getWidth();
    	int height = muteBmp.getHeight();
    	int newWidth = 0;
    	int newHeight = 0;
    	if(mRotationDegree==90 || mRotationDegree == 270){
    		newWidth = targetH;
    		newHeight = targetW;
    	}
    	else{
    		newWidth = targetW;
    		newHeight = targetH;
    	}
    	float scaleWidth = ((float) newWidth) / width;
    	float scaleHeight = ((float) newHeight) / height;
    	Log.d(DEBUGTAG,"bmp width:"+width+", height:"+height+", newWidth:"+newWidth+", newHeight:"+newHeight);
    	Matrix mtx = new Matrix();
    	mtx.postScale(scaleWidth, scaleHeight);
    	return Bitmap.createBitmap(muteBmp, 0, 0, width, height, mtx, true);
    }
    
    @Override
    public void destory()
    {
    	Log.d(DEBUGTAG, "destory()");
    	
//        mLocalLayout = null;
        mLocalPreview = null;
        synchronized (mSurfaceLock) {
        	mCameraPreview = null;
        }
        
        mCamera = null;
//        mLocalViewThread = null;
        mSink = null;
        mFrameDataDepository = null;
        mpI420 = null;
        if (mMuteCameraBitmap != null) {
            mMuteCameraBitmap.recycle();
            mMuteCameraBitmap = null;
        } 
        if (muteBmp != null){
        	muteBmp.recycle();
        	muteBmp = null;
        }
        mTimerHandler = null;
    }
    
    static
    {
        System.loadLibrary("media_camera_source");
    }
    private static native int _init();
    private static native byte[] _rotate(final byte[] input, final int realWidth, final int realHeight, 
    											final int rotation);
    private static native byte[] _rotateAndtoYuv420SP(final byte[] input, final int realWidth, final int realHeight, final int rotation);
    private static native byte[] _rotateAndtoYuv420(final byte[] input, final int realWidth, final int realHeight, final int rotation);
    private static native byte[] _rotateAndCrop(final byte[] input,final int targetWidth,final int targetHeight, 
    											final int realWidth, final int realHeight, final int rotation);
    private static native byte[] _rotateAndCropAndtoYuv420SP(final byte[] input,final int targetWidth,final int targetHeight, 
			final int realWidth, final int realHeight, final int rotation);
    private static native byte[] _rotateAndCropAndtoYuv420(final byte[] input,final int targetWidth,final int targetHeight, 
			final int realWidth, final int realHeight, final int rotation);
}
