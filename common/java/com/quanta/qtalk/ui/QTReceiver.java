package com.quanta.qtalk.ui;

import android.app.Activity;
import android.app.AlarmManager;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.text.TextUtils;
import android.text.format.Time;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.Flag;
import com.quanta.qtalk.QtalkAction;
import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.QtalkEngine;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.VersionDefinition;
import com.quanta.qtalk.call.MDP;
import com.quanta.qtalk.provision.PackageChecker;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;

import java.io.IOException;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;
import java.util.Hashtable;

public class QTReceiver extends BroadcastReceiver 
{
    private static final String DEBUGTAG = "QSE_QTReceiver";
    public static Context        mContext     = null;
    public static QtalkEngine    mEngine      = null;
    
    public static boolean Mobile3G = false;
	public static boolean MobileWifi = false;
	
	private final int PROVISION_UPDATE_SUCCESS = 1;
	private final int PHONEBOOK_UPDATE_SUCCESS = 2;
	private final int KEEPALIVE_UPDATE_SUCCESS = 3;
	
	private final int FAIL = 0;
    QtalkSettings qtalk_settings;
    long prev_time;
    
    Time lastCheckProvisionTime = new Time();
    Time currentTime = new Time();
    long CheckProvisionUpdatePeriod = 0;
    static String prev_ip = new String();
    static final String nonetwork = "has been no network, sip may no login";

    private static QTService mService = null;

    public void setQTService(QTService s) {
    	mService = s;
	}
    
    public String getLocalIpAddress() {  
    	try {  
    		for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();) {  
    			NetworkInterface intf = en.nextElement();
    			for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements();) { 
    				InetAddress inetAddress = enumIpAddr.nextElement();
    				if (!inetAddress.isLoopbackAddress()  && inetAddress instanceof Inet4Address) { 
    					return inetAddress.getHostAddress().toString();
    				}  
    			}  
    		}  
    	} catch (SocketException ex) {  
    		Log.e(DEBUGTAG, ex.toString());  
    	}  
    	return null;
	}
    
    String ntohl(int i)
    {
    	int i5 = i & 0xFF;
        int i6 = (i >> 8) & 0xFF;
        int i7 = (i >> 16) & 0xFF;
        int i8 = (i >> 24) & 0xFF;
		return ""+i5+"."+i6+"."+i7+"."+i8;		
    }
    
    Handler mHandler = new Handler(){
		public void handleMessage(Message msg)
        {
    		switch(msg.what)
            {
            	default: break;
            	case PHONEBOOK_UPDATE_SUCCESS:
            		QThProvisionUtility.provision_info.lastPhonebookFileTime = msg.getData().getString("timestamp");
            		Log.d(DEBUGTAG, "lastPhonebookFileTime:"+QThProvisionUtility.provision_info.lastPhonebookFileTime);
    				try {
    					QTReceiver.engine(mContext, false).updateSetting(mContext, QThProvisionUtility.provision_info);
					} catch (FailedOperateException e) {
						Log.e(DEBUGTAG,"PHONEBOOK_UPDATE_SUCCESS:context=null");
					}            		
            		lastCheckProvisionTime.setToNow();
            		break;
    			case PROVISION_UPDATE_SUCCESS:
    				if(ProvisionSetupActivity.debugMode) {
	    				Log.d(DEBUGTAG,"(provision)lastUpdateTime:"+msg.getData().getString("timestamp"));
	    				Log.d(DEBUGTAG,"(provision)session:"+msg.getData().getString("session"));
	    				Log.d(DEBUGTAG,"(provision)key:"+msg.getData().getString("key"));
    				}
					if(QThProvisionUtility.provision_info != null) {
						QThProvisionUtility.provision_info.lastProvisionFileTime = msg.getData().getString("timestamp");
	    				try {
	    					QTReceiver.engine(mContext, false).updateSetting(mContext, QThProvisionUtility.provision_info);
						} catch (FailedOperateException e) {
                            Log.e(DEBUGTAG,"PROVISION_UPDATE_SUCCESS:context=null");
						}
	                    PackageChecker.setProvisionInfo(QThProvisionUtility.provision_info);                    
	                    lastCheckProvisionTime.setToNow();
					}
    				break;
    			case KEEPALIVE_UPDATE_SUCCESS:
    				if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"(keepalive)lastUpdateTime:"+msg.getData().getString("timestamp"));

    				QThProvisionUtility.provision_info.lastProvisionFileTime = msg.getData().getString("timestamp");
    				try {
    					QTReceiver.engine(mContext, false).updateSetting(mContext, QThProvisionUtility.provision_info);
					} catch (FailedOperateException e) {
                        Log.e(DEBUGTAG,"KEEPALIVE_UPDATE_SUCCESS:context=null");
					}
                    PackageChecker.setProvisionInfo(QThProvisionUtility.provision_info);                    
                    lastCheckProvisionTime.setToNow();
    				break;    				
    			case FAIL:
    				break;
            }
        }
    };
    
    QThProvisionUtility qtnMessenger = new QThProvisionUtility(mHandler, null);
    
	@Override
    public void onReceive(final Context context,final Intent intent)
    {
        final String intentAction = intent.getAction();
        if(!TextUtils.equals(intentAction, "android.intent.action.alarm"))
            Log.d(DEBUGTAG, "onReceive(), intent:"+intentAction);
        if (intentAction.equals(Intent.ACTION_TIME_TICK) && VersionDefinition.CURRENT == VersionDefinition.PROVISION )
        {
        	try {
				qtalk_settings = QTReceiver.engine(context, false).getSettings(context);
			} catch (FailedOperateException e) {
                Log.e(DEBUGTAG,"onReceive:context=null 1");
                return;
			}
        	currentTime.setToNow();
        	currentTime.toMillis(false);
        	
        	if( currentTime.toMillis(false) - lastCheckProvisionTime.toMillis(false) >= CheckProvisionUpdatePeriod*1000)
        	{
        		Log.d(DEBUGTAG, "mEngine.isInSession():"+mEngine.isInSession()+
                                ", config_file_forced_provision_periodic_timer:"+qtalk_settings.config_file_forced_provision_periodic_timer+
                                ", config_file_download_error_retry_timer:"+ qtalk_settings.config_file_download_error_retry_timer);
        		if(mEngine.isInSession() && qtalk_settings.config_file_forced_provision_periodic_timer != null) {
        			CheckProvisionUpdatePeriod = Integer.valueOf( qtalk_settings.config_file_forced_provision_periodic_timer );        			
        		}	
        		else if( qtalk_settings.config_file_forced_provision_periodic_timer != null) {
        			//perform check here...
        			new Thread( new Runnable(){
        				QtalkDB qtalkdb = new QtalkDB();
        	            @Override
        	            public void run()
        	            {
        	            	try{
	        	            	String provision_url = "https://"+qtalk_settings.provisionServer+
	        	            							qtalk_settings.provisionType+
	        	            							"&service=provision&action=check&uid="+
	        	            							qtalk_settings.provisionID+"&session="+qtalk_settings.session+"&timestamp="+qtalk_settings.lastProvisionFileTime;
	        	            	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"provision url:"+provision_url);
	        	            	
	        		            qtnMessenger.httpsConnect(provision_url, qtalk_settings.session, qtalk_settings.versionNumber);
	        		            String phonebook_uri = "https://"+qtalk_settings.provisionServer+
	        	                  						qtalk_settings.provisionType+
	        	                   						"&service=phonebook&action=check&uid="+
	        	                   						qtalk_settings.provisionID+"&session="+qtalk_settings.session+"&timestamp="+qtalk_settings.lastPhonebookFileTime;
	        	                
	        	                try {
	        	                	qtnMessenger.httpsConnect(phonebook_uri);
	        	                } 
	        	                catch (IOException e) {
	        	                	Log.e(DEBUGTAG,"onReceive:"+e);
	        	                	Message msg1 = new Message();
	        	                	msg1.what = FAIL;
	        	                	Bundle bundle = new Bundle();
	        	                	bundle.putString(ProvisionSetupActivity.MESSAGE, "網路連線失敗\r\n");
	        	                	msg1.setData(bundle);
	        	                	mHandler.sendMessage(msg1);
	        	                }       		            	
        	            	} 
        	            	catch (IOException e) {
        	            		Log.e(DEBUGTAG,"onReceive:"+e);
        						Message msg = new Message();
        			            msg.what = FAIL;
        			            Bundle bundle = new Bundle();
        			            bundle.putString(ProvisionSetupActivity.MESSAGE, "網路連線失敗\r\n");
        			            msg.setData(bundle);
        			            mHandler.sendMessage(msg);
        					}
        	            	qtalkdb.close();
        	            }
        			}).start();
        		}
        		if(true) {
        			// download fail
        			CheckProvisionUpdatePeriod = Integer.valueOf( qtalk_settings.config_file_download_error_retry_timer );
        			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"CheckProvisionUpdatePeriod:"+CheckProvisionUpdatePeriod);
        		}
        		
        	}// end of checking provision
			try {
				qtalk_settings = QTReceiver.engine(context, false).getSettings(context);
				String keepalive_url = "https://"+qtalk_settings.provisionServer+"?model=qtha&session="+qtalk_settings.session+"&service=keepalive";
				qtnMessenger.httpsConnect(keepalive_url);
			}
			catch (FailedOperateException e) { //for getSettings
				Log.e(DEBUGTAG,"onReceive:context=null 2");
			}
			catch (IOException e) { //for httpsConnect
				Log.e(DEBUGTAG,"finishQT:"+e);
				Message msg1 = new Message();
				msg1.what = FAIL;
				Bundle bundle = new Bundle();
				bundle.putString(ProvisionSetupActivity.MESSAGE, "網路連線失敗\r\n");
				msg1.setData(bundle);
				mHandler.sendMessage(msg1);
			}
        }
        else if (intentAction.equals(Intent.ACTION_BOOT_COMPLETED) || intentAction.equals(QtalkAction.ACTION_APP_START)) {
        	Log.d(DEBUGTAG,"receive action=" + intent.getAction());
        }
        else if (intentAction.equals(ConnectivityManager.CONNECTIVITY_ACTION)) {
        	ConnectivityManager connManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE); 
        	NetworkInfo info=connManager.getActiveNetworkInfo();        	
        	if(info != null)
        	{
        		if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"Type:"+info.getTypeName()+" TypeNum:"+info.getType()+ " State:"+info.getDetailedState()+" describeContents:"+info.describeContents()+" SubType:"+info.getSubtypeName());
        		if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"getLocalIpAddress:"+getLocalIpAddress()+" prev_ip:"+prev_ip);
        		WifiManager wifi_service = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        		WifiInfo wifiinfo = wifi_service.getConnectionInfo();
        		
        		SupplicantState sstate = wifiinfo.getSupplicantState();
        		WifiInfo.getDetailedStateOf(sstate);
				String now_ip = getLocalIpAddress();
				
	        	if(info.getType() == ConnectivityManager.TYPE_MOBILE && !prev_ip.equals(now_ip))
	        	{
	        		Log.d(DEBUGTAG,"NeworkType is MOBILE 3G");
	        		if(mEngine != null)
	        		{
	        			Log.d(DEBUGTAG , "Network change!!(wifi to 3G)");
	        			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "prev_ip="+prev_ip+"   now_ip="+now_ip);
	        			if(prev_ip.equals(nonetwork)){
	        				QTReceiver.StartQTService(context);
	        			}
	        			mEngine.sr2_switch_network_interface(1, now_ip);
	        			prev_ip = new String(now_ip);
	        			Mobile3G = true;
	        			MobileWifi = false;
	        			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG , "Mobile3G="+Mobile3G+"     MobileWifi="+MobileWifi);
	        		}
	        	}
	        	else if(info.getType() == ConnectivityManager.TYPE_WIFI && !prev_ip.equals(now_ip))
	        	{
	        		Log.d(DEBUGTAG,"NeworkType is Wifi");
	        		if(mEngine != null) 
	        		{
	        			Log.d(DEBUGTAG, "Network change to wifi!!");
	        			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "prev_ip="+prev_ip+"   now_ip="+now_ip);
	        			if(now_ip==null) // for HTC 
	        				now_ip = "";
	        			if(prev_ip.equals(nonetwork)&&!now_ip.equals("")){
	        				QTReceiver.StartQTService(context);
	        			}
	        			mEngine.sr2_switch_network_interface(2, now_ip);
	        			prev_ip = new String(now_ip);
	        			Mobile3G = false;
	        			MobileWifi = true;
	        			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG , "Mobile3G="+Mobile3G+"     MobileWifi="+MobileWifi);
	        		}
	        	}
	        	else if(info.getType() == ConnectivityManager.TYPE_ETHERNET && !prev_ip.equals(now_ip))
	        	{
	        		Log.d(DEBUGTAG,"NeworkType is ETH");
	        		if(mEngine != null) 
	        		{
	        			Log.d(DEBUGTAG, "Network change to ETH!!");
	        			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "prev_ip="+prev_ip+"   now_ip="+now_ip);
	        			if(prev_ip.equals("has been no network, sip may no login")){
	        				QTReceiver.StartQTService(context);
	        			}
	        			mEngine.sr2_switch_network_interface(3, now_ip);
	        			prev_ip = new String(now_ip);
	        			Mobile3G = false;
	        			MobileWifi = true;
	        			if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG , "Mobile3G="+Mobile3G+"     MobileWifi="+MobileWifi);
	        		}
	        	}        		
        	}
        	else {
        		prev_ip = nonetwork;
        		if(mEngine != null) {
        			mEngine.sr2_switch_network_interface(0, "");
        			Log.d(DEBUGTAG,"NetworkInfo is null");
        		}
        	}        	
        }
		else if (intentAction.equals(QTService.ACTION_ALARM)) {
			if((!mPhonebookPause) && (mService != null)) {
				new Thread(PhonebookCheckRunnable).start();
            }
			int sec = intent.getIntExtra("seconds", 5);
			Hack.set_alarm(context, QTReceiver.class, QTService.ACTION_ALARM, sec);
			if(mAlarmCount >= 20) {
                if(Hack.isQF3a())
				    Hack.dumpAllResource(context, DEBUGTAG);
				mAlarmCount = 0;
			}
			else {
				mAlarmCount++;
			}
		}
        else if (intentAction.equals(QTService.ACTION_PHONEBOOK)) {
            mPhonebookPause = intent.getBooleanExtra("pause", false);
            if(mPhonebookPause)
                Hack.cancel_alarm(context);
            else
                Hack.set_alarm(context, QTReceiver.class, QTService.ACTION_ALARM, 5);
			Log.d(DEBUGTAG,"ACTION_PHONEBOOK received pause="+mPhonebookPause);
		}
    }
    static int mAlarmCount = 0;
    static boolean mPhonebookPause = false;
	Runnable PhonebookCheckRunnable = new Runnable() {
		@Override
		public void run() {
			mService.phonebookcheck();
		}
	};
    //=====================================================
    public static boolean isExited()
    {
        return (mEngine == null);
    }

    public static synchronized QtalkEngine engine(final Context context,final boolean isStartService)
    {
    	if(context instanceof QTService)
    		mContext = context;
        if (mEngine == null) {
            mEngine = QtalkEngine.getInstance();
        }
        if (isStartService && !(context instanceof QTService)) {
        	QTReceiver.StartQTService(context);
        }
        return mEngine;
    }
    
    public static void finishQT(final Context context)
    {
    	if( VersionDefinition.CURRENT == VersionDefinition.PROVISION )
    	{
    		Thread logout_th = new Thread( new Runnable(){	    		
	            @Override
	            public void run()
	            {       
			    	QtalkSettings pi = null;
			    	try {
			    		pi = mEngine.getSettings(context);
						pi.lastProvisionFileTime = "";
						pi.lastPhonebookFileTime = "";
						if(mContext!=null)
							mEngine.updateSetting(mContext, pi);
						else
							Log.e(DEBUGTAG,"finishQT:context is not QTService.");
					} catch (FailedOperateException e) {
                        Log.e(DEBUGTAG,"finishQT:context=null.");
					}
	            }
	    	});
	    	logout_th.start();
	    	try {
				logout_th.join(30000);
			} catch (InterruptedException e) {
				Log.e(DEBUGTAG,"finishQT:"+e);
			}
    	}
        if (mEngine != null) {
            //mEngine.eixt();
            mEngine = null;
        }
        if(mService != null)
            mService = null;
        context.stopService(new Intent(context,QTService.class));
        final NotificationManager notificationManager = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancel(NOTIFICATION_ID_LOGON);
        notificationManager.cancel(NOTIFICATION_ID_CALLLOG);
    }
    
    protected static void alarm(final int renew_time,final Class <?>cls)
    {
    	Log.d(DEBUGTAG, "==>alarm:renew_time="+renew_time);
        Intent intent = new Intent(mContext, cls);
        PendingIntent sender = PendingIntent.getBroadcast(mContext,0, intent, PendingIntent.FLAG_IMMUTABLE);
        AlarmManager am = (AlarmManager)mContext.getSystemService(Context.ALARM_SERVICE);
        am.cancel(sender);
        if (renew_time > 0)
              am.set(AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime()+renew_time*1000, sender);
        Log.d(DEBUGTAG, "<==alarm:"+(SystemClock.elapsedRealtime()+renew_time*1000));
    }

//==================================================================================
//    public static final int NOTIFICATION_ID_MISSED_CALL = 1;
//    public static final int NOTIFICATION_ID_ONLINE      = 2;
//    public static final int NOTIFICATION_ID_OFFLINE     = 3;
    public static final int NOTIFICATION_ID_LOGON       = 3333;
    public static final int NOTIFICATION_ID_CALLLOG     = 1;
    //==================================================================================
    public static final String ACTION_INCOMING_CALL     = "com.quanta.qtalk.ui.incomingcall";
    public static final String ACTION_OUTGOING_CALL     = "com.quanta.qtalk.ui.outgoingcall";
    public static final String ACTION_OUTGOING_CALL_FAIL= "com.quanta.qtalk.ui.outgoingcallfail";
    public static final String ACTION_SETUP             = "com.quanta.qtalk.ui.setup";
    public static final String ACTION_HISTORY           = "com.quanta.qtalk.ui.history";
    public static final String ACTION_HISTORY_RESUME    = "com.quanta.qtalk.ui.history.resume";

    public static final String ACTION_DIALER            = "com.quanta.qtalk.ui.dialer";
    public static final String ACTION_VIDEO_SESSION     = "com.quanta.qtalk.ui.videosession";
    public static final String ACTION_AUDIO_SESSION     = "com.quanta.qtalk.ui.audiosession";
    public static final String ACTION_RESUME            = "com.quanta.qtalk.ui.resume";
    public static final String ACTION_LOGIN_RETRY       = "com.quanta.qtalk.ui.login.retry";
    public static final String ACTION_SIP_LOGIN         = "com.quanta.qtalk.ui.login.sip";
    public static final String ACTION_PTT_SESSION       = "com.quanta.qtalk.ui.pttsession";
    
    public static final String KEY_LOGON_STATE          = "LOGON_STATE";
    public static final String KEY_LOGON_ID             = "LOGON_ID";
    public static final String KEY_LOGON_MSG            = "LOGON_MSG";

    public static final String KEY_REMOTE_ID            = "REMOTE_ID";
    public static final String KEY_CALL_ID              = "CALL_ID";
    public static final String KEY_CALL_TYPE            = "CALL_TYPE";
    public static final String KEY_ENABLE_VIDEO         = "ENABLE_VIDEO";
    
    public static final String KEY_ERRORCODE            = "ERROR_CODE";
    
    
    public static final String KEY_AUDIO_DEST_IP        = "AUDIO_DEST_IP";
    public static final String KEY_AUDIO_LOCAL_PORT     = "AUDIO_LOCAL_PORT";
    public static final String KEY_AUDIO_DEST_PORT      = "AUDIO_DEST_PORT";
    public static final String KEY_AUDIO_PAYLOAD_TYPE   = "AUDIO_PAYLOAD_TYPE";
    public static final String KEY_AUDIO_SAMPLE_RATE    = "AUDIO_SAMPLERATE";
    public static final String KEY_AUDIO_BITRATE        = "AUDIO_BITRATE";
    public static final String KEY_AUDIO_MIME_TYPE      = "AUDIO_MIMETYPE";
    public static final String KEY_AUDIO_SSRC           = "AUDIO_SSRC";

    public static final String KEY_VIDEO_DEST_IP        = "VIDEO_DEST_IP";
    public static final String KEY_VIDEO_LOCAL_PORT     = "VIDEO_LOCAL_PORT";
    public static final String KEY_VIDEO_DEST_PORT      = "VIDEO_DEST_PORT";
    public static final String KEY_VIDEO_PAYLOAD_TYPE   = "VIDEO_PAYLOAD_TYPE";
    public static final String KEY_VIDEO_SAMPLE_RATE    = "VIDEO_SAMPLERATE";
    public static final String KEY_VIDEO_MIME_TYPE      = "VIDEO_MIMETYPE";
    public static final String KEY_VIDEO_SSRC           = "VIDEO_SSRC";
    
    public static final String KEY_CAMERA_INDEX         = "CAMERA_INDEX"; 
    public static final String KEY_SESSION_START_TIME   = "SESSION_START_TIME";
    public static final String KEY_PRODUCT_TYPE         = "PRODUCT_TYPE";
    public static final String KEY_CALL_HOLD            = "CALL_HOLD";
    public static final String KEY_CALL_MUTE            = "CALL_MUTE";
    public static final String KEY_CALL_ONHOLD          = "CALL_ONHOLD";
    public static final String KEY_SPEAKER_ON           = "SPEAKER_ON";
    
    public static final String KEY_VIDEO_WIDTH          = "VIDEO_WIDTH";
    public static final String KEY_VIDEO_HEIGHT         = "VIDEO_HEIGHT";
    public static final String KEY_VIDEO_FPS            = "VIDEO_FPS";
    public static final String KEY_VIDEO_KBPS           = "VIDEO_KBPS";
    public static final String KEY_ENABLE_PLI           = "ENABLE_PLI";
    public static final String KEY_ENABLE_FIR           = "ENABLE_FIR";
    public static final String KEY_ENABLE_TEL_EVENT     = "ENABLE_TEL_EVENT";
    
    public static final String KEY_AUDIO_FEC_CTRL				= "AUDIO_FEC_CTRL";		//eason - FEC info
    public static final String KEY_AUDIO_FEC_SEND_PTYPE		= "AUDIO_FEC_SEND_PTYPE";
    public static final String KEY_AUDIO_FEC_RECV_PTYPE		= "AUDIO_FEC_RECV_PTYPE";
    public static final String KEY_VIDEO_FEC_CTRL				= "VIDEO_FEC_CTRL";
    public static final String KEY_VIDEO_FEC_SEND_PTYPE		= "VIDEO_FEC_SEND_PTYPE";
    public static final String KEY_VIDEO_FEC_RECV_PTYPE		= "VIDEO_FEC_RECV_PTYPE";
    
    public static final String KEY_ENALBE_AUDIO_SRTP		    = "ENALBE_AUDIO_SRTP";    //Iron - SRTP info
    public static final String KEY_AUDIO_SRTP_SEND_KEY		= "AUDIO_SRTP_SEND_KEY";
    public static final String KEY_AUDIO_SRTP_RECEIVE_KEY		= "AUDIO_SRTP_RECEIVE_KEY";
    public static final String KEY_ENALBE_VIDEO_SRTP		    = "ENALBE_VIDEO_SRTP";
    public static final String KEY_VIDEO_SRTP_SEND_KEY		= "VIDEO_SRTP_SEND_KEY";
    public static final String KEY_VIDEO_SRTP_RECEIVE_KEY		= "VIDEO_SRTP_RECEIVE_KEY";
    public static final String KEY_VIDEO_ENABLE_CAMERA_MUTE        = "VIDEO_ENABLE_CAMERA_MUTE";

    
    //UI Related
    protected static void startIncomingCallActivity(final Context context,final String callID,final  String remoteID, final boolean enableVideo)
    {
    	Log.d(DEBUGTAG, "startIncomingCallActivity(), callID:"+callID+", remoteID:"+remoteID);
        if (context!=null) {
            Intent intent = new Intent();
            intent.setAction(ACTION_INCOMING_CALL);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            // create bundle & Assign Bundle to Intent
            Bundle bundle = new Bundle();
            bundle.putString(KEY_CALL_ID  , callID);
            bundle.putString(KEY_REMOTE_ID, remoteID);
            bundle.putBoolean(KEY_ENABLE_VIDEO, enableVideo);
            if(remoteID.startsWith(QtalkSettings.MCUPrefix)){
            	bundle.putBoolean("KEY_PERFORM_ACCEPT", true);
            }else{
            	bundle.putBoolean("KEY_PERFORM_ACCEPT", false);
            }
            intent.putExtras(bundle);
            // start Activity Class
            context.startActivity(intent);
        }
    }
    protected static void startOutgoingCallActivity(final Context context,final String callID,final  String remoteID, final boolean enableVideo)
    {
    	Log.d(DEBUGTAG, "startOutgoingCallActivity(), callID:"+callID+", remoteID:"+remoteID);
        if (context!=null) { 
            Intent intent = new Intent();
            intent.setAction(ACTION_OUTGOING_CALL);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            // create bundle & Assign Bundle to Intent
            Bundle bundle = new Bundle();
            bundle.putString(KEY_CALL_ID  , callID);
            bundle.putString(KEY_REMOTE_ID, remoteID);
            bundle.putBoolean(KEY_ENABLE_VIDEO, enableVideo);
            intent.putExtras( bundle );
            // start Activity Class
            if(!(remoteID.startsWith(QtalkSettings.MCUPrefix))){  // if broadcast (ppt), shouldnt open outgoing
            	context.startActivity(intent);
            }
        }
    }
    protected static void startOutgoingCallFailActivity(final Context context,final int callState)
    {
    	Log.d(DEBUGTAG, "startOutgoingCallFailActivity(), callState:"+callState);
        if (context!=null) { 
            Intent intent = new Intent();
            intent.setAction(ACTION_OUTGOING_CALL_FAIL);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            // create bundle & Assign Bundle to Intent
            Bundle bundle = new Bundle();
            bundle.putInt(KEY_ERRORCODE  , callState);
            intent.putExtras( bundle );
            // start Activity Class
            context.startActivity(intent);
        }
    }
    protected static void startVideoSessionActivity(final Context context,final String callID, final int callType,final String remoteID,
                                                    final String audioDestIP,final int audioLocalPort,final int audioDestPort,final MDP audioMdp,
                                                    final String videoDestIP,final int videoLocalPort,final int videoDestPort,final MDP videoMdp, final int productType, final long startTime,
                                                    final int width, final int height, final int fps, final int kbps, final boolean enablePLI, final boolean enableFIR, final boolean enableTelEvt,
                                                    final boolean enableCameraMute)
    {
    	Log.d(DEBUGTAG, "startVideoSessionActivity(), callID:" +callID +
                                    ", remoteID:" +remoteID +
                                    ", remoteIP:" +audioDestIP +
                                    ", audioLocalPort:" +audioLocalPort +
                                    ", audioDestPort:" +audioDestPort +
                                    ", videoLocalPort:" +videoLocalPort +
                                    ", videoDestPort:" +videoDestPort +
                                    ", audioMdp:" +audioMdp+
                                    ", videoMdp:"+videoMdp+
                                    ", productType:"+productType+
                                    ", startTime:"+startTime+
                                    ", width:"+width+
                                    ", height:"+height+
                                    ", fps:"+fps+
                                    ", kbps:"+kbps+
                                    ", enablePLI:"+enablePLI+
                                    ", enableFIR:"+enableFIR+
                                    ", enableTelEvt:"+enableTelEvt+
                                    ", fecCtrl:"+videoMdp.FEC_info.FECCtrl + 
                                    ", fecSPType:"+videoMdp.FEC_info.FECSendPayloadType+
                                    ", fecRPType:" +videoMdp.FEC_info.FECRecvPayloadType+
                                    ", enableCameraMute:"+enableCameraMute);
        if (context!=null)
        {
            Intent intent = new Intent();
            intent.setAction(ACTION_VIDEO_SESSION);
			intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            //intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_CLEAR_TOP|Intent.FLAG_ACTIVITY_SINGLE_TOP);
            // create bundle & Assign Bundle to Intent
            Bundle bundle = new Bundle();
            bundle.putString(KEY_CALL_ID              , callID);
            bundle.putInt(KEY_CALL_TYPE               , callType);
            bundle.putString(KEY_REMOTE_ID            , remoteID);
            bundle.putBoolean(KEY_ENABLE_VIDEO        , true);
            bundle.putInt(KEY_PRODUCT_TYPE            , productType);
            bundle.putLong(KEY_SESSION_START_TIME     , startTime);
            bundle.putBoolean(KEY_ENABLE_TEL_EVENT    , enableTelEvt);
            if (audioMdp!=null)
            {
                bundle.putString(KEY_AUDIO_DEST_IP    , audioDestIP);
                bundle.putInt(KEY_AUDIO_LOCAL_PORT    , audioLocalPort);
                bundle.putInt(KEY_AUDIO_DEST_PORT     , audioDestPort);
                bundle.putInt(KEY_AUDIO_PAYLOAD_TYPE  , audioMdp.PayloadID);
                bundle.putInt(KEY_AUDIO_SAMPLE_RATE   , audioMdp.ClockRate);
                bundle.putInt(KEY_AUDIO_BITRATE       , audioMdp.BitRate);
                bundle.putString(KEY_AUDIO_MIME_TYPE  , audioMdp.MimeType);
                bundle.putInt(KEY_AUDIO_FEC_CTRL		, audioMdp.FEC_info.FECCtrl);
                bundle.putInt(KEY_AUDIO_FEC_SEND_PTYPE	, audioMdp.FEC_info.FECSendPayloadType);
                bundle.putInt(KEY_AUDIO_FEC_RECV_PTYPE	, audioMdp.FEC_info.FECRecvPayloadType);
                bundle.putBoolean(KEY_ENALBE_AUDIO_SRTP     , audioMdp.SRTP_info.enableSRTP);
                bundle.putString(KEY_AUDIO_SRTP_SEND_KEY    , audioMdp.SRTP_info.SRTPSendKey);
                bundle.putString(KEY_AUDIO_SRTP_RECEIVE_KEY , audioMdp.SRTP_info.SRTPReceiveKey);
            }
            if (videoMdp!=null)
            {
                bundle.putString(KEY_VIDEO_DEST_IP    , videoDestIP);
                bundle.putInt(KEY_VIDEO_LOCAL_PORT    , videoLocalPort);
                bundle.putInt(KEY_VIDEO_DEST_PORT     , videoDestPort);
                bundle.putInt(KEY_VIDEO_PAYLOAD_TYPE  , videoMdp.PayloadID);
                bundle.putInt(KEY_VIDEO_SAMPLE_RATE   , videoMdp.ClockRate);
                bundle.putString(KEY_VIDEO_MIME_TYPE  , videoMdp.MimeType);
                bundle.putInt(KEY_VIDEO_WIDTH         , width);
                bundle.putInt(KEY_VIDEO_HEIGHT        , height);
                bundle.putInt(KEY_VIDEO_FPS           , fps);
                bundle.putInt(KEY_VIDEO_KBPS          , kbps);
                bundle.putBoolean(KEY_ENABLE_PLI      , enablePLI);
                bundle.putBoolean(KEY_ENABLE_FIR      , enableFIR);
                bundle.putInt(KEY_VIDEO_FEC_CTRL		, videoMdp.FEC_info.FECCtrl);
                bundle.putInt(KEY_VIDEO_FEC_SEND_PTYPE	, videoMdp.FEC_info.FECSendPayloadType);
                bundle.putInt(KEY_VIDEO_FEC_RECV_PTYPE	, videoMdp.FEC_info.FECRecvPayloadType);
                
                bundle.putBoolean(KEY_ENALBE_VIDEO_SRTP     , videoMdp.SRTP_info.enableSRTP);
                bundle.putString(KEY_VIDEO_SRTP_SEND_KEY    , videoMdp.SRTP_info.SRTPSendKey);
                bundle.putString(KEY_VIDEO_SRTP_RECEIVE_KEY , videoMdp.SRTP_info.SRTPReceiveKey);
                bundle.putBoolean(KEY_VIDEO_ENABLE_CAMERA_MUTE	, enableCameraMute);
            }
            intent.putExtras(bundle);
            // start Activity Class
            context.startActivity(intent);
        }
    }
    protected static void startAudioSessionActivity(final Context context,final String callID,final int callType,final String remoteID,final String remoteIP,int audioLocalPort,int audioDestPort,final MDP audioMdp, final long startTime, final boolean enableTelEvt, final int productType)
    {
    	Log.d(DEBUGTAG, "startAudioSessionActivity(), callID:" +callID +
                                    ", remoteID:" +remoteID +
                                    ", remoteIP:" +remoteIP +
                                    ", audioLocalPort:" +audioLocalPort +
                                    ", audioDestPort:" +audioDestPort +
                                    ", audioMdp:" +audioMdp +
                                    ", startTime:"+startTime +
                                    ", fecCtrl:"+audioMdp.FEC_info.FECCtrl + 
                                    ", fecSPType:"+audioMdp.FEC_info.FECSendPayloadType+ 
                                    ", fecRPType" +audioMdp.FEC_info.FECRecvPayloadType);
        if (context!=null)
        {
            Intent intent = new Intent();
            if(productType==8){ // ptt_session
            	intent.setAction(ACTION_PTT_SESSION);
            }
            else{
            	intent.setAction(ACTION_AUDIO_SESSION);
            }
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|Intent.FLAG_ACTIVITY_SINGLE_TOP);
            // create bundle & Assign Bundle to Intent
            Bundle bundle = new Bundle();
            bundle.putString(KEY_CALL_ID            , callID);
            bundle.putInt(KEY_CALL_TYPE             , callType);
            bundle.putString(KEY_REMOTE_ID          , remoteID);
            bundle.putBoolean(KEY_ENABLE_VIDEO      , false);
            bundle.putLong(KEY_SESSION_START_TIME   , startTime);
            bundle.putBoolean(KEY_ENABLE_TEL_EVENT    , enableTelEvt);
            bundle.putInt(KEY_PRODUCT_TYPE			, productType);
            if (audioMdp!=null)
            {
                bundle.putString(KEY_AUDIO_DEST_IP  , remoteIP);
                bundle.putInt(KEY_AUDIO_LOCAL_PORT  , audioLocalPort);
                bundle.putInt(KEY_AUDIO_DEST_PORT   , audioDestPort);
                bundle.putInt(KEY_AUDIO_PAYLOAD_TYPE, audioMdp.PayloadID);
                bundle.putInt(KEY_AUDIO_SAMPLE_RATE , audioMdp.ClockRate);
                bundle.putInt(KEY_AUDIO_BITRATE     , audioMdp.BitRate);
                bundle.putString(KEY_AUDIO_MIME_TYPE, audioMdp.MimeType);
                bundle.putInt(KEY_AUDIO_FEC_CTRL		, audioMdp.FEC_info.FECCtrl);
                bundle.putInt(KEY_AUDIO_FEC_SEND_PTYPE	, audioMdp.FEC_info.FECSendPayloadType);
                bundle.putInt(KEY_AUDIO_FEC_RECV_PTYPE	, audioMdp.FEC_info.FECRecvPayloadType);
                bundle.putBoolean(KEY_ENALBE_AUDIO_SRTP,     audioMdp.SRTP_info.enableSRTP);
                bundle.putString(KEY_AUDIO_SRTP_SEND_KEY,    audioMdp.SRTP_info.SRTPSendKey);
                bundle.putString(KEY_AUDIO_SRTP_RECEIVE_KEY,    audioMdp.SRTP_info.SRTPReceiveKey);
            }
            intent.putExtras(bundle);
            // start Activity Class
            context.startActivity(intent);
        }
    }
    public static Intent createCallLogIntent() 
    {
        Intent intent = new Intent(ACTION_HISTORY_RESUME, null);
        //intent.setType("vnd.android.cursor.dir/calls");
        return intent;
    }
    //============================================================================
	private static final Hashtable<String,Activity>    mActivityTable = new Hashtable<String, Activity>();
    public static void registerActivity(final String callID,final  Activity activity)
    {
    	Log.d(DEBUGTAG, "registerActivity(), callID:"+callID +", activity:"+activity);
    	if(callID == null)
    		return;
        Activity old = null;
        synchronized(mActivityTable)
        {
            old = mActivityTable.put(callID, activity);
        }
        if (old!=null){
        	old.finish();
        }
        if (mEngine!=null && !mEngine.isCallExisted(callID))
        {
            activity.finish();
        }
    }
    public static void unregisterActivity(final String callID,final Activity activity)
    {
    	Log.d(DEBUGTAG, "unregisterActivity(), callID:" +callID +", activity:"+activity);
        synchronized(mActivityTable)
        {
            if (callID!=null && mActivityTable.contains(activity))
                mActivityTable.remove(callID);
        }
    }

    public static void onSessionEnd(final String callID)
    {
        Activity old = null;
        Log.d(DEBUGTAG, "onSessionEnd(), callID:" +callID);
        synchronized(mActivityTable)
        {
            old = mActivityTable.remove(callID);
        }
        if (old!=null)
        {
        	//old.finish();
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "old.finish:"+old );
        }
    }
    
    public static void onMCUSessionEnd(final String callID)
    {
        Activity old = null;
        if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "onMCUSessionEnd: callID:" +callID);
        synchronized(mActivityTable)
        {
            old = mActivityTable.remove(callID);
        }
        if (old!=null)
        {
        	//old.finish();
            if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG, "old.finish:"+old );
        }
    }
    
    public static void showRetryActivity(final Context context,final String msg)
    {
        //Destroy other viro activity
        Log.d(DEBUGTAG,"showRetryActivity");
        Flag.SIPRunnable.setState(false);
        Activity activity = ViroActivityManager.getLastActivity();
        if (activity!=null)
        {
            activity.finish();
        }
        //Show login retry activity
        if(ViroActivityManager.isViroForeGround(context))
        {
        	context.stopService(new Intent(context,QTService.class));
        	Intent myintent = new Intent();
        	myintent.setAction(ACTION_LOGIN_RETRY);
        	myintent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        	myintent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        	myintent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
            Bundle bundle = new Bundle();
            bundle.putString(ProvisionSetupActivity.MESSAGE, msg);
            myintent.putExtras(bundle);
            //-------------------------------------------------[PT]
            if(QtalkSettings.Login_by_launcher == false)
            context.startActivity(myintent);
            
        }
        else
        	if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG,"showRetryActivity isViroForeGround = false");
    }
	public static final String QTSERVICE_NAME = "com.quanta.qtalk.ui.QTService";
    public static void StartQTService(Context context) {

        if (!(QtalkEngine.isServiceExisted(context, QTSERVICE_NAME))) {
            Log.d(DEBUGTAG, "QTService is not exist, startQTService ");
            context.startForegroundService(new Intent(context, QTService.class));
        } else {
            Log.d(DEBUGTAG, "QTService is exist but start again");
            context.startForegroundService(new Intent(context, QTService.class));
        }
    }
}
