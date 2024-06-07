package com.quanta.qtalk.media.video;

import java.io.IOException;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Vector;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;

import android.content.Context;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.os.ConditionVariable;
import android.os.Handler;
import android.util.AttributeSet;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

public class CameraSource implements IVideoSource,Camera.PreviewCallback
{
//    private static final String TAG = "CameraSource";
    private static final String DEBUGTAG = "QTFa_CameraSource";
    private final Handler mHandler      = new Handler();
    private LocalPreview  mLocalPreview = null;
    private final Object  mCameraLock   = new Object();
    private Camera        mCamera       = null;
    private int           mFormat       = PixelFormat.YCbCr_420_SP;
    private int           mWidth        = 0;
    private int           mHeight       = 0;
    private int           mCameraID     = 1;
    private int           mFrameRate    = 12;
    private long          mStartTime  = 0;
    private OutputThread  mOutputThread = null;
    private IVideoSink    mSink = null;
    private boolean       mIsStarted = false;
    private int           mFrameSize = 0;
    
    private static boolean mIsDualCamera = false;
    
    public static final short MODE_NA = -1;
    public static final short MODE_ONE = 0;
    public static final short MODE_BACK = 1;
    public static final short MODE_FACE = 2;
    static class FrameData
    {
        public byte[]       frameData       = null;
        public int          frameSize       = 0;
        public long         sampleTime      = 0;
        public ByteBuffer   frameBuffer     = null;
        FrameData(int size)
        {
            frameSize = size;
            frameData = new byte[frameSize];
            frameBuffer = ByteBuffer.wrap(frameData);
        }
    }
    
    //Method to show current properties
    private String propertiesToString()
    {
        StringBuffer sb = new StringBuffer();
        sb.append("mCamera:"+mCamera);
        sb.append("mFormat:"+mFormat);
        sb.append("mWidth:"+mWidth);
        sb.append("mHeight:"+mHeight);
        sb.append("mStartTime:"+mStartTime);
        sb.append("mOutputThread:"+mOutputThread);
        return sb.toString();
    }
    public CameraSource()
    {
        // Must call this before calling addCallbackBuffer to get all the
        // reflection variables setup
        initForACB();
        initForPCWB();
    }
    public Size getResolution()
    {
        Size result = null;
        synchronized(mCameraLock)
        {
            if (mCamera!=null)
            {
                Camera.Parameters parameters = mCamera.getParameters();
                result = parameters.getPreviewSize();
            }
        }
        return result;
    }
    public short getCameraMode()    //0 for backend only, 1 for frontend, 1 for backend
    {
        short result = -1;
        synchronized(mCameraLock)
        {
            if (mCamera!=null)
            {
                result = 0;
                Camera.Parameters parameters = mCamera.getParameters();
                String camera_id = parameters.get("camera-id");
                if (camera_id!=null)
                {
                    result = Short.parseShort(camera_id);
                }
            }
        }
        return result;
    }
    public View getPreview(Context context)
    {
        mLocalPreview = new LocalPreview(context);
        return mLocalPreview;
    }
    public void setupCamera(int format, int width, int height,int camera_id) throws FailedOperateException
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setupCamera:camera_id:"+mCameraID+"=>"+camera_id);
        boolean restart = false;
        if (mFormat!=format || mWidth!=width || mHeight!=height)
        {
            mFormat     = format;
            mWidth      = width;
            mHeight     = height;
            restart = true;
        }
        if (mCameraID!=camera_id)
        {
            mCameraID = camera_id;
            if (mCamera!=null && mIsStarted)
            {
                stop();
                start();
            }
        }else
        {
            if (restart)
            {
                if (mCamera!=null && mIsStarted)
                {
                    internalStop();
                    internalStart();
                }
            }
        }
    }
    private void internalStop()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>internalStop");
        synchronized(mCameraLock)
        {
            if (mCamera != null) 
            {
                clearPreviewCallbackWithBuffer();
                mCamera.stopPreview();
            }
        }
        if (mOutputThread!=null)
        {
            mOutputThread.mExit = true;
            mOutputThread.addData(null);
            mOutputThread = null;
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==internalStop");
    }
    private void interalDestroy()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>interalDestroy");
        synchronized(mCameraLock)
        {
            if (mCamera != null) 
            {
                mCamera.release();
                mCamera = null;
            }
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==interalDestroy");
    }
    private void internalStart() throws FailedOperateException
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>internalStart");
        synchronized(mCameraLock)
        {
            if (mCamera!=null)
            {
                Camera.Parameters parameters = mCamera.getParameters();
                if (parameters.get("camera-id")!=null)
                    parameters.set("camera-id", mCameraID);    //fish
                
                parameters.setPreviewSize(mWidth, mHeight);
                parameters.setPreviewFormat(mFormat);
                parameters.setPreviewFrameRate(mFrameRate);
                mCamera.setParameters(parameters);
                if (mLocalPreview!=null && mLocalPreview.mIsReady && mLocalPreview.mHolder!=null)
                {
                    try
                    {
                        mCamera.setPreviewDisplay(mLocalPreview.mHolder);
                    } catch (IOException e)
                    {
                        Log.e(DEBUGTAG, "start", e);
                        throw new FailedOperateException(e);
                    }
                }
            }
        }
        mHandler.post(new Runnable()
        {
            public void run()
            {
            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>internalStart.run");
                synchronized(mCameraLock)
                {
                    if (mCamera!=null)
                    {
                        mCamera.startPreview();
                        Camera.Parameters parameters = mCamera.getParameters();
                        Size size = parameters.getPreviewSize();
                        if (size.width != mWidth || size.height != mHeight)
                        {
                            List<Size>size_list =  parameters.getSupportedPreviewSizes();
                            Size result_size = parameters.getPreviewSize();
                            for (int i=0;i<size_list.size();i++)
                            {
                                Size tmp_size = size_list.get(i);
                                if (tmp_size.width == mWidth && tmp_size.height == mHeight)                         //Seaching for matched preview size
                                {
                                    result_size.width = tmp_size.width;
                                    result_size.height = tmp_size.height;
                                    break;
                                }else if (tmp_size.width<result_size.width && tmp_size.height<result_size.height)   //Searching for minimun support preview size
                                {
                                    result_size.width = tmp_size.width;
                                    result_size.height = tmp_size.height;
                                }
                            }
                            if (mWidth != result_size.width || mHeight != result_size.height)
                            {
                                mWidth = result_size.width;
                                mHeight = result_size.height;
                                parameters.setPreviewSize(mWidth, mHeight);
                                mCamera.setParameters(parameters);
                            }
                        }
                        try
                        {
                            mSink.setFormat(parameters.getPreviewFormat(), mWidth, mHeight);
                        }catch (Throwable e) 
                        {
                            Log.e(DEBUGTAG, "start", e);
                        }
                        // Add buffers to the buffer queue. I re-queue them once they are
                        // used in onPreviewFrame, so we should not need many of them.
                        PixelFormat pixelinfo = new PixelFormat();
                        int pixelformat = mCamera.getParameters().getPreviewFormat();
                        PixelFormat.getPixelFormatInfo(pixelformat, pixelinfo);
                        mFrameSize = mWidth * mHeight * pixelinfo.bitsPerPixel / 8;
                        mOutputThread = new OutputThread();
                        mOutputThread.start();
                        setPreviewCallbackWithBuffer();
                        if (mLocalPreview!=null)
                            mLocalPreview.setAspectRatio(mWidth,mHeight);
                    }
                }
                if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==internalStart.run");
            }
        });
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==internalStart");
    }
    @Override
    public void start() throws FailedOperateException 
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>start:CameraID="+mCameraID);
        boolean result = false;
        if (mCamera!=null)
        {
            stop();
        }
        if (mCamera==null)
        {
            mCamera = Camera.open();
            if (mCamera != null) 
            {
                try 
                {
                    Camera.Parameters parameters = mCamera.getParameters();
                    String camera_id = parameters.get("camera-id");
                    if (camera_id!=null)
                    {
                        mIsDualCamera = true;
                    }
                    if (mLocalPreview!=null && !mLocalPreview.mIsReady)
                    {
                        mIsStarted = true;
                        result = true;
                    }else
                    {
                        result = true;
                        mIsStarted = true;
                        internalStart();
                    }
                } catch (Throwable e) 
                {
                    Log.e(DEBUGTAG, "start", e);
                    throw new FailedOperateException(e);
                } 
            }
        }
        if (!result)
            throw new FailedOperateException("Failed on start CameraSource:"+propertiesToString());
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==start:CameraID="+mCameraID);
    }
    @Override
    public void stop()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>stop");
        mIsStarted = false;
        internalStop();
        interalDestroy();
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==stop");
    }

    @Override
    public void setSink(IVideoSink sink) throws FailedOperateException, UnSupportException  
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>setSink");
        try
        {
            if (mCamera!=null)
            {
                Camera.Parameters parameters = mCamera.getParameters();
                Size size = parameters.getPreviewSize();
                sink.setFormat(parameters.getPreviewFormat(), size.width, size.height);
            }else
            {
                sink.setFormat(mFormat, mWidth, mHeight);
            }
            mSink = sink;
        }catch (Throwable e) 
        {
            Log.e(DEBUGTAG, "setSink", e);
            stop();
            throw new FailedOperateException("Failed on CameraSource.SetSink:"+propertiesToString());
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==setSink");
    }
    /**
     * This method will list all methods of the android.hardware.Camera class,
     * even the hidden ones. With the information it provides, you can use the
     * same approach I took below to expose methods that were written but hidden
     * in eclair
     */
    private void listAllCameraMethods() 
    {
        try
        {
            Class<?> c = Class.forName("android.hardware.Camera");
            Method[] m = c.getMethods();
            for (int i = 0; i < m.length; i++) {
            	if(ProvisionSetupActivity.debugMode) Log.d("NativePreviewer", "  method:" + m[i].toString());
            }
        } catch (Exception e)
        {
                Log.e("NativePreviewer", e.toString());
        }
    }

    /**
     * These variables are re-used over and over by addCallbackBuffer
     */
    Method mAcb;

    private void initForACB() 
    {
        try
        {
            mAcb = Class.forName("android.hardware.Camera").getMethod("addCallbackBuffer", byte[].class);
        } catch (Exception e) 
        {
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"Problem setting up for addCallbackBuffer: "+ e.toString());
        }
    }

    /**
     * This method allows you to add a byte buffer to the queue of buffers to be
     * used by preview. See:
     * http://android.git.kernel.org/?p=platform/frameworks
     * /base.git;a=blob;f=core/java/android/hardware/Camera.java;hb=9d
     * b3d07b9620b4269ab33f78604a36327e536ce1
     * 
     * @param b
     *            The buffer to register. Size should be width * height *
     *            bitsPerPixel / 8.
     */
    private void addCallbackBuffer(byte[] b)
    {
        try
        {
            mAcb.invoke(mCamera, b);
        } catch (Exception e) {
            Log.e(DEBUGTAG, "invoking addCallbackBuffer failed: " + e.toString());
        }
    }

    Method mPCWB;
    private void initForPCWB() 
    {
        try
        {
            mPCWB = Class.forName("android.hardware.Camera").getMethod("setPreviewCallbackWithBuffer", PreviewCallback.class);
        } catch (Exception e) 
        {
            Log.e(DEBUGTAG,"Problem setting up for setPreviewCallbackWithBuffer: "+ e.toString());
        }
    }

    /**
     * Use this method instead of setPreviewCallback if you want to use manually
     * allocated buffers. Assumes that "this" implements Camera.PreviewCallback
     */
    private void setPreviewCallbackWithBuffer()
    {
        try
        {
            // If we were able to find the setPreviewCallbackWithBuffer method of Camera,
            // we can now invoke it on our Camera instance, setting 'this' to be the
            // callback handler
            if (mCamera!=null)
                mPCWB.invoke(mCamera, this);
            // Log.d("NativePrevier","setPreviewCallbackWithBuffer: Called method");
        } catch (Exception e) {
            Log.e(DEBUGTAG, e.toString());
        }
    }

    protected void clearPreviewCallbackWithBuffer() 
    {
        // mCamera.setPreviewCallback(this);
        // return;
        try 
        {
            // If we were able to find the setPreviewCallbackWithBuffer method of Camera,
            // we can now invoke it on our Camera instance, setting 'this' to be the
            // callback handler
            if (mCamera!=null)
                mPCWB.invoke(mCamera, (PreviewCallback) null);
            // Log.d("NativePrevier","setPreviewCallbackWithBuffer: cleared");
        } catch (Exception e) {
            Log.e("NativePreviewer", e.toString());
        }
    }
    @Override
    public void onPreviewFrame(final byte[] data, Camera camera) 
    {
        try
        {
            if (mOutputThread!=null)
            {
                 mOutputThread.addData(data);
            }
        }catch (Throwable e) 
        {
            Log.e(DEBUGTAG, "onPreviewFrame", e);
        }
    }
    class OutputThread extends Thread
    {
        private Vector<FrameData> mTmpDataVector = new Vector<FrameData>();
        private Vector<FrameData> mFrameDataVector = new Vector<FrameData>();
        private final ConditionVariable mOnNewData = new ConditionVariable();
        boolean mExit = false;
        public void addData(final byte[] data)
        {
            //Log.d(DEBUGTAG,"==>addData");
            if (!mExit && data!=null)
            {
                if (mTmpDataVector.size()>0)
                {
                    final FrameData frame_data = mTmpDataVector.remove(0);
                    frame_data.sampleTime = System.currentTimeMillis();
                    mFrameDataVector.add(frame_data);
                    mOnNewData.open();
                }else
                {
                    //addCallbackBuffer(data);
                }
            }else
                mOnNewData.open();

            //Log.d(DEBUGTAG,"<==addData");
        }

        @Override
        public void run()
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>OutputThread.run");
            //this.setPriority(MAX_PRIORITY);
            for (int i=0;i<1;i++)
            {
                final FrameData tmp_data = new FrameData(mFrameSize);
                mTmpDataVector.add(tmp_data);
                addCallbackBuffer(tmp_data.frameData);
            }
            while (!mExit)
            {
                if (mFrameDataVector.size()==0)
                {
                    mOnNewData.block();
                    mOnNewData.close();
                }else
                {
                    if (!mExit && mFrameDataVector.size()>0)
                    {
                        final FrameData frame_data= mFrameDataVector.remove(0);
                        try
                        {
                            mSink.onMedia(frame_data.frameBuffer,frame_data.frameSize, frame_data.sampleTime, (short)99);//0:rotation
                        } catch (Throwable e) 
                        {
                            Log.e(DEBUGTAG, "handleMessage", e);
                        }finally
                        {
                            mTmpDataVector.add(frame_data);
                            addCallbackBuffer(frame_data.frameData);
                        }
                    }
                }
            }
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==OutputThread.run");
        }
    }
    static class VideoPreview extends SurfaceView 
    {
        private float mAspectRatio;
        private int mHorizontalTileSize = 1;
        private int mVerticalTileSize = 1;

        /**
         * Setting the aspect ratio to this value means to not enforce an aspect ratio.
         */
        static float DONT_CARE = 0.0f;

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
                postInvalidate();
                //requestLayout();
                //invalidate();
            }
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
        {
            if (mAspectRatio != DONT_CARE) 
            {
                int widthSpecSize =  MeasureSpec.getSize(widthMeasureSpec);
                int heightSpecSize =  MeasureSpec.getSize(heightMeasureSpec);

                int width = widthSpecSize;
                int height = heightSpecSize;

                if (width > 0 && height > 0)
                {
                    float defaultRatio = ((float) width) / ((float) height);
                    if (defaultRatio < mAspectRatio) 
                    {   // Need to reduce height
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

        private int roundUpToTile(int dimension, int tileSize, int maxDimension) {
            return Math.min(((dimension + tileSize - 1) / tileSize) * tileSize, maxDimension);
        }
    }
    class LocalPreview extends VideoPreview implements SurfaceHolder.Callback 
    {
        SurfaceHolder mHolder;
        boolean       mIsReady;
        LocalPreview(Context context)
        {
            super(context);
            // Install a SurfaceHolder.Callback so we get notified when the
            // underlying surface is created and destroyed.
            mHolder = getHolder();
            mHolder.addCallback(this);
            mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
            mIsReady = false;
        }
        
        public void surfaceCreated(SurfaceHolder holder) 
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>surfaceCreated");
            mHolder = holder;
            mIsReady = true;
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==surfaceCreated");
        }
        public void surfaceDestroyed(SurfaceHolder holder) 
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>surfaceDestroyed");
            // Surface will be destroyed when we return, so stop the preview.
            // Because the CameraDevice object is not a shared resource, it's very
            // important to release it when the activity is paused.
            mHolder = null;
            mIsReady = false;
            if (mIsStarted)
            {
                try
                {
                    stop();
                    mIsStarted = true;
                }catch (Throwable e) 
                {
                    Log.e(DEBUGTAG,"surfaceCreated", e);
                }
            }
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==surfaceDestroyed");
        }
        
        public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) 
        {
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "==>surfaceChanged");
            // Now that the size is known, set up the camera parameters and begin
            // the preview.
            if (mIsStarted)
            {
                try
                {
                    if (mCamera!=null)
                    {
                        internalStop();
                        internalStart();
                    }else
                        start();
                }catch (Throwable e) 
                {
                    Log.e(DEBUGTAG,"surfaceCreated", e);
                }
            }
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "<==surfaceChanged");
        }
    }
    
    @Override
    public void destory()
    {
        ;
    }
}