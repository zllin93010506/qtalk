package com.quanta.qtalk.media.video;

import java.nio.ByteBuffer;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.QtalkMain;
import com.quanta.qtalk.R;
import com.quanta.qtalk.UnSupportException;
import com.quanta.qtalk.ui.ContactsActivity;
import com.quanta.qtalk.ui.InVideoSessionActivity;
import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

class VideoPreview extends SurfaceView
{
    private float mAspectRatio;
    private int mHorizontalTileSize = 1;
    private int mVerticalTileSize = 1;
    int mTargetWidth = 0;
    int mTargetHeight = 0;
    private final Handler mHandler = new Handler();
    /**
     * Setting the aspect ratio to this value means to not enforce an aspect
     * ratio.
     */
    public static float DONT_CARE = 0.0f;

    public VideoPreview(Context context)
    {
        super(context);
        mAspectRatio = DONT_CARE;
    }

    public VideoPreview(Context context, AttributeSet attrs)
    {
        super(context, attrs);
        mAspectRatio = DONT_CARE;
    }

    public VideoPreview(Context context, AttributeSet attrs, int defStyle)
    {
        super(context, attrs, defStyle);
        mAspectRatio = DONT_CARE;
    }

    public void setTileSize(int horizontalTileSize, int verticalTileSize)
    {
        if ((mHorizontalTileSize != horizontalTileSize)
                || (mVerticalTileSize != verticalTileSize))
        {
            mHorizontalTileSize = horizontalTileSize;
            mVerticalTileSize = verticalTileSize;
            mHandler.post(new Runnable()
            {
                public void run()
                {
                    requestLayout();
                    invalidate();
                }
            });
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
            mHandler.post(new Runnable()
            {
                public void run()
                {
                    requestLayout();
                    invalidate();
                }
            });
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
                
                width = roundUpToTile(width, mHorizontalTileSize,
                        widthSpecSize);
                height= roundUpToTile(height, mVerticalTileSize,
                        heightSpecSize);
                //Do scale-up iif (image_ratio>1 && screen_ratio>1) || (image_ratio<1 && screen_ratio<1)
                if ((defaultRatio>1 && mAspectRatio>1) || (defaultRatio<1 && mAspectRatio<1))
                {
                    float enlarge_ratio = defaultRatio/mAspectRatio;
                    if (defaultRatio<mAspectRatio)
                        enlarge_ratio = mAspectRatio/defaultRatio;
                    width = (int) (width*enlarge_ratio);
                	height = (int) (height*enlarge_ratio);
                }
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


public class VideoRenderSurfacePlus extends VideoPreview 
                                    implements IVideoSink, IVideoSinkPollbased, IVideoStreamReportCB
{
    private final static     String TAG = "VideoRenderPlus";
    private static final String DEBUGTAG = "QTFa_VideoRenderPlus";
    private Bitmap.Config mConfig = Bitmap.Config.ALPHA_8;
    private int            mWidth = 0;
    private int            mHeight = 0;
    private final Matrix   mMatrix = new Matrix();
    private final Matrix   mLBWMatrix = new Matrix();
    private int            mRotationDegree = 0;    
    private short          mDataRotation = 0;
    private int            mDisplayRotation = 0;
    private IVideoStreamReportCB mVideoStreamReportListener = null;
    private                BitmapFactory.Options mBitmapOptions = new BitmapFactory.Options();
    private int            screensize  = -1;
    
    private static final Paint    mBackGround =  new Paint(Color.BLACK);

    private SurfaceHolder mHolder = null;
    private Context mcontext = null;
    private static Handler mHandler;
    public void setHandler(Handler handler)
    {
    	mHandler = handler;
    }
    public void setDisplayRotation(int rotation)
    {
        if (mDisplayRotation!=rotation)
        {
            mDisplayRotation = rotation;
            onDegreeChange();
        }
    }

    private synchronized void onDegreeChange()
    {
        int width = mTargetWidth;
        int height =mTargetHeight;
        mRotationDegree = (mDataRotation)%360;
        if (mDisplayRotation==90 || mDisplayRotation==270)
        {
            int tmp = width;
            width = height;
            height = tmp;
        }
        if (mRotationDegree==90 || mRotationDegree==270)
            _GetPreferedWidthAndHeight(mHeight,mWidth,width, height);
        else
            _GetPreferedWidthAndHeight(mWidth,mHeight, width, height);
        
    }
    public VideoRenderSurfacePlus(Context context)
    {
        super(context);
        initial(context);
    }
    public VideoRenderSurfacePlus(Context context,Handler handler)
    {
        super(context);
        mHandler = handler;
        initial(context);
    }
    public VideoRenderSurfacePlus(Context context, AttributeSet attrs)
    {
        super(context,attrs);
        initial(context);
    }
    private Surface mSurface;
    public void setVideoReportListener(IVideoStreamReportCB cbListener)
    {
        mVideoStreamReportListener = cbListener;
    }
    private void initial(Context context)
    {
        mcontext = context;
        Configuration config = getResources().getConfiguration();
		screensize = (config.screenLayout&Configuration.SCREENLAYOUT_SIZE_MASK);
		
        getHolder().addCallback(new Callback()
        {
            public void surfaceChanged(SurfaceHolder holder, int format,
                    int width, int height)
            {
                mTargetWidth = width;
                mTargetHeight = height;
                if (mSurface==null)
                {
                    synchronized(VideoRenderSurfacePlus.this)
                    {
                        mSurface=holder.getSurface();
                        mHolder = holder;
                    }
                }
                Log.d("ST","surfaceChanged:"+width+"x"+height);
                onDegreeChange();
                DrawLowBandWidth(mShowLowBandwidth);
            }

			public void surfaceCreated(SurfaceHolder holder) 
            {
                synchronized(VideoRenderSurfacePlus.this)
                {
                    mSurface = holder.getSurface();
                    mHolder = holder;
                }
            }

            public void surfaceDestroyed(SurfaceHolder holder) 
            {
                synchronized(VideoRenderSurfacePlus.this)
                {
                    mSurface=null;
                    mHolder = null;
                } 
            }
        });
    }
    private void DrawLowBandWidth(boolean ShowLowBandwidth) {
    	Log.d(DEBUGTAG,"ShowLowBandwidth :"+ShowLowBandwidth);
    	if(ShowLowBandwidth){
    		if(screensize>4){// do not use for this phase
	    		Canvas canvas = mHolder.lockCanvas(null);
	    		if(mLowBandwidthBmp==null){
	    			
	    			if(getResources().getConfiguration().locale.getDefault().getLanguage().equals("zh")){
	    				mLowBandwidthBmp = Hack.getBitmap(mcontext, R.drawable.img_lowbandwidth_cn);
	    			}else{
	    				mLowBandwidthBmp = Hack.getBitmap(mcontext, R.drawable.img_lowbandwidth_en);
	    			}
	    		}
	            _GetPreferedWidthAndHeightForLBW(mLowBandwidthBmp.getWidth(), mLowBandwidthBmp.getHeight(), getWidth(), getHeight());
	            canvas.drawBitmap(mLowBandwidthBmp, mLBWMatrix, null);
	            mHolder.unlockCanvasAndPost(canvas);                   
	                
	            if (StatisticAnalyzer.ENABLE)
	                StatisticAnalyzer.onRenderSuccess();
    		}
    	}
	}
    @Override
    public synchronized boolean onMedia(final ByteBuffer data,final int length,final long timestamp, final short rotation)
    {
        if (data!=null)
        {
            if (mHolder!=null)
            {
                try
                {
                	
                	if (false == mShowLowBandwidth)
                	{
                		Canvas canvas = mHolder.lockCanvas(null);
                		if(mBitmap == null){
                			mBufferSize = mWidth*mHeight*2;
                			mByteArray = new byte[mBufferSize];
                			mByteBuffer = ByteBuffer.wrap(mByteArray);
                			mBitmap = Bitmap.createBitmap(mWidth,mHeight,mConfig);
                			mEnableRedraw = false;
                        }
                		mBitmap.copyPixelsFromBuffer(data.rewind());  

                		if(canvas!=null)
                		{
                			canvas.drawRect(0, 0, this.getWidth() ,this.getHeight(), mBackGround);
                			canvas.drawBitmap(mBitmap, mMatrix, null);                        
                		}
                		mHolder.unlockCanvasAndPost(canvas);  
                	}
                	else 
                	{
                		DrawLowBandWidth(mShowLowBandwidth);
                	}
                	if (mReportStr!=null)
                    {
                       // canvas.drawText(mReportStr,10, 10, mPaint);
                    }
                    
                	                 
                        
                    if (StatisticAnalyzer.ENABLE)
                        StatisticAnalyzer.onRenderSuccess();
                } catch (Exception e)
                {
                    Log.e(DEBUGTAG,"onMedia",e);
                }
            }else
            {
            	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"onMedia: mSurface==null");
            }
            if (mDataRotation != rotation)
            {
                mDataRotation = rotation;
                onDegreeChange();
                //Log.d("ST","onMedia:"+getWidth()+"x"+getHeight());
            }
        }else
        {
        	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"onMedia: data==null");
        }
        return true;
    }

    @Override
    public synchronized void setFormat(int format, int width, int height) throws UnSupportException 
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setFormat:"+format+" "+width+" "+height);
        Bitmap.Config config = null;
        int pixel_size = 1;
        switch(format)
        {
        case PixelFormat.A_8:
            config = Bitmap.Config.ALPHA_8;
            break;
        case PixelFormat.RGB_565:
            config = Bitmap.Config.RGB_565;
            pixel_size = 2;
            break;
        case PixelFormat.RGBA_8888:
            config = Bitmap.Config.ARGB_8888;
            pixel_size = 4;
            break;
        case PixelFormat.RGBA_4444:
            config = Bitmap.Config.ARGB_4444;
            pixel_size = 2;
            break;
//        case PixelFormat.YCbCr_420_SP:
//        case PixelFormat.YCbCr_422_SP:
//            break;
        default:
            throw new UnSupportException("Failed on VideoRender.setFormat: Not support video format type:"+format);
        }
        if (config!=mConfig || mWidth!=width || mHeight!=height)
        {
            mConfig = config;
            mWidth = width;
            mHeight = height;
            _GetPreferedWidthAndHeight(width,height, getWidth(), getHeight());//new Rect(0, 0, getWidth() - 1, getHeight() - 1);
            
            mBitmapOptions.outHeight = height;
            mBitmapOptions.outWidth = width;
            synchronized(this)
            {
                mBufferSize = width*height*pixel_size;
                mByteArray = new byte[mBufferSize];
                mByteBuffer = ByteBuffer.wrap(mByteArray);
                mBitmap = Bitmap.createBitmap(width,height,config);
                mEnableRedraw = false;
            }
        }
    }
    private static final boolean EXTEND = false;
    private void _GetPreferedWidthAndHeight(int resWidth,int resHeight, int desWidth, int desHeight)
    {
        if (resWidth==0 || resHeight==0)
            return;
        super.setAspectRatio(resWidth,resHeight);
        if (desWidth==0 || desHeight==0)
            return;
//        int resultW = (desHeight*resWidth)/resHeight;//extend to resultW*desHeight
//        int resultH = (desWidth*resHeight)/resWidth;//extend to desWidth*resultH
//        if (resultW <= desWidth)
//        {
//            resultH = desHeight;
//        }
//        else if (resultH <= desHeight)
//        {
//            resultW = desWidth;
//        }
        float scale_x  = (float)desWidth/(float)resWidth;
        float scale_y  = (float)desHeight/(float)resHeight;
        
        int rotate_shift = Math.abs(desWidth -desHeight) /2;
        mMatrix.reset();
        mMatrix.preRotate(mRotationDegree, resWidth/2, resHeight/2);//*(i%4)
        mMatrix.postScale(scale_x, scale_y);
        Log.d("ST", desWidth+"/"+resWidth+"X"+desHeight+"/"+resHeight);
        if(mRotationDegree == 90)
            mMatrix.postTranslate(-rotate_shift, -rotate_shift);
        else if(mRotationDegree == 270)
            mMatrix.postTranslate(rotate_shift, rotate_shift);
    }
    private void _GetPreferedWidthAndHeightForLBW(int resWidth,int resHeight, int desWidth, int desHeight)
    {
        if (resWidth==0 || resHeight==0)
            return;
        super.setAspectRatio(resWidth,resHeight);
        if (desWidth==0 || desHeight==0)
            return;

        float scale_x  = (float)desWidth/(float)resWidth;
        float scale_y  = (float)desHeight/(float)resHeight;
        
        int rotate_shift = Math.abs(desWidth -desHeight) /2;
        mLBWMatrix.reset();
        mLBWMatrix.preRotate(mRotationDegree, resWidth/2, resHeight/2);//*(i%4)
        mLBWMatrix.postScale(scale_x, scale_y);
        if(mRotationDegree == 90)
        	mLBWMatrix.postTranslate(-rotate_shift, -rotate_shift);
        else if(mRotationDegree == 270)
        	mLBWMatrix.postTranslate(rotate_shift, rotate_shift);
    }
    
    private IVideoSourcePollbased mSource = null;
    private byte[]                mByteArray = null;
    private ByteBuffer            mByteBuffer= null;
    private int                   mBufferSize = 0;
    private Bitmap                mBitmap = null;
    private boolean               mEnableRedraw = false;
    private String                mReportStr = null;
    private final Paint           mPaint = new Paint(Color.WHITE);
    
    @Override
    public void setSource(IVideoSourcePollbased source)
            throws FailedOperateException, UnSupportException
    {
        mSource = source;
    }
    
    @Override
    public void onReport(int kbpsRecv, byte fpsRecv, int kbpsSend,
            byte fpsSend, double pkloss, int usRtt)
    {
        mReportStr = "RECV: "+String.format("%4d", kbpsRecv)+"kbps, "+String.format("%2d", fpsRecv)+"fps    SEND: "+String.format("%4d", kbpsSend)+"kbps, "+String.format("%2d", fpsSend)+"fps    pkloss: "+String.format("%3.2f", 100*pkloss)+"%    rtt: "+String.format("%5.3f", usRtt/1000.000)+"ms";
        if (fpsRecv==0)
            postInvalidate();
        if(mVideoStreamReportListener != null)
            mVideoStreamReportListener.onReport(kbpsRecv, fpsRecv, kbpsSend, fpsSend, pkloss, usRtt);
    }
    
    private boolean mShowLowBandwidth = false;
    private Bitmap  mLowBandwidthBmp = null;				
    @Override
    public void setLowBandwidth(boolean showLowBandwidth)
    {
    	Log.d(DEBUGTAG,"StreamEngine showLowBandwidth:"+showLowBandwidth+" "+getResources().getConfiguration().locale.getDefault().getLanguage());
        mShowLowBandwidth  = showLowBandwidth;

        Message msg = mHandler.obtainMessage(1, "showLowBandwidth");
		if(showLowBandwidth){
			msg.what = InVideoSessionActivity.LOWBANDWIDTH_ON;
		}else{
			msg.what = InVideoSessionActivity.LOWBANDWIDTH_OFF;
		}
        mHandler.sendMessage(msg);

        DrawLowBandWidth(showLowBandwidth);
        
    }
    
    @Override
    public synchronized void release()
    {
        mVideoStreamReportListener = null;
        mBitmapOptions = null;

        mSource = null;
        mByteArray = null;
        mByteBuffer= null;
        mReportStr = null;
        
        if (mBitmap != null) {
            mBitmap.recycle();
        }
        mBitmap = null;    

        if (mLowBandwidthBmp != null) {
            mLowBandwidthBmp.recycle();
        }
        mLowBandwidthBmp = null;
    }
}