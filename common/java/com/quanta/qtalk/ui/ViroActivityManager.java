package com.quanta.qtalk.ui;

import java.util.List;

import com.quanta.qtalk.util.Log;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningTaskInfo;
import android.content.ComponentName;
import android.content.Context;

public class ViroActivityManager
{
    private final static Object mLock = new Object();
    private static Activity mOldActivity = null;
//    private static final String TAG = "ViroActivityManager";
    private static final String DEBUGTAG = "QTFa_ViroActivityManager";
    public static void setActivity(Activity activity)
    {
        // keep last Activity in specific Activities.
        // skip setting SetupActivity page
        if(AbstractSetupActivity.class.isInstance(activity) && QTReceiver.engine(activity,false).isLogon())
        {
            return;
        }
        synchronized(mLock)
        {
            mOldActivity = activity;
        }
    }
    public static Activity getLastActivity()
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"getLastActivity:"+mOldActivity);
        return mOldActivity;
    }
    
    public static boolean isViroForeGround(Context context)
    {
        boolean result = false;
        try{
            ActivityManager am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
            // get the info from the currently running task
            List<RunningTaskInfo> taskInfo = am.getRunningTasks(1);
            
            //Log.d(DEBUGTAG, "CURRENT Activity ::"+ taskInfo.get(0).topActivity.getClassName()+", package name:"+taskInfo.get(0).topActivity.getPackageName());
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"isViroForeGround getPackageName():"+context.getPackageName());
            ComponentName component_info = taskInfo.get(0).topActivity;
            if(component_info != null && component_info.getClassName() != null && component_info.getPackageName().compareTo(context.getPackageName())==0 )
            {
            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"component_info.getPackageName():"+component_info.getPackageName());
                result = true;
            }
        }
        catch(Exception e)
        {
            Log.e(DEBUGTAG,"isViroForeGround",e);
        }
        return result;
    }
    
    public static boolean isCurrentPageTopActivity(Context context, String className)
    {
        boolean result = false;
        try{
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"==>isCurrentPageTopActivity");
            ActivityManager am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
            // get the info from the currently running task
            List<RunningTaskInfo> taskInfo = am.getRunningTasks(1);
            ComponentName component_info = taskInfo.get(0).topActivity;
            if(component_info != null && component_info.getClassName() != null && component_info.getPackageName().compareTo(context.getPackageName())==0 )
            {
                if(component_info.getClassName().compareTo(className)==0)
                    result = true;
            }
        }
        catch(Exception e)
        {
            Log.e(DEBUGTAG,"isCurrentPageTopActivity",e);
        }
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"<==isCurrentPageTopActivity:"+result);
        return result;
    }
}
