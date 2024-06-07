package com.quanta.qtalk.ui;

import com.quanta.qtalk.Flag;
import com.quanta.qtalk.QtalkEngine;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.preference.PreferenceManager;
import com.quanta.qtalk.util.Log;

public class QTRAAlarm extends BroadcastReceiver 
{
//	public static final String TAG = "QTRAAlarm";
	private static final String DEBUGTAG = "QTFa_QTRAAlarm";
    public static final String 	PREF_WLAN = "wlan";
    public static final String 	PREF_3G = "3g";
    public static final String 	PREF_EDGE = "edge";
    public static final String 	PREF_VPN = "vpn";
    public static final boolean DEFAULT_WLAN = true;
    public static final boolean DEFAULT_3G = false;
    public static final boolean DEFAULT_EDGE = false;
    public static final boolean DEFAULT_VPN = false;
    @Override
    public void onReceive(Context context, Intent intent) 
    {
    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "onReceive QTRAAlarm");
    	if(ProvisionSetupActivity.debugMode) {
    		Log.d(DEBUGTAG, " PREF_WLAN : "+PreferenceManager.getDefaultSharedPreferences(context).getBoolean(PREF_WLAN, DEFAULT_WLAN)
    				+",PREF_3G : "+PreferenceManager.getDefaultSharedPreferences(context).getBoolean(PREF_3G, DEFAULT_3G)
    				+",PREF_VPN : "+PreferenceManager.getDefaultSharedPreferences(context).getBoolean(PREF_VPN, DEFAULT_VPN) 
    				+",PREF_EDGE : "+PreferenceManager.getDefaultSharedPreferences(context).getBoolean(PREF_EDGE, DEFAULT_EDGE)
    				+",SIPRunnalbe : " + Flag.SIPRunnable.getState());
    	}
	    if ((PreferenceManager.getDefaultSharedPreferences(context).getBoolean(PREF_WLAN, DEFAULT_WLAN) ||
	                    PreferenceManager.getDefaultSharedPreferences(context).getBoolean(PREF_3G, DEFAULT_3G) ||
	                    PreferenceManager.getDefaultSharedPreferences(context).getBoolean(PREF_VPN, DEFAULT_VPN) ||
	                    PreferenceManager.getDefaultSharedPreferences(context).getBoolean(PREF_EDGE, DEFAULT_EDGE))
	                    && Flag.SIPRunnable.getState() == true) {
	    	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "onReceive QTRAAlarm  startForegroundService QTService."+Flag.SIPRunnable.getState());
	    		QtalkEngine.StartQTService(context);
	    		//context.startForegroundService(new Intent(context,QTService.class));
	            if (QTReceiver.engine(context,false).isLogon())
	                QTReceiver.engine(context,false).keepAlive(context);
	            else
	                if(ViroActivityManager.isViroForeGround(context))
                    {
                        Intent myintent = new Intent();
                        myintent.setAction(QTReceiver.ACTION_SIP_LOGIN);//ACTION_RESUME);
                        myintent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                        context.startActivity(myintent);
                    }
	    } else
	    {
	            context.stopService(new Intent(context,QTService.class));
	            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "onReceive QTService is stopped");
	    }
    }
}