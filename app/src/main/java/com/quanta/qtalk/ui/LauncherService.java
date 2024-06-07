package com.quanta.qtalk.ui;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;

import com.quanta.qtalk.QtalkAction;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.R;


public class LauncherService extends Service
{
	private static LauncherReceiver mReceiver = null;
	private static String DEBUGTAG = "QTFa_LauncherService";
    public static final String ACTION_VC_LOGIN ="android.intent.action.vc.login";
	public final static String ACTION_VC_MESSAGE = "android.intent.action.vc.message";
    public static final String ACTION_VC_GET_LOGIN_DATA ="android.intent.action.vc.get.login.data";
    public static final String ACTION_VC_RETURN_LOGIN_DATA ="android.intent.action.vc.return.login.data";    
    public static final String ACTION_VC_MAKE_CALL = "android.intent.action.vc.makecall";
    public static final String ACTION_VC_MAKE_CALL_CB = "android.intent.action.vc.makecall.cb";
    public static final String ACTION_VC_NURSE_CALL = "android.intent.action.vc.nurse.call";
    public static final String ACTION_VC_LOGOUT = "android.intent.action.vc.logout";
    public static final String ACTION_VC_TTS_LIST = "android.intent.action.vc.tts.list";
    public static final String ACTION_VC_TTS = "android.intent.action.vc.tts";
    public static final String ACTION_VC_TTS_CB = "android.intent.action.vc.tts.cb";
    public static final String ACTION_VC_OPEN_CONTACT = "android.intent.action.vc.contacts";
    public static final String ACTION_VC_OPEN_HISTORY = "android.intent.action.vc.history";
    public static final String ACTION_VC_LOGON_STATE = "android.intent.action.vc.logon.state";
    public static final String ACTION_VC_LOGON_STATE_CB = "android.intent.action.vc.logon.state.cb";
	public static final String ACTION_VC_ALARM = "android.intent.action.vc.alarm";
    public static final String ACTION_GET_TTS = "android.intent.action.ptms.tts";
    public static final String PHONE_NEW_NOTIFICATION = "android.intent.action.qlauncher.notification.count";
    public static final String PHONE_QUERY_NOTIFICATION = "android.intent.action.qlauncher.notification.query";
    public static final String HISTORY_TTS_FROM_LAUNCHER = "android.intent.action.tts.broadcast.msg";
    public static final String HANDSET_PICKUP = "com.quanta.qspt.handset.pickup";
    public static final String HANDSET_HANGUP = "com.quanta.qspt.handset.hangup";
    public static final String PTA_OP_GET = "pta.rq.1";
    public static final String PTA_OP_GET_RC = "pta.rc.1";
	public final static String PTA_RESOURCE_CHANGE     = "pta.resource.change";
    
    
	@Override
	public void onCreate() {
	// TODO Auto-generated method stub
		super.onCreate();
		Hack.init(this);
		PermissionRequestActivity.requestPermission(this);
		Hack.getStoragePath();
		Hack.checkLogFolder(this);
		new Thread (new Runnable() {
			@Override
			public void run() {
				// TODO Auto-generated method stub
				Log.stop();
			}
		}).start();
		
		Log.d(DEBUGTAG,"==> onCreate process id:"+android.os.Process.myPid());
		//startRecordingLogs();
		if (mReceiver == null) {
			IntentFilter intentfilter = new IntentFilter();
			intentfilter.addAction(ACTION_VC_LOGIN);
			intentfilter.addAction(ACTION_VC_GET_LOGIN_DATA);
			intentfilter.addAction(ACTION_VC_RETURN_LOGIN_DATA);			
			intentfilter.addAction(ACTION_VC_NURSE_CALL);
			intentfilter.addAction(ACTION_VC_MESSAGE);
			intentfilter.addAction(Intent.ACTION_BOOT_COMPLETED);
			intentfilter.addAction(Intent.ACTION_USER_PRESENT);
			intentfilter.addAction(Intent.ACTION_SCREEN_ON);
			intentfilter.addAction(Intent.ACTION_SCREEN_OFF);
			intentfilter.addAction(Intent.ACTION_USER_PRESENT);
			intentfilter.addAction(QtalkAction.ACTION_APP_START);
			intentfilter.addAction("android.intent.action.vc.login.cb");
			intentfilter.addAction("android.intent.action.vc.logout.cb.login.after");
			intentfilter.addAction("android.intent.action.vc.logout.cb");
			intentfilter.addAction(ACTION_VC_MAKE_CALL);
			intentfilter.addAction(ACTION_VC_LOGOUT);
			intentfilter.addAction(ACTION_VC_TTS_LIST);
			intentfilter.addAction(ACTION_VC_OPEN_CONTACT);
			intentfilter.addAction(ACTION_VC_OPEN_HISTORY);
			intentfilter.addAction(ACTION_VC_LOGON_STATE);
			intentfilter.addAction(ACTION_GET_TTS);
			intentfilter.addAction(HISTORY_TTS_FROM_LAUNCHER);
			intentfilter.addAction(HANDSET_PICKUP);
			intentfilter.addAction(HANDSET_HANGUP);
			intentfilter.addAction(PTA_OP_GET_RC);
			intentfilter.addAction(PTA_RESOURCE_CHANGE);
			intentfilter.addAction(ACTION_VC_ALARM);
			
			registerReceiver(mReceiver = new LauncherReceiver(), intentfilter);
			Intent loginintent=new Intent();
			loginintent.setPackage("com.quanta.qoca.spt.nursingapp");
			loginintent.setAction(LauncherService.ACTION_VC_GET_LOGIN_DATA);
			this.sendBroadcast(loginintent);
			Log.d(DEBUGTAG,"ACTION_VC_GET_LOGIN_DATA");
			Hack.set_alarm(this, LauncherReceiver.class, LauncherService.ACTION_VC_ALARM, 60);

//			Log.d(DEBUGTAG,"<== start receiver");
		}
//		Log.d(DEBUGTAG,"<== service onCreate");
	}
	@Override
	public IBinder onBind(Intent intent) {
		// TODO Auto-generated method stub
		return null;
	}

	private void startForegroundNotification(){
		Log.d(DEBUGTAG, "start foreground service");

		String CHANNEL_ID = "PhoneApp";
		NotificationChannel channel = new NotificationChannel(CHANNEL_ID,
				CHANNEL_ID, NotificationManager.IMPORTANCE_DEFAULT);
		channel.setLockscreenVisibility(Notification.VISIBILITY_PRIVATE);
		channel.setSound(null, null);
		((NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE)).createNotificationChannel(channel);
		Notification notification = new Notification.Builder(this, CHANNEL_ID)
				.setWhen(System.currentTimeMillis())
				.setSmallIcon(R.drawable.qspt_app)
				.setContentTitle("LauncherService")
				.setGroup("qtalk_server")
				.setPriority(Notification.PRIORITY_LOW)
				.setContentText("").build();
		startForeground(1111, notification);
	}


	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		// TODO Auto-generated method stub
		super.onStartCommand(intent, flags | Service.START_FLAG_REDELIVERY, startId);
		startForegroundNotification();
		return START_STICKY;
	}
	public void onDestroy() {
		Log.d(DEBUGTAG,"==> onDestroy");
		//stopRecordingLogs();
		if(mReceiver != null) {
			unregisterReceiver(mReceiver);
			mReceiver = null;
		}
	};
	/*
	ReadThread thread = null;
	

	public void startRecordingLogs() {
		if (thread == null || !thread.isAlive()) {
			thread = new ReadThread();
			thread.start();
		}
	}

	public String stopRecordingLogs() {
		String results = thread.stopLogging();
		thread = null;
		return results;
	}
	
	private class ReadThread extends Thread {

		String results;
		Process process;
		Object lockObject = new Object();
		StringBuilder log;
		String path = "/sdcard" + java.io.File.separatorChar +"QSPT_VCLOG"+java.io.File.separatorChar+ "VC_Log1";

		public void run() {
			Log.d(DEBUGTAG,"==> ReadThread:"+this.hashCode());
	    	File dirFile = new File("/sdcard/QSPT_VCLOG");
	    	if (!dirFile.exists())
	    	{
	    		dirFile.mkdir();
	    	}
			
			
			File logFile = new File(path);
			FileOutputStream fos=null;
			try {
				if(logFile.length()>0){
					fos = new FileOutputStream(logFile,true);
				}else{
					fos = new FileOutputStream(logFile);
				}
			} catch (FileNotFoundException e1) {
				Log.d(DEBUGTAG,"open vc log file error");
				Log.e(DEBUGTAG,"",e1);
			}
			
			

			synchronized (lockObject) {
				try {
					process = Runtime.getRuntime().exec("logcat -v time");

					BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));

					String line;
					String separator = System.getProperty("line.separator");

					while ((line = reader.readLine()) != null) {
						fos.write(line.getBytes());
						fos.write(separator.getBytes());
						if (logFile.length() > 5000000) {
							Runtime.getRuntime().exec("cp /sdcard/QSPT_VCLOG/VC_Log1 /sdcard/QSPT_VCLOG/VC_Log2");
							Thread.sleep(2000);
							fos = new FileOutputStream(logFile);
						}
					}
					reader.close();
				} catch (IOException e) {
					Log.d(DEBUGTAG,"write log error");
					Log.e(DEBUGTAG,"",e);
				} catch (InterruptedException e) {
					Log.e(DEBUGTAG,"",e);
				}
			}

			Log.d(DEBUGTAG,"<== ReadThread:"+this.hashCode());
		}

		public String stopLogging() {
			process.destroy();
			synchronized (lockObject) {
				return results;
			}
		}
	}*/

}
