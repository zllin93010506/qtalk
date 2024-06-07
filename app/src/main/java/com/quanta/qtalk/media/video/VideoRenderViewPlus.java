package com.quanta.qtalk.media.video;

import java.nio.ByteBuffer;
import java.util.Vector;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.UnSupportException;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
//import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.os.ConditionVariable;
import android.util.AttributeSet;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;
import android.view.View;

public class VideoRenderViewPlus extends View implements IVideoSink, IVideoSinkPollbased, IVideoStreamReportCB
{
    private final static     String TAG = "VideoRenderPlus";
    private static final String DEBUGTAG = "QTFa_VideoRenderPlus";
    private Bitmap.Config mConfig = Bitmap.Config.ALPHA_8;
    private int            mWidth = 0;
    private int            mHeight = 0;
    private final Matrix   mMatrix = new Matrix();
    private int            mMatrixDegree = 0;    
    private short          mSenderDegree = 0;
    private int            mDisplayOrientation = 0;
    private boolean        mFirstDraw = true;
    private final BitmapFactory.Options mBitmapOptions = new BitmapFactory.Options(); 
    private Rect           mTargetRect = null;

    private BitmapDepository mDepository = null;
    private final Vector<Bitmap>   mData2Draw = new Vector<Bitmap>();
    static class BitmapDepository
    {
        final Vector<Bitmap> mDataVector = new Vector<Bitmap>();
        final ConditionVariable mOnDataReturn = new ConditionVariable();
        public BitmapDepository(int size, int width, int height,Bitmap.Config config)
        {
            for (int i=0;i<size;i++)
            {
                mDataVector.add(Bitmap.createBitmap(width,height,config));
            }
        }
        
        public Bitmap borrowData()
        {
            if (mDataVector.size()==0)
            {
                //Log.e(DEBUGTAG,"BitmapDepository:block");
                mOnDataReturn.block(10);
                mOnDataReturn.close();
            }
            if (mDataVector.size()>0)
                return mDataVector.remove(0);
            else
                return null;
        }
        public void returnData(Bitmap data)
        {
            mDataVector.add(data);
            mOnDataReturn.open();
        }
    }
    public void setDisplayOrientation(int orientation)
    {
        if (mDisplayOrientation!=orientation)
        {
            mDisplayOrientation = orientation;
            onDegreeChange();
        }
    }

    private void onDegreeChange()
    {
        mMatrixDegree = (mSenderDegree)%360;
        if (mTargetRect!=null)
        {
            if (mMatrixDegree==90 || mMatrixDegree==270)
                if(mDisplayOrientation%2 == 1)
                {
                    _GetPreferedWidthAndHeight(mHeight,mWidth,0,0, getWidth() - 1, getHeight() - 1);
                }
                else
                {                    
                    _GetPreferedWidthAndHeight(mHeight,mWidth,0,0, getWidth() - 1, getHeight() - 1);
                }
            else
                if(mDisplayOrientation%2 == 0)
                {
                    _GetPreferedWidthAndHeight(mWidth,mHeight,0,0, getWidth() - 1, getHeight() - 1);
                }
                else
                {
                    _GetPreferedWidthAndHeight(mWidth,mHeight,0,0, getWidth() - 1, getHeight() - 1);
                }
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "mSenderDegree: "+mSenderDegree+" mMatrixDegree: "+mMatrixDegree+" mDisplayOrientation: "+mDisplayOrientation);
    }
    public VideoRenderViewPlus(Context context)
    {
        super(context);
    }
    public VideoRenderViewPlus(Context context, AttributeSet attrs)
    {
        super(context,attrs);
    }
    @Override
    protected void onSizeChanged (int w, int h, int oldw, int oldh)
    {
        if (mTargetRect!=null)
        {
             if (mMatrixDegree==90 || mMatrixDegree==270)
                  mTargetRect = _GetPreferedWidthAndHeight(mHeight,mWidth,0,0, w - 1, h - 1);
             else
                  mTargetRect = _GetPreferedWidthAndHeight(mWidth,mHeight,0,0, w - 1, h - 1);
        }
    }
    @Override
    protected void onDraw (Canvas canvas)
    {
        if (mFirstDraw)
        {
            android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_DISPLAY);
            mFirstDraw = false;
        }
        if (ENABLE_POLL_BASED)
        {
            if (mSource!=null)
            {
                synchronized(this)
                {
                    long timestamp = 0;
                    short rotation = 0;
                    int result = 0;
                    try
                    {
                        result = mSource.getMedia(mByteArray, mBufferSize);
                    } catch (Throwable e)
                    {
                        Log.e(DEBUGTAG, "onDraw",e);
                    }
                    if (result>0)
                    {
                        mEnableRedraw = true;
                        try
                        {
                            mBitmap.copyPixelsFromBuffer(mByteBuffer.rewind());
                            canvas.drawBitmap(mBitmap, mMatrix, null);
                        }catch(java.lang.Throwable e)
                        {
                            Log.e(DEBUGTAG, "onDraw",e);
                        }
                    }else if (mEnableRedraw)
                    {
                        canvas.drawBitmap(mBitmap, mMatrix, null);
                    }
                }
            }
        }else
        {
            if (mData2Draw.size()>0)
            {
                Bitmap tmpBitmap = mData2Draw.remove(0);
                try
                {
                    canvas.drawBitmap(tmpBitmap, mMatrix, null);
                }catch(java.lang.Throwable e)
                {
                    Log.e(DEBUGTAG, "onDraw",e);
                }finally
                {
                    mBitmap = tmpBitmap;
                    mDepository.returnData(tmpBitmap);
                }
            }else if (mBitmap!=null)
            {
                canvas.drawBitmap(mBitmap, mMatrix, null);
            }
        }
        if (mReportStr!=null && mTargetRect!=null)
            canvas.drawText(mReportStr,mTargetRect.left+10, mTargetRect.bottom-10, mPaint);
    }
    @Override
    public synchronized boolean onMedia(final ByteBuffer data,final int length,final long timestamp, final short rotation)
    {
        if (data!=null)
        {
            try
            {
                Bitmap tmpBitmap = mDepository.borrowData();
                if (tmpBitmap==null)
                {
                	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"Error mDepository is null, not data for borrow");
                    if (mData2Draw.size()>0)
                    {
                        tmpBitmap = mData2Draw.remove(0);
                    }
                }
                if (tmpBitmap!=null)
                {
                    tmpBitmap.copyPixelsFromBuffer(data.rewind());
                    mData2Draw.add(tmpBitmap);
                    if (mSenderDegree != rotation)
                    {
                        mSenderDegree = rotation;
                        onDegreeChange();
                    }
                    postInvalidate();                // force a redraw
                }else
                	if(ProvisionSetupActivity.debugMode) Log.e(DEBUGTAG,"Error no Bitmap for drawing ");
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG, "onMedia:length="+length,e);
            }
        }else
        {
            postInvalidate();                // force a redraw
            if (mSenderDegree != rotation)
            {
                mSenderDegree = rotation;
                onDegreeChange();
            }
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
            mDepository = new BitmapDepository(3,mWidth,mHeight,mConfig);
            mTargetRect = _GetPreferedWidthAndHeight(width,height,0,0, getWidth() - 1, getHeight() - 1);//new Rect(0, 0, getWidth() - 1, getHeight() - 1);
            
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
    private Rect _GetPreferedWidthAndHeight(int resWidth,int resHeight,int desX,int desY, int desWidth, int desHeight)
    {
        Rect rect = new Rect();
        desX = 0;
        desY = 0;
        desWidth++;
        desHeight++;
        int resultW = ((desHeight-desY) * resWidth) / resHeight;
        int resultH = ((desWidth-desX) * resHeight) / resWidth;
        if (desWidth !=0 && desHeight != 0)
        {
            rect.left = desX;
            rect.top = desY;
            
            if (resultW < (desWidth-desX))
            {
                rect.left = desX + (desWidth - resultW) / 2;
                resultH = desHeight;
            }
            else if (resultH < (desHeight-desY))
            {
                resultW = desWidth;
                rect.top = desY + (desHeight - resultH) / 2;
            }
            rect.right = rect.left + resultW;
            rect.bottom = rect.top + resultH;
        }
        else
        {
            rect.right = 0;
            rect.bottom =0;
            rect.left =0;
            rect.top = 0;
        }

         float scale_x  = (float)resultW/resWidth;
         float scale_y  = (float)resultH/resHeight;
         int rotate_shift = Math.abs(resultW -resultH) /2;
         mMatrix.reset();    
         mMatrix.preRotate(mMatrixDegree, resWidth/2, resHeight/2);//*(i%4)
         mMatrix.postScale(scale_x, scale_y);
         if(mMatrixDegree == 90)
              mMatrix.postTranslate(-rotate_shift, -rotate_shift);
         else if(mMatrixDegree == 270)
              mMatrix.postTranslate(rotate_shift, rotate_shift);
         mMatrix.postTranslate((desWidth-resultW)/2, (desHeight-resultH)/2);
        return rect;
    }
    private IVideoSourcePollbased mSource = null;
    private byte[]                mByteArray = null;
    private ByteBuffer            mByteBuffer= null;
    private int                   mBufferSize = 0;
    private Bitmap                mBitmap = null;
    private boolean               mEnableRedraw = false;
    private String                mReportStr = null;
    private static         Paint  mPaint = new Paint();
    static
    {
        //mPaint.setAlpha(0x40); 
        mPaint.setColor(Color.WHITE);
    }
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
    }
    
    @Override
    public void setLowBandwidth(boolean showLowBandWidth)
    {
        return ;
    }

    @Override
    public void release()
    {
        ;
    }
}
