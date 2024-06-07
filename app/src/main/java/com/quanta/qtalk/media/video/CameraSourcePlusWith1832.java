package com.quanta.qtalk.media.video;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.R;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.ui.InVideoSessionActivity;
import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Matrix;
import android.graphics.PixelFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;

@TargetApi(Build.VERSION_CODES.GINGERBREAD)
public class CameraSourcePlusWith1832 implements IVideoSource, IVideoMute, IVideoEncoderCB, Camera.PreviewCallback
{	
//    private static final String TAG = "CameraSourcePlus";
    private static final String DEBUGTAG = "QTFa_CameraSourcePlus1832";
    private Context mContext = null;
    private CameraPreview mCameraPreview = null;
    private H264Preview mH264Preview = null;
    private Camera mCamera = null;
    private static final Object mCameraLock = new Object();
    private static final Object mSurfaceLock = new Object();
    private int mFormat = ImageFormat.NV21;
    //private int mFormat = PixelFormat.YCbCr_420_SP;
    private int mWidth = 0;
    private int mHeight = 0;
    private int mCameraID = 2;
    private int mRotationDegree = 0;
    private int mDisplayRotation = 0;
    private float m_x_scale=1.0f,m_y_scale=1.0f;
    private static final int MINIMUM_FPS = 15;
    private int mFrameRate = MINIMUM_FPS;
    private boolean mOnPreviewing = false;
    private long mStartTime = 0;
    
    
    private IVideoSink mSink = null;
    private boolean mIsStarted = false;
    private final Handler mHandler = new Handler();
    private Handler UpdateLocalHandler = new Handler();

    private static boolean mIsDualCamera = true;
    
    //mute Camera
    private boolean mIsSilence = false;
    private byte [] mpI420 = null;
    private Bitmap mMuteCameraBitmap = null;
    private Handler mTimerHandler = new Handler();
    private boolean mEnableCameraMute = true;
    
    public static final short MODE_NA = -1;
    public static final short MODE_ONE = 0;
    public static final short MODE_BACK = 1;
    public static final short MODE_FACE = 2;
    public boolean  isScreenPortrait=true;
    public static int xx = 0;
    private int targetW = 1280;
    private int targetH = 720;
    
	private static final String     mRecordPath = "/sdcard/h264/";
	private static FileOutputStream m264 = null;
	private static File             mFile = null;
	
	private VideoLevelID levelid = new VideoLevelID();
	
	private int mKbps  = levelid.bit_rate;
	private int mGop   = levelid.GOP;
	
	private int frameCnt        = 0;
	
	private Bitmap muteBmp = null;
	
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

	public CameraSourcePlusWith1832()
    {
        // Must call this before calling addCallbackBuffer to get all the
        // reflection variables setup
        // initForACB();
        // initForPCWB();
    }
	public CameraSourcePlusWith1832(Context context)
    {
        mContext = context;
    }
	byte[] getNV21(int inputWidth, int inputHeight, Bitmap scaled){
		
		int [] argb = new int[inputWidth*inputHeight];
		scaled.getPixels(argb, 0, inputWidth, 0, 0, inputWidth, inputHeight);
		
		byte [] yuv = new byte[inputWidth*inputHeight*3/2];
		encodeYUV420SP(yuv, argb, inputWidth, inputHeight);
		
		//scaled.recycle();
		
		return yuv;
	}
	
	byte[] getEncodedMute() throws IOException{
		InputStream input = null;
		if(targetH>=720)
			input = mContext.getResources().openRawResource(R.raw.mute_1280x720);
		else
			input = mContext.getResources().openRawResource(R.raw.mute_640x480);
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
    private Runnable muteTimer = new Runnable() 
    {
        public void run()
        {
            // stop timer
            if(!mIsSilence)
            {
                return;
            }

            int size = (int) (mWidth*mHeight*1.5f);
            if(mpI420==null)
            {  
                mpI420 = new byte[size];
                if(mMuteCameraBitmap == null)
                {
                	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"Doesn't find the image of mute camera");
                }
                else
                {
                    //pixel convert process
//                  int layer = 3;
//                  Log.d(DEBUGTAG,"RGB byte array = "+mWidth+"x"+mHeight);
//                  byte [] pRGBData = new byte[mWidth*mHeight*layer];      
//                  setMuteCameraImage(pRGBData,layer);
//                  BGRAtoI420(pRGBData ,mpI420 ,layer);
                	//mpI420 = getNV21(mMuteCameraBitmap.getWidth(), mMuteCameraBitmap.getHeight(),mMuteCameraBitmap);
                	try {
						mpI420 = getEncodedMute();
					} catch (IOException e) {
						// TODO Auto-generated catch block
						Log.e(DEBUGTAG,"",e);
					}

                	
                	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"Set RGB data to I420 format");
                }
            }
            
            try
            {
                //if (mIsStarted)
                if(mSink != null){
                    mSink.onMedia(ByteBuffer.wrap(mpI420),
                    			  mpI420.length,
                                  System.currentTimeMillis(), 
                                  (short) 0);
                }
            } catch (Throwable e)
            {
                Log.e(DEBUGTAG, "handleMessage", e);
            }
        
            //recall timer
            if(frameCnt<10){
            	frameCnt ++;
            	if(mTimerHandler !=null)
            		mTimerHandler.postDelayed(this, 1000/mFrameRate);
            }
            else{
            	if(mTimerHandler !=null)
            		mTimerHandler.postDelayed(this, 2000);
            }
        }
    };
        
    
    public void setMute (boolean enable, Bitmap bitmap)
    {
    	frameCnt = 0;
    	mEnableCameraMute = enable;
        if(mIsSilence == enable)
            return;
        
        mIsSilence = enable;
        if(mIsSilence) //camera->mute image
        {   
            mMuteCameraBitmap = bitmap;//BitmapFactory.de.decodeResource(context.getResources(), R.drawable.mute_icon);
            if(mMuteCameraBitmap != null)
            	Log.d(DEBUGTAG, "mMuteCameraBitmap "+mMuteCameraBitmap.getHeight()+"X"+mMuteCameraBitmap.getWidth());
            //set Timer
            mpI420 = null;
            if(mTimerHandler==null)
            	mTimerHandler = new Handler();
            mTimerHandler.removeCallbacks(muteTimer);
            mTimerHandler.postDelayed(muteTimer, 1000/mFrameRate);
            if(UpdateLocalHandler!=null){
            	Message msg = new Message();
            	Bundle bundle = new Bundle();
            	msg.what = InVideoSessionActivity.UPDATE_LOCAL_RESULOTION;
            	bundle.putInt(InVideoSessionActivity.LOCALVIEW_WIDTH, 1280);
            	bundle.putInt(InVideoSessionActivity.LOCALVIEW_HEIGHT, 720);

            	msg.setData(bundle);
            	UpdateLocalHandler.sendMessage(msg);
            }
            internalStop();
        }
        else
        {
        	stopH264Preview();
            try 
            {
                start();
            } 
            catch (FailedOperateException e) 
            {
                Log.e(DEBUGTAG, "setMute", e);
            }
        }
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

    public SurfaceView getCameraView(Context context)
    {
    	mContext = context;
        mCameraPreview = new CameraPreview(context);
        return mCameraPreview;
    }
    @TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
	public TextureView getH264View(Context context, Handler handler)
    {
    	Log.d(DEBUGTAG,"getH264View");
    	mContext = context;
    	if(mH264Preview == null) 
    		mH264Preview = new H264Preview(context, handler);
    	return mH264Preview;
    }
    
    public void setScale(float x_scale, float y_scale)
    {
        m_x_scale = x_scale;
        m_y_scale = y_scale;
    }

    
    public void setupCamera(int format, int width, int height, int camera_id, int frameRate, int kbps, boolean enableCamera)
            throws FailedOperateException
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>setupCamera:" + mCameraID + " " + width + "x" + height + 
    														" -camera_id:" +camera_id+" frameRate:"+frameRate+" kbps:"+kbps+" enableCamera:"+enableCamera);
    	boolean format_changed = false;
        if (mFormat != format || mWidth != width || mHeight != height || mFrameRate != frameRate || mKbps != kbps || mEnableCameraMute != enableCamera)
        {
            mFormat = format;
            mWidth = width;
            mHeight = height;
            targetW = width;
            targetH = height;
            mFrameRate = frameRate;
            mKbps = kbps;
            mEnableCameraMute = enableCamera;
            format_changed = true;
            if(width>levelid.width||height>levelid.height){
            	mWidth = levelid.width;
            	mWidth = levelid.height;
            	targetW = levelid.width;
                targetH = levelid.height;
            }
            if(frameRate>levelid.frame_rate){
            	mFrameRate = levelid.frame_rate;
            }
            if(kbps>levelid.bit_rate){
            	mKbps = levelid.bit_rate;
            }
            if(mWidth==1280){
            	mKbps = 2000;
            }
        }
        if (mCameraID != camera_id)
        {
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
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==setupCamera:" + mCameraID  + mCameraID + " " + width + "x" + height + " -camera_id:" +camera_id+" frameRate:"+frameRate+
        													" kbps:"+mKbps+" EnableCameraMute:"+mEnableCameraMute);
    }

    private void setResolution(int width, int height)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setResolution:" + width + "x" + height);
        try
        {
            mWidth = width;
            mHeight = height;
            if (mCamera != null && mIsStarted)
            {
                stop();
                start();
            }
/*            if (mLocalPreview != null)
                mLocalPreview.setAspectRatio(mHeight, mWidth);*/
        } catch (FailedOperateException e)
        {
            Log.e(DEBUGTAG, "setResolution", e);
        }
    }

    private void cameraRestart() throws FailedOperateException
    {
        mHandler.post(new Runnable()
        {
            @SuppressLint("NewApi")
			public void run()
            {
            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>cameraRestart:" + mWidth + "x" + mHeight);
                // mCamera.setPreviewCallback(null);

                synchronized (mCameraLock)
                {
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
                        	//Log.d(DEBUGTAG,"CameraSize at restart: "+previewSize.width+"x"+previewSize.height);
                        	if(mRotationDegree == 270 || mRotationDegree == 90){
    	                       	if(previewSize.width>=target_height && previewSize.height>=target_width)
    	                    	{
    	                    		mWidth = previewSize.width;
    	                    		mHeight = previewSize.height;	
    	                    	}else{
    	                    		break;
    	                    	}                	
                        	}else if (mRotationDegree ==0 || mRotationDegree == 180){
                        		if(previewSize.width>=target_width && previewSize.height>=target_height)
    	                    	{
    	                    		mWidth = previewSize.width;
    	                    		mHeight = previewSize.height;	
    	                    	}else{
    	                    		break;
    	                    	}
                        	}          	
                        }
                        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"selected Size at restart : "+mWidth+"x"+mHeight );
                        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"***Assign camera parameters at restart:"+mWidth+"x"+mHeight+"x"+mFrameRate +"x"+mKbps+"x"+mGop+"***");

                        //getSupportedPreviewFramerate(parameters);
                        parameters.set("Requset_264", "true");
                        parameters.setPreviewSize(mWidth, mHeight);
                        parameters.set("Set_Bit_Rate", mKbps);
                        parameters.set("Set_Frame_Rate", mFrameRate);
                        parameters.set("Iframe_interval", mGop);
                        
                        //Log.d(DEBUGTAG,"CameraHardware 7501 framerate:"+parameters.getInt("Set_Frame_Rate"));
                        
                        mCamera.setParameters(parameters);
                    }
                }
                try
                {
                    postStart();
                } catch (Throwable e)
                {
                    Log.e(DEBUGTAG, "postStart", e);
                }
                if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==cameraRestart");
            }
        });
    }
    public static boolean mEnableCameraRotation = false;
    public static void setEnableCameraRotation(boolean enableRoatation)
    {
        mEnableCameraRotation = enableRoatation;
    }
    private void postStart() throws IOException, FailedOperateException,
            UnSupportException
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>postStart");
        Camera.Parameters parameters = null;
        Size size = null;
        try
        {
            synchronized (mCameraLock)
            {
                if (mCamera != null && !mOnPreviewing)
                {
                    mRotationDegree = getRotationDegree(mCameraID, mDisplayRotation);
                    if(mEnableCameraRotation)
                    {
                        //Setup the roatation degree
                        if (MODE_FACE == mCameraID && Build.VERSION.SDK_INT >= 9) //for 2.3+
                        {   // compensate the mirror
                        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "set display orientation:"+(360 - mRotationDegree) % 360);
                            mCamera.setDisplayOrientation((360 - mRotationDegree) % 360);
                        }else
                        {
                            mCamera.setDisplayOrientation(mRotationDegree);
                        }
                    }
                    else
                        mCamera.setDisplayOrientation(0);
                    mCamera.setPreviewCallback(this);
                	//start264Debug();
                    mCamera.startPreview();
                    if(UpdateLocalHandler!=null){
                    	Message msg = new Message();
                    	Bundle bundle = new Bundle();
                    	msg.what = InVideoSessionActivity.UPDATE_LOCAL_RESULOTION;
                    	bundle.putInt(InVideoSessionActivity.LOCALVIEW_WIDTH, mWidth);
                    	bundle.putInt(InVideoSessionActivity.LOCALVIEW_HEIGHT, mHeight);

                    	msg.setData(bundle);
                    	UpdateLocalHandler.sendMessage(msg);
                    }
                    parameters = mCamera.getParameters();
                    size = parameters.getPreviewSize();
                    mIsStarted = true;
                    mOnPreviewing = true;
                }
            }
        } 
        catch (Throwable err)
        {
            Log.e(DEBUGTAG, "postStart", err);
            mHandler.post(new Runnable()
            {
                public void run()
                {
                    internalStop();
                    try
                    {
                        start();
                    } catch (FailedOperateException e)
                    {
                        Log.e(DEBUGTAG, "postStart", e);
                    }
                }
            });
            throw new FailedOperateException(err);
        }
        //Log.d(DEBUGTAG, "size.width:"+size.width+",mWidth:"+mWidth+", size.height:"+size.height+", mHeight:"+mHeight);
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
                            //Log.d(DEBUGTAG, "tmp size:"+tmp_size.width+"x"+tmp_size.height+"x"+tmp_dist);
                            //Log.d(DEBUGTAG, "result size:"+result_size.width+"x"+result_size.height+"x"+result_dist);
                            if (tmp_size.width < result_size.width && 
                                tmp_size.height < result_size.height &&
                                tmp_dist < result_dist)
                            {
                                //Log.d(DEBUGTAG, "OK");
                                result_size.width = tmp_size.width;
                                result_size.height = tmp_size.height;
                                result_dist = tmp_dist;                                
                            }
                        }
                        //Log.d(DEBUGTAG, "result_size.width"+result_size.width+"result_size.height:"+result_size.height);
                        setResolution(result_size.width, result_size.height);
                    }
                }
            });
        } else
        {
            if (parameters != null && size != null){
            	//mSink.setFormat(parameters.getPreviewFormat(),targetW,targetH);}
            	mSink.setFormat(parameters.getPreviewFormat(),mWidth,mHeight);}
            onDegreeChanged();
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==postStart");
    }

	@SuppressLint("NewApi")
	@Override
    public void start() throws FailedOperateException
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>start");
    	if(Hack.isNEC()){
    		targetW = 320;
    		targetH = 240;
    	}
        boolean result = false;
        if (mCamera != null)
        {
            internalStop();
        }
        mIsSilence = false;
        
        if(mEnableCameraMute){
        	mIsSilence = true;
        	if(muteBmp==null)
        		muteBmp = Hack.getBitmap(mContext, R.drawable.bg_vc_audio_vga);
            if(mMuteCameraBitmap==null)
            	mMuteCameraBitmap = getMuteImage();;//BitmapFactory.de.decodeResource(context.getResources(), R.drawable.mute_icon);
            mpI420 = null;
            mTimerHandler.removeCallbacks(muteTimer);
            mTimerHandler.postDelayed(muteTimer, 1000/mFrameRate);
            internalStop();
        	return;
        }
        
        synchronized (mCameraLock)
        {
        	mCamera = openFrontFacingCamera2();
        	Camera.Parameters parameters = mCamera.getParameters();
        	
        	//mCamera.setParameters(parameters);
        	//parameters = mCamera.getParameters();
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
            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"CameraSize at start: "+previewSize.width+"x"+previewSize.height );
            }
            mRotationDegree = getRotationDegree(mCameraID, mDisplayRotation);
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"mRotationDegree at start :"+mRotationDegree);
            for(int i = 0 ;i < previewSizes.size();i++) {
            	Camera.Size previewSize = previewSizes.get(i);
            	
            	if(mRotationDegree == 270 || mRotationDegree == 90) {
                   	if(previewSize.width>=target_height && previewSize.height>=target_width) {
                		mWidth = previewSize.width;
                		mHeight = previewSize.height;
                	}
                   	else {
                		break;
                	}                	
            	}
            	else if(mRotationDegree==0 || mRotationDegree==180) {
            		if(previewSize.width>=target_width && previewSize.height>=target_height) {
                		mWidth = previewSize.width;
                		mHeight = previewSize.height;	
                	}
            		else {
                		break;
                	}
            	}
            }
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"selected Size at start() : "+mWidth+"x"+mHeight );
        	
        	
            //getSupportedPreviewFramerate(parameters);
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"***Assign camera parameters:"+mWidth+"x"+mHeight+"x"+mFrameRate +"x"+mKbps+"x"+mGop+"***");
            parameters.set("Requset_264", "true");
            parameters.setPreviewSize(mWidth, mHeight);
            parameters.setPreviewFormat(mFormat);
            
            parameters.set("Set_Bit_Rate", mKbps);
            parameters.set("Set_Frame_Rate", mFrameRate);
            parameters.set("Iframe_interval", mGop);
/*            Log.d(DEBUGTAG,"getExposureCompensation: "+parameters.getExposureCompensation()
            		+",getMaxExposureCompensation:"+parameters.getMaxExposureCompensation()
            		+",getMinExposureCompensation:"+parameters.getMinExposureCompensation()
            		+",getExposureCompensationStep:"+parameters.getExposureCompensationStep());
            parameters.setExposureCompensation(parameters.getMinExposureCompensation());
            parameters.setAutoExposureLock(true);*/
            mCamera.setParameters(parameters);
            

            // Add buffers to the buffer queue. I re-queue them once
            // they are
            // used in onPreviewFrame, so we should not need many of
            // them.
            PixelFormat pixelinfo = new PixelFormat();
            int pixelformat = mCamera.getParameters().getPreviewFormat();
            PixelFormat.getPixelFormatInfo(pixelformat, pixelinfo);
            mCamera.setPreviewCallback(this);

            mIsStarted = true;
            result = true;
        }
        try
        {
        	postStart();
        } 
        catch (Throwable e)
        {
            Log.e(DEBUGTAG, "start", e);
            throw new FailedOperateException(e);
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==start:" + result);
        if (!result)
            throw new FailedOperateException("Failed on start CameraSource:"
                    + propertiesToString());
    }

    @Override
    public void stop()
    {
        internalStop();
        mIsSilence = false;

    }
    public void internalStop()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>camera stop");
        synchronized (mCameraLock)
        {
            if (mCamera != null)
            {
	            mCamera.stopPreview();
	            mCamera.setPreviewCallback(null);
	            mCamera.release();
	            mCamera = null;
	            if(mCameraPreview!=null){
	            	if(mCameraPreview.mHolder!=null){
	            		mCameraPreview.mHolder.removeCallback(mCameraPreview);
	            		mCameraPreview.mHolder=null;
	                }
	                synchronized (mSurfaceLock) {
                	
	                	mCameraPreview.mH264Display.Stop();
	                	mCameraPreview = null;
                	}
                }
                //mCamera.setPreviewCallback(null);
                //mCamera.stopPreview();
                //mCamera.release();
                //mCamera = null;
	            mOnPreviewing = false;
	            stopH264Preview();
            }else{
            	if(mEnableCameraMute){
            		stopH264Preview();
            	}
            }
        }
        mIsStarted = false;
       
        //stop264Debug();
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==camera stop");
    }
    private void stopH264Preview(){
    	if(mH264Preview!=null){
        	mH264Preview.mH264Display.Stop();
        	mH264Preview.mH264Display = null;
        	mH264Preview = null;
        }
    }

    @Override
    public void setSink(IVideoSink sink) throws FailedOperateException,
            UnSupportException
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setSink:" + sink);
        try
        {
            if (mCamera != null)
            {
                Camera.Parameters parameters = mCamera.getParameters();
                sink.setFormat(parameters.getPreviewFormat(), mWidth,
                		mHeight);
            } else
            {
            	sink.setFormat(mFormat, mWidth, mHeight);
            }
            mSink = sink;
        } catch (Throwable e)
        {
            Log.e(DEBUGTAG, "setSink", e);
            internalStop();
            throw new FailedOperateException("Failed on CameraSource.SetSink:"
                    + propertiesToString());
        }
    }
    @Override
    public void onPreviewFrame(final byte[] data, Camera camera)
    {
        try
        {
        	synchronized (mCameraLock)
            {
	        	if(mIsStarted){
	        		//Log.d(DEBUGTAG,"SEND now mili sec CA:"+System.currentTimeMillis());
	            	int data_length = 0;
	            	
	            	data_length += data[3]*256*256*256;
	            	data_length += data[2]*256*256;
	            	if(data[1]>=0){
	            		data_length += data[1]*256;
	            	}else{
	            		data_length += (256+data[1])*256;
	            	}
	            	if(data[0]>=0){
	            		data_length += data[0];
	            	}else{
	            		data_length += (256+data[0]);
	            	}
	            	/*Log.d(DEBUGTAG,"data[3]="+data[3]);
	            	Log.d(DEBUGTAG,"data[2]="+data[2]);
	            	Log.d(DEBUGTAG,"data[1]="+data[1]);
	            	Log.d(DEBUGTAG,"data[0]="+data[0]);
	            	Log.d(DEBUGTAG,"1832_test data_length="+data_length);*/
	            	byte[] data1 = Arrays.copyOfRange(data, 4, data_length+4);
	            	
	            	//write264(data1);
	            	
            		mSink.onMedia(ByteBuffer.wrap(data1), data_length, System.currentTimeMillis(), (short) mRotationDegree);// 0:rotation
	        		if(Hack.isK72()||Hack.isQF3()) {
	        			if(mH264Preview!=null && mH264Preview.mH264Display!=null)
	        				mH264Preview.mH264Display.Write(data1, data_length);
	        			//mCameraPreview.mH264Display.Write(data1, data_length);
	        		}
	        		else {
	        			if(mCameraPreview!=null && mH264Preview.mH264Display!=null)
	        				mCameraPreview.mH264Display.Write(data1, data_length);
	        		}
	        	}
            }
        }
        catch (Throwable e)
        {
            Log.e(DEBUGTAG, "onPreviewFrame", e);
        }
    }

    class CameraPreview extends VideoPreview
    				    implements SurfaceHolder.Callback
    {
    	private VideoPreview   mVideoRender = null;
    	private SurfaceHolder mHolder = null;
    	private H264Display mH264Display = null;
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
    		mH264Display = null;
    		mHolder = null;
    	}

        public void Pause() {
    		Log.d(DEBUGTAG, "Pause");
    	}

        public void Resume() {
    		Log.d(DEBUGTAG, "Resume");
    	}

        public CameraPreview(Context context, AttributeSet attrs) {
            super(context, attrs);
            Log.d(DEBUGTAG, "VideoRender");
            mH264Display = new H264Display();
            mHolder = this.getHolder();
            mHolder.addCallback(this);
            this.getResources();
        	mVideoRender = this;
        }
        public CameraPreview(Context context) {
            super(context);
            Log.d(DEBUGTAG, "VideoRender");
            mH264Display = new H264Display();
            mHolder = this.getHolder();
            mHolder.addCallback(this);
            this.getResources();
        	mVideoRender = this;
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
    		Log.d(DEBUGTAG, "surfaceChanged, " + width + " x "
    	          + height + ", color:" + format);
    		mHolder = holder;
    		if(mH264Display!=null)
    			mH264Display.Config(1280, 720, mVideoRender, true, null);
    	}

    	@Override
    	public void surfaceCreated(SurfaceHolder holder) {
    		Log.d(DEBUGTAG, "surfaceCreated");
    		mHolder = holder;
    	}

    	@Override
    	public void surfaceDestroyed(SurfaceHolder holder) {
    		Log.d(DEBUGTAG, "==>surfaceDestroyed");
            mHolder = null;
    		Log.d(DEBUGTAG, "<==surfaceDestroyed");
    	}
        
    	/***************************************************************************
    	 *  				Private function implement
    	 **************************************************************************/
        private  void _Config(int width, int height) {
        	
    	}
    }
    
	@TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
	@SuppressLint("NewApi")
	class H264Preview extends TextureView implements TextureView.SurfaceTextureListener{
		private H264Display mH264Display = null;
		Surface display;
		VideoLevelID levelid = new VideoLevelID();
		public H264Preview(Context context, Handler handler) {
			super(context);
			Log.d(DEBUGTAG,"H264Preview");
			mH264Display = new H264Display();
			setSurfaceTextureListener(this);
			UpdateLocalHandler = handler;
			this.setScaleX(-1);
		}
		@Override
		public void onSurfaceTextureAvailable(SurfaceTexture surface,
				int width, int height) {
			// TODO Auto-generated method stub
			Log.d(DEBUGTAG,"onSurfaceTextureAvailable, width: "+width+" height: "+height);
			display = new Surface(surface);
			if(mH264Display!=null)
				mH264Display.Config(levelid.width, levelid.height, display, true);
		}

		@Override
		public void onSurfaceTextureSizeChanged(SurfaceTexture surface,
				int width, int height) {
			// TODO Auto-generated method stub
			
		}

		@Override
		public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
			// TODO Auto-generated method stub
			Log.d(DEBUGTAG,"onSurfaceTextureDestroyed");
			return false;
		}

		@Override
		public void onSurfaceTextureUpdated(SurfaceTexture surface) {
			// TODO Auto-generated method stub
		}


	}

    private static final int getRotationDegree(int cameraID, int rotation)
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "cameraID:"+cameraID);
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "rotation:"+rotation);
        int result = getRotationDegree2_3(cameraID,rotation);
        if (result>=0)
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
     * @return a Camera object
     */
    private static final int getRotationDegree2_3(int cameraID,int rotation)
    {
        // Look for front-facing camera, using the Gingerbread API.
        // Java reflection is used for backwards compatibility with pre-Gingerbread APIs.
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
                getCameraInfoMethod.invoke( null, cameraID==MODE_FACE?1:0, cameraInfo );//CameraInfo.CAMERA_FACING_BACK:0,CameraInfo.CAMERA_FACING_FRONT,1
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
            // Log.d(DEBUGTAG, "CAMERA_FACING_FRONT: result:"+result+
            // " rotation:"+rotation);
        } else // Camera.CameraInfo.CAMERA_FACING_BACK
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
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>setDisplayRotation:");
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "mDisplayRotation : "+mDisplayRotation+" rotation : "+rotation);
        if (mDisplayRotation != rotation)
        {
            mDisplayRotation = rotation;
            try
            {
                if (mCamera != null && mIsStarted)
                {
                    cameraRestart();
                }
            } catch (Throwable e)
            {
                Log.e(DEBUGTAG, "setDisplayRotation", e);
            }
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==setCameraDisplayOrientation:");
    }

    private void onDegreeChanged()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d("Iron","mRotationDegree"+mRotationDegree);
        mHandler.post(new Runnable()
        {
            public void run()
            {
                if (mRotationDegree == 90 || mRotationDegree == 270)
                {
                    /*if (mLocalPreview != null)
                        mLocalPreview.setAspectRatio(mHeight, mWidth);
                    if (mLocalLayout != null)
                        mLocalLayout.setAspectRatio(mHeight, mWidth);*/
                } else
                {
                    /*if (mLocalPreview != null)
                        mLocalPreview.setAspectRatio(mWidth, mHeight);
                    if (mLocalLayout != null)
                        mLocalLayout.setAspectRatio(mWidth, mHeight);*/
                }
            }
        });
    }
    private static Camera openFrontFacingCamera() throws IllegalArgumentException, IllegalAccessException, InvocationTargetException
    {
        Camera camera = null;

        //0. trying to use android 2.3 library
        if((camera = openFrontFacingCamera2_3()) != null){
            return camera;
        }
        //1. From mapper
        if((camera = FrontFacingCameraMapper.getPreferredCamera()) != null){
            return camera;
        }

        //2. Use switcher
        if(FrontFacingCameraSwitcher.getSwitcher() != null){
            camera = Camera.open();
            FrontFacingCameraSwitcher.getSwitcher().invoke(camera, (int)1);
            return camera;
        }
        return camera;
    }
    /**
     * Open the camera.  First attempt to find and open the front-facing camera.
     * If that attempt fails, then fall back to whatever camera is available.
     *
     * @return a Camera object
     */
    private static Camera openFrontFacingCamera2_3()
    {
        Camera camera = null;
     
        // Look for front-facing camera, using the Gingerbread API.
        // Java reflection is used for backwards compatibility with pre-Gingerbread APIs.
        try {
            Class<?> cameraClass = Class.forName("android.hardware.Camera");
            Object cameraInfo = null;
            Field field = null;
            int cameraCount = 0;
            Method getNumberOfCamerasMethod = cameraClass.getMethod( "getNumberOfCameras" );
            if ( getNumberOfCamerasMethod != null ) {
                cameraCount = (Integer) getNumberOfCamerasMethod.invoke( null, (Object[]) null );
            }
            Class<?> cameraInfoClass = Class.forName("android.hardware.Camera$CameraInfo");
            if ( cameraInfoClass != null ) {
                cameraInfo = cameraInfoClass.newInstance();
            }
            if ( cameraInfo != null ) {
                field = cameraInfo.getClass().getField( "facing" );
            }
            Method getCameraInfoMethod = cameraClass.getMethod( "getCameraInfo", Integer.TYPE, cameraInfoClass );
            if ( getCameraInfoMethod != null && cameraInfoClass != null && field != null ) {
                for ( int camIdx = 0; camIdx < cameraCount; camIdx++ ) {
                    getCameraInfoMethod.invoke( null, camIdx, cameraInfo );
                    int facing = field.getInt( cameraInfo );
                    if ( facing == 1 ) { // Camera.CameraInfo.CAMERA_FACING_FRONT
                        try {
                            Method cameraOpenMethod = cameraClass.getMethod( "open", Integer.TYPE );
                            if ( cameraOpenMethod != null ) {
                                camera = (Camera) cameraOpenMethod.invoke( null, camIdx );
                            }
                        } catch (RuntimeException e) {
                            Log.e(DEBUGTAG, "Camera failed to open: " + e.getLocalizedMessage());
                        }
                    }
                }
            }
        }
        // Ignore the bevy of checked exceptions the Java Reflection API throws - if it fails, who cares.
        catch ( ClassNotFoundException e        ) {Log.e(DEBUGTAG, "ClassNotFoundException" + e.getLocalizedMessage());}
        catch ( NoSuchMethodException e         ) {Log.e(DEBUGTAG, "NoSuchMethodException" + e.getLocalizedMessage());}
        catch ( NoSuchFieldException e          ) {Log.e(DEBUGTAG, "NoSuchFieldException" + e.getLocalizedMessage());}
        catch ( IllegalAccessException e        ) {Log.e(DEBUGTAG, "IllegalAccessException" + e.getLocalizedMessage());}
        catch ( InvocationTargetException e     ) {Log.e(DEBUGTAG, "InvocationTargetException" + e.getLocalizedMessage());}
        catch ( InstantiationException e        ) {Log.e(DEBUGTAG, "InstantiationException" + e.getLocalizedMessage());}
        catch ( SecurityException e             ) {Log.e(DEBUGTAG, "SecurityException" + e.getLocalizedMessage());}
     
        return camera;
    }
    /***
     * FrontFacingCameraSwitcher
     * @author Mamadou Diop
     *
     */
    static class FrontFacingCameraSwitcher
    {
            private static Method DualCameraSwitchMethod;
    
            static{
                    try{
                            FrontFacingCameraSwitcher.DualCameraSwitchMethod = Class.forName("android.hardware.Camera").getMethod("DualCameraSwitch",int.class);
                    }
                    catch(Exception e){
                            Log.d(DEBUGTAG, e.toString());
                    }
            }
    
            static Method getSwitcher(){
                    return FrontFacingCameraSwitcher.DualCameraSwitchMethod;
            }
    }
    
    /**
     * FrontFacingCameraMapper
     * @author Mamadou Diop
     *
     */
    static class FrontFacingCameraMapper
    {
        private static int preferredIndex = -1;

        static FrontFacingCameraMapper Map[] = {
                new FrontFacingCameraMapper("android.hardware.HtcFrontFacingCamera", "getCamera"),
                // Sprint: HTC EVO 4G and Samsung Epic 4G
                // DO not forget to change the manifest if you are using OS 1.6 and later
                new FrontFacingCameraMapper("com.sprint.hardware.twinCamDevice.FrontFacingCamera", "getFrontFacingCamera"),
                // Huawei U8230
                new FrontFacingCameraMapper("android.hardware.CameraSlave", "open"),
                // Motorola Atrix
                // <uses-library  android:name="com.motorola.hardware.frontcamera"/> in AndroidManifest.xml, add the following line above the line "</application>".
                new FrontFacingCameraMapper("com.motorola.hardware.frontcamera.FrontCamera", "getFrontCamera")  //<uses-library  android:name="com.motorola.hardware.frontcamera"/>
                // Default: Used for test reflection
                //new FrontFacingCameraMapper("android.hardware.Camera","open")
        };

        static{
                int index = 0;
                for(FrontFacingCameraMapper ffc: FrontFacingCameraMapper.Map)
                {
                    try
                    {
                        Class.forName(ffc.className).getDeclaredMethod(ffc.methodName);
                        FrontFacingCameraMapper.preferredIndex = index;
                        break;
                    }
                    catch(Exception e)
                    {
                            Log.e(DEBUGTAG, e.toString());
                    }
                    ++index;
                }
        }

        private final String className;
        private final String methodName;

        FrontFacingCameraMapper(String className, String methodName)
        {
                this.className = className;
                this.methodName = methodName;
        }

        static Camera getPreferredCamera()
        {
            if(FrontFacingCameraMapper.preferredIndex == -1)
            {
                    return null;
            }

            try{
                    Method method = Class.forName(FrontFacingCameraMapper.Map[FrontFacingCameraMapper.preferredIndex].className)
                    .getDeclaredMethod(FrontFacingCameraMapper.Map[FrontFacingCameraMapper.preferredIndex].methodName);
                    return (Camera)method.invoke(null);
            }
            catch(Exception e){
                    Log.e(DEBUGTAG, e.toString());
            }
            return null;
        }
    }

    private Camera openFrontFacingCamera2() {
    	int cameraCount = 0;
    	Camera cam = null;
    	Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
    	
    	cameraCount = Camera.getNumberOfCameras();
    	for (int camIdx = 0; camIdx<cameraCount; camIdx++) {
    	    Camera.getCameraInfo(camIdx, cameraInfo);
    	    if (cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
    	        try {
    	            cam = Camera.open(camIdx);
//    	            if(Hack.isQF3()){
//    	            	cam = Camera.open(1);
//    	            }
    	            Log.d(DEBUGTAG, "Choose camera index : "+camIdx);
    	            break;
    	        } catch (RuntimeException e) {
    	            Log.e(DEBUGTAG, "Facing Camera failed to open: " + e.getLocalizedMessage());
    	        }
    	    }
    	}
    	if(cam == null){
    		Log.d(DEBUGTAG, "This device may has not facing camera, open others");
    		cam = Camera.open();
    	}
    	return cam;
    }
    Bitmap getMuteImage(){
    	int width = muteBmp.getWidth();
    	int height = muteBmp.getHeight();
    	int newWidth = targetW;
    	int newHeight = targetH;
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
    	Log.d(DEBUGTAG,"==> destory");

        mSink = null;

        mpI420 = null;
        if (mMuteCameraBitmap != null) {
            mMuteCameraBitmap.recycle();
            mMuteCameraBitmap = null;
        } 
        if (muteBmp != null){
        	muteBmp.recycle();
        	muteBmp = null;
        }
        if(mTimerHandler!=null){
        	mTimerHandler.removeCallbacks(muteTimer);
        	mTimerHandler = null;
        }
        Log.d(DEBUGTAG,"<== destory");
    }
    public static void start264Debug() {
		String file_name = null;
		
		if (mFile == null) {
			mFile = new File(mRecordPath);
			mFile.mkdirs();
		}

		if (m264 == null) {
			file_name = mRecordPath+"X264Test";
			try {
				m264 = new FileOutputStream(file_name);
			} catch (FileNotFoundException e) {
				Log.e(DEBUGTAG,"",e);
			}
		}
	}
	public static void write264(byte[] data) {
		if (m264 != null) {
			try {
				m264.write(data);
			} catch (IOException e) {
				Log.e(DEBUGTAG,"",e);
			}
		}
	}
	public static void stop264Debug() {
		if (m264 != null) {
			try {
				m264.close();
			} catch (IOException e) {
				Log.e(DEBUGTAG,"",e);
			}
			m264 = null;
		}
		
		
	}

	@Override
	public void setGop(int gop) {
		Log.d(DEBUGTAG,"setGop="+gop);
		if(mCamera!=null){
/*			Camera.Parameters para = mCamera.getParameters();
			//para.set("Iframe_interval", gop);
			mCamera.setParameters(para);*/
		}
	}

	@Override
	public void setBitRate(int kbsBitRate) {
		Log.d(DEBUGTAG,"setBitRate="+kbsBitRate);
		
		if(mCamera!=null){
			Camera.Parameters para = mCamera.getParameters();
			para.set("Set_Bit_Rate", kbsBitRate);
			mCamera.setParameters(para);
		}
	}

	@Override
	public void setFrameRate(byte fpsFramerate) {
		Log.d(DEBUGTAG,"setFrameRate="+fpsFramerate);
		if(mCamera!=null){
/*			Camera.Parameters para = mCamera.getParameters();
			para.set("Set_Frame_Rate", fpsFramerate);
			mCamera.setParameters(para);*/
		}
	}

	@Override
	public void requestIDR() {
		Log.d(DEBUGTAG,"requestIDR");
		if(mCamera!=null){
			Camera.Parameters para = mCamera.getParameters();
			para.set("IDR_Request", "true");
			mCamera.setParameters(para);
		}
	}
}
