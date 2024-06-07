package com.quanta.qtalk.media.video;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

public class StatisticAnalyzer
{
    private static final String TAG = "StatisticAnalyzer";
    private static final String DEBUGTAG = "StatisticAnalyzer";
    public static final boolean ENABLE = true;
    private static long mCountCapture = 0;
    private static long mCountControl = 0;
    private static long mCountEncode = 0;
    private static long mCountSend = 0;
    private static long mCountRecv = 0;
    private static long mCountDecode = 0;
    private static long mCountRender = 0;
    private static DisplayThread mDthread = null;
    public static void onCaptureSuccess()
    {
        ++mCountCapture;
    }
    public static void onControlSuccess()
    {
        ++mCountControl;
    }
    public static void onEncodeSuccess()
    {
        ++mCountEncode;
    }
    public static void onSendSuccess()
    {
        ++mCountSend;
    }
    public static void onRecvSuccess()
    {
        ++mCountRecv;
    }
    public static void onDecodeSuccess()
    {
        ++mCountDecode;
    }
    public static void onRenderSuccess()
    {
        ++mCountRender;
    }
    public static void start()
    {
        if (mDthread==null)
        {
            mCountCapture = 0;
            mCountControl = 0;
            mCountEncode = 0;
            mCountSend = 0;
            mCountRecv = 0;
            mCountDecode = 0;
            mCountRender = 0;
            mDthread = new DisplayThread();
            mDthread.start();
        }
    }
    public static void stop()
    {
        if (mDthread!=null)
        {
            mDthread.mRunning = false;
            mDthread = null;
        }
    }
    static class DisplayThread extends Thread
    {
        private boolean mRunning = false;
        public DisplayThread()
        {
            mRunning = true;
        }
        @Override
        public void run()
        {
            long start = System.currentTimeMillis();
            long gap = 0;
            // Count and Display
            while(mRunning)
            {   
                try
                {
                    Thread.sleep(3000);
                } catch (InterruptedException e)
                {
                }
                gap = (System.currentTimeMillis() - start)/1000;
                if(ProvisionSetupActivity.debugMode) Log.l(DEBUGTAG,"[video] capture:"+mCountCapture/gap+
								                          ", control:"+mCountControl/gap+
								                          ", encode:"+mCountEncode/gap+
								                          ", send:"+mCountSend/gap+
								                          ", recv:"+mCountRecv/gap+
								                          ", decode:"+mCountDecode/gap+
								                          ", render:"+mCountRender/gap);
            }
        }
    }
}
