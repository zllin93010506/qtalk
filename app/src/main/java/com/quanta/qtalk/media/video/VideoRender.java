package com.quanta.qtalk.media.video;

import java.nio.ByteBuffer;
import java.util.Vector;

import com.quanta.qtalk.UnSupportException;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
//import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.os.ConditionVariable;
import android.util.AttributeSet;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;
import android.view.View;

public class VideoRender extends View implements IVideoSink 
{
    private final static     String TAG = "VideoRender";
    private static final String DEBUGTAG = "QTFa_VideoRender";
    private Bitmap.Config mConfig = Bitmap.Config.ALPHA_8;
    private int     mWidth = 0;
    private int     mHeight = 0;
    private BitmapDepository mDepository = null;
    private final Vector<Bitmap>   mData2Draw = new Vector<Bitmap>();
//    private int     mFormat = 0;
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
    private final BitmapFactory.Options mBitmapOptions = new BitmapFactory.Options(); 
    private Rect    mTargetRect = null;
    
    public VideoRender(Context context)
    {
        super(context);
    }
    public VideoRender(Context context, AttributeSet attrs)
    {
        super(context,attrs);
    }
    @Override
    protected void onSizeChanged (int w, int h, int oldw, int oldh)
    {
        if (mTargetRect!=null)
            mTargetRect = _GetPreferedWidthAndHeight(mWidth,mHeight,0,0, w - 1, h - 1);
    }
    @Override
    protected void onDraw (Canvas canvas)
    {
        if (mData2Draw.size()>0)
        {
            Bitmap tmpBitmap = mData2Draw.remove(0);
            try
            {
                canvas.drawBitmap(tmpBitmap, null, mTargetRect, null);
                
            }catch(java.lang.Throwable e)
            {
                Log.e(DEBUGTAG, "onDraw",e);
            }finally
            {
                mDepository.returnData(tmpBitmap);
                if (mData2Draw.size()>0)
                    postInvalidate();// force a redraw
            }
        }
    }
    @Override
    public synchronized boolean onMedia(final ByteBuffer data,final int length,final long timestamp, final short rotation)
    {
        // Update Bitmap
        try
        {
            Bitmap tmpBitmap = mDepository.borrowData();
            if (tmpBitmap!=null)
            {
                tmpBitmap.copyPixelsFromBuffer(data.rewind());
                mData2Draw.add(tmpBitmap);
                postInvalidate();                // force a redraw
            }
        }catch(java.lang.Throwable e)
        {
            Log.e(DEBUGTAG, "onMedia:length="+length,e);
        }
        return true;
    }

    @Override
    public synchronized void setFormat(int format, int width, int height) throws UnSupportException 
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "setFormat:"+format+" "+width+" "+height);
        Bitmap.Config config = null;
        switch(format)
        {
        case PixelFormat.A_8:
            config = Bitmap.Config.ALPHA_8;
            break;
        case PixelFormat.RGB_565:
            config = Bitmap.Config.RGB_565;
            break;
        case PixelFormat.RGBA_8888:
            config = Bitmap.Config.ARGB_8888;
            break;
        case PixelFormat.RGBA_4444:
            config = Bitmap.Config.ARGB_4444;
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
            mDepository = new BitmapDepository(5,mWidth,mHeight,mConfig);
            mTargetRect = _GetPreferedWidthAndHeight(width,height,0,0, getWidth() - 1, getHeight() - 1);//new Rect(0, 0, getWidth() - 1, getHeight() - 1);
            
            mBitmapOptions.outHeight = height;
            mBitmapOptions.outWidth = width;
        }
    }
    private Rect _GetPreferedWidthAndHeight(int resWidth,int resHeight,int desX,int desY, int desWidth, int desHeight)
    {
        Rect rect = new Rect();
        if (desWidth !=0 && desHeight != 0)
        {
            rect.left = desX;
            rect.top = desY;
            int resultW = ((desHeight-desY) * resWidth) / resHeight;
            int resultH = ((desWidth-desX) * resHeight) / resWidth;

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
        }else
        {
            rect.right = 0;
            rect.bottom =0;
            rect.left =0;
            rect.top = 0;
        }
        return rect;
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
