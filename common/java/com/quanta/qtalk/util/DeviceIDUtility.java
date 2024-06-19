package com.quanta.qtalk.util;

import com.quanta.qtalk.ui.ProvisionSetupActivity;
import com.quanta.qtalk.util.Log;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.provider.Settings;
import android.telephony.TelephonyManager;

public class DeviceIDUtility
{
 //   private static final String TAG = "DeviceIDUtility";
    private static final String DEBUGTAG = "QTFa_DeviceIDUtility";
    public static String getDeviceMACAddress(Context context)
    {
        String result = null;
        
		String deviceId = Settings.System.getString(context.getContentResolver(),
                Settings.System.ANDROID_ID);
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"deviceId:"+deviceId);
        
        TelephonyManager manager= (TelephonyManager)context.getSystemService(Context.TELEPHONY_SERVICE);
        String serial= manager.getDeviceId();
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"serial:"+serial);
        
        WifiManager wimanager = (WifiManager)context.getSystemService(Context.WIFI_SERVICE);
        result = wimanager.getConnectionInfo().getMacAddress();
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"getDeviceMACAddress:"+result);
        return result;
    }
    
    public static String getSerialNumber(Context context)
    {
        String result = null;
        TelephonyManager manager= (TelephonyManager)context.getSystemService(Context.TELEPHONY_SERVICE);
        String serial= manager.getDeviceId();
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"serial:"+serial);
        
        result = serial;
        return result;
    }
}
