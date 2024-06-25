package com.quanta.qtalk.ui;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;

import com.quanta.qtalk.FailedOperateException;
import com.quanta.qtalk.Flag;
import com.quanta.qtalk.IQtalkEngineListener;
import com.quanta.qtalk.QtalkAction;
import com.quanta.qtalk.QtalkDB;
import com.quanta.qtalk.QtalkEngine;
import com.quanta.qtalk.QtalkSettings;
import com.quanta.qtalk.call.CallState;
import com.quanta.qtalk.call.MDP;
import com.quanta.qtalk.provision.DrawableTransfer;
import com.quanta.qtalk.provision.ProvisionUtility;
import com.quanta.qtalk.ui.contact.ContactManager;
import com.quanta.qtalk.ui.contact.ContactQT;
import com.quanta.qtalk.ui.contact.QTContact2;
import com.quanta.qtalk.ui.history.HistoryManager;
import com.quanta.qtalk.util.Hack;
import com.quanta.qtalk.util.Log;
import com.quanta.qtalk.util.PtmsData;
import com.quanta.qtalk.R;

import org.xml.sax.SAXException;

import java.io.File;
import java.io.IOException;
import java.net.ConnectException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;

import javax.xml.parsers.ParserConfigurationException;

public class QTService extends Service implements IQtalkEngineListener {
	private static final String DEBUGTAG = "QSE_QTService";
    public static final String ACTION_PROVISION_LOGIN   = "com.quanta.qtalk.ui.provision.login";
    public static final String ACTION_GET_TTS = "android.intent.action.ptms.tts";
	public static final String ACTION_ALARM = "android.intent.action.alarm";
	public static final String ACTION_PHONEBOOK = "android.intent.action.phonebook";
	public static final String ACTION_RESTART_VIDEO_SESSION = "android.intent.action.restart.video.session";
	private QTReceiver mReceiver = null;
	private QtalkEngine mEngine = null;
	private boolean mLogon = false;
	private static String session = null;
	private String key = null;
	private String provisionID = null;
	private String phonebook_uri = null;
	private String provisionType = null;
	private String provisionServer = null;
	private static String lastPhonebookFileTime = null;
	protected int phonebook_time = 5000;
	private String server = null;
	private static final int DEACTIVATE = 6;
	private static final int AvatarUpdate = 2;
	private static final int REPROVISION = 1;
	private static final int AVATAR_FAIL = 8;
	private static final int AVATAR_SUCCESS = 7;
	private static Activity LastActivity = null;
	private static Context mcontext = null;
	private int size = 0;
	private PhonebookCheckThread mPhonebookCheckThread = null;
	public static int phonebookhashcode = 0;
	private static final Object mLock = new Object();
	private static long lastStartTime = 0;
	private void acquireWakeLock() {
		/*
		if (null == mWakeLock) {
			PowerManager pm = (PowerManager) this
					.getSystemService(Context.POWER_SERVICE);
			mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "qtalk:QTService");
		}
		if ((null != mWakeLock) && (!mWakeLock.isHeld()) ) {
			mWakeLock.acquire();
			Log.d(DEBUGTAG,"acquireWakeLock");
		}*/
	}




	private String PTTDelayAccept_callID = null;
	private String PTTDelayAccept_remoteID = null;
	private boolean PTTDelayAccept_enableVideo = false;

	final Runnable DelayAcceptThread = new Runnable() {
	        public void run() {
	        	if(PTTDelayAccept_callID!=null&&PTTDelayAccept_remoteID!=null)
					try {
						/*QTReceiver.mEngine.acceptCall(PTTDelayAccept_callID,
													  PTTDelayAccept_remoteID,
													  PTTDelayAccept_enableVideo);*/
						QTReceiver.engine(QTService.this, false).acceptCall(PTTDelayAccept_callID,
								  PTTDelayAccept_remoteID,
								  PTTDelayAccept_enableVideo,
								  false);
						//QTReceiver.engine(QTService.this, false).hangupCall(PTTDelayAccept_callID, PTTDelayAccept_remoteID, false);
					} catch (FailedOperateException e) {
						// TODO Auto-generated catch block
						Log.e(DEBUGTAG,"",e);
					}
	        }
	    };
	// 釋放設備電源鎖
	private void releaseWakeLock() {
		/*
		if ((null != mWakeLock) && mWakeLock.isHeld()) {
			mWakeLock.release();
			Log.d(DEBUGTAG,"releaseWakeLock");
		}
		*/
	}

	private void freeWakeLock() {
		/*
		releaseWakeLock();
		mWakeLock = null;
		Log.d(DEBUGTAG,"freeWakeLock");
		*/
	}

	class PhonebookCheckThread extends Thread
    {
		@Override
		public void run() {
			phonebookhashcode = this.hashCode();
			while(true) {

				try {
					Hack.set_alarm(QTService.this, QTReceiver.class, QTService.ACTION_ALARM, phonebook_time/1000);
					Thread.sleep(phonebook_time);
					break;
				} catch (InterruptedException e) {
					Log.e(DEBUGTAG,"",e);
				}
				/*
				if(phonebookhashcode == this.hashCode()){
					try {
						releaseWakeLock();
						phonebookcheck();
						acquireWakeLock();
					} catch (Exception e) {
						Log.e(DEBUGTAG,"",e);
					}
				}
				else{
					break;
				}*/
			}
		}

    }
	private static final Handler mHandler = new Handler(Looper.getMainLooper())
    {
        public void handleMessage(Message msg)
        {
          switch(msg.what)
          {
	          case DEACTIVATE:
	        	  Log.d(DEBUGTAG,"Receive deactivate, recv session:"+msg.getData().getString("session")+",session:"+session);;
		          if(msg.getData().getString("session").equals(session)){
			          phonebookhashcode = 0;
		        	  Intent deactivation = new Intent(mcontext,CommandService.class);
					  Bundle bundle = new Bundle();
					  bundle.putInt("KEY_COMMAND", 1);
					  bundle.putString("KEY_LOGIN_AFTER", "NO");
					  deactivation.putExtras(bundle);
					  if(mcontext!=null)
						  mcontext.startService(deactivation);
		          }
	       		  break;
	          case AvatarUpdate:
	        	  //Log.d(DEBUGTAG," It's need to update phonebook!!");
	        	  lastPhonebookFileTime = msg.getData().getString("timestamp");
	        	  String phonebook_url = Hack.getStoragePath()+"phonebook.xml";
	        	  ArrayList<QTContact2> temp_list = null;
	        	  Flag.Phonebook.setState(true);
	        	  try {
	        		  	temp_list = QtalkDB.parseProvisionPhonebook(phonebook_url);
					} catch (ParserConfigurationException e) {
						// TODO Auto-generated catch block
						Log.e(DEBUGTAG,"",e);
					} catch (SAXException e) {
						// TODO Auto-generated catch block
						Log.e(DEBUGTAG,"",e);
					} catch (IOException e) {
						// TODO Auto-generated catch block
						Log.e(DEBUGTAG,"",e);
					}
	        	  DrawableTransfer.DeletePhonebookCornerPhoto(temp_list);
	        	  break;
	          case REPROVISION:
            	  //Log.d(DEBUGTAG," It's going to provision!!");
            	  boolean state = Flag.InsessionFlag.getState();
            	  if (state == false)
            	  {
            		  if(ProvisionSetupActivity.debugMode) Log.d(DEBUGTAG," It can provision now!!");
            		  Flag.Reprovision.setState(true);
            		  Intent reprovision = new Intent();
            		  reprovision.setAction(ACTION_PROVISION_LOGIN);
            		  reprovision.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            		  reprovision.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
            		  reprovision.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
            		  if(mcontext!=null){
            			  mcontext.startActivity(reprovision);
            			  mcontext.stopService(new Intent(mcontext,QTService.class));
            		  }
	        		  LastActivity = ViroActivityManager.getLastActivity();
	        		  if (LastActivity != null)
	        			  LastActivity.finish();

            	  }
                  break;
	          case AVATAR_FAIL:
	          		Flag.AvatarSuccess.setState(false);
	          		break;
	          case AVATAR_SUCCESS:
	          		Flag.AvatarSuccess.setState(true);
	          		break;
          }
        }
    };

	@SuppressLint("NewApi")
	@SuppressWarnings("deprecation")
	@Override
	public void onStart(Intent intent, int startId) {
		super.onStart(intent, startId);
		Log.d(DEBUGTAG,"onStart(" + startId+"), enter");

		mcontext = this;
		/*
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
			String packageName = getPackageName();
			Log.d(DEBUGTAG, "Build.VERSION.SDK_INT = " + Build.VERSION.SDK_INT + "Build.VERSION_CODES.M="
					+ Build.VERSION_CODES.M);
			PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
			if (!pm.isIgnoringBatteryOptimizations(packageName)) {
				Intent bintent = new Intent();
				bintent.setAction(android.provider.Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS);
				bintent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				bintent.setData(Uri.parse("package:" + packageName));
				startActivity(bintent);
			}
		}*/
		acquireWakeLock();
		long nowStartTime = System.currentTimeMillis();
		if(nowStartTime-lastStartTime > 1000) {
			lastStartTime = nowStartTime;
			if (mEngine == null) {
				mEngine = QTReceiver.engine(this, false);
			}

			if (mEngine != null) {
				mEngine.setListener(this);
				mHandler.post(new Runnable() {
					public void run() {
						synchronized (mLock) {
							try {
								if (mEngine != null) {
									if (mEngine.isLogon())
										mEngine.keepAlive(QTService.this);
									else
										mEngine.login(QTService.this);
								}
							} catch (FailedOperateException e) {
								LogonStateReceiver.notifyLogonState(QTService.this, false,
										                    mEngine != null ? mEngine.getLogonName() : null,
		                                                    getString(R.string.busy_network)+"\r\n"+getString(R.string.retry_msg)+"\r\n");
								Log.e(DEBUGTAG, "onStart", e);
							}
						}
					}
				});
			}
		}
		else {
			Log.d(DEBUGTAG,"skip this start becuz open time too close");
		}
		Log.d(DEBUGTAG,"onStart(" + startId + "), exit");
	}

	private void startForegroundNotification(){
		Log.d(DEBUGTAG, "start foreground service");

		Notification notification = LogonStateReceiver.buildNotification(this, 0, "", "");
		startForeground(QTReceiver.NOTIFICATION_ID_LOGON, notification);
	}
	
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		// TODO Auto-generated method stub
		super.onStartCommand(intent, flags | Service.START_FLAG_REDELIVERY, startId);
		startForegroundNotification();
		Log.d(DEBUGTAG,"onStartCommand");
		return START_REDELIVER_INTENT;
	}

	final Runnable PhonebookrunnableThread = new Runnable() {
		public void run() {
			if (Hack.isPhone() && (!Hack.isScreenOn(QTService.this))) {
				mHandler.postDelayed(this, phonebook_time);
				Log.d(DEBUGTAG, "PhonebookrunnableThread(), skip");
				return;
			}
			Log.d(DEBUGTAG, "PhonebookrunnableThread(), enter");

			int size1=0;
			QThProvisionUtility qtnMessenger = new QThProvisionUtility(QTService.this, mHandler, null);
			QtalkDB qtalkdb = QtalkDB.getInstance(QTService.this);
			
			if (session == null || key == null || lastPhonebookFileTime == null) {
				Cursor cs = qtalkdb.returnAllATEntry();
				size = cs.getCount();
//				if (ProvisionSetupActivity.debugMode)
//					Log.d(DEBUGTAG, "AT_DB size=" + size);
				cs.moveToFirst();
				for (int cscnt = 0; cscnt < cs.getCount(); cscnt++) {
					session = QtalkDB.getString(cs, "Session");
					key = QtalkDB.getString(cs, "Key");
					provisionID = QtalkDB.getString(cs, "Uid");
					provisionType = QtalkDB.getString(cs, "Type");
					provisionServer = QtalkDB.getString(cs, "Server");
					cs.moveToNext();
				}
				cs.close();
				cs = null;

				Cursor cs1 = qtalkdb.returnAllPTEntry();
				size1 = cs1.getCount();
//				if (ProvisionSetupActivity.debugMode)
//					Log.d(DEBUGTAG, "PT_DB size=" + size1);
				cs1.moveToFirst();
				for (int cscnt = 0; cscnt < cs1.getCount(); cscnt++) {
					lastPhonebookFileTime = QtalkDB.getString(cs1, "lastPhonebookFileTime");
					cs1.moveToNext();
				}
				cs1.close();
				cs1 = null;
			}

			if(size > 0)
			{
				if(QThProvisionUtility.provision_info !=null)
				{
					Log.d(DEBUGTAG, "lastPhonebookFileTime=" + lastPhonebookFileTime);
					phonebook_uri = QThProvisionUtility.provision_info.phonebook_uri
							+ provisionType
							+ "&service=phonebook&action=check&uid="
							+ provisionID
							+ "&session="
							+ session
							+ "&timestamp="
							+ lastPhonebookFileTime;
					try {
						qtnMessenger.httpsConnect(phonebook_uri, session,
												  lastPhonebookFileTime, provisionID,
												  provisionType,
												  provisionServer, key);
						boolean isNotifyLogonStateNative = LogonStateReceiver.isNotifyLogonStateNative();
						boolean SIPRunnablegetState = Flag.SIPRunnable.getState();
						Log.e(DEBUGTAG, "1 isNotifyLogonStateNative="+isNotifyLogonStateNative+", SIPRunnablegetState="+SIPRunnablegetState);
						if(!isNotifyLogonStateNative && !SIPRunnablegetState)
						{
							Flag.SIPRunnable.setState(true);
							LogonStateReceiver.notifyLogonState(QTService.this, true, null, "server is reachable", false);
						}
					} catch(ConnectException connex){
						Log.e(DEBUGTAG, "ConnectException", connex);
						if(Flag.SIPRunnable.getState()) {
							Flag.SIPRunnable.setState(false);
							LogonStateReceiver.notifyLogonState(QTService.this, false, null, "server is unreachable", false);
						}
					} catch (Exception e) {
						Log.e(DEBUGTAG, "PhonebookrunnableThread", e);
					}
				}
				else {
					String provision_file = Hack.getStoragePath()+"provision.xml";
					try {
						QThProvisionUtility.provision_info = ProvisionUtility.parseProvisionConfig(provision_file);
					} catch (ParserConfigurationException e) {
						Log.e(DEBUGTAG,"",e);
					} catch (SAXException e) {
						Log.e(DEBUGTAG,"",e);
					} catch (IOException e) {
						Log.e(DEBUGTAG,"",e);
					}
				}
			}

			if(Flag.AvatarSuccess.getState()==false)
			{
				if (QThProvisionUtility.provision_info != null)
	        	{
	        		server = QThProvisionUtility.provision_info.phonebook_uri;
	        		String UploadFile = null;

	        		File file = new File(AvatarImagePickActivity.path);
	        		if(file.exists())
	        			UploadFile = AvatarImagePickActivity.path;
	        		else
	        			UploadFile = Hack.getStoragePath()+"all_icn_member.png";

	        		try {
	 		        	qtnMessenger.doFileUpload(UploadFile ,provisionID, provisionType, session, server);
	 		        }
	 		        catch (Exception e) {
	 		        	Log.d(DEBUGTAG,e.getMessage());
	 		        	Message msg = new Message();
	 		        	msg.what = AVATAR_FAIL;
	 		        }

	        	}
			}
			mHandler.postDelayed(this, phonebook_time);
			qtalkdb.close();
			qtalkdb = null;
			Log.d(DEBUGTAG,"PhonebookrunnableThread(), exit");
		}
	};

	public static void outgoingcallfail(){
		QtalkSettings.nurseCall = false;
        Intent intent = new Intent();
        intent.setAction(QTReceiver.ACTION_OUTGOING_CALL_FAIL);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        // create bundle & Assign Bundle to Intent
        Bundle bundle = new Bundle();
        bundle.putInt("KEY_ERRORCODE"  , CallState.MAKE_CALL_WITHOUT_LOGIN);
        intent.putExtras( bundle );
        // start Activity Class
        if(mcontext!=null)
        	mcontext.startActivity(intent);
	}

	@Override
	public void onCreate() {
		super.onCreate();
		PermissionRequestActivity.requestPermission(this);
		if (ProvisionSetupActivity.debugMode)
			Log.d(DEBUGTAG, "onCreate QTService");
		//check provision period
		if (QThProvisionUtility.provision_info != null)
			phonebook_time = Integer.valueOf(QThProvisionUtility.provision_info.short_period_check_timer)*1000;
		//if (phonebook_time <= 30000)
			//phonebook_time = 30000;
		//mHandler.postDelayed(PhonebookrunnableThread, phonebook_time);
		mPhonebookCheckThread = new PhonebookCheckThread();
		//if(!(mPhonebookCheckThread.isAlive())){
			mPhonebookCheckThread.start();
		//}
		//mHandler.postDelayed(PhonebookrunnableThread, 5000000);
		if (QTReceiver.mContext == null)
			QTReceiver.mContext = this;
		if (mReceiver == null) {
			IntentFilter intentfilter = new IntentFilter();
			intentfilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
			intentfilter.addAction(QtalkAction.ACTION_PHONE_STATE_CHANGED);
			intentfilter.addAction(Intent.ACTION_HEADSET_PLUG);
			//intentfilter.addAction(WifiManager.WIFI_STATE_CHANGED_ACTION);
			intentfilter.addAction(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
			intentfilter.addAction(ACTION_ALARM);
			intentfilter.addAction(ACTION_PHONEBOOK);
			intentfilter.addAction(ACTION_RESTART_VIDEO_SESSION);
			// intentfilter.addAction(Receiver.ACTION_DOCK_EVENT);
			// intentfilter.addAction(Receiver.ACTION_DATA_STATE_CHANGED);
			// intentfilter.addAction(Intent.ACTION_USER_PRESENT);
			// intentfilter.addAction(Intent.ACTION_SCREEN_OFF);
			// intentfilter.addAction(Intent.ACTION_SCREEN_ON);
			// intentfilter.addAction(Receiver.ACTION_VPN_CONNECTIVITY);
			// intentfilter.addAction(Receiver.ACTION_SCO_AUDIO_STATE_CHANGED);
			// intentfilter.addAction(Intent.ACTION_TIME_TICK);
			registerReceiver(mReceiver = new QTReceiver(), intentfilter);
		}
	}

	@Override
	public void onDestroy() {
		if (ProvisionSetupActivity.debugMode)
			Log.d(DEBUGTAG, "onDestroy QTService "+this.hashCode());
		phonebookhashcode = 0;
		QTReceiver.alarm(0, QTRAAlarm.class);
		if (mEngine != null) {
			mEngine.logout("");
			mEngine = null;
		}
		if (mReceiver != null) {
			unregisterReceiver(mReceiver);
			mReceiver.setQTService(null);
			mReceiver = null;
		}
		//mHandler.removeCallbacks(PhonebookrunnableThread);

		freeWakeLock();
		mPhonebookCheckThread.interrupt();
		Hack.cancel_alarm(this);
		super.onDestroy();

		/*
		 * if (Flag.Activation.getState() == false) { Log.d(TAG,
		 * "onDestroy QTService, app is deactivate!"); System.exit(1); }
		 */
	}

	@Override
	public IBinder onBind(Intent arg0) {
		if (ProvisionSetupActivity.debugMode)
			Log.d(DEBUGTAG, "onBind");
		return null;
	}

	// ================================================================
	int mKeepAliveInterval = 0;

	@Override
	public void onLogonStateChange(final boolean state, final String msg,
			final int expireTime) {
		if (ProvisionSetupActivity.debugMode)
			Log.d(DEBUGTAG, "==>onLogonStateChange:state=" + state);
		if (!state || mLogon != state) {
			Log.d(DEBUGTAG,"LogonStateReceiver.notifyLogonState");
			mLogon = state;
			LogonStateReceiver.notifyLogonState(QTService.this, state,
					mEngine != null ? mEngine.getLogonName() : null, msg);
		}
		if (state)
			mKeepAliveInterval = expireTime * 9 / 10;

		if (mKeepAliveInterval == 0) {
			mKeepAliveInterval = 60;
		}
		//QTReceiver.alarm(mKeepAliveInterval, QTRAAlarm.class);
		if (!state) {
			if (Flag.Reprovision.getState() == false) {
				if (ProvisionSetupActivity.debugMode)
					Log.d(DEBUGTAG, "==>msg=" + msg);
				QTReceiver.showRetryActivity(this, getString(R.string.busy_network)+"\r\n"+getString(R.string.retry_msg)+"\r\n");
			}
		}
		if (ProvisionSetupActivity.debugMode)
			Log.d(DEBUGTAG, "<==onLogonStateChange:expireTime=" + expireTime);
	}

	@Override
	public void onNewIncomingCall(final String callID, final String remoteID, final boolean enableVideo) {
		if (ProvisionSetupActivity.debugMode)
			Log.d(DEBUGTAG, "onNewIncomingCall:" + callID);
		if(QtalkSettings.nurseCall){ //when seq-call, will auto reject any incoming call
			try {
				Log.d(DEBUGTAG,"onNewIncomingCall reject, during the nursecall ");
				QTReceiver.mEngine.hangupCall(callID, remoteID, false);
			} catch (FailedOperateException e) {
				// TODO Auto-generated catch block
				Log.e(DEBUGTAG,"",e);
			}
		}
		else {
			QTReceiver.startIncomingCallActivity(this, callID, remoteID, enableVideo);
		}
	}

	@Override
	public void onNewMCUIncomingCall(final String callID, final String remoteID,
			final boolean enableVideo) {
		LastActivity = ViroActivityManager.getLastActivity();
		try {
			QTReceiver.mEngine.acceptCall(callID, remoteID, enableVideo, false);
		} catch (FailedOperateException e) {
			// TODO Auto-generated catch block
			Log.e(DEBUGTAG,"",e);
		}
		if (LastActivity != null)
			  LastActivity.finish();
	}

	@Override
	public void onNewPTTIncomingCall(final String callID, final String remoteID,
			final boolean enableVideo) {
		if (ProvisionSetupActivity.debugMode)
			Log.d(DEBUGTAG, "onNewPTTIncomingCall:" + callID + " remoteID:"+remoteID+" enableVideo:"+enableVideo);
			PTTDelayAccept_callID = callID;
			PTTDelayAccept_remoteID = remoteID;
			PTTDelayAccept_enableVideo = enableVideo;
			mHandler.postDelayed(DelayAcceptThread, 1000);


		if (LastActivity != null)
			  LastActivity.finish();

	}


	@Override
	public void onNewOutGoingCall(String callID, String remoteID, int callState, final boolean enableVideo) {

		Log.d(DEBUGTAG, "onNewOutGoingCall:" + callID + ", callState:"+ callState);

		if(callState >= 0) {
			Flag.SIPRunnable.setState(true);
			QTReceiver.startOutgoingCallActivity(this, callID, remoteID,enableVideo);
		}
		else {
			if((callState != CallState.MAKE_CALL_DURING_SESSION) &&
			   (callState != CallState.MAKE_CALL_DURING_INVITE)) {
				Flag.SIPRunnable.setState(false);
				QTReceiver.startOutgoingCallFailActivity(this, callState);
			}
			else {
				Log.d(DEBUGTAG,"outgoing fail");
				if(QtalkSettings.nurseCall)
					CommandService.shouldNextNurseCall = true;
			}
		}
	}


	@Override
	public void onNewVideoSession(final String callID, final int callType,
			final String remoteID, final String audioDestIP,
			final int audioLocalPort, final int audioDestPort, final MDP audio,
			final String videoDestIP, final int videoLocalPort,
			final int videoDestPort, final MDP video, final int productType,
			final long startTime, final int width, final int height,
			final int fps, final int kbps, final boolean enablePLI,
			final boolean enableFIR, final boolean enableTelEvt,
			final boolean enableCameraMute) {
		if (ProvisionSetupActivity.debugMode)
			Log.d(DEBUGTAG, "onNewVideoSession:" + productType);
		QTReceiver.startVideoSessionActivity(this, callID, callType, remoteID,
				audioDestIP, audioLocalPort, audioDestPort, audio, videoDestIP,
				videoLocalPort, videoDestPort, video, productType, startTime,
				width, height, fps, kbps, enablePLI, enableFIR, enableTelEvt,
				enableCameraMute);
	}

	@Override
	public void onNewAudioSession(final String callID, final int callType,
			final String remoteID, final String remoteIP,
			final int audioLocalPort, final int audioDestPort, final MDP audio,
			final long startTime, final boolean enableTelEvt, final int productType) {
		if (ProvisionSetupActivity.debugMode)
			Log.d(DEBUGTAG, "onNewAudioSession");
		QTReceiver.startAudioSessionActivity(this, callID, callType, remoteID,
				remoteIP, audioLocalPort, audioDestPort, audio, startTime,
				enableTelEvt, productType);
	}

	// @Override
	// public void onRemoteHangup(final String callID,final String msg) {
	// if (QTReceiver.ENABLE_DEBUG) Log.d(TAG, "onRemoteHangup:"+callID +
	// ", msg:"+msg);
	// QTReceiver.onCallDestroy(callID,msg);
	// }
	@Override
	public void onLogCall(final String remoteID, final String remoteDisplayName,
			final int callType, final long startTime, final boolean isVideoCall) {
		if (ProvisionSetupActivity.debugMode)
			Log.d(DEBUGTAG, "onLogCall:remoteID:" + remoteID +" remoteDisplayName:"+remoteDisplayName+
					" callType:" + callType + " startTime:" + startTime + " isVideoCall:"
					+ isVideoCall);
		long nowTime = System.currentTimeMillis();
		Log.d(DEBUGTAG, " startTime:" + startTime + " Now :" +System.currentTimeMillis());
		ContactQT contact = ContactManager
				.getContactByQtAccount(this, remoteID);

    	//----------------------------------------------------[PT]
    	String missName = "unknown";
		String missAlias = "unknown";
    	String missRole = "unknown";
    	String missUid = "unknown";
    	String missTitle = "unknown";
    	String remoteCallID = null;
    	if(remoteID.startsWith(QtalkSettings.MCUPrefix) && remoteDisplayName!=null){
    		remoteCallID = remoteDisplayName;
    	}else{
    		remoteCallID = remoteID;
    	}
		PtmsData data = null;
		if(Hack.isPhone()) {
			data = PtmsData.findData(remoteCallID);
		}
		if(data==null) {
			ArrayList<QTContact2> temp_list;
			String phonebook_url = Hack.getStoragePath() + "phonebook.xml";
			try {
				temp_list = QtalkDB.parseProvisionPhonebook(phonebook_url);
				for (int i = 0; i < temp_list.size(); i++) {
					if (temp_list.get(i).getPhoneNumber().contentEquals(remoteCallID)) {
						missRole = temp_list.get(i).getRole();
						missName = temp_list.get(i).getName();
						missAlias = temp_list.get(i).getAlias();
						missUid = temp_list.get(i).getUid();
						missTitle = temp_list.get(i).getTitle();
					}
				}
			} catch (ParserConfigurationException e) {
				Log.e(DEBUGTAG, "parseProvisionPhonebook", e);
			} catch (SAXException e) {
				Log.e(DEBUGTAG, "parseProvisionPhonebook", e);
			} catch (IOException e) {
				Log.e(DEBUGTAG, "parseProvisionPhonebook", e);
				Message msg1 = new Message();
				msg1.what = 0;
				Bundle bundle = new Bundle();
				bundle.putString(ProvisionSetupActivity.MESSAGE, getString(R.string.loading_phonebook_fail) + "\r\n" + e.toString());
				msg1.setData(bundle);
				Handler mHandler = new Handler();
				mHandler.sendMessage(msg1);
			}
		} else {
			missRole = "patient";
			missName = data.name;
			missAlias = data.alias;
			missUid = data.bedid;
			missTitle = data.title;
			Log.d(DEBUGTAG, "onLogCall:missRole:" + missRole +" missName:"+missName+
					" missAlias:" + missAlias + " missUid:" + missUid + " missTitle:"
					+ missTitle);
		}
    	//----------------------------------------------------[add role and name to contact]

    	//----------------------------------------------------[PT]
    	if(callType==3){
    		SendNotification(remoteID, missName, missTitle, missRole, missUid);
    	}

    	//----------------------------------------------------
    	if(remoteID.startsWith(QtalkSettings.MCUPrefix)){
    		if(remoteDisplayName==null){
    			missName = getString(R.string.all_nurse);
    			missTitle = "";
    		}
    		//missName = remoteDisplayName;
    		missRole = "mcu";
    	}
    	if(missName.contentEquals("unknown")){
    		missName = remoteID;
    		missTitle = "";
    		missRole = "cmuh";
    	}
    	String duringTime = computeDuringTime(startTime,nowTime,callType,missRole);
		HistoryManager.addCallLog(this, contact, remoteID, missName, missAlias, missTitle, missRole, missUid, callType, startTime, nowTime, duringTime, isVideoCall);
	}

 	private String computeDuringTime(long startTime, long nowTime, int callType, String Role) {

 		long second = (nowTime/1000)-(startTime/1000);
 		long hour = 0, min = 0, sec =0;
 		hour = second/3600;
 		min = (second%3600)/60;
 		sec = ((second%3600)%60);
 		String duringTime ="";
 		if(startTime ==0){
 			if(callType==2){
 	            if(Role.contentEquals("tts")){
 	            	duringTime = "";
 	            }else{
 					duringTime = getString(R.string.cancel_call);
 	            }
 				return duringTime;
 			}else{
 				return duringTime;
 			}
 		}
 		if(getString(R.string.language).contentEquals("中文"))
 		{
	 		if(hour!=0){
	 			duringTime = String.valueOf(hour)+"時";
	 		}
	 		if(hour!=0 && min ==0){
	 			duringTime = duringTime +"0分";
	 		}
	 		if(min !=0){
	 			duringTime = duringTime+String.valueOf(min)+"分";
	 		}
	 		duringTime = duringTime +String.valueOf(sec) +"秒";
	 		return duringTime;
 		}else{
 			duringTime = "Duration: ";
 			if(hour!=0){
	 			duringTime = String.valueOf(hour)+":";
	 		}
	 		if(hour!=0 && min ==0){
	 			duringTime = duringTime +"00:";
	 		}else if(hour==0 && min ==0){
	 			duringTime = duringTime +"0:";
	 		}
	 		if(min !=0){
	 			duringTime = duringTime+String.valueOf(min)+":";
	 		}
	 		if(sec>=10){
	 			duringTime = duringTime +String.valueOf(sec) +"";
	 		}else{
	 			duringTime = duringTime +"0"+String.valueOf(sec) +"";
	 		}
	 		return duringTime;
 		}

 	}
	private void SendNotification(String remoteID, String missName,String missTitle, String missRole, String missUid) {
		Date date = new Date();
		DateFormat fullFormat = new SimpleDateFormat("MM/dd/yyyy HH:mm");
		if (remoteID.startsWith(QtalkSettings.MCUPrefix)) {
			Intent broadcast_intent = new Intent();
			broadcast_intent.setAction("android.intent.action.vc.notification");
			Bundle bundle = new Bundle();
			bundle.putInt("TYPE", 2);
			bundle.putString("TIME", fullFormat.format(date));
			bundle.putString("NAME", missName);
			broadcast_intent.putExtras(bundle);
			Log.d(DEBUGTAG, "Send Broadcast to vc broadcast notification ");
			sendBroadcast(broadcast_intent);
		} else {

			Intent broadcast_intent = new Intent();
			broadcast_intent.setAction("android.intent.action.vc.notification");
			Bundle bundle = new Bundle();
			bundle.putInt("TYPE", 1);
			String reMissName = notificationName(missName, missRole, missTitle);
			bundle.putString("NAME", reMissName);
			bundle.putString("TIME", fullFormat.format(date));
			bundle.putString("ROLE", missRole);
			bundle.putString("UID", missUid);
			broadcast_intent.putExtras(bundle);
			Log.d(DEBUGTAG, "Send Broadcast to vc misscall notification ");
			sendBroadcast(broadcast_intent);
			if (ProvisionSetupActivity.debugMode)
					Log.d(DEBUGTAG, "SEND notification"
						+ remoteID + " missName: "
						+ missName + " missTime: " + fullFormat.format(date)
						+ " missRole: " + missRole + " missUid: " + missUid);

		}
	}
	public String notificationName(String name, String role, String title){
		String roleAndName = null;
		if(title!=null){
			if(!title.contentEquals("")){
				if(title.contentEquals("head_nurse")){
        			roleAndName = getString(R.string.head_nurse)+"::"+name;
        		}else if(title.contentEquals("clerk")) {
        			roleAndName = getString(R.string.secreatary)+"::"+name;
        		}else{
        			roleAndName = title+"::"+name;
        		}
			}else{
				if(role.contentEquals("nurse")){
					roleAndName = getString(R.string.nurse)+"::" +name;
				}else if(role.contentEquals("station")){
					roleAndName = getString(R.string.nurse_station)+"::" +name;
				}else if(role.contentEquals("staff")){
					roleAndName = getString(R.string.cleaner)+"::" +name;
				}else{
					roleAndName = "Unknown::" +name;
				}
			}
		}else{
			roleAndName = name;
		}
		return roleAndName;

	}

	@Override
	public void onExit() {
		if (ProvisionSetupActivity.debugMode)
			Log.d(DEBUGTAG, "onExit");
		QTReceiver.alarm(0, QTRAAlarm.class);
		if (mEngine != null) {
			mEngine.logout("");
			mEngine = null;
		}
		if (mReceiver != null) {
			unregisterReceiver(mReceiver);
			mReceiver = null;
		}
		this.stopSelf();
	}

	@Override
	public void onSessionEnd(String callID) {
		if (ProvisionSetupActivity.debugMode)
			Log.d(DEBUGTAG, "onSessionEnd:" + callID);
		QTReceiver.onSessionEnd(callID); // Remove the previous in-session
											// window
	}

	@Override
	public void onMCUSessionEnd(String callID) {
		if (ProvisionSetupActivity.debugMode)
			Log.d(DEBUGTAG, "onSessionEnd:" + callID);
		QTReceiver.onMCUSessionEnd(callID); // Remove the previous in-session
											// window
	}

	public void phonebookcheck(){
		//Log.d(DEBUGTAG, "PhonebookCheck(), start");

		int size1=0;
		QThProvisionUtility qtnMessenger = new QThProvisionUtility(this, mHandler, null);
		QtalkDB qtalkdb = QtalkDB.getInstance(QTService.this);
		
		if (session == null || key == null || lastPhonebookFileTime == null) {
			Cursor cs = qtalkdb.returnAllATEntry();
			size = cs.getCount();
//			Log.d(DEBUGTAG, "AT_DB size=" + size);
			cs.moveToFirst();
			for (int cscnt = 0; cscnt < cs.getCount(); cscnt++) {
				session = QtalkDB.getString(cs, "Session");
				key = QtalkDB.getString(cs, "Key");
				provisionID = QtalkDB.getString(cs, "Uid");
				provisionType = QtalkDB.getString(cs, "Type");
				provisionServer = QtalkDB.getString(cs, "Server");
				cs.moveToNext();
			}
			cs.close();
			cs = null;

			Cursor cs1 = qtalkdb.returnAllPTEntry();
			size1 = cs1.getCount();
//			Log.d(DEBUGTAG, "PT_DB size=" + size1);
			cs1.moveToFirst();
			for (int cscnt = 0; cscnt < cs1.getCount(); cscnt++) {
				lastPhonebookFileTime = QtalkDB.getString(cs1, "lastPhonebookFileTime");
				cs1.moveToNext();
			}
			cs1.close();
			cs1 = null;
		}

		if(size > 0)
		{
			if(QThProvisionUtility.provision_info !=null)
			{
				//Log.d(DEBUGTAG, "lastPhonebookFileTime=" + lastPhonebookFileTime);
				phonebook_uri = QThProvisionUtility.provision_info.phonebook_uri
						+ provisionType
						+ "&service=phonebook&action=check&uid="
						+ provisionID
						+ "&session="
						+ session
						+ "&timestamp="
						+ lastPhonebookFileTime;
				try {
					qtnMessenger.httpsConnect(phonebook_uri, session,
							lastPhonebookFileTime, provisionID, provisionType,
							provisionServer, key);
                    boolean isNotifyLogonStateNative = LogonStateReceiver.isNotifyLogonStateNative();
                    boolean SIPRunnablegetState = Flag.SIPRunnable.getState();
                    //Log.e(DEBUGTAG, "2 isNotifyLogonStateNative="+isNotifyLogonStateNative+", SIPRunnablegetState="+SIPRunnablegetState);
                    if(!isNotifyLogonStateNative && !SIPRunnablegetState)
                    {
                        Flag.SIPRunnable.setState(true);
                        LogonStateReceiver.notifyLogonState(QTService.this, true, "server is reachable", null, false);
                    }
				} catch(ConnectException connex){
					Log.e(DEBUGTAG, "ConnectException", connex);
					if(Flag.SIPRunnable.getState()) {
						Flag.SIPRunnable.setState(false);
						LogonStateReceiver.notifyLogonState(this, false, null, "server is unreachable", false);
					}
				} catch (Exception e) {
					Log.e(DEBUGTAG, "PhonebookrunnableThread", e);
				}
			}
			else {
				String provision_file = Hack.getStoragePath()+"provision.xml";
				try {
					QThProvisionUtility.provision_info = ProvisionUtility.parseProvisionConfig(provision_file);
				} catch (ParserConfigurationException e) {
					Log.e(DEBUGTAG,"",e);
				} catch (SAXException e) {
					Log.e(DEBUGTAG,"",e);
				} catch (IOException e) {
					Log.e(DEBUGTAG,"",e);
				}
			}
		}
		if(Flag.AvatarSuccess.getState()==false)
		{
			if (QThProvisionUtility.provision_info != null)
        	{
        		server = QThProvisionUtility.provision_info.phonebook_uri;
        		String UploadFile = null;

        		File file = new File(AvatarImagePickActivity.path);
        		if(file.exists())
        			UploadFile = AvatarImagePickActivity.path;
        		else
        			UploadFile = Hack.getStoragePath()+"all_icn_member.png";

        		try {
 		        	qtnMessenger.doFileUpload(UploadFile ,provisionID, provisionType, session, server);
 		        }
 		        catch (Exception e) {
 		        	Log.d(DEBUGTAG, e.getMessage());
 		        	Message msg = new Message();
 		        	msg.what = AVATAR_FAIL;
 		        }
        	}
		}
		qtalkdb.close();
		qtalkdb = null;

		//Log.d(DEBUGTAG, "PhonebookCheck(), exit");
	}

}
