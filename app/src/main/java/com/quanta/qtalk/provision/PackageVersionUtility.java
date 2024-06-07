package com.quanta.qtalk.provision;

import android.content.Context;

import com.quanta.qtalk.provision.PackageChecker.AppInfo;

public class PackageVersionUtility
{
    public static String getVersion(Context context)
    {
        String result = null;
        StringBuilder sb = new StringBuilder();
        AppInfo qt_info = getQTInfo(context);
        if(qt_info != null)
        {
            String qt_version = qt_info.getVersionName()+"."+qt_info.getVersionCode();
            sb.append(qt_info.getAppName());
            sb.append(":");
            sb.append(qt_version);
        }
        /*
        try
        {
            AppInfo callengine_info = PackageChecker.getInstalledCallEngineInfo(context);
            String callengine_version = callengine_info.getVersionName()+"."+callengine_info.getVersionCode();
            sb.append(" ");
            sb.append(callengine_info.getAppName());
            sb.append(":");
            sb.append(callengine_version);
        }catch(Exception e){}
        */
        try
        {
            AppInfo x264_info = PackageChecker.getInstalledX264Info(context);
            String x264_version = x264_info.getVersionName()+"."+x264_info.getVersionCode();
            sb.append(" ");
            sb.append(x264_info.getAppName());
            sb.append(":");
            sb.append(x264_version);
        }catch(Exception e){}
        result = sb.toString();
        return result;
    }
    
    public static AppInfo getQTInfo(Context context)
    {
        AppInfo qt_info = null;
        qt_info = PackageChecker.getInstalledQTInfo(context);
        return qt_info;
    }
    
    public static AppInfo getX264ServiceInfo(Context context)
    {
        AppInfo x264_info = null;
        x264_info = PackageChecker.getInstalledX264Info(context);
        return x264_info;
    }

}
